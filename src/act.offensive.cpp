/***************************************************************************
 *   File: act.offensive.c                               Part of FieryMUD  *
 *  Usage: player-level commands of an offensive nature                    *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "casting.hpp"
#include "chars.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "defines.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* extern functions */
ACMD(do_return);
CharData *check_guard(CharData *ch, CharData *victim, int gag_output);
bool is_grouped(CharData *ch, CharData *tch);
void quickdeath(CharData *victim, CharData *ch);
void abort_casting(CharData *ch);
void appear(CharData *ch);
bool displaced(CharData *ch, CharData *victim);

void aggro_lose_spells(CharData *ch) {
    if (!EFF_FLAGGED(ch, EFF_REMOTE_AGGR)) {
        if (affected_by_spell(ch, SPELL_INVISIBLE) || affected_by_spell(ch, SPELL_NATURES_EMBRACE))
            appear(ch);
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_INVISIBLE);
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_CAMOUFLAGED);
        if (EFF_FLAGGED(ch, EFF_GLORY))
            effect_from_char(ch, SPELL_GLORY);
        GET_HIDDENNESS(ch) = 0;
    }
}

bool switch_ok(CharData *ch) {
    if (GET_SKILL(ch, SKILL_SWITCH) <= 0) {
        act("You are already busy fighting with $N.", false, ch, 0, FIGHTING(ch), TO_CHAR);
        return false;
    }

    if (random_number(1, 101) > GET_SKILL(ch, SKILL_SWITCH)) {
        act("$n tries to switch opponents, but becomes confused!&0", false, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You try to switch opponents and become confused.&0\n");
        stop_fighting(ch);
        improve_skill_offensively(ch, FIGHTING(ch), SKILL_SWITCH);
        return false;
    }

    stop_fighting(ch);
    act("$n switches opponents!&0", false, ch, 0, 0, TO_ROOM);
    char_printf(ch, "You switch opponents!&0\n");
    improve_skill_offensively(ch, FIGHTING(ch), SKILL_SWITCH);
    return true;
}

const struct breath_type {
    const char *name;
    const int spell;
    const int skill;
    const char *to_char;
    const char *to_room;
} breath_info[] = {
    {"fire", SPELL_FIRE_BREATH, SKILL_BREATHE_FIRE, "&1You snort and &bf&3i&7r&1e&0&1 shoots out of your nostrils!&0\n",
     "&1$n&1 snorts and a gout of &bf&3i&7r&1e&0&1 shoots out of $s nostrils!&0"},
    {"gas", SPELL_GAS_BREATH, SKILL_BREATHE_GAS,
     "&2You heave and a &bnoxious gas&0&2 rolls rapidly out of your nostrils!&0\n",
     "&2$n&2 rumbles and a &bnoxious gas&0&2 rolls out of $s nostrils!&0"},
    {"frost", SPELL_FROST_BREATH, SKILL_BREATHE_FROST,
     "&7&bYou shiver as a shaft of &0&4f&br&7o&4s&0&4t&7&b leaps from your mouth!&0\n",
     "&7&b$n&7&b shivers as a shaft of &0&4f&br&7o&4s&0&4t&7&b leaps from $s mouth!&0"},
    {"acid", SPELL_ACID_BREATH, SKILL_BREATHE_ACID,
     "&9&bYour stomach heaves as a wash of &2&ba&0&2ci&bd&9 leaps from your mouth!&0\n",
     "&9&b$n&9&b looks pained as a wash of &2&ba&0&2ci&2&bd&9 leaps from $s mouth!&0"},
    {"lightning", SPELL_LIGHTNING_BREATH, SKILL_BREATHE_LIGHTNING,
     "&4You open your mouth and bolts of &blightning&0&4 shoot out!&0\n",
     "&4$n&4 opens $s mouth and bolts of &blightning&0&4 shoot out!&0"},
    {nullptr, 0, 0, nullptr, nullptr},
};

CharData *random_attack_target(CharData *ch, CharData *target, bool verbose) {
    CharData *chosen = nullptr;
    CharData *i;
    int count = 0;

    for (i = world[ch->in_room].people; i; i = i->next_in_room) {
        if (i != ch && CAN_SEE(ch, i) && attack_ok(ch, i, false)) {
            if (chosen == nullptr || random_number(0, count) == 0)
                chosen = i;
            count++;
        }
    }

    if (!chosen)
        chosen = target;

    if (verbose && chosen != target) {
        act("&5You got confused and attacked $N&0&5 instead!&0", false, ch, 0, chosen, TO_CHAR);
        act("&5$n&0&5 got confused and attacked $N&0&5!&0", true, ch, 0, chosen, TO_NOTVICT);
        act("&5$n&0&5 gets confused and attacks YOU!&0", true, ch, 0, chosen, TO_VICT);
    }

    return chosen;
}

ACMD(do_breathe) {
    ACMD(do_action);
    CharData *tch, *next_tch;
    int type;
    bool realvictims = false;
    int breath_energy;

    if (!ch || ch->in_room == NOWHERE)
        return;

    /* Don't allow reanimated undead to do this - justification:
     * you need actual life to generate poison gas! */
    /*
    if (GET_SKILL(ch, SKILL_BREATHE) < 1 || EFF_FLAGGED(ch, EFF_CHARM)) {
        char_printf(ch, "You huff and puff but to no avail.\n");
        act("$n huffs and puffs but to no avail.", false, ch, 0, 0, TO_ROOM);
        return;
    }
    */
    if (EFF_FLAGGED(ch, EFF_CHARM) ||
        (GET_SKILL(ch, SKILL_BREATHE_GAS) < 1 && GET_SKILL(ch, SKILL_BREATHE_FIRE) < 1 &&
         GET_SKILL(ch, SKILL_BREATHE_FROST) < 1 && GET_SKILL(ch, SKILL_BREATHE_ACID) < 1 &&
         GET_SKILL(ch, SKILL_BREATHE_LIGHTNING) < 1)) {
        char_printf(ch, "You huff and puff but to no avail.\n");
        act("$n huffs and puffs but to no avail.", false, ch, 0, 0, TO_ROOM);
        return;
    }
    one_argument(argument, arg);

    for (type = 0; breath_info[type].name; ++type)
        if (is_abbrev(arg, breath_info[type].name))
            break;

    if (!breath_info[type].name) {
        char_printf(ch, "Usage: breathe <fire / gas / frost / acid / lightning>\n");
        return;
    }

    if (!GET_SKILL(ch, breath_info[type].skill)) {
        char_printf(ch, "You cannot breathe that kind of energy!\n");
        return;
    }

    if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        if (!GET_COOLDOWN(ch, CD_BREATHE)) {
            if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) {
                SET_COOLDOWN(ch, CD_BREATHE, 3 MUD_HR);
            }
        } else {
            int seconds = GET_COOLDOWN(ch, CD_BREATHE) / 10;
            char_printf(ch, "You will have rebuilt your energy in {:d} {}.\n", seconds,
                        seconds == 1 ? "second" : "seconds");
            return;
        }
    }

    char_printf(ch, breath_info[type].to_char);
    act(breath_info[type].to_room, false, ch, 0, 0, TO_ROOM);

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        if (tch == ch)
            continue;
        if (is_grouped(ch, tch))
            continue;
        if (!mass_attack_ok(ch, tch, false))
            continue;
        if (PRF_FLAGGED(tch, PRF_NOHASSLE))
            continue;
        /* Mobs don't hit other mobs, unless they're pets */
        if (!IS_PC(ch) && !IS_PC(tch) && !PLAYERALLY(ch) && !PLAYERALLY(tch))
            continue;
        if (IS_NPC(ch))
            call_magic(ch, tch, 0, breath_info[type].spell, GET_LEVEL(ch), CAST_BREATH);
        else
            call_magic(ch, tch, 0, breath_info[type].spell, GET_SKILL(ch, breath_info[type].skill), CAST_BREATH);
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realvictims = true;
    }
    /*
    if (realvictims)
        improve_skill(ch, SKILL_BREATHE);

    */
    if (realvictims) {
        improve_skill(ch, breath_info[type].skill);
    }

    if (GET_LEVEL(ch) < LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_roar) {
    CharData *tch, *next_tch;
    bool realvictims = false;
    ACMD(do_flee);
    ACMD(do_action);

    if (!ch || ch->in_room == NOWHERE)
        return;

    if (subcmd == SCMD_HOWL) {
        if (!GET_SKILL(ch, SKILL_BATTLE_HOWL) || !EFF_FLAGGED(ch, EFF_SPIRIT_WOLF) || !EFF_FLAGGED(ch, EFF_BERSERK)) {
            if (SUN(IN_ROOM(ch)) == SUN_DARK && CH_OUTSIDE(ch)) {
                char_printf(ch, "You form an O with your mouth and howl at the moon.\n");
                act("$n starts howling at the moon.   Eerie.", false, ch, 0, 0, TO_ROOM);
            } else {
                char_printf(ch, "You howl madly, making a fool of yourself.\n");
                act("$n howls madly, looking like a fool.", false, ch, 0, 0, TO_ROOM);
            }
            return;
        }
    } else if (!GET_SKILL(ch, SKILL_ROAR) || EFF_FLAGGED(ch, EFF_CHARM)) {
        do_action(ch, argument, cmd, subcmd);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        act("$n opens $s mouth wide and lets out a little cough.", false, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You take a deep breath and release a vicious cough!\n");
        return;
    }

    if (subcmd == SCMD_HOWL) {
        act("$n opens his mouth and a &1demonic &0&1howl&0 echoes out!", false, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You let out a demonic battle howl, striking fear into your enemies!\n");
    } else {
        act("&9&b$n&9&b makes your soul quake with a vicious "
            "&1ROOOOOAAAAAARRRRRR!&0",
            false, ch, 0, 0, TO_ROOM);
        char_printf(ch, "&9&bYou take a deep breath and release a vicious &1ROOOOOAAAAARRRRRR!&0\n");
    }

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        if (tch == ch)
            continue;
        if (GET_STANCE(tch) < STANCE_SLEEPING)
            continue;
        /* Mobs don't fear other mobs, unless they're pets. */
        if (!IS_PC(ch) && !IS_PC(tch) && !PLAYERALLY(ch) && !PLAYERALLY(tch))
            continue;
        if (is_grouped(ch, tch))
            continue;
        if (!attack_ok(ch, tch, false))
            continue;
        if (mag_savingthrow(tch, SAVING_PARA))
            continue;
        if (PRF_FLAGGED(tch, PRF_NOHASSLE))
            continue;
        /* Twice as hard to roar at a sentinel mob. */
        if (MOB_FLAGGED(tch, MOB_SENTINEL) && mag_savingthrow(tch, SAVING_PARA))
            continue;
        if (MOB_FLAGGED(tch, MOB_AWARE) || MOB_FLAGGED(tch, MOB_NOSUMMON))
            continue;
        if (EFF_FLAGGED(tch, EFF_PROTECT_EVIL) && GET_ALIGNMENT(ch) <= -500) {
            act("Your holy protection strengthens your resolve against $N\'s roar!\n", false, tch, 0, ch, TO_CHAR);
            continue;
        }
        if (EFF_FLAGGED(tch, EFF_PROTECT_GOOD) && GET_ALIGNMENT(ch) <= 500) {
            act("Your unholy protection strengthens your resolve against $N\'s roar!\n", false, tch, 0, ch, TO_CHAR);
            continue;
        }
        mag_affect(GET_LEVEL(ch), ch, tch, SPELL_FEAR, SAVING_PARA, CAST_BREATH);

        if (SLEEPING(tch)) {
            if (random_number(0, 1)) {
                sprintf(buf, "A loud %s jolts you from your slumber!\n",
                        subcmd == SCMD_HOWL ? "OOOOAAAOAOOHHH howl" : "ROAAARRRRRR");
                char_printf(tch, buf);
                act("$n jumps up dazedly, awakened by the noise!", true, tch, 0, 0, TO_ROOM);
                GET_POS(tch) = POS_SITTING;
                GET_STANCE(tch) = STANCE_ALERT;
                WAIT_STATE(tch, PULSE_VIOLENCE);
            }
        } else if (GET_DEX(tch) - 15 < random_number(0, 100) && GET_POS(tch) >= POS_STANDING) {
            char_printf(tch, "In your panicked rush to flee, you trip!\n");
            act("In a panicked rush to flee, $n trips!", false, tch, 0, 0, TO_ROOM);
            GET_POS(tch) = POS_SITTING;
            GET_STANCE(tch) = STANCE_ALERT;
            WAIT_STATE(tch, PULSE_VIOLENCE);
        } else
            do_flee(tch, nullptr, 0, 0);
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realvictims = true;
    }

    if (realvictims) {
        if (subcmd == SCMD_HOWL)
            improve_skill(ch, SKILL_BATTLE_HOWL);
        else
            improve_skill(ch, SKILL_ROAR);
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_sweep) {
    CharData *tch, *next_tch;
    bool realvictims = false;
    ACMD(do_flee);

    if (!ch || ch->in_room == NOWHERE)
        return;

    if (GET_SKILL(ch, SKILL_SWEEP) < 1 || EFF_FLAGGED(ch, EFF_CHARM)) {
        char_printf(ch, "Huh?!?\n");
        return;
    }

    act("&2$n&2 sweeps with $s enormous tail!&0", false, ch, 0, 0, TO_ROOM);
    char_printf(ch, "&2You sweep with your enormous tail!&0\n");

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        if (tch == ch)
            continue;
        /* Mobs don't sweep other mobs, unless they're pets. */
        if (!IS_PC(ch) && !IS_PC(tch) && !PLAYERALLY(ch) && !PLAYERALLY(tch))
            continue;
        if (is_grouped(ch, tch))
            continue;
        if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_IMMORT)
            continue;
        if (GET_VIEWED_DEX(tch) - 15 > random_number(0, 100) || GET_STANCE(tch) < STANCE_FIGHTING)
            continue;
        if (!attack_ok(ch, tch, false))
            continue;
        if (PRF_FLAGGED(tch, PRF_NOHASSLE))
            continue;
        if (damage_evasion(tch, ch, 0, DAM_CRUSH)) {
            act(EVASIONCLR "$N's" EVASIONCLR " tail passes right through you.&0", false, tch, 0, ch, TO_CHAR);
            act(EVASIONCLR "$N's" EVASIONCLR " tail passes harmlessly through $n.&0", false, tch, 0, ch, TO_ROOM);
            set_fighting(tch, ch, true);
            continue;
        }

        act("&3You are slammed to the ground by $N's&3 tail!&0", false, tch, 0, ch, TO_CHAR);
        act("&3$n&3 is slammed to the ground by a mighty tail sweep!&0", false, tch, 0, 0, TO_ROOM);
        GET_POS(tch) = POS_SITTING;
        GET_STANCE(tch) = STANCE_ALERT;
        WAIT_STATE(tch, PULSE_VIOLENCE);
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realvictims = true;
    }

    if (realvictims)
        improve_skill(ch, SKILL_SWEEP);
    if (GET_LEVEL(ch) < LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_assist) {
    CharData *helpee, *opponent;

    one_argument(argument, arg);

    if (FIGHTING(ch))
        char_printf(ch, "You're already fighting!\n");
    else if (!*arg)
        char_printf(ch, "Whom do you wish to assist?\n");
    else if (!(helpee = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
        char_printf(ch, NOPERSON);
    else if (helpee == ch)
        char_printf(ch, "Usually, you assist someone else.\n");
    else {
        for (opponent = world[ch->in_room].people; opponent && (FIGHTING(opponent) != helpee);
             opponent = opponent->next_in_room)
            ;

        if (!opponent)
            act("But nobody is fighting $M!", false, ch, 0, helpee, TO_CHAR);
        else if (!CAN_SEE(ch, opponent))
            act("You can't see who is fighting $M!", false, ch, 0, helpee, TO_CHAR);
        else if (attack_ok(ch, opponent, true)) {
            act("You assist $N&0 heroically.", false, ch, 0, helpee, TO_CHAR);
            act("$n&0 assists you!", 0, ch, 0, helpee, TO_VICT);
            act("$n&0 heroically assists $N.", false, ch, 0, helpee, TO_NOTVICT);
            if (CONFUSED(ch))
                opponent = random_attack_target(ch, opponent, true);
            attack(ch, opponent);
        }
    }
}

ACMD(do_disengage) {
    ACMD(do_abort);
    ACMD(do_hide);

    if (CASTING(ch)) {
        do_abort(ch, argument, 0, 0);
        return;
    }

    if (!FIGHTING(ch)) {
        char_printf(ch, "You are not fighting anyone.\n");
        return;
    }

    if (FIGHTING(FIGHTING(ch)) == ch) {
        char_printf(ch, "No way! You are fighting for your life!\n");
        return;
    }

    stop_fighting(ch);
    char_printf(ch, "You disengage from combat.\n");
    if (GET_CLASS(ch) == CLASS_ROGUE || GET_CLASS(ch) == CLASS_THIEF) {
        do_hide(ch, 0, 0, 0);
    } else {
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
}

ACMD(do_hit) {
    CharData *vict;

    one_argument(argument, arg);

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS))
        char_printf(ch, "It is just too dark!&0\n");
    else if (EFF_FLAGGED(ch, EFF_BLIND))
        char_printf(ch, "You can't see a thing!\n");
    else if (!*arg)
        char_printf(ch, "Hit who?\n");
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
        char_printf(ch, "They don't seem to be here.\n");
    else if (vict == ch) {
        char_printf(ch, "You hit yourself...OUCH!.\n");
        act("$n hits $mself, and says OUCH!", false, ch, 0, vict, TO_ROOM);
    } else if (EFF_FLAGGED(ch, EFF_CHARM) && (ch->master == vict))
        act("$N is just such a good friend, you simply can't hit $M.", false, ch, 0, vict, TO_CHAR);
    else if (attack_ok(ch, vict, true)) {
        if (vict == FIGHTING(ch)) {
            char_printf(ch, "&7You're doing the best you can!&0\n");
            return;
        } else if (!FIGHTING(ch) || switch_ok(ch)) {
            if (CONFUSED(ch))
                vict = random_attack_target(ch, vict, true);
            attack(ch, vict);
        }
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
}

ACMD(do_kill) {
    CharData *vict;

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS)) {
        char_printf(ch, "It is just too damn dark!&0\n");
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "You feel ashamed trying to disturb the peace of this room.\n");
        return;
    }

    if ((GET_LEVEL(ch) < LVL_GOD) || IS_NPC(ch)) {
        do_hit(ch, argument, cmd, subcmd);
        return;
    }
    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Kill who?\n");
    } else {
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
            char_printf(ch, "They aren't here.\n");
        else if (ch == vict)
            char_printf(ch, "Your mother would be so sad.. :(\n");
        else if (GET_LEVEL(vict) == LVL_IMPL)
            char_printf(ch, "&1You dare NOT do that!&0\n");
        else {
            act("You chop $M to pieces!   Ah!   The blood!", false, ch, 0, vict, TO_CHAR);
            act("$N chops you to pieces!", false, vict, 0, ch, TO_CHAR);
            act("$n brutally slays $N!", false, ch, 0, vict, TO_NOTVICT);
            die(vict, ch);
        }
    }
}

/* I've made this a higher end functionality of backstab instead of some 'tarded
 * skill that rarely works - RLS 02/12/05*/
bool instantkill(CharData *ch, CharData *victim) {
    int chance = 0;

    if (!victim || !ch || ch == victim || ch == nullptr || victim == nullptr)
        return false;

    /* No more instant kills until the timer runs out */
    if (GET_COOLDOWN(ch, CD_INSTANT_KILL))
        return false;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_INSTANT_KILL) || DECEASED(victim) || (ch->in_room != victim->in_room) ||
        (GET_LEVEL(victim) > LVL_IMMORT))
        return false;

    improve_skill_offensively(ch, victim, SKILL_INSTANT_KILL);
    if (random_number(1, 101) > GET_SKILL(ch, SKILL_INSTANT_KILL))
        return false;

    /*chance is now checking DEX and shouldn't happen as often */
    chance = 1000 - (GET_SKILL(ch, SKILL_INSTANT_KILL) - (100 - GET_DEX(ch)) - (GET_LEVEL(victim) * 10)) / 10;

    /* der.. can you say coup de grace?
     * needed to add additional condition to prevent it from instant killing dying mobs and robbing parties of exp */
    if (!AWAKE(victim) && GET_HIT(victim) > 0)
        chance = 0;

    if (random_number(1, 1000) >= chance) {
        quickdeath(victim, ch);
        SET_COOLDOWN(ch, CD_INSTANT_KILL, (120 - (GET_LEVEL(ch))) * PULSE_COOLDOWN);
        return true;
    }
    return false;
}

void slow_death(CharData *victim) {
    if (!victim) {
        log(LogSeverity::Warn, LVL_GOD, "Attempting to use slow_death on a NULL character!");
        return;
    }

    if (victim->attackers) {
        /* Someone is fighting this person, so don't die just yet! */
        GET_HIT(victim) = HIT_MORTALLYW;
        hp_pos_check(victim, nullptr, 0);
        return;
    }

    act("With a soft groan, $n slips off into the cold sleep of death.&0", true, victim, 0, 0, TO_ROOM);
    act("$n is dead!   R.I.P.&0", true, victim, 0, 0, TO_ROOM);
    if (AWAKE(victim)) {
        act("You feel yourself slipping away and falling into the abyss.&0", false, victim, 0, 0, TO_CHAR);
        char_printf(victim, "&0Your life fades away ....\n");
    }

    die(victim, nullptr);
}

void quickdeath(CharData *victim, CharData *ch) {
    if (GET_LEVEL(victim) >= LVL_IMMORT || (ch && MOB_FLAGGED(ch, MOB_ILLUSORY)))
        return;

    char_printf(ch, "You deliver the killing blow.\n");
    act("$n's strike upon $N is faster than the eye can see.", true, ch, 0, victim, TO_NOTVICT);
    char_printf(victim, "You feel a sharp sting, and all goes black.\n");

    hurt_char(victim, ch, GET_MAX_HIT(victim) + 20, false);
}

ACMD(do_backstab) {
    CharData *vict, *tch;
    effect eff;
    int percent, prob, percent2, prob2, hidden;
    ObjData *weapon;

    if (GET_COOLDOWN(ch, CD_BACKSTAB)) {
        char_printf(ch, "Give yourself a chance to get back into position!\n");
        return;
    }

    if (GET_SKILL(ch, SKILL_BACKSTAB) < 1) {
        char_printf(ch, "You don't know how.\n");
        return;
    }

    one_argument(argument, buf);

    if (!buf || !*buf) {
        if (FIGHTING(ch)) {
            vict = FIGHTING(ch);
        } else {
            char_printf(ch, "Backstab who?\n");
            return;
        }

    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) {
        char_printf(ch, "Backstab who?\n");
        return;
    }

    if (vict == ch) {
        char_printf(ch, "How can you sneak up on yourself?\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BLIND)) {
        char_printf(ch, "You can't see a thing!\n");
        return;
    }

    /* Got a weapon? Any weapon? */
    weapon = GET_EQ(ch, WEAR_WIELD);
    if (!weapon)
        weapon = GET_EQ(ch, WEAR_WIELD2);
    if (!weapon)
        weapon = GET_EQ(ch, WEAR_2HWIELD);

    if (!weapon) {
        char_printf(ch, "Backstab with what, your fist?\n");
        return;
    }

    /* If wielding something unsuitable in first hand, use weapon in second hand
     */
    if (!IS_WEAPON_PIERCING(weapon) && GET_EQ(ch, WEAR_WIELD2))
        weapon = GET_EQ(ch, WEAR_WIELD2);

    if (!IS_WEAPON_PIERCING(weapon)) {
        char_printf(ch, "Piercing weapons must be used to backstab.\n");
        return;
    }

    if (!attack_ok(ch, vict, true))
        return;

    /* going to use this to pass to the damage code, but still want hiddenness removed */
    if (GET_HIDDENNESS(ch) > 0 && GET_CLASS(ch) == CLASS_ROGUE) {
        hidden = GET_HIDDENNESS(ch);
        GET_HIDDENNESS(ch) = 0;
    } else {
        GET_HIDDENNESS(ch) = 0;
        hidden = 0;
    }

    /* You can backstab as long as you're not the tank */
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (FIGHTING(tch) == ch) {
            if (FIGHTING(ch) == tch)
                act("$N's facing the wrong way!\n", false, ch, 0, tch, TO_CHAR);
            else if (FIGHTING(ch))
                act("You're too busy fighting $N to backstab anyone!", false, ch, 0, FIGHTING(ch), TO_CHAR);
            else
                act("$N is coming in for the attack - you cannot backstab $M now.", false, ch, 0, tch, TO_CHAR);
            return;
        }
    }

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    if (damage_evasion(vict, ch, weapon, DAM_PIERCE)) {
        damage_evasion_message(ch, vict, weapon, DAM_PIERCE);
        set_fighting(vict, ch, true);
        return;
    }

    /* If the mob is flagged aware, is not sleeping, and is not currently
       fighting, then you can not backstab the mob */
    if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict) && !FIGHTING(vict) && !EFF_FLAGGED(vict, EFF_MINOR_PARALYSIS) &&
        !EFF_FLAGGED(vict, EFF_MAJOR_PARALYSIS)) {
        act("You notice $N sneaking up on you!", false, vict, 0, ch, TO_CHAR);
        act("$e notices you sneaking up on $m!", false, vict, 0, ch, TO_VICT);
        act("$n notices $N sneaking up on $m!", false, vict, 0, ch, TO_NOTVICT);
        attack(vict, ch);
        return;
    }

    /* 50% chance the mob is aware, even in combat */
    if (MOB_FLAGGED(vict, MOB_AWARE) && CAN_SEE(ch, vict) && !EFF_FLAGGED(vict, EFF_MINOR_PARALYSIS) &&
        !EFF_FLAGGED(vict, EFF_MAJOR_PARALYSIS) && FIGHTING(vict) &&
        random_number(1, GET_LEVEL(vict)) > GET_SKILL(ch, SKILL_BACKSTAB) / 2) {
        act("You notice $N trying to sneaking up on you!", false, vict, 0, ch, TO_CHAR);
        act("You failed - $e notices you sneaking up on $m!", false, vict, 0, ch, TO_VICT);
        act("$n notices $N trying to sneak up on $m!", false, vict, 0, ch, TO_NOTVICT);

        attack(vict, ch);

        WAIT_STATE(ch, PULSE_VIOLENCE);
        /* 6 seconds == 1.5 combat rounds, 12 seconds == 3 combat rounds. */
        if (GET_CLASS(ch) == CLASS_ILLUSIONIST) {
            SET_COOLDOWN(ch, CD_BACKSTAB, 12 * PULSE_COOLDOWN);
        } else {
            SET_COOLDOWN(ch, CD_BACKSTAB, 6 * PULSE_COOLDOWN);
        }
        if (!EFF_FLAGGED(vict, EFF_AWARE)) {
            memset(&eff, 0, sizeof(eff));
            eff.type = SKILL_AWARE;
            eff.duration = 2;
            eff.modifier = 0;
            eff.location = APPLY_NONE;
            SET_FLAG(eff.flags, EFF_AWARE);
            effect_to_char(vict, &eff);
        }
        return;
    }

    percent = random_number(1, 101); /* 101% is a complete failure */

    if (EFF_FLAGGED(vict, EFF_AWARE) && AWAKE(vict) && !FIGHTING(vict))
        percent = 150; /*silent failure */

    if (EFF_FLAGGED(vict, EFF_AWARE) && FIGHTING(vict))
        percent += random_number(1, 10); /* It's a little harder to backstab a mob you've already backstabed */

    prob = std::min(97, GET_SKILL(ch, SKILL_BACKSTAB) - GET_LEVEL(vict) + 90);

    if (!CAN_SEE(vict, ch))
        prob += GET_SKILL(ch, SKILL_BACKSTAB) / 2; /* add blindfighting skill */

    if (instantkill(ch, vict))
        return;

    if (AWAKE(vict) && (percent > prob)) {
        /* Backstab failed */
        act("$N tried to backstab you, but you moved away in time!", false, vict, 0, ch, TO_CHAR);
        act("You failed to backstab $n, but manage to take a swing at $m!", false, vict, 0, ch, TO_VICT);
        act("$N tried to backstab $n, but missed and managed to take a swing "
            "instead!",
            false, vict, 0, ch, TO_NOTVICT);
        hit(ch, vict, weapon == GET_EQ(ch, WEAR_WIELD2) ? SKILL_DUAL_WIELD : TYPE_UNDEFINED);
    } else {
        /* Backstab succeeded */
        GET_HIDDENNESS(ch) = hidden;
        hit(ch, vict, weapon == GET_EQ(ch, WEAR_WIELD2) ? SKILL_2BACK : SKILL_BACKSTAB);
    }

    if (hidden > 0)
        improve_skill_offensively(ch, vict, SKILL_SNEAK_ATTACK);

    improve_skill_offensively(ch, vict, SKILL_BACKSTAB);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    /* 6 seconds == 1.5 combat rounds, 12 seconds == 3 combat rounds. */
    if (GET_CLASS(ch) == CLASS_ILLUSIONIST) {
        SET_COOLDOWN(ch, CD_BACKSTAB, 12 * PULSE_COOLDOWN);
    } else if (GET_CLASS(ch) == CLASS_THIEF) {
        SET_COOLDOWN(ch, CD_BACKSTAB, 4 * PULSE_COOLDOWN);
    } else {
        SET_COOLDOWN(ch, CD_BACKSTAB, 6 * PULSE_COOLDOWN);
    }

    if (DECEASED(vict))
        return;

    /* Make the victim aware for a while */
    if (!EFF_FLAGGED(vict, EFF_AWARE)) {
        memset(&eff, 0, sizeof(eff));
        eff.type = SKILL_AWARE;
        eff.duration = 2;
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        SET_FLAG(eff.flags, EFF_AWARE);
        effect_to_char(vict, &eff);
    }

    /* Assassins may perform a dual backstab. */
    if (GET_CLASS(ch) != CLASS_ASSASSIN)
        return;

    /* But only if wielding two piercing weapons */
    if (!GET_EQ(ch, WEAR_WIELD2) || weapon == GET_EQ(ch, WEAR_WIELD2) || !IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD2)))
        return;

    percent2 = random_number(1, 101); /* 101% is a complete failure */

    if (EFF_FLAGGED(vict, EFF_AWARE) && AWAKE(vict) && !FIGHTING(vict)) {
        percent2 = 150; /*silent failure */
    }

    if (EFF_FLAGGED(vict, EFF_AWARE) && FIGHTING(vict))
        percent2 += random_number(1, 10); /* It's a little harder to backstab a mob you've already backstabed */

    prob2 = GET_SKILL(ch, SKILL_BACKSTAB);

    /* Bonus points if the backstabee can't see the backstaber */
    if (!CAN_SEE(vict, ch)) {
        prob2 += GET_SKILL(ch, SKILL_BACKSTAB) / 2;
    }

    if (AWAKE(vict) && (percent2 > prob2)) {
        /* Backstab failed */
        attack(ch, vict);
    } else {
        /* Backstab succeeded */
        hit(ch, vict, SKILL_2BACK);
    }
    improve_skill_offensively(ch, vict, SKILL_BACKSTAB);
}

ACMD(do_flee) {
    int i, attempt;

    if (!ch || ch->in_room == NOWHERE)
        return;

    /* Don't show this message for sleeping and below */
    if (GET_STANCE(ch) > STANCE_SLEEPING && (IS_NPC(ch) ? GET_MOB_WAIT(ch) : CHECK_WAIT(ch))) {
        char_printf(ch, "You cannot flee yet!\n");
        return;
    }

    if (FIGHTING(ch) && EFF_FLAGGED(ch, EFF_BERSERK)) {
        char_printf(ch, "You're too angry to leave this fight!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS)) {
        char_printf(ch, "You can't move!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_IMMOBILIZED) && affected_by_spell(ch, SPELL_WEB)) {
        char_printf(ch, "You're stuck in the webs!\n");
        return;
    }

    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        char_printf(ch, "It's a bit too late for that.\n");
        break;
    case STANCE_SLEEPING:
        char_printf(ch, "You dream of fleeing!\n");
        break;
    default:
        switch (GET_POS(ch)) {
        case POS_PRONE:
        case POS_SITTING:
        case POS_KNEELING:
            abort_casting(ch);
            act("Looking panicked, $n scrambles madly to $s feet!", true, ch, 0, 0, TO_ROOM);
            char_printf(ch, "You scramble madly to your feet!\n");
            GET_POS(ch) = POS_STANDING;
            GET_STANCE(ch) = STANCE_ALERT;
            if (IS_NPC(ch))
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            break;
        default:
            if (IS_CORNERED(ch)) {
                act("$n tries to flee, but is unable to escape from $N!", true, ch, 0, ch->cornered_by, TO_NOTVICT);
                act("$n tries to flee, but is unable to escape from you!", true, ch, 0, ch->cornered_by, TO_VICT);
                act("PANIC!   You couldn't escape from $N!", true, ch, 0, ch->cornered_by, TO_CHAR);
                return;
            }
            for (i = 0; i < 6; i++) {                        /* Make 6 attempts */
                attempt = random_number(0, NUM_OF_DIRS - 1); /* Select a random direction */

                if (CAN_GO(ch, attempt) && !ROOM_FLAGGED(CH_NDEST(ch, attempt), ROOM_DEATH)) {
                    abort_casting(ch);
                    act("$n panics, and attempts to flee!", true, ch, 0, 0, TO_ROOM);
                    if (do_simple_move(ch, attempt, true)) {
                        char_printf(ch, "&0You panic and flee {}!&0\n", dirs[attempt]);
                    } else
                        act("$n tries to flee, but can't!", true, ch, 0, 0, TO_ROOM);
                    return;
                }
            }
            /* All 6 attempts failed! */
            act("$n tries to flee, but PANICS instead!", true, ch, 0, 0, TO_ROOM);
            char_printf(ch, "PANIC!   You couldn't escape!\n");
        }
    }
}

ACMD(do_retreat) {
    int dir, to_room;
    CharData *vict;

    if (!ch || !argument)
        return;

    if (!GET_SKILL(ch, SKILL_RETREAT)) {
        char_printf(ch, "Try flee instead!\n");
        return;
    }

    if (!FIGHTING(ch)) {
        char_printf(ch, "You're not fighting anyone!\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Retreat where!?\n");
        return;
    }

    dir = searchblock(arg, dirs, false);

    if (dir < 0 || !CH_DEST(ch, dir)) {
        char_printf(ch, "You can't retreat that way!\n");
        return;
    }

    /*
       for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
       if (FIGHTING(tch) == ch) {
       char_printf(ch, "You cannot retreat while tanking!&0\n");
       return;
       }
     */

    vict = FIGHTING(ch);

    /* Successful retreat? */
    if (GET_SKILL(ch, SKILL_RETREAT) > random_number(0, 81) && CAN_GO(ch, dir) &&
        !ROOM_FLAGGED(CH_NDEST(ch, dir), ROOM_DEATH) && do_simple_move(ch, dir, true)) {

        /* Send message back to original room. */
        sprintf(buf, "$n carefully retreats from combat, leaving %s.", dirs[dir]);
        to_room = ch->in_room;
        ch->in_room = vict->in_room;
        act(buf, true, ch, 0, 0, TO_ROOM);
        ch->in_room = to_room;

        char_printf(ch, "\nYou skillfully retreat {}.\n", dirs[dir]);
    }
    /* If fighting a mob that can switch, maybe get attacked. */
    else if (IS_NPC(FIGHTING(ch)) && FIGHTING(FIGHTING(ch)) != ch && GET_SKILL(FIGHTING(ch), SKILL_SWITCH) &&
             GET_SKILL(FIGHTING(ch), SKILL_SWITCH) > random_number(1, 101)) {
        stop_fighting(FIGHTING(ch));
        act("$n fails to retreat, catching $N's attention!", true, ch, 0, FIGHTING(ch), TO_NOTVICT);
        act("You notice $n trying to escape and attack $m!", false, ch, 0, FIGHTING(ch), TO_VICT);
        char_printf(ch, "You fail to retreat, and catch the attention of your opponent!\n");
        attack(FIGHTING(ch), ch);
    } else {
        act("$n stumbles and trips as $e fails to retreat!", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You stumble and trip as you try to retreat!\n");
        if (GET_LEVEL(ch) < LVL_GOD) {
            GET_POS(ch) = POS_SITTING;
            GET_STANCE(ch) = STANCE_ALERT;
        }
    }

    improve_skill_offensively(ch, vict, SKILL_RETREAT);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_gretreat) {
    int dir, opponents, was_in, to_room;
    CharData *tch;
    FollowType *k, *next_k;
    bool realopponents = false;

    if (!ch || !argument)
        return;

    if (!GET_SKILL(ch, SKILL_GROUP_RETREAT)) {
        char_printf(ch, "Try flee instead!\n");
        return;
    }

    if (!FIGHTING(ch)) {
        char_printf(ch, "You're not fighting anyone!\n");
        return;
    }

    argument = one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Retreat where!?\n");
        return;
    }

    dir = searchblock(arg, dirs, false);

    if (dir < 0 || !CH_DEST(ch, dir)) {
        char_printf(ch, "You can't retreat that way!\n");
        return;
    }

    was_in = IN_ROOM(ch);

    opponents = 0;
    for (tch = ch->attackers; tch; tch = tch->next_attacker) {
        ++opponents;
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realopponents = true;
    }

    /*
     * Can the followers see the leader before it leaves?
     */
    for (k = ch->followers; k; k = k->next)
        k->can_see_master = CAN_SEE(k->follower, ch);

    if (!opponents)
        char_printf(ch, "You must be tanking to successfully lead your group in retreat!\n");
    else if (GET_SKILL(ch, SKILL_GROUP_RETREAT) < opponents * random_number(20, 24))
        char_printf(ch, "There are too many opponents to retreat from!\n");

    /* Successful retreat? */
    else if (GET_SKILL(ch, SKILL_GROUP_RETREAT) > random_number(0, 81) &&
             !ROOM_FLAGGED(CH_NDEST(ch, dir), ROOM_DEATH) && CAN_GO(ch, dir) && do_simple_move(ch, dir, true)) {
        /* Echo line back to the original room. */
        sprintf(buf, "$n carefully retreats from combat, leading $s group %s.", dirs[dir]);

        to_room = ch->in_room;
        ch->in_room = was_in;
        act(buf, true, ch, 0, 0, TO_ROOM);
        ch->in_room = to_room;
        char_printf(ch, "\nYou skillfully lead your group {}.\n", dirs[dir]);

        for (k = ch->followers; k; k = next_k) {
            next_k = k->next;
            if (k->follower->in_room == was_in && GET_STANCE(k->follower) >= STANCE_ALERT && k->can_see_master) {
                abort_casting(k->follower);
                sprintf(buf, "You follow $N %s.", dirs[dir]);
                act(buf, false, k->follower, 0, ch, TO_CHAR);
                perform_move(k->follower, dir, 1, false);
            }
        }
    } else {
        act("$n stumbles and trips as $e fails to retreat!", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You stumble and trip as you try to retreat!\n");
        if (GET_LEVEL(ch) < LVL_GOD) {
            GET_POS(ch) = POS_SITTING;
            GET_STANCE(ch) = STANCE_ALERT;
        }
    }

    if (realopponents)
        improve_skill(ch, SKILL_GROUP_RETREAT);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_bash) {
    CharData *vict = nullptr;
    int percent, prob, skill, rounds, message = 0;

    switch (subcmd) {
    case SCMD_BODYSLAM:
        skill = SKILL_BODYSLAM;
        break;
    case SCMD_MAUL:
        skill = SKILL_MAUL;
        break;
    case SCMD_BASH:
    default:
        skill = SKILL_BASH;
        break;
    }

    if (!GET_SKILL(ch, skill)) {
        char_printf(ch, "You're not really sure how...\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS)) {
            char_printf(ch, "It's just too dark!&0\n");
            return;
        }

        if (EFF_FLAGGED(ch, EFF_BLIND)) {
            char_printf(ch, "You can't see a thing!\n");
            return;
        }

        if (skill == SKILL_BODYSLAM && FIGHTING(ch)) {
            char_printf(ch, "You can't bodyslam in combat...\n");
            return;
        }

        if (GET_COOLDOWN(ch, CD_BASH)) {
            char_printf(ch, "You haven't reoriented yourself for another {} yet!\n", skills[skill].name);
            return;
        }
    }

    one_argument(argument, arg);

    if (!*arg || !(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        vict = FIGHTING(ch);
        if (!vict || IN_ROOM(ch) != IN_ROOM(vict) || !CAN_SEE(ch, vict)) {
            char_printf(ch, "{} who?\n", capitalize(skills[skill].name));
            return;
        }
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (skill == SKILL_MAUL && (!EFF_FLAGGED(ch, EFF_BERSERK) || !EFF_FLAGGED(ch, EFF_SPIRIT_BEAR))) {
            act("You're not angry enough to tear $M limb from limb.\n", false, ch, 0, vict, TO_CHAR);
            return;
        }
    }

    if (vict == ch) {
        char_printf(ch, "Aren't we funny today...\n");
        return;
    }
    if (vict == ch->guarding) {
        act("You can't do that while you are guarding $M.", false, ch, 0, vict, TO_CHAR);
        return;
    }

    /* check for pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, true))
        return;
    vict = check_guard(ch, vict, false);
    if (!attack_ok(ch, vict, true))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    if (GET_POS(vict) <= POS_SITTING) {
        act("$E has already been knocked down.", false, ch, 0, vict, TO_CHAR);
        return;
    }

    prob = GET_SKILL(ch, skill);
    percent = random_number(1, 101); /* 101 is a complete failure */
    switch (skill) {
    case SKILL_BODYSLAM:
        prob = random_number(1, 100); /* bodyslam uses random num instead of skill */
        prob += GET_LEVEL(ch);
        prob += GET_HITROLL(ch) - monk_weight_penalty(ch);
        percent += GET_SKILL(vict, SKILL_DODGE);
        percent += GET_LEVEL(vict);
        rounds = 3;
        break;
    case SKILL_BASH:
        if (GET_EQ(ch, WEAR_2HWIELD)) /* 2H wield precludes a shield */
            prob /= 2;
        else if (!GET_EQ(ch, WEAR_SHIELD))
            prob /= 10; /* no shield or 2h weapon: 10% skill */
        rounds = 2;
        break;
    case SKILL_MAUL:
    default:
        rounds = 2;
        break;
    }

    if (GET_LEVEL(vict) >= LVL_IMMORT)
        percent = prob + 1; /* insta-fail */

    if (prob > percent || MOB_FLAGGED(vict, MOB_NOBASH)) {
        if (damage_evasion(vict, ch, 0, DAM_CRUSH) || MOB_FLAGGED(vict, MOB_ILLUSORY))
            message = 1;
        else if (displaced(ch, vict))
            message = 2;
    }

    if (message == 1 || message == 2) {
        if (message == 1) {
            act(EVASIONCLR "You charge right through $N&7&b!&0", false, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n" EVASIONCLR " charges right through $N" EVASIONCLR "!&0", false, ch, 0, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR " charges right through you!&0", false, ch, 0, vict, TO_VICT);
        }
        char_printf(ch, "You fall down!\n");
        act("$n falls down!", false, ch, 0, 0, TO_ROOM);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_ALERT;
        set_fighting(vict, ch, false);
        return;
    }

    /* NO BASH - you fail. */
    if (MOB_FLAGGED(vict, MOB_NOBASH)) {
        act("You &3&bslam&0 into $N, but $E seems quite unmoved.", false, ch, 0, vict, TO_CHAR);
        act("$n &3&bslams&0 into $N, who seems as solid as a rock!", false, ch, 0, vict, TO_NOTVICT);
        act("$n &3&bslams&0 into you, attempting to knock you down.", false, ch, 0, vict, TO_VICT);
        /* A pause... but you don't fall down. */
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        set_fighting(vict, ch, false);
        return;
    }

    if (GET_SIZE(vict) - GET_SIZE(ch) > 1) {
        char_printf(ch, "&7&bYou fall over as you try to {} someone so large!&0\n", skills[skill].name);
        act("&7&b$n BOUNCES off $N, as $e tries to $t $N's much larger size.&0", false, ch, skills[skill].name, vict,
            TO_NOTVICT);
        act("&7&b$n BOUNCES off you as $e tries to $t your much larger size.&0", false, ch, skills[skill].name, vict,
            TO_VICT);
        percent = prob + 1; /* insta-fail */
    } else if (GET_SIZE(ch) - GET_SIZE(vict) > 2) {
        char_printf(ch, "&7&bYou fall over as you try to {} someone with such small size.&0\n", skills[skill].name);
        act("&7&b$n trips over $N, as $e tries to $t $N's much smaller size.&0", false, ch, skills[skill].name, vict,
            TO_NOTVICT);
        act("&7&b$n trips over you as $e tries to $t your much smaller size.&0", false, ch, skills[skill].name, vict,
            TO_VICT);
        percent = prob + 1; /* insta-fail */
    }

    if (prob > percent) { /* Success! */
        WAIT_STATE(vict, PULSE_VIOLENCE * rounds);
        damage(ch, vict, dam_suscept_adjust(ch, vict, 0, GET_LEVEL(ch) >> 1, DAM_CRUSH), skill);
        if (GET_POS(vict) > POS_SITTING)
            alter_pos(vict, POS_SITTING, STANCE_ALERT);
    } else {
        if (skill == SKILL_BASH && !GET_EQ(ch, WEAR_SHIELD))
            char_printf(ch, "You need to wear a shield to make it a success!\n");
        /* damage comes before alter_pos here. If alter_pos came first, then if
         * the fight was started by this action, you might be in a sitting
         * position when the fight began, which would make you automatically
         * stand. We don't want that. */
        damage(ch, vict, 0, skill);
        alter_pos(ch, POS_SITTING, STANCE_ALERT);
    }

    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    SET_COOLDOWN(ch, CD_BASH, PULSE_VIOLENCE * 2);
    improve_skill_offensively(ch, vict, skill);
}

ACMD(do_rescue) {
    CharData *vict, *attacker, *c;
    int percent, prob, num;

    one_argument(argument, arg);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, "Whom do you want to rescue?\n");
        return;
    }
    if (vict == ch) {
        char_printf(ch, "What about fleeing instead?\n");
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "You feel ashamed trying to disturb the peace of this room.\n");
        return;
    }
    if (FIGHTING(ch) == vict) {
        char_printf(ch, "How can you rescue someone you are trying to kill?\n");
        return;
    }
    if (!vict->attackers) {
        act("But nobody is fighting $M!", false, ch, 0, vict, TO_CHAR);
        return;
    }
    if (GET_SKILL(ch, SKILL_RESCUE) == 0) {
        char_printf(ch, "But only true warriors can do this!\n");
        return;
    }

    if (GET_SKILL(ch, SKILL_BERSERK) && !EFF_FLAGGED(ch, EFF_BERSERK)) {
        act("You aren't angry enough to rescue $M!", false, ch, 0, vict, TO_CHAR);
        return;
    }

    /* Choose a random attacker from those fighting vict */
    attacker = vict->attackers;
    num = 1;
    for (c = attacker->next_attacker; c; c = c->next_attacker) {
        num++;
        if (random_number(1, num) == 1)
            attacker = c;
    }

    percent = random_number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_RESCUE);

    if (percent > prob) {
        char_printf(ch, "You fail the rescue!\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
        improve_skill_offensively(ch, attacker, SKILL_RESCUE);
        return;
    }
    char_printf(ch, "Banzai!   To the rescue...\n");
    act("You are rescued by $N, you are confused!", false, vict, 0, ch, TO_CHAR);
    act("$n heroically rescues $N!", false, ch, 0, vict, TO_NOTVICT);

    if (FIGHTING(vict) == attacker)
        stop_fighting(vict);
    stop_fighting(attacker);
    if (FIGHTING(ch))
        stop_fighting(ch);

    set_fighting(ch, attacker, true);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    WAIT_STATE(vict, PULSE_VIOLENCE + 2);

    improve_skill_offensively(ch, attacker, SKILL_RESCUE);
}

ACMD(do_kick) {
    CharData *vict;
    int percent, prob;

    if (GET_SKILL(ch, SKILL_KICK) == 0) {
        char_printf(ch, "You'd better leave all the martial arts to fighters.\n");
        return;
    }

    one_argument(argument, arg);

    if (!arg || !*arg) {
        if (FIGHTING(ch)) {
            vict = FIGHTING(ch);
        } else {
            char_printf(ch, "Kick who?\n");
            return;
        }
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, "Kick who?\n");
        return;
    }

    if (vict == ch) {
        char_printf(ch, "Aren't we funny today...\n");
        return;
    }

    if (!attack_ok(ch, vict, true))
        return;

    if (EFF_FLAGGED(ch, EFF_IMMOBILIZED)) {
        char_printf(ch, "You can't lift your legs!\n");
        return;
    }

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    /* Need to see whether this player is fighting already. Kick should not
       allow for the player to switch without a switch probability being
       calculated into the mix. (DEMOLITUM) 
       
       Edited to allow kicks from roundhouse skill without switching (DAEDELA) */
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (subcmd != SKILL_ROUNDHOUSE) {
        if (FIGHTING(ch) && FIGHTING(ch) != vict && !switch_ok(ch))
            return;
    }

    percent = ((10 - ((GET_AC(vict) + (monk_weight_penalty(vict) * 5)) / 10)) << 1) + random_number(1, 101);
    prob = GET_SKILL(ch, SKILL_KICK);
    if (percent > prob) {
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        damage(ch, vict, 0, SKILL_KICK);
    } else {
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        if (displaced(ch, vict))
            return;

        if (damage_evasion(vict, ch, 0, DAM_CRUSH)) {
            act(EVASIONCLR "Your foot passes harmlessly through $N" EVASIONCLR "!&0", false, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n&7&b sends $s foot whistling right through $N" EVASIONCLR ".&0", false, ch, 0, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR " tries to kick you, but $s foot passes through you harmlessly.&0", false,
                ch, 0, vict, TO_VICT);
            set_fighting(vict, ch, true);
            return;
        }
        damage(ch, vict, dam_suscept_adjust(ch, vict, 0, GET_LEVEL(ch) >> 1, DAM_CRUSH), SKILL_KICK);
    }
    improve_skill_offensively(ch, vict, SKILL_KICK);
}

ACMD(do_eye_gouge) {
    CharData *vict;
    int percent, prob;
    effect eff;

    if (GET_SKILL(ch, SKILL_EYE_GOUGE) == 0) {
        char_printf(ch, "You would do such a despicable act!?   Horrible!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BLIND)) {
        char_printf(ch, "It's going to be hard to gouge someone's eyes out if you can't even see.\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            char_printf(ch, "Whose eyes do you want to gouge out?\n");
            return;
        }
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, "Nobody here by that name.\n");
        return;
    }

    if (FIGHTING(ch) && FIGHTING(ch) != vict) {
        char_printf(ch, "You need to be facing each other for this to work.\n");
        return;
    }

    if (vict == ch) {
        char_printf(ch, "That would make life difficult, wouldn't it?\n");
        return;
    }

    /* check pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, true))
        return;

    if (GET_COOLDOWN(ch, CD_EYE_GOUGE)) {
        char_printf(ch, "You aren't able to find an opening yet!\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_GOD)
        SET_COOLDOWN(ch, CD_EYE_GOUGE, 3 * PULSE_VIOLENCE);

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    percent = random_number(1, 101);
    prob = GET_SKILL(ch, SKILL_EYE_GOUGE);
    WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
    if (displaced(ch, vict))
        return;
    if (percent > prob && AWAKE(vict))
        damage(ch, vict, 0, SKILL_EYE_GOUGE); /* Miss message */
    else if (damage_evasion(vict, ch, 0, DAM_PIERCE)) {
        act(EVASIONCLR "Your thumbs poke harmlessly at $N" EVASIONCLR ".   If $E even has eyes.&0", false, ch, 0, vict,
            TO_CHAR);
        act(EVASIONCLR "$n" EVASIONCLR " tries poking at $N's eyes, but nothing seems to happen.&0", false, ch, 0, vict,
            TO_NOTVICT);
        act(EVASIONCLR "$n" EVASIONCLR " pokes fruitlessly at you with $s thumbs.&0", false, ch, 0, vict, TO_VICT);
        set_fighting(vict, ch, true);
        return;
    } else if (GET_LEVEL(vict) >= LVL_IMMORT && GET_LEVEL(vict) > GET_LEVEL(ch)) {
        act("Ouch!   You hurt your thumbs trying to poke out $N's eyes!", false, ch, 0, vict, TO_CHAR);
        act("$n tries poking out $N's eyes - what a laugh!", false, ch, 0, vict, TO_NOTVICT);
        act("$n pokes harmlessly at your eyes with $s thumbs.", false, ch, 0, vict, TO_VICT);
        return;
    } else {
        if (!MOB_FLAGGED(vict, MOB_NOBLIND) && !EFF_FLAGGED(vict, EFF_BLIND)) {
            memset(&eff, 0, sizeof(eff));
            eff.type = SKILL_EYE_GOUGE;
            eff.duration = 1;
            eff.modifier = -2 - GET_SKILL(ch, SKILL_EYE_GOUGE) / 10;
            eff.location = APPLY_HITROLL;
            SET_FLAG(eff.flags, EFF_BLIND);
            effect_to_char(vict, &eff);
        }
        damage(ch, vict, dam_suscept_adjust(ch, vict, 0, (GET_SKILL(ch, SKILL_EYE_GOUGE) + percent) / 4, DAM_PIERCE),
               SKILL_EYE_GOUGE);
    }

    if (!MOB_FLAGGED(vict, MOB_NOBLIND) && !EFF_FLAGGED(vict, EFF_BLIND))
        improve_skill_offensively(ch, vict, SKILL_EYE_GOUGE);
}

ACMD(do_springleap) {
    CharData *vict;
    int percent, prob, dmg;

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS) && !CAN_SEE_IN_DARK(ch)) {
        char_printf(ch, "It is too dark!&0\n");
        return;
    }

    if (!GET_SKILL(ch, SKILL_SPRINGLEAP)) {
        char_printf(ch, "You'd better leave all the martial arts to monks.\n");
        return;
    }

    if (GET_POS(ch) > POS_SITTING) {
        char_printf(ch, "You can't spring from that position, try sitting!\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_IMMOBILIZED)) {
        char_printf(ch, "You can't lift your legs!\n");
        return;
    }

    one_argument(argument, arg);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            char_printf(ch, "&0Spring-leap whom?&0\n");
            return;
        }
    }

    if (vict == ch) {
        char_printf(ch, "That might hurt too much...\n");
        return;
    }

    /* check pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, true))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    percent = random_number(6, 77) - (GET_AC(vict) + (5 * monk_weight_penalty(vict))) / 20;

    prob = GET_SKILL(ch, SKILL_SPRINGLEAP);

    if (GET_POS(vict) <= POS_SITTING)
        percent = 101;

    if (GET_STANCE(vict) < STANCE_FIGHTING)
        prob -= 20;

    if (percent > prob) {
        act("&0&6You try to take $N down but you spring over $S head!&0", false, ch, 0, vict, TO_CHAR);
        act("&0&6$N springs from the ground at you but soars over your head!&0", false, vict, 0, ch, TO_CHAR);
        act("&0&6$N springs from the ground at $n but misses by a mile!&0", false, vict, 0, ch, TO_NOTVICT);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        damage(ch, vict, 0, SKILL_SPRINGLEAP);
        if (AWAKE(ch)) {
            GET_POS(ch) = POS_SITTING;
            GET_STANCE(ch) = STANCE_ALERT;
        }
    } else if (damage_evasion(vict, ch, 0, DAM_CRUSH)) {
        act(EVASIONCLR "You hurtle right through $N" EVASIONCLR " and land in a heap on the other side!&0", false, ch, 0,
            vict, TO_CHAR);
        act(EVASIONCLR "$n" EVASIONCLR " leaps at $N" EVASIONCLR " but flies right on through!&0", false, ch, 0, vict,
            TO_NOTVICT);
        act(EVASIONCLR "$n" EVASIONCLR " comes flying at you, but just passes through and hits the ground.&0", false, ch,
            0, vict, TO_VICT);
        /* You fall */
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_ALERT;
        set_fighting(vict, ch, false);
        return;
    } else if (displaced(ch, vict)) {
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_ALERT;
        set_fighting(vict, ch, false);
        return;
    } else if (percent > 0.95 * prob) {
        dmg = dam_suscept_adjust(ch, vict, 0, GET_LEVEL(ch) >> 1, DAM_CRUSH);
        act("&0&6You manage to take $N down but also &bfall down yourself!&0 (&3$i&0)", false, ch, dmg, vict, TO_CHAR);
        act("&0&6$N springs from the ground and knocks you down - &bbut falls in the process!&0 (&1$i&0)", false, vict,
            dmg, ch, TO_CHAR);
        act("&0&6$N springs from the ground, knocking $n down and &bfalling in the process!&0 (&4$i&0)", false, vict,
            dmg, ch, TO_NOTVICT);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        WAIT_STATE(vict, (PULSE_VIOLENCE * 3) / 2);
        damage(ch, vict, dmg, SKILL_SPRINGLEAP);
        if (AWAKE(vict) && IN_ROOM(ch) == IN_ROOM(vict)) {
            abort_casting(vict);
            GET_POS(vict) = POS_SITTING;
            GET_STANCE(vict) = STANCE_ALERT;
        }
        if (AWAKE(ch)) {
            GET_POS(ch) = POS_SITTING;
            GET_STANCE(ch) = STANCE_ALERT;
        }
    } else {
        dmg = dam_suscept_adjust(ch, vict, 0, GET_LEVEL(ch) >> 1, DAM_CRUSH);
        act("&0&bYou spring from the ground, knocking $N off balance.&0 (&1$i&0)", false, ch, dmg, vict, TO_CHAR);
        act("&0&b$N springs from the ground and knocks you down!&0 (&3$i&0)", false, vict, dmg, ch, TO_CHAR);
        act("&0&b$N springs from the ground, knocking $n down!&0 (&4$i&0)", false, vict, dmg, ch, TO_NOTVICT);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        WAIT_STATE(vict, (PULSE_VIOLENCE * 3) / 2);
        damage(ch, vict, dmg, SKILL_SPRINGLEAP);
        if (AWAKE(vict) && IN_ROOM(ch) == IN_ROOM(vict)) {
            abort_casting(vict);
            GET_POS(vict) = POS_SITTING;
            GET_STANCE(vict) = STANCE_ALERT;
        }
        if (AWAKE(ch)) {
            GET_POS(ch) = POS_STANDING;
            GET_STANCE(ch) = FIGHTING(ch) ? STANCE_FIGHTING : STANCE_ALERT;
        }
    }

    improve_skill_offensively(ch, vict, SKILL_SPRINGLEAP);
} /* end springleap */

/* Throatcutting weapons: slashing, one-handed. */
#define WEAPON_CAN_THROATCUT(obj) (IS_WEAPON_SLASHING(obj) && CAN_WEAR(obj, ITEM_WEAR_WIELD))
#define THROATCUT_DAMAGE DAM_SLASH

ACMD(do_throatcut) {
    CharData *vict;
    effect eff;
    ObjData *weapon;
    int random, chance, percent, dam, expReduction;
    char buf1[255];
    char buf2[255];
    char buf3[255];
    char stop_buf1[255];
    char stop_buf2[255];
    bool skipcast = false;

    if (GET_COOLDOWN(ch, CD_THROATCUT)) {
        int seconds = GET_COOLDOWN(ch, CD_THROATCUT) / 10;
        char_printf(ch,
                    "You've drawn too much attention to yourself to throatcut now!\n"
                    "You can attempt to throatcut again in {:d} {}.\n",
                    seconds, seconds == 1 ? "second" : "seconds");
        return;
    }

    random = roll_dice(1, 6);

    if ((GET_LEVEL(ch) < LVL_IMMORT) && (GET_SKILL(ch, SKILL_THROATCUT) == 0)) {
        char_printf(ch, "You aren't skilled enough!\n");
        return;
    }

    if (FIGHTING(ch)) {
        char_printf(ch, "You can't be stealthy enough to do this while fighting!\n");
        return;
    }

    if (RIDING(ch)) {
        char_printf(ch, "Cut someone's throat while riding???   I don't think so!\n");
        return;
    }

    one_argument(argument, buf);

    if ((!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) && (!(vict == ch))) {
        char_printf(ch, "Cut whose throat?\n");
        return;
    }

    if (vict == ch) {
        char_printf(ch, "Hey, life's not that bad!\n");
        return;
    }

    /* Got a weapon? Any weapon? */
    weapon = GET_EQ(ch, WEAR_WIELD);
    if (!weapon) {
        weapon = GET_EQ(ch, WEAR_WIELD2);
    }
    if (!weapon) {
        weapon = GET_EQ(ch, WEAR_2HWIELD);
    }

    if (!weapon) {
        char_printf(ch, "&0You need a one-handed slashing weapon to cut throats.&0\n");
        return;
    }

    /* If wielding something unsuitable in first hand, use weapon in second hand
     */
    if (!WEAPON_CAN_THROATCUT(weapon) && GET_EQ(ch, WEAR_WIELD2)) {
        weapon = GET_EQ(ch, WEAR_WIELD2);
    }

    if (!WEAPON_CAN_THROATCUT(weapon)) {
        char_printf(ch, "&0You need a one-handed slashing weapon to cut throats.&0\n");
        return;
    }

    if (FIGHTING(vict)) {
        char_printf(ch, "&0You can't cut the throat of a fighting person -- they're too alert!&0\n");
        return;
    }

    if (!attack_ok(ch, vict, true))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    /* Can't throatcut dragons, nor mobs that are twice your size */
    /*
    if (GET_RACE(vict) == RACE_DRAGON) {
    */
    if (GET_RACE(vict) == RACE_DRAGON_GENERAL || GET_RACE(vict) == RACE_DRAGON_FIRE ||
        GET_RACE(vict) == RACE_DRAGON_ACID || GET_RACE(vict) == RACE_DRAGON_FROST ||
        GET_RACE(vict) == RACE_DRAGON_LIGHTNING || GET_RACE(vict) == RACE_DRAGON_GAS) {
        char_printf(ch, "Cut the throat... of a dragon... RIGHT!!!!!\n");
        return;
    } else if ((GET_SIZE(vict) > GET_SIZE(ch) + 2) || (GET_SIZE(vict) < GET_SIZE(ch) - 2)) {
        char_printf(ch, "Maybe if you were close to the same size it would work!!\n");
        return;
    }

    SET_COOLDOWN(ch, CD_THROATCUT, 3 MUD_HR);

    percent = roll_dice(1, 100);

    if ((MOB_FLAGGED(vict, MOB_AWARE) || EFF_FLAGGED(vict, EFF_AWARE)) && AWAKE(vict) &&
        !EFF_FLAGGED(vict, EFF_MINOR_PARALYSIS) && !EFF_FLAGGED(vict, EFF_MAJOR_PARALYSIS)) {
        act("You notice $N sneaking up on you!", false, vict, 0, ch, TO_CHAR);
        act("$e notices you sneaking up on $m!", false, vict, 0, ch, TO_VICT);
        act("$n notices $N sneaking up on $m!", false, vict, 0, ch, TO_NOTVICT);
        attack(vict, ch);
        return;
    }

    chance = GET_SKILL(ch, SKILL_THROATCUT) - GET_LEVEL(vict) + 90;

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        chance += 1000;

    if (!AWAKE(vict))
        chance += 1000; /* sleeping should always be a coup de grace */

    if (!CAN_SEE(vict, ch))
        chance += 15;

    switch (world[vict->in_room].sector_type) {
    case SECT_CITY:
        chance += 15;
        break;
    case SECT_FIELD:
        chance -= 10;
        break;
    case SECT_ROAD:
        chance -= 5;
        break;
    case SECT_BEACH:
        chance -= 10;
        break;
    case SECT_GRASSLANDS:
        chance -= 10;
        break;
    case SECT_FOREST:
        chance -= 15;
        break;
    case SECT_RUINS:
        chance += 5;
        break;
    case SECT_SWAMP:
        chance -= 20;
        break;
    case SECT_SHALLOWS:
        chance -= 50;
        break;
    case SECT_WATER:
        chance -= 75;
        break;
    case SECT_AIR:
        chance += 20;
        break;
    default:
        chance += 0;
        break;
    }

    if ((!IS_NPC(vict)) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
        act("$N laughs out loud at your miserable attempt!&0", false, ch, 0, vict, TO_CHAR);
        act("$n just tried to cut your throat. &0&6How cute!&0", false, ch, 0, vict, TO_VICT);
        return;
    }

    /* you get tense when someone tries to cut your throat! */
    memset(&eff, 0, sizeof(eff));
    eff.type = SKILL_AWARE;
    eff.duration = 5;
    eff.modifier = 0;
    eff.location = APPLY_NONE;
    SET_FLAG(eff.flags, EFF_AWARE);
    effect_to_char(vict, &eff);

    if (chance > 95) {
        random = 7;
        chance = 95;
    }

    /* The moment of excitement! */
    if (percent < chance) {
        if (damage_evasion(vict, ch, weapon, THROATCUT_DAMAGE)) {
            damage_evasion_message(ch, vict, weapon, THROATCUT_DAMAGE);
            set_fighting(vict, ch, true);
            return;
        }
        if (displaced(ch, vict))
            return;
        /* Switch for dam %.   1 & 6: really goods, 2 & 5: decent, 3 & 4: common
         * shaving nicks, 7 mega, 8 miss penalty */
        switch (random) {
        case 1:
        case 6:
            dam = (GET_MAX_HIT(vict) * 3) / 4;
            expReduction = (GET_EXP(vict) * 3) / 4; /* rip same % exp from the mob... since they're doing less work! */

            sprintf(buf1, "&1&bYou nearly sever the head of $N with %s.&0", weapon->short_description);
            sprintf(buf2, "&1&b$n nearly severs your head with %s!&0", weapon->short_description);
            sprintf(buf3, "&1&b$n nearly severs the head of $N with %s!&0", weapon->short_description);
            sprintf(stop_buf1, "Your profuse bleeding interrupts your chanting!");
            sprintf(stop_buf2, "$n stops chanting abruptly!");
            break;
        case 2:
        case 5:
            dam = GET_MAX_HIT(vict) / 4;
            expReduction = GET_EXP(vict) / 4; /* rip same % exp from the mob... since they're doing less work! */

            sprintf(buf1, "&1&bBlood splatters all over you as you cut into $N with %s.&0", weapon->short_description);
            sprintf(buf2, "&1&bBlood splatters all over $n as $e cuts into you with %s!&0", weapon->short_description);
            sprintf(buf3, "&1&bBlood splatters all over $n as $e dices $N with %s!&0", weapon->short_description);
            sprintf(stop_buf1, "Your chanting is interrupted by your coughing up blood!");
            sprintf(stop_buf2, "$n stops chanting abruptly!");
            break;
        case 3:
        case 4:
            dam = GET_MAX_HIT(vict) / 8;
            expReduction = GET_EXP(vict) / 8; /* rip same % exp from the mob... since they're doing less work! */

            sprintf(buf1, "&1&b$N gasps as you slice into $S throat with %s.&0", weapon->short_description);
            sprintf(buf2, "&1&bYou gasp with fear as $n slices into your throat with %s!&0", weapon->short_description);
            sprintf(buf3, "&1&b$N looks horrified as $n slices into $S throat with %s!&0", weapon->short_description);
            sprintf(stop_buf1, "Your gasp abruptly interrupts your chanting!");
            sprintf(stop_buf2, "$n stops chanting abruptly!");
            break;
        case 7:
            dam = (GET_MAX_HIT(vict) * 9) / 10;
            expReduction = (GET_EXP(vict) * 9) / 10; /* rip same % exp from the mob... since
                                                        they're doing less work! */

            sprintf(buf1, "&1&bBlood spews everywhere as you nearly incapacitate $N with %s.&0",
                    weapon->short_description);
            sprintf(buf2, "&1&bBlood spews everywhere as $n nearly incapacitates you with %s!&0",
                    weapon->short_description);
            sprintf(buf3, "&1&bBlood spews everywhere as $n nearly incapacitates $N with %s!&0",
                    weapon->short_description);
            sprintf(stop_buf1, "Your chanting is interrupted by your gurgling of blood!");
            sprintf(stop_buf2, "$n stops chanting abruptly!");
            break;
        default:
            dam = expReduction = 0;
            break;
        }
    } else {
        dam = 0;
        skipcast = true;
        expReduction = 0; /* rip same % exp from the mob... since they're doing less work! */

        /* If we want silent misses for non-critical misses.. remove the act txt */
        sprintf(buf1, "&3&b$N jumps back before you have a chance to even get close!&0");
        sprintf(buf2, "&3&b$n just tried to cut your throat!&0");
        sprintf(buf3, "&3&b$n misses $N with $s throat cut!&0");
    }

    if (IS_NPC(vict))
        GET_EXP(vict) = std::max(1l, GET_EXP(vict) - expReduction); /* make sure we don't have
                                                                 negative exp gain for ch */

    if (damage_amounts) {
        if (dam <= 0)
            sprintf(buf, " (&1%d&0)", dam);
        else
            sprintf(buf, " (&3%d&0)", dam);

        strcat(buf1, buf);
        act(buf1, false, ch, nullptr, vict, TO_CHAR);

        strcat(buf2, buf);
        act(buf2, false, ch, nullptr, vict, TO_VICT);

        strcat(buf3, buf);
        act(buf3, false, ch, nullptr, vict, TO_NOTVICT);
    } else {
        act(buf1, false, ch, nullptr, vict, TO_CHAR);
        act(buf2, false, ch, nullptr, vict, TO_VICT);
        act(buf3, false, ch, nullptr, vict, TO_NOTVICT);
    }

    if (dam > 0) {
        memset(&eff, 0, sizeof(eff));
        eff.type = SKILL_THROATCUT;
        eff.duration = 2;
        eff.modifier = 0;
        eff.location = APPLY_NONE;
        SET_FLAG(eff.flags, EFF_HURT_THROAT);
        effect_to_char(vict, &eff);
    }

    GET_HIT(vict) -= dam;
    damage(ch, vict, 0, SKILL_THROATCUT);

    improve_skill_offensively(ch, vict, SKILL_THROATCUT);

    if (!skipcast && CASTING(vict)) {
        STOP_CASTING(vict);
        act(stop_buf1, false, vict, 0, 0, TO_CHAR);
        act(stop_buf2, false, vict, 0, 0, TO_ROOM);
    }
}

ACMD(do_disarm) {
    int pos, ch_pos, chance, rnd_num, skl_bonus, move_cost;
    ObjData *obj, *ch_obj;          /* Object to disarm */
    CharData *tch, *vict = nullptr; /* Target */
    bool disarm_prim = true;

    if (GET_SKILL(ch, SKILL_DISARM) == 0) {
        char_printf(ch, "You don't know how to disarm.\n");
        return;
    }

    /* Make sure we're fighting someone who'll be disarmed */
    if (!FIGHTING(ch)) {
        char_printf(ch, "You can only disarm someone who you're fighting.\n");
        return;
    }

    tch = FIGHTING(ch);

    /* Fighting yourself? Unlikely... but anyway: */
    if (ch == tch) {
        char_printf(ch, "Try 'remove' instead.\n");
        return;
    }

    move_cost = 11 - GET_SKILL(ch, SKILL_DISARM) / 6;

    /* Need mv to perform disarm */
    if (GET_MOVE(ch) < move_cost) {
        char_printf(ch, "You don't have the energy to do that.\n");
        return;
    }

    if (GET_COOLDOWN(ch, CD_FUMBLING_PRIMARY) || GET_COOLDOWN(ch, CD_FUMBLING_SECONDARY)) {
        char_printf(ch, "Impossible!  You're already fumbling for your own weapon.\n");
        return;
    }

    /* Can't disarm them if you can't see them */
    if (!CAN_SEE(ch, tch)) {
        char_printf(ch, "It's pretty hard to disarm someone you can't even see...\n");
        return;
    }

    /* Make sure disarmer is wielding a weapon. - might want to reconsider this
     * later : Pergus */
    if ((ch_obj = GET_EQ(ch, WEAR_WIELD))) {
        ch_pos = WEAR_WIELD;
    } else if ((ch_obj = GET_EQ(ch, WEAR_2HWIELD))) {
        ch_pos = WEAR_2HWIELD;
    } else if ((ch_obj = GET_EQ(ch, WEAR_WIELD2))) {
        ch_pos = WEAR_WIELD2;
        disarm_prim = false;
    } else {
        char_printf(ch, "You must be wielding some kind of weapon.\n");
        return;
    }

    /* Determine what item is to be wrested away */

    one_argument(argument, arg);
    if (!*arg) {
        /* Nothing specified: look for primary, then secondary weapon */
        if ((obj = GET_EQ(tch, WEAR_WIELD))) {
            pos = WEAR_WIELD;
        } else if ((obj = GET_EQ(tch, WEAR_WIELD2))) {
            pos = WEAR_WIELD2;
        } else if ((obj = GET_EQ(tch, WEAR_2HWIELD))) {
            pos = WEAR_2HWIELD;
        } else {
            act("$N isn't even wielding a weapon!", false, ch, 0, tch, TO_CHAR);
            return;
        }
    } else if ((obj = GET_EQ(tch, WEAR_WIELD)) && isname(arg, obj->name) && /* Same name */
               CAN_SEE_OBJ(ch, obj)) {                                      /* Can see it */
        pos = WEAR_WIELD;
    } else if ((obj = GET_EQ(tch, WEAR_WIELD2)) && isname(arg, obj->name) && /* Same name */
               CAN_SEE_OBJ(ch, obj)) {                                       /* Can see it */
        pos = WEAR_WIELD2;
    } else if ((obj = GET_EQ(tch, WEAR_2HWIELD)) && isname(arg, obj->name) && /* Same name */
               CAN_SEE_OBJ(ch, obj)) {                                        /* Can see it */
        pos = WEAR_2HWIELD;
    } else {
        act("$N doesn't seem to be wielding any such thing.", false, ch, 0, tch, TO_CHAR);
        return;
    }

    /* Unlikely, but but just in case: */
    if (GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
        char_printf(ch, "You can only disarm weapons.\n");
        return;
    }

    if (CONFUSED(ch)) {
        char_printf(ch, "You're way too addle-headed to disarm anyone.\n");
        return;
    }

    /* The attempt may take place! */

    /* Calculate the outcome, based on:
     *
     * - disarm skill
     * - dexterity
     * - level difference
     */

    chance = GET_SKILL(ch, SKILL_DISARM) + skill_stat_bonus[GET_DEX(ch)].rogue_skills + (GET_LEVEL(ch) - GET_LEVEL(tch));

    /* 1 - 35 extra points to account for superlative dex or xp diff */
    skl_bonus = (int)((chance - 69) / 2);

    if (skl_bonus > 0)
        chance += skl_bonus;

    chance = std::max(1, chance);

    /* has char tried to disarm within delay period?  if so, penalize them */
    if (GET_COOLDOWN(ch, CD_DISARM))
        chance -= 30; /* 30% pts */

    rnd_num = random_number(1, chance);

    /*  ** Outcomes **

       A is the person disarming, and B is being disarmed:

       1-5    -- A drops weapon in miserably failed disarm attempt.
       6-25   -- A fumbles his weapon in failed disarm attempt.
       26-75  -- Nothing happens.
       76-95  -- A forces B to fumble weapon.
       96+    -- A knocks B's weapon to the ground.

     */

    /* if char tries to disarm again within 1->3 rds of violence, chance of
     * success cut by a ~1/5 */
    SET_COOLDOWN(ch, CD_DISARM, random_number(1, 3) * PULSE_VIOLENCE);

    if (rnd_num <= 5) {
        act("$n fails $s disarming maneuver so badly, $e drops $s own weapon.", false, ch, 0, tch, TO_NOTVICT);
        act("$n tries to disarm but drops $s weapon!", false, ch, 0, tch, TO_VICT);
        act("You try to disarm $N but drop your $o instead!", false, ch, ch_obj, tch, TO_CHAR);

        pos = ch_pos;
        obj = ch_obj;
        vict = ch;
    } else if (rnd_num <= 25) {
        act("$n flubs an attempt at disarming $N.", false, ch, 0, tch, TO_NOTVICT);
        act("$e fumbles $s own weapon.", false, ch, 0, 0, TO_NOTVICT);
        act("$n fumbles $s weapon while trying to disarm you.", false, ch, 0, tch, TO_VICT);
        act("Oops!  You fumbled your $o!", false, ch, ch_obj, 0, TO_CHAR);

        pos = ch_pos;
        obj = ch_obj;
        vict = ch;
    } else if (rnd_num >= 26 && rnd_num <= 75) {
        act("$n tries to disarm $N, but $E keeps a firm grip on $S weapon.", false, ch, 0, tch, TO_NOTVICT);
        act("$n tries to disarm you, but you maintain your weapon.", false, ch, 0, tch, TO_VICT);
        act("You try to disarm $N, but $E keeps hold of $S weapon.", false, ch, 0, tch, TO_CHAR);
    } else if (rnd_num <= 95) {
        act("$n causes $N to fumble $S weapon.", false, ch, 0, tch, TO_NOTVICT);
        act("$n causes you to fumble your weapon.", false, ch, 0, tch, TO_VICT);
        act("You cause $N to fumble $S weapon.", false, ch, 0, tch, TO_CHAR);

        vict = tch;
    } else {
        act("$n successfully knocks $N's weapon from $S grip!", false, ch, 0, tch, TO_NOTVICT);
        act("$n forces $p out of your hands with a fancy disarming maneuver.", false, ch, obj, tch, TO_VICT);
        if (CH_OUTSIDE(tch)) {
            act("You send $N's weapon crashing to the ground.", false, ch, 0, tch, TO_CHAR);
        } else {
            act("You send $N's weapon crashing to the floor.", false, ch, 0, tch, TO_CHAR);
        }

        vict = tch;
    }

    improve_skill_offensively(ch, tch, SKILL_DISARM);

    /* handle cases where either A or B loses hold of weapon */
    if (rnd_num <= 5 || rnd_num >= 96) {
        if ((OBJ_FLAGGED(obj, ITEM_NODROP))) { /* Cursed? */
            obj_to_char(unequip_char(vict, pos), vict);
            act("&3&b$p&3&b magically returns to your&0 &B&3inventory!&0", true, vict, obj, nullptr, TO_CHAR);
            act("&3&b$p&3&b magically returns to $s&0 &B&3inventory!&0", true, vict, obj, nullptr, TO_ROOM);
        } else {
            obj_to_room(unequip_char(vict, pos), vict->in_room);
            sprintf(buf, "%s lands on the %s.", obj->short_description, CH_OUTSIDE(vict) ? "ground" : "floor");
            act(buf, false, vict, 0, 0, TO_ROOM);
            act(buf, false, vict, 0, 0, TO_CHAR);
        }
        /* Move secondary hand weapon to primary hand. */
        if (pos == WEAR_WIELD && GET_EQ(vict, WEAR_WIELD2))
            equip_char(vict, unequip_char(vict, WEAR_WIELD2), WEAR_WIELD);

        /* delay is in units of "passes".  since a mob/pc can be disarmed */
        /* mutliple times, the delay count must be cumulative.  this count is
         * decremented */
        /* in perform_violence(). */
        if (disarm_prim) {
            SET_COOLDOWN(vict, CD_DROPPED_PRIMARY, 2 * PULSE_VIOLENCE);
        } else {
            SET_COOLDOWN(vict, CD_DROPPED_SECONDARY, 2 * PULSE_VIOLENCE);
        }

        /* items ONLY have this set when a MOB is successfuly disarmed */
        /* the item lies on the ground with this bit set, so when someone */
        /* attempts to get it, do_get() can handle it appropriately */
        SET_FLAG(GET_OBJ_FLAGS(obj), ITEM_WAS_DISARMED);

        /* we must remember who last held this item, so the MOB scanning for its */
        /* disarmed item knows THAT is it. */
        obj->last_to_hold = vict;

    } else if (rnd_num <= 25 || rnd_num >= 76) {
        obj_to_char(unequip_char(vict, pos), vict);

        /* Move secondary hand weapon to primary hand. */
        if (pos == WEAR_WIELD && GET_EQ(vict, WEAR_WIELD2))
            equip_char(vict, unequip_char(vict, WEAR_WIELD2), WEAR_WIELD);

        /* delay is in units of "passes".  since a mob/pc can be disarmed */
        /* mutliple times, the delay count must be cumulative.  this count is
         * decremented */
        /* in perform_violence(). */
        if (disarm_prim) {
            SET_COOLDOWN(vict, CD_FUMBLING_PRIMARY, 2 * PULSE_VIOLENCE);
        } else {
            SET_COOLDOWN(vict, CD_FUMBLING_SECONDARY, 2 * PULSE_VIOLENCE);
        }
    }

    if (GET_LEVEL(ch) < LVL_IMMORT)
        alter_move(ch, move_cost);
    WAIT_STATE(ch, PULSE_VIOLENCE + 2);
}

ACMD(do_hitall) {
    CharData *mob, *next_mob, *orig_target;
    byte percent;
    bool hit_all = false, realvictims = false, success = false;

    if (!ch || ch->in_room == NOWHERE)
        return;

    if (subcmd == SCMD_TANTRUM) {
        if (!GET_SKILL(ch, SKILL_TANTRUM)) {
            char_printf(ch, "You throw a hissy-fit, hoping someone will notice you.\n");
            act("$n sobs to $mself loudly, soliciting attention.", true, ch, 0, 0, TO_ROOM);
            return;
        }
        if (!EFF_FLAGGED(ch, EFF_BERSERK)) {
            char_printf(ch, "You're not feeling quite up to throwing a tantrum right now.\n");
            return;
        }
    } else if (!GET_SKILL(ch, SKILL_HITALL)) {
        char_printf(ch, "You don't know how to.\n");
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "You feel ashamed trying to disturb the peace of this room.\n");
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
        char_printf(ch, "Sorry, it's too cramped here for nasty maneuvers!\n");
        return;
    }

    if (FIGHTING(ch))
        orig_target = FIGHTING(ch);

    /* Find out whether to hit "all" or just aggressive monsters */
    one_argument(argument, arg);
    if (!strcasecmp(arg, "all") || subcmd == SCMD_TANTRUM)
        hit_all = 1;

    /* Hit all aggressive monsters in room */

    percent = random_number(1, 131);
    WAIT_STATE(ch, PULSE_VIOLENCE);

    if (subcmd == SCMD_TANTRUM) {
        act("$n flings $s limbs around wildly, swiping at everything nearby!", false, ch, 0, 0, TO_ROOM);
        char_printf(ch, "You throw an incensed tantrum, attacking all nearby!\n");
        if (GET_SKILL(ch, SKILL_TANTRUM) >= percent)
            success = true;
    } else {
        act("$n makes a concerted circular attack at everything nearby!", false, ch, 0, 0, TO_NOTVICT);
        char_printf(ch, "You spin in a circle, attempting to hit everything within range.\n");
        if (GET_SKILL(ch, SKILL_HITALL) >= percent)
            success = true;
    }

    for (mob = world[ch->in_room].people; mob; mob = next_mob) {
        next_mob = mob->next_in_room;

        /* Basic area attack check */
        if (!area_attack_target(ch, mob))
            continue;

        /* If I just entered plain "hitall", don't attack bystanders who aren't
         * aggro to me */
        if (!battling_my_group(ch, mob) && !hit_all && !is_aggr_to(mob, ch))
            continue;

        if (!MOB_FLAGGED(mob, MOB_ILLUSORY))
            realvictims = true;

        if (success) {
            if (damage_evasion(mob, ch, 0, physical_damtype(ch))) {
                damage_evasion_message(ch, mob, equipped_weapon(ch), physical_damtype(ch));
                set_fighting(mob, ch, true);
            } else if (subcmd == SCMD_TANTRUM && random_number(0, 1))
                hit(ch, mob, SKILL_BAREHAND);
            else {
                if (mob != orig_target)
                    attack(ch, mob);
            }
        }
    }

    if (success) {
        if (orig_target)
            attack(ch, orig_target);
    }

    if (realvictims)
        improve_skill(ch, subcmd == SCMD_TANTRUM ? SKILL_TANTRUM : SKILL_HITALL);
}

ACMD(do_corner) {
    CharData *vict;
    int chance;

    if (!GET_SKILL(ch, SKILL_CORNER)) {
        char_printf(ch, "You aren't skilled enough to corner an opponent!\n");
        return;
    }

    one_argument(argument, arg);

    /* You can only corner the person you're fighting. */
    if (!*arg && FIGHTING(ch))
        vict = FIGHTING(ch);
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))) || vict != FIGHTING(ch)) {
        char_printf(ch, "You have to be fighting someone to corner them!\n");
        return;
    }

    if (CONFUSED(ch)) {
        char_printf(ch, "You're far too confused to corner anyone.\n");
        return;
    }

    if (ch->cornering) {
        if (ch->cornering->cornered_by == ch)
            ch->cornering->cornered_by = nullptr;
        ch->cornering = nullptr;
    }

    chance = GET_SKILL(ch, SKILL_CORNER);
    chance += 3 * skill_stat_bonus[GET_DEX(ch)].small;
    chance += 10 * (GET_SIZE(ch) - GET_SIZE(vict));
    chance += (GET_LEVEL(ch) - GET_LEVEL(vict)) / 2;
    if (!CAN_SEE(vict, ch))
        chance *= 2;

    if (chance > random_number(1, 101)) {
        act("You stand in $N's way, cornering $M!", false, ch, 0, vict, TO_CHAR);
        act("$n stands in your way, cornering you!", false, ch, 0, vict, TO_VICT);
        act("$n stands in $N's way, cornering $M!", true, ch, 0, vict, TO_NOTVICT);
        ch->cornering = vict;
        vict->cornered_by = ch;
    } else
        act("You attempt to corner $N, but $E evades you!", false, ch, 0, vict, TO_CHAR);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    improve_skill_offensively(ch, vict, SKILL_CORNER);
}

ACMD(do_peck) {
    CharData *vict;
    int dam;

    if (!ch)
        return;

    if (GET_SKILL(ch, SKILL_PECK) <= 0) {
        char_printf(ch, "How do you expect to do that?\n");
        return;
    }

    one_argument(argument, arg);
    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            char_printf(ch, "Peck whom?\n");
            return;
        }
    }
    if (ch == vict) {
        char_printf(ch, "Ouch, that hurts!\n");
        return;
    }

    /* Is this attack allowed? */
    if (!attack_ok(ch, vict, true))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    /* If attacking someone else, check switch skill. */
    if (FIGHTING(ch) && FIGHTING(ch) != vict && !switch_ok(ch))
        return;

    /* Determine the damage amount.   0 is a miss. */
    if (random_number(0, 101) > GET_SKILL(ch, SKILL_PECK))
        dam = 0;
    else if (damage_evasion(vict, ch, 0, DAM_PIERCE)) {
        damage_evasion_message(ch, vict, 0, DAM_PIERCE);
        set_fighting(vict, ch, true);
        return;
    } else if (displaced(ch, vict))
        return;
    else
        dam = random_number(GET_SKILL(ch, SKILL_PECK), GET_LEVEL(ch));

    damage(ch, vict, dam_suscept_adjust(ch, vict, 0, dam, DAM_PIERCE), SKILL_PECK);
    improve_skill_offensively(ch, vict, SKILL_PECK);
}

ACMD(do_claw) {
    CharData *vict;
    int dam;

    if (!ch)
        return;

    if (GET_SKILL(ch, SKILL_CLAW) <= 0) {
        char_printf(ch, "Grow some longer fingernails first!\n");
        return;
    }

    one_argument(argument, arg);
    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            char_printf(ch, "Claw whom?\n");
            return;
        }
    }
    if (ch == vict) {
        char_printf(ch, "Ouch, that hurts!\n");
        return;
    }

    /* Can we allow this attack to occur? */
    if (!attack_ok(ch, vict, true))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    /* If attacking someone else, check skill in switch. */
    if (FIGHTING(ch) && FIGHTING(ch) != vict && !switch_ok(ch))
        return;

    /* Determine damage amount. */
    if (random_number(0, 101) > GET_SKILL(ch, SKILL_CLAW))
        dam = 0;
    else if (damage_evasion(vict, ch, 0, DAM_SLASH)) {
        damage_evasion_message(ch, vict, 0, DAM_SLASH);
        set_fighting(vict, ch, true);
        return;
    } else if (displaced(ch, vict))
        return;
    else
        dam = random_number(GET_SKILL(ch, SKILL_CLAW), GET_LEVEL(ch));

    damage(ch, vict, dam_suscept_adjust(ch, vict, 0, dam, DAM_SLASH), SKILL_CLAW);
    improve_skill_offensively(ch, vict, SKILL_CLAW);
}

ACMD(do_electrify) {
    if (!ch || ch->in_room == NOWHERE)
        return;

    if (GET_SKILL(ch, SKILL_ELECTRIFY) <= 0) {
        char_printf(ch, "Good luck with that one!\n");
        return;
    }

    if (GET_SKILL(ch, SKILL_ELECTRIFY) > random_number(0, 101))
        mag_area(GET_SKILL(ch, SKILL_ELECTRIFY), ch, SKILL_ELECTRIFY, SAVING_BREATH);
    else {
        if (IS_WATER(ch->in_room))
            char_printf(ch, "The water around you sizzles, but you are unable to gatherany power...\n");
        else
            char_printf(ch, "The air around you crackles, but you are unable to gather any power...\n");
        act("A quick spike of electricity runs across $n's skin.", true, ch, 0, 0, TO_ROOM);
    }

    improve_skill(ch, SKILL_ELECTRIFY);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

void start_berserking(CharData *ch) {
    effect eff;

    memset(&eff, 0, sizeof(eff));
    eff.type = SKILL_BERSERK;
    eff.duration = 1000; /* arbitrarily long time */
    eff.modifier = 0;
    eff.location = APPLY_NONE;
    SET_FLAG(eff.flags, EFF_BERSERK);
    GET_STANCE(ch) = STANCE_ALERT;
    GET_POS(ch) = POS_STANDING;
    effect_to_char(ch, &eff);
    check_regen_rates(ch);
}

void stop_berserking(CharData *ch) {
    effect_from_char(ch, SKILL_BERSERK);
    effect_from_char(ch, CHANT_SPIRIT_WOLF);
    effect_from_char(ch, CHANT_SPIRIT_BEAR);
    effect_from_char(ch, CHANT_INTERMINABLE_WRATH);
    GET_RAGE(ch) = 0;
}

ACMD(do_berserk) {
    if (!GET_SKILL(ch, SKILL_BERSERK)) {
        char_printf(ch, "You flail your arms about, acting like a crazy person.\n");
        act("$n goes berserk, thrashing about the area.", false, ch, 0, 0, TO_ROOM);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        char_printf(ch, "You're already out of control!\n");
        return;
    }

    if (GET_RAGE(ch) < RAGE_ANNOYED) {
        char_printf(ch, "You're not angry enough yet!\n");
        return;
    }

    char_printf(ch, "You feel your blood begin to boil, and your self-control starts to slip...\n");
    act("$n's eyes flash and anger clouds $s face.", true, ch, 0, 0, TO_ROOM);

    start_berserking(ch);
}

/*
 * Be careful when calling do_stomp manually (i.e., not from the command
 * interpreter.   If the cmd number is wrong, and the command gets passed
 * to do_action, the game may crash.
 */
ACMD(do_stomp) {
    CharData *tch, *next_tch;
    bool real_victims = false;

    extern ACMD(do_action);

    if (!GET_SKILL(ch, SKILL_GROUND_SHAKER) || !EFF_FLAGGED(ch, EFF_SPIRIT_BEAR) || !EFF_FLAGGED(ch, EFF_BERSERK)) {
        do_action(ch, argument, cmd, subcmd);
        return;
    }

    if (CH_INDOORS(ch)) {
        char_printf(ch, "You MUST be crazy, trying to do that in here!\n");
        return;
    }

    if (!QUAKABLE(IN_ROOM(ch))) {
        char_printf(ch, "There's no ground to stomp on here!\n");
        return;
    }

    char_printf(ch, "&3You stomp one foot on the ground heavily, shaking the earth!&0\n");
    act("&3$n crashes a foot into the ground, causing it to crack around $m...&0", true, ch, 0, 0, TO_ROOM);

    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        /* Basic area attack check */
        if (!area_attack_target(ch, tch))
            continue;

        /* Can't harm flying folks with this skill */
        if (GET_POS(tch) == POS_FLYING)
            continue;

        if (!damage_evasion(tch, ch, 0, DAM_CRUSH)) {
            if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
                real_victims = true;

            if (GET_DEX(tch) < random_number(0, 100))
                damage(ch, tch,
                       dam_suscept_adjust(ch, tch, 0, random_number(50, GET_SKILL(ch, SKILL_GROUND_SHAKER)), DAM_CRUSH),
                       SKILL_GROUND_SHAKER);
            else if (GET_STR(tch) < random_number(0, 100)) {
                if (GET_POS(tch) > POS_SITTING) {
                    damage(ch, tch, 0, SKILL_GROUND_SHAKER);
                    if (IN_ROOM(ch) == IN_ROOM(tch))
                        alter_pos(tch, POS_KNEELING, STANCE_ALERT);
                    WAIT_STATE(ch, PULSE_VIOLENCE);
                }
            }
        } else {
            set_fighting(tch, ch, true);
        }
    }

    if (real_victims)
        improve_skill(ch, SKILL_GROUND_SHAKER);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_cartwheel) {
    CharData *vict;
    int percent, prob, dmg, hidden, message;
    ObjData *weapon;

    hidden = GET_HIDDENNESS(ch);
    GET_HIDDENNESS(ch) = 0;

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS) && !CAN_SEE_IN_DARK(ch)) {
        char_printf(ch, "It is too dark!&0\n");
        return;
    }

    if (!GET_SKILL(ch, SKILL_CARTWHEEL)) {
        char_printf(ch, "You'd better leave all the tumbling to the rogues.\n");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_IMMOBILIZED)) {
        char_printf(ch, "You don't have the mobility!\n");
        return;
    }

    one_argument(argument, arg);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            char_printf(ch, "&0Cartwheel at whom?&0\n");
            return;
        }
    }

    if (vict == ch) {
        char_printf(ch, "You can't cartwheel into yourself!\n");
        return;
    }

    /* check pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, true))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    if (GET_POS(vict) <= POS_SITTING) {
        act("$E has already been knocked down.", false, ch, 0, vict, TO_CHAR);
        return;
    }

    if (MOB_FLAGGED(vict, MOB_NOBASH)) {
        act("You &3&bslam&0 into $N, but $E seems quite unmoved.", false, ch, 0, vict, TO_CHAR);
        act("$n &3&bslams&0 into $N, who seems as solid as a rock!", false, ch, 0, vict, TO_NOTVICT);
        act("$n &3&bslams&0 into you, attempting to knock you down.", false, ch, 0, vict, TO_VICT);
        /* A pause... but you don't fall down. */
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        set_fighting(vict, ch, false);
        return;
    }

    if (GET_SIZE(vict) - GET_SIZE(ch) > 1) {
        char_printf(ch, "&7&bYou fall over as you try to knock down someone so large!&0\n");
        act("&7&b$n BOUNCES off $N, as $e tries to knock down $N's much larger size.&0", false, ch, 0, vict,
            TO_NOTVICT);
        act("&7&b$n BOUNCES off you as $e tries to knock down your much larger size.&0", false, ch, 0, vict, TO_VICT);
        percent = prob + 1;
    } else if (GET_SIZE(ch) - GET_SIZE(vict) > 2) {
        char_printf(ch, "&7&bYou fall over as you try to knock down someone with such small size.&0\n");
        act("&7&b$n trips over $N, as $e tries to knock down $N's much smaller size.&0", false, ch, 0, vict,
            TO_NOTVICT);
        act("&7&b$n trips over you as $e tries to knock down your much smaller size.&0", false, ch, 0, vict, TO_VICT);
        percent = prob + 1;
    }

    prob = GET_SKILL(ch, SKILL_CARTWHEEL);
    prob += (skill_stat_bonus[GET_DEX(ch)].large / 2);
    prob += (skill_stat_bonus[GET_INT(ch)].large / 2);
    prob += GET_HITROLL(ch) - monk_weight_penalty(ch);
    percent = random_number(1, 100);
    percent += GET_SKILL(vict, SKILL_DODGE);

    if (prob > 0.9 * percent) {
        /* attack evaded */
        if (damage_evasion(vict, ch, 0, DAM_CRUSH) || MOB_FLAGGED(vict, MOB_ILLUSORY))
            message = 1;
        if (displaced(ch, vict))
            message == 2;
        if (message == 1 || message == 2) {
            if (message == 1) {
                act(EVASIONCLR "You cartwheel right through $N" EVASIONCLR " and fall in a heap on the other side!&0",
                    false, ch, 0, vict, TO_CHAR);
                act(EVASIONCLR "$n" EVASIONCLR " cartwheels at $N" EVASIONCLR " but tumbles right on through!&0", false,
                    ch, 0, vict, TO_NOTVICT);
                act(EVASIONCLR "$n" EVASIONCLR " cartwheels at you, but just passes through and hits the ground.&0",
                    false, ch, 0, vict, TO_VICT);
            }
            /* You fall */
            WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
            GET_POS(ch) = POS_SITTING;
            GET_STANCE(ch) = STANCE_ALERT;
            set_fighting(vict, ch, false);
            return;
            /* hits */
        } else {
            /* full success, knock down victim */
            if (prob > percent) {
                act("&0&bYou spring into a cartwheel, knocking $N off balance.&0", false, ch, 0, vict, TO_CHAR);
                act("&0&b$N springs into a cartwheel and knocks you down!&0", false, vict, 0, ch, TO_CHAR);
                act("&0&b$N springs into a cartwheel, knocking down $n!&0", false, vict, 0, ch, TO_NOTVICT);
                WAIT_STATE(ch, PULSE_VIOLENCE);

                /* partial success, both fall down */
            } else if (prob > 0.9 * percent) {
                act("&0&6You manage to take $N down but also &bfall down yourself!&0", false, ch, 0, vict, TO_CHAR);
                act("&0&6$N cartwheels at you and knocks you down - &bbut falls in the process!&0", false, vict, 0, ch,
                    TO_CHAR);
                act("&0&6$N cartwheels at $n, knocking $m down and &bfalling in the process!&0", false, vict, 0, ch,
                    TO_NOTVICT);
                WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
                if (AWAKE(ch)) {
                    GET_POS(ch) = POS_SITTING;
                    GET_STANCE(ch) = STANCE_ALERT;
                }
            }

            /* attack was at least partially successful, see if a backstab can be attempted */
            GET_HIDDENNESS(ch) = hidden;
            weapon = GET_EQ(ch, WEAR_WIELD);
            if (!weapon)
                weapon = GET_EQ(ch, WEAR_WIELD2);
            if (!weapon)
                weapon = GET_EQ(ch, WEAR_2HWIELD);

            if (weapon) {
                /* If wielding something unsuitable in first hand, use weapon in second hand */
                if (!IS_WEAPON_PIERCING(weapon) && GET_EQ(ch, WEAR_WIELD2))
                    weapon = GET_EQ(ch, WEAR_WIELD2);

                /* If piercing weapon found, use it */
                if (IS_WEAPON_PIERCING(weapon)) {
                    act("&0&bYou use the momentum to backstab $N!&0", false, ch, 0, vict, TO_CHAR);
                    do_backstab(ch, arg, 0, 0);
                    set_fighting(ch, vict, false);
                } else {
                    /* no piercing weapon */
                    damage(ch, vict, 0, SKILL_CARTWHEEL); /* damage should always be 0; this is a hack to force
                                                             combatants to stand back up */
                    set_fighting(ch, vict, false);
                }
            } else {
                /* no weapon */
                damage(ch, vict, 0, SKILL_CARTWHEEL); /* damage should always be 0; this is a hack to force combatants
                                                         to stand back up */
                set_fighting(ch, vict, false);
            }

            WAIT_STATE(vict, (PULSE_VIOLENCE * 3) / 2);
            if (AWAKE(vict) && IN_ROOM(ch) == IN_ROOM(vict)) {
                abort_casting(vict);
                GET_POS(vict) = POS_SITTING;
                GET_STANCE(vict) = STANCE_ALERT;
            }
        }
    } else {
        act("&0&6$N sidesteps your fancy cartwheel and you land in a heap!&0", false, ch, 0, vict, TO_CHAR);
        act("&0&6$N cartwheel charges at you but tumbles right past!&0", false, vict, 0, ch, TO_CHAR);
        act("&0&6$N carthweel charges at $n but tumbles right past!&0", false, vict, 0, ch, TO_NOTVICT);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        damage(ch, vict, 0,
               SKILL_CARTWHEEL); /* damage should always be 0; this is a hack to force combatants to stand back up */
        set_fighting(vict, ch, false);
        if (AWAKE(ch)) {
            GET_POS(ch) = POS_SITTING;
            GET_STANCE(ch) = STANCE_ALERT;
        }
    }

    improve_skill_offensively(ch, vict, SKILL_CARTWHEEL);
} /* end cartwheel */

ACMD(do_lure) {
    int dir, was_in, to_room;
    CharData *vict;
    FollowType *k, *next_k;

    argument = delimited_arg(argument, arg, '\'');
    skip_spaces(&argument);
    vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg));

    if (!ch)
        return;

    if (IS_NPC(ch)) {
        char_printf(ch, "NPCs can't lure things.\n");
        return;
    }

    if (!GET_SKILL(ch, SKILL_LURE)) {
        char_printf(ch, "You have no idea how to do that.\n");
        return;
    }

    if (FIGHTING(ch)) {
        char_printf(ch, "You can't lure someone while you're in combat!\n");
        return;
    }

    if (!*arg || !vict) {
        char_printf(ch, "Lure who?!\n");
        return;
    }

    if (vict == ch) {
        char_printf(ch, "You can't lure yourself!\n");
        return;
    }

    if (!*argument) {
        act("Lure $M where!?", false, ch, 0, vict, TO_CHAR);
        return;
    }

    dir = searchblock(argument, dirs, false);

    if (FIGHTING(vict)) {
        char_printf(ch, "You can't lure someone away from combat!\n");
        return;
    }

    /* check for paralysis */
    if (EFF_FLAGGED(vict, EFF_IMMOBILIZED) || EFF_FLAGGED(vict, EFF_MAJOR_PARALYSIS) ||
        EFF_FLAGGED(vict, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(vict, EFF_MESMERIZED)) {
        act("$N is unable to move!", false, ch, 0, vict, TO_CHAR);
        return;
    }

    /* is that a valid direction? */
    if (dir < 0 || !CH_DEST(ch, dir)) {
        char_printf(ch, "You can't lure someone in that direction!\n");
        return;
    }

    /* set a value for success */
    int trick;
    trick = random_number(0, 81);

    /* high int helps */
    trick -= skill_stat_bonus[GET_NATURAL_INT(ch)].small;

    /* Ventriloquate helps */
    if (affected_by_spell(ch, SPELL_VENTRILOQUATE))
        trick -= 10;

    /* being confused makes it harder to ignore */
    if (EFF_FLAGGED(vict, EFF_CONFUSION))
        trick -= 15;

    /* being insane make it harder to ignore */
    if (EFF_FLAGGED(vict, EFF_INSANITY))
        trick -= 15;

    /* being silenced makes it very hard to succeed */
    if (EFF_FLAGGED(ch, EFF_SILENCE))
        trick += 20;

    /* Must be undetectable */
    if (!CAN_SEE(vict, ch)) {
        /* Check the trick number */
        if (GET_SKILL(ch, SKILL_LURE) > trick) {
            /* Can't lure Sentinel mobs */
            if (MOB_FLAGGED(vict, MOB_SENTINEL)) {
                act("$N will not move from $S place!", false, ch, 0, vict, TO_CHAR);
                return;
            } else {
                for (k = vict->followers; k; k = k->next)
                    k->can_see_master = CAN_SEE(k->follower, vict);

                /* Success!  The mob can go that way */
                if (CAN_GO(vict, dir)) {
                    act("You cleverly lure $N away!", false, ch, 0, vict, TO_CHAR);
                    act("$n cleverly lures $N away!", false, ch, 0, vict, TO_NOTVICT);
                    perform_move(vict, dir, 1, false);

                    /* Mob should go, but there's a door in the way */
                } else {
                    act("You try to lure $N but the way is blocked!", false, ch, 0, vict, TO_CHAR);
                }
            }
            /* Trick number failed */
        } else {
            /* Not invisible and the victim isn't blind */
            if (!EFF_FLAGGED(ch, EFF_INVISIBLE) && EFF_FLAGGED(vict, EFF_BLIND)) {
                GET_HIDDENNESS(ch) = 0;
                act("$n accidentally catches $N's attention!", true, ch, 0, vict, TO_NOTVICT);
                act("You notice $n trying trick you into leaving the room and attack!", false, ch, 0, vict, TO_VICT);
                act("You accidentally grab $N's attention instead!", false, ch, 0, vict, TO_CHAR);
                attack(vict, ch);
                /* Can't see because invis, blind, or other, just fail */
            } else
                act("$N isn't distracted enough to leave.", false, ch, 0, vict, TO_CHAR);
        }
        /* Can be seen! */
    } else {
        /* Character wasn't invisible but seen, so hide failed */
        if (!(EFF_FLAGGED(ch, EFF_INVISIBLE)) ||
            EFF_FLAGGED(ch, EFF_INVISIBLE) && EFF_FLAGGED(vict, EFF_DETECT_INVIS)) {
            act("$n accidentally catches $N's attention!", true, ch, 0, vict, TO_NOTVICT);
            act("You notice $n trying trick you into leaving the room and attack!", false, ch, 0, vict, TO_VICT);
            act("You accidentally grab $N's attention instead!", false, ch, 0, vict, TO_CHAR);
        } else {
            act("$N can clearly see you and attacks!", false, ch, 0, vict, TO_CHAR);
        }
        attack(vict, ch);
    }

    improve_skill_offensively(ch, vict, SKILL_LURE);
    WAIT_STATE(ch, PULSE_VIOLENCE);
    return;
}

ACMD(do_rend) {
    CharData *vict;
    int percent, prob;
    effect eff;
    bool affected_by_armor_spells(CharData * vict, int skill = SKILL_REND);

    if (GET_SKILL(ch, SKILL_REND) == 0) {
        char_printf(ch, "You have no idea how to rend armor.\n");
        return;
    }

    one_argument(argument, arg);

    if (!arg || !*arg) {
        if (FIGHTING(ch)) {
            vict = FIGHTING(ch);
        } else {
            char_printf(ch, "Who's armor do you want to rend?\n");
            return;
        }
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, "Who's armor do you want to rend?\n");
        return;
    }

    if (vict == ch) {
        char_printf(ch, "You shouldn't rend your own defenses!\n");
        return;
    }

    if (!attack_ok(ch, vict, true))
        return;

    if (EFF_FLAGGED(vict, EFF_EXPOSED)) {
        act("You can't shred $S armor any more than it already is!", true, ch, 0, vict, TO_CHAR);
        return;
    }

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    /* Need to see whether this player is fighting already. Rend should not
       allow for the player to switch without a switch probability being
       calculated into the mix. */
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (FIGHTING(ch) && FIGHTING(ch) != vict && !switch_ok(ch))
        return;

    percent = ((10 - ((GET_AC(vict) + (monk_weight_penalty(vict) * 5)) / 10)) << 1) + random_number(1, 101);
    prob = (GET_SKILL(ch, SKILL_REND) + (skill_stat_bonus[GET_DEX(ch)].large / 2) + (skill_stat_bonus[GET_INT(ch)].large / 2));

    if (percent > prob) {
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        act("&9&bYou can't open a weakness on $N's armor!&0", false, ch, 0, vict, TO_CHAR);
        act("&9&b$n tries to rend $N's armor but can't make a mark.&0", false, ch, 0, vict, TO_NOTVICT);
        act("&9&b$n tries to rent your armor but can't make a mark.&0", false, ch, 0, vict, TO_VICT);
    } else {
        int i, counter;
        const int armor_spell[] = {SPELL_ARMOR,       SPELL_BARKSKIN,  SPELL_BONE_ARMOR, SPELL_DEMONSKIN,
                                   SPELL_GAIAS_CLOAK, SPELL_ICE_ARMOR, SPELL_MIRAGE,     0};

        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        if (affected_by_armor_spells(vict, SKILL_REND)) {
            for (i = 0, counter = 0; armor_spell[i]; i++) {
                if (affected_by_spell(vict, armor_spell[i])) {
                    effect_from_char(vict, armor_spell[i]);
                    act("&9&bYou rob $N of $S magical protection!&0", false, ch, 0, vict, TO_CHAR);
                    act("&9&b$n robs $N of $S magical protection!&0", false, ch, 0, vict, TO_NOTVICT);
                    act("&9&b$n robs you of your magical protection!&0", false, ch, 0, vict, TO_VICT);
                }
            }
        } else {
            act("&7&bYou shred $N's armor apart!&0", false, ch, 0, vict, TO_CHAR);
            act("&7&b$n rends $N's armor apart.&0", false, ch, 0, vict, TO_NOTVICT);
            act("&7&b$n rends your armor apart.&0", false, ch, 0, vict, TO_VICT);
            memset(&eff, 0, sizeof(eff));
            eff.type = SKILL_REND;
            eff.duration = (GET_SKILL(ch, SKILL_REND) / 10);
            eff.modifier = -1 - (GET_SKILL(ch, SKILL_REND) / 4) - (skill_stat_bonus[GET_DEX(ch)].large / 2) -
                           (skill_stat_bonus[GET_INT(ch)].large / 2);
            eff.location = APPLY_AC;
            SET_FLAG(eff.flags, EFF_EXPOSED);
            effect_to_char(vict, &eff);
        }
    }

    set_fighting(vict, ch, true);
    improve_skill_offensively(ch, vict, SKILL_REND);
}

ACMD(do_tripup) {
    CharData *vict = nullptr, *tch;
    int percent, prob, message;

    if (GET_RACE(ch) != RACE_HALFLING) {
        char_printf(ch, "Only halflings can pull that off!\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS)) {
            char_printf(ch, "It's just too dark!&0\n");
            return;
        }

        if (EFF_FLAGGED(ch, EFF_BLIND)) {
            char_printf(ch, "You can't see a thing!\n");
            return;
        }
    }

    one_argument(argument, arg);

    if (!*arg || !(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        vict = FIGHTING(ch);
        if (!vict || IN_ROOM(ch) != IN_ROOM(vict) || !CAN_SEE(ch, vict)) {
            char_printf(ch, "Trip up who?\n");
            return;
        }
    }

    if (vict == ch) {
        char_printf(ch, "How can you make yourself fall down?\n");
        return;
    }

    if (!FIGHTING(vict)) {
        char_printf(ch, "You can only trip things in combat with others!\n");
        return;
    }

    if (vict == ch->guarding) {
        act("You can't do that while you are guarding $M.", false, ch, 0, vict, TO_CHAR);
        return;
    }

    /* check for pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, true))
        return;
    vict = check_guard(ch, vict, false);
    if (!attack_ok(ch, vict, true))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, true);

    if (GET_POS(vict) <= POS_SITTING) {
        act("$E has already been knocked down.", false, ch, 0, vict, TO_CHAR);
        return;
    }

    if (GET_POS(vict) == POS_FLYING) {
        char_printf(ch, "You can't trip up things in the air!\n");
        return;
    }

    /* You can tripup as long as you're not the tank */
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (FIGHTING(tch) == ch) {
            if (FIGHTING(ch) == tch)
                act("$N's already attacking you!\n", false, ch, 0, tch, TO_CHAR);
            else if (FIGHTING(ch))
                act("You're too busy fighting $N to trip up anyone!", false, ch, 0, FIGHTING(ch), TO_CHAR);
            else
                act("$N is coming in for the attack - you cannot trip $M up now.", false, ch, 0, tch, TO_CHAR);
            return;
        }
    }

    prob = random_number(1, 100); /* tripup uses random num instead of skill */
    prob += GET_LEVEL(ch);
    prob += GET_HITROLL(ch) - monk_weight_penalty(ch);
    prob += skill_stat_bonus[GET_INT(ch)].small;
    prob += skill_stat_bonus[GET_DEX(ch)].small;
    percent = random_number(1, 101);
    percent += GET_SKILL(vict, SKILL_DODGE);
    percent += GET_LEVEL(vict);

    if (GET_LEVEL(vict) >= LVL_IMMORT)
        percent = prob + 1; /* insta-fail */

    if ((prob > percent || MOB_FLAGGED(vict, MOB_NOBASH)) &&
        (damage_evasion(vict, ch, 0, DAM_CRUSH) || MOB_FLAGGED(vict, MOB_ILLUSORY)))
        message = 1;

    if ((prob > percent || MOB_FLAGGED(vict, MOB_NOBASH)) && displaced(ch, vict))
        message = 2;

    if (message == 1 || message == 2) {
        act(EVASIONCLR "You slip right through $N&7&b!&0", false, ch, 0, vict, TO_CHAR);
        act(EVASIONCLR "$n" EVASIONCLR " slips right through $N" EVASIONCLR "!&0", false, ch, 0, vict, TO_NOTVICT);
        act(EVASIONCLR "$n" EVASIONCLR " slips right through you!&0", false, ch, 0, vict, TO_VICT);
        char_printf(ch, "You fall down!\n");
        act("$n falls down!", false, ch, 0, 0, TO_ROOM);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        set_fighting(vict, ch, false);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_ALERT;
        return;
    }

    /* NO BASH - you fail. */
    if (MOB_FLAGGED(vict, MOB_NOBASH)) {
        act("You scurry through $N's legs, but $E seems quite unmoved.", false, ch, 0, vict, TO_CHAR);
        act("$n scurries through $N's legs, who seems as stable as a rock!", false, ch, 0, vict, TO_NOTVICT);
        act("$n scurries through your legs, attempting to trip you up.", false, ch, 0, vict, TO_VICT);
        /* A pause... but you don't fall down. */
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        set_fighting(vict, ch, false);
        return;
    }

    /* Can affect targets up to four sizes larger */
    if (GET_SIZE(vict) - GET_SIZE(ch) > 4) {
        act("&7&bYou run right between $N's giant legs without any effect!&0", false, ch, 0, vict, TO_CHAR);
        act("&7&b$n harmlessly runs right between $N's giant legs.&0", false, ch, 0, vict, TO_NOTVICT);
        act("&7&b$n harmlessly runs between your towering legs like a little bug.&0", false, ch, 0, vict, TO_VICT);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        set_fighting(vict, ch, false);
        return;

        /* Can only affect targets bigger than yourself */
    } else if (GET_SIZE(ch) - GET_SIZE(vict) > 0) {
        char_printf(ch, "&7&bYou can't trip someone so small.&0\n");
        act("&7&b$n tries to trip up $N, but can't get under someone so small.&0", false, ch, 0, vict, TO_NOTVICT);
        act("&7&b$n tries to trip you up, but can't under your legs to do it.&0", false, ch, 0, vict, TO_VICT);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        set_fighting(vict, ch, false);
        return;
    }

    if (prob > percent) { /* Success! */
        WAIT_STATE(vict, PULSE_VIOLENCE * 3);
        act("&7&bYou tangle $N in $S own legs, bringing them crashing down!&0", false, ch, 0, vict, TO_CHAR);
        act("&7&b$n tangles $N in $S legs and trips $N.&0", false, ch, 0, vict, TO_NOTVICT);
        act("&7&b$n makes you get tangled in your own legs and you fall on your face!&0", false, ch, 0, vict, TO_VICT);
        set_fighting(ch, vict, false);
        if (GET_POS(vict) > POS_SITTING)
            alter_pos(vict, POS_SITTING, STANCE_ALERT);
    } else {
        act("You try to trip up $N but trip over $M instead.&0", false, ch, 0, vict, TO_CHAR);
        act("$n tries to trip up $N, but trips over $M instead.&0", false, ch, 0, vict, TO_NOTVICT);
        act("$n tries to trip you up, but trips over you instead.&0", false, ch, 0, vict, TO_VICT);
        /* damage comes before alter_pos here. If alter_pos came first, then if
         * the fight was started by this action, you might be in a sitting
         * position when the fight began, which would make you automatically
         * stand. We don't want that. */
        set_fighting(vict, ch, false);
        alter_pos(ch, POS_SITTING, STANCE_ALERT);
    }

    WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
}

ACMD(do_roundhouse) {
    CharData *mob, *next_mob, *orig_target;
    byte percent;
    bool kick_all = false, realvictims = false, success = false;

    if (!ch || ch->in_room == NOWHERE)
        return;

    if (!GET_SKILL(ch, SKILL_ROUNDHOUSE)) {
        char_printf(ch, "You don't know how to.\n");
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        char_printf(ch, "You feel ashamed trying to disturb the peace of this room.\n");
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
        char_printf(ch, "Sorry, it's too cramped here for nasty maneuvers!\n");
        return;
    }

    if (FIGHTING(ch))
        orig_target = FIGHTING(ch);

    /* Find out whether to hit "all" or just aggressive monsters */
    one_argument(argument, arg);
    if (!strcasecmp(arg, "all"))
        kick_all = 1;

    /* Hit all aggressive monsters in room */

    percent = random_number(1, 131);
    WAIT_STATE(ch, PULSE_VIOLENCE);

    act("$n unleashes a flying spin kick at everything nearby!", false, ch, 0, 0, TO_NOTVICT);
    char_printf(ch, "You unleash a flying spin kick at everything nearby!\n");
    if (GET_SKILL(ch, SKILL_ROUNDHOUSE) >= percent)
        success = true;

    for (mob = world[ch->in_room].people; mob; mob = next_mob) {
        next_mob = mob->next_in_room;

        /* Basic area attack check */
        if (!area_attack_target(ch, mob))
            continue;

        /* If I just entered plain "hitall", don't attack bystanders who aren't
         * aggro to me */
        if (!battling_my_group(ch, mob) && !kick_all && !is_aggr_to(mob, ch))
            continue;

        if (!MOB_FLAGGED(mob, MOB_ILLUSORY))
            realvictims = true;

        if (success) {
            if (mob != orig_target)
                do_kick(ch, GET_NAME(mob), 0, SKILL_ROUNDHOUSE);
        }
    }

    if (success) {
        if (orig_target)
            do_kick(ch, GET_NAME(orig_target), 0, SKILL_ROUNDHOUSE);
    }

    if (realvictims)
        improve_skill(ch, SKILL_ROUNDHOUSE);
}


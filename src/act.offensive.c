/***************************************************************************
 *   File: act.offensive.c                               Part of FieryMUD  *
 *  Usage: player-level commands of an offensive nature                    *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "casting.h"
#include "chars.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "cooldowns.h"
#include "damage.h"
#include "db.h"
#include "directions.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "magic.h"
#include "math.h"
#include "movement.h"
#include "races.h"
#include "regen.h"
#include "screen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* extern variables */
extern int pk_allowed;
extern int summon_allowed;
extern int charm_allowed;
extern int sleep_allowed;
extern int damage_amounts;

/* extern functions */
ACMD(do_return);
struct char_data *check_guard(struct char_data *ch, struct char_data *victim, int gag_output);
bool is_grouped(struct char_data *ch, struct char_data *tch);
void quickdeath(struct char_data *victim, struct char_data *ch);
void abort_casting(struct char_data *ch);
void appear(struct char_data *ch);

void aggro_lose_spells(struct char_data *ch) {
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

bool switch_ok(struct char_data *ch) {
    if (GET_SKILL(ch, SKILL_SWITCH) <= 0) {
        act("You are already busy fighting with $N.", FALSE, ch, 0, FIGHTING(ch), TO_CHAR);
        return FALSE;
    }

    if (number(1, 101) > GET_SKILL(ch, SKILL_SWITCH)) {
        act("&8$n tries to switch opponents, but becomes confused!&0", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char("&8You try to switch opponents and become confused.&0\r\n", ch);
        stop_fighting(ch);
        improve_skill_offensively(ch, FIGHTING(ch), SKILL_SWITCH);
        return FALSE;
    }

    stop_fighting(ch);
    act("&8$n switches opponents!&0", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("&8You switch opponents!&0\r\n", ch);
    improve_skill_offensively(ch, FIGHTING(ch), SKILL_SWITCH);
    return TRUE;
}

const struct breath_type {
    const char *name;
    const int spell;
    char *to_char;
    char *to_room;
} breath_info[] = {
    {"fire", SPELL_FIRE_BREATH, "&1You snort and &bf&3i&7r&1e&0&1 shoots out of your nostrils!&0\r\n",
     "&1$n&1 snorts and a gout of &bf&3i&7r&1e&0&1 shoots out of $s "
     "nostrils!&0"},
    {"gas", SPELL_GAS_BREATH,
     "&2You heave and a &bnoxious gas&0&2 rolls rapidly out of your "
     "nostrils!&0\r\n",
     "&2$n&2 rumbles and a &bnoxious gas&0&2 rolls out of $s nostrils!&0"},
    {"frost", SPELL_FROST_BREATH,
     "&7&bYou shiver as a shaft of &0&4f&br&7o&4s&0&4t&7&b leaps from your "
     "mouth!&0\r\n",
     "&7&b$n&7&b shivers as a shaft of &0&4f&br&7o&4s&0&4t&7&b leaps from $s "
     "mouth!&0"},
    {"acid", SPELL_ACID_BREATH,
     "&9&bYou stomach heaves as a wash of &2&ba&0&2ci&bd&9 leaps from your "
     "mouth!&0\r\n",
     "&9&b$n&9&b looks pained as a wash of &2&ba&0&2ci&2&bd&9 leaps from $s "
     "mouth!&0"},
    {"lightning", SPELL_LIGHTNING_BREATH, "&4You open your mouth and bolts of &blightning&0&4 shoot out!&0\r\n",
     "&4$n&4 opens $s mouth and bolts of &blightning&0&4 shoot out!&0"},
    {NULL, 0, NULL, NULL},
};

struct char_data *random_attack_target(struct char_data *ch, struct char_data *target, bool verbose) {
    struct char_data *chosen = NULL;
    struct char_data *i;
    int count = 0;

    for (i = world[ch->in_room].people; i; i = i->next_in_room) {
        if (i != ch && CAN_SEE(ch, i) && attack_ok(ch, i, FALSE)) {
            if (chosen == NULL || number(0, count) == 0)
                chosen = i;
            count++;
        }
    }

    if (!chosen)
        chosen = target;

    if (verbose && chosen != target) {
        act("&5You got confused and attacked $N&0&5 instead!&0", FALSE, ch, 0, chosen, TO_CHAR);
        act("&5$n&0&5 got confused and attacked $N&0&5!&0", TRUE, ch, 0, chosen, TO_NOTVICT);
        act("&5$n&0&5 gets confused and attacks YOU!&0", TRUE, ch, 0, chosen, TO_VICT);
    }

    return chosen;
}

ACMD(do_breathe) {
    ACMD(do_action);
    struct char_data *tch, *next_tch;
    int type;
    bool realvictims = FALSE;
    int breath_energy;

    if (!ch || ch->in_room == NOWHERE)
        return;

    /* Don't allow reanimated undead to do this - justification:
     * you need actual life to generate poison gas! */
    /*
    if (GET_SKILL(ch, SKILL_BREATHE) < 1 || EFF_FLAGGED(ch, EFF_CHARM)) {
        send_to_char("You huff and puff but to no avail.\r\n", ch);
        act("$n huffs and puffs but to no avail.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }
    */
    if  (EFF_FLAGGED(ch, EFF_CHARM) || (GET_SKILL(ch, SKILL_BREATHE_GAS) < 1 && 
        GET_SKILL(ch, SKILL_BREATHE_FIRE) < 1 && GET_SKILL(ch, SKILL_BREATHE_FROST) < 1 
        && GET_SKILL(ch, SKILL_BREATHE_ACID) < 1 && GET_SKILL(ch, SKILL_BREATHE_LIGHTNING) < 1)) {
        send_to_char("You huff and puff but to no avail.\r\n", ch);
        act("$n huffs and puffs but to no avail.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }
    one_argument(argument, arg);

    for (type = 0; breath_info[type].name; ++type)
        if (is_abbrev(arg, breath_info[type].name))
            break;

    if (!breath_info[type].name) {
        send_to_char("Usage: breathe <fire / gas / frost / acid / lightning>\r\n", ch);
        return;
    }
    
    if ((GET_SKILL(ch, SKILL_BREATHE_FIRE) > 0) && (breath_info[type].name == "fire")) {
        breath_energy = SKILL_BREATHE_FIRE;
    }
    else if ((GET_SKILL(ch, SKILL_BREATHE_FROST) > 0) && (breath_info[type].name == "frost")) {
        breath_energy = SKILL_BREATHE_FROST;
    }
    else if ((GET_SKILL(ch, SKILL_BREATHE_ACID) > 0) && (breath_info[type].name == "acid")) {
        breath_energy = SKILL_BREATHE_ACID;
    }
    else if ((GET_SKILL(ch, SKILL_BREATHE_GAS) > 0) && (breath_info[type].name == "gas")) {
        breath_energy = SKILL_BREATHE_GAS;
    }
    else if ((GET_SKILL(ch, SKILL_BREATHE_LIGHTNING) > 0) && (breath_info[type].name == "lightning")) {
        breath_energy = SKILL_BREATHE_LIGHTNING;
    }
    else {
        send_to_char("You cannot breathe that kind of energy!\r\n", ch);
        return; 
    }

    if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        if (!GET_COOLDOWN(ch, CD_BREATHE)) {
            if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC)) {
                SET_COOLDOWN(ch, CD_BREATHE, 4 MUD_HR);
            }
        } else {
            cprintf(ch, "You will have rebuilt your energy in %d seconds.\r\n", (GET_COOLDOWN(ch, CD_BREATHE) / 10));
            return;
        }
    }

    send_to_char(breath_info[type].to_char, ch);
    act(breath_info[type].to_room, FALSE, ch, 0, 0, TO_ROOM);

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
        next_tch = tch->next_in_room;

        if (tch == ch)
            continue;
        if (is_grouped(ch, tch))
            continue;
        if (!mass_attack_ok(ch, tch, FALSE))
            continue;
        if (PRF_FLAGGED(tch, PRF_NOHASSLE))
            continue;
        /* Mobs don't hit other mobs, unless they're pets */
        if (!IS_PC(ch) && !IS_PC(tch) && !PLAYERALLY(ch) && !PLAYERALLY(tch))
            continue;
        call_magic(ch, tch, 0, breath_info[type].spell, GET_LEVEL(ch), CAST_BREATH);
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realvictims = TRUE;
    }
    /*
    if (realvictims)
        improve_skill(ch, SKILL_BREATHE);

    */
    if (realvictims) {
        improve_skill(ch, breath_energy);
    }
    
    if (GET_LEVEL(ch) < LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_roar) {
    struct char_data *tch, *next_tch;
    bool realvictims = FALSE;
    ACMD(do_flee);
    ACMD(do_action);

    if (!ch || ch->in_room == NOWHERE)
        return;

    if (subcmd == SCMD_HOWL) {
        if (!GET_SKILL(ch, SKILL_BATTLE_HOWL) || !EFF_FLAGGED(ch, EFF_SPIRIT_WOLF) || !EFF_FLAGGED(ch, EFF_BERSERK)) {
            if (SUN(IN_ROOM(ch)) == SUN_DARK && CH_OUTSIDE(ch)) {
                send_to_char("You form an O with your mouth and howl at the moon.\r\n", ch);
                act("$n starts howling at the moon.   Eerie.", FALSE, ch, 0, 0, TO_ROOM);
            } else {
                send_to_char("You howl madly, making a fool of yourself.\r\n", ch);
                act("$n howls madly, looking like a fool.", FALSE, ch, 0, 0, TO_ROOM);
            }
            return;
        }
    } else if (!GET_SKILL(ch, SKILL_ROAR) || EFF_FLAGGED(ch, EFF_CHARM)) {
        do_action(ch, argument, cmd, subcmd);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        act("$n opens $s mouth wide and lets out a little cough.", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char("You take a deep breath and release a vicious cough!\r\n", ch);
        return;
    }

    if (subcmd == SCMD_HOWL) {
        act("$n opens his mouth and a &1&8demonic &0&1howl&0 echoes out!", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char("You let out a demonic battle howl, striking fear into your enemies!\r\n",
                     ch);
    } else {
        act("&9&b$n&9&b makes your soul quake with a vicious "
            "&1ROOOOOAAAAAARRRRRR!&0",
            FALSE, ch, 0, 0, TO_ROOM);
        send_to_char("&9&bYou take a deep breath and release a vicious &1ROOOOOAAAAARRRRRR!&0\r\n", ch);
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
        if (!attack_ok(ch, tch, FALSE))
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
            send_to_char("Your holy protection strengthens your resolve against $n's roar!\r\n", tch);
            continue;
        }
        if (EFF_FLAGGED(tch, EFF_PROTECT_GOOD) && GET_ALIGNMENT(ch) <= 500) {
            send_to_char("Your unholy protection strengthens your resolve against $n's roar!\r\n", tch);
            continue;
        }
        mag_affect(GET_LEVEL(ch), ch, tch, SPELL_FEAR, SAVING_PARA, CAST_BREATH);

        if (SLEEPING(tch)) {
            if (number(0, 1)) {
                sprintf(buf, "A loud %s jolts you from your slumber!\r\n",
                        subcmd == SCMD_HOWL ? "OOOOAAAOAOOHHH howl" : "ROAAARRRRRR");
                send_to_char(buf, tch);
                act("$n jumps up dazedly, awakened by the noise!", TRUE, tch, 0, 0, TO_ROOM);
                GET_POS(tch) = POS_SITTING;
                GET_STANCE(tch) = STANCE_ALERT;
                WAIT_STATE(tch, PULSE_VIOLENCE);
            }
        } else if (GET_DEX(tch) - 15 < number(0, 100) && GET_POS(tch) >= POS_STANDING) {
            send_to_char("In your panicked rush to flee, you trip!\r\n", tch);
            act("In a panicked rush to flee, $n trips!", FALSE, tch, 0, 0, TO_ROOM);
            GET_POS(tch) = POS_SITTING;
            GET_STANCE(tch) = STANCE_ALERT;
            WAIT_STATE(tch, PULSE_VIOLENCE);
        } else
            do_flee(tch, NULL, 0, 0);
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realvictims = TRUE;
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
    struct char_data *tch, *next_tch;
    bool realvictims = FALSE;
    ACMD(do_flee);

    if (!ch || ch->in_room == NOWHERE)
        return;

    if (GET_SKILL(ch, SKILL_SWEEP) < 1 || EFF_FLAGGED(ch, EFF_CHARM)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    act("&2$n&2 sweeps with $s enormous tail!&0", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("&2You sweep with your enormous tail!&0\r\n", ch);

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
        if (GET_VIEWED_DEX(tch) - 15 > number(0, 100) || GET_STANCE(tch) < STANCE_FIGHTING)
            continue;
        if (!attack_ok(ch, tch, FALSE))
            continue;
        if (PRF_FLAGGED(tch, PRF_NOHASSLE))
            continue;
        if (damage_evasion(tch, ch, 0, DAM_CRUSH)) {
            act(EVASIONCLR "$N's" EVASIONCLR " tail passes right through you.&0", FALSE, tch, 0, ch, TO_CHAR);
            act(EVASIONCLR "$N's" EVASIONCLR " tail passes harmlessly through $n.&0", FALSE, tch, 0, 0, TO_ROOM);
            set_fighting(tch, ch, TRUE);
            continue;
        }

        act("&3You are slammed to the ground by $N's&3 tail!&0", FALSE, tch, 0, ch, TO_CHAR);
        act("&3$n&3 is slammed to the ground by a mighty tail sweep!&0", FALSE, tch, 0, 0, TO_ROOM);
        GET_POS(tch) = POS_SITTING;
        GET_STANCE(tch) = STANCE_ALERT;
        WAIT_STATE(tch, PULSE_VIOLENCE);
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realvictims = TRUE;
    }

    if (realvictims)
        improve_skill(ch, SKILL_SWEEP);
    if (GET_LEVEL(ch) < LVL_IMMORT)
        WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_assist) {
    struct char_data *helpee, *opponent;

    one_argument(argument, arg);

    if (FIGHTING(ch))
        send_to_char("You're already fighting!\r\n", ch);
    else if (!*arg)
        send_to_char("Whom do you wish to assist?\r\n", ch);
    else if (!(helpee = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
        send_to_char(NOPERSON, ch);
    else if (helpee == ch)
        send_to_char("Usually, you assist someone else.\r\n", ch);
    else {
        for (opponent = world[ch->in_room].people; opponent && (FIGHTING(opponent) != helpee);
             opponent = opponent->next_in_room)
            ;

        if (!opponent)
            act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
        else if (!CAN_SEE(ch, opponent))
            act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
        else if (attack_ok(ch, opponent, TRUE)) {
            act("You assist $N&0 heroically.", FALSE, ch, 0, helpee, TO_CHAR);
            act("$n&0 assists you!", 0, ch, 0, helpee, TO_VICT);
            act("$n&0 heroically assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
            if (CONFUSED(ch))
                opponent = random_attack_target(ch, opponent, TRUE);
            attack(ch, opponent);
        }
    }
}

ACMD(do_disengage) {
    ACMD(do_abort);

    if (CASTING(ch)) {
        do_abort(ch, argument, 0, 0);
        return;
    }

    if (!FIGHTING(ch)) {
        send_to_char("You are not fighting anyone.\r\n", ch);
        return;
    }

    if (FIGHTING(FIGHTING(ch)) == ch) {
        send_to_char("No way! You are fighting for your life!\r\n", ch);
        return;
    }

    stop_fighting(ch);
    send_to_char("You disengage from combat.\r\n", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

ACMD(do_hit) {
    struct char_data *vict;

    one_argument(argument, arg);

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS))
        send_to_char("&8It is just too dark!&0\r\n", ch);
    else if (EFF_FLAGGED(ch, EFF_BLIND))
        send_to_char("You can't see a thing!\r\n", ch);
    else if (!*arg)
        send_to_char("Hit who?\r\n", ch);
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
        send_to_char("They don't seem to be here.\r\n", ch);
    else if (vict == ch) {
        send_to_char("You hit yourself...OUCH!.\r\n", ch);
        act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
    } else if (EFF_FLAGGED(ch, EFF_CHARM) && (ch->master == vict))
        act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
    else if (attack_ok(ch, vict, TRUE)) {
        if (vict == FIGHTING(ch)) {
            send_to_char("&7You're doing the best you can!&0\r\n", ch);
            return;
        } else if (!FIGHTING(ch) || switch_ok(ch)) {
            if (CONFUSED(ch))
                vict = random_attack_target(ch, vict, TRUE);
            attack(ch, vict);
        }
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
}

ACMD(do_kill) {
    struct char_data *vict;

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS)) {
        send_to_char("&8It is just too damn dark!&0\r\n", ch);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        send_to_char("You feel ashamed trying to disturb the peace of this room.\r\n", ch);
        return;
    }

    if ((GET_LEVEL(ch) < LVL_GOD) || IS_NPC(ch)) {
        do_hit(ch, argument, cmd, subcmd);
        return;
    }
    one_argument(argument, arg);

    if (!*arg) {
        send_to_char("Kill who?\r\n", ch);
    } else {
        if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
            send_to_char("They aren't here.\r\n", ch);
        else if (ch == vict)
            send_to_char("Your mother would be so sad.. :(\r\n", ch);
        else if (GET_LEVEL(vict) == LVL_IMPL)
            send_to_char("&1You dare NOT do that!&0\r\n", ch);
        else {
            act("You chop $M to pieces!   Ah!   The blood!", FALSE, ch, 0, vict, TO_CHAR);
            act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
            act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
            die(vict, ch);
        }
    }
}

/* I've made this a higher end functionality of backstab instead of some 'tarded
 * skill that rarely works - RLS 02/12/05*/
bool instantkill(struct char_data *ch, struct char_data *victim) {
    int chance = 0;

    if (!victim || !ch || ch == victim || ch == NULL || victim == NULL)
        return FALSE;

    /* No more instant kills until the timer runs out */
    if (GET_COOLDOWN(ch, CD_INSTANT_KILL))
        return FALSE;

    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_INSTANT_KILL) || DECEASED(victim) || (ch->in_room != victim->in_room) ||
        (GET_LEVEL(victim) > LVL_IMMORT))
        return FALSE;

    improve_skill_offensively(ch, victim, SKILL_INSTANT_KILL);
    if (number(1, 101) > GET_SKILL(ch, SKILL_INSTANT_KILL))
        return FALSE;

    /*chance is now checking DEX and shouldn't happen as often */
    chance = 1000 - (GET_SKILL(ch, SKILL_INSTANT_KILL) - (100 - GET_DEX(ch)) - (GET_LEVEL(victim) * 10)) / 10;

    if (!AWAKE(victim))
        chance = 0; /* der.. can you say coup de grace? */

    if (number(1, 1000) >= chance) {
        quickdeath(victim, ch);
        SET_COOLDOWN(ch, CD_INSTANT_KILL, (120 - (GET_LEVEL(ch))) * PULSE_COOLDOWN);
        return TRUE;
    }
    return FALSE;
}

void slow_death(struct char_data *victim) {
    if (!victim) {
        mudlog("Attempting to use slow_death on a NULL character!", BRF, LVL_GOD, TRUE);
        log("Attempting to use slow_death on a NULL character!");
        return;
    }

    if (victim->attackers) {
        /* Someone is fighting this person, so don't die just yet! */
        GET_HIT(victim) = HIT_MORTALLYW;
        hp_pos_check(victim, NULL, 0);
        return;
    }

    act("&8With a soft groan, $n slips off into the cold sleep of death.&0", TRUE, victim, 0, 0, TO_ROOM);
    act("&8$n is dead!   R.I.P.&0", TRUE, victim, 0, 0, TO_ROOM);
    if (AWAKE(victim)) {
        act("&8You feel yourself slipping away and falling into the abyss.&0", FALSE, victim, 0, 0, TO_CHAR);
        send_to_char("&0&8Your life fades away ....\r\n", victim);
    }

    die(victim, NULL);
}

void quickdeath(struct char_data *victim, struct char_data *ch) {
    if (GET_LEVEL(victim) >= LVL_IMMORT || (ch && MOB_FLAGGED(ch, MOB_ILLUSORY)))
        return;

    send_to_char("You deliver the killing blow.\r\n", ch);
    act("$n's strike upon $N is faster than the eye can see.", TRUE, ch, 0, victim, TO_NOTVICT);
    send_to_char("You feel a sharp sting, and all goes black.\r\n", victim);

    hurt_char(victim, ch, GET_MAX_HIT(victim) + 20, FALSE);
}

ACMD(do_backstab) {
    struct char_data *vict, *tch;
    struct effect eff;
    int percent, prob, percent2, prob2;
    struct obj_data *weapon;

    if (GET_COOLDOWN(ch, CD_BACKSTAB)) {
        send_to_char("Give yourself a chance to get back into position!\r\n", ch);
        return;
    }

    if (GET_SKILL(ch, SKILL_BACKSTAB) < 1) {
        send_to_char("You don't know how.\r\n", ch);
        return;
    }

    one_argument(argument, buf);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) {
        if (!FIGHTING(ch)) {
            send_to_char("Backstab who?\r\n", ch);
            return;
        } else {
            vict = FIGHTING(ch);
        }
    }

    if (vict == ch) {
        send_to_char("How can you sneak up on yourself?\r\n", ch);
        return;
    }

    if (!attack_ok(ch, vict, TRUE))
        return;

    /* You can backstab as long as you're not the tank */
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
        if (FIGHTING(tch) == ch) {
            if (FIGHTING(ch) == tch)
                act("$N's facing the wrong way!\r\n", FALSE, ch, 0, tch, TO_CHAR);
            else if (FIGHTING(ch))
                act("You're too busy fighting $N to backstab anyone!", FALSE, ch, 0, FIGHTING(ch), TO_CHAR);
            else
                act("$N is coming in for the attack - you cannot backstab $M now.", FALSE, ch, 0, tch, TO_CHAR);
            return;
        }
    }

    if (EFF_FLAGGED(ch, EFF_BLIND)) {
        send_to_char("You can't see a thing!\r\n", ch);
        return;
    }

    /* Got a weapon? Any weapon? */
    weapon = GET_EQ(ch, WEAR_WIELD);
    if (!weapon)
        weapon = GET_EQ(ch, WEAR_WIELD2);
    if (!weapon)
        weapon = GET_EQ(ch, WEAR_2HWIELD);

    if (!weapon) {
        send_to_char("Backstab with what, your fist?\r\n", ch);
        return;
    }

    /* If wielding something unsuitable in first hand, use weapon in second hand
     */
    if (!IS_WEAPON_PIERCING(weapon) && GET_EQ(ch, WEAR_WIELD2))
        weapon = GET_EQ(ch, WEAR_WIELD2);

    if (!IS_WEAPON_PIERCING(weapon)) {
        send_to_char("Piercing weapons must be used to backstab.\r\n", ch);
        return;
    }

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    if (damage_evasion(vict, ch, weapon, DAM_PIERCE)) {
        damage_evasion_message(ch, vict, weapon, DAM_PIERCE);
        set_fighting(vict, ch, TRUE);
        return;
    }

    /* If the mob is flagged aware, is not sleeping, and is not currently
       fighting, then you can not backstab the mob */
    if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict) && !FIGHTING(vict) && !EFF_FLAGGED(vict, EFF_MINOR_PARALYSIS) &&
        !EFF_FLAGGED(vict, EFF_MAJOR_PARALYSIS)) {
        act("You notice $N sneaking up on you!", FALSE, vict, 0, ch, TO_CHAR);
        act("$e notices you sneaking up on $m!", FALSE, vict, 0, ch, TO_VICT);
        act("$n notices $N sneaking up on $m!", FALSE, vict, 0, ch, TO_NOTVICT);
        attack(vict, ch);
        return;
    }

    /* 50% chance the mob is aware, even in combat */
    if (MOB_FLAGGED(vict, MOB_AWARE) && CAN_SEE(ch, vict) && !EFF_FLAGGED(vict, EFF_MINOR_PARALYSIS) &&
        !EFF_FLAGGED(vict, EFF_MAJOR_PARALYSIS) && FIGHTING(vict) &&
        number(1, GET_LEVEL(vict)) > GET_SKILL(ch, SKILL_BACKSTAB) / 2) {
        act("You notice $N trying to sneaking up on you!", FALSE, vict, 0, ch, TO_CHAR);
        act("You failed - $e notices you sneaking up on $m!", FALSE, vict, 0, ch, TO_VICT);
        act("$n notices $N trying to sneak up on $m!", FALSE, vict, 0, ch, TO_NOTVICT);

        attack(vict, ch);

        WAIT_STATE(ch, PULSE_VIOLENCE);
        /* 6 seconds == 1.5 combat rounds, 12 seconds == 3 combat rounds. */
        if (GET_CLASS(ch) == CLASS_ILLUSIONIST {
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

    percent = number(1, 101); /* 101% is a complete failure */

    if (EFF_FLAGGED(vict, EFF_AWARE) && AWAKE(vict) && !FIGHTING(vict))
        percent = 150; /*silent failure */

    if (EFF_FLAGGED(vict, EFF_AWARE) && FIGHTING(vict))
        percent += number(1, 10); /* It's a little harder to backstab a mob you've
                                     already backstabed */

    prob = MIN(97, GET_SKILL(ch, SKILL_BACKSTAB) - GET_LEVEL(vict) + 90);

    if (!CAN_SEE(vict, ch))
        prob += GET_SKILL(ch, SKILL_BACKSTAB) / 2; /* add blindfighting skill */

    if (instantkill(ch, vict))
        return;

    if (AWAKE(vict) && (percent > prob)) {
        /* Backstab failed */
        act("$N tried to backstab you, but you moved away in time!", FALSE, vict, 0, ch, TO_CHAR);
        act("You failed to backstab $n, but manage to take a swing at $m!", FALSE, vict, 0, ch, TO_VICT);
        act("$N tried to backstab $n, but missed and managed to take a swing "
            "instead!",
            FALSE, vict, 0, ch, TO_NOTVICT);
        hit(ch, vict, weapon == GET_EQ(ch, WEAR_WIELD2) ? SKILL_DUAL_WIELD : TYPE_UNDEFINED);
    } else {
        /* Backstab succeeded */
        hit(ch, vict, weapon == GET_EQ(ch, WEAR_WIELD2) ? SKILL_2BACK : SKILL_BACKSTAB);
    }
    improve_skill_offensively(ch, vict, SKILL_BACKSTAB);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    /* 6 seconds == 1.5 combat rounds, 12 seconds == 3 combat rounds. */
    if (GET_CLASS(ch) == CLASS_ILLUSIONIST {
        SET_COOLDOWN(ch, CD_BACKSTAB, 12 * PULSE_COOLDOWN);
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

    percent2 = number(1, 101); /* 101% is a complete failure */

    if (EFF_FLAGGED(vict, EFF_AWARE) && AWAKE(vict) && !FIGHTING(vict)) {
        percent2 = 150; /*silent failure */
    }

    if (EFF_FLAGGED(vict, EFF_AWARE) && FIGHTING(vict))
        percent2 += number(1, 10); /* It's a little harder to backstab a mob you've
                                      already backstabed */

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
        send_to_char("You cannot flee yet!\r\n", ch);
        return;
    }

    if (FIGHTING(ch) && EFF_FLAGGED(ch, EFF_BERSERK)) {
        send_to_char("You're too angry to leave this fight!\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS)) {
        cprintf(ch, "You can't move!\r\n");
        return;
    }

    switch (GET_STANCE(ch)) {
    case STANCE_DEAD:
    case STANCE_MORT:
    case STANCE_INCAP:
    case STANCE_STUNNED:
        send_to_char("It's a bit too late for that.\r\n", ch);
        break;
    case STANCE_SLEEPING:
        send_to_char("You dream of fleeing!\r\n", ch);
        break;
    default:
        switch (GET_POS(ch)) {
        case POS_PRONE:
        case POS_SITTING:
        case POS_KNEELING:
            abort_casting(ch);
            act("Looking panicked, $n scrambles madly to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char("You scramble madly to your feet!\r\n", ch);
            GET_POS(ch) = POS_STANDING;
            GET_STANCE(ch) = STANCE_ALERT;
            if (IS_NPC(ch))
                WAIT_STATE(ch, PULSE_VIOLENCE * 2);
            break;
        default:
            if (IS_CORNERED(ch)) {
                act("$n tries to flee, but is unable to escape from $N!", TRUE, ch, 0, ch->cornered_by, TO_NOTVICT);
                act("$n tries to flee, but is unable to escape from you!", TRUE, ch, 0, ch->cornered_by, TO_VICT);
                act("PANIC!   You couldn't escape from $N!", TRUE, ch, 0, ch->cornered_by, TO_CHAR);
                return;
            }
            for (i = 0; i < 6; i++) {                 /* Make 6 attempts */
                attempt = number(0, NUM_OF_DIRS - 1); /* Select a random direction */

                if (CAN_GO(ch, attempt) && !ROOM_FLAGGED(CH_NDEST(ch, attempt), ROOM_DEATH)) {
                    abort_casting(ch);
                    act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
                    if (do_simple_move(ch, attempt, TRUE)) {
                        sprintf(buf, "&0You panic and flee %s!&0\r\n", dirs[attempt]);
                        send_to_char(buf, ch);
                    } else
                        act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
                    return;
                }
            }
            /* All 6 attempts failed! */
            act("$n tries to flee, but PANICS instead!", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char("PANIC!   You couldn't escape!\r\n", ch);
        }
    }
}

ACMD(do_retreat) {
    int dir, to_room;
    struct char_data *vict;

    if (!ch || !argument)
        return;

    if (!GET_SKILL(ch, SKILL_RETREAT)) {
        send_to_char("Try flee instead!\r\n", ch);
        return;
    }

    if (!FIGHTING(ch)) {
        send_to_char("You're not fighting anyone!\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char("Retreat where!?\r\n", ch);
        return;
    }

    dir = searchblock(arg, dirs, FALSE);

    if (dir < 0 || !CH_DEST(ch, dir)) {
        send_to_char("You can't retreat that way!\r\n", ch);
        return;
    }

    /*
       for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
       if (FIGHTING(tch) == ch) {
       send_to_char("&8You cannot retreat while tanking!&0\r\n", ch);
       return;
       }
     */

    vict = FIGHTING(ch);

    /* Successful retreat? */
    if (GET_SKILL(ch, SKILL_RETREAT) > number(0, 81) && CAN_GO(ch, dir) &&
        !ROOM_FLAGGED(CH_NDEST(ch, dir), ROOM_DEATH) && do_simple_move(ch, dir, TRUE)) {

        /* Send message back to original room. */
        sprintf(buf, "$n carefully retreats from combat, leaving %s.", dirs[dir]);
        to_room = ch->in_room;
        ch->in_room = vict->in_room;
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
        ch->in_room = to_room;

        sprintf(buf, "\r\nYou skillfully retreat %s.\r\n", dirs[dir]);
        send_to_char(buf, ch);
    }
    /* If fighting a mob that can switch, maybe get attacked. */
    else if (IS_NPC(FIGHTING(ch)) && FIGHTING(FIGHTING(ch)) != ch && GET_SKILL(FIGHTING(ch), SKILL_SWITCH) &&
             GET_SKILL(FIGHTING(ch), SKILL_SWITCH) > number(1, 101)) {
        stop_fighting(FIGHTING(ch));
        act("$n fails to retreat, catching $N's attention!", TRUE, ch, 0, FIGHTING(ch), TO_NOTVICT);
        act("You notice $n trying to escape and attack $m!", FALSE, ch, 0, FIGHTING(ch), TO_VICT);
        send_to_char("You fail to retreat, and catch the attention of your opponent!\r\n", ch);
        attack(FIGHTING(ch), ch);
    } else {
        act("$n stumbles and trips as $e fails to retreat!", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You stumble and trip as you try to retreat!\r\n", ch);
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
    struct char_data *tch;
    struct follow_type *k, *next_k;
    bool realopponents = FALSE;

    if (!ch || !argument)
        return;

    if (!GET_SKILL(ch, SKILL_GROUP_RETREAT)) {
        send_to_char("Try flee instead!\r\n", ch);
        return;
    }

    if (!FIGHTING(ch)) {
        send_to_char("You're not fighting anyone!\r\n", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (!*arg) {
        send_to_char("Retreat where!?\r\n", ch);
        return;
    }

    dir = searchblock(arg, dirs, FALSE);

    if (dir < 0 || !CH_DEST(ch, dir)) {
        send_to_char("You can't retreat that way!\r\n", ch);
        return;
    }

    was_in = IN_ROOM(ch);

    opponents = 0;
    for (tch = ch->attackers; tch; tch = tch->next_attacker) {
        ++opponents;
        if (!MOB_FLAGGED(tch, MOB_ILLUSORY))
            realopponents = TRUE;
    }

    /*
     * Can the followers see the leader before it leaves?
     */
    for (k = ch->followers; k; k = k->next)
        k->can_see_master = CAN_SEE(k->follower, ch);

    if (!opponents)
        send_to_char("You must be tanking to successfully lead your group in retreat!\r\n", ch);
    else if (GET_SKILL(ch, SKILL_GROUP_RETREAT) < opponents * number(20, 24))
        send_to_char("There are too many opponents to retreat from!\r\n", ch);

    /* Successful retreat? */
    else if (GET_SKILL(ch, SKILL_GROUP_RETREAT) > number(0, 81) && !ROOM_FLAGGED(CH_NDEST(ch, dir), ROOM_DEATH) &&
             CAN_GO(ch, dir) && do_simple_move(ch, dir, TRUE)) {
        /* Echo line back to the original room. */
        sprintf(buf, "$n carefully retreats from combat, leading $s group %s.", dirs[dir]);

        to_room = ch->in_room;
        ch->in_room = was_in;
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
        ch->in_room = to_room;
        sprintf(buf, "\r\nYou skillfully lead your group %s.\r\n", dirs[dir]);
        send_to_char(buf, ch);

        for (k = ch->followers; k; k = next_k) {
            next_k = k->next;
            if (k->follower->in_room == was_in && GET_STANCE(k->follower) >= STANCE_ALERT && k->can_see_master) {
                abort_casting(k->follower);
                sprintf(buf, "You follow $N %s.", dirs[dir]);
                act(buf, FALSE, k->follower, 0, ch, TO_CHAR);
                perform_move(k->follower, dir, 1, FALSE);
            }
        }
    } else {
        act("$n stumbles and trips as $e fails to retreat!", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You stumble and trip as you try to retreat!\r\n", ch);
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
    struct char_data *vict;
    int percent, prob, skill, rounds;

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
        send_to_char("You're not really sure how...\r\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS)) {
            send_to_char("&8It's just too dark!&0\r\n", ch);
            return;
        }

        if (EFF_FLAGGED(ch, EFF_BLIND)) {
            send_to_char("You can't see a thing!\r\n", ch);
            return;
        }

        if (skill == SKILL_BODYSLAM && FIGHTING(ch)) {
            send_to_char("You can't bodyslam in combat...\r\n", ch);
            return;
        }

        if (GET_COOLDOWN(ch, CD_BASH)) {
            sprintf(buf, "You haven't reoriented yourself for another %s yet!\r\n", skills[skill].name);
            send_to_char(buf, ch);
            return;
        }
    }

    one_argument(argument, arg);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        vict = FIGHTING(ch);
        if (!vict || IN_ROOM(ch) != IN_ROOM(vict) || !CAN_SEE(ch, vict)) {
            sprintf(buf, "%s who?\r\n", skills[skill].name);
            send_to_char(CAP(buf), ch);
            return;
        }
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (skill == SKILL_MAUL && (!EFF_FLAGGED(ch, EFF_BERSERK) || !EFF_FLAGGED(ch, EFF_SPIRIT_BEAR))) {
            act("You're not angry enough to tear $M limb from limb.\r\n", FALSE, ch, 0, vict, TO_CHAR);
            return;
        }
    }

    if (vict == ch) {
        send_to_char("Aren't we funny today...\r\n", ch);
        return;
    }
    if (vict == ch->guarding) {
        act("You can't do that while you are guarding $M.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    /* check for pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, TRUE))
        return;
    vict = check_guard(ch, vict, FALSE);
    if (!attack_ok(ch, vict, TRUE))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    if (GET_POS(vict) <= POS_SITTING) {
        act("$E has already been knocked down.", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    prob = GET_SKILL(ch, skill);
    percent = number(1, 101); /* 101 is a complete failure */
    switch (skill) {
    case SKILL_BODYSLAM:
        prob = number(1, 100); /* bodyslam uses random num instead of skill */
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

    if ((prob > percent || MOB_FLAGGED(vict, MOB_NOBASH)) && damage_evasion(vict, ch, 0, DAM_CRUSH)) {
        act(EVASIONCLR "You charge right through $N&7&b!&0", FALSE, ch, 0, vict, TO_CHAR);
        act(EVASIONCLR "$n" EVASIONCLR " charges right through $N" EVASIONCLR "!&0", FALSE, ch, 0, vict, TO_NOTVICT);
        act(EVASIONCLR "$n" EVASIONCLR " charges right through you!&0", FALSE, ch, 0, vict, TO_VICT);
        send_to_char("You fall down!\r\n", ch);
        act("$n falls down!", FALSE, ch, 0, 0, TO_ROOM);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_ALERT;
        set_fighting(vict, ch, FALSE);
        return;
    }

    /* NO BASH - you fail. */
    if (MOB_FLAGGED(vict, MOB_NOBASH)) {
        act("You &3&bslam&0 into $N, but $E seems quite unmoved.", FALSE, ch, 0, vict, TO_CHAR);
        act("$n &3&bslams&0 into $N, who seems as solid as a rock!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n &3&bslams&0 into you, attempting to knock you down.", FALSE, ch, 0, vict, TO_VICT);
        /* A pause... but you don't fall down. */
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        set_fighting(vict, ch, FALSE);
        return;
    }

    if (GET_SIZE(vict) - GET_SIZE(ch) > 1) {
        sprintf(buf, "&7&bYou fall over as you try to %s someone so large!&0\r\n", skills[skill].name);
        send_to_char(buf, ch);
        act("&7&b$n BOUNCES off $N, as $e tries to $t $N's much larger size.&0", FALSE, ch, (void *)skills[skill].name,
            vict, TO_NOTVICT);
        act("&7&b$n BOUNCES off you as $e tries to $t your much larger size.&0", FALSE, ch, (void *)skills[skill].name,
            vict, TO_VICT);
        percent = prob + 1; /* insta-fail */
    } else if (GET_SIZE(ch) - GET_SIZE(vict) > 2) {
        sprintf(buf,
                "&7&bYou fall over as you try to %s someone with such small "
                "size.&0\r\n",
                skills[skill].name);
        send_to_char(buf, ch);
        act("&7&b$n trips over $N, as $e tries to $t $N's much smaller size.&0", FALSE, ch, (void *)skills[skill].name,
            vict, TO_NOTVICT);
        act("&7&b$n trips over you as $e tries to $t your much smaller size.&0", FALSE, ch, (void *)skills[skill].name,
            vict, TO_VICT);
        percent = prob + 1; /* insta-fail */
    }

    if (prob > percent) { /* Success! */
        WAIT_STATE(vict, PULSE_VIOLENCE * rounds);
        damage(ch, vict, dam_suscept_adjust(ch, vict, 0, GET_LEVEL(ch) >> 1, DAM_CRUSH), skill);
        if (GET_POS(vict) > POS_SITTING)
            alter_pos(vict, POS_SITTING, STANCE_ALERT);
    } else {
        if (skill == SKILL_BASH && !GET_EQ(ch, WEAR_SHIELD))
            send_to_char("You need to wear a shield to make it a success!\r\n", ch);
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
    struct char_data *vict, *attacker, *c;
    int percent, prob, num;

    one_argument(argument, arg);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        send_to_char("Whom do you want to rescue?\r\n", ch);
        return;
    }
    if (vict == ch) {
        send_to_char("What about fleeing instead?\r\n", ch);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        send_to_char("You feel ashamed trying to disturb the peace of this room.\r\n", ch);
        return;
    }
    if (FIGHTING(ch) == vict) {
        send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
        return;
    }
    if (!vict->attackers) {
        act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }
    if (GET_SKILL(ch, SKILL_RESCUE) == 0) {
        send_to_char("But only true warriors can do this!\r\n", ch);
        return;
    }

    /* Choose a random attacker from those fighting vict */
    attacker = vict->attackers;
    num = 1;
    for (c = attacker->next_attacker; c; c = c->next_attacker) {
        num++;
        if (number(1, num) == 1)
            attacker = c;
    }

    percent = number(1, 101); /* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_RESCUE);

    if (percent > prob) {
        send_to_char("You fail the rescue!\r\n", ch);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        improve_skill_offensively(ch, attacker, SKILL_RESCUE);
        return;
    }
    send_to_char("Banzai!   To the rescue...\r\n", ch);
    act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

    if (FIGHTING(vict) == attacker)
        stop_fighting(vict);
    stop_fighting(attacker);
    if (FIGHTING(ch))
        stop_fighting(ch);

    set_fighting(ch, attacker, TRUE);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    WAIT_STATE(vict, PULSE_VIOLENCE + 2);

    improve_skill_offensively(ch, attacker, SKILL_RESCUE);
}

ACMD(do_kick) {
    struct char_data *vict;
    int percent, prob;

    if (GET_SKILL(ch, SKILL_KICK) == 0) {
        send_to_char("You'd better leave all the martial arts to fighters.\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch)) {
            vict = FIGHTING(ch);
        } else {
            send_to_char("Kick who?\r\n", ch);
            return;
        }
    }
    if (vict == ch) {
        send_to_char("Aren't we funny today...\r\n", ch);
        return;
    }

    if (!attack_ok(ch, vict, TRUE))
        return;

    if (EFF_FLAGGED(ch, EFF_IMMOBILIZED)) {
        send_to_char("You can't lift your legs!\r\n", ch);
        return;
    }

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    /* Need to see whether this player is fighting already. Kick should not
       allow for the player to switch without a switch probability being
       calculated into the mix. (DEMOLITUM) */
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (FIGHTING(ch) && FIGHTING(ch) != vict && !switch_ok(ch))
        return;

    percent = ((10 - ((GET_AC(vict) + (monk_weight_penalty(vict) * 5)) / 10)) << 1) + number(1, 101);
    prob = GET_SKILL(ch, SKILL_KICK);
    if (percent > prob) {
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        damage(ch, vict, 0, SKILL_KICK);
    } else {
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        if (damage_evasion(vict, ch, 0, DAM_CRUSH)) {
            act(EVASIONCLR "Your foot passes harmlessly through $N" EVASIONCLR "!&0", FALSE, ch, 0, vict, TO_CHAR);
            act(EVASIONCLR "$n&7&b sends $s foot whistling right through $N" EVASIONCLR ".&0", FALSE, ch, 0, vict,
                TO_NOTVICT);
            act(EVASIONCLR "$n" EVASIONCLR " tries to kick you, but $s foot passes through you harmlessly.&0", FALSE,
                ch, 0, vict, TO_VICT);
            set_fighting(vict, ch, TRUE);
            return;
        }
        damage(ch, vict, dam_suscept_adjust(ch, vict, 0, GET_LEVEL(ch) >> 1, DAM_CRUSH), SKILL_KICK);
    }
    improve_skill_offensively(ch, vict, SKILL_KICK);
}

ACMD(do_eye_gouge) {
    struct char_data *vict;
    int percent, prob;
    struct effect eff;

    if (GET_SKILL(ch, SKILL_EYE_GOUGE) == 0) {
        send_to_char("You would do such a despicable act!?   Horrible!\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BLIND)) {
        send_to_char("It's going to be hard to gouge someone's eyes out if you can't even see.\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            send_to_char("Whose eyes do you want to gouge out?\r\n", ch);
            return;
        }
    } else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        send_to_char("Nobody here by that name.\r\n", ch);
        return;
    }

    if (FIGHTING(ch) && FIGHTING(ch) != vict) {
        send_to_char("You need to be facing each other for this to work.\r\n", ch);
        return;
    }

    if (vict == ch) {
        send_to_char("That would make life difficult, wouldn't it?\r\n", ch);
        return;
    }

    /* check pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, TRUE))
        return;

    if (GET_COOLDOWN(ch, CD_EYE_GOUGE)) {
        send_to_char("You aren't able to find an opening yet!\r\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < LVL_GOD)
        SET_COOLDOWN(ch, CD_EYE_GOUGE, 3 * PULSE_VIOLENCE);

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    percent = number(1, 101);
    prob = GET_SKILL(ch, SKILL_EYE_GOUGE);
    WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
    if (percent > prob && AWAKE(vict))
        damage(ch, vict, 0, SKILL_EYE_GOUGE); /* Miss message */
    else if (damage_evasion(vict, ch, 0, DAM_PIERCE)) {
        act(EVASIONCLR "Your thumbs poke harmlessly at $N" EVASIONCLR ".   If $E even has eyes.", FALSE, ch, 0, vict,
            TO_CHAR);
        act(EVASIONCLR "$n" EVASIONCLR " tries poking at $N's eyes, but nothing seems to happen.", FALSE, ch, 0, vict,
            TO_NOTVICT);
        act(EVASIONCLR "$n" EVASIONCLR " pokes fruitlessly at you with $s thumbs.", FALSE, ch, 0, vict, TO_VICT);
        set_fighting(vict, ch, TRUE);
        return;
    } else if (GET_LEVEL(vict) >= LVL_IMMORT && GET_LEVEL(vict) > GET_LEVEL(ch)) {
        act("Ouch!   You hurt your thumbs trying to poke out $N's eyes!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n tries poking out $N's eyes - what a laugh!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n pokes harmlessly at your eyes with $s thumbs.", FALSE, ch, 0, vict, TO_VICT);
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
    struct char_data *vict;
    int percent, prob, dmg;

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_DARKNESS) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char("&8It is too dark!&0\r\n", ch);
        return;
    }

    if (!GET_SKILL(ch, SKILL_SPRINGLEAP)) {
        send_to_char("You'd better leave all the martial arts to monks.\r\n", ch);
        return;
    }

    if (GET_POS(ch) > POS_SITTING) {
        send_to_char("You can't spring from that position, try sitting!\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_IMMOBILIZED)) {
        send_to_char("You can't lift your legs!\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            send_to_char("&0Spring-leap whom?&0\r\n", ch);
            return;
        }
    }

    if (vict == ch) {
        send_to_char("That might hurt too much...\r\n", ch);
        return;
    }

    /* check pk/pets/shapeshifts */
    if (!attack_ok(ch, vict, TRUE))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    percent = number(6, 77) - (GET_AC(vict) + (5 * monk_weight_penalty(vict))) / 20;

    prob = GET_SKILL(ch, SKILL_SPRINGLEAP);

    if (GET_POS(vict) <= POS_SITTING)
        percent = 101;

    if (GET_STANCE(vict) < STANCE_FIGHTING)
        prob -= 20;

    if (percent > prob) {
        act("&0&6You try to take $N down but you spring over $S head!&0", FALSE, ch, 0, vict, TO_CHAR);
        act("&0&6$N springs from the ground at you but soars over your head!&0", FALSE, vict, 0, ch, TO_CHAR);
        act("&0&6$N springs from the ground at $n but misses by a mile!&0", FALSE, vict, 0, ch, TO_NOTVICT);
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        damage(ch, vict, 0, SKILL_SPRINGLEAP);
        if (AWAKE(ch)) {
            GET_POS(ch) = POS_SITTING;
            GET_STANCE(ch) = STANCE_ALERT;
        }
    } else if (damage_evasion(vict, ch, 0, DAM_CRUSH)) {
        act(EVASIONCLR "You hurtle right through $N" EVASIONCLR " and land in a heap on the other side!", FALSE, ch, 0,
            vict, TO_CHAR);
        act(EVASIONCLR "$n" EVASIONCLR " leaps at $N" EVASIONCLR " but flies right on through!", FALSE, ch, 0, vict,
            TO_NOTVICT);
        act(EVASIONCLR "$n" EVASIONCLR " comes flying at you, but just passes through and hits the ground.", FALSE, ch,
            0, vict, TO_VICT);
        /* You fall */
        WAIT_STATE(ch, (PULSE_VIOLENCE * 3) / 2);
        GET_POS(ch) = POS_SITTING;
        GET_STANCE(ch) = STANCE_ALERT;
        set_fighting(vict, ch, FALSE);
        return;
    } else if (percent > 0.95 * prob) {
        dmg = dam_suscept_adjust(ch, vict, 0, GET_LEVEL(ch) >> 1, DAM_CRUSH);
        act("&0&6You manage to take $N down but also &bfall down yourself!&0 "
            "(&3$i&0)",
            FALSE, ch, (void *)dmg, vict, TO_CHAR);
        act("&0&6$N springs from the ground and knocks you down - &bbut falls in "
            "the process!&0 (&1&8$i&0)",
            FALSE, vict, (void *)dmg, ch, TO_CHAR);
        act("&0&6$N springs from the ground, knocking $n down and &bfalling in the "
            "process!&0 (&4$i&0)",
            FALSE, vict, (void *)dmg, ch, TO_NOTVICT);
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
        act("&0&b&8You spring from the ground, knocking $N off balance.&0 "
            "(&1&8$i&0)",
            FALSE, ch, (void *)dmg, vict, TO_CHAR);
        act("&0&b&8$N springs from the ground and knocks you down!&0 (&3$i&0)", FALSE, vict, (void *)dmg, ch, TO_CHAR);
        act("&0&b&8$N springs from the ground, knocking $n down!&0 (&4$i&0)", FALSE, vict, (void *)dmg, ch, TO_NOTVICT);
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
    struct char_data *vict;
    struct effect eff;
    struct obj_data *weapon;
    int random, chance, percent, dam, expReduction;
    char buf1[255];
    char buf2[255];
    char buf3[255];
    char stop_buf1[255];
    char stop_buf2[255];
    bool skipcast = FALSE;

    if (GET_COOLDOWN(ch, CD_THROATCUT)) {
        send_to_char("You've drawn too much attention to yourself to throatcut now!\r\n", ch);
        return;
    }

    random = dice(1, 6);

    if ((GET_LEVEL(ch) < LVL_IMMORT) && (GET_SKILL(ch, SKILL_THROATCUT) == 0)) {
        send_to_char("You aren't skilled enough!\r\n", ch);
        return;
    }

    if (FIGHTING(ch)) {
        send_to_char("You can't be stealthy enough to do this while fighting!\r\n", ch);
        return;
    }

    if (RIDING(ch)) {
        send_to_char("Cut someone's throat while riding???   I don't think so!\r\n", ch);
        return;
    }

    one_argument(argument, buf);

    if ((!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) && (!(vict == ch))) {
        send_to_char("Cut whose throat?\r\n", ch);
        return;
    }

    if (vict == ch) {
        send_to_char("Hey, life's not that bad!\r\n", ch);
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
        send_to_char("&0You need a one-handed slashing weapon to cut throats.&0\r\n", ch);
        return;
    }

    /* If wielding something unsuitable in first hand, use weapon in second hand
     */
    if (!WEAPON_CAN_THROATCUT(weapon) && GET_EQ(ch, WEAR_WIELD2)) {
        weapon = GET_EQ(ch, WEAR_WIELD2);
    }

    if (!WEAPON_CAN_THROATCUT(weapon)) {
        send_to_char("&0You need a one-handed slashing weapon to cut throats.&0\r\n", ch);
        return;
    }

    if (FIGHTING(vict)) {
        send_to_char("&0You can't cut the throat of a fighting person -- they're too alert!&0\r\n", ch);
        return;
    }

    if (!attack_ok(ch, vict, TRUE))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    /* Can't throatcut dragons, nor mobs that are twice your size */
    /*
    if (GET_RACE(vict) == RACE_DRAGON) {
    */
    if (GET_RACE(vict) == RACE_DRAGON_GENERAL || GET_RACE(vict) == RACE_DRAGON_FIRE 
    || GET_RACE(vict) == RACE_DRAGON_ACID || GET_RACE(vict) == RACE_DRAGON_FROST 
    || GET_RACE(vict) == RACE_DRAGON_LIGHTNING || GET_RACE(vict) == RACE_DRAGON_GAS) {
        send_to_char("Cut the throat... of a dragon... RIGHT!!!!!\r\n", ch);
        return;
    } else if ((GET_SIZE(vict) > GET_SIZE(ch) + 2) || (GET_SIZE(vict) < GET_SIZE(ch) - 2)) {
        send_to_char("Maybe if you were close to the same size it would work!!\r\n", ch);
        return;
    }

    SET_COOLDOWN(ch, CD_THROATCUT, 3 MUD_HR);

    percent = dice(1, 100);

    if ((MOB_FLAGGED(vict, MOB_AWARE) || EFF_FLAGGED(vict, EFF_AWARE)) && AWAKE(vict) &&
        !EFF_FLAGGED(vict, EFF_MINOR_PARALYSIS) && !EFF_FLAGGED(vict, EFF_MAJOR_PARALYSIS)) {
        act("You notice $N sneaking up on you!", FALSE, vict, 0, ch, TO_CHAR);
        act("$e notices you sneaking up on $m!", FALSE, vict, 0, ch, TO_VICT);
        act("$n notices $N sneaking up on $m!", FALSE, vict, 0, ch, TO_NOTVICT);
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
        act("$N laughs out loud at your miserable attempt!&0", FALSE, ch, 0, vict, TO_CHAR);
        act("$n just tried to cut your throat. &0&6How cute!&0", FALSE, ch, 0, vict, TO_VICT);
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
            set_fighting(vict, ch, TRUE);
            return;
        }
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
            sprintf(buf2,
                    "&1&bBlood spews everywhere as $n nearly incapacitates you with "
                    "%s!&0",
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
        skipcast = TRUE;
        expReduction = 0; /* rip same % exp from the mob... since they're doing less work! */

        /* If we want silent misses for non-critical misses.. remove the act txt */
        sprintf(buf1, "&3&b$N jumps back before you have a chance to even get close!&0");
        sprintf(buf2, "&3&b$n just tried to cut your throat!&0");
        sprintf(buf3, "&3&b$n misses $N with $s throat cut!&0");
    }

    if (IS_NPC(vict))
        GET_EXP(vict) = MAX(1, GET_EXP(vict) - expReduction); /* make sure we don't have
                                                                 negative exp gain for ch */

    if (damage_amounts) {
        if (dam <= 0)
            sprintf(buf, " (&1%d&0)", dam);
        else
            sprintf(buf, " (&3%d&0)", dam);

        strcat(buf1, buf);
        act(buf1, FALSE, ch, NULL, vict, TO_CHAR);

        strcat(buf2, buf);
        act(buf2, FALSE, ch, NULL, vict, TO_VICT);

        strcat(buf3, buf);
        act(buf3, FALSE, ch, NULL, vict, TO_NOTVICT);
    } else {
        act(buf1, FALSE, ch, NULL, vict, TO_CHAR);
        act(buf2, FALSE, ch, NULL, vict, TO_VICT);
        act(buf3, FALSE, ch, NULL, vict, TO_NOTVICT);
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
        act(stop_buf1, FALSE, vict, 0, 0, TO_CHAR);
        act(stop_buf2, FALSE, vict, 0, 0, TO_ROOM);
    }
}

ACMD(do_disarm) {
    int pos, ch_pos, chance, rnd_num, skl_bonus, move_cost;
    struct obj_data *obj, *ch_obj;       /* Object to disarm */
    struct char_data *tch, *vict = NULL; /* Target */
    bool disarm_prim = TRUE;

    if (GET_SKILL(ch, SKILL_DISARM) == 0) {
        send_to_char("You don't know how to disarm.\r\n", ch);
        return;
    }

    /* Make sure we're fighting someone who'll be disarmed */
    if (!FIGHTING(ch)) {
        send_to_char("You can only disarm someone who you're fighting.\r\n", ch);
        return;
    }

    tch = FIGHTING(ch);

    /* Fighting yourself? Unlikely... but anyway: */
    if (ch == tch) {
        send_to_char("Try 'remove' instead.\r\n", ch);
        return;
    }

    move_cost = 11 - GET_SKILL(ch, SKILL_DISARM) / 6;

    /* Need mv to perform disarm */
    if (GET_MOVE(ch) < move_cost) {
        send_to_char("You don't have the energy to do that.\r\n", ch);
        return;
    }

    if (GET_COOLDOWN(ch, CD_FUMBLING_PRIMARY) || GET_COOLDOWN(ch, CD_FUMBLING_SECONDARY)) {
        send_to_char("Impossible!  You're already fumbling for your own weapon.\r\n", ch);
        return;
    }

    /* Can't disarm them if you can't see them */
    if (!CAN_SEE(ch, tch)) {
        send_to_char("It's pretty hard to disarm someone you can't even see...\r\n", ch);
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
        disarm_prim = FALSE;
    } else {
        send_to_char("You must be wielding some kind of weapon.\r\n", ch);
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
            act("$N isn't even wielding a weapon!", FALSE, ch, 0, tch, TO_CHAR);
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
        act("$N doesn't seem to be wielding any such thing.", FALSE, ch, 0, tch, TO_CHAR);
        return;
    }

    /* Unlikely, but but just in case: */
    if (GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
        send_to_char("You can only disarm weapons.\r\n", ch);
        return;
    }

    if (CONFUSED(ch)) {
        send_to_char("You're way too addle-headed to disarm anyone.\r\n", ch);
        return;
    }

    /* The attempt may take place! */

    /* Calculate the outcome, based on:
     *
     * - disarm skill
     * - dexterity
     * - level difference
     */

    chance = GET_SKILL(ch, SKILL_DISARM) + dex_app_skill[GET_DEX(ch)].p_pocket + (GET_LEVEL(ch) - GET_LEVEL(tch));

    /* 1 - 35 extra points to account for superlative dex or xp diff */
    skl_bonus = (int)((chance - 69) / 2);

    if (skl_bonus > 0)
        chance += skl_bonus;

    chance = MAX(1, chance);

    /* has char tried to disarm within delay period?  if so, penalize them */
    if (GET_COOLDOWN(ch, CD_DISARM))
        chance -= 30; /* 30% pts */

    rnd_num = number(1, chance);

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
    SET_COOLDOWN(ch, CD_DISARM, number(1, 3) * PULSE_VIOLENCE);

    if (rnd_num <= 5) {
        act("$n fails $s disarming maneuver so badly, $e drops $s own weapon.", FALSE, ch, 0, tch, TO_NOTVICT);
        act("$n tries to disarm but drops $s weapon!", FALSE, ch, 0, tch, TO_VICT);
        act("You try to disarm $N but drop your $o instead!", FALSE, ch, ch_obj, tch, TO_CHAR);

        pos = ch_pos;
        obj = ch_obj;
        vict = ch;
    } else if (rnd_num <= 25) {
        act("$n flubs an attempt at disarming $N.", FALSE, ch, 0, tch, TO_NOTVICT);
        act("$e fumbles $s own weapon.", FALSE, ch, 0, 0, TO_NOTVICT);
        act("$n fumbles $s weapon while trying to disarm you.", FALSE, ch, 0, tch, TO_VICT);
        act("Oops!  You fumbled your $o!", FALSE, ch, ch_obj, 0, TO_CHAR);

        pos = ch_pos;
        obj = ch_obj;
        vict = ch;
    } else if (rnd_num >= 26 && rnd_num <= 75) {
        act("$n tries to disarm $N, but $E keeps a firm grip on $S weapon.", FALSE, ch, 0, tch, TO_NOTVICT);
        act("$n tries to disarm you, but you maintain your weapon.", FALSE, ch, 0, tch, TO_VICT);
        act("You try to disarm $N, but $E keeps hold of $S weapon.", FALSE, ch, 0, tch, TO_CHAR);
    } else if (rnd_num <= 95) {
        act("$n causes $N to fumble $S weapon.", FALSE, ch, 0, tch, TO_NOTVICT);
        act("$n causes you to fumble your weapon.", FALSE, ch, 0, tch, TO_VICT);
        act("You cause $N to fumble $S weapon.", FALSE, ch, 0, tch, TO_CHAR);

        vict = tch;
    } else {
        act("$n successfully knocks $N's weapon from $S grip!", FALSE, ch, 0, tch, TO_NOTVICT);
        act("$n forces $p out of your hands with a fancy disarming maneuver.", FALSE, ch, obj, tch, TO_VICT);
        if (CH_OUTSIDE(tch)) {
            act("You send $N's weapon crashing to the ground.", FALSE, ch, 0, tch, TO_CHAR);
        } else {
            act("You send $N's weapon crashing to the floor.", FALSE, ch, 0, tch, TO_CHAR);
        }

        vict = tch;
    }

    improve_skill_offensively(ch, tch, SKILL_DISARM);

    /* handle cases where either A or B loses hold of weapon */
    if (rnd_num <= 5 || rnd_num >= 96) {
        if ((OBJ_FLAGGED(obj, ITEM_NODROP))) { /* Cursed? */
            obj_to_char(unequip_char(vict, pos), vict);
            act("&3&b$p&3&b magically returns to your&0 &B&3inventory!&0", TRUE, vict, obj, NULL, TO_CHAR);
            act("&3&b$p&3&b magically returns to $s&0 &B&3inventory!&0", TRUE, vict, obj, NULL, TO_ROOM);
        } else {
            obj_to_room(unequip_char(vict, pos), vict->in_room);
            sprintf(buf, "%s lands on the %s.", obj->short_description, CH_OUTSIDE(vict) ? "ground" : "floor");
            act(buf, FALSE, vict, 0, 0, TO_ROOM);
            act(buf, FALSE, vict, 0, 0, TO_CHAR);
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
    struct char_data *mob, *next_mob;
    byte percent;
    bool hit_all = FALSE, realvictims = FALSE, success = FALSE;

    if (!ch || ch->in_room == NOWHERE)
        return;

    if (subcmd == SCMD_TANTRUM) {
        if (!GET_SKILL(ch, SKILL_TANTRUM)) {
            send_to_char("You throw a hissy-fit, hoping someone will notice you.\r\n", ch);
            act("$n sobs to $mself loudly, soliciting attention.", TRUE, ch, 0, 0, TO_ROOM);
            return;
        }
        if (!EFF_FLAGGED(ch, EFF_BERSERK)) {
            send_to_char("You're not feeling quite up to throwing a tantrum right now.\r\n", ch);
            return;
        }
    } else if (!GET_SKILL(ch, SKILL_HITALL)) {
        send_to_char("You don't know how to.\r\n", ch);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        send_to_char("You feel ashamed trying to disturb the peace of this room.\r\n", ch);
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
        send_to_char("Sorry, it's too cramped here for nasty maneuvers!\r\n", ch);
        return;
    }

    /* Find out whether to hit "all" or just aggressive monsters */
    one_argument(argument, arg);
    if (!str_cmp(arg, "all") || subcmd == SCMD_TANTRUM)
        hit_all = 1;

    /* Hit all aggressive monsters in room */

    percent = number(1, 131);
    WAIT_STATE(ch, PULSE_VIOLENCE);

    if (subcmd == SCMD_TANTRUM) {
        act("$n flings $s limbs around wildly, swiping at everything nearby!", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char("You throw an incensed tantrum, attacking all nearby!\r\n", ch);
        if (GET_SKILL(ch, SKILL_TANTRUM) >= percent)
            success = TRUE;
    } else {
        act("$n makes a concerted circular attack at everything nearby!", FALSE, ch, 0, 0, TO_NOTVICT);
        send_to_char("You spin in a circle, attempting to hit everything within range.\r\n", ch);
        if (GET_SKILL(ch, SKILL_HITALL) >= percent)
            success = TRUE;
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
            realvictims = TRUE;

        if (success) {
            if (damage_evasion(mob, ch, 0, physical_damtype(ch))) {
                damage_evasion_message(ch, mob, equipped_weapon(ch), physical_damtype(ch));
                set_fighting(mob, ch, TRUE);
            } else if (subcmd == SCMD_TANTRUM && number(0, 1))
                hit(ch, mob, SKILL_BAREHAND);
            else
                attack(ch, mob);
        }
    }

    if (realvictims)
        improve_skill(ch, subcmd == SCMD_TANTRUM ? SKILL_TANTRUM : SKILL_HITALL);
}

ACMD(do_corner) {
    struct char_data *vict;
    int chance;

    if (!GET_SKILL(ch, SKILL_CORNER)) {
        send_to_char("You aren't skilled enough to corner an opponent!\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    /* You can only corner the person you're fighting. */
    if (!*arg && FIGHTING(ch))
        vict = FIGHTING(ch);
    else if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))) || vict != FIGHTING(ch)) {
        send_to_char("You have to be fighting someone to corner them!\r\n", ch);
        return;
    }

    if (CONFUSED(ch)) {
        send_to_char("You're far too confused to corner anyone.\r\n", ch);
        return;
    }

    if (ch->cornering) {
        if (ch->cornering->cornered_by == ch)
            ch->cornering->cornered_by = NULL;
        ch->cornering = NULL;
    }

    chance = GET_SKILL(ch, SKILL_CORNER);
    chance += 3 * dex_app[GET_DEX(ch)].reaction;
    chance += 10 * (GET_SIZE(ch) - GET_SIZE(vict));
    chance += (GET_LEVEL(ch) - GET_LEVEL(vict)) / 2;
    if (!CAN_SEE(vict, ch))
        chance *= 2;

    if (chance > number(1, 101)) {
        act("You stand in $N's way, cornering $M!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n stands in your way, cornering you!", FALSE, ch, 0, vict, TO_VICT);
        act("$n stands in $N's way, cornering $M!", TRUE, ch, 0, vict, TO_NOTVICT);
        ch->cornering = vict;
        vict->cornered_by = ch;
    } else
        act("You attempt to corner $N, but $E evades you!", FALSE, ch, 0, vict, TO_CHAR);

    WAIT_STATE(ch, PULSE_VIOLENCE);
    improve_skill_offensively(ch, vict, SKILL_CORNER);
}

ACMD(do_peck) {
    struct char_data *vict;
    int dam;

    if (!ch)
        return;

    if (GET_SKILL(ch, SKILL_PECK) <= 0) {
        send_to_char("How do you expect to do that?\r\n", ch);
        return;
    }

    one_argument(argument, arg);
    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            send_to_char("Peck whom?\r\n", ch);
            return;
        }
    }
    if (ch == vict) {
        send_to_char("Ouch, that hurts!\r\n", ch);
        return;
    }

    /* Is this attack allowed? */
    if (!attack_ok(ch, vict, TRUE))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    /* If attacking someone else, check switch skill. */
    if (FIGHTING(ch) && FIGHTING(ch) != vict && !switch_ok(ch))
        return;

    /* Determine the damage amount.   0 is a miss. */
    if (number(0, 101) > GET_SKILL(ch, SKILL_PECK))
        dam = 0;
    else if (damage_evasion(vict, ch, 0, DAM_PIERCE)) {
        damage_evasion_message(ch, vict, 0, DAM_PIERCE);
        set_fighting(vict, ch, TRUE);
        return;
    } else
        dam = number(GET_SKILL(ch, SKILL_PECK), GET_LEVEL(ch));

    damage(ch, vict, dam_suscept_adjust(ch, vict, 0, dam, DAM_PIERCE), SKILL_PECK);
    improve_skill_offensively(ch, vict, SKILL_PECK);
}

ACMD(do_claw) {
    struct char_data *vict;
    int dam;

    if (!ch)
        return;

    if (GET_SKILL(ch, SKILL_CLAW) <= 0) {
        send_to_char("Grow some longer fingernails first!\r\n", ch);
        return;
    }

    one_argument(argument, arg);
    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        if (FIGHTING(ch))
            vict = FIGHTING(ch);
        else {
            send_to_char("Claw whom?\r\n", ch);
            return;
        }
    }
    if (ch == vict) {
        send_to_char("Ouch, that hurts!\r\n", ch);
        return;
    }

    /* Can we allow this attack to occur? */
    if (!attack_ok(ch, vict, TRUE))
        return;

    if (CONFUSED(ch))
        vict = random_attack_target(ch, vict, TRUE);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    /* If attacking someone else, check skill in switch. */
    if (FIGHTING(ch) && FIGHTING(ch) != vict && !switch_ok(ch))
        return;

    /* Determine damage amount. */
    if (number(0, 101) > GET_SKILL(ch, SKILL_CLAW))
        dam = 0;
    else if (damage_evasion(vict, ch, 0, DAM_SLASH)) {
        damage_evasion_message(ch, vict, 0, DAM_SLASH);
        set_fighting(vict, ch, TRUE);
        return;
    } else
        dam = number(GET_SKILL(ch, SKILL_CLAW), GET_LEVEL(ch));

    damage(ch, vict, dam_suscept_adjust(ch, vict, 0, dam, DAM_SLASH), SKILL_CLAW);
    improve_skill_offensively(ch, vict, SKILL_CLAW);
}

ACMD(do_electrify) {
    if (!ch || ch->in_room == NOWHERE)
        return;

    if (GET_SKILL(ch, SKILL_ELECTRIFY) <= 0) {
        send_to_char("Good luck with that one!\r\n", ch);
        return;
    }

    if (GET_SKILL(ch, SKILL_ELECTRIFY) > number(0, 101))
        mag_area(GET_SKILL(ch, SKILL_ELECTRIFY), ch, SKILL_ELECTRIFY, SAVING_BREATH);
    else {
        if (IS_WATER(ch->in_room))
            send_to_char("The water around you sizzles, but you are unable to gatherany power...\r\n",
                         ch);
        else
            send_to_char("The air around you crackles, but you are unable to gather any power...\r\n",
                         ch);
        act("A quick spike of electricity runs across $n's skin.", TRUE, ch, 0, 0, TO_ROOM);
    }

    improve_skill(ch, SKILL_ELECTRIFY);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

void start_berserking(struct char_data *ch) {
    struct effect eff;

    memset(&eff, 0, sizeof(eff));
    eff.type = SKILL_BERSERK;
    eff.duration = 1000; /* arbitrarily long time */
    eff.modifier = 0;
    eff.location = APPLY_NONE;
    SET_FLAG(eff.flags, EFF_BERSERK);
    effect_to_char(ch, &eff);
    check_regen_rates(ch);
}

void stop_berserking(struct char_data *ch) {
    effect_from_char(ch, SKILL_BERSERK);
    effect_from_char(ch, CHANT_SPIRIT_WOLF);
    effect_from_char(ch, CHANT_SPIRIT_BEAR);
    effect_from_char(ch, CHANT_INTERMINABLE_WRATH);
    GET_RAGE(ch) = 0;
}

ACMD(do_berserk) {
    if (!GET_SKILL(ch, SKILL_BERSERK)) {
        send_to_char("You flail your arms about, acting like a crazy person.\r\n", ch);
        act("$n goes berserk, thrashing about the area.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        send_to_char("You're already out of control!\r\n", ch);
        return;
    }

    if (GET_RAGE(ch) < RAGE_ANGRY) {
        send_to_char("You're not angry enough yet!\r\n", ch);
        return;
    }

    send_to_char("You feel your blood begin to boil, and your self-control starts to slip...\r\n", ch);
    act("$n's eyes flash and anger clouds $s face.", TRUE, ch, 0, 0, TO_ROOM);

    start_berserking(ch);
}

/*
 * Be careful when calling do_stomp manually (i.e., not from the command
 * interpreter.   If the cmd number is wrong, and the command gets passed
 * to do_action, the game may crash.
 */
ACMD(do_stomp) {
    struct char_data *tch, *next_tch;
    bool real_victims = FALSE;

    extern ACMD(do_action);

    if (!GET_SKILL(ch, SKILL_GROUND_SHAKER) || !EFF_FLAGGED(ch, EFF_SPIRIT_BEAR) || !EFF_FLAGGED(ch, EFF_BERSERK)) {
        do_action(ch, argument, cmd, subcmd);
        return;
    }

    if (CH_INDOORS(ch)) {
        send_to_char("You MUST be crazy, trying to do that in here!\r\n", ch);
        return;
    }

    if (!QUAKABLE(IN_ROOM(ch))) {
        send_to_char("There's no ground to stomp on here!\r\n", ch);
        return;
    }

    send_to_char("&8&3You stomp one foot on the ground heavily, shaking the earth!&0\r\n", ch);
    act("&8&3$n crashes a foot into the ground, causing it to crack around "
        "$m...&0",
        TRUE, ch, 0, 0, TO_ROOM);

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
                real_victims = TRUE;

            if (GET_DEX(tch) < number(0, 100))
                damage(ch, tch,
                       dam_suscept_adjust(ch, tch, 0, number(50, GET_SKILL(ch, SKILL_GROUND_SHAKER)), DAM_CRUSH),
                       SKILL_GROUND_SHAKER);
            else if (GET_STR(tch) < number(0, 100)) {
                if (GET_POS(tch) > POS_SITTING) {
                    damage(ch, tch, 0, SKILL_GROUND_SHAKER);
                    if (IN_ROOM(ch) == IN_ROOM(tch))
                        alter_pos(tch, POS_KNEELING, STANCE_ALERT);
                    WAIT_STATE(ch, PULSE_VIOLENCE);
                }
            }
        } else {
            set_fighting(tch, ch, TRUE);
        }
    }

    if (real_victims)
        improve_skill(ch, SKILL_GROUND_SHAKER);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}
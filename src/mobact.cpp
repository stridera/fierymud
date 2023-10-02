/***************************************************************************
 *   File: mobact.c                                      Part of FieryMUD  *
 *  Usage: Functions for generating intelligent behavior in mobiles.       *
 *         Memory procedures are here, but combat AI is outsourced.        *
 *  All rights reserved.  See license.doc for complete information.        *
 *  Redesigned by: Ben Horner (Proky of HubisMUD)                          *
 *  Redesigned again by: Laoris of FieryMUD                                *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.hpp"
#include "ai.hpp"
#include "casting.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "rooms.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* External functions */
ACMD(do_stand);
ACMD(do_recline);
ACMD(do_sit);
ACMD(do_kneel);
ACMD(do_fly);
ACMD(do_sweep);
ACMD(do_breathe);
ACMD(do_roar);
ACMD(do_track);
ACMD(do_hide);
ACMD(do_sneak);
ACMD(do_douse);
bool update_inventory(CharData *ch, ObjData *obj, int where);
CharData *check_guard(CharData *ch, CharData *victim, int gag_output);
void get_check_money(CharData *ch, ObjData *obj);
void perform_wear(CharData *ch, ObjData *obj, int where, bool collective = false);
int find_eq_pos(CharData *ch, ObjData *obj, char *arg);
bool mob_cast(CharData *ch, CharData *tch, ObjData *tobj, int spellnum);
void hunt_victim(CharData *ch);
void start_studying(CharData *ch);

/* Local functions */
bool check_mob_status(CharData *ch);
void remember(CharData *ch, CharData *victim);
void mob_attempt_equip(CharData *ch);
bool mob_memory_action(CharData *ch, bool allow_spells);
void memory_attack_announce(CharData *ch, CharData *victim);
void memory_attack(CharData *ch, CharData *vict);
void track_victim_check(CharData *ch, CharData *vict);
bool mob_memory_check(CharData *ch);
/* mobile_activity subfunctions */
void mob_scavenge(CharData *ch);
bool mob_movement(CharData *ch);
bool mob_assist(CharData *ch);
void mob_attack(CharData *ch, CharData *victim);
bool check_spellbank(CharData *ch);

void mobile_activity(void) {
    CharData *ch, *next_ch;
    extern int no_specials;

    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        /* Players, sleeping mobs, and paralyzed, and casting mobs don't do
         * anything. */
        if (!IS_MOB(ch) || CASTING(ch) || !AWAKE(ch) || EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) ||
            EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MESMERIZED))
            continue;

        /* Don't execute procs when someone is switched in. */
        if (POSSESSED (ch))
            continue;

        /* Don't do anything if recovering spells */
        if (EVENT_FLAGGED(ch, EVENT_REGEN_SPELLSLOT))
            continue;

        /* If lower than default position, get up. */
        if (GET_MOB_WAIT(ch) <= 0 && GET_DEFAULT_POS(ch) > GET_POS(ch) && GET_STANCE(ch) >= STANCE_RESTING) {
            switch (GET_DEFAULT_POS(ch)) {
            case POS_PRONE:
                do_recline(ch, "", 0, 0);
                break;
            case POS_SITTING:
                do_sit(ch, "", 0, 0);
                break;
            case POS_KNEELING:
                do_kneel(ch, "", 0, 0);
                break;
            case POS_STANDING:
                do_stand(ch, "", 0, 0);
                break;
            case POS_FLYING:
                do_fly(ch, "", 0, 0);
                break;
            }
        }

        /* Execute any special procs. */
        if (MOB_PERFORMS_SCRIPTS(ch) && MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
            if (mob_index[GET_MOB_RNUM(ch)].func == nullptr) {
                log("{} (#{:d}): Attempting to call non-existing mob func", GET_NAME(ch), GET_MOB_VNUM(ch));
                REMOVE_FLAG(MOB_FLAGS(ch), MOB_SPEC);
            } else if ((mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, ""))
                /* If it executes okay, go on to the next mob. */
                continue;
        }

        /* And now scavenger mobs pick up objects and wear them. */
        if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !MOB_FLAGGED(ch, MOB_ILLUSORY)) {
            mob_scavenge(ch);
            mob_attempt_equip(ch);
        }

        /*
         * All AI activity past this point in the function should only occur
         * when the mobile is not in combat.
         */
        if (FIGHTING(ch))
            continue;

        /*
         * Mobs on fire attempt to douse themselves (50% chance to attempt).
         */
        if (EFF_FLAGGED(ch, EFF_ON_FIRE) && random_number(0, 1)) {
            /* If the mob is flagged !CLASS_AI, then just douse, don't try to cast. */
            if (MOB_FLAGGED(ch, MOB_NO_CLASS_AI) ? true : !mob_cast(ch, ch, nullptr, SPELL_EXTINGUISH))
                do_douse(ch, "", 0, 0);
            continue;
        }

        /*
         * None of the following actions should be taken by charmies.
         */
        if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master && ch->master->in_room != ch->in_room)
            continue;

        /*
         * These actions are special AI.  They're mainly class specific.
         */
        if (!MOB_FLAGGED(ch, MOB_NO_CLASS_AI)) {

            /*
             * This calls check_sorcerer_status() and check_cleric_status()
             * which cast spells like remove curse, etc.
             */
            if (check_mob_status(ch))
                continue;

            /*
             * Check the mob's spellbank and attempt to remem spells.
             */
            if (check_spellbank(ch))
                continue;

            /*
             * Only do these actions if not immortal, since immortals get
             * all skills, but we don't necessarily want them running
             * around stealing and making zombies.
             */
            if (GET_LEVEL(ch) < LVL_IMMORT) {

                /*
                 * Attempt to hide/sneak for those mobs who can do it.
                 */
                if (GET_SKILL(ch, SKILL_HIDE) && GET_HIDDENNESS(ch) == 0)
                    do_hide(ch, "", 0, 0);

                /* Attempt to steal something from a player in the room. */
                if (GET_SKILL(ch, SKILL_STEAL) && random_number(0, 101) < GET_SKILL(ch, SKILL_STEAL))
                    if (mob_steal(ch))
                        continue;

                /* Attempt to make a zombie! */
                if (GET_SKILL(ch, SPELL_ANIMATE_DEAD) && !ch->followers && !MOB_FLAGGED(ch, MOB_ANIMATED))
                    if (mob_animate(ch))
                        continue;
            }
        }

        /*
         * Mob movement
         *
         * Only move if the mob is standing or flying.
         */
        if (!MOB_FLAGGED(ch, MOB_SENTINEL) && GET_POS(ch) >= POS_STANDING)
            if (mob_movement(ch))
                continue;

        /*
         * Helper mobs
         *
         * This is where we assist.  A helper mob will assist any other mob,
         * unless it is fighting that mob, that mob is fighting a player
         * level 20 or less.
         */
        if (MOB_ASSISTER(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
            if (mob_assist(ch))
                continue;

        /*
         * Slow track
         *
         * This is where slow track is taken care of, because mobile_activity
         * is called once every PULSE_MOBILE.  Tracking only occurs when the
         * mob isn't sentinel, isn't fighting, is marked slow track, is
         * marked memory, has someone in its memory.
         */
        if (!MOB_FLAGGED(ch, MOB_SENTINEL) && MOB_FLAGGED(ch, MOB_SLOW_TRACK) && MOB_FLAGGED(ch, MOB_MEMORY))
            if (mob_memory_action(ch, false))
                continue;
    }
}

/*
 * mobile_spec_activity
 *
 * This handles special mobile activity, called every PULSE_VIOLENCE.
 */
void mobile_spec_activity(void) {
    CharData *ch, *next_ch, *vict;
    int found = false;

    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        /* Only for mobiles who aren't fighting, asleep, or casting */
        if (!IS_MOB(ch) || FIGHTING(ch) || !AWAKE(ch) || CASTING(ch))
            continue;

        /* Skip mobiles with someone switched in. */
        if (POSSESSED(ch))
            continue;

        /* Skip mobiles who are paralyzed. */
        if (EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS))
            continue;

        /* If mob isn't fighting, just decrease wait state. */
        if (GET_MOB_WAIT(ch) > 0) {
            WAIT_STATE(ch, std::max(GET_MOB_WAIT(ch) - PULSE_VIOLENCE, 0));
            continue;
        }

        /* Pets, courtesy of Banyal */
        if (PLAYERALLY(ch) && PRF_FLAGGED(ch->master, PRF_PETASSIST)) {
            found = false;
            for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
                if (ch != vict && (ch->master == vict) && FIGHTING(vict) && ch != FIGHTING(vict)) {
                    act("&7$n assists $s master!&0", false, ch, 0, vict, TO_ROOM);
                    attack(ch, check_guard(ch, FIGHTING(vict), false));
                    found = true;
                    break;
                }
            if (found)
                continue;
        }

        /* Should I start a fight? */

        /* Don't start a fight if you're too scared */
        if (GET_HIT(ch) < (GET_MAX_HIT(ch) >> 2) && MOB_FLAGGED(ch, MOB_WIMPY))
            continue;

        /* Look for people I'd like to attack */
        if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && !EFF_FLAGGED(ch, EFF_MESMERIZED) &&
            (!EFF_FLAGGED(ch, EFF_CHARM) || (ch->master && ch->master->in_room != ch->in_room))) {
            if ((vict = find_aggr_target(ch))) {
                mob_attack(ch, vict);
                continue;
            }
        }

        /*
         * Simple mob memory
         *
         * Checks room for memory targets.
         */
        if (MOB_FLAGGED(ch, MOB_MEMORY) && mob_memory_check(ch))
            continue;

        /* If cleric-type and hurt, then heal. */
        if (!MOB_FLAGGED(ch, MOB_NO_CLASS_AI))
            if ((GET_CLASS(ch) == CLASS_CLERIC || GET_CLASS(ch) == CLASS_DRUID || GET_CLASS(ch) == CLASS_PRIEST ||
                 GET_CLASS(ch) == CLASS_DIABOLIST || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_RANGER ||
                 GET_CLASS(ch) == CLASS_BARD) &&
                GET_MAX_HIT(ch) > 0 && 100 * GET_HIT(ch) / GET_MAX_HIT(ch) < 95)
                if (mob_heal_up(ch))
                    continue;

        /*
         * Advanced mob memory
         *
         * Attempt to summon victim, dim door to victim, or just fast track.
         */
        if (GET_SKILL(ch, SPELL_SUMMON) || GET_SKILL(ch, SPELL_DIMENSION_DOOR) || MOB_FLAGGED(ch, MOB_FAST_TRACK))
            if (mob_memory_action(ch, true))
                continue;
    }
}

/*
   This function is called every other combat round.  It allows NPCs who
   are in battle to attempt special skills, such as bash or spellcasting.
*/
void perform_mob_violence(void) {
    CharData *ch;

    for (ch = combat_list; ch; ch = next_combat_list) {
        next_combat_list = ch->next_fighting;
        if (IS_NPC(ch) && !ch->desc) {
            if (GET_POS(ch) > POS_KNEELING)
                GET_STANCE(ch) = STANCE_FIGHTING;

            if (GET_MOB_WAIT(ch) > 0)
                /* If the NPC is delayed, continue waiting */
                WAIT_STATE(ch, std::max((GET_MOB_WAIT(ch) - (PULSE_VIOLENCE / 2)), 0));
            else {
                /* This NPC is ready to perform actions */
                if (GET_POS(ch) < POS_STANDING) {
                    GET_POS(ch) = POS_STANDING;
                    GET_STANCE(ch) = STANCE_FIGHTING;
                    act("&0&3$n scrambles to $s feet!&0", true, ch, 0, 0, TO_ROOM);
                } else
                    mob_attack(ch, FIGHTING(ch));
            }
        }
    }
}

bool mob_movement(CharData *ch) {
    CharData *vict;
    int door = random_number(0, 18);

    if (door >= NUM_OF_DIRS || !CAN_GO(ch, door))
        return false;
    if (ROOM_FLAGGED(CH_NDEST(ch, door), ROOM_NOMOB) || ROOM_FLAGGED(CH_NDEST(ch, door), ROOM_DEATH))
        return false;
    if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && CH_DEST(ch, door)->zone != CH_ROOM(ch)->zone)
        return false;
    if (EFF_FLAGGED(ch, EFF_CHARM) && !MOB_FLAGGED(ch, MOB_ILLUSORY))
        return false;

    /* Don't wander off while someone is fighting you! */
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch)
            return false;

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_ISOLATION)) {
        act("$n looks vainly about for an exit.", true, ch, 0, 0, TO_ROOM);
        /* True since it did *something* (looking around) */
        return true;
    }

    /* If we made it through all the checks above, then move! */

    /* Mobs with misdirection like to be sneaky */
    if (EFF_FLAGGED(ch, EFF_MISDIRECTION)) {
        if (random_number(1, NUM_OF_DIRS) == 1) {
            /* Decided to stay but pretend to move */
            perform_move(ch, door, 1, true);
        } else {
            /* Decided to move */
            perform_move(ch, random_number(0, NUM_OF_DIRS - 1), 1, true);
            SET_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
            perform_move(ch, door, 1, false);
            REMOVE_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
        }
    } else
        perform_move(ch, door, 1, false);
    return true;
}

bool mob_assist(CharData *ch) {
    CharData *vict, *cvict = nullptr;

    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
        if (!will_assist(ch, vict))
            continue;
        /*
           if (ch == vict || !FIGHTING(vict) || ch == FIGHTING(vict))
           continue;
           if (FIGHTING(vict)->in_room != ch->in_room)
           continue;
           if (!IS_NPC(vict) || MOB_FLAGGED(vict, MOB_PLAYER_PHANTASM))
           continue;
           if (IS_NPC(FIGHTING(vict)) && !MOB_FLAGGED(FIGHTING(vict),
           MOB_PLAYER_PHANTASM)) continue;
         */
        if (GET_LEVEL(FIGHTING(vict)) <= 20)
            cvict = vict;
        else if (EFF_FLAGGED(FIGHTING(vict), EFF_FAMILIARITY) &&
                 random_number(1, 100 + GET_CHA(FIGHTING(vict)) - GET_CHA(vict)) < 50) {
            act("$n moves to join the fight, but gets a good look at $N and stops, "
                "confused.",
                true, ch, 0, FIGHTING(vict), TO_ROOM);
            act("You want to help, but $N looks like your friend too!", false, ch, 0, FIGHTING(vict), TO_CHAR);
            return false;
        } else {
            act("$n jumps to the aid of $N!", false, ch, 0, vict, TO_ROOM);
            mob_attack(ch, FIGHTING(vict));
            return true;
        }
    }
    if (cvict) {
        int chuckle = random_number(1, 10);
        if (chuckle == 1)
            act("$n watches the battle in amusement.", false, ch, 0, cvict, TO_ROOM);
        else if (chuckle == 2)
            act("$n chuckles as $e watches the fight.", false, ch, 0, cvict, TO_ROOM);
        else if (chuckle == 3) {
            act("$n takes note of your battle tactics.", false, ch, 0, FIGHTING(cvict), TO_CHAR);
            act("$n takes note of $N's battle tactics.", false, ch, 0, FIGHTING(cvict), TO_ROOM);
        }
    }

    return false;
}

void mob_attack(CharData *ch, CharData *victim) {
    if (CASTING(ch))
        return;

    /* Mesmerized or paralyzed mobs should not attack.
     * Mob memory attack seems to be by-passing attack_ok check */
    if (EFF_FLAGGED(ch, EFF_MESMERIZED) || EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS))
        return;

    /* Mob should not execute any special mobile AI procedures. */
    if (!MOB_FLAGGED(ch, MOB_NO_CLASS_AI)) {

        /* Attempt special class actions first. */
        switch (GET_CLASS(ch)) {
        case CLASS_RANGER:
        case CLASS_PALADIN:
        case CLASS_ANTI_PALADIN:
            if (cleric_ai_action(ch, victim))
                return;
            /* Otherwise go on down to warrior ai action. */
        case CLASS_WARRIOR:
        case CLASS_MONK:
        case CLASS_MERCENARY:
        case CLASS_BERSERKER:
            if (warrior_ai_action(ch, victim))
                return;
            break;
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_DIABOLIST:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            victim = weakest_attacker(ch, victim);
            if (cleric_ai_action(ch, victim))
                return;
            break;
        case CLASS_SORCERER:
        case CLASS_PYROMANCER:
        case CLASS_CRYOMANCER:
        case CLASS_NECROMANCER:
        case CLASS_ILLUSIONIST:
        case CLASS_CONJURER:
            victim = weakest_attacker(ch, victim);
            if (sorcerer_ai_action(ch, victim))
                return;
            break;
        case CLASS_ASSASSIN:
        case CLASS_THIEF:
        case CLASS_ROGUE:
            if (rogue_ai_action(ch, victim))
                return;
            break;
        case CLASS_BARD:
            if (bard_ai_action(ch, victim))
                return;
            break;
        }

        switch (GET_RACE(ch)) {
        /*
        New dragon race stuff?
        case RACE_DRAGON:
        */
        case RACE_DRAGON_GENERAL:
        case RACE_DRAGON_FIRE:
        case RACE_DRAGON_FROST:
        case RACE_DRAGON_ACID:
        case RACE_DRAGON_GAS:
        case RACE_DRAGON_LIGHTNING:
        case RACE_DRAGONBORN_FIRE:
        case RACE_DRAGONBORN_FROST:
        case RACE_DRAGONBORN_ACID:
        case RACE_DRAGONBORN_LIGHTNING:
        case RACE_DRAGONBORN_GAS:
        case RACE_DEMON:
            if (dragonlike_attack(ch))
                return;
            break;
        }
    }

    if (!FIGHTING(ch))
        attack(ch, victim);
}

void mob_scavenge(CharData *ch) {
    int best_value, value;
    ObjData *best_obj, *obj;

    /* If there's nothing to pick up, return.  50% chance otherwise. */
    if (!world[ch->in_room].contents || !random_number(0, 1))
        return;

    best_value = 0;
    best_obj = nullptr;

    /* Find the most desirable item in the room. */
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
        if (CAN_GET_OBJ(ch, obj) && (value = appraise_item(ch, obj)) > best_value) {
            best_obj = obj;
            best_value = value;
        }

    /* And then pick it up. */
    if (best_obj != nullptr) {
        GetContext *context = begin_get_transaction(ch);
        perform_get_from_room(context, best_obj);
        end_get_transaction(context, nullptr);
    }
}

/*
 * mob_attempt_equip
 *
 * Makes the mob attempt to wear an item in its inventory.
 * Returns 1 as soon as something is worn or removed.  0
 * otherwise.
 */
void mob_attempt_equip(CharData *ch) {
    int where;
    ObjData *obj;

    if (GET_RACE(ch) == RACE_ANIMAL)
        return;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (!CAN_SEE_OBJ(ch, obj))
            continue;

        where = find_eq_pos(ch, obj, nullptr);
        if (where >= 0) {
            if (GET_EQ(ch, where)) {
                if (appraise_item(ch, GET_EQ(ch, where)) >= appraise_item(ch, obj))
                    continue;
                perform_remove(ch, where);
            }
            perform_wear(ch, obj, where);
            continue;
        } else { /* where < 0 means not wearable */
                 /* Maybe handle other types of items here; eat food, I dunno */
        }
    }
}

/*
 * mob_memory_action
 *
 * Expects a mob with the MOB_MEMORY flag.
 */
bool mob_memory_action(CharData *ch, bool allow_spells) {
    DescriptorData *d, *next_d;
    MemoryRec *names;

    if (!MEMORY(ch))
        return false;

    /* Check everyone logged on against the memory. */
    for (d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if (STATE(d) != CON_PLAYING)
            continue;

        /* Check all the names in memory against this descriptor. */
        for (names = MEMORY(ch); names; names = names->next) {
            if (names->id != GET_IDNUM(d->character))
                continue;

            if (allow_spells && !MOB_FLAGGED(ch, MOB_NO_CLASS_AI)) {
                /* Now attempt spells that require the tracker and victim to be
                   in the same zone. */
                if (world[ch->in_room].zone == world[d->character->in_room].zone) {
                    /* First try to summon. */
                    if (GET_LEVEL(d->character) <= GET_SKILL(ch, SKILL_SPHERE_SUMMON) + 3)
                        if (mob_cast(ch, d->character, nullptr, SPELL_SUMMON))
                            return true;
                    /* Then try to dimension door. */
                    if (mob_cast(ch, d->character, nullptr, SPELL_DIMENSION_DOOR))
                        return true;
                }
            }

            if ((MOB_FLAGGED(ch, MOB_SLOW_TRACK) || MOB_FLAGGED(ch, MOB_FAST_TRACK)) &&
                !EVENT_FLAGGED(ch, EVENT_TRACK)) {
                do_track(ch, GET_NAMELIST(d->character), 0, 0);
                track_victim_check(ch, d->character);
                return true;
            }
        }
    }
    return false;
}

bool mob_memory_check(CharData *ch) {
    CharData *vict;
    MemoryRec *names;

    if (!MEMORY(ch))
        return false;

    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
        if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
            continue;

        for (names = MEMORY(ch); names; names = names->next) {
            if (names->id != GET_IDNUM(vict))
                continue;
            track_victim_check(ch, vict);
            return true;
        }
    }
    return false;
}

void memory_attack_announce(CharData *ch, CharData *vict) {
    /* No announcement if the mob is trying to be sneaky. */
    if (EFF_FLAGGED(ch, EFF_SNEAK) || GET_HIDDENNESS(ch) > 0)
        return;

    /* No announcement if the room is peaceful or mesmerized. (Because there's no attack.) */
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) || EFF_FLAGGED(ch, EFF_MESMERIZED))
        return;

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        act("$n silently moves in for the attack.", true, ch, 0, vict, TO_ROOM);
    } else if (GET_RACE(ch) == RACE_ANIMAL) {
        act("$n growls angrily at $N, and attacks!", false, ch, 0, vict, TO_NOTVICT);
        act("$n growls angrily at you, and attacks!", false, ch, 0, vict, TO_VICT);
    } else {
        switch (random_number(1, 10)) {
        case 1:
            act("$n growls, 'Thought you could walk away from a fight, eh?'", false, ch, 0, vict, TO_ROOM);
            break;
        case 2:
            /* Using GET_NAME because a mob wouldn't growl "someone" */
            sprintf(buf, "$n growls, 'This ends here and now, %s!'", GET_NAME(vict));
            act(buf, false, ch, 0, vict, TO_ROOM);
            break;
        case 10:
        default:
            act("$n snarls, 'You're not getting away that easily!'", false, ch, 0, vict, TO_ROOM);
            break;
            /*
             * Add more messages here for variety!
             */
        }
    }
}

void track_victim_check(CharData *ch, CharData *vict) {
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
        return;
    if (ch->in_room == vict->in_room || ((vict = find_char_around_char(ch, find_vis_by_name(ch, GET_NAME(vict)))) &&
                                         MOB_FLAGGED(vict, MOB_PLAYER_PHANTASM)))
        if (!CASTING(ch) && !FIGHTING(ch)) {
            memory_attack_announce(ch, vict);
            mob_attack(ch, vict);
        }
}

/* Add victim to ch's memory list. */
void remember(CharData *ch, CharData *victim) {
    MemoryRec *tmp;
    bool present = false;

    if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
        return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
        if (tmp->id == GET_IDNUM(victim))
            present = true;

    if (!present) {
        CREATE(tmp, MemoryRec, 1);
        tmp->next = MEMORY(ch);
        tmp->id = GET_IDNUM(victim);
        MEMORY(ch) = tmp;
    }
}

/* Make character forget a victim. */
void forget(CharData *ch, CharData *victim) {
    MemoryRec *curr, *prev = nullptr;

    if (!(curr = MEMORY(ch)))
        return;

    while (curr && curr->id != GET_IDNUM(victim)) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return; /* person wasn't there at all. */

    if (curr == MEMORY(ch))
        MEMORY(ch) = curr->next;
    else
        prev->next = curr->next;

    free(curr);
}

/* erase ch's memory */
void clear_memory(CharData *ch) {
    MemoryRec *curr, *next;

    curr = MEMORY(ch);

    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    MEMORY(ch) = nullptr;
}

/* Returns true if victim is in the mob's memory, false otherwise */
bool in_memory(CharData *ch, CharData *vict) {
    MemoryRec *memory;

    for (memory = MEMORY(ch); memory; memory = memory->next)
        if (memory->id == GET_IDNUM(vict))
            return true;

    return false;
}

/* This function checks mobs for spell ups */
bool check_mob_status(CharData *ch) {
    switch (GET_CLASS(ch)) {
    case CLASS_BARD:
        if (check_bard_status(ch))
            return true;
        break;
    case CLASS_SORCERER:
    case CLASS_PYROMANCER:
    case CLASS_CRYOMANCER:
    case CLASS_NECROMANCER:
    case CLASS_ILLUSIONIST:
        if (check_sorcerer_status(ch))
            return true;
        break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_PALADIN:
    case CLASS_ANTI_PALADIN:
        /* check_cleric_status is useless in combat */
        if (!FIGHTING(ch) && check_cleric_status(ch))
            return true;
        break;
    }
    return false;
}

bool check_spellbank(CharData *ch) {
    int i;
    // If any spell circles are not fully charged, start memming and return true
    for (i = 1; i < NUM_SPELL_CIRCLES; ++i)
        if (GET_MOB_SPLBANK(ch, i) < spells_of_circle[(int)GET_LEVEL(ch)][i]) {
            do_sit(ch, nullptr, 0, 0);
            if (GET_POS(ch) == POS_SITTING && GET_STANCE(ch) >= STANCE_RESTING && GET_STANCE(ch) <= STANCE_ALERT) {
                act("$n begins to meditate.", true, ch, 0, 0, TO_ROOM);
                // start_studying(ch);
                return true;
            }
        }
    return false;
}

void remove_from_all_memories(CharData *ch) {
    CharData *tch, *next_tch;

    for (tch = character_list; tch; tch = next_tch) {
        next_tch = tch->next;

        if (!MOB_FLAGGED(tch, MOB_MEMORY) || !MEMORY(tch))
            continue;
        if (tch == ch)
            continue;
        if (POSSESSED(tch))
            continue;
        else
            forget(tch, ch);
    }
}

bool dragonlike_attack(CharData *ch) {
    int roll = random_number(0, 150 - GET_LEVEL(ch));

    /* At level 100, 10% chance to breath, 2% chance for each different type */
    /*
    if (roll < 5 && (GET_SKILL(ch, SKILL_BREATHE)) {
    */
    if (roll < 5 &&
        (GET_SKILL(ch, SKILL_BREATHE_FIRE) || GET_SKILL(ch, SKILL_BREATHE_FROST) || GET_SKILL(ch, SKILL_BREATHE_ACID) ||
         GET_SKILL(ch, SKILL_BREATHE_GAS) || GET_SKILL(ch, SKILL_BREATHE_LIGHTNING))) {
        switch (GET_COMPOSITION(ch)) {
        case COMP_EARTH:
        case COMP_STONE:
            do_breathe(ch, "acid", 0, 0);
            break;
        case COMP_AIR:
        case COMP_ETHER:
            do_breathe(ch, "lightning", 0, 0);
            break;
        case COMP_FIRE:
        case COMP_LAVA:
            do_breathe(ch, "fire", 0, 0);
            break;
        case COMP_WATER:
        case COMP_ICE:
        case COMP_MIST:
            do_breathe(ch, "frost", 0, 0);
            break;
        case COMP_METAL:
        case COMP_BONE:
        case COMP_PLANT:
            do_breathe(ch, "gas", 0, 0);
            break;
        default:
            switch (roll) {
            case 0:
                do_breathe(ch, "fire", 0, 0);
                break;
            case 1:
                do_breathe(ch, "gas", 0, 0);
                break;
            case 2:
                do_breathe(ch, "frost", 0, 0);
                break;
            case 3:
                do_breathe(ch, "acid", 0, 0);
                break;
            case 4:
            default:
                do_breathe(ch, "lightning", 0, 0);
                break;
            }
        }
        return true;
    }

    /* At level 100, 10% chance to sweep */
    else if (roll < 10 && GET_SKILL(ch, SKILL_SWEEP)) {
        do_sweep(ch, "", 0, 0);
        return true;
    }

    /* At level 100, 10% chance to roar */
    else if (roll < 15 && GET_SKILL(ch, SKILL_ROAR)) {
        int cmd_roar = find_command("roar");
        /*
         * Since do_roar can map to do_action, we MUST find the correct
         * cmd value for roar here.  If we do not and the roar fails, it
         * may get passed to do_action, which will cause the game to
         * crash if passed an invalid cmd value.
         */
        do_roar(ch, "", cmd_roar, 0);
        return true;
    }
    return false;
}
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

#include "act.hpp" #include "ai.hpp"
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
#include "math.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "rooms.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* external structs */
extern struct char_data *combat_list; /* head of l-list of fighting chars */
extern struct char_data *next_combat_list;

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
bool update_inventory(char_data *ch, obj_data *obj, int where);
struct char_data *check_guard(char_data *ch, char_data *victim, int gag_output);
void get_check_money(char_data *ch, obj_data *obj);
void perform_wear(char_data *ch, obj_data *obj, int where);
int find_eq_pos(char_data *ch, obj_data *obj, char *arg);
bool mob_cast(char_data *ch, char_data *tch, obj_data *tobj, int spellnum);
void hunt_victim(char_data *ch);
void start_memming(char_data *ch);

/* Local functions */
bool check_mob_status(char_data *ch);
void remember(char_data *ch, char_data *victim);
void mob_attempt_equip(char_data *ch);
bool mob_memory_action(char_data *ch, bool allow_spells);
void memory_attack_announce(char_data *ch, char_data *victim);
void memory_attack(char_data *ch, char_data *vict);
void track_victim_check(char_data *ch, char_data *vict);
bool mob_memory_check(char_data *ch);
/* mobile_activity subfunctions */
void mob_scavenge(char_data *ch);
bool mob_movement(char_data *ch);
bool mob_assist(char_data *ch);
void mob_attack(char_data *ch, char_data *victim);
bool check_spellbank(char_data *ch);

void mobile_activity(void) {
    struct char_data *ch, *next_ch;
    extern int no_specials;

    for (ch = character_list; ch; ch = next_ch) {
        next_ch = ch->next;

        /* Players, sleeping mobs, and paralyzed, and casting mobs don't do
         * anything. */
        if (!IS_MOB(ch) || CASTING(ch) || !AWAKE(ch) || EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) ||
            EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MESMERIZED))
            continue;

        /* Don't execute procs when someone is switched in. */
        if (POSSESSED(ch) || MEMMING(ch))
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
            if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
                sprintf(buf, "%s (#%d): Attempting to call non-existing mob func", GET_NAME(ch), GET_MOB_VNUM(ch));
                log(buf);
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
        if (EFF_FLAGGED(ch, EFF_ON_FIRE) && number(0, 1)) {
            /* If the mob is flagged !CLASS_AI, then just douse, don't try to cast. */
            if (MOB_FLAGGED(ch, MOB_NO_CLASS_AI) ? TRUE : !mob_cast(ch, ch, NULL, SPELL_EXTINGUISH))
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
                if (GET_SKILL(ch, SKILL_STEAL) && number(0, 101) < GET_SKILL(ch, SKILL_STEAL))
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
            if (mob_memory_action(ch, FALSE))
                continue;
    }
}

/*
 * mobile_spec_activity
 *
 * This handles special mobile activity, called every PULSE_VIOLENCE.
 */
void mobile_spec_activity(void) {
    struct char_data *ch, *next_ch, *vict;
    int found = FALSE;

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
            WAIT_STATE(ch, MAX(GET_MOB_WAIT(ch) - PULSE_VIOLENCE, 0));
            continue;
        }

        /* Pets, courtesy of Banyal */
        if (PLAYERALLY(ch) && PRF_FLAGGED(ch->master, PRF_PETASSIST)) {
            found = FALSE;
            for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
                if (ch != vict && (ch->master == vict) && FIGHTING(vict) && ch != FIGHTING(vict)) {
                    act("&7$n assists $s master!&0", FALSE, ch, 0, vict, TO_ROOM);
                    attack(ch, check_guard(ch, FIGHTING(vict), FALSE));
                    found = TRUE;
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
                100 * GET_HIT(ch) / GET_MAX_HIT(ch) < 95)
                if (mob_heal_up(ch))
                    continue;

        /*
         * Advanced mob memory
         *
         * Attempt to summon victim, dim door to victim, or just fast track.
         */
        if (GET_SKILL(ch, SPELL_SUMMON) || GET_SKILL(ch, SPELL_DIMENSION_DOOR) || MOB_FLAGGED(ch, MOB_FAST_TRACK))
            if (mob_memory_action(ch, TRUE))
                continue;
    }
}

/*
   This function is called every other combat round.  It allows NPCs who
   are in battle to attempt special skills, such as bash or spellcasting.
*/
void perform_mob_violence(void) {
    struct char_data *ch;

    for (ch = combat_list; ch; ch = next_combat_list) {
        next_combat_list = ch->next_fighting;
        if (IS_NPC(ch) && !ch->desc) {
            if (GET_POS(ch) > POS_KNEELING)
                GET_STANCE(ch) = STANCE_FIGHTING;

            if (GET_MOB_WAIT(ch) > 0)
                /* If the NPC is delayed, continue waiting */
                WAIT_STATE(ch, MAX((GET_MOB_WAIT(ch) - (PULSE_VIOLENCE / 2)), 0));
            else {
                /* This NPC is ready to perform actions */
                if (GET_POS(ch) < POS_STANDING) {
                    GET_POS(ch) = POS_STANDING;
                    GET_STANCE(ch) = STANCE_FIGHTING;
                    act("&0&3$n scrambles to $s feet!&0", TRUE, ch, 0, 0, TO_ROOM);
                } else
                    mob_attack(ch, FIGHTING(ch));
            }
        }
    }
}

bool mob_movement(char_data *ch) {
    struct char_data *vict;
    int door = number(0, 18);

    if (door >= NUM_OF_DIRS || !CAN_GO(ch, door))
        return FALSE;
    if (ROOM_FLAGGED(CH_NDEST(ch, door), ROOM_NOMOB) || ROOM_FLAGGED(CH_NDEST(ch, door), ROOM_DEATH))
        return FALSE;
    if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && CH_DEST(ch, door)->zone != CH_ROOM(ch)->zone)
        return FALSE;
    if (EFF_FLAGGED(ch, EFF_CHARM) && !MOB_FLAGGED(ch, MOB_ILLUSORY))
        return FALSE;

    /* Don't wander off while someone is fighting you! */
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch)
            return FALSE;

    if (ROOM_EFF_FLAGGED(ch->in_room, ROOM_EFF_ISOLATION)) {
        act("$n looks vainly about for an exit.", TRUE, ch, 0, 0, TO_ROOM);
        /* True since it did *something* (looking around) */
        return TRUE;
    }

    /* If we made it through all the checks above, then move! */

    /* Mobs with misdirection like to be sneaky */
    if (EFF_FLAGGED(ch, EFF_MISDIRECTION)) {
        if (number(1, NUM_OF_DIRS) == 1) {
            /* Decided to stay but pretend to move */
            perform_move(ch, door, 1, TRUE);
        } else {
            /* Decided to move */
            perform_move(ch, number(0, NUM_OF_DIRS - 1), 1, TRUE);
            SET_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
            perform_move(ch, door, 1, FALSE);
            REMOVE_FLAG(EFF_FLAGS(ch), EFF_MISDIRECTING);
        }
    } else
        perform_move(ch, door, 1, FALSE);
    return TRUE;
}

bool mob_assist(char_data *ch) {
    struct char_data *vict, *cvict = NULL;

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
                 number(1, 100 + GET_CHA(FIGHTING(vict)) - GET_CHA(vict)) < 50) {
            act("$n moves to join the fight, but gets a good look at $N and stops, "
                "confused.",
                TRUE, ch, 0, FIGHTING(vict), TO_ROOM);
            act("You want to help, but $N looks like your friend too!", FALSE, ch, 0, FIGHTING(vict), TO_CHAR);
            return FALSE;
        } else {
            act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
            mob_attack(ch, FIGHTING(vict));
            return TRUE;
        }
    }
    if (cvict) {
        int chuckle = number(1, 10);
        if (chuckle == 1)
            act("$n watches the battle in amusement.", FALSE, ch, 0, cvict, TO_ROOM);
        else if (chuckle == 2)
            act("$n chuckles as $e watches the fight.", FALSE, ch, 0, cvict, TO_ROOM);
        else if (chuckle == 3) {
            act("$n takes note of your battle tactics.", FALSE, ch, 0, FIGHTING(cvict), TO_CHAR);
            act("$n takes note of $N's battle tactics.", FALSE, ch, 0, FIGHTING(cvict), TO_ROOM);
        }
    }

    return FALSE;
}

void mob_attack(char_data *ch, char_data *victim) {
    if (CASTING(ch))
        return;

    /* Mesmerized or paralyzed mobs should not attack.
     * Mob memory attack seems to be by-passing attack_ok check */
    if (EFF_FLAGGED(ch, EFF_MESMERIZED) || EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS))
        return;

    /* See if anyone is guarding the victim.
     * But guarding doesn't apply if this NPC was already fighting the victim. */
    if (FIGHTING(ch) != victim)
        victim = check_guard(ch, victim, FALSE);

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

void mob_scavenge(char_data *ch) {
    int best_value, value;
    struct obj_data *best_obj, *obj;

    /* If there's nothing to pick up, return.  50% chance otherwise. */
    if (!world[ch->in_room].contents || !number(0, 1))
        return;

    best_value = 0;
    best_obj = NULL;

    /* Find the most desirable item in the room. */
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
        if (CAN_GET_OBJ(ch, obj) && (value = appraise_item(ch, obj)) > best_value) {
            best_obj = obj;
            best_value = value;
        }

    /* And then pick it up. */
    if (best_obj != NULL) {
        struct get_context *context = begin_get_transaction(ch);
        perform_get_from_room(context, best_obj);
        end_get_transaction(context, NULL);
    }
}

/*
 * mob_attempt_equip
 *
 * Makes the mob attempt to wear an item in its inventory.
 * Returns 1 as soon as something is worn or removed.  0
 * otherwise.
 */
void mob_attempt_equip(char_data *ch) {
    int where;
    struct obj_data *obj;

    if (GET_RACE(ch) == RACE_ANIMAL)
        return;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (!CAN_SEE_OBJ(ch, obj))
            continue;

        where = find_eq_pos(ch, obj, NULL);
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
bool mob_memory_action(char_data *ch, bool allow_spells) {
    struct descriptor_data *d, *next_d;
    memory_rec *names;

    if (!MEMORY(ch))
        return FALSE;

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
                        if (mob_cast(ch, d->character, NULL, SPELL_SUMMON))
                            return TRUE;
                    /* Then try to dimension door. */
                    if (mob_cast(ch, d->character, NULL, SPELL_DIMENSION_DOOR))
                        return TRUE;
                }
            }

            if ((MOB_FLAGGED(ch, MOB_SLOW_TRACK) || MOB_FLAGGED(ch, MOB_FAST_TRACK)) &&
                !EVENT_FLAGGED(ch, EVENT_TRACK)) {
                do_track(ch, GET_NAMELIST(d->character), 0, 0);
                track_victim_check(ch, d->character);
                return TRUE;
            }
        }
    }
    return FALSE;
}

bool mob_memory_check(char_data *ch) {
    struct char_data *vict;
    memory_rec *names;

    if (!MEMORY(ch))
        return FALSE;

    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
        if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
            continue;

        for (names = MEMORY(ch); names; names = names->next) {
            if (names->id != GET_IDNUM(vict))
                continue;
            track_victim_check(ch, vict);
            return TRUE;
        }
    }
    return FALSE;
}

void memory_attack_announce(char_data *ch, char_data *vict) {
    /* No announcement if the mob is trying to be sneaky. */
    if (EFF_FLAGGED(ch, EFF_SNEAK) || GET_HIDDENNESS(ch) > 0)
        return;

    /* No announcement if the room is peaceful or mesmerized. (Because there's no attack.) */
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) || EFF_FLAGGED(ch, EFF_MESMERIZED))
        return;

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        act("$n silently moves in for the attack.", TRUE, ch, 0, vict, TO_ROOM);
    } else if (GET_RACE(ch) == RACE_ANIMAL) {
        act("$n growls angrily at $N, and attacks!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n growls angrily at you, and attacks!", FALSE, ch, 0, vict, TO_VICT);
    } else {
        switch (number(1, 10)) {
        case 1:
            act("$n growls, 'Thought you could walk away from a fight, eh?'", FALSE, ch, 0, vict, TO_ROOM);
            break;
        case 2:
            /* Using GET_NAME because a mob wouldn't growl "someone" */
            sprintf(buf, "$n growls, 'This ends here and now, %s!'", GET_NAME(vict));
            act(buf, FALSE, ch, 0, vict, TO_ROOM);
            break;
        case 10:
        default:
            act("$n snarls, 'You're not getting away that easily!'", FALSE, ch, 0, vict, TO_ROOM);
            break;
            /*
             * Add more messages here for variety!
             */
        }
    }
}

void track_victim_check(char_data *ch, char_data *vict) {
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
void remember(char_data *ch, char_data *victim) {
    memory_rec *tmp;
    bool present = FALSE;

    if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
        return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
        if (tmp->id == GET_IDNUM(victim))
            present = TRUE;

    if (!present) {
        CREATE(tmp, memory_rec, 1);
        tmp->next = MEMORY(ch);
        tmp->id = GET_IDNUM(victim);
        MEMORY(ch) = tmp;
    }
}

/* Make character forget a victim. */
void forget(char_data *ch, char_data *victim) {
    memory_rec *curr, *prev = NULL;

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
void clear_memory(char_data *ch) {
    memory_rec *curr, *next;

    curr = MEMORY(ch);

    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    MEMORY(ch) = NULL;
}

/* Returns true if victim is in the mob's memory, false otherwise */
bool in_memory(char_data *ch, char_data *vict) {
    memory_rec *memory;

    for (memory = MEMORY(ch); memory; memory = memory->next)
        if (memory->id == GET_IDNUM(vict))
            return TRUE;

    return FALSE;
}

/* This function checks mobs for spell ups */
bool check_mob_status(char_data *ch) {
    switch (GET_CLASS(ch)) {
    case CLASS_BARD:
        if (check_bard_status(ch))
            return TRUE;
        break;
    case CLASS_SORCERER:
    case CLASS_PYROMANCER:
    case CLASS_CRYOMANCER:
    case CLASS_NECROMANCER:
    case CLASS_ILLUSIONIST:
        if (check_sorcerer_status(ch))
            return TRUE;
        break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_PALADIN:
    case CLASS_ANTI_PALADIN:
        /* check_cleric_status is useless in combat */
        if (!FIGHTING(ch) && check_cleric_status(ch))
            return TRUE;
        break;
    }
    return FALSE;
}

bool check_spellbank(char_data *ch) {
    int i;
    /* If any spell circles are not fully charged, start memming and
     * return TRUE */
    for (i = 1; i < NUM_SPELL_CIRCLES; ++i)
        if (GET_MOB_SPLBANK(ch, i) < spells_of_circle[(int)GET_LEVEL(ch)][i]) {
            do_sit(ch, NULL, 0, 0);
            if (GET_POS(ch) == POS_SITTING && GET_STANCE(ch) >= STANCE_RESTING && GET_STANCE(ch) <= STANCE_ALERT) {
                act(MEM_MODE(ch) == PRAY ? "$n begins praying to $s deity."
                                         : "$n takes out $s books and begins to study.",
                    TRUE, ch, 0, 0, TO_ROOM);
                start_memming(ch);
                return TRUE;
            }
        }
    return FALSE;
}

void remove_from_all_memories(char_data *ch) {
    struct char_data *tch, *next_tch;

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

bool dragonlike_attack(char_data *ch) {
    int roll = number(0, 150 - GET_LEVEL(ch));

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
        return TRUE;
    }

    /* At level 100, 10% chance to sweep */
    else if (roll < 10 && GET_SKILL(ch, SKILL_SWEEP)) {
        do_sweep(ch, "", 0, 0);
        return TRUE;
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
        return TRUE;
    }
    return FALSE;
}
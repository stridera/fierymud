/***************************************************************************
 *   File: warrior.c                                      Part of FieryMUD *
 *  Usage: controls Warrior type mobs, it is closly related to ai.h,       *
 *         and ai_util.c                                                   *
 *     By: Proky of HubisMUD                                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "ai.hpp"
#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "events.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Commands */
ACMD(do_backstab);
ACMD(do_bash);
ACMD(do_hitall);
ACMD(do_kick);
ACMD(do_rescue);
ACMD(do_roundhouse);

/* External functions */
bool has_piercing_weapon(CharData *ch);

/*
 * warrior_ai_action
 *
 * Basic warrior mob AI.  Returns true if an action is taken, false
 * otherwise.
 */
bool warrior_ai_action(CharData *ch, CharData *victim) {
    int roll, i;
    CharData *tch;

    if (!victim) {
        log(LogSeverity::Stat, LVL_GOD, "No victim in warrior AI action.");
        return false;
    }

    /*
     * BASH
     *
     * If someone is
     *   + in the room,
     *   + grouped with the tank,
     *   + casting
     *   + a player
     *   + visible to the mob
     *   + within bashing size
     * then attempt to bash them first.
     */
    if (!EFF_FLAGGED(ch, EFF_BLIND) && GET_SKILL(ch, SKILL_BASH) && FIGHTING(ch) && GET_EQ(ch, WEAR_SHIELD) &&
        (random_number(25, 60) - GET_SKILL(ch, SKILL_BASH)) <= 0)
        for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
            i = GET_SIZE(ch) - GET_SIZE(tch);
            if (is_grouped(FIGHTING(ch), tch) && !IS_NPC(tch) && CAN_SEE(ch, tch) && CASTING(tch) && i <= 2 &&
                i >= -1) {
                do_bash(ch, GET_NAME(tch), 0, 0);
                return true;
            }
        }

    /*
     * BREATHE / SWEEP / ROAR
     *
     * If the mob is a dragon/demon try to use dragon/demon skills.
     * There is a 15% chance of something occurring in this function,
     * if the mobile has the skills.
     */
    if (dragonlike_attack(ch))
        return true;

    /*
     * KICK / BASH / HITALL / BODYSLAM / BACKSTAB (for mercs) / RESCUE
     *
     * Random chance to bash and kick proportional to level.
     * Increased chance to bash mages and clerics.
     */
    roll = random_number(0, 101);
    if (roll < GET_LEVEL(ch)) {

        roll *= 100 / GET_LEVEL(ch);
        i = GET_SIZE(ch) - GET_SIZE(victim);
        if (CAN_SEE(ch, victim) && roll < GET_LEVEL(ch) && GET_SKILL(ch, SKILL_BODYSLAM) && !FIGHTING(ch) &&
            GET_POS(victim) >= POS_STANDING) {
            do_bash(ch, GET_NAME(victim), 0, SCMD_BODYSLAM);
            return true;
        }
        if (roll > 75 && GET_SKILL(ch, SKILL_HITALL) && FIGHTING(ch)) {
            do_hitall(ch, "", 0, SCMD_HITALL);
            return true;
        }
        if (roll > 75 && GET_SKILL(ch, SKILL_ROUNDHOUSE) && FIGHTING(ch)) {
            do_roundhouse(ch, "", 0, 0);
            return true;
        }
        /*
         * BACKSTAB
         *
         * Mercenaries share a lot of skills with warriors, but they have
         * backstab too.  Attempt to start combat using backstab.
         */
        if (CAN_SEE(ch, victim) && roll > 95 && GET_SKILL(ch, SKILL_BACKSTAB) && has_piercing_weapon(ch) &&
            !is_tanking(ch)) {
            do_backstab(ch, GET_NAME(victim), 0, 0);
            return true;
        }
        /*
         * To attempt a bash we need a) skill in bash, b) a shield, c)
         * opponent standing, d) realistic size, and e) opponent not !bash.
         * If the opponent is a caster, increased chance to attempt a bash.
         * Higher level mobs get a better chance of attempting a bash.
         * (A level 99 mob has about a 50% chance of attempting to bash a
         * warrior opponent.)
         */
        if (CAN_SEE(ch, victim) &&
            roll > (80 - GET_SKILL(ch, SKILL_BASH) / 2 - (classes[(int)GET_CLASS(victim)].magical ? 20 : 0)) &&
            GET_SKILL(ch, SKILL_BASH) && GET_EQ(ch, WEAR_SHIELD) && GET_POS(victim) >= POS_STANDING && i <= 2 &&
            i > -1 && !MOB_FLAGGED(victim, MOB_NOBASH)) {
            do_bash(ch, GET_NAME(victim), 0, SCMD_BASH);
            return true;
        }
        /* This would probably not work very well, because the GET_NAME refers to
           a mob, and we don't know if it's 1.guard or 2.guard etc.  Skills need
           to be re-thought-out for use with mobs.  Maybe this is v3 territory.
            else if (GET_SKILL(ch, SKILL_RESCUE) &&
                    FIGHTING(ch) && FIGHTING(victim) &&
                    roll > 65 &&
                    (tch = FIGHTING(victim)) != ch &&
                    GET_LEVEL(tch) < GET_LEVEL(ch) + 5 &&
                    GET_HIT(tch) < GET_MAX_HIT(tch) / 4 &&
                    GET_HIT(ch) > GET_MAX_HIT(ch) / 2 &&
                    GET_HIT(ch) > GET_HIT(tch)) {
              do_rescue(ch, GET_NAME(tch), 0, 0);
              return true;
            }
        */
        if (GET_SKILL(ch, SKILL_KICK)) {
            do_kick(ch, GET_NAME(victim), 0, 0);
            return true;
        }
    }

    return false;
}

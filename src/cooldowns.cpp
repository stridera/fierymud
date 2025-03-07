/***************************************************************************
 *  File: cooldowns.c                                    Part of FieryMUD  *
 *  Usage: Skill cooldowns                                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "cooldowns.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "fight.hpp"
#include "logging.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

const std::string_view cooldowns[NUM_COOLDOWNS + 1] = {"backstab",
                                                       "bash",
                                                       "instant kill",
                                                       "disarm",
                                                       "fumbling primary weapon",
                                                       "dropped primary weapon",
                                                       "fumbling secondary weapon",
                                                       "dropped secondary weapon",
                                                       "summon mount",
                                                       "lay hands",
                                                       "first aid",
                                                       "eye gouge",
                                                       "throatcut",
                                                       "shapechange",
                                                       "defense chant",
                                                       "innate invis",
                                                       "innate chaz",
                                                       "innate darkness",
                                                       "innate feather fall",
                                                       "innate syll",
                                                       "innate tren",
                                                       "innate tass",
                                                       "innate brill",
                                                       "innate ascen",
                                                       "innate harness",
                                                       "breathe",
                                                       "innate create",
                                                       "innate illumination",
                                                       "innate faerie step",
                                                       "music 1",
                                                       "music 2",
                                                       "music 3",
                                                       "music 4",
                                                       "music 5",
                                                       "music 6",
                                                       "music 7",
                                                       "innate blinding beauty",
                                                       "innate statue",
                                                       "innate barkskin",
                                                       "offense chant",
                                                       "\n"};

void cooldown_wearoff(CharData *ch, int cooldown) {
    switch (cooldown) {
    case CD_DROPPED_PRIMARY:
    case CD_DROPPED_SECONDARY:
        if (IS_NPC(ch))
            pickup_dropped_weapon(ch);
        break;
    case CD_FUMBLING_PRIMARY:
    case CD_FUMBLING_SECONDARY:
        act("$n finally regains control of $s weapon.", false, ch, 0, 0, TO_ROOM);
        act("You finally regain control of your weapon.", false, ch, 0, 0, TO_CHAR);
        break;
    }
}

EVENTFUNC(cooldown_handler) {
    CharData *ch = (CharData *)event_obj;
    int i, found = false;

    for (i = 0; i < NUM_COOLDOWNS; ++i) {
        if (GET_COOLDOWN(ch, i)) {
            /* Decrement cooldown counter */
            if ((GET_COOLDOWN(ch, i) -= PULSE_COOLDOWN) > 0) {
                /* Cooldown still nonzero */
                found = true;
                continue;
            } else
                /* Set to zero in case it went negative */
                GET_COOLDOWN(ch, i) = 0;
            /* Cooldown has just worn off */
            cooldown_wearoff(ch, i);
        }
    }

    if (found)
        return PULSE_COOLDOWN;

    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_COOLDOWN);
    return EVENT_FINISHED;
}

void SET_COOLDOWN(CharData *ch, int type, int amount) {
    GET_COOLDOWN(ch, type) = amount;
    GET_COOLDOWN_MAX(ch, type) = amount;

    if (amount && !EVENT_FLAGGED(ch, EVENT_COOLDOWN)) {
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_COOLDOWN);
        event_create(EVENT_COOLDOWN, cooldown_handler, ch, false, &ch->events, PULSE_COOLDOWN);
    }
}

void clear_cooldowns(CharData *ch) {
    int i;
    for (i = 0; i < NUM_COOLDOWNS; ++i)
        GET_COOLDOWN(ch, i) = 0;
    cancel_event(GET_EVENTS(ch), EVENT_COOLDOWN);
}
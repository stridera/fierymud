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

#include "cooldowns.h"

#include "comm.h"
#include "conf.h"
#include "fight.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

const char *cooldowns[NUM_COOLDOWNS + 1] = {"backstab",
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
                                            "chant",
                                            "innate invis",
                                            "innate chaz",
                                            "innate darkness",
                                            "innate levitate",
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
                                            "innate blinding beauty"
                                            "\n"};

void cooldown_wearoff(struct char_data *ch, int cooldown) {
    switch (cooldown) {
    case CD_DROPPED_PRIMARY:
    case CD_DROPPED_SECONDARY:
        if (IS_NPC(ch))
            pickup_dropped_weapon(ch);
        break;
    case CD_FUMBLING_PRIMARY:
    case CD_FUMBLING_SECONDARY:
        act("$n finally regains control of $s weapon.", FALSE, ch, 0, 0, TO_ROOM);
        act("You finally regain control of your weapon.", FALSE, ch, 0, 0, TO_CHAR);
        break;
    }
}

EVENTFUNC(cooldown_handler) {
    struct char_data *ch = (struct char_data *)event_obj;
    int i, found = FALSE;

    for (i = 0; i < NUM_COOLDOWNS; ++i) {
        if (GET_COOLDOWN(ch, i)) {
            /* Decrement cooldown counter */
            if ((GET_COOLDOWN(ch, i) -= PULSE_COOLDOWN) > 0) {
                /* Cooldown still nonzero */
                found = TRUE;
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

void SET_COOLDOWN(struct char_data *ch, int type, int amount) {
    GET_COOLDOWN(ch, type) = amount;
    GET_COOLDOWN_MAX(ch, type) = amount;

    if (amount && !EVENT_FLAGGED(ch, EVENT_COOLDOWN)) {
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_COOLDOWN);
        event_create(EVENT_COOLDOWN, cooldown_handler, ch, FALSE, &ch->events, PULSE_COOLDOWN);
    }
}

void clear_cooldowns(struct char_data *ch) {
    int i;
    for (i = 0; i < NUM_COOLDOWNS; ++i)
        GET_COOLDOWN(ch, i) = 0;
    cancel_event(GET_EVENTS(ch), EVENT_COOLDOWN);
}
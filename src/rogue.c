/***************************************************************************
 *   File: rogue.c                                        Part of FieryMUD *
 *  Usage: Control of rogue type mobs, It is closly related to ai.h,       *
 *         and ai_util.c.                                                  *
 *     By: Proky of HubisMUD                                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "ai.h"
#include "casting.h"
#include "comm.h"
#include "conf.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* External functions */
ACMD(do_backstab);
ACMD(do_corner);
ACMD(do_eye_gouge);
ACMD(do_kick);
ACMD(do_steal);
ACMD(do_throatcut);

bool has_piercing_weapon(struct char_data *ch) {
    if (GET_EQ(ch, WEAR_WIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD)))
        return TRUE;
    if (GET_EQ(ch, WEAR_WIELD2) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD2)))
        return TRUE;
    if (GET_EQ(ch, WEAR_2HWIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_2HWIELD)))
        return TRUE;
    return FALSE;
}

bool rogue_ai_action(struct char_data *ch, struct char_data *victim) {
    int roll;

    if (!victim) {
        mudlog("No victim in rogue AI action.", NRM, LVL_GOD, FALSE);
        return FALSE;
    }

    /* Success in doing an action? */
    roll = number(0, 101);
    if (roll >= GET_LEVEL(ch))
        return FALSE;
    roll *= 100 / GET_LEVEL(ch);

    /*
     * Backstab requires a piercing weapon.
     */
    if (CAN_SEE(ch, victim) && roll > 95 && GET_SKILL(ch, SKILL_BACKSTAB) && has_piercing_weapon(ch) &&
        !is_tanking(ch)) {
        do_backstab(ch, GET_NAME(victim), 0, 0);
        return TRUE;
    }

    if (CAN_SEE(ch, victim) && roll > 94 && GET_SKILL(ch, SKILL_THROATCUT) && has_piercing_weapon(ch) &&
        !FIGHTING(ch)) {
        do_throatcut(ch, GET_NAME(victim), 0, 0);
        return TRUE;
    }

    if (CAN_SEE(ch, victim) && roll > 70 && GET_SKILL(ch, SKILL_CORNER) && FIGHTING(ch) == victim && !ch->cornering) {
        do_corner(ch, GET_NAME(victim), 0, 0);
        return TRUE;
    }

    if (CAN_SEE(ch, victim) && roll > 50 && GET_SKILL(ch, SKILL_EYE_GOUGE)) {
        do_eye_gouge(ch, GET_NAME(victim), 0, 0);
        return TRUE;
    }

    if (GET_SKILL(ch, SKILL_KICK)) {
        do_kick(ch, GET_NAME(victim), 0, 0);
        return TRUE;
    }

    return FALSE;
}

bool mob_steal(struct char_data *ch) {
    struct char_data *vict = get_random_char_around(ch, RAND_AGGRO | RAND_PLAYERS);

    if (vict && GET_LEVEL(ch) + 5 > GET_LEVEL(vict)) {
        if (vict->carrying && CAN_SEE_OBJ(ch, vict->carrying) && number(0, 1))
            sprintf(buf1, "%s %s", fname(vict->carrying->short_description), GET_NAME(vict));
        else
            sprintf(buf1, "%s %s", "coins", GET_NAME(vict));
        do_steal(ch, buf1, 0, 0);
        return TRUE;
    }

    return FALSE;
}

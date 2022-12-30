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

#include "rogue.hpp"

#include "ai.hpp"
#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* External functions */
ACMD(do_backstab);
ACMD(do_corner);
ACMD(do_eye_gouge);
ACMD(do_kick);
ACMD(do_steal);
ACMD(do_throatcut);

bool has_piercing_weapon(CharData *ch) {
    if (GET_EQ(ch, WEAR_WIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD)))
        return true;
    if (GET_EQ(ch, WEAR_WIELD2) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD2)))
        return true;
    if (GET_EQ(ch, WEAR_2HWIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_2HWIELD)))
        return true;
    return false;
}

bool rogue_ai_action(CharData *ch, CharData *victim) {
    int roll;

    if (!victim) {
        log(LogSeverity::Stat, LVL_GOD, "No victim in rogue AI action.");
        return false;
    }

    /* Success in doing an action? */
    roll = number(0, 101);
    if (roll >= GET_LEVEL(ch))
        return false;
    roll *= 100 / GET_LEVEL(ch);

    /*
     * Backstab requires a piercing weapon.
     */
    if (CAN_SEE(ch, victim) && roll > 95 && GET_SKILL(ch, SKILL_BACKSTAB) && has_piercing_weapon(ch) &&
        !is_tanking(ch)) {
        do_backstab(ch, GET_NAME(victim), 0, 0);
        return true;
    }

    if (CAN_SEE(ch, victim) && roll > 94 && GET_SKILL(ch, SKILL_THROATCUT) && has_piercing_weapon(ch) &&
        !FIGHTING(ch)) {
        do_throatcut(ch, GET_NAME(victim), 0, 0);
        return true;
    }

    if (CAN_SEE(ch, victim) && roll > 70 && GET_SKILL(ch, SKILL_CORNER) && FIGHTING(ch) == victim && !ch->cornering) {
        do_corner(ch, GET_NAME(victim), 0, 0);
        return true;
    }

    if (CAN_SEE(ch, victim) && roll > 50 && GET_SKILL(ch, SKILL_EYE_GOUGE)) {
        do_eye_gouge(ch, GET_NAME(victim), 0, 0);
        return true;
    }

    if (GET_SKILL(ch, SKILL_KICK)) {
        do_kick(ch, GET_NAME(victim), 0, 0);
        return true;
    }

    return false;
}

bool mob_steal(CharData *ch) {
    CharData *vict = get_random_char_around(ch, RAND_AGGRO | RAND_PLAYERS);

    if (vict && GET_LEVEL(ch) + 5 > GET_LEVEL(vict)) {
        if (vict->carrying && CAN_SEE_OBJ(ch, vict->carrying) && number(0, 1))
            sprintf(buf1, "%s %s", fname(vict->carrying->short_description), GET_NAME(vict));
        else
            sprintf(buf1, "%s %s", "coins", GET_NAME(vict));
        do_steal(ch, buf1, 0, 0);
        return true;
    }

    return false;
}

/* Bard combat AI */

int mob_cast(CharData *ch, CharData *tch, ObjData *tobj, int spellnum);

/*
 * bard_ai_action
 *
 *
 */
bool bard_ai_action(CharData *ch, CharData *victim) {
    int my_health, victim_health, i, counter, action = 0, roll;

    if (!victim) {
        log(LogSeverity::Stat, LVL_GOD, "No victim in bard AI action.");
        return false;
    }

    /* Success in doing an action? */
    roll = number(0, 101);
    if (roll >= GET_LEVEL(ch))
        return false;
    roll *= 100 / GET_LEVEL(ch);

    /*
     * Backstab requires a piercing weapon.
     */
    if (CAN_SEE(ch, victim) && roll > 95 && GET_SKILL(ch, SKILL_BACKSTAB) && has_piercing_weapon(ch) &&
        !is_tanking(ch)) {
        do_backstab(ch, GET_NAME(victim), 0, 0);
        return true;
    }

    /* Well no chance of casting any spells today. */
    if (EFF_FLAGGED(ch, EFF_SILENCE))
        return false;

    /* Calculate mob health as a percentage. */
    my_health = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
    victim_health = (100 * GET_HIT(victim)) / GET_MAX_HIT(victim);

    if ((my_health < 30 && victim_health > 20) && mob_heal_up(ch))
        return true;

    /* Otherwise kill or harm in some fashion */

    /* If the victim is grouped, then try an area spell. */
    if (group_size(victim) > 1) {
        counter = 0;
        for (i = 0; mob_bard_area_spells[i]; i++) {
            if (!GET_SKILL(ch, mob_bard_area_spells[i]))
                continue;
            if (mob_cast(ch, victim, nullptr, mob_bard_area_spells[i]))
                return true;
            /* Only try the mob's best two spells. */
            if (++counter >= 2)
                break;
        }
    }

    /* Try to cause an offensive affection. Only attempt one. */
    for (i = 0; mob_bard_hindrances[i].spell; i++) {
        if (!GET_SKILL(ch, mob_bard_hindrances[i].spell))
            continue;
        if (!has_effect(victim, &mob_bard_hindrances[i])) {
            if (mob_cast(ch, victim, nullptr, mob_bard_hindrances[i].spell))
                return true;
            else
                break;
        }
    }

    counter = 0;
    /* Single-target offensive spell */
    for (i = 0; mob_bard_offensives[i]; i++) {
        if (!GET_SKILL(ch, mob_bard_offensives[i]))
            continue;

        if (mob_cast(ch, victim, nullptr, mob_bard_offensives[i]))
            return true;

        /* Only attempt to cast this mob's best 3 spells. */
        if (++counter >= 3)
            break;
    }

    return false;
}

/*
 * check_bard_status
 *
 * Makes the bard mob check its spells.
 */
bool check_bard_status(CharData *ch) {
    int i;

    /* Check bad affects */
    for (i = 0; mob_bard_hindrances[i].spell; i++) {
        if (!GET_SKILL(ch, mob_bard_hindrances[i].spell))
            continue;

        /* If the spell can be removed and the mob has it, try to remove it */
        if (mob_bard_hindrances[i].remover && has_effect(ch, &mob_bard_hindrances[i]))
            if (mob_cast(ch, ch, nullptr, mob_bard_hindrances[i].remover))
                return true;
        /* 10% chance to cancel if in combat. */
        if (FIGHTING(ch) && !number(0, 9))
            return false;
    }

    /* Check other spells */
    for (i = 0; mob_bard_buffs[i].spell; i++) {
        if (!GET_SKILL(ch, mob_bard_buffs[i].spell) || !check_fluid_spell_ok(ch, ch, mob_bard_buffs[i].spell, true))
            continue;

        if (has_effect(ch, &mob_bard_buffs[i]))
            continue;

        if (mob_cast(ch, ch, nullptr, mob_bard_buffs[i].spell))
            return true;
    }

    return false;
}

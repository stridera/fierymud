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

#include "ai.hpp"
#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
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

bool has_piercing_weapon(char_data *ch) {
    if (GET_EQ(ch, WEAR_WIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD)))
        return TRUE;
    if (GET_EQ(ch, WEAR_WIELD2) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_WIELD2)))
        return TRUE;
    if (GET_EQ(ch, WEAR_2HWIELD) && IS_WEAPON_PIERCING(GET_EQ(ch, WEAR_2HWIELD)))
        return TRUE;
    return FALSE;
}

bool rogue_ai_action(char_data *ch, char_data *victim) {
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

bool mob_steal(char_data *ch) {
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

/* Bard combat AI */

int mob_cast(char_data *ch, char_data *tch, obj_data *tobj, int spellnum);

/* Bard spell lists */
const struct spell_pair mob_bard_buffs[] = {
    {SPELL_DETECT_INVIS, 0, EFF_DETECT_INVIS}, {SPELL_HASTE, 0, EFF_HASTE}, {0, 0, 0}};

/* These spells should all be castable in combat. */
const struct spell_pair mob_bard_hindrances[] = {{SPELL_BLINDNESS, SPELL_CURE_BLIND, EFF_BLIND},
                                                 {SPELL_WEB, SPELL_REMOVE_PARALYSIS, EFF_IMMOBILIZED},
                                                 {SPELL_INSANITY, SPELL_SANE_MIND, EFF_INSANITY},
                                                 {SPELL_POISON, SPELL_REMOVE_POISON, EFF_POISON},
                                                 {SPELL_DISEASE, SPELL_HEAL, EFF_DISEASE},
                                                 {SPELL_SILENCE, 0, 0}, /* Try to cast this, but there's no cure */
                                                 {SPELL_CURSE, SPELL_REMOVE_CURSE, EFF_CURSE},
                                                 {SPELL_ENTANGLE, SPELL_REMOVE_PARALYSIS, 0},
                                                 {0, 0, 0}};

const int mob_bard_offensives[] = {
    SPELL_VICIOUS_MOCKERY, SPELL_HARM,          SPELL_SHOCKING_GRASP, SPELL_CAUSE_CRITIC, SPELL_CHILL_TOUCH,
    SPELL_CAUSE_SERIOUS,   SPELL_BURNING_HANDS, SPELL_MAGIC_MISSILE,  SPELL_CAUSE_LIGHT,  0};

const int mob_bard_area_spells[] = {SPELL_CLOUD_OF_DAGGERS, SPELL_COLOR_SPRAY, 0};

const int mob_bard_heals[] = {SPELL_HEAL, SPELL_CURE_CRITIC, SPELL_CURE_SERIOUS, SPELL_CURE_LIGHT, 0};

/*
 * bard_ai_action
 *
 *
 */
bool bard_ai_action(char_data *ch, char_data *victim) {
    int my_health, victim_health, i, counter, action = 0, roll;

    if (!victim) {
        mudlog("No victim in bard AI action.", NRM, LVL_GOD, FALSE);
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

    /* Well no chance of casting any spells today. */
    if (EFF_FLAGGED(ch, EFF_SILENCE))
        return FALSE;

    /* Calculate mob health as a percentage. */
    my_health = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
    victim_health = (100 * GET_HIT(victim)) / GET_MAX_HIT(victim);

    if ((my_health < 30 && victim_health > 20) && mob_heal_up(ch))
        return TRUE;

    /* Otherwise kill or harm in some fashion */

    /* If the victim is grouped, then try an area spell. */
    if (group_size(victim) > 1) {
        counter = 0;
        for (i = 0; mob_bard_area_spells[i]; i++) {
            if (!GET_SKILL(ch, mob_bard_area_spells[i]))
                continue;
            if (mob_cast(ch, victim, NULL, mob_bard_area_spells[i]))
                return TRUE;
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
            if (mob_cast(ch, victim, NULL, mob_bard_hindrances[i].spell))
                return TRUE;
            else
                break;
        }
    }

    counter = 0;
    /* Single-target offensive spell */
    for (i = 0; mob_bard_offensives[i]; i++) {
        if (!GET_SKILL(ch, mob_bard_offensives[i]))
            continue;

        if (mob_cast(ch, victim, NULL, mob_bard_offensives[i]))
            return TRUE;

        /* Only attempt to cast this mob's best 3 spells. */
        if (++counter >= 3)
            break;
    }

    return FALSE;
}

/*
 * check_bard_status
 *
 * Makes the bard mob check its spells.
 */
bool check_bard_status(char_data *ch) {
    int i;

    /* Check bad affects */
    for (i = 0; mob_bard_hindrances[i].spell; i++) {
        if (!GET_SKILL(ch, mob_bard_hindrances[i].spell))
            continue;

        /* If the spell can be removed and the mob has it, try to remove it */
        if (mob_bard_hindrances[i].remover && has_effect(ch, &mob_bard_hindrances[i]))
            if (mob_cast(ch, ch, NULL, mob_bard_hindrances[i].remover))
                return TRUE;
        /* 10% chance to cancel if in combat. */
        if (FIGHTING(ch) && !number(0, 9))
            return FALSE;
    }

    /* Check other spells */
    for (i = 0; mob_bard_buffs[i].spell; i++) {
        if (!GET_SKILL(ch, mob_bard_buffs[i].spell) || !check_fluid_spell_ok(ch, ch, mob_bard_buffs[i].spell, TRUE))
            continue;

        if (has_effect(ch, &mob_bard_buffs[i]))
            continue;

        if (mob_cast(ch, ch, NULL, mob_bard_buffs[i].spell))
            return TRUE;
    }

    return FALSE;
}

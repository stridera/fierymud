/***************************************************************************
 *  File: cleric.c                                        Part of FieryMUD *
 *  Usage: Control Cleric type mobs, this file is closely related to ai.h  *
 *         and ai_util.c                                                   *
 *  By: Ben Horner (Proky of HubisMUD)                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/
#include "cleric.hpp"

#include "ai.hpp"
#include "casting.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "races.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Clerical spell lists */
const SpellPair mob_cleric_buffs[] = {{SPELL_SOULSHIELD, 0, EFF_SOULSHIELD},
                                      {SPELL_PROT_FROM_EVIL, 0, EFF_PROTECT_EVIL},
                                      {SPELL_PROT_FROM_GOOD, 0, EFF_PROTECT_GOOD},
                                      {SPELL_ARMOR, 0, 0},
                                      {SPELL_DEMONSKIN, 0, 0},
                                      {SPELL_GAIAS_CLOAK, 0, 0},
                                      {SPELL_BARKSKIN, 0, 0},
                                      {SPELL_DEMONIC_MUTATION, 0, 0},
                                      {SPELL_DEMONIC_ASPECT, 0, 0},
                                      {SPELL_SENSE_LIFE, 0, EFF_SENSE_LIFE},
                                      {SPELL_PRAYER, 0, 0},
                                      {SPELL_DARK_PRESENCE, 0, 0},
                                      {0, 0, 0}};

/* These spells should all be castable in combat. */
const SpellPair mob_cleric_hindrances[] = {{SPELL_BLINDNESS, SPELL_CURE_BLIND, EFF_BLIND},
                                           {SPELL_POISON, SPELL_REMOVE_POISON, EFF_POISON},
                                           {SPELL_DISEASE, SPELL_HEAL, EFF_DISEASE},
                                           {SPELL_CURSE, SPELL_REMOVE_CURSE, EFF_CURSE},
                                           {SPELL_INSANITY, SPELL_SANE_MIND, EFF_INSANITY},
                                           {SPELL_SILENCE, 0, 0}, /* Try to cast this, but there's no cure */
                                           {SPELL_ENTANGLE, SPELL_REMOVE_PARALYSIS, 0},
                                           {SPELL_WEB, SPELL_REMOVE_PARALYSIS, EFF_IMMOBILIZED},
                                           {0, 0, 0}};

const int mob_cleric_offensives[] = {
    SPELL_FULL_HARM,    SPELL_SUNRAY,         SPELL_FLAMESTRIKE,      SPELL_DIVINE_RAY,
    SPELL_HARM,         SPELL_DESTROY_UNDEAD, SPELL_STYGIAN_ERUPTION, SPELL_DISPEL_EVIL,
    SPELL_DISPEL_GOOD,  SPELL_WRITHING_WEEDS, SPELL_HELL_BOLT,        SPELL_DIVINE_BOLT,
    SPELL_CAUSE_CRITIC, SPELL_CAUSE_SERIOUS,  SPELL_CAUSE_LIGHT,      0};

const int mob_cleric_area_spells[] = {SPELL_HOLY_WORD, SPELL_UNHOLY_WORD, SPELL_EARTHQUAKE, 0};

const int mob_cleric_heals[] = {SPELL_FULL_HEAL,    SPELL_HEAL,       SPELL_CURE_CRITIC,
                                SPELL_CURE_SERIOUS, SPELL_CURE_LIGHT, 0};

/* External functions */
int mob_cast(CharData *ch, CharData *tch, ObjData *tobj, int spellnum);
bool affected_by_armor_spells(CharData *victim, int spellnum);

/*
 * cleric_ai_action
 *
 *
 */
bool cleric_ai_action(CharData *ch, CharData *victim) {
    int my_health, victim_health, i, counter, action = 0;
    CharData *next_victim;

    if (!victim) {
        log("No victim in cleric AI action.");
        return false;
    }

    /* Well no chance of casting any spells today. */
    if (EFF_FLAGGED(ch, EFF_SILENCE))
        return false;

    /* Most classes using clerical spells have alignment restrictions. */
    if ((GET_CLASS(ch) == CLASS_DIABOLIST && !IS_EVIL(ch)) || (GET_CLASS(ch) == CLASS_PRIEST && !IS_GOOD(ch)) ||
        (GET_CLASS(ch) == CLASS_PALADIN && !IS_GOOD(ch)) || (GET_CLASS(ch) == CLASS_RANGER && !IS_GOOD(ch)) ||
        (GET_CLASS(ch) == CLASS_ANTI_PALADIN && !IS_EVIL(ch)))
        return false;

    /* Calculate mob and victim health as a percentage. */
    my_health = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
    victim_health = (100 * GET_HIT(victim)) / GET_MAX_HIT(victim);

    if (my_health > 90)
        action = 10;
    else if (my_health > 60)
        action = 4;
    else if (my_health < 30)
        action = 2;
    if (victim_health < 20)
        action += 3;
    else if (victim_health < 10)
        action += 5;

    /* If action < 6 then heal. */
    if (action < 6 && mob_heal_up(ch))
        return true;

    /* Otherwise kill or harm in some fashion */

    /* Area effects first if the victim is grouped. */
    if (group_size(victim, true) > 1) {
        counter = 0;
        for (i = 0; mob_cleric_area_spells[i]; i++) {
            if (!GET_SKILL(ch, mob_cleric_area_spells[i]))
                continue;
            switch (mob_cleric_area_spells[i]) {
            case SPELL_HOLY_WORD:
                /* Don't cast if it's suicidal or useless. */
                if (IS_EVIL(ch) || !evil_in_group(victim))
                    continue;
                break;
            case SPELL_UNHOLY_WORD:
                if (IS_GOOD(ch) || !good_in_group(victim))
                    continue;
                break;
            case SPELL_EARTHQUAKE:
                /* Only cast earthquake outside and in ground. */
                if (!QUAKABLE(CH_NROOM(ch)))
                    continue;
                break;
            }
            if (mob_cast(ch, victim, nullptr, mob_cleric_area_spells[i]))
                return true;
            /* Only try the mob's best two spells. */
            if (++counter >= 2)
                break;
        }
    }

    /* Try to cause an offensive affection to main attacker up to 20 levels higher. Only attempt one spell. */
    for (i = 0; mob_cleric_hindrances[i].spell; i++) {
        if (!GET_SKILL(ch, mob_cleric_hindrances[i].spell))
            continue;
        if (!has_effect(victim, &mob_cleric_hindrances[i]) && GET_LEVEL(victim) < (GET_LEVEL(ch) + 20)) {
            if (mob_cast(ch, victim, nullptr, mob_cleric_hindrances[i].spell))
                return true;
        } else
            break;
    }

    /* Try to debilitate enemy spellcasters up to 20 levels higher */
    for (i = 0; mob_cleric_hindrances[i].spell; i++) {
        if (!GET_SKILL(ch, mob_cleric_hindrances[i].spell))
            continue;
        switch (mob_cleric_hindrances[i].spell) {
        case SPELL_INSANITY:
        case SPELL_SILENCE:
            for (victim = world[ch->in_room].people; victim; victim = next_victim) {
                next_victim = victim->next_in_room;
                if (IS_SPELLCASTER(victim) && FIGHTING(victim) == ch &&
                    !has_effect(victim, &mob_cleric_hindrances[i]) && GET_LEVEL(victim) < (GET_LEVEL(ch) + 20)) {
                    if (mob_cast(ch, victim, nullptr, mob_cleric_hindrances[i].spell))
                        return true;
                }
            }
        }
    }

    /* Now attempt a harming spell. */
    for (i = 0, counter = 0; mob_cleric_offensives[i]; i++) {
        switch (mob_cleric_offensives[i]) {
        case SPELL_DISPEL_EVIL:
            if (!IS_GOOD(ch) || !IS_EVIL(victim))
                continue;
            break;
        case SPELL_DISPEL_GOOD:
            if (!IS_EVIL(ch) || !IS_GOOD(victim))
                continue;
            break;
        case SPELL_FLAMESTRIKE:
            if (!IS_GOOD(ch))
                continue; /* Not worth casting if alignment is too low */
            break;
        case SPELL_DIVINE_RAY:
            if (GET_ALIGNMENT(ch) <= 650)
                continue;
            break;
        case SPELL_DESTROY_UNDEAD:
            if (GET_LIFEFORCE(victim) != LIFE_UNDEAD)
                continue;
            break;
        case SPELL_STYGIAN_ERUPTION:
            if (!IS_EVIL(ch))
                continue;
            break;
        case SPELL_HELL_BOLT:
            if (IS_EVIL(victim))
                continue;
            break;
        case SPELL_DIVINE_BOLT:
            if (IS_GOOD(victim))
                continue;
            break;
        }
        if (mob_cast(ch, victim, nullptr, mob_cleric_offensives[i]))
            return true;
        else
            counter++;
        /* Only attempt the mob's two best spells.  The rest are worthless. */
        if (counter > 2)
            break;
    }

    return false;
}

/*
 * check_cleric_status
 *
 * Makes the cleric mob check its spells.  Unlike the sorcerer function of
 * similar name, this one shouldn't be called when the mob is in combat.
 * Cleric spells are all pretty useless in battle.
 */
bool check_cleric_status(CharData *ch) {
    int i;

    /* Check bad affects */
    for (i = 0; mob_cleric_hindrances[i].spell; i++) {
        if (!GET_SKILL(ch, mob_cleric_hindrances[i].spell))
            continue;

        /* If the spell can be removed and the mob has it, try to remove it */
        if (mob_cleric_hindrances[i].remover && has_effect(ch, &mob_cleric_hindrances[i]))
            if (mob_cast(ch, ch, nullptr, mob_cleric_hindrances[i].remover))
                return true;
        /* 10% chance to cancel if in combat. */
        if (FIGHTING(ch) && !random_number(0, 9))
            return false;
    }

    /* Check other spells */
    for (i = 0; mob_cleric_buffs[i].spell; i++) {
        if (!GET_SKILL(ch, mob_cleric_buffs[i].spell) || !check_fluid_spell_ok(ch, ch, mob_cleric_buffs[i].spell, true))
            continue;
        switch (mob_cleric_buffs[i].spell) {
        case SPELL_GAIAS_CLOAK:
            if (CH_INDOORS(ch) || SECT(ch->in_room) == SECT_UNDERWATER || SECT(ch->in_room) == SECT_UNDERDARK)
                continue;
            /* The armor spells don't mix. */
            if (affected_by_armor_spells(ch, mob_cleric_buffs[i].spell))
                continue;
            break;
        case SPELL_DEMONSKIN:
            if (IS_GOOD(ch))
                continue;
        case SPELL_ARMOR:
        case SPELL_BARKSKIN:
            /* The armor spells don't mix. */
            if (affected_by_armor_spells(ch, mob_cleric_buffs[i].spell))
                continue;
            break;
        case SPELL_DEMONIC_ASPECT:
        case SPELL_DEMONIC_MUTATION:
            /* Demonic mutation and demonic aspect don't mix. */
            if (affected_by_spell(ch, SPELL_DEMONIC_ASPECT) || affected_by_spell(ch, SPELL_DEMONIC_MUTATION))
                continue;
            break;
        case SPELL_PROT_FROM_EVIL:
            if (IS_EVIL(ch) || EFF_FLAGGED(ch, EFF_PROTECT_EVIL))
                continue;
            break;
        case SPELL_PROT_FROM_GOOD:
            if (IS_GOOD(ch) || EFF_FLAGGED(ch, EFF_PROTECT_GOOD))
                continue;
            break;
        case SPELL_SOULSHIELD:
            if (IS_NEUTRAL(ch))
                continue;
            if (EFF_FLAGGED(ch, EFF_SOULSHIELD))
                continue;
            break;
        case SPELL_DARK_PRESENCE:
            if (IS_GOOD(ch) || affected_by_spell(ch, SPELL_BLESS) || affected_by_spell(ch, SPELL_DARK_PRESENCE))
                continue;
            break;
        default:
            if (has_effect(ch, &mob_cleric_buffs[i]))
                continue;
        }
        if (mob_cast(ch, ch, nullptr, mob_cleric_buffs[i].spell))
            return true;
    }

    return false;
}

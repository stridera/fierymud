/***************************************************************************
 *   File: sorcerer.c                                     Part of FieryMUD *
 *  Usage: AI actions for sorcerer-type mobiles.                           *
 *     By: Laoris of FieryMUD                                              *
 *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "sorcerer.hpp"

#include "ai.hpp"
#include "casting.hpp"
#include "comm.hpp"
#include "composition.hpp"
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
#include "spell_parser.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Local functions */
bool check_sorcerer_status(CharData *ch);

/* External functions */
int mob_cast(CharData *ch, CharData *tch, ObjData *tobj, int spellnum);
bool affected_by_armor_spells(CharData *ch, int spellnum);

bool sorcerer_ai_action(CharData *ch, CharData *victim) {
    int my_health, victim_health, i, counter;

    if (!victim) {
        log(LogSeverity::Stat, LVL_GOD, "No victim in sorcerer AI action.");
        return false;
    }

    my_health = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
    victim_health = (100 * GET_HIT(victim)) / GET_MAX_HIT(victim);

    /* If mob has low health, maybe try to teleport. */
    if (my_health < 10 && !PLAYERALLY(ch) && !random_number(0, 3) && mob_cast(ch, ch, nullptr, SPELL_TELEPORT))
        return true;

    /* Check buff spells */
    if (victim_health > 15 && check_sorcerer_status(ch))
        return true;

    /* If the victim is grouped, try an area spell first. */
    if (group_size(victim) > 1) {
        counter = 0;
        for (i = 0; mob_sorcerer_area_spells[i]; i++) {
            if (!GET_SKILL(ch, mob_sorcerer_area_spells[i]))
                continue;
            if (mob_cast(ch, victim, nullptr, mob_sorcerer_area_spells[i]))
                return true;
            /* Only try the mob's best two spells. */
            if (++counter >= 2)
                break;
        }
    }

    /* Sorcerers get 13% chance to cast harness. */
    if (GET_SKILL(ch, SPELL_HARNESS) && !EFF_FLAGGED(ch, EFF_HARNESS) && FIGHTING(ch) && victim_health > 75 &&
        !random_number(0, 6) && mob_cast(ch, ch, nullptr, SPELL_HARNESS))
        return true;

    /* Necromancers get 10% chance to cast poison. */
    if (GET_SKILL(ch, SPELL_POISON) && !EFF_FLAGGED(ch, EFF_POISON) && victim_health > 75 && !random_number(0, 9) &&
        mob_cast(ch, victim, nullptr, SPELL_POISON))
        return true;

    /* Try and cast ray of enfeeblement! */
    if (GET_SKILL(ch, SPELL_RAY_OF_ENFEEB) && !EFF_FLAGGED(victim, EFF_RAY_OF_ENFEEB) &&
        /* Better chance to attempt if victim in good condition. */
        !random_number(0, (victim_health > 80) ? 2 : 7) && mob_cast(ch, victim, nullptr, SPELL_RAY_OF_ENFEEB))
        return true;

    counter = 0;
    /* Single-target offensive spell */
    for (i = 0; mob_sorcerer_offensives[i]; i++) {
        if (!GET_SKILL(ch, mob_sorcerer_offensives[i]))
            continue;

        switch (mob_sorcerer_offensives[i]) {
        case SPELL_DEGENERATION:
        case SPELL_SOUL_TAP:
        case SPELL_ENERGY_DRAIN:
            if (GET_LIFEFORCE(victim) == LIFE_UNDEAD || GET_LIFEFORCE(victim) == LIFE_MAGIC)
                continue;
            break;
        case SPELL_REBUKE_UNDEAD:
            if (GET_LIFEFORCE(victim) != LIFE_UNDEAD)
                continue;
            break;
        }

        if (mob_cast(ch, victim, nullptr, mob_sorcerer_offensives[i]))
            return true;

        /* Only attempt to cast this mob's best 3 spells. */
        if (++counter >= 3)
            break;
    }

    return false;
}

/* This checks mob buffs. */
bool check_sorcerer_status(CharData *ch) {
    int i;

    for (i = 0; mob_sorcerer_buffs[i].spell; i++) {
        if (!GET_SKILL(ch, mob_sorcerer_buffs[i].spell) ||
            !check_fluid_spell_ok(ch, ch, mob_sorcerer_buffs[i].spell, true))
            continue;

        if (!valid_cast_stance(ch, i))
            continue;
        /* 20% chance to cancel if in combat. */
        if (FIGHTING(ch) && random_number(0, 9) < 2)
            return false;
        switch (mob_sorcerer_buffs[i].spell) {
        case SPELL_COLDSHIELD:
        case SPELL_FIRESHIELD:
            /* Coldshield and fireshield don't mix. */
            if (EFF_FLAGGED(ch, EFF_COLDSHIELD) || EFF_FLAGGED(ch, EFF_FIRESHIELD))
                continue;
            break;
        case SPELL_MINOR_GLOBE:
        case SPELL_MAJOR_GLOBE:
            /* Major globe and minor globe don't mix either. */
            if (EFF_FLAGGED(ch, EFF_MINOR_GLOBE) || EFF_FLAGGED(ch, EFF_MAJOR_GLOBE))
                continue;
            break;
        case SPELL_MIRAGE:
        case SPELL_ICE_ARMOR:
        case SPELL_BONE_ARMOR:
            /* Armor spells don't mix. */
            if (affected_by_armor_spells(ch, mob_sorcerer_buffs[i].spell))
                continue;
            break;
        case SPELL_WATERFORM:
            if (GET_COMPOSITION(ch) != COMP_FLESH)
                continue;
            break;
        case SPELL_VAPORFORM:
            if (GET_COMPOSITION(ch) != COMP_FLESH)
                continue;
            break;
        default:
            /* For other spells, just check to see if they have it. */
            if (has_effect(ch, &mob_sorcerer_buffs[i]))
                continue;
        }
        if (mob_cast(ch, ch, nullptr, mob_sorcerer_buffs[i].spell))
            return true;
    }

    return false;
}

bool mob_animate(CharData *ch) {
    ObjData *obj;

    if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) || EFF_FLAGGED(ch, EFF_SILENCE))
        return false;

    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
        /* There's no IS_NPC_CORPSE macro */
        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) == CORPSE_NPC &&
            GET_OBJ_MOB_FROM(obj) != NOBODY && GET_LEVEL(mob_proto + GET_OBJ_MOB_FROM(obj)) + 10 < GET_LEVEL(ch))
            return mob_cast(ch, nullptr, obj, SPELL_ANIMATE_DEAD);

    return false;
}

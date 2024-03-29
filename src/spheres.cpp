/***************************************************************************
 *  File: spheres.c                                      Part of FieryMUD  *
 *  Usage: Spell spheres                                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "spheres.hpp"

#include "conf.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/* Values for the spheredef struct:
 *
 *   Name, color, skill
 */

SphereDef spheres[NUM_SPHERES] = {{"generic", "&5", SKILL_SPHERE_GENERIC},  {"fire", "&1", SKILL_SPHERE_FIRE},
                                  {"water", "&4", SKILL_SPHERE_WATER},      {"earth", "&3", SKILL_SPHERE_EARTH},
                                  {"air", "&6", SKILL_SPHERE_AIR},          {"healing", "&2", SKILL_SPHERE_HEALING},
                                  {"protection", "&4", SKILL_SPHERE_PROT},  {"enchantment", "&5", SKILL_SPHERE_ENCHANT},
                                  {"summoning", "&3", SKILL_SPHERE_SUMMON}, {"death", "&9&b", SKILL_SPHERE_DEATH},
                                  {"divination", "&6", SKILL_SPHERE_DIVIN}};

int _skill_to_sphere(int skill, int recursed) {
    switch (skill) {
    case SKILL_SPHERE_GENERIC:
        return SPHERE_GENERIC;
    case SKILL_SPHERE_FIRE:
        return SPHERE_FIRE;
    case SKILL_SPHERE_WATER:
        return SPHERE_WATER;
    case SKILL_SPHERE_EARTH:
        return SPHERE_EARTH;
    case SKILL_SPHERE_AIR:
        return SPHERE_AIR;
    case SKILL_SPHERE_HEALING:
        return SPHERE_HEALING;
    case SKILL_SPHERE_PROT:
        return SPHERE_PROT;
    case SKILL_SPHERE_ENCHANT:
        return SPHERE_ENCHANT;
    case SKILL_SPHERE_SUMMON:
        return SPHERE_SUMMON;
    case SKILL_SPHERE_DEATH:
        return SPHERE_DEATH;
    case SKILL_SPHERE_DIVIN:
        return SPHERE_DIVIN;
    default:
        if (recursed) {
            /* ERROR */
            return SPHERE_GENERIC;
        } else {
            return _skill_to_sphere(skills[skill].sphere, 1);
        }
    }
}

int skill_to_sphere(int skill) { return _skill_to_sphere(skill, 0); }

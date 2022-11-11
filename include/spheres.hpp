/***************************************************************************
 *  File: spheres.h                                       Part of FieryMUD *
 *  Usage: header file for spell spheres                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "defines.hpp"

#define SPHERE_GENERIC 0
#define SPHERE_FIRE 1
#define SPHERE_WATER 2
#define SPHERE_EARTH 3
#define SPHERE_AIR 4
#define SPHERE_HEALING 5
#define SPHERE_PROT 6
#define SPHERE_ENCHANT 7
#define SPHERE_SUMMON 8
#define SPHERE_DEATH 9
#define SPHERE_DIVIN 10

#define NUM_SPHERES 11

struct SphereDef {
    const char *name;
    const char *color;
    int skill;
};

/* Values for the spheredef struct:
 *
 *   Name, color, skill
 */

struct SphereDef spheres[NUM_SPHERES] = {
    {"generic", "&5", SKILL_SPHERE_GENERIC},  {"fire", "&1", SKILL_SPHERE_FIRE},
    {"water", "&4", SKILL_SPHERE_WATER},      {"earth", "&3", SKILL_SPHERE_EARTH},
    {"air", "&6", SKILL_SPHERE_AIR},          {"healing", "&2", SKILL_SPHERE_HEALING},
    {"protection", "&4", SKILL_SPHERE_PROT},  {"enchantment", "&5", SKILL_SPHERE_ENCHANT},
    {"summoning", "&3", SKILL_SPHERE_SUMMON}, {"death", "&9&b", SKILL_SPHERE_DEATH},
    {"divination", "&6", SKILL_SPHERE_DIVIN}};

int skill_to_sphere(int skill);

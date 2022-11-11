/***************************************************************************
 *  File: lifeforce.h                                     Part of FieryMUD *
 *  Usage: header file for character life forces                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

#define LIFE_UNDEFINED -1
#define LIFE_LIFE 0 /* normal folks */
#define LIFE_UNDEAD 1
#define LIFE_MAGIC 2     /* golems */
#define LIFE_CELESTIAL 3 /* angels */
#define LIFE_DEMONIC 4
#define LIFE_ELEMENTAL 5
#define NUM_LIFEFORCES 6 /* keep updated */

struct LifeDef {
    /* The first element of this struct, 'name', is used by parse_obj_name() and must not be changed. */
    const char *name;
    const char *color;
    int sus_heal;
    int sus_discorporate;
    int sus_dispel;
    int sus_mental;
};

/* Values:
 *
 *   Name, Color,
 *   SUSCEPTIBILITY: heal, discorporate, dispel magic, mental
 */

struct LifeDef lifeforces[NUM_LIFEFORCES] = {{"life", "&2&b", 100, 0, 0, 100},    {"undead", "&9&b", 75, 50, 0, 100},
                                             {"magic", "&4&b", 0, 120, 50, 0},    {"celestial", "&6", 100, 50, 0, 75},
                                             {"demonic", "&1&b", 100, 50, 0, 75}, {"elemental", "&3", 50, 100, 0, 50}};

int parse_lifeforce(CharData *ch, char *arg);
void convert_lifeforce(CharData *ch, int newlifeforce);
#define VALID_LIFEFORCENUM(num) ((num) >= 0 && (num) < NUM_LIFEFORCES)
#define VALID_LIFEFORCE(ch) (VALID_LIFEFORCENUM(GET_LIFEFORCE(ch)))
#define LIFEFORCE_NAME(ch) (VALID_LIFEFORCE(ch) ? lifeforces[GET_LIFEFORCE(ch)].name : "<INVALID LIFEFORCE>")
#define LIFEFORCE_COLOR(ch) (VALID_LIFEFORCE(ch) ? lifeforces[GET_LIFEFORCE(ch)].color : "")

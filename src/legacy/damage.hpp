/***************************************************************************
 *  File: damage.h                                        Part of FieryMUD *
 *  Usage: header file for damage types                                    *
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

/* When adding a damage type, keep in mind that the susceptibility may vary
 * by composition and/or life force.  Consider adding a field to one or
 * both of those structs.
 *
 * Also add a case statement to the susceptibility() function.
 * The default vulnerability for any damage type is 100.
 */
#define DAM_UNDEFINED -1
#define DAM_SLASH 0
#define DAM_PIERCE 1
#define DAM_CRUSH 2
#define DAM_SHOCK 3
#define DAM_FIRE 4
#define DAM_WATER 5
#define DAM_COLD 6
#define DAM_ACID 7
#define DAM_POISON 8
#define DAM_HEAL 9
#define DAM_ALIGN 10
#define DAM_DISPEL 11
#define DAM_DISCORPORATE 12
#define DAM_MENTAL 13
#define NUM_DAMTYPES 14 /* keep updated */

struct DamageDef {
    const char *name;
    const char *color;
    const char *verb1st;
    const char *verb2nd;
    const char *action;
};
extern DamageDef damtypes[NUM_DAMTYPES];

#define VALID_DAMTYPE(d) (d >= 0 && d < NUM_DAMTYPES)

/* name, color, verb1st, verb2nd, action */

int parse_damtype(CharData *ch, char *arg);

int skill_to_dtype(int skill);
int convert_weapon_damage(ObjData *weapon);
int convert_weapon_type(ObjData *weapon);
void damage_evasion_message(CharData *ch, CharData *vict, ObjData *weapon, int dtype);
int weapon_damtype(ObjData *obj);
int physical_damtype(CharData *ch);

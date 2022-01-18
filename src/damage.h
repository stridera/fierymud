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

#ifndef __FIERY_DAMAGE_H
#define __FIERY_DAMAGE_H

#include "structs.h"
#include "sysdep.h"

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

struct damdef {
    char *name;
    char *color;
    char *verb1st;
    char *verb2nd;
    char *action;
};

#define VALID_DAMTYPE(d) (d >= 0 && d < NUM_DAMTYPES)

extern struct damdef damtypes[];
extern int parse_damtype(struct char_data *ch, char *arg);

extern int skill_to_dtype(int skill);
extern void damage_evasion_message(struct char_data *ch, struct char_data *vict, struct obj_data *weapon, int dtype);
extern int weapon_damtype(struct obj_data *obj);
extern int physical_damtype(struct char_data *ch);

#endif

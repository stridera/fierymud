/***************************************************************************
 *  File: chars.h                                         Part of FieryMUD *
 *  Usage: header file for character structures and constants              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_CHARS_H
#define __FIERY_CHARS_H

#include "structs.h"
#include "sysdep.h"

#define NUM_CLASSES 25

/* Indexing of stats */
#define STAT_STR 0
#define STAT_DEX 1
#define STAT_CON 2
#define STAT_WIS 3
#define STAT_INT 4
#define STAT_CHA 5
#define NUM_STATS 6 /* Update this if adding a stat */

/* Saving throws */
#define SAVING_PARA 0
#define SAVING_ROD 1
#define SAVING_PETRI 2
#define SAVING_BREATH 3
#define SAVING_SPELL 4
#define NUM_SAVES 5

/* Extreme values for stats */
#define MIN_AC (-100)
#define MAX_AC 100
#define MIN_DAMROLL (-40)
#define MIN_HITROLL (-40)
#define MAX_DAMROLL 40
#define MAX_HITROLL 40
#define MIN_ALIGNMENT (-1000)
#define MAX_ALIGNMENT (1000)

extern int class_ok_race[][NUM_CLASSES];
extern const char *stats_display;

extern int get_base_saves(struct char_data *ch, int type);
extern void roll_natural_abils(struct char_data *ch);
extern int roll_mob_skill(int level);
extern int roll_skill(struct char_data *ch, int skill);

extern struct obj_data *equipped_weapon(struct char_data *ch);
/* Whether you evade a simple yes-or-no attack, like sleep or word of command */
extern bool boolean_attack_evasion(struct char_data *ch, int power, int dtype);
extern int dam_suscept_adjust(struct char_data *ch, struct char_data *victim, struct obj_data *weapon, int dam,
                              int dtype);
extern bool damage_evasion(struct char_data *ch, struct char_data *attacker, struct obj_data *weapon, int dtype);
#define EVASIONCLR "&6"

#define DAMVERB1(ch) (damtypes[COMPOSITION_DAM(ch)].verb1st)
#define DAMVERB2(ch) (damtypes[COMPOSITION_DAM(ch)].verb2nd)

#define SOLIDCHAR(ch) (RIGID(ch) && !MOB_FLAGGED((ch), MOB_ILLUSORY))

extern void composition_check(struct char_data *ch);
extern int susceptibility(struct char_data *ch, int dtype);
extern const char *align_color(int align);
extern void critical_stance_message(struct char_data *ch);
extern void alter_pos(struct char_data *ch, int newpos, int newstance);
extern void hp_pos_check(struct char_data *ch, struct char_data *attacker, int dam);

#endif

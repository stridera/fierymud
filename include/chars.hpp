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

#pragma once

#include "races.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

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

const char *stats_display =
    "&0&7&b[s]&0 Strength      &0&7&b[i]&0 Intelligence\r\n"
    "&0&7&b[w]&0 Wisdom        &0&7&b[c]&0 Constitution\r\n"
    "&0&7&b[d]&0 Dexterity     &0&7&b[m]&0 Charisma\r\n\r\n";

#define Y true
#define N false

int class_ok_race[NUM_RACES][NUM_CLASSES] = {
    /* RACE   So Cl Th Wa Pa An Ra Dr Sh As Me Ne Co Mo Be Pr Di My Ro Ba Py Cr Il Hu */
    /* Hu */ {Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, N},
    /* El */ {Y, Y, Y, Y, Y, N, Y, Y, N, N, N, N, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N},
    /* Gn */ {Y, Y, Y, N, N, N, N, Y, Y, N, N, N, Y, N, N, Y, N, N, Y, Y, Y, Y, Y, N},
    /* Dw */ {N, Y, Y, Y, Y, N, N, N, N, N, Y, N, Y, N, Y, Y, N, N, Y, Y, N, N, N, N},
    /* Tr */ {N, N, N, Y, N, N, N, N, Y, N, Y, N, N, N, Y, N, N, N, Y, N, N, N, N, Y},
    /* Dr */ {Y, Y, N, Y, N, Y, N, N, Y, Y, Y, Y, Y, N, N, N, Y, N, Y, N, Y, Y, Y, Y},
    /* Du */ {N, Y, Y, Y, N, Y, N, N, N, Y, Y, N, N, N, Y, N, Y, N, Y, N, N, N, N, Y},
    /* Og */ {N, N, N, Y, N, N, N, N, Y, N, Y, N, N, N, Y, N, N, N, Y, N, N, N, N, Y},
    /* Or */ {Y, Y, Y, Y, N, Y, N, N, Y, Y, Y, Y, Y, N, Y, N, Y, N, Y, N, Y, Y, Y, Y},
    /* HE */ {Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, Y, Y, N, Y, N, N, Y, Y, Y, Y, Y, N},
    /* Ba */ {N, N, N, Y, N, N, N, N, Y, N, Y, N, N, N, Y, N, N, N, Y, N, N, N, N, N},
    /* Ha */ {Y, Y, Y, Y, N, N, N, N, N, N, N, N, Y, N, N, Y, N, N, Y, Y, Y, Y, Y, N},
    /*plnt*/ {},
    /*hmnd*/ {},
    /*anml*/ {},
    /*drgn*/ {},
    /*gint*/ {},
    /*othr*/ {},
    /*gbln*/ {},
    /*demn*/ {},
    /*brwn*/ {},
    /*fire*/ {},
    /*frst*/ {},
    /*acid*/ {},
    /*ligh*/ {},
    /*gas */ {},
    /*DbFi*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, N, Y, N},
    /*DbFr*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, N, Y, Y, N},
    /*DbAc*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbLi*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*DbGa*/ {Y, Y, N, Y, Y, Y, N, N, Y, N, N, Y, Y, N, Y, Y, Y, Y, N, N, Y, Y, Y, N},
    /*svrf*/ {Y, Y, Y, N, N, N, N, N, Y, Y, N, Y, Y, N, N, N, Y, Y, Y, Y, Y, Y, Y, N},
    /*SFae*/ {Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, Y, N, N, N, N, Y, Y, Y, Y, Y, Y, N},
    /*UFae*/ {Y, Y, Y, N, N, N, N, Y, N, N, N, Y, Y, N, N, N, N, Y, Y, Y, Y, Y, Y, N},
    /*nmph*/ {Y, Y, N, Y, N, N, Y, Y, Y, N, N, N, Y, N, N, N, N, Y, Y, Y, Y, Y, Y, N},
};

int get_base_saves(CharData *ch, int type);
void roll_natural_abils(CharData *ch);
int roll_mob_skill(int level);
int roll_skill(CharData *ch, int skill);

// extern obj_data *equipped_weapon(CharData *ch);
// /* Whether you evade a simple yes-or-no attack, like sleep or word of command */
bool boolean_attack_evasion(CharData *ch, int power, int dtype);
int dam_suscept_adjust(CharData *ch, CharData *victim, ObjData *weapon, int dam, int dtype);
bool damage_evasion(CharData *ch, CharData *attacker, ObjData *weapon, int dtype);
#define EVASIONCLR "&6"

#define DAMVERB1(ch) (damtypes[COMPOSITION_DAM(ch)].verb1st)
#define DAMVERB2(ch) (damtypes[COMPOSITION_DAM(ch)].verb2nd)

#define SOLIDCHAR(ch) (RIGID(ch) && !MOB_FLAGGED((ch), MOB_ILLUSORY))

void composition_check(CharData *ch);
int susceptibility(CharData *ch, int dtype);
const char *align_color(int align);
void critical_stance_message(CharData *ch);
void alter_pos(CharData *ch, int newpos, int newstance);
void hp_pos_check(CharData *ch, CharData *attacker, int dam);
ObjData *equipped_weapon(CharData *ch);
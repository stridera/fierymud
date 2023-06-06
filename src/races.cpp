/***************************************************************************
 *   File: races.c                                        Part of FieryMUD *
 *  Usage: Aligns race situations                                          *
 * Author: Brian Williams <bmw@efn.org>                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 98, 99               *
 ***************************************************************************/

#include "races.hpp"

#include "casting.hpp"
#include "charsize.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "regen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

/* Prototypes */
void set_init_height_weight(CharData *ch);
static flagvector race_effects_mask[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];

/* races[]
 *
 * The individual members of these struct definitions have been arranged
 * in a regular fashion, so that they can easily be located.  Please maintain
 * this arrangement:
 *
 *   name, names, displayname, fullname, plainname,
 *   playable, humanoid, racealign, def_size, def_align, \
 *     bonus_damroll, bonus_hitroll,
 *   def_lifeforce, def_composition,
 *   mweight_lo, mweight_hi, mheight_lo, mheight_hi, \
 *     fweight_lo, fweight_hi, fheight_lo, fheight_hi
 *   attrib_scales[], (str, dex, int, wis, con, cha)
 *   exp_factor, hit_factor, hd_factor, dice_factor, copper_factor, ac_factor,
 *   move_verb, leave_verb
 */

/* This defines the basic attributes of a race. */

RaceDef races[NUM_RACES] = {
    /* HUMAN */
    {"human",                   /* name as found by triggers */
     "human",                   /* all race keywords */
     "&6Human&0",               /* name as displayed at character creation, on who, and score */
     "&6Human&0",               /* name as displayed in show race command and when setting races */
     "Human",                   /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     120,                       /* male weight low */
     180,                       /* male weight high */
     60,                        /* male height low */
     76,                        /* male height high */
     95,                        /* female weight low */
     150,                       /* female weight high */
     60,                        /* female height low */
     70,                        /* female height high */
     {76, 76, 76, 76, 76, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* ELF */
    {"elf",                     /* name as found by triggers */
     "elf",                     /* all race keywords */
     "Elf&0",                   /* name as displayed at character creation, on who, and score */
     "Elf&0",                   /* name as displayed in show race command and when setting races */
     "Elf",                     /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     1000,                      /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     90,                        /* male weight low */
     160,                       /* male weight high */
     60,                        /* male height low */
     70,                        /* male height high */
     90,                        /* female weight low */
     160,                       /* female weight high */
     59,                        /* female height low */
     68,                        /* female height high */
     {64, 80, 88, 86, 64, 78},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* GNOME */
    {"gnome",                   /* name as found by triggers */
     "gnome",                   /* all race keywords */
     "&1&d&bGnome&0",           /* name as displayed at character creation, on who, and score */
     "&1&d&bGnome&0",           /* name as displayed in show race command and when setting races */
     "Gnome",                   /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_SMALL,                /* default size */
     1000,                      /* default alignment */
     2,                         /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     40,                        /* male weight low */
     90,                        /* male weight high */
     26,                        /* male height low */
     38,                        /* male height high */
     40,                        /* female weight low */
     90,                        /* female weight high */
     26,                        /* female height low */
     38,                        /* female height high */
     {64, 76, 88, 84, 68, 80},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DWARF */
    {"dwarf",                   /* name as found by triggers */
     "dwarf mountain",          /* all race keywords */
     "&3Dwarf&0",               /* name as displayed at character creation, on who, and score */
     "&3Dwarf&0",               /* name as displayed in show race command and when setting races */
     "Dwarf",                   /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     1000,                      /* default alignment */
     4,                         /* damroll bonus */
     5,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     170,                       /* male weight low */
     200,                       /* male weight high */
     38,                        /* female height low */
     50,                        /* female height high */
     150,                       /* female weight low */
     190,                       /* female weight high */
     38,                        /* female height low */
     50,                        /* female height high */
     {84, 76, 64, 86, 84, 68},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* TROLL */
    {"troll",                   /* name as found by triggers */
     "swamp troll",             /* all race keywords */
     "&2&dTroll&0",             /* name as displayed at character creation, on who, and score */
     "&2&dTroll&0",             /* name as displayed in show race command and when setting races */
     "Troll",                   /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_LARGE,                /* default size */
     -1000,                     /* default alignment */
     4,                         /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     130,                       /* male weight low */
     290,                       /* male weight high */
     72,                        /* male height low */
     90,                        /* male height high */
     130,                       /* female weight low */
     290,                       /* female weight high */
     72,                        /* female height low */
     90,                        /* female height high */
     {80, 72, 56, 56, 100, 64}, /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     130,                       /* hp factor for mobs */
     110,                       /* hitroll/damroll factor for mobs */
     110,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     "prowls in",               /* verb for entering a room */
     "prowls",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DROW */
    {"drow",                    /* name as found by triggers */
     "drow",                    /* all race keywords */
     "&5Drow&0",                /* name as displayed at character creation, on who, and score */
     "&5Drow&0",                /* name as displayed in show race command and when setting races */
     "Drow",                    /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     -1000,                     /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     90,                        /* male weight low */
     160,                       /* male weight high */
     60,                        /* male height low */
     70,                        /* male height high */
     90,                        /* female weight low */
     160,                       /* female weight high */
     60,                        /* female height low */
     70,                        /* female height high */
     {64, 80, 88, 80, 64, 72},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DUERGAR */
    {"duergar",                 /* name as found by triggers */
     "duergar",                 /* all race keywords */
     "&1Duergar&0",             /* name as displayed at character creation, on who, and score */
     "&1Duergar&0",             /* name as displayed in show race command and when setting races */
     "Duergar",                 /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     -1000,                     /* default alignment */
     4,                         /* damroll bonus */
     5,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     170,                       /* male weight low */
     200,                       /* male weight high */
     38,                        /* female height low */
     50,                        /* female height high */
     150,                       /* female weight low */
     190,                       /* female weight high */
     38,                        /* female height low */
     50,                        /* female height high */
     {84, 76, 68, 72, 84, 64},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     "skulks in",               /* verb for entering a room */
     "skulks",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* OGRE */
    {"ogre",                    /* name as found by triggers */
     "ogre",                    /* all race keywords */
     "&4Ogre&0",                /* name as displayed at character creation, on who, and score */
     "&4Ogre&0",                /* name as displayed in show race command and when setting races */
     "Ogre",                    /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_LARGE,                /* default size */
     -1000,                     /* default alignment */
     5,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     390,                       /* male weight low */
     530,                       /* male weight high */
     93,                        /* male height low */
     119,                       /* male height high */
     390,                       /* female weight low */
     530,                       /* female weight high */
     93,                        /* female height low */
     119,                       /* female height high */
     {100, 64, 52, 60, 80, 60}, /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     110,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     120,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     85,                        /* AC factor for mobs */
     "lumbers in",              /* verb for entering a room */
     "lumbers",                 /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* ORC */
    {"orc",                     /* name as found by triggers */
     "orc",                     /* all race keywords */
     "&9&bOrc&0",               /* name as displayed at character creation, on who, and score */
     "&9&bOrc&0",               /* name as displayed in show race command and when setting races */
     "Orc",                     /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     -1000,                     /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     90,                        /* male weight low */
     150,                       /* male weight high */
     58,                        /* male height low */
     68,                        /* male height high */
     90,                        /* female weight low */
     150,                       /* female weight high */
     58,                        /* female height low */
     68,                        /* female height high */
     {80, 72, 72, 72, 76, 68},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* HALF-ELF */
    {"halfelf",                /* name as found by triggers */
     "half-elf half elf",       /* all race keywords */
     "&6&bHalf-&0&6&dElf&0",    /* name as displayed at character creation, on who, and score */
     "&6&bHalf-&0&6&dElf&0",    /* name as displayed in show race command and when setting races */
     "Half-Elf",                /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     1000,                      /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     100,                       /* male weight low */
     170,                       /* male weight high */
     60,                        /* male height low */
     76,                        /* male height high */
     94,                        /* female weight low */
     155,                       /* female weight high */
     60,                        /* female height low */
     70,                        /* female height high */
     {68, 76, 76, 76, 68, 84},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* BARBARIAN */
    {"barbarian",               /* name as found by triggers */
     "barbarian",               /* all race keywords */
     "&4Barbarian&0",           /* name as displayed at character creation, on who, and score */
     "&4Barbarian&0",           /* name as displayed in show race command and when setting races */
     "Barbarian",               /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_LARGE,                /* default size */
     0,                         /* default alignment */
     5,                         /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     170,                       /* male weight low */
     260,                       /* male weight high */
     69,                        /* male height low */
     88,                        /* male height high */
     130,                       /* female weight low */
     210,                       /* female height low */
     69,                        /* female height low */
     80,                        /* female height high */
     {88, 68, 60, 60, 88, 64},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* HALFLING */
    {"halfling",                /* name as found by triggers */
     "halfling",                /* all race keywords */
     "&3&dHalfling&0",          /* name as displayed at character creation, on who, and score */
     "&3&dHalfling&0",          /* name as displayed in show race command and when setting races */
     "Halfling",                /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_SMALL,                /* default size */
     1000,                      /* default alignment */
     3,                         /* damroll bonus */
     6,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     90,                        /* male weight low */
     160,                       /* male weight high */
     35,                        /* male height low */
     42,                        /* male height high */
     90,                        /* female weight low */
     160,                       /* female weight high */
     35,                        /* female height low */
     42,                        /* female height high */
     {68, 96, 80, 80, 64, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* PLANT */
    {"plant",                   /* name as found by triggers */
     "plant",                   /* all race keywords */
     "&2Plant&0",               /* name as displayed at character creation, on who, and score */
     "&2Plant&0",               /* name as displayed in show race command and when setting races */
     "Plant",                   /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_PLANT,                /* default composition */
     80,                        /* male weight low */
     180,                       /* male weight high */
     40,                        /* male height low */
     96,                        /* male height high */
     80,                        /* female weight low */
     180,                       /* female weight high */
     40,                        /* female height low */
     96,                        /* female height high */
     {72, 52, 32, 72, 100, 72}, /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     120,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* HUMANOID */
    {"humanoid",                /* name as found by triggers */
     "humanoid",                /* all race keywords */
     "&7Humanoid&0",            /* name as displayed at character creation, on who, and score */
     "&7Humanoid&0",            /* name as displayed in show race command and when setting races */
     "Humanoid",                /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     120,                       /* male weight low */
     180,                       /* male weight high */
     60,                        /* male height low */
     76,                        /* male height high */
     95,                        /* female weight low */
     150,                       /* female weight high */
     60,                        /* female height low */
     70,                        /* female height high */
     {72, 72, 72, 72, 72, 72},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     100,                       /* money drop factor for mobs */
     60,                        /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* ANIMAL */
    {"animal",                  /* name as found by triggers */
     "animal",                  /* all race keywords */
     "&2Animal&0",              /* name as displayed at character creation, on who, and score */
     "&2Animal&0",              /* name as displayed in show race command and when setting races */
     "Animal",                  /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     120,                       /* male weight low */
     180,                       /* male weight high */
     60,                        /* male height low */
     76,                        /* male height high */
     95,                        /* female weight low */
     150,                       /* female weight high */
     60,                        /* female height low */
     70,                        /* female height high */
     {72, 72, 72, 72, 72, 72},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     0,                         /* money drop factor for mobs */
     65,                        /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGON - GENERAL */
    {"dragon_general",          /* name as found by triggers */
     "dragon general",          /* all race keywords */
     "&5&bDragon&0",            /* name as displayed at character creation, on who, and score */
     "&5&bDragon&0",            /* name as displayed in show race command and when setting races */
     "General Dragon",          /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_GARGANTUAN,           /* default size */
     0,                         /* default alignment */
     10,                        /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     16000,                     /* male weight low */
     64000,                     /* male weight high */
     768,                       /* male height low */
     1536,                      /* male height high */
     16000,                     /* female weight low */
     64000,                     /* female weight high */
     768,                       /* female height low */
     1536,                      /* female height high */
     {100, 72, 100, 72, 72, 100}, /* max stat: str, dex, int, wis, con, cha */
     130,                       /* experience reward factor for mobs */
     130,                       /* hp factor for mobs */
     140,                       /* hitroll/damroll factor for mobs */
     140,                       /* damage dice factor for mobs */
     500,                       /* money drop factor for mobs */
     140,                       /* AC factor for mobs */
     "stomps in",               /* verb for entering a room */
     "stomps",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* GIANT */
    {"giant",                   /* name as found by triggers */
     "giant",                   /* all race keywords */
     "&2&bGiant&0",             /* name as displayed at character creation, on who, and score */
     "&2&bGiant&0",             /* name as displayed in show race command and when setting races */
     "Giant",                   /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_HUGE,                 /* default size */
     0,                         /* default alignment */
     7,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     1000,                      /* male weight low */
     4000,                      /* male weight high */
     196,                       /* male height low */
     384,                       /* male height high */
     1000,                      /* female weight low */
     4000,                      /* female weight high */
     196,                       /* female height low */
     384,                       /* female height high */
     {100, 72, 44, 64, 80, 72}, /* max stat: str, dex, int, wis, con, cha */
     110,                       /* experience reward factor for mobs */
     120,                       /* hp factor for mobs */
     120,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     125,                       /* money drop factor for mobs */
     120,                       /* AC factor for mobs */
     "lumbers in",              /* verb for entering a room */
     "lumbers",                 /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* OTHER */
    {"other",                   /* name as found by triggers */
     "other",                   /* all race keywords */
     "&4&bOther&0",             /* name as displayed at character creation, on who, and score */
     "&4&bOther&0",             /* name as displayed in show race command and when setting races */
     "Other",                   /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     120,                       /* male weight low */
     180,                       /* male weight high */
     60,                        /* male height low */
     76,                        /* male height high */
     95,                        /* female weight low */
     150,                       /* female weight high */
     60,                        /* female height low */
     70,                        /* female height high */
     {72, 72, 72, 72, 72, 72},  /* max stat: str, dex, int, wis, con, cha */
     80,                        /* experience reward factor for mobs */
     110,                       /* hp factor for mobs */
     120,                       /* hitroll/damroll factor for mobs */
     80,                        /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     105,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* GOBLIN */
    {"goblin",                  /* name as found by triggers */
     "goblin",                  /* all race keywords */
     "&4&bGoblin&0",            /* name as displayed at character creation, on who, and score */
     "&4&bGoblin&0",            /* name as displayed in show race command and when setting races */
     "Goblin",                  /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_SMALL,                /* default size */
     -500,                      /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     60,                        /* male weight low */
     90,                        /* male weight high */
     30,                        /* male height low */
     38,                        /* male height high */
     55,                        /* female weight low */
     80,                        /* female weight high */
     30,                        /* female height low */
     35,                        /* female height high */
     {76, 72, 64, 72, 84, 64},  /* max stat: str, dex, int, wis, con, cha */
     60,                        /* experience reward factor for mobs */
     60,                        /* hp factor for mobs */
     60,                        /* hitroll/damroll factor for mobs */
     60,                        /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     90,                        /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DEMON */
    {"demon",                   /* name as found by triggers */
     "demon",                   /* all race keywords */
     "&1&bDemon&0",             /* name as displayed at character creation, on who, and score */
     "&1&bDemon&0",             /* name as displayed in show race command and when setting races */
     "Demon",                   /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_LARGE,                /* default size */
     -1000,                     /* default alignment */
     6,                         /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_DEMONIC,              /* default life force */
     COMP_FLESH,                /* default composition */
     130,                       /* male weight low */
     290,                       /* male weight high */
     72,                        /* male height low */
     90,                        /* male height high */
     130,                       /* female weight low */
     290,                       /* female weight high */
     72,                        /* female height low */
     90,                        /* female height high */
     {80, 100, 68, 68, 58, 58}, /* max stat: str, dex, int, wis, con, cha */
     120,                       /* experience reward factor for mobs */
     120,                       /* hp factor for mobs */
     120,                       /* hitroll/damroll factor for mobs */
     120,                       /* damage dice factor for mobs */
     150,                       /* money drop factor for mobs */
     120,                       /* AC factor for mobs */
     "stalks in",               /* verb for entering a room */
     "stalks",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* BROWNIE */
    {"brownie",                 /* name as found by triggers */
     "brownie",                 /* all race keywords */
     "&3Brownie&0",             /* name as displayed at character creation, on who, and score */
     "&3Brownie&0",             /* name as displayed in show race command and when setting races */
     "Brownie",                 /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_SMALL,                /* default size */
     500,                       /* default alignment */
     1,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     20,                        /* male weight low */
     30,                        /* male weight high */
     20,                        /* male height low */
     30,                        /* male height high */
     20,                        /* female weight low */
     30,                        /* female weight high */
     20,                        /* female height low */
     30,                        /* female height high */
     {60, 80, 60, 78, 70, 72},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGON - FIRE */
    {"dragon_fire",             /* name as found by triggers */
     "dragon fire",             /* all race keywords */
     "&1&bDragon&0",            /* name as displayed at character creation, on who, and score */
     "&1&bDragon&0",            /* name as displayed in show race command and when setting races */
     "Fire Dragon",             /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_GARGANTUAN,           /* default size */
     0,                         /* default alignment */
     10,                        /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     16000,                     /* male weight low */
     64000,                     /* male weight high */
     768,                       /* male height low */
     1536,                      /* male height high */
     16000,                     /* female weight low */
     64000,                     /* female weight high */
     768,                       /* female height low */
     1536,                      /* female height high */
     {100, 72, 100, 72, 72, 100}, /* max stat: str, dex, int, wis, con, cha */
     130,                       /* experience reward factor for mobs */
     130,                       /* hp factor for mobs */
     140,                       /* hitroll/damroll factor for mobs */
     140,                       /* damage dice factor for mobs */
     500,                       /* money drop factor for mobs */
     140,                       /* AC factor for mobs */
     "stomps in",               /* verb for entering a room */
     "stomps",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGON - ICE */
    {"dragon_frost",            /* name as found by triggers */
     "dragon frost",            /* all race keywords */
     "&7&bDragon&0",            /* name as displayed at character creation, on who, and score */
     "&7&bDragon&0",            /* name as displayed in show race command and when setting races */
     "Frost Dragon",            /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_GARGANTUAN,           /* default size */
     0,                         /* default alignment */
     10,                        /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     16000,                     /* male weight low */
     64000,                     /* male weight high */
     768,                       /* male height low */
     1536,                      /* male height high */
     16000,                     /* female weight low */
     64000,                     /* female weight high */
     768,                       /* female height low */
     1536,                      /* female height high */
     {100, 72, 100, 72, 72, 100}, /* max stat: str, dex, int, wis, con, cha */
     130,                       /* experience reward factor for mobs */
     130,                       /* hp factor for mobs */
     140,                       /* hitroll/damroll factor for mobs */
     140,                       /* damage dice factor for mobs */
     500,                       /* money drop factor for mobs */
     140,                       /* AC factor for mobs */
     "stomps in",               /* verb for entering a room */
     "stomps",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGON - ACID */
    {"dragon_acid",             /* name as found by triggers */
     "dragon acid",             /* all race keywords */
     "&9&bDragon&0",            /* name as displayed at character creation, on who, and score */
     "&9&bDragon&0",            /* name as displayed in show race command and when setting races */
     "Acid Dragon",             /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_GARGANTUAN,           /* default size */
     0,                         /* default alignment */
     10,                        /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     16000,                     /* male weight low */
     64000,                     /* male weight high */
     768,                       /* male height low */
     1536,                      /* male height high */
     16000,                     /* female weight low */
     64000,                     /* female weight high */
     768,                       /* female height low */
     1536,                      /* female height high */
     {100, 72, 100, 72, 72, 100}, /* max stat: str, dex, int, wis, con, cha */
     130,                       /* experience reward factor for mobs */
     130,                       /* hp factor for mobs */
     140,                       /* hitroll/damroll factor for mobs */
     140,                       /* damage dice factor for mobs */
     500,                       /* money drop factor for mobs */
     140,                       /* AC factor for mobs */
     "stomps in",               /* verb for entering a room */
     "stomps",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGON - LIGHTNING */
    {"dragon_lightning",        /* name as found by triggers */
     "dragon lightning",        /* all race keywords */
     "&4&bDragon&0",            /* name as displayed at character creation, on who, and score */
     "&4&bDragon&0",            /* name as displayed in show race command and when setting races */
     "Lightning Dragon",        /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_GARGANTUAN,           /* default size */
     0,                         /* default alignment */
     10,                        /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     16000,                     /* male weight low */
     64000,                     /* male weight high */
     768,                       /* male height low */
     1536,                      /* male height high */
     16000,                     /* female weight low */
     64000,                     /* female weight high */
     768,                       /* female height low */
     1536,                      /* female height high */
     {100, 72, 100, 72, 72, 100}, /* max stat: str, dex, int, wis, con, cha */
     130,                       /* experience reward factor for mobs */
     130,                       /* hp factor for mobs */
     140,                       /* hitroll/damroll factor for mobs */
     140,                       /* damage dice factor for mobs */
     500,                       /* money drop factor for mobs */
     140,                       /* AC factor for mobs */
     "stomps in",               /* verb for entering a room */
     "stomps",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGON - GAS */
    {"dragon_gas",              /* name as found by triggers */
     "dragon gas",              /* all race keywords */
     "&2&bDragon&0",            /* name as displayed at character creation, on who, and score */
     "&2&bDragon&0",            /* name as displayed in show race command and when setting races */
     "Gas Dragon",              /* name as displayed in medit vsearch stat and enlightenment */
     false,                     /* playable? */
     false,                     /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_GARGANTUAN,           /* default size */
     0,                         /* default alignment */
     10,                        /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     16000,                     /* male weight low */
     64000,                     /* male weight high */
     768,                       /* male height low */
     1536,                      /* male height high */
     16000,                     /* female weight low */
     64000,                     /* female weight high */
     768,                       /* female height low */
     1536,                      /* female height high */
     {100, 72, 100, 72, 72, 100}, /* max stat: str, dex, int, wis, con, cha */
     130,                       /* experience reward factor for mobs */
     130,                       /* hp factor for mobs */
     140,                       /* hitroll/damroll factor for mobs */
     140,                       /* damage dice factor for mobs */
     500,                       /* money drop factor for mobs */
     140,                       /* AC factor for mobs */
     "stomps in",               /* verb for entering a room */
     "stomps",                  /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGONBORN - FIRE */
    {"dragonborn_fire",         /* name as found by triggers */
     "dragonborn fire",         /* all race keywords */
     "&1Dr&ba&3g&1on&0&1b&1&bo&3r&1&bn&0",  /* name as displayed at character creation, on who, and score */
     "&1Fire Dragonborn&0",     /* name as displayed in show race command and when setting races */
     "Fire Dragonborn",         /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     180,                       /* male weight low */
     370,                       /* male weight high */
     70,                        /* male height low */
     80,                        /* male height high */
     180,                       /* female weight low */
     370,                       /* female weight high */
     70,                        /* female height low */
     80,                        /* female height high */
     {78, 64, 76, 72, 78, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGONBORN - FROST */
    {"dragonborn_frost",        /* name as found by triggers */
     "dragonborn frost",        /* all race keywords */
     "&7&bDr&b&4ag&7&bonb&b&4or&7&bn&0",  /* name as displayed at character creation, on who, and score */
     "&7&bFrost Dragonborn&0",  /* name as displayed in show race command and when setting races */
     "Frost Dragonborn",        /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     180,                       /* male weight low */
     370,                       /* male weight high */
     70,                        /* male height low */
     80,                        /* male height high */
     180,                       /* female weight low */
     370,                       /* female weight high */
     70,                        /* female height low */
     80,                        /* female height high */
     {78, 64, 76, 72, 78, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGONBORN - ACID */
    {"dragonborn_acid",         /* name as found by triggers */
     "dragonborn acid",         /* all race keywords */
     "&9&bDr&2a&0&2g&bo&9nb&2o&0&2r&b&9n&0",  /* name as displayed at character creation, on who, and score */
     "&9&bAcid Dragonborn&0",   /* name as displayed in show race command and when setting races */
     "Acid Dragonborn",         /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     180,                       /* male weight low */
     370,                       /* male weight high */
     70,                        /* male height low */
     80,                        /* male height high */
     180,                       /* female weight low */
     370,                       /* female weight high */
     70,                        /* female height low */
     80,                        /* female height high */
     {78, 64, 76, 72, 78, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGONBORN - LIGHTNING */
    {"dragonborn_lightning",    /* name as found by triggers */
     "dragonborn lightning",    /* all race keywords */
     "&b&4Dr&6a&4go&6n&4b&6or&4n&0",  /* name as displayed at character creation, on who, and score */
     "&b&4Lightning Dragonborn&0",    /* name as displayed in show race command and when setting races */
     "Lightning Dragonborn",    /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     180,                       /* male weight low */
     370,                       /* male weight high */
     70,                        /* male height low */
     80,                        /* male height high */
     180,                       /* female weight low */
     370,                       /* female weight high */
     70,                        /* female height low */
     80,                        /* female height high */
     {78, 64, 76, 72, 78, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* DRAGONBORN - GAS */
    {"dragonborn_gas",          /* name as found by triggers */
     "dragonborn gas",          /* all race keywords */
     "&2&bDra&3g&2onb&3or&2n&0",  /* name as displayed at character creation, on who, and score */
     "&2&bGas Dragonborn&0",    /* name as displayed in show race command and when setting races */
     "Gas Dragonborn",          /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     0,                         /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     180,                       /* male weight low */
     370,                       /* male weight high */
     70,                        /* male height low */
     80,                        /* male height high */
     180,                       /* female weight low */
     370,                       /* female weight high */
     70,                        /* female height low */
     80,                        /* female height high */
     {78, 64, 76, 72, 78, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* SVERFNEBLIN */
    {"sverfneblin",             /* name as found by triggers */
     "sverfneblin",             /* all race keywords */
     "&9&d&bSverfneblin&0",     /* name as displayed at character creation, on who, and score */
     "&9&d&bSverfneblin&0",     /* name as displayed in show race command and when setting races */
     "Sverfneblin",             /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_SMALL,                /* default size */
     -1000,                     /* default alignment */
     2,                         /* damroll bonus */
     4,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     40,                        /* male weight low */
     90,                        /* male weight high */
     26,                        /* male height low */
     38,                        /* male height high */
     40,                        /* female weight low */
     90,                        /* female weight high */
     26,                        /* female height low */
     38,                        /* female height high */
     {64, 76, 88, 84, 68, 76},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* FAERIE - SEELIE */
    {"faerie_seelie",           /* name as found by triggers */
     "faerie seelie",           /* all race keywords */
     "&b&7F&3aeri&7e&0",        /* name as displayed at character creation, on who, and score */
     "&b&7S&3eeli&7e F&3aeri&7e&0", /* name as displayed in show race command and when setting races */
     "Seelie Faerie",           /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_TINY,                 /* default size */
     1000,                      /* default alignment */
     0,                         /* damroll bonus */
     6,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     5,                         /* male weight low */
     10,                        /* male weight high */
     6,                         /* male height low */
     10,                        /* male height high */
     5,                         /* female weight low */
     10,                        /* female weight high */
     6,                         /* female height low */
     10,                        /* female height high */
     {30, 95, 90, 90, 30, 95},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     80,                        /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     "flutters in",             /* verb for entering a room */
     "flutters",                /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* FAERIE - UNSEELIE */
    {"faerie_unseelie",         /* name as found by triggers */
     "faerie unseelie",         /* all race keywords */
     "&b&9F&0&5aeri&9&be&0",    /* name as displayed at character creation, on who, and score */
     "&b&9U&0&5ns&b&9ee&0&5li&b&9e F&0&5aeri&9&be&0", /* name as displayed in show race command and when setting races */
     "Unseelie Faerie",         /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_EVIL,           /* race alignment */
     SIZE_TINY,                 /* default size */
     -1000,                     /* default alignment */
     0,                         /* damroll bonus */
     6,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     5,                         /* male weight low */
     10,                        /* male weight high */
     6,                         /* male height low */
     10,                        /* male height high */
     5,                         /* female weight low */
     10,                        /* female weight high */
     6,                         /* female height low */
     10,                        /* female height high */
     {30, 95, 90, 90, 30, 95},  /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     80,                        /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     "flutters in",             /* verb for entering a room */
     "flutters",                /* verb for walking out of a room */
     {0, 0}},                   /* function numbers - always set as {0, 0}}, to close struct */

    /* NYMPH */
    {"nymph",                   /* name as found by triggers */
     "nymph",                   /* all race keywords */
     "&3&bN&0&2ym&3&bph&0",     /* name as displayed at character creation, on who, and score */
     "&3&bN&0&2ym&3&bph&0",     /* name as displayed in show race command and when setting races */
     "Nymph",                   /* name as displayed in medit vsearch stat and enlightenment */
     true,                      /* playable? */
     true,                      /* humanoid? */
     RACE_ALIGN_GOOD,           /* race alignment */
     SIZE_MEDIUM,               /* default size */
     1000,                      /* default alignment */
     3,                         /* damroll bonus */
     3,                         /* hitroll bonus */
     LIFE_LIFE,                 /* default life force */
     COMP_FLESH,                /* default composition */
     90,                        /* male weight low */
     160,                       /* male weight high */
     59,                        /* male height low */
     70,                        /* male height high */
     68,                        /* female weight low */
     160,                       /* female weight high */
     59,                        /* female height low */
     68,                        /* female height high */
     {65, 72, 80, 80, 65, 100}, /* max stat: str, dex, int, wis, con, cha */
     100,                       /* experience reward factor for mobs */
     100,                       /* hp factor for mobs */
     100,                       /* hitroll/damroll factor for mobs */
     100,                       /* damage dice factor for mobs */
     75,                        /* money drop factor for mobs */
     100,                       /* AC factor for mobs */
     nullptr,                   /* verb for entering a room */
     nullptr,                   /* verb for walking out of a room */
     {0, 0}}                   /* function numbers - always set as {0, 0}}, to close struct */
     };
     
const char *race_align_abbrevs[] = {"&0&3&bGOOD&0", "&0&1&bEVIL&0"};

void init_races(void) {
#define PERM_EFF(r, f) SET_FLAG(races[(r)].effect_flags, (f))
#define ADD_SKILL(s, p)                                                                                                \
    do {                                                                                                               \
        races[race].skills[pos].skill = (s);                                                                           \
        races[race].skills[pos].proficiency = (p);                                                                     \
        ++pos;                                                                                                         \
    } while (0)

    int race, pos;

    /*
     * Add permanent effects to races here.
     */

    PERM_EFF(RACE_BROWNIE, EFF_INFRAVISION);

    PERM_EFF(RACE_DRAGON_ACID, EFF_FLY);
    PERM_EFF(RACE_DRAGON_FIRE, EFF_FLY);
    PERM_EFF(RACE_DRAGON_FROST, EFF_FLY);
    PERM_EFF(RACE_DRAGON_GAS, EFF_FLY);
    PERM_EFF(RACE_DRAGON_LIGHTNING, EFF_FLY);  
    PERM_EFF(RACE_DRAGON_GENERAL, EFF_FLY);

    PERM_EFF(RACE_DRAGONBORN_FIRE, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_FROST, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_ACID, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_LIGHTNING, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_GAS, EFF_INFRAVISION);

    PERM_EFF(RACE_DROW, EFF_INFRAVISION);
    PERM_EFF(RACE_DROW, EFF_ULTRAVISION);

    PERM_EFF(RACE_DUERGAR, EFF_INFRAVISION);
    PERM_EFF(RACE_DUERGAR, EFF_ULTRAVISION);

    PERM_EFF(RACE_DWARF, EFF_DETECT_POISON);
    PERM_EFF(RACE_DWARF, EFF_INFRAVISION);
    PERM_EFF(RACE_DWARF, EFF_ULTRAVISION);
    
    PERM_EFF(RACE_ELF, EFF_INFRAVISION);

    PERM_EFF(RACE_FAERIE_SEELIE, EFF_FLY);

    PERM_EFF(RACE_FAERIE_UNSEELIE, EFF_FLY);

    PERM_EFF(RACE_GNOME, EFF_INFRAVISION);

    PERM_EFF(RACE_HALF_ELF, EFF_INFRAVISION);

    PERM_EFF(RACE_HALFLING, EFF_INFRAVISION);
    PERM_EFF(RACE_HALFLING, EFF_SENSE_LIFE);

    PERM_EFF(RACE_OGRE, EFF_INFRAVISION);
    PERM_EFF(RACE_OGRE, EFF_ULTRAVISION);

    PERM_EFF(RACE_SVERFNEBLIN, EFF_INFRAVISION);
    PERM_EFF(RACE_SVERFNEBLIN, EFF_ULTRAVISION);
    
    PERM_EFF(RACE_TROLL, EFF_INFRAVISION);
    PERM_EFF(RACE_TROLL, EFF_ULTRAVISION);

    /*
     * Add race skills to the switch below.
     * If a constant value is declared, the skill will always reset back to that value.
     * Use 'proficiency' or 'ROLL_SKILL_PROF' instead.
     */
    for (race = 0; race < NUM_RACES; ++race) {
        memset(races[race].skills, 0, sizeof(races[race].skills));
        pos = 0;
        switch (race) {
        case RACE_ELF:
            ADD_SKILL(SKILL_SLASHING, ROLL_SKILL_PROF);
            break;
        case RACE_TROLL:
            ADD_SKILL(SKILL_DOORBASH, 1000);
            ADD_SKILL(SKILL_BODYSLAM, 1000);
            break;
        case RACE_OGRE:
            ADD_SKILL(SKILL_DOORBASH, 1000);
            ADD_SKILL(SKILL_BODYSLAM, 1000);
            break;
        case RACE_BARBARIAN:
            ADD_SKILL(SKILL_DOORBASH, 1000);
            ADD_SKILL(SKILL_BODYSLAM, 1000);
            break;
        case RACE_DRAGON_GENERAL:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGON_FIRE:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_FIRE:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            break;
        case RACE_DRAGON_FROST:
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_FROST:
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            break;
        case RACE_DRAGON_ACID:
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_ACID:
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            break;
        case RACE_DRAGON_LIGHTNING:
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_LIGHTNING:
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGON_GAS:
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_GAS:
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            break;
        case RACE_DEMON:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_BROWNIE:
            ADD_SKILL(SKILL_SNEAK, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_HIDE, ROLL_SKILL_PROF);
            break;
        }
        if (pos > NUM_RACE_SKILLS) {
            log("init_races: Too many skills assigned to race {}.  Increase NUM_RACE_SKILLS in races.h to at least {:d}",
                races[race].name, pos);
            exit(1);
        }
    }

    CLEAR_FLAGS(race_effects_mask, NUM_EFF_FLAGS);
    for (race = 0; race < NUM_RACES; ++race)
        SET_FLAGS(race_effects_mask, races[race].effect_flags, NUM_EFF_FLAGS);

#undef ADD_SKILL
#undef PERM_EFF
}

/* parse_race
 *
 * Identifies a race from a string.  Will do partial matches.
 *
 * Code is present to prohibit a player from being set to the wrong
 * race, but it's disabled.  If it were enabled, it would only take
 * effect if "vict" were not null.
 *
 * ch is someone who's trying to change vict's race (e.g., a wizard
 * manually setting someone to a race due to a quest).
 *
 * If RACE_UNDEFINED is returned, this function will already have provided
 * feedback to ch (if specified) as to the reason for the failure.  Otherwise,
 * it does not provide feedback.
 */
int parse_race(CharData *ch, CharData *vict, char *arg) {
    int i, race = RACE_UNDEFINED, altname = RACE_UNDEFINED, best = RACE_UNDEFINED;

    if (!*arg) {
        if (ch)
            char_printf(ch, "What race?\n");
        return RACE_UNDEFINED;
    }

    for (i = 0; i < NUM_RACES; i++) {
        if (!strncasecmp(arg, races[i].name, strlen(arg))) {
            if (!strcasecmp(arg, races[i].name)) {
                race = i;
                break;
            }
            if (best == RACE_UNDEFINED)
                best = i;
        } else if (isname(arg, races[i].names)) {
            if (altname == RACE_UNDEFINED)
                altname = i;
        } else if (is_abbrev(arg, races[i].name)) {
            if (best == RACE_UNDEFINED)
                best = i;
        }
    }

    if (race == RACE_UNDEFINED)
        race = altname;
    if (race == RACE_UNDEFINED)
        race = best;
    if (race == RACE_UNDEFINED) {
        if (ch)
            char_printf(ch, "There is no such race.\n");
    }

    /* There are no validity checks. */
    return race;

    /* The following code could be used to prevent deities from assigning
     * a race racee to a player if:
     *
     *  - The race is not "playable"
     *  - The player's race does not allow the race
     *
     * It's currently not used. */

    /* Bypass validity checks for immortal victims (or no specified victim). */
    if (!vict || GET_LEVEL(vict) > LVL_MAX_MORT)
        return race;

    /* The race has been identified, and there is a mortal victim.
     * Make sure this race is available to the victim. */

    if (!races[race].playable) {
        if (ch) {
            char_printf(ch, "The {} race is not available to mortals.\n", races[race].plainname);
        }
        return RACE_UNDEFINED;
    }

    if (!class_ok_race[race][(int)GET_CLASS(vict)]) {
        if (ch) {
            sprintf(buf, "As %s, $n can't be %s.", with_indefinite_article(classes[(int)GET_CLASS(vict)].displayname),
                    with_indefinite_article(races[race].displayname));
            act(buf, false, vict, 0, ch, TO_VICT);
        }
        return RACE_UNDEFINED;
    }

    return race;
}

/* Send a menu to someone who's creating a character, listing the available
 * races.  We assume that this function would not have been called if
 * "races_allowed" were false. */
void send_race_menu(DescriptorData *d) {
    extern int evil_races_allowed;
    char idx;
    int i;

    for (i = 0, idx = 'a'; i < NUM_RACES; i++) {
        if (races[i].playable && (evil_races_allowed || races[i].racealign == RACE_ALIGN_GOOD)) {
            sprintf(buf, "  &7%c)&0 %s\n", idx, races[i].fullname);
            string_to_output(d, buf);
            idx++;
        }
    }
}

/* Someone who's creating a character typed a letter to indicate which
 * race they wanted.  Determine which race they indicated, using the same
 * rules as send_race_menu() -- skip over inactive/unavailable races. */
int interpret_race_selection(char arg) {
    extern int evil_races_allowed;
    char idx;
    int i;

    for (i = 0, idx = 'a'; i < NUM_RACES; i++) {
        if (races[i].playable && (evil_races_allowed || races[i].racealign == RACE_ALIGN_GOOD)) {
            if (arg == idx)
                return i;
            idx++;
        }
    }
    return RACE_UNDEFINED;
}

/* Oddly enough, the base value for movement points is not stored
 * anywhere.  Thus, it would be impossible to increase the value
 * as a player advances in level.  Anyway, the same value gets set,
 * based on CON, whenever a player logs in.
 *
 * There are times when you want to see this unaffected value, so
 * here's the function to find it.  This is also used at character
 * creation time and when logging. */
int natural_move(CharData *ch) {
    if (IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0) {

        /* Mountable mobs will have their mv points set according to level.
         * The second parameter to pow (now 0.8) controls how the points
         * increase as the level increases. If it were 1, the points would
         * increase in a straight line as the level increases. If it were
         * greater than 1, the points would increase slowly at first, and
         * then sharply curve up to their maximum as the level got close
         * to the maximum level. When it's below 1, the points increase
         * quickly at first, and then slowly reach their maximum. */

        if (MOB_FLAGGED(ch, MOB_MOUNTABLE)) {
            if (GET_LEVEL(ch) > MAX_MOUNT_LEVEL)
                return MOUNT_MAXMOVE + 2 * (GET_LEVEL(ch) - MAX_MOUNT_LEVEL) + random_number(0, 9);
            else
                return (int)(MOUNT_MINMOVE + (MOUNT_MAXMOVE - MOUNT_MINMOVE) *
                                                 pow((GET_LEVEL(ch) - 1) / (double)(MAX_MOUNT_LEVEL - 1), 0.8)) +
                       random_number(0, 9);
        } else
            return mob_proto[GET_MOB_RNUM(ch)].points.max_move;
    } else {
        return std::max(100, GET_CON(ch) * 2);
    }
}

/* init_proto_race()
 *
 * Sets beginning values on a mob prototype, according to race.
 */

void init_proto_race(CharData *ch) {
    set_base_size(ch, races[(int)GET_RACE(ch)].def_size);
    GET_LIFEFORCE(ch) = races[(int)GET_RACE(ch)].def_lifeforce;
    BASE_COMPOSITION(ch) = races[(int)GET_RACE(ch)].def_composition;
    GET_COMPOSITION(ch) = BASE_COMPOSITION(ch);
}

/* init_char_race()
 *
 * Sets beginning values that are appropriate for a brand-new character,
 * according to race. */

void init_char_race(CharData *ch) {
    if (!IS_NPC(ch) && VALID_RACE(ch)) {
        GET_BASE_DAMROLL(ch) = races[(int)GET_RACE(ch)].bonus_damroll;
        GET_BASE_HITROLL(ch) = races[(int)GET_RACE(ch)].bonus_hitroll;
    }

    /* NPCs will have their own align defined at build time,
     * and it might have been adjusted by the builder, too. */
    if (!IS_NPC(ch) && VALID_RACE(ch))
        GET_ALIGNMENT(ch) = races[(int)GET_RACE(ch)].def_align;
    set_init_height_weight(ch);

    GET_MAX_MOVE(ch) = natural_move(ch);
}

void update_char_race(CharData *ch) {
    if (!VALID_RACE(ch)) {
        log("update_char_race: {} doesn't have a valid race ({:d}).", GET_NAME(ch), GET_RACE(ch));
        return;
    }

    GET_RACE_ALIGN(ch) = races[(int)GET_RACE(ch)].racealign;

    /* Any bits that might get set below should be cleared here first. */
    REMOVE_FLAGS(EFF_FLAGS(ch), race_effects_mask, NUM_EFF_FLAGS);

    /* Reset effect flags for this race */
    SET_FLAGS(EFF_FLAGS(ch), races[(int)GET_RACE(ch)].effect_flags, NUM_EFF_FLAGS);
}

/*
 * Returns a positive value for skills that this race has.
 *
 * Doesn't disqualify any skills! Only enables them.
 */

int racial_skill_proficiency(int skill, int race, int level) {
    int i;

    for (i = 0; races[race].skills[i].skill > 0 && i < NUM_RACE_SKILLS; ++i)
        if (races[race].skills[i].skill == skill) {
            return races[race].skills[i].proficiency;
        }

    return 0;
}

/* convert_race does no checking.  It expects a valid race and ch.
 * This function changes a player's race and converts the skills/spells
 * accordingly, keeping the old values if they are better.
 * It also transfers quest spells. */
void convert_race(CharData *ch, int newrace) {
    int skill;
    sh_int old_skills[TOP_SKILL + 1];
    sh_int new_skills[TOP_SKILL + 1];

    /* read in the player's old skills */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        old_skills[skill] = GET_ISKILL(ch, skill);
    }

    /* set race/align */
    GET_RACE(ch) = newrace;

    /* Big changes occur here: */
    update_char(ch);

    /* read the new skills */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        new_skills[skill] = GET_ISKILL(ch, skill);
    }

    /* compare old and new */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        if (new_skills[skill]) {
            /* keep the value of the old skill if you still have the skill */
            if (old_skills[skill] > new_skills[skill]) {
                SET_SKILL(ch, skill, old_skills[skill]);
            }
        }

        /* keep any quest spells you might have earned */
        if ((old_skills[skill]) && (skills[skill].quest)) {
            SET_SKILL(ch, skill, old_skills[skill]);
        }
    }
    check_regen_rates(ch);
}

void scale_attribs(CharData *ch) {
    if (VALID_RACE(ch)) {
        GET_AFFECTED_STR(ch) = (GET_VIEWED_STR(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_STR - 1]) / 100;
        GET_AFFECTED_DEX(ch) = (GET_VIEWED_DEX(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_DEX - 1]) / 100;
        GET_AFFECTED_INT(ch) = (GET_VIEWED_INT(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_INT - 1]) / 100;
        GET_AFFECTED_WIS(ch) = (GET_VIEWED_WIS(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_WIS - 1]) / 100;
        GET_AFFECTED_CON(ch) = (GET_VIEWED_CON(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_CON - 1]) / 100;
        GET_AFFECTED_CHA(ch) = (GET_VIEWED_CHA(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_CHA - 1]) / 100;
    } else {
        GET_AFFECTED_STR(ch) = GET_VIEWED_STR(ch) * 72 / 100;
        GET_AFFECTED_DEX(ch) = GET_VIEWED_DEX(ch) * 72 / 100;
        GET_AFFECTED_INT(ch) = GET_VIEWED_INT(ch) * 72 / 100;
        GET_AFFECTED_WIS(ch) = GET_VIEWED_WIS(ch) * 72 / 100;
        GET_AFFECTED_CON(ch) = GET_VIEWED_CON(ch) * 72 / 100;
        GET_AFFECTED_CHA(ch) = GET_VIEWED_CHA(ch) * 72 / 100;
    }
}

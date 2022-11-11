/***************************************************************************
 *  File: class.h                                         Part of FieryMUD *
 *  Usage: header file for class structures and constants                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "chars.hpp"
#include "objects.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define CLASS_UNDEFINED -1
#define CLASS_SORCERER 0
#define CLASS_CLERIC 1
#define CLASS_THIEF 2
#define CLASS_WARRIOR 3
#define CLASS_PALADIN 4
#define CLASS_ANTI_PALADIN 5
#define CLASS_RANGER 6
#define CLASS_DRUID 7
#define CLASS_SHAMAN 8
#define CLASS_ASSASSIN 9
#define CLASS_MERCENARY 10
#define CLASS_NECROMANCER 11
#define CLASS_CONJURER 12
#define CLASS_MONK 13
#define CLASS_BERSERKER 14
#define CLASS_PRIEST 15
#define CLASS_DIABOLIST 16
#define CLASS_MYSTIC 17
#define CLASS_ROGUE 18
#define CLASS_BARD 19
#define CLASS_PYROMANCER 20
#define CLASS_CRYOMANCER 21
#define CLASS_ILLUSIONIST 22
#define CLASS_HUNTER 23
#define CLASS_LAYMAN 24
/* NUM_CLASSES defined in chars.h */

#define CLASS_DEFAULT CLASS_LAYMAN

/* How do class members get spells ready to cast? */
#define MEM_NONE -1
#define MEMORIZE 0
#define PRAY 1

/* This defines the basic attributes of a class. */

struct ClassDef {
    char name[80];            /* The unadorned name (i.e., without colors) */
    char altname[80];         /* An alternative name for searching purposes: anti-paladin
                                 and antipaladin */
    char displayname[80];     /* The name with colors and strategic capitalization */
    char plainname[80];       /* The name with capitalization, but no colors */
    char fmtname[80];         /* The full name, formatted to take up 12 spaces */
    char abbrev[80];          /* The three-character abbreviation, with colors */
    char stars[40];           /* The two asterisks for maxed-out mortals, with colors */
    bool magical;             /* Do they cast spells? */
    int mem_mode;             /* Memorize or pray? */
    bool active;              /* Whether this class is available to mortals */
    bool is_subclass;         /* Is it a subclass of any other class? */
    int subclass_of;          /* The class that it's a subclass of */
    int max_subclass_level;   /* Highest level at which you can subclass (if base
                                 class) */
    int homeroom;             /* Room you'll start in.  Used in character creation only. */
    int statorder[NUM_STATS]; /* When generating a new character, which stats
                                 should be highest? */
    int saves[NUM_SAVES];     /* Class-related base saving throws */
    int hp_lev;               /* How many hit points gained per level */
    int thac0;                /* How good you are at hitting people. Lower=better */
    int nowear_flag;          /* What ITEM_ flag will prevent wearing an object? */
    int hit_regen_factor;     /* Factor: 100 = normal regen */
    int mana_regen_factor;
    int mv_regen_factor;
    double exp_gain_factor; /* Exp factor: 1 = normal exp needed to gain a level */

    /* The following values primarily adjust stats on mob prototypes.  See db.c.
     */
    int exp_factor;
    int hit_factor;
    int hd_factor;
    int dice_factor;
    int copper_factor;
    int ac_factor;

    int newbie_eq[20]; /* Objects given to new player characters (in addition to
                          common newbie eq */

    /*
     * The following data members should come last in the struct so
     * that they can be omitted when classes are being defined.  Their
     * values are filled in init_classes at runtime.
     */

    /* List of permanent effect flags */
    flagvector effect_flags[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
};

/* classes[]
 *
 * The individual members of these struct definitions have been arranged
 * in a regular fashion, so that they can easily be located.  Please maintain
 * this arrangement:
 *
 *   name, altname, displayname, plainname,
 *   fmtname, abbrev, stars,
 *   magical, mem_mode, active, is_subclass, subclass_of, max_subclass_level,
 * homeroom statorder saves hp_lev, thac0, nowear_flag, hit_regen_factor,
 * mana_regen_factor, mv_regen_factor, exp_gain_factor, exp_factor, hit_factor,
 * hd_factor, dice_factor, copper_factor, ac_factor, newbie_eq[]
 *
 * The is_subclass value of inactive classes (such as mystic) should be taken
 * with a grain of salt.  If/when an inactive class is made active, its
 * subclass status should be reevaluated.
 */
struct ClassDef classes[NUM_CLASSES] = {

    /* SORCERER */
    {"sorcerer",
     "",
     "&5&bSorcerer&0",
     "Sorcerer",
     "&5&bSorcerer&0    ",
     "&5&bSor&0",
     "&5&b**&0",
     true,
     MEMORIZE,
     true,
     false,
     CLASS_UNDEFINED,
     45,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     6,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1.2,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 1003, 1027, 1012, -1}},

    /* CLERIC */
    {"cleric",
     "",
     "&6Cleric&0",
     "Cleric",
     "&6Cleric&0      ",
     "&6Cle&0",
     "&6**&0",
     true,
     PRAY,
     true,
     false,
     CLASS_UNDEFINED,
     45,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     4,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {1019, 1022, 1025, 1027, 1011, -1}},

    /* THIEF */
    {"thief",
     "",
     "&1&bThief&0",
     "Thief",
     "&1&bThief&0       ",
     "&1&bThi&0",
     "&1&b**&0",
     false,
     MEM_NONE,
     true,
     true,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 90, 100, 110, 110},
     8,
     1,
     ITEM_ANTI_ROGUE,
     100,
     100,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {1019, 1022, 1025, 1027, 1012, -1}},

    /* WARRIOR */
    {"warrior",
     "",
     "&4&bWarrior&0",
     "Warrior",
     "&4&bWarrior&0     ",
     "&4&bWar&0",
     "&4&b**&0",
     false,
     MEM_NONE,
     true,
     false,
     CLASS_UNDEFINED,
     25,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     12,
     -5,
     ITEM_ANTI_WARRIOR,
     100,
     0,
     100,
     1.1,
     100,
     120,
     120,
     120,
     100,
     120,
     {1019, 1022, 1024, 1026, 1014, -1}},

    /* PALADIN */
    {"paladin",
     "",
     "&8Paladin&0",
     "Paladin",
     "&8Paladin&0     ",
     "&8Pal&0",
     "&8**&0",
     true,
     PRAY,
     true,
     true,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 115, 100, 105, 90},
     10,
     -4,
     ITEM_ANTI_PALADIN,
     100,
     50,
     100,
     1.15,
     100,
     120,
     120,
     120,
     100,
     120,
     {1019, 1022, 1024, 1026, 1014, -1}},

    /* ANTI_PALADIN */
    {"anti-paladin",
     "antipaladin",
     "&1&bAnti-&9Paladin&0",
     "Anti-Paladin",
     "&1&bAnti-&9Paladin&0",
     "&1&bAnt&0",
     "&1&b**&0",
     true,
     PRAY,
     true,
     true,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 115, 100, 105, 90},
     10,
     -4,
     ITEM_ANTI_ANTI_PALADIN,
     100,
     50,
     100,
     1.15,
     100,
     120,
     120,
     120,
     100,
     120,
     {1019, 1022, 1024, 1026, 1014, -1}},

    /* RANGER */
    {"ranger",
     "",
     "&2&bRanger&0",
     "Ranger",
     "&2&d&bRanger&0      ",
     "&2&bRan&0",
     "&2&b**&0",
     true,
     MEMORIZE,
     true,
     true,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_INT, STAT_WIS, STAT_CHA},
     {95, 115, 100, 105, 90},
     10,
     -4,
     ITEM_ANTI_RANGER,
     100,
     50,
     100,
     1.15,
     100,
     120,
     120,
     120,
     100,
     120,
     {1019, 1022, 1024, 1026, 1014, -1}},

    /* DRUID */
    {"druid",
     "",
     "&2Druid&0",
     "Druid",
     "&2Druid&0       ",
     "&2Dru&0",
     "&2**&0",
     true,
     PRAY,
     true,
     true,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     5,
     ITEM_ANTI_DRUID,
     85,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {1019, 1022, 1025, 1027, 1011, -1}},

    /* SHAMAN */
    {"shaman",
     "",
     "&6&bShaman&0",
     "Shaman",
     "&6&bShaman&0      ",
     "&6&bSha&0",
     "&6&b**&0",
     true,
     MEM_NONE,
     false,
     false,
     CLASS_UNDEFINED,
     -1,
     3001,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {105, 115, 105, 110, 110},
     8,
     6,
     ITEM_ANTI_SHAMAN,
     80,
     100,
     100,
     1,
     100,
     100,
     100,
     100,
     100,
     100,
     {1019, 1022, 1025, 1027, 1011, -1}},

    /* ASSASSIN */
    {"assassin",
     "",
     "&1Assassin&0",
     "Assassin",
     "&1Assassin&0    ",
     "&1Ass&0",
     "&1**&0",
     false,
     MEM_NONE,
     true,
     true,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {95, 90, 100, 110, 110},
     8,
     2,
     ITEM_ANTI_ASSASSIN,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {1019, 1022, 1025, 1027, 1012, -1}},

    /* MERCENARY */
    {"mercenary",
     "",
     "&3Mercenary&0",
     "Mercenary",
     "&3Mercenary&0   ",
     "&3Mer&0",
     "&3**&0",
     false,
     MEM_NONE,
     true,
     true,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_INT, STAT_WIS, STAT_CHA},
     {95, 90, 100, 110, 110},
     11,
     2,
     ITEM_ANTI_MERCENARY,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {1019, 1022, 1024, 1026, 1012, 1014, -1}},

    /* NECROMANCER */
    {"necromancer",
     "",
     "&5Necromancer&0",
     "Necromancer",
     "&5Necromancer&0 ",
     "&5Nec&0",
     "&5**&0",
     true,
     MEMORIZE,
     true,
     true,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_NECROMANCER,
     80,
     100,
     100,
     1.3,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 1003, 26, 30, -1}},

    /* CONJURER */
    {"conjurer",
     "",
     "&3&bConjurer&0",
     "Conjurer",
     "&3&bConjurer&0    ",
     "&3&bCon&0",
     "&3&b**&0",
     true,
     MEMORIZE,
     false,
     true,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_CONJURER,
     80,
     100,
     100,
     1,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 1003, 1027, 1012, -1}},

    /* MONK */
    {"monk",
     "",
     "&9&bM&0&7on&9&bk&0",
     "Monk",
     "&9&bM&0&7on&9&bk&0        ",
     "&9&bMon&0",
     "&9&b**&0",
     false,
     MEM_NONE,
     true,
     true,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_CON, STAT_STR, STAT_DEX, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     10,
     -4,
     ITEM_ANTI_MONK,
     100,
     0,
     200,
     1.3,
     100,
     120,
     120,
     120,
     100,
     120,
     {1019, 1022, 1024, 1026, 1014, -1}},

    /* BERSERKER */
    {"berserker",
     "",
     "&9&bBer&1ser&9ker&0",
     "Berserker",
     "&9&bBer&1ser&9ker&0   ",
     "&9&bBe&1r&0",
     "&9&b**&0",
     false,
     MEM_NONE,
     false,
     true,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     12,
     -4,
     ITEM_ANTI_BERSERKER,
     100,
     0,
     100,
     1.1,
     100,
     120,
     120,
     120,
     100,
     120,
     {1019, 1022, 1024, 1026, 1014, -1}},

    /* PRIEST */
    {"priest",
     "",
     "&6&bPr&7ie&6st&0",
     "Priest",
     "&6&bPr&7ie&6st&0      ",
     "&6&bPr&7i&0",
     "&6&b**&0",
     true,
     PRAY,
     true,
     true,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     5,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {1019, 1022, 1025, 1027, 1011, -1}},

    /* DIABOLIST */
    {"diabolist",
     "",
     "&5Dia&9&bbol&0&5ist&0",
     "Diabolist",
     "&5Dia&9&bbol&0&5ist&0   ",
     "&5Di&9&ba&0",
     "&5**&0",
     true,
     PRAY,
     true,
     true,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     5,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {1019, 1022, 1025, 1027, 1011, -1}},

    /* MYSTIC */
    {"mystic",
     "",
     "&7&bM&0&7ys&9&bti&7c&0",
     "Mystic",
     "&7&bM&0&7ys&9&bti&7c&0      ",
     "&7&bM&0&7ys&0",
     "&7&b**&0",
     true,
     PRAY,
     false,
     true,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     7,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {1019, 1022, 1025, 1027, 1011, -1}},

    /* ROGUE */
    {"rogue",
     "",
     "&9&bRogue&0",
     "Rogue",
     "&9&bRogue&0       ",
     "&9&bRog&0",
     "&9&b**&0",
     false,
     MEM_NONE,
     true,
     false,
     CLASS_UNDEFINED,
     25,
     3038,
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 90, 100, 110, 110},
     8,
     2,
     ITEM_ANTI_ROGUE,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {1019, 1022, 1025, 1027, 1012, -1}},

    /* BARD */
    {"bard",
     "",
     "&4B&9&bar&0&4d&0",
     "Bard",
     "&4B&9&bar&0&4d&0        ",
     "&4B&9&bar&0",
     "&4**&0",
     true,
     MEMORIZE,
     false,
     true,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 90, 100, 110, 110},
     9,
     2,
     ITEM_ANTI_BARD,
     100,
     100,
     100,
     1.2,
     100,
     90,
     100,
     100,
     100,
     80,
     {1019, 1022, 1025, 1027, 1012, -1}},

    /* PYROMANCER */
    {"pyromancer",
     "",
     "&1P&byr&0&1o&9&bma&0&7nc&9&ber&0",
     "Pyromancer",
     "&1P&byr&0&1o&9&bma&0&7nc&9&ber&0  ",
     "&1P&byr&0",
     "&1**&0",
     true,
     MEMORIZE,
     true,
     true,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1.2,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 1003, 1027, 1012, -1}},

    /* CRYOMANCER */
    {"cryomancer",
     "",
     "&4C&bry&0&4o&7ma&9&bnc&0&7er&0",
     "Cryomancer",
     "&4C&bry&0&4o&7ma&9&bnc&0&7er&0  ",
     "&4C&bry&0",
     "&4**&0",
     true,
     MEMORIZE,
     true,
     true,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1.2,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 1003, 1027, 1012, -1}},

    /* ILLUSIONIST */
    {"illusionist",
     "",
     "&4I&5l&4l&5u&4s&5i&4o&5n&4i&5s&4t&0",
     "Illusionist",
     "&4I&5l&4l&5u&4s&5i&4o&5n&4i&5s&4t&0 ",
     "&4I&5ll&0",
     "&4**&0",
     true,
     MEMORIZE,
     false,
     true,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1.2,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 1003, 1027, 1012, -1}},

    /* HUNTER */
    {"hunter",
     "",
     "&9&bHun&0&2te&9&br&0",
     "Hunter",
     "&9&bHun&0&2te&9&br&0      ",
     "&9&bHun&0",
     "&9&b**&0",
     false,
     MEM_NONE,
     false,
     true,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     10,
     -4,
     0,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {1019, 1022, 1025, 1027, 1012, -1}},

    /* LAYMAN */
    {"layman",
     "",
     "Layman",
     "Layman",
     "Layman      ",
     "Lay",
     "**",
     false,
     MEM_NONE,
     false,
     false,
     CLASS_UNDEFINED,
     -1,
     3001,
     {STAT_CHA, STAT_CON, STAT_DEX, STAT_STR, STAT_WIS, STAT_INT},
     {100, 100, 100, 100, 100},
     7,
     0,
     0,
     100,
     0,
     100,
     1,
     80,
     100,
     130,
     100,
     75,
     105,
     {26, 27, 30, 38, -1}}};

#define VALID_CLASSNUM(num) (num < NUM_CLASSES)
#define VALID_CLASS(ch) VALID_CLASSNUM(GET_CLASS(ch))
#define CLASS_ABBR(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].abbrev : "--")
#define CLASS_FULL(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].displayname : "--")
#define CLASS_WIDE(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].fmtname : "--")
#define CLASS_STARS(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].stars : "**")
#define CLASS_NAME(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].name : "--")
#define CLASS_PLAINNAME(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].plainname : "--")
#define MEM_MODE(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].mem_mode : MEM_NONE)

#define HIT_REGEN_FACTOR(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].hit_regen_factor : 100)
#define MANA_REGEN_FACTOR(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].mana_regen_factor : 100)
#define MV_REGEN_FACTOR(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].mv_regen_factor : 100)
#define EXP_GAIN_FACTOR(cls) (VALID_CLASSNUM(cls) ? classes[cls].exp_gain_factor : 1)

#define CLASS_EXPFACTOR(cls) (VALID_CLASSNUM(cls) ? classes[cls].exp_factor : 100)
#define CLASS_HITFACTOR(cls) (VALID_CLASSNUM(cls) ? classes[cls].hit_factor : 100)
#define CLASS_HDFACTOR(cls) (VALID_CLASSNUM(cls) ? classes[cls].hd_factor : 100)
#define CLASS_DICEFACTOR(cls) (VALID_CLASSNUM(cls) ? classes[cls].dice_factor : 100)
#define CLASS_COPPERFACTOR(cls) (VALID_CLASSNUM(cls) ? classes[cls].copper_factor : 100)
#define CLASS_ACFACTOR(cls) (VALID_CLASSNUM(cls) ? classes[cls].ac_factor : 100)

#define IS_MAGIC_USER(ch)                                                                                              \
    (VALID_CLASS(ch) ? GET_CLASS(ch) == CLASS_SORCERER || classes[(int)GET_CLASS(ch)].subclass_of == CLASS_SORCERER    \
                     : false)
#define IS_CLERIC(ch)                                                                                                  \
    (VALID_CLASS(ch) ? GET_CLASS(ch) == CLASS_CLERIC || classes[(int)GET_CLASS(ch)].subclass_of == CLASS_CLERIC : false)
#define IS_ROGUE(ch)                                                                                                   \
    (VALID_CLASS(ch) ? GET_CLASS(ch) == CLASS_ROGUE || classes[(int)GET_CLASS(ch)].subclass_of == CLASS_ROGUE : false)
#define IS_WARRIOR(ch)                                                                                                 \
    (VALID_CLASS(ch) ? GET_CLASS(ch) == CLASS_WARRIOR || classes[(int)GET_CLASS(ch)].subclass_of == CLASS_WARRIOR      \
                     : false)

/* Is this character prohibited from wearing this equipment due to a class
 * restriction?  Would return true if, for example, a warrior tried to wear
 * an object with the !WARRIOR wear flag. */
#define NOWEAR_CLASS(ch, obj)                                                                                          \
    (VALID_CLASS(ch)                                                                                                   \
         ? classes[(int)GET_CLASS(ch)].nowear_flag && OBJ_FLAGGED(obj, classes[(int)GET_CLASS(ch)].nowear_flag)        \
         : 0)

enum level_action { LEVEL_GAIN, LEVEL_LOSE };

void init_classes(void);
void assign_class_skills(void);
int parse_class(CharData *ch, CharData *vict, char *arg);
int getbaseclass(int class_num);
void init_char_class(CharData *ch);
void update_char_class(CharData *ch);
void advance_level(CharData *ch, enum level_action);
void convert_class(CharData *ch, int newclass);
int return_max_skill(CharData *ch, int skill);
void give_newbie_eq(CharData *vict);

/***************************************************************************
 *  File: class.c                                        Part of FieryMUD  *
 *  Usage: Source file for class-specific code                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */
#include "class.hpp"

#include "casting.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "races.hpp"
#include "regen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

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
ClassDef classes[NUM_CLASSES] = {

    /* SORCERER */
    {"sorcerer",                                                        /* standard name */
     "",
     "&5&bSorcerer&0",                                                  
     "Sorcerer",                                                        /* name as found by triggers */
     "&5&bSorcerer&0    ",
     "&5&bSor&0",                                                       /* name as displayed on who */
     "&5&b**&0",                                                        /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     false,                                                             /* is this a subclass? */
     CLASS_UNDEFINED,                                                   /* base class, if a subclass */
     45,                                                                /* max level to subclass */
     3046,                                                              /* homeroom */
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {90, 85, 95, 105, 80},                                             /* saves: para, rod, petri, breath, spell */
     3,                                                                 /* static HP gain over level 30 */
     6,                                                                 /* THAC0 */
     ITEM_ANTI_SORCERER,                                                /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.2,                                                               /* exp needed to level factor */
     120,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     60,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     75,                                                                /* AC factor for mobs */
     {1029, 1154, 38, 1003, 1027, 1012, -1}},                           /* starting equipment for newbies */
     /*spellbook, quill, linen leggings, ebony hood, leather sandals, thin dagger*/

    /* CLERIC */
    {"cleric",                                                          /* standard name */
     "",
     "&6Cleric&0",                                                      
     "Cleric",                                                          /* name as found by triggers */
     "&6Cleric&0      ",
     "&6Cle&0",                                                         /* name as displayed on who */
     "&6**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     PRAY,                                                              /* type of spell prep */
     true,                                                              /* playable? */
     false,                                                             /* is this a subclass? */
     CLASS_UNDEFINED,                                                   /* base class, if a subclass */
     45,                                                                /* max level to subclass */
     3003,                                                              /* homeroom */
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {85, 110, 85, 115, 90},                                            /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     4,                                                                 /* THAC0 */
     ITEM_ANTI_CLERIC,                                                  /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     70,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     100,                                                               /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1011, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, crude mace*/

    /* THIEF */
    {"thief",                                                           /* standard name */
     "",

     "&1&bThief&0",                                                     
     "Thief",                                                           /* name as found by triggers */
     "&1&bThief&0       ",                                              
     "&1&bThi&0",                                                       /* name as displayed on who */
     "&1&b**&0",                                                        /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_ROGUE,                                                       /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3038,                                                              /* homeroom */
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {95, 90, 100, 110, 110},                                           /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     1,                                                                 /* THAC0 */
     ITEM_ANTI_THIEF,                                                   /* !class bit */
     100,                                                               /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     90,                                                                /* hp factor for mobs */
     100,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     80,                                                                /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1012, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, thin dagger*/

    /* WARRIOR */
    {"warrior",                                                         /* standard name */
     "",
     "&4&bWarrior&0",
     "Warrior",                                                         /* name as found by triggers */
     "&4&bWarrior&0     ",
     "&4&bWar&0",                                                       /* name as displayed on who */
     "&4&b**&0",                                                        /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     false,                                                             /* is this a subclass? */
     CLASS_UNDEFINED,                                                   /* base class, if a subclass */
     25,                                                                /* max level to subclass */
     3022,                                                              /* homeroom */
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {105, 115, 100, 100, 110},                                         /* saves: para, rod, petri, breath, spell */
     10,                                                                /* static HP gain over level 30 */
     -5,                                                                /* THAC0 */
     ITEM_ANTI_WARRIOR,                                                 /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.1,                                                               /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     120,                                                               /* hp factor for mobs */
     120,                                                               /* hitroll/damroll factor for mobs */
     120,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     120,                                                               /* AC factor for mobs */
     {1019, 1022, 1024, 1026, 1014, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, studded leather helmet, crude leather boots, crude longsword*/

    /* PALADIN */
    {"paladin",                                                         /* standard name */
     "",
     "Paladin&0",
     "Paladin",                                                         /* name as found by triggers */
     "Paladin&0     ",                                                  
     "Pal&0",                                                           /* name as displayed on who */
     "**&0",                                                            /* max level symbol */
     true,                                                              /* spell casting? */
     PRAY,                                                              /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_WARRIOR,                                                     /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3022,                                                              /* homeroom */
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {95, 115, 100, 105, 90},                                           /* saves: para, rod, petri, breath, spell */
     8,                                                                 /* static HP gain over level 30 */
     -4,                                                                /* THAC0 */
     ITEM_ANTI_PALADIN,                                                 /* !class bit */
     100,                                                               /* hp regen factor */
     50,                                                                /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.15,                                                              /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     120,                                                               /* hp factor for mobs */
     120,                                                               /* hitroll/damroll factor for mobs */
     120,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     120,                                                               /* AC factor for mobs */
     {1019, 1022, 1024, 1026, 1014, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, studded leather helmet, crude leather boots, crude longsword*/

    /* ANTI_PALADIN */
    {"anti-paladin",                                                    /* standard name */
     "antipaladin",
     "&1&bAnti-&9Paladin&0",
     "Anti-Paladin",                                                    /* name as found by triggers */
     "&1&bAnti-&9Paladin&0",
     "&1&bAnt&0",                                                       /* name as displayed on who */
     "&1&b**&0",                                                        /* max level symbol */
     true,                                                              /* spell casting? */
     PRAY,                                                              /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_WARRIOR,                                                     /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3022,                                                              /* homeroom */
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {95, 115, 100, 105, 90},                                           /* saves: para, rod, petri, breath, spell */
     8,                                                                 /* static HP gain over level 30 */
     -4,                                                                /* THAC0 */
     ITEM_ANTI_ANTI_PALADIN,                                            /* !class bit */
     100,                                                               /* hp regen factor */
     50,                                                                /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.15,                                                              /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     120,                                                               /* hp factor for mobs */
     120,                                                               /* hitroll/damroll factor for mobs */
     120,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     120,                                                               /* AC factor for mobs */
     {1019, 1022, 1024, 1026, 1014, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, studded leather helmet, crude leather boots, crude longsword*/

    /* RANGER */
    {"ranger",                                                          /* standard name */
     "",
     "&2&bRanger&0",                                                    
     "Ranger",                                                          /* name as found by triggers */
     "&2&d&bRanger&0      ",
     "&2&bRan&0",                                                       /* name as displayed on who */
     "&2&b**&0",                                                        /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_WARRIOR,                                                     /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3022,                                                              /* homeroom */
     {STAT_STR, STAT_DEX, STAT_CON, STAT_INT, STAT_WIS, STAT_CHA},      /* stat order preference for character creation */
     {95, 115, 100, 105, 90},                                           /* saves: para, rod, petri, breath, spell */
     8,                                                                 /* static HP gain over level 30 */
     -4,                                                                /* THAC0 */
     ITEM_ANTI_RANGER,                                                  /* !class bit */
     100,                                                               /* hp regen factor */
     50,                                                                /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.15,                                                              /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     120,                                                               /* hp factor for mobs */
     120,                                                               /* hitroll/damroll factor for mobs */
     120,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     120,                                                               /* AC factor for mobs */
     {1019, 1022, 1024, 1026, 1014, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, studded leather helmet, crude leather boots, crude longsword*/

    /* DRUID */
    {"druid",                                                           /* standard name */
     "",
     "&2Druid&0",
     "Druid",                                                           /* name as found by triggers */
     "&2Druid&0       ",                    
     "&2Dru&0",                                                         /* name as displayed on who */
     "&2**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     PRAY,                                                              /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_CLERIC,                                                      /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3003,                                                              /* homeroom */
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {85, 110, 85, 115, 90},                                            /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     5,                                                                 /* THAC0 */
     ITEM_ANTI_DRUID,                                                   /* !class bit */
     85,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     70,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     100,                                                               /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1011, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, crude mace*/

    /* SHAMAN */
    {"shaman",                                                          /* standard name */
     "",
     "&6&bShaman&0",
     "Shaman",                                                          /* name as found by triggers */
     "&6&bShaman&0      ",
     "&6&bSha&0",                                                       /* name as displayed on who */
     "&6&b**&0",                                                        /* max level symbol */
     true,                                                              /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     false,                                                             /* playable? */
     false,                                                             /* is this a subclass? */
     CLASS_UNDEFINED,                                                   /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3001,                                                              /* homeroom */
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {105, 115, 105, 110, 110},                                         /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     6,                                                                 /* THAC0 */
     ITEM_ANTI_SHAMAN,                                                  /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     100,                                                               /* hp factor for mobs */
     100,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     100,                                                               /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1011, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, crude mace*/

    /* ASSASSIN */
    {"assassin",                                                        /* standard name */
     "",
     "&1Assassin&0",
     "Assassin",                                                        /* name as found by triggers */
     "&1Assassin&0    ",
     "&1Ass&0",                                                         /* name as displayed on who */
     "&1**&0",                                                          /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_ROGUE,                                                       /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3038,                                                              /* homeroom */
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {95, 90, 100, 110, 110},                                           /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     2,                                                                 /* THAC0 */
     ITEM_ANTI_ASSASSIN,                                                /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     90,                                                                /* hp factor for mobs */
     100,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     80,                                                                /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1012, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, thin dagger*/

    /* MERCENARY */
    {"mercenary",                                                       /* standard name */
     "",
     "&3Mercenary&0",
     "Mercenary",                                                       /* name as found by triggers */
     "&3Mercenary&0   ",
     "&3Mer&0",                                                         /* name as displayed on who */
     "&3**&0",                                                          /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_ROGUE,                                                       /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3038,                                                              /* homeroom */
     {STAT_STR, STAT_DEX, STAT_CON, STAT_INT, STAT_WIS, STAT_CHA},      /* stat order preference for character creation */
     {95, 90, 100, 110, 110},                                           /* saves: para, rod, petri, breath, spell */
     9,                                                                 /* static HP gain over level 30 */
     2,                                                                 /* THAC0 */
     ITEM_ANTI_MERCENARY,                                               /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     90,                                                                /* hp factor for mobs */
     100,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     80,                                                                /* AC factor for mobs */
     {1019, 1022, 1024, 1026, 1014, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, studded leather helmet, crude leather boots, crude longsword*/

    /* NECROMANCER */
    {"necromancer",                                                     /* standard name */
     "",
     "&5Necromancer&0",
     "Necromancer",                                                     /* name as found by triggers */
     "&5Necromancer&0 ",
     "&5Nec&0",                                                         /* name as displayed on who */
     "&5**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_SORCERER,                                                    /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3046,                                                              /* homeroom */
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {90, 85, 95, 105, 80},                                             /* saves: para, rod, petri, breath, spell */
     3,                                                                 /* static HP gain over level 30 */
     7,                                                                 /* THAC0 */
     ITEM_ANTI_NECROMANCER,                                             /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.3,                                                               /* exp needed to level factor */
     120,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     60,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     75,                                                                /* AC factor for mobs */
     {1029, 1154, 38, 1003, 1027, 1012, -1}},                           /* starting equipment for newbies */
     /*spellbook, quill, linen leggings, ebony hood, leather sandals, thin dagger*/

    /* CONJURER */
    {"conjurer",                                                        /* standard name */
     "",
     "&3&bConjurer&0",
     "Conjurer",                                                        /* name as found by triggers */
     "&3&bConjurer&0    ",
     "&3&bCon&0",                                                       /* name as displayed on who */
     "&3&b**&0",                                                        /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     false,                                                             /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_SORCERER,                                                    /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3046,                                                              /* homeroom */
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {90, 85, 95, 105, 80},                                             /* saves: para, rod, petri, breath, spell */
     3,                                                                 /* static HP gain over level 30 */
     7,                                                                 /* THAC0 */
     ITEM_ANTI_CONJURER,                                                /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     120,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     60,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     75,                                                                /* AC factor for mobs */
     {1029, 1154, 38, 1003, 1027, 1012, -1}},                           /* starting equipment for newbies */
     /*spellbook, quill, linen leggings, ebony hood, leather sandals, thin dagger*/

    /* MONK */
    {"monk",                                                            /* standard name */
     "",
     "&9&bM&0&7on&9&bk&0",
     "Monk",                                                            /* name as found by triggers */
     "&9&bM&0&7on&9&bk&0        ",
     "&9&bMon&0",                                                       /* name as displayed on who */
     "&9&b**&0",                                                        /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_WARRIOR,                                                     /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3022,                                                              /* homeroom */
     {STAT_CON, STAT_STR, STAT_DEX, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {105, 115, 100, 100, 110},                                         /* saves: para, rod, petri, breath, spell */
     8,                                                                 /* static HP gain over level 30 */
     -4,                                                                /* THAC0 */
     ITEM_ANTI_MONK,                                                    /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     200,                                                               /* mv regen factor */
     1.3,                                                               /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     120,                                                               /* hp factor for mobs */
     120,                                                               /* hitroll/damroll factor for mobs */
     120,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     120,                                                               /* AC factor for mobs */
     {1019, 1022, 1024, 1026, 1014, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, studded leather helmet, crude leather boots, crude longsword*/

    /* BERSERKER */
    {"berserker",                                                       /* standard name */
     "",
     "&9&bBer&1ser&9ker&0",
     "Berserker",                                                       /* name as found by triggers */
     "&9&bBer&1ser&9ker&0   ",
     "&9&bBe&1r&0",                                                     /* name as displayed on who */
     "&9&b**&0",                                                        /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     false,                                                             /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_WARRIOR,                                                     /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3022,                                                              /* homeroom */
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {105, 115, 100, 100, 110},                                         /* saves: para, rod, petri, breath, spell */
     10,                                                                /* static HP gain over level 30 */
     -4,                                                                /* THAC0 */
     ITEM_ANTI_BERSERKER,                                               /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.1,                                                               /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     120,                                                               /* hp factor for mobs */
     120,                                                               /* hitroll/damroll factor for mobs */
     120,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     120,                                                               /* AC factor for mobs */
     {1019, 1022, 1024, 1026, 1014, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, studded leather helmet, crude leather boots, crude longsword*/

    /* PRIEST */
    {"priest",                                                          /* standard name */
     "",
     "&6&bPr&7ie&6st&0",
     "Priest",                                                          /* name as found by triggers */
     "&6&bPr&7ie&6st&0      ",
     "&6&bPr&7i&0",                                                     /* name as displayed on who */
     "&6&b**&0",                                                        /* max level symbol */
     true,                                                              /* spell casting? */
     PRAY,                                                              /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_CLERIC,                                                      /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3003,                                                              /* homeroom */
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {85, 110, 85, 115, 90},                                            /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     5,                                                                 /* THAC0 */
     ITEM_ANTI_PRIEST,                                                  /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     70,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     100,                                                               /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1011, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, crude mace*/

    /* DIABOLIST */
    {"diabolist",                                                       /* standard name */
     "",
     "&5Dia&9&bbol&0&5ist&0",
     "Diabolist",                                                       /* name as found by triggers */
     "&5Dia&9&bbol&0&5ist&0   ",
     "&5Di&9&ba&0",                                                     /* name as displayed on who */
     "&5**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     PRAY,                                                              /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_CLERIC,                                                      /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3003,                                                              /* homeroom */
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {85, 110, 85, 115, 90},                                            /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     5,                                                                 /* THAC0 */
     ITEM_ANTI_DIABOLIST,                                                  /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     70,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     100,                                                               /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1011, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, crude mace*/

    /* MYSTIC */
    {"mystic",                                                          /* standard name */
     "",
     "&7&bM&0&7ys&9&bti&7c&0",
     "Mystic",                                                          /* name as found by triggers */
     "&7&bM&0&7ys&9&bti&7c&0      ",
     "&7&bM&0&7ys&0",                                                   /* name as displayed on who */
     "&7&b**&0",                                                        /* max level symbol */
     true,                                                              /* spell casting? */
     PRAY,                                                              /* type of spell prep */
     false,                                                             /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_CLERIC,                                                      /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3003,                                                              /* homeroom */
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {85, 110, 85, 115, 90},                                            /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     7,                                                                 /* THAC0 */
     ITEM_ANTI_CLERIC,                                                  /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     70,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     100,                                                               /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1011, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, crude mace*/

    /* ROGUE */
    {"rogue",                                                           /* standard name */
     "",
     "&9&bRogue&0",
     "Rogue",                                                           /* name as found by triggers */
     "&9&bRogue&0       ",
     "&9&bRog&0",                                                       /* name as displayed on who */
     "&9&b**&0",                                                        /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     false,                                                             /* is this a subclass? */
     CLASS_UNDEFINED,                                                   /* base class, if a subclass */
     25,                                                                /* max level to subclass */
     3038,                                                              /* homeroom */
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {95, 90, 100, 110, 110},                                           /* saves: para, rod, petri, breath, spell */
     6,                                                                 /* static HP gain over level 30 */
     2,                                                                 /* THAC0 */
     ITEM_ANTI_ROGUE,                                                   /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     90,                                                                /* hp factor for mobs */
     100,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     80,                                                                /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1012, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, thin dagger*/

    /* BARD */
    {"bard",                                                            /* standard name */
     "",
     "&4B&9&bar&0&4d&0",
     "Bard",                                                            /* name as found by triggers */
     "&4B&9&bar&0&4d&0        ",
     "&4B&9&bar&0",                                                     /* name as displayed on who */
     "&4**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_ROGUE,                                                       /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3038,                                                              /* homeroom */
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {95, 90, 100, 110, 110},                                           /* saves: para, rod, petri, breath, spell */
     3,                                                                 /* static HP gain over level 30 */
     2,                                                                 /* THAC0 */
     ITEM_ANTI_BARD,                                                    /* !class bit */
     100,                                                               /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.2,                                                               /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     90,                                                                /* hp factor for mobs */
     100,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     80,                                                                /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1012, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, thin dagger*/

    /* PYROMANCER */
    {"pyromancer",                                                      /* standard name */
     "",
     "&1P&byr&0&1o&9&bma&0&7nc&9&ber&0",
     "Pyromancer",                                                      /* name as found by triggers */
     "&1P&byr&0&1o&9&bma&0&7nc&9&ber&0  ",
     "&1P&byr&0",                                                       /* name as displayed on who */
     "&1**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_SORCERER,                                                    /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3046,                                                              /* homeroom */
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {90, 85, 95, 105, 80},                                             /* saves: para, rod, petri, breath, spell */
     3,                                                                 /* static HP gain over level 30 */
     7,                                                                 /* THAC0 */
     ITEM_ANTI_PYROMANCER,                                             /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.2,                                                               /* exp needed to level factor */
     120,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     60,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     75,                                                                /* AC factor for mobs */
     {1029, 1154, 38, 1003, 1027, 1012, -1}},                           /* starting equipment for newbies */
     /*spellbook, quill, linen leggings, ebony hood, leather sandals, thin dagger*/

    /* CRYOMANCER */
    {"cryomancer",                                                      /* standard name */
     "",
     "&4C&bry&0&4o&7ma&9&bnc&0&7er&0",
     "Cryomancer",                                                      /* name as found by triggers */
     "&4C&bry&0&4o&7ma&9&bnc&0&7er&0  ",
     "&4C&bry&0",                                                       /* name as displayed on who */
     "&4**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_SORCERER,                                                    /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3046,                                                              /* homeroom */
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {90, 85, 95, 105, 80},                                             /* saves: para, rod, petri, breath, spell */
     3,                                                                 /* static HP gain over level 30 */
     7,                                                                 /* THAC0 */
     ITEM_ANTI_CRYOMANCER,                                             /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.2,                                                               /* exp needed to level factor */
     120,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     60,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     75,                                                                /* AC factor for mobs */
     {1029, 1154, 38, 1003, 1027, 1012, -1}},                           /* starting equipment for newbies */
     /*spellbook, quill, linen leggings, ebony hood, leather sandals, thin dagger*/

    /* ILLUSIONIST */
    {"illusionist",                                                     /* standard name */
     "",
     "&4I&5l&4l&5u&4s&5i&4o&5n&4i&5s&4t&0",
     "Illusionist",                                                     /* name as found by triggers */
     "&4I&5l&4l&5u&4s&5i&4o&5n&4i&5s&4t&0 ",
     "&4I&5ll&0",                                                       /* name as displayed on who */
     "&4**&0",                                                          /* max level symbol */
     true,                                                              /* spell casting? */
     MEMORIZE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_SORCERER,                                                    /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3046,                                                              /* homeroom */
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},      /* stat order preference for character creation */
     {90, 85, 95, 105, 80},                                             /* saves: para, rod, petri, breath, spell */
     3,                                                                 /* static HP gain over level 30 */
     7,                                                                 /* THAC0 */
     ITEM_ANTI_ILLUSIONIST,                                             /* !class bit */
     80,                                                                /* hp regen factor */
     100,                                                               /* mana regen factor */
     100,                                                               /* mv regen factor */
     1.2,                                                               /* exp needed to level factor */
     120,                                                               /* exp reward factor for mobs*/
     80,                                                                /* hp factor for mobs */
     80,                                                                /* hitroll/damroll factor for mobs */
     60,                                                                /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     75,                                                                /* AC factor for mobs */
     {1029, 1154, 38, 1003, 1027, 1012, -1}},                           /* starting equipment for newbies */
     /*spellbook, quill, linen leggings, ebony hood, leather sandals, thin dagger*/

    /* HUNTER */
    {"hunter",                                                          /* standard name */
     "",
     "&9&bHun&0&2te&9&br&0",
     "Hunter",                                                          /* name as found by triggers */
     "&9&bHun&0&2te&9&br&0      ",
     "&9&bHun&0",                                                       /* name as displayed on who */
     "&9&b**&0",                                                        /* max level symbol */
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     true,                                                              /* playable? */
     true,                                                              /* is this a subclass? */
     CLASS_ROGUE,                                                       /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3038,                                                              /* homeroom */
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},      /* stat order preference for character creation */
     {105, 115, 100, 100, 110},                                         /* saves: para, rod, petri, breath, spell */
     8,                                                                 /* static HP gain over level 30 */
     -4,                                                                /* THAC0 */
     0,                                                                 /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     100,                                                               /* exp reward factor for mobs*/
     90,                                                                /* hp factor for mobs */
     100,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     100,                                                               /* money factor for mob drops */
     80,                                                                /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1012, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, thin dagger*/

    /* LAYMAN */
    {"layman",                                                          /* standard name */
     "",
     "Layman",
     "Layman",                                                          /* name as found by triggers */
     "Layman      ",
     "Lay",                                                             /* name as displayed on who */
     "**",
     false,                                                             /* spell casting? */
     MEM_NONE,                                                          /* type of spell prep */
     false,                                                             /* playable? */
     false,                                                             /* is this a subclass? */
     CLASS_UNDEFINED,                                                   /* base class, if a subclass */
     -1,                                                                /* max level to subclass */
     3001,                                                              /* homeroom */
     {STAT_CHA, STAT_CON, STAT_DEX, STAT_STR, STAT_WIS, STAT_INT},      /* stat order preference for character creation */
     {100, 100, 100, 100, 100},                                         /* saves: para, rod, petri, breath, spell */
     5,                                                                 /* static HP gain over level 30 */
     0,                                                                 /* THAC0 */
     0,                                                                 /* !class bit */
     100,                                                               /* hp regen factor */
     0,                                                                 /* mana regen factor */
     100,                                                               /* mv regen factor */
     1,                                                                 /* exp needed to level factor */
     80,                                                                /* exp reward factor for mobs*/
     100,                                                               /* hp factor for mobs */
     130,                                                               /* hitroll/damroll factor for mobs */
     100,                                                               /* damage dice factor for mobs */
     75,                                                                /* money factor for mob drops */
     105,                                                               /* AC factor for mobs */
     {1019, 1022, 1025, 1027, 1011, -1}},                               /* starting equipment for newbies */
     /*crude leather pants, canvas sleeves, leather skullcap, leather sandals, crude mace*/
};

/* The newbie equipment
 *
 * 18     a small leather bag           All
 * 19     some iron rations             All
 * 20     a leather water skin          All
 * 1019   crude leather pants           warrior, rogue, cleric
 * 1022   quilted canvas sleeves        warrior, rogue, cleric
 * 23     a wooden torch                All
 * 1024   a studded leather helmet      warrior
 * 1026   studded leather boots         warrior
 * 1027   some plain leather sandals    rogue, cleric
 * 1025   a leather skullcap            rogue, cleric
 * 1011   a crude mace                  cleric
 * 1014   a crude longsword             warrior
 * 1012   a thin dagger                 rogue, mage
 * 38     black linen leggings          mage
 * 1003   an ebony hood                 mage
 * 1029   a spellbook                   mage
 * 1154   a feather quill               mage
 * 3090   a small map of mielikki       All
 */

const int common_newbie_eq[] = {18, 19, 20, 23, 3090, -1};

static flagvector class_effects_mask[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
void init_classes(void) {
#define PERM_EFF(c, f) SET_FLAG(classes[(c)].effect_flags, (f))

    PERM_EFF(CLASS_PRIEST, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_DIABOLIST, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_PALADIN, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_PALADIN, EFF_PROTECT_EVIL);
    PERM_EFF(CLASS_ANTI_PALADIN, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_ANTI_PALADIN, EFF_PROTECT_GOOD);
    PERM_EFF(CLASS_RANGER, EFF_FARSEE);

#undef PERM_EFF
}

/* parse_class
 *
 * Identifies a class from a string.  Will do partial matches.
 *
 * Code is present to prohibit a player from being set to the wrong
 * class, but it's disabled.  If it were enabled, it would only take
 * effect if "vict" were not null.
 *
 * ch is someone who's trying to change vict's class (e.g., a wizard
 * manually setting someone to a class due to a quest).
 *
 * If CLASS_UNDEFINED is returned, this function will already have provided
 * feedback to ch (if specified) as to the reason for the failure.  Otherwise,
 * it does not provide feedback.
 */
int parse_class(CharData *ch, CharData *vict, char *arg) {
    int i, class_num = CLASS_UNDEFINED, best = CLASS_UNDEFINED;

    if (!*arg) {
        if (ch)
            char_printf(ch, "What class?\n");
        return CLASS_UNDEFINED;
    }

    for (i = 0; i < NUM_CLASSES; i++) {
        if (!strncasecmp(arg, classes[i].name, strlen(arg))) {
            if (!strcasecmp(arg, classes[i].name)) {
                class_num = i;
                break;
            }
            if (best == CLASS_UNDEFINED)
                best = i;
        } else if (!strncasecmp(arg, classes[i].altname, strlen(arg))) {
            if (!strcasecmp(arg, classes[i].altname)) {
                class_num = i;
                break;
            }
            if (best == CLASS_UNDEFINED)
                best = i;
        }
    }

    if (class_num == CLASS_UNDEFINED)
        class_num = best;

    if (class_num == CLASS_UNDEFINED) {
        if (ch)
            char_printf(ch, "There is no such class.\n");
    }

    /* There are no validity checks. */
    return class_num;

    /* The following code could be used to prevent deities from assigning
     * a class classe to a player if:
     *
     *  - The class is not "active"
     *  - The player's race does not allow the class
     *
     * It's currently not used. */

    /* Bypass validity checks for immortal victims (or no specified victim). */
    if (!vict || GET_LEVEL(vict) > LVL_MAX_MORT)
        return class_num;

    /* The class has been identified, and there is a mortal victim.
     * Make sure this class is available to the victim. */

    if (!classes[class_num].active) {
        if (ch) {
            char_printf(ch, "The {} class is not available to mortals.\n", classes[class_num].name);
        }
        return CLASS_UNDEFINED;
    }

    if (!class_ok_race[(int)GET_RACE(vict)][class_num]) {
        if (ch) {
            sprintf(buf, "As %s, $n can't become %s.", with_indefinite_article(races[(int)GET_RACE(vict)].displayname),
                    with_indefinite_article(classes[class_num].name));
            act(buf, false, vict, 0, ch, TO_VICT);
        }
        return CLASS_UNDEFINED;
    }

    return class_num;
}

void give_newbie_eq(CharData *ch) {
    ObjData *obj;
    int i;

    if (!VALID_CLASS(ch)) {
        log("SYSERR: give_newbie_eq() called for char with invalid class of {:d}", GET_CLASS(ch));
        return;
    }

    if (GET_LEVEL(ch) > 4)
        return;

    for (i = 0; common_newbie_eq[i] >= 0; i++) {
        obj = read_object(common_newbie_eq[i], VIRTUAL);
        obj_to_char(obj, ch);
    }

    for (i = 0; classes[(int)GET_CLASS(ch)].newbie_eq[i] >= 0; i++) {
        obj = read_object(classes[(int)GET_CLASS(ch)].newbie_eq[i], VIRTUAL);
        obj_to_char(obj, ch);
    }
}

/* GUILD GUARDS
 *
 * When a mob with the guild guard specproc is placed in a room, it will
 * by default do no blocking at all.
 *
 * When a guild guard is instructed to allow a class to go in a particular
 * direction, that direction becomes restricted.  The guard will now block
 * all other classes from passing that way.
 *
 * More than one class may be permitted to go in a particular direction.
 * This is useful in the case of base-class guildmasters, such as the
 * warrior guildmaster beyond room 3021.  All warrior classes should be
 * permitted to go there.  They must all be mentioned specifically,
 * however.
 */
int guild_info[][3] = {

    /* Mielikki */ /* Room    Direction */
    {CLASS_SORCERER, 3045, SCMD_EAST},
    {CLASS_PYROMANCER, 3045, SCMD_EAST},
    {CLASS_CRYOMANCER, 3045, SCMD_EAST},
    {CLASS_NECROMANCER, 3045, SCMD_EAST},
    {CLASS_ILLUSIONIST, 3045, SCMD_EAST},
    {CLASS_PYROMANCER, 3045, SCMD_UP},
    {CLASS_CRYOMANCER, 3045, SCMD_DOWN},
    {CLASS_ILLUSIONIST, 3207, SCMD_SOUTH},
    {CLASS_NECROMANCER, 16950, SCMD_UP}, /* Haunted House */

    {CLASS_CLERIC, 3004, SCMD_NORTH},
    {CLASS_DIABOLIST, 3004, SCMD_NORTH},
    {CLASS_DRUID, 3004, SCMD_NORTH},
    {CLASS_PRIEST, 3004, SCMD_NORTH},
    {CLASS_DRUID, 3004, SCMD_WEST},
    {CLASS_PRIEST, 3004, SCMD_UP},

    {CLASS_ROGUE, 3037, SCMD_DOWN},
    {CLASS_ASSASSIN, 3037, SCMD_DOWN},
    {CLASS_THIEF, 3037, SCMD_DOWN},
    {CLASS_MERCENARY, 3037, SCMD_DOWN},
    {CLASS_BARD, 3037, SCMD_DOWN},
    {CLASS_BARD, 5310, SCMD_EAST}, /* Grey Castle */

    {CLASS_WARRIOR, 3021, SCMD_NORTH},
    {CLASS_ANTI_PALADIN, 3021, SCMD_NORTH},
    {CLASS_RANGER, 3021, SCMD_NORTH},
    {CLASS_MONK, 3021, SCMD_NORTH},
    {CLASS_PALADIN, 3021, SCMD_NORTH},
    {CLASS_BERSERKER, 3021, SCMD_NORTH},
    {CLASS_RANGER, 3549, SCMD_WEST},     /* Light Forest */
    {CLASS_MONK, 5307, SCMD_EAST},       /* Grey Castle */
    {CLASS_PALADIN, 5305, SCMD_EAST},    /* Grey Castle */
    {CLASS_BERSERKER, 3211, SCMD_SOUTH}, /* Great Road */

    /* Ogakh */ /* Room    Direction */
    {CLASS_SORCERER, 30072, SCMD_WEST},
    {CLASS_PYROMANCER, 30072, SCMD_WEST},
    {CLASS_CRYOMANCER, 30072, SCMD_WEST},
    {CLASS_NECROMANCER, 30072, SCMD_WEST},
    {CLASS_ILLUSIONIST, 30072, SCMD_WEST},
    {CLASS_PYROMANCER, 30102, SCMD_NORTH},
    {CLASS_CRYOMANCER, 30102, SCMD_WEST},
    {CLASS_NECROMANCER, 30112, SCMD_UP},
    {CLASS_ILLUSIONIST, 30120, SCMD_UP},

    {CLASS_CLERIC, 30069, SCMD_WEST},
    {CLASS_DIABOLIST, 30069, SCMD_WEST},
    {CLASS_DRUID, 30069, SCMD_WEST},
    {CLASS_PRIEST, 30069, SCMD_WEST},
    {CLASS_DIABOLIST, 30107, SCMD_SOUTH},

    {CLASS_ROGUE, 30018, SCMD_EAST},
    {CLASS_ASSASSIN, 30018, SCMD_EAST},
    {CLASS_THIEF, 30018, SCMD_EAST},
    {CLASS_MERCENARY, 30018, SCMD_EAST},
    {CLASS_BARD, 30018, SCMD_EAST},

    {CLASS_WARRIOR, 30029, SCMD_WEST},
    {CLASS_ANTI_PALADIN, 30029, SCMD_WEST},
    {CLASS_RANGER, 30029, SCMD_WEST},
    {CLASS_MONK, 30029, SCMD_WEST},
    {CLASS_PALADIN, 30029, SCMD_WEST},
    {CLASS_BERSERKER, 30029, SCMD_WEST},
    {CLASS_ANTI_PALADIN, 30107, SCMD_NORTH},
    {CLASS_BERSERKER, 30121, SCMD_WEST},

    /* Anduin */
    {CLASS_ANTI_PALADIN, 6079, SCMD_EAST},

    {CLASS_WARRIOR, 6148, SCMD_SOUTH},
    {CLASS_MONK, 6148, SCMD_SOUTH},
    {CLASS_PALADIN, 6148, SCMD_SOUTH},
    {CLASS_RANGER, 6148, SCMD_SOUTH},
    {CLASS_BERSERKER, 6148, SCMD_SOUTH},
    {CLASS_BERSERKER, 55796, SCMD_NORTH}, /* Black Rock Trail */

    {CLASS_ROGUE, 6067, SCMD_NORTH},
    {CLASS_THIEF, 6067, SCMD_NORTH},
    {CLASS_MERCENARY, 6106, SCMD_UP},
    {CLASS_ASSASSIN, 6083, SCMD_NORTH},
    {CLASS_BARD, 6092, SCMD_NORTH},

    {CLASS_CLERIC, 6217, SCMD_EAST},
    {CLASS_PRIEST, 6217, SCMD_EAST},
    {CLASS_DIABOLIST, 6075, SCMD_SOUTH},
    {CLASS_DRUID, 6217, SCMD_UP},

    {CLASS_SORCERER, 6219, SCMD_UP},
    {CLASS_NECROMANCER, 6219, SCMD_DOWN},
    {CLASS_PYROMANCER, 6219, SCMD_NORTH},
    {CLASS_CRYOMANCER, 6219, SCMD_SOUTH},
    {CLASS_ILLUSIONIST, 6233, SCMD_SOUTH},

    /* Ickle */
    {CLASS_ROGUE, 10047, SCMD_EAST},
    {CLASS_MERCENARY, 10047, SCMD_EAST},
    {CLASS_ASSASSIN, 10047, SCMD_EAST},
    {CLASS_THIEF, 10047, SCMD_EAST},
    {CLASS_BARD, 10047, SCMD_EAST},

    {CLASS_WARRIOR, 10014, SCMD_WEST},
    {CLASS_ANTI_PALADIN, 10014, SCMD_WEST},
    {CLASS_PALADIN, 10014, SCMD_WEST},
    {CLASS_MONK, 10014, SCMD_WEST},
    {CLASS_RANGER, 10014, SCMD_WEST},
    {CLASS_BERSERKER, 10014, SCMD_WEST},
    {CLASS_BERSERKER, 10241, SCMD_EAST}, /* Mt. Frostbite */

    {CLASS_CLERIC, 10004, SCMD_WEST},
    {CLASS_PRIEST, 10004, SCMD_WEST},
    {CLASS_DIABOLIST, 10004, SCMD_WEST},
    {CLASS_DRUID, 10004, SCMD_WEST},

    {CLASS_SORCERER, 10036, SCMD_NORTH},
    {CLASS_NECROMANCER, 10036, SCMD_NORTH},
    {CLASS_PYROMANCER, 10036, SCMD_NORTH},
    {CLASS_CRYOMANCER, 10036, SCMD_NORTH},
    {CLASS_ILLUSIONIST, 10036, SCMD_NORTH},

    /* Timun's Citadel of testing guards */
    {-999, 17200, SCMD_WEST},

    /* Example to block everyone:
      {-999<--all  ,     5065,   SCMD_WEST},
    */

    /* this must go last -- add new guards above! */
    {-1, -1, -1}};

/* This function is used to limit the skill a players class can get.
 * The defult is full skill 1000, so dont use it if it is that.
 * It actually limits skill accross the levels, ie:

 MAX SKILL = 600/50 * lvl.  eg if 1000 was max 1000/50 = 20 *lvl

 */

int level_max_skill(CharData *ch, int level, int skill) {
    int max_skill = 1000;

    /* The proficiency of individual spells comes from their related skills,
     * not the skill level of the spell itself.  Spells are set to a so-called
     * "proficiency" of 1000 when a quest spell is awarded. */
    if (IS_SPELL(skill))
        return 1000;

    switch (GET_RACE(ch)) {
    case RACE_OGRE:
        switch (skill) {
        case SKILL_PARRY:
        case SKILL_RIPOSTE:
            max_skill = 750;
            break;
        }
    case RACE_TROLL:
        switch (skill) {
        case SKILL_DODGE:
        case SKILL_PARRY:
            max_skill = 700;
            break;
        }
    }
    switch (GET_CLASS(ch)) {
    case CLASS_RANGER:
        switch (skill) {
        case SKILL_HUNT:
            max_skill = 750;
            break;
        }
        break;
    case CLASS_PRIEST:
        switch (skill) {
        case SKILL_DOUBLE_ATTACK:
            max_skill = 500;
            break;
        }
        break;
    case CLASS_HUNTER:
        switch (skill) {}
        break;
    case CLASS_WARRIOR:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 600;
            break;
        }
        break;
    case CLASS_MERCENARY:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 850;
            break;
        }
        break;
    case CLASS_ASSASSIN:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 750;
            break;
        }
        break;
    case CLASS_THIEF:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 600;
            break;
        }
        break;
    case CLASS_ROGUE:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 650;
            break;
        }
        break;
    case CLASS_ILLUSIONIST:
        switch (skill) {
        case SKILL_HIDE:
            max_skill = 500;
            break;
        }
        break;
    case CLASS_BARD:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 500;
            break;
        case SKILL_SPHERE_HEALING:
            max_skill = 750;
            break;
        case SKILL_SPHERE_FIRE:
            max_skill = 500;
            break;
        case SKILL_SPHERE_WATER:
            max_skill = 500;
            break;
        case SKILL_SPHERE_AIR:
            max_skill = 500;
            break;
        }
        break;
    }
    return (std::min((10 * level) + 50, max_skill));
}

int return_max_skill(CharData *ch, int skill) { return level_max_skill(ch, GET_LEVEL(ch), skill); }

void init_char_class(CharData *ch) { /* Nothing much to do here. */
}

void update_char_class(CharData *ch) {
    if (!VALID_CLASS(ch)) {
        char buf[500];
        log("update_char_class: {} doesn't have a valid class ({:d}).", GET_NAME(ch), GET_CLASS(ch));
        return;
    }

    /* Any bits that might get set below should be cleared here first. */
    REMOVE_FLAGS(EFF_FLAGS(ch), class_effects_mask, NUM_EFF_FLAGS);

    /* Reset effect flags for this race */
    SET_FLAGS(EFF_FLAGS(ch), classes[(int)GET_CLASS(ch)].effect_flags, NUM_EFF_FLAGS);
}

/*
 * This function controls the change to maxmove, maxmana, and maxhp whenever
 * someone gains or loses a level. */

/*modified by proky so if lose level uses GET_LASTLEVEL(ch)*/

void advance_level(CharData *ch, enum level_action action) {
    int add_hp = 0, c = GET_CLASS(ch);

    /*
     * First determine how much hp the player should gain based on their
     * class under normal circumstances.
     */
    switch (c) {
    case CLASS_SORCERER:
    case CLASS_CRYOMANCER:
    case CLASS_ILLUSIONIST:
    case CLASS_PYROMANCER:
    case CLASS_NECROMANCER:
    case CLASS_CONJURER:
    case CLASS_BARD:
        if (action == LEVEL_GAIN)
            add_hp += random_number(1, 6);
        else
            add_hp -= 3;
        break;

    case CLASS_CLERIC:
    case CLASS_SHAMAN:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_MYSTIC:
        if (action == LEVEL_GAIN)
            add_hp += random_number(3, 8);
        else
            add_hp -= 5;
        break;

    case CLASS_DRUID:
        if (action == LEVEL_GAIN)
            add_hp += random_number(3, 9);
        else
            add_hp -= 6;
        break;

    case CLASS_ASSASSIN:
        if (action == LEVEL_GAIN)
            add_hp += random_number(5, 10);
        else
            add_hp -= 7;
        break;

    case CLASS_THIEF:
    case CLASS_ROGUE:
        if (action == LEVEL_GAIN)
            add_hp += random_number(5, 11);
        else
            add_hp -= 8;
        break;

    case CLASS_MERCENARY:
        if (action == LEVEL_GAIN)
            add_hp += random_number(5, 12);
        else
            add_hp -= 9;
        break;

    case CLASS_RANGER:
        if (action == LEVEL_GAIN)
            add_hp += random_number(7, 11);
        else
            add_hp -= 8;
        break;

    case CLASS_PALADIN:
    case CLASS_ANTI_PALADIN:
        if (action == LEVEL_GAIN)
            add_hp += random_number(7, 12);
        else
            add_hp -= 9;
        break;

    case CLASS_WARRIOR:
    case CLASS_MONK:
    case CLASS_BERSERKER:
    case CLASS_HUNTER:
        if (action == LEVEL_GAIN)
            add_hp += random_number(8, 13);
        else
            add_hp -= 9;
        break;

    default:
        if (action == LEVEL_GAIN)
            add_hp += random_number(3, 8);
        else
            add_hp -= 6;
        log("SYSERR: Unrecognized class {:d} in advance_char", c);
    }

    /*
     * If this is a magical class and the char is losing a level,
     * run init_mem_list to clear out spells they shouldn't know.
     */
    if (classes[c].magical && action == LEVEL_LOSE)
        init_mem_list(ch);

    /*
     * If losing a level, and there is a value in GET_LASTLEVEL,
     * use that instead of whatever we calculated above.
     */
    if (action == LEVEL_LOSE && GET_LASTLEVEL(ch) != 0)
        add_hp = 0 - (GET_LASTLEVEL(ch) + random_number(0, 1));

    /* If over level 30, use constants for hitpoints */
    if (GET_LEVEL(ch) > 30) {
        if (action == LEVEL_GAIN)
            add_hp = classes[c].hp_lev;
        else
            add_hp = 0 - classes[c].hp_lev;
    }

    /* Adjust by Con mod */
    if (action == LEVEL_GAIN)
        add_hp += con_app[GET_CON(ch)].hitp;
    else
        add_hp -= con_app[GET_CON(ch)].hitp;

    /* Add to natural hitpoints and then recalculate other hitpoints */
    ch->points.max_hit += add_hp;
    GET_BASE_HIT(ch) += add_hp;
    effect_total(ch);


    /* Calculate mana */
    if (GET_LEVEL(ch) > 10)
        ch->points.max_mana = (GET_LEVEL(ch) * GET_LEVEL(ch)) / 10;
    else
        ch->points.max_mana = 0;


    /* Add MV to maxmove until level 50 */
    if (GET_LEVEL(ch) <= 50) {
        int add_mv = 2;
        if (action == LEVEL_GAIN) {
            if (GET_DEX(ch) >= 54 && GET_DEX(ch) <= 76)
                add_mv += 1;
            else if (GET_DEX(ch) > 76)
                add_mv += 2;
        } else {
            add_mv = -2;
            if (GET_DEX(ch) >= 54 && GET_DEX(ch) <= 76)
                add_mv -= 1;
            else if (GET_DEX(ch) > 76)
                add_mv -= 2;
        }

        GET_MAX_MOVE(ch) += add_mv;
        effect_total(ch);
    }

    /* Give immortals special treatment */
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        GET_COND(ch, FULL) = (char)-1;
        GET_COND(ch, THIRST) = (char)-1;
        GET_COND(ch, DRUNK) = (char)-1;
        SET_FLAG(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }

    /* Modify clan power */
    if (GET_CLAN(ch) && IS_CLAN_MEMBER(ch)) {
        if (action == LEVEL_GAIN)
            ++GET_CLAN(ch)->power;
        else
            --GET_CLAN(ch)->power;
    }

    check_regen_rates(ch); /* start regening new points */
    update_char(ch);       /* update skills/spells/innates/etc. for new level */
    save_player_char(ch);
}

/*
 * ASSIGNMENT OF SPELLS AND SKILLS.  Here we define which spells, skills,
 * chants, and songs are assigned to which classes, and the minimum level
 * a character must be to use them.
 */
#define spell_assign(spellnum, class_num, level) skill_assign(spellnum, class_num, level)
#define chant_assign(chantnum, class_num, level) skill_assign(chantnum, class_num, level)
#define song_assign(songnum, class_num, level) skill_assign(songnum, class_num, level)

void assign_class_skills(void) {
    int i;

    /* Set up some skills common to multiple classes */
    for (i = 0; i < NUM_CLASSES; ++i) {

        /* Skills that all classes get. */
        skill_assign(SKILL_MOUNT, i, 1);
        skill_assign(SKILL_RIDING, i, 1);
        skill_assign(SKILL_BANDAGE, i, 1);
        skill_assign(SKILL_DOUSE, i, 1);
        skill_assign(SKILL_FIRST_AID, i, 1);

        /* Magic only */
        if (classes[i].magical) {
            skill_assign(SKILL_MEDITATE, i, 1);
            skill_assign(SKILL_KNOW_SPELL, i, 1);
            skill_assign(SKILL_QUICK_CHANT, i, 1);
        }

        /* Scribe users */
        if (classes[i].mem_mode == MEMORIZE)
            skill_assign(SKILL_SCRIBE, i, 1);

        /* Arcane magic types */
        if (i == CLASS_SORCERER || classes[i].subclass_of == CLASS_SORCERER) {
            skill_assign(SKILL_PIERCING, i, 1);
            skill_assign(SKILL_DODGE, i, 20);
        }
    }

    /* From here on down, the classes are listed in alphabetical order.
     * Within each class, skills should be sorted by level and spells
     * tend to be grouped by circle. */

    /* ANTI_PALADIN */
    skill_assign(SKILL_BLUDGEONING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_PIERCING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_SLASHING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_KICK, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_BASH, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_DODGE, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_TAME, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_GUARD, CLASS_ANTI_PALADIN, 10);
    skill_assign(SKILL_SWITCH, CLASS_ANTI_PALADIN, 10);
    skill_assign(SKILL_RESCUE, CLASS_ANTI_PALADIN, 10);
    skill_assign(SKILL_SUMMON_MOUNT, CLASS_ANTI_PALADIN, 15);
    skill_assign(SKILL_DUAL_WIELD, CLASS_ANTI_PALADIN, 20);
    skill_assign(SKILL_PARRY, CLASS_ANTI_PALADIN, 20);
    skill_assign(SKILL_RIPOSTE, CLASS_ANTI_PALADIN, 40);
    skill_assign(SKILL_VAMP_TOUCH, CLASS_ANTI_PALADIN, 45);
    skill_assign(SKILL_RETREAT, CLASS_ANTI_PALADIN, 60);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_ANTI_PALADIN, 70);
    skill_assign(SKILL_HITALL, CLASS_ANTI_PALADIN, 80);
    skill_assign(SKILL_DISARM, CLASS_ANTI_PALADIN, 50);

    spell_assign(SPELL_CAUSE_LIGHT, CLASS_ANTI_PALADIN, CIRCLE_1);

    spell_assign(SPELL_DARK_PRESENCE, CLASS_ANTI_PALADIN, CIRCLE_2);
    spell_assign(SPELL_DEMONSKIN, CLASS_ANTI_PALADIN, CIRCLE_2);

    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_ANTI_PALADIN, CIRCLE_3);
    spell_assign(SPELL_CREATE_FOOD, CLASS_ANTI_PALADIN, CIRCLE_3);
    spell_assign(SPELL_CREATE_WATER, CLASS_ANTI_PALADIN, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_GOOD, CLASS_ANTI_PALADIN, CIRCLE_3);

    spell_assign(SPELL_CURSE, CLASS_ANTI_PALADIN, CIRCLE_4);

    spell_assign(SPELL_CAUSE_CRITIC, CLASS_ANTI_PALADIN, CIRCLE_5);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_ANTI_PALADIN, CIRCLE_5);
    spell_assign(SPELL_POISON, CLASS_ANTI_PALADIN, CIRCLE_5);

    spell_assign(SPELL_BLINDNESS, CLASS_ANTI_PALADIN, CIRCLE_6);
    spell_assign(SPELL_CURE_BLIND, CLASS_ANTI_PALADIN, CIRCLE_6);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_ANTI_PALADIN, CIRCLE_7);

    spell_assign(SPELL_SOULSHIELD, CLASS_ANTI_PALADIN, CIRCLE_8);
    spell_assign(SPELL_SUMMON_CORPSE, CLASS_ANTI_PALADIN, CIRCLE_8);

    spell_assign(SPELL_HARM, CLASS_ANTI_PALADIN, CIRCLE_9);

    spell_assign(SPELL_UNHOLY_WORD, CLASS_ANTI_PALADIN, CIRCLE_11);

    /* ASSASSIN */
    skill_assign(SKILL_BLUDGEONING, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_PIERCING, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_SLASHING, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_INSTANT_KILL, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_SNEAK, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_BACKSTAB, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_HIDE, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_DODGE, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_PICK_LOCK, CLASS_ASSASSIN, 5);
    skill_assign(SKILL_KICK, CLASS_ASSASSIN, 10);
    skill_assign(SKILL_TRACK, CLASS_ASSASSIN, 10);
    skill_assign(SKILL_DUAL_WIELD, CLASS_ASSASSIN, 15);
    skill_assign(SKILL_THROATCUT, CLASS_ASSASSIN, 30);
    skill_assign(SKILL_PARRY, CLASS_ASSASSIN, 40);
    skill_assign(SKILL_SHADOW, CLASS_ASSASSIN, 40);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_ASSASSIN, 70);

    /* BARD */
    skill_assign(SKILL_BLUDGEONING, CLASS_BARD, 1);
    skill_assign(SKILL_PIERCING, CLASS_BARD, 1);
    skill_assign(SKILL_SLASHING, CLASS_BARD, 1);
    skill_assign(SKILL_BACKSTAB, CLASS_BARD, 1);
    skill_assign(SKILL_PERFORM, CLASS_BARD, 1);
    skill_assign(SKILL_MEDITATE, CLASS_BARD, 1);
    skill_assign(SKILL_SCRIBE, CLASS_BARD, 1);
    skill_assign(SKILL_HIDE, CLASS_BARD, 10);
    skill_assign(SKILL_PICK_LOCK, CLASS_BARD, 10);
    skill_assign(SKILL_SNEAK, CLASS_BARD, 10);
    skill_assign(SKILL_STEAL, CLASS_BARD, 10);
    skill_assign(SKILL_DODGE, CLASS_BARD, 20);
    skill_assign(SKILL_PARRY, CLASS_BARD, 40);
    skill_assign(SKILL_TRACK, CLASS_BARD, 50);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_BARD, 50);
    skill_assign(SKILL_DUAL_WIELD, CLASS_BARD, 70);

    spell_assign(SPELL_BURNING_HANDS, CLASS_BARD, CIRCLE_1);
    spell_assign(SPELL_CAUSE_LIGHT, CLASS_BARD, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_BARD, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_BARD, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_BARD, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_BARD, CIRCLE_1);
    spell_assign(SPELL_VENTRILOQUATE, CLASS_BARD, CIRCLE_1);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_BARD, CIRCLE_1);

    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_BARD, CIRCLE_2);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_BARD, CIRCLE_2);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_BARD, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_BARD, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_BARD, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_BARD, CIRCLE_2);
    spell_assign(SPELL_ENHANCE_ABILITY, CLASS_BARD, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_BARD, CIRCLE_2);

    spell_assign(SPELL_CAUSE_CRITIC, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_FEAR, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_IDENTIFY, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_MESMERIZE, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_BARD, CIRCLE_3);
    spell_assign(SPELL_WEB, CLASS_BARD, CIRCLE_3);

    spell_assign(SPELL_CONFUSION, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_FEATHER_FALL, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_MINOR_PARALYSIS, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_POISON, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_REMOVE_PARALYSIS, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_REVEAL_HIDDEN, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_TELEPORT, CLASS_BARD, CIRCLE_4);
    spell_assign(SPELL_WORLD_TELEPORT, CLASS_BARD, CIRCLE_4);

    spell_assign(SPELL_COLOR_SPRAY, CLASS_BARD, CIRCLE_5);
    spell_assign(SPELL_CURSE, CLASS_BARD, CIRCLE_5);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_BARD, CIRCLE_5);
    spell_assign(SPELL_INVISIBLE, CLASS_BARD, CIRCLE_5);
    spell_assign(SPELL_SANE_MIND, CLASS_BARD, CIRCLE_5);

    spell_assign(SPELL_HARM, CLASS_BARD, CIRCLE_6);
    spell_assign(SPELL_HASTE, CLASS_BARD, CIRCLE_6);
    spell_assign(SPELL_HEAL, CLASS_BARD, CIRCLE_6);
    spell_assign(SPELL_SILENCE, CLASS_BARD, CIRCLE_6);
    spell_assign(SPELL_VICIOUS_MOCKERY, CLASS_BARD, CIRCLE_6);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_BARD, CIRCLE_7);
    spell_assign(SPELL_INSANITY, CLASS_BARD, CIRCLE_7);

    spell_assign(SPELL_CLOUD_OF_DAGGERS, CLASS_BARD, CIRCLE_8);
    spell_assign(SPELL_ILLUSORY_WALL, CLASS_BARD, CIRCLE_8);

    spell_assign(SPELL_ENCHANT_WEAPON, CLASS_BARD, CIRCLE_9);
    spell_assign(SPELL_FAMILIARITY, CLASS_BARD, CIRCLE_9);
    spell_assign(SPELL_INVIGORATE, CLASS_BARD, CIRCLE_9);

    spell_assign(SPELL_MAJOR_PARALYSIS, CLASS_BARD, CIRCLE_10);
    spell_assign(SPELL_MASS_INVIS, CLASS_BARD, CIRCLE_10);

    spell_assign(SPELL_ENLARGE, CLASS_BARD, CIRCLE_11);
    spell_assign(SPELL_REDUCE, CLASS_BARD, CIRCLE_11);
    spell_assign(SPELL_GLORY, CLASS_BARD, CIRCLE_11);

    spell_assign(SPELL_CHARM, CLASS_BARD, CIRCLE_12);

    song_assign(SONG_INSPIRATION, CLASS_BARD, 1);
    song_assign(SONG_TERROR, CLASS_BARD, 10);
    song_assign(SONG_SONG_OF_REST, CLASS_BARD, 20);
    song_assign(SONG_JOYFUL_NOISE, CLASS_BARD, 40);
    song_assign(SONG_FREEDOM_SONG, CLASS_BARD, 50);
    song_assign(SONG_HEROIC_JOURNEY, CLASS_BARD, 60);
    song_assign(SONG_BALLAD_OF_TEARS, CLASS_BARD, 70);
    song_assign(SONG_HEARTHSONG, CLASS_BARD, 70);
    song_assign(SONG_CROWN_OF_MADNESS, CLASS_BARD, 80);
    song_assign(SONG_ENRAPTURE, CLASS_BARD, 90);

    /* BERSERKER */
    skill_assign(SKILL_BLUDGEONING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_PIERCING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_SLASHING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_SWITCH, CLASS_BERSERKER, 1);
    skill_assign(SKILL_KICK, CLASS_BERSERKER, 1);
    skill_assign(SKILL_DODGE, CLASS_BERSERKER, 1);
    skill_assign(SKILL_BERSERK, CLASS_BERSERKER, 10);
    skill_assign(SKILL_PARRY, CLASS_BERSERKER, 15);
    skill_assign(SKILL_DUAL_WIELD, CLASS_BERSERKER, 20);
    skill_assign(SKILL_CHANT, CLASS_BERSERKER, 25);
    skill_assign(SKILL_MAUL, CLASS_BERSERKER, 30);
    skill_assign(SKILL_TANTRUM, CLASS_BERSERKER, 45);
    skill_assign(SKILL_MEDITATE, CLASS_BERSERKER, 50);
    skill_assign(SKILL_RIPOSTE, CLASS_BERSERKER, 50);
    skill_assign(SKILL_BATTLE_HOWL, CLASS_BERSERKER, 65);
    skill_assign(SKILL_GROUND_SHAKER, CLASS_BERSERKER, 75);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_BERSERKER, 85);

    chant_assign(CHANT_SPIRIT_WOLF, CLASS_BERSERKER, 25);
    chant_assign(CHANT_SPIRIT_BEAR, CLASS_BERSERKER, 60);
    chant_assign(CHANT_INTERMINABLE_WRATH, CLASS_BERSERKER, 90);

    /* CLERIC */
    skill_assign(SKILL_BLUDGEONING, CLASS_CLERIC, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_CLERIC, 1);
    skill_assign(SKILL_DODGE, CLASS_CLERIC, 20);

    spell_assign(SPELL_ARMOR, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CAUSE_LIGHT, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_CLERIC, CIRCLE_1);

    spell_assign(SPELL_BLESS, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_CLERIC, CIRCLE_2);

    spell_assign(SPELL_CAUSE_CRITIC, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_CURE_BLIND, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_ENDURANCE, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_CLERIC, CIRCLE_3);

    spell_assign(SPELL_BLINDNESS, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_FLAMESTRIKE, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_SENSE_LIFE, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_CLERIC, CIRCLE_4);

    spell_assign(SPELL_DESTROY_UNDEAD, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_EARTHQUAKE, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_HARM, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_HEAL, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_REMOVE_PARALYSIS, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_SOULSHIELD, CLASS_CLERIC, CIRCLE_5);

    spell_assign(SPELL_DARKNESS, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_HOLY_WORD, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_ILLUMINATION, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_SILENCE, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_UNHOLY_WORD, CLASS_CLERIC, CIRCLE_6);

    spell_assign(SPELL_FULL_HARM, CLASS_CLERIC, CIRCLE_7);
    spell_assign(SPELL_FULL_HEAL, CLASS_CLERIC, CIRCLE_7);
    spell_assign(SPELL_VITALITY, CLASS_CLERIC, CIRCLE_7);
    spell_assign(SPELL_WATERWALK, CLASS_CLERIC, CIRCLE_7);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_CLERIC, CIRCLE_8);
    spell_assign(SPELL_GROUP_HEAL, CLASS_CLERIC, CIRCLE_8);

    spell_assign(SPELL_GREATER_VITALITY, CLASS_CLERIC, CIRCLE_9);

    spell_assign(SPELL_GROUP_ARMOR, CLASS_CLERIC, CIRCLE_10);

    spell_assign(SPELL_RESURRECT, CLASS_CLERIC, CIRCLE_11);

    spell_assign(SPELL_DRAGONS_HEALTH, CLASS_CLERIC, CIRCLE_12);

    /* CONJURER */
    spell_assign(SPELL_DETECT_MAGIC, CLASS_CONJURER, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_CONJURER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_CONJURER, CIRCLE_1);

    spell_assign(SPELL_BURNING_HANDS, CLASS_CONJURER, CIRCLE_2);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_CONJURER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_CONJURER, CIRCLE_2);
    spell_assign(SPELL_ENHANCE_ABILITY, CLASS_CONJURER, CIRCLE_2);

    spell_assign(SPELL_CONCEALMENT, CLASS_CONJURER, CIRCLE_3);
    spell_assign(SPELL_IDENTIFY, CLASS_CONJURER, CIRCLE_3);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_CONJURER, CIRCLE_3);
    spell_assign(SPELL_SUMMON_ELEMENTAL, CLASS_CONJURER, CIRCLE_3);

    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_CONJURER, CIRCLE_4);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_CONJURER, CIRCLE_4);

    spell_assign(SPELL_COLOR_SPRAY, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_INFRAVISION, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_CONE_OF_COLD, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_WALL_OF_FOG, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_WALL_OF_STONE, CLASS_CONJURER, CIRCLE_5);

    spell_assign(SPELL_DETECT_POISON, CLASS_CONJURER, CIRCLE_6);
    spell_assign(SPELL_FIREBALL, CLASS_CONJURER, CIRCLE_6);
    spell_assign(SPELL_FLY, CLASS_CONJURER, CIRCLE_6);
    spell_assign(SPELL_STONE_SKIN, CLASS_CONJURER, CIRCLE_6);

    spell_assign(SPELL_FARSEE, CLASS_CONJURER, CIRCLE_7);
    spell_assign(SPELL_HASTE, CLASS_CONJURER, CIRCLE_7);
    spell_assign(SPELL_INVISIBLE, CLASS_CONJURER, CIRCLE_7);

    spell_assign(SPELL_DIMENSION_DOOR, CLASS_CONJURER, CIRCLE_8);

    spell_assign(SPELL_ENCHANT_WEAPON, CLASS_CONJURER, CIRCLE_9);

    spell_assign(SPELL_SUMMON_DEMON, CLASS_CONJURER, CIRCLE_10);

    spell_assign(SPELL_SUMMON_GREATER_DEMON, CLASS_CONJURER, CIRCLE_11);

    /* CRYOMANCER */
    spell_assign(SPELL_DETECT_MAGIC, CLASS_CRYOMANCER, CIRCLE_1);
    spell_assign(SPELL_ICE_DARTS, CLASS_CRYOMANCER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_CRYOMANCER, CIRCLE_1);
    spell_assign(SPELL_WATERWALK, CLASS_CRYOMANCER, CIRCLE_1);

    spell_assign(SPELL_CHILL_TOUCH, CLASS_CRYOMANCER, CIRCLE_2);
    spell_assign(SPELL_CONCEALMENT, CLASS_CRYOMANCER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_CRYOMANCER, CIRCLE_2);
    spell_assign(SPELL_ENHANCE_ABILITY, CLASS_CRYOMANCER, CIRCLE_2);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_EXTINGUISH, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_ICE_ARMOR, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_CRYOMANCER, CIRCLE_3);

    spell_assign(SPELL_COLDSHIELD, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_ICE_DAGGER, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_FEATHER_FALL, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_RAY_OF_ENFEEB, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_TELEPORT, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_WORLD_TELEPORT, CLASS_CRYOMANCER, CIRCLE_4);

    spell_assign(SPELL_CONE_OF_COLD, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_FARSEE, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_FREEZING_WIND, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_INVISIBLE, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_MINOR_PARALYSIS, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_CRYOMANCER, CIRCLE_5);

    spell_assign(SPELL_HASTE, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_ICE_STORM, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_MINOR_GLOBE, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_RAIN, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_STONE_SKIN, CLASS_CRYOMANCER, CIRCLE_6);

    spell_assign(SPELL_FREEZE, CLASS_CRYOMANCER, CIRCLE_7);

    spell_assign(SPELL_CHAIN_LIGHTNING, CLASS_CRYOMANCER, CIRCLE_8);
    spell_assign(SPELL_FLY, CLASS_CRYOMANCER, CIRCLE_8);
    spell_assign(SPELL_MAJOR_GLOBE, CLASS_CRYOMANCER, CIRCLE_8);
    spell_assign(SPELL_WALL_OF_ICE, CLASS_CRYOMANCER, CIRCLE_8);

    spell_assign(SPELL_ICEBALL, CLASS_CRYOMANCER, CIRCLE_9);
    spell_assign(SPELL_MASS_INVIS, CLASS_CRYOMANCER, CIRCLE_9);
    spell_assign(SPELL_RELOCATE, CLASS_CRYOMANCER, CIRCLE_9);

    spell_assign(SPELL_NEGATE_COLD, CLASS_CRYOMANCER, CIRCLE_10);
    spell_assign(SPELL_WATERFORM, CLASS_CRYOMANCER, CIRCLE_10);

    spell_assign(SPELL_FLOOD, CLASS_CRYOMANCER, CIRCLE_11);

    spell_assign(SPELL_ICE_SHARDS, CLASS_CRYOMANCER, CIRCLE_12);

    /* DIABOLIST */
    skill_assign(SKILL_BLUDGEONING, CLASS_DIABOLIST, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_DIABOLIST, 1);
    skill_assign(SKILL_DODGE, CLASS_DIABOLIST, 20);

    spell_assign(SPELL_CAUSE_LIGHT, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_DEMONSKIN, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_DIABOLIST, CIRCLE_1);

    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DARK_FEAST, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DARK_PRESENCE, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_DIABOLIST, CIRCLE_2);

    spell_assign(SPELL_CAUSE_CRITIC, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_CURE_BLIND, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_ENDURANCE, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_HELL_BOLT, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_GOOD, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_DIABOLIST, CIRCLE_3);

    spell_assign(SPELL_BLINDNESS, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_DEMONIC_ASPECT, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_DISEASE, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_ELEMENTAL_WARDING, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_SENSE_LIFE, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_DIABOLIST, CIRCLE_4);

    spell_assign(SPELL_EARTHQUAKE, CLASS_DIABOLIST, CIRCLE_5);
    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_DIABOLIST, CIRCLE_5);
    spell_assign(SPELL_HEAL, CLASS_DIABOLIST, CIRCLE_5);
    spell_assign(SPELL_REMOVE_PARALYSIS, CLASS_DIABOLIST, CIRCLE_5);
    spell_assign(SPELL_SOULSHIELD, CLASS_DIABOLIST, CIRCLE_5);

    spell_assign(SPELL_DARKNESS, CLASS_DIABOLIST, CIRCLE_6);
    spell_assign(SPELL_SANE_MIND, CLASS_DIABOLIST, CIRCLE_6);
    spell_assign(SPELL_SILENCE, CLASS_DIABOLIST, CIRCLE_6);
    spell_assign(SPELL_STYGIAN_ERUPTION, CLASS_DIABOLIST, CIRCLE_6);

    spell_assign(SPELL_DEMONIC_MUTATION, CLASS_DIABOLIST, CIRCLE_7);
    spell_assign(SPELL_FULL_HEAL, CLASS_DIABOLIST, CIRCLE_7);
    spell_assign(SPELL_INSANITY, CLASS_DIABOLIST, CIRCLE_7);
    spell_assign(SPELL_WATERWALK, CLASS_DIABOLIST, CIRCLE_7);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_DIABOLIST, CIRCLE_8);
    spell_assign(SPELL_GROUP_HEAL, CLASS_DIABOLIST, CIRCLE_8);
    spell_assign(SPELL_HELLFIRE_BRIMSTONE, CLASS_DIABOLIST, CIRCLE_8);
    spell_assign(SPELL_SPEAK_IN_TONGUES, CLASS_DIABOLIST, CIRCLE_8);

    spell_assign(SPELL_BANISH, CLASS_DIABOLIST, CIRCLE_9);
    spell_assign(SPELL_UNHOLY_WORD, CLASS_DIABOLIST, CIRCLE_9);
    spell_assign(SPELL_WINGS_OF_HELL, CLASS_DIABOLIST, CIRCLE_9);

    spell_assign(SPELL_FULL_HARM, CLASS_DIABOLIST, CIRCLE_10);
    spell_assign(SPELL_WORD_OF_COMMAND, CLASS_DIABOLIST, CIRCLE_10);

    spell_assign(SPELL_HELLS_GATE, CLASS_DIABOLIST, CIRCLE_11);
    spell_assign(SPELL_RESURRECT, CLASS_DIABOLIST, CIRCLE_11);

    /* DRUID */
    skill_assign(SKILL_BLUDGEONING, CLASS_DRUID, 1);
    skill_assign(SKILL_PIERCING, CLASS_DRUID, 1);
    skill_assign(SKILL_SLASHING, CLASS_DRUID, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_DRUID, 1);
    skill_assign(SKILL_SHAPECHANGE, CLASS_DRUID, 1);
    skill_assign(SKILL_TAME, CLASS_DRUID, 1);
    skill_assign(SKILL_DODGE, CLASS_DRUID, 20);

    spell_assign(SPELL_BARKSKIN, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_DRUID, CIRCLE_1);

    spell_assign(SPELL_CURE_LIGHT, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_EARTH_BLESSING, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_NIGHT_VISION, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_DRUID, CIRCLE_2);

    spell_assign(SPELL_CURE_SERIOUS, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_ENDURANCE, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_MOONBEAM, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_WRITHING_WEEDS, CLASS_DRUID, CIRCLE_3);

    spell_assign(SPELL_CREATE_SPRING, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_CURE_BLIND, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_CURE_CRITIC, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_EARTHQUAKE, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_ELEMENTAL_WARDING, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_DRUID, CIRCLE_4);

    spell_assign(SPELL_CONTROL_WEATHER, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_HARM, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_NOURISHMENT, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_REMOVE_PARALYSIS, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_REVEAL_HIDDEN, CLASS_DRUID, CIRCLE_5);

    spell_assign(SPELL_DARKNESS, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_HEAL, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_WATERWALK, CLASS_DRUID, CIRCLE_6);

    spell_assign(SPELL_CALL_LIGHTNING, CLASS_DRUID, CIRCLE_7);
    spell_assign(SPELL_GAIAS_CLOAK, CLASS_DRUID, CIRCLE_7);
    spell_assign(SPELL_ILLUMINATION, CLASS_DRUID, CIRCLE_7);

    spell_assign(SPELL_ACID_FOG, CLASS_DRUID, CIRCLE_8);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_DRUID, CIRCLE_8);
    spell_assign(SPELL_ENTANGLE, CLASS_DRUID, CIRCLE_8);
    spell_assign(SPELL_NATURES_EMBRACE, CLASS_DRUID, CIRCLE_8);
    spell_assign(SPELL_URBAN_RENEWAL, CLASS_DRUID, CIRCLE_8);

    spell_assign(SPELL_ARMOR_OF_GAIA, CLASS_DRUID, CIRCLE_9);
    spell_assign(SPELL_INVIGORATE, CLASS_DRUID, CIRCLE_9);
    spell_assign(SPELL_SUNRAY, CLASS_DRUID, CIRCLE_9);

    spell_assign(SPELL_MOONWELL, CLASS_DRUID, CIRCLE_10);
    spell_assign(SPELL_WANDERING_WOODS, CLASS_DRUID, CIRCLE_10);

    spell_assign(SPELL_CREEPING_DOOM, CLASS_DRUID, CIRCLE_11);

    /* HUNTER */
    skill_assign(SKILL_GUARD, CLASS_HUNTER, 1);
    skill_assign(SKILL_SWITCH, CLASS_HUNTER, 1);
    skill_assign(SKILL_KICK, CLASS_HUNTER, 1);
    skill_assign(SKILL_RESCUE, CLASS_HUNTER, 1);
    skill_assign(SKILL_BASH, CLASS_HUNTER, 1);
    skill_assign(SKILL_DUAL_WIELD, CLASS_HUNTER, 1);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_HUNTER, 1);
    skill_assign(SKILL_DODGE, CLASS_HUNTER, 1);
    skill_assign(SKILL_PARRY, CLASS_HUNTER, 1);
    skill_assign(SKILL_RIPOSTE, CLASS_HUNTER, 1);
    skill_assign(SKILL_BLUDGEONING, CLASS_HUNTER, 1);
    skill_assign(SKILL_PIERCING, CLASS_HUNTER, 1);
    skill_assign(SKILL_SLASHING, CLASS_HUNTER, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_HUNTER, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_HUNTER, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_HUNTER, 1);
    skill_assign(SKILL_TAME, CLASS_HUNTER, 7);
    skill_assign(SKILL_TRACK, CLASS_HUNTER, 36);
    skill_assign(SKILL_HUNT, CLASS_HUNTER, 41);

    /* ILLUSIONIST */
    skill_assign(SKILL_CONCEAL, CLASS_ILLUSIONIST, 10);
    skill_assign(SKILL_BACKSTAB, CLASS_ILLUSIONIST, 15);
    skill_assign(SKILL_HIDE, CLASS_ILLUSIONIST, 20);

    spell_assign(SPELL_DETECT_MAGIC, CLASS_ILLUSIONIST, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_ILLUSIONIST, CIRCLE_1);
    spell_assign(SPELL_VENTRILOQUATE, CLASS_ILLUSIONIST, CIRCLE_1);

    spell_assign(SPELL_CHILL_TOUCH, CLASS_ILLUSIONIST, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_ILLUSIONIST, CIRCLE_2);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_ILLUSIONIST, CIRCLE_2);
    spell_assign(SPELL_MAGIC_TORCH, CLASS_ILLUSIONIST, CIRCLE_2);
    spell_assign(SPELL_PHANTASM, CLASS_ILLUSIONIST, CIRCLE_2);

    spell_assign(SPELL_CHARM, CLASS_ILLUSIONIST, CIRCLE_3);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_ILLUSIONIST, CIRCLE_3);
    spell_assign(SPELL_FEAR, CLASS_ILLUSIONIST, CIRCLE_3);
    spell_assign(SPELL_INVISIBLE, CLASS_ILLUSIONIST, CIRCLE_3);
    spell_assign(SPELL_MESMERIZE, CLASS_ILLUSIONIST, CIRCLE_3);

    spell_assign(SPELL_BLINDNESS, CLASS_ILLUSIONIST, CIRCLE_4);
    spell_assign(SPELL_CONFUSION, CLASS_ILLUSIONIST, CIRCLE_4);
    spell_assign(SPELL_MISDIRECTION, CLASS_ILLUSIONIST, CIRCLE_4);
    spell_assign(SPELL_NIGHTMARE, CLASS_ILLUSIONIST, CIRCLE_4);
    spell_assign(SPELL_REVEAL_HIDDEN, CLASS_ILLUSIONIST, CIRCLE_4);

    spell_assign(SPELL_COLOR_SPRAY, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_DISCORPORATE, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_INFRAVISION, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_SIMULACRUM, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_ILLUSIONIST, CIRCLE_5);

    spell_assign(SPELL_DIMENSION_DOOR, CLASS_ILLUSIONIST, CIRCLE_6);
    spell_assign(SPELL_ILLUMINATION, CLASS_ILLUSIONIST, CIRCLE_6);
    spell_assign(SPELL_ISOLATION, CLASS_ILLUSIONIST, CIRCLE_6);

    spell_assign(SPELL_FARSEE, CLASS_ILLUSIONIST, CIRCLE_7);
    spell_assign(SPELL_HYSTERIA, CLASS_ILLUSIONIST, CIRCLE_7);
    spell_assign(SPELL_INSANITY, CLASS_ILLUSIONIST, CIRCLE_7);

    spell_assign(SPELL_ILLUSORY_WALL, CLASS_ILLUSIONIST, CIRCLE_8);
    spell_assign(SPELL_MASS_INVIS, CLASS_ILLUSIONIST, CIRCLE_8);

    spell_assign(SPELL_FAMILIARITY, CLASS_ILLUSIONIST, CIRCLE_9);

    spell_assign(SPELL_SEVERANCE, CLASS_ILLUSIONIST, CIRCLE_10);

    spell_assign(SPELL_GLORY, CLASS_ILLUSIONIST, CIRCLE_11);

    spell_assign(SPELL_SOUL_REAVER, CLASS_ILLUSIONIST, CIRCLE_12);

    /* MERCENARY */
    skill_assign(SKILL_BLUDGEONING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_PIERCING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_SLASHING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_KICK, CLASS_MERCENARY, 1);
    skill_assign(SKILL_BASH, CLASS_MERCENARY, 1);
    skill_assign(SKILL_DODGE, CLASS_MERCENARY, 1);
    skill_assign(SKILL_GUARD, CLASS_MERCENARY, 10);
    skill_assign(SKILL_BACKSTAB, CLASS_MERCENARY, 10);
    skill_assign(SKILL_DUAL_WIELD, CLASS_MERCENARY, 15);
    skill_assign(SKILL_BIND, CLASS_MERCENARY, 16);
    skill_assign(SKILL_HIDE, CLASS_MERCENARY, 20);
    skill_assign(SKILL_DISARM, CLASS_MERCENARY, 20);
    skill_assign(SKILL_PARRY, CLASS_MERCENARY, 30);
    skill_assign(SKILL_TRACK, CLASS_MERCENARY, 30);
    skill_assign(SKILL_SWITCH, CLASS_MERCENARY, 40);
    skill_assign(SKILL_RETREAT, CLASS_MERCENARY, 40);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_MERCENARY, 50);
    skill_assign(SKILL_RIPOSTE, CLASS_MERCENARY, 60);
    skill_assign(SKILL_GROUP_RETREAT, CLASS_MERCENARY, 80);

    /* MONKS */
    skill_assign(SKILL_CHANT, CLASS_MONK, 15);
    skill_assign(SKILL_SAFEFALL, CLASS_MONK, 1);
    skill_assign(SKILL_BAREHAND, CLASS_MONK, 1);
    skill_assign(SKILL_KICK, CLASS_MONK, 1);
    skill_assign(SKILL_DUAL_WIELD, CLASS_MONK, 1);
    skill_assign(SKILL_DODGE, CLASS_MONK, 1);
    skill_assign(SKILL_BLUDGEONING, CLASS_MONK, 1);
    skill_assign(SKILL_PARRY, CLASS_MONK, 10);
    skill_assign(SKILL_RIPOSTE, CLASS_MONK, 20);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_MONK, 30);
    skill_assign(SKILL_SWITCH, CLASS_MONK, 40);
    skill_assign(SKILL_SPRINGLEAP, CLASS_MONK, 50);
    skill_assign(SKILL_CORNER, CLASS_MONK, 80);

    chant_assign(CHANT_REGENERATION, CLASS_MONK, 15);
    chant_assign(CHANT_BATTLE_HYMN, CLASS_MONK, 30);
    chant_assign(CHANT_SHADOWS_SORROW_SONG, CLASS_MONK, 30);
    chant_assign(CHANT_IVORY_SYMPHONY, CLASS_MONK, 45);
    chant_assign(CHANT_ARIA_OF_DISSONANCE, CLASS_MONK, 60);
    chant_assign(CHANT_SONATA_OF_MALAISE, CLASS_MONK, 60);
    chant_assign(CHANT_PEACE, CLASS_MONK, 70);
    chant_assign(CHANT_APOCALYPTIC_ANTHEM, CLASS_MONK, 75);
    chant_assign(CHANT_WAR_CRY, CLASS_MONK, 90);
    chant_assign(CHANT_SEED_OF_DESTRUCTION, CLASS_MONK, 99);

    /* MYSTIC */
    skill_assign(SKILL_BLUDGEONING, CLASS_MYSTIC, 1);
    skill_assign(SKILL_SLASHING, CLASS_MYSTIC, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_MYSTIC, 1);
    skill_assign(SKILL_DODGE, CLASS_MYSTIC, 1);

    spell_assign(SPELL_ARMOR, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_BLESS, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_MYSTIC, CIRCLE_1);

    spell_assign(SPELL_CURE_BLIND, CLASS_MYSTIC, CIRCLE_2);
    spell_assign(SPELL_CURE_CRITIC, CLASS_MYSTIC, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_MYSTIC, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_MYSTIC, CIRCLE_2);

    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_MYSTIC, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_MYSTIC, CIRCLE_3);
    spell_assign(SPELL_SUMMON, CLASS_MYSTIC, CIRCLE_3);

    spell_assign(SPELL_BLINDNESS, CLASS_MYSTIC, CIRCLE_4);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_MYSTIC, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_MYSTIC, CIRCLE_4);
    spell_assign(SPELL_SENSE_LIFE, CLASS_MYSTIC, CIRCLE_4);

    spell_assign(SPELL_EARTHQUAKE, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_HARM, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_HEAL, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_VITALITY, CLASS_MYSTIC, CIRCLE_5);

    spell_assign(SPELL_DARKNESS, CLASS_MYSTIC, CIRCLE_6);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_MYSTIC, CIRCLE_6);

    spell_assign(SPELL_FULL_HEAL, CLASS_MYSTIC, CIRCLE_7);

    spell_assign(SPELL_GROUP_HEAL, CLASS_MYSTIC, CIRCLE_8);

    spell_assign(SPELL_FULL_HARM, CLASS_MYSTIC, CIRCLE_9);
    spell_assign(SPELL_INFRAVISION, CLASS_MYSTIC, CIRCLE_9);

    spell_assign(SPELL_GROUP_ARMOR, CLASS_MYSTIC, CIRCLE_10);

    /* NECRO */
    spell_assign(SPELL_DECAY, CLASS_NECROMANCER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_NECROMANCER, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_NECROMANCER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_NECROMANCER, CIRCLE_1);

    spell_assign(SPELL_BURNING_HANDS, CLASS_NECROMANCER, CIRCLE_2);
    spell_assign(SPELL_CONCEALMENT, CLASS_NECROMANCER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_NECROMANCER, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_NECROMANCER, CIRCLE_2);

    spell_assign(SPELL_ANIMATE_DEAD, CLASS_NECROMANCER, CIRCLE_3);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_NECROMANCER, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_NECROMANCER, CIRCLE_3);
    spell_assign(SPELL_ENHANCE_ABILITY, CLASS_NECROMANCER, CIRCLE_3);

    spell_assign(SPELL_BONE_ARMOR, CLASS_NECROMANCER, CIRCLE_4);
    spell_assign(SPELL_COLDSHIELD, CLASS_NECROMANCER, CIRCLE_4);
    spell_assign(SPELL_SPINECHILLER, CLASS_NECROMANCER, CIRCLE_4);

    spell_assign(SPELL_CONE_OF_COLD, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_ENERGY_DRAIN, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_IDENTIFY, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_INFRAVISION, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_NECROMANCER, CIRCLE_5);

    spell_assign(SPELL_DETECT_POISON, CLASS_NECROMANCER, CIRCLE_6);
    spell_assign(SPELL_INVISIBLE, CLASS_NECROMANCER, CIRCLE_6);
    spell_assign(SPELL_PYRE, CLASS_NECROMANCER, CIRCLE_6);
    spell_assign(SPELL_SUMMON_CORPSE, CLASS_NECROMANCER, CIRCLE_6);

    spell_assign(SPELL_FARSEE, CLASS_NECROMANCER, CIRCLE_7);
    spell_assign(SPELL_HASTE, CLASS_NECROMANCER, CIRCLE_7);
    spell_assign(SPELL_IRON_MAIDEN, CLASS_NECROMANCER, CIRCLE_7);

    spell_assign(SPELL_DIMENSION_DOOR, CLASS_NECROMANCER, CIRCLE_8);
    spell_assign(SPELL_FRACTURE, CLASS_NECROMANCER, CIRCLE_8);

    spell_assign(SPELL_SOUL_TAP, CLASS_NECROMANCER, CIRCLE_9);
    spell_assign(SPELL_STONE_SKIN, CLASS_NECROMANCER, CIRCLE_9);

    spell_assign(SPELL_BONE_CAGE, CLASS_NECROMANCER, CIRCLE_10);
    spell_assign(SPELL_REBUKE_UNDEAD, CLASS_NECROMANCER, CIRCLE_10);

    spell_assign(SPELL_DEGENERATION, CLASS_NECROMANCER, CIRCLE_11);

    spell_assign(SPELL_STONE_SKIN, CLASS_NECROMANCER, CIRCLE_12);

    spell_assign(SPELL_SHIFT_CORPSE, CLASS_NECROMANCER, CIRCLE_13);

    /* PALADIN */
    skill_assign(SKILL_BLUDGEONING, CLASS_PALADIN, 1);
    skill_assign(SKILL_PIERCING, CLASS_PALADIN, 1);
    skill_assign(SKILL_SLASHING, CLASS_PALADIN, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_PALADIN, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_PALADIN, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_PALADIN, 1);
    skill_assign(SKILL_KICK, CLASS_PALADIN, 1);
    skill_assign(SKILL_BASH, CLASS_PALADIN, 1);
    skill_assign(SKILL_TAME, CLASS_PALADIN, 1);
    skill_assign(SKILL_DODGE, CLASS_PALADIN, 1);
    skill_assign(SKILL_GUARD, CLASS_PALADIN, 10);
    skill_assign(SKILL_SWITCH, CLASS_PALADIN, 10);
    skill_assign(SKILL_RESCUE, CLASS_PALADIN, 10);
    skill_assign(SKILL_SUMMON_MOUNT, CLASS_PALADIN, 15);
    skill_assign(SKILL_DUAL_WIELD, CLASS_PALADIN, 20);
    skill_assign(SKILL_PARRY, CLASS_PALADIN, 20);
    skill_assign(SKILL_RIPOSTE, CLASS_PALADIN, 40);
    skill_assign(SKILL_HITALL, CLASS_PALADIN, 80);
    skill_assign(SKILL_DISARM, CLASS_PALADIN, 50);
    skill_assign(SKILL_RETREAT, CLASS_PALADIN, 60);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_PALADIN, 70);

    spell_assign(SPELL_BLESS, CLASS_PALADIN, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_PALADIN, CIRCLE_1);

    spell_assign(SPELL_ARMOR, CLASS_PALADIN, CIRCLE_2);

    spell_assign(SPELL_CREATE_FOOD, CLASS_PALADIN, CIRCLE_3);
    spell_assign(SPELL_CREATE_WATER, CLASS_PALADIN, CIRCLE_3);

    spell_assign(SPELL_CURE_SERIOUS, CLASS_PALADIN, CIRCLE_4);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_PALADIN, CIRCLE_4);

    spell_assign(SPELL_DETECT_POISON, CLASS_PALADIN, CIRCLE_5);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_PALADIN, CIRCLE_5);

    spell_assign(SPELL_CURE_BLIND, CLASS_PALADIN, CIRCLE_6);
    spell_assign(SPELL_CURE_CRITIC, CLASS_PALADIN, CIRCLE_6);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_PALADIN, CIRCLE_7);
    spell_assign(SPELL_REMOVE_PARALYSIS, CLASS_PALADIN, CIRCLE_7);
    spell_assign(SPELL_REMOVE_POISON, CLASS_PALADIN, CIRCLE_7);

    spell_assign(SPELL_SOULSHIELD, CLASS_PALADIN, CIRCLE_8);

    spell_assign(SPELL_HEAL, CLASS_PALADIN, CIRCLE_9);

    spell_assign(SPELL_HOLY_WORD, CLASS_PALADIN, CIRCLE_10);

    /* PRIEST */
    skill_assign(SKILL_BLUDGEONING, CLASS_PRIEST, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_PRIEST, 1);
    skill_assign(SKILL_DODGE, CLASS_PRIEST, 20);

    spell_assign(SPELL_ARMOR, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CAUSE_LIGHT, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_PRIEST, CIRCLE_1);

    spell_assign(SPELL_BLESS, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_CIRCLE_OF_LIGHT, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_PRIEST, CIRCLE_2);

    spell_assign(SPELL_CAUSE_CRITIC, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_CURE_BLIND, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_DIVINE_BOLT, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_ENDURANCE, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_PRIEST, CIRCLE_3);

    spell_assign(SPELL_BLINDNESS, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_ELEMENTAL_WARDING, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_REMOVE_PARALYSIS, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_SENSE_LIFE, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_PRIEST, CIRCLE_4);

    spell_assign(SPELL_DESTROY_UNDEAD, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_EARTHQUAKE, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_HEAL, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_PRAYER, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_SANE_MIND, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_SOULSHIELD, CLASS_PRIEST, CIRCLE_5);

    spell_assign(SPELL_DIVINE_RAY, CLASS_PRIEST, CIRCLE_6);
    spell_assign(SPELL_ILLUMINATION, CLASS_PRIEST, CIRCLE_6);
    spell_assign(SPELL_LESSER_EXORCISM, CLASS_PRIEST, CIRCLE_6);
    spell_assign(SPELL_SILENCE, CLASS_PRIEST, CIRCLE_6);

    spell_assign(SPELL_FULL_HEAL, CLASS_PRIEST, CIRCLE_7);
    spell_assign(SPELL_VITALITY, CLASS_PRIEST, CIRCLE_7);
    spell_assign(SPELL_WATERWALK, CLASS_PRIEST, CIRCLE_7);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_ENLIGHTENMENT, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_GROUP_ARMOR, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_GROUP_HEAL, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_SPEAK_IN_TONGUES, CLASS_PRIEST, CIRCLE_8);

    spell_assign(SPELL_BANISH, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_EXORCISM, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_GREATER_VITALITY, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_HOLY_WORD, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_WINGS_OF_HEAVEN, CLASS_PRIEST, CIRCLE_9);

    spell_assign(SPELL_DIVINE_ESSENCE, CLASS_PRIEST, CIRCLE_10);
    spell_assign(SPELL_FULL_HARM, CLASS_PRIEST, CIRCLE_10);
    spell_assign(SPELL_WORD_OF_COMMAND, CLASS_PRIEST, CIRCLE_10);

    spell_assign(SPELL_HEAVENS_GATE, CLASS_PRIEST, CIRCLE_11);
    spell_assign(SPELL_RESURRECT, CLASS_PRIEST, CIRCLE_11);

    spell_assign(SPELL_DRAGONS_HEALTH, CLASS_PRIEST, CIRCLE_12);

    /* PYROMANCER */
    spell_assign(SPELL_BURNING_HANDS, CLASS_PYROMANCER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_PYROMANCER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_PYROMANCER, CIRCLE_1);

    spell_assign(SPELL_CONCEALMENT, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_DETONATION, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_FIRE_DARTS, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_MAGIC_TORCH, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_ENHANCE_ABILITY, CLASS_PYROMANCER, CIRCLE_2);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_PYROMANCER, CIRCLE_3);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_PYROMANCER, CIRCLE_3);
    spell_assign(SPELL_PHOSPHORIC_EMBERS, CLASS_PYROMANCER, CIRCLE_3);
    spell_assign(SPELL_SMOKE, CLASS_PYROMANCER, CIRCLE_3);

    spell_assign(SPELL_FIREBALL, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_FIRESHIELD, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_FLAME_BLADE, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_INFRAVISION, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_FEATHER_FALL, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_MIRAGE, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_RAY_OF_ENFEEB, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_TELEPORT, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_WORLD_TELEPORT, CLASS_PYROMANCER, CIRCLE_4);

    spell_assign(SPELL_DIMENSION_DOOR, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_FARSEE, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_INVISIBLE, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_MINOR_PARALYSIS, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_POSITIVE_FIELD, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_PYROMANCER, CIRCLE_5);

    spell_assign(SPELL_FIRESTORM, CLASS_PYROMANCER, CIRCLE_6);
    spell_assign(SPELL_HASTE, CLASS_PYROMANCER, CIRCLE_6);
    spell_assign(SPELL_MINOR_GLOBE, CLASS_PYROMANCER, CIRCLE_6);
    spell_assign(SPELL_STONE_SKIN, CLASS_PYROMANCER, CIRCLE_6);

    spell_assign(SPELL_ACID_BURST, CLASS_PYROMANCER, CIRCLE_7);

    spell_assign(SPELL_FLY, CLASS_PYROMANCER, CIRCLE_8);
    spell_assign(SPELL_MAJOR_GLOBE, CLASS_PYROMANCER, CIRCLE_8);
    spell_assign(SPELL_MELT, CLASS_PYROMANCER, CIRCLE_8);

    spell_assign(SPELL_CIRCLE_OF_FIRE, CLASS_PYROMANCER, CIRCLE_9);
    spell_assign(SPELL_IMMOLATE, CLASS_PYROMANCER, CIRCLE_9);
    spell_assign(SPELL_MASS_INVIS, CLASS_PYROMANCER, CIRCLE_9);
    spell_assign(SPELL_RELOCATE, CLASS_PYROMANCER, CIRCLE_9);

    spell_assign(SPELL_CREMATE, CLASS_PYROMANCER, CIRCLE_10);
    spell_assign(SPELL_NEGATE_HEAT, CLASS_PYROMANCER, CIRCLE_10);

    spell_assign(SPELL_METEORSWARM, CLASS_PYROMANCER, CIRCLE_11);

    spell_assign(SPELL_SUPERNOVA, CLASS_PYROMANCER, CIRCLE_12);

    /* RANGER */
    skill_assign(SKILL_BLUDGEONING, CLASS_RANGER, 1);
    skill_assign(SKILL_PIERCING, CLASS_RANGER, 1);
    skill_assign(SKILL_SLASHING, CLASS_RANGER, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_RANGER, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_RANGER, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_RANGER, 1);
    skill_assign(SKILL_KICK, CLASS_RANGER, 1);
    skill_assign(SKILL_TRACK, CLASS_RANGER, 1);
    skill_assign(SKILL_BASH, CLASS_RANGER, 1);
    skill_assign(SKILL_TAME, CLASS_RANGER, 1);
    skill_assign(SKILL_DUAL_WIELD, CLASS_RANGER, 1);
    skill_assign(SKILL_DODGE, CLASS_RANGER, 10);
    skill_assign(SKILL_SWITCH, CLASS_RANGER, 10);
    skill_assign(SKILL_RESCUE, CLASS_RANGER, 35);
    skill_assign(SKILL_PARRY, CLASS_RANGER, 30);
    skill_assign(SKILL_RIPOSTE, CLASS_RANGER, 40);
    skill_assign(SKILL_GUARD, CLASS_RANGER, 50);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_RANGER, 60);

    spell_assign(SPELL_DETECT_ALIGN, CLASS_RANGER, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_RANGER, CIRCLE_1);

    spell_assign(SPELL_CURE_LIGHT, CLASS_RANGER, CIRCLE_2);

    spell_assign(SPELL_BARKSKIN, CLASS_RANGER, CIRCLE_3);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_RANGER, CIRCLE_3);

    spell_assign(SPELL_CURE_SERIOUS, CLASS_RANGER, CIRCLE_4);
    spell_assign(SPELL_NIMBLE, CLASS_RANGER, CIRCLE_4);

    spell_assign(SPELL_SHOCKING_GRASP, CLASS_RANGER, CIRCLE_5);

    spell_assign(SPELL_CURE_CRITIC, CLASS_RANGER, CIRCLE_6);

    spell_assign(SPELL_SENSE_LIFE, CLASS_RANGER, CIRCLE_7);

    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_RANGER, CIRCLE_8);

    spell_assign(SPELL_NATURES_GUIDANCE, CLASS_RANGER, CIRCLE_9);

    spell_assign(SPELL_BLUR, CLASS_RANGER, CIRCLE_11);

    /* ROGUE */
    skill_assign(SKILL_BLUDGEONING, CLASS_ROGUE, 1);
    skill_assign(SKILL_PIERCING, CLASS_ROGUE, 1);
    skill_assign(SKILL_SLASHING, CLASS_ROGUE, 1);
    skill_assign(SKILL_HIDE, CLASS_ROGUE, 1);
    skill_assign(SKILL_DODGE, CLASS_ROGUE, 1);
    skill_assign(SKILL_SNEAK, CLASS_ROGUE, 1);
    skill_assign(SKILL_SNEAK_ATTACK, CLASS_ROGUE, 1);
    skill_assign(SKILL_BACKSTAB, CLASS_ROGUE, 1);
    skill_assign(SKILL_PICK_LOCK, CLASS_ROGUE, 5);
    skill_assign(SKILL_DUAL_WIELD, CLASS_ROGUE, 15);
    skill_assign(SKILL_CONCEAL, CLASS_ROGUE, 25);
    skill_assign(SKILL_TRACK, CLASS_ROGUE, 30);
    skill_assign(SKILL_PARRY, CLASS_ROGUE, 40);
    skill_assign(SKILL_STEALTH, CLASS_ROGUE, 50);
    skill_assign(SKILL_CORNER, CLASS_ROGUE, 60);
    skill_assign(SKILL_SHADOW, CLASS_ROGUE, 60);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_ROGUE, 70);

    /* SHAMAN */
    skill_assign(SKILL_DODGE, CLASS_SHAMAN, 1);
    skill_assign(SKILL_BLUDGEONING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_PIERCING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_SLASHING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_TAME, CLASS_SHAMAN, 7);

    spell_assign(SPELL_ARMOR, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_DETECT_POISON, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_SPIRIT_ARROWS, CLASS_SHAMAN, CIRCLE_1);

    spell_assign(SPELL_BLESS, CLASS_SHAMAN, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_SHAMAN, CIRCLE_2);

    spell_assign(SPELL_CURE_CRITIC, CLASS_SHAMAN, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_SHAMAN, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_GOOD, CLASS_SHAMAN, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_SHAMAN, CIRCLE_3);
    spell_assign(SPELL_WEB, CLASS_SHAMAN, CIRCLE_3);

    spell_assign(SPELL_CURE_BLIND, CLASS_SHAMAN, CIRCLE_4);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_SHAMAN, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_SHAMAN, CIRCLE_4);
    spell_assign(SPELL_REVEAL_HIDDEN, CLASS_SHAMAN, CIRCLE_4);

    spell_assign(SPELL_HARM, CLASS_SHAMAN, CIRCLE_5);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_SHAMAN, CIRCLE_5);
    spell_assign(SPELL_REMOVE_PARALYSIS, CLASS_SHAMAN, CIRCLE_5);
    spell_assign(SPELL_SUMMON, CLASS_SHAMAN, CIRCLE_5);

    spell_assign(SPELL_ANCESTRAL_VENGEANCE, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_BLINDNESS, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_DARKNESS, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_EARTHQUAKE, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_FIREBALL, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_HEAL, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_VITALITY, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_SHAMAN, CIRCLE_6);

    spell_assign(SPELL_BALEFUL_POLYMORPH, CLASS_SHAMAN, CIRCLE_7);
    spell_assign(SPELL_DETECT_INVIS, CLASS_SHAMAN, CIRCLE_7);
    spell_assign(SPELL_HASTE, CLASS_SHAMAN, CIRCLE_7);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_SHAMAN, CIRCLE_7);

    spell_assign(SPELL_CIRCLE_OF_DEATH, CLASS_SHAMAN, CIRCLE_8);
    spell_assign(SPELL_ACID_FOG, CLASS_SHAMAN, CIRCLE_8);

    spell_assign(SPELL_SPIRIT_RAY, CLASS_SHAMAN, CIRCLE_9);

    spell_assign(SPELL_GROUP_HEAL, CLASS_SHAMAN, CIRCLE_10);

    /* SORCERER */
    spell_assign(SPELL_BURNING_HANDS, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_CONCEALMENT, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_SORCERER, CIRCLE_1);

    spell_assign(SPELL_CHILL_TOUCH, CLASS_SORCERER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_SORCERER, CIRCLE_2);
    spell_assign(SPELL_ENHANCE_ABILITY, CLASS_SORCERER, CIRCLE_2);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_SORCERER, CIRCLE_3);
    spell_assign(SPELL_IDENTIFY, CLASS_SORCERER, CIRCLE_3);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_SORCERER, CIRCLE_3);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_SORCERER, CIRCLE_3);
    spell_assign(SPELL_WEB, CLASS_SORCERER, CIRCLE_3);

    spell_assign(SPELL_COLDSHIELD, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_FIRESHIELD, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_INFRAVISION, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_FEATHER_FALL, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_MINOR_GLOBE, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_MINOR_PARALYSIS, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_RAY_OF_ENFEEB, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_TELEPORT, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_WORLD_TELEPORT, CLASS_SORCERER, CIRCLE_4);

    spell_assign(SPELL_COLOR_SPRAY, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_CONE_OF_COLD, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_FARSEE, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_INVISIBLE, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_SORCERER, CIRCLE_5);

    spell_assign(SPELL_FIREBALL, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_FLY, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_HASTE, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_ICE_STORM, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_STONE_SKIN, CLASS_SORCERER, CIRCLE_6);

    spell_assign(SPELL_BIGBYS_CLENCHED_FIST, CLASS_SORCERER, CIRCLE_7);

    spell_assign(SPELL_CHAIN_LIGHTNING, CLASS_SORCERER, CIRCLE_8);
    spell_assign(SPELL_HARNESS, CLASS_SORCERER, CIRCLE_8);
    spell_assign(SPELL_MAJOR_GLOBE, CLASS_SORCERER, CIRCLE_8);

    spell_assign(SPELL_DISINTEGRATE, CLASS_SORCERER, CIRCLE_9);
    spell_assign(SPELL_MASS_INVIS, CLASS_SORCERER, CIRCLE_9);
    spell_assign(SPELL_RELOCATE, CLASS_SORCERER, CIRCLE_9);

    spell_assign(SPELL_METEORSWARM, CLASS_SORCERER, CIRCLE_10);

    spell_assign(SPELL_ENLARGE, CLASS_SORCERER, CIRCLE_11);
    spell_assign(SPELL_REDUCE, CLASS_SORCERER, CIRCLE_11);
    spell_assign(SPELL_WIZARD_EYE, CLASS_SORCERER, CIRCLE_11);

    spell_assign(SPELL_CHARM, CLASS_SORCERER, CIRCLE_12);

    /* THIEF */
    skill_assign(SKILL_BLUDGEONING, CLASS_THIEF, 1);
    skill_assign(SKILL_PIERCING, CLASS_THIEF, 1);
    skill_assign(SKILL_SLASHING, CLASS_THIEF, 1);
    skill_assign(SKILL_SNEAK, CLASS_THIEF, 1);
    skill_assign(SKILL_BACKSTAB, CLASS_THIEF, 1);
    skill_assign(SKILL_HIDE, CLASS_THIEF, 1);
    skill_assign(SKILL_DODGE, CLASS_THIEF, 1);
    skill_assign(SKILL_PICK_LOCK, CLASS_THIEF, 1);
    skill_assign(SKILL_STEAL, CLASS_THIEF, 10);
    skill_assign(SKILL_EYE_GOUGE, CLASS_THIEF, 10);
    skill_assign(SKILL_CONCEAL, CLASS_THIEF, 10);
    skill_assign(SKILL_DUAL_WIELD, CLASS_THIEF, 15);
    skill_assign(SKILL_PARRY, CLASS_THIEF, 30);
    skill_assign(SKILL_REND, CLASS_THIEF, 30);
    skill_assign(SKILL_TRACK, CLASS_THIEF, 40);
    skill_assign(SKILL_DISARM, CLASS_THIEF, 40);
    skill_assign(SKILL_STEALTH, CLASS_THIEF, 50);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_THIEF, 75);

    /* WARRIOR */
    skill_assign(SKILL_BLUDGEONING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_PIERCING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_SLASHING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_KICK, CLASS_WARRIOR, 1);
    skill_assign(SKILL_BASH, CLASS_WARRIOR, 1);
    skill_assign(SKILL_DODGE, CLASS_WARRIOR, 1);
    skill_assign(SKILL_SWITCH, CLASS_WARRIOR, 10);
    skill_assign(SKILL_RESCUE, CLASS_WARRIOR, 15);
    skill_assign(SKILL_DISARM, CLASS_WARRIOR, 20);
    skill_assign(SKILL_GUARD, CLASS_WARRIOR, 25);
    skill_assign(SKILL_DUAL_WIELD, CLASS_WARRIOR, 25);
    skill_assign(SKILL_PARRY, CLASS_WARRIOR, 30);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_WARRIOR, 35);
    skill_assign(SKILL_RIPOSTE, CLASS_WARRIOR, 40);
    skill_assign(SKILL_HITALL, CLASS_WARRIOR, 50);
    skill_assign(SKILL_RETREAT, CLASS_WARRIOR, 60);
}

/* convert_class does no checking.  It expects a valid class and ch.
 *
 * This function changes a player's class and converts the skills/spells
 * accordingly, keeping the old values if they are better.
 * It also transfers quest spells. */
void convert_class(CharData *ch, int newclass) {
    int skill;
    sh_int old_skills[TOP_SKILL + 1];
    sh_int new_skills[TOP_SKILL + 1];

    /* read in the player's old skills */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        old_skills[skill] = GET_ISKILL(ch, skill);
    }
    /* set class */
    GET_CLASS(ch) = newclass;

    /* Big changes occur here: */
    update_char(ch);

    /* read the new skills */
    for (skill = 0; skill < MAX_SKILLS + 1; skill++) {
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

int getbaseclass(int class_num) {
    if (classes[class_num].is_subclass)
        return classes[class_num].subclass_of;
    return class_num;
}
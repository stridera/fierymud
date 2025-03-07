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
#include "races.hpp"
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
    int max_subclass_level;   /* Highest level at which you can subclass (if base class) */
    int homeroom;             /* Room you'll start in.  Used in character creation only. */
    int statorder[NUM_STATS]; /* When generating a new character, which stats should be highest? */
    int saves[NUM_SAVES];     /* Class-related base saving throws */
    int hp_lev;               /* How many hit points gained per level */
    int thac0;                /* How good you are at hitting people. Lower=better */
    int nowear_flag;          /* What ITEM_ flag will prevent wearing an object? */
    int hit_regen_factor;     /* Factor: 100 = normal regen */
    int bonus_focus;          /* Bonus focus (how quickly you recover spell slots) for your class */
    int mv_regen_factor;
    double exp_gain_factor; /* Exp factor: 1 = normal exp needed to gain a level */

    /* The following values primarily adjust stats on mob prototypes.  See db.c. */
    int exp_factor;
    int hit_factor;
    int hd_factor;
    int dice_factor;
    int copper_factor;
    int ac_factor;

    int newbie_eq[20]; /* Objects given to new player characters (in addition to common newbie eq */

    /*
     * The following data members should come last in the struct so
     * that they can be omitted when classes are being defined.  Their
     * values are filled in init_classes at runtime.
     */

    /* List of permanent effect flags */
    flagvector effect_flags[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
};
extern ClassDef classes[NUM_CLASSES];

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

#define IS_SPELLCASTER_CLASS(ch) (VALID_CLASS(ch) ? classes[(int)GET_CLASS(ch)].magical : false)
#define IS_SPELLCASTER(ch) (IS_SPELLCASTER_CLASS(ch) || IS_SPELLCASTER_RACE(ch))

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
int parse_class(CharData *ch, CharData *vict, std::string_view arg);
int getbaseclass(int class_num);
void init_char_class(CharData *ch);
void update_char_class(CharData *ch);
void advance_level(CharData *ch, enum level_action);
void convert_class(CharData *ch, int newclass);
int return_max_skill(CharData *ch, int skill);
void give_newbie_eq(CharData *vict);

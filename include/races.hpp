/***************************************************************************
 *  File: races.h                                         Part of FieryMUD *
 *  Usage: header file for race structures and constants                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "composition.hpp"
#include "lifeforce.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/* The race of an individual is stored as a byte in struct char_player_data. */

/* The races */
#define RACE_UNDEFINED -1
#define RACE_HUMAN 0
#define RACE_ELF 1
#define RACE_GNOME 2
#define RACE_DWARF 3
#define RACE_TROLL 4
#define RACE_DROW 5
#define RACE_DUERGAR 6
#define RACE_OGRE 7
#define RACE_ORC 8
#define RACE_HALF_ELF 9
#define RACE_BARBARIAN 10
#define RACE_HALFLING 11
#define RACE_PLANT 12
#define RACE_HUMANOID 13
#define RACE_ANIMAL 14
#define RACE_DRAGON_GENERAL 15
#define RACE_GIANT 16
#define RACE_OTHER 17
#define RACE_GOBLIN 18
#define RACE_DEMON 19
#define RACE_BROWNIE 20
#define RACE_DRAGON_FIRE 21
#define RACE_DRAGON_FROST 22
#define RACE_DRAGON_ACID 23
#define RACE_DRAGON_LIGHTNING 24
#define RACE_DRAGON_GAS 25
#define RACE_DRAGONBORN_FIRE 26
#define RACE_DRAGONBORN_FROST 27
#define RACE_DRAGONBORN_ACID 28
#define RACE_DRAGONBORN_LIGHTNING 29
#define RACE_DRAGONBORN_GAS 30
#define RACE_SVERFNEBLIN 31
#define RACE_FAERIE_SEELIE 32
#define RACE_FAERIE_UNSEELIE 33
#define RACE_NYMPH 34
#define RACE_ARBOREAN 35

/* Make sure to update this number if you add a race. */
#define NUM_RACES 36

#define DEFAULT_RACE RACE_OTHER

/* Race-aligns */
#define RACE_ALIGN_UNKNOWN -1
#define RACE_ALIGN_GOOD 0
#define RACE_ALIGN_EVIL 1

#define NUM_RACE_SKILLS 15

#define GET_RACE(ch) ((ch)->player.race)
#define GET_RACE_ALIGN(ch) ((ch)->player.race_align)

#define VALID_RACENUM(num) (num < NUM_RACES)
#define VALID_RACE(ch) (VALID_RACENUM(GET_RACE(ch)))
#define RACE_ABBR(ch) (VALID_RACE(ch) ? races[(int)GET_RACE(ch)].displayname : "--")
#define RACE_FULL(ch) (VALID_RACE(ch) ? races[(int)GET_RACE(ch)].fullname : "--")
#define RACE_PLAINNAME(ch) (VALID_RACE(ch) ? races[(int)GET_RACE(ch)].plainname : "unknown")

#define RACE_ALIGN_ABBR(ch) (GET_RACE_ALIGN(ch) <= RACE_ALIGN_EVIL ? race_align_abbrevs[(int)GET_RACE_ALIGN(ch)] : 0)
#define IS_HUMANOID(ch) (VALID_RACE(ch) ? races[(int)GET_RACE(ch)].humanoid : false)

#define ALIGN_OF_RACE(race) (VALID_RACENUM(race) ? races[race].racealign : RACE_ALIGN_GOOD)
#define SIZE_OF_RACE(race) (VALID_RACENUM(race) ? races[race].def_size : SIZE_MEDIUM)

#define RACE_EXPFACTOR(race) (VALID_RACENUM(race) ? races[race].exp_factor : 100)
#define RACE_HITFACTOR(race) (VALID_RACENUM(race) ? races[race].hit_factor : 100)
#define RACE_HDFACTOR(race) (VALID_RACENUM(race) ? races[race].hd_factor : 100)
#define RACE_DICEFACTOR(race) (VALID_RACENUM(race) ? races[race].dice_factor : 100)
#define RACE_COPPERFACTOR(race) (VALID_RACENUM(race) ? races[race].copper_factor : 100)
#define RACE_ACFACTOR(race) (VALID_RACENUM(race) ? races[race].ac_factor : 100)
struct RaceDef {
    const char *name;        /* The basic name, uncapitalized and uncolored. */
    const char *names;       /* Additional names for searching purposes. */
    const char *displayname; /* The name with colors and strategic capitalization. */
    const char *fullname;    /* The long name with colors and capitalization */
    const char *plainname;   /* The name with capitalization but no colors. */
    bool playable;           /* Available to mortals? */
    bool humanoid;           /* Is it humanoid? */
    int racealign;           /* Is it considered a good or evil race? */
    int def_size;            /* The default size for a member of this race. */
    int def_align;           /* Default alignment */
    int bonus_damroll;
    int bonus_hitroll;
    int bonus_focus;     /* Bonus to focus */
    int def_lifeforce;   /* Default life force */
    int def_composition; /* Default composition */
    int mweight_lo;      /* Minimum weight (male) */
    int mweight_hi;      /* Maximum weight (male) */
    int mheight_lo;      /* Minimum height (male) */
    int mheight_hi;      /* Maximum height (male) */
    int fweight_lo;      /* Minimum weight (female) */
    int fweight_hi;      /* Maximum weight (female) */
    int fheight_lo;      /* Minimum height (female) */
    int fheight_hi;      /* Maximum height (female) */

    int attrib_scales[6];

    /* The following values primarily adjust stats on mob prototypes.  See db.c.
     */
    int exp_factor;
    int hit_factor;
    int hd_factor;
    int dice_factor;
    int copper_factor;
    int ac_factor;

    /* These may be NULL so that the default values of "enters" and "leaves"
     * will be used. */
    const char *enter_verb;
    const char *leave_verb;

    /*
     * The following data members should come last in the struct so
     * that they can be omitted when races are being defined.  Their
     * values are filled in init_races at runtime.
     */

    /* List of permanent effect flags */
    flagvector effect_flags[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
    struct {
        int skill;             /* Skill number */
        int proficiency;       /* Default proficiency (can be ROLL_SKILL_PROF) */
    } skills[NUM_RACE_SKILLS]; /* List of racial skills */
};
extern RaceDef races[NUM_RACES];
extern const char *race_align_abbrevs[];

void init_races(void);
int parse_race(CharData *ch, CharData *vict, char *arg);
int race_by_menu(char arg);
void send_race_menu(DescriptorData *d);
int interpret_race_selection(char arg);
void init_proto_race(CharData *ch);
void init_char_race(CharData *ch);
void update_char_race(CharData *ch);
int racial_skill_proficiency(int skill, int race, int level);
void convert_race(CharData *ch, int newrace);
void apply_racial_bonus_hit_and_dam(CharData *ch);
int natural_move(CharData *ch);
void scale_attribs(CharData *ch);

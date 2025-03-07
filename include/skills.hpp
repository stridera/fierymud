/***************************************************************************
 *  File: skills.h                                        Part of FieryMUD *
 *  Usage: header file for character skill management                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "casting.hpp"
#include "class.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define ROLL_SKILL_PROF (-1)

#define IS_SKILL(n) ((n) > MAX_SPELLS && (n) <= MAX_SKILLS)
#define IS_SPELL(n) ((n) >= 1 && (n) <= MAX_SPELLS)
#define IS_CHANT(n) ((n) > MAX_SONGS && (n) <= MAX_CHANTS)
#define IS_SONG(n) ((n) > MAX_SKILLS && (n) <= MAX_SONGS)

/* If you add another spell sphere, you'll have to modify these defines. */
#define IS_SPHERE_SKILL(n) ((n) >= SKILL_SPHERE_GENERIC && (n) <= SKILL_SPHERE_DIVIN)
#define NUM_SPHERE_SKILLS (SKILL_SPHERE_DIVIN - SKILL_SPHERE_GENERIC + 1)

struct SkillDef {
    const std::string_view name;
    int minpos;       /* Minimum position to cast */
    bool fighting_ok; /* Whether it can be cast when fighting */
    int mana_min;     /* Min amount of mana used by a spell (highest lev) */
    int mana_max;     /* Max amount of mana used by a spell (lowest lev) */
    int mana_change;  /* Change in mana used by spell from lev to lev */
    int min_level[NUM_CLASSES];
    int lowest_level;
    bool humanoid; /* Generally only available to humanoids */
    int routines;
    byte violent;
    int targets; /* See spells.h for use with TAR_XXX  */
    int addl_mem_time;
    int cast_time;
    int damage_type;
    int sphere;
    int pages; /* base number of pages for spell in spellbook */
    int quest; /* weather the spell is a quest spell or not   */
    const std::string_view wearoff;
    int min_race_level[NUM_RACES];
};

extern SkillDef skills[TOP_SKILL_DEFINE + 1];

#define SINFO skills[spellnum]

int level_to_circle(int level);
int circle_to_level(int circle);
#define IS_QUEST_SPELL(spellnum) (skills[(spellnum)].quest)
#define SKILL_LEVEL(ch, skillnum)                                                                                      \
    ((skills[(skillnum)].min_level[(int)GET_CLASS(ch)] <= skills[(skillnum)].min_race_level[(int)GET_RACE(ch)])        \
         ? skills[(skillnum)].min_level[(int)GET_CLASS(ch)]                                                            \
         : skills[(skillnum)].min_race_level[(int)GET_RACE(ch)])
#define SPELL_CIRCLE(ch, spellnum) (level_to_circle(SKILL_LEVEL(ch, spellnum)))
#define CIRCLE_ABBR(ch, spellnum) (circle_abbrev[SPELL_CIRCLE((ch), (spellnum))])
#define SKILL_IS_TARGET(skill, tartype)                                                                                \
    ((skill) > 0 && (skill) <= TOP_SKILL_DEFINE && IS_SET(skills[skill].targets, (tartype)))

/* Function prototypes */

void init_skills(void);
void sort_skills(void);

int find_talent_num(std::string_view name, int should_restrict);
int find_skill_num(std::string_view name);
int find_spell_num(std::string_view name);
int find_chant_num(std::string_view name);
int find_song_num(std::string_view name);

const std::string_view skill_name(int num);
void improve_skill(CharData *ch, int skill);
void improve_skill_offensively(CharData *ch, CharData *victim, int skill);
void update_skills(CharData *ch);
void skill_assign(int skillnum, int class_code, int level);
void race_skill_assign(int skillnum, int race_code, int level);
int talent_type(int skill_num);
bool get_spell_assignment_circle(CharData *ch, int spell, int *circle_assignment, int *level_assignment);

extern const std::string_view talent_types[5];
extern const std::string_view targets[NUM_TAR_FLAGS + 1];
extern const std::string_view routines[NUM_ROUTINE_TYPES + 1];
extern int skill_sort_info[TOP_SKILL + 1];

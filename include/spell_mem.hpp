/***************************************************************************
 *   File: spell_mem.h                                    Part of FieryMUD *
 *  Usage: header file for spell memorization and scribing                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium.       *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University.                                       *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "defines.hpp"
#include "structs.hpp"

/* This is the scructure for spells being scribed */
struct Scribing {
    int spell;
    int scribe_time;
    int pages;
    int pages_left;
    Scribing *next;
};

extern const char *circle_abbrev[NUM_SPELL_CIRCLES + 1];
extern int spells_of_circle[(LVL_IMPL + 1)][(NUM_SPELL_CIRCLES + 1)];

/* Function prototypes */
bool has_spellbook(CharData *ch);
void rem_memming(CharData *ch);
int spell_slot_available(CharData *ch, int spell);
ObjData *find_spellbook_with_spell(CharData *ch, int spell);
int get_spellslot_restore_rate(CharData *ch);
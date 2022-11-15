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

/* This structure is for memorized spells */
struct MemorizedList {
    int spell;
    int mem_time;
    bool can_cast;
    MemorizedList *next;
};

struct SpellMemory {
    MemorizedList *list_head; /* spells in mem queue */
    MemorizedList *list_tail;
    int num_spells;                        /* number of spells in mem list */
    int num_memmed;                        /* hw many are currently memmed */
    int num_circle[NUM_SPELL_CIRCLES + 1]; /* number of spells memmed from each circle */
    int mem_status;                        /* is the PC memming now? */
};

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

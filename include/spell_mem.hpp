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

#include "sysdep.hpp"

#define CIRCLE_1 1
#define CIRCLE_2 9
#define CIRCLE_3 17
#define CIRCLE_4 25
#define CIRCLE_5 33
#define CIRCLE_6 41
#define CIRCLE_7 49
#define CIRCLE_8 57
#define CIRCLE_9 65
#define CIRCLE_10 73
#define CIRCLE_11 81
#define CIRCLE_12 89
#define CIRCLE_13 97
#define CIRCLE_14 105
#define NUM_SPELL_CIRCLES 14
extern const char *circle_abbrev[NUM_SPELL_CIRCLES + 1];

/* This structure is for memorized spells */
struct mem_list {
    int spell;
    int mem_time;
    bool can_cast;
    struct mem_list *next;
};

struct spell_memory {
    struct mem_list *list_head; /* spells in mem queue */
    struct mem_list *list_tail;
    int num_spells;                        /* number of spells in mem list */
    int num_memmed;                        /* hw many are currently memmed */
    int num_circle[NUM_SPELL_CIRCLES + 1]; /* number of spells memmed from each circle */
    int mem_status;                        /* is the PC memming now? */
};

/* This is the scructure for spells being scribed */
struct scribing {
    int spell;
    int scribe_time;
    int pages;
    int pages_left;
    struct scribing *next;
};
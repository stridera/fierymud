/***************************************************************************
 *  File: trophy.h                                        Part of FieryMUD *
 *  Usage: header file for player trophy lists                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

/* Trophy kill types */
#define TROPHY_NONE 0
#define TROPHY_MOBILE 1
#define TROPHY_PLAYER 2

#define TROPHY_LENGTH 21

struct TrophyNode {
    int kill_type;    /* mobile or player */
    int id;           /* if mobile, the VNUM; player, the id */
    float amount;     /* 1.0 if you killed it alone... */
    TrophyNode *next; /* VOID when at end of list */
    TrophyNode *prev;
};

#define GET_TROPHY(ch) ((ch)->player_specials->trophy)

void init_trophy(CharData *ch);
void add_trophy(CharData *ch, int kill_type, int id, float amount);
void kill_to_trophy(CharData *vict, CharData *killer, float amount);
void load_trophy(FILE *file, CharData *ch);
void save_trophy(FILE *file, CharData *ch);
void free_trophy(CharData *ch);
void show_trophy(CharData *ch, CharData *vict);
float exp_trophy_modifier(CharData *ch, CharData *vict);
float get_trophy_kills(CharData *ch, CharData *vict);
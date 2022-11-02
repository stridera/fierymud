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

struct trophy_node {
    int kill_type;            /* mobile or player */
    int id;                   /* if mobile, the VNUM; player, the id */
    float amount;             /* 1.0 if you killed it alone... */
    struct trophy_node *next; /* VOID when at end of list */
    struct trophy_node *prev;
};

#define GET_TROPHY(ch) ((ch)->player_specials->trophy)

void init_trophy(char_data *ch);
void add_trophy(char_data *ch, int kill_type, int id, float amount);
void kill_to_trophy(char_data *vict, char_data *killer, float amount);
void load_trophy(FILE *file, char_data *ch);
void save_trophy(FILE *file, char_data *ch);
void free_trophy(char_data *ch);
void show_trophy(char_data *ch, char_data *vict);
float exp_trophy_modifier(char_data *ch, char_data *vict);
float get_trophy_kills(char_data *ch, char_data *vict);
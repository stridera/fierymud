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

#ifndef __FIERY_TROPHY_H
#define __FIERY_TROPHY_H

#include "sysdep.h"
#include "structs.h"

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

void init_trophy(struct char_data *ch);
void add_trophy(struct char_data *ch, int kill_type, int id, float amount);
void kill_to_trophy(struct char_data *vict, struct char_data *killer, float amount);
void load_trophy(FILE *file, struct char_data *ch);
void save_trophy(FILE *file, struct char_data *ch);
void free_trophy(struct char_data *ch);
void show_trophy(struct char_data *ch, struct char_data *vict);
float exp_trophy_modifier(struct char_data *ch, struct char_data *vict);
float get_trophy_kills(struct char_data *ch, struct char_data *vict);
#endif

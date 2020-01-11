/***************************************************************************
 *  File: act.comm.h                                      Part of FieryMUD *
 *  Usage: header file for player comm actions                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_RETAINED_COMMS_H
#define __FIERY_RETAINED_COMMS_H

#include "sysdep.h"
#include "structs.h"

#define MAX_RETAINED_COMMS 10

#define TYPE_RETAINED_TELLS 0
#define TYPE_RETAINED_GOSSIPS 1

#define GET_RETAINED_COMMS(ch) ((ch)->player_specials->comms)
#define GET_RETAINED_TELLS(ch) ((ch)->player_specials->comms->tells)
#define GET_RETAINED_GOSSIPS(ch) ((ch)->player_specials->comms->gossips)
#define GET_RETAINED_COMM_TYPE(ch, type)                                                                               \
    (type == TYPE_RETAINED_TELLS ? GET_RETAINED_TELLS(ch) : GET_RETAINED_GOSSIPS(ch))
#define SET_RETAINED_COMM_TYPE(ch, type, val)                                                                          \
    do {                                                                                                               \
        if (type == TYPE_RETAINED_TELLS)                                                                               \
            GET_RETAINED_TELLS(ch) = val;                                                                              \
        else if (type == TYPE_RETAINED_GOSSIPS)                                                                        \
            GET_RETAINED_GOSSIPS(ch) = val;                                                                            \
    } while (0)

struct comm_node {
    time_t time; /* time of msg */
    char *msg;
    struct comm_node *next;
};

struct retained_comms {
    struct comm_node *tells;
    struct comm_node *gossips;
};

void init_retained_comms(struct char_data *ch);
void add_retained_comms(struct char_data *ch, int type, char *msg);
void load_retained_comms(FILE *file, struct char_data *ch, int type);
void save_retained_comms(FILE *file, struct char_data *ch, int type);
void free_comms_node_list(struct comm_node *root);
void show_retained_comms(struct char_data *ch, struct char_data *vict, int type);

#endif /* __FIERY_RETAINED_COMMS_H */

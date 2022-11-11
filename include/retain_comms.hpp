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

#include "structs.hpp"
#include "sysdep.hpp"

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

struct CommNode {
    time_t time; /* time of msg */
    char *msg;
    CommNode *next;
};

struct RetainedComms {
    CommNode *tells;
    CommNode *gossips;
};

void init_retained_comms(CharData *ch);
void add_retained_comms(CharData *ch, int type, char *msg);
void load_retained_comms(FILE *file, CharData *ch, int type);
void save_retained_comms(FILE *file, CharData *ch, int type);
void free_comms_node_list(CommNode *root);
void show_retained_comms(CharData *ch, CharData *vict, int type);

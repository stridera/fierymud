/***************************************************************************
 *  File: exits.h                                         Part of FieryMUD *
 *  Usage: header file for room exits and doors                            *
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

/* The EX_xxxx constants are used in exit_info. */
#define EX_ISDOOR (1 << 0)    /* Exit is a door             */
#define EX_CLOSED (1 << 1)    /* The door is closed         */
#define EX_LOCKED (1 << 2)    /* The door is locked         */
#define EX_PICKPROOF (1 << 3) /* Lock can't be picked       */
#define EX_HIDDEN (1 << 4)    /* exit is hidden             */
#define EX_DESCRIPT (1 << 5)  /* Just an extra description  */

struct Exit {
    char *general_description; /* When look DIR.                     */
    char *keyword;             /* for open/close                     */
    int exit_info;             /* Exit info                          */
    obj_num key;               /* Key's vnum (-1 for no key)         */
    room_num to_room;          /* Where it leads (real number)       */
};

extern const char *cmd_door[];

#define EXIT_IS_DOOR(e) ((e)->exit_info & EX_ISDOOR)
#define EXIT_IS_CLOSED(e) ((e)->exit_info & EX_CLOSED)
#define EXIT_IS_OPEN(e) (!EXIT_IS_DOOR(e) || !((e)->exit_info & EX_CLOSED))
#define EXIT_IS_LOCKED(e) ((e)->exit_info & EX_LOCKED)
#define EXIT_DOES_LOCK(e) ((e)->key != -1)
#define EXIT_IS_PICKPROOF(e) ((e)->exit_info & EX_PICKPROOF)
#define EXIT_IS_HIDDEN(e) ((e)->exit_info & EX_HIDDEN)
#define EXIT_IS_DESCRIPTION(e) ((e)->exit_info & EX_DESCRIPT)

#define EXIT_NDEST(e) ((e)->to_room)
#define EXIT_DEST(e) (EXIT_NDEST(e) == NOWHERE ? NULL : &world[EXIT_NDEST(e)])

#define CAN_GO(ch, dir) (CH_EXIT(ch, dir) && EXIT_DEST(CH_EXIT(ch, dir)) && !EXIT_IS_CLOSED(CH_EXIT(ch, dir)))

char *exit_dest_desc(Exit *e);
Exit *create_exit(int dest_room);
bool exit_has_keyword(Exit *exit, const char *name);
Exit *opposite_exit(Exit *exit, room_num roomvnum, int dir);
const char *exit_name(Exit *exit);

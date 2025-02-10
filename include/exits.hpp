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

struct Exit {
    char *general_description; /* When look DIR.                     */
    char *keyword;             /* for open/close                     */
    ExitInfoFlags exit_info;   /* Exit info                          */
    obj_num key;               /* Key's vnum (-1 for no key)         */
    room_num to_room;          /* Where it leads (real number)       */
};

extern const char *cmd_door[];

#define EXIT_IS_DOOR(e) ((e)->exit_info.test(EX_ISDOOR))
#define EXIT_IS_CLOSED(e) ((e)->exit_info.test(EX_CLOSED))
#define EXIT_IS_OPEN(e) (!EXIT_IS_DOOR(e) || !((e)->exit_info.test(EX_CLOSED)))
#define EXIT_IS_LOCKED(e) ((e)->exit_info.test(EX_LOCKED))
#define EXIT_DOES_LOCK(e) ((e)->key != -1)
#define EXIT_IS_PICKPROOF(e) ((e)->exit_info.test(EX_PICKPROOF))
#define EXIT_IS_HIDDEN(e) ((e)->exit_info.test(EX_HIDDEN))
#define EXIT_IS_DESCRIPTION(e) ((e)->exit_info.test(EX_DESCRIPT))

#define EXIT_NDEST(e) ((e)->to_room)
#define EXIT_DEST(e) (EXIT_NDEST(e) == NOWHERE ? NULL : &world[EXIT_NDEST(e)])

#define CAN_GO(ch, dir) (CH_EXIT(ch, dir) && EXIT_DEST(CH_EXIT(ch, dir)) && !EXIT_IS_CLOSED(CH_EXIT(ch, dir)))

const char *exit_dest_desc(Exit *e);
Exit *create_exit(int dest_room);
bool exit_has_keyword(Exit *exit, const char *name);
Exit *opposite_exit(Exit *exit, room_num roomvnum, int dir);
const char *exit_name(Exit *exit);

/***************************************************************************
 *   File: house.h                                        Part of FieryMUD *
 *  Usage: Header file for handling of player houses                       *
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

#define MAX_HOUSES 100
#define MAX_GUESTS 10

#define HOUSE_PRIVATE 0

struct house_control_rec {
    int vnum;                /* vnum of this house		*/
    int atrium;              /* vnum of atrium		*/
    int exit_num;            /* direction of house's exit	*/
    time_t built_on;         /* date this house was built	*/
    int mode;                /* mode of ownership		*/
    long owner;              /* idnum of house's owner	*/
    int num_of_guests;       /* how many guests for house	*/
    long guests[MAX_GUESTS]; /* idnums of house's guests	*/
    time_t last_payment;     /* date of last house payment   */
    long spare0;
    long spare1;
    long spare2;
    long spare3;
    long spare4;
    long spare5;
    long spare6;
    long spare7;
};

#define TOROOM(room, dir) (world[room].exits[dir] ? world[room].exits[dir]->to_room : NOWHERE)

void House_listrent(char_data *ch, int vnum);
void House_boot(void);
void House_save_all(void);
int House_can_enter(char_data *ch, int house);
void House_crashsave(int vnum);

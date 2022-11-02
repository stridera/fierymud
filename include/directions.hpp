/***************************************************************************
 *  File: directions.h                                    Part of FieryMUD *
 *  Usage: header file for directions                                      *
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

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define UP 4
#define DOWN 5
#define NUM_OF_DIRS 6 /* number of directions in a room (nsewud) */

extern const char *dirs[NUM_OF_DIRS + 1];
extern const char *capdirs[NUM_OF_DIRS + 1];
extern const char *dirpreposition[NUM_OF_DIRS + 1];
extern const int rev_dir[NUM_OF_DIRS];

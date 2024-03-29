/***************************************************************************
 *   File: directions.c                                   Part of FieryMUD *
 *  Usage: Constants for directions                                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "directions.hpp"

#include "conf.hpp"
#include "sysdep.hpp"

const char *dirs[NUM_OF_DIRS + 1] = {"north", "east", "south", "west", "up", "down", "\n"};

const char *capdirs[NUM_OF_DIRS + 1] = {"N", "E", "S", "W", "U", "D", "\n"};

const char *dirpreposition[NUM_OF_DIRS + 1] = {
    "to the north", "to the east", "to the south", "to the west", "in the ceiling", "in the floor", "\n"};

const int rev_dir[NUM_OF_DIRS] = {SOUTH, WEST, NORTH, EAST, DOWN, UP};

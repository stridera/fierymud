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

#include "interpreter.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#include <array>

constexpr std::string_view dirs[NUM_OF_DIRS + 1] = {"north", "east", "south", "west", "up", "down", "\n"};
constexpr std::string_view capdirs[NUM_OF_DIRS + 1] = {"N", "E", "S", "W", "U", "D", "\n"};
constexpr std::string_view dirpreposition[NUM_OF_DIRS + 1] = {
    "to the north", "to the east", "to the south", "to the west", "in the ceiling", "in the floor", "\n"};
constexpr int rev_dir[NUM_OF_DIRS] = {SOUTH, WEST, NORTH, EAST, DOWN, UP};

inline int parse_direction(std::string_view dir) {
    for (int i = 0; i < NUM_OF_DIRS; i++) {
        if (matches_start(dir, dirs[i])) {
            return i;
        }
    }
    return -1;
}

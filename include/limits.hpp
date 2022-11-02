/***************************************************************************
 *  File: limits.h                                        Part of FieryMUD *
 *  Usage: header file for regen and decompositions                        *
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

/* Thirst is relative to ounces of water. */
#define MAX_THIRST 48
#define HOURLY_THIRST_CHANGE 2

/* gain_exp modes */
#define GAIN_REGULAR (0)
#define GAIN_IGNORE_LEVEL_BOUNDARY (1 << 0)
#define GAIN_IGNORE_MORTAL_BOUNDARY (1 << 1)
#define GAIN_IGNORE_LOCATION (1 << 2)
#define GAIN_IGNORE_NAME_BOUNDARY (1 << 3)
#define GAIN_IGNORE_CHUNK_LIMITS (1 << 4)
#define GAIN_IGNORE_ALL ((1 << 5) - 1)

int mana_gain(char_data *ch);
int hit_gain(char_data *ch);
int move_gain(char_data *ch);
void set_title(char_data *ch, char *title);
void gain_exp(char_data *ch, long gain, unsigned int mode);
void gain_condition(char_data *ch, int condition, int value);
void check_idling(char_data *ch);
void point_update(void);
void start_decomposing(obj_data *obj);
void stop_decomposing(obj_data *obj);

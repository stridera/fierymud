/***************************************************************************
 *  File: charsize.h                                      Part of FieryMUD *
 *  Usage: header file for character sizes                                 *
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

struct sizedef {
    const char *name;
    const char *color;
    int weight_min;
    int weight_max;
    int height_min;
    int height_max;
};

extern struct sizedef sizes[NUM_SIZES];

#define VALID_SIZENUM(num) ((num) >= 0 && (num) < NUM_SIZES)
#define VALID_SIZE(ch) (VALID_SIZENUM(GET_SIZE(ch)))
#define SIZENAME(i) (sizes[i].name)
#define SIZE_DESC(ch) (VALID_SIZE(ch) ? sizes[GET_SIZE(ch)].name : "<INVALID SIZE>")
#define SIZE_COLOR(ch) (VALID_SIZE(ch) ? sizes[GET_SIZE(ch)].color : "")

int parse_size(CharData *ch, char *arg);
void reset_height_weight(CharData *ch);
void set_base_size(CharData *ch, int newsize);
void change_natural_size(CharData *ch, int newsize);
void adjust_size(CharData *ch, int delta);
void show_sizes(CharData *ch);

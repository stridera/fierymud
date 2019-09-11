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

#ifndef __FIERY_CHARSIZE_H
#define __FIERY_CHARSIZE_H

#define SIZE_UNDEFINED    -1
#define SIZE_TINY          0
#define SIZE_SMALL         1
#define SIZE_MEDIUM        2
#define SIZE_LARGE         3
#define SIZE_HUGE          4
#define SIZE_GIANT         5
#define SIZE_GARGANTUAN    6
#define SIZE_COLOSSAL      7
#define SIZE_TITANIC       8
#define SIZE_MOUNTAINOUS   9
#define NUM_SIZES         10

struct sizedef {
   char *name;
   char *color;
   int weight_min;
   int weight_max;
   int height_min;
   int height_max;
};
extern struct sizedef sizes[];

#define VALID_SIZENUM(num) ((num) >= 0 && (num) < NUM_SIZES)
#define VALID_SIZE(ch) (VALID_SIZENUM(GET_SIZE(ch)))
#define SIZENAME(i) (sizes[i].name)
#define SIZE_DESC(ch) \
   (VALID_SIZE(ch) ? sizes[GET_SIZE(ch)].name : "<INVALID SIZE>")
#define SIZE_COLOR(ch) \
   (VALID_SIZE(ch) ? sizes[GET_SIZE(ch)].color : "")

extern int parse_size(struct char_data *ch, char *arg);
extern void reset_height_weight(struct char_data *ch);
extern void set_base_size(struct char_data *ch, int newsize);
extern void change_natural_size(struct char_data *ch, int newsize);
extern void adjust_size(struct char_data *ch, int delta);
extern void show_sizes(struct char_data *ch);

#endif

/***************************************************************************
 * $Log: charsize.h,v $
 * Revision 1.1  2009/03/08 21:42:55  jps
 * Initial revision
 *
 ***************************************************************************/

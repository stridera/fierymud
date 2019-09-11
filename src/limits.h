/***************************************************************************
 * $Id: limits.h,v 1.3 2009/06/09 19:33:50 myc Exp $
 ***************************************************************************/
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

#ifndef __FIERY_LIMITS_H
#define __FIERY_LIMITS_H

/* Thirst is relative to ounces of water. */
#define MAX_THIRST 48
#define HOURLY_THIRST_CHANGE 2

/* gain_exp modes */
#define GAIN_REGULAR                          (0)
#define GAIN_IGNORE_LEVEL_BOUNDARY       (1 << 0)
#define GAIN_IGNORE_MORTAL_BOUNDARY      (1 << 1)
#define GAIN_IGNORE_LOCATION             (1 << 2)
#define GAIN_IGNORE_NAME_BOUNDARY        (1 << 3)
#define GAIN_IGNORE_CHUNK_LIMITS         (1 << 4)
#define GAIN_IGNORE_ALL                 ((1 << 5) - 1)

int mana_gain(struct char_data *ch);
int hit_gain(struct char_data *ch);
int move_gain(struct char_data *ch);
void set_title(struct char_data *ch, char *title);
void gain_exp(struct char_data *ch, long gain, unsigned int mode);
void gain_condition(struct char_data *ch, int condition, int value);
void check_idling(struct char_data *ch);
void point_update(void);
void start_decomposing(struct obj_data *obj);
void stop_decomposing(struct obj_data *obj);

#endif

/***************************************************************************
 * $Log: limits.h,v $
 * Revision 1.3  2009/06/09 19:33:50  myc
 * Rewrote gain_exp and retired gain_exp_regardless.
 *
 * Revision 1.2  2008/09/29 03:24:44  jps
 * Make container weight automatic. Move some liquid container functions to objects.c.
 *
 * Revision 1.1  2008/09/02 06:50:50  jps
 * Initial revision
 *
 ***************************************************************************/

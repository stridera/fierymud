/***************************************************************************
 * $Id: movement.h,v 1.4 2008/09/14 01:47:41 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: movement.h                                      Part of FieryMUD *
 *  Usage: header file for movement functions                              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_MOVEMENT_H
#define __FIERY_MOVEMENT_H

#include "sysdep.h"
#include "structs.h"

/* MISCELLANEOUS */
bool can_travel_on_water(struct char_data *ch);
/*(this looks like the start of a flying section...)*/
bool too_heavy_to_fly(struct char_data *ch);
void start_char_falling(struct char_data *ch);
void start_obj_falling(struct obj_data *obj);
void falling_yell(struct char_data *ch);
void gravity_assisted_landing(struct char_data *ch, int distance_fallen);
void falling_check(struct char_data *ch);

/* FOLLOWING */
void stop_follower(struct char_data *ch, int violent);
void die_follower(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);

/* GROUPING */
void add_groupee(struct char_data *master, struct char_data *groupee);
void disband_group(struct char_data *master, bool verbose, bool forceful);
void ungroup(struct char_data *ch, bool verbose, bool forceful);
bool is_grouped(struct char_data *ch, struct char_data *tch);
bool battling_my_group(struct char_data *ch, struct char_data *tch);

/* MOUNTS */
void mount_char(struct char_data *ch, struct char_data *mount);
void dismount_char(struct char_data *ch);
int ideal_mountlevel(struct char_data *ch);
int ideal_ridelevel(struct char_data *ch);
int ideal_tamelevel(struct char_data *ch);
int mountlevel(struct char_data *ch);
int movement_bucked(struct char_data *ch, struct char_data *mount);
int mount_bucked(struct char_data *ch, struct char_data *mount);
int mount_fall(struct char_data *ch, struct char_data *mount);
void mount_warning(struct char_data *ch, struct char_data *vict);
void mount_pos_check(struct char_data *mount);

/* in act.movmement.c */
bool do_simple_move(struct char_data *ch, int dir, int following);
bool perform_move(struct char_data *ch, int dir, int following, bool misdirection);

#endif

/***************************************************************************
 * $Log: movement.h,v $
 * Revision 1.4  2008/09/14 01:47:41  jps
 * Added function battling_my_group().
 *
 * Revision 1.3  2008/09/13 17:21:59  jps
 * Added mount_pos_check, in which a rider may fall off if the mount changes
 *position.
 *
 * Revision 1.2  2008/09/07 01:28:34  jps
 * Add prototypes
 *
 * Revision 1.1  2008/09/01 23:47:35  jps
 * Initial revision
 *
 ***************************************************************************/

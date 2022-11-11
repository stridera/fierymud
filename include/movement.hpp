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

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

/* MISCELLANEOUS */
bool can_travel_on_water(CharData *ch);
/*(this looks like the start of a flying section...)*/
bool too_heavy_to_fly(CharData *ch);
void start_char_falling(CharData *ch);
void start_obj_falling(ObjData *obj);
void falling_yell(CharData *ch);
void gravity_assisted_landing(CharData *ch, int distance_fallen);
void falling_check(CharData *ch);

/* FOLLOWING */
void stop_follower(CharData *ch, int violent);
void die_follower(CharData *ch);
void add_follower(CharData *ch, CharData *leader);

/* GROUPING */
void add_groupee(CharData *master, CharData *groupee);
void disband_group(CharData *master, bool verbose, bool forceful);
void ungroup(CharData *ch, bool verbose, bool forceful);
bool is_grouped(CharData *ch, CharData *tch);
bool battling_my_group(CharData *ch, CharData *tch);

/* MOUNTS */
void mount_char(CharData *ch, CharData *mount);
void dismount_char(CharData *ch);
int ideal_mountlevel(CharData *ch);
int ideal_ridelevel(CharData *ch);
int ideal_tamelevel(CharData *ch);
int mountlevel(CharData *ch);
int movement_bucked(CharData *ch, CharData *mount);
int mount_bucked(CharData *ch, CharData *mount);
int mount_fall(CharData *ch, CharData *mount);
void mount_warning(CharData *ch, CharData *vict);
void mount_pos_check(CharData *mount);

/* in act.movmement.c */
bool do_simple_move(CharData *ch, int dir, int following);
bool perform_move(CharData *ch, int dir, int following, bool misdirection);

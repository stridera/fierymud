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
bool can_travel_on_water(char_data *ch);
/*(this looks like the start of a flying section...)*/
bool too_heavy_to_fly(char_data *ch);
void start_char_falling(char_data *ch);
void start_obj_falling(obj_data *obj);
void falling_yell(char_data *ch);
void gravity_assisted_landing(char_data *ch, int distance_fallen);
void falling_check(char_data *ch);

/* FOLLOWING */
void stop_follower(char_data *ch, int violent);
void die_follower(char_data *ch);
void add_follower(char_data *ch, char_data *leader);

/* GROUPING */
void add_groupee(char_data *master, char_data *groupee);
void disband_group(char_data *master, bool verbose, bool forceful);
void ungroup(char_data *ch, bool verbose, bool forceful);
bool is_grouped(char_data *ch, char_data *tch);
bool battling_my_group(char_data *ch, char_data *tch);

/* MOUNTS */
void mount_char(char_data *ch, char_data *mount);
void dismount_char(char_data *ch);
int ideal_mountlevel(char_data *ch);
int ideal_ridelevel(char_data *ch);
int ideal_tamelevel(char_data *ch);
int mountlevel(char_data *ch);
int movement_bucked(char_data *ch, char_data *mount);
int mount_bucked(char_data *ch, char_data *mount);
int mount_fall(char_data *ch, char_data *mount);
void mount_warning(char_data *ch, char_data *vict);
void mount_pos_check(char_data *mount);

/* in act.movmement.c */
bool do_simple_move(char_data *ch, int dir, int following);
bool perform_move(char_data *ch, int dir, int following, bool misdirection);

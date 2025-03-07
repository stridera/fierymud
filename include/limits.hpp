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

int mana_gain(CharData *ch);
int hit_gain(CharData *ch);
int move_gain(CharData *ch);
void spell_slot_restore_tick(CharData *ch);
void set_title(CharData *ch, std::string_view title);
void gain_exp(CharData *ch, long gain, unsigned int mode);
void gain_condition(CharData *ch, int condition, int value);
void check_idling(CharData *ch);
void point_update(void);
void start_decomposing(ObjData *obj);
void stop_decomposing(ObjData *obj);
void sick_update(void);

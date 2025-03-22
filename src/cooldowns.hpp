/***************************************************************************
 *  File: cooldowns.h                                    Part of FieryMUD  *
 *  Usage: Skill cooldowns                                                 *
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

/*
 * The CD_<NAME> and MAX_COOLDOWNS constants are defined in structs.h
 * because the player structure has an array of cooldowns.
 */

#define PULSE_COOLDOWN (1 RL_SEC)
extern const char *cooldowns[NUM_COOLDOWNS + 1];

#define CD_CURRENT 0
#define CD_MAX 1

#define GET_COOLDOWN(ch, i) ((ch)->char_specials.cooldowns[(i)][CD_CURRENT])
#define GET_COOLDOWN_MAX(ch, i) ((ch)->char_specials.cooldowns[(i)][CD_MAX])
void SET_COOLDOWN(CharData *ch, int type, int amount);
void clear_cooldowns(CharData *ch);

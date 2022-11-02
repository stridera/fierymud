/***************************************************************************
 *  File: regen.h                                         Part of FieryMUD *
 *  Usage: header file for hp/mana/mv alteration                           *
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

/* void hp_check_position( char_data *ch); */
void alter_hit(char_data *ch, int amount, bool cap_amount);
void hurt_char(char_data *ch, char_data *attacker, int amount, bool cap_amount);
void alter_mana(char_data *ch, int amount);
void alter_move(char_data *ch, int amount);
void check_regen_rates(char_data *ch);

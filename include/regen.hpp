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

/* void hp_check_position( CharData *ch); */
void alter_hit(CharData *ch, int amount, bool cap_amount);
void hurt_char(CharData *ch, CharData *attacker, int amount, bool cap_amount);
void alter_mana(CharData *ch, int amount);
void alter_move(CharData *ch, int amount);
void check_regen_rates(CharData *ch);
void set_regen_event(CharData *ch, int eventtype);
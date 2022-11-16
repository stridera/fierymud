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

#ifndef __FIERY_REGEN_H
#define __FIERY_REGEN_H

#include "structs.h"
#include "sysdep.h"

/* void hp_check_position(struct char_data *ch); */
void alter_hit(struct char_data *ch, int amount, bool cap_amount);
void hurt_char(struct char_data *ch, struct char_data *attacker, int amount, bool cap_amount);
void alter_mana(struct char_data *ch, int amount);
void alter_move(struct char_data *ch, int amount);
void check_regen_rates(struct char_data *ch);

#endif
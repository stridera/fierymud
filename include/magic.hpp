/***************************************************************************
 *  File: spells.h                                        Part of FieryMUD *
 *  Usage: header file: constants and fn prototypes for magic              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium.       *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University.                                       *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

int mag_savingthrow(char_data *ch, int type);
bool evades_spell(char_data *caster, char_data *vict, int spellnum, int power);
int defensive_spell_damage(char_data *attacker, char_data *victim, int dam);
bool wall_block_check(char_data *actor, char_data *motivator, int dir);
bool wall_charge_check(char_data *ch, int dir);
void remove_char_spell(char_data *ch, int spellnum);
void remove_unsuitable_spells(char_data *ch);
bool check_fluid_spell_ok(char_data *ch, char_data *victim, int spellnum, bool quiet);

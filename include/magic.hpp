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

int mag_savingthrow(CharData *ch, int type);
bool evades_spell(CharData *caster, CharData *vict, int spellnum, int power);
int defensive_spell_damage(CharData *attacker, CharData *victim, int dam);
bool wall_block_check(CharData *actor, CharData *motivator, int dir);
bool wall_charge_check(CharData *ch, int dir);
void remove_char_spell(CharData *ch, int spellnum);
void remove_unsuitable_spells(CharData *ch);
bool check_fluid_spell_ok(CharData *ch, CharData *victim, int spellnum, bool quiet);

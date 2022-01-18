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

#ifndef __FIERY_MAGIC_H
#define __FIERY_MAGIC_H

#include "structs.h"
#include "sysdep.h"

int mag_savingthrow(struct char_data *ch, int type);
bool evades_spell(struct char_data *caster, struct char_data *vict, int spellnum, int power);
int defensive_spell_damage(struct char_data *attacker, struct char_data *victim, int dam);
bool wall_block_check(struct char_data *actor, struct char_data *motivator, int dir);
bool wall_charge_check(struct char_data *ch, int dir);
void remove_char_spell(struct char_data *ch, int spellnum);
void remove_unsuitable_spells(struct char_data *ch);
bool check_fluid_spell_ok(struct char_data *ch, struct char_data *victim, int spellnum, bool quiet);

#endif

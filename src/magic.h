/***************************************************************************
 * $Id: magic.h,v 1.6 2008/09/14 02:22:53 jps Exp $
 ***************************************************************************/
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

int mag_savingthrow(struct char_data *ch, int type);
bool evades_spell(struct char_data *caster, struct char_data *vict, int spellnum, int power);
int defensive_spell_damage(struct char_data *attacker, struct char_data *victim, int dam);
bool wall_block_check(struct char_data *actor, struct char_data *motivator, int dir);
bool wall_charge_check(struct char_data *ch, int dir);
void remove_char_spell(struct char_data *ch, int spellnum);
void remove_unsuitable_spells(struct char_data *ch);
bool check_fluid_spell_ok(struct char_data *ch, struct char_data *victim, int spellnum, bool quiet);

#endif

/***************************************************************************
 * $Log: magic.h,v $
 * Revision 1.6  2008/09/14 02:22:53  jps
 * Expand the suitability-for-fluid-chars check to be used for all mag_effect spells.
 *
 * Revision 1.5  2008/09/13 18:05:29  jps
 * Added functions to remove spells from characters when necessary.
 *
 * Revision 1.4  2008/08/17 20:24:12  jps
 * Add protos for wall_block_check and wall_charge_check.
 *
 * Revision 1.3  2008/05/11 06:13:31  jps
 * Adding defensive_spell_damage(), which calculates damage to attackers
 * due to spells like fireshield, and sends messages.
 *
 * Revision 1.2  2008/04/12 21:29:25  jps
 * Adding prototype for evades_spell.
 *
 * Revision 1.1  2008/04/12 21:12:43  jps
 * Initial revision
 *
 ***************************************************************************/

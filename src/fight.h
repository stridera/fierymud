/***************************************************************************
 * $Id: fight.h,v 1.7 2008/09/21 21:49:59 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: fight.h                                         Part of FieryMUD *
 *  Usage: header file for aggressive stuff                                *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_FIGHT_H
#define __FIERY_FIGHT_H

#include "events.h"

extern struct attack_hit_type attack_hit_text[];

/* General target linking */
void set_battling(struct char_data *ch, struct char_data *target);
void stop_battling(struct char_data *ch);
void stop_attackers(struct char_data *ch);
void set_fighting(struct char_data *ch, struct char_data *vict, bool reciprocate);
void stop_merciful_attackers(struct char_data *ch);
#define stop_fighting stop_battling
void transfer_battle(struct char_data *ch, struct char_data *tch);

void check_killer(struct char_data *ch, struct char_data *vict);
bool check_disarmed(struct char_data *ch);
bool is_aggr_to(struct char_data *ch, struct char_data *target);
bool sling_weapon(struct char_data *ch, int position);
int weapon_proficiency(struct obj_data *weapon, int position);
int calc_thac0(int level, int thac0_01, int thac0_00);
EVENTFUNC(quick_aggro_event);
void load_messages(void);
void free_messages(void);
void set_fighting(struct char_data *ch, struct char_data *vict, bool reciprocate);
void death_cry(struct char_data *ch);
void perform_die(struct char_data *ch, struct char_data *killer);
void die(struct char_data *ch, struct char_data *killer);
bool skill_message(int dam, struct char_data *ch, struct char_data *vict,
      int attacktype, bool death);
int damage(struct char_data *ch, struct char_data *victim, int dam,
      int attacktype);
void hit(struct char_data *ch, struct char_data *victim, int type);
void perform_violence(void);
void pickup_dropped_weapon(struct char_data *ch);
int blessed_blow(struct char_data *ch, struct obj_data *weapon);
bool area_attack_target(struct char_data *ch, struct char_data *tch);
bool attack_ok(struct char_data *ch, struct char_data *victim, bool verbose);
bool mass_attack_ok(struct char_data *ch, struct char_data *victim, bool verbose);
void stop_fighting(struct char_data *ch);
void hit(struct char_data *ch, struct char_data *victim, int type);
#define attack(ch, victim)      hit(ch, victim, TYPE_UNDEFINED)
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype);
bool skill_message(int dam, struct char_data *ch, struct char_data *vict,
      int attacktype, bool death);
void die(struct char_data *ch, struct char_data *killer);


#endif

/***************************************************************************
 * $Log: fight.h,v $
 * Revision 1.7  2008/09/21 21:49:59  jps
 * Added transfer_battle(), which allows one character to seamlessly
 * take the place of another in a battle.
 *
 * Revision 1.6  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.5  2008/09/20 07:26:33  jps
 * Moved protos from handler.h to fight.h.
 *
 * Revision 1.4  2008/09/14 02:08:01  jps
 * Use standardized area attack targetting
 *
 * Revision 1.3  2008/09/13 18:52:00  jps
 * Removing prototype found in ai.h.
 *
 * Revision 1.2  2008/08/24 19:29:11  jps
 * Apply damage susceptibility reductions to the various physical attack skills.
 *
 * Revision 1.1  2008/05/18 20:16:11  jps
 * Initial revision
 *
 ***************************************************************************/

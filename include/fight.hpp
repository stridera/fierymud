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

#pragma once

#include "events.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/* Weapon attack texts */

struct AttackHitType {
    const std::string_view singular;
    const std::string_view plural;
};
extern AttackHitType attack_hit_text[];

/* General target linking */
void set_battling(CharData *ch, CharData *target);
void stop_battling(CharData *ch);
void stop_attackers(CharData *ch);
void set_fighting(CharData *ch, CharData *vict, bool reciprocate);
void stop_merciful_attackers(CharData *ch);
#define stop_fighting stop_battling
void transfer_battle(CharData *ch, CharData *tch);

void check_killer(CharData *ch, CharData *vict);
bool check_disarmed(CharData *ch);
bool is_aggr_to(CharData *ch, CharData *target);
bool sling_weapon(CharData *ch, int position);
int weapon_proficiency(ObjData *weapon, int position);
int calc_thac0(int level, int thac0_01, int thac0_00);
EVENTFUNC(quick_aggro_event);
void load_messages(void);
void free_messages(void);
void death_cry(CharData *ch);
void perform_die(CharData *ch, CharData *killer);
void die(CharData *ch, CharData *killer);
bool skill_message(int dam, CharData *ch, CharData *vict, int attacktype, bool death);
int damage(CharData *ch, CharData *victim, int dam, int attacktype);
void hit(CharData *ch, CharData *victim, int type);
void perform_violence(void);
void pickup_dropped_weapon(CharData *ch);
int blessed_blow(CharData *ch, ObjData *weapon);
bool area_attack_target(CharData *ch, CharData *tch);
bool attack_ok(CharData *ch, CharData *victim, bool verbose);
bool mass_attack_ok(CharData *ch, CharData *victim, bool verbose);
void stop_fighting(CharData *ch);
#define attack(ch, victim) hit(ch, victim, TYPE_UNDEFINED)
bool displaced(CharData *ch, CharData *victim);

/* Structures */
extern CharData *combat_list;
extern CharData *next_combat_list;
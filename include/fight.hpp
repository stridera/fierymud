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

// extern struct attack_hit_type attack_hit_text[];

/* General target linking */
void set_battling(char_data *ch, char_data *target);
void stop_battling(char_data *ch);
void stop_attackers(char_data *ch);
void set_fighting(char_data *ch, char_data *vict, bool reciprocate);
void stop_merciful_attackers(char_data *ch);
#define stop_fighting stop_battling
void transfer_battle(char_data *ch, char_data *tch);

void check_killer(char_data *ch, char_data *vict);
bool check_disarmed(char_data *ch);
bool is_aggr_to(char_data *ch, char_data *target);
bool sling_weapon(char_data *ch, int position);
int weapon_proficiency(obj_data *weapon, int position);
int calc_thac0(int level, int thac0_01, int thac0_00);
EVENTFUNC(quick_aggro_event);
void load_messages(void);
void free_messages(void);
void death_cry(char_data *ch);
void perform_die(char_data *ch, char_data *killer);
void die(char_data *ch, char_data *killer);
bool skill_message(int dam, char_data *ch, char_data *vict, int attacktype, bool death);
int damage(char_data *ch, char_data *victim, int dam, int attacktype);
void hit(char_data *ch, char_data *victim, int type);
void perform_violence(void);
void pickup_dropped_weapon(char_data *ch);
int blessed_blow(char_data *ch, obj_data *weapon);
bool area_attack_target(char_data *ch, char_data *tch);
bool attack_ok(char_data *ch, char_data *victim, bool verbose);
bool mass_attack_ok(char_data *ch, char_data *victim, bool verbose);
void stop_fighting(char_data *ch);
#define attack(ch, victim) hit(ch, victim, TYPE_UNDEFINED)

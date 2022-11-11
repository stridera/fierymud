/***************************************************************************
 *   File: ai.h                                           Part of FieryMUD *
 *  Usage: To be used with other mob AI files, all casting variables are   *
 *         within this file and can be adjusted as needed.                 *
 * Author: Ben Horner (Proky of HubisMUD)                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define MOB_ASSISTER(ch)                                                                                               \
    (!MOB_FLAGGED((ch), MOB_PEACEFUL) &&                                                                               \
     (MOB_FLAGGED((ch), MOB_HELPER) || MOB_FLAGGED((ch), MOB_PROTECTOR) || MOB_FLAGGED((ch), MOB_PEACEKEEPER)))

/* Function prototypes in ai_utils.c */
bool try_cast(CharData *ch, CharData *victim, int spellnum);
bool evil_in_group(CharData *victim);
bool good_in_group(CharData *victim);
int group_size(CharData *ch);
bool is_tanking(CharData *ch);
bool mob_heal_up(CharData *ch);
CharData *weakest_attacker(CharData *ch, CharData *victim);
bool has_effect(CharData *ch, const SpellPair *effect);
bool check_weapon_value(CharData *ch, ObjData *obj);
void mob_remove_weapon(CharData *ch, ObjData *obj, int where);
void perform_remove(CharData *ch, int pos);
int appraise_item(CharData *ch, ObjData *obj);
bool will_assist(CharData *ch, CharData *vict);
CharData *find_aggr_target(CharData *ch);
void glorion_distraction(CharData *ch, CharData *glorion);
int appraise_opponent(CharData *ch, CharData *vict);
bool is_aggr_to(CharData *ch, CharData *tch);

/* Class AI functions */
bool sorcerer_ai_action(CharData *ch, CharData *victim);
bool cleric_ai_action(CharData *ch, CharData *victim);
bool rogue_ai_action(CharData *ch, CharData *victim);
bool warrior_ai_action(CharData *ch, CharData *victim);
bool bard_ai_action(CharData *ch, CharData *victim);
bool mob_steal(CharData *ch);
bool mob_animate(CharData *ch);

/* Function prototypes in mobact.c */
bool check_sorcerer_status(CharData *ch);
bool check_cleric_status(CharData *ch);
bool check_bard_status(CharData *ch);
bool dragonlike_attack(CharData *ch);
bool in_memory(CharData *ch, CharData *vict);

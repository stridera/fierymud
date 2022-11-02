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

struct spell_pair {
    int spell;
    int remover;
    int flag;
};

#define MOB_ASSISTER(ch)                                                                                               \
    (!MOB_FLAGGED((ch), MOB_PEACEFUL) &&                                                                               \
     (MOB_FLAGGED((ch), MOB_HELPER) || MOB_FLAGGED((ch), MOB_PROTECTOR) || MOB_FLAGGED((ch), MOB_PEACEKEEPER)))

/* Function prototypes in ai_utils.c */
bool try_cast(char_data *ch, char_data *victim, int spellnum);
bool evil_in_group(char_data *victim);
bool good_in_group(char_data *victim);
int group_size(char_data *ch);
bool is_tanking(char_data *ch);
bool mob_heal_up(char_data *ch);
char_data *weakest_attacker(char_data *ch, char_data *victim);
bool has_effect(char_data *ch, const spell_pair *effect);
bool check_weapon_value(char_data *ch, obj_data *obj);
void mob_remove_weapon(char_data *ch, obj_data *obj, int where);
void perform_remove(char_data *ch, int pos);
int appraise_item(char_data *ch, obj_data *obj);
bool will_assist(char_data *ch, char_data *vict);
char_data *find_aggr_target(char_data *ch);
void glorion_distraction(char_data *ch, char_data *glorion);
int appraise_opponent(char_data *ch, char_data *vict);
bool is_aggr_to(char_data *ch, char_data *tch);

/* Class AI functions */
bool sorcerer_ai_action(char_data *ch, char_data *victim);
bool cleric_ai_action(char_data *ch, char_data *victim);
bool rogue_ai_action(char_data *ch, char_data *victim);
bool warrior_ai_action(char_data *ch, char_data *victim);
bool bard_ai_action(char_data *ch, char_data *victim);
bool mob_steal(char_data *ch);
bool mob_animate(char_data *ch);

/* Function prototypes in mobact.c */
bool check_sorcerer_status(char_data *ch);
bool check_cleric_status(char_data *ch);
bool check_bard_status(char_data *ch);
bool dragonlike_attack(char_data *ch);
bool in_memory(char_data *ch, char_data *vict);

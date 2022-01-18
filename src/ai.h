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

#ifndef __FIERY_AI_H
#define __FIERY_AI_H

#include "skills.h"
#include "structs.h"
#include "sysdep.h"

struct spell_pair {
    int spell;
    int remover;
    int flag;
};

#define MOB_ASSISTER(ch)                                                                                               \
    (!MOB_FLAGGED((ch), MOB_PEACEFUL) &&                                                                               \
     (MOB_FLAGGED((ch), MOB_HELPER) || MOB_FLAGGED((ch), MOB_PROTECTOR) || MOB_FLAGGED((ch), MOB_PEACEKEEPER)))

/* Function prototypes in ai_utils.c */
bool try_cast(struct char_data *ch, struct char_data *victim, int spellnum);
bool evil_in_group(struct char_data *victim);
bool good_in_group(struct char_data *victim);
int group_size(struct char_data *ch);
bool is_tanking(struct char_data *ch);
bool mob_heal_up(struct char_data *ch);
struct char_data *weakest_attacker(struct char_data *ch, struct char_data *victim);
bool has_effect(struct char_data *ch, const struct spell_pair *effect);
bool check_weapon_value(struct char_data *ch, struct obj_data *obj);
void mob_remove_weapon(struct char_data *ch, struct obj_data *obj, int where);
void perform_remove(struct char_data *ch, int pos);
int appraise_item(struct char_data *ch, struct obj_data *obj);
bool will_assist(struct char_data *ch, struct char_data *vict);
struct char_data *find_aggr_target(struct char_data *ch);
void glorion_distraction(struct char_data *ch, struct char_data *glorion);
int appraise_opponent(struct char_data *ch, struct char_data *vict);
bool is_aggr_to(struct char_data *ch, struct char_data *tch);

/* Class AI functions */
bool sorcerer_ai_action(struct char_data *ch, struct char_data *victim);
bool cleric_ai_action(struct char_data *ch, struct char_data *victim);
bool rogue_ai_action(struct char_data *ch, struct char_data *victim);
bool warrior_ai_action(struct char_data *ch, struct char_data *victim);
bool mob_steal(struct char_data *ch);
bool mob_animate(struct char_data *ch);

/* Function prototypes in mobact.c */
bool check_sorcerer_status(struct char_data *ch);
bool check_cleric_status(struct char_data *ch);
bool dragonlike_attack(struct char_data *ch);
bool in_memory(struct char_data *ch, struct char_data *vict);

#endif /* __FIERY_AI_H */

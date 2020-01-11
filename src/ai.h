/***************************************************************************
 * $Id: ai.h,v 1.16 2008/08/31 20:54:28 jps Exp $
 ***************************************************************************/
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

/***************************************************************************
 * $Log: ai.h,v $
 * Revision 1.16  2008/08/31 20:54:28  jps
 * Moved find_aggr_target, glorion_distraction, appraise_opponent, and
 *is_aggr_to to ai_utils.c from fight.c. Added will_assist. Incorporated the
 *PROTECTOR and PEACEKEEPER flags.
 *
 * Revision 1.15  2008/03/29 17:34:31  myc
 * No need for AGGR_TO_ALIGN.
 *
 * Revision 1.14  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.13  2008/02/23 01:03:54  myc
 * Getting rid of a duplicate macro.
 *
 * Revision 1.12  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.11  2008/01/13 23:06:04  myc
 * Added the appraise_item function.
 *
 * Revision 1.10  2008/01/12 23:13:20  myc
 * Cleaned up mobact.c some more and added some new mob actions.
 *
 * Revision 1.9  2008/01/12 19:08:14  myc
 * Rewrote a lot of mob AI functionality.
 *
 * Revision 1.8  2006/11/17 22:52:59  jps
 * Change AGGR_GOOD/EVIL_ALIGN to AGGR_GOOD/EVIL_RACE
 *
 * Revision 1.7  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.6  2000/11/20 05:29:49  rsd
 * Added back rlog comments from prior to the addition
 * of the $log$ string.
 *
 * Revision 1.5  2000/09/22 23:44:16  rsd
 * Altered the comment header to indicate that this is
 * fierymud code now.
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/01/30 17:26:15  mud
 * Changed comment header to make sure proper credit was given
 * By: Ben Horner (Proky of HubisMUD)
 *
 * Revision 1.2  1999/01/30 17:11:54  mud
 * Indented entire file
 * Added a standard looking comment header at the
 * beginning of the file.
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

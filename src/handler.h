/***************************************************************************
 * $Id: handler.h,v 1.31 2009/03/09 20:36:00 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: handler.h                                      Part of FieryMUD *
 *  Usage: header file: prototypes of handling and utility functions       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_HANDLER_H
#define __FIERY_HANDLER_H

/* Prototypes for finding objs/chars */
#include "find.h"

/* handling the affected-structures */
void effect_total(struct char_data *ch);
void do_campout(struct char_data * ch);
void effect_modify(struct char_data *ch, byte loc, sh_int mod, flagvector bitv[], bool add);
void effect_to_char(struct char_data *ch, struct effect *eff);
void effect_remove(struct char_data *ch, struct effect *eff);
void active_effect_remove(struct char_data *ch, struct effect *effect);
void effect_from_char(struct char_data *ch, int type);
void active_effect_from_char(struct char_data *ch, int type);
bool affected_by_spell(struct char_data *ch, int type);
void effect_join(struct char_data *ch, struct effect *eff,
bool add_dur, bool avg_dur, bool add_mod, bool avg_mod, bool refresh);


/* utility */
 int	isname(const char *str, const char *namelist);
char	*fname(const char *namelist);

/* ******** objects *********** */

void	obj_to_char(struct obj_data *object, struct char_data *ch);
void	obj_from_char(struct obj_data *object);

enum equip_result {
   EQUIP_RESULT_ERROR,   /* Internal error; object has not been manipulated */
   EQUIP_RESULT_REFUSE,  /* Not equipped for gameplay reason; object is returned to inventory */
   EQUIP_RESULT_SUCCESS  /* Object was equipped */
};
void count_hand_eq(struct char_data *ch, int *hands_used, int *weapon_hands_used);
bool may_wear_eq(struct char_data * ch, struct obj_data * obj,
   int *where, bool sendmessage);
enum equip_result equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);

void	obj_to_room(struct obj_data *object, int room);
void	obj_from_room(struct obj_data *object);
void	obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void	obj_from_obj(struct obj_data *obj);
void	object_list_new_owner(struct obj_data *list, struct char_data *ch);

void	extract_obj(struct obj_data *obj);

/* ******* characters ********* */

void init_char(struct char_data *ch);
void update_char(struct char_data *ch);

void	char_from_room(struct char_data *ch);
void	char_to_room(struct char_data *ch, int room);
void	extract_char(struct char_data *ch);
/* Buru 13/12/97 - coin converters */
void	money_convert(struct char_data * ch, int amount);
void	copper_to_coins(struct char_data * ch);
void	convert_coins_copper(struct char_data * ch);

/* mobact.c */
void forget(struct char_data *ch, struct char_data *victim);
void remember(struct char_data *ch, struct char_data *victim);

#endif

/***************************************************************************
 * $Log: handler.h,v $
 * Revision 1.31  2009/03/09 20:36:00  myc
 * Moved money functions from here to money.c.
 *
 * Revision 1.30  2009/03/07 22:28:59  jps
 * Add function active_effect_from_char, which is called to remove effects
 * in-game and provide feedback.
 *
 * Revision 1.29  2009/03/03 19:43:44  myc
 * Split off target-finding protocol from handler into find.c.
 *
 * Revision 1.28  2008/09/20 07:30:49  jps
 * Removed unused Crash_* protos. Moved fight.c's protos to fight.h.
 *
 * Revision 1.27  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.26  2008/08/31 01:19:54  jps
 * You screwed up holding items in slots with two of the same position!
 * Fix.
 *
 * Revision 1.25  2008/08/30 20:25:38  jps
 * Moved count_hand_eq() into handler.c and mentioned it in handler.h.
 *
 * Revision 1.24  2008/08/30 20:21:07  jps
 * Moved equipment-wearability checks into function may_wear_eq() and moved
 * it to handler.c.
 *
 * Revision 1.23  2008/08/17 06:52:40  jps
 * equip_char will return one of several result codes.
 *
 * Revision 1.22  2008/05/26 18:24:48  jps
 * Removed code that deletes player object files.
 *
 * Revision 1.21  2008/05/12 00:43:00  jps
 * Pass "death" to skill_message since it wouldn't otherwise know
 * whether the victim died due to the attack.
 *
 * Revision 1.20  2008/05/11 07:10:46  jps
 * Moved active_effect_remove from spells.c to handler.c.
 *
 * Revision 1.19  2008/05/11 05:41:03  jps
 * Moved relevant prototypes to regen.h.
 *
 * Revision 1.18  2008/04/02 03:24:44  myc
 * Declaring die as a public function.
 *
 * Revision 1.17  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.16  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.15  2008/01/25 21:05:45  myc
 * Added attack() as a macro alias for hit() with fewer arguments.
 *
 * Revision 1.14  2008/01/12 23:13:20  myc
 * Added multi-purpose get_random_char_around function to pick random chars
 * in a room.
 *
 * Revision 1.13  2008/01/10 05:39:43  myc
 * damage() now returns the amount of damage it caused.  Negative values
 * indicate healing, and a return value of VICTIM_DEAD indicates that the
 * victim is dead.
 *
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.12  2008/01/05 05:35:58  jps
 * Exporting functions init_char() and update_char().
 *
 * Revision 1.11  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  Fixed a lot of affect-handling
 * code to work more smoothly (and sanely).
 *
 * Revision 1.10  2007/09/03 23:49:40  jps
 * Add mass_attack_ok() so that you could kill your own pet specifically,
 * but your area spells will not harm it.
 *
 * Revision 1.9  2007/08/26 21:10:17  jps
 * Change return type of skill_message() to bool.
 *
 * Revision 1.8  2007/08/03 03:51:44  myc
 * check_pk is now attack_ok, and covers many more cases than before,
 * including peaced rooms, shapeshifted pk, and arena rooms.  Almost all
 * offensive attacks now use attack_ok to determine whether an attack is
 * allowed.
 *
 * Revision 1.7  2007/08/02 01:08:27  myc
 * check_pk() now works for all PK cases.  Moved from magic.c to fight.c
 *
 * Revision 1.6  2007/07/14 04:17:35  jps
 * Updated call to stop_follower(), which cares whether this is being
 * done due to a violent action or not.
 *
 * Revision 1.5  2007/04/11 14:15:28  jps
 * Give money piles proper keywords and make them dissolve when stolen.
 *
 * Revision 1.4  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.3  2000/11/20 03:16:43  rsd
 * Altered comment header and added back rlog messgaes from
 * prior to the addition of the $log$ string
 *
 * Revision 1.2  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial Revision
 *
 ***************************************************************************/

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
#include "structs.h"
#include "sysdep.h"

/* handling the affected-structures */
void effect_total(struct char_data *ch);
void do_campout(struct char_data *ch);
void effect_modify(struct char_data *ch, byte loc, sh_int mod, flagvector bitv[], bool add);
void effect_to_char(struct char_data *ch, struct effect *eff);
void effect_remove(struct char_data *ch, struct effect *eff);
void active_effect_remove(struct char_data *ch, struct effect *effect);
void effect_from_char(struct char_data *ch, int type);
void active_effect_from_char(struct char_data *ch, int type);
bool affected_by_spell(struct char_data *ch, int type);
void effect_join(struct char_data *ch, struct effect *eff, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod,
                 bool refresh);

/* utility */
int isname(const char *str, const char *namelist);
char *fname(const char *namelist);

/* ******** objects *********** */

void obj_to_char(struct obj_data *object, struct char_data *ch);
void obj_from_char(struct obj_data *object);

enum equip_result {
    EQUIP_RESULT_ERROR,  /* Internal error; object has not been manipulated */
    EQUIP_RESULT_REFUSE, /* Not equipped for gameplay reason; object is returned
                            to inventory */
    EQUIP_RESULT_SUCCESS /* Object was equipped */
};
void count_hand_eq(struct char_data *ch, int *hands_used, int *weapon_hands_used);
bool may_wear_eq(struct char_data *ch, struct obj_data *obj, int *where, bool sendmessage);
enum equip_result equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);

void obj_to_room(struct obj_data *object, int room);
void obj_from_room(struct obj_data *object);
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void obj_from_obj(struct obj_data *obj);
void object_list_new_owner(struct obj_data *list, struct char_data *ch);

void extract_obj(struct obj_data *obj);

/* ******* characters ********* */

void init_char(struct char_data *ch);
void update_char(struct char_data *ch);

void char_from_room(struct char_data *ch);
void char_to_room(struct char_data *ch, int room);
void extract_char(struct char_data *ch);
/* Buru 13/12/97 - coin converters */
void money_convert(struct char_data *ch, int amount);
void copper_to_coins(struct char_data *ch);
void convert_coins_copper(struct char_data *ch);

/* mobact.c */
void forget(struct char_data *ch, struct char_data *victim);
void remember(struct char_data *ch, struct char_data *victim);

#endif

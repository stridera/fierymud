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

#pragma once

/* Prototypes for finding objs/chars */
#include "find.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/* handling the affected-structures */
void effect_total(char_data *ch);
void do_campout(char_data *ch);
void effect_modify(char_data *ch, byte loc, sh_int mod, flagvector bitv[], bool add);
void effect_to_char(char_data *ch, effect *eff);
void effect_remove(char_data *ch, effect *eff);
void active_effect_remove(char_data *ch, effect *effect);
void effect_from_char(char_data *ch, int type);
void active_effect_from_char(char_data *ch, int type);
bool affected_by_spell(char_data *ch, int type);
void effect_join(char_data *ch, effect *eff, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod, bool refresh);

/* utility */
int isname(const char *str, const char *namelist);
char *fname(const char *namelist);

/* ******** objects *********** */

void obj_to_char(obj_data *object, char_data *ch);
void obj_from_char(obj_data *object);

enum equip_result {
    EQUIP_RESULT_ERROR,  /* Internal error; object has not been manipulated */
    EQUIP_RESULT_REFUSE, /* Not equipped for gameplay reason; object is returned
                            to inventory */
    EQUIP_RESULT_SUCCESS /* Object was equipped */
};
void count_hand_eq(char_data *ch, int *hands_used, int *weapon_hands_used);
bool may_wear_eq(char_data *ch, obj_data *obj, int *where, bool sendmessage);
enum equip_result equip_char(char_data *ch, obj_data *obj, int pos);
struct obj_data *unequip_char(char_data *ch, int pos);

void obj_to_room(obj_data *object, int room);
void obj_from_room(obj_data *object);
void obj_to_obj(obj_data *obj, obj_data *obj_to);
void obj_from_obj(obj_data *obj);
void object_list_new_owner(obj_data *list, char_data *ch);

void extract_obj(obj_data *obj);

/* ******* characters ********* */

void init_char(char_data *ch);
void update_char(char_data *ch);

void char_from_room(char_data *ch);
void char_to_room(char_data *ch, int room);
void extract_char(char_data *ch);
/* Buru 13/12/97 - coin converters */
void money_convert(char_data *ch, int amount);
void copper_to_coins(char_data *ch);
void convert_coins_copper(char_data *ch);

/* mobact.c */
void forget(char_data *ch, char_data *victim);
void remember(char_data *ch, char_data *victim);

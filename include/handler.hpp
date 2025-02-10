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
void effect_total(CharData *ch);
void do_campout(CharData *ch);
void effect_modify(CharData *ch, byte loc, sh_int mod, EffectFlags flags, bool add);
void effect_to_char(CharData *ch, effect *eff);
void effect_remove(CharData *ch, effect *eff);
void active_effect_remove(CharData *ch, effect *effect);
void effect_from_char(CharData *ch, int type);
void active_effect_from_char(CharData *ch, int type);
bool affected_by_spell(CharData *ch, int type);
void effect_join(CharData *ch, effect *eff, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod, bool refresh);

/* utility */
int isname(const char *str, const char *namelist);
std::string fname(std::string_view namelist);
char *fname(const char *namelist);

/* ******** objects *********** */

void obj_to_char(ObjData *obj, CharData *ch);
void obj_from_char(ObjData *obj);

enum equip_result {
    EQUIP_RESULT_ERROR,  /* Internal error; object has not been manipulated */
    EQUIP_RESULT_REFUSE, /* Not equipped for gameplay reason; object is returned
                            to inventory */
    EQUIP_RESULT_SUCCESS /* Object was equipped */
};
void count_hand_eq(CharData *ch, int *hands_used, int *weapon_hands_used);
bool may_wear_eq(CharData *ch, ObjData *obj, int *where, bool sendmessage);
enum equip_result equip_char(CharData *ch, ObjData *obj, int pos);
ObjData *unequip_char(CharData *ch, int pos);

void obj_to_room(ObjData *obj, int room);
void obj_from_room(ObjData *obj);
void obj_to_obj(ObjData *obj, ObjData *obj_to);
void obj_from_obj(ObjData *obj);
void object_list_new_owner(ObjData *list, CharData *ch);

void extract_obj(ObjData *obj);

/* ******* characters ********* */

void init_char(CharData *ch);
void update_char(CharData *ch);

void char_from_room(CharData *ch);
void char_to_room(CharData *ch, int room);
void extract_char(CharData *ch);
/* Buru 13/12/97 - coin converters */
void money_convert(CharData *ch, int amount);
void copper_to_coins(CharData *ch);
void convert_coins_copper(CharData *ch);

/* mobact.c */
void forget(CharData *ch, CharData *victim);
void remember(CharData *ch, CharData *victim);

/* Global object iterator
 *
 * Helps the object extractor in limits.c to iterate over all objects
 * in the world while destroying some of them. */

extern ObjData *go_iterator;
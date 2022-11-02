/***************************************************************************
 *   File: act.h                                          Part of FieryMUD *
 *  Usage: header file for player-level commands and helper functions      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "interpreter.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/***************************************************************************
 * act.informative.c                                                       *
 ***************************************************************************/

/* Commands */
ACMD(do_color);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_experience);
ACMD(do_gen_ps);
ACMD(do_help);
ACMD(do_identify);
ACMD(do_innate);
ACMD(do_inventory);
ACMD(do_last_gossips);
ACMD(do_last_tells);
ACMD(do_listspells);
ACMD(do_look);
ACMD(do_music);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_search);
ACMD(do_skills);
ACMD(do_songs);
ACMD(do_spells);
ACMD(do_time);
ACMD(do_trophy);
ACMD(do_users);
ACMD(do_viewdam);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);

/* Constants */
#define SHOW_SHORT_DESC 0
#define SHOW_LONG_DESC 1
#define SHOW_BASIC_DESC 2
#define SHOW_FULL_DESC 3
#define SHOW_NO_FAIL_MSG (1 << 4)
#define SHOW_STACK (1 << 5)
#define SHOW_FLAGS (1 << 6)
#define SHOW_SKIP_SELF (1 << 7)
#define SHOW_MASK (SHOW_NO_FAIL_MSG | SHOW_STACK | SHOW_FLAGS | SHOW_SKIP_SELF)

#define YOU_ARE_BLIND "You can't see a damned thing; you're blind!\r\n"

/* Public functions */
extern void garble_text(char *string, int percent);
extern char *drunken_speech(char *string, int drunkenness);

bool senses_living(char_data *ch, char_data *vict, int basepct);
bool senses_living_only(char_data *ch, char_data *vict, int basepct);
const char *relative_location_str(int bits);
void split_coins(char_data *ch, int coins[], unsigned int mode);

void print_obj_to_char(obj_data *obj, char_data *ch, int mode, char *additional_args);
void list_obj_to_char(obj_data *list, char_data *ch, int mode);
void print_char_to_char(char_data *targ, char_data *ch, int mode);
void list_char_to_char(char_data *list, char_data *ch, int mode);
void print_room_to_char(room_num room_nr, char_data *ch, bool ignore_brief);
void look_at_room(char_data *ch, int ignore_brief);
void check_new_surroundings(char_data *ch, bool old_room_was_dark, bool tx_obvious);
void look_in_direct(char_data *ch, int dir);
void look_in_obj(char_data *ch, char *arg);
void look_at_target(char_data *ch, char *arg);
void identify_obj(obj_data *obj, char_data *ch, int location);
const char *status_string(int cur, int max, int mode);

/* status_string mode codes */
#define STATUS_COLOR 0
#define STATUS_ALIAS 1
#define STATUS_PHRASE 2

const char *save_message(int save);
const char *align_message(int align);
const char *hitdam_message(int value);
const char *armor_message(int ac);
const char *perception_message(int perception);
const char *hiddenness_message(int hiddenness);
const char *ability_message(int value);
long xp_percentage(char_data *ch);
const char *exp_message(char_data *ch);
const char *exp_bar(char_data *ch, int length, int gradations, int sub_gradations, bool color);
const char *cooldown_bar(char_data *ch, int cooldown, int length, int gradations, bool color);
const char *proficiency_message(int proficiency);

/* item functions */
// extern get_context *begin_get_transaction(char_data *ch);
// extern void end_get_transaction(get_context *context, const void *vict_obj);
// extern void perform_get_from_room(get_context *context, obj_data *obj);
// extern void perform_get_from_container(get_context *context, obj_data *obj, obj_data *cont);
// extern void get_random_object(get_context *context);

// extern void get_from_container(char_data *ch, obj_data *cont, char *name, int *amount);
// extern void get_from_room(char_data *ch, char *name, int amount);
// extern bool can_take_obj(char_data *ch, obj_data *obj);
// extern void get_check_money(char_data *ch, obj_data *obj);

// extern bool has_corpse_consent(char_data *ch, obj_data *cont);
// extern bool check_get_disarmed_obj(char_data *ch, char_data *last_to_hold, obj_data *obj);
// extern int conceal_roll(char_data *ch, obj_data *obj);

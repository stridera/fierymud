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
#include "money.hpp"
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

#define YOU_ARE_BLIND "You can't see a damned thing; you're blind!\n"

struct StackNode {
    ObjData *obj;
    int count;
    const char *to_char;
    const char *to_room;
    StackNode *next;
};
struct GetContext {
    CharData *ch;
    StackNode *stack;
    int coins[NUM_COIN_TYPES];
};

/* Public functions */
void garble_text(char *string, int percent);
std::string drunken_speech(std::string speech, int drunkenness);

bool senses_living(CharData *ch, CharData *vict, int basepct);
bool senses_living_only(CharData *ch, CharData *vict, int basepct);
const char *relative_location_str(int bits);
void split_coins(CharData *ch, Money coins, unsigned int mode);

void print_obj_to_char(ObjData *obj, CharData *ch, int mode, char *additional_args);
void list_obj_to_char(ObjData *list, CharData *ch, int mode);
void print_char_to_char(CharData *targ, CharData *ch, int mode);
void list_char_to_char(CharData *list, CharData *ch, int mode);
void print_room_to_char(room_num room_nr, CharData *ch, bool ignore_brief);
void look_at_room(CharData *ch, int ignore_brief);
void check_new_surroundings(CharData *ch, bool old_room_was_dark, bool tx_obvious);
void look_in_direct(CharData *ch, int dir);
void look_in_obj(CharData *ch, char *arg);
void look_at_target(CharData *ch, char *arg);
void identify_obj(ObjData *obj, CharData *ch, int location);
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
std::string_view ability_message(int value);
long xp_percentage(CharData *ch);
const char *exp_message(CharData *ch);
const char *exp_bar(CharData *ch, int length, int gradations, int sub_gradations, bool color);
const char *cooldown_bar(CharData *ch, int cooldown, int length, int gradations, bool color);
const char *hp_regen_message(int regen);
const char *focus_message(int focus);

/* item functions */
GetContext *begin_get_transaction(CharData *ch);
void end_get_transaction(GetContext *context, ObjData *vict_obj);
void perform_get_from_room(GetContext *context, ObjData *obj);
void perform_get_from_container(GetContext *context, ObjData *obj, ObjData *cont);
void get_random_object(GetContext *context);

void get_from_container(CharData *ch, ObjData *cont, char *name, int *amount);
void get_from_room(CharData *ch, char *name, int amount);
bool can_take_obj(CharData *ch, ObjData *obj);
void get_check_money(CharData *ch, ObjData *obj);

bool has_corpse_consent(CharData *ch, ObjData *cont);
bool check_get_disarmed_obj(CharData *ch, CharData *last_to_hold, ObjData *obj);
int conceal_roll(CharData *ch, ObjData *obj);

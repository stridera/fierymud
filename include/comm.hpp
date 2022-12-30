/***************************************************************************
 *   File: comm.h                                         Part of FieryMUD *
 *  Usage: header file: prototypes of public communication functions       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

#include <fmt/format.h>
#include <functional>
#include <string_view>
#include <variant>

#define NUM_RESERVED_DESCS 8

#define HOTBOOT_FILE "hotboot.dat"

// #define CBP_FUNC(name) int(name)(CharData *, int)
using CBP_FUNC = std::function<int(CharData *, int)>;

// Updated functions to use variadic templates and std::string_view
template <typename... Args> void all_printf(std::string_view str, Args &&...args) {
    all_printf(fmt::vformat(str, fmt::make_format_args(args...)));
}
void all_printf(std::string_view str);
template <typename... Args> void all_except_printf(CharData *ch, std::string_view str, Args &&...args) {
    all_except_printf(ch, fmt::vformat(str, fmt::make_format_args(args...)));
}
void all_except_printf(CharData *ch, std::string_view str);
template <typename... Args> void char_printf(const CharData *ch, std::string_view str, Args &&...args) {
    char_printf(ch, fmt::vformat(str, fmt::make_format_args(args...)));
}
void char_printf(const CharData *ch, std::string_view str);
template <typename... Args> void room_printf(int rvnum, std::string_view str, Args &&...args) {
    room_printf(rvnum, fmt::vformat(str, fmt::make_format_args(args...)));
}
void room_printf(int rvnum, std::string_view str);
template <typename... Args>
void zone_printf(int zone_vnum, int skip_room, int min_stance, std::string_view str, Args &&...args) {
    zone_printf(zone_vnum, skip_room, min_stance, fmt::vformat(str, fmt::make_format_args(args...)));
}
void zone_printf(int zone_vnum, int skip_room, int min_stance, std::string_view str);
template <typename... Args>
void callback_printf(CBP_FUNC(callback), int min_stance, std::string_view str, Args &&...args) {
    callback_printf(callback, min_stance, fmt::vformat(str, fmt::make_format_args(args...)));
}
void callback_printf(CBP_FUNC(callback), int min_stance, std::string_view str);

template <typename... Args> void outdoor_printf(int zone_num, std::string_view str, Args &&...args) {
    outdoor_printf(zone_num, fmt::vformat(str, fmt::make_format_args(args...)));
}
void outdoor_printf(int zone_num, std::string_view str);

void close_socket(DescriptorData *d);
int speech_ok(CharData *ch, int quiet);
void send_gmcp_room(CharData *ch);

/* deprecated functions */
void send_to_all(const char *messg);
void send_to_char(const char *messg, const CharData *ch);
void send_to_room(const char *messg, int room);
void send_to_zone(const char *messg, int zone_vnum, int skip_room, int min_stance);
void write_to_output(const char *txt, DescriptorData *d);

using ActArg = std::variant<std::nullptr_t, ObjData *, CharData *, const char *, int>;
void format_act(char *rtn, const char *orig, const CharData *ch, ActArg obj, ActArg vict_obj, const CharData *to);
void act(const char *str, int hide_invisible, const CharData *ch, ActArg obj, ActArg vict_obj, int type);

/* act() targets */
#define TO_ROOM 1
#define TO_VICT 2
#define TO_NOTVICT 3
#define TO_CHAR 4
#define TO_SLEEP (1 << 7)    /* sleeping persons can see msg */
#define TO_OLC (1 << 8)      /* persons doing OLC may see msg */
#define TO_VICTROOM (1 << 9) /* destination room will be vict's, not char's */

void string_to_output(DescriptorData *t, std::string_view txt);
int write_to_descriptor(socket_t desc, std::string_view txt);
void write_to_q(char *txt, txt_q *queue, int aliased, DescriptorData *d);
// void desc_printf(DescriptorData *t, const char *txt, ...) __attribute__((format(printf, 2, 3)));
template <typename... Args> void desc_printf(DescriptorData *t, std::string_view txt, Args &&...args) {
    desc_printf(t, fmt::vformat(txt, fmt::make_format_args(args...)));
}
inline void desc_printf(DescriptorData *t, std::string_view txt) { string_to_output(t, txt); }

typedef RETSIGTYPE sigfunc(int);

extern DescriptorData *descriptor_list;
extern unsigned long global_pulse;
extern unsigned long pulse;

/* local globals */
extern char comm_buf[MAX_STRING_LENGTH];
extern txt_block *bufpool;
extern int buf_largecount;
extern int buf_overflows;
extern int buf_switches;
extern int circle_shutdown;
extern int circle_reboot;
extern int no_specials;
extern int max_players;
extern int tics;
extern int scheck;
extern int dg_act_check;
extern int gossip_channel_active;

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

#include <variant>

#define NUM_RESERVED_DESCS 8

#define HOTBOOT_FILE "hotboot.dat"

#define CBP_FUNC(name) int(name)(void *obj, void *data)

/* comm.c */
void all_printf(const char *messg, ...) __attribute__((format(printf, 1, 2)));
void all_except_printf(CharData *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void char_printf(const CharData *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void room_printf(int rrnum, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void zone_printf(int zone_vnum, int skip_room, int min_stance, const char *messg, ...)
    __attribute__((format(printf, 4, 5)));
void callback_printf(CBP_FUNC(callback), void *data, const char *messg, ...) __attribute__((format(printf, 3, 4)));
void close_socket(DescriptorData *d);
int speech_ok(CharData *ch, int quiet);
void send_gmcp_room(CharData *ch);

/* deprecated functions */
void send_to_all(const char *messg);
void send_to_char(const char *messg, CharData *ch);
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

int write_to_descriptor(socket_t desc, const char *txt);
void write_to_q(char *txt, txt_q *queue, int aliased, DescriptorData *d);
void desc_printf(DescriptorData *t, const char *txt, ...) __attribute__((format(printf, 2, 3)));
extern void string_to_output(DescriptorData *t, const char *txt);

/* Convenience aliases */
#define cprintf char_printf
#define dprintf desc_printf
#define cbprintf callback_printf

typedef RETSIGTYPE sigfunc(int);

DescriptorData *descriptor_list = nullptr; /* master desc list */
unsigned long global_pulse = 0;            /* number of pulses since game start */
unsigned long pulse = 0;                   /* number of pulses since game started */

/* local globals */
static char comm_buf[MAX_STRING_LENGTH] = {'\0'};
txt_block *bufpool = 0;        /* pool of large output buffers */
int buf_largecount = 0;        /* # of large buffers which exist */
int buf_overflows = 0;         /* # of overflows of output */
int buf_switches = 0;          /* # of switches from small to large buf */
int circle_shutdown = 0;       /* clean shutdown */
int circle_reboot = 0;         /* reboot the game after a shutdown */
int no_specials = 0;           /* Suppress ass. of special routines */
int max_players = 0;           /* max descriptors available */
int tics = 0;                  /* for extern checkpointing */
int scheck = 0;                /* for syntax checking mode */
int dg_act_check;              /* toggle for act_trigger */
int gossip_channel_active = 1; /* Flag for turning on or off gossip for the whole MUD */

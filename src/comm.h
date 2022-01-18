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

#ifndef __FIERY_COMM_H
#define __FIERY_COMM_H

#include "structs.h"
#include "sysdep.h"

#define NUM_RESERVED_DESCS 8

#define HOTBOOT_FILE "hotboot.dat"

#define CBP_FUNC(name) int(name)(void *object, void *data)

/* comm.c */
struct char_data;
extern void all_printf(const char *messg, ...) __attribute__((format(printf, 1, 2)));
extern void all_except_printf(struct char_data *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
extern void char_printf(const struct char_data *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
extern void room_printf(room_num rrnum, const char *messg, ...) __attribute__((format(printf, 2, 3)));
extern void zone_printf(int zone_vnum, int skip_room, int min_stance, const char *messg, ...)
    __attribute__((format(printf, 4, 5)));
extern void callback_printf(CBP_FUNC(callback), void *data, const char *messg, ...)
    __attribute__((format(printf, 3, 4)));
extern void close_socket(struct descriptor_data *d);
extern int speech_ok(struct char_data *ch, int quiet);
void send_gmcp_room(struct char_data *ch);

/* deprecated functions */
extern void send_to_all(const char *messg);
extern void send_to_char(const char *messg, struct char_data *ch);
extern void send_to_room(const char *messg, int room);
extern void send_to_zone(const char *messg, int zone_vnum, int skip_room, int min_stance);
extern void write_to_output(const char *txt, struct descriptor_data *d);

extern void format_act(char *rtn, const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj,
                       const struct char_data *to);

extern void act(const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj,
                int type);

/* act() targets */
#define TO_ROOM 1
#define TO_VICT 2
#define TO_NOTVICT 3
#define TO_CHAR 4
#define TO_SLEEP (1 << 7)    /* sleeping persons can see msg */
#define TO_OLC (1 << 8)      /* persons doing OLC may see msg */
#define TO_VICTROOM (1 << 9) /* destination room will be vict's, not char's */

extern int write_to_descriptor(socket_t desc, char *txt);
extern void write_to_q(char *txt, struct txt_q *queue, int aliased, struct descriptor_data *d);
extern void desc_printf(struct descriptor_data *t, const char *txt, ...) __attribute__((format(printf, 2, 3)));
extern void string_to_output(struct descriptor_data *t, const char *txt);

/* Convenience aliases */
#define cprintf char_printf
#define dprintf desc_printf
#define cbprintf callback_printf

typedef RETSIGTYPE sigfunc(int);

#ifndef __COMM_C__
extern struct descriptor_data *descriptor_list;
#endif

/* Function prototypes for brain-dead OS's */

#if defined(__COMM_C__) && defined(__GNUC__)

#ifndef accept
extern int accept();
#endif

#ifndef bind
extern int bind();
#endif

#ifndef chdir
extern int chdir();
#endif

#ifndef close
extern int close();
#endif
/* removed by Gurlaek
#ifndef fcntl
   extern int fcntl();
#endif
*/
#ifndef getpeername
extern int getpeername();
#endif

#ifndef getrlimit
extern int getrlimit();
#endif

#ifndef getsockname
extern int getsockname();
#endif

/*#ifndef htonl
   extern u_long htonl();
#endif
*/
/* removed by Gurlaek
#ifndef htons
  extern u_short htons();
#endif
*/
#ifndef listen
extern int listen();
#endif

/*#ifndef ntohl
   extern u_long ntohl();
#endif
*/
#ifndef read
extern ssize_t read();
#endif

#ifndef select
extern int select();
#endif

#ifndef setitimer
extern int setitimer();
#endif

#ifndef setrlimit
extern int setrlimit();
#endif

#ifndef setsockopt
extern int setsockopt();
#endif

#ifndef socket
extern int socket();
#endif

#ifndef write
extern ssize_t write();
#endif

#endif /* __COMM_C__ */

#endif /* __FIERY_COMM_H */

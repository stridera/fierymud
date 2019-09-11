/***************************************************************************
 * $Id: comm.h,v 1.29 2009/03/07 22:30:11 jps Exp $
 ***************************************************************************/
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

#define NUM_RESERVED_DESCS        8

#define HOTBOOT_FILE "hotboot.dat"

#define CBP_FUNC(name)     int (name)(void *object, void *data)

/* comm.c */
extern void all_printf(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
extern void all_except_printf(struct char_data *ch, const char *messg, ...)
      __attribute__ ((format (printf, 2, 3)));
extern void char_printf(const struct char_data *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
extern void room_printf(room_num rrnum, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
extern void zone_printf(int zone_vnum, int skip_room, int min_stance, const char *messg, ...) __attribute__ ((format (printf, 4, 5)));
extern void callback_printf(CBP_FUNC(callback), void *data, const char *messg, ...) __attribute__ ((format (printf, 3, 4))) ;
extern void close_socket(struct descriptor_data *d);
extern int speech_ok(struct char_data *ch, int quiet);

/* deprecated functions */
extern void send_to_all(const char *messg);
extern void send_to_char(const char *messg, struct char_data *ch);
extern void send_to_room(const char *messg, int room);
extern void send_to_zone(const char *messg, int zone_vnum, int skip_room, int min_stance);
extern void write_to_output(const char *txt, struct descriptor_data *d);

extern void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
                 const void *vict_obj, const struct char_data *to);

extern void act(const char *str, int hide_invisible, struct char_data *ch,
         struct obj_data *obj, const void *vict_obj, int type);

/* act() targets */
#define TO_ROOM      1
#define TO_VICT      2
#define TO_NOTVICT   3
#define TO_CHAR      4
#define TO_SLEEP      (1 << 7)  /* sleeping persons can see msg */
#define TO_OLC        (1 << 8)  /* persons doing OLC may see msg */
#define TO_VICTROOM   (1 << 9)  /* destination room will be vict's, not char's */


extern int write_to_descriptor(socket_t desc, char *txt);
extern void write_to_q(char *txt, struct txt_q *queue, int aliased, struct descriptor_data *d);
extern void desc_printf(struct descriptor_data *t, const char *txt, ...) __attribute__ ((format (printf, 2, 3)));
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
/***************************************************************************
 * $Log: comm.h,v $
 * Revision 1.29  2009/03/07 22:30:11  jps
 * Add act flag TO_VICTROOM which makes the message go to the victim's room.
 *
 * Revision 1.28  2008/09/07 20:08:54  jps
 * Added all_except_printf.
 *
 * Revision 1.27  2008/08/25 02:31:30  jps
 * The logic of the act() targets bits didn't fit well with the way
 * the targets were being used, so I undid the change.
 *
 * Revision 1.26  2008/08/23 14:03:30  jps
 * Changed the way act() messages are targeted. Now there are three basic
 * target flags, which may be or'd together in any combination, plus three
 * modifier flags for sleep, old, and trigger. The old TO_NOTVICT and
 * TO_ROOM targets are defined in terms of the new flags.
 *
 * Revision 1.25  2008/08/16 23:04:03  jps
 * Added speech_ok() to comm.h.
 *
 * Revision 1.24  2008/08/14 23:02:11  myc
 * Added vararg capability to all the standard output functions (like
 * send_to_char and write_to_output).  The old functions are still
 * available.  The new ones follow a *printf naming convention.
 * However, removed the send_to_outdoor functionality, and replaced
 * it with callback_printf.
 *
 * Revision 1.23  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.22  2008/06/09 23:00:13  myc
 * Added 'extern' to all the function declarations.
 *
 * Revision 1.21  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.20  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.19  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.18  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.17  2008/02/24 17:31:13  myc
 * Added a TO_OLC flag to act() to allow messages to be sent to people
 * while in OLC if they have OLCComm toggled on.
 *
 * Revision 1.16  2008/02/09 07:05:37  myc
 * Copyover is now renamed to hotboot.
 *
 * Revision 1.15  2008/02/09 03:04:23  myc
 * Adding the 'copyover' command, which allows you to do a hot-boot
 * without disconnecting anybody.
 *
 * Revision 1.14  2008/02/06 21:53:53  myc
 * Make the format arg to act() const.
 *
 * Revision 1.13  2008/02/02 19:38:20  myc
 * Adding the string_write function for putting a descriptor
 * into the string editor.
 *
 * Revision 1.12  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.11  2008/01/29 16:51:12  myc
 * Making the vict_obj of act() const.
 *
 * Revision 1.10  2007/12/19 20:46:13  myc
 * Added const modifiers to the char arguments to write_to_output,
 * send_to_char, send_to_zone, send_to_room, and parse_color, which
 * allows you to output a const string without casting it.  save_char()
 * no longer requires a save room (which wasn't being used anyway).
 *
 * Revision 1.9  2007/08/27 21:18:00  myc
 * You can now queue up commands while casting as well as abort midcast.
 * Casting commands such as look and abort are caught and interpreted
 * before the input is normally queued up by the game loop.
 *
 * Revision 1.8  2007/07/25 00:38:03  jps
 * Give send_to_zone a room to skip, and make it use virtual zone number.
 *
 * Revision 1.7  2007/07/24 23:34:00  jps
 * Add a parameter min_position to send_to_zone()
 *
 * Revision 1.6  2006/11/10 21:04:05  jps
 * Updated perform_act() with new format codes.
 *
 * Revision 1.5  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.4  2000/11/20 20:05:24  rsd
 * Fixed comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/04/23 23:27:10  jimmy
 * Fixed warnings/errors associated with the addition of the pendantic compiler flag
 * yeeeeehaaawwww.  --gurlaek
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

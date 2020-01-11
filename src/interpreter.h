/***************************************************************************
 * $Id: interpreter.h,v 1.60 2009/07/16 22:27:56 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: interpreter.h                                  Part of FieryMUD *
 *  Usage: header file: public procs, macro defs, subcommand defines       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_INTERPRETER_H
#define __FIERY_INTERPRETER_H

#include "structs.h"
#include "sysdep.h"

#define ACMD(name) void(name)(struct char_data * ch, char *argument, int cmd, int subcmd)

#define CMD_NAME (cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcmp(cmd_name, cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (cmdnum >= 1 && cmdnum <= 6)

void command_interpreter(struct char_data *ch, char *argument);
void list_similar_commands(struct char_data *ch, char *arg);
int searchblock(char *arg, const char **list, bool exact);
int search_block(const char *arg, const char **list, bool exact);
#define parse_direction(arg) (search_block(arg, dirs, FALSE))
char lower(char c);
char *one_argument(char *argument, char *first_arg);
char *one_word(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
char *delimited_arg(char *argument, char *quoted_arg, char delimiter);
char *delimited_arg_case(char *argument, char *quoted_arg, char delimiter);
char *delimited_arg_all(char *argument, char *quoted_arg, char delimiter);
int fill_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
void nanny(struct descriptor_data *d, char *arg);
int is_abbrev(const char *arg1, const char *arg2);
bool is_integer(const char *str);
bool is_positive_integer(const char *str);
bool is_negative_integer(const char *str);
bool is_number(const char *str);
int find_command(char *command);
int parse_command(char *command);
void skip_slash(char **string);
void skip_spaces(char **string);
char *delete_doubledollar(char *string);

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)

#define CMD_NONE 0
#define CMD_MEDITATE (1 << 0)
#define CMD_MAJOR_PARA (1 << 1)
#define CMD_MINOR_PARA (1 << 2)
#define CMD_HIDE (1 << 3)
#define CMD_BOUND (1 << 4)
#define CMD_CAST (1 << 5)
#define CMD_OLC (1 << 6)
#define CMD_NOFIGHT (1 << 7)
#define CMD_ANY ((1 << 8) - 1 - CMD_NOFIGHT)

struct command_info {
    char *command;
    byte minimum_position;
    byte minimum_stance;
    void (*command_pointer)(struct char_data *ch, char *argument, int cmd, int subcmd);
    sh_int minimum_level;
    int subcmd;
    long flags;
};

struct sort_struct {
    int sort_pos;
    byte is_social;
};

/* necessary for CMD_IS macro */
#ifndef __INTERPRETER_C__
extern struct command_info cmd_info[];
extern const char *command_flags[];
extern int num_of_cmds;
extern struct sort_struct *cmd_sort_info;
#endif

/* this is the new xnames structure --Gurlaek 6/9/1999 */
struct xname {
    /* the +3 is for the # \n and NULL chars */
    char name[MAX_NAME_LENGTH + 3];
    struct xname *next;
};
#define NAME_TIMEOUT 1 * (30 RL_SEC) /* 5 minutes */

extern void free_alias(struct alias_data *alias);
extern void free_aliases(struct alias_data *alias_list);

#define ALIAS_SIMPLE 0
#define ALIAS_COMPLEX 1
#define ALIAS_NONE -1

#define ALIAS_SEP_CHAR ';'
#define ALIAS_VAR_CHAR '$'
#define ALIAS_GLOB_CHAR '*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_STAY 0
#define SCMD_NORTH 1
#define SCMD_EAST 2
#define SCMD_SOUTH 3
#define SCMD_WEST 4
#define SCMD_UP 5
#define SCMD_DOWN 6

/* name approval */
#define SCMD_ACCEPT 0
#define SCMD_DECLINE 1
#define SCMD_LIST 2

/* Toggles */

/* These numbers are synchronized with a data structure that do_toggle()
 * uses.  So don't change them! */
#define SCMD_NOSUMMON 0
#define SCMD_NOHASSLE 1
#define SCMD_BRIEF 2
#define SCMD_COMPACT 3
#define SCMD_NOTELL 4
#define SCMD_AFK 5
#define SCMD_DEAF 6
#define SCMD_NOGOSSIP 7
#define SCMD_NOGRATZ 8
#define SCMD_NOWIZ 9
#define SCMD_QUEST 10
#define SCMD_ROOMFLAGS 11
#define SCMD_NOREPEAT 12
#define SCMD_HOLYLIGHT 13
#define SCMD_AUTOEXIT 14
#define SCMD_NOPETI 15
#define SCMD_NONAME 16
#define SCMD_ANON 17
#define SCMD_SHOWVNUMS 18
#define SCMD_WIMPY 19
#define SCMD_NICEAREA 20
#define SCMD_VICIOUS 21
#define SCMD_PASSIVE 22
#define SCMD_PAGELENGTH 23
#define SCMD_NO_FOLLOW 24
#define SCMD_ROOMVIS 25
#define SCMD_NOCLANCOMM 26
#define SCMD_OLCCOMM 27
#define SCMD_LINENUMS 28
#define SCMD_AUTOLOOT 29
#define SCMD_AUTOTREAS 30
#define SCMD_AUTOINVIS 31
#define SCMD_EXPANDOBJS 32
#define SCMD_EXPANDMOBS 33
#define SCMD_SACRIFICIAL 34
#define SCMD_TELNETGA 35

/* do_wizutil */
#define SCMD_REROLL 0
#define SCMD_PARDON 1
#define SCMD_NOTITLE 2
#define SCMD_SQUELCH 3
#define SCMD_FREEZE 4
#define SCMD_THAW 5
#define SCMD_UNAFFECT 6

/* do_spec_com */
#define SCMD_WHISPER 0
#define SCMD_ASK 1

/* do_gen_com */
#define SCMD_HOLLER 0
#define SCMD_SHOUT 1
#define SCMD_GOSSIP 2
#define SCMD_AUCTION 3
#define SCMD_GRATZ 4

/* do_gen_ps */
#define SCMD_CLEAR 0
#define SCMD_VERSION 1
#define SCMD_WHOAMI 2

/* do_shutdown */
#define SCMD_SHUTDOW 0
#define SCMD_SHUTDOWN 1

/* do_quit */
#define SCMD_QUI 0
#define SCMD_QUIT 1

/* do_date */
#define SCMD_DATE 0
#define SCMD_UPTIME 1

/* do_commands */
#define SCMD_COMMANDS 0
#define SCMD_SOCIALS 1
#define SCMD_WIZHELP 2

/* do_drop */
#define SCMD_DROP 0 /* "drop" command was used */
#define SCMD_JUNK 1 /* "junk" command was used */
#define SCMD_LETDROP                                                                                                   \
    2 /* Item is falling to the ground because someone tried                                                           \
         to give it to an insubstantial person */

/* do_gen_write */
#define SCMD_BUG 0
#define SCMD_TYPO 1
#define SCMD_IDEA 2
#define SCMD_NOTE 3

/* do_qcomm */
#define SCMD_QSAY 0
#define SCMD_QECHO 1

/* do_pour */
#define SCMD_POUR 0
#define SCMD_FILL 1

/* do_poof */
#define SCMD_POOFIN 0
#define SCMD_POOFOUT 1

/* do_hit */
#define SCMD_HIT 0
#define SCMD_MURDER 1

/* do_eat */
#define SCMD_EAT 0
#define SCMD_TASTE 1
#define SCMD_DRINK 2
#define SCMD_SIP 3

/* do_use */
#define SCMD_USE 0
#define SCMD_QUAFF 1
#define SCMD_RECITE 2

/* do_echo */
#define SCMD_ECHO 0
#define SCMD_EMOTE 1
#define SCMD_EMOTES 2

/* do_gen_door */
#define SCMD_OPEN 0
#define SCMD_CLOSE 1
#define SCMD_UNLOCK 2
#define SCMD_LOCK 3
#define SCMD_PICK 4

/*. do_olc .*/
#define SCMD_OLC_REDIT 0
#define SCMD_OLC_OEDIT 1
#define SCMD_OLC_ZEDIT 2
#define SCMD_OLC_MEDIT 3
#define SCMD_OLC_SEDIT 4
#define SCMD_OLC_SAVEINFO 5
#define SCMD_OLC_HEDIT 6
#define SCMD_OLC_TRIGEDIT 7
#define SCMD_OLC_SDEDIT 8

/* do_light */
#define SCMD_LIGHT 0
#define SCMD_EXTINGUISH 1

/* do_follow */
#define SCMD_FOLLOW 0
#define SCMD_SHADOW 1

/* do_stat */
#define SCMD_STAT 0
#define SCMD_RSTAT 1
#define SCMD_SSTAT 2

/* do_vstat */
#define SCMD_VSTAT 0
#define SCMD_MSTAT 1
#define SCMD_OSTAT 2

/* do_estat */
#define SCMD_ESTAT 0
#define SCMD_OESTAT 1
#define SCMD_RESTAT 2

/* do_vsearch */
#define SCMD_VSEARCH 0
#define SCMD_VLIST 1
#define SCMD_VWEAR 2
#define SCMD_VITEM 3
#define SCMD_VNUM 4

/* do_report */
#define SCMD_REPORT 0
#define SCMD_GREPORT 1

/* do_cast */
#define SCMD_CAST 0
#define SCMD_CHANT 1
#define SCMD_SING 2

/* do_hitall */
#define SCMD_HITALL 0
#define SCMD_TANTRUM 1

/* do_bash */
#define SCMD_BASH 0
#define SCMD_BODYSLAM 1
#define SCMD_MAUL 2

/* do_roar */
#define SCMD_ROAR 0
#define SCMD_HOWL 1

#endif

/***************************************************************************
 * $Log: interpreter.h,v $
 * Revision 1.60  2009/07/16 22:27:56  myc
 * Add guard clause.
 *
 * Revision 1.59  2009/06/09 05:42:04  myc
 * Renamed NOCTELL to NOCLANCOMM.
 *
 * Revision 1.58  2009/03/20 23:02:59  myc
 * Move text file handling routines into text.c
 *
 * Revision 1.57  2009/03/09 20:36:00  myc
 * Removed SCMD_DONATE subcommand.
 *
 * Revision 1.56  2009/03/09 02:22:32  myc
 * Added edit command.  Modified is_number to allow leading spaces.
 *
 * Revision 1.55  2009/03/07 11:12:05  jps
 * Separated the read command from the look command.
 *
 * Revision 1.54  2009/02/11 17:03:39  myc
 * Adding delimited_arg_case, which is exactly like delimited_arg,
 * but it doesn't make everything lower case.  (There's got to be
 * a better way to do this.)
 *
 * Revision 1.53  2009/01/18 06:58:53  myc
 * Adding "emote's" command so you can emote stuff like
 * "Laoris's arms are tired."
 *
 * Revision 1.52  2008/09/28 19:06:32  jps
 * Change SCMD_NOTES to SCMD_NOTE.
 *
 * Revision 1.51  2008/08/23 21:36:22  myc
 * Make parse_direction use search_block instead of searchblock so
 * it's case-insensitive.
 *
 * Revision 1.50  2008/08/17 20:16:00  jps
 * Added macro for parse_direction
 *
 * Revision 1.49  2008/08/14 23:10:35  myc
 * Made one of the arguments to search_block const.
 *
 * Revision 1.48  2008/07/15 18:53:39  myc
 * Added an array of strings for command flags.
 *
 * Revision 1.47  2008/07/15 17:59:03  myc
 * Added parse_command and list_similar_commands.
 *
 * Revision 1.46  2008/05/19 06:53:04  jps
 * Got rid of fup and fdown commands.
 *
 * Revision 1.45  2008/05/09 22:04:33  jps
 * Add delimited_arg_all(), which is like delimited_arg() except that
 * when there's no delimiter, it will return everything as the arg
 * (not just the first word).
 *
 * Revision 1.44  2008/04/13 00:57:21  jps
 * Added an auto-treasure loot pref.
 *
 * Revision 1.43  2008/04/07 04:31:42  jps
 * Add CMD_NOFIGHT to mark commands that shouldn't be available to
 * fighting characters..
 *
 * Revision 1.42  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.41  2008/04/06 19:45:21  jps
 * Add another drop subcommand so you can use the drop code for object
 * that fall accidentally when you give them to a fluid person.
 *
 * Revision 1.40  2008/04/04 06:12:52  myc
 * is_number should point to is_positive_integer
 *
 * Revision 1.39  2008/04/03 17:33:34  jps
 * Added a toggle for autoinvis.
 *
 * Revision 1.38  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.37  2008/03/10 18:01:17  myc
 * Made bodyslam and maul subcommands of bash.  Made tantrum a subcommand
 * of hitall.  Made battle howl a subcommand of roar.  Added ground
 * shaker command as stomp.
 *
 * Revision 1.36  2008/03/09 18:15:17  jps
 * Added a movement subcommand of 'stay', which is most useful when
 * misdirecting your movements.
 *
 * Revision 1.35  2008/03/05 03:03:54  myc
 * Exporting free_alias and free_aliases.
 *
 * Revision 1.34  2008/02/24 17:31:13  myc
 * You can now execute certain actions by preceding them with a ~
 * in OLC if the command is marked CMD_OLC (or CMD_ANY).
 *
 * Revision 1.33  2008/02/16 20:31:32  myc
 * Moving command sorting code here from act.informative.c.
 *
 * Revision 1.32  2008/02/02 19:38:20  myc
 * Moved HUH and NOONE from here to config.c
 *
 * Revision 1.31  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.30  2008/01/17 01:29:10  myc
 * Replaced is_number with is_integer, is_positive_integer, and
 * is_negative_integer.  is_number is now a macro aliased to
 * is_positive_integer.
 *
 * Revision 1.29  2007/12/19 20:52:57  myc
 * Added a const modifier to is_number.  Added HUH and NOONE
 * macro constants.  Renamed the CLOAKED toggle to ROOMVIS.
 *
 * Revision 1.28  2007/10/23 20:21:00  myc
 * Slightly redesigned the master command list, replacing the six boolean
 * variables on each line with a single bitvector.  Also replaced all of
 * the compiler ifdef checks with administration levels defined in
 * structs.h.
 *
 * Revision 1.27  2007/10/17 17:18:04  myc
 * Renamed the search_block and search_block2 functions.
 * searchblock is now case sensitive, and search_block is not.
 *
 * Revision 1.26  2007/10/11 20:14:48  myc
 * Chant command is now a subcommand of do_cast.  Songs command moved
 * to act.informative.c.
 *
 * Revision 1.25  2007/10/02 02:52:27  myc
 * Report command now has subcommands for greport and mreport.
 *
 * Revision 1.24  2007/09/28 20:49:35  myc
 * The vnum, mnum, onum, rnum, tnum, mlist, olist, rlist, tlist, slist,
 * vwear, and vitem commands now use the vsearch command suite, which is
 * now also available through the vsearch, vfind, osearch, ofind, msearch,
 * mfind, tsearch, tfind, ssearch, sfind, rsearch, rfind, and vlist
 * commands.
 * Added a delimited_arg() function (actually just renamed one_word) that
 * lets you return multi-word arguments surrounded by a given character,
 * such as a quote.  This is useful for spell casting, for example.
 *
 * Revision 1.23  2007/09/20 21:20:43  myc
 * Added cloaked toggle.
 *
 * Revision 1.22  2007/09/11 16:34:24  myc
 * Added claw, electrify, and peck skills.
 * Changed is_abbrev to accept const strings.
 *
 * Revision 1.21  2007/08/26 08:49:36  jps
 * Added commands estat, oestat, and restat, for viewing extra
 * descriptions on objects and rooms.
 *
 * Revision 1.20  2007/08/24 22:10:21  jps
 * Add sstat (shop stat) as a subcommand of stat.
 *
 * Revision 1.19  2007/08/24 17:01:36  myc
 * Adding ostat and mstat commands as shorthand for vstat, rstat for stat
 * room, and mnum and onum for vnum.  Also adding rnum and znum with new
 * functionality.
 *
 * Revision 1.18  2007/08/14 22:43:07  myc
 * Adding shadow as subcommand of follow.
 *
 * Revision 1.17  2007/07/19 15:32:01  jps
 * Add "extinguish" as a subcommand of light.
 *
 * Revision 1.16  2005/07/26 05:38:13  jwk
 * Added void skip_slash(char **string);
 *
 * Revision 1.15  2003/07/29 03:49:05  jjl
 * Reordered an extern definition, to remove compile warnings.
 *
 * Revision 1.14  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.13  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.12  2000/11/22 19:29:30  rsd
 * Altered comment header and added back rlog messages from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.11  2000/02/22 00:50:23  rsd
 * Changed the time out on name approval to 30 seconds.
 *
 * Revision 1.10  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.9  1999/07/23 01:54:15  mud
 * Removed ooc and slowns form the define list for do gen toggle
 * SCMD_OOC and SCMD_SLOWNS were removed and the other SCMD's
 * for do gen toggle were renumbered accordingly.
 *
 * Revision 1.8  1999/06/10 16:56:28  mud
 *
 *
 * Revision 1.7  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.6  1999/03/26 19:44:35  jen
 * Added a mortal gossip channel with 103+ godly control
 *
 * Revision 1.5  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.4  1999/02/06 00:40:36  jimmy
 * Major change to incorporate aliases into the pfile
 * moved alias structure from interpreter.h to structs.h
 * heavily modified alias code in interpreter.c
 * Jimmy Kincaid AKA fingon
 *
 * Revision 1.3  1999/02/01 08:16:30  jimmy
 * improved build counter
 *
 * Revision 1.2  1999/02/01 04:10:48  jimmy
 * Added buildcounter to GREETING --Fingon
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

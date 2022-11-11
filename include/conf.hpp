/***************************************************************************
 *  File: conf.h                                          Part of FieryMUD *
 *  Usage: Uh, like defines and stuff based on system dependencies         *
 *  By: Generated automatically by configure.                              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "defines.hpp"

#include <time.h>

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if you have the crypt function.  */
#define HAVE_CRYPT 1

/* Define if you have the random function.  */
#define HAVE_RANDOM 1

/* Define if you have the <arpa/telnet.h> header file.  */
#define HAVE_ARPA_TELNET_H 1

/* Define if you have the <assert.h> header file.  */
#define HAVE_ASSERT_H 1

/* Define if you have the <crypt.h> header file.  */
#define HAVE_CRYPT_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <net/errno.h> header file.  */
/* #undef HAVE_NET_ERRNO_H */

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/fcntl.h> header file.  */
#define HAVE_SYS_FCNTL_H 1

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the crypt library (-lcrypt).  */
#define HAVE_LIBCRYPT 1

/* Define if you have the malloc library (-lmalloc).  */
#define HAVE_LIBMALLOC 1

/* Define if you have the nsl library (-lnsl).  */
#define HAVE_LIBNSL 1

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/

/* GAME PLAY OPTIONS */

#define true 1
#define YES 1
#define false 0
#define NO 0

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
int pk_allowed = NO;
int summon_allowed = YES;
int charm_allowed = NO;
int sleep_allowed = NO;
int roomeffect_allowed = YES;

#ifdef PRODUCTION
int races_allowed = YES;
int evil_races_allowed = NO; /* Allows good races only. */
#else
int races_allowed = YES;
int evil_races_allowed = YES;
#endif

/* do you have to visit your guild to level? */
int level_gain = YES;

/* Show damage amounts to players? */
int damage_amounts = YES;

/* is playerthieving allowed? */
int pt_allowed = NO;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 1;

/* default group level difference, zero is OFF */
int max_group_difference = NO;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 3500;  /* 3500 ticks is approximately 3 real days  */
int short_pc_corpse_time = 100; /* 100 ticks = about 2 real hours */

/* approve new character names being allowing them to enter the game? */
int approve_names = NO;

/* new character name approval causes a pause before entering the game? */
int napprove_pause = NO;

/* should items in death traps automatically be junked? */
int dts_are_dumps = YES;

/* Automatic rebooting */
int reboot_hours_base = 140;     /* Average time to reboot (real hours) */
int reboot_hours_deviation = 30; /* added to or subtracted from reboot_hours_base */
/* between 2.5 and 5 days */

/* How long before rebooting do warnings begin? */
int reboot_warning_minutes = 10;

/* Setting this to false will prevent the mud from rebooting itself */
#ifdef PRODUCTION
int reboot_auto = true;
#else
int reboot_auto = false;
#endif

/* reboot_pulse is the time, on the pulse clock (global_pulse),
 * when the mud will actually reboot. This initial value is temporary.
 * It will be recalculated with a randomized value when the mud starts. */
long reboot_pulse = 3600 * PASSES_PER_SEC * 25;

int reboot_warning = 0; /* Whether any reboot warning has been given */
/* The number of minutes-till, when the last reboot warning was given */
int last_reboot_warning = 0;

/****************************************************************************/
/****************************************************************************/

/* ROOM NUMBERS */

/* vnum of room that mortals should enter at */
int mortal_start_room = 3001;

/* vnum of room that immorts should enter at by default */
int immort_start_room = 1200;

/* vnum of room that frozen players should enter at */
int frozen_start_room = 1202;

/*
 * vnum numbers of donation rooms.  note: you must change code in
 * do_drop of act.item.c if you change the number of non-NOWHERE
 * donation rooms.
 */
int donation_room_1 = NOWHERE;
int donation_room_2 = NOWHERE; /* unused - room for expansion */
int donation_room_3 = NOWHERE; /* unused - room for expansion */

/****************************************************************************/
/****************************************************************************/

/* GAME OPERATION OPTIONS */
time_t *boot_time = nullptr;         /* times of mud boots (size = 1 + num_hotboots) */
int num_hotboots = 0;                /* are we doing a hotboot? */
int should_restrict = 0;             /* level of game restriction         */
int restrict_reason = RESTRICT_NONE; /* reason for should_restrict > 0 */

/*
 * This is the default port the game should run on if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the
 * port number there will override this setting.  Change the PORT= line in
 * instead of (or in addition to) changing this.
 */
int DFLT_PORT = 9999;

/* default directory to use as data directory */
const char *DFLT_DIR = "lib";

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 300;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 3;

/*
 * Some nameservers are very slow and cause the game to lag terribly every
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = YES;

const char *MENU =
    "\r\n"
    "   ~~~ Welcome to &1&bFieryMUD&0 ~~~\r\n"
    "\r\n"
    "    0) Exit from &1&bFiery&0.\r\n"
    "    1) Enter the game.\r\n"
    "    2) Read the background story.\r\n"
    "    3) Change password.\r\n"
    "\r\n"
    "       Make your choice: ";

const char *GREETINGS =
    "\r\n\r\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
    "      .       &3&b ________&0                                             . \r\n"
    "      .       &3&b/   ___\\_}              ___    _____ ______&0           . \r\n"
    "      .       &3&b|  |   __  ________     |_ \\  /  | | |  |   \\&0         . \r\n"
    "      .       &3&b|  |__/  \\/  __ \\ |\\__ / /  \\/   | | |  |    \\&0        . \r\n";

const char *GREETINGS2 =
    "      .       &3&b|  ____\\ |  {_/ /  __ / /        | | |  | |\\  \\&0       . \r\n"
    "      .       &1&b|  |  |  | {___/  /\\ Y /|  /\\/\\  | | |  | | |  |&0      . \r\n"
    "      .       &1&b|  |  |  | \\___|  |/  / |  |  |  | \\_/  | L_|  |&0      . \r\n"
    "      .       &1&b|__|  \\__/_____/__|__/  |__|  |__|_____/______/&0       . \r\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
    "      .      &7&bMud based on: Copper DikuMud I (B.1) by Swiftest&0       "
    ". \r\n";

const char *GREETINGS3 =
    "      .    &7&bDikuMud creators: Hans Henrik Staerfeldt, Katja Nyboe,&0   "
    ". \r\n"
    "      .      &7&bTom Madsen, Michael Seifert, and Sebastian Hammer.&0     "
    ". \r\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
    "      .          &1&bFor Help or Information: &3&bwww.fierymud.org&0      "
    "    . \r\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    "\r\n";

const char *GREETINGS4 = "                                                     Build No.";

const char *TEST_GREETING =
    "\r\n\r\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
    "      .      &3&b________         _________________  _______________&0    "
    ". \r\n"
    "      .     &3&b/   ___\\_}        \\____      ____/ \\/    \\     "
    "____/&0    . \r\n"
    "      .     &3&b|  |   __  ________    \\_   /|  ___/   __/    /&0        "
    " . \r\n"
    "      .     &3&b|  |__/  \\/  __ \\ |\\__ / / | |  |__   /   |  |&0       "
    "   . \r\n";
const char *TEST_GREETING2 =
    "      .     &3&b|  ____\\ |  {_/ /  __ / /  | |   __|   \\  |  |&0        "
    "  . \r\n"
    "      .     &1&b|  |  |  | {___/  /\\ Y /|  | |  |__ \\   \\ |  |&0       "
    "   . \r\n"
    "      .     &1&b|  |  |  | \\___|  |/  / |  | |     \\_\\   \\|  |&0      "
    "    . \r\n"
    "      .     &1&b|__|  \\__/_____/__|__/  |__| \\_____/_____/|__|&0        "
    "  . \r\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
    "      .      Mud based on: Copper DikuMud I (B.1) by Swiftest       . "
    "\r\n";
const char *TEST_GREETING3 =
    "      .    DikuMud creators: Hans Henrik Staerfeldt, Katja Nyboe,   . \r\n"
    "      .      Tom Madsen, Michael Seifert, and Sebastian Hammer.     . \r\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
    "                                                     Build No.";

const char *WHOAREYOU =
    "\r\n\r\n"
    "                        Based on HubisMUD 1.0\r\n"
    "                       Based on CircleMUD 3.0,\r\n"
    "                       Created by Jeremy Elson\r\n"
    "\r\n"
    "By what name do you wish to be known? ";

const char *WELC_MESSG =
    "\r\n"
    "Welcome to FieryMUD!\r\n"
    "\r\n\r\n";

const char *START_MESSG =
    "Welcome.  This is your new character! Type help display "
    "and help newbie.\r\n";

const char *NAMES_EXPLANATION =
    "\r\n"
    "Our name policy is fairly relaxed, but does have the following "
    "restrictions:\r\n"
    "\r\n"
    "1) No dictionary words, or names resembling dictionary words, or\r\n"
    "   compound words. Examples: BraveBlade, Jesterx, Rex.\r\n"
    "2) No well-known names whether modern or historical, or from\r\n"
    "   within Fiery's world.\r\n"
    "3) Nothing that is deemed offensive or inappropriate by our staff.\r\n"
    "\r\n";

const char *BANNEDINTHEUSA =
    "\r\n"
    "                       IF YOU SEE THIS IT IS BAD!\r\n"
    "\r\n"
    "        FFFFFFFFFF  IIIIIII   EEEEEEEEEE   RRRRRR      YYY    YYY  \r\n"
    "       FFFFFFFFFF    IIIII   EEEEEEEEEE   RRRRRRRRR    YYY    YYY  \r\n"
    "       FFF            III    EEE          RRR   RRRR   YYY    YYY  \r\n"
    "       FFFFFF         III    EEEEEE       RRR    RRR    YYYYYYYY   \r\n"
    "       FFF            III    EEE          RRRRRRRRR         YYY    \r\n"
    "       FFF           IIIII   EEEEEEEEEE   RRR    RRR       YYY     \r\n";

const char *BANNEDINTHEUSA2 =
    "       FFF          IIIIIII   EEEEEEEEEE  RRRR    RRRR   YYYY      \r\n"
    "\r\n"
    " You have been banned from playing on FieryMUD. We are sorry if that is "
    "an\r\n"
    " inconvenience however it has been done with good reason.  Do not "
    "attempt\r\n"
    " to continually reconnect to FieryMUD as all connection will be met "
    "with\r\n"
    " this message. All connections are logged. If we have trouble from a "
    "site\r\n"
    " we can and will contact the administrators of the site or your "
    "Internet\r\n";

const char *BANNEDINTHEUSA3 =
    " service provider.\r\n"
    "                         YOU HAVE BEEN WARNED!\r\n"
    "\r\n"
    " We will review the ban list once every six months to determine if any "
    "site\r\n"
    " merits removal from the list.  If you feel you have been unjustly "
    "denied\r\n"
    " access to FieryMUD send mail to gods@fierymud.org.\r\n";

const char *NEWSUPDATED1 =
    "&2 __    __                                            &0\n"
    "&2|  \\  |  \\    &2&bTHE                              &0\n"
    "&2| $$\\ | $$  ______   __   __   __   _______         &0\n"
    "&2| $$$\\| $$ /      \\ |  \\ |  \\ |  \\ /       \\   &0\n"
    "&2| $$$$\\ $$|  $$$$$$\\| $$ | $$ | $$|  $$$$$$$       &0\n";
const char *NEWSUPDATED2 =
    "&2| $$\\$$ $$| $$    $$| $$ | $$ | $$ \\$$    \\       &0\n"
    "&2| $$ \\$$$$| $$$$$$$$| $$_/ $$_/ $$ _\\$$$$$$\\      &0\n"
    "&2| $$  \\$$$ \\$$     \\ \\$$   $$   $$|       $$     &0\n"
    "&2 \\$$   \\$$  \\$$$$$$$  \\$$$$$\\$$$$  \\$$$$$$$    &0\n"
    "&2&b      HAVE BEEN UPDATED!                           &0\n"
    "&2&b      Type 'news' in game to read it!              &0\n\n";

/****************************************************************************/
/****************************************************************************/

/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
/*  int use_autowiz = NO; */

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
/*  int min_wizlist_lev = LVL_GOD; */

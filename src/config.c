/***************************************************************************
 * $Id: config.c,v 1.53 2008/09/06 19:08:55 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: config.c                                       Part of FieryMUD *
 *  Usage: Configuration of various aspects of FieryMUD operation          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#define __CONFIG_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"

#define TRUE	1
#define YES	1
#define FALSE	0
#define NO	0

/* If you have problems change this to NO */
/* #define FUCKED YES */ 

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
int max_pc_corpse_time = 3500; /* 3500 ticks is approximately 3 real days  */
int short_pc_corpse_time = 100;  /* 100 ticks = about 2 real hours */

/* approve new character names being allowing them to enter the game? */
int approve_names = NO;

/* new character name approval causes a pause before entering the game? */
int napprove_pause = NO;

/* should items in death traps automatically be junked? */
int dts_are_dumps = YES;

/* "okay" etc. */
const char *OK = "Okay.\r\n";
const char *HUH = "Huh?!?\r\n";
const char *NOPERSON = "There is no one by that name here.\r\n";
const char *NOEFFECT = "Nothing seems to happen.\r\n";

/* Automatic rebooting */
int reboot_hours_base = 90;      /* Average time to reboot (real hours) */
int reboot_hours_deviation = 30; /* added to or subtracted from reboot_hours_base */
/* between 2.5 and 5 days */

/* How long before rebooting do warnings begin? */
int reboot_warning_minutes = 10;

/* Setting this to FALSE will prevent the mud from rebooting itself */
#ifdef PRODUCTION
int reboot_auto = TRUE;
#else
int reboot_auto = FALSE;
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

/* virtual number of room that mortals should enter at */
int mortal_start_room = 3001;

/* virtual number of room that immorts should enter at by default */
int immort_start_room = 1200;

/* virtual number of room that frozen players should enter at */
int frozen_start_room = 1202;

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.item.c if you change the number of non-NOWHERE
 * donation rooms.
 */
int donation_room_1 = NOWHERE;
int donation_room_2 = NOWHERE;	/* unused - room for expansion */
int donation_room_3 = NOWHERE;	/* unused - room for expansion */


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/*
 * This is the default port the game should run on if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the
 * port number there will override this setting.  Change the PORT= line in
 * instead of (or in addition to) changing this.
 */
int DFLT_PORT = 9999;

/* default directory to use as data directory */
char *DFLT_DIR = "lib";

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

char *MENU =
"\r\n"
"   ~~~ Welcome to &1&bFieryMUD&0 ~~~\r\n"
"\r\n"
"    0) Exit from &1&bFiery&0.\r\n"
"    1) Enter the game.\r\n"
"    2) Read the background story.\r\n"
"    3) Change password.\r\n"
"\r\n"
"       Make your choice: ";


char *GREETINGS =
"\r\n\r\n"
"      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
"      .       &3&b ________&0                                             . \r\n"
"      .       &3&b/   ___\\_}              ___    _____ ______&0           . \r\n"
"      .       &3&b|  |   __  ________     |_ \\  /  | | |  |   \\&0         . \r\n"
"      .       &3&b|  |__/  \\/  __ \\ |\\__ / /  \\/   | | |  |    \\&0        . \r\n";

char *GREETINGS2 = 
"      .       &3&b|  ____\\ |  {_/ /  __ / /        | | |  | |\\  \\&0       . \r\n"
"      .       &1&b|  |  |  | {___/  /\\ Y /|  /\\/\\  | | |  | | |  |&0      . \r\n"  
"      .       &1&b|  |  |  | \\___|  |/  / |  |  |  | \\_/  | L_|  |&0      . \r\n"  
"      .       &1&b|__|  \\__/_____/__|__/  |__|  |__|_____/______/&0       . \r\n"
"      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
"      .      &7&bMud based on: Copper DikuMud I (B.1) by Swiftest&0       . \r\n";

char *GREETINGS3 = 
"      .    &7&bDikuMud creators: Hans Henrik Staerfeldt, Katja Nyboe,&0   . \r\n"
"      .      &7&bTom Madsen, Michael Seifert, and Sebastian Hammer.&0     . \r\n"
"      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
"      .          &1&bFor Help or Information: &3&bwww.fierymud.org&0          . \r\n"
"      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n";

char *GREETINGS4 = 
"                                                     Build No.";






char *TEST_GREETING =
"\r\n\r\n"
"      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
"      .      &3&b________         _________________  _______________&0    . \r\n"    
"      .     &3&b/   ___\\_}        \\____      ____/ \\/    \\     ____/&0    . \r\n"
"      .     &3&b|  |   __  ________    \\_   /|  ___/   __/    /&0         . \r\n"
"      .     &3&b|  |__/  \\/  __ \\ |\\__ / / | |  |__   /   |  |&0          . \r\n";
char *TEST_GREETING2 =

"      .     &3&b|  ____\\ |  {_/ /  __ / /  | |   __|   \\  |  |&0          . \r\n" 
"      .     &1&b|  |  |  | {___/  /\\ Y /|  | |  |__ \\   \\ |  |&0          . \r\n"
"      .     &1&b|  |  |  | \\___|  |/  / |  | |     \\_\\   \\|  |&0          . \r\n"
"      .     &1&b|__|  \\__/_____/__|__/  |__| \\_____/_____/|__|&0          . \r\n"
"      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
"      .      Mud based on: Copper DikuMud I (B.1) by Swiftest       . \r\n";
char *TEST_GREETING3 =
"      .    DikuMud creators: Hans Henrik Staerfeldt, Katja Nyboe,   . \r\n"
"      .      Tom Madsen, Michael Seifert, and Sebastian Hammer.     . \r\n"
"      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \r\n"
"                                                     Build No.";


char *WHOAREYOU =

"\r\n\r\n"
"                        Based on HubisMUD 1.0\r\n"
"                       Based on CircleMUD 3.0,\r\n"
"                       Created by Jeremy Elson\r\n"
"\r\n"
"By what name do you wish to be known? ";


char *WELC_MESSG =
"\r\n"
"Welcome to FieryMUD!\r\n" 
"\r\n\r\n";

char *START_MESSG =
"Welcome.  This is your new character! Type help display and help newbie.\r\n";

char *NAMES_EXPLANATION =

"\r\n"
"Our name policy is fairly relaxed, but does have the following restrictions:\r\n"
"\r\n"
"1) No dictionary words, or names resembling dictionary words, or\r\n"
"   compound words. Examples: BraveBlade, Jesterx, Rex.\r\n"
"2) No well-known names whether modern or historical, or from\r\n"
"   within Fiery's world.\r\n"
"3) Nothing that is deemed offensive or inappropriate by our staff.\r\n"
"\r\n";

char *BANNEDINTHEUSA =
"\r\n"
"                       IF YOU SEE THIS IT IS BAD!\r\n"
"\r\n"
"        FFFFFFFFFF  IIIIIII   EEEEEEEEEE   RRRRRR      YYY    YYY  \r\n"
"       FFFFFFFFFF    IIIII   EEEEEEEEEE   RRRRRRRRR    YYY    YYY  \r\n"
"       FFF            III    EEE          RRR   RRRR   YYY    YYY  \r\n"
"       FFFFFF         III    EEEEEE       RRR    RRR    YYYYYYYY   \r\n"
"       FFF            III    EEE          RRRRRRRRR         YYY    \r\n"
"       FFF           IIIII   EEEEEEEEEE   RRR    RRR       YYY     \r\n";

char *BANNEDINTHEUSA2 =
"       FFF          IIIIIII   EEEEEEEEEE  RRRR    RRRR   YYYY      \r\n"
"\r\n"
" You have been banned from playing on FieryMUD. We are sorry if that is an\r\n"
" inconvenience however it has been done with good reason.  Do not attempt\r\n"
" to continually reconnect to FieryMUD as all connection will be met with\r\n"
" this message. All connections are logged. If we have trouble from a site\r\n"
" we can and will contact the administrators of the site or your Internet\r\n";

char *BANNEDINTHEUSA3 =
" service provider.\r\n"
"                         YOU HAVE BEEN WARNED!\r\n"
"\r\n"
" We will review the ban list once every six months to determine if any site\r\n"
" merits removal from the list.  If you feel you have been unjustly denied\r\n"
" access to FieryMUD send mail to gods@fierymud.org.\r\n";


/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
/*  int use_autowiz = NO; */

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
/*  int min_wizlist_lev = LVL_GOD; */

/***************************************************************************
 * $Log: config.c,v $
 * Revision 1.53  2008/09/06 19:08:55  jps
 * Change PC corpse time to 3500 ticks when full, 100 otherwise.
 *
 * Revision 1.52  2008/08/21 08:13:07  jps
 * Tweak formatting of game menu.
 *
 * Revision 1.51  2008/08/16 08:23:13  jps
 * Took desc and delete out of the pre-game menu.
 *
 * Revision 1.50  2008/08/14 23:02:11  myc
 * Removed the ANSI string from constants.c.
 *
 * Revision 1.49  2008/07/27 05:23:28  jps
 * Removed rent options since we have free rent.
 *
 * Revision 1.48  2008/05/26 18:29:59  jps
 * Changed autoboot time to 2.5-5 days.
 *
 * Revision 1.47  2008/05/26 18:24:48  jps
 * Removed code that deletes player object files.
 *
 * Revision 1.46  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.45  2008/02/02 19:38:20  myc
 * Moving HUH here from interpreter, and merging NOPERSON and NOONE.
 * Also making OK, NOEFFECT, and NOPERSON const.
 *
 * Revision 1.44  2008/01/04 02:30:32  jps
 * Change "good_races_allowed" to "evil_races_allowed". Set "races_allowed"
 * to default to YES.
 *
 * Revision 1.43  2007/11/18 19:30:46  jps
 * Change reboot time to 3-8 days.
 *
 * Revision 1.42  2007/08/30 10:15:39  jps
 * Made max rent items 1000000, effectively unlimited.
 *
 * Revision 1.41  2007/08/16 20:14:13  jps
 * Automatic rebooting defaults to off for non-production builds.
 *
 * Revision 1.40  2007/08/14 20:14:34  jps
 * Added variables to control and track automatic rebooting.
 *
 * Revision 1.39  2007/08/13 19:57:20  jps
 * Turn off automatic need for name approval.
 *
 * Revision 1.38  2007/06/04 22:24:41  jps
 * Add game-toggle for name approval pause and set name approval to default on.
 *
 * Revision 1.37  2006/11/26 08:31:17  jps
 * Changed name acceptability blurb, and moved it up in the character creation process.
 *
 * Revision 1.36  2006/11/22 22:20:48  jps
 * The splash text no longer says BETA TEST
 *
 * Revision 1.35  2004/11/11 23:01:22  rsd
 * Ok, I split up the char stars used for the login splash screens
 * into smaller char stars so the compiler would stop crying about
 * them being longer than 509 bytes.
 *
 * Revision 1.34  2003/06/23 07:18:32  jjl
 * Updated the name screen you get.
 *
 * Revision 1.33  2003/03/14 00:41:49  rsd
 * Ok, I made damage amount showing on by default, and name
 * approval off by default
 *
 * Revision 1.32  2002/12/04 09:09:30  rls
 * added define for max group level diff and set it to default off
 *
 * Revision 1.31  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.30  2002/07/05 17:45:06  rls
 * Colorized test and production login screens w/ more modern look
 *
 * Revision 1.24  2002/03/30 19:20:14  dce
 * Added damage amounts as a toggable item.
 *
 * Revision 1.23  2000/11/21 00:23:55  rsd
 * Added rlog messages from prior to the addition of
 * the $log$ string.
 *
 * Revision 1.22  2000/11/13 02:36:57  rsd
 * Made player corpse decay time 96 75 second ticks
 * Hopefully that's 2 hours.
 *
 * Revision 1.21  2000/10/13 04:01:36  cmc
 * added flag for activating "level gain" code
 *
 * Revision 1.20  2000/05/22 22:34:54  rsd
 * added a char star for the test mud. as well as altered the
 * start message to reflect current information.
 *
 * Revision 1.19  2000/04/30 18:13:28  rsd
 * allowing good races in production
 *
 * Revision 1.18  2000/04/26 22:50:28  rsd
 * added delete player back into the player menu.
 *
 * Revision 1.17  2000/04/14 00:51:28  rsd
 * Re-worked comment header to fieryize it.  Also added a new message to be
 * output to banned players. BANNEDINTHEUSA.
 *
 * Revision 1.16  2000/03/20 04:30:10  rsd
 * Added ifdefs for test/prod builds so test has race login
 * no name checking.  Also commented out autowiz options.
 *
 * Revision 1.15  1999/12/12 06:58:04  rsd
 * Changed charm_allowed and sleep_allowed to NO, Also
 * added int good_races_allowed = NO for the good race
 * character creation default boot state. Changed max
 * object save to 50 from 30000, some suspicion that renting
 * with a bunch of objects causes corruption.
 *
 * Revision 1.13  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.12  1999/07/11 04:17:08  mud
 * commented out the line in the game menu about
 * deleting yourself because it is causing problems.
 *
 * Revision 1.11  1999/06/30 18:25:04  jimmy
 * >> This is a major conversion from the 18 point attribute system to the
 * >> 100 point attribute system.  A few of the major changes are:
 * >> All attributes are now on a scale from 0-100
 * >> Everyone views attribs the same but, the attribs for one race
 * >>   may be differeent for that of another even if they are the
 * >>   same number.
 * >> Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * >> Mobs now have individual random attributes based on race/class.
 * >> The STR_ADD attrib has been completely removed.
 * >> All bonus tables for attribs in constants.c have been replaced by
 * >>   algorithims that closely duplicate the tables except on a 100 scale.
 * >> Some minor changes:
 * >> Race selection at char creation can now be toggled by using
 * >>   <world races off>
 * >> Lots of cleanup done to affected areas of code.
 * >> Setting attributes for mobs in the .mob file no longer functions
 * >>   but is still in the code for later use.
 * >> We now have a spare attribut structure in the pfile because the new
 * >>   system only used three instead of four.
 * >> --gurlaek 6/30/1999
 *
 * Revision 1.10  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.9  1999/03/22 21:07:59  mud
 * Changed the names explanation a bit to help with confusion.
 *
 * Revision 1.8  1999/03/14 00:53:03  mud
 * In class.c added a new line before the fiery mud class explanation
 * in config.c added the variable for name explanations and added the
 * text for the variable
 * in interpreter.c added the con_state stuff, whatever that was and
 * added the CON_NAME_CHECK affirmation section to the creation menu
 * loop or nanny.
 * In structs.h added the CON_NAME_CHECK define..
 * I also drove Jimmy absolutely insane with the deail in information
 * I put into our change control system.
 * lala
 *
 * Revision 1.7  1999/03/12 18:05:43  dce
 * No pk at all...only temporary.
 *
 * Revision 1.6  1999/03/03 21:52:55  mud
 * Removed the newline from the bottom of the login
 * menue it looked odd. grumble
 *
 * Revision 1.5  1999/03/01 17:39:40  mud
 * Added some carriage returns and spaces to the login menue
 *
 * Revision 1.4  1999/02/04 00:02:59  jimmy
 * max/min exp loss/gain set to 2 notches.
 *
 * Revision 1.3  1999/02/01 04:58:34  jimmy
 * cosmetic changes
 *
 * Revision 1.2  1999/02/01 04:10:22  jimmy
 * Added buildcounter to GREETING --Fingon
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

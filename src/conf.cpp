#include "conf.hpp"

#include <string>

using namespace std::string_literals;

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

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * false, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to true - and anything goes.
 */
bool pk_allowed = false;
bool summon_allowed = true;
bool charm_allowed = false;
bool sleep_allowed = false;
bool roomeffect_allowed = true;

bool races_allowed = true;
bool evil_races_allowed = false; /* Allows good races only. */

/* do you have to visit your guild to level? */
bool level_gain = true;

/* Show damage amounts to players? */
bool damage_amounts = true;

/* is playerthieving allowed? */
bool pt_allowed = false;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 1;

/* default group level difference, zero is OFF */
bool max_group_difference = false;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 3500;  /* 3500 ticks is approximately 3 real days  */
int short_pc_corpse_time = 100; /* 100 ticks = about 2 real hours */

/* approve new character names being allowing them to enter the game? */
bool approve_names = false;

/* new character name approval causes a pause before entering the game? */
bool napprove_pause = false;

/* should items in death traps automatically be junked? */
bool dts_are_dumps = true;

/* Automatic rebooting */
int reboot_hours_base = 140;     /* Average time to reboot (real hours) */
int reboot_hours_deviation = 30; /* added to or subtracted from reboot_hours_base */
/* between 2.5 and 5 days */

/* How long before rebooting do warnings begin? */
int reboot_warning_minutes = 10;

/* Setting this to false will prevent the mud from rebooting itself */
bool reboot_auto = true;

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

int r_mortal_start_room, r_immort_start_room, r_frozen_start_room;

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

int environment = ENV_PROD; /* 0 = production, 1 = test, 2 = dev */
const std::string_view environments[] = {"PROD", "TEST", "DEV"};

/*
 * This is the default port the game should run on if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the
 * port number there will override this setting.  Change the PORT= line in
 * instead of (or in addition to) changing this.
 */
int DFLT_PORT = 9999;

/* default directory to use as data directory */
const std::string_view DFLT_DIR = "lib";

/* default environment */
const std::string_view DFLT_ENV = "test";

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
 * If your nameserver is fast, set the variable below to false.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to true.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

bool nameserver_is_slow = true;

const std::string_view MENU =
    "\n"
    "   ~~~ Welcome to &1&bFieryMUD&0 ~~~\n"
    "\n"
    "    0) Exit from &1&bFiery&0.\n"
    "    1) Enter the game.\n"
    "    2) Read the background story.\n"
    "    3) Change password.\n"
    "\n"
    "       Make your choice: ";

const std::string_view GREETINGS =
    "\n\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "      .       &3&b ________&0                                             . \n"
    "      .       &3&b/   ___\\_}              ___    _____ ______&0           . \n"
    "      .       &3&b|  |   __  ________     |_ \\  /  | | |  |   \\&0         . \n"
    "      .       &3&b|  |__/  \\/  __ \\ |\\__ / /  \\/   | | |  |    \\&0        . \n"
    "      .       &3&b|  ____\\ |  {_/ /  __ / /        | | |  | |\\  \\&0       . \n"
    "      .       &1&b|  |  |  | {___/  /\\ Y /|  /\\/\\  | | |  | | |  |&0      . \n"
    "      .       &1&b|  |  |  | \\___|  |/  / |  |  |  | \\_/  | L_|  |&0      . \n"
    "      .       &1&b|__|  \\__/_____/__|__/  |__|  |__|_____/______/&0       . \n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "      .      &7&bMud based on: Copper DikuMud I (B.1) by Swiftest&0       . \n"
    "      .    &7&bDikuMud creators: Hans Henrik Staerfeldt, Katja Nyboe,&0   . \n"
    "      .      &7&bTom Madsen, Michael Seifert, and Sebastian Hammer.&0     . \n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "      .          &1&bFor Help or Information: &3&bwww.fierymud.org&0          . \n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "
    "\n"
    "                                                     Build No.";
const std::string_view TEST_GREETING =
    "\n\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "      .      &3&b________         _________________  _______________&0    . \n"
    "      .     &3&b/   ___\\_}        \\____      ____/ \\/    \\     ____/&0    . \n"
    "      .     &3&b|  |   __  ________    \\_   /|  ___/   __/    /&0         . \n"
    "      .     &3&b|  |__/  \\/  __ \\ |\\__ / / | |  |__   /   |  |&0          . \n"
    "      .     &3&b|  ____\\ |  {_/ /  __ / /  | |   __|   \\  |  |&0          . \n"
    "      .     &1&b|  |  |  | {___/  /\\ Y /|  | |  |__ \\   \\ |  |&0          . \n"
    "      .     &1&b|  |  |  | \\___|  |/  / |  | |     \\_\\  \\|  |&0          . \n"
    "      .     &1&b|__|  \\__/_____/__|__/  |__| \\_____/_____/|__|&0          . \n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "      .      Mud based on: Copper DikuMud I (B.1) by Swiftest       . \n"
    "      .    DikuMud creators: Hans Henrik Staerfeldt, Katja Nyboe,   . \n"
    "      .      Tom Madsen, Michael Seifert, and Sebastian Hammer.     . \n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "                                                     Build No.";

const std::string_view DEV_GREETING =
    "\n\n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "      .     &3&b ________               _____   _______ _        ___    &0. \n"
    "      .     &3&b/   _____}             |     \\ |       \\ \\      /  /    &0. \n"
    "      .     &3&b|  |   __  ________    |   _  \\|  _____/  \\    /  /     &0. \n"
    "      .     &3&b|  |__/  \\/  __ \\ |\\__ / /| \\  \\  |__ \\    \\  /  /      &0. \n"
    "      .     &3&b|  ____\\ |  {_/ /  __ / / | /  /   __| \\    \\/  /       &0. \n"
    "      .     &1&b|  |  |  | {___/  /\\ Y /  |/  /|  |____ \\      /        &0. \n"
    "      .     &1&b|  |  |  | \\___|  |/  /|     / |       \\ \\    /         &0. \n"
    "      .     &1&b|__|  \\__/_____/__|__/ |____/  |_______/  \\__/          &0. \n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "      .      Mud based on: Copper DikuMud I (B.1) by Swiftest       . \n"
    "      .    DikuMud creators: Hans Henrik Staerfeldt, Katja Nyboe,   . \n"
    "      .      Tom Madsen, Michael Seifert, and Sebastian Hammer.     . \n"
    "      . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . \n"
    "                                                     Build No.";

const std::string_view WHOAREYOU =
    "\n\n"
    "                        Based on HubisMUD 1.0\n"
    "                       Based on CircleMUD 3.0,\n"
    "                       Created by Jeremy Elson\n"
    "\n"
    "By what name do you wish to be known? ";

const std::string_view WELC_MESSG =
    "\n"
    "Welcome to FieryMUD!\n"
    "\n\n";

const std::string_view START_MESSG =
    "Welcome.  This is your new character! Type help display "
    "and help newbie.\n";

const std::string_view NAMES_EXPLANATION =
    "\n"
    "Our name policy is fairly relaxed, but does have the following "
    "restrictions:\n"
    "\n"
    "1) No dictionary words, or names resembling dictionary words, or\n"
    "   compound words. Examples: BraveBlade, Jesterx, Rex.\n"
    "2) No well-known names whether modern or historical, or from\n"
    "   within Fiery's world.\n"
    "3) Nothing that is deemed offensive or inappropriate by our staff.\n"
    "\n";

const std::string_view BANNEDINTHEUSA =
    "\n"
    "                       IF YOU SEE THIS IT IS BAD!\n"
    "\n"
    "        FFFFFFFFFF  IIIIIII   EEEEEEEEEE   RRRRRR      YYY    YYY  \n"
    "       FFFFFFFFFF    IIIII   EEEEEEEEEE   RRRRRRRRR    YYY    YYY  \n"
    "       FFF            III    EEE          RRR   RRRR   YYY    YYY  \n"
    "       FFFFFF         III    EEEEEE       RRR    RRR    YYYYYYYY   \n"
    "       FFF            III    EEE          RRRRRRRRR         YYY    \n"
    "       FFF           IIIII   EEEEEEEEEE   RRR    RRR       YYY     \n"
    "       FFF          IIIIIII   EEEEEEEEEE  RRRR    RRRR   YYYY      \n"
    "\n"
    " You have been banned from playing on FieryMUD. We are sorry if that is an\n"
    " inconvenience however it has been done with good reason.  Do not attempt\n"
    " to continually reconnect to FieryMUD as all connection will be met with\n"
    " this message. All connections are logged. If we have trouble from a site\n"
    " we can and will contact the administrators of the site or your Internet\n"
    " service provider.\n"
    "                         YOU HAVE BEEN WARNED!\n"
    "\n"
    " We will review the ban list once every six months to determine if any site\n"
    " merits removal from the list.  If you feel you have been unjustly denied\n"
    " access to FieryMUD send mail to gods@fierymud.org.\n";

const std::string_view NEWSUPDATED =
    "&2 __    __                                            &0\n"
    "&2|  \\  |  \\    &2&bTHE                              &0\n"
    "&2| $$\\ | $$  ______   __   __   __   _______         &0\n"
    "&2| $$$\\| $$ /      \\ |  \\ |  \\ |  \\ /       \\   &0\n"
    "&2| $$$$\\ $$|  $$$$$$\\| $$ | $$ | $$|  $$$$$$$       &0\n"
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
/*  int use_autowiz = false; */

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
/*  int min_wizlist_lev = LVL_GOD; */

/***************************************************************************
 * $Id: structs.h,v 1.205 2010/06/09 22:32:01 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: structs.h                                      Part of FieryMUD *
 *  Usage: header file for central structures and contstants               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_STRUCTS_H
#define __FIERY_STRUCTS_H

#include "spell_mem.h"
#include "specprocs.h"  /* For SPECIAL() */
#include "money.h"      /* For NUM_COIN_TYPES */
#include "prefs.h"      /* For NUM_PRF_FLAGS */

/* char and mob-related defines *****************************************/

/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2
#define NUM_SEXES     3


/* Positions */
#define POS_PRONE       0
#define POS_SITTING     1
#define POS_KNEELING    2
#define POS_STANDING    3
#define POS_FLYING      4
#define NUM_POSITIONS   5

/* Stances */
#define STANCE_DEAD       0        /* dead                 */
#define STANCE_MORT       1        /* mortally wounded     */
#define STANCE_INCAP      2        /* incapacitated        */
#define STANCE_STUNNED    3        /* stunned              */
#define STANCE_SLEEPING   4        /* sleeping             */
#define STANCE_RESTING    5        /* resting              */
#define STANCE_ALERT      6        /* alert                */
#define STANCE_FIGHTING   7        /* fighting             */
#define NUM_STANCES       8

#define HIT_INCAP      -3        /* The hit level for incapacitation   */
#define HIT_MORTALLYW  -6        /* The hit level for mortally wound   */
#define HIT_DEAD       -11       /* The point you never want to get to */

#define DAMAGE_WILL_KILL(ch, dmg) (GET_HIT(ch) - dmg <= HIT_DEAD)


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER      0   /* a player-killer                           */
#define PLR_THIEF       1   /* a player-thief                            */
#define PLR_FROZEN      2   /* is frozen                                 */
#define PLR_DONTSET     3   /* Don't EVER set (ISNPC bit)                */
#define PLR_WRITING     4   /* writing (board/mail/olc)                  */
#define PLR_MAILING     5   /* is writing mail                           */
#define PLR_AUTOSAVE    6   /* needs to be autosaved                     */
#define PLR_SITEOK      7   /* has been site-cleared                     */
#define PLR_NOSHOUT     8   /* not allowed to shout/goss                 */
#define PLR_NOTITLE     9   /* not allowed to set title       (not used) */
#define PLR_DELETED    10   /* deleted - space reusable       (not used) */
#define PLR_LOADROOM   11   /* uses nonstandard loadroom      (not used) */
#define PLR_NOWIZLIST  12   /* shouldn't be on wizlist        (not used) */
#define PLR_NODELETE   13   /* shouldn't be deleted           (may be used outside the server) */
#define PLR_INVSTART   14   /* should enter game wizinvis     (not used) */
#define PLR_CRYO       15   /* is cryo-saved (purge prog)     (not used) */
#define PLR_MEDITATE   16   /* meditating - improves spell memorization  */
#define PLR_CASTING    17   /* currently casting a spell      (not used) */
#define PLR_BOUND      18   /* tied up                        (not used) */
#define PLR_SCRIBE     19   /* scribing                       (not used) */
#define PLR_TEACHING   20   /* teaching a skill/spell         (not used) */
#define PLR_NAPPROVE   21   /* name not approved yet                     */
#define PLR_NEWNAME    22   /* needs to choose a new name                */
#define PLR_REMOVING   23   /* player is being removed and doesn't need emergency save */
#define PLR_SAVING     24   /* player is being saved to file and effect changes are not relevant */
#define PLR_GOTSTARS   25   /* player has achieved ** already            */
#define NUM_PLR_FLAGS  26

/* Privilege flags: used by char_data.player_specials.privileges */
#define PRV_CLAN_ADMIN  0   /* clan administrator */
#define PRV_TITLE       1   /* can change own title */
#define PRV_ANON_TOGGLE 2   /* can toggle anon */
#define PRV_AUTO_GAIN   3   /* don't need to level gain */
#define NUM_PRV_FLAGS   4

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC              0  /* Mob has a callable spec-proc       */
#define MOB_SENTINEL          1  /* Mob should not move                */
#define MOB_SCAVENGER         2  /* Mob picks up stuff on the ground   */
#define MOB_ISNPC             3  /* (R) Automatically set on all Mobs  */
#define MOB_AWARE             4  /* Mob can't be backstabbed           */
#define MOB_AGGRESSIVE        5  /* Mob hits players in the room       */
#define MOB_STAY_ZONE         6  /* Mob shouldn't wander out of zone   */
#define MOB_WIMPY             7  /* Mob flees if severely injured      */
#define MOB_AGGR_EVIL         8  /* auto attack evil PC's              */
#define MOB_AGGR_GOOD         9  /* auto attack good PC's              */
#define MOB_AGGR_NEUTRAL     10  /* auto attack neutral PC's           */
#define MOB_MEMORY           11  /* remember attackers if attacked     */
#define MOB_HELPER           12  /* attack PCs fighting other NPCs     */
#define MOB_NOCHARM          13  /* Mob can't be charmed               */
#define MOB_NOSUMMON         14  /* Mob can't be summoned              */
#define MOB_NOSLEEP          15  /* Mob can't be slept                 */
#define MOB_NOBASH           16  /* Mob can't be bashed (e.g. trees)   */
#define MOB_NOBLIND          17  /* Mob can't be blinded               */
#define MOB_MOUNTABLE        18
#define MOB_NO_EQ_RESTRICT   19
#define MOB_FAST_TRACK       20
#define MOB_SLOW_TRACK       21
#define MOB_CASTING          22  /* mob casting            (not used)  */
#define MOB_SUMMONED_MOUNT   23  /* resets CD_SUMMON_MOUNT when extracted */
#define MOB_AQUATIC          24  /* Mob can't enter non-water rooms    */
#define MOB_AGGR_EVIL_RACE   25
#define MOB_AGGR_GOOD_RACE   26
#define MOB_NOSILENCE        27
#define MOB_NOVICIOUS        28
#define MOB_TEACHER          29
#define MOB_ANIMATED         30  /* mob is animated - die if no anim effect */
#define MOB_PEACEFUL         31  /* mob can't be attacked.             */
#define MOB_NOPOISON         32  /* Mob cannot be poisoned.            */
#define MOB_ILLUSORY         33  /* is an illusion: does no harm, leaves no corpse */
#define MOB_PLAYER_PHANTASM  34  /* illusion of player; mobs are aggro to */
#define MOB_NO_CLASS_AI      35  /* Mob does not execute class AI      */
#define MOB_NOSCRIPT         36  /* Mob does not execute triggers or specprocs */
#define MOB_PEACEKEEPER      37  /* Attacks mobs with over 1350 align diff. Assists other PEACEKEEPERs */
#define MOB_PROTECTOR        38  /* Assists players under attack, but not against PEACEKEEPER/PROTECTOR mobs */
#define NUM_MOB_FLAGS        39  /* Update this when you add a flag! */

/* Some mount stuff */
#define MAX_MOUNT_LEVEL    27   /* The maximum level of mountable mobs */

/* mount_level_fudge is how far above the ideal mountable level you could possibly
 * hope to mount.  For example if your skill allowed you to mount level 10 mobs
 * easily, and mount_level_fudge was 4, you could mount level 14 mobs with great
 * difficulty, but nothing higher. */
#define MOUNT_LEVEL_FUDGE  (double)(MAX_MOUNT_LEVEL / 6)

#define MOUNT_MINMOVE 20
#define MOUNT_MAXMOVE 250

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING         0                /* Playing - Nominal state         */
#define CON_CLOSE           1                /* Disconnecting                   */
#define CON_GET_NAME        2                /* By what name ..?                */
#define CON_NAME_CNFRM      3                /* Did I get that right, x?        */
#define CON_PASSWORD        4                /* Password:                       */
#define CON_NEWPASSWD       5                /* Give me a password for x        */
#define CON_CNFPASSWD       6                /* Please retype password:         */
#define CON_QSEX            7                /* Sex?                            */
#define CON_QCLASS          8                /* Class?                          */
#define CON_RMOTD           9                /* PRESS RETURN after MOTD         */
#define CON_MENU           10                /* Your choice: (main menu)        */
#define CON_EXDESC         11                /* Enter a new description:        */
#define CON_CHPWD_GETOLD   12                /* Changing passwd: get old        */
#define CON_CHPWD_GETNEW   13                /* Changing passwd: get new        */
#define CON_CHPWD_VRFY     14                /* Verify new password             */
#define CON_DELCNF1        15                /* Delete confirmation 1           */
#define CON_DELCNF2        16                /* Delete confirmation 2           */
#define CON_QRACE          17                /* Complete Race Seletion Menu     */
#define CON_QANSI          18                /* Prompt for term type            */
#define CON_OEDIT          19                /*. OLC mode - object edit        .*/
#define CON_REDIT          20                /*. OLC mode - room edit          .*/
#define CON_ZEDIT          21                /*. OLC mode - zone info edit     .*/
#define CON_MEDIT          22                /*. OLC mode - mobile edit        .*/
#define CON_SEDIT          23                /*. OLC mode - shop edit          .*/
#define CON_QROLLSTATS     24
#define CON_QHOMETOWN      25
#define CON_QBONUS1        26
#define CON_QBONUS2        27
#define CON_QBONUS3        28
#define CON_QCANCHAR       29
#define CON_HEDIT          30                /*. OLC mode - help edit          .*/
#define CON_TRIGEDIT       31                /*. OLC mode - trigger edit       .*/
#define CON_CLASSHELP      32                /* Char Gen Class Help             */
#define CON_SDEDIT         33
#define CON_NAME_CHECK     34
#define CON_NAME_WAIT_APPROVAL 35            /* await imm aprroval of name      */
#define CON_NEW_NAME       36                /* name declined, get a new one    */
#define CON_QGOODRACE      37                /* Menu Choice for Good races.     */
#define CON_ISPELL_BOOT    38                /* Obligatory disconnect bad names */
#define CON_GEDIT          39                /* OLC mode - grant group edit */
#define CON_IEDIT          40                /*. OLC mode - iobject edit        .*/
#define NUM_CON_MODES      41


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_WIELD2    17
#define WEAR_HOLD      18
#define WEAR_HOLD2     19
#define WEAR_2HWIELD   20
#define WEAR_EYES      21
#define WEAR_FACE      22
#define WEAR_LEAR      23
#define WEAR_REAR      24
#define WEAR_BADGE     25
#define WEAR_OBELT     26
#define NUM_WEARS      27        /* This must be the # of eq positions!! */


/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2

/* Levels of rage */
#define RAGE_NONE         0
#define RAGE_ANNOYED    250
#define RAGE_ANGRY      500
#define RAGE_IRATE      750
#define RAGE_CRAZED    1000


/*
 * Weather data used by utils.h
 */
/* Sun state */
#define SUN_DARK        0
#define SUN_RISE        1
#define SUN_LIGHT       2
#define SUN_SET         3

#define HEMISPHERE_NORTHWEST        0
#define HEMISPHERE_NORTHEAST        1
#define HEMISPHERE_SOUTHWEST        2
#define HEMISPHERE_SOUTHEAST        3
#define NUM_HEMISPHERES             4


/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 */
#define LVL_IMPL       105 /* Overlord */
#define LVL_OVERLORD   105 /* Overlord */
#define LVL_HEAD_C     104 /* Implementer */
#define LVL_HEAD_B     103 /* Greater God */
#define LVL_GRGOD      102 /* Lesser God */
#define LVL_GOD        101 /* Demi-God */
#define LVL_IMMORT     100 /* Avatar */
#define LVL_MAX_MORT    99 /* Mortal */

#ifdef PRODUCTION
  #define LVL_ADMIN       104
  #define LVL_BUILDER     104
  #define LVL_GAMEMASTER  103
  #define LVL_ATTENDANT   102
  #define LVL_RESTORE     103
  #define LVL_PURGE       103
#else
  #define LVL_ADMIN       103
  #define LVL_BUILDER     101
  #define LVL_GAMEMASTER  102
  #define LVL_ATTENDANT   101
  #define LVL_RESTORE       1
  #define LVL_PURGE       101
#endif

#define LVL_FREEZE        LVL_GRGOD

/* Level required for rebooting, shutdown, autoboot */

#ifdef PRODUCTION
   /* Full use of "autoboot" and "shutdown" commands.
    * You can cancel/enable/set time of automatic reboot,
    * or (obviously) use shutdown to stop or restart the mud immediately */
   #define LVL_REBOOT_MASTER LVL_HEAD_B
   /* You can stop a reboot for a while with "autoboot postpone" */
   #define LVL_REBOOT_POSTPONE  LVL_GOD
   /* You can get the time of the next automatic reboot with "autoboot" or "world" */
   #define LVL_REBOOT_VIEW      LVL_GOD
#else
   #define LVL_REBOOT_MASTER    LVL_HEAD_B
   #define LVL_REBOOT_POSTPONE  LVL_IMMORT
   #define LVL_REBOOT_VIEW      1
#endif

#define LVL_GOSSIP        1

   /* Reasons why should_restrict > 0 */
#define RESTRICT_NONE      0  /* No restriction, or don't know */
#define RESTRICT_ARGUMENT  1  /* Mud started with -r option */
#define RESTRICT_MANUAL    2  /* Set by a god with the wizlock command */
#define RESTRICT_AUTOBOOT  3  /* Set automatically due to imminent automatic reboot */

/* damage() return codes */
#define VICTIM_DEAD        (1 << 30)

#define MAX_SKILL_TIMERS 100

/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset players
 * as actions (such as large speedwalking chains) take longer to execute.
 *
 * RL_SEC is used with other macros and constants to define how often
 * heartbeats an action in the main game loop gets executed.  Helps to
 * translate pulse counts to real seconds for human comprehension.
 */
#define OPT_USEC        100000        /* 10 passes per second */
#define PASSES_PER_SEC  (1000000 / OPT_USEC)
#define RL_SEC          * PASSES_PER_SEC
#define MUD_HR          * SECS_PER_MUD_HOUR * PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  (4 RL_SEC) /* changed to 4 seconds from 2 7/14/99 */
#define PULSE_AUTOSAVE  (60 RL_SEC)

#define SMALL_BUFSIZE   1024
#define LARGE_BUFSIZE   (48 * 1024)
#define GARBAGE_SPACE   32

#define MAX_STRING_LENGTH     20000
#define MAX_STRING_LENGTH_BIG 24000
#define MAX_DESC_LENGTH       8096       /* object action desc length */
#define MAX_INPUT_LENGTH      256        /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH  512        /* Max size of *raw* input */
#define MAX_MESSAGES          150
#define MAX_NAME_LENGTH        20
#define MAX_PWD_LENGTH         10
#define MAX_TITLE_LENGTH       80
#define HOST_LENGTH            30

#define PLAYER_DESC_LENGTH   1200
#define PLAYER_DESC_LINES      15
#define IMMORT_DESC_LENGTH  10000
#define IMMORT_DESC_LINES      50

#define MAX_TONGUE              3
#define TOP_SKILL             650
#define MAX_EFFECT             32
#define PAGE_SCRIBE_TIME        1  /* this is the time per page to scribe */
#define MAX_DAMAGE           1000  /* Maximum per hit allowed by damage() */
#define EVENT_FLAG_FIELDS       4  /* Number of fields for event flags */
#define MIN_ABILITY_VALUE      25  /* Min for stuff like str/cha */
#define MAX_ABILITY_VALUE     100  /* Max for stuff like str/cha */

/* ***** Cooldowns ***** */
#define CD_BACKSTAB             0
#define CD_BASH                 1
#define CD_INSTANT_KILL         2
#define CD_DISARM               3        /* pulse violence */
#define CD_FUMBLING_PRIMARY     4        /* pulse violence */
#define CD_DROPPED_PRIMARY      5        /* pulse violence */
#define CD_FUMBLING_SECONDARY   6        /* pulse violence */
#define CD_DROPPED_SECONDARY    7        /* pulse violence */
#define CD_SUMMON_MOUNT         8
#define CD_LAY_HANDS            9
#define CD_FIRST_AID           10
#define CD_EYE_GOUGE           11
#define CD_THROATCUT           12
#define CD_SHAPECHANGE         13
#define CD_CHANT               14
#define CD_INNATE_INVISIBLE    15
#define CD_INNATE_STRENGTH     16
#define CD_INNATE_DARKNESS     17
#define CD_INNATE_LEVITATE     18
#define NUM_COOLDOWNS          19

/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char            sbyte;
typedef unsigned char          ubyte;
typedef signed short int       sh_int;
typedef unsigned short int     ush_int;
typedef struct witness_data    wtns_rec;

#ifndef CIRCLE_WINDOWS
typedef char                   byte;
#endif

typedef int                    room_num;
typedef int                    obj_num;
typedef int                    zone_vnum;

#define NOWHERE    -1    /* nil reference for room-database        */
#define NOTHING    -1    /* nil reference for objects              */
#define NOBODY     -1    /* nil reference for mobiles              */

/* exits.h depends on obj_num */
#include "exits.h"

/** Bitvector type for 32 bit unsigned long bitvectors. 'unsigned long long'
 * will give you at least 64 bits if you have GCC. You'll have to search
 * throughout the code for "bitvector_t" and change them yourself if you'd
 * like this extra flexibility. */
typedef unsigned long int      flagvector;
#define FLAGBLOCK_SIZE         ((flagvector) 8 * sizeof(flagvector)) /* 8 bits = 1 byte */
#define FLAGVECTOR_SIZE(flags)	(((flags) - 1) / FLAGBLOCK_SIZE + 1)

#include "objects.h"

/* hemispheres for weather and time NE< NW< SE< SW */
struct hemisphere_data
{
  char *name;
  int season;
  int sunlight;
};

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char        *keyword;                 /* Keyword in look/examine          */
   char        *description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};

struct casting
{
        int spell;
        int casting_time;
        struct char_data *tch;  /* set up the targets */
        struct obj_data *obj;
        char *misc;
        int target_status;
};

struct spell_dam {
    char *note;
    sh_int spell;
    sh_int intern_dam;
    sh_int npc_static;
    sh_int npc_no_dice;
    sh_int npc_no_face;
    sh_int pc_static;
    sh_int pc_no_dice;
    sh_int pc_no_face;
    sh_int npc_reduce_factor;
    sh_int use_bonus;
    sh_int max_bonus;
    sh_int lvl_mult;
};
/* ======================================================================= */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long        id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int    played;     /* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data {
  char passwd[MAX_PWD_LENGTH+1]; /* character's password      */
  char *namelist;     /* PC / NPC s name (kill ...  )         */
  char *short_descr;  /* for NPC 'actions'                    */
  char *long_descr;   /* for NPC 'look'                       */
  char *description;  /* Extra descriptions                   */
  char *title;        /* PC / NPC's title                     */
  char *prompt;       /* Player prompt                        */
  ush_int sex;        /* PC / NPC's sex                       */
  ush_int class;      /* PC / NPC's class                     */
  ush_int race;       /* PC / NPC's race                      */
  ush_int race_align; /* PC / NPC's race_align                */
  ush_int level;      /* PC / NPC's level                     */
  int lifeforce;      /* What empowers it - see LIFE_* in chars.h */
  int base_composition;
  int composition;    /* What its body is made of - see COMP_* in chars.h */
  room_num homeroom;  /* PC s Homeroom                        */


  struct time_data time;  /* PC's AGE in days                 */

  /* size/height/weight */
  int weight;
  int height;
  int affected_size;
  int mod_size;
  int natural_size;
  int base_weight;
  int base_height;
  int base_size;
};


/* Char's abilities. */
struct char_ability_data {
   int str;
   int intel;
   int wis;
   int dex;
   int con;
   int cha;
};


/* Char's points. */
struct char_point_data {
   int mana;
   int max_mana;               /* Max move for PC/NPC                     */
   int hit;
   int max_hit;                /* Max hit for PC/NPC                      */
   int move;
   int max_move;               /* Max move for PC/NPC                     */
   int armor;                  /* Internal -100..100, external -10..10 AC */
   int coins[NUM_COIN_TYPES];
   int bank[NUM_COIN_TYPES];
   long exp;                   /* The experience of the player            */

   int base_hitroll;           /* Any bonus or penalty to the hit roll    */
   int hitroll;                /* Value used for calculations and viewing */
   int base_damroll;           /* Any bonus or penalty to the damage roll */
   int damroll;                /* Value used for calculations and viewing */
};

struct alias_data {
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

/* Special playing constants shared by PCs and NPCs */
struct char_special_data {
   struct char_data *hunting;        /* Char hunted by this char                */
   struct char_data *riding;
   struct char_data *ridden_by;
   struct char_data *consented;
   int position;                     /* Prone, Sitting, Standing, etc.        */
   int stance;                       /* Sleeping, Alert, Fighting, etc.       */

   int cooldowns[NUM_COOLDOWNS][2];  /* Skill/action cooldowns (current/max)  */

   float carry_weight;               /* Carried weight                        */
   int carry_items;                  /* Number of items carried               */
   int timer;                        /* Inactivity timer for players          */
   int hitgain;                      /* Bonus hit regen, from APPLY_HIT_REGEN */
   int managain;                     /* Bonus mana regen, from APPLY_MANA_REGEN */
   int rage;                         /* For berserking                        */

   int   alignment;                  /* +/- 1000 for alignment                */
   long  idnum;                      /* player's idnum; -1 for mobiles        */
   /* act flag for NPC; player flag for PC  */
   flagvector  act[FLAGVECTOR_SIZE(NUM_MOB_FLAGS > NUM_PLR_FLAGS ? NUM_MOB_FLAGS : NUM_PLR_FLAGS)];

   long  perception;
   long  hiddenness;

   /* Bitvectors for spells/skills effects  */
   flagvector effects[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
   sh_int apply_saving_throw[5];     /* Saving throw (Bonuses)                */

   sh_int skills[TOP_SKILL+1];       /* array of skills plus skill 0          */

   /* These keep track of how much a player has been speaking (gossipping,
    * telling, whatever else is deemed appropriate) and is used to decide
    * whether to automatically quiet the player. */
   double speech_rate;
   long last_speech_time;            /* Taken from global_pulse               */

   int quit_reason;                  /* How or why you left the game          */
};


struct olc_zone_list {
  zone_vnum zone;
  struct olc_zone_list *next;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs.  This structure
 * can be changed freely.
 */
struct player_special_data {
  long last_tell;                /* idnum of last tell from                */
  void *last_olc_targ;           /* olc control                            */
  int last_olc_mode;             /* olc control                            */
  byte roll[6];                  /* for rolling stats in player creation   */
  struct char_data *ignored;

 /* List of the last X comms as defined in retained_comms.h */
  struct retained_comms *comms;
  bool talks[MAX_TONGUE];        /* PC s Tongues 0 for NPC                 */
  int wimp_level;                /* Below this # of hit points, flee!      */
  int aggressive;                /* Above this # of hit points, autoattack */
  byte freeze_level;             /* Level of god who froze char, if any    */
  byte autoinvis_level;          /* Level of invisibility to take when entering game */
  byte invis_level;              /* level of invisibility                  */
  room_num load_room;            /* Which room to place char in            */
  room_num save_room;            /* Where the player was when saved        */
  /* preference flags for PC's.             */
  flagvector pref[FLAGVECTOR_SIZE(NUM_PRF_FLAGS)];
  /* privilege flags for PC's */
  flagvector privileges[FLAGVECTOR_SIZE(NUM_PRV_FLAGS)];
  ubyte bad_pws;                 /* number of bad password attempts        */
  sbyte conditions[3];           /* Drunk, full, thirsty                   */
  struct trophy_node *trophy;
  struct alias_data *aliases;

  flagvector *grant_cache;       /* cache of granted commands              */
  flagvector *revoke_cache;      /* cache of revoked commands              */
  struct grant_type *grants;     /* Commands granted to this player        */
  struct grant_type *revokes;    /* Commands revoked from this player      */
  /* Groups of commands granted to this player */
  struct grant_type *grant_groups;
  /* Groups of commands revoked from this player */
  struct grant_type *revoke_groups;

  ubyte page_length;
  struct clan_membership *clan;
  struct clan_snoop *clan_snoop;
  struct olc_zone_list *olc_zones;
  int lastlevel;
  int base_hit;
  int log_view;                  /* Level of syslog displayed              */
  char *poofin;
  char *poofout;
  char **perm_titles;
  char *long_descr;
  char *wiz_title;
  char *host;
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
  long nr;                 /* Mob's rnum                              */
  int ex_platinum;
  int ex_gold;
  sbyte ex_face;
  int ex_no_dice;
  int zone;
  sbyte ex_damroll;
  sbyte ex_hitroll;
  sbyte ex_hpnumdice;
  sbyte ex_hpsizedice;
  int ex_main_hp;
  long ex_exp;
  byte class;
  sh_int ex_hit;
  sh_int ex_max_hit;
  byte last_direction;     /* The last direction the monster went     */
  int        attack_type;  /* The Attack Type Bitvector for NPC's     */
  byte default_pos;        /* Default position for NPC                */
  memory_rec *memory;      /* List of attackers to remember           */
  sbyte damnodice;         /* The number of damage dice's             */
  sbyte damsizedice;       /* The size of the damage dice's           */
  sbyte ex_damnodice;
  sbyte ex_damsizedice;
  int wait_state;          /* Wait state for bashed mobs              */
  int spell_bank[NUM_SPELL_CIRCLES + 1]; /* circle 0 is unused */
  int spell_mem_time;
  sh_int ex_armor;
  long mob2_flags;
};

/* An effect structure. */
struct effect {
   int type;           /* The type of spell that caused this       */
   int duration;       /* For how long its effects will last       */
   int modifier;       /* This is added to apropriate ability      */
   byte location;      /* Tells which ability to change(APPLY_XXX) */
   /* Tells which flags to set (EFF_XXX) */
   flagvector flags[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];

   struct effect *next;
};


/* Structure used for chars following other chars */
struct follow_type {
  struct char_data *follower;
  bool can_see_master;
  struct follow_type *next;
};

struct know_spell {
  struct char_data *sch;
  struct know_spell *next;
};

/* ================== Structure for player/non-player ===================== */
struct char_data {
    /* Character stuff */
  long id;                                     /* Global unique ID used by DG triggers */
  room_num in_room;                            /* Location (real room number) */
  room_num was_in_room;                        /* location for linkdead people */
  struct char_ability_data natural_abils;      /* natural rolls */
  struct char_ability_data actual_abils;       /* natural_abils + effects */
  struct char_ability_data affected_abils;     /* viewed_abils * racial percentage */
  struct char_point_data points;               /* Points */
  struct effect *effects;                      /* effected by what spells */
  struct obj_data *equipment[NUM_WEARS];       /* Equipment array */
  struct obj_data *carrying;                   /* Head of list */
  struct know_spell *see_spell;                /* list of chars that guessed caster's spell */
    /* Other characters */
  struct char_data *forward;                   /* for shapechange/switch */
  struct char_data *next_in_room;              /* For room->people - list */
  struct char_data *next;                      /* For either monster or ppl-list */
  struct char_data *guarded_by;                /* Character guarding this char */
  struct char_data *guarding;                  /* Char this char is guarding */
  struct char_data *cornered_by;               /* Char preventing this char from fleeing */
  struct char_data *cornering;                 /* Char this char is preventing from fleeing*/
  struct follow_type *followers;               /* List of chars followers */
  struct char_data *master;                    /* Who is char following? */
  struct group_type *groupees;                 /* list of chars grouped */
  struct char_data *group_master;              /* group master */
  struct char_data *next_caster;               /* A list of casters I'm in */
  struct char_data *casters;                   /* Chars who are casting spells at me */
    /* Battling */
  struct char_data *next_fighting;             /* Part of list of all fighting characters in mud */
  struct char_data *target;                    /* Who I'm fighting */
  struct char_data *attackers;                 /* List of characters who are fighting me */
  struct char_data *next_attacker;             /* List of fighting characters I'm in */
    /* Player stuff */
  int pfilepos;                                /* playerfile pos */
  struct quest_list *quests;
    /* Spell mem/scribe stuff */
  struct spell_memory spell_memory;
  struct scribing *scribe_list;                /* spells queued for scribing */
    /* Mobile stuff */
  struct trig_proto_list *proto_script;        /* list of default triggers */
  struct script_data *script;                  /* script info for the object */
    /* Substructs of varying application */
  struct char_player_data player;              /* Normal data */
  struct char_special_data char_specials;      /* PC/NPC specials */
  struct player_special_data *player_specials; /* PC specials */
  struct mob_special_data mob_specials;        /* NPC specials  */
  struct descriptor_data *desc;                /* NULL for mobiles */
    /* Events */
  struct casting casting;                      /* note this is NOT a pointer */
  struct event *events;                        /* List of events related to this character */
  int event_flags[EVENT_FLAG_FIELDS];          /* Bitfield of events active on this character */
};

/* ====================================================================== */



/* descriptor-related structures ******************************************/


struct txt_block {
   char        *text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct paging_line {
   char *line;
   struct paging_line *next;
};


struct descriptor_data {
   socket_t        descriptor;         /* file descriptor for socket            */
   char        host[HOST_LENGTH+1];    /* hostname                              */
   byte        bad_pws;                /* number of bad pw attemps this login   */
   byte idle_tics;                     /* tics idle at password prompt          */
   int        connected;               /* mode of 'connectedness'               */
   int        wait;                    /* wait for how many loops               */
   int        desc_num;                /* unique num assigned to desc           */
   time_t login_time;                  /* when the person connected             */
   int mail_vnum;
   int        prompt_mode;             /* control of prompt-printing            */
   char        inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input            */
   char        last_input[MAX_INPUT_LENGTH]; /* the last input                  */
   char small_outbuf[SMALL_BUFSIZE];   /* standard output buffer                */
   char *output;                       /* ptr to the current output buffer      */
   int  bufptr;                        /* ptr to end of current output          */
   int        bufspace;                /* space left in the output buffer       */
   struct txt_block *large_outbuf;     /* ptr to large buffer, if we need it    */
   struct txt_q input;                 /* q of unprocessed input                */
   struct char_data *character;        /* linked to char                        */
   struct char_data *original;         /* original char if switched             */
   struct descriptor_data *snooping;   /* Who is this char snooping             */
   struct descriptor_data *snoop_by;   /* And who is snooping this char         */
   struct descriptor_data *next;       /* link to next descriptor               */
   struct olc_data *olc;
   char *storage;

   /* Editing a buffer */
   char    **str;                      /* for the modify-str system             */
   char    *backstr;                   /* added for handling abort buffers      */
   size_t  max_str;                    /*                -                      */
   long    mail_to;                    /* name for mail system                  */
   int     max_buffer_lines;           /* limitation on the number of lines     */

   struct editor_data *editor;

   /* The pager */
   struct paging_line *paging_lines;   /* The text that is to be paged through  */
   struct paging_line *paging_tail;    /* End of the list of lines              */
   char *paging_fragment;              /* Intermediate leftover string          */
   int paging_numlines;                /* Number of lines in the list           */
   int paging_numpages;                /* Number of pages to be paged through   */
   int paging_curpage;                 /* The page which is currently showing   */
   int paging_bufsize;                 /* Amount of memory currently used       */
   int paging_skipped;                 /* Number of lines discarded due to overflow */
};

/* other miscellaneous structures ***************************************/


struct msg_type {
   char        *attacker_msg;  /* message to attacker */
   char        *victim_msg;    /* message to victim   */
   char        *room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;        /* messages when death             */
   struct msg_type miss_msg;       /* messages when miss              */
   struct msg_type hit_msg;        /* messages when hit               */
   struct msg_type god_msg;        /* messages when hit on god        */
   struct msg_type heal_msg;       /* message when healing            */
   struct message_type *next;      /* to next messages of this kind.  */
};


struct message_list {
   int        a_type;               /* Attack type                                */
   int        number_of_attacks;    /* How many attack messages to chose from. */
   struct message_type *msg;        /* List of messages.                        */
};


struct group_type {
        struct char_data *groupee;
        struct group_type *next;
};

struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
};

struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
};


struct str_app_type {
   sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   sh_int todam;    /* Damage Bonus/Penalty                */
   sh_int carry_w;  /* Maximum weight that can be carrried */
   sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
   byte bonus;       /* how many practices player gains per lev */
};


struct int_app_type {
   byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
   sh_int hitp;
   sh_int shock;
};


struct weather_data {
   int        pressure;        /* How is the pressure ( Mb ) */
   int        change;        /* How fast and what way does it change. */
   int        sky;        /* How is the sky. */
   int        sunlight;        /* And how much sun. */
};

/* element in monster and object index-tables   */
struct index_data {
   int        virtual;    /* virtual number of this mob/obj           */
   int        number;     /* number of existing units of this mob/obj        */
   SPECIAL(*func);
   char *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};

struct camp_event {
     struct char_data *ch;
     int was_in;
   };


#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

#endif

/***************************************************************************
 * $Log: structs.h,v $
 * Revision 1.205  2010/06/09 22:32:01  mud
 * Moving toggle command and prf flags into prefs.[ch]
 *
 * Revision 1.204  2010/06/05 18:35:47  mud
 * Make pyre auto-target caster if sacrificial preference is
 * toggled on.
 *
 * Revision 1.203  2009/08/02 20:20:38  myc
 * Had to up the MAX_MESSAGE limit for new spells.
 *
 * Revision 1.202  2009/07/17 00:48:17  myc
 * Added anon toggle and auto gain privileges.
 *
 * Revision 1.201  2009/06/09 05:49:19  myc
 * Renaming NOCTELL to NOCLANCOMM so it covers other clan communication.
 * Adding privilege flags.  Removing clan desc editing connection
 * state.  Modifying character clan references to match new
 * interface.
 *
 * Revision 1.200  2009/03/21 19:11:37  myc
 * Save the duration each cooldown started at.
 *
 * Revision 1.199  2009/03/20 23:02:59  myc
 * Remove text editor connection state.
 *
 * Revision 1.198  2009/03/20 20:19:51  myc
 * Add MAX_DESC_LENGTH since MAX_MSG_LENGTH from boards.h is
 * no longer available.
 *
 * Revision 1.197  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.196  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.195  2009/03/09 05:09:22  jps
 * Moved effect flags and strings into effects.h and effects.c.
 *
 * Revision 1.194  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.193  2009/03/09 03:45:17  jps
 * Extract some spell-mem related stuff from structs.h and put it in spell_mem.h
 *
 * Revision 1.192  2009/03/08 03:54:21  jps
 * Update the comments of some structs and format code
 *
 * Revision 1.191  2009/03/08 02:17:46  jps
 * Delete jail event struct.
 *
 * Revision 1.190  2009/03/07 22:27:10  jps
 * Add effect flag remote_aggr, which keeps your aggressive action from
 * removing things like invis. Useful for those spells that keep on hurting.
 *
 * Revision 1.189  2009/02/11 17:03:39  myc
 * Reduce IMMORT_DESC_LENGTH from 40k to 10k characters.
 *
 * Revision 1.188  2009/01/19 09:25:23  myc
 * Replacing MOB_PET flag with MOB_SUMMONED_MOUNT flag, which
 * tracks a mount summoned using the skill, so proper cooldowns
 * can be set.
 *
 * Revision 1.187  2008/09/24 05:49:22  jps
 * Made autoboot control level 103
 *
 * Revision 1.186  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.185  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.184  2008/09/21 04:54:23  myc
 * Added grant caches to the player structure to make can_use_command
 * take less execution time.
 *
 * Revision 1.183  2008/09/07 20:06:47  jps
 * Added flag PLR_GOTSTARS which means you have achieved ** at least once.
 *
 * Revision 1.182  2008/09/07 01:30:07  jps
 * Add a flag for player saving, so that effect changes in the midst of it
 * can be ignored.
 *
 * Revision 1.181  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.180  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.179  2008/08/31 21:44:03  jps
 * Renamed StackObjs and StackMobs prefs to ExpandObjs and ExpandMobs.
 *
 * Revision 1.178  2008/08/31 20:55:40  jps
 * Added PROTECTOR and PEACEKEEPER mob flags.
 *
 * Revision 1.177  2008/08/30 20:21:39  jps
 * Added flag MOB_NO_EQ_RESTRICT, which allows a mobile to wear equipment
 * without regard to align, class, or level restrictions.
 *
 * Revision 1.176  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.175  2008/08/29 19:25:08  myc
 * Removed some unused members from the character structure, and made
 * some sbytes into ints.
 *
 * Revision 1.174  2008/08/29 19:18:05  myc
 * Fixed abilities so that no information is lost; the caps occur
 * only when the viewed stats are accessed.
 *
 * Revision 1.173  2008/08/29 04:16:26  myc
 * Added toggles for stacking objects and mobiles in lists.
 *
 * Revision 1.172  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.171  2008/08/16 08:22:41  jps
 * Added the 'desc' command and took player description-editing out of the pre-game menu.
 *
 * Revision 1.170  2008/08/15 03:59:08  jps
 * Added pprintf for paging, and changed page_string to take a character.
 *
 * Revision 1.169  2008/08/14 23:10:35  myc
 * Added immortal log severity view preference to player structure.
 *
 * Revision 1.168  2008/08/14 15:40:29  jps
 * Added pager buffer size limits.
 *
 * Revision 1.167  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.166  2008/08/13 05:52:51  jps
 * Moved laryngitis variables so that NPCs can be affected too.
 *
 * Revision 1.165  2008/07/27 05:23:45  jps
 * Added a flag to put on players when extracting so they don't get saved twice.
 *
 * Revision 1.164  2008/07/22 07:25:26  myc
 * Added basic iedit (unique item editor) functionality.
 *
 * Revision 1.163  2008/07/15 17:55:06  myc
 * Added grants and grant groups to player structure, as well
 * as a connection mode for gedit.
 *
 * Revision 1.162  2008/06/21 17:27:18  jps
 * Changed several player struct elements to unsigned ints, since the
 * compiler doesn't like us using chars very much.
 *
 * Revision 1.161  2008/06/09 23:00:13  myc
 * Removed some redundant and outdated defines.
 *
 * Revision 1.160  2008/06/07 19:06:46  myc
 * Moved object-related constants and routines to objects.h.
 *
 * Revision 1.159  2008/06/05 02:07:43  myc
 * Changed object flags to use flagvectors.  Rewrote rent saving
 * and loading code to use ascii files, so got rid of a few
 * structs (which are still available in legacy_structs.h).
 *
 * Revision 1.158  2008/05/19 20:19:50  jps
 * Using stdbool.h.
 *
 * Revision 1.157  2008/05/19 06:53:31  jps
 * Got rid of fup and fdown directions.
 *
 * Revision 1.156  2008/05/19 05:46:04  jps
 * Add effect for being mesmerized.
 *
 * Revision 1.155  2008/05/18 17:58:21  jps
 * Adding effect of familiarity.
 *
 * Revision 1.154  2008/05/18 02:00:47  jps
 * Moved a lot of constants into rooms.h.
 *
 * Revision 1.153  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.152  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.151  2008/05/11 05:42:24  jps
 * Changed position and stance to ints.
 *
 * Revision 1.150  2008/04/20 03:54:17  jps
 * Add bfs distance variable to rooms.
 *
 * Revision 1.149  2008/04/14 05:11:40  jps
 * Renamed EFF_FLYING to EFF_FLY, since it only indicates an ability
 * to fly - not that the characer is actually flying.
 *
 * Revision 1.148  2008/04/14 02:17:59  jps
 * Adding def for Glory effect.
 *
 * Revision 1.147  2008/04/13 18:29:28  jps
 * Add effect for confusion.
 *
 * Revision 1.146  2008/04/13 00:57:07  jps
 * Add an auto-treasure loot pref.
 *
 * Revision 1.145  2008/04/07 04:31:10  jps
 * Update comments on position and stance.
 *
 * Revision 1.144  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.143  2008/04/05 18:07:09  myc
 * Re-implementing stealth for hide points.
 *
 * Revision 1.142  2008/04/05 16:49:45  myc
 * Fix FLAGVECTOR_SIZE macro so it doesn't always return 1 more than it needs to.
 *
 * Revision 1.141  2008/04/04 06:12:52  myc
 * Removed justice and dieites/worship code.
 *
 * Revision 1.140  2008/04/03 17:34:09  jps
 * Retired the player flag INVSTART.  Added a byte value autoinvis_level to
 * struct player_special_data.
 *
 * Revision 1.139  2008/04/02 05:36:19  myc
 * Added the autoloot and autosplit toggles.
 *
 * Revision 1.138  2008/04/02 04:55:59  myc
 * Got rid of the coins struct.
 *
 * Revision 1.137  2008/04/02 03:24:44  myc
 * Rewrote group code and removed major group code.
 *
 * Revision 1.136  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.135  2008/03/27 17:28:13  jps
 * ITEM_BLESS is unused now, and AFF3_BLESS and AFF3_HEX are here.
 *
 * Revision 1.134  2008/03/26 23:10:44  jps
 * Retire the WATERFORM and VAPORFORM effects.
 *
 * Revision 1.133  2008/03/26 18:11:02  jps
 * Added a BLESS effect so that characters may be blessed.
 *
 * Revision 1.132  2008/03/23 00:23:22  jps
 * Add an apply type for composition. Add a base_composition value
 * to characters so you can use the apply.
 *
 * Revision 1.131  2008/03/22 19:09:46  jps
 * Added lifeforce and composition to characters.
 *
 * Revision 1.130  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.129  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.128  2008/03/16 00:19:33  jps
 * Moving trophy structs to trophy.h. struct char_data now has a
 * single pointer to a trophy node.
 *
 * Revision 1.127  2008/03/11 19:50:55  myc
 * Changed the way allowed olc zones are saved on an immortal from
 * a fixed number of slots to a variable-length linked list.  Also
 * got rid of practice points.
 *
 * Revision 1.126  2008/03/11 02:55:30  jps
 * Use affected_size, mod_size, and natural_size to keep track of size.
 *
 * Revision 1.125  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.  Moving innate timers to cooldown system.
 * Reformatted structures some more.  Renamed hometown to homeroom.
 *
 * Revision 1.124  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.123  2008/03/10 18:01:17  myc
 * Re-ordered postures to be from most-prone to most-upright, somewhat
 * like positions.
 *
 * Revision 1.122  2008/03/09 18:11:31  jps
 * Added two aff3 flags - one for misdirection, which means that the
 * char is capable of misdirected movement. The other is for current
 * misdirecting, meaning that the move in progress is masked by illusion.
 *
 * Revision 1.121  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.120  2008/03/09 00:05:40  jps
 * Moved some NUM_foo_FLAGS defs from olc.h to structs.h.
 *
 * Revision 1.119  2008/03/08 23:54:04  jps
 * Added MOB2_NOSCRIPT flag, which prevents specprocs and triggers.
 *
 * Revision 1.118  2008/03/08 22:29:06  myc
 * Moving shapechange and chant to the cooldown system.
 *
 * Revision 1.117  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.116  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.115  2008/03/06 04:35:12  myc
 * Cleaned up formatting throughout the file.  Moved the IS_VICIOUS macro
 * from here to utils.h.  Added PULSE_AUTOSAVE to regulate how often
 * autosaves occur.
 *
 * Revision 1.114  2008/03/05 05:21:56  myc
 * Took out char_file_u struct and a bunch of random messages about it.
 * Made bank coins into ints instead of longs.  Took out frags too.
 *
 * Revision 1.113  2008/03/05 03:03:54  myc
 * Added NUM_SEXES and several trophy constants.  Removed a few constants
 * no longer necessary for pfiles.  Added a typedef for bitvectors.
 * Updated trophy, alias, spell memory, and strings for player files.
 *
 * Revision 1.112  2008/02/24 17:31:13  myc
 * Added OLCComm and NoClanTell toggles (prf bits).
 *
 * Revision 1.111  2008/02/16 20:31:32  myc
 * Include zmalloc.h in all files when memory debugging.
 *
 * Revision 1.110  2008/02/09 21:07:50  myc
 * Casting uses event flags instead of plr/mob flags now.
 *
 * Revision 1.109  2008/02/09 18:29:11  myc
 * Camping and tracking now use event flags instead of having
 * their own event fields on the char_data struct.
 *
 * Revision 1.108  2008/02/09 06:19:44  jps
 * Add "nohints" toggle for whether you receive command suggestions
 * after entering a typo.
 *
 * Revision 1.107  2008/02/09 03:06:17  myc
 * Adding a nice friendly 'do not mess with this' message to the perma
 * title define.
 *
 * Revision 1.106  2008/02/02 19:38:20  myc
 * Claimed some spare char space in player_special_data_saved
 * for player permanent titles.
 *
 * Revision 1.105  2008/01/30 19:20:57  myc
 * Removing the ch->regenerating field and replacing it with an event
 * flags member.
 *
 * Revision 1.104  2008/01/27 21:14:59  myc
 * Adding affection flags for berserk and berserk-related chants.
 * Adding rage to the player structure.
 *
 * Revision 1.103  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.102  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.101  2008/01/25 21:12:26  myc
 * Added 'rage' for berserking.
 *
 * Revision 1.100  2008/01/20 23:18:52  myc
 * Fixed mob AI to only leave out class actions.
 *
 * Revision 1.99  2008/01/20 22:58:39  myc
 * Added some new drinks.
 *
 * Revision 1.98  2008/01/12 19:08:14  myc
 * Rerowte a lot of mob AI functionality.
 *
 * Revision 1.97  2008/01/10 05:39:43  myc
 * Had to add a LVL_PURGE to let the purge command be 101 on test and 103
 * on production.
 *
 * damage() now returns the amount of damage it caused.  Negative values
 * indicate healing, and a return value of VICTIM_DEAD indicates that the
 * victim is dead.
 *
 * Added a heal_msg to the message_type struct.
 *
 * Revision 1.96  2008/01/09 08:31:32  jps
 * Change height and weight variables for characters to ints.
 * Note that the player file stores only bytes.  The code in
 * races.c has limited player values for these measurements
 * accordingly.
 *
 * Revision 1.95  2008/01/09 04:15:42  jps
 * Remove next_memming and next_scribing from struct char_data.
 *
 * Revision 1.94  2008/01/09 02:29:01  jps
 * Remove unused fields from struct char_data. Move mobile nr to mob_specials.
 *
 * Revision 1.93  2008/01/09 01:50:06  jps
 * Classify the elements of struct char_data. Remove the specifically stored
 * points events. Add int regenerating so we know which regeneration events
 * are on a character.
 *
 * Revision 1.92  2008/01/07 10:35:43  jps
 * Add a flag for a player phantasm.  It allows mobs to be aggressive to
 * it without removing the NPC flag.
 *
 * Revision 1.91  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.90  2008/01/06 17:34:29  jps
 * Get rid of obsolete struct class_thac0.
 *
 * Revision 1.89  2008/01/06 05:33:27  jps
 * use "sorcerer" and "rogue" instead of "magic user" and "thief"
 *
 * Revision 1.88  2008/01/05 21:55:50  jps
 * Added circular-dependency prevention defs.
 *
 * Revision 1.87  2008/01/05 20:32:31  jps
 * I hate tabs
 *
 * Revision 1.86  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.85  2008/01/02 02:11:03  jps
 * Moved class definition info to class.h.
 *
 * Revision 1.84  2007/12/29 00:05:10  jps
 * Changed name of BASE_SCRIBE_TIME to reflect the fact that it's
 * the time to scribe a page in a spellbook.
 *
 * Revision 1.83  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.82  2007/12/19 20:56:42  myc
 * Renaming the CLOAKED toggle to ROOMVIS.  Added a new connection
 * status for the clan description editor.  Added a NUM_COIN_TYPES
 * define.  Changed clan_rank from unsigned to signed in the
 * player structure.  Added a clan_id field to descriptor_data for
 * use by the clan description editor.
 *
 * Revision 1.81  2007/11/25 00:04:59  jps
 * Spell targets will keep close track of whoever's casting a spell
 * at them.  This allows spells to be safely aborted if the target
 * is removed from the game before the spell is completed.
 *
 * Revision 1.80  2007/11/18 16:51:55  myc
 * Renamed LVL_QUESTMASTER as LVL_GAMEMASTER.
 *
 * Revision 1.79  2007/10/23 20:19:25  myc
 * Created 'administration levels' to clean up the master command list.
 *
 * Revision 1.78  2007/10/13 20:13:09  myc
 * ITEM_NOLOCATE now prevents items from being found using the
 * locate object spell.
 *
 * Revision 1.77  2007/10/11 20:14:48  myc
 * Changed skill defines to support chants and songs as skills, but
 * slightly distinguished from spells and skills.  TOP_SKILL is the
 * old MAX_SKILLS.  Chants and songs now each have a block of 50
 * defines above the new MAX_SKILLS (550).  This is important
 * because MAX_SKILLS (now TOP_SKILL) is used in the pfile.
 *
 * Revision 1.76  2007/10/04 16:20:24  myc
 * Got rid of struct portal_decay_type.
 *
 * Revision 1.75  2007/10/02 02:52:27  myc
 * Removed AFF_HIDE, and put AFF_SNEAK back in.  Added character forwarding
 * for switching/shapechanging.
 *
 * Revision 1.74  2007/09/21 08:44:45  jps
 * Added object type "touchstone" and command "touch" so you can set
 * your home room by touching specific objects.
 *
 * Revision 1.73  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  AFF_HIDE, AFF_SNEAK, and ITEM_HIDDEN
 * are now unused.  Hiddenness replaces bitvector in obj_file_elem.
 *
 * Revision 1.72  2007/09/15 15:36:48  myc
 * Added camouflage aff3 bit for use by natures embrace.  Removed defunct
 * ITEM_ bitvector flags.  They were duplicating AFF flags.
 *
 * Revision 1.71  2007/09/15 05:37:15  myc
 * Adding new liquids.
 *
 * Revision 1.70  2007/09/15 05:03:46  myc
 * AFF_DROPPED_PRIM and AFF_DROPPED_SECOND were incorrectly marked as AFF1
 * flags, but should be AFF2 flags.  Added MOB2 flags, which are saved as
 * an espec in the mob files.  Implemented MOB2_NOPOISON flag.
 *
 * Revision 1.69  2007/09/11 16:34:24  myc
 * Replaced MOB_NOGEAR with MOB_AQUATIC, which allows you to limit aquatic
 * mobs to water rooms.
 *
 * Revision 1.68  2007/09/04 06:49:19  myc
 * Getting rid of defunct weather_data constants.  Changing hemisphere data
 * structs.
 *
 * Revision 1.67  2007/08/23 00:31:48  jps
 * Add !AIR and !EARTH flags, for elemental immunities.
 *
 * Revision 1.66  2007/08/22 17:58:05  jps
 * Add definitions for what levels are required for various rebooting
 * actions.  Also to identify the reason for the game being restricted
 * (wizlocked).
 *
 * Revision 1.65  2007/08/14 22:43:07  myc
 * Adding conceal, corner, shadow, and stealth skills.
 *
 * Revision 1.64  2007/08/14 10:41:31  jps
 * Add variables to struct player_special_data to prevent spamming.
 *
 * Revision 1.63  2007/08/04 14:40:35  myc
 * Added MOB_PEACEFUL flag to prevent players from attacking certain mobs.
 *
 * Revision 1.62  2007/08/03 22:00:11  myc
 * Added PK observatories that work adjacent to arena rooms.
 *
 * Revision 1.61  2007/08/03 03:51:44  myc
 * check_pk is now attack_ok, and covers many more cases than before,
 * including peaced rooms, shapeshifted pk, and arena rooms.  Almost all
 * offensive attacks now use attack_ok to determine whether an attack is
 * allowed.
 *
 * Revision 1.60  2007/07/19 17:51:36  jps
 * Move NUM_LIQ_TYPES from olc.h to structs.h, so the LIQ defines will
 * all be in one place.
 *
 * Revision 1.59  2007/07/18 21:05:00  jps
 * Added an IS_VICIOUS macro that works for mobs and players.
 *
 * Revision 1.58  2007/07/14 02:16:22  jps
 * Added some new constants related to mounts.
 *
 * Revision 1.57  2007/05/28 22:36:26  jps
 * Reduce the <base-class>_subclass arrays to the subclasses that are live.
 *
 * Revision 1.56  2007/05/11 20:13:28  myc
 * Vaporform is a new circle 13 spell for cryomancers.  It significantly
 * increases the caster's chance of dodging a hit.  It is a quest spell.
 *
 * Revision 1.55  2007/04/19 07:03:14  myc
 * Renamed RAY_OF_ENFEB as RAY_OF_ENFEEB.
 *
 * Revision 1.54  2007/04/15 08:30:49  jps
 * Make scribing much, much faster. Also fix various idiosyncrasies related
 * to scribing and make it more user-friendly.
 *
 * Revision 1.53  2007/03/27 04:27:05  myc
 * Added new size, colossal.  Renamed innate constants to be more descriptive.
 *
 * Revision 1.52  2007/02/08 01:30:00  myc
 * Level 1s can gossip again.
 *
 * Revision 1.51  2007/02/04 18:12:31  myc
 * Page length now saves as a part of player specials.
 *
 * Revision 1.50  2006/12/08 05:06:58  myc
 * Coin indicies for coin arrays moved here from act.item.c.
 *
 * Revision 1.49  2006/11/18 21:01:09  jps
 * Reworked disarm skill and disarmed-weapon retrieval.
 *
 * Revision 1.48  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.47  2006/11/17 22:52:59  jps
 * Change AGGR_GOOD/EVIL_ALIGN to AGGR_GOOD/EVIL_RACE
 *
 * Revision 1.46  2006/11/08 09:16:04  jps
 * Fixed some loose-lose typos.
 *
 * Revision 1.45  2006/04/11 09:08:46  rls
 * mods for medit.
 *
 * Revision 1.44  2004/11/01 06:02:01  jjl
 * Updating the buffer size for triggers
 *
 * Revision 1.43  2003/06/25 05:06:59  jjl
 * More updates.  I seem to be off of my game.
 *
 * Revision 1.41  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.40  2003/04/16 02:00:22  jjl
 * Added skill timers for Zzur.  They don't save to file, so they were a
 * quickie.
 *
 * Revision 1.39  2002/10/19 18:29:52  jjl
 * New and improved red green and blue scrolls of recall. Yummy!
 *
 * Revision 1.38  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.37  2002/08/29 17:37:50  rsd
 * Added a define for a new con state CON_ISPELL_BOOT to work
 * with new player name checking routines.
 *
 * Revision 1.36  2002/05/23 00:33:51  rls
 * Struct for new thac0 function.
 *
 * Revision 1.35  2001/04/08 17:13:10  dce
 * Added an alwayslit flag that makes a room lit no matter
 * of the sector or room type...
 *
 * Revision 1.34  2001/04/01 22:04:28  mtp
 * remove MAX_BASE_CLASS as CLASS_ROGUE is beyond its bounds
 *
 * Revision 1.33  2001/03/24 05:12:01  dce
 * Objects will now accept a level through olc and upon
 * booting the objects. The level code for the players will
 * follow.
 *
 * Revision 1.32  2001/02/03 00:59:30  mtp
 * added MAX_BASE_CLASS just for less hardcoding in the subclass area
 *
 * Revision 1.31  2000/11/28 01:14:40  mtp
 * removed mobprog references
 *
 * Revision 1.30  2000/11/25 02:33:15  rsd
 * Altered comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.29  2000/11/22 01:09:13  mtp
 * added motere mob classes (all the ones that are available for players)
 *
 * Revision 1.28  2000/11/07 01:32:57  mtp
 * changes d WARRIOR_SUBCLASSES to 5 and ROGUE_SUBCLASSES to 5
 *
 * Revision 1.27  2000/11/03 05:43:18  jimmy
 * removed the quest.h and put it where it should be
 *
 * Revision 1.26  2000/10/27 00:34:45  mtp
 * included quest.h and added member to char_data structure for quests
 *
 * Revision 1.25  2000/04/21 00:58:02  rsd
 * added a bool can_see_master to struct follow_type to work with
 * follow code into the dark etc...
 *
 * Revision 1.24  2000/01/31 00:01:41  rsd
 * added defines for good_race login, also fixed some tabs
 * for the defines.
 *
 * Revision 1.23  1999/12/10 05:11:40  cso
 * I moved one line down one line to make sense of it, line 205.
 *
 * Revision 1.22  1999/11/29 00:08:51  cso
 * added defines for MOB_ANIMATED and AFF3_ANIMATED
 *
 * Revision 1.21  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.20  1999/07/20 19:45:51  jimmy
 * This is the spanky New Spell recognition code.
 * This code allows mobs/players that have the KNOW_SPELL skill
 * to make a skill check to guess the spell.  A good roll will show both
 * the spell and the target.  A bad roll will show the spell garbled and
 * then an INT check for the target.  If a really bad roll is made, the spell
 * will be replaced by an incorrect one.  the heart of this system is
 * start_chant(), end_chant, and bad_guess().
 * --gurlaek 7/20/1999
 *
 * Revision 1.19  1999/07/15 03:27:34  jimmy
 * Mob casters can not hit while casting.
 * Updated spell cast times to be more realistic
 * changed combat to 4 seconds per round.
 * Removed do_order semantics that told the order to onlookers.
 *
 * Revision 1.18  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their race/class/level
 * that exactly align with PC's.  PC's no longer have to rent to use skills gained
 * by leveling or when first creating a char.  Languages no longer reset to defaults
 * when a PC levels.  Discovered that languages have been defined right in the middle
 * of the spell area.  This needs to be fixed.  A conversion util neeDs to be run on
 * the mob files to compensate for the 13 to -1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.17  1999/06/30 18:11:09  jimmy
 * act.offensive.c    config.c      handler.c    spells.c
 * This is a major conversion from the 18 point attribute system to the
 * 100 point attribute system.  A few of the major changes are:
 * All attributes are now on a scale from 0-100
 * Everyone views attribs the same but, the attribs for one race
 *   may be differeent for that of another even if they are the
 *   same number.
 * Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * Mobs now have individual random attributes based on race/class.
 * The STR_ADD attrib has been completely removed.
 * All bonus tables for attribs in constants.c have been replaced by
 *   algorithims that closely duplicate the tables except on a 100 scale.
 * Some minor changes:
 * Race selection at char creation can now be toggled by using
 *   <world races off>
 * Lots of cleanup done to affected areas of code.
 * Setting attributes for mobs in the .mob file no longer functions
 *   but is still in the code for later use.
 * We now have a spare attribut structure in the pfile because the new
 *   system only used three instead of four.
 * --gurlaek 6/30/1999
 *
 * Revision 1.16  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.15  1999/04/16 03:55:09  dce
 * Removed some things temporarly until they can be fixed.
 *
 * Revision 1.14  1999/04/07 01:20:18  dce
 * Allows extra descriptions on no exits.
 *
 * Revision 1.13  1999/03/26 19:44:35  jen
 * Added a mortal gossip channel with 103+ godly control
 *
 * Revision 1.12  1999/03/14 00:53:03  mud
 * In class.c added a new line before the fiery mud class explanation
 * in config.c added the variable for name explanations and added the
 * text for the variable
 * in interpreter.c added the con_state stuff, whatever that was and
 * added the CON_NAME_CHECK affirmation section to the creation menu
 * loop or nanny.
 * In structs.h added the CON_NAME_CHECK define..
 * I also drove Jimmy absolutely insane with the deail in information
 * I put into our change control system.
 *
 * Revision 1.11  1999/03/06 23:51:54  dce
 * Add's chant songs, and can only chant once every four hours
 *
 * Revision 1.10  1999/03/05 20:02:36  dce
 * Chant added to, and songs craeted
 *
 * Revision 1.9  1999/03/03 20:11:02  jimmy
 * Many enhancements to scribe and spellbooks.  Lots of checks added.  Scribe is now a skill.
 * Spellbooks now have to be held to scribe as well as a quill in the other hand.
 *
 * -fingon
 *
 * Revision 1.8  1999/03/01 05:31:34  jimmy
 * Rewrote spellbooks.  Moved the spells from fingh's PSE to a standard linked
 * list.  Added Spellbook pages.  Rewrote Scribe to be a time based event based
 * on the spell mem code.  Very basic at this point.  All spells are 5 pages long,
 * and take 20 seconds to scribe each page.  This will be more dynamic when the
 * SCRIBE skill is introduced.  --Fingon.
 *
 * Revision 1.7  1999/02/12 15:33:17  jimmy
 * Brand new spell table, thanks to Zzur
 * Glad I didnt' have to do it...
 * fingon
 *
 * Revision 1.6  1999/02/11 22:17:40  jimmy
 * Moved spell circles to every 8 levels.  Filled in the
 * spells array to extend from level 70 to 105.
 * fingon
 *
 * Revision 1.5  1999/02/10 22:21:42  jimmy
 * Added do_wiztitle that allows gods to edit their
 * godly title ie Overlord.  Also added this title
 * to the playerfile
 * fingon
 *
 * Revision 1.4  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.3  1999/02/06 00:40:36  jimmy
 * Major change to incorporate aliases into the pfile
 * moved alias structure from interpreter.h to structs.h
 * heavily modified alias code in interpreter.c
 * Jimmy Kincaid AKA fingon
 *
 * Revision 1.2  1999/02/05 07:47:42  jimmy
 * Added Poofs to the playerfile as well as 4 extra strings for
 * future use.  fingon
 *
 * Revision 1.1  1999/01/29 01:23:32  mud
 * Initial revision
 *
 ***************************************************************************/

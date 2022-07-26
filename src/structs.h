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

#include "money.h"     /* For NUM_COIN_TYPES */
#include "prefs.h"     /* For NUM_PRF_FLAGS */
#include "specprocs.h" /* For SPECIAL() */
#include "spell_mem.h"
#include "sysdep.h"

/* char and mob-related defines *****************************************/

/* Sex */
#define SEX_NEUTRAL 0
#define SEX_MALE 1
#define SEX_FEMALE 2
#define NUM_SEXES 3

/* Positions */
#define POS_PRONE 0
#define POS_SITTING 1
#define POS_KNEELING 2
#define POS_STANDING 3
#define POS_FLYING 4
#define NUM_POSITIONS 5

/* Stances */
#define STANCE_DEAD 0     /* dead                 */
#define STANCE_MORT 1     /* mortally wounded     */
#define STANCE_INCAP 2    /* incapacitated        */
#define STANCE_STUNNED 3  /* stunned              */
#define STANCE_SLEEPING 4 /* sleeping             */
#define STANCE_RESTING 5  /* resting              */
#define STANCE_ALERT 6    /* alert                */
#define STANCE_FIGHTING 7 /* fighting             */
#define NUM_STANCES 8

#define HIT_INCAP -3     /* The hit level for incapacitation   */
#define HIT_MORTALLYW -6 /* The hit level for mortally wound   */
#define HIT_DEAD -11     /* The point you never want to get to */

#define DAMAGE_WILL_KILL(ch, dmg) (GET_HIT(ch) - dmg <= HIT_DEAD)

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER 0     /* a player-killer                           */
#define PLR_THIEF 1      /* a player-thief                            */
#define PLR_FROZEN 2     /* is frozen                                 */
#define PLR_DONTSET 3    /* Don't EVER set (ISNPC bit)                */
#define PLR_WRITING 4    /* writing (board/mail/olc)                  */
#define PLR_MAILING 5    /* is writing mail                           */
#define PLR_AUTOSAVE 6   /* needs to be autosaved                     */
#define PLR_SITEOK 7     /* has been site-cleared                     */
#define PLR_NOSHOUT 8    /* not allowed to shout/goss                 */
#define PLR_NOTITLE 9    /* not allowed to set title       (not used) */
#define PLR_DELETED 10   /* deleted - space reusable       (not used) */
#define PLR_LOADROOM 11  /* uses nonstandard loadroom      (not used) */
#define PLR_NOWIZLIST 12 /* shouldn't be on wizlist        (not used) */
#define PLR_NODELETE 13  /* shouldn't be deleted           (may be used outside the server) */
#define PLR_INVSTART 14  /* should enter game wizinvis     (not used) */
#define PLR_CRYO 15      /* is cryo-saved (purge prog)     (not used) */
#define PLR_MEDITATE 16  /* meditating - improves spell memorization  */
#define PLR_CASTING 17   /* currently casting a spell      (not used) */
#define PLR_BOUND 18     /* tied up                        (not used) */
#define PLR_SCRIBE 19    /* scribing                       (not used) */
#define PLR_TEACHING 20  /* teaching a skill/spell         (not used) */
#define PLR_NAPPROVE 21  /* name not approved yet                     */
#define PLR_NEWNAME 22   /* needs to choose a new name                */
#define PLR_REMOVING 23  /* player is being removed and doesn't need emergency save */
#define PLR_SAVING 24    /* player is being saved to file and effect changes are not relevant */
#define PLR_GOTSTARS 25  /* player has achieved ** already            */
#define NUM_PLR_FLAGS 26

/* Privilege flags: used by char_data.player_specials.privileges */
#define PRV_CLAN_ADMIN 0  /* clan administrator */
#define PRV_TITLE 1       /* can change own title */
#define PRV_ANON_TOGGLE 2 /* can toggle anon */
#define PRV_AUTO_GAIN 3   /* don't need to level gain */
#define NUM_PRV_FLAGS 4

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC 0          /* Mob has a callable spec-proc       */
#define MOB_SENTINEL 1      /* Mob should not move                */
#define MOB_SCAVENGER 2     /* Mob picks up stuff on the ground   */
#define MOB_ISNPC 3         /* (R) Automatically set on all Mobs  */
#define MOB_AWARE 4         /* Mob can't be backstabbed           */
#define MOB_AGGRESSIVE 5    /* Mob hits players in the room       */
#define MOB_STAY_ZONE 6     /* Mob shouldn't wander out of zone   */
#define MOB_WIMPY 7         /* Mob flees if severely injured      */
#define MOB_AGGR_EVIL 8     /* auto attack evil PC's              */
#define MOB_AGGR_GOOD 9     /* auto attack good PC's              */
#define MOB_AGGR_NEUTRAL 10 /* auto attack neutral PC's           */
#define MOB_MEMORY 11       /* remember attackers if attacked     */
#define MOB_HELPER 12       /* attack PCs fighting other NPCs     */
#define MOB_NOCHARM 13      /* Mob can't be charmed               */
#define MOB_NOSUMMON 14     /* Mob can't be summoned              */
#define MOB_NOSLEEP 15      /* Mob can't be slept                 */
#define MOB_NOBASH 16       /* Mob can't be bashed (e.g. trees)   */
#define MOB_NOBLIND 17      /* Mob can't be blinded               */
#define MOB_MOUNTABLE 18
#define MOB_NO_EQ_RESTRICT 19
#define MOB_FAST_TRACK 20
#define MOB_SLOW_TRACK 21
#define MOB_CASTING 22        /* mob casting            (not used)  */
#define MOB_SUMMONED_MOUNT 23 /* resets CD_SUMMON_MOUNT when extracted */
#define MOB_AQUATIC 24        /* Mob can't enter non-water rooms    */
#define MOB_AGGR_EVIL_RACE 25
#define MOB_AGGR_GOOD_RACE 26
#define MOB_NOSILENCE 27
#define MOB_NOVICIOUS 28
#define MOB_TEACHER 29
#define MOB_ANIMATED 30        /* mob is animated - die if no anim effect */
#define MOB_PEACEFUL 31        /* mob can't be attacked.             */
#define MOB_NOPOISON 32        /* Mob cannot be poisoned.            */
#define MOB_ILLUSORY 33        /* is an illusion: does no harm, leaves no corpse */
#define MOB_PLAYER_PHANTASM 34 /* illusion of player; mobs are aggro to */
#define MOB_NO_CLASS_AI 35     /* Mob does not execute class AI      */
#define MOB_NOSCRIPT 36        /* Mob does not execute triggers or specprocs */
#define MOB_PEACEKEEPER 37     /* Attacks mobs with over 1350 align diff. Assists other PEACEKEEPERs */
#define MOB_PROTECTOR 38       /* Assists players under attack, but not against PEACEKEEPER/PROTECTOR mobs */
#define MOB_PET 39             /* Mob was purchased or tamed and is now a pet to a player. */
#define NUM_MOB_FLAGS 40       /* Update this when you add a flag! */

/* Some mount stuff */
#define MAX_MOUNT_LEVEL 27 /* The maximum level of mountable mobs */

/* mount_level_fudge is how far above the ideal mountable level you could
 * possibly hope to mount.  For example if your skill allowed you to mount level
 * 10 mobs easily, and mount_level_fudge was 4, you could mount level 14 mobs
 * with great difficulty, but nothing higher. */
#define MOUNT_LEVEL_FUDGE (double)(MAX_MOUNT_LEVEL / 6)

#define MOUNT_MINMOVE 20
#define MOUNT_MAXMOVE 250

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING 0       /* Playing - Nominal state         */
#define CON_CLOSE 1         /* Disconnecting                   */
#define CON_GET_NAME 2      /* By what name ..?                */
#define CON_NAME_CNFRM 3    /* Did I get that right, x?        */
#define CON_PASSWORD 4      /* Password:                       */
#define CON_NEWPASSWD 5     /* Give me a password for x        */
#define CON_CNFPASSWD 6     /* Please retype password:         */
#define CON_QSEX 7          /* Sex?                            */
#define CON_QCLASS 8        /* Class?                          */
#define CON_RMOTD 9         /* PRESS RETURN after MOTD         */
#define CON_MENU 10         /* Your choice: (main menu)        */
#define CON_EXDESC 11       /* Enter a new description:        */
#define CON_CHPWD_GETOLD 12 /* Changing passwd: get old        */
#define CON_CHPWD_GETNEW 13 /* Changing passwd: get new        */
#define CON_CHPWD_VRFY 14   /* Verify new password             */
#define CON_DELCNF1 15      /* Delete confirmation 1           */
#define CON_DELCNF2 16      /* Delete confirmation 2           */
#define CON_QRACE 17        /* Complete Race Seletion Menu     */
#define CON_QANSI 18        /* Prompt for term type            */
#define CON_OEDIT 19        /*. OLC mode - object edit        .*/
#define CON_REDIT 20        /*. OLC mode - room edit          .*/
#define CON_ZEDIT 21        /*. OLC mode - zone info edit     .*/
#define CON_MEDIT 22        /*. OLC mode - mobile edit        .*/
#define CON_SEDIT 23        /*. OLC mode - shop edit          .*/
#define CON_QROLLSTATS 24
#define CON_QHOMETOWN 25
#define CON_QBONUS1 26
#define CON_QBONUS2 27
#define CON_QBONUS3 28
#define CON_QCANCHAR 29
#define CON_HEDIT 30     /*. OLC mode - help edit          .*/
#define CON_TRIGEDIT 31  /*. OLC mode - trigger edit       .*/
#define CON_CLASSHELP 32 /* Char Gen Class Help             */
#define CON_SDEDIT 33
#define CON_NAME_CHECK 34
#define CON_NAME_WAIT_APPROVAL 35 /* await imm aprroval of name      */
#define CON_NEW_NAME 36           /* name declined, get a new one    */
#define CON_QGOODRACE 37          /* Menu Choice for Good races.     */
#define CON_ISPELL_BOOT 38        /* Obligatory disconnect bad names */
#define CON_GEDIT 39              /* OLC mode - grant group edit */
#define CON_IEDIT 40              /*. OLC mode - iobject edit        .*/
#define NUM_CON_MODES 41

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT 0
#define WEAR_FINGER_R 1
#define WEAR_FINGER_L 2
#define WEAR_NECK_1 3
#define WEAR_NECK_2 4
#define WEAR_BODY 5
#define WEAR_HEAD 6
#define WEAR_LEGS 7
#define WEAR_FEET 8
#define WEAR_HANDS 9
#define WEAR_ARMS 10
#define WEAR_SHIELD 11
#define WEAR_ABOUT 12
#define WEAR_WAIST 13
#define WEAR_WRIST_R 14
#define WEAR_WRIST_L 15
#define WEAR_WIELD 16
#define WEAR_WIELD2 17
#define WEAR_HOLD 18
#define WEAR_HOLD2 19
#define WEAR_2HWIELD 20
#define WEAR_EYES 21
#define WEAR_FACE 22
#define WEAR_LEAR 23
#define WEAR_REAR 24
#define WEAR_BADGE 25
#define WEAR_OBELT 26
#define WEAR_HOVER 27
#define NUM_WEARS 28 /* This must be the # of eq positions!! */

/* other miscellaneous defines *******************************************/

/* Player conditions */
#define DRUNK 0
#define FULL 1
#define THIRST 2

/* Levels of rage */
#define RAGE_NONE 0
#define RAGE_ANNOYED 250
#define RAGE_ANGRY 500
#define RAGE_IRATE 750
#define RAGE_CRAZED 1000

/*
 * Weather data used by utils.h
 */
/* Sun state */
#define SUN_DARK 0
#define SUN_RISE 1
#define SUN_LIGHT 2
#define SUN_SET 3

#define HEMISPHERE_NORTHWEST 0
#define HEMISPHERE_NORTHEAST 1
#define HEMISPHERE_SOUTHWEST 2
#define HEMISPHERE_SOUTHEAST 3
#define NUM_HEMISPHERES 4

/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 */
#define LVL_IMPL 105     /* Overlord */
#define LVL_OVERLORD 105 /* Overlord */
#define LVL_HEAD_C 104   /* Implementer */
#define LVL_HEAD_B 103   /* Greater God */
#define LVL_GRGOD 102    /* Lesser God */
#define LVL_GOD 101      /* Demi-God */
#define LVL_IMMORT 100   /* Avatar */
#define LVL_MAX_MORT 99  /* Mortal */

#ifdef PRODUCTION
#define LVL_ADMIN 104
#define LVL_BUILDER 104
#define LVL_GAMEMASTER 103
#define LVL_ATTENDANT 102
#define LVL_RESTORE 103
#define LVL_PURGE 103
#else
#define LVL_ADMIN 103
#define LVL_BUILDER 101
#define LVL_GAMEMASTER 102
#define LVL_ATTENDANT 101
#define LVL_RESTORE 1
#define LVL_PURGE 101
#endif

#define LVL_FREEZE LVL_GRGOD

/* Level required for rebooting, shutdown, autoboot */

#ifdef PRODUCTION
/* Full use of "autoboot" and "shutdown" commands.
 * You can cancel/enable/set time of automatic reboot,
 * or (obviously) use shutdown to stop or restart the mud immediately */
#define LVL_REBOOT_MASTER LVL_HEAD_B
/* You can stop a reboot for a while with "autoboot postpone" */
#define LVL_REBOOT_POSTPONE LVL_GOD
/* You can get the time of the next automatic reboot with "autoboot" or "world"
 */
#define LVL_REBOOT_VIEW LVL_GOD
#else
#define LVL_REBOOT_MASTER LVL_HEAD_B
#define LVL_REBOOT_POSTPONE LVL_IMMORT
#define LVL_REBOOT_VIEW 1
#endif

#define LVL_GOSSIP 1

/* Reasons why should_restrict > 0 */
#define RESTRICT_NONE 0     /* No restriction, or don't know */
#define RESTRICT_ARGUMENT 1 /* Mud started with -r option */
#define RESTRICT_MANUAL 2   /* Set by a god with the wizlock command */
#define RESTRICT_AUTOBOOT 3 /* Set automatically due to imminent automatic reboot */

/* damage() return codes */
#define VICTIM_DEAD (1 << 30)

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
#define OPT_USEC 100000 /* 10 passes per second */
#define PASSES_PER_SEC (1000000 / OPT_USEC)
#define RL_SEC *PASSES_PER_SEC
#define MUD_HR *SECS_PER_MUD_HOUR *PASSES_PER_SEC

#define PULSE_ZONE (10 RL_SEC)
#define PULSE_MOBILE (10 RL_SEC)
#define PULSE_VIOLENCE (4 RL_SEC) /* changed to 4 seconds from 2 7/14/99 */
#define PULSE_AUTOSAVE (60 RL_SEC)

#define SMALL_BUFSIZE 1024
#define LARGE_BUFSIZE (48 * 1024)
#define GARBAGE_SPACE 32

#define MAX_STRING_LENGTH 20000
#define MAX_STRING_LENGTH_BIG 24000
#define MAX_DESC_LENGTH 8096        /* object action desc length */
#define MAX_INPUT_LENGTH 1000       /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH 150000 /* Max size of *raw* input */
#define MAX_MESSAGES 150
#define MAX_NAME_LENGTH 20
#define MAX_PWD_LENGTH 10
#define MAX_TITLE_LENGTH 80
#define HOST_LENGTH 30

#define PLAYER_DESC_LENGTH 1200
#define PLAYER_DESC_LINES 15
#define IMMORT_DESC_LENGTH 10000
#define IMMORT_DESC_LINES 50

#define MAX_TONGUE 3
#define TOP_SKILL 650
#define MAX_EFFECT 32
#define PAGE_SCRIBE_TIME 1    /* this is the time per page to scribe */
#define MAX_DAMAGE 1000       /* Maximum per hit allowed by damage() */
#define EVENT_FLAG_FIELDS 4   /* Number of fields for event flags */
#define MIN_ABILITY_VALUE 25  /* Min for stuff like str/cha */
#define MAX_ABILITY_VALUE 100 /* Max for stuff like str/cha */

/* ***** Cooldowns ***** */
#define CD_BACKSTAB 0
#define CD_BASH 1
#define CD_INSTANT_KILL 2
#define CD_DISARM 3             /* pulse violence */
#define CD_FUMBLING_PRIMARY 4   /* pulse violence */
#define CD_DROPPED_PRIMARY 5    /* pulse violence */
#define CD_FUMBLING_SECONDARY 6 /* pulse violence */
#define CD_DROPPED_SECONDARY 7  /* pulse violence */
#define CD_SUMMON_MOUNT 8
#define CD_LAY_HANDS 9
#define CD_FIRST_AID 10
#define CD_EYE_GOUGE 11
#define CD_THROATCUT 12
#define CD_SHAPECHANGE 13
#define CD_CHANT 14
#define CD_INNATE_INVISIBLE 15
#define CD_INNATE_CHAZ 16
#define CD_INNATE_DARKNESS 17
#define CD_INNATE_LEVITATE 18
#define CD_INNATE_SYLL 19
#define CD_INNATE_TREN 20
#define CD_INNATE_TASS 21
#define CD_INNATE_BRILL 22
#define CD_INNATE_ASCEN 23
#define CD_INNATE_HARNESS 24
#define CD_BREATHE 25
#define CD_INNATE_CREATE 26
#define CD_MUSIC_1 27
#define CD_MUSIC_2 28
#define CD_MUSIC_3 29
#define CD_MUSIC_4 30
#define CD_MUSIC_5 31
#define CD_MUSIC_6 32
#define CD_MUSIC_7 33
#define NUM_COOLDOWNS 34
#define CD_INNATE_BLINDING_BEAUTY 25
#define NUM_COOLDOWNS 36

/**********************************************************************
 * Structures                                                          *
 **********************************************************************/

typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;
typedef struct witness_data wtns_rec;

#ifndef CIRCLE_WINDOWS
typedef char byte;
#endif

typedef int room_num;
typedef int obj_num;
typedef int zone_vnum;

#define NOWHERE -1 /* nil reference for room-database        */
#define NOTHING -1 /* nil reference for objects              */
#define NOBODY -1  /* nil reference for mobiles              */

/* exits.h depends on obj_num */
#include "exits.h"

/** Bitvector type for 32 bit unsigned long bitvectors. 'unsigned long long'
 * will give you at least 64 bits if you have GCC. You'll have to search
 * throughout the code for "bitvector_t" and change them yourself if you'd
 * like this extra flexibility. */
typedef unsigned long int flagvector;
#define FLAGBLOCK_SIZE ((flagvector)8 * sizeof(flagvector)) /* 8 bits = 1 byte */
#define FLAGVECTOR_SIZE(flags) (((flags)-1) / FLAGBLOCK_SIZE + 1)

#include "objects.h"

/* hemispheres for weather and time NE< NW< SE< SW */
struct hemisphere_data {
    char *name;
    int season;
    int sunlight;
};

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
    char *keyword;                 /* Keyword in look/examine          */
    char *description;             /* What to see                      */
    struct extra_descr_data *next; /* Next in list                     */
};

struct casting {
    int spell;
    int casting_time;
    struct char_data *tch; /* set up the targets */
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
    long id;
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
    time_t birth; /* This represents the characters age                */
    time_t logon; /* Time of the last logon (used to calculate played) */
    int played;   /* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data {
    char passwd[MAX_PWD_LENGTH + 1]; /* character's password      */
    char *namelist;                  /* PC / NPC s name (kill ...  )         */
    char *short_descr;               /* for NPC 'actions'                    */
    char *long_descr;                /* for NPC 'look'                       */
    char *description;               /* Extra descriptions                   */
    char *title;                     /* PC / NPC's title                     */
    char *prompt;                    /* Player prompt                        */
    ush_int sex;                     /* PC / NPC's sex                       */
    ush_int class;                   /* PC / NPC's class                     */
    ush_int race;                    /* PC / NPC's race                      */
    ush_int race_align;              /* PC / NPC's race_align                */
    ush_int level;                   /* PC / NPC's level                     */
    int lifeforce;                   /* What empowers it - see LIFE_* in chars.h */
    int base_composition;
    int composition;   /* What its body is made of - see COMP_* in chars.h */
    room_num homeroom; /* PC s Homeroom                        */

    struct time_data time; /* PC's AGE in days                 */

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

#include "money.h"

/* Char's points. */
struct char_point_data {
    int mana;
    int max_mana; /* Max move for PC/NPC                     */
    int hit;
    int max_hit; /* Max hit for PC/NPC                      */
    int move;
    int max_move; /* Max move for PC/NPC                     */
    int armor;    /* Internal -100..100, external -10..10 AC */
    int coins[NUM_COIN_TYPES];
    int bank[NUM_COIN_TYPES];
    long exp; /* The experience of the player            */

    int base_hitroll; /* Any bonus or penalty to the hit roll    */
    int hitroll;      /* Value used for calculations and viewing */
    int base_damroll; /* Any bonus or penalty to the damage roll */
    int damroll;      /* Value used for calculations and viewing */
};

struct alias_data {
    char *alias;
    char *replacement;
    int type;
    struct alias_data *next;
};

/* Special playing constants shared by PCs and NPCs */
struct char_special_data {
    struct char_data *hunting; /* Char hunted by this char                */
    struct char_data *riding;
    struct char_data *ridden_by;
    struct char_data *consented;
    int position; /* Prone, Sitting, Standing, etc.        */
    int stance;   /* Sleeping, Alert, Fighting, etc.       */

    int cooldowns[NUM_COOLDOWNS][2]; /* Skill/action cooldowns (current/max)  */

    float carry_weight; /* Carried weight                        */
    int carry_items;    /* Number of items carried               */
    int timer;          /* Inactivity timer for players          */
    int hitgain;        /* Bonus hit regen, from APPLY_HIT_REGEN */
    int managain;       /* Bonus mana regen, from APPLY_MANA_REGEN */
    int rage;           /* For berserking                        */

    int alignment; /* +/- 1000 for alignment                */
    long idnum;    /* player's idnum; -1 for mobiles        */
    /* act flag for NPC; player flag for PC  */
    flagvector act[FLAGVECTOR_SIZE(NUM_MOB_FLAGS > NUM_PLR_FLAGS ? NUM_MOB_FLAGS : NUM_PLR_FLAGS)];

    long perception;
    long hiddenness;

    /* Bitvectors for spells/skills effects  */
    flagvector effects[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
    sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)                */

    sh_int skills[TOP_SKILL + 1]; /* array of skills plus skill 0          */

    /* These keep track of how much a player has been speaking (gossipping,
     * telling, whatever else is deemed appropriate) and is used to decide
     * whether to automatically quiet the player. */
    double speech_rate;
    long last_speech_time; /* Taken from global_pulse               */

    int quit_reason; /* How or why you left the game          */
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
    long last_tell;      /* idnum of last tell from                */
    void *last_olc_targ; /* olc control                            */
    int last_olc_mode;   /* olc control                            */
    byte roll[6];        /* for rolling stats in player creation   */
    struct char_data *ignored;

    /* List of the last X comms as defined in retained_comms.h */
    struct retained_comms *comms;
    bool talks[MAX_TONGUE]; /* PC s Tongues 0 for NPC                 */
    int wimp_level;         /* Below this # of hit points, flee!      */
    int aggressive;         /* Above this # of hit points, autoattack */
    byte freeze_level;      /* Level of god who froze char, if any    */
    byte autoinvis_level;   /* Level of invisibility to take when entering game */
    byte invis_level;       /* level of invisibility                  */
    room_num load_room;     /* Which room to place char in            */
    room_num save_room;     /* Where the player was when saved        */
    /* preference flags for PC's.             */
    flagvector pref[FLAGVECTOR_SIZE(NUM_PRF_FLAGS)];
    /* privilege flags for PC's */
    flagvector privileges[FLAGVECTOR_SIZE(NUM_PRV_FLAGS)];
    ubyte bad_pws;       /* number of bad password attempts        */
    sbyte conditions[3]; /* Drunk, full, thirsty                   */
    struct trophy_node *trophy;
    struct alias_data *aliases;

    flagvector *grant_cache;    /* cache of granted commands              */
    flagvector *revoke_cache;   /* cache of revoked commands              */
    struct grant_type *grants;  /* Commands granted to this player        */
    struct grant_type *revokes; /* Commands revoked from this player      */
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
    int log_view; /* Level of syslog displayed              */
    char *poofin;
    char *poofout;
    char **perm_titles;
    char *long_descr;
    char *wiz_title;
    char *host;
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    long nr; /* Mob's rnum                              */
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
    byte last_direction; /* The last direction the monster went     */
    int attack_type;     /* The Attack Type Bitvector for NPC's     */
    byte default_pos;    /* Default position for NPC                */
    memory_rec *memory;  /* List of attackers to remember           */
    sbyte damnodice;     /* The number of damage dice's             */
    sbyte damsizedice;   /* The size of the damage dice's           */
    sbyte ex_damnodice;
    sbyte ex_damsizedice;
    int wait_state;                        /* Wait state for bashed mobs              */
    int spell_bank[NUM_SPELL_CIRCLES + 1]; /* circle 0 is unused */
    int spell_mem_time;
    sh_int ex_armor;
    long mob2_flags;
};

/* An effect structure. */
struct effect {
    int type;      /* The type of spell that caused this       */
    int duration;  /* For how long its effects will last       */
    int modifier;  /* This is added to apropriate ability      */
    byte location; /* Tells which ability to change(APPLY_XXX) */
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
    long id;                                 /* Global unique ID used by DG triggers */
    room_num in_room;                        /* Location (real room number) */
    room_num was_in_room;                    /* location for linkdead people */
    struct char_ability_data natural_abils;  /* natural rolls */
    struct char_ability_data actual_abils;   /* natural_abils + effects */
    struct char_ability_data affected_abils; /* viewed_abils * racial percentage */
    struct char_point_data points;           /* Points */
    struct effect *effects;                  /* effected by what spells */
    struct obj_data *equipment[NUM_WEARS];   /* Equipment array */
    struct obj_data *carrying;               /* Head of list */
    struct know_spell *see_spell;            /* list of chars that guessed caster's spell */
    /* Other characters */
    struct char_data *forward;      /* for shapechange/switch */
    struct char_data *next_in_room; /* For room->people - list */
    struct char_data *next;         /* For either monster or ppl-list */
    struct char_data *guarded_by;   /* Character guarding this char */
    struct char_data *guarding;     /* Char this char is guarding */
    struct char_data *cornered_by;  /* Char preventing this char from fleeing */
    struct char_data *cornering;    /* Char this char is preventing from fleeing*/
    struct follow_type *followers;  /* List of chars followers */
    struct char_data *master;       /* Who is char following? */
    struct group_type *groupees;    /* list of chars grouped */
    struct char_data *group_master; /* group master */
    struct char_data *next_caster;  /* A list of casters I'm in */
    struct char_data *casters;      /* Chars who are casting spells at me */
    /* Battling */
    struct char_data *next_fighting; /* Part of list of all fighting characters in mud */
    struct char_data *target;        /* Who I'm fighting */
    struct char_data *attackers;     /* List of characters who are fighting me */
    struct char_data *next_attacker; /* List of fighting characters I'm in */
    /* Player stuff */
    int pfilepos; /* playerfile pos */
    struct quest_list *quests;
    /* Spell mem/scribe stuff */
    struct spell_memory spell_memory;
    struct scribing *scribe_list; /* spells queued for scribing */
    /* Mobile stuff */
    struct trig_proto_list *proto_script; /* list of default triggers */
    struct script_data *script;           /* script info for the object */
    /* Substructs of varying application */
    struct char_player_data player;              /* Normal data */
    struct char_special_data char_specials;      /* PC/NPC specials */
    struct player_special_data *player_specials; /* PC specials */
    struct mob_special_data mob_specials;        /* NPC specials  */
    struct descriptor_data *desc;                /* NULL for mobiles */
    /* Events */
    struct casting casting;             /* note this is NOT a pointer */
    struct event *events;               /* List of events related to this character */
    int event_flags[EVENT_FLAG_FIELDS]; /* Bitfield of events active on this
                                           character */
};

/* ====================================================================== */

/* descriptor-related structures ******************************************/

struct txt_block {
    char *text;
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
    socket_t descriptor;        /* file descriptor for socket            */
    char host[HOST_LENGTH + 1]; /* hostname                              */
    byte bad_pws;               /* number of bad pw attemps this login   */
    byte idle_tics;             /* tics idle at password prompt          */
    int connected;              /* mode of 'connectedness'               */
    int wait;                   /* wait for how many loops               */
    bool gmcp_enabled;          /* Shall we send additional GMCP data    */
    int desc_num;               /* unique num assigned to desc           */
    time_t login_time;          /* when the person connected             */
    int mail_vnum;
    int prompt_mode;                   /* control of prompt-printing            */
    char inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input            */
    char last_input[MAX_INPUT_LENGTH]; /* the last input                  */
    char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer                */
    char *output;                      /* ptr to the current output buffer      */
    int bufptr;                        /* ptr to end of current output          */
    int bufspace;                      /* space left in the output buffer       */
    struct txt_block *large_outbuf;    /* ptr to large buffer, if we need it    */
    struct txt_q input;                /* q of unprocessed input                */
    struct char_data *character;       /* linked to char                        */
    struct char_data *original;        /* original char if switched             */
    struct descriptor_data *snooping;  /* Who is this char snooping             */
    struct descriptor_data *snoop_by;  /* And who is snooping this char         */
    struct descriptor_data *next;      /* link to next descriptor               */
    struct olc_data *olc;
    char *storage;

    /* Editing a buffer */
    char **str;           /* for the modify-str system             */
    char *backstr;        /* added for handling abort buffers      */
    size_t max_str;       /*                -                      */
    long mail_to;         /* name for mail system                  */
    int max_buffer_lines; /* limitation on the number of lines     */

    struct editor_data *editor;

    /* The pager */
    struct paging_line *paging_lines; /* The text that is to be paged through  */
    struct paging_line *paging_tail;  /* End of the list of lines              */
    char *paging_fragment;            /* Intermediate leftover string          */
    int paging_numlines;              /* Number of lines in the list           */
    int paging_numpages;              /* Number of pages to be paged through   */
    int paging_curpage;               /* The page which is currently showing   */
    int paging_bufsize;               /* Amount of memory currently used       */
    int paging_skipped;               /* Number of lines discarded due to overflow */
};

/* other miscellaneous structures ***************************************/

struct msg_type {
    char *attacker_msg; /* message to attacker */
    char *victim_msg;   /* message to victim   */
    char *room_msg;     /* message to room     */
};

struct message_type {
    struct msg_type die_msg;   /* messages when death             */
    struct msg_type miss_msg;  /* messages when miss              */
    struct msg_type hit_msg;   /* messages when hit               */
    struct msg_type god_msg;   /* messages when hit on god        */
    struct msg_type heal_msg;  /* message when healing            */
    struct message_type *next; /* to next messages of this kind.  */
};

struct message_list {
    int a_type;               /* Attack type                                */
    int number_of_attacks;    /* How many attack messages to chose from. */
    struct message_type *msg; /* List of messages.                        */
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
    sh_int tohit;   /* To Hit (THAC0) Bonus/Penalty        */
    sh_int todam;   /* Damage Bonus/Penalty                */
    sh_int carry_w; /* Maximum weight that can be carrried */
    sh_int wield_w; /* Maximum weight that can be wielded  */
};

struct wis_app_type {
    byte bonus; /* how many practices player gains per lev */
};

struct int_app_type {
    byte learn; /* how many % a player learns a spell/skill */
    byte bonus; /* bonus to skills */
};

struct con_app_type {
    sh_int hitp;
    sh_int shock;
};

struct cha_app_type {
    sh_int music; /* how many bardic music cooldowns one can have */
};

struct weather_data {
    int pressure; /* How is the pressure ( Mb ) */
    int change;   /* How fast and what way does it change. */
    int sky;      /* How is the sky. */
    int sunlight; /* And how much sun. */
};

/* element in monster and object index-tables   */
struct index_data {
    int virtual; /* virtual number of this mob/obj           */
    int number;  /* number of existing units of this mob/obj        */
    SPECIAL(*func);
    char *farg;              /* string argument for special function     */
    struct trig_data *proto; /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
    int vnum;                     /* vnum of the trigger   */
    struct trig_proto_list *next; /* next trigger          */
};

struct camp_event {
    struct char_data *ch;
    int was_in;
};

#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

#endif
#pragma once

// Do not include files here, this is a header file for the defines only.

#define NOWHERE -1 /* nil reference for room-database        */
#define NOTHING -1 /* nil reference for objects              */
#define NOBODY -1  /* nil reference for mobiles              */

/* char and mob-related defines *****************************************/

/* Sex */
#define SEX_NEUTRAL 0
#define SEX_MALE 1
#define SEX_FEMALE 2
#define SEX_NONBINARY 3
#define NUM_SEXES 4

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

/* Player flags: used by CharData.char_specials.act */
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

/* Privilege flags: used by CharData.player_specials.privileges */
#define PRV_CLAN_ADMIN 0  /* clan administrator */
#define PRV_TITLE 1       /* can change own title */
#define PRV_ANON_TOGGLE 2 /* can toggle anon */
#define PRV_AUTO_GAIN 3   /* don't need to level gain */
#define NUM_PRV_FLAGS 4

/* Mobile flags: used by CharData.char_specials.act */
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
#define CON_RACEHELP 18     /* */
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
#define CON_QSTAT_CNFRM 41        /* Confirm stat arrangement */
#define CON_SELECT_STR 42         /* Set STR */
#define CON_SELECT_CON 43         /* Set CON */
#define CON_SELECT_DEX 44         /* Set DEX */
#define CON_SELECT_INT 45         /* Set INT */
#define CON_SELECT_WIS 46         /* Set WIS */
#define CON_SELECT_CHA 47         /* Set CHA */
#define NUM_CON_MODES 48

/* Character equipment positions: used as index for CharData.equipment[] */
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
#define RAGE_ANNOYED 100
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

#define LVL_ADMIN 104
#define LVL_BUILDER 104
#define LVL_GAMEMASTER 103
#define LVL_ATTENDANT 102
#define LVL_RESTORE 103
#define LVL_PURGE 103

#define LVL_FREEZE LVL_GRGOD

#define MAX_SPELLS 400
#define MAX_SKILLS 550
#define MAX_SONGS 600
#define MAX_CHANTS 650

/* Level required for rebooting, shutdown, autoboot */

#define ENV_PROD 0
#define ENV_TEST 1
#define ENV_DEV 2

/* Full use of "autoboot" and "shutdown" commands.
 * You can cancel/enable/set time of automatic reboot,
 * or (obviously) use shutdown to stop or restart the mud immediately */
#define LVL_REBOOT_MASTER LVL_HEAD_B
/* You can stop a reboot for a while with "autoboot postpone" */
#define LVL_REBOOT_POSTPONE LVL_GOD
/* You can get the time of the next automatic reboot with "autoboot" or "world"
 */
#define LVL_REBOOT_VIEW LVL_GOD

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
#define CD_INNATE_FEATHER_FALL 18
#define CD_INNATE_SYLL 19
#define CD_INNATE_TREN 20
#define CD_INNATE_TASS 21
#define CD_INNATE_BRILL 22
#define CD_INNATE_ASCEN 23
#define CD_INNATE_HARNESS 24
#define CD_BREATHE 25
#define CD_INNATE_CREATE 26
#define CD_INNATE_ILLUMINATION 27
#define CD_INNATE_FAERIE_STEP 28
#define CD_MUSIC_1 29
#define CD_MUSIC_2 30
#define CD_MUSIC_3 31
#define CD_MUSIC_4 32
#define CD_MUSIC_5 33
#define CD_MUSIC_6 34
#define CD_MUSIC_7 35
#define CD_INNATE_BLINDING_BEAUTY 36
#define CD_INNATE_STATUE 37
#define CD_INNATE_BARKSKIN 38
#define NUM_COOLDOWNS 39

// Money
#define PLATINUM 0
#define PLAT PLATINUM
#define GOLD 1
#define SILVER 2
#define COPPER 3
#define NUM_COIN_TYPES 4

// Effects
/* Effect bits: used in CharData.char_specials.effects */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define EFF_BLIND 0         /* (R) Char is blind            */
#define EFF_INVISIBLE 1     /* Char is invisible            */
#define EFF_DETECT_ALIGN 2  /* Char is sensitive to align   */
#define EFF_DETECT_INVIS 3  /* Char can see invis chars     */
#define EFF_DETECT_MAGIC 4  /* Char is sensitive to magic   */
#define EFF_SENSE_LIFE 5    /* Char can sense hidden life   */
#define EFF_WATERWALK 6     /* Char can walk on water       */
#define EFF_SANCTUARY 7     /* Char protected by sanct.     */
#define EFF_CONFUSION 8     /* Char is confused             */
#define EFF_CURSE 9         /* Char is cursed               */
#define EFF_INFRAVISION 10  /* Char can see in dark         */
#define EFF_POISON 11       /* (R) Char is poisoned         */
#define EFF_PROTECT_EVIL 12 /* Char protected from evil     */
#define EFF_PROTECT_GOOD 13 /* Char protected from good     */
#define EFF_SLEEP 14        /* (R) Char magically asleep    */
#define EFF_NOTRACK 15      /* Char can't be tracked        */
#define EFF_TAMED 16        /* Tamed!                       */
#define EFF_BERSERK 17      /* Char is berserking           */
#define EFF_SNEAK 18        /* Char is sneaking             */
#define EFF_STEALTH 19      /* Char is using stealth        */
#define EFF_FLY 20          /* Char has the ability to fly  */
#define EFF_CHARM 21        /* Char is charmed              */
#define EFF_STONE_SKIN 22
#define EFF_FARSEE 23
#define EFF_HASTE 24
#define EFF_BLUR 25
#define EFF_VITALITY 26
#define EFF_GLORY 27
#define EFF_MAJOR_PARALYSIS 28
#define EFF_FAMILIARITY 29 /* Char is considered friend    */
#define EFF_MESMERIZED 30  /* Super fasciated by something */
#define EFF_IMMOBILIZED 31 /* Char cannot move             */
#define EFF_LIGHT 32
#define EFF_NIMBLE 33
#define EFF_MINOR_PARALYSIS 34
#define EFF_HURT_THROAT 35
#define EFF_FEATHER_FALL 36
#define EFF_WATERBREATH 37
#define EFF_SOULSHIELD 38
#define EFF_SILENCE 39
#define EFF_PROT_FIRE 40
#define EFF_PROT_COLD 41
#define EFF_PROT_AIR 42
#define EFF_PROT_EARTH 43
#define EFF_FIRESHIELD 44
#define EFF_COLDSHIELD 45
#define EFF_MINOR_GLOBE 46
#define EFF_MAJOR_GLOBE 47
#define EFF_HARNESS 48
#define EFF_ON_FIRE 49
#define EFF_FEAR 50
#define EFF_TONGUES 51
#define EFF_DISEASE 52
#define EFF_INSANITY 53
#define EFF_ULTRAVISION 54
#define EFF_NEGATE_HEAT 55
#define EFF_NEGATE_COLD 56
#define EFF_NEGATE_AIR 57
#define EFF_NEGATE_EARTH 58
#define EFF_REMOTE_AGGR 59    /* Your aggro action won't remove invis/bless etc. */
#define EFF_FIREHANDS 60      /* Make Monks do burn damage with their hands */
#define EFF_ICEHANDS 61       /* Make Monks do cold damage with their hands */
#define EFF_LIGHTNINGHANDS 62 /* Make Monks do shock damage with their hands */
#define EFF_ACIDHANDS 63      /* Make Monks do acid damage with their hands */
#define EFF_AWARE 64
#define EFF_REDUCE 65
#define EFF_ENLARGE 66
#define EFF_VAMP_TOUCH 67
#define EFF_RAY_OF_ENFEEB 68
#define EFF_ANIMATED 69
#define EFF_EXPOSED 70
#define EFF_SHADOWING 71
#define EFF_CAMOUFLAGED 72
#define EFF_SPIRIT_WOLF 73
#define EFF_SPIRIT_BEAR 74
#define EFF_WRATH 75
#define EFF_MISDIRECTION 76  /* Capable of performing misdirection */
#define EFF_MISDIRECTING 77  /* Currently actually moving but misdirecting */
#define EFF_BLESS 78         /* When blessed, your barehand attacks hurt ether chars */
                             /* AVAILABLE */
#define EFF_DETECT_POISON 80 /* Char is sensitive to poison */
#define EFF_SONG_OF_REST 81
#define EFF_DISPLACEMENT 82
#define EFF_GREATER_DISPLACEMENT 83
#define EFF_FIRE_WEAPON 84
#define EFF_ICE_WEAPON 85
#define EFF_POISON_WEAPON 86
#define EFF_ACID_WEAPON 87
#define EFF_SHOCK_WEAPON 88
#define EFF_RADIANT_WEAPON 89
#define NUM_EFF_FLAGS 90 /* Keep me updated */

/* Preference flags: used by CharData.player_specials.pref */
#define PRF_BRIEF 0       /* Room descs won't normally be shown */
#define PRF_COMPACT 1     /* No extra CRLF pair before prompts  */
#define PRF_DEAF 2        /* Can't hear shouts                  */
#define PRF_NOTELL 3      /* Can't receive tells                */
#define PRF_OLCCOMM 4     /* Can hear communication in OLC      */
#define PRF_LINENUMS 5    /* Autodisplay linenums in stringedit */
#define PRF_AUTOLOOT 6    /* Auto loot corpses when you kill    */
#define PRF_AUTOEXIT 7    /* Display exits in a room            */
#define PRF_NOHASSLE 8    /* Aggr mobs won't attack             */
#define PRF_QUEST 9       /* On quest                           */
#define PRF_SUMMONABLE 10 /* Can be summoned                    */
#define PRF_NOREPEAT 11   /* No repetition of comm commands     */
#define PRF_HOLYLIGHT 12  /* Can see in dark                    */
#define PRF_COLOR_1 13    /* Color (low bit)                    */
#define PRF_COLOR_2 14    /* Color (high bit)                   */
#define PRF_NOWIZ 15      /* Can't hear wizline                 */
#define PRF_LOG1 16       /* On-line System Log (low bit)       */
#define PRF_LOG2 17       /* On-line System Log (high bit)      */
#define PRF_AFK 18        /* away from keyboard                 */
#define PRF_NOGOSS 19     /* Can't hear gossip channel          */
#define PRF_NOHINTS 20    /* No hints when mistyping commands   */
#define PRF_ROOMFLAGS 21  /* Can see room flags (ROOM_x)        */
#define PRF_NOPETI 22     /* Can't hear petitions               */
#define PRF_AUTOSPLIT 23  /* Auto split coins from corpses      */
#define PRF_NOCLANCOMM 24 /* Can't hear clan communication      */
#define PRF_ANON 25       /* Anon flag                          */
#define PRF_SHOWVNUMS 26  /* Show Virtual Numbers               */
#define PRF_NICEAREA 27
#define PRF_VICIOUS 28
#define PRF_PASSIVE 29 /* char will not engage upon being cast on */
#define PRF_ROOMVIS 30
#define PRF_NOFOLLOW 31  /* Cannot follow / well to this player*/
#define PRF_AUTOTREAS 32 /* Automatically loots treasure from corpses */
#define PRF_EXPAND_OBJS 33
#define PRF_EXPAND_MOBS 34
#define PRF_SACRIFICIAL 35 /* Sacrificial spells autotarget self */
#define PRF_PETASSIST 36   /* Should your pet assist you as you fight */
#define NUM_PRF_FLAGS 37

#define CIRCLE_1 1
#define CIRCLE_2 9
#define CIRCLE_3 17
#define CIRCLE_4 25
#define CIRCLE_5 33
#define CIRCLE_6 41
#define CIRCLE_7 49
#define CIRCLE_8 57
#define CIRCLE_9 65
#define CIRCLE_10 73
#define CIRCLE_11 81
#define CIRCLE_12 89
#define CIRCLE_13 97
#define CIRCLE_14 105
#define NUM_SPELL_CIRCLES 14

#define SIZE_UNDEFINED -1
#define SIZE_TINY 0
#define SIZE_SMALL 1
#define SIZE_MEDIUM 2
#define SIZE_LARGE 3
#define SIZE_HUGE 4
#define SIZE_GIANT 5
#define SIZE_GARGANTUAN 6
#define SIZE_COLOSSAL 7
#define SIZE_TITANIC 8
#define SIZE_MOUNTAINOUS 9
#define NUM_SIZES 10

// Objects
#define SPELL_ARMOR 1
#define SPELL_TELEPORT 2
#define SPELL_BLESS 3
#define SPELL_BLINDNESS 4
#define SPELL_BURNING_HANDS 5
#define SPELL_CALL_LIGHTNING 6
#define SPELL_CHARM 7
#define SPELL_CHILL_TOUCH 8
#define SPELL_CLONE 9
#define SPELL_COLOR_SPRAY 10
#define SPELL_CONTROL_WEATHER 11
#define SPELL_CREATE_FOOD 12
#define SPELL_CREATE_WATER 13
#define SPELL_CURE_BLIND 14
#define SPELL_CURE_CRITIC 15
#define SPELL_CURE_LIGHT 16
#define SPELL_CURSE 17
#define SPELL_DETECT_ALIGN 18
#define SPELL_DETECT_INVIS 19
#define SPELL_DETECT_MAGIC 20
#define SPELL_DETECT_POISON 21
#define SPELL_DISPEL_EVIL 22
#define SPELL_EARTHQUAKE 23
#define SPELL_ENCHANT_WEAPON 24
#define SPELL_ENERGY_DRAIN 25
#define SPELL_FIREBALL 26
#define SPELL_HARM 27
#define SPELL_HEAL 28
#define SPELL_INVISIBLE 29
#define SPELL_LIGHTNING_BOLT 30
#define SPELL_LOCATE_OBJECT 31
#define SPELL_MAGIC_MISSILE 32
#define SPELL_POISON 33
#define SPELL_PROT_FROM_EVIL 34
#define SPELL_REMOVE_CURSE 35
#define SPELL_SANCTUARY 36
#define SPELL_SHOCKING_GRASP 37
#define SPELL_SLEEP 38
#define SPELL_ENHANCE_ABILITY 39
#define SPELL_SUMMON 40
#define SPELL_VENTRILOQUATE 41
#define SPELL_WORD_OF_RECALL 42
#define SPELL_REMOVE_POISON 43
#define SPELL_SENSE_LIFE 44
#define SPELL_ANIMATE_DEAD 45
#define SPELL_DISPEL_GOOD 46
#define SPELL_GROUP_ARMOR 47
#define SPELL_GROUP_HEAL 48
#define SPELL_GROUP_RECALL 49
#define SPELL_INFRAVISION 50
#define SPELL_WATERWALK 51
#define SPELL_STONE_SKIN 52
#define SPELL_FULL_HEAL 53
#define SPELL_FULL_HARM 54
#define SPELL_WALL_OF_FOG 55
#define SPELL_WALL_OF_STONE 56
#define SPELL_FLY 57
#define SPELL_SUMMON_DRACOLICH 58
#define SPELL_SUMMON_ELEMENTAL 59
#define SPELL_SUMMON_DEMON 60
#define SPELL_SUMMON_GREATER_DEMON 61
#define SPELL_DIMENSION_DOOR 62
#define SPELL_CREEPING_DOOM 63
#define SPELL_DOOM 64
#define SPELL_METEORSWARM 65
#define SPELL_BIGBYS_CLENCHED_FIST 66
#define SPELL_FARSEE 67
#define SPELL_HASTE 68
#define SPELL_BLUR 69
#define SPELL_GREATER_ENDURANCE 70
#define SPELL_MOONWELL 71
#define SPELL_INN_CHAZ 72
#define SPELL_DARKNESS 73
#define SPELL_ILLUMINATION 74
#define SPELL_COMPREHEND_LANG 75
#define SPELL_CONE_OF_COLD 76
#define SPELL_ICE_STORM 77
#define SPELL_ICE_SHARDS 78
#define SPELL_MAJOR_PARALYSIS 79
#define SPELL_VAMPIRIC_BREATH 80
#define SPELL_RESURRECT 81
#define SPELL_INCENDIARY_NEBULA 82
#define SPELL_MINOR_PARALYSIS 83
#define SPELL_CAUSE_LIGHT 84
#define SPELL_CAUSE_SERIOUS 85
#define SPELL_CAUSE_CRITIC 86
#define SPELL_PRESERVE 87
#define SPELL_CURE_SERIOUS 88
#define SPELL_VIGORIZE_LIGHT 89
#define SPELL_VIGORIZE_SERIOUS 90
#define SPELL_VIGORIZE_CRITIC 91
#define SPELL_SOULSHIELD 92
#define SPELL_DESTROY_UNDEAD 93
#define SPELL_SILENCE 94
#define SPELL_FLAMESTRIKE 95
#define SPELL_UNHOLY_WORD 96
#define SPELL_HOLY_WORD 97
#define SPELL_PLANE_SHIFT 98
#define SPELL_DISPEL_MAGIC 99
#define SPELL_MINOR_CREATION 100
#define SPELL_CONCEALMENT 101
#define SPELL_RAY_OF_ENFEEB 102
#define SPELL_FEATHER_FALL 103
#define SPELL_WIZARD_EYE 104
#define SPELL_FIRESHIELD 105
#define SPELL_COLDSHIELD 106
#define SPELL_MINOR_GLOBE 107
#define SPELL_MAJOR_GLOBE 108
#define SPELL_DISINTEGRATE 109
#define SPELL_HARNESS 110
#define SPELL_CHAIN_LIGHTNING 111
#define SPELL_MASS_INVIS 112
#define SPELL_RELOCATE 113
#define SPELL_FEAR 114
#define SPELL_CIRCLE_OF_LIGHT 115
#define SPELL_DIVINE_BOLT 116
#define SPELL_PRAYER 117
#define SPELL_ELEMENTAL_WARDING 118
#define SPELL_DIVINE_RAY 119
#define SPELL_LESSER_EXORCISM 120
#define SPELL_DECAY 121
#define SPELL_SPEAK_IN_TONGUES 122
#define SPELL_ENLIGHTENMENT 123
#define SPELL_EXORCISM 124
#define SPELL_SPINECHILLER 125
#define SPELL_WINGS_OF_HEAVEN 126
#define SPELL_BANISH 127
#define SPELL_WORD_OF_COMMAND 128
#define SPELL_DIVINE_ESSENCE 129
#define SPELL_HEAVENS_GATE 130
#define SPELL_DARK_PRESENCE 131
#define SPELL_DEMONSKIN 132
#define SPELL_DARK_FEAST 133
#define SPELL_HELL_BOLT 134
#define SPELL_DISEASE 135
#define SPELL_INSANITY 136
#define SPELL_DEMONIC_ASPECT 137
#define SPELL_HELLFIRE_BRIMSTONE 138
#define SPELL_STYGIAN_ERUPTION 139
#define SPELL_DEMONIC_MUTATION 140
#define SPELL_WINGS_OF_HELL 141
#define SPELL_SANE_MIND 142
#define SPELL_HELLS_GATE 143
#define SPELL_BARKSKIN 144
#define SPELL_NIGHT_VISION 145
#define SPELL_WRITHING_WEEDS 146
#define SPELL_CREATE_SPRING 147
#define SPELL_NOURISHMENT 148
#define SPELL_GAIAS_CLOAK 149
#define SPELL_NATURES_EMBRACE 150
#define SPELL_ENTANGLE 151
#define SPELL_INVIGORATE 152
#define SPELL_WANDERING_WOODS 153
#define SPELL_URBAN_RENEWAL 154
#define SPELL_SUNRAY 155
#define SPELL_ARMOR_OF_GAIA 156
#define SPELL_FIRE_DARTS 157
#define SPELL_MAGIC_TORCH 158
#define SPELL_SMOKE 159
#define SPELL_MIRAGE 160
#define SPELL_FLAME_BLADE 161
#define SPELL_POSITIVE_FIELD 162
#define SPELL_FIRESTORM 163
#define SPELL_MELT 164
#define SPELL_CIRCLE_OF_FIRE 165
#define SPELL_IMMOLATE 166
#define SPELL_SUPERNOVA 167
#define SPELL_CREMATE 168
#define SPELL_NEGATE_HEAT 169
#define SPELL_ACID_BURST 170
#define SPELL_ICE_DARTS 171
#define SPELL_ICE_ARMOR 172
#define SPELL_ICE_DAGGER 173
#define SPELL_FREEZING_WIND 174
#define SPELL_FREEZE 175
#define SPELL_WALL_OF_ICE 176
#define SPELL_ICEBALL 177
#define SPELL_FLOOD 178
#define SPELL_VAPORFORM 179
#define SPELL_NEGATE_COLD 180
#define SPELL_WATERFORM 181
#define SPELL_EXTINGUISH 182
#define SPELL_RAIN 183
#define SPELL_REDUCE 184
#define SPELL_ENLARGE 185
#define SPELL_IDENTIFY 186
#define SPELL_BONE_ARMOR 187
#define SPELL_SUMMON_CORPSE 188
#define SPELL_SHIFT_CORPSE 189
#define SPELL_GLORY 190
#define SPELL_ILLUSORY_WALL 191
#define SPELL_NIGHTMARE 192
#define SPELL_DISCORPORATE 193
#define SPELL_ISOLATION 194
#define SPELL_FAMILIARITY 195
#define SPELL_HYSTERIA 196
#define SPELL_MESMERIZE 197
#define SPELL_SEVERANCE 198
#define SPELL_SOUL_REAVER 199
#define SPELL_DETONATION 200
#define SPELL_FIRE_BREATH 201
#define SPELL_GAS_BREATH 202
#define SPELL_FROST_BREATH 203
#define SPELL_ACID_BREATH 204
#define SPELL_LIGHTNING_BREATH 205
#define SPELL_LESSER_ENDURANCE 206
#define SPELL_ENDURANCE 207
#define SPELL_VITALITY 208
#define SPELL_GREATER_VITALITY 209
#define SPELL_DRAGONS_HEALTH 210
#define SPELL_REBUKE_UNDEAD 211
#define SPELL_DEGENERATION 212
#define SPELL_SOUL_TAP 213
#define SPELL_NATURES_GUIDANCE 214
#define SPELL_MOONBEAM 215
#define SPELL_PHANTASM 216
#define SPELL_SIMULACRUM 217
#define SPELL_MISDIRECTION 218
#define SPELL_CONFUSION 219
#define SPELL_PHOSPHORIC_EMBERS 220
#define SPELL_RECALL 221
#define SPELL_PYRE 222
#define SPELL_IRON_MAIDEN 223
#define SPELL_FRACTURE 224
#define SPELL_FRACTURE_SHRAPNEL 225
#define SPELL_BONE_CAGE 226
#define SPELL_PYRE_RECOIL 227
#define SPELL_WORLD_TELEPORT 228
#define SPELL_INN_SYLL 229
#define SPELL_INN_TREN 230
#define SPELL_INN_TASS 231
#define SPELL_INN_BRILL 232
#define SPELL_INN_ASCEN 233
#define SPELL_SPIRIT_ARROWS 234
#define SPELL_PROT_FROM_GOOD 235
#define SPELL_ANCESTRAL_VENGEANCE 236
#define SPELL_CIRCLE_OF_DEATH 237
#define SPELL_BALEFUL_POLYMORPH 238
#define SPELL_SPIRIT_RAY 239
#define SPELL_VICIOUS_MOCKERY 240
#define SPELL_REMOVE_PARALYSIS 241
#define SPELL_CLOUD_OF_DAGGERS 242
#define SPELL_REVEAL_HIDDEN 243
#define SPELL_BLINDING_BEAUTY 244
#define SPELL_ACID_FOG 245
#define SPELL_WEB 246
#define SPELL_EARTH_BLESSING 247
#define SPELL_PROTECT_FIRE 248
#define SPELL_PROTECT_COLD 249
#define SPELL_PROTECT_ACID 250
#define SPELL_PROTECT_SHOCK 251
#define SPELL_ENHANCE_STR 252
#define SPELL_ENHANCE_DEX 253
#define SPELL_ENHANCE_CON 254
#define SPELL_ENHANCE_INT 255
#define SPELL_ENHANCE_WIS 256
#define SPELL_ENHANCE_CHA 257
#define SPELL_MONK_FIRE 258
#define SPELL_MONK_COLD 259
#define SPELL_MONK_ACID 260
#define SPELL_MONK_SHOCK 261
#define SPELL_STATUE 262
#define SPELL_WATER_BLAST 263
#define SPELL_DISPLACEMENT 264
#define SPELL_GREATER_DISPLACEMENT 265
#define SPELL_NIMBLE 266
#define SPELL_CLARITY 267

/* Insert new spells here, up to MAX_SPELLS */

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB 401
#define SKILL_BASH 402
#define SKILL_HIDE 403
#define SKILL_KICK 404
#define SKILL_PICK_LOCK 405
#define SKILL_PUNCH 406
#define SKILL_RESCUE 407
#define SKILL_SNEAK 408
#define SKILL_STEAL 409
#define SKILL_TRACK 410
#define SKILL_DUAL_WIELD 411
#define SKILL_DOUBLE_ATTACK 412
#define SKILL_BERSERK 413
#define SKILL_SPRINGLEAP 414
#define SKILL_MOUNT 415
#define SKILL_RIDING 416
#define SKILL_TAME 417
#define SKILL_THROATCUT 418
#define SKILL_DOORBASH 419
#define SKILL_PARRY 420
#define SKILL_DODGE 421
#define SKILL_RIPOSTE 422
#define SKILL_MEDITATE 423
#define SKILL_QUICK_CHANT 424
#define SKILL_2BACK 425
#define SKILL_CIRCLE 426
#define SKILL_BODYSLAM 427
#define SKILL_BIND 428
#define SKILL_SHAPECHANGE 429
#define SKILL_SWITCH 430
#define SKILL_DISARM 431
#define SKILL_DISARM_FUMBLING_WEAP 432
#define SKILL_DISARM_DROPPED_WEAP 433
#define SKILL_GUARD 434
#define SKILL_BREATHE_LIGHTNING 435
#define SKILL_SWEEP 436
#define SKILL_ROAR 437
#define SKILL_DOUSE 438
#define SKILL_AWARE 439
#define SKILL_INSTANT_KILL 440
#define SKILL_HITALL 441
#define SKILL_HUNT 442
#define SKILL_BANDAGE 443
#define SKILL_FIRST_AID 444
#define SKILL_VAMP_TOUCH 445
#define SKILL_CHANT 446
#define SKILL_SCRIBE 447
#define SKILL_SAFEFALL 448
#define SKILL_BAREHAND 449
#define SKILL_SUMMON_MOUNT 450
#define SKILL_KNOW_SPELL 451
#define SKILL_SPHERE_GENERIC 452
#define SKILL_SPHERE_FIRE 453
#define SKILL_SPHERE_WATER 454
#define SKILL_SPHERE_EARTH 455
#define SKILL_SPHERE_AIR 456
#define SKILL_SPHERE_HEALING 457
#define SKILL_SPHERE_PROT 458
#define SKILL_SPHERE_ENCHANT 459
#define SKILL_SPHERE_SUMMON 460
#define SKILL_SPHERE_DEATH 461
#define SKILL_SPHERE_DIVIN 462
#define SKILL_BLUDGEONING 463
#define SKILL_PIERCING 464
#define SKILL_SLASHING 465
#define SKILL_2H_BLUDGEONING 466
#define SKILL_2H_PIERCING 467
#define SKILL_2H_SLASHING 468
#define SKILL_MISSILE 469
/* what's this doing here ? */
/* We need a skill define for fire so we can have a damage message in the messages file. */
#define SPELL_ON_FIRE 470
#define SKILL_LAY_HANDS 471
#define SKILL_EYE_GOUGE 472
#define SKILL_RETREAT 473
#define SKILL_GROUP_RETREAT 474
#define SKILL_CORNER 475
#define SKILL_STEALTH 476
#define SKILL_SHADOW 477
#define SKILL_CONCEAL 478
#define SKILL_PECK 479
#define SKILL_CLAW 480
#define SKILL_ELECTRIFY 481
#define SKILL_TANTRUM 482
#define SKILL_GROUND_SHAKER 483
#define SKILL_BATTLE_HOWL 484
#define SKILL_MAUL 485
#define SKILL_BREATHE_FIRE 486
#define SKILL_BREATHE_FROST 487
#define SKILL_BREATHE_ACID 488
#define SKILL_BREATHE_GAS 489
#define SKILL_PERFORM 490
#define SKILL_CARTWHEEL 491
#define SKILL_LURE 492
#define SKILL_SNEAK_ATTACK 493
#define SKILL_REND 494

/* IF THIS GETS PAST 499, update CharData for skill timers! */

/* Bardic songs start at 551 and go to 600 */
#define SONG_INSPIRATION 551
#define SONG_TERROR 552
#define SONG_ENRAPTURE 553
#define SONG_HEARTHSONG 554
#define SONG_CROWN_OF_MADNESS 555
#define SONG_SONG_OF_REST 556
#define SONG_BALLAD_OF_TEARS 557
#define SONG_HEROIC_JOURNEY 558
#define SONG_FREEDOM_SONG 559
#define SONG_JOYFUL_NOISE 560

/* Monk chants go from 601 to 650 */
#define CHANT_REGENERATION 601
#define CHANT_BATTLE_HYMN 602
#define CHANT_WAR_CRY 603
#define CHANT_PEACE 604
#define CHANT_SHADOWS_SORROW_SONG 605
#define CHANT_IVORY_SYMPHONY 606
#define CHANT_ARIA_OF_DISSONANCE 607
#define CHANT_SONATA_OF_MALAISE 608
#define CHANT_APOCALYPTIC_ANTHEM 609
#define CHANT_SEED_OF_DESTRUCTION 610
#define CHANT_SPIRIT_WOLF 611
#define CHANT_SPIRIT_BEAR 612
#define CHANT_INTERMINABLE_WRATH 613
#define CHANT_HYMN_OF_SAINT_AUGUSTINE 614

/* New skills may be added here up to MAX_ABILITIES (650) */
/* Don't add spells/skills/songs/chants that will be saved past 650.  The
 * pfile won't support it.  However, you can add "npc" or "object" spells,
 * meaning that they may only be used in the game, and cannot be set on
 * players.
 */

#define TOP_SKILL_DEFINE 750
/* NEW NPC/OBJECT SPELLS can be inserted here up to 750 */

/* WEAPON ATTACK TYPES */

#define TYPE_HIT 751
#define TYPE_STING 752
#define TYPE_WHIP 753
#define TYPE_SLASH 754
#define TYPE_BITE 755
#define TYPE_BLUDGEON 756
#define TYPE_CRUSH 757
#define TYPE_POUND 758
#define TYPE_CLAW 759
#define TYPE_MAUL 760
#define TYPE_THRASH 761
#define TYPE_PIERCE 762
#define TYPE_BLAST 763
#define TYPE_PUNCH 764
#define TYPE_STAB 765
#define TYPE_FIRE 766
#define TYPE_COLD 767
#define TYPE_ACID 768
#define TYPE_SHOCK 769
#define TYPE_POISON 770
#define TYPE_ALIGN 771

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING 850

/* The general term for the abilities known as spells, skills, chants, and songs is TALENT. */
#define TALENT 0
#define SPELL 1
#define SKILL 2
#define CHANT 3
#define SONG 4

// Directions

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define UP 4
#define DOWN 5
#define NUM_OF_DIRS 6 /* number of directions in a room (nsewud) */

/* Thirst is relative to ounces of water. */
#define MAX_THIRST 48
#define HOURLY_THIRST_CHANGE 2

/* gain_exp modes */
#define GAIN_REGULAR (0)
#define GAIN_IGNORE_LEVEL_BOUNDARY (1 << 0)
#define GAIN_IGNORE_MORTAL_BOUNDARY (1 << 1)
#define GAIN_IGNORE_LOCATION (1 << 2)
#define GAIN_IGNORE_NAME_BOUNDARY (1 << 3)
#define GAIN_IGNORE_CHUNK_LIMITS (1 << 4)
#define GAIN_IGNORE_ALL ((1 << 5) - 1)

/*[ CONSTANTS ]***************************************************************/

/* Vnums for portal objects */
#define OBJ_VNUM_HEAVENSGATE 72
#define OBJ_VNUM_MOONWELL 33
#define OBJ_VNUM_HELLGATE 74

/* Item types: used by ObjData.obj_flags.type_flag */
#define ITEM_LIGHT 1       /* Item is a light source          */
#define ITEM_SCROLL 2      /* Item is a scroll                */
#define ITEM_WAND 3        /* Item is a wand                  */
#define ITEM_STAFF 4       /* Item is a staff                 */
#define ITEM_WEAPON 5      /* Item is a weapon                */
#define ITEM_FIREWEAPON 6  /* Unimplemented                   */
#define ITEM_MISSILE 7     /* Unimplemented                   */
#define ITEM_TREASURE 8    /* Item is a treasure, not gold    */
#define ITEM_ARMOR 9       /* Item is armor                   */
#define ITEM_POTION 10     /* Item is a potion                */
#define ITEM_WORN 11       /* Unimplemented                   */
#define ITEM_OTHER 12      /* Misc object                     */
#define ITEM_TRASH 13      /* Trash - shopkeeps won't buy     */
#define ITEM_TRAP 14       /* Unimplemented                   */
#define ITEM_CONTAINER 15  /* Item is a container             */
#define ITEM_NOTE 16       /* Item is note                    */
#define ITEM_DRINKCON 17   /* Item is a drink container       */
#define ITEM_KEY 18        /* Item is a key                   */
#define ITEM_FOOD 19       /* Item is food                    */
#define ITEM_MONEY 20      /* Item is money (gold)            */
#define ITEM_PEN 21        /* Item is a pen                   */
#define ITEM_BOAT 22       /* Item is a boat                  */
#define ITEM_FOUNTAIN 23   /* Item is a fountain              */
#define ITEM_PORTAL 24     /* Item teleports to another room  */
#define ITEM_ROPE 25       /* Item is used to bind chars      */
#define ITEM_SPELLBOOK 26  /* Spells can be scribed for mem   */
#define ITEM_WALL 27       /* Blocks passage in one direction */
#define ITEM_TOUCHSTONE 28 /* Item sets homeroom when touched */
#define ITEM_BOARD 29
#define ITEM_INSTRUMENT 30 /* Item is a musical instrument    */
#define NUM_ITEM_TYPES 31

/* Take/Wear flags: used by ObjData.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE (1 << 0)     /* Item can be takes         */
#define ITEM_WEAR_FINGER (1 << 1)   /* Can be worn on finger     */
#define ITEM_WEAR_NECK (1 << 2)     /* Can be worn around neck   */
#define ITEM_WEAR_BODY (1 << 3)     /* Can be worn on body       */
#define ITEM_WEAR_HEAD (1 << 4)     /* Can be worn on head       */
#define ITEM_WEAR_LEGS (1 << 5)     /* Can be worn on legs       */
#define ITEM_WEAR_FEET (1 << 6)     /* Can be worn on feet       */
#define ITEM_WEAR_HANDS (1 << 7)    /* Can be worn on hands      */
#define ITEM_WEAR_ARMS (1 << 8)     /* Can be worn on arms       */
#define ITEM_WEAR_SHIELD (1 << 9)   /* Can be used as a shield   */
#define ITEM_WEAR_ABOUT (1 << 10)   /* Can be worn about body    */
#define ITEM_WEAR_WAIST (1 << 11)   /* Can be worn around waist  */
#define ITEM_WEAR_WRIST (1 << 12)   /* Can be worn on wrist      */
#define ITEM_WEAR_WIELD (1 << 13)   /* Can be wielded            */
#define ITEM_WEAR_HOLD (1 << 14)    /* Can be held               */
#define ITEM_WEAR_2HWIELD (1 << 15) /* Can be wielded two handed */
#define ITEM_WEAR_EYES (1 << 16)    /* Can be worn on eyes       */
#define ITEM_WEAR_FACE (1 << 17)    /* Can be worn upon face     */
#define ITEM_WEAR_EAR (1 << 18)     /* Can be worn in ear        */
#define ITEM_WEAR_BADGE (1 << 19)   /* Can be worn as badge      */
#define ITEM_WEAR_OBELT (1 << 20)   /* Can be worn on belt       */
#define ITEM_WEAR_HOVER (1 << 21)   /* Will hover around you     */
#define NUM_ITEM_WEAR_FLAGS 22

/* Extra object flags: used by ObjData.obj_flags.extra_flags */
#define ITEM_GLOW 0               /* Item is glowing               */
#define ITEM_HUM 1                /* Item is humming               */
#define ITEM_NORENT 2             /* Item cannot be rented         */
#define ITEM_ANTI_BERSERKER 3     /* Not usable by berserkers      */
#define ITEM_NOINVIS 4            /* Item cannot be made invis     */
#define ITEM_INVISIBLE 5          /* Item is invisible             */
#define ITEM_MAGIC 6              /* Item is magical               */
#define ITEM_NODROP 7             /* Item can't be dropped         */
#define ITEM_PERMANENT 8          /* Item doesn't decompose        */
#define ITEM_ANTI_GOOD 9          /* Not usable by good people     */
#define ITEM_ANTI_EVIL 10         /* Not usable by evil people     */
#define ITEM_ANTI_NEUTRAL 11      /* Not usable by neutral people  */
#define ITEM_ANTI_SORCERER 12     /* Not usable by sorcerers       */
#define ITEM_ANTI_CLERIC 13       /* Not usable by clerics         */
#define ITEM_ANTI_ROGUE 14        /* Not usable by rogues          */
#define ITEM_ANTI_WARRIOR 15      /* Not usable by warriors        */
#define ITEM_NOSELL 16            /* Shopkeepers won't touch it    */
#define ITEM_ANTI_PALADIN 17      /* Not usable by paladins        */
#define ITEM_ANTI_ANTI_PALADIN 18 /* Not usable by anti-paladins   */
#define ITEM_ANTI_RANGER 19       /* Not usable by rangers         */
#define ITEM_ANTI_DRUID 20        /* Not usable by druids          */
#define ITEM_ANTI_SHAMAN 21       /* Not usable by shamans         */
#define ITEM_ANTI_ASSASSIN 22     /* Not usable by assassins       */
#define ITEM_ANTI_MERCENARY 23    /* Not usable by mercenaries     */
#define ITEM_ANTI_NECROMANCER 24  /* Not usable by necromancers    */
#define ITEM_ANTI_CONJURER 25     /* Not usable by conjurers       */
#define ITEM_NOBURN 26            /* Not destroyed by purge/fire   */
#define ITEM_NOLOCATE 27          /* Cannot be found by locate obj */
#define ITEM_DECOMP 28            /* Item is currently decomposing */
#define ITEM_FLOAT 29             /* Floats in water rooms         */
#define ITEM_NOFALL 30            /* Doesn't fall - unaffected by gravity */
#define ITEM_WAS_DISARMED 31      /* Disarmed from mob             */
#define ITEM_ANTI_MONK 32         /* Not usable by monks           */
#define ITEM_ANTI_BARD 33
#define ITEM_ELVEN 34   /* Item usable by Elves          */
#define ITEM_DWARVEN 35 /* Item usable by Dwarves        */
#define ITEM_ANTI_THIEF 36
#define ITEM_ANTI_PYROMANCER 37
#define ITEM_ANTI_CRYOMANCER 38
#define ITEM_ANTI_ILLUSIONIST 39
#define ITEM_ANTI_PRIEST 40
#define ITEM_ANTI_DIABOLIST 41
#define ITEM_ANTI_TINY 42
#define ITEM_ANTI_SMALL 43
#define ITEM_ANTI_MEDIUM 44
#define ITEM_ANTI_LARGE 45
#define ITEM_ANTI_HUGE 46
#define ITEM_ANTI_GIANT 47
#define ITEM_ANTI_GARGANTUAN 48
#define ITEM_ANTI_COLOSSAL 49
#define ITEM_ANTI_TITANIC 50
#define ITEM_ANTI_MOUNTAINOUS 51
#define NUM_ITEM_FLAGS 52

/* Modifier constants used with obj effects ('A' fields) */
#define APPLY_NONE 0           /* No effect                       */
#define APPLY_STR 1            /* Apply to strength               */
#define APPLY_DEX 2            /* Apply to dexterity              */
#define APPLY_INT 3            /* Apply to constitution           */
#define APPLY_WIS 4            /* Apply to wisdom                 */
#define APPLY_CON 5            /* Apply to constitution           */
#define APPLY_CHA 6            /* Apply to charisma               */
#define APPLY_CLASS 7          /* Reserved                        */
#define APPLY_LEVEL 8          /* Reserved                        */
#define APPLY_AGE 9            /* Apply to age                    */
#define APPLY_CHAR_WEIGHT 10   /* Apply to weight                 */
#define APPLY_CHAR_HEIGHT 11   /* Apply to height                 */
#define APPLY_MANA 12          /* Apply to max mana               */
#define APPLY_HIT 13           /* Apply to max hit points         */
#define APPLY_MOVE 14          /* Apply to max move points        */
#define APPLY_GOLD 15          /* Reserved                        */
#define APPLY_EXP 16           /* Reserved                        */
#define APPLY_AC 17            /* Apply to Armor Class            */
#define APPLY_HITROLL 18       /* Apply to hitroll                */
#define APPLY_DAMROLL 19       /* Apply to damage roll            */
#define APPLY_SAVING_PARA 20   /* Apply to save throw: paralz     */
#define APPLY_SAVING_ROD 21    /* Apply to save throw: rods       */
#define APPLY_SAVING_PETRI 22  /* Apply to save throw: petrif     */
#define APPLY_SAVING_BREATH 23 /* Apply to save throw: breath     */
#define APPLY_SAVING_SPELL 24  /* Apply to save throw: spells     */
#define APPLY_SIZE 25          /* Apply to size                   */
#define APPLY_HIT_REGEN 26
#define APPLY_FOCUS 27 /* Apply to focus level */
#define APPLY_PERCEPTION 28
#define APPLY_HIDDENNESS 29
#define APPLY_COMPOSITION 30
#define NUM_APPLY_TYPES 31

#define VAL_LIGHT_LIT 0
#define VAL_LIGHT_CAPACITY 1
#define VAL_LIGHT_REMAINING 2
#define LIGHT_PERMANENT (-1) /* For both light values 1 and 2 */

#define VAL_SCROLL_LEVEL 0
#define VAL_SCROLL_SPELL_1 1
#define VAL_SCROLL_SPELL_2 2
#define VAL_SCROLL_SPELL_3 3

#define VAL_WAND_LEVEL 0
#define VAL_WAND_MAX_CHARGES 1
#define VAL_WAND_CHARGES_LEFT 2
#define VAL_WAND_SPELL 3

#define VAL_STAFF_LEVEL 0
#define VAL_STAFF_MAX_CHARGES 1
#define VAL_STAFF_CHARGES_LEFT 2
#define VAL_STAFF_SPELL 3

#define VAL_WEAPON_HITROLL 0
#define VAL_WEAPON_DICE_NUM 1
#define VAL_WEAPON_DICE_SIZE 2
#define VAL_WEAPON_DAM_TYPE 3

#define VAL_TREASURE_AC 0
#define VAL_ARMOR_AC 0

#define VAL_POTION_LEVEL 0
#define VAL_POTION_SPELL_1 1
#define VAL_POTION_SPELL_2 2
#define VAL_POTION_SPELL_3 3

#define VAL_TRAP_SPELL 0
#define VAL_TRAP_HITPOINTS 1

#define VAL_BLOOD_ROOM 0

#define VAL_CONTAINER_CAPACITY 0
#define VAL_CONTAINER_BITS 1
#define CONT_CLOSEABLE (1 << 0) /* Container can be closed       */
#define CONT_PICKPROOF (1 << 1) /* Container is pickproof        */
#define CONT_CLOSED (1 << 2)    /* Container is closed           */
#define CONT_LOCKED (1 << 3)    /* Container is locked           */
#define VAL_CONTAINER_KEY 2
#define VAL_CONTAINER_WEIGHT_REDUCTION                                                                                 \
    4 /* Used to allow bags of holding, which reduce the weight of items carried.                                      \
       */
#define VAL_CORPSE_ID 2
#define VAL_CONTAINER_CORPSE 3
#define NOT_CORPSE 0
#define CORPSE_PC 1
#define CORPSE_NPC 2
#define CORPSE_NPC_NORAISE 3

#define VAL_DRINKCON_CAPACITY 0
#define VAL_DRINKCON_REMAINING 1
#define VAL_DRINKCON_LIQUID 2
#define LIQ_WATER 0
#define LIQ_BEER 1
#define LIQ_WINE 2
#define LIQ_ALE 3
#define LIQ_DARKALE 4
#define LIQ_WHISKY 5
#define LIQ_LEMONADE 6
#define LIQ_FIREBRT 7
#define LIQ_LOCALSPC 8
#define LIQ_SLIME 9
#define LIQ_MILK 10
#define LIQ_TEA 11
#define LIQ_COFFEE 12
#define LIQ_BLOOD 13
#define LIQ_SALTWATER 14
#define LIQ_RUM 15
#define LIQ_NECTAR 16
#define LIQ_SAKE 17
#define LIQ_CIDER 18
#define LIQ_TOMATOSOUP 19
#define LIQ_POTATOSOUP 20
#define LIQ_CHAI 21
#define LIQ_APPLEJUICE 22
#define LIQ_ORNGJUICE 23
#define LIQ_PNAPLJUICE 24
#define LIQ_GRAPEJUICE 25
#define LIQ_POMJUICE 26
#define LIQ_MELONAE 27
#define LIQ_COCOA 28
#define LIQ_ESPRESSO 29
#define LIQ_CAPPUCCINO 30
#define LIQ_MANGOLASSI 31
#define LIQ_ROSEWATER 32
#define LIQ_GREENTEA 33
#define LIQ_CHAMOMILE 34
#define LIQ_GIN 35
#define LIQ_BRANDY 36
#define LIQ_MEAD 37
#define LIQ_CHAMPAGNE 38
#define LIQ_VODKA 39
#define LIQ_TEQUILA 40
#define LIQ_ABSINTHE 41
#define NUM_LIQ_TYPES 42
#define VAL_DRINKCON_POISONED 3 /* Must = fountain/food poisoned */

#define VAL_FOOD_FILLINGNESS 0
#define VAL_FOOD_POISONED 3 /* Must = drink/fountain poisoned*/

#define VAL_MONEY_PLATINUM 0
#define VAL_MONEY_GOLD 1
#define VAL_MONEY_SILVER 2
#define VAL_MONEY_COPPER 3

#define VAL_FOUNTAIN_CAPACITY 0
#define VAL_FOUNTAIN_REMAINING 1
#define VAL_FOUNTAIN_LIQUID 2   /* Use LIQ_ constants above      */
#define VAL_FOUNTAIN_POISONED 3 /* Must = drinkcon/food poisoned */

#define VAL_PORTAL_DESTINATION 0
#define VAL_PORTAL_ENTRY_MSG 1 /* Message to room portal is in  */
#define VAL_PORTAL_CHAR_MSG 2  /* Message to character entering */
#define VAL_PORTAL_EXIT_MSG 3  /* Message to portal target room */

#define VAL_SPELLBOOK_PAGES 0
#define DEF_SPELLBOOK_PAGES 100
#define MAX_SPELLBOOK_PAGES 2000 /* Kind of arbitrary             */

#define VAL_WALL_DIRECTION 0
#define VAL_WALL_DISPELABLE 1
#define VAL_WALL_HITPOINTS 2
#define VAL_WALL_SPELL 3

#define VAL_BOARD_NUMBER 0

#define VAL_MIN (-100000) /* Arbitrary default min value */
#define VAL_MAX 100000    /* Arbitrary default max value */

#define NUM_VALUES 7

#define MAX_OBJ_APPLIES 6

#define MEDITATE_BONUS 10

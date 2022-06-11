/***************************************************************************
 * $Id: structs.h,v 1.1 2008/03/05 03:49:56 myc Exp $
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

/* preamble *************************************************************/

#define NOWHERE -1 /* nil reference for room-database        */
#define NOTHING -1 /* nil reference for objects                */
#define NOBODY -1  /* nil reference for mobiles                */

#define SPECIAL(name) int(name)(struct char_data * ch, void *me, int cmd, char *argument)
/* misc editor defines **************************************************/

/* format modes for format_text */
#define FORMAT_INDENT (1 << 0)

/* room-related defines *************************************************/

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define UP 4
#define DOWN 5
#define FUP 6
#define FDOWN 7

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK (1 << 0)         /* Dark                         */
#define ROOM_DEATH (1 << 1)        /* Death trap                   */
#define ROOM_NOMOB (1 << 2)        /* MOBs not allowed             */
#define ROOM_INDOORS (1 << 3)      /* Indoors                      */
#define ROOM_PEACEFUL (1 << 4)     /* Violence not allowed         */
#define ROOM_SOUNDPROOF (1 << 5)   /* Shouts, gossip blocked       */
#define ROOM_NOTRACK (1 << 6)      /* Track won't go through       */
#define ROOM_NOMAGIC (1 << 7)      /* Magic not allowed            */
#define ROOM_TUNNEL (1 << 8)       /* room for only 2 pers         */
#define ROOM_PRIVATE (1 << 9)      /* Can't teleport in            */
#define ROOM_GODROOM (1 << 10)     /* LVL_GOD+ only allowed        */
#define ROOM_HOUSE (1 << 11)       /* (R) Room is a house          */
#define ROOM_HOUSE_CRASH (1 << 12) /* (R) House needs saving       */
#define ROOM_ATRIUM (1 << 13)      /* (R) The door to a house      */
#define ROOM_OLC (1 << 14)         /* (R) Modifyable/!compress     */
#define ROOM_BFS_MARK (1 << 15)    /* (R) breadth-first srch mrk   */
#define ROOM_NOWELL (1 << 16)      /* No spell portals like moonwell */
#define ROOM_NORECALL (1 << 17)    /* No recalling                 */
#define ROOM_UNDERDARK (1 << 18)   /*                                      (not used) */
#define ROOM_NOSUMMON (1 << 19)    /* Can't summon to or from. Can't banish here. */
#define ROOM_NOSHIFT (1 << 20)     /* no plane shift                       (not used) */
#define ROOM_GUILDHALL (1 << 21)   /*                                      (not used) */
#define ROOM_NOSCAN (1 << 22)      /* Unable to scan to/from rooms */
#define ROOM_ALT_EXIT (1 << 23)    /* The exits to this room have been altered */
#define ROOM_MAP (1 << 24)         /* Room on the surface map              (not used) */
#define ROOM_ALWAYSLIT (1 << 25)   /* Makes the room lit */
#define ROOM_ARENA (1 << 26)       /* (safe) PK allowed in this room */
#define ROOM_OBSERVATORY (1 << 27) /* see into adjacent ARENA rooms */

/* Room affections */
#define RAFF_FOG (1 << 0)          /* Tough to see anything */
#define RAFF_DARKNESS (1 << 1)     /* Magically made dark */
#define RAFF_ILLUMINATION (1 << 2) /* Magically made lit */
#define RAFF_FOREST (1 << 3)       /* Um, magically made foresty */
#define RAFF_CIRCLE_FIRE (1 << 4)  /* This spell hurts people */

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR (1 << 0)    /* Exit is a door             */
#define EX_CLOSED (1 << 1)    /* The door is closed         */
#define EX_LOCKED (1 << 2)    /* The door is locked         */
#define EX_PICKPROOF (1 << 3) /* Lock can't be picked       */
#define EX_HIDDEN (1 << 4)    /* exit is hidden             */
#define EX_DESCRIPT (1 << 5)  /* Just an extra description  */

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE 0       /* Indoors                   */
#define SECT_CITY 1         /* In a city                 */
#define SECT_FIELD 2        /* In a field                */
#define SECT_FOREST 3       /* In a forest               */
#define SECT_HILLS 4        /* In the hills              */
#define SECT_MOUNTAIN 5     /* On a mountain             */
#define SECT_WATER_SWIM 6   /* Swimmable water           */
#define SECT_WATER_NOSWIM 7 /* Water - need a boat       */
#define SECT_UNDERWATER 8   /* Underwater                */
#define SECT_FLYING 9       /* Wheee!                    */
#define SECT_ROAD 10
#define SECT_GRASSLANDS 11
#define SECT_OCEAN 12
#define SECT_RUINS 13
#define SECT_SWAMP 14
#define SECT_BEACH 15
#define SECT_UNDERDARK 16
#define SECT_ASTRALPLANE 17
#define SECT_AIRPLANE 18
#define SECT_FIREPLANE 19
#define SECT_EARTHPLANE 20
#define SECT_EATHREALPLANE 21
#define SECT_AVERNUS 22

/* char and mob-related defines *****************************************/

/* Sex */
#define SEX_NEUTRAL 0
#define SEX_MALE 1
#define SEX_FEMALE 2

/* Size */
#define SIZE_UNDEFINED -1
#define SIZE_TINY 0
#define SIZE_SMALL 1
#define SIZE_MEDIUM 2
#define SIZE_LARGE 3
#define SIZE_HUGE 4
#define SIZE_GIANT 5
#define SIZE_GARGANTUAN 6
#define SIZE_COLOSSAL 7
#define NUM_SIZES 8

/*postures*/
#define POS1_PRONE 0
#define POS1_KNEELING 1
#define POS1_SITTING 2
#define POS1_RESTING 3
#define POS1_STANDING 4

/* Positions */
#define POS_DEAD 0      /* dead                 */
#define POS_MORTALLYW 1 /* mortally wounded     */
#define POS_INCAP 2     /* incapacitated        */
#define POS_STUNNED 3   /* stunned              */
#define POS_SLEEPING 4  /* sleeping             */
#define POS_RESTING 5   /* resting              */
#define POS_SITTING 6   /* sitting              */
#define POS_FIGHTING 7  /* fighting             */
#define POS_STANDING 8  /* standing             */
#define POS_FLYING 9    /* flying               */

#define HIT_INCAP -3     /* The hit level for incapacitation   */
#define HIT_MORTALLYW -6 /* The hit level for mortally wound   */
#define HIT_DEAD -11     /* The point you never want to get to */

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER (1 << 0)     /* a player-killer                           */
#define PLR_THIEF (1 << 1)      /* a player-thief                            */
#define PLR_FROZEN (1 << 2)     /* is frozen                                 */
#define PLR_DONTSET (1 << 3)    /* Don't EVER set (ISNPC bit)                */
#define PLR_WRITING (1 << 4)    /* writing (board/mail/olc)                  */
#define PLR_MAILING (1 << 5)    /* is writing mail                           */
#define PLR_CRASH (1 << 6)      /* needs to be crash-saved                   */
#define PLR_SITEOK (1 << 7)     /* has been site-cleared                     */
#define PLR_NOSHOUT (1 << 8)    /* not allowed to shout/goss                 */
#define PLR_NOTITLE (1 << 9)    /* not allowed to set title       (not used) */
#define PLR_DELETED (1 << 10)   /* deleted - space reusable       (not used) */
#define PLR_LOADROOM (1 << 11)  /* uses nonstandard loadroom      (not used) */
#define PLR_NOWIZLIST (1 << 12) /* shouldn't be on wizlist        (not used) */
#define PLR_NODELETE (1 << 13)  /* shouldn't be deleted           (may be used outside the server) */
#define PLR_INVSTART (1 << 14)  /* should enter game wizinvis                */
#define PLR_CRYO (1 << 15)      /* is cryo-saved (purge prog)     (not used) */
#define PLR_MEDITATE (1 << 16)  /* meditating - improves spell memorization  */
#define PLR_CASTING (1 << 17)   /* currently casting a spell      (not used) */
#define PLR_BOUND (1 << 18)     /* tied up                        (not used) */
#define PLR_SCRIBE (1 << 19)    /* scribing                       (not used) */
#define PLR_TEACHING (1 << 20)  /* teaching a skill/spell         (not used) */
#define PLR_NAPPROVE (1 << 21)  /* name not approved yet                     */
#define PLR_NEWNAME (1 << 22)   /* needs to choose a new name                */

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC (1 << 0)          /* Mob has a callable spec-proc       */
#define MOB_SENTINEL (1 << 1)      /* Mob should not move                */
#define MOB_SCAVENGER (1 << 2)     /* Mob picks up stuff on the ground   */
#define MOB_ISNPC (1 << 3)         /* (R) Automatically set on all Mobs  */
#define MOB_AWARE (1 << 4)         /* Mob can't be backstabbed           */
#define MOB_AGGRESSIVE (1 << 5)    /* Mob hits players in the room       */
#define MOB_STAY_ZONE (1 << 6)     /* Mob shouldn't wander out of zone   */
#define MOB_WIMPY (1 << 7)         /* Mob flees if severely injured      */
#define MOB_AGGR_EVIL (1 << 8)     /* auto attack evil PC's              */
#define MOB_AGGR_GOOD (1 << 9)     /* auto attack good PC's              */
#define MOB_AGGR_NEUTRAL (1 << 10) /* auto attack neutral PC's           */
#define MOB_MEMORY (1 << 11)       /* remember attackers if attacked     */
#define MOB_HELPER (1 << 12)       /* attack PCs fighting other NPCs     */
#define MOB_NOCHARM (1 << 13)      /* Mob can't be charmed               */
#define MOB_NOSUMMON (1 << 14)     /* Mob can't be summoned              */
#define MOB_NOSLEEP (1 << 15)      /* Mob can't be slept                 */
#define MOB_NOBASH (1 << 16)       /* Mob can't be bashed (e.g. trees)   */
#define MOB_NOBLIND (1 << 17)      /* Mob can't be blinded               */
#define MOB_MOUNTABLE (1 << 18)
#define MOB_ARRESTOR (1 << 19)
#define MOB_FAST_TRACK (1 << 20)
#define MOB_SLOW_TRACK (1 << 21)
#define MOB_CASTING (1 << 22) /* mob casting            (not used)  */
#define MOB_PET (1 << 23)
#define MOB_AQUATIC (1 << 24) /* Mob can't enter non-water rooms */
#define MOB_AGGR_EVIL_RACE (1 << 25)
#define MOB_AGGR_GOOD_RACE (1 << 26)
#define MOB_NOSILENCE (1 << 27)
#define MOB_NOVICIOUS (1 << 28)
#define MOB_TEACHER (1 << 29)
#define MOB_ANIMATED (1 << 30) /* mob is animated - die if no anim effect */
#define MOB_PEACEFUL (1 << 31) /* mob can't be attacked.                */

#define MOB2_NOPOISON (1 << 0)        /* Mob cannot be poisoned.           */
#define MOB2_ILLUSORY (1 << 1)        /* is an illusion: does no harm, leaves no corpse */
#define MOB2_PLAYER_PHANTASM (1 << 2) /* illusion of player; mobs are aggro to */
#define MOB2_NO_CLASS_AI (1 << 3)     /* Mob does not execute class AI */

#define IS_VICIOUS(ch)                                                                                                 \
    ((IS_NPC(ch) && !MOB_FLAGGED((ch), MOB_NOVICIOUS)) || (!IS_NPC(ch) && PRF_FLAGGED((ch), PRF_VICIOUS)))

/* Some mount stuff */
#define MAX_MOUNT_LEVEL 27 /* The maximum level of mountable mobs */

/* mount_level_fudge is how far above the ideal mountable level you could possibly
 * hope to mount.  For example if your skill allowed you to mount level 10 mobs
 * easily, and mount_level_fudge was 4, you could mount level 14 mobs with great
 * difficulty, but nothing higher. */
#define MOUNT_LEVEL_FUDGE (double)(MAX_MOUNT_LEVEL / 6)

#define MOUNT_MINMOVE 20
#define MOUNT_MAXMOVE 250

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF (1 << 0)       /* Room descs won't normally be shown */
#define PRF_COMPACT (1 << 1)     /* No extra CRLF pair before prompts  */
#define PRF_DEAF (1 << 2)        /* Can't hear shouts                  */
#define PRF_NOTELL (1 << 3)      /* Can't receive tells                */
#define PRF_OLCCOMM (1 << 4)     /* Can hear communication in OLC      */
#define PRF_UNUSED1 (1 << 5)     /* Display mana points in prompt      */
#define PRF_UNUSED2 (1 << 6)     /* Display move points in prompt      */
#define PRF_AUTOEXIT (1 << 7)    /* Display exits in a room            */
#define PRF_NOHASSLE (1 << 8)    /* Aggr mobs won't attack             */
#define PRF_QUEST (1 << 9)       /* On quest                           */
#define PRF_SUMMONABLE (1 << 10) /* Can be summoned                    */
#define PRF_NOREPEAT (1 << 11)   /* No repetition of comm commands     */
#define PRF_HOLYLIGHT (1 << 12)  /* Can see in dark                    */
#define PRF_COLOR_1 (1 << 13)    /* Color (low bit)                    */
#define PRF_COLOR_2 (1 << 14)    /* Color (high bit)                   */
#define PRF_NOWIZ (1 << 15)      /* Can't hear wizline                 */
#define PRF_LOG1 (1 << 16)       /* On-line System Log (low bit)       */
#define PRF_LOG2 (1 << 17)       /* On-line System Log (high bit)      */
#define PRF_AFK (1 << 18)        /* away from keyboard                 */
#define PRF_NOGOSS (1 << 19)     /* Can't hear gossip channel          */
#define PRF_NOHINTS (1 << 20)    /* No hints when mistyping commands   */
#define PRF_ROOMFLAGS (1 << 21)  /* Can see room flags (ROOM_x)        */
#define PRF_NOPETI (1 << 22)     /* Can't hear petitions               */
#define PRF_NONAME (1 << 23)     /* lets god hide name on title        */
#define PRF_NOCTELL (1 << 24)    /* Can't hear clan tell               */
#define PRF_ANON (1 << 25)       /* Anon flag                          */
#define PRF_SHOWVNUMS (1 << 26)  /* Show Virtual Numbers               */
#define PRF_NICEAREA (1 << 27)
#define PRF_VICIOUS (1 << 28)
#define PRF_PASSIVE (1 << 29) /* char will not engage upon being cast on */
#define PRF_ROOMVIS (1 << 30)
#define PRF_NOFOLLOW (1 << 31) /* Cannot follow / well to this player*/

/* max defines here is at 31!!!! And we're there. */

#define ACT_DELAY_BASH 0
#define ACT_DELAY_BERSERK 1
#define ACT_DELAY_INSTANTKILL 2
#define ACT_DELAY_DISARM 3          /* pulse violence */
#define ACT_DELAY_FUMBLING_PRIM 4   /* pulse violence */
#define ACT_DELAY_DROPPED_PRIM 5    /* pulse violence */
#define ACT_DELAY_FUMBLING_SECOND 6 /* pulse violence */
#define ACT_DELAY_DROPPED_SECOND 7  /* pulse violence */
#define ACT_DELAY_HEADBUTT 8        /* pulse violence */
#define ACT_DELAY_9_UNDEFINED 9
#define ACT_DELAY_ARCHER 10
#define ACT_DELAY_SUMMON_MOUNT 11
#define ACT_DELAY_LAY_HANDS 12
#define ACT_DELAY_FIRST_AID 13
#define MAX_ACTION_DELAYS 14 /* number of commands with special delays */

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND (1 << 0)         /* (R) Char is blind                */
#define AFF_INVISIBLE (1 << 1)     /* Char is invisible                */
#define AFF_DETECT_ALIGN (1 << 2)  /* Char is sensitive to align*/
#define AFF_DETECT_INVIS (1 << 3)  /* Char can see invis chars  */
#define AFF_DETECT_MAGIC (1 << 4)  /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE (1 << 5)    /* Char can sense hidden life*/
#define AFF_WATERWALK (1 << 6)     /* Char can walk on water        */
#define AFF_SANCTUARY (1 << 7)     /* Char protected by sanct.        */
#define AFF_GROUP (1 << 8)         /* (R) Char is grouped        */
#define AFF_CURSE (1 << 9)         /* Char is cursed                */
#define AFF_INFRAVISION (1 << 10)  /* Char can see in dark        */
#define AFF_POISON (1 << 11)       /* (R) Char is poisoned        */
#define AFF_PROTECT_EVIL (1 << 12) /* Char protected from evil  */
#define AFF_PROTECT_GOOD (1 << 13) /* Char protected from good  */
#define AFF_SLEEP (1 << 14)        /* (R) Char magically asleep        */
#define AFF_NOTRACK (1 << 15)      /* Char can't be tracked        */
#define AFF_TAMED (1 << 16)        /* Room for future expansion        */
#define AFF_BERSERK (1 << 17)      /* Char is berserking */
#define AFF_SNEAK (1 << 18)        /* Char is sneaking                */
                                   /* Room for future expansion        (19) */
#define AFF_FLYING (1 << 20)       /* Room for future expansion        */
#define AFF_CHARM (1 << 21)        /* Char is charmed                */
#define AFF_STONE_SKIN (1 << 22)
#define AFF_FARSEE (1 << 23)
#define AFF_HASTE (1 << 24)
#define AFF_BLUR (1 << 25)
#define AFF_VIT (1 << 26)
#define AFF_COMP_LANG (1 << 27)
#define AFF_MAJOR_PARA (1 << 28)
#define AFF_FUMBLING_PRIM (1 << 29)
#define AFF_FUMBLING_SECOND (1 << 30)

/* AFF2_XXX -> affected_by2 O yeah baby Banyal*/
#define AFF2_LIGHT (1 << 0)
#define AFF2_MGROUP (1 << 1)
#define AFF2_MINOR_PARALYSIS (1 << 2)
#define AFF2_KNOCKED_OUT (1 << 3)
#define AFF2_LEVITATE (1 << 4)
#define AFF2_WATERBREATH (1 << 5)
#define AFF2_SOULSHIELD (1 << 6)
#define AFF2_SILENCE (1 << 7)
#define AFF2_PROT_FIRE (1 << 8)
#define AFF2_PROT_COLD (1 << 9)
#define AFF2_PROT_AIR (1 << 10)
#define AFF2_PROT_EARTH (1 << 11)
#define AFF2_FIRESHIELD (1 << 12)
#define AFF2_COLDSHIELD (1 << 13)
#define AFF2_MINOR_GLOBE (1 << 14)
#define AFF2_MAJOR_GLOBE (1 << 15)
#define AFF2_HARNESS (1 << 16)
#define AFF2_ON_FIRE (1 << 17)
#define AFF2_FEAR (1 << 18)
#define AFF2_TONGUES (1 << 19)
#define AFF2_DISEASE (1 << 20)
#define AFF2_INSANITY (1 << 21)
#define AFF2_ULTRAVISION (1 << 22)
#define AFF2_NEGATE_HEAT (1 << 23)
#define AFF2_NEGATE_COLD (1 << 24)
#define AFF2_NEGATE_AIR (1 << 25)
#define AFF2_NEGATE_EARTH (1 << 26)
#define AFF2_WATERFORM (1 << 27)
#define AFF2_DROPPED_PRIM (1 << 28)
#define AFF2_DROPPED_SECOND (1 << 29)

/* AFF3_XXX -> affected_by3 O yeah baby Banyal*/
#define AFF3_AWARE (1 << 0)
#define AFF3_REDUCE (1 << 1)
#define AFF3_ENLARGE (1 << 2)
#define AFF3_VAMP_TOUCH (1 << 3)
#define AFF3_RAY_OF_ENFEEB (1 << 4)
#define AFF3_ANIMATED (1 << 5)
#define AFF3_VAPORFORM (1 << 6)
#define AFF3_SHADOWING (1 << 7)
#define AFF3_CAMOUFLAGED (1 << 8)
#define AFF3_SPIRIT_WOLF (1 << 9)
#define AFF3_SPIRIT_BEAR (1 << 10)
#define AFF3_WRATH (1 << 11)

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING 0       /* Playing - Nominal state        */
#define CON_CLOSE 1         /* Disconnecting                */
#define CON_GET_NAME 2      /* By what name ..?                */
#define CON_NAME_CNFRM 3    /* Did I get that right, x?        */
#define CON_PASSWORD 4      /* Password:                        */
#define CON_NEWPASSWD 5     /* Give me a password for x        */
#define CON_CNFPASSWD 6     /* Please retype password:        */
#define CON_QSEX 7          /* Sex?                                */
#define CON_QCLASS 8        /* Class?                        */
#define CON_RMOTD 9         /* PRESS RETURN after MOTD        */
#define CON_MENU 10         /* Your choice: (main menu)        */
#define CON_EXDESC 11       /* Enter a new description:        */
#define CON_CHPWD_GETOLD 12 /* Changing passwd: get old        */
#define CON_CHPWD_GETNEW 13 /* Changing passwd: get new        */
#define CON_CHPWD_VRFY 14   /* Verify new password                */
#define CON_DELCNF1 15      /* Delete confirmation 1        */
#define CON_DELCNF2 16      /* Delete confirmation 2        */
#define CON_QRACE 17        /* Complete Race Seletion Menu  */
#define CON_QANSI 18        /* Prompt for term type         */
#define CON_OEDIT 19        /*. OLC mode - object edit     .*/
#define CON_REDIT 20        /*. OLC mode - room edit       .*/
#define CON_ZEDIT 21        /*. OLC mode - zone info edit  .*/
#define CON_MEDIT 22        /*. OLC mode - mobile edit     .*/
#define CON_SEDIT 23        /*. OLC mode - shop edit       .*/
#define CON_QROLLSTATS 24
#define CON_QHOMETOWN 25
#define CON_QBONUS1 26
#define CON_QBONUS2 27
#define CON_QBONUS3 28
#define CON_QCANCHAR 29
#define CON_TEXTED 30
#define CON_HEDIT 31     /*. OLC mode - help edit       .*/
#define CON_TRIGEDIT 32  /*. OLC mode - trigger edit    .*/
#define CON_CLASSHELP 33 /* Char Gen Class Help          */
#define CON_SDEDIT 34
#define CON_QDIETY 35
#define CON_NAME_CHECK 36
#define CON_NAME_WAIT_APPROVAL 37 /* await imm aprroval of name   */
#define CON_NEW_NAME 38           /* name declined, get a new one */
#define CON_QGOODRACE 39          /* Menu Choice for Good races.  */
#define CON_ISPELL_BOOT 40        /* Obligatory disconnect bad names */
#define CON_CLAN_DESC_EDIT 41

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

/* object-related defines ********************************************/

/* vnums for portal objects */
#define HEAVENSGATE_OBJ 72
#define MOONWELL_OBJ 33
#define HELLGATE_OBJ 74

/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT 1      /* Item is a light source        */
#define ITEM_SCROLL 2     /* Item is a scroll                */
#define ITEM_WAND 3       /* Item is a wand                */
#define ITEM_STAFF 4      /* Item is a staff                */
#define ITEM_WEAPON 5     /* Item is a weapon                */
#define ITEM_FIREWEAPON 6 /* Unimplemented                */
#define ITEM_MISSILE 7    /* Unimplemented                */
#define ITEM_TREASURE 8   /* Item is a treasure, not gold        */
#define ITEM_ARMOR 9      /* Item is armor                */
#define ITEM_POTION 10    /* Item is a potion                */
#define ITEM_WORN 11      /* Unimplemented                */
#define ITEM_OTHER 12     /* Misc object                        */
#define ITEM_TRASH 13     /* Trash - shopkeeps won't buy        */
#define ITEM_TRAP 14      /* Unimplemented                */
#define ITEM_CONTAINER 15 /* Item is a container                */
#define ITEM_NOTE 16      /* Item is note                 */
#define ITEM_DRINKCON 17  /* Item is a drink container        */
#define ITEM_KEY 18       /* Item is a key                */
#define ITEM_FOOD 19      /* Item is food                        */
#define ITEM_MONEY 20     /* Item is money (gold)                */
#define ITEM_PEN 21       /* Item is a pen                */
#define ITEM_BOAT 22      /* Item is a boat                */
#define ITEM_FOUNTAIN 23  /* Item is a fountain                */
#define ITEM_PORTAL 24
#define ITEM_ROPE 25
#define ITEM_SPELLBOOK 26
#define ITEM_WALL 27
#define ITEM_TOUCHSTONE 28 /* Item sets homeroom when touched */

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE (1 << 0)     /* Item can be takes                */
#define ITEM_WEAR_FINGER (1 << 1)   /* Can be worn on finger        */
#define ITEM_WEAR_NECK (1 << 2)     /* Can be worn around neck         */
#define ITEM_WEAR_BODY (1 << 3)     /* Can be worn on body         */
#define ITEM_WEAR_HEAD (1 << 4)     /* Can be worn on head         */
#define ITEM_WEAR_LEGS (1 << 5)     /* Can be worn on legs        */
#define ITEM_WEAR_FEET (1 << 6)     /* Can be worn on feet        */
#define ITEM_WEAR_HANDS (1 << 7)    /* Can be worn on hands        */
#define ITEM_WEAR_ARMS (1 << 8)     /* Can be worn on arms        */
#define ITEM_WEAR_SHIELD (1 << 9)   /* Can be used as a shield        */
#define ITEM_WEAR_ABOUT (1 << 10)   /* Can be worn about body         */
#define ITEM_WEAR_WAIST (1 << 11)   /* Can be worn around waist         */
#define ITEM_WEAR_WRIST (1 << 12)   /* Can be worn on wrist         */
#define ITEM_WEAR_WIELD (1 << 13)   /* Can be wielded                */
#define ITEM_WEAR_HOLD (1 << 14)    /* Can be held                */
#define ITEM_WEAR_2HWIELD (1 << 15) /* Can be wielded two handed */
#define ITEM_WEAR_EYES (1 << 16)
#define ITEM_WEAR_FACE (1 << 17)
#define ITEM_WEAR_EAR (1 << 18)
#define ITEM_WEAR_BADGE (1 << 19)
#define ITEM_WEAR_OBELT (1 << 20)
#define ITEM_WEAR_HOVER (1 << 21)

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW (1 << 0)           /* Item is glowing               */
#define ITEM_HUM (1 << 1)            /* Item is humming               */
#define ITEM_NORENT (1 << 2)         /* Item cannot be rented         */
#define ITEM_NODONATE (1 << 3)       /* Item cannot be donated        */
#define ITEM_NOINVIS (1 << 4)        /* Item cannot be made invis     */
#define ITEM_INVISIBLE (1 << 5)      /* Item is invisible             */
#define ITEM_MAGIC (1 << 6)          /* Item is magical               */
#define ITEM_NODROP (1 << 7)         /* Item can't be dropped         */
#define ITEM_BLESS (1 << 8)          /* Item is blessed               */
#define ITEM_ANTI_GOOD (1 << 9)      /* Not usable by good people     */
#define ITEM_ANTI_EVIL (1 << 10)     /* Not usable by evil people     */
#define ITEM_ANTI_NEUTRAL (1 << 11)  /* Not usable by neutral people  */
#define ITEM_ANTI_SORCERER (1 << 12) /* Not usable by sorcerers       */
#define ITEM_ANTI_CLERIC (1 << 13)   /* Not usable by clerics         */
#define ITEM_ANTI_ROGUE (1 << 14)    /* Not usable by rogues          */
#define ITEM_ANTI_WARRIOR (1 << 15)  /* Not usable by warriors        */
#define ITEM_NOSELL (1 << 16)        /* Shopkeepers won't touch it    */
#define ITEM_ANTI_PALADIN (1 << 17)
#define ITEM_ANTI_ANTI_PALADIN (1 << 18)
#define ITEM_ANTI_RANGER (1 << 19)
#define ITEM_ANTI_DRUID (1 << 20)
#define ITEM_ANTI_SHAMAN (1 << 21)
#define ITEM_ANTI_ASSASSIN (1 << 22)
#define ITEM_ANTI_MERCENARY (1 << 23)
#define ITEM_ANTI_NECROMANCER (1 << 24)
#define ITEM_ANTI_CONJURER (1 << 25)
#define ITEM_NOBURN (1 << 26)
#define ITEM_NOLOCATE (1 << 27)
#define ITEM_TRANSIENT (1 << 28)
#define ITEM_FLOAT (1 << 29)
#define ITEM_CONT_LIGHT (1 << 30) /* NOT USED - available */
#define ITEM_WAS_DISARMED (1 << 31)
#define ITEM_ANTI_MONK (1 << 32)

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE 0           /* No effect                        */
#define APPLY_STR 1            /* Apply to strength                */
#define APPLY_DEX 2            /* Apply to dexterity                */
#define APPLY_INT 3            /* Apply to constitution        */
#define APPLY_WIS 4            /* Apply to wisdom                */
#define APPLY_CON 5            /* Apply to constitution        */
#define APPLY_CHA 6            /* Apply to charisma                */
#define APPLY_CLASS 7          /* Reserved                        */
#define APPLY_LEVEL 8          /* Reserved                        */
#define APPLY_AGE 9            /* Apply to age                        */
#define APPLY_CHAR_WEIGHT 10   /* Apply to weight                */
#define APPLY_CHAR_HEIGHT 11   /* Apply to height                */
#define APPLY_MANA 12          /* Apply to max mana                */
#define APPLY_HIT 13           /* Apply to max hit points        */
#define APPLY_MOVE 14          /* Apply to max move points        */
#define APPLY_GOLD 15          /* Reserved                        */
#define APPLY_EXP 16           /* Reserved                        */
#define APPLY_AC 17            /* Apply to Armor Class                */
#define APPLY_HITROLL 18       /* Apply to hitroll                */
#define APPLY_DAMROLL 19       /* Apply to damage roll                */
#define APPLY_SAVING_PARA 20   /* Apply to save throw: paralz        */
#define APPLY_SAVING_ROD 21    /* Apply to save throw: rods        */
#define APPLY_SAVING_PETRI 22  /* Apply to save throw: petrif        */
#define APPLY_SAVING_BREATH 23 /* Apply to save throw: breath        */
#define APPLY_SAVING_SPELL 24  /* Apply to save throw: spells        */
#define APPLY_SIZE 25          /*WELL SHIT LETS SET SIZE...BANYAL*/
#define APPLY_HIT_REGEN 26
#define APPLY_MANA_REGEN 27
#define APPLY_PERCEPTION 28
#define APPLY_HIDDENNESS 29

/* Container flags - value[1] */
#define CONT_CLOSEABLE (1 << 0) /* Container can be closed        */
#define CONT_PICKPROOF (1 << 1) /* Container is pickproof        */
#define CONT_CLOSED (1 << 2)    /* Container is closed                */
#define CONT_LOCKED (1 << 3)    /* Container is locked                */

/* Some different kind of liquids for use in values of drink containers */
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
#define NUM_LIQ_TYPES 37

/* other miscellaneous defines *******************************************/

/* Player conditions */
#define DRUNK 0
#define FULL 1
#define THIRST 2

/*player innatetimes */
#define INNATE_INVISIBLE 0
#define INNATE_STRENGTH 1
#define INNATE_DARKNESS 2
#define INNATE_LEVITATE 3
#define INNATE_GRACE 4
#define INNATE_FORTITUDE 5
#define INNATE_INSIGHT 6
#define INNATE_BRILL 7
#define INNATE_SPLENDOR 8
#define INNATE_HARNESS 9

/* Coin indices for coin arrays */
#define PLAT 0
#define GOLD 1
#define SILVER 2
#define COPPER 3

#define NUM_COIN_TYPES 4

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

/* Rent codes */
#define RENT_UNDEF 0
#define RENT_CRASH 1
#define RENT_RENTED 2
#define RENT_CRYO 3
#define RENT_FORCED 4
#define RENT_TIMEDOUT 5

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
#define LVL_REBOOT_MASTER LVL_HEAD_C
/* You can stop a reboot for a while with "autoboot postpone" */
#define LVL_REBOOT_POSTPONE LVL_GOD
/* You can get the time of the next automatic reboot with "autoboot" or "world" */
#define LVL_REBOOT_VIEW LVL_GOD
#else
#define LVL_REBOOT_MASTER LVL_HEAD_B
#define LVL_REBOOT_POSTPONE LVL_IMMORT
#define LVL_REBOOT_VIEW 1
#endif

#define LVL_GOSSIP 1

/* Reasons why restrict > 0 */
#define RESTRICT_NONE 0     /* No restriction, or don't know */
#define RESTRICT_ARGUMENT 1 /* Mud started with -r option */
#define RESTRICT_MANUAL 2   /* Set by a god with the wizlock command */
#define RESTRICT_AUTOBOOT 3 /* Set automatically due to imminent automatic reboot */

/* damage() return codes */
#define VICTIM_DEAD (1 << 30)

/* PC spell circles */
/* change as needed */
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

/* MAX_CHAR SPELLS is the maximum number of spells a max_level char can */
/* have   Use this value with spell_save fcn's  DO NOT MODIFY independent */
/* of the circle/level assignment table in spell_mem.c */
#define MAX_CHAR_SPELLS 131

/*trophy defines*/
#define MAX_TROPHY                                                                                                     \
    60                /*Max amount you can increase trphy to that wont                                                 \
ruin pfile*/
#define USE_TROPHY 20 /*the amount of trophy game using*/

#define MAX_SKILL_TIMERS 100

#define NUM_OF_DIRS 8 /* number of directions in a room (nsewud) */

#define OPT_USEC 100000 /* 10 passes per second */
#define PASSES_PER_SEC (1000000 / OPT_USEC)
#define RL_SEC *PASSES_PER_SEC

#define PULSE_ZONE (10 RL_SEC)
#define PULSE_MOBILE (10 RL_SEC)
#define PULSE_VIOLENCE (4 RL_SEC) /* changed to 4 seconds from 2 7/14/99 */

#define SMALL_BUFSIZE 1024
#define LARGE_BUFSIZE (48 * 1024)
#define GARBAGE_SPACE 32

#define MOB_DROPPED_WEAP_DELAY 2 /* rounds til mob can recover a dropped weapon */
#define MOB_FUMBLING_DELAY 2     /* rounds for mob to recover from fumble */
#define MOB_HEADBUTT_DELAY 3     /* rounds between mob headbutts */
#define PC_BASH_DELAY 10         /* rounds between bashes */
#define PC_BERSERK_DELAY 5       /* rounds between berserks */
#define PC_DROPPED_WEAP_DELAY 2  /* rounds til pc can recover a dropped weapon */
#define PC_FUMBLING_DELAY 2      /* rounds for pc to recover from fumble */
#define PC_HEADBUTT_DELAY 3      /* rounds between pc headbutts */
#define PC_INSTANTKILL_DELAY 10  /* rounds between instantkills */
#define MAX_STRING_LENGTH 20000
#define MAX_STRING_LENGTH_BIG 24000
#define MAX_INPUT_LENGTH 256     /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH 512 /* Max size of *raw* input */
#define MAX_MESSAGES 100
#define MAX_NAME_LENGTH 20       /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH 10        /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH 80      /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_WIZTITLE_LENGTH 30   /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH 30           /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH 240        /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE 3             /* Used in char_file_u *DO*NOT*CHANGE* */
#define TOP_SKILL 650            /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT 32            /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_ALIAS_LENGTH 8       /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_REPLACE_LENGTH 48    /* Used in char_file_u *DO*NOT*CHANGE* */
#define NUM_ALIASES 20           /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT 6         /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_SPELLBOOK_PAGES 100  /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define SPELLBOOK_ENTRY_LENGTH 9 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define SPELLBOOK_SIZE (SPELLBOOK_ENTRY_LENGTH * MAX_SPELLBOOK_PAGES) + 1
#define PAGE_SCRIBE_TIME 1  /* this is the time per page to scribe */
#define MAX_DAMAGE 1000     /* Maximum per hit allowed by damage() */
#define EVENT_FLAG_FIELDS 4 /* Number of fields for event flags */
#define NUM_P_TITLES 9      /* Used in player_special_data *DO*NOT*CHANGE* */

/**********************************************************************
 * Structures                                                          *
 **********************************************************************/

typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;
typedef char bool;
typedef struct witness_data wtns_rec;

#ifndef CIRCLE_WINDOWS
typedef char byte;
#endif

typedef int room_num;
typedef int obj_num;

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

struct char_coins_data {
    int plat;         /*carried*/
    int gold;         /*carried*/
    int silver;       /*carried*/
    int copper;       /*carriedt*/
    long bank_plat;   /*coins in bank*/
    long bank_gold;   /*coins in bank*/
    long bank_silver; /*silver in bank*/
    long bank_copper; /*copper in bank*/
};
typedef struct char_coins_data char_coins;

/* object-related structures ******************************************/

/* object flags; used in obj_data */
struct obj_flag_data {
    int value[4];                 /* Values of the item (see list)    */
    byte type_flag;               /* Type of item                            */
    unsigned long int wear_flags; /* Where you can wear it            */
    int extra_flags;              /* If it hums, glows, etc.            */
    int weight;                   /* Weigt what else                  */
    int cost;                     /* Value when sold (gp.)            */
    int cost_per_day;             /* Cost to keep pr. real day        */
    int level_obj;                /* Level of the object -> Zantir 3/23/01 */
    int timer;                    /* Timer for object                 */
    int spell_flags;              /* Object Spell affections - buru 25/5/98 */
    int spell_flags2;
    int spell_flags3;
    long hiddenness; /* How difficult it is to see obj   */
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

struct spell_book_list {
    int spell;
    int spell_length;
    struct spell_book_list *next;
};

/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
    byte location;   /* Which ability to change (APPLY_XXX) */
    sh_int modifier; /* How much it changes by              */
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
    obj_num item_number;                               /* Where in data-base                        */
    room_num in_room;                                  /* In what room -1 when conta/carr        */
    int mob_from;                                      /* where the mob is from*/
    struct obj_flag_data obj_flags;                    /* Object information               */
    struct obj_affected_type affected[MAX_OBJ_AFFECT]; /* affects */
    char *name;                                        /* Title of object :get etc.        */
    char *description;                                 /* When in room                     */
    char *short_description;                           /* when worn/carry/in cont.         */
    char *action_description;                          /* What to write when used          */
    struct extra_descr_data *ex_description;           /* extra descriptions     */
    struct char_data *carried_by;                      /* Carried by :NULL in room/conta   */
    struct char_data *worn_by;                         /* Worn by?                              */
    sh_int worn_on;                                    /* Worn where?                      */
    struct obj_data *in_obj;                           /* In what object NULL when none    */
    struct obj_data *contains;                         /* Contains objects                 */
    long id;                                           /* used by DG triggers              */
    struct trig_proto_list *proto_script;              /* list of default triggers  */
    struct script_data *script;                        /* script info for the object       */
    struct char_data *last_to_hold;                    /* If MOB forcibly loses item      */
    struct obj_data *next_content;                     /* For 'contains' lists             */
    struct obj_data *next;                             /* For the object list              */
    struct spell_book_list *spell_book;                /* list of all spells in book if obj is spellbook */
    int spell_book_length;                             /* number of pages in spellbook     */
    sh_int spell_component;
    int object_limitation;
    int spare1;
    long spare2;
    sh_int spare3;
    struct char_data *casters; /* Characters who are casting spells at this */
    struct event *events;      /* List of events related to this object */
    int event_flags[EVENT_FLAG_FIELDS];
    /* Bitfield of events active on this object */
};
/* ======================================================================= */

/* ====================== File Element for Objects ======================= */
/*                BEWARE: Changing it will ruin rent files                   */
struct obj_file_elem {
    obj_num item_number;
    sh_int locate; /* that's the (1+)wear-location (when equipped) or
                       (20+)index in obj file (if it's in a container)  */
    int value[4];
    int extra_flags;
    int weight;
    int timer;
    int spell_flags; /* Object Spell affections - buru 25/5/98 */
    int spell_flags2;
    int spell_flags3;
    long hiddenness;
    char spells_in_book[SPELLBOOK_SIZE]; /* spell list and how many pages each spell takes up */
    int spell_book_length;               /* number of pages in spellbook */
    struct obj_affected_type affected[MAX_OBJ_AFFECT];
};

struct current_info {
    int room_vnum;
    int direction;
    int percent;
};

/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
    int time;
    int rentcode;
    int net_cost_per_diem;
    int nitems;
    struct char_coins_data coins;
    int spare6;
    int spare7;
    int spare8;
    long spare9;
    long spare10;
    sh_int spare11;
};

/* ======================================================================= */

/* room-related structures ************************************************/

struct room_direction_data {
    char *general_description; /* When look DIR.                        */

    char *keyword; /* for open/close                        */

    int exit_info;    /* Exit info                                */
    obj_num key;      /* Key's number (-1 for no key)                */
    room_num to_room; /* Where direction leads (NOWHERE)        */
};

struct raff_node {
    room_num room;  /* location in the world[] array of the room */
    int timer;      /* how many ticks this affection lasts */
    long affection; /* which affection does this room have */
    int spell;      /* the spell number */

    struct raff_node *next; /* link to the next node */
};

/* ================== Memory Structure for room ======================= */
struct room_data {
    room_num number;                                     /* Rooms number        (vnum)                      */
    int zone;                                            /* Room zone (for resetting)          */
    int sector_type;                                     /* sector type (move/hide)            */
    char *name;                                          /* Rooms name 'You are ...'           */
    char *description;                                   /* Shown when entered                 */
    struct extra_descr_data *ex_description;             /* for examine/look       */
    struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
    int room_flags;                                      /* DEATH,DARK ... etc                 */

    byte light; /* Number of lightsources in room     */
    SPECIAL(*func);

    struct trig_proto_list *proto_script; /* list of default triggers  */
    struct script_data *script;           /* script info for the object         */

    struct obj_data *contents; /* List of items in room              */
    struct char_data *people;  /* List of NPC / PC in room          */
    long room_affections;      /* bitvector for spells/skills */
};
/* ====================================================================== */

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

/* This structure is for memorized spells */
struct mem_list {
    int spell;
    int mem_time;
    int can_cast;
    struct mem_list *next;
};
/* This is the scructure for spells being scribed */
struct scribing {
    int spell;
    int scribe_time;
    int pages;
    int pages_left;
    struct scribing *next;
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data {
    char passwd[MAX_PWD_LENGTH + 1]; /* character's password      */
    char *name;                      /* PC / NPC s name (kill ...  )         */
    char *short_descr;               /* for NPC 'actions'                    */
    char *long_descr;                /* for NPC 'look'                */
    char *description;               /* Extra descriptions                   */
    char *title;                     /* PC / NPC's title  */
    char *prompt;                    /* Player prompt*/
    byte sex;                        /* PC / NPC's sex                       */
    byte class;                      /* PC / NPC's class                       */
    byte race;                       /* PC / NPC's race              */
    byte race_align;                 /*PC / NPC's race_align*/
    byte level;                      /* PC / NPC's level                     */
    int hometown;                    /* PC s Hometown (zone)                 */
    struct time_data time;           /* PC's AGE in days                 */

    /* Note that height and weight are ints here, while in the player file
     * they are ubyte.  Thus, player races cannot represent numbers over
     * 255.  This is a big problem, as ogres obviously weigh far more than 300
     * pounds!  Sigh.  Anyway, the solution is to cap PLAYERS at around 240
     * for each value.  Mobiles, not being saved, can get much bigger. */
    int weight;
    int height;
    byte size;
};

/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
    sbyte str;
    sbyte spare;
    sbyte intel;
    sbyte wis;
    sbyte dex;
    sbyte con;
    sbyte cha;
};

/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
    sh_int mana;
    sh_int max_mana; /* Max move for PC/NPC                           */
    sh_int hit;
    sh_int max_hit; /* Max hit for PC/NPC                      */
    sh_int move;
    sh_int max_move; /* Max move for PC/NPC                     */
    sh_int armor;    /* Internal -100..100, external -10..10 AC */
    struct char_coins_data coins;
    long exp; /* The experience of the player            */
              /* The experience of the player            */

    sbyte hitroll; /* Any bonus or penalty to the hit roll    */
    sbyte damroll; /* Any bonus or penalty to the damage roll */
};

struct trophy_data {
    int virtual;
    float value;
};

struct alias {
    char alias[MAX_ALIAS_LENGTH];
    char replacement[MAX_REPLACE_LENGTH];
    int type;
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
    int alignment; /* +-1000 for alignments                */
    long idnum;    /* player's idnum; -1 for mobiles        */
    long act;      /* act flag for NPC's; player flag for PC's */

    long affected_by; /* Bitvector for spells/skills affected by */
    long affected_by2;
    long affected_by3;
    sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)                */
};

/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
    struct char_data *fighting; /* Opponent                                */
    struct char_data *hunting;  /* Char hunted by this char                */
    struct char_data *riding;
    struct char_data *ridden_by;
    struct char_data *consented;
    byte position; /* Standing, fighting, sleeping, etc.        */
    byte position1;

    /* this was ubyte before, which only gives up to 255 pulse delay */
    unsigned long action_delays[MAX_ACTION_DELAYS];

    wtns_rec *witnessed; /* linked list of witness records */
    sh_int jail_time;
    struct char_data *arrest_by;
    struct char_data *arrest_link;
    struct char_data *witnessing;
    struct char_data *witness_vict;
    int witness_cmd;

    int carry_weight; /* Carried weight                        */
    byte carry_items; /* Number of items carried                */
    int timer;        /* Timer for update                        */
    sbyte hitgain;
    sbyte managain;
    int rage; /* For berserking */

    struct char_special_data_saved saved; /* constants saved in plrfile        */

    /*
     * This is where perception and hiddenness are accessed during the
     * game.  For players, they are copied to the player_specials.saved
     * struct when the player is saved.
     */
    long perception;
    long hiddenness;
};

/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
    sh_int skills[TOP_SKILL + 1]; /* array of skills plus skill 0                 */
    byte PADDING0;                /* used to be spells_to_learn                */
    bool talks[MAX_TONGUE];       /* PC s Tongues 0 for NPC                */
    int wimp_level;               /* Below this # of hit points, flee!        */
    byte freeze_level;            /* Level of god who froze char, if any        */
    sh_int invis_level;           /* level of invisibility                */
    room_num load_room;           /* Which room to place char in                */
    long pref;                    /* preference flags for PC's.                */
    ubyte bad_pws;                /* number of bad password attemps        */
    sbyte conditions[3];          /* Drunk, full, thirsty                        */
    sbyte innatetime[4];          /*Innate timers banyal*/
    sh_int speaking;
    struct trophy_data trophy[MAX_TROPHY];
    struct alias aliases[NUM_ALIASES];
    int top;
    float frag;
    /* PC spell memory save */
    /* spellnum and can_cast flag*/
    int memmed_spells[MAX_CHAR_SPELLS][2];

    /* number of spells in memory*/
    int spells_in_mem;

    /* spares below for future expansion.  You can change the names from
       'sparen' to something meaningful, but don't change the order.  */

    ubyte spheres[6];
    ubyte page_length;
    ubyte spare1;
    ubyte clan;
    byte clan_rank;
    ubyte spare2;
    ubyte spare3;
    ubyte spare4;
    ubyte spare5;
    int spells_to_learn; /* How many can you learn yet this level*/
    int olc_zone;
    int olc2_zone;
    int aggressive;
    int olc3_zone;
    int rage;
    int olc4_zone;
    int olc5_zone;
    int diety;
    int chant; /* Chant counter */
    int spare23;
    int lastlevel;
    int nathps;
    long perception;
    long hiddenness;
    long spare44;
    long spare19;
    long spare20;
    long spare25;
    long spare21;
    sh_int spare31;
    sh_int spare32;
    char poofin[EXDSCR_LENGTH];
    char poofout[EXDSCR_LENGTH];
    char titles[NUM_P_TITLES][MAX_TITLE_LENGTH];
    char spare33[EXDSCR_LENGTH];
    char long_descr[EXDSCR_LENGTH];
    char wiz_title[MAX_WIZTITLE_LENGTH];
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
    struct player_special_data_saved saved;
    /* struct alias *aliases;*/
    long last_tell;      /* idnum of last tell from                */
    void *last_olc_targ; /* olc control                                */
    int last_olc_mode;   /* olc control                                */
    byte roll[6];        /* (NOT USED) */
    struct char_data *ignored;

    int last_skill_use[MAX_SKILL_TIMERS];

    /* These keep track of how much a player has been speaking (gossipping,
     * telling, whatever else is deemed appropriate) and is used to decide
     * whether to automatically quiet the player. */
    double speech_rate;
    long last_speech_time; /* Taken from global_pulse */
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    long nr; /* Mob's rnum */
    int ex_plat;
    int ex_gold;
    sbyte ex_face;
    int ex_no_dice;
    int zone;
    sbyte ex_damroll;
    sbyte ex_hitroll;
    sbyte ex_hpnumdice;
    sbyte ex_hpsizedice;
    int ex_main_hp;
    sh_int skills[TOP_SKILL + 1];
    long ex_exp;
    byte class;
    sh_int ex_hit;
    sh_int ex_max_hit;
    byte last_direction; /* The last direction the monster went     */
    int attack_type;     /* The Attack Type Bitvector for NPC's     */
    byte default_pos;    /* Default position for NPC                */
    memory_rec *memory;  /* List of attackers to remember               */
    sbyte damnodice;     /* The number of damage dice's               */
    sbyte damsizedice;   /* The size of the damage dice's           */
    sbyte ex_damnodice;
    sbyte ex_damsizedice;
    int wait_state; /* Wait state for bashed mobs*/
    int mem_state[12];
    int mem_memed[12];
    sh_int ex_armor;
    long mob2_flags;
};

/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
    sh_int type;     /* The type of spell that caused this      */
    sh_int duration; /* For how long its effects will last      */
    sh_int modifier; /* This is added to apropriate ability     */
    byte location;   /* Tells which ability to change(APPLY_XXX)*/
    long bitvector;  /* Tells which bits to set (AFF_XXX)       */
    long bitvector2;
    long bitvector3;

    struct affected_type *next;
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
    struct char_ability_data viewed_abils;   /* natural_abils + affectations VIEWING ONLY */
    struct char_ability_data affected_abils; /* (viewed_abils)*(racial percentage) */
    struct char_point_data points;           /* Points */
    struct affected_type *affected;          /* affected by what spells */
    struct obj_data *equipment[NUM_WEARS];   /* Equipment array */
    struct obj_data *carrying;               /* Head of list */
    struct know_spell *see_spell;            /* list of chars that guessed caster's spell */
                                             /* Other characters */
    struct char_data *forward;               /* for shapechange/switch */
    struct char_data *next_in_room;          /* For room->people - list */
    struct char_data *next;                  /* For either monster or ppl-list */
    struct char_data *next_fighting;         /* For fighting list */
    struct char_data *guarded_by;            /* Character guarding this char */
    struct char_data *guarding;              /* Char this char is guarding */
    struct char_data *cornered_by;           /* Char preventing this char from fleeing */
    struct char_data *cornering;             /* Char this char is preventing from fleeing*/
    struct follow_type *followers;           /* List of chars followers */
    struct char_data *master;                /* Who is char following? */
    struct group_type *groupees;             /* list of chars grouped */
    struct char_data *groupmaster;           /* group master */
    struct mgroup_type *mgroupees;           /* major groups list */
    struct char_data *mgroupmaster;          /* groups master */
    struct char_data *next_caster;           /* A list of casters I'm in */
    struct char_data *casters;               /* Chars who are casting spells at me */
                                             /* Player stuff */
    int pfilepos;                            /* playerfile pos */
    int clan_snoop;                          /* clan chat snoop */
    struct quest_list *quests;
    /* Spell mem/scribe stuff */
    struct mem_list *spell_list;                     /* spells in mem queue */
    int num_spells;                                  /* number of spells in mem list */
    int num_memmed;                                  /* hw many are currently memmed */
    int mem_status;                                  /* is the PC memming now? */
    int spells_memmed_circle[NUM_SPELL_CIRCLES + 1]; /* number of spells memmed from each circle */
    struct scribing *scribe_list;                    /* spells queued for scribing */
                                                     /* Mobile stuff */
    struct trig_proto_list *proto_script;            /* list of default triggers */
    struct script_data *script;                      /* script info for the object */
                                                     /* Substructs of varying application */
    struct char_player_data player;                  /* Normal data */
    struct char_special_data char_specials;          /* PC/NPC specials */
    struct player_special_data *player_specials;     /* PC specials */
    struct mob_special_data mob_specials;            /* NPC specials  */
    struct descriptor_data *desc;                    /* NULL for mobiles */
                                                     /* Events */
    struct event *jail_event;                        /* get out of jail free... */
    struct casting casting;                          /* note this is NOT a pointer */
    struct event *events;                            /* List of events related to this character */
    int event_flags[EVENT_FLAG_FIELDS];              /* Bitfield of events active on this character */
};

/* in order to make certain pieces of code eaiser to port, and more readable
 * i type deff'ed to this...

 typedef struct char_data P_char;
*/
/*It was also crashing mud, the fix is teted and proven it somehow ws corrutping ch
Banyal*/

/* ====================================================================== */

/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile                  */
struct char_file_u {
    /* char_player_data */
    char name[MAX_NAME_LENGTH + 1];
    char description[EXDSCR_LENGTH];
    char title[MAX_TITLE_LENGTH + 1];
    char prompt[MAX_INPUT_LENGTH + 1];
    byte sex;
    byte class;
    byte race;
    byte race_align;
    byte level;
    int hometown;
    time_t birth; /* Time of birth of character     */
    int played;   /* Number of secs played in total */
    ubyte weight;
    ubyte height;
    byte size;

    char pwd[MAX_PWD_LENGTH + 1]; /* character's password */

    struct char_special_data_saved char_specials_saved;
    struct player_special_data_saved player_specials_saved;
    struct char_ability_data abilities;
    struct char_point_data points;
    struct affected_type affected[MAX_AFFECT];

    time_t last_logon;          /* Time (in secs) of last logon */
    char host[HOST_LENGTH + 1]; /* host of last logon */
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

struct descriptor_data {
    socket_t descriptor;        /* file descriptor for socket                */
    char host[HOST_LENGTH + 1]; /* hostname                                */
    byte bad_pws;               /* number of bad pw attemps this login        */
    byte idle_tics;             /* tics idle at password prompt                */
    int connected;              /* mode of 'connectedness'                */
    int wait;                   /* wait for how many loops                */
    int desc_num;               /* unique num assigned to desc                */
    time_t login_time;          /* when the person connected                */
    char *showstr_head;         /* for keeping track of an internal str        */
    char **showstr_vector;      /* for paging through texts                */
    int showstr_count;          /* number of pages to page through        */
    int showstr_page;           /* which page are we currently showing?        */
    char **str;                 /* for the modify-str system                */
    char *backstr;              /* added for handling abort buffers     */
    size_t max_str;             /*                -                        */
    long mail_to;               /* name for mail system                        */
    int mail_vnum;
    int prompt_mode;                   /* control of prompt-printing                */
    char inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input                */
    char last_input[MAX_INPUT_LENGTH]; /* the last input                        */
    char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer                */
    char *output;                      /* ptr to the current output buffer        */
    int bufptr;                        /* ptr to end of current output                */
    int bufspace;                      /* space left in the output buffer        */
    struct txt_block *large_outbuf;    /* ptr to large buffer, if we need it */
    struct txt_q input;                /* q of unprocessed input                */
    struct char_data *character;       /* linked to char                        */
    struct char_data *original;        /* original char if switched                */
    struct descriptor_data *snooping;  /* Who is this char snooping        */
    struct descriptor_data *snoop_by;  /* And who is snooping this char        */
    struct descriptor_data *next;      /* link to next descriptor                */
    struct olc_data *olc;
    char *storage;
    int clan_id;
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

struct mgroup_type {
    struct char_data *mgroupee;
    struct mgroup_type *next;
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
};

struct con_app_type {
    sh_int hitp;
    sh_int shock;
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
    int object_limit;        /*vairies during game NOT saved to file*/
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
struct witness_data {
    time_t time;    /* When did it happen? */
    char *attacker; /* who did it? */
    char *victim;   /* who did they do it to? */
    ubyte crime;    /* what did they do? */
    int room;       /* Where did they do it?  (VIRTUAL!) */
    wtns_rec *next; /* next record (or NULL if none) */
};

struct player_frags_data {
    int playerid;
    char name[MAX_NAME_LENGTH + 20];
    float frag;
};

#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

#endif

/***************************************************************************
 * $Log: structs.h,v $
 * Revision 1.1  2008/03/05 03:49:56  myc
 * Initial revision
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

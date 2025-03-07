/***************************************************************************
 *   File: legacy_structs.h                               Part of FieryMUD *
 *  Usage: header file for legacy central structures and contstants        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

#define LEGACY_USE_TROPHY 20
#define LEGACY_MAX_TROPHY 60
#define LEGACY_MAX_CHAR_SPELLS 131
#define LEGACY_MAX_INPUT_LENGTH 256
#define LEGACY_MAX_NAME_LENGTH 20
#define LEGACY_MAX_PWD_LENGTH 10
#define LEGACY_MAX_TITLE_LENGTH 80
#define LEGACY_MAX_WIZTITLE_LENGTH 30
#define LEGACY_HOST_LENGTH 30
#define LEGACY_EXDSCR_LENGTH 240
#define LEGACY_MAX_TONGUE 3
#define LEGACY_TOP_SKILL 650
#define LEGACY_MAX_AFFECT 32
#define LEGACY_MAX_ALIAS_LENGTH 8
#define LEGACY_MAX_REPLACE_LENGTH 48
#define LEGACY_NUM_ALIASES 20
#define LEGACY_MAX_OBJ_AFFECT 6
#define LEGACY_MAX_SPELLBOOK_PAGES 100
#define LEGACY_SPELLBOOK_ENTRY_LENGTH 9
#define LEGACY_SPELLBOOK_SIZE (LEGACY_SPELLBOOK_ENTRY_LENGTH * LEGACY_MAX_SPELLBOOK_PAGES) + 1
#define LEGACY_NUM_P_TITLES 9

/**********************************************************************
 * Structures                                                          *
 **********************************************************************/

struct legacy_char_coins {
    int plat;         /*carried*/
    int gold;         /*carried*/
    int silver;       /*carried*/
    int copper;       /*carriedt*/
    long bank_plat;   /*coins in bank*/
    long bank_gold;   /*coins in bank*/
    long bank_silver; /*silver in bank*/
    long bank_copper; /*copper in bank*/
};

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
    int effect_flags; /* Object Spell affections - buru 25/5/98 */
    int effect_flags2;
    int effect_flags3;
    long hiddenness;
    char spells_in_book[LEGACY_SPELLBOOK_SIZE]; /* spell list and how many pages
                                                   each spell takes up */
    int spell_book_length;                      /* number of pages in spellbook */
    struct {
        byte location;   /* Which ability to change (APPLY_XXX) */
        sh_int modifier; /* How much it changes by              */
    } affected[LEGACY_MAX_OBJ_AFFECT];
};

/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
    int time;
    int rentcode;
    int net_cost_per_diem;
    int nitems;
    legacy_char_coins coins;
    int spare6;
    int spare7;
    int spare8;
    long spare9;
    long spare10;
    sh_int spare11;
};

/* ======================================================================= */

/* Char's abilities. */
struct legacy_char_ability_data {
    sbyte str;
    sbyte spare;
    sbyte intel;
    sbyte wis;
    sbyte dex;
    sbyte con;
    sbyte cha;
};

/* Char's points. */
struct legacy_char_point_data {
    sh_int mana;
    sh_int max_mana; /* Max move for PC/NPC                           */
    sh_int hit;
    sh_int max_hit; /* Max hit for PC/NPC                      */
    sh_int move;
    sh_int max_move; /* Max move for PC/NPC                     */
    sh_int armor;    /* Internal -100..100, external -10..10 AC */
    legacy_char_coins coins;
    long exp; /* The experience of the player            */
              /* The experience of the player            */

    sbyte hitroll; /* Any bonus or penalty to the hit roll    */
    sbyte damroll; /* Any bonus or penalty to the damage roll */
};

struct legacy_trophy_data {
    int vnum;
    float value;
};

struct legacy_alias {
    char alias[LEGACY_MAX_ALIAS_LENGTH];
    char replacement[LEGACY_MAX_REPLACE_LENGTH];
    int type;
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 */
struct legacy_char_special_data_saved {
    int alignment; /* +-1000 for alignments                */
    long idnum;    /* player's idnum; -1 for mobiles        */
    long act;      /* act flag for NPC's; player flag for PC's */

    long affected_by; /* Bitvector for spells/skills affected by */
    long affected_by2;
    long affected_by3;
    sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)                */
};

struct legacy_player_special_data_saved {
    sh_int skills[LEGACY_TOP_SKILL + 1]; /* array of skills plus skill 0 */
    byte PADDING0;                       /* used to be spells_to_learn                */
    int wimp_level;                      /* Below this # of hit points, flee!        */
    byte freeze_level;                   /* Level of god who froze char, if any        */
    sh_int invis_level;                  /* level of invisibility                */
    room_num load_room;                  /* Which room to place char in                */
    long pref;                           /* preference flags for PC's.                */
    ubyte bad_pws;                       /* number of bad password attemps        */
    sbyte conditions[3];                 /* Drunk, full, thirsty                        */
    sbyte innatetime[4];                 /*Innate timers banyal*/
    sh_int speaking;
    legacy_trophy_data trophy[LEGACY_MAX_TROPHY];
    legacy_alias aliases[LEGACY_NUM_ALIASES];
    int top;
    float frag;
    /* PC spell memory save */
    /* spellnum and can_cast flag*/
    int memmed_spells[LEGACY_MAX_CHAR_SPELLS][2];

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
    char poofin[LEGACY_EXDSCR_LENGTH];
    char poofout[LEGACY_EXDSCR_LENGTH];
    char titles[LEGACY_NUM_P_TITLES][LEGACY_MAX_TITLE_LENGTH];
    char spare33[LEGACY_EXDSCR_LENGTH];
    char long_descr[LEGACY_EXDSCR_LENGTH];
    char wiz_title[LEGACY_MAX_WIZTITLE_LENGTH];
};

struct legacy_affected_type {
    sh_int type;     /* The type of spell that caused this      */
    sh_int duration; /* For how long its effects will last      */
    sh_int modifier; /* This is added to apropriate ability     */
    byte location;   /* Tells which ability to change(APPLY_XXX)*/
    long bitvector;  /* Tells which bits to set (AFF_XXX)       */
    long bitvector2;
    long bitvector3;

    legacy_affected_type *next;
};

struct char_file_u {
    /* char_player_data */
    char name[LEGACY_MAX_NAME_LENGTH + 1];
    char description[LEGACY_EXDSCR_LENGTH];
    char title[LEGACY_MAX_TITLE_LENGTH + 1];
    char prompt[LEGACY_MAX_INPUT_LENGTH + 1];
    byte sex;
    byte class_num;
    byte race;
    byte race_align;
    byte level;
    int hometown;
    time_t birth; /* Time of birth of character     */
    int played;   /* Number of secs played in total */
    ubyte weight;
    ubyte height;
    byte size;

    char pwd[LEGACY_MAX_PWD_LENGTH + 1]; /* character's password */

    legacy_char_special_data_saved char_specials_saved;
    legacy_player_special_data_saved player_specials_saved;
    legacy_char_ability_data abilities;
    legacy_char_point_data points;
    legacy_affected_type affected[LEGACY_MAX_AFFECT];

    time_t last_logon;                 /* Time (in secs) of last logon */
    char host[LEGACY_HOST_LENGTH + 1]; /* host of last logon */
};
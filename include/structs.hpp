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

#pragma once

#include "defines.hpp"
#include "sysdep.hpp"

#include <list>
#include <string>
#include <vector>

typedef int room_num;
typedef int obj_num;
typedef int zone_vnum;

#define DAMAGE_WILL_KILL(ch, dmg) (GET_HIT(ch) - dmg <= HIT_DEAD)

// TODO: Refactor this file so we don't need all these forward declarations.
struct CharData;
struct ObjData;

/**********************************************************************
 * Structures                                                          *
 **********************************************************************/

typedef char byte;
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;

/** Bitvector type for 32 bit unsigned long bitvectors. 'unsigned long long'
 * will give you at least 64 bits if you have GCC. You'll have to search
 * throughout the code for "bitvector_t" and change them yourself if you'd
 * like this extra flexibility. */
typedef unsigned long int flagvector;
#define FLAGBLOCK_SIZE (flagvector)32
//((flagvector)8 * sizeof(flagvector)) /* 8 bits = 1 byte */
#define FLAGVECTOR_SIZE(flags) (((flags)-1) / FLAGBLOCK_SIZE + 1)

/* Extra description: used in objects, mobiles, and rooms */
struct ExtraDescriptionData {
    char *keyword;              /* Keyword in look/examine          */
    char *description;          /* What to see                      */
    ExtraDescriptionData *next; /* Next in list                     */
};

/* This is the structure for keeping track of cast spells and how long ago. */
struct SpellCast {
    int spellnum; // The spell that was cast.
    int ticks;    // Time required studying/praying/etc to clear this slot.  Determined by spell slot.
};

struct Casting {
    int spell;
    int casting_time;
    CharData *tch; /* set up the targets */
    ObjData *obj;
    char *misc;
    int target_status;
};

struct SpellPair {
    int spell;
    int remover;
    int flag;
};

struct SpellDamage {
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
struct MemoryRec {
    long id;
    MemoryRec *next;
};

// typedef struct memory_rec_struct memory_rec;

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct TimeInfoData {
    int hours, day, month;
    sh_int year;
};

/* These data contain information about a players time data */
struct TimeData {
    time_t birth; /* This represents the characters age                */
    time_t logon; /* Time of the last logon (used to calculate played) */
    int played;   /* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct CharPlayerData {
    char passwd[MAX_PWD_LENGTH + 1]; /* character's password      */
    char *namelist;                  /* PC / NPC s name (kill ...  )         */
    char *short_descr;               /* for NPC 'actions'                    */
    char *long_descr;                /* for NPC 'look'                       */
    char *description;               /* Extra descriptions                   */
    char *title;                     /* PC / NPC's title                     */
    char *prompt;                    /* Player prompt                        */
    ush_int sex;                     /* PC / NPC's sex                       */
    ush_int class_num;               /* PC / NPC's class                     */
    ush_int race;                    /* PC / NPC's race                      */
    ush_int race_align;              /* PC / NPC's race_align                */
    ush_int level;                   /* PC / NPC's level                     */
    int lifeforce;                   /* What empowers it - see LIFE_* in chars.h */
    int base_composition;
    int composition;   /* What its body is made of - see COMP_* in chars.h */
    room_num homeroom; /* PC s Homeroom                        */

    TimeData time; /* PC's AGE in days                 */

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
struct CharAbilityData {
    int str;
    int intel;
    int wis;
    int dex;
    int con;
    int cha;
};

/* Char's points. */
struct CharPointData {
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

struct AliasData {
    char *alias;
    char *replacement;
    int type;
    AliasData *next;
};

/* Special playing constants shared by PCs and NPCs */
struct CharSpecialData {
    CharData *hunting; /* Char hunted by this char                */
    CharData *riding;
    CharData *ridden_by;
    CharData *consented;
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

struct OLCZoneList {
    zone_vnum zone;
    OLCZoneList *next;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs.  This structure
 * can be changed freely.
 */
struct ClanMembership;
struct ClanSnoop;
struct GrantType;
struct RetainedComms;
struct TrophyNode;
struct PlayerSpecialData {
    long last_tell;      /* idnum of last tell from                */
    void *last_olc_targ; /* olc control                            */
    int last_olc_mode;   /* olc control                            */
    byte roll[6];        /* for rolling stats in player creation   */
    CharData *ignored;

    /* List of the last X comms as defined in retained_comms.h */
    RetainedComms *comms;
    int wimp_level;       /* Below this # of hit points, flee!      */
    int aggressive;       /* Above this # of hit points, autoattack */
    byte freeze_level;    /* Level of god who froze char, if any    */
    byte autoinvis_level; /* Level of invisibility to take when entering game */
    int invis_level;      /* level of invisibility                  */
    room_num load_room;   /* Which room to place char in            */
    room_num save_room;   /* Where the player was when saved        */
    /* preference flags for PC's.             */
    flagvector pref[FLAGVECTOR_SIZE(NUM_PRF_FLAGS)];
    /* privilege flags for PC's */
    flagvector privileges[FLAGVECTOR_SIZE(NUM_PRV_FLAGS)];
    ubyte bad_pws;       /* number of bad password attempts        */
    sbyte conditions[3]; /* Drunk, full, thirsty                   */
    TrophyNode *trophy;
    AliasData *aliases;

    flagvector *grant_cache;  /* cache of granted commands              */
    flagvector *revoke_cache; /* cache of revoked commands              */
    GrantType *grants;        /* Commands granted to this player        */
    GrantType *revokes;       /* Commands revoked from this player      */
                              /* Groups of commands granted to this player */
    GrantType *grant_groups;
    /* Groups of commands revoked from this player */
    GrantType *revoke_groups;

    ubyte page_length;
    ClanMembership *clan;
    ClanSnoop *clan_snoop;
    OLCZoneList *olc_zones;
    int lastlevel;
    int base_hit;
    int log_view; /* Level of syslog displayed              */
    char *poofin;
    char *poofout;
    char **perm_titles;
    char *long_descr;
    char *wiz_title;
    char *host;
    std::string client{"Unknown"};
};

/* Specials used by NPCs, not PCs */
struct MobSpecialData {
    long nr; /* Mob's rnum                              */
    int ex_copper;
    int ex_silver;
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
    byte class_num;
    sh_int ex_hit;
    sh_int ex_max_hit;
    byte last_direction; /* The last direction the monster went     */
    int attack_type;     /* The Attack Type Bitvector for NPC's     */
    byte default_pos;    /* Default position for NPC                */
    MemoryRec *memory;   /* List of attackers to remember           */
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

    effect *next;
};

struct GroupType {
    CharData *groupee;
    GroupType *next;
};

/* Structure used for chars following other chars */
struct FollowType {
    CharData *follower;
    bool can_see_master;
    FollowType *next;
};

struct KnowSpell {
    CharData *sch;
    KnowSpell *next;
};

/* ================== Structure for player/non-player ===================== */
// TODO: Break CharData out so we don't require these.
struct Casting;
struct DescriptorData;
struct Event;
struct QuestList;
struct Scribing;
struct ScriptData;
struct TriggerPrototypeList;

struct CharData {
    /* Character stuff */
    long id;                        /* Global unique ID used by DG triggers */
    room_num in_room;               /* Location (real room number) */
    room_num was_in_room;           /* location for linkdead people */
    CharAbilityData natural_abils;  /* natural rolls */
    CharAbilityData actual_abils;   /* natural_abils + effects */
    CharAbilityData affected_abils; /* viewed_abils * racial percentage */
    CharPointData points;           /* Points */
    effect *effects;                /* effected by what spells */
    ObjData *equipment[NUM_WEARS];  /* Equipment array */
    ObjData *carrying;              /* Head of list */
    KnowSpell *see_spell;           /* list of chars that guessed caster's spell */
                                    /* Other characters */
    CharData *forward;              /* for shapechange/switch */
    CharData *next_in_room;         /* For room->people - list */
    CharData *next;                 /* For either monster or ppl-list */
    CharData *guarded_by;           /* Character guarding this char */
    CharData *guarding;             /* Char this char is guarding */
    CharData *cornered_by;          /* Char preventing this char from fleeing */
    CharData *cornering;            /* Char this char is preventing from fleeing*/
    FollowType *followers;          /* List of chars followers */
    CharData *master;               /* Who is char following? */
    GroupType *groupees;            /* list of chars grouped */
    CharData *group_master;         /* group master */
    CharData *next_caster;          /* A list of casters I'm in */
    CharData *casters;              /* Chars who are casting spells at me */
                                    /* Battling */
    CharData *next_fighting;        /* Part of list of all fighting characters in mud */
    CharData *target;               /* Who I'm fighting */
    CharData *attackers;            /* List of characters who are fighting me */
    CharData *next_attacker;        /* List of fighting characters I'm in */
    /* Player stuff */
    int pfilepos; /* playerfile pos */
    QuestList *quests;
    std::vector<SpellCast> spellcasts;  /* Spell mem/scribe stuff */
    Scribing *scribe_list;              /* spells queued for scribing */
                                        /* Mobile stuff */
    TriggerPrototypeList *proto_script; /* list of default triggers */
    ScriptData *script;                 /* script info for the object */
                                        /* Substructs of varying application */
    CharPlayerData player;              /* Normal data */
    CharSpecialData char_specials;      /* PC/NPC specials */
    PlayerSpecialData *player_specials; /* PC specials */
    MobSpecialData mob_specials;        /* NPC specials  */
    DescriptorData *desc;               /* NULL for mobiles */
                                        /* Events */
    Casting casting;                    /* note this is NOT a pointer */
    Event *events;                      /* List of events related to this character */
    int event_flags[EVENT_FLAG_FIELDS]; /* Bitfield of events active on this character */
};

/* ====================================================================== */

/* descriptor-related structures ******************************************/

struct txt_block {
    char *text;
    int aliased;
    txt_block *next;
};

struct txt_q {
    txt_block *head;
    txt_block *tail;
};

struct paging_line {
    char *line;
    paging_line *next;
};

struct EditorData;
struct OLCData;
struct DescriptorData {
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
    std::string output;                /* ptr to the current output buffer      */
    txt_q input;                       /* q of unprocessed input                */
    CharData *character;               /* linked to char                        */
    CharData *original;                /* original char if switched             */
    DescriptorData *snooping;          /* Who is this char snooping             */
    DescriptorData *snoop_by;          /* And who is snooping this char         */
    DescriptorData *next;              /* link to next descriptor               */
    OLCData *olc;
    char *storage;

    /* Editing a buffer */
    char **str;           /* for the modify-str system             */
    char *backstr;        /* added for handling abort buffers      */
    size_t max_str;       /*                -                      */
    long mail_to;         /* name for mail system                  */
    int max_buffer_lines; /* limitation on the number of lines     */

    EditorData *editor;

    std::list<std::string> *page_outbuf; /* buffer for paged output */
    int paging_curpage;                  /* The page which is currently showing   */

    /* The pager */
    paging_line *paging_lines; /* The text that is to be paged through  */
    paging_line *paging_tail;  /* End of the list of lines              */
    char *paging_fragment;     /* Intermediate leftover string          */
    int paging_numlines;       /* Number of lines in the list           */
    int paging_numpages;       /* Number of pages to be paged through   */
    int paging_bufsize;        /* Amount of memory currently used       */
    int paging_skipped;        /* Number of lines discarded due to overflow */
};

/* other miscellaneous structures ***************************************/

struct msg_type {
    char *attacker_msg; /* message to attacker */
    char *victim_msg;   /* message to victim   */
    char *room_msg;     /* message to room     */
};

struct message_type {
    msg_type die_msg;   /* messages when death             */
    msg_type miss_msg;  /* messages when miss              */
    msg_type hit_msg;   /* messages when hit               */
    msg_type god_msg;   /* messages when hit on god        */
    msg_type heal_msg;  /* message when healing            */
    message_type *next; /* to next messages of this kind.  */
};

struct message_list {
    int a_type;            /* Attack type                                */
    int number_of_attacks; /* How many attack messages to chose from. */
    message_type *msg;     /* List of messages.                        */
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
    sh_int bonus; /* a bonus to skills */
};

struct weather_data {
    int pressure; /* How is the pressure ( Mb ) */
    int change;   /* How fast and what way does it change. */
    int sky;      /* How is the sky. */
    int sunlight; /* And how much sun. */
};

/* element in monster and object index-tables   */
struct TrigData; // TODO: Refactor this out
struct IndexData {
    int vnum;   /* vnum of this mob/obj           */
    int number; /* number of existing units of this mob/obj        */
    // SPECIAL(*func);
    int (*func)(CharData *ch, void *me, int cmd, char *argument);
    char *farg;      /* string argument for special function     */
    TrigData *proto; /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct TriggerPrototypeList {
    int vnum;                   /* vnum of the trigger   */
    TriggerPrototypeList *next; /* next trigger          */
};

struct CampEvent {
    CharData *ch;
    int was_in;
};

/***************************************************************************
 *  File: constats.h                                      Part of FieryMUD *
 *  Usage: header file for constants                                       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#pragma once

#include "objects.hpp"
#include "prefs.hpp"
#include "structs.hpp"
#include "utils.hpp"

/* global variables */
const char circlemud_version[] = {"CircleMUD, version 3.00 beta patchlevel 11\r\n"};

const char mudlet_client_version[] = {"0.3"};
const char mudlet_client_url[] = {"http://www.fierymud.org/mudlet/FierymudOfficial.mpackage"};
const char mudlet_map_url[] = {"http://www.fierymud.org/mudlet/default_map.dat"};

/* MINOR CREATION ITEMS */
const char *minor_creation_items[] = {"backpack",                                                       /* 0  */
                                      "sack",      "robe",       "hood",       "lantern",   "torch",    /* 5  */
                                      "waterskin", "barrel",     "rations",    "raft",      "club",     /* 10 */
                                      "mace",      "dagger",     "greatsword", "longsword", "staff",    /* 15 */
                                      "shield",    "shortsword", "jacket",     "pants",     "leggings", /* 20 */
                                      "gauntlets", "sleeves",    "gloves",     "helmet",    "skullcap", /* 25 */
                                      "boots",     "sandals",    "cloak",      "book",      "quill",    /* 30 */
                                      "belt",      "ring",       "bracelet",   "bottle",    "keg",      /* 35 */
                                      "mask",      "earring",    "scarf",      "bracer",    "\n"};

/* EX_x */
const char *exit_bits[] = {"DOOR", "CLOSED", "LOCKED", "PICKPROOF", "HIDDEN", "DESCRIPT", "\n"};

/* SEX_x */
const char *genders[NUM_SEXES + 1] = {"Neuter", "Male", "Female", "Nonbinary", "\n"};

/* STANCE_x */
const char *stance_types[NUM_STANCES + 1] = {
    "dead", "mortally wounded", "incapacitated", "stunned", "sleeping", "resting", "alert", "fighting", "\n"};

/* POS_x */
const char *position_types[NUM_POSITIONS + 1] = {"prone", "sitting", "kneeling", "standing", "flying", "\n"};

/* PLR_x */
const char *player_bits[NUM_PLR_FLAGS + 1] = {"KILLER",     "THIEF",
                                              "FROZEN",     "DONTSET",
                                              "WRITING",    "MAILING",
                                              "AUTOSAVE",   "SITEOK",
                                              "NOSHOUT",    "NOTITLE",
                                              "DELETED",    "LOADRM",
                                              "!WIZL",      "!DEL",
                                              "INVST",      "CRYO",
                                              "MEDITATING", "CASTING",
                                              "BOUND",      "SCRIBE",
                                              "TEACHING",   "NAMENEEDSAPPROVE",
                                              "RENAME",     "REMOVING",
                                              "SAVING",     "GOTSTARS",
                                              "\n"};

/* MOB_x */
const char *action_bits[NUM_MOB_FLAGS + 1] = {
    "SPEC",      "SENTINEL",       "SCAVENGER",      "ISNPC",      "AWARE",           "AGGR",
    "STAY_ZONE", "WIMPY",          "AGGR_EVIL",      "AGGR_GOOD",  "AGGR_NEUTRAL",    "MEMORY",
    "HELPER",    "!CHARM",         "!SUMMN",         "!SLEEP",     "!BASH",           "!BLIND",
    "MOUNTABLE", "NO_EQ_RESTRICT", "FAST_TRACK",     "SLOW_TRACK", "CASTINGDONTUSE",  "SUMMONED_MOUNT",
    "AQUATIC",   "AGGR_EVIL_RACE", "AGGR_GOOD_RACE", "!SILENCE",   "NOVICIOUS",       "TEACHER",
    "ANIMATED",  "PEACEFUL",       "!POISON",        "ILLUSORY",   "PLAYER_PHANTASM", "!CLASS_AI",
    "!SCRIPT",   "PEACEKEEPER",    "PROTECTOR",      "\n"};

/* PRF_x */
const char *preference_bits[NUM_PRF_FLAGS + 1] = {
    "BRIEF",     "COMPACT", "DEAF",     "!TELL",       "OLCCOMM", "LINENUMS", "AUTOLOOT",  "AUTOEXIT",
    "!HASSLE",   "QUEST",   "SUMMON",   "!REPEAT",     "LIGHT",   "COLOR1",   "COLOR2",    "!WIZNET",
    "LOG1",      "LOG2",    "!AUCTION", "!GOSSIP",     "!HINTS",  "ROOMFLAG", "!PETITION", "AUTOSPLIT",
    "!CLANCOMM", "ANON",    "VNUMS",    "NICEAREA",    "VICIOUS", "PASSIVE",  "ROOMVIS",   "!FOLLOW",
    "AUTOTREAS", "STK_OBJ", "STK_MOB",  "SACRIFICIAL", "\n"};

/* PRV_x */
const char *privilege_bits[NUM_PRV_FLAGS + 1] = {"CLAN_ADMIN", "TITLE", "ANON_TOGGLE", "AUTO_GAIN", "\n"};

/* CON_x */
const char *connected_types[NUM_CON_MODES + 1] = {
    "Playing",         "Disconnecting",   "Get name",        "Confirm name",    "Get password",    "Get new PW",
    "Confirm new PW",  "Select gender",   "Select class",    "Reading MOTD",    "Main Menu",       "Get descript.",
    "Changing PW 1",   "Changing PW 2",   "Changing PW 3",   "Self-Delete 1",   "Self-Delete 2",   "Select race",
    "Con ANSI",        "Object edit",     "Room edit",       "Zone edit",       "Mobile edit",     "Shop edit",
    "Rolling stats",   "Hometown choice", "Picking bonuses", "Picking bonuses", "Picking bonuses", "Confirm stats",
    "Help edit",       "Trigger Edit",    "Class Help",      "Spell dam edit",  "Name check",      "Name approval",
    "Choose new name", "Select race",     "Bad name boot",   "Cmd group edit",  "Item edit",       "\n"};

/* WEAR_x - for eq list */
const char *where[NUM_WEARS] = {
    "<used as light>      ", "<worn on finger>     ", "<worn on finger>     ", "<worn around neck>   ",
    "<worn around neck>   ", "<worn on body>       ", "<worn on head>       ", "<worn on legs>       ",
    "<worn on feet>       ", "<worn on hands>      ", "<worn on arms>       ", "<worn as shield>     ",
    "<worn about body>    ", "<worn about waist>   ", "<worn around wrist>  ", "<worn around wrist>  ",
    "<wielded>            ", "<wielded secondary>  ", "<held>               ", "<held>               ",
    "<wielded two-handed> ", "<worn on eyes>       ", "<worn on face>       ", "<worn in left ear>   ",
    "<worn in right ear>  ", "<worn as badge>      ", "<attached to belt>   ", "<hovering>           "};

/* WEAR_x - for stat */
const char *equipment_types[NUM_WEARS + 1] = {"Used as light",
                                              "Worn on right finger",
                                              "Worn on left finger",
                                              "First worn around Neck",
                                              "Second worn around Neck",
                                              "Worn on body",
                                              "Worn on head",
                                              "Worn on legs",
                                              "Worn on feet",
                                              "Worn on hands",
                                              "Worn on arms",
                                              "Worn as shield",
                                              "Worn about body",
                                              "Worn around waist",
                                              "Worn around right wrist",
                                              "Worn around left wrist",
                                              "Wielded",
                                              "Wielded secondary",
                                              "Held",
                                              "Held",
                                              "Wielded two-handed",
                                              "Worn on eyes",
                                              "Worn on face",
                                              "Worn in left ear",
                                              "Worn in right ear",
                                              "Worn as badge",
                                              "Attached to belt",
                                              "Hovering",
                                              "\n"};

/* WEAR_x - for scripts */
const char *wear_positions[NUM_WEARS + 1] = {
    "light",   "rfinger", "lfinger",   "neck1", "neck2",  "body",   "head",  "legs",   "feet", "hands",
    "arms",    "shield",  "aboutbody", "waist", "rwrist", "lwrist", "wield", "wield2", "held", "held2",
    "2hwield", "eyes",    "face",      "lear",  "rear",   "badge",  "belt",  "hover",  "\n"};

const int wear_order_index[NUM_WEARS] = {
    WEAR_BADGE,   WEAR_HEAD,  WEAR_EYES,     WEAR_FACE,     WEAR_REAR,  WEAR_LEAR,   WEAR_NECK_1,
    WEAR_NECK_2,  WEAR_ABOUT, WEAR_BODY,     WEAR_WAIST,    WEAR_OBELT, WEAR_ARMS,   WEAR_WRIST_R,
    WEAR_WRIST_L, WEAR_HANDS, WEAR_FINGER_R, WEAR_FINGER_L, WEAR_WIELD, WEAR_WIELD2, WEAR_2HWIELD,
    WEAR_HOLD,    WEAR_HOLD2, WEAR_LIGHT,    WEAR_SHIELD,   WEAR_LEGS,  WEAR_FEET,   WEAR_HOVER};

const int wear_flags[NUM_WEARS] = {
    /* WEAR_LIGHT    */ -1,
    /* WEAR_FINGER_R */ ITEM_WEAR_FINGER,
    /* WEAR_FINGER_L */ ITEM_WEAR_FINGER,
    /* WEAR_NECK_1   */ ITEM_WEAR_NECK,
    /* WEAR_NECK_2   */ ITEM_WEAR_NECK,
    /* WEAR_BODY     */ ITEM_WEAR_BODY,
    /* WEAR_HEAD     */ ITEM_WEAR_HEAD,
    /* WEAR_LEGS     */ ITEM_WEAR_LEGS,
    /* WEAR_FEET     */ ITEM_WEAR_FEET,
    /* WEAR_HANDS    */ ITEM_WEAR_HANDS,
    /* WEAR_ARMS     */ ITEM_WEAR_ARMS,
    /* WEAR_SHIELD   */ ITEM_WEAR_SHIELD,
    /* WEAR_ABOUT    */ ITEM_WEAR_ABOUT,
    /* WEAR_WAIST    */ ITEM_WEAR_WAIST,
    /* WEAR_WRIST_R  */ ITEM_WEAR_WRIST,
    /* WEAR_WRIST_L  */ ITEM_WEAR_WRIST,
    /* WEAR_WIELD    */ ITEM_WEAR_WIELD,
    /* WEAR_WIELD2   */ ITEM_WEAR_WIELD,
    /* WEAR_HOLD     */ ITEM_WEAR_HOLD,
    /* WEAR_HOLD2    */ ITEM_WEAR_HOLD,
    /* WEAR_2HWIELD  */ ITEM_WEAR_2HWIELD,
    /* WEAR_EYES     */ ITEM_WEAR_EYES,
    /* WEAR_FACE     */ ITEM_WEAR_FACE,
    /* WEAR_LEAR     */ ITEM_WEAR_EAR,
    /* WEAR_REAR     */ ITEM_WEAR_EAR,
    /* WEAR_BADGE    */ ITEM_WEAR_BADGE,
    /* WEAR_OBELT    */ ITEM_WEAR_OBELT,
    /* WEAR_HOVER    */ ITEM_WEAR_HOVER,
};

/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[NUM_ITEM_WEAR_FLAGS + 1] = {
    "TAKE",  "FINGER", "NECK", "BODY",    "HEAD", "LEGS", "FEET", "HANDS", "ARMS",  "SHIELD", "ABOUT", "WAIST",
    "WRIST", "WIELD",  "HOLD", "2HWIELD", "EYES", "FACE", "EAR",  "BADGE", "OBELT", "HOVER",  "\n"};

/* ITEM_x (extra bits) */
const char *extra_bits[NUM_ITEM_FLAGS + 1] = {
    "GLOW",         "HUM",       "!RENT",         "!BERSERKER", "!INVIS",      "INVISIBLE", "MAGIC",     "!DROP",
    "PERMANENT",    "!GOOD",     "!EVIL",         "!NEUTRAL",   "!SORCERER",   "!CLERIC",   "!ROGUE",    "!WARRIOR",
    "!SELL",        "!PALADIN",  "!ANTI_PALADIN", "!RANGER",    "!DRUID",      "!SHAMAN",   "!ASSASSIN", "!MERCENARY",
    "!NECROMANCER", "!CONJURER", "!BURN",         "!LOCATE",    "DECOMPOSING", "FLOAT",     "!FALL",     "DISARMED",
    "!MONK",        "!BARD",     "ELVEN",         "DWARVEN",    "\n"};

/* APPLY_x */
const char *apply_types[NUM_APPLY_TYPES + 1] = {
    "NONE",         "STR",   "DEX",         "INT",         "WIS",         "CON",        "CHA",          "CLASS",
    "LEVEL",        "AGE",   "CHAR_WEIGHT", "CHAR_HEIGHT", "MAXMANA",     "HITPOINTS",  "MAXMOVE",      "GOLD",
    "EXP",          "ARMOR", "HITROLL",     "DAMROLL",     "SAVING_PARA", "SAVING_ROD", "SAVING_PETRI", "SAVING_BREATH",
    "SAVING_SPELL", "SIZE",  "HIT_REGEN",   "MANA_REGEN",  "PERCEPTION",  "HIDDENNESS", "COMPOSITION",  "\n"};

/* APPLY_x */
const char *apply_abbrevs[NUM_APPLY_TYPES + 1] = {"none", "str",  "dex",   "int",    "wis",  "con",  "cha", "cls",
                                                  "lvl",  "age",  "lbs",   "in.",    "mp",   "hp",   "mv",  "gld",
                                                  "exp",  "ac",   "hr",    "dr",     "spa",  "sr",   "spe", "sb",
                                                  "ss",   "size", "regen", "mregen", "perc", "hide", "\n"};

/* CONT_x */
const char *container_bits[] = {
    "CLOSEABLE", "PICKPROOF", "CLOSED", "LOCKED", "\n",
};

const char *carry_desc[] = {"Weightless", "Featherweight", "Paltry",         "Very Light",   "Light",      "Moderate",
                            "Burdensome", "Very Heavy",    "Instant hernia", "Immobilizing", "Intolerable"};

const char *weekdays[DAYS_PER_WEEK] = {"the Day of the Run", "the Day of the Fight", "the Day of Remembrance",
                                       "the Day of Storm",   "the Day of Fire",      "the Day of Conquest",
                                       "the Day of Rest"};

const char *rolls_abils_result[] = {"Fantastic  ", "Very Good  ", "Good       ", "Fair       ", "Bad        "};

const char *month_name[MONTHS_PER_YEAR] = {"Month of Winter", /* 0 */
                                           "Month of the Dark Destiny",
                                           "Month of the Arcane Power",
                                           "Month of the Moonless Night",
                                           "Month of the Spring",
                                           "Month of the Diabolical Awakening",
                                           "Month of Resistance",
                                           "Month of the Shal Du Stauk",
                                           "Month of the Firestorm",
                                           "Month of the Long Journey",
                                           "Month of Discovery",
                                           "Month of the Stranger",
                                           "Month of Heroes and Valor",
                                           "Month of the Great Deceit",
                                           "Month of the Rift War",
                                           "Month of the Wicked Deception"};

const int sharp[] = {0, 0, 0, 1,  /* Slashing */
                     0, 0, 0, 0,  /* Bludgeon */
                     0, 0, 0, 0}; /* Pierce   */

#define DEFAULT_PROMPT 4

/*
 * Only access this array directly if you are listing them all out.
 * Otherwise use sprint_log_severity and parse_log_severity to
 * convert a value to a string and vice-versa.
 */
const char *log_severities[8] = {"fine",        /* 10 */
                                 "informative", /* 20 */
                                 "debug",       /* 30 */
                                 "status",      /* 40 */
                                 "warning",     /* 50 */
                                 "error",       /* 60 */
                                 "critical",    /* 70 */
                                 "\n"};

const char *default_prompts[][2] = {{"Basic", "&0%hhp %vmv>&0 "},
                                    {"Colorized Basic", "&1&b%h&0&1hp &2&b%v&0&2mv&0> "},
                                    {"Basic Percentages", "&1&b%ph&0&1hp &2&b%pv&0&2mv&0> "},
                                    {"Full-Featured",
                                     "&6Opponent&0: &4&b%o &7&b/ &0&6Tank&0: &4&b%t%_&0&1%h&0(&1&b%H&0)"
                                     "hitp &2%v&0(&2&b%V&0)&7move&0> "},
                                    {"Standard",
                                     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b>%_"
                                     "&0<%t&0>:<&0%o&0> "},
                                    {"Complete w/ Spells",
                                     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
                                     "<&0&2%aA&2&b> <&0%l&2&b>%_&0<%t&0>:<&0%o&0> "},
                                    {"Complete w/ Exp",
                                     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
                                     "<&0&2%aA&2&b> <&0%e&2&b>%_&0<%t&0>:<&0%o&0> "},
                                    {"Complete w/ Hide Pts",
                                     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
                                     "<&0&2%aA&2&b> <&0&2%ih&2&b>%_&0<%t&0>:<&0%o&0> "},
                                    {"Complete w/ Rage",
                                     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
                                     "<&0&2%aA&2&b> <&0&2%rr&2&b>%_&0<%t&0>:<&0%o&0> "},
                                    {"Complete w/ 1st Aid",
                                     "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
                                     "<&0&2%aA&2&b> <&0&2%df&2&b>%_&0<%t&0>:<&0%o&0> "},
                                    {nullptr, nullptr}};

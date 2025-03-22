/***************************************************************************
 *   File: constants.c                                    Part of FieryMUD *
 *  Usage: Numeric and string contants used by the MUD                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "constants.hpp"

#include "conf.hpp"
#include "db.hpp"
#include "logging.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

// Globals

// Utility
const std::string_view number_words[] = {"zero", "one",   "two",   "three", "four", "five",
                                         "six",  "seven", "eight", "nine",  "ten"};

/* MINOR CREATION ITEMS */
const std::string_view minor_creation_items[] = {
    "backpack",                                                       /* 0  */
    "sack",      "robe",       "hood",       "lantern",   "torch",    /* 5  */
    "waterskin", "barrel",     "rations",    "raft",      "club",     /* 10 */
    "mace",      "dagger",     "greatsword", "longsword", "staff",    /* 15 */
    "shield",    "shortsword", "jacket",     "pants",     "leggings", /* 20 */
    "gauntlets", "sleeves",    "gloves",     "helmet",    "skullcap", /* 25 */
    "boots",     "sandals",    "cloak",      "book",      "quill",    /* 30 */
    "belt",      "ring",       "bracelet",   "bottle",    "keg",      /* 35 */
    "mask",      "earring",    "scarf",      "bracer",    "\n"};

/* EX_x */
const std::string_view exit_bits[] = {"DOOR", "CLOSED", "LOCKED", "PICKPROOF", "HIDDEN", "DESCRIPT", "\n"};

/* SEX_x */
const std::string_view genders[NUM_SEXES + 1] = {"Neuter", "Male", "Female", "Nonbinary", "\n"};

/* STANCE_x */
const std::string_view stance_types[NUM_STANCES + 1] = {
    "dead", "mortally wounded", "incapacitated", "stunned", "sleeping", "resting", "alert", "fighting", "\n"};

/* POS_x */
const std::string_view position_types[NUM_POSITIONS + 1] = {"prone", "sitting", "kneeling", "standing", "flying", "\n"};

/* PLR_x */
const std::string_view player_bits[NUM_PLR_FLAGS + 1] = {"KILLER",     "THIEF",
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
const std::string_view action_bits[NUM_MOB_FLAGS + 1] = {"SPEC",
                                                         "SENTINEL",
                                                         "SCAVENGER",
                                                         "ISNPC",
                                                         "AWARE",
                                                         "AGGR",
                                                         "STAY_ZONE",
                                                         "WIMPY",
                                                         "AGGR_EVIL",
                                                         "AGGR_GOOD",
                                                         "AGGR_NEUTRAL",
                                                         "MEMORY",
                                                         "HELPER",
                                                         "!CHARM",
                                                         "!SUMMN",
                                                         "!SLEEP",
                                                         "!BASH",
                                                         "!BLIND",
                                                         "MOUNTABLE",
                                                         "NO_EQ_RESTRICT",
                                                         "FAST_TRACK",
                                                         "SLOW_TRACK",
                                                         "CASTINGDONTUSE",
                                                         "SUMMONED_MOUNT",
                                                         "AQUATIC",
                                                         "AGGR_EVIL_RACE",
                                                         "AGGR_GOOD_RACE",
                                                         "!SILENCE",
                                                         "NOVICIOUS",
                                                         "TEACHER",
                                                         "ANIMATED",
                                                         "PEACEFUL",
                                                         "!POISON",
                                                         "ILLUSORY",
                                                         "PLAYER_PHANTASM",
                                                         "!CLASS_AI",
                                                         "!SCRIPT",
                                                         "PEACEKEEPER",
                                                         "PROTECTOR",
                                                         "PET",
                                                         "MEDITATEDONTUSE,"
                                                         "\n"};

/* PRF_x */
const std::string_view preference_bits[NUM_PRF_FLAGS + 1] = {
    "BRIEF",     "COMPACT", "DEAF",     "!TELL",       "OLCCOMM", "LINENUMS", "AUTOLOOT",  "AUTOEXIT",
    "!HASSLE",   "QUEST",   "SUMMON",   "!REPEAT",     "LIGHT",   "COLOR1",   "COLOR2",    "!WIZNET",
    "LOG1",      "LOG2",    "!AUCTION", "!GOSSIP",     "!HINTS",  "ROOMFLAG", "!PETITION", "AUTOSPLIT",
    "!CLANCOMM", "ANON",    "VNUMS",    "NICEAREA",    "VICIOUS", "PASSIVE",  "ROOMVIS",   "!FOLLOW",
    "AUTOTREAS", "STK_OBJ", "STK_MOB",  "SACRIFICIAL", "\n"};

/* PRV_x */
const std::string_view privilege_bits[NUM_PRV_FLAGS + 1] = {"CLAN_ADMIN", "TITLE", "ANON_TOGGLE", "AUTO_GAIN", "\n"};

/* CON_x */
const std::string_view connected_types[NUM_CON_MODES + 1] = {
    "Playing",         "Disconnecting",   "Get name",        "Confirm name",    "Get password",    "Get new PW",
    "Confirm new PW",  "Select gender",   "Select class",    "Reading MOTD",    "Main Menu",       "Get descript.",
    "Changing PW 1",   "Changing PW 2",   "Changing PW 3",   "Self-Delete 1",   "Self-Delete 2",   "Select race",
    "Con ANSI",        "Object edit",     "Room edit",       "Zone edit",       "Mobile edit",     "Shop edit",
    "Rolling stats",   "Hometown choice", "Picking bonuses", "Picking bonuses", "Picking bonuses", "Confirm stats",
    "Help edit",       "Trigger Edit",    "Class Help",      "Spell dam edit",  "Name check",      "Name approval",
    "Choose new name", "Select race",     "Bad name boot",   "Cmd group edit",  "Item edit",       "Select str",
    "Select dex",      "Select con",      "select int",      "Select wis",      "Select cha",      "\n"};

/* WEAR_x - for eq list */
const std::string_view where[NUM_WEARS] = {
    "<used as light>      ", "<worn on finger>     ", "<worn on finger>     ", "<worn around neck>   ",
    "<worn around neck>   ", "<worn on body>       ", "<worn on head>       ", "<worn on legs>       ",
    "<worn on feet>       ", "<worn on hands>      ", "<worn on arms>       ", "<worn as shield>     ",
    "<worn about body>    ", "<worn about waist>   ", "<worn around wrist>  ", "<worn around wrist>  ",
    "<wielded>            ", "<wielded secondary>  ", "<held>               ", "<held>               ",
    "<wielded two-handed> ", "<worn on eyes>       ", "<worn on face>       ", "<worn in left ear>   ",
    "<worn in right ear>  ", "<worn as badge>      ", "<attached to belt>   ", "<hovering>           "};

/* WEAR_x - for stat */
const std::string_view equipment_types[NUM_WEARS + 1] = {"Used as light",
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
const std::string_view wear_positions[NUM_WEARS + 1] = {
    "light",   "rfinger", "lfinger",   "neck1", "neck2",  "body",   "head",  "legs",   "feet", "hands",
    "arms",    "shield",  "aboutbody", "waist", "rwrist", "lwrist", "wield", "wield2", "held", "held2",
    "2hwield", "eyes",    "face",      "lear",  "rear",   "badge",  "belt",  "hover",  "\n"};

int wear_order_index[NUM_WEARS] = {WEAR_BADGE,  WEAR_HEAD,    WEAR_EYES,    WEAR_FACE,  WEAR_REAR,     WEAR_LEAR,
                                   WEAR_NECK_1, WEAR_NECK_2,  WEAR_ABOUT,   WEAR_BODY,  WEAR_WAIST,    WEAR_OBELT,
                                   WEAR_ARMS,   WEAR_WRIST_R, WEAR_WRIST_L, WEAR_HANDS, WEAR_FINGER_R, WEAR_FINGER_L,
                                   WEAR_WIELD,  WEAR_WIELD2,  WEAR_2HWIELD, WEAR_HOLD,  WEAR_HOLD2,    WEAR_LIGHT,
                                   WEAR_SHIELD, WEAR_LEGS,    WEAR_FEET,    WEAR_HOVER};

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
const std::string_view wear_bits[NUM_ITEM_WEAR_FLAGS + 1] = {
    "TAKE",  "FINGER", "NECK", "BODY",    "HEAD", "LEGS", "FEET", "HANDS", "ARMS",  "SHIELD", "ABOUT", "WAIST",
    "WRIST", "WIELD",  "HOLD", "2HWIELD", "EYES", "FACE", "EAR",  "BADGE", "OBELT", "HOVER",  "\n"};

/* ITEM_x (extra bits) */
const std::string_view extra_bits[NUM_ITEM_FLAGS + 1] = {
    "GLOW",          "HUM",         "!RENT",       "!BERSERKER",   "!INVIS",      "INVISIBLE",
    "MAGIC",         "!DROP",       "PERMANENT",   "!GOOD",        "!EVIL",       "!NEUTRAL",
    "!SORCERER",     "!CLERIC",     "!ROGUE",      "!WARRIOR",     "!SELL",       "!PALADIN",
    "!ANTI_PALADIN", "!RANGER",     "!DRUID",      "!SHAMAN",      "!ASSASSIN",   "!MERCENARY",
    "!NECROMANCER",  "!CONJURER",   "!BURN",       "!LOCATE",      "DECOMPOSING", "FLOAT",
    "!FALL",         "DISARMED",    "!MONK",       "!BARD",        "ELVEN",       "DWARVEN",
    "!THIEF",        "!PYROMANCER", "!CRYOMANCER", "!ILLUSIONIST", "!PRIEST",     "!DIABOLIST",
    "!TINY",         "!SMALL",      "!MEDIUM",     "!LARGE",       "!HUGE",       "!GIANT",
    "!GARGANTUAN",   "!COLOSSAL",   "!TITANIC",    "!MOUNTAINOUS", "!ARBOREAN",   "\n"};

/* APPLY_x */
const std::string_view apply_types[NUM_APPLY_TYPES + 1] = {
    "NONE",         "STR",   "DEX",         "INT",         "WIS",         "CON",        "CHA",          "CLASS",
    "LEVEL",        "AGE",   "CHAR_WEIGHT", "CHAR_HEIGHT", "MAXMANA",     "HITPOINTS",  "MAXMOVE",      "GOLD",
    "EXP",          "ARMOR", "HITROLL",     "DAMROLL",     "SAVING_PARA", "SAVING_ROD", "SAVING_PETRI", "SAVING_BREATH",
    "SAVING_SPELL", "SIZE",  "HIT_REGEN",   "FOCUS",       "PERCEPTION",  "HIDDENNESS", "COMPOSITION",  "\n"};

/* APPLY_x */
const std::string_view apply_abbrevs[NUM_APPLY_TYPES + 1] = {
    "none", "str", "dex", "int",  "wis",   "con",    "cha",  "cls",  "lvl", "age", "lbs",
    "in.",  "mp",  "hp",  "mv",   "gld",   "exp",    "ac",   "hr",   "dr",  "spa", "sr",
    "spe",  "sb",  "ss",  "size", "regen", "mregen", "perc", "hide", "\n"};

/* CONT_x */
const std::string_view container_bits[] = {
    "CLOSEABLE", "PICKPROOF", "CLOSED", "LOCKED", "\n",
};

const std::string_view carry_desc[] = {"Weightless",     "Featherweight", "Paltry",     "Very Light",
                                       "Light",          "Moderate",      "Burdensome", "Very Heavy",
                                       "Instant hernia", "Immobilizing",  "Intolerable"};

const std::string_view weekdays[DAYS_PER_WEEK] = {
    "the Day of the Run", "the Day of the Fight", "the Day of Remembrance", "the Day of Storm",
    "the Day of Fire",    "the Day of Conquest",  "the Day of Rest"};

const std::string_view rolls_abils_result[] = {"Fantastic  ", "Very Good  ", "Average    ", "Mediocre   ",
                                               "Bad        "};

const std::string_view month_name[MONTHS_PER_YEAR] = {"Month of Winter", /* 0 */
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

/* The following functions replace the hard coded bonus */
/* tables for attributes.  This was done for the conversion */
/* to the 100 base system.  These functions closely approximate */
/* the older 18 base hard coded arrays. -gurlaek 6/24/1999 */

void load_stat_bonus(void) {
    int x;

    for (x = 0; x <= 100; x++) {
        /* (x, bonus) where [bonus=Mx + B] is the notation used below */
        /* M and B were determined from the old numbers */

        /* tohit bonus */
        if (x <= 28 && x >= 0) /* linear from (0,-5) to (28,-1) */
            stat_bonus[x].tohit = (sh_int)((((float)1 / 7) * (float)x) - 5);
        if (x <= 64 && x >= 29) /* no bonus */
            stat_bonus[x].tohit = 0;
        if (x <= 100 && x >= 65) /* linear from (65,1) to (100,7) */
            stat_bonus[x].tohit = (sh_int)((((float)6 / 35) * (float)x) - ((float)75 / 7));

        /* todam bonus */
        if (x <= 20 && x >= 0) /* linear from (0,-4) to (20,-1) */
            stat_bonus[x].todam = (sh_int)((((float)3 / 20) * (float)x) - 4);
        if (x <= 60 && x >= 21) /* no bonus */
            stat_bonus[x].todam = 0;
        if (x <= 100 && x >= 61) /* linear from (61,1) to (100,14) */
            stat_bonus[x].todam = (sh_int)((((float)13 / 39) * (float)x) - ((float)754 / 39));

        /* defensive (mostly AC) bonus */
        if (x <= 24 && x >= 0) /* linear from (0,6) to (24,1) */
            stat_bonus[x].defense = (sh_int)((((float)-5 / 24) * (float)x) + 6);
        if (x <= 56 && x >= 25) /* zero */
            stat_bonus[x].defense = 0;
        if (x <= 100 && x >= 57) /* linear from (57,-1) to (100,-6) */
            stat_bonus[x].defense = (sh_int)((((float)-5 / 43) * (float)x) + ((float)242 / 43));

#define CARRY_100_STR 786.0
#define CARRY_72_STR 300.0

        /* carry */
        if (x <= 72 && x >= 0)
            stat_bonus[x].carry = (sh_int)((CARRY_72_STR * x) / 72);
        if (x <= 100 && x >= 73)
            stat_bonus[x].carry = (sh_int)(CARRY_72_STR + ((CARRY_100_STR - CARRY_72_STR) * ((x - 72) / 28.0)));

        /* wield */
        if (x <= 72 && x >= 0) /* linear from (0,0) to (72, 20) */
            stat_bonus[x].wield = (sh_int)((((float)5 / 18) * (float)x));
        if (x <= 100 && x >= 73) /* linear from (73,40) (100,70) */
            stat_bonus[x].wield = (sh_int)((((float)30 / 27) * (float)x) - ((float)370 / 9));

        /* bonus to spells */
        if (x <= 44 && x >= 0) /*  zero */
            stat_bonus[x].magic = 0;
        if (x <= 100 && x >= 45) /* linear from (45,2) to (100,7) */
            stat_bonus[x].magic = (sh_int)((((float)1 / 11) * (float)x) - ((float)23 / 11));

        /* hit point bonus */
        if (x <= 24 && x >= 0) /* linear from (0,-4) to (24,-1) */
            stat_bonus[x].hpgain = (sh_int)((((float)1 / 8) * (float)x) - 4);
        if (x <= 56 && x >= 25) /* zero */
            stat_bonus[x].hpgain = 0;
        if (x <= 100 && x >= 57) /* linear from (57,1) to (96,5) */
            stat_bonus[x].hpgain = (sh_int)((((float)1 / 8) * (float)x) - ((float)121 / 20));
        if (x > 96) /* five */
            stat_bonus[x].hpgain = 5;

        /* small bonus */
        if (x <= 44 && x >= 0) /* linear from (0,-7) to (44,-1) */
            stat_bonus[x].skill_small = (sh_int)((((float)6 / 44) * (float)x) - 7);
        if (x <= 59 && x >= 45) /* zero */
            stat_bonus[x].skill_small = 0;
        if (x <= 100 && x >= 60) /* linear from (60,1) to (100,5) */
            stat_bonus[x].skill_small = (sh_int)((((float)1 / 10) * (float)x) - 5);

        /* medium bonus */
        if (x <= 44 && x >= 0) /* linear from (0,-18) to (44,-2) */
            stat_bonus[x].skill_medium = (sh_int)((((float)3 / 8) * (float)x) - ((float)27 / 2));
        if (x <= 54 && x >= 45) /* zero */
            stat_bonus[x].skill_medium = 0;
        if (x <= 100 && x >= 55) /* linear from (55,2) to (100,10) */
            stat_bonus[x].skill_medium = (sh_int)((((float)89 / 500) * (float)x) - ((float)779 / 100));

        /* large bonus */
        if (x <= 44 && x >= 0) /* linear from (0,-28) to (44,-1) */
            stat_bonus[x].skill_large = (sh_int)((((float)5 / 8) * (float)x) - ((float)57 / 2));
        if (x <= 49 && x >= 45) /* zero */
            stat_bonus[x].skill_large = 0;
        if (x <= 100 && x >= 50) /* linear from (50,3) to (100,15) */
            stat_bonus[x].skill_large = (sh_int)((((float)6 / 25) * (float)x) - 9);

        /* Rogue skills with big numbers like sneak, hide, pick pocket, etc. */
        if (x <= 48 && x >= 0) /* linear from (0,-99) to (48,-5) */
            stat_bonus[x].rogue_skills = (sh_int)((((float)47 / 24) * (float)x) - 99);
        if (x <= 64 && x >= 49) /* zero */
            stat_bonus[x].rogue_skills = 0;
        if (x <= 100 && x >= 65) /* linear from (65,5) to (100,25) */
            stat_bonus[x].rogue_skills = (sh_int)((((float)4 / 7) * (float)x) - ((float)225 / 7));
    }
}

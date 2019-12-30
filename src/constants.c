/***************************************************************************
 * $Id: constants.c,v 1.148 2010/06/05 18:35:47 mud Exp $
 ***************************************************************************/
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

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"

/* global variables */
const char circlemud_version[] = {
  "CircleMUD, version 3.00 beta patchlevel 11\r\n"};

const char mudlet_client_version[] = {"0.1"};
const char mudlet_client_url[] = {"http://fierymud.org/mudlet/fierymud_0_1.mpackage"};
const char mudlet_map_url[] = {"http://fierymud.org/mudlet/default_map.dat"};

/* MINOR CREATION ITEMS */
const char *minor_creation_items[] = {
  "backpack",  	/* 0  */
  "sack",
  "robe",
  "ebony",
  "lantern",
  "torch",	/* 5  */
  "waterskin",
  "barrel",
  "rations",
  "raft",
  "club",	/* 10 */
  "mace",
  "dagger",
  "sword",
  "longsword",
  "staff",	/* 15 */
  "shield",
  "tunic",
  "jerkin",
  "leggings",
  "pants",	/* 20 */
  "gauntlets",
  "sleeves",
  "gloves",
  "helmet",
  "skullcap",	/* 25 */
  "boots",
  "sandals",
  "cloak",
  "book",
  "quill",	/* 30 */
  "belt",
  "ring",
  "rope",
  "bottle",
  "keg",	/* 35 */
  "mask",
  "earring",
  "scarf",
  "bracer",
  "\n"
};


/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "HIDDEN",
  "DESCRIPT",
  "\n"
};


/* SEX_x */
const char *genders[NUM_SEXES + 1] =
{
  "Neuter",
  "Male",
  "Female",
  "\n"
};

/* STANCE_x */
const char *stance_types[NUM_STANCES + 1] = {
  "dead",
  "mortally wounded",
  "incapacitated",
  "stunned",
  "sleeping",
  "resting",
  "alert",
  "fighting",
  "\n"
};

/* POS_x */
const char *position_types[NUM_POSITIONS + 1] = {
  "prone",
  "sitting",
  "kneeling",
  "standing",
  "flying",
  "\n"
};


/* PLR_x */
const char *player_bits[NUM_PLR_FLAGS + 1] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "AUTOSAVE",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "!WIZL",
  "!DEL",
  "INVST",
  "CRYO",
  "MEDITATING",
  "CASTING",
  "BOUND",
  "SCRIBE",
  "TEACHING",
  "NAMENEEDSAPPROVE",
  "RENAME",
  "REMOVING",
  "SAVING",
  "GOTSTARS",
  "\n"
};


/* MOB_x */
const char *action_bits[NUM_MOB_FLAGS + 1] = {
  "SPEC",
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
  "\n"
};


/* PRF_x */
const char *preference_bits[NUM_PRF_FLAGS + 1] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "!TELL",
  "OLCCOMM",
  "LINENUMS",
  "AUTOLOOT",
  "AUTOEXIT",
  "!HASSLE",
  "QUEST",
  "SUMMON",
  "!REPEAT",
  "LIGHT",
  "COLOR1",
  "COLOR2",
  "!WIZNET",
  "LOG1",
  "LOG2",
  "!AUCTION",
  "!GOSSIP",
  "!HINTS",
  "ROOMFLAG",
  "!PETITION",
  "AUTOSPLIT",
  "!CLANCOMM",
  "ANON",
  "VNUMS",
  "NICEAREA",
  "VICIOUS",
  "PASSIVE",
  "ROOMVIS",
  "!FOLLOW",
  "AUTOTREAS",
  "STK_OBJ",
  "STK_MOB",
  "SACRIFICIAL",
  "\n"
};

/* PRV_x */
const char *privilege_bits[NUM_PRV_FLAGS + 1] = {
  "CLAN_ADMIN",
  "TITLE",
  "ANON_TOGGLE",
  "AUTO_GAIN",
  "\n"
};

/* CON_x */
const char *connected_types[NUM_CON_MODES + 1] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Select race",
  "Con ANSI",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Rolling stats",
  "Hometown choice",
  "Picking bonuses",
  "Picking bonuses",
  "Picking bonuses",
  "Confirm stats",
  "Help edit",
  "Trigger Edit",
  "Class Help",
  "Spell dam edit",
  "Name check",
  "Name approval",
  "Choose new name",
  "Select race",
  "Bad name boot",
  "Cmd group edit",
  "Item edit",
  "\n"
};


/* WEAR_x - for eq list */
const char *where[NUM_WEARS] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<wielded secondary>  ",
  "<held>               ",
  "<held>               ",
  "<wielded two-handed> ",
  "<worn on eyes>       ",
  "<worn on face>       ",
  "<worn in left ear>   ",
  "<worn in right ear>  ",
  "<worn as badge>      ",
  "<attached to belt>   "
};


/* WEAR_x - for stat */
const char *equipment_types[NUM_WEARS + 1] = {
  "Used as light",
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
  "\n"
};

/* WEAR_x - for scripts */
const char *wear_positions[NUM_WEARS + 1] = {
  "light",
  "rfinger",
  "lfinger",
  "neck1",
  "neck2",
  "body",
  "head",
  "legs",
  "feet",
  "hands",
  "arms",
  "shield",
  "aboutbody",
  "waist",
  "rwrist",
  "lwrist",
  "wield",
  "wield2",
  "held",
  "held2",
  "2hwield",
  "eyes",
  "face",
  "lear",
  "rear",
  "badge",
  "belt",
  "\n"
};

const int wear_order_index[NUM_WEARS] = {
  WEAR_BADGE,
  WEAR_HEAD,
  WEAR_EYES,
  WEAR_FACE,
  WEAR_REAR,
  WEAR_LEAR,
  WEAR_NECK_1,
  WEAR_NECK_2,
  WEAR_ABOUT,
  WEAR_BODY,
  WEAR_WAIST,
  WEAR_OBELT,
  WEAR_ARMS,
  WEAR_WRIST_R,
  WEAR_WRIST_L,
  WEAR_HANDS,
  WEAR_FINGER_R,
  WEAR_FINGER_L,
  WEAR_WIELD,
  WEAR_WIELD2,
  WEAR_2HWIELD,
  WEAR_HOLD,
  WEAR_HOLD2,
  WEAR_LIGHT,
  WEAR_SHIELD,
  WEAR_LEGS,
  WEAR_FEET
};

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
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[NUM_ITEM_WEAR_FLAGS + 1] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "2HWIELD",
  "EYES",
  "FACE",
  "EAR",
  "BADGE",
  "OBELT",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[NUM_ITEM_FLAGS + 1] = {
  "GLOW",
  "HUM",
  "!RENT",
  "!DONATE",
  "!INVIS",
  "INVISIBLE",
  "MAGIC",
  "!DROP",
  "PERMANENT",
  "!GOOD",
  "!EVIL",
  "!NEUTRAL",
  "!SORCERER",
  "!CLERIC",
  "!ROGUE",
  "!WARRIOR",
  "!SELL",
  "!PALADIN",
  "!ANTI_PALADIN",
  "!RANGER",
  "!DRUID",
  "!SHAMAN",
  "!ASSASSIN",
  "!MERCENARY",
  "!NECROMANCER",
  "!CONJURER",
  "!BURN",
  "!LOCATE",
  "DECOMPOSING",
  "FLOAT",
  "!FALL",
  "DISARMED",
  "\n"
};


/* APPLY_x */
const char *apply_types[NUM_APPLY_TYPES + 1] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "HITPOINTS",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "SIZE",
  "HIT_REGEN",
  "MANA_REGEN",
  "PERCEPTION",
  "HIDDENNESS",
  "COMPOSITION",
  "\n"
};


/* APPLY_x */
const char *apply_abbrevs[NUM_APPLY_TYPES + 1] = {
  "none",
  "str",
  "dex",
  "int",
  "wis",
  "con",
  "cha",
  "cls",
  "lvl",
  "age",
  "lbs",
  "in.",
  "mp",
  "hp",
  "mv",
  "gld",
  "exp",
  "ac",
  "hr",
  "dr",
  "spa",
  "sr",
  "spe",
  "sb",
  "ss",
  "size",
  "regen",
  "mregen",
  "perc",
  "hide",
  "\n"
};


/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


const char *carry_desc[] =
{
  "Weightless",
  "Featherweight",
  "Paltry",
  "Very Light",
  "Light",
  "Moderate",
  "Burdensome",
  "Very Heavy",
  "Instant hernia",
  "Immobilizing",
  "Intolerable"
};


const char *weekdays[DAYS_PER_WEEK] = {
  "the Day of the Run",
  "the Day of the Fight",
  "the Day of Remembrance",
  "the Day of Storm",
  "the Day of Fire",
  "the Day of Conquest",
  "the Day of Rest"
};

const char *rolls_abils_result[] = {
  "Fantastic  ",
  "Pretty Good",
  "Not bad    ",
  "Fair       ",
  "Bad        "
};

const char *month_name[MONTHS_PER_YEAR] = {
  "Month of Winter",		/* 0 */
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
  "Month of the Wicked Deception"
};

const int sharp[] = {
  0,
  0,
  0,
  1,				/* Slashing */
  0,
  0,
  0,
  0,				/* Bludgeon */
  0,
  0,
  0,
0};				/* Pierce   */

/* The following functions replace the hard coded bonus */
/* tables for attributes.  This was done for the conversion */
/* to the 100 base system.  These functions closely approximate */
/* the older 18 base hard coded arrays. -gurlaek 6/24/1999 */

void load_str_app(void) {
  int x;

  for(x = 0; x <= 100; x++) {
    /* (x, bonus) where [bonus=Mx + B] is the notation used below */
    /* M and B were determined from the old numbers */
    
    /* tohit bonus */
    if(x <= 28 && x >= 0) /* linear from (0,-5) to (28,-1) */
      str_app[x].tohit = (sh_int)((((float)1/7) * (float)x) - 5);
    if(x <= 64 && x >= 29) /* no bonus */
      str_app[x].tohit = 0;
    if(x <= 100 && x >= 65) /* linear from (65,1) to (100,7) */
      str_app[x].tohit = (sh_int)((((float)6/35) * (float)x) - ((float)75/7));
    
    /* todam bonus */
    if(x <= 20 && x >= 0) /* linear from (0,-4) to (20,-1) */
      str_app[x].todam = (sh_int)((((float)3/20) * (float)x) - 4);
    if(x <= 60 && x >= 21) /* no bonus */
      str_app[x].todam = 0;
    if(x <= 100 && x >= 61) /* linear from (61,1) to (100,14) */
      str_app[x].todam = (sh_int)((((float)13/39) * (float)x) - ((float)754/39));

#define CARRY_72_STR    300.0
#define CARRY_100_STR   786.0
    /* carry_w */
    if (x <= 72 && x >= 0)
      str_app[x].carry_w = (sh_int)((CARRY_72_STR * x) / 72);
    if (x <= 100 && x >= 73)
      str_app[x].carry_w = (sh_int)
        (CARRY_72_STR + ((CARRY_100_STR - CARRY_72_STR) * ((x - 72) / 28.0)));

    /* wield_w */
    if(x <= 72 && x >= 0) /* linear from (0,0) to (72, 20) */
      str_app[x].wield_w = (sh_int)((((float)5/18) * (float)x));
    if(x <= 100 && x >= 73) /* linear from (73,40) (100,70) */
      str_app[x].wield_w = (sh_int)((((float)30/27) * (float)x) - ((float)370/9));
  }
}

void load_thief_dex(void) {
  int x;

  for(x = 0; x <= 100; x++) {
    /* (x, bonus) where [bonus=Mx + B] is the notation used below */
    /* M and B were determined from the old numbers */
    
    /* pick pockets */
    if(x <= 44 && x >= 0) /* linear from (0,-99) to (44,-5) */
      dex_app_skill[x].p_pocket = (sh_int)((((float)47/22) * (float)x) - 99);
    if(x <= 64 && x >=45) /* zero */
      dex_app_skill[x].p_pocket = 0;
    if(x <= 100 && x >= 65) /* linear from (65,5) to (100,25) */
      dex_app_skill[x].p_pocket = (sh_int)((((float)4/7) * (float)x) - ((float)225/7));

    /* pick locks */
    if(x <= 40 && x >= 0) /* linear from (0,-99) to (40,-5) */
      dex_app_skill[x].p_locks = (sh_int)((((float)47/20) * (float)x) - 99);
    if(x <= 60 && x >=41) /* zero */
      dex_app_skill[x].p_locks = 0;
    if(x <= 100 && x >= 61) /* linear from (61,5) to (100,30) */
      dex_app_skill[x].p_locks = (sh_int)((((float)25/39) * (float)x) - ((float)1330/39));
    
    /* traps */
    if(x <= 44 && x >= 0) /* linear from (0,-90) to (44,-5) */
      dex_app_skill[x].traps = (sh_int)((((float)85/44) * (float)x) - 90);
    if(x <= 68 && x >=45) /* zero */
      dex_app_skill[x].traps = 0;
    if(x <= 100 && x >= 69) /* linear from (69,5) to (100,15) */
      dex_app_skill[x].traps = (sh_int)((((float)10/31) * (float)x) - ((float)535/31));

    /* sneak */
    if(x <= 48 && x >= 0) /* linear from (0,-99) to (48,-5) */
      dex_app_skill[x].sneak = (sh_int)((((float)47/24) * (float)x) - 99);
    if(x <= 64 && x >=49) /* zero */
      dex_app_skill[x].sneak = 0;
    if(x <= 100 && x >= 65) /* linear from (65,5) to (100,25) */
      dex_app_skill[x].sneak = (sh_int)((((float)4/7) * (float)x) - ((float)225/7));

    /* hide */
    if(x <= 40 && x >= 0) /* linear from (0,-99) to (40,-5) */
      dex_app_skill[x].hide = (sh_int)((((float)11/8) * (float)x) - 60);
    if(x <= 64 && x >=41) /* zero */
      dex_app_skill[x].hide = 0;
    if(x <= 100 && x >= 65) /* linear from (65,5) to (100,25) */
      dex_app_skill[x].hide = (sh_int)((((float)4/7) * (float)x) - ((float)225/7));
  }
}

void load_dex_app(void) {
  int x;

  for(x = 0; x <= 100; x++) {
    /* (x, bonus) where [bonus=Mx + B] is the notation used below */
    /* M and B were determined from the old numbers */
    
    /* reaction adjustment */
    if(x <= 20 && x >= 0) /* linear from (0,-7) to (20,-2) */
      dex_app[x].reaction = (sh_int)((((float)6/20) * (float)x) - 7);
    if(x <= 60 && x >= 21) /* zero */
      dex_app[x].reaction = 0;
    if(x <= 100 && x >= 61) /* linear from (61,1) to (100,5) */
      dex_app[x].reaction = (sh_int)((((float)4/39) * (float)x) - ((float)205/39));

    /* missile attacks to hit bonus */
    if(x <= 20 && x >= 0) /* linear from (0,-7) to (20,-2) */
      dex_app[x].miss_att = (sh_int)((((float)6/20) * (float)x) - 7);
    if(x <= 60 && x >= 21) /* zero */
      dex_app[x].miss_att = 0;
    if(x <= 100 && x >= 61) /* linear from (61,1) to (100,5) */
      dex_app[x].miss_att = (sh_int)((((float)4/39) * (float)x) - ((float)205/39));

    /* AC bonus */
     if(x <= 24 && x >= 0) /* linear from (0,6) to (24,1) */
      dex_app[x].defensive = (sh_int)((((float)-5/24) * (float)x) + 6);
    if(x <= 56 && x >= 25) /* zero */
      dex_app[x].defensive = 0;
    if(x <= 100 && x >= 57) /* linear from (57,-1) to (100,-6) */
      dex_app[x].defensive = (sh_int)((((float)-5/43) * (float)x) + ((float)242/43));
   
  }
}

void load_con_app(void) {
  int x;

  for(x = 0; x <= 100; x++) {
    /* (x, bonus) where [bonus=Mx + B] is the notation used below */
    /* M and B were determined from the old numbers */
    
    /* hit point bonus */
    if(x <= 24 && x >= 0) /* linear from (0,-4) to (24,-1) */
      con_app[x].hitp = (sh_int)((((float)1/8) * (float)x) - 4);
    if(x <= 56 && x >= 25) /* zero */
      con_app[x].hitp = 0;
    if(x <= 100 && x >= 57) /* linear from (57,1) to (100,5) */
      con_app[x].hitp = (sh_int)((((float)4/43) * (float)x) - ((float)185/43));
    
    /* system shock survival percentage */
    if(x <= 68 && x >= 0) /* linear from (0,20) to (68,97) */
      con_app[x].shock = (sh_int)((((float)77/68) * (float)x) + 20);
    if(x <= 100 && x >= 69)
      con_app[x].shock = 99;
  }
}

void load_int_app(void) {
  int x;

  for(x = 0; x <= 100; x++) {
    /* percent to learn spell or skill */
    if(x <= 100 && x >= 0) /* linear from (0,3) to (100,60) */
      int_app[x].learn = (byte)((((float)57/100) * (float)x) + 3);
  }
}

void load_wis_app(void) {
  int x;

  for(x = 0; x <= 100; x++) {
    /* bonus practices per level */
    if(x <= 44 && x >= 0) /*  zero */
      wis_app[x].bonus = 0;
    if(x <= 100 && x >= 45) /* linear from (45,2) to (100,7) */
      wis_app[x].bonus = (byte)((((float)1/11) * (float)x) - ((float)23/11));
  }
}

const char *default_prompts[][2] = {
  { "Basic",
    "&0%hhp %vmv>&0 " },
  { "Colorized Basic",
    "&1&b%h&0&1hp &2&b%v&0&2mv&0> " },
  { "Basic Percentages",
    "&1&b%ph&0&1hp &2&b%pv&0&2mv&0> " },
  { "Full-Featured",
    "&6Opponent&0: &4&b%o &7&b/ &0&6Tank&0: &4&b%t%_&0&1%h&0(&1&b%H&0)"
    "hitp &2%v&0(&2&b%V&0)&7move&0> " },
  { "Standard",
    "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b>%_"
    "&0<%t&0>:<&0%o&0> " },
  { "Complete w/ Spells",
    "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
    "<&0&2%aA&2&b> <&0%l&2&b>%_&0<%t&0>:<&0%o&0> " },
  { "Complete w/ Exp",
    "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
    "<&0&2%aA&2&b> <&0%e&2&b>%_&0<%t&0>:<&0%o&0> " },
  { "Complete w/ Hide Pts",
    "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
    "<&0&2%aA&2&b> <&0&2%ih&2&b>%_&0<%t&0>:<&0%o&0> " },
  { "Complete w/ Rage",
    "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
    "<&0&2%aA&2&b> <&0&2%rr&2&b>%_&0<%t&0>:<&0%o&0> " },
  { "Complete w/ 1st Aid",
    "&2&b<&0&2%hh&0(&2&b%HH&0) &2%vv&0(&2&b%VV&0)&2&b> "
    "<&0&2%aA&2&b> <&0&2%df&2&b>%_&0<%t&0>:<&0%o&0> " },
  { NULL, NULL }
};

/*
 * Only access this array directly if you are listing them all out.
 * Otherwise use sprint_log_severity and parse_log_severity to
 * convert a value to a string and vice-versa.
 */
const char *log_severities[8] = {
  "fine",        /* 10 */
  "informative", /* 20 */
  "debug",       /* 30 */
  "status",      /* 40 */
  "warning",     /* 50 */
  "error",       /* 60 */
  "critical",    /* 70 */
  "\n"
};

/***************************************************************************
 * $Log: constants.c,v $
 * Revision 1.148  2010/06/05 18:35:47  mud
 * Make pyre auto-target caster if sacrificial preference is
 * toggled on.
 *
 * Revision 1.147  2009/07/17 00:48:17  myc
 * Added anon toggle and auto gain privileges.
 *
 * Revision 1.146  2009/06/09 05:35:54  myc
 * Renamed the !CTELL toggle to !CLANCOMM to cover clan communication
 * beyond ctell, like notifications.
 *
 * Revision 1.145  2009/03/21 19:11:37  myc
 * Add cooldown bar to prompt.
 *
 * Revision 1.144  2009/03/20 23:02:59  myc
 * Remove text editor connection state.
 *
 * Revision 1.143  2009/03/15 07:09:24  jps
 * Add !FALL flag for objects
 *
 * Revision 1.142  2009/03/09 05:44:20  jps
 * Moved coin info to money.c
 *
 * Revision 1.141  2009/03/09 05:09:22  jps
 * Moved effect flags and strings into effects.h and effects.c.
 *
 * Revision 1.140  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.139  2009/03/09 03:45:17  jps
 * Extract some spell-mem related stuff from structs.h and put it in spell_mem.h
 *
 * Revision 1.138  2009/03/07 22:27:50  jps
 * Add effect flag remote_aggr, which keeps your aggressive action from
 * removing things like invis. Useful for those spells that keep on hurting.
 *
 * Revision 1.137  2009/01/19 09:27:47  myc
 * Replaced MOB_PET flag with MOB_SUMMONED_MOUNT flag, which
 * tracks a mount summoned using the skill, so proper cooldowns
 * can be set.
 *
 * Revision 1.136  2008/10/02 05:43:58  jps
 * Removing - from all string representations of flags. That messes with
 * scripting, since the - is interpreted as an operator, not part of a string.
 *
 * Revision 1.135  2008/09/29 03:23:31  jps
 * Change weight capacity to 300 at 72, 786 at 100.
 *
 * Revision 1.134  2008/09/07 20:07:15  jps
 * Added flag PLR_GOTSTARS which means you have achieved ** at least once.
 *
 * Revision 1.133  2008/09/07 06:47:48  jps
 * Raised character carry capacity by 1/6, now that the worn equipment
 * continues to weigh something.
 *
 * Revision 1.132  2008/09/07 01:30:25  jps
 * Add a flag for player saving, so that effect changes in the midst of it
 * can be ignored.
 *
 * Revision 1.131  2008/09/03 17:34:08  myc
 * Moved liquid information into a def struct array.
 *
 * Revision 1.130  2008/09/02 07:16:00  mud
 * Changing object TIMER uses into DECOMP where appropriate
 *
 * Revision 1.129  2008/09/02 06:47:51  jps
 * New object flags: permanent and decomposing.
 *
 * Revision 1.128  2008/09/01 18:29:38  jps
 * consolidating cooldown code in skills.c/h
 *
 * Revision 1.127  2008/08/31 20:55:40  jps
 * Added PROTECTOR and PEACEKEEPER mob flags.
 *
 * Revision 1.126  2008/08/30 20:21:39  jps
 * Added flag MOB_NO_EQ_RESTRICT, which allows a mobile to wear equipment
 * without regard to align, class, or level restrictions.
 *
 * Revision 1.125  2008/08/30 18:20:53  myc
 * Removed UNIQUE item flag.
 *
 * Revision 1.124  2008/08/29 04:16:26  myc
 * Added toggles for stacking objects and stacking mobiles.
 * Removed global fullness description array.
 *
 * Revision 1.123  2008/08/14 23:02:11  myc
 * Added an array of graduated log severity names.
 *
 * Revision 1.122  2008/07/27 05:24:35  jps
 * Renamed player crash flag to autosave. Added "removing" flag for when you're
 * being removed from the game.
 *
 * Revision 1.121  2008/07/22 07:25:26  myc
 * Added iedit connection mode and unique flag for objects.
 *
 * Revision 1.120  2008/06/21 16:19:30  jps
 * Change string of sexless char from 'Neutral' to 'Neuter'.
 *
 * Revision 1.119  2008/06/19 18:53:12  myc
 * Replaced the item_types and item_type_desc arrays with a
 * struct array in objects.c
 *
 * Revision 1.118  2008/06/05 02:07:43  myc
 * Added a list of mappings from wear positions to item wear flags.
 *
 * Revision 1.117  2008/05/19 06:52:18  jps
 * Got rid of directions fup and fdown, whatever they were.
 *
 * Revision 1.116  2008/05/19 05:46:20  jps
 * Add effect for being mesmerized.
 *
 * Revision 1.115  2008/05/18 17:58:35  jps
 * Adding effect of familiarity.
 *
 * Revision 1.114  2008/05/18 02:01:06  jps
 * Moved some room-related constants into rooms.c and rooms.h.
 *
 * Revision 1.113  2008/04/14 05:11:40  jps
 * Renamed EFF_FLYING to EFF_FLY, since it only indicates an ability
 * to fly - not that the characer is actually flying.
 *
 * Revision 1.112  2008/04/14 02:18:14  jps
 * Adding string for GLORY effect.
 *
 * Revision 1.111  2008/04/13 18:29:40  jps
 * Add string for effect of confusion.
 *
 * Revision 1.110  2008/04/13 11:08:58  jps
 * Mark waterform flag UNUSED.
 *
 * Revision 1.109  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.108  2008/04/05 18:07:09  myc
 * Re-implementing stealth for hide points.
 *
 * Revision 1.107  2008/04/04 06:12:52  myc
 * Removed dieties/worship code.
 *
 * Revision 1.106  2008/04/02 05:36:19  myc
 * Added the autoloot and autosplit toggles and removed the noname one.
 *
 * Revision 1.105  2008/04/02 04:55:59  myc
 * Added coin names array.
 *
 * Revision 1.104  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.103  2008/03/27 17:27:41  jps
 * ITEM_BLESS is unused now, and AFF3_BLESS and AFF3_HEX are here.
 *
 * Revision 1.102  2008/03/26 23:11:02  jps
 * Retire the WATERFORM and VAPORFORM effects.
 *
 * Revision 1.101  2008/03/26 18:11:23  jps
 * Added a BLESS effect so that characters may be blessed.
 *
 * Revision 1.100  2008/03/23 00:22:44  jps
 * Add string for COMPOSITIO apply.
 *
 * Revision 1.99  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.98  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.97  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.96  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.95  2008/03/10 18:01:17  myc
 * Added a lookup list for posture types, and added a couple more
 * default prompts.
 *
 * Revision 1.94  2008/03/09 18:12:26  jps
 * Added strings for the two misdirection flags.
 *
 * Revision 1.93  2008/03/08 23:54:22  jps
 * Added MOB2_NOSCRIPT flag, which prevents specprocs and triggers.
 *
 * Revision 1.92  2008/03/08 22:29:06  myc
 * Cooldowns are now listed on stat.
 *
 * Revision 1.91  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.90  2008/03/05 03:03:54  myc
 * Moved default prompts here from the do_display command.
 *
 * Revision 1.89  2008/02/24 17:31:13  myc
 * New OLCComm and NoClanTell PRF flags.
 *
 * Revision 1.88  2008/02/09 06:19:44  jps
 * Add "nohints" toggle for whether you receive command suggestions
 * after entering a typo.
 *
 * Revision 1.87  2008/02/07 01:46:14  myc
 * Made the sizes all lowercase, and added two more big sizes (which
 * are unavailable in the game), just to help buffer undefined values.
 *
 * Revision 1.86  2008/02/06 21:53:53  myc
 * Adding an apply_abbrevs array.
 *
 * Revision 1.85  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.84  2008/01/27 21:09:12  myc
 * Adding berserk, spirit of the wolf, spirit of the bear, and
 * interminable wrath affection flags.
 *
 * Revision 1.83  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.82  2008/01/27 11:14:42  jps
 * Move newbie eq to class.c.
 *
 * Revision 1.81  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.80  2008/01/20 23:18:52  myc
 * Fixed mob AI to only leave out class actions.
 *
 * Revision 1.79  2008/01/20 22:58:39  myc
 * Added fup and fdown to the direction arrays so we don't get any bad
 * memory reads.  Made the preference bits more descriptive.  Added
 * some new drinks.  Replaced the integer constants in rev_dir with
 * direction defines.
 *
 * Revision 1.78  2008/01/17 01:29:10  myc
 * Added a list of single word references for wear positions.
 *
 * Revision 1.77  2008/01/13 03:19:53  myc
 * Added !AI flag.
 *
 * Revision 1.76  2008/01/11 17:42:20  myc
 * Adding constants to connected_types to match the number of CON_* defines
 * in structs.h.
 *
 * Revision 1.75  2008/01/09 04:13:44  jps
 * New player flag, MEMMING.
 *
 * Revision 1.74  2008/01/07 10:37:42  jps
 * Identify mob illusionists when they occur. Add player phantasm flag.
 *
 * Revision 1.73  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.72  2008/01/06 05:33:54  jps
 * Rename !mage and !thief to !sorcerer and !rogue.
 *
 * Revision 1.71  2007/12/20 23:12:29  myc
 * Shortened a couple of the carry descs.
 *
 * Revision 1.70  2007/12/19 20:46:43  myc
 * Renamed the Cloaked toggle to RoomVis.
 *
 * Revision 1.69  2007/11/21 00:58:41  jps
 * Increase carrying capacity again - it really got knocked down too far
 * last time. By me.
 *
 * Revision 1.68  2007/11/12 19:00:35  jps
 * *** empty log message ***
 *
 * Revision 1.67  2007/10/13 20:13:09  myc
 * ITEM_NOLOCATE now prevents items from being found using the
 * locate object spell.
 *
 * Revision 1.66  2007/10/11 20:14:48  myc
 * Took out the song/spell_wear_off_msg arrays.  Spell wearoff messages
 * are now defined in the spello call in spell_parser.c.
 *
 * Revision 1.65  2007/10/04 16:20:24  myc
 * Transient item flag now makes things decay when they are on the
 * ground.  Moved portal decay messages to limits.c.
 *
 * Revision 1.64  2007/10/02 02:52:27  myc
 * Removed HIDE and HIDDEN flags.
 *
 * Revision 1.63  2007/09/28 20:49:35  myc
 * Added a "\n" to the mclass_types, genders, and sizes lists so we can use
 * them with search_block().  Removed spaces from FIRE WEAPON and LIQ
 * CONTAINER so we can use them with vsearch.
 *
 * Revision 1.62  2007/09/21 08:44:45  jps
 * Added object type "touchstone" and command "touch" so you can set
 * your home room by touching specific objects.
 *
 * Revision 1.61  2007/09/20 21:20:43  myc
 * Replaced NOBROADCAST with CLOAKED, which lets you appear visible to
 * people in a room, but not everyone else.  Added perception and hide
 * points.
 *
 * Revision 1.60  2007/09/15 15:36:48  myc
 * Natures embrace now sets camouflage bit, which lets you be hidden as long
 * as you are outside.
 *
 * Revision 1.59  2007/09/15 05:37:15  myc
 * Adding new liquids.
 *
 * Revision 1.58  2007/09/15 05:03:46  myc
 * Removing the spell_apply_types array since it duplicated affect_bits.
 * Added action_bits2 for use with MOB2 flags.  Made lemonade yellow
 * instead of red.
 *
 * Revision 1.57  2007/09/11 16:34:24  myc
 * Moved the breath messages out of constants and closer to the breathe
 * command in act.offensive.c.  Added MOB_AQUATIC flag, replacing
 *
 * MOB_NOGEAR.
 *
 * Revision 1.56  2007/09/08 22:05:03  jps
 * Fix typo in chant wear-off mesasge
 *
 * Revision 1.55  2007/09/07 19:41:01  jps
 * Added descriptive names for the object types, for use when someone
 * is trying to identify an object but only has the vaguest clue.
 *
 * Revision 1.54  2007/09/04 06:49:19  myc
 * Moved climate and hemisphere names to data arrays in weather.c.
 *
 * Revision 1.53  2007/09/02 23:03:03  jps
 * Change the fly spell wear-off message so that it doesn't sound
 * silly to someone who is sleeping, resting or otherwise already
 * on the ground.
 *
 * Revision 1.52  2007/09/01 22:52:14  jps
 * Colorized circle of fire die-out message.
 *
 * Revision 1.51  2007/08/30 10:14:52  jps
 * Changed max weight calculation so that it doesn't have a huge
 * leap from str 72-73, and made the max cap (str 100) 698.
 *
 * Revision 1.50  2007/08/23 00:32:13  jps
 * Add !AIR and !EARTH flags, for elemental immunities.
 *
 * Revision 1.49  2007/08/14 22:43:07  myc
 * Added AFF3_SHADOWING flag for use by shadow skill.
 *
 * Revision 1.48  2007/08/04 21:34:40  jps
 * Add a list of prepositional phrases for directions.
 *
 * Revision 1.47  2007/08/04 14:40:35  myc
 * Preference bit array was incomplete.  Added MOB_PEACEFUL flag to prevent
 * players from attacking certain mobs.
 *
 * Revision 1.46  2007/08/04 01:09:57  jps
 * Add moonbeam to spell_wear_off_msg.
 *
 * Revision 1.45  2007/08/03 22:00:11  myc
 * Added PK observatories that work adjacent to arena rooms.
 *
 * Revision 1.44  2007/08/03 03:51:44  myc
 * check_pk is now attack_ok, and covers many more cases than before,
 * including peaced rooms, shapeshifted pk, and arena rooms.  Almost all
 * offensive attacks now use attack_ok to determine whether an attack is
 * allowed.
 *
 * Revision 1.43  2007/07/31 23:02:42  jps
 * Add lists of hemisphere and climate name strings.
 *
 * Revision 1.42  2007/06/28 00:10:10  jps
 * Rename BFS_MARK flag to '*BFS_MARK*' so it isn't so damn
 * mysterious when it shows up.
 *
 * Revision 1.41  2007/06/16 00:15:49  myc
 * Three spells for necromancers: soul tap, rebuke undead,
 * and degeneration.  One spell for rangers: natures guidance.
 *
 * Revision 1.40  2007/05/27 17:36:28  jps
 * Fix typo when innate str wears off.
 *
 * Revision 1.39  2007/05/11 20:13:28  myc
 * Vaporform is a new circle 13 spell for cryomancers.  It significantly
 * increases the caster's chance of dodging a hit.  It is a quest spell.
 *
 * Revision 1.38  2007/03/27 04:27:05  myc
 * New size, colossal.
 *
 * Revision 1.37  2007/02/14 03:54:53  myc
 * Replaced firewalk and greater firewalk with combust and cremate.
 *
 * Revision 1.36  2006/11/18 21:00:28  jps
 * Reworked disarm skill and disarmed-weapon retrieval.
 *
 * Revision 1.35  2006/11/18 19:23:37  jps
 * Fix typo rememberance -> remembrance
 *
 * Revision 1.34  2006/11/18 05:09:27  jps
 * Make sure old continual_light items don't print "UNUSED" when identified.
 *
 * Revision 1.33  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.32  2006/11/17 23:09:57  jps
 * Fix reversed strings that identified obj extra flags for
 * continual light and disarmed.
 *
 * Revision 1.31  2006/11/17 22:52:59  jps
 * Change AGGR_GOOD/EVIL_ALIGN to AGGR_GOOD/EVIL_RACE
 *
 * Revision 1.30  2006/11/08 08:55:18  jps
 * Fix missing period and formatting in barkskin wear-off message.
 *
 * Revision 1.29  2006/11/08 08:50:28  jps
 * Fix typo 'existance' -> 'existence'.
 *
 * Revision 1.28  2006/11/07 16:53:50  jps
 * Movement cost in WATER_NOSWIM changed to 2.
 *
 * Revision 1.27  2006/07/28 06:01:27  cjd
 * added rum to the liquids list and adjusted water so
 * that it no longer adds to the hunger of a person.
 *
 * Revision 1.26  2006/07/20 07:37:01  cjd
 * Typo fixes.
 *
 * Revision 1.25  2005/06/27 14:40:31  cjd
 * added mielikki map to newbie eq
 *
 * Revision 1.24  2002/10/14 02:16:08  jjl
 * An update to turn vitality into a set of 6 spells, lesser endurance,
 * endurance, greater endurance, vitality, greater vitality, and dragon's
 * health.  Greater endurance is what vitality was.  The rest are scaled
 * appropriately.    The higher end may need scaled down, or may not.
 *
 * Revision 1.23  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.22  2002/07/16 23:22:21  rls
 * added in new necro spell bone armor
 *
 * Revision 1.21  2001/04/08 17:13:10  dce
 * Added an alwayslit flag that makes a room lit no matter
 * of the sector or room type...
 *
 * Revision 1.20  2000/11/28 01:32:46  mtp
 * remove mobprog code
 *
 * Revision 1.19  2000/11/22 01:09:13  mtp
 * added motere mob classes (all the ones that are available for players)
 *
 * Revision 1.18  2000/11/21 00:38:34  rsd
 * Altered the comment header and added back rlog messages
 * from prior to the $log$ string being added.
 *
 * Revision 1.17  1999/12/10 05:12:42  cso
 * added const char *sizes[], starting line 373
 *
 * Revision 1.16  1999/11/28 22:58:55  cso
 * added ANIMATED to action_bits
 * added ANIMATE to affected_bits3
 *
 * Revision 1.15  1999/11/26 05:30:21  cso
 * Changed some vnumbers in the struct of newbie gear in do_newbie.
 * that's it, honest.
 *
 * Revision 1.14  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.13  1999/07/06 19:57:05  jimmy
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
 * Revision 1.12  1999/06/30 18:11:09  jimmy
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
 * Revision 1.11  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.10  1999/04/07 01:20:18  dce
 * Allows extra descriptions on no exits.
 *
 * Revision 1.9  1999/03/14 14:28:11  jimmy
 * Movement now has bite!  removed extra "flying" from
 * movement_loss in constants.c to fix the mv bug.  reduced the
 * movement gain by 5 for all ages in limits.c.  Removed the +5
 * and +6 static movement gain so that it now actually updates
 * based on the function in regen.c.  Gosh i'm a bastard.
 * Fingon
 *
 * Revision 1.8  1999/03/08 04:47:16  dce
 * Chant semantics added.
 *
 * Revision 1.7  1999/03/06 23:51:54  dce
 * Add's chant songs, and can only chant once every four hours
 *
 * Revision 1.6  1999/03/05 20:02:36  dce
 * Chant added to, and songs craeted
 *
 * Revision 1.5  1999/03/02 20:14:36  mud
 * Changed some of the weight semantics listings.
 *
 * Revision 1.4  1999/03/02 05:22:13  mud
 * changed the months and days
 * also added object 1154 to the mage eq load
 *
 * Revision 1.3  1999/02/16 09:12:34  jimmy
 * Added spellbooks (1029) to mages newbie eq
 * fingon
 *
 * Revision 1.2  1999/02/13 19:37:12  dce
 * Rewrote Continual Light and Darkness to be manual spells to meet our needs.
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

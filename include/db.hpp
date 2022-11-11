/***************************************************************************
 *   File: db.h                                           Part of FieryMUD *
 *  Usage: header file for database handling                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "quest.hpp"
#include "rooms.hpp"
#include "specprocs.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "zone.hpp"

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD 0
#define DB_BOOT_MOB 1
#define DB_BOOT_OBJ 2
#define DB_BOOT_ZON 3
#define DB_BOOT_SHP 4
#define DB_BOOT_HLP 5
#define DB_BOOT_TRG 6

/* names of various files and directories */
static const char *INDEX_FILE = "index";              /* index of world files		*/
static const char *MINDEX_FILE = "index.mini";        /* ... and for mini-mud-mode	*/
static const char *WLD_PREFIX = "world/wld";          /* room definitions		*/
static const char *MOB_PREFIX = "world/mob";          /* monster prototypes		*/
static const char *OBJ_PREFIX = "world/obj";          /* object prototypes		*/
static const char *ZON_PREFIX = "world/zon";          /* zon defs & command tables	*/
static const char *SHP_PREFIX = "world/shp";          /* shop definitions		*/
static const char *MOB_DIR = "world/prg";             /* Mob programs			*/
static const char *TRG_PREFIX = "world/trg";          /* for script triggers          */
static const char *HLP_PREFIX = "text/help";          /* for HELP <keyword>		*/
static const char *PLR_PREFIX = "players";            /* player files directory       */
static const char *CLAN_PREFIX = "etc/clans";         /* clan directory		*/
static const char *CLAN_PREFIX_OLD = "etc/clans.old"; /* clan directory		*/

static const char *PLR_SUFFIX = ".plr";       /* player file suffix		*/
static const char *POBJ_SUFFIX = ".objs";     /* player object file suffix	*/
static const char *PNOTES_SUFFIX = ".notes";  /* player notes file suffix	*/
static const char *PQUEST_SUFFIX = ".quest";  /* player quest file suffix	*/
static const char *CLAN_SUFFIX = ".clan";     /* clan file suffix		*/
static const char *PTEMP_SUFFIX = ".temp";    /* temporary file suffix	*/
static const char *CORPSE_SUFFIX = ".corpse"; /* player corpse file suffix	*/
static const char *PET_SUFFIX = ".pet";       /* Players pet file suffix */

static const char *HELP_FILE = "text/help/help.hlp"; /* unified help file       */

static const char *ALL_QUEST_FILE = "misc/quests"; /*list of all available quests  */
static const char *IDEA_FILE = "misc/ideas";       /* for the 'idea'-command	*/
static const char *TYPO_FILE = "misc/typos";       /*         'typo'		*/
static const char *BUG_FILE = "misc/bugs";         /*         'bug'		*/
static const char *MESS_FILE = "misc/messages";    /* damage messages		*/
static const char *SOCMESS_FILE = "misc/socials";  /* messgs for social acts	*/
static const char *XNAME_FILE = "misc/xnames";     /* invalid name substrings	*/

static const char *MAIL_FILE = "etc/plrmail";           /* for the mudmail system	*/
static const char *BAN_FILE = "etc/badsites";           /* for the siteban system	*/
static const char *HCONTROL_FILE = "etc/hcontrol";      /* for the house system		*/
static const char *CCONTROL_FILE = "etc/ccontrol";      /* for the corpse save system   */
static const char *CLAN_INDEX_FILE = "etc/clans/index"; /* list of clans	*/
static const char *MAKE_COUNT = "../log/make_count";    /* for build counter        */
static const char *GROUP_FILE = "etc/cmdgroups";        /* for cmd group grant system	*/

static const char *INFODUMP_PREFIX = "infodump";

/* public procedures in db.c */
void boot_db(void);
void destroy_db(void);
void free_text_files(void);
void zone_update(void);
int real_room(int vnum);
int real_quest(unsigned short vnum);
char *fread_string(FILE *fl, char *error);
int vnum_room(char *searchname, CharData *ch);
int vnum_zone(char *searchname, CharData *ch);

void init_player(CharData *ch);
CharData *create_char(void);
CharData *read_mobile(int nr, int type);
int real_mobile(int vnum);
int vnum_mobile(char *searchname, CharData *ch);
void clear_char(CharData *ch);
void reset_char(CharData *ch);
void free_char(CharData *ch);

ObjData *create_obj(void);
void clear_object(ObjData *obj);
void free_obj(ObjData *obj);
int real_object(int vnum);
ObjData *read_object(int nr, int type);
int vnum_object(char *searchname, CharData *ch);
bool _parse_name(char *arg, char *name);
void start_player(CharData *ch);

long asciiflag_conv(char *flag);
void tag_argument(char *argument, char *tag);
#define TAG_IS(tagname) (!strcmp(tag, tagname))
void kill_ems(char *str);
void copy_extra_descriptions(ExtraDescriptionData **to, ExtraDescriptionData *from);

#define MAX_VNUM 99999
#define REAL 0
#define VIRTUAL 1

struct PlayerIndexElement {
    char *name;
    long id;
    int level;
    int flags;
    time_t last;
};

struct HelpIndexElement {
    char *keyword;
    char *entry;
    int min_level;
    int duplicate;
};

/* don't change these */
#define BAN_NOT 0
#define BAN_NEW 1
#define BAN_SELECT 2
#define BAN_ALL 3

#define BANNED_SITE_LENGTH 50
struct BanListElement {
    char site[BANNED_SITE_LENGTH + 1];
    int type;
    time_t date;
    char name[MAX_NAME_LENGTH + 1];
    BanListElement *next;
};

RoomData *world = nullptr;                  /* array of rooms                 */
int top_of_world = 0;                       /* ref to top element of world         */
RoomEffectNode *room_effect_list = nullptr; /* list of room effects */

CharData *character_list = nullptr; /* global linked list of chars */

IndexData **trig_index; /* index table for triggers      */
int top_of_trigt = 0;   /* top of trigger index table    */
long max_id = 100000;   /* for unique mob/obj id's       */

IndexData *mob_index;                       /* index table for mobile file         */
CharData *mob_proto;                        /* prototypes for mobs                 */
ObjData *obj_proto;                         /* prototypes for objs                 */
int top_of_mobt = 0;                        /* top of mobile index table         */
ObjData *object_list = nullptr;             /* global linked list of objs         */
IndexData *obj_index;                       /* index table for object file         */
                                            /* prototypes for objs                 */
int top_of_objt = 0;                        /* top of object index table         */
SpellDamage spell_dam_info[MAX_SPELLS + 1]; /*internal spell dam */
ZoneData *zone_table;                       /* zone table                         */
int top_of_zone_table = 0;                  /* top element of zone tab         */
message_list fight_messages[MAX_MESSAGES];  /* fighting messages */

PlayerIndexElement *player_table = nullptr; /* index to plr file */
int top_of_p_table = 0;                     /* ref to top of table                 */
int top_of_p_file = 0;                      /* ref of size of p file         */
long top_idnum = 0;                         /* highest idnum in use                 */

HelpIndexElement *help_table = 0; /* the help table         */
int top_of_helpt = 0;             /* top of help index table         */

// ObjData *obj_proto;
// int top_of_objt;
// IndexData *mob_index;
// CharData *mob_proto;
// int top_of_mobt;
// ZoneData *zone_table;
// int top_of_zone_table;
// IndexData **trig_index;
// int top_of_trigt;
// long max_id;

// int top_of_helpt;
// HelpIndexElement *help_table;

TimeInfoData time_info;

str_app_type str_app[101];
dex_skill_type dex_app_skill[101];
dex_app_type dex_app[101];
con_app_type con_app[101];
int_app_type int_app[101];
wis_app_type wis_app[101];
cha_app_type cha_app[101];

// #endif

/* global buffering system */

char buf[MAX_STRING_LENGTH];
char buf1[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
char arg[MAX_STRING_LENGTH];

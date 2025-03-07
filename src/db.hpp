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

#include <string_view>
#include <fstream>
#include <array>
#include <algorithm>

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD 0
#define DB_BOOT_MOB 1
#define DB_BOOT_OBJ 2
#define DB_BOOT_ZON 3
#define DB_BOOT_SHP 4
#define DB_BOOT_HLP 5
#define DB_BOOT_TRG 6

/* names of various files and directories */
constexpr std::string_view INDEX_FILE = "index";              /* index of world files		*/
constexpr std::string_view MINDEX_FILE = "index.mini";        /* ... and for mini-mud-mode	*/
constexpr std::string_view WLD_PREFIX = "world/wld";          /* room definitions		*/
constexpr std::string_view MOB_PREFIX = "world/mob";          /* monster prototypes		*/
constexpr std::string_view OBJ_PREFIX = "world/obj";          /* object prototypes		*/
constexpr std::string_view ZON_PREFIX = "world/zon";          /* zon defs & command tables	*/
constexpr std::string_view SHP_PREFIX = "world/shp";          /* shop definitions		*/
constexpr std::string_view MOB_DIR = "world/prg";             /* Mob programs			*/
constexpr std::string_view TRG_PREFIX = "world/trg";          /* for script triggers          */
constexpr std::string_view HLP_PREFIX = "text/help";          /* for HELP <keyword>		*/
constexpr std::string_view PLR_PREFIX = "players";            /* player files directory       */
constexpr std::string_view CLAN_PREFIX = "etc/clans";         /* clan directory		*/
constexpr std::string_view CLAN_PREFIX_OLD = "etc/clans.old"; /* clan directory		*/

constexpr std::string_view PLR_SUFFIX = ".plr";       /* player file suffix		*/
constexpr std::string_view POBJ_SUFFIX = ".objs";     /* player object file suffix	*/
constexpr std::string_view PNOTES_SUFFIX = ".notes";  /* player notes file suffix	*/
constexpr std::string_view PQUEST_SUFFIX = ".quest";  /* player quest file suffix	*/
constexpr std::string_view CLAN_SUFFIX = ".clan";     /* clan file suffix		*/
constexpr std::string_view PTEMP_SUFFIX = ".temp";    /* temporary file suffix	*/
constexpr std::string_view CORPSE_SUFFIX = ".corpse"; /* player corpse file suffix	*/
constexpr std::string_view PET_SUFFIX = ".pet";       /* Players pet file suffix */

constexpr std::string_view HELP_FILE = "text/help/help.hlp"; /* unified help file       */

constexpr std::string_view ALL_QUEST_FILE = "misc/quests"; /*list of all available quests  */
constexpr std::string_view IDEA_FILE = "misc/ideas";       /* for the 'idea'-command	*/
constexpr std::string_view TYPO_FILE = "misc/typos";       /*         'typo'		*/
constexpr std::string_view BUG_FILE = "misc/bugs";         /*         'bug'		*/
constexpr std::string_view MESS_FILE = "misc/messages";    /* damage messages		*/
constexpr std::string_view SOCMESS_FILE = "misc/socials";  /* messgs for social acts	*/
constexpr std::string_view XNAME_FILE = "misc/xnames";     /* invalid name substrings	*/

constexpr std::string_view MAIL_FILE = "etc/plrmail";           /* for the mudmail system	*/
constexpr std::string_view BAN_FILE = "etc/badsites";           /* for the siteban system	*/
constexpr std::string_view HCONTROL_FILE = "etc/hcontrol";      /* for the house system		*/
constexpr std::string_view CCONTROL_FILE = "etc/ccontrol";      /* for the corpse save system   */
constexpr std::string_view CLAN_INDEX_FILE = "etc/clans/index"; /* list of clans	*/
constexpr std::string_view GROUP_FILE = "etc/cmdgroups";        /* for cmd group grant system	*/

constexpr std::string_view INFODUMP_PREFIX = "infodump";

/* public procedures in db.c */
void boot_db(void);
void destroy_db(void);
void free_text_files(void);
void zone_update(void);
int real_room(int vnum);
int real_quest(unsigned short vnum);
std::string fread_string(std::ifstream &fl, const std::string_view error);
int vnum_room(std::string_view searchname, CharData *ch);
int vnum_zone(std::string_view searchname, CharData *ch);

void init_player(CharData *ch);
CharData *create_char(void);
CharData *read_mobile(int nr, int type);
int real_mobile(int vnum);
int vnum_mobile(std::string_view searchname, CharData *ch);
void clear_char(CharData *ch);
void reset_char(CharData *ch);
void free_char(CharData *ch);

ObjData *create_obj(void);
void clear_object(ObjData *obj);
void free_obj(ObjData *obj);
int real_object(int vnum);
ObjData *read_object(int nr, int type);
int vnum_object(std::string_view searchname, CharData *ch);
[[nodiscard]] std::string _parse_name(std::string_view arg);
void start_player(CharData *ch);

long asciiflag_conv(std::string_view flag);
void tag_argument(std::string_view argument, std::string_view tag);
#define TAG_IS(tagname) (matches(tag, tagname))
void copy_extra_descriptions(ExtraDescriptionData **to, ExtraDescriptionData *from);

#define MAX_VNUM 99999
#define REAL 0
#define VIRTUAL 1

struct PlayerIndexElement {
    std::string name;
    long id;
    int level;
    int flags;
    time_t last;
};

struct HelpIndexElement {
    std::string_view keyword;
    std::string_view entry;
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
    std::string site;
    int type;
    time_t date;
    std::string name;
};

// Global Variables

extern PlayerIndexElement *player_table;
extern int top_of_p_table;
extern int top_of_p_file;
extern long top_idnum;

extern RoomData *world; /* array of rooms */
extern int top_of_world;
extern RoomEffectNode *room_effect_list;
extern CharData *character_list;
extern ObjData *object_list;

extern IndexData *obj_index;
extern ObjData *obj_proto;
extern int top_of_objt;
extern IndexData *mob_index;
extern CharData *mob_proto;
extern int top_of_mobt;
extern ZoneData *zone_table;
extern int top_of_zone_table;
extern IndexData **trig_index;
extern int top_of_trigt;
extern long max_id;

extern int top_of_helpt;
extern HelpIndexElement *help_table;

extern QuestInfo *all_quests;

extern TimeInfoData time_info;

extern stat_bonus_type stat_bonus[101];

extern message_list fight_messages[MAX_MESSAGES];
extern SpellDamage spell_dam_info[MAX_SPELLS + 1];

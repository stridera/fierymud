/***************************************************************************
 * $Id: db.h,v 1.25 2009/03/20 23:02:59 myc Exp $
 ***************************************************************************/
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

#ifndef __FIERY_DB_H
#define __FIERY_DB_H

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_SHP	4
#define DB_BOOT_HLP	5
#define DB_BOOT_TRG	6

/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files		*/
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode	*/
#define WLD_PREFIX	"world/wld"	/* room definitions		*/
#define MOB_PREFIX	"world/mob"	/* monster prototypes		*/
#define OBJ_PREFIX	"world/obj"	/* object prototypes		*/
#define ZON_PREFIX	"world/zon"	/* zon defs & command tables	*/
#define SHP_PREFIX	"world/shp"	/* shop definitions		*/
#define MOB_DIR		"world/prg"	/* Mob programs			*/
#define TRG_PREFIX	"world/trg"	/* for script triggers          */
#define HLP_PREFIX	"text/help"	/* for HELP <keyword>		*/
#define PLR_PREFIX      "players"       /* player files directory       */
#define CLAN_PREFIX	"etc/clans"	/* clan directory		*/

#define PLR_SUFFIX	".plr"		/* player file suffix		*/
#define POBJ_SUFFIX	".objs"		/* player object file suffix	*/
#define PNOTES_SUFFIX	".notes"	/* player notes file suffix	*/
#define PQUEST_SUFFIX	".quest"	/* player quest file suffix	*/
#define CLAN_SUFFIX	".clan"		/* clan file suffix		*/
#define PTEMP_SUFFIX	".temp"		/* temporary file suffix	*/
#define CORPSE_SUFFIX	".corpse"	/* player corpse file suffix	*/

#define HELP_FILE	"text/help/help.hlp" /* unified help file       */

#define ALL_QUEST_FILE	"misc/quests"   /*list of all available quests  */
#define IDEA_FILE	"misc/ideas"	/* for the 'idea'-command	*/
#define TYPO_FILE	"misc/typos"	/*         'typo'		*/
#define BUG_FILE	"misc/bugs"	/*         'bug'		*/
#define MESS_FILE	"misc/messages"	/* damage messages		*/
#define SOCMESS_FILE	"misc/socials"	/* messgs for social acts	*/
#define XNAME_FILE	"misc/xnames"	/* invalid name substrings	*/

#define MAIL_FILE	"etc/plrmail"	/* for the mudmail system	*/
#define BAN_FILE	"etc/badsites"	/* for the siteban system	*/
#define HCONTROL_FILE	"etc/hcontrol"  /* for the house system		*/
#define CCONTROL_FILE   "etc/ccontrol"  /* for the corpse save system   */
#define CLAN_INDEX_FILE	"etc/clans/index"	/* list of clans	*/
#define MAKE_COUNT	"../log/make_count" /* for build counter        */
#define GROUP_FILE	"etc/cmdgroups"	/* for cmd group grant system	*/

#define INFODUMP_PREFIX "infodump"

/* public procedures in db.c */
void	boot_db(void);
void	destroy_db(void);
void	free_text_files(void);
void	zone_update(void);
int	real_room(int virtual);
int	real_quest(unsigned short virtual);
char	*fread_string(FILE *fl, char *error);
int	vnum_room(char *searchname, struct char_data *ch);
int	vnum_zone(char *searchname, struct char_data *ch);

void	init_player(struct char_data *ch);
struct char_data* create_char(void);
struct char_data *read_mobile(int nr, int type);
int	real_mobile(int virtual);
int	vnum_mobile(char *searchname, struct char_data *ch);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);

struct obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
int	real_object(int virtual);
struct obj_data *read_object(int nr, int type);
int	vnum_object(char *searchname, struct char_data *ch);
bool _parse_name(char *arg, char *name);
void start_player(struct char_data * ch);

long    asciiflag_conv(char *flag);
void    tag_argument(char *argument, char *tag);
#define TAG_IS(tagname)	(!strcmp(tag, tagname))
void    kill_ems(char *str);
void	copy_extra_descriptions(struct extra_descr_data **to, struct extra_descr_data *from);

#define MAX_VNUM 99999
#define REAL 0
#define VIRTUAL 1

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   /*int  arg4;*/
   char *sarg;		/*string command oneday be alot*/
   int line;		/* line number this command appears on  */

   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
   */
};


/* zone definition structure. for the 'zone-table'   */
struct zone_data {
   char	*name;		    /* name of this zone                  */
   int	lifespan;           /* how long between resets (minutes)  */
   int	age;                /* current age of this zone (minutes) */
   int	top;                /* upper limit for rooms in this zone */
   int  zone_factor;
   int	reset_mode;         /* conditions for reset (see below)   */
   int	number;		    /* virtual number of this zone	  */
   
   /* weather information */
   int  hemisphere;
   int  temperature;
   int  precipitation;
   int  climate;
   int  wind_speed;
   int  wind_dir;

   /* if disaster_type = 0, no disaster in effect.  Otherwise, the 
    * value is the disaster type in effect 
    */
   int disaster_type;            
   int disaster_duration;

   struct reset_com *cmd;   /* command table for reset	          */

   /*
	*  Reset mode:                              *
	*  0: Don't reset, and don't update age.    *
	*  1: Reset if no PC's are located in zone. *
	*  2: Just reset.                           *
   */
};

/* for queueing zones for update   */
struct reset_q_element {
   int	zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};

/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};

struct player_index_element {
   char	*name;
   long id;
   int level;
   int flags;
   time_t last;
};


struct help_index_element {
   char	*keyword;
   char *entry;
   int min_level;
   int duplicate;
};


/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};

/* Variables provided in db.c */
#ifndef __DB_C__
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int top_of_p_file;
extern long top_idnum;

extern struct room_data *world; /* array of rooms */
extern int top_of_world;
extern struct room_effect_node *room_effect_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;

extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern int top_of_objt;
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern int top_of_mobt;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct index_data **trig_index;
extern int top_of_trigt;
extern long max_id;

extern int top_of_helpt;
extern struct help_index_element *help_table;

extern struct quest_info *all_quests;

extern struct time_info_data time_info;

extern struct str_app_type str_app[101];
extern struct dex_skill_type dex_app_skill[101];
extern struct dex_app_type dex_app[101];
extern struct con_app_type con_app[101];
extern struct int_app_type int_app[101];
extern struct wis_app_type wis_app[101];

#endif


/* global buffering system */

#ifdef __DB_C__
char	buf[MAX_STRING_LENGTH];
char	buf1[MAX_STRING_LENGTH];
char	buf2[MAX_STRING_LENGTH];
char	arg[MAX_STRING_LENGTH];
#else
extern char	buf[MAX_STRING_LENGTH];
extern char	buf1[MAX_STRING_LENGTH];
extern char	buf2[MAX_STRING_LENGTH];
extern char	arg[MAX_STRING_LENGTH];
#endif

#ifndef __CONFIG_C__
extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;
extern char     *HUH;
#endif

#endif

/***************************************************************************
 * $Log: db.h,v $
 * Revision 1.25  2009/03/20 23:02:59  myc
 * Move text file handling routines into text.c, including
 * the reload command.
 *
 * Revision 1.24  2008/08/10 02:58:40  jps
 * Added infodump command for outputting game data to text files.
 *
 * Revision 1.23  2008/07/22 07:25:26  myc
 * Added copy_extra_descriptions function.
 *
 * Revision 1.22  2008/07/15 17:49:24  myc
 * Boot command groups from file during boot-up sequence.
 *
 * Revision 1.21  2008/06/05 02:07:43  myc
 * Added corpse suffix constant.h
 *
 * Revision 1.20  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.19  2008/03/08 18:12:36  jps
 * Added player file type TEMP.
 *
 * Revision 1.18  2008/03/05 03:03:54  myc
 * Moved many player functions from db.c to players.c.  Also
 * added some SUFFIX constants for player files.
 *
 * Revision 1.17  2008/02/16 20:26:04  myc
 * Adding functions to free spell dams, extra descriptions, mobiles,
 * objects, rooms, zones, triggers, text files, players, and the
 * help table at program termination.
 *
 * Revision 1.16  2008/02/02 19:38:20  myc
 * Moved HUH to config.c.
 *
 * Revision 1.15  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.14  2008/01/26 12:29:26  jps
 * Export world.
 *
 * Revision 1.13  2008/01/05 05:38:37  jps
 * Changed name of save_char() to save_player().
 *
 * Revision 1.12  2007/12/19 20:49:11  myc
 * save_player() no longer requires the save room (which wasn't being used
 * anyway).  Updated clan checking code in store_to_char.  Added
 * tag_argument() function for use with ASCII files.  Currently used by
 * clan code, but could possibly be used for pfiles in the future.  Also
 * added kill_ems().  These two functions ported from v3.  Added clan
 * file defines and TAG_IS macro for use with tag_argument().
 *
 * Revision 1.11  2007/08/24 22:10:01  jps
 * Move #define MAX_VNUM to this file.
 *
 * Revision 1.10  2007/08/24 17:01:36  myc
 * Adding ostat and mstat commands as shorthand for vstat, rstat for stat
 * room, and mnum and onum for vnum.  Also adding rnum and znum with new
 * functionality.
 *
 * Revision 1.9  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.8  2000/11/21 01:12:10  rsd
 * Added a new comment header, fixed some spacing, and added
 * back rlog messages from prior to the addition of the
 * $log$ string.
 *
 * Revision 1.7  2000/11/07 01:40:45  mtp
 * changed type of real_quest arg
 *
 * Revision 1.6  2000/10/31 23:31:34  mtp
 * added ALL_QUEST_FILE alias for quest file name
 *
 * Revision 1.5  2000/10/27 00:34:45  mtp
 * extra info for booting quests and a real_quest fn to find array
 * value given vnum of quest
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/08/12 04:25:39  jimmy
 * This is a Mass ci of the new pfile system.  The pfile has been split into
 * one file for each player in a directory A-Z.  The object files are also
 * located in the A-Z directories.  Fixed a stupid bug in pfilemaint that
 * screwed up the IDNUM of the person who typed it.  Commented out the frag
 * system completely.  It is slated for removal.  Fixed the rename command.
 * Fixed all supporting functions for the new system, I hope!
 * --Gurlaek 8/11/1999
 *
 * Revision 1.2  1999/02/01 08:19:11  jimmy
 * improved build counter 
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

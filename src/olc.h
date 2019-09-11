/***************************************************************************
 * $Id: olc.h,v 1.46 2009/03/09 20:36:00 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: olc.h                                          Part of FieryMUD *
 *  Usage: OASIS OLC -                                                     *
 *     By: Harvey Gilpin of TwyliteMud                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  Copyright 1996 Harvey Gilpin.                                          *
 ***************************************************************************/

/*
 * If you don't want a short explanation of each field in your zone files,
 * change the number below to a 0 instead of a 1.
 */
#if 0
#define ZEDIT_HELP_IN_FILE
#endif

/*
 * If you want to clear the screen before certain Oasis menus, set to 1.
 */
#if 0
#define CLEAR_SCREEN	1
#endif

/*. CONFIG DEFINES - these should be in structs.h, but here is easyer.*/

#define NUM_ATTACK_TYPES	15

#define NUM_SHOP_FLAGS 		2
#define NUM_TRADERS 		7

/*
 * Define this to how many MobProg scripts you have.
 */
#define NUM_PROGS		12


/* medit.c */
bool delete_mobile(obj_num rnum);

/*. Utils exported from olc.c .*/
void strip_string(char *);
void cleanup_olc(struct descriptor_data *d, byte cleanup_type);
void get_char_cols(struct char_data *ch);
void olc_add_to_save_list(int zone, byte type);
void olc_remove_from_save_list(int zone, byte type);
void free_save_list(void);
void free_olc_zone_list(struct char_data *ch);
bool has_olc_access(struct char_data *ch, zone_vnum zone);
int real_zone(int number);

/*. OLC structs .*/

struct olc_data {
  int mode;
  int zone_num;
  int number;
  int value;
  char *storage;
  struct char_data *mob;
  struct room_data *room;
  struct obj_data *obj;
  struct zone_data *zone;
  struct shop_data *shop;
    struct spell_dam *spell;
  struct extra_descr_data *desc;
  struct help_index_element *help;
  struct trig_data *trig;
  int script_mode;
  int trigger_position;
  int item_type;
  struct trig_proto_list *script;
 /* char *storage;*/ /* for holding commands etc.. */
  struct olc_command_group *group;
  struct obj_data *item;
};

struct olc_save_info {
  int zone;
  char type;
  struct olc_save_info *next;
};

/*
 * Exported globals.
 */
#ifdef _OASIS_OLC_
char *nrm, *grn, *cyn, *yel, *blk, *red;
struct olc_save_info *olc_save_list = NULL;
#else
extern char *nrm, *grn, *cyn, *yel, *blk, *red;
extern struct olc_save_info *olc_save_list;
#endif

/*
 * Descriptor access macros.
 */
#define OLC_MODE(d) 	((d)->olc->mode)	/* Parse input mode.	*/
#define OLC_NUM(d) 	((d)->olc->number)	/* Room/Obj VNUM.	*/
#define OLC_VAL(d) 	((d)->olc->value)	/* Scratch variable.	*/
#define OLC_ZNUM(d) 	((d)->olc->zone_num)	/* Real zone number.	*/
#define OLC_ROOM(d) 	((d)->olc->room)	/* Room structure.	*/
#define OLC_OBJ(d) 	((d)->olc->obj)		/* Object structure.	*/
#define OLC_IOBJ(d) 	((d)->olc->item)	/* Iedit object ref.	*/
#define OLC_ZONE(d)     ((d)->olc->zone)	/* Zone structure.	*/
#define OLC_MOB(d)	((d)->olc->mob)		/* Mob structure.	*/
#define OLC_SHOP(d) 	((d)->olc->shop)	/* Shop structure.	*/
#define OLC_DESC(d) 	((d)->olc->desc)	/* Extra description.	*/
#define OLC_STORAGE(d)  ((d)->olc->storage)     /*. For command storage . */
#define OLC_HELP(d)   ((d)->olc->help)  /*. Help structure    . */
#define OLC_SD(d)       ((d)->olc->spell) /*sdedit structure*/
#define OLC_TRIG(d)     ((d)->olc->trig)        /* Trigger structure.   */
#define OLC_STORAGE(d)  ((d)->olc->storage)    /* For command storage  */
#define OLC_GROUP(d)	((d)->olc->group)


/*. Other macros .*/

#define OLC_EXIT(d)	(OLC_ROOM(d)->exits[OLC_VAL(d)])
#define GET_OLC_ZONES(c)	((c)->player_specials->olc_zones)

/*
 * Cleanup types.
 */
#define CLEANUP_ALL		(byte)	1	/* Free the whole lot.	*/
#define CLEANUP_STRUCTS 	(byte)	2	/* Don't free strings.	*/

/*
 * Add/Remove save list types.
 */
#define OLC_SAVE_ROOM		(byte)	0
#define OLC_SAVE_OBJ		(byte)	1
#define OLC_SAVE_ZONE		(byte)	2
#define OLC_SAVE_MOB		(byte)	3
#define OLC_SAVE_SHOP		(byte)	4
#define OLC_SAVE_HELP		(byte)	5

#define SDEDIT_LVL_MULT_MENU 13
#define SDEDIT_MAIN_MENU 1	
#define SDEDIT_CONFIRM_SAVESTRING 0
#define SDEDIT_NPC_NO_FACE_ENTRY  3
#define SDEDIT_NPC_NO_DICE_ENTRY  2
#define SDEDIT_PC_NO_FACE_ENTRY  4
#define SDEDIT_PC_NO_DICE_ENTRY  5
#define SDEDIT_USE_BONUS 6
#define SDEDIT_MAX_BONUS 7
#define SDEDIT_NPC_REDUCE_FACTOR 8
#define SDEDIT_INTERN_DAM 9
#define SDEDIT_NPC_STATIC 10
#define SDEDIT_PC_STATIC 11
#define SDEDIT_NOTE 12

/* Submodes of HEDIT connectedness      */
#define HEDIT_CONFIRM_SAVESTRING	0
#define HEDIT_CONFIRM_EDIT		1
#define HEDIT_CONFIRM_ADD		2
#define HEDIT_MAIN_MENU			3
#define HEDIT_ENTRY			4
#define HEDIT_MIN_LEVEL			5

/* hedit permission */
#define HEDIT_PERMISSION		666

/*
 * Submodes of OEDIT connectedness.
 */
#define OEDIT_MAIN_MENU                  1
#define OEDIT_EDIT_NAMELIST              2
#define OEDIT_SHORTDESC                  3
#define OEDIT_LONGDESC                   4
#define OEDIT_ACTDESC                    5
#define OEDIT_TYPE                       6
#define OEDIT_EXTRAS                     7
#define OEDIT_WEAR                       8
#define OEDIT_WEIGHT                     9
#define OEDIT_COST                      10
#define OEDIT_COSTPERDAY                11
#define OEDIT_TIMER                     12
#define OEDIT_LEVEL                     13
#define OEDIT_VALUE_1                   14
#define OEDIT_VALUE_2                   15
#define OEDIT_VALUE_3                   16
#define OEDIT_VALUE_4                   17
#define OEDIT_APPLY                     18
#define OEDIT_APPLYMOD                  19
#define OEDIT_EXTRADESC_KEY             20
#define OEDIT_CONFIRM_SAVEDB            21
#define OEDIT_CONFIRM_SAVESTRING        22
#define OEDIT_PROMPT_APPLY              23
#define OEDIT_EXTRADESC_DESCRIPTION     24
#define OEDIT_EXTRADESC_MENU            25
#define OEDIT_SPELL_APPLY               26
#define OEDIT_SPELL_COMPONENT           27
#define OEDIT_PURGE_OBJECT              28
#define OEDIT_HIDDENNESS                29


/* Submodes of REDIT connectedness */
#define REDIT_MAIN_MENU 		1
#define REDIT_NAME 			2
#define REDIT_DESC 			3
#define REDIT_FLAGS 			4
#define REDIT_SECTOR 			5
#define REDIT_EXIT_MENU 		6
#define REDIT_CONFIRM_SAVEDB 		7
#define REDIT_CONFIRM_SAVESTRING 	8
#define REDIT_EXIT_NUMBER 		9
#define REDIT_EXIT_DESCRIPTION 		10
#define REDIT_EXIT_KEYWORD 		11
#define REDIT_EXIT_KEY 			12
#define REDIT_EXIT_DOORFLAGS 		13
#define REDIT_EXTRADESC_MENU 		14
#define REDIT_EXTRADESC_KEY 		15
#define REDIT_EXTRADESC_DESCRIPTION 	16


/*. Submodes of ZEDIT connectedness 	.*/
#define ZEDIT_MAIN_MENU              	0
#define ZEDIT_DELETE_ENTRY		1
#define ZEDIT_NEW_ENTRY			2
#define ZEDIT_CHANGE_ENTRY		3
#define ZEDIT_COMMAND_TYPE		4
#define ZEDIT_IF_FLAG			5
#define ZEDIT_ARG1			6
#define ZEDIT_ARG2			7
#define ZEDIT_ARG3			8
#define ZEDIT_ZONE_NAME			9
#define ZEDIT_ZONE_LIFE			10
#define ZEDIT_ZONE_TOP			11
#define ZEDIT_ZONE_RESET		12
#define ZEDIT_CONFIRM_SAVESTRING	13
#define ZEDIT_ZONE_FACTOR		14
#define ZEDIT_SARG			15
#define ZEDIT_ZONE_HEMISPHERE           16
#define ZEDIT_ZONE_CLIMATE              17

/*. Submodes of MEDIT connectedness 	.*/
#define MEDIT_MAIN_MENU              	0
#define MEDIT_ALIAS			1
#define MEDIT_S_DESC			2
#define MEDIT_L_DESC			3
#define MEDIT_D_DESC			4
#define MEDIT_NPC_FLAGS			5
#define MEDIT_AFF_FLAGS			6
#define MEDIT_CONFIRM_SAVESTRING	9
#define MEDIT_PURGE_MOBILE		10
/*. Numerical responses .*/
#define MEDIT_NUMERICAL_RESPONSE	11
#define MEDIT_SEX			12
#define MEDIT_HITROLL			13
#define MEDIT_DAMROLL			14
#define MEDIT_NDD			15
#define MEDIT_SDD			16
#define MEDIT_NUM_HP_DICE		17
#define MEDIT_SIZE_HP_DICE		18
#define MEDIT_ADD_HP			19
#define MEDIT_AC			20
#define MEDIT_EXP			21
#define MEDIT_EX_GOLD			22
#define MEDIT_POS			23
#define MEDIT_DEFAULT_POS		24
#define MEDIT_ATTACK			25
#define MEDIT_LEVEL			26
#define MEDIT_ALIGNMENT			27
#define MEDIT_CLASS                     28
#define MEDIT_RACE                      29
#define MEDIT_EX_PLATINUM               30
#define MEDIT_SIZE                      31
#define MEDIT_PERCEPTION                32
#define MEDIT_HIDDENNESS                33
#define MEDIT_LIFEFORCE                 34
#define MEDIT_COMPOSITION               35
#define MEDIT_STANCE                    36

/*. Submodes of SEDIT connectedness 	.*/
#define SEDIT_MAIN_MENU              	0
#define SEDIT_CONFIRM_SAVESTRING	1
#define SEDIT_NOITEM1			2
#define SEDIT_NOITEM2			3
#define SEDIT_NOCASH1			4
#define SEDIT_NOCASH2			5
#define SEDIT_NOBUY			6
#define SEDIT_BUY			7
#define SEDIT_SELL			8
#define SEDIT_PRODUCTS_MENU		11
#define SEDIT_ROOMS_MENU		12
#define SEDIT_NAMELIST_MENU		13
#define SEDIT_NAMELIST			14
/*. Numerical responses .*/
#define SEDIT_NUMERICAL_RESPONSE	20
#define SEDIT_OPEN1			21
#define SEDIT_OPEN2			22
#define SEDIT_CLOSE1			23
#define SEDIT_CLOSE2			24
#define SEDIT_KEEPER			25
#define SEDIT_BUY_PROFIT		26
#define SEDIT_SELL_PROFIT		27
#define SEDIT_TYPE_MENU			29
#define SEDIT_DELETE_TYPE		30
#define SEDIT_DELETE_PRODUCT		31
#define SEDIT_NEW_PRODUCT		32
#define SEDIT_DELETE_ROOM		33
#define SEDIT_NEW_ROOM			34
#define SEDIT_SHOP_FLAGS		35
#define SEDIT_NOTRADE			36
#define SEDIT_UNIQUE			37

/* Submodes of gedit connectedness */
#define GEDIT_MAIN_MENU			0
#define GEDIT_ALIAS			1
#define GEDIT_NAME			2
#define GEDIT_DESCRIPTION		3
#define GEDIT_ADD_COMMAND		4
#define GEDIT_REMOVE_COMMAND		5
#define GEDIT_CONFIRM_SAVE		6
#define GEDIT_LEVEL			7

/*. Limit info .*/
#define MAX_ROOM_NAME	75
#define MAX_MOB_NAME	50
#define MAX_OBJ_NAME	50
#define MAX_ROOM_DESC	2048
#define MAX_EXIT_DESC	512
#define MAX_EXTRA_DESC  512
#define MAX_MOB_DESC	1024
#define MAX_OBJ_DESC	512

/***************************************************************************
 * $Log: olc.h,v $
 * Revision 1.46  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.45  2008/08/17 06:49:29  jps
 * Added function prototype for delete_mobile.
 *
 * Revision 1.44  2008/07/22 07:25:26  myc
 * Added basic iedit (unique item editor) functionality.
 *
 * Revision 1.43  2008/07/15 18:53:39  myc
 * Renamed the command group struct.
 *
 * Revision 1.42  2008/07/15 17:55:06  myc
 * Added grant group data to the olc structure.
 * Added gedit connection mode defines.
 *
 * Revision 1.41  2008/06/19 18:53:12  myc
 * Moving the real_zone declaration into a header file.
 *
 * Revision 1.40  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.39  2008/04/07 17:24:37  jps
 * Allow mediting of stance.
 *
 * Revision 1.38  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.37  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.36  2008/03/23 00:23:07  jps
 * Update number of applies.
 *
 * Revision 1.35  2008/03/22 21:24:07  jps
 * Add life force and composition medit states.
 *
 * Revision 1.34  2008/03/11 19:50:55  myc
 * Changed the way allowed olc zones are saved on an immortal from
 * a fixed number of slots to a variable-length linked list.
 * ..
 *
 * Revision 1.33  2008/03/09 00:05:40  jps
 * Moved some NUM_foo_FLAGS defs from olc.h to structs.h.
 *
 * Revision 1.32  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.31  2008/02/16 20:31:32  myc
 * Adding function to free save list at program termination.
 *
 * Revision 1.30  2008/02/06 21:53:53  myc
 * Adding blk, red, and bld things for olc color.
 *
 * Revision 1.29  2008/01/27 21:14:59  myc
 * Increased number of AFF3 flags to include berserker chants.
 *
 * Revision 1.28  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.27  2008/01/13 03:19:53  myc
 * Added !AI flag.
 *
 * Revision 1.26  2008/01/07 10:38:09  jps
 * Update mob2 flag count
 *
 * Revision 1.25  2007/09/21 08:44:45  jps
 * Added object type "touchstone" and command "touch" so you can set
 * your home room by touching specific objects.
 *
 * Revision 1.24  2007/09/20 21:20:43  myc
 * Hide points and perception are in, along with apply constants for each.
 *
 * Revision 1.23  2007/09/15 05:17:36  myc
 * Removing the in-game distinction between AFF 1, 2, and 3 flags.
 * Added MOB2 flags.
 *
 * Revision 1.22  2007/08/14 22:50:20  myc
 * Adding AFF3_SHADOWING flag for use by shadow skill.
 *
 * Revision 1.21  2007/08/14 15:51:52  myc
 * Updating number of aff2 and aff3 flags in medit menus.
 *
 * Revision 1.20  2007/08/04 14:40:35  myc
 * Added MOB_PEACEFUL flag to prevent players from attacking certain mobs.
 *
 * Revision 1.19  2007/08/03 22:00:11  myc
 * Added PK observatories that work adjacent to arena rooms.
 *
 * Revision 1.18  2007/08/03 03:51:44  myc
 * check_pk is now attack_ok, and covers many more cases than before,
 * including peaced rooms, shapeshifted pk, and arena rooms.  Almost all
 * offensive attacks now use attack_ok to determine whether an attack is
 * allowed.
 *
 * Revision 1.17  2007/07/19 17:51:36  jps
 * Move NUM_LIQ_TYPES from olc.h to structs.h, so the LIQ defines will
 * all be in one place.
 *
 * Revision 1.16  2007/07/18 01:21:34  jps
 * You can edit AFF2/AFF3 flags with oedit.
 *
 * Revision 1.15  2006/07/17 05:15:18  cjd
 * Increased buffer on exits to support Doom's length of descrips
 *
 * Revision 1.14  2006/07/15 04:45:53  cjd
 * adjusted max_mob_desc values to allow for larger mob
 * descriptions required by doom.
 *
 * Revision 1.13  2006/04/11 09:08:24  rls
 * Mods for medit.
 *
 * Revision 1.12  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.11  2001/06/26 00:46:22  mtp
 * added purge which deletes from world and file
 *
 * Revision 1.10  2001/04/08 17:13:10  dce
 * Added an alwayslit flag that makes a room lit no matter
 * of the sector or room type...
 *
 * Revision 1.9  2001/03/24 05:12:01  dce
 * Objects will now accept a level through olc and upon
 * booting the objects. The level code for the players will
 * follow.
 *
 * Revision 1.8  2000/11/28 01:36:34  mtp
 * remveove last vestiges of mobprgos (damn these things get around)
 *
 * Revision 1.7  2000/11/28 01:21:09  mtp
 * removed mobprog stuff
 * renumbered MEDIT_EX_PLAT and MEDIT_SIZE to fill gaps
 *
 * Revision 1.6  2000/11/14 03:11:57  rsd
 * Updated the comment header to reflect that it's Fiery code
 * now.
 *
 * Revision 1.5  1999/12/10 05:13:40  cso
 * added a #define for medit_size.
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 ***************************************************************************/

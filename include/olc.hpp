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

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

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
#define CLEAR_SCREEN 1
#endif

/*. CONFIG DEFINES - these should be in structs.h, but here is easyer.*/

#define NUM_ATTACK_TYPES 21

#define NUM_SHOP_FLAGS 2
#define NUM_TRADERS 7

/*
 * Define this to how many MobProg scripts you have.
 */
#define NUM_PROGS 12

/* medit.c */
bool delete_mobile(obj_num rnum);

/*. Utils exported from olc.c .*/
void strip_string(char *);
void cleanup_olc(DescriptorData *d, byte cleanup_type);
void get_char_cols(CharData *ch);
void olc_add_to_save_list(int zone, byte type);
void olc_remove_from_save_list(int zone, byte type);
void free_save_list(void);
void free_olc_zone_list(CharData *ch);
bool has_olc_access(CharData *ch, zone_vnum zone);
int real_zone(int number);

/*. OLC structs .*/
struct OLCCommandGroup {
    char *alias;
    char *name;
    char *description;
    int minimum_level;
    int *commands;
};

struct RoomData;
struct ZoneData;
struct HelpIndexElement;
struct ShopData;
struct OLCData {
    int mode;
    int zone_num;
    int number;
    int value;
    char *storage;
    CharData *mob;
    RoomData *room;
    ObjData *obj;
    ZoneData *zone;
    ShopData *shop;
    SpellDamage *spell;
    ExtraDescriptionData *desc;
    HelpIndexElement *help;
    TrigData *trig;
    int script_mode;
    int trigger_position;
    int item_type;
    TriggerPrototypeList *script;
    /* char *storage;*/ /* for holding commands etc.. */
    OLCCommandGroup *group;
    ObjData *item;
};

struct OLCSaveInfo {
    int zone;
    char type;
    OLCSaveInfo *next;
};

/*
 * Exported globals.
 */
// #ifdef _OASIS_OLC_
// char *nrm, *grn, *cyn, *yel, *blk, *red;
//  OLCSaveInfo *olc_save_list = NULL;
// #else
const char *nrm, *grn, *cyn, *yel, *blk, *red;
OLCSaveInfo *olc_save_list;
// #endif

/*
 * Descriptor access macros.
 */
#define OLC_MODE(d) ((d)->olc->mode)       /* Parse input mode.	*/
#define OLC_NUM(d) ((d)->olc->number)      /* Room/Obj VNUM.	*/
#define OLC_VAL(d) ((d)->olc->value)       /* Scratch variable.	*/
#define OLC_ZNUM(d) ((d)->olc->zone_num)   /* Real zone number.	*/
#define OLC_ROOM(d) ((d)->olc->room)       /* Room structure.	*/
#define OLC_OBJ(d) ((d)->olc->obj)         /* Object structure.	*/
#define OLC_IOBJ(d) ((d)->olc->item)       /* Iedit object ref.	*/
#define OLC_ZONE(d) ((d)->olc->zone)       /* Zone structure.	*/
#define OLC_MOB(d) ((d)->olc->mob)         /* Mob structure.	*/
#define OLC_SHOP(d) ((d)->olc->shop)       /* Shop structure.	*/
#define OLC_DESC(d) ((d)->olc->desc)       /* Extra description.	*/
#define OLC_STORAGE(d) ((d)->olc->storage) /*. For command storage . */
#define OLC_HELP(d) ((d)->olc->help)       /*. Help structure    . */
#define OLC_SD(d) ((d)->olc->spell)        /*sdedit structure*/
#define OLC_TRIG(d) ((d)->olc->trig)       /* Trigger structure.   */
#define OLC_STORAGE(d) ((d)->olc->storage) /* For command storage  */
#define OLC_GROUP(d) ((d)->olc->group)

/*. Other macros .*/

#define OLC_EXIT(d) (OLC_ROOM(d)->exits[OLC_VAL(d)])
#define GET_OLC_ZONES(c) ((c)->player_specials->olc_zones)

/*
 * Cleanup types.
 */
#define CLEANUP_ALL (byte)1     /* Free the whole lot.	*/
#define CLEANUP_STRUCTS (byte)2 /* Don't free strings.	*/

/*
 * Add/Remove save list types.
 */
#define OLC_SAVE_ROOM (byte)0
#define OLC_SAVE_OBJ (byte)1
#define OLC_SAVE_ZONE (byte)2
#define OLC_SAVE_MOB (byte)3
#define OLC_SAVE_SHOP (byte)4
#define OLC_SAVE_HELP (byte)5

#define SDEDIT_LVL_MULT_MENU 13
#define SDEDIT_MAIN_MENU 1
#define SDEDIT_CONFIRM_SAVESTRING 0
#define SDEDIT_NPC_NO_FACE_ENTRY 3
#define SDEDIT_NPC_NO_DICE_ENTRY 2
#define SDEDIT_PC_NO_FACE_ENTRY 4
#define SDEDIT_PC_NO_DICE_ENTRY 5
#define SDEDIT_USE_BONUS 6
#define SDEDIT_MAX_BONUS 7
#define SDEDIT_NPC_REDUCE_FACTOR 8
#define SDEDIT_INTERN_DAM 9
#define SDEDIT_NPC_STATIC 10
#define SDEDIT_PC_STATIC 11
#define SDEDIT_NOTE 12

/* Submodes of HEDIT connectedness      */
#define HEDIT_CONFIRM_SAVESTRING 0
#define HEDIT_CONFIRM_EDIT 1
#define HEDIT_CONFIRM_ADD 2
#define HEDIT_MAIN_MENU 3
#define HEDIT_ENTRY 4
#define HEDIT_MIN_LEVEL 5

/* hedit permission */
#define HEDIT_PERMISSION 666

/*
 * Submodes of OEDIT connectedness.
 */
#define OEDIT_MAIN_MENU 1
#define OEDIT_EDIT_NAMELIST 2
#define OEDIT_SHORTDESC 3
#define OEDIT_LONGDESC 4
#define OEDIT_ACTDESC 5
#define OEDIT_TYPE 6
#define OEDIT_EXTRAS 7
#define OEDIT_WEAR 8
#define OEDIT_WEIGHT 9
#define OEDIT_COST 10
#define OEDIT_COSTPERDAY 11
#define OEDIT_TIMER 12
#define OEDIT_LEVEL 13
#define OEDIT_VALUE_1 14
#define OEDIT_VALUE_2 15
#define OEDIT_VALUE_3 16
#define OEDIT_VALUE_4 17
#define OEDIT_APPLY 18
#define OEDIT_APPLYMOD 19
#define OEDIT_EXTRADESC_KEY 20
#define OEDIT_CONFIRM_SAVEDB 21
#define OEDIT_CONFIRM_SAVESTRING 22
#define OEDIT_PROMPT_APPLY 23
#define OEDIT_EXTRADESC_DESCRIPTION 24
#define OEDIT_EXTRADESC_MENU 25
#define OEDIT_SPELL_APPLY 26
#define OEDIT_SPELL_COMPONENT 27
#define OEDIT_PURGE_OBJECT 28
#define OEDIT_HIDDENNESS 29

/* Submodes of REDIT connectedness */
#define REDIT_MAIN_MENU 1
#define REDIT_NAME 2
#define REDIT_DESC 3
#define REDIT_FLAGS 4
#define REDIT_SECTOR 5
#define REDIT_EXIT_MENU 6
#define REDIT_CONFIRM_SAVEDB 7
#define REDIT_CONFIRM_SAVESTRING 8
#define REDIT_EXIT_NUMBER 9
#define REDIT_EXIT_DESCRIPTION 10
#define REDIT_EXIT_KEYWORD 11
#define REDIT_EXIT_KEY 12
#define REDIT_EXIT_DOORFLAGS 13
#define REDIT_EXTRADESC_MENU 14
#define REDIT_EXTRADESC_KEY 15
#define REDIT_EXTRADESC_DESCRIPTION 16

/*. Submodes of ZEDIT connectedness 	.*/
#define ZEDIT_MAIN_MENU 0
#define ZEDIT_DELETE_ENTRY 1
#define ZEDIT_NEW_ENTRY 2
#define ZEDIT_CHANGE_ENTRY 3
#define ZEDIT_COMMAND_TYPE 4
#define ZEDIT_IF_FLAG 5
#define ZEDIT_ARG1 6
#define ZEDIT_ARG2 7
#define ZEDIT_ARG3 8
#define ZEDIT_ZONE_NAME 9
#define ZEDIT_ZONE_LIFE 10
#define ZEDIT_ZONE_TOP 11
#define ZEDIT_ZONE_RESET 12
#define ZEDIT_CONFIRM_SAVESTRING 13
#define ZEDIT_ZONE_FACTOR 14
#define ZEDIT_SARG 15
#define ZEDIT_ZONE_HEMISPHERE 16
#define ZEDIT_ZONE_CLIMATE 17

/*. Submodes of MEDIT connectedness 	.*/
#define MEDIT_MAIN_MENU 0
#define MEDIT_ALIAS 1
#define MEDIT_S_DESC 2
#define MEDIT_L_DESC 3
#define MEDIT_D_DESC 4
#define MEDIT_NPC_FLAGS 5
#define MEDIT_AFF_FLAGS 6
#define MEDIT_CONFIRM_SAVESTRING 9
#define MEDIT_PURGE_MOBILE 10
/*. Numerical responses .*/
#define MEDIT_NUMERICAL_RESPONSE 11
#define MEDIT_SEX 12
#define MEDIT_HITROLL 13
#define MEDIT_DAMROLL 14
#define MEDIT_NDD 15
#define MEDIT_SDD 16
#define MEDIT_NUM_HP_DICE 17
#define MEDIT_SIZE_HP_DICE 18
#define MEDIT_ADD_HP 19
#define MEDIT_AC 20
#define MEDIT_EXP 21
#define MEDIT_EX_GOLD 22
#define MEDIT_POS 23
#define MEDIT_DEFAULT_POS 24
#define MEDIT_ATTACK 25
#define MEDIT_LEVEL 26
#define MEDIT_ALIGNMENT 27
#define MEDIT_CLASS 28
#define MEDIT_RACE 29
#define MEDIT_EX_PLATINUM 30
#define MEDIT_SIZE 31
#define MEDIT_PERCEPTION 32
#define MEDIT_HIDDENNESS 33
#define MEDIT_LIFEFORCE 34
#define MEDIT_COMPOSITION 35
#define MEDIT_STANCE 36

/*. Submodes of SEDIT connectedness 	.*/
#define SEDIT_MAIN_MENU 0
#define SEDIT_CONFIRM_SAVESTRING 1
#define SEDIT_NOITEM1 2
#define SEDIT_NOITEM2 3
#define SEDIT_NOCASH1 4
#define SEDIT_NOCASH2 5
#define SEDIT_NOBUY 6
#define SEDIT_BUY 7
#define SEDIT_SELL 8
#define SEDIT_PRODUCTS_MENU 11
#define SEDIT_ROOMS_MENU 12
#define SEDIT_NAMELIST_MENU 13
#define SEDIT_NAMELIST 14
/*. Numerical responses .*/
#define SEDIT_NUMERICAL_RESPONSE 20
#define SEDIT_OPEN1 21
#define SEDIT_OPEN2 22
#define SEDIT_CLOSE1 23
#define SEDIT_CLOSE2 24
#define SEDIT_KEEPER 25
#define SEDIT_BUY_PROFIT 26
#define SEDIT_SELL_PROFIT 27
#define SEDIT_TYPE_MENU 29
#define SEDIT_DELETE_TYPE 30
#define SEDIT_DELETE_PRODUCT 31
#define SEDIT_NEW_PRODUCT 32
#define SEDIT_DELETE_ROOM 33
#define SEDIT_NEW_ROOM 34
#define SEDIT_SHOP_FLAGS 35
#define SEDIT_NOTRADE 36
#define SEDIT_UNIQUE 37

/* Submodes of gedit connectedness */
#define GEDIT_MAIN_MENU 0
#define GEDIT_ALIAS 1
#define GEDIT_NAME 2
#define GEDIT_DESCRIPTION 3
#define GEDIT_ADD_COMMAND 4
#define GEDIT_REMOVE_COMMAND 5
#define GEDIT_CONFIRM_SAVE 6
#define GEDIT_LEVEL 7

/*. Limit info .*/
#define MAX_ROOM_NAME 75
#define MAX_MOB_NAME 50
#define MAX_OBJ_NAME 50
#define MAX_ROOM_DESC 2048
#define MAX_EXIT_DESC 512
#define MAX_EXTRA_DESC 512
#define MAX_MOB_DESC 1024
#define MAX_OBJ_DESC 512

const char *save_info_msg[5] = {"Rooms", "Objects", "Zone info", "Mobiles", "Shops"};
/*
 * Internal data structures.
 */

struct OLCSCommandData {
    char *text;
    int con_type;
};

struct OLCSCommandData olc_scmd_info[] = {{"room", CON_REDIT},      {"object", CON_OEDIT}, {"room", CON_ZEDIT},
                                          {"mobile", CON_MEDIT},    {"shop", CON_SEDIT},   {"help", CON_HEDIT},
                                          {"trigger", CON_TRIGEDIT}};

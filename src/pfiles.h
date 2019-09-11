/***************************************************************************
 * $Id: pfiles.h,v 1.10 2008/09/01 22:15:59 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: pfiles.h                                        Part of FieryMUD *
 *  Usage: header file: Player save files                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_PFILES_H
#define __FIERY_PFILES_H

#define PLAYER_FILENAME_LENGTH 40

/*
 * The code used for items located in a player's inventory when saving.
 * It should not collide with any of the WEAR_x constants in structs.h
 */
#define WEAR_INVENTORY 127

#define MAX_CONTAINER_DEPTH  50

/* Receptionist modes */
#define SAVE_UNKNOWN  0
#define SAVE_AUTO     1
#define SAVE_CRYO     2
#define SAVE_RENT     3

extern void save_quests(struct char_data *ch);
extern void load_quests(struct char_data *ch);

extern void save_player_objects(struct char_data *ch);
extern bool load_objects(struct char_data *ch);
extern bool build_object(FILE *fl, struct obj_data **obj, int *location);
extern void extract_objects(struct char_data *ch);
extern bool write_objects(struct obj_data *obj, FILE *fl, int location);
extern void auto_save_all(void);
extern void show_rent(struct char_data *ch, char *argument);
extern bool delete_player_obj_file(struct char_data *ch);
extern FILE *open_player_obj_file(char *player_name, struct char_data *ch, bool quiet);
extern void convert_player_obj_files(struct char_data *ch);
extern void convert_single_player_obj_file(struct char_data *ch, char *name);
extern void save_player(struct char_data *ch);

#endif

/***************************************************************************
 * $Log: pfiles.h,v $
 * Revision 1.10  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.9  2008/07/27 05:26:32  jps
 * Added several save mode constants.
 *
 * Revision 1.8  2008/07/26 21:33:27  jps
 * Added a function for opening the player object save file.
 * Added functions for converting old binary object files to ASCII format.
 *
 * Revision 1.7  2008/07/13 19:04:52  jps
 * Added a hotboot rent code, which allows people to save their keys
 * over a hotboot.
 *
 * Revision 1.6  2008/06/05 02:07:43  myc
 * Completely rewrote the rent file saving and loading to use
 * an ascii text format.  Some of the old legacy binary code
 * remains so that we don't have to actively convert old
 * object files to the new format.  When old rent files are
 * encountered, they are lazily loaded into the game, and
 * replaced by the new format when the player is saved.
 *
 * Revision 1.5  2008/05/26 18:24:48  jps
 * Removed code that deletes player object files.
 *
 * Revision 1.4  2008/04/13 03:40:47  jps
 * Adding definition for the length of a player filename, such as the
 * main save file, the quest file, or the objects file.
 *
 * Revision 1.3  2008/03/30 17:32:25  jps
 * Added prototype for update_obj_file(), which is called from db.c.
 *
 * Revision 1.2  2008/03/30 17:29:55  jps
 * Adding prototypes for loadpfile_quest and loadpfile_objs.
 *
 * Revision 1.1  2008/03/30 17:11:17  jps
 * Initial revision
 *
 ***************************************************************************/

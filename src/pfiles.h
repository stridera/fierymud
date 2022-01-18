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

#include "structs.h"
#include "sysdep.h"

#define PLAYER_FILENAME_LENGTH 40

/*
 * The code used for items located in a player's inventory when saving.
 * It should not collide with any of the WEAR_x constants in structs.h
 */
#define WEAR_INVENTORY 127

#define MAX_CONTAINER_DEPTH 50

/* Receptionist modes */
#define SAVE_UNKNOWN 0
#define SAVE_AUTO 1
#define SAVE_CRYO 2
#define SAVE_RENT 3

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

void load_pets(struct char_data *ch);

#endif

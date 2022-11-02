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

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

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

extern void save_quests(char_data *ch);
extern void load_quests(char_data *ch);

extern void save_player_objects(char_data *ch);
extern bool load_objects(char_data *ch);
extern bool build_object(FILE *fl, obj_data **obj, int *location);
extern void extract_objects(char_data *ch);
extern bool write_objects(obj_data *obj, FILE *fl, int location);
extern void auto_save_all(void);
extern void show_rent(char_data *ch, char *argument);
extern bool delete_player_obj_file(char_data *ch);
extern FILE *open_player_obj_file(char *player_name, char_data *ch, bool quiet);
extern void convert_player_obj_files(char_data *ch);
extern void convert_single_player_obj_file(char_data *ch, char *name);
extern void save_player(char_data *ch);

void load_pets(char_data *ch);

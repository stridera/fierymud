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

void save_quests(CharData *ch);
void load_quests(CharData *ch);

void save_player_objects(CharData *ch);
bool load_objects(CharData *ch);
bool build_object(FILE *fl, ObjData **obj, int *location);
void extract_objects(CharData *ch);
bool write_objects(ObjData *obj, FILE *fl, int location);
void auto_save_all(void);
void show_rent(CharData *ch, Arguments argument);
bool delete_player_obj_file(CharData *ch);
FILE *open_player_obj_file(const std::string_view player_name, CharData *ch, bool quiet);
void convert_player_obj_files(CharData *ch);
void convert_single_player_obj_file(CharData *ch, std::string_view name);
void save_player(CharData *ch);

void load_pets(CharData *ch);

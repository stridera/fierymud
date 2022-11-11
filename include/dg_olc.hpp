/***************************************************************************
 *  File: dg_olc.h                                        Part of FieryMUD *
 * Usage: This header file is used in extending Oasis style OLC for        *
 *        dg-scripts onto a CircleMUD that already has dg-scripts (as      *
 *        released by Mark Heilpern on 1/1/98) implemented.                *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "dg_scripts.hpp"
#include "structs.hpp"

/* prototype exported functions from dg_olc.c */
void script_copy(void *dst, void *src, int type);
void script_save_to_disk(FILE *fp, void *item, int type);
void dg_olc_script_free(DescriptorData *d);
void dg_olc_script_copy(DescriptorData *d);
void dg_script_menu(DescriptorData *d);
int dg_script_edit_parse(DescriptorData *d, char *arg);

/* define the largest set of commands for as trigger */
#define MAX_CMD_LENGTH 16384 /* 16k should be plenty and then some */

/*
 * Submodes of TRIGEDIT connectedness.
 */
#define TRIGEDIT_MAIN_MENU 0
#define TRIGEDIT_TRIGTYPE 1
#define TRIGEDIT_CONFIRM_SAVESTRING 2
#define TRIGEDIT_NAME 3
#define TRIGEDIT_INTENDED 4
#define TRIGEDIT_TYPES 5
#define TRIGEDIT_COMMANDS 6
#define TRIGEDIT_NARG 7
#define TRIGEDIT_ARGUMENT 8

#define OLC_SCRIPT_EDIT 82766
#define SCRIPT_MAIN_MENU 0
#define SCRIPT_NEW_TRIGGER 1
#define SCRIPT_DEL_TRIGGER 2

#define OLC_SCRIPT_EDIT_MODE(d) ((d)->olc->script_mode) /* parse input mode */
#define OLC_SCRIPT(d) ((d)->olc->script)                /* script editing   */
#define OLC_ITEM_TYPE(d) ((d)->olc->item_type)          /* mob/obj/room     */

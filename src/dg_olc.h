/***************************************************************************
 * $Id: dg_olc.h,v 1.7 2008/02/02 04:27:55 myc Exp $
 ***************************************************************************/
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

#ifndef __FIERY_DG_OLC_H
#define __FIERY_DG_OLC_H

#include "dg_scripts.h"
#include "structs.h"

/* prototype exported functions from dg_olc.c */
void script_copy(void *dst, void *src, int type);
void script_save_to_disk(FILE *fp, void *item, int type);
void dg_olc_script_free(struct descriptor_data *d);
void dg_olc_script_copy(struct descriptor_data *d);
void dg_script_menu(struct descriptor_data *d);
int dg_script_edit_parse(struct descriptor_data *d, char *arg);

/* define the largest set of commands for as trigger */
#define MAX_CMD_LENGTH 16384 /* 16k should be plenty and then some */

#define NUM_TRIG_TYPE_FLAGS 22

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

#endif /* __FIERY_DG_OLC_H */
/***************************************************************************
 * $Log: dg_olc.h,v $
 * Revision 1.7  2008/02/02 04:27:55  myc
 * Increasing the number of trigger type flags to 20.
 *
 * Revision 1.6  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.5  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.4  2000/11/21 03:57:30  rsd
 * Altered the comment header to resemble standard Fiery code
 * and added the initial revision rlog message.
 *
 * Revision 1.3  2000/11/11 01:37:49  mtp
 * added ASK trigger
 *
 * Revision 1.2  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

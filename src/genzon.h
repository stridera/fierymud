/***************************************************************************
 *  File: genzon.h                                        Part of FieryMUD *
 *  Usage: header file for zone routines                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_GENZON_H
#define __FIERY_GENZON_H

#include "db.h"
#include "structs.h"

int count_commands(struct reset_com *list);
void delete_zone_command(struct zone_data *zone, int pos);

#endif

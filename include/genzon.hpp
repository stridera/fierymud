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

#pragma once

#include "db.hpp"
#include "structs.hpp"

int count_commands(ResetCommand *list);
void delete_zone_command(ZoneData *zone, int pos);

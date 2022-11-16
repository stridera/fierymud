/***************************************************************************
 *  File: specprocs.h                                     Part of FieryMUD *
 *  Usage: header file special procedures                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_SPECPROCS_H
#define __FIERY_SPECPROCS_H

#include "structs.h"
#include "sysdep.h"

#define SPECIAL(name) int(name)(struct char_data * ch, void *me, int cmd, char *argument)

#endif
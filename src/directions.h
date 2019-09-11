/***************************************************************************
 * $Id: directions.h,v 1.2 2009/03/09 04:33:20 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: directions.h                                    Part of FieryMUD *
 *  Usage: header file for directions                                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_DIRECTIONS_H
#define __FIERY_DIRECTIONS_H

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define NUM_OF_DIRS    6        /* number of directions in a room (nsewud) */

extern const char *dirs[NUM_OF_DIRS + 1];
extern const char *capdirs[NUM_OF_DIRS + 1];
extern const char *dirpreposition[NUM_OF_DIRS + 1];
extern const int rev_dir[NUM_OF_DIRS];

#endif

/***************************************************************************
 * $Log: directions.h,v $
 * Revision 1.2  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.1  2009/03/09 04:24:47  jps
 * Initial revision
 *
 ***************************************************************************/

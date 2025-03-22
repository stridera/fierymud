/***************************************************************************
 *   File: diskio.h                                       Part of FieryMUD *
 *  Usage: Fast file buffering                                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  (C) Copyright 1998 by Brian Boyle   Version 1.3                        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998-2002 by the Fiery Consortium               *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#define FB_READ (1 << 0)   /* read from disk	*/
#define FB_WRITE (1 << 1)  /* write to disk	*/
#define FB_APPEND (1 << 2) /* write with append	*/

#define FB_STARTSIZE 4192 /* 4k starting buffer for writes */

#ifndef IS_SET
#define IS_SET(flag, bits) ((flag) & (bits))
#endif

struct FBFILE {
    char *buf;  /* start of buffer			*/
    char *ptr;  /* current location pointer		*/
    int size;   /* size in bytes of buffer		*/
    int flags;  /* read/write/append, future expansion	*/
    char *name; /* filename (for delayed writing)	*/
};

void tag_argument(char *argument, char *tag);
int fbgetline(FBFILE *fbfl, char *line);
FBFILE *fbopen(char *fname, int mode);
int fbclose(FBFILE *fbfl);
int fbwrite(FBFILE *fbfl, const char *string);
int fbprintf(FBFILE *fbfl, const char *format, ...);
void fbrewind(FBFILE *fbfl);
int fbcat(char *fromfilename, FBFILE *tofile);
char *fbgetstring(FBFILE *fl);

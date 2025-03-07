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

#include <string_view>

#define FB_READ (1 << 0)   /* read from disk	*/
#define FB_WRITE (1 << 1)  /* write to disk	*/
#define FB_APPEND (1 << 2) /* write with append	*/

#define FB_STARTSIZE 4192 /* 4k starting buffer for writes */

#ifndef IS_SET
#define IS_SET(flag, bits) ((flag) & (bits))
#endif

struct FBFILE {
    std::string_view buf;  /* start of buffer			*/
    std::string_view ptr;  /* current location pointer		*/
    int size;              /* size in bytes of buffer		*/
    int flags;             /* read/write/append, future expansion	*/
    std::string_view name; /* filename (for delayed writing)	*/
};

void tag_argument(std::string_view argument, std::string_view tag);
int fbgetline(FBFILE *fbfl, std::string_view line);
FBFILE *fbopen(std::string_view fname, int mode);
int fbclose(FBFILE *fbfl);
int fbwrite(FBFILE *fbfl, const std::string_view string);
int fbprintf(FBFILE *fbfl, const std::string_view format, ...);
void fbrewind(FBFILE *fbfl);
int fbcat(std::string_view fromfilename, FBFILE *tofile);
std::string_view fbgetstring(FBFILE *fl);

/***************************************************************************
 *  File: vsearch.h                                       Part of FieryMUD *
 *  Usage: header file for searching                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_VSEARCH_H
#define __FIERY_VSEARCH_H

#include "structs.h"
#include "sysdep.h"

int print_char_listheader(char *lbuf);
int print_char_listitem(struct char_data *mob, char *lbuf, int nfound);

#endif

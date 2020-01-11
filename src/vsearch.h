/***************************************************************************
 * $Id: vsearch.h,v 1.4 2009/03/17 07:55:37 jps Exp $
 ***************************************************************************/
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

#include "sysdep.h"
#include "structs.h"

int print_char_listheader(char *lbuf);
int print_char_listitem(struct char_data *mob, char *lbuf, int nfound);

#endif

/***************************************************************************
 * $Log: vsearch.h,v $
 * Revision 1.4  2009/03/17 07:55:37  jps
 * Moved ellipsis string formatting to strings.c.
 *
 * Revision 1.3  2009/02/18 19:26:11  myc
 * Renamed NUMBER domain as INTEGER.  Rewrote code that
 * truncates strings and puts ellipses on the end.  See
 * the ELLIPSIS_FMT and ELLIPSIS_STR macros.
 *
 * Revision 1.2  2008/04/20 17:50:27  jps
 * Returning number of bytes printed by the character list functions.
 *
 * Revision 1.1  2008/04/20 03:50:56  jps
 * Initial revision
 *
 ***************************************************************************/

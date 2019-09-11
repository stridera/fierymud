/***************************************************************************
 * $Id: specprocs.h,v 1.1 2009/03/09 04:33:20 jps Exp $
 ***************************************************************************/
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

#define SPECIAL(name) \
      int (name)(struct char_data *ch, void *me, int cmd, char *argument)

#endif

/***************************************************************************
 * $Log: specprocs.h,v $
 * Revision 1.1  2009/03/09 04:33:20  jps
 * Initial revision
 *
 ***************************************************************************/

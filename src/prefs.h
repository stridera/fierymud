/***************************************************************************
 * $Id: prefs.h,v 1.1 2010/06/09 22:32:01 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: prefs.h                                        Part of FieryMUD *
 *  Usage: header file for player preferences                              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_PREFS_H
#define __FIERY_PREFS_H

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF 0       /* Room descs won't normally be shown */
#define PRF_COMPACT 1     /* No extra CRLF pair before prompts  */
#define PRF_DEAF 2        /* Can't hear shouts                  */
#define PRF_NOTELL 3      /* Can't receive tells                */
#define PRF_OLCCOMM 4     /* Can hear communication in OLC      */
#define PRF_LINENUMS 5    /* Autodisplay linenums in stringedit */
#define PRF_AUTOLOOT 6    /* Auto loot corpses when you kill    */
#define PRF_AUTOEXIT 7    /* Display exits in a room            */
#define PRF_NOHASSLE 8    /* Aggr mobs won't attack             */
#define PRF_QUEST 9       /* On quest                           */
#define PRF_SUMMONABLE 10 /* Can be summoned                    */
#define PRF_NOREPEAT 11   /* No repetition of comm commands     */
#define PRF_HOLYLIGHT 12  /* Can see in dark                    */
#define PRF_COLOR_1 13    /* Color (low bit)                    */
#define PRF_COLOR_2 14    /* Color (high bit)                   */
#define PRF_NOWIZ 15      /* Can't hear wizline                 */
#define PRF_LOG1 16       /* On-line System Log (low bit)       */
#define PRF_LOG2 17       /* On-line System Log (high bit)      */
#define PRF_AFK 18        /* away from keyboard                 */
#define PRF_NOGOSS 19     /* Can't hear gossip channel          */
#define PRF_NOHINTS 20    /* No hints when mistyping commands   */
#define PRF_ROOMFLAGS 21  /* Can see room flags (ROOM_x)        */
#define PRF_NOPETI 22     /* Can't hear petitions               */
#define PRF_AUTOSPLIT 23  /* Auto split coins from corpses      */
#define PRF_NOCLANCOMM 24 /* Can't hear clan communication      */
#define PRF_ANON 25       /* Anon flag                          */
#define PRF_SHOWVNUMS 26  /* Show Virtual Numbers               */
#define PRF_NICEAREA 27
#define PRF_VICIOUS 28
#define PRF_PASSIVE 29 /* char will not engage upon being cast on */
#define PRF_ROOMVIS 30
#define PRF_NOFOLLOW 31  /* Cannot follow / well to this player*/
#define PRF_AUTOTREAS 32 /* Automatically loots treasure from corpses */
#define PRF_EXPAND_OBJS 33
#define PRF_EXPAND_MOBS 34
#define PRF_SACRIFICIAL 35 /* Sacrificial spells autotarget self */
#define PRF_PETASSIST 36   /* Should your pet assist you as you fight */
#define NUM_PRF_FLAGS 37

#endif

/***************************************************************************
 * $Log: prefs.h,v $
 * Revision 1.1  2010/06/09 22:32:01  mud
 * Initial revision
 *
 ***************************************************************************/

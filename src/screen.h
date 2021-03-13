/***************************************************************************
 * $Id: screen.h,v 1.7 2008/05/23 18:47:00 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: screen.h                                       Part of FieryMUD *
 *  Usage: header file with ANSI color codes for online color              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/
#ifndef __FIERY_SCREEN_H
#define __FIERY_SCREEN_H

#include "structs.h"
#include "sysdep.h"

#define CLR_PARSE 0
#define CLR_ESCAPE 1
#define CLR_STRIP 2

extern void init_colors(void);
extern int process_colors(char *out, size_t max_len, const char *in, int mode);
extern int count_color_chars(const char *string);
extern int ansi_strlen(const char *string);
extern char *strip_ansi(const char *string);
extern char *escape_ansi(const char *string);

/* General color codes */
#define ANUL ""         /* No effect                            */
#define ANRM "\x1B[0m"  /* Reset to normal                      */
#define ABLD "\x1B[1m"  /* Brighten colors                      */
#define ADAR "\x1B[2m"  /* Dim colors                           */
#define AEMP "\x1B[3m"  /* Italicize (not widely supported)     */
#define AUND "\x1B[4m"  /* Underline                            */
#define AFSH "\x1B[5m"  /* Flashing text                        */
#define AFST "\x1B[6m"  /* Flashing text (fast)                 */
#define ARVS "\x1B[7m"  /* Reverse video                        */
#define AHID "\x1B[8m"  /* Hide text                            */
#define ADUN "\x1B[21m" /* Double underline                     */
#define ANRI "\x1B[22m" /* Normal intensity (not dim/bright)    */
#define ANUN "\x1B[24m" /* Turn off underline                   */
#define ANFS "\x1B[25m" /* Turn off flashing                    */
#define APOS "\x1B[27m" /* Positive image                       */
#define AUNH "\x1B[28m" /* Unhide text                          */

#define ACLR "\x1B[2J" /* Clear screen                         */
#define AALM "\x07"    /* Alarm (beep)                         */
#define CREL '&'       /* Relative color change character      */
#define AREL "&"       /* Relative color change character      */
#define CABS '@'       /* Absolute color change character      */
#define AABS "@"       /* Absolute color change character      */

/* Normal (relative) text colors */
#define FBLK "\x1B[30m" /* Foreground Black                     */
#define FRED "\x1B[31m" /* Foreground Red                       */
#define FGRN "\x1B[32m" /* Foreground Green                     */
#define FYEL "\x1B[33m" /* Foreground Yellow                    */
#define FBLU "\x1B[34m" /* Foreground Blue                      */
#define FMAG "\x1B[35m" /* Foreground Magenta                   */
#define FCYN "\x1B[36m" /* Foreground Cyan                      */
#define FWHT "\x1B[37m" /* Foreground White (Light Gray)        */

/* Highlit (relative) text colors */
#define HBLK "\x1B[1;30m" /* Foreground Bright Black (Dark Gray)  */
#define HRED "\x1B[1;31m" /* Foreground Bright Red                */
#define HGRN "\x1B[1;32m" /* Foreground Bright Green              */
#define HYEL "\x1B[1;33m" /* Foreground Bright Yellow             */
#define HBLU "\x1B[1;34m" /* Foreground Bright Blue               */
#define HMAG "\x1B[1;35m" /* Foreground Bright Magenta            */
#define HCYN "\x1B[1;36m" /* Foreground Bright Cyan               */
#define HWHT "\x1B[1;37m" /* Foreground Bright White              */

/* Normal (relative) background colors */
#define BBLK "\x1B[40m"  /* Background Black                     */
#define BRED "\x1B[41m"  /* Background Red                       */
#define BGRN "\x1B[42m"  /* Background Green                     */
#define BYEL "\x1B[43m"  /* Background Yellow                    */
#define BBLU "\x1B[44m"  /* Background Blue                      */
#define BMAG "\x1B[45m"  /* Background Magenta                   */
#define BCYN "\x1B[46m"  /* Background Cyan                      */
#define BWHT "\x1B[47m"  /* Background White                     */
#define BLBK "\x1b[100m" /* Background Light Black               */

/* Normal (absolute) text colors */
#define AFBLK "\x1B[0;30m" /* Foreground Black                     */
#define AFRED "\x1B[0;31m" /* Foreground Red                       */
#define AFGRN "\x1B[0;32m" /* Foreground Green                     */
#define AFYEL "\x1B[0;33m" /* Foreground Yellow                    */
#define AFBLU "\x1B[0;34m" /* Foreground Blue                      */
#define AFMAG "\x1B[0;35m" /* Foreground Magenta                   */
#define AFCYN "\x1B[0;36m" /* Foreground Cyan                      */
#define AFWHT "\x1B[0;37m" /* Foreground White (Light Gray)        */

/* Bright (absolute) text colors */
#define AHBLK "\x1B[0;1;30m" /* Foreground Bright Black (Dark Gray)*/
#define AHRED "\x1B[0;1;31m" /* Foreground Bright Red              */
#define AHGRN "\x1B[0;1;32m" /* Foreground Bright Green            */
#define AHYEL "\x1B[0;1;33m" /* Foreground Bright Yellow           */
#define AHBLU "\x1B[0;1;34m" /* Foreground Bright Blue             */
#define AHMAG "\x1B[0;1;35m" /* Foreground Bright Magenta          */
#define AHCYN "\x1B[0;1;36m" /* Foreground Bright Cyan             */
#define AHWHT "\x1B[0;1;37m" /* Foreground Bright White            */

/* Flashing (absolute) text colors */
#define ABBLK "\x1B[0;5;30m" /* Foreground Blinking Black          */
#define ABRED "\x1B[0;5;31m" /* Foreground Blinking Red            */
#define ABGRN "\x1B[0;5;32m" /* Foreground Blinking Green          */
#define ABYEL "\x1B[0;5;33m" /* Foreground Blinking Yellow         */
#define ABBLU "\x1B[0;5;34m" /* Foreground Blinking Blue           */
#define ABMAG "\x1B[0;5;35m" /* Foreground Blinking Magenta        */
#define ABCYN "\x1B[0;5;36m" /* Foreground Blinking Cyan           */
#define ABWHT "\x1B[0;5;37m" /* Foreground Blinking White          */

/* Bright flashing (absolute) text colors */
#define ABHBLK "\x1B[1;5;30m" /* Foreground Blinking Bright Black   */
#define ABHRED "\x1B[1;5;31m" /* Foreground Blinking Bright Red     */
#define ABHGRN "\x1B[1;5;32m" /* Foreground Blinking Bright Green   */
#define ABHYEL "\x1B[1;5;33m" /* Foreground Blinking Bright Yellow  */
#define ABHBLU "\x1B[1;5;34m" /* Foreground Blinking Bright Blue    */
#define ABHMAG "\x1B[1;5;35m" /* Foreground Blinking Bright Magenta */
#define ABHCYN "\x1B[1;5;36m" /* Foreground Blinking Bright Cyan    */
#define ABHWHT "\x1B[1;5;37m" /* Foreground Blinking Bright White   */

/* Conditional color.  pass it a pointer to a char_data and a color level. */
#define C_OFF 0
#define C_SPR 1
#define C_NRM 2
#define C_CMP 3
#define COLOR_LEV(ch) ((PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + (PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0))

#define CLRLV(ch, clr, lev) (COLOR_LEV(ch) >= (lev) ? (clr) : ANUL)
#define CLR(ch, clr) CLRLV((ch), (clr), C_NRM)

#define QNRM CLR(ch, ANRM)

/* Normal (absolute) text colors */
#define QBLK CLR(ch, AFBLK)
#define QRED CLR(ch, AFRED)
#define QGRN CLR(ch, AFGRN)
#define QYEL CLR(ch, AFYEL)
#define QBLU CLR(ch, AFBLU)
#define QMAG CLR(ch, AFMAG)
#define QCYN CLR(ch, AFCYN)
#define QWHT CLR(ch, AFWHT)

/* Bright (absolute) text colors */
#define QHBLK CLR(ch, AHBLK)
#define QHRED CLR(ch, AHRED)
#define QHGRN CLR(ch, AHGRN)
#define QHYEL CLR(ch, AHYEL)
#define QHBLU CLR(ch, AHBLU)
#define QHMAG CLR(ch, AHMAG)
#define QHCYN CLR(ch, AHCYN)
#define QHWHT CLR(ch, AHWHT)

#endif /* __FIERY_SCREEN_H */

/***************************************************************************
 * $Log: screen.h,v $
 * Revision 1.7  2008/05/23 18:47:00  myc
 * Added ansi_strlen function that counts the length of a string sans
 * color codes.
 *
 * Revision 1.6  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.5  2008/01/15 06:51:47  myc
 * Reformatted the conditional color macros.
 *
 * Revision 1.4  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.3  2000/11/24 21:17:12  rsd
 * Altered comment header and added back rlog messgaes from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.2  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

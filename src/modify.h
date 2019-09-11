/***************************************************************************
 * $Id: modify.h,v 1.6 2009/03/20 23:02:59 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: modify.h                                        Part of FieryMUD *
 *  Usage: header file for string paging and editing                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_MODIFY_H
#define __FIERY_MODIFY_H

extern void string_write_limit(struct descriptor_data *d, char **writeto, size_t len, int maxlines);
extern void string_write(struct descriptor_data *d, char **writeto, size_t len);
extern void mail_write(struct descriptor_data *d, char **writeto, size_t len, long recipient);

/* PAGING */

/* page_string and page_string_desc will also start paging */
extern void page_string(struct char_data *ch, const char *str);
extern void page_string_desc(struct descriptor_data *d, const char *str);

/* paging_printf will collect data, but you must then call start_paging */
extern void paging_printf(const struct char_data *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
extern void desc_paging_printf(struct descriptor_data *d, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));

extern void start_paging(struct char_data *ch);
extern void start_paging_desc(struct descriptor_data *desc);
extern void free_paged_text(struct descriptor_data *d);
extern void get_paging_input(struct descriptor_data *d, char *input);

#define pprintf paging_printf
#define pdprintf desc_paging_printf
#define PAGING(d)           (d->paging_numlines)
#define PAGING_PAGE(d)      (d->paging_curpage)
#define PAGING_NUMPAGES(d)  (d->paging_numpages)

#endif

/***************************************************************************
 * $Log: modify.h,v $
 * Revision 1.6  2009/03/20 23:02:59  myc
 * Remove text editor connection state.  Make paging input
 * strings declared const.
 *
 * Revision 1.5  2009/02/11 17:03:39  myc
 * Make some functions static and add desc_paging_printf(),
 * which is just like paging_printf, but it takes a descriptor
 * instead of a character.
 *
 * Revision 1.4  2008/08/16 08:22:41  jps
 * Added the 'desc' command and took player description-editing out of the pre-game menu.
 *
 * Revision 1.3  2008/08/15 03:59:08  jps
 * Added pprintf for paging, and changed page_string to take a character.
 *
 * Revision 1.2  2008/08/14 09:44:36  jps
 * Eliminate the unused keep_internal parameter from page_string.
 *
 * Revision 1.1  2008/08/14 09:39:02  jps
 * Initial revision
 *
 ***************************************************************************/

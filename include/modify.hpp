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

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

extern void string_write_limit(descriptor_data *d, char **writeto, size_t len, int maxlines);
extern void string_write(descriptor_data *d, char **writeto, size_t len);
extern void mail_write(descriptor_data *d, char **writeto, size_t len, long recipient);

/* PAGING */

/* page_string and page_string_desc will also start paging */
extern void page_string(char_data *ch, const char *str);
extern void page_string_desc(descriptor_data *d, const char *str);

/* paging_printf will collect data, but you must then call start_paging */
extern void paging_printf(const struct char_data *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
extern void desc_paging_printf(descriptor_data *d, const char *messg, ...) __attribute__((format(printf, 2, 3)));

extern void start_paging(char_data *ch);
extern void start_paging_desc(descriptor_data *desc);
extern void free_paged_text(descriptor_data *d);
extern void get_paging_input(descriptor_data *d, char *input);

#define pprintf paging_printf
#define pdprintf desc_paging_printf
#define PAGING(d) (d->paging_numlines)
#define PAGING_PAGE(d) (d->paging_curpage)
#define PAGING_NUMPAGES(d) (d->paging_numpages)

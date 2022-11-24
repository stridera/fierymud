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

void string_write_limit(DescriptorData *d, char **writeto, size_t len, int maxlines);
void string_write(DescriptorData *d, char **writeto, size_t len);
void mail_write(DescriptorData *d, char **writeto, size_t len, long recipient);

/* PAGING */

/* page_string and page_string_desc will also start paging */
void page_string(CharData *ch, const char *str);
void page_string_desc(DescriptorData *d, const char *str);

/* paging_printf will collect data, but you must then call start_paging */
void paging_printf(CharData *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void desc_paging_printf(DescriptorData *d, const char *messg, ...) __attribute__((format(printf, 2, 3)));

void start_paging(CharData *ch);
void start_paging_desc(DescriptorData *desc);
void free_paged_text(DescriptorData *d);
void get_paging_input(DescriptorData *d, char *input);

#define pprintf paging_printf
#define pdprintf desc_paging_printf
#define PAGING(d) (d->paging_numlines)
#define PAGING_PAGE(d) (d->paging_curpage)
#define PAGING_NUMPAGES(d) (d->paging_numpages)
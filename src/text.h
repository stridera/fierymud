/***************************************************************************
 * $Id: text.h,v 1.2 2009/06/09 05:50:07 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: text.h                                         Part of FieryMUD *
 *  Usage: header file: text management functions                          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_TEXT_H
#define __FIERY_TEXT_H

#include "sysdep.h"
#include "structs.h"

#define FORMAT_INDENT (1 << 0)

void smash_tilde(char *str);
int replace_str(char **string, const char *pattern, const char *replacement, int rep_all, int max_size);
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen);
char *stripcr(char *dest, const char *src);
char *next_line(char **src);
char *cap_by_color(char *s);
char *capitalize(char *s);
bool isplural(char *namelist);
bool startsvowel(const char *s);

char *with_indefinite_article(const char *s);
const char *without_article(const char *s);
const char *pluralize(const char *s);

int levenshtein_distance(const char *s1, const char *s2);

void trim_spaces(char *buffer);

#define S_WHITESPACE " \t\n\v\f\r"
#define S_DIGITS "1234567890"
const char *skip_over(const char *string, const char *skip);
const char *skip_chars(const char *string, char skip);
const char *fetch_word(const char *string, char *buf, size_t space);
char *strip_chars(char *str, const char *chars);
char *filter_chars(char *buf, register const char *src, const char *chars);

typedef struct screen_buf_t *screen_buf;
screen_buf new_screen_buf(void);
const char *sb_get_buffer(screen_buf);
void sb_use_buf(screen_buf, char *buf, size_t buf_capacity);
size_t sb_get_capacity(screen_buf);
size_t sb_get_length(screen_buf);
size_t sb_get_lines(screen_buf);
size_t sb_get_width(screen_buf);
size_t sb_get_first_indentation(screen_buf);
size_t sb_get_other_indentation(screen_buf);
bool sb_using_capitalization(screen_buf);
const char *sb_get_line(screen_buf, size_t line);
void sb_set_width(screen_buf, size_t columns);
void sb_set_first_indentation(screen_buf, size_t indentation);
void sb_set_other_indentation(screen_buf, size_t indentation);
void sb_use_capitalization(screen_buf, bool);
void sb_append(screen_buf, const char *msg, ...) __attribute__((format(printf, 2, 3)));
void free_screen_buf(screen_buf);

#endif

/***************************************************************************
 * $Log: text.h,v $
 * Revision 1.2  2009/06/09 05:50:07  myc
 * Moving all old textfile code from here to textfiles.[ch].
 * Moving lots of text management functions from strings.c to
 * here.  Also adding screen-buffer API for formatting text
 * on the go.
 *
 ***************************************************************************/

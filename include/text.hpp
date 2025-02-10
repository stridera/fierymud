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

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

#define FORMAT_INDENT (1 << 0)

void smash_tilde(char *str);
int replace_str(char **string, const char *pattern, const char *replacement, int rep_all, int max_size);
void format_text(char **ptr_string, int mode, DescriptorData *d, int maxlen);
char *stripcr(char *dest, const char *src);
char *next_line(char **src);
char *cap_by_color(char *s);
const char *capitalize(const char *s);
bool isplural(const char *namelist);
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
char *filter_chars(char *buf, const char *src, const char *chars);

#define SB_EXTERNAL 0
#define SB_USE_CAPS 1
#define NUM_SB_FLAGS 2
typedef std::bitset<NUM_SB_FLAGS> ScreenBufFlags;

#define SB_INITIAL_BUF_CAP 1000
#define SB_INITIAL_LINES_CAP 10

struct ScreenBuf {
    char *buf;
    size_t capacity;
    size_t length;
    ScreenBufFlags flags;

    char **lines;
    size_t line_capacity;
    size_t line_count;
    size_t line_width;
    size_t first_indent;
    size_t other_indent;
};

ScreenBuf *new_screen_buf(void);
const char *sb_get_buffer(ScreenBuf *);
void sb_use_buf(ScreenBuf *, char *buf, size_t buf_capacity);
size_t sb_get_capacity(ScreenBuf *);
size_t sb_get_length(ScreenBuf *);
size_t sb_get_lines(ScreenBuf *);
size_t sb_get_width(ScreenBuf *);
size_t sb_get_first_indentation(ScreenBuf *);
size_t sb_get_other_indentation(ScreenBuf *);
bool sb_using_capitalization(ScreenBuf *);
const char *sb_get_line(ScreenBuf *, size_t line);
void sb_set_width(ScreenBuf *, size_t columns);
void sb_set_first_indentation(ScreenBuf *, size_t indentation);
void sb_set_other_indentation(ScreenBuf *, size_t indentation);
void sb_use_capitalization(ScreenBuf *, bool);
void sb_append(ScreenBuf *, const char *msg, ...) __attribute__((format(printf, 2, 3)));
void free_screen_buf(ScreenBuf *);

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

void smash_tilde(std::string_view str);
void format_text(std::string_view ptr_string, int mode, DescriptorData *d, int maxlen);
std::string_view stripcr(std::string_view dest, const std::string_view src);
bool isplural(const std::string_view namelist);
bool startsvowel(const std::string_view s);

std::string with_indefinite_article(const std::string_view s);
std::string without_article(const std::string_view s);
std::string pluralize(const std::string_view s);

int levenshtein_distance(const std::string_view s1, const std::string_view s2);

void trim_spaces(std::string_view buffer);

#define S_WHITESPACE " \t\n\v\f\r"
#define S_DIGITS "1234567890"
std::string skip_over(const std::string_view string, const std::string_view skip);
std::string skip_chars(const std::string_view string, const char skip);
std::string_view fetch_word(std::string_view string, std::string &buf);
std::string_view strip_chars(std::string_view str, const std::string_view chars);
std::string_view filter_chars(const std::string_view src, const std::string_view chars);

#define SB_EXTERNAL 0
#define SB_USE_CAPS 1
#define NUM_SB_FLAGS 2

#define SB_INITIAL_BUF_CAP 1000
#define SB_INITIAL_LINES_CAP 10

struct ScreenBuf {
    std::string_view buf;
    size_t capacity;
    size_t length;
    flagvector flags[FLAGVECTOR_SIZE(NUM_SB_FLAGS)];

    std::string_view lines;
    size_t line_capacity;
    size_t line_count;
    size_t line_width;
    size_t first_indent;
    size_t other_indent;
};

ScreenBuf *new_screen_buf(void);
const std::string_view sb_get_buffer(ScreenBuf *);
void sb_use_buf(ScreenBuf *, std::string_view buf, size_t buf_capacity);
size_t sb_get_capacity(ScreenBuf *);
size_t sb_get_length(ScreenBuf *);
size_t sb_get_lines(ScreenBuf *);
size_t sb_get_width(ScreenBuf *);
size_t sb_get_first_indentation(ScreenBuf *);
size_t sb_get_other_indentation(ScreenBuf *);
bool sb_using_capitalization(ScreenBuf *);
const std::string_view sb_get_line(ScreenBuf *, size_t line);
void sb_set_width(ScreenBuf *, size_t columns);
void sb_set_first_indentation(ScreenBuf *, size_t indentation);
void sb_set_other_indentation(ScreenBuf *, size_t indentation);
void sb_use_capitalization(ScreenBuf *, bool);
template <typename... Args> void sb_append(ScreenBuf *d, fmt::string_view str, Args &&...args) {
    sb_append(d, fmt::vformat(str, fmt::make_format_args(args...)));
}
void free_screen_buf(ScreenBuf *);

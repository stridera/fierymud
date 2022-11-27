/***************************************************************************
 *   File: screen.c                                       Part of FieryMUD *
 *  Usage: ANSI color code functions for online color                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "screen.hpp"

#include "conf.hpp"
#include "math.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#define CHARS (1 << (8 * sizeof(char)))

static const char *REL_COLORS[CHARS];
static const char *ABS_COLORS[CHARS];

void init_colors(void) {
    const struct {
        const char code;
        const char *ansi;
    } rel_color_list[] = {{CREL, AREL}, {'0', ANRM}, {'1', FRED}, {'2', FGRN}, {'3', FYEL}, {'4', FBLU},
                          {'5', FMAG},  {'6', FCYN}, {'7', FWHT}, {'8', ABLD}, {'9', FBLK}, {'u', AUND},
                          {'b', ABLD},  {'d', ADAR}, {'R', BRED}, {'G', BGRN}, {'Y', BYEL}, {'B', BBLU},
                          {'M', BMAG},  {'C', BCYN}, {'W', BWHT}, {'L', BBLK}, {'K', BLBK}, {'_', "\n"}};
    const struct {
        const char code;
        const char *ansi;
    } abs_color_list[] = {
        {CABS, AABS}, {'0', ANRM},  {'1', AFRED}, {'2', AFGRN}, {'3', AFYEL}, {'4', AFBLU}, {'5', AFMAG},
        {'6', AFCYN}, {'7', AFWHT}, {'9', AFBLK}, {'r', AFRED}, {'g', AFGRN}, {'y', AFYEL}, {'b', AFBLU},
        {'m', AFMAG}, {'c', AFCYN}, {'w', AFWHT}, {'d', AFBLK}, {'R', AHRED}, {'G', AHGRN}, {'Y', AHYEL},
        {'B', AHBLU}, {'M', AHMAG}, {'C', AHCYN}, {'W', AHWHT}, {'L', AHBLK},
    };
    const int rel_color_count = sizeof(rel_color_list) / sizeof(*rel_color_list);
    const int abs_color_count = sizeof(abs_color_list) / sizeof(*abs_color_list);
    int i;

    /* Initialize the color lists with null pointers */
    for (i = 0; i < CHARS; ++i)
        REL_COLORS[i] = ABS_COLORS[i] = nullptr;

    /* Copy relative color codes to the lists */
    for (i = 0; i < rel_color_count; ++i)
        REL_COLORS[(int)rel_color_list[i].code] = rel_color_list[i].ansi;

    /* Copy absolute color codes to the lists */
    for (i = 0; i < abs_color_count; ++i)
        ABS_COLORS[(int)abs_color_list[i].code] = abs_color_list[i].ansi;
}

int process_colors(char *out, size_t max_len, const char *in, int mode) {
    char *dst, *tmp, type;
    const char *code = nullptr, *src;

    if (!in || !out)
        return 0;

    src = in;
    if (in == out) {
        CREATE(tmp, char, max_len);
        dst = tmp;
    } else
        dst = tmp = out;

    while (*src && dst - tmp < max_len) {
        if (*src != CREL && *src != CABS) {
            *dst++ = *src++;
            continue;
        }

        type = *src++;

        if (!*src)
            break;

        /* If parsing for color, fill in the right color code. */
        if (mode == CLR_PARSE) {
            if (type == CREL)
                code = REL_COLORS[(int)*src];
            else if (type == CABS)
                code = ABS_COLORS[(int)*src];
            else
                assert(false);
            if (code)
                while (*code && dst - tmp < max_len)
                    *dst++ = *code++;
        } else if (mode == CLR_ESCAPE && dst + 4 - tmp < max_len) {
            *dst++ = type;
            *dst++ = type;
            *dst++ = *src;
        }
        /* Otherwise don't copy the code, but replace @@ or &&. */
        else if (mode == CLR_STRIP && dst + 1 - tmp < max_len && *src == type) {
            *dst++ = *src;
            *dst++ = *src;
        }

        ++src;
    }

    /* Make sure it's nul-terminated. */
    *dst = '\0';

    if (in == out) {
        strncpy(out, tmp, max_len - 1);
        free(tmp);
        return MIN(dst - tmp, max_len - 1);
    } else
        return MIN(dst - out, max_len - 1);
}

int count_color_chars(const char *string) {
    int num = 0;

    if (!string || !*string)
        return 0;

    for (; *string; ++string) {
        if (*string == CREL || *string == CABS) {
            ++string;
            ++num;
            if (*string != CREL && *string != CABS)
                ++num;
        }
    }

    return num;
}

int ansi_strlen(const char *string) {
    int num;

    if (!string)
        return 0;

    for (num = 0; *string; ++string) {
        if (*string == CREL || *string == CABS)
            ++string;
        else
            ++num;
    }

    return num;

    /*
       return strlen(string) - count_color_chars(string);
     */
}

char *strip_ansi(const char *string) {
    static char buffer[MAX_STRING_LENGTH];

    process_colors(buffer, sizeof(buffer), string, CLR_STRIP);

    return buffer;
}

char *escape_ansi(const char *string) {
    static char buffer[MAX_STRING_LENGTH];

    process_colors(buffer, sizeof(buffer), string, CLR_ESCAPE);

    return buffer;
}

/***************************************************************************
 *   File: strings.c                                      Part of FieryMUD *
 *  Usage: functions for string management                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "string_utils.hpp"

#include "screen.hpp"
#include "utils.hpp"

#include <algorithm>
#include <ranges>
#include <string>

static struct {
    int visited;
    const char *str;
    size_t totallen;
    const char *ellipsis;
} __ellipsis = {(1 << 3) - 1, nullptr, 0, 0};

const void *ellipsis(const char *str, int maxlen, int mode) {

    if (__ellipsis.visited == (1 << 3) - 1 || __ellipsis.str != str) {
        int clr, vis, i;

        __ellipsis.visited = 0;
        __ellipsis.str = str;

        for (vis = clr = 0; *str; ++str) {
            if (*str == CREL || *str == CABS) {
                ++str;
                ++clr;
                if (*str != CREL && *str != CABS)
                    ++clr;
                else
                    ++vis;
            } else
                ++vis;
            if (vis > maxlen)
                break;
        }

        if (vis > maxlen)
            for (i = 4; i > 0; --i) {
                --str;
                if (*(str - 1) == CREL || *(str - 1) == CABS) {
                    --str;
                    clr -= 2;
                    ++i;
                }
            }

        __ellipsis.totallen = clr + (vis <= maxlen ? maxlen : maxlen - 3);
        __ellipsis.ellipsis = vis > maxlen ? ANRM "..." : ANRM;
    }

    __ellipsis.visited |= mode;

    if (mode == __ELLIPSIS_W1 || mode == __ELLIPSIS_W2)
        return (const void *)__ellipsis.totallen;
    else if (mode == __ELLIPSIS_E)
        return __ellipsis.ellipsis;
    else
        return 0; /* major error */
}

void sprintbit(long bitvector, const char *names[], char *result) {
    long i;
    char *orig_pos = result;

    /* Assuming 8 bits to a byte... */
    for (i = 0; *names[i] != '\n'; i++) {
        if (IS_SET(bitvector, (1 << i))) {
            strcpy(result, names[i]);
            result += strlen(result);
            *(result++) = ' ';
        }
    }

    if (orig_pos == result)
        strcpy(result, "NO BITS");
    else
        *result = '\0'; /* Nul terminate */
}

void sprinttype(int type, const char *names[], char *result) {
    int nr = 0;

    while (type && *names[nr] != '\n') {
        type--;
        nr++;
    }

    if (*names[nr] != '\n')
        strcpy(result, names[nr]);
    else {
        strcpy(result, "UNDEFINED");
        log("SYSERR: Unknown type %d in sprinttype.", type);
    }
}

void sprintflag(char *result, flagvector flags[], int num_flags, const char *names[]) {
    int i, nr = 0;
    char *orig_pos = result;

    for (i = 0; i < num_flags; ++i) {
        if (IS_FLAGGED(flags, i)) {
            if (*names[nr] != '\n')
                strcpy(result, names[nr]);
            else
                strcpy(result, "UNDEFINED");
            result += strlen(result);
            *(result++) = ' ';
        }
        if (*names[nr] != '\n')
            ++nr;
    }

    if (orig_pos == result)
        strcpy(result, "NO FLAGS");
    else
        *(result - 1) = '\0'; /* Nul terminate */
}

int sprintascii(char *out, flagvector bits) {
    int i, j = 0;
    /* 32 bits, don't just add letters to try to get more unless flagvector is
     * also as large. */
    const char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

    for (i = 0; flags[i]; ++i)
        if (bits & (1 << i))
            out[j++] = flags[i];

    if (j == 0) /* Didn't write anything. */
        out[j++] = '0';

    /* Nul terminate the output string. */
    out[j++] = '\0';
    return j;
}

bool is_equals(const std::string_view &lhs, const std::string_view &rhs) {
    auto to_lower{std::ranges::views::transform(::tolower)};
    return std::ranges::equal(lhs | to_lower, rhs | to_lower);
}
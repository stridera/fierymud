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

#include "conf.h"
#include "math.h"
#include "screen.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* str_cmp: a case-insensitive version of strcmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(const char *arg1, const char *arg2) {
    int chk, i;

    for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
        if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) {
            if (chk < 0)
                return (-1);
            else
                return (1);
        }
    return (0);
}

/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(const char *arg1, const char *arg2, int n) {
    int chk, i;

    for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
        if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) {
            if (chk < 0)
                return (-1);
            else
                return (1);
        }
    return (0);
}

#ifndef strnlen
size_t strnlen(register const char *str, register size_t maxlen) {
    const char *end = (const char *)memchr(str, '\0', maxlen);
    return end ? end - str : maxlen;
}
#endif

char *strdupf(const char *fmt, ...) {
    char buf[MAX_STRING_LENGTH];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    return strdup(buf);
}

void sprintbit(long bitvector, const char *names[], char *result) {
    long nr, i;
    char *orig_pos = result;

    /* Assuming 8 bits to a byte... */
    for (i = 0, nr = 0; (i < sizeof(bitvector) * 8) && bitvector; ++i) {
        if (IS_SET(bitvector, (1 << i))) {
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
    else
        strcpy(result, "UNDEFINED");
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
    char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

    for (i = 0; flags[i]; ++i)
        if (bits & (1 << i))
            out[j++] = flags[i];

    if (j == 0) /* Didn't write anything. */
        out[j++] = '0';

    /* Nul terminate the output string. */
    out[j++] = '\0';
    return j;
}

#define MAX_STR_TOKENS 50
#define STR_HASH_BUCKETS 32
#define VALID_STR_TOKEN(tok) ((tok) >= 0 && (tok) < MAX_STR_TOKENS)
static struct str_token {
    char *buf;
    char *pos;
    size_t size;
    str_token next_in_hash;
} str_data[MAX_STR_TOKENS];
static int next_str_token = 0;
static str_token str_hash[STR_HASH_BUCKETS] = {NULL};

#define STR_HASH(addr) (((unsigned int)(addr)) % STR_HASH_BUCKETS)
#define SPACE_USED(tok) ((tok)->pos - (tok)->buf + 1)
#define SPACE_LEFT(tok) ((tok)->size - SPACE_USED(tok))

static void kill_str_token(char *buf) {
    str_token x, tok;
    for (x = str_hash[STR_HASH(buf)]; x; x = x->next_in_hash) {
        if (x->buf == buf) {
            if (x == str_hash[STR_HASH(buf)])
                str_hash[STR_HASH(buf)] = x->next_in_hash;
            else
                for (tok = str_hash[STR_HASH(buf)]; tok && tok->next_in_hash; tok = tok->next_in_hash)
                    if (tok->next_in_hash == x)
                        tok->next_in_hash = x->next_in_hash;
            break;
        }
    }
}

static str_token new_str_token(char *buf, size_t max_size) {
    str_token tok;
    str_token new_token = &str_data[(next_str_token++) % MAX_STR_TOKENS];

    kill_str_token(buf);
    kill_str_token(new_token->buf);

    new_token->buf = buf;
    new_token->pos = buf;
    new_token->size = max_size;
    new_token->next_in_hash = NULL;

    for (tok = str_hash[STR_HASH(buf)]; tok && tok->next_in_hash; tok = tok->next_in_hash)
        ;
    if (tok)
        tok->next_in_hash = new_token;
    else
        str_hash[STR_HASH(buf)] = new_token;

    return new_token;
}

static str_token find_str_token(char *buf) {
    str_token tok;
    for (tok = str_hash[STR_HASH(buf)]; tok; tok = tok->next_in_hash)
        if (tok->buf == buf)
            return tok;
    return NULL;
}

static void str_update(str_token token) {
    if (*token->pos)
        token->pos += strlen(token->pos);
}

str_token str_start(char *buf, size_t max_size) {
    *buf = '\0';
    return new_str_token(buf, max_size);
}

size_t str_space(char *buf) {
    str_token token = find_str_token(buf);
    str_update(token);
    return token ? SPACE_LEFT(token) : 0;
}

void str_cat(char *buf, const char *str) {
    str_token token = find_str_token(buf);

    if (token) {
        str_update(token);
        if (SPACE_LEFT(token) > 0) {
            /* Using strncat because strncpy is stupid */
            strncat(token->pos, str, SPACE_LEFT(token));
            str_update(token);
        }
    } else
        strcat(buf, str);
}

void strn_cat(char *buf, const char *str, size_t size) {
    str_token token = find_str_token(buf);

    if (token) {
        str_update(token);
        if (SPACE_LEFT(token) > 0) {
            /* Using strncat because strncpy is stupid */
            strncat(token->pos, str, MIN(SPACE_LEFT(token), size));
            str_update(token);
        }
    } else
        strncat(buf, str, size);
}

void str_catf(char *buf, const char *format, ...) {
    va_list args;
    str_token token = find_str_token(buf);
    int size;

    va_start(args, format);

    if (token) {
        str_update(token);
        if (SPACE_LEFT(token) > 0) {
            size = vsnprintf(token->pos, SPACE_LEFT(token), format, args);
            token->pos += (size > SPACE_LEFT(token) ? SPACE_LEFT(token) : size);
        }
    } else {
        buf += strlen(buf);
        vsprintf(buf, format, args);
    }

    va_end(args);
}

void strn_catf(char *buf, size_t size, const char *format, ...) {
    va_list args;
    str_token token = find_str_token(buf);
    int printed;

    va_start(args, format);

    if (token) {
        str_update(token);
        size = MIN(SPACE_LEFT(token), size);
        if (size > 0) {
            printed = vsnprintf(token->pos, size, format, args);
            token->pos += (printed > size ? size : printed);
        }
    } else {
        buf += strlen(buf);
        vsnprintf(buf, size, format, args);
    }

    va_end(args);
}

char *str_end(char *buf) {
    str_token token = find_str_token(buf);
    if (!token)
        return buf + strlen(buf);
    str_update(token);
    return token->pos;
}

static struct {
    int visited;
    const char *str;
    int totallen;
    const char *ellipsis;
} __ellipsis = {(1 << 3) - 1, NULL, 0, 0};

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
        return (void *)__ellipsis.totallen;
    else if (mode == __ELLIPSIS_E)
        return __ellipsis.ellipsis;
    else
        return 0; /* major error */
}

/* Return pointer to first occurrence in string ct in */
/* cs, or NULL if not present.  Case insensitive */
const char *str_str(const char *cs, const char *ct) {
    const char *s, *t;

    if (!cs || !ct)
        return NULL;

    while (*cs) {
        t = ct;

        while (*cs && (LOWER(*cs) != LOWER(*t)))
            cs++;

        s = cs;

        while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
            t++;
            cs++;
        }

        if (!*t)
            return s;
    }

    return NULL;
}

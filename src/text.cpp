/***************************************************************************
 *   File: text.c                                         Part of FieryMUD *
 *  Usage: functions for text management                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "text.hpp"

#include "conf.hpp"
#include "editor.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#define MEGABUF_SIZE 100000
static char megabuf[MEGABUF_SIZE]; /* Arbitrary large size */
static char *megabuf_tail = nullptr;
#define MEGABUF_LEFT (&megabuf[MEGABUF_SIZE - 1] - megabuf_tail)

/*
 * Returns a pointer to a character buffer which is guaranteed
 * to contain no fewer than MAX_STRING_LENGTH characters.
 *
 * megabuf_get will return the same pointer until megabuf_used
 * or megabuf_done is called.
 *
 * WARNING: megabuf_get is NOT re-entrant.  You must take care not
 * to call a function that will call megabuf_get before you call
 * megabuf_used or megabuf_done.  (Calling megabuf_get "erases" any
 * prior calls to megabuf_get.)
 */
static char *megabuf_get(void) {
    /* Initialize megabuf tail if necessary */
    if (!megabuf_tail)
        megabuf_tail = megabuf;

    /* Less than MAX_STRING_LENGTH remains - reset */
    if (MEGABUF_LEFT < MAX_STRING_LENGTH)
        megabuf_tail = megabuf;

    /* Initialize string */
    *megabuf_tail = '\0';
    return megabuf_tail;
}

/*
 * 'Release' the megabuf pointer used.  Callers of megabuf_get must
 * call either this or megabuf_done before they return to their
 * callers.  To signify that whatever space was 'allocated' with
 * megabuf_get, users can call megabuf_used(0).
 */
static void megabuf_used(int length) {
    /* Negative length?  Could mean the caller is giving us a value
     * directly from a standard library function.  Since we don't know
     * what else to do, release a zero-length string and move on
     */
    if (length < 0)
        *(megabuf_tail++) = '\0';
    else
        megabuf_tail += length + 1;
}

/*
 * 'Release' the megabuf pointer used.  Automatically determines
 * how much space was used by calculating the string length.
 */
static void megabuf_done() { megabuf_tail += strlen(megabuf_tail) + 1; }

void smash_tilde(char *str) {
    /* Erase any _line ending_ tildes inserted in the editor.  The load
     * mechanism can't handle those yet. -Welcor */
    char *p = str;
    for (; *p; p++)
        if (*p == '~' && (*(p + 1) == '\r' || *(p + 1) == '\n' || *(p + 1) == '\0'))
            *p = ' ';
}

/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements or -1 if none were made
 * due to lack of space */
int replace_str(char **string, const char *pattern, const char *replacement, int rep_all, int max_size) {
    char *replace_buffer = nullptr;
    char *flow, *jetsam, temp;
    int len, i;

    if ((int)((strlen(*string) - strlen(pattern)) + strlen(replacement)) >= max_size)
        return -1;

    CREATE(replace_buffer, char, max_size + 1);
    i = 0;
    jetsam = *string;
    flow = *string;
    *replace_buffer = '\0';
    if (rep_all) {
        while ((flow = (char *)strcasestr(flow, pattern)) != nullptr) {
            if ((int)((strlen(replace_buffer) + strlen(jetsam) - strlen(pattern) + strlen(replacement))) > max_size) {
                i = -1;
                break;
            }
            i++;
            temp = *flow;
            *flow = '\0';
            strcat(replace_buffer, jetsam);
            strcat(replace_buffer, replacement);
            *flow = temp;
            flow += strlen(pattern);
            jetsam = flow;
        }
        strcat(replace_buffer, jetsam);
    } else {
        if ((flow = (char *)strcasestr(*string, pattern)) != nullptr) {
            i++;
            flow += strlen(pattern);
            len = ((char *)flow - (char *)*string) - strlen(pattern);

            strncpy(replace_buffer, *string, len);
            strcat(replace_buffer, replacement);
            strcat(replace_buffer, flow);
        }
    }
    if (i > 0) {
        RECREATE(*string, char, strlen(replace_buffer) + 3);
        strcpy(*string, replace_buffer);
    }
    free(replace_buffer);
    return i;
}

/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(char **ptr_string, int mode, DescriptorData *d, int maxlen) {
    int total_chars, cap_next = true, cap_next_next = false;
    char *flow, *start = nullptr, temp;
    /* warning: do not edit messages with maxlen > MAX_STRING_LENGTH */
    char *formatted = megabuf_get();

    flow = *ptr_string;
    if (!flow)
        return;

    if (IS_SET(mode, FORMAT_INDENT)) {
        strcpy(formatted, "   ");
        total_chars = 3;
    } else {
        *formatted = '\0';
        total_chars = 0;
    }

    while (*flow != '\0') {
        while ((*flow == '\n') || (*flow == '\r') || (*flow == '\f') || (*flow == '\t') || (*flow == '\v') ||
               (*flow == ' '))
            flow++;

        if (*flow != '\0') {

            start = flow++;
            while ((*flow != '\0') && (*flow != '\n') && (*flow != '\r') && (*flow != '\f') && (*flow != '\t') &&
                   (*flow != '\v') && (*flow != ' ') && (*flow != '.') && (*flow != '?') && (*flow != '!'))
                flow++;

            if (cap_next_next) {
                cap_next_next = false;
                cap_next = true;
            }

            /* this is so that if we stopped on a sentance .. we move off the sentance
             * delim. */
            while ((*flow == '.') || (*flow == '!') || (*flow == '?')) {
                cap_next_next = true;
                flow++;
            }

            temp = *flow;
            *flow = '\0';

            if ((total_chars + strlen(start) + 1) > 79) {
                strcat(formatted, "\n");
                total_chars = 0;
            }

            if (!cap_next) {
                if (total_chars > 0) {
                    strcat(formatted, " ");
                    total_chars++;
                }
            } else {
                cap_next = false;
                *start = UPPER(*start);
            }

            total_chars += strlen(start);
            strcat(formatted, start);

            *flow = temp;
        }

        if (cap_next_next) {
            if ((total_chars + 3) > 79) {
                strcat(formatted, "\n");
                total_chars = 0;
            } else {
                strcat(formatted, "  ");
                total_chars += 2;
            }
        }
    }

    /* Trim trailing spaces, + possible extraneous newline. */
    flow = formatted + strlen(formatted) - 1;
    while (flow > formatted && isspace(*flow))
        *(flow--) = 0;

    strcat(formatted, "\n");

    if ((int)(strlen(formatted)) > maxlen)
        formatted[maxlen] = '\0';
    RECREATE(*ptr_string, char, std::min<int>(maxlen + 1, strlen(formatted) + 3));
    strcpy(*ptr_string, formatted);
}

/* strips \r's from line */
char *stripcr(char *dest, const char *src) {
    int i, length;
    char *temp;

    if (!dest || !src)
        return nullptr;
    temp = &dest[0];
    length = strlen(src);
    for (i = 0; *src && (i < length); i++, src++)
        if (*src != '\r')
            *(temp++) = *src;
    *temp = '\0';
    return dest;
}

/* Returns a duplicate of the next line of text in src, up to the
 * newline.  Moves src to the beginning of the line after that.
 * Returns NULL if there is no more text. */
char *next_line(char **src) {
    char *l, *s = *src;
    int len = 0;

    if (!*s)
        return nullptr;

    for (len = 0; *s && *s != '\r' && *s != '\n'; s++, len++)
        ;

    l = (char *)malloc(len + 1);
    memcpy(l, *src, len);
    l[len] = '\0';

    while (*s && (*s == '\r' || *s == '\n'))
        s++;
    *src = s;

    return l;
}

char *cap_by_color(char *s) {
    char *b = s;

    while (*b && (*b == CREL || *b == CABS))
        b += 2;

    if (*b)
        *b = UPPER(*b);

    return s;
}

const char *capitalize(const char *s) {
    char *b = megabuf_get();
    strncat(b, s, MAX_STRING_LENGTH - 1);
    cap_by_color(b);
    megabuf_done();
    return b;
}

bool isplural(const char *namelist) {
    const char *point;

    for (point = namelist; *point && *point != ' '; point++)
        ;

    if (point == namelist || *(point - 1) != 's')
        return false;

    return true;
}

bool startsvowel(const char *s) {
    while (*s) {
        if (*s == CREL || *s == CABS) {
            s++;
            if (*s == '\0')
                return 0;
            s++;
        } else if (*s == 'A' || *s == 'a' || *s == 'E' || *s == 'e' || *s == 'I' || *s == 'i' || *s == 'O' ||
                   *s == 'o' || *s == 'U' || *s == 'u')
            return 1;
        else
            return 0;
    }
    return 0;
}

/* Return a string that has the given string, with "a " or "an " prepended,
 * depending on whether the given string starts with a vowel.
 *
 * This is intended for short strings like names, which is why we use a
 * somewhat small buffer.  The returned value should be used immediately
 * and not modified, as it's a static buffer here.  If the passed string
 * is too long for the static buffer, the returned string will be truncated.
 */

char *with_indefinite_article(const char *s) {
    const char *t;
    char *u, *ret, fcbuf[2];
    bool isvowel = false;

    /* Find the first alphabetical character in the string, skipping
     * ansi codes.  Even though it will skip over spaces here, you should
     * not pass a string with any leading spaces, because the result will
     * have too many. */
    t = s;
    while (*t && !isalpha(*t)) {
        if (*t == CREL || *t == CABS) {
            t++;
            if (!*t)
                break;
        }
        t++;
    }

    /* Create a string of length one to pass to strcasestr(). */
    if (*t) {
        fcbuf[0] = *t;
        fcbuf[1] = '\0';
        if (strcasestr("AEIOUaeiou", fcbuf))
            isvowel = true;
    }

    /* Construct the returned string.  Start with the article + a space. */
    u = megabuf_get();
    ret = u;
    *u++ = 'a';
    if (isvowel)
        *u++ = 'n';
    *u++ = ' ';

    /* Append the provided string to the returned string. */
    t = s;
    while (*t && u - ret < MAX_STRING_LENGTH - 1)
        *u++ = *t++;
    *u = '\0';

    megabuf_used(u - ret);

    return ret;
}

/* without_article()
 *
 * Returns a string which matches the given string, except that any
 * leading article has been removed.
 */

const char *without_article(const char *s) {
    /* - end this list with "\n"
     * - each entry must have a trailing space
     * - if an entry is the same as the start of another entry,
     *   then the longer one must come first (e.g., "a pair " must
     *   come before "a ").
     */
    const char *articles[] = {"a pair ", "a ", "an ", "some ", "the ", "\n"};

    int i;

    for (i = 0; articles[i][0] != '\n'; i++) {
        if (!strncasecmp(articles[i], s, strlen(articles[i]))) {
            return s + strlen(articles[i]);
        }
    }
    return s;
}

#define IS_LETTER(c) (isalpha(c))
#define IS_VOWEL(c)                                                                                                    \
    (tolower(c) == 'a' || tolower(c) == 'e' || tolower(c) == 'i' || tolower(c) == 'o' || tolower(c) == 'u')
#define IS_CONSONANT(c) (IS_LETTER(c) && !IS_VOWEL(c))

/* Create a plural of a single word.
 * You could pass an entire phrase if you wanted.
 * This function will only look at the last word in the string.
 */
const char *word_pluralize(const char *s) {
    static char u[MAX_STRING_LENGTH];
    int len, newlen;

    /* Apply comon rules */

    len = strlen(s);
    /* We will need to add up to 2 characters on the end */
    newlen = std::min(MAX_STRING_LENGTH - 3, len);
    strncpy(u, without_article(s), newlen);
    u[newlen] = '\0';
    len = strlen(u);

    /* Single character: add 's (and avoid errors with u[len - 2] below) */
    if (len < 2) {
        u[len] = '\'';
        u[len + 1] = 's';
        u[len + 2] = '\0';
        return u;
    }

    /* Ending in ez or iz: add zes */
    if ((u[len - 2] == 'e' || u[len - 2] == 'i') && u[len - 1] == 'z') {
        u[len] = 'z';
        u[len + 1] = 'e';
        u[len + 2] = 's';
        u[len + 3] = '\0';
        return u;
    }

    /* Ending in: x, z (except ez or iz, handled above), sh, ss, ch, <C>o, <C>y:
     * add es */
    if (u[len - 1] == 'x' || u[len - 1] == 'z' || (u[len - 2] == 's' && u[len - 1] == 'h') ||
        (u[len - 2] == 's' && u[len - 1] == 's') || (u[len - 2] == 'c' && u[len - 1] == 'h') ||
        (IS_CONSONANT(u[len - 2]) && u[len - 1] == 'o') || (IS_CONSONANT(u[len - 2]) && u[len - 1] == 'y')) {
        u[len] = 'e';
        u[len + 1] = 's';
        u[len + 2] = '\0';
        if (u[len - 1] == 'y')
            u[len - 1] = 'i'; /* -y = -ies */
        return u;
    }

    /* Other: add s */
    u[len] = 's';
    u[len + 1] = '\0';

    return u;
}

const char *pluralize(const char *s) {
    char *u;
    const char *word;
    int i, spaces, spacepos[2], newlen;

    u = megabuf_get();

    /* Begin analyzing the given string */

    /* First deal with "some" */
    if (!strncasecmp("some ", s, strlen("some "))) {
        megabuf_used(sprintf(u, "quantities of %s", s + strlen("some ")));
        return u;
    }

    /* Then look for "<article> <name> OF <...>" */
    spaces = 0;
    for (i = 0; i < MAX_STRING_LENGTH && spaces < 2 && s[i]; i++) {
        if (s[i] == ' ') {
            spacepos[spaces] = i;
            spaces++;
        }
    }
    if (spaces == 2 && s[i - 1] == ' ' && s[i] == 'o' && s[i + 1] == 'f' && s[i + 2] == ' ') {
        /* We have <article> <name> OF */
        /* Such as "a handful of sand" */
        strncpy(u, s + spacepos[0] + 1, spacepos[1] - spacepos[0] - 1); /* "handful" */
        u[spacepos[1] - spacepos[0] - 1] = '\0';
        word = word_pluralize(u);                                     /* "handfuls" */
        megabuf_used(sprintf(u, "%s %s", word, s + spacepos[1] + 1)); /* "handfuls of sand" */
        return u;
        /* "a ring of fire" -> "rings of fire" */
    }

    /* Finally just apply common rules */
    word = word_pluralize(s);
    newlen = std::min<int>(MAX_STRING_LENGTH - 1, strlen(word));
    strncpy(u, word, newlen);
    u[newlen] = '\0';
    megabuf_used(newlen);
    return u;
}

int levenshtein_distance(const char *s1, const char *s2) {
    int s1_len = strlen(s1), s2_len = strlen(s2);
    int **d, i, j;

    CREATE(d, int *, s1_len + 1);

    for (i = 0; i <= s1_len; ++i) {
        CREATE(d[i], int, s2_len + 1);
        d[i][0] = i;
    }

    for (j = 0; j <= s2_len; ++j)
        d[0][j] = j;

    for (i = 1; i <= s1_len; ++i)
        for (j = 1; j <= s2_len; ++j)
            d[i][j] = std::min(d[i - 1][j] + 1,
                               std::min(d[i][j - 1] + 1, d[i - 1][j - 1] + ((s1[i - 1] == s2[j - 1]) ? 0 : 1)));

    i = d[s1_len][s2_len];

    for (j = 0; j <= s1_len; ++j)
        free(d[j]);
    free(d);

    return i;
}

/*
 * This trims spaces in place, shifting the string to the left if necessary.
 */
void trim_spaces(char *buffer) {
    char *pos;

    if (!buffer)
        return;

    /* Trim leading spaces */
    if (isspace(*buffer)) {
        char *wpos = buffer;
        pos = buffer;
        while (isspace(*pos))
            ++pos;
        while (*pos)
            *(wpos++) = *(pos++);
        *wpos = '\0';
    }

    /* Trim trailing spaces */
    pos = buffer + strlen(buffer) - 1;
    while (pos > buffer && isspace(*pos))
        *(pos--) = '\0';
}

char *strip_chars(char *str, const char *chars) { return filter_chars(str, str, chars); }

char *filter_chars(char *buf, const char *src, const char *chars) {
    char *dest = buf;

    if (!src)
        return "";

    while (*dest = *src) {
        if (!strchr(chars, *dest))
            ++dest;
        ++src;
    }

    return buf;
}

/*
 * Takes a pointer to a const string, then skips over any and all
 * characters contained in the skip string.  Returns a pointer to
 * the first character not contained in the skip string.
 */
const char *skip_over(const char *string, const char *skip) {
    while (*string && strchr(skip, *string))
        ++string;
    return string;
}

/*
 * Takes a pointer to a const string, then skips over every
 * character in the string equal to the skip character.  Returns
 * a pointer to the first character not equal to the skip character.
 */
const char *skip_chars(const char *string, char skip) {
    while (*string && *string == skip)
        ++string;
    return string;
}

/*
 * Gets the first word from the input string and copies it to
 * the specified buffer.  Skips any initial whitespace.  Returns
 * a pointer to the first character after the first word.
 * Similar to any_one_arg, but takes a const input string and
 * returns a similarly const pointer.
 */
const char *fetch_word(const char *string, char *buf, size_t buf_space) {
    string = skip_over(string, S_WHITESPACE);

    while (*string && !strchr(S_WHITESPACE, *string) && --buf_space > 0)
        *(buf++) = *(string++);

    *buf = '\0';

    return string;
}

/*
 * Create a new screen buffer.
 */
ScreenBuf *new_screen_buf(void) {
    ScreenBuf *sb;
    CREATE(sb, ScreenBuf, 1);
    sb->buf = nullptr;
    sb->lines = nullptr;
    sb->line_width = ED_DEFAULT_PAGE_WIDTH;
    return sb;
}

/*
 * Increase the line cache buffer by <how_much> lines.
 */
static void sb_increase_lines(ScreenBuf *sb, size_t how_much) {
    if (sb->lines) {
        RECREATE(sb->lines, char *, sb->line_capacity + how_much);
        sb->line_capacity += how_much;
    } else if (how_much) {
        CREATE(sb->lines, char *, how_much);
        sb->line_capacity = how_much;
    } else {
        CREATE(sb->lines, char *, SB_INITIAL_LINES_CAP);
        sb->line_capacity = SB_INITIAL_LINES_CAP;
    }
}

/*
 * Count the lines in the buffer, and cache them.
 */
static void sb_compute_lines(ScreenBuf *sb) {
    char *p = sb->buf;
    size_t line = 0;

    if (!sb->buf)
        return;

    while (*p) {
        if (line >= sb->line_capacity)
            sb_increase_lines(sb, sb->line_capacity);
        sb->lines[line++] = p;
        while (*p && *p != '\n')
            ++p;
    }

    sb->line_count = line;
}

/*
 * Increase the string buffer by <how_much> characters.
 * Callers should be aware that if the buffer is external,
 * no change will be made, so callers should check the
 * capacity afterwards.
 */
static void sb_increase_buffer(ScreenBuf *sb, size_t how_much) {
    if (!IS_FLAGGED(sb->flags, SB_EXTERNAL)) {
        if (sb->buf) {
            char *buf = sb->buf;
            RECREATE(sb->buf, char, sb->capacity + how_much);
            sb->capacity += how_much;
            if (buf != sb->buf)
                sb_compute_lines(sb);
        } else if (how_much) {
            CREATE(sb->buf, char, how_much);
            sb->capacity = how_much;
        } else {
            CREATE(sb->buf, char, SB_INITIAL_BUF_CAP);
            sb->capacity = SB_INITIAL_BUF_CAP;
        }
    }
}

/*
 * Figure out line endings for the buffer starting at line <start_line>.
 */
static void sb_compile_lines(ScreenBuf *sb, size_t start_line) {
    char *copy, *read, *write, *start, temp, *next;
    size_t line = start_line;
    size_t line_len = 0;
    bool cap_next = true, cap_next_next = false, hyphenated = false, was_hyphenated = false;
    unsigned int periods, non_periods;

    if (!sb->buf)
        return;

    copy = strdup(sb->lines[start_line]);
    read = copy;
    write = sb->lines[start_line];

    while (line_len < sb->first_indent) {
        *(write++) = ' ';
        ++line_len;
    }

    while (*read) {
        /* Skip whitespace */
        while (isspace(*read))
            ++read;

        if (*read) {
            start = read++;

            if (hyphenated) {
                hyphenated = false;
                was_hyphenated = true;
            }

            while (*read && !isspace(*read) && *read != '-' && *read != '.' && *read != '?' && *read != '!')
                ++read;

            /* Read ahead to determine if the next token is a single
             * punctuation mark.  If it is, collapse whitespace between
             * current word and the punctuation.
             */
            next = read;
            while (*next && isspace(*next))
                ++next;
            if (isspace(*(next + 1))) {
                if (*next == ',' || *next == ':' || *next == ';' || *next == '.' || *next == '?' || *next == '!') {
                    *(read++) = *next;
                    *next = ' ';
                }
            }

            if (cap_next_next) {
                cap_next_next = false;
                cap_next = true;
            }

            periods = 0;
            non_periods = 0;
            /* If we stopped at the end of a sentence, move off the delimiter */
            while (*read == '.' || *read == '!' || *read == '?') {
                if (*read == '.')
                    ++periods;
                else
                    ++non_periods;
                cap_next_next = true;
                ++read;
            }
            /* Next char is a quote?  Include in this 'word'. */
            if ((*read == '\'' || *read == '"') && *(read + 1) == ' ')
                ++read;
            /* Only periods?  Probably an ellipsis (...) so don't capitalize */
            else if (periods >= 3 && non_periods == 0)
                cap_next_next = false;

            while (*read == '-') {
                hyphenated = true;
                ++read;
            }

            temp = *read;
            *read = '\0';

            /* If the word is too long to fit on this line, bump it to the
             * next one. */
            if (line_len + strlen(start) + 1 > sb->line_width - 1) {
                if (write - sb->buf + 2 + sb->other_indent >= sb->capacity - 1) {
                    size_t offset = write - sb->buf;
                    sb_increase_buffer(sb, sb->capacity);
                    write = sb->buf + offset;
                }
                strcpy(write, "\n");
                write += 2;
                line_len = 0;

                if (++line >= sb->line_capacity)
                    sb_increase_lines(sb, sb->line_capacity);
                sb->lines[line] = write;
                sb->line_count = line + 1;

                /* Indent */
                while (line_len < sb->other_indent) {
                    *(write++) = ' ';
                    ++line_len;
                }
            }

            if (cap_next) {
                cap_next = false;
                if (IS_FLAGGED(sb->flags, SB_USE_CAPS))
                    *start = UPPER(*start);
            }
            /* Don't print a leading space if the last word printed was
             * hyphenated.
             */
            else if (was_hyphenated)
                was_hyphenated = false;
            else if (line_len > 0) {
                if (write - sb->buf + 1 >= sb->capacity - 1) {
                    size_t offset = write - sb->buf;
                    sb_increase_buffer(sb, sb->capacity);
                    write = sb->buf + offset;
                }
                *(write++) = ' ';
                ++line_len;
            }

            if (write - sb->buf + strlen(start) >= sb->capacity - 1) {
                size_t offset = write - sb->buf;
                sb_increase_buffer(sb, sb->capacity);
                write = sb->buf + offset;
            }
            strcpy(write, start);
            write += strlen(start);
            line_len += ansi_strlen(start);

            *read = temp;
        }

        if (cap_next_next) {
            if (line_len + 3 > sb->line_width - 1) {
                if (write - sb->buf + 2 >= sb->capacity - 1) {
                    size_t offset = write - sb->buf;
                    sb_increase_buffer(sb, sb->capacity);
                    write = sb->buf + offset;
                }
                strcpy(write, "\n");
                write += 2;
                line_len = 0;

                if (++line >= sb->line_capacity)
                    sb_increase_lines(sb, sb->line_capacity);
                sb->lines[line] = write;
                sb->line_count = line + 1;

                /* Indent */
                while (line_len < sb->other_indent) {
                    *(write++) = ' ';
                    ++line_len;
                }
            } else {
                if (write - sb->buf + 2 >= sb->capacity - 1) {
                    size_t offset = write - sb->buf;
                    sb_increase_buffer(sb, sb->capacity);
                    write = sb->buf + offset;
                }
                strcpy(write, "  ");
                write += 2;
                line_len += 2;
            }
        }
    }

    *write = '\0';

    /* Trim trailing spaces and extraneous newlines. */
    --write;
    while (write > sb->buf && isspace(*write)) {
        if (*write == '\n' && write - 1 > sb->buf && *(write - 1) == '\r')
            sb->line_count--;
        *(write--) = '\0';
    }

    free(copy);
}

/*
 *
 */
void sb_use_buf(ScreenBuf *sb, char *buf, size_t buf_capacity) {
    *buf = '\0';
    if (sb->buf) {
        strncat(buf, sb->buf, buf_capacity); /* strncpy is stupid */
        if (IS_FLAGGED(sb->flags, SB_EXTERNAL))
            free(sb->buf);
    }
    sb->buf = buf;
    sb->capacity = buf_capacity;
    SET_FLAG(sb->flags, SB_EXTERNAL);
    sb_compute_lines(sb);
}

const char *sb_get_buffer(ScreenBuf *sb) { return sb->buf; }

size_t sb_get_capacity(ScreenBuf *sb) { return sb->capacity; }

size_t sb_get_length(ScreenBuf *sb) { return sb->length; }

size_t sb_get_lines(ScreenBuf *sb) { return sb->line_count; }

size_t sb_get_width(ScreenBuf *sb) { return sb->line_width; }

size_t sb_get_first_indentation(ScreenBuf *sb) { return sb->first_indent; }

size_t sb_get_other_indentation(ScreenBuf *sb) { return sb->other_indent; }

const char *sb_get_line(ScreenBuf *sb, size_t line) { return sb->lines[line]; }

void sb_set_width(ScreenBuf *sb, size_t columns) {
    if (sb->line_width != columns) {
        sb->line_width = columns;
        sb_compile_lines(sb, 0);
    }
}

void sb_set_first_indentation(ScreenBuf *sb, size_t indentation) {
    if (sb->first_indent != indentation) {
        sb->first_indent = indentation;
        sb_compile_lines(sb, 0);
    }
}

void sb_set_other_indentation(ScreenBuf *sb, size_t indentation) {
    if (sb->other_indent != indentation) {
        sb->other_indent = indentation;
        sb_compile_lines(sb, 0);
    }
}

void sb_use_capitalization(ScreenBuf *sb, bool use_caps) {
    bool same = (IS_FLAGGED(sb->flags, SB_USE_CAPS) == use_caps);
    if (use_caps)
        SET_FLAG(sb->flags, SB_USE_CAPS);
    else
        REMOVE_FLAG(sb->flags, SB_USE_CAPS);
    if (!same && use_caps)
        sb_compile_lines(sb, 0);
}

void sb_append(ScreenBuf *sb, const char *msg, ...) {
    va_list args;
    char *buffer = megabuf_get();
    size_t len;

    /* Prefix with space */
    *buffer = ' ';

    va_start(args, msg);
    vsnprintf(buffer + 1, MAX_STRING_LENGTH - 2, msg, args);
    va_end(args);

    /* No need to call megabuf_done/megabuf_used because we're not
     * returning a reference to any callers and, more importantly, we're
     * not calling any other functions that use the megabuf.
     */

    if (!sb->buf) {
        sb_increase_lines(sb, 0);
        sb_increase_buffer(sb, 0);
        sb->lines[0] = sb->buf;
        sb->line_count = 1;
    }

    for (len = strlen(buffer) + 1; len + sb->length >= sb->capacity; sb_increase_buffer(sb, sb->capacity))
        ;

    strcat(sb->buf, buffer);

    sb_compile_lines(sb, sb->line_count - 1);
}

void free_screen_buf(ScreenBuf *sb) {
    if (!IS_FLAGGED(sb->flags, SB_EXTERNAL))
        free(sb->buf);
    free(sb->lines);
    free(sb);
}

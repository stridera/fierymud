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
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* re-formats message type formatted std::string_view  */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(std::string_view flow, int mode, DescriptorData *d, int maxlen) {
    int total_chars, cap_next = true, cap_next_next = false;
    std::string_view flow, start, temp;
    /* warning: do not edit messages with maxlen > MAX_STRING_LENGTH */
    std::string formatted;

    if (flow.empty())
        return;

    if (IS_SET(mode, FORMAT_INDENT)) {
        formatted = "   ";
    }

    while (!flow.empty()) {
        while (!flow.empty() && std::isspace(flow.front()))
            flow.remove_prefix(1);

        if (!flow.empty()) {
            start = flow;
            while (!flow.empty() && !std::isspace(flow.front()) && flow.front() != '.' && flow.front() != '?' &&
                   flow.front() != '!')
                flow.remove_prefix(1);

            if (cap_next_next) {
                cap_next_next = false;
                cap_next = true;
            }

            while (!flow.empty() && (flow.front() == '.' || flow.front() == '!' || flow.front() == '?')) {
                cap_next_next = true;
                flow.remove_prefix(1);
            }

            temp = flow;
            flow = flow.substr(0, 0); // Temporarily set flow to empty to use start as a null-terminated string

            if ((total_chars + start.size() + 1) > 79) {
                formatted += "\n";
                total_chars = 0;
            }

            if (!cap_next) {
                if (total_chars > 0) {
                    formatted += " ";
                    total_chars++;
                }
            } else {
                cap_next = false;
                formatted += std::toupper(start.front());
                start.remove_prefix(1);
            }

            total_chars += start.size();
            formatted += start;

            flow = temp;
        }

        if (cap_next_next) {
            if ((total_chars + 3) > 79) {
                formatted += "\n";
                total_chars = 0;
            } else {
                formatted += "  ";
                total_chars += 2;
            }
        }
    }

    // Trim trailing spaces and possible extraneous newline.
    while (!formatted.empty() && std::isspace(formatted.back()))
        formatted.pop_back();

    if (static_cast<int>(formatted.size()) > maxlen)
        formatted.resize(maxlen);

    d->str = formatted;
}

/* strips \r's from line */
std::string stripcr(const std::string_view src) {
    std::string dest;
    dest.reserve(src.size());

    for (char c : src) {
        if (c != '\r') {
            dest.push_back(c);
        }
    }

    return dest;
}

bool isplural(const std::string_view namelist) {
    auto point = namelist.find(' ');

    if (point == std::string_view::npos || namelist[point - 1] != 's')
        return false;

    return true;
}

bool startsvowel(const std::string_view s) {
    auto it = s.begin();
    while (it != s.end()) {
        if (*it == CREL || *it == CABS) {
            ++it;
            if (it == s.end())
                return false;
            ++it;
        } else if (std::string_view("AEIOUaeiou").find(*it) != std::string_view::npos) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

/* Return a string that has the given string, with "a " or "an " prepended,
 * depending on whether the given string starts with a vowel.
 */

std::string with_indefinite_article(const std::string_view s) {
    if (startsvowel(s))
        return std::string("an ") + std::string(s);
    else
        return std::string("a ") + std::string(s);
}

/* without_article()
 *
 * Returns a string which matches the given string, except that any
 * leading article has been removed.
 */

std::string without_article(const std::string_view s) {
    /* - end this list with "\n"
     * - each entry must have a trailing space
     * - if an entry is the same as the start of another entry,
     *   then the longer one must come first (e.g., "a pair " must
     *   come before "a ").
     */
    const std::string_view articles[] = {"a pair ", "a ", "an ", "some ", "the ", "\n"};

    for (int i = 0; articles[i][0] != '\n'; i++) {
        if (matches(articles[i], s)) {
            return std::string(s.substr(articles[i].size()));
        }
    }
    return std::string(s);
}

#define IS_LETTER(c) (isalpha(c))
#define IS_VOWEL(c)                                                                                                    \
    (tolower(c) == 'a' || tolower(c) == 'e' || tolower(c) == 'i' || tolower(c) == 'o' || tolower(c) == 'u')
#define IS_CONSONANT(c) (IS_LETTER(c) && !IS_VOWEL(c))

/* Create a plural of a single word.
 * You could pass an entire phrase if you wanted.
 * This function will only look at the last word in the string.
 */
std::string_view word_pluralize(const std::string_view s) {
    std::string u;
    int len = s.size();

    /* Apply comon rules */

    u = without_article(s);

    /* Single character: add 's (and avoid errors with u[len - 2] below) */
    if (len < 2) {
        return u + "\'s";
    }

    /* Ending in ez or iz: add zes */
    if ((u[len - 2] == 'e' || u[len - 2] == 'i') && u[len - 1] == 'z') {
        u.append("zes");
        return u;
    }

    /* Ending in: x, z (except ez or iz, handled above), sh, ss, ch, <C>o, <C>y: add es */
    char last = u[len - 1];
    char second_last = u[len - 2];

    bool ends_with_special_case =
        (last == 'x' || last == 'z' || (second_last == 's' && last == 'h') || (second_last == 's' && last == 's') ||
         (second_last == 'c' && last == 'h') || (IS_CONSONANT(second_last) && last == 'o') ||
         (IS_CONSONANT(second_last) && last == 'y'));

    if (ends_with_special_case) {
        if (last == 'y') {
            u[len - 1] = 'i'; /* -y = -ies */
            u.append("es");
        } else {
            u.append("es");
        }
        return u;
    }

    /* Other: add s */
    u += 's';

    return u;
}

std::string pluralize(const std::string_view s) {
    std::string u;
    std::string_view word;
    int spaces = 0;
    std::array<int, 2> spacepos;

    /* Begin analyzing the given string */

    /* First deal with "some" */
    if (matches("some ", s)) {
        return fmt::format("quantities of {}", s.substr(strlen("some ")));
    }

    /* Then look for "<article> <name> OF <...>" */
    for (size_t i = 0; i < s.size() && spaces < 2; ++i) {
        if (s[i] == ' ') {
            spacepos[spaces] = i;
            ++spaces;
        }
    }
    if (spaces == 2 && s.substr(spacepos[1] + 1, 3) == "of ") {
        /* We have <article> <name> OF */
        /* Such as "a handful of sand" */
        word = s.substr(spacepos[0] + 1, spacepos[1] - spacepos[0] - 1);           /* "handful" */
        u = fmt::format("{} {}", word_pluralize(word), s.substr(spacepos[1] + 1)); /* "handfuls of sand" */
        return u;
        /* "a ring of fire" -> "rings of fire" */
    }

    /* Finally just apply common rules */
    u = word_pluralize(s);

    return u;
}

int levenshteinDistance(const std::string_view s1, const std::string_view s2) {
    const int len1 = s1.size();
    const int len2 = s2.size();

    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

    for (int i = 0; i <= len1; ++i) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= len2; ++j) {
        dp[0][j] = j;
    }

    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1,          // Deletion
                                 dp[i][j - 1] + 1,          // Insertion
                                 dp[i - 1][j - 1] + cost}); // Substitution
        }
    }
    return dp[len1][len2];
}

std::string_view filter_chars(const std::string_view src, const std::string_view chars) {
    std::string dest;

    if (src.empty())
        return "";

    for (char c : src) {
        if (chars.find(c) == std::string_view::npos)
            dest.push_back(c);
    }
    return dest;
}

/*
 * Takes a pointer to a const string, then skips over any and all
 * characters contained in the skip string.  Returns a pointer to
 * the first character not contained in the skip string.
 */
std::string skip_over(const std::string_view string, const std::string_view skip) {
    std::string_view::size_type pos = 0;
    while (pos < string.size() && skip.find(string[pos]) != std::string_view::npos)
        ++pos;
    return string.substr(pos);
}

/*
 * Takes a pointer to a const string, then skips over every
 * character in the string equal to the skip character.  Returns
 * a pointer to the first character not equal to the skip character.
 */
std::string skip_chars(const std::string_view string, char skip) {
    const char *str = string.data();
    while (*str && *str == skip)
        ++str;
    return std::string_view(str, string.size() - (str - string.data()));
}

/*
 * Return the first word and take it off the original string.
 */
std::string_view fetch_word(std::string_view &string) {
    std::string_view::size_type pos = 0;
    while (pos < string.size() && !std::isspace(string[pos]))
        ++pos;

    std::string_view word = string.substr(0, pos);
    string.remove_prefix(pos);
    string = skip_over(string, " \t");
    return word;
}

/*
 * Create a new screen buffer.
 */
ScreenBuf *new_screen_buf(void) {
    ScreenBuf *sb;
    CREATE(sb, ScreenBuf, 1);
    sb->buf;
    sb->lines;
    sb->line_width = ED_DEFAULT_PAGE_WIDTH;
    return sb;
}

/*
 * Figure out line endings for the buffer starting at line <start_line>.
 */
static void sb_compile_lines(ScreenBuf *sb, size_t start_line) {
    if (!sb->buf)
        return;

    std::string_view read = sb->lines[start_line];
    std::string write;
    write.reserve(sb->capacity);

    size_t line = start_line;
    size_t line_len = 0;
    bool cap_next = true, cap_next_next = false, hyphenated = false, was_hyphenated = false;
    unsigned int periods = 0, non_periods = 0;

    while (line_len < sb->first_indent) {
        write.push_back(' ');
        ++line_len;
    }

    while (!read.empty()) {
        // Skip whitespace
        while (!read.empty() && std::isspace(read.front()))
            read.remove_prefix(1);

        if (!read.empty()) {
            auto start = read;

            if (hyphenated) {
                hyphenated = false;
                was_hyphenated = true;
            }

            while (!read.empty() && !std::isspace(read.front()) && read.front() != '-' && read.front() != '.' &&
                   read.front() != '?' && read.front() != '!')
                read.remove_prefix(1);

            // Read ahead to determine if the next token is a single punctuation mark
            auto next = read;
            while (!next.empty() && std::isspace(next.front()))
                next.remove_prefix(1);

            if (!next.empty() && std::isspace(next[1])) {
                if (next.front() == ',' || next.front() == ':' || next.front() == ';' || next.front() == '.' ||
                    next.front() == '?' || next.front() == '!') {
                    read.remove_prefix(1);
                    read.front() = ' ';
                }
            }

            if (cap_next_next) {
                cap_next_next = false;
                cap_next = true;
            }

            periods = 0;
            non_periods = 0;
            // If we stopped at the end of a sentence, move off the delimiter
            while (!read.empty() && (read.front() == '.' || read.front() == '!' || read.front() == '?')) {
                if (read.front() == '.')
                    ++periods;
                else
                    ++non_periods;
                cap_next_next = true;
                read.remove_prefix(1);
            }

            // Next char is a quote? Include in this 'word'
            if (!read.empty() && (read.front() == '\'' || read.front() == '"') && read.size() > 1 && read[1] == ' ')
                read.remove_prefix(1);
            // Only periods? Probably an ellipsis (...) so don't capitalize_first
            else if (periods >= 3 && non_periods == 0)
                cap_next_next = false;

            while (!read.empty() && read.front() == '-') {
                hyphenated = true;
                read.remove_prefix(1);
            }

            auto temp = read;
            read = read.substr(0, 0); // Temporarily set read to empty to use start as a null-terminated string

            // If the word is too long to fit on this line, bump it to the next one
            if (line_len + start.size() + 1 > sb->line_width - 1) {
                write += "\n";
                line_len = 0;

                if (++line >= sb->line_capacity)
                    sb_increase_lines(sb, sb->line_capacity);
                sb->lines[line] = write;
                sb->line_count = line + 1;

                // Indent
                while (line_len < sb->other_indent) {
                    write.push_back(' ');
                    ++line_len;
                }
            }

            if (cap_next) {
                cap_next = false;
                if (IS_FLAGGED(sb->flags, SB_USE_CAPS))
                    start.front() = std::toupper(start.front());
            } else if (was_hyphenated) {
                was_hyphenated = false;
            } else if (line_len > 0) {
                write.push_back(' ');
                ++line_len;
            }

            write += start;
            line_len += start.size();

            read = temp;
        }

        if (cap_next_next) {
            if (line_len + 3 > sb->line_width - 1) {
                write += "\n";
                line_len = 0;

                if (++line >= sb->line_capacity)
                    sb_increase_lines(sb, sb->line_capacity);
                sb->lines[line] = write;
                sb->line_count = line + 1;

                // Indent
                while (line_len < sb->other_indent) {
                    write.push_back(' ');
                    ++line_len;
                }
            } else {
                write += "  ";
                line_len += 2;
            }
        }
    }

    // Trim trailing spaces and extraneous newlines
    while (!write.empty() && std::isspace(write.back())) {
        if (write.back() == '\n' && write.size() > 1 && write[write.size() - 2] == '\r')
            sb->line_count--;
        write.pop_back();
    }

    sb->buf = write;
}

/*
 *
 */
void sb_use_buf(ScreenBuf *sb, std::string_view buf, size_t buf_capacity) {
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

const std::string_view sb_get_buffer(ScreenBuf *sb) { return sb->buf; }

size_t sb_get_capacity(ScreenBuf *sb) { return sb->capacity; }

size_t sb_get_length(ScreenBuf *sb) { return sb->length; }

size_t sb_get_lines(ScreenBuf *sb) { return sb->line_count; }

size_t sb_get_width(ScreenBuf *sb) { return sb->line_width; }

size_t sb_get_first_indentation(ScreenBuf *sb) { return sb->first_indent; }

size_t sb_get_other_indentation(ScreenBuf *sb) { return sb->other_indent; }

const std::string_view sb_get_line(ScreenBuf *sb, size_t line) { return sb->lines[line]; }

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

void sb_append(ScreenBuf *sb, const std::string_view msg, ...) {
    va_list args;
    std::string_view buffer = megabuf_get();
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

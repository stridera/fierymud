#pragma once

#include "structs.hpp"

#include <string>
#include <string_view>

[[nodiscard]] std::string ellipsis(const std::string str, int maxlen);

void sprintbit(long vektor, const char *names[], char *result);
void sprinttype(int type, const char *names[], char *result);

template <std::size_t N> void sprintflag(char *result, const std::bitset<N> &flags, const char *names[]) {
    char *orig_pos = result;

    for (std::size_t i = 0; i < N; ++i) {
        if (flags.test(i)) {
            if (*names[i] != '\n')
                strcpy(result, names[i]);
            else
                strcpy(result, "UNDEFINED");
            result += strlen(result);
            *(result++) = ' ';
        }
    }

    if (orig_pos == result)
        strcpy(result, "NO FLAGS");
    else
        *(result - 1) = '\0'; /* Nul terminate */
}

template <std::size_t N> int sprintascii(char *out, const std::bitset<N> &bits) {
    int j = 0;
    /* 32 bits, don't just add letters to try to get more unless std::bitset is also as large. */
    const char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

    for (std::size_t i = 0; flags[i]; ++i)
        if (bits.test(i))
            out[j++] = flags[i];

    if (j == 0) /* Didn't write anything. */
        out[j++] = '0';

    /* Nul terminate the output string. */
    out[j++] = '\0';
    return j;
}

bool is_equals(const std::string_view &lhs, const std::string_view &rhs);

// Similar to matches() but checks if rhs starts with lhs, case insensitively.
// lhs must be at least one character long and must not be longer than rhs.
[[nodiscard]] bool matches_start(std::string_view lhs, std::string_view rhs);

[[nodiscard]] constexpr std::string_view trim_left(std::string_view s) {
    return s.substr(std::min(s.find_first_not_of(" \f\n\r\t\v"), s.size()));
}

[[nodiscard]] constexpr std::string_view trim_right(std::string_view s) {
    return s.substr(0, std::min(s.find_last_not_of(" \f\n\r\t\v") + 1, s.size()));
}

[[nodiscard]] constexpr std::string_view trim(std::string_view s) { return trim_left(trim_right(s)); }
[[nodiscard]] std::string capitalize_first(std::string_view sv);

// // Compares two strings: are they referring to the same thing. That currently means "case insensitive
// comparison".
// [[nodiscard]] bool matches(std::string_view lhs, std::string_view rhs);

// // Similar to matches_start() but checks if rhs ends with lhs, case insensitively.
// // lhs must be at least one character long and must not be longer than rhs.
// [[nodiscard]] bool matches_end(std::string_view lhs, std::string_view rhs);

// // Is 'needle' contained inside 'haystack' case insensitively?
// [[nodiscard]] bool matches_inside(std::string_view needle, std::string_view haystack);

[[nodiscard]] std::string progress_bar(int current, int wall = 0, int max = 1000);
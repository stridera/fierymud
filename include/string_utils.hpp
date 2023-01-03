#pragma once

#include "structs.hpp"

#include <string>
#include <string_view>

[[nodiscard]] std::string ellipsis(const std::string str, int maxlen);

void sprintbit(long vektor, const char *names[], char *result);
void sprinttype(int type, const char *names[], char *result);
void sprintflag(char *result, flagvector flags[], int num_flags, const char *names[]);
int sprintascii(char *out, flagvector bits);
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

// // Compares two strings: are they referring to the same thing. That currently means "case insensitive
// comparison".
// [[nodiscard]] bool matches(std::string_view lhs, std::string_view rhs);

// // Similar to matches_start() but checks if rhs ends with lhs, case insensitively.
// // lhs must be at least one character long and must not be longer than rhs.
// [[nodiscard]] bool matches_end(std::string_view lhs, std::string_view rhs);

// // Is 'needle' contained inside 'haystack' case insensitively?
// [[nodiscard]] bool matches_inside(std::string_view needle, std::string_view haystack);
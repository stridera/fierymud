#pragma once

#include "structs.hpp"

#include <string>

const std::string_view ellipsis(const std::string str, int maxlen);

void sprintbit(long vektor, const char *names[], char *result);
void sprinttype(int type, const char *names[], char *result);
void sprintflag(char *result, flagvector flags[], int num_flags, const char *names[]);
int sprintascii(char *out, flagvector bits);
bool is_equals(const std::string_view &lhs, const std::string_view &rhs);

// Similar to matches() but checks if rhs starts with lhs, case insensitively.
// lhs must be at least one character long and must not be longer than rhs.
[[nodiscard]] bool matches_start(std::string_view lhs, std::string_view rhs);
[[nodiscard]] std::string_view trim(std::string_view str);
[[nodiscard]] std::string_view rtrim(std::string_view str, std::string_view chars = " \t\r\n");

// // Compares two strings: are they referring to the same thing. That currently means "case insensitive comparison".
// [[nodiscard]] bool matches(std::string_view lhs, std::string_view rhs);

// // Similar to matches_start() but checks if rhs ends with lhs, case insensitively.
// // lhs must be at least one character long and must not be longer than rhs.
// [[nodiscard]] bool matches_end(std::string_view lhs, std::string_view rhs);

// // Is 'needle' contained inside 'haystack' case insensitively?
// [[nodiscard]] bool matches_inside(std::string_view needle, std::string_view haystack);
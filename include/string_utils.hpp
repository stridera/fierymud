#pragma once

#include "structs.hpp"

#include <string>

const void *ellipsis(const char *str, int maxlen, int mode);

#define ELLIPSIS_FMT "%-*.*s%s"
#define __ELLIPSIS_W1 (1 << 0)
#define __ELLIPSIS_W2 (1 << 1)
#define __ELLIPSIS_E (1 << 2)
#define ELLIPSIS_STR(str, maxlen)                                                                                      \
    (intptr_t) ellipsis((str), (maxlen), __ELLIPSIS_W1), (intptr_t)ellipsis((str), (maxlen), __ELLIPSIS_W2), (str),    \
        (char *)ellipsis((str), (maxlen), __ELLIPSIS_E)

void sprintbit(long vektor, const char *names[], char *result);
void sprinttype(int type, const char *names[], char *result);
void sprintflag(char *result, flagvector flags[], int num_flags, const char *names[]);
int sprintascii(char *out, flagvector bits);
bool is_equals(const std::string_view &lhs, const std::string_view &rhs);

// Similar to matches() but checks if rhs starts with lhs, case insensitively.
// lhs must be at least one character long and must not be longer than rhs.
[[nodiscard]] bool matches_start(std::string_view lhs, std::string_view rhs);

// // Compares two strings: are they referring to the same thing. That currently means "case insensitive comparison".
// [[nodiscard]] bool matches(std::string_view lhs, std::string_view rhs);

// // Similar to matches_start() but checks if rhs ends with lhs, case insensitively.
// // lhs must be at least one character long and must not be longer than rhs.
// [[nodiscard]] bool matches_end(std::string_view lhs, std::string_view rhs);

// // Is 'needle' contained inside 'haystack' case insensitively?
// [[nodiscard]] bool matches_inside(std::string_view needle, std::string_view haystack);
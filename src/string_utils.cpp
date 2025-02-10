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

#include "logging.hpp"
#include "screen.hpp"
#include "utils.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <utility>

std::string ellipsis(const std::string str, int maxlen) {
    if (str.length() < maxlen - 3)
        return str;

    std::string result;
    bool in_code = false;
    int len = 0;
    for (auto c : str) {
        if (len >= maxlen - 3)
            break;

        if (in_code || c == CREL || c == CABS) {
            in_code = !in_code;
        } else {
            len++;
        }
        result += c;
    }
    return result + "...";
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

std::string cap_string(std::string_view str) {
    std::string result;
    bool cap_next = true;
    for (auto c : str) {
        if (cap_next) {
            result += toupper(c);
            cap_next = false;
        } else {
            result += c;
        }
        if (c == ' ')
            cap_next = true;
    }
    return result;
}

std::string capitalize_first(std::string_view sv) {
    std::string result(sv);
    // Uppercase the first non-colour-sequence letter.
    bool skip_next{false};
    for (auto &c : result) {
        if (std::exchange(skip_next, false))
            continue;
        if (c == CREL || c == CABS)
            skip_next = true;
        else {
            c = toupper(c);
            break;
        }
    }
    return result;
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
        log("SYSERR: Unknown type {} in sprinttype.", type);
    }
}

bool is_equals(const std::string_view &lhs, const std::string_view &rhs) {
    auto to_lower{std::ranges::views::transform(::tolower)};
    return std::ranges::equal(lhs | to_lower, rhs | to_lower);
}

bool matches_start(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() > rhs.size() || lhs.empty())
        return false;
    return is_equals(lhs, rhs.substr(0, lhs.size()));
}

// c++23
// bool matches(std::string_view lhs, std::string_view rhs) {
//     if (lhs.size() != rhs.size())
//         return false;
//     return std::ranges::all_of(std::ranges::zip_view(lhs, rhs),
//                                [](auto pr) { return tolower(pr.first) == tolower(pr.second); });
// }

// bool matches_end(std::string_view lhs, std::string_view rhs) {
//     if (lhs.size() > rhs.size() || lhs.empty())
//         return false;
//     auto rhs_remaining = rhs.substr(rhs.size() - lhs.size(), lhs.size());
//     auto zipped_reverse = std::ranges::zip_view(lhs, rhs_remaining) | std::ranges::views::reverse;
//     return std::ranges::all_of(zipped_reverse, [](auto pr) { return tolower(pr.first) == tolower(pr.second); });
// }

// bool matches_inside(std::string_view needle, std::string_view haystack) {
//     auto needle_low = needle | std::ranges::views::transform(tolower);
//     auto haystack_low = haystack | std::ranges::views::transform(tolower);
//     return !std::ranges::search(haystack_low, needle_low).empty();
// }

std::string progress_bar(int current, int level_max, int max) {
    double percentage = static_cast<double>(current) / max * 100;
    double level_percentage = static_cast<double>(current) / level_max * 100;
    level_percentage = std::clamp(level_percentage, 0.0, 100.0);

    // Define progress bar lengths
    const int bar_length = 50;

    if (current >= max) {
        return fmt::format("[{:-^{}}]", " Mastered ", bar_length);
    }

    // Calculate filled portion for each bar
    int fill = static_cast<int>(percentage / 100 * bar_length);
    int level_fill = static_cast<int>(level_percentage / 100 * bar_length);

    // Create progress bars
    std::string progress_bar = "[" + std::string(fill, '*') + std::string(level_fill - fill, '=');
    if (bar_length - level_fill > 0)
        progress_bar += std::string(bar_length - level_fill, '-');
    progress_bar += "]";

    return progress_bar;
}

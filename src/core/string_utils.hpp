#pragma once

#include <list>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals::string_view_literals;

[[nodiscard]] std::string ellipsis(const std::string_view str, int maxlen);

[[nodiscard]] bool is_equals(const std::string_view &lhs, const std::string_view &rhs);

// Similar to matches() but checks if rhs starts with lhs, case insensitively.
// lhs must be at least one character long and must not be longer than rhs.
[[nodiscard]] bool matches_start(std::string_view lhs, std::string_view rhs);

// // Similar to matches_start() but checks if rhs ends with lhs, case insensitively.
// // lhs must be at least one character long and must not be longer than rhs.
[[nodiscard]] bool matches_end(std::string_view lhs, std::string_view rhs);

[[nodiscard]] constexpr std::string_view trim_left(std::string_view s) {
    return s.substr(std::min(s.find_first_not_of(" \f\n\r\t\v"), s.size()));
}

[[nodiscard]] constexpr std::string_view trim_right(std::string_view s) {
    return s.substr(0, std::min(s.find_last_not_of(" \f\n\r\t\v") + 1, s.size()));
}
[[nodiscard]] bool is_integer(std::string_view sv) noexcept;
[[nodiscard]] constexpr std::string_view trim(std::string_view s) { return trim_left(trim_right(s)); }
[[nodiscard]] std::string capitalize_first(std::string_view sv);

// Compares two strings: are they referring to the same thing. That currently means "case insensitive comparison".
[[nodiscard]] bool matches(std::string_view lhs, std::string_view rhs);

// // Is 'needle' contained inside 'haystack' case insensitively?
[[nodiscard]] bool matches_inside(std::string_view needle, std::string_view haystack);

[[nodiscard]] std::string filter_characters(std::string_view input, bool (*filter_func)(char));

void skip_spaces(std::string_view str);
[[nodiscard]] bool string_in_list(std::string_view target, const std::list<std::string_view> stringList);
[[nodiscard]] std::string replace_string(std::string_view str, std::string_view from, std::string_view to);
[[nodiscard]] int svtoi(std::string_view sv, int default_value = -1) noexcept;
[[nodiscard]] std::string to_lower(std::string_view str);

[[nodiscard]] std::string progress_bar(int current, int wall = 0, int max = 1000);

/* Given a string, change all instances of double dollar signs ($$) to single dollar signs ($).  When strings come in,
 * all $'s are changed to $$'s to avoid having users be able to crash the system if the inputted string is eventually
 * sent to act().  If you are using user input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT THROUGH THE
 * act() FUNCTION (i.e., do_gecho, do_title, but NOT do_gsay), you can call delete_doubledollar() to make the output
 * look correct. */
// void delete_doubledollar(std::string &str) { str = replace_string(str, "$$", "$"); }

[[nodiscard]] std::string to_lowercase(std::string_view str);
[[nodiscard]] std::string to_uppercase(std::string_view str);
[[nodiscard]] std::string_view getline(std::string_view &input, char delim);

[[nodiscard]] std::string join_strings(const std::vector<std::string> &strings, const std::string_view separator,
                                       const std::string_view last_separator);

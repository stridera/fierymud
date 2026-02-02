#include "string_utils.hpp"

#include "core/logging.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <utility>

std::string ellipsis(const std::string_view str, int maxlen) {
    if (static_cast<int>(str.length()) < maxlen - 3)
        return std::string(str);

    std::string result;
    bool in_code = false;
    int len = 0;
    for (auto c : str) {
        if (len >= maxlen - 3)
            break;

        if (in_code || c == '&' || c == '|') {
            in_code = !in_code;
        } else {
            len++;
        }
        result += c;
    }
    return result + "...";
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

// Tests to see if the string_view is a (POSITIVE) integer
[[nodiscard]] bool is_integer(std::string_view sv) noexcept {
    return !sv.empty() && std::ranges::all_of(trim(sv), ::isdigit);
}

std::string capitalize_first(std::string_view sv) {
    std::string result(sv);
    // Uppercase the first non-colour-sequence letter.
    bool skip_next{false};
    for (auto &c : result) {
        if (std::exchange(skip_next, false))
            continue;
        if (c == '&' || c == '|')
            skip_next = true;
        else {
            c = toupper(c);
            break;
        }
    }
    return result;
}

std::string to_lower(std::string_view str) {
    std::string result;
    std::transform(str.begin(), str.end(), std::back_inserter(result), ::tolower);
    return result;
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

std::string filter_characters(const std::string_view input, bool (*filter_func)(char)) {
    std::string result = "";
    for (char c : input) {
        if (filter_func(c)) {
            result += c;
        }
    }
    return result;
}

void skip_spaces(std::string str) { str.erase(0, str.find_first_not_of(" \t\f\n\r")); }

int svtoi(std::string_view sv, int default_value) noexcept {
    int value = default_value;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (ec == std::errc()) {
        return value;
    }
    return value;
}

bool string_in_list(std::string_view target, const std::list<std::string_view> stringList) {
    for (std::string_view element : stringList) {
        if (target == element) {
            return true;
        }
    }
    return false;
}

// c++23
bool matches(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size())
        return false;
    return std::ranges::all_of(std::ranges::zip_view(lhs, rhs),
                               [](auto pr) { return tolower(std::get<0>(pr)) == tolower(std::get<1>(pr)); });
}

bool matches_end(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() > rhs.size() || lhs.empty())
        return false;
    auto rhs_remaining = rhs.substr(rhs.size() - lhs.size(), lhs.size());
    auto zipped_reverse = std::ranges::zip_view(lhs, rhs_remaining) | std::ranges::views::reverse;
    return std::ranges::all_of(zipped_reverse,
                               [](auto pr) { return tolower(std::get<0>(pr)) == tolower(std::get<1>(pr)); });
}

bool matches_inside(std::string_view needle, std::string_view haystack) {
    auto needle_low = needle | std::ranges::views::transform(tolower);
    auto haystack_low = haystack | std::ranges::views::transform(tolower);
    return !std::ranges::search(haystack_low, needle_low).empty();
}

std::string replace_string(std::string_view str, std::string_view from, std::string_view to) {
    std::string result(str);
    size_t pos = result.find(from);
    while (pos != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos = result.find(from, pos + to.length());
    }
    return result;
}

std::string to_lowercase(std::string_view str) {
    std::string result;
    std::transform(str.begin(), str.end(), std::back_inserter(result), ::tolower);
    return result;
}

std::string to_uppercase(std::string_view str) {
    std::string result;
    std::transform(str.begin(), str.end(), std::back_inserter(result), ::toupper);
    return result;
}

std::string_view getline(std::string_view &input, char delim) {
    auto pos = input.find(delim);
    if (pos == std::string_view::npos) {
        auto line = input;
        input.remove_prefix(input.size());
        return line;
    }
    auto line = input.substr(0, pos);
    input.remove_prefix(pos + 1);
    return line;
}

std::string join_strings(const std::vector<std::string> &strings, const std::string_view separator,
                         const std::string_view last_separator) {
    if (strings.empty()) {
        return "";
    }
    if (strings.size() == 1) {
        return std::string(strings[0]);
    }

    return std::accumulate(strings.begin(), strings.end() - 1, std::string{},
                           [&separator](const std::string &a, const std::string &b) {
                               return a.empty() ? b : a + std::string(separator) + b;
                           }) +
           std::string(last_separator) + strings.back();
}

bool matches_words(std::string_view search, std::string_view target) {
    // Split search term into words (separated by spaces or underscores)
    std::vector<std::string_view> search_words;
    std::string_view remaining = search;
    while (!remaining.empty()) {
        size_t start = remaining.find_first_not_of(" _");
        if (start == std::string_view::npos) break;
        remaining = remaining.substr(start);
        size_t end = remaining.find_first_of(" _");
        if (end == std::string_view::npos) {
            search_words.push_back(remaining);
            break;
        }
        search_words.push_back(remaining.substr(0, end));
        remaining = remaining.substr(end);
    }

    if (search_words.empty()) return false;

    // Split target into words
    std::vector<std::string_view> target_words;
    remaining = target;
    while (!remaining.empty()) {
        size_t start = remaining.find_first_not_of(" _");
        if (start == std::string_view::npos) break;
        remaining = remaining.substr(start);
        size_t end = remaining.find_first_of(" _");
        if (end == std::string_view::npos) {
            target_words.push_back(remaining);
            break;
        }
        target_words.push_back(remaining.substr(0, end));
        remaining = remaining.substr(end);
    }

    // Each search word must be a prefix of the corresponding target word
    if (search_words.size() > target_words.size()) return false;

    for (size_t i = 0; i < search_words.size(); ++i) {
        if (!matches_start(search_words[i], target_words[i])) {
            return false;
        }
    }
    return true;
}

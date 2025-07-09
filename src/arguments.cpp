#include "arguments.hpp"

#include <optional>

std::string_view Arguments::command_shift(bool strict) {
    arg = trim(arg);

    if (arg.empty()) {
        return {};
    }

    // If the first character is non-alpha, return just that character.
    if (!isalpha(arg[0])) {
        char first_char = arg[0];
        arg = arg.substr(1);
        // Store the single character in a static string to ensure it persists
        static std::string single_char_buffer;
        single_char_buffer = first_char;
        return single_char_buffer;
    }

    if (strict)
        return {};

    return shift();
}

std::string_view Arguments::shift() {
    arg = trim(arg);

    if (arg.empty()) {
        return {};
    }

    // If the argument is quoted, return the quoted string.
    std::string_view output;
    if (arg[0] == '"' || arg[0] == '\'') {
        size_t end = arg.find(arg[0], 1);
        if (end == std::string::npos) {
            output = arg.substr(1);
            arg.clear();
        } else {
            output = arg.substr(1, end - 1);
            arg = arg.substr(end + 1);
        }
    } else {
        size_t end = arg.find(' ');
        if (end == std::string::npos) {
            output = arg;
            arg.clear();
        } else {
            output = arg.substr(0, end);
            arg = arg.substr(end);
        }
    }

    return output;
}

std::string_view Arguments::shift_clean() {
    std::string_view arg = shift();
    return trim(replace_string(arg, "$$", "$"));
}

std::optional<int> Arguments::try_shift_number() {
    std::string_view arg = shift();
    if (arg.empty()) {
        return std::nullopt;
    }

    if (!is_integer(arg)) {
        return std::nullopt;
    }

    return svtoi(arg);
}

std::optional<std::pair<int, std::string_view>> Arguments::try_shift_number_and_arg() {
    std::string_view arg = shift();
    if (arg.empty()) {
        return std::nullopt;
    }

    size_t dot_pos = arg.find('.');
    std::string_view number_str = arg.substr(0, dot_pos);
    std::string_view rest = arg.substr(dot_pos + 1);

    if (number_str.empty() || rest.empty()) {
        return std::make_pair(1, arg);
    }

    // If the number is 'all', return the maximum number of items.
    if (number_str == "all") {
        return std::make_pair(MAX_ITEMS, rest);
    }

    // If there is no dot, or the rest is empty, or the number is not an integer, return 1 and the original argument.
    if (dot_pos == std::string::npos || rest.empty() || !is_integer(number_str)) {
        return std::make_pair(1, arg);
    }

    auto number = svtoi(number_str);
    return std::make_pair(number, rest);
}
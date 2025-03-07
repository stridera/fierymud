#include "arguments.hpp"

#include <optional>

std::string_view Arguments::command_shift(bool strict) {
    arg = trim(arg);

    if (arg.empty()) {
        return {};
    }

    // If the first character is non-alpha, return just that character.
    if (!isalpha(arg[0])) {
        std::string_view output = arg.substr(0, 1);
        arg = arg.substr(1);
        return output;
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
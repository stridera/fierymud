#include "arguments.hpp"

#include <optional>

std::string_view Arguments::command_shift(bool strict) {
    arg_ = trim(arg_);

    if (arg_.empty()) {
        return {};
    }

    // If the first character is non-alpha, return just that character.
    if (!isalpha(arg_[0])) {
        std::string_view output = arg_.substr(0, 1);
        arg_ = arg_.substr(1);
        return output;
    }

    if (strict)
        return {};

    return shift();
}

std::string_view Arguments::shift() {
    arg_ = trim(arg_);

    if (arg_.empty()) {
        return {};
    }

    // If the argument is quoted, return the quoted string.
    std::string_view output;
    if (arg_[0] == '"' || arg_[0] == '\'') {
        size_t end = arg_.find(arg_[0], 1);
        if (end == std::string::npos) {
            output = arg_.substr(1);
            arg_ = std::string_view{};
        } else {
            output = arg_.substr(1, end - 1);
            arg_ = arg_.substr(end + 1);
        }
    } else {
        size_t end = arg_.find(' ');
        if (end == std::string::npos) {
            output = arg_;
            arg_ = std::string_view{};
        } else {
            output = arg_.substr(0, end);
            arg_ = arg_.substr(end);
        }
    }

    return output;
}

std::string_view Arguments::shift_clean() {
    std::string_view arg_ = shift();
    return trim(replace_string(arg_, "$$", "$"));
}

std::optional<int> Arguments::try_shift_number() {
    std::string_view arg_ = shift();
    if (arg_.empty()) {
        return std::nullopt;
    }

    if (!is_integer(arg_)) {
        return std::nullopt;
    }

    return svtoi(arg_);
}

std::optional<std::pair<int, std::string_view>> Arguments::try_shift_number_and_arg() {
    std::string_view arg_ = shift();
    if (arg_.empty()) {
        return std::nullopt;
    }

    size_t dot_pos = arg_.find('.');
    std::string_view number_str = arg_.substr(0, dot_pos);
    std::string_view rest = arg_.substr(dot_pos + 1);

    if (number_str.empty() || rest.empty()) {
        return std::make_pair(1, arg_);
    }

    // If the number is 'all', return the maximum number of items.
    if (number_str == "all") {
        return std::make_pair(MAX_ITEMS, rest);
    }

    // If there is no dot, or the rest is empty, or the number is not an integer, return 1 and the original argument.
    if (dot_pos == std::string::npos || rest.empty() || !is_integer(number_str)) {
        return std::make_pair(1, arg_);
    }

    auto number = svtoi(number_str);
    return std::make_pair(number, rest);
}
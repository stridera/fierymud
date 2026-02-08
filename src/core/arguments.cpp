#include "arguments.hpp"

#include "text/string_utils.hpp"

std::string_view Arguments::get() const { return trim(arg_); }

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

std::string Arguments::shift_clean() {
    std::string_view arg_ = shift();
    return replace_string(arg_, "$$", "$");
}

std::optional<int> Arguments::try_shift_number() {
    // Save the current state in case we need to restore it
    std::string_view saved_arg = arg_;

    std::string_view arg_ = shift();
    if (arg_.empty()) {
        // Restore state since we couldn't parse anything
        this->arg_ = saved_arg;
        return std::nullopt;
    }

    if (!is_integer(arg_)) {
        // Restore state since parsing failed
        this->arg_ = saved_arg;
        return std::nullopt;
    }

    // Parsing succeeded, keep the shifted state
    return svtoi(arg_);
}

std::optional<std::pair<int, std::string_view>> Arguments::try_shift_number_and_arg() {
    // Save the current state in case we need to restore it
    std::string_view saved_arg = arg_;

    std::string_view arg_ = shift();
    if (arg_.empty()) {
        // Restore state since we couldn't parse anything
        this->arg_ = saved_arg;
        return std::nullopt;
    }

    size_t dot_pos = arg_.find('.');

    // If there is no dot, this is not a number.item format, so restore and fail
    if (dot_pos == std::string::npos) {
        this->arg_ = saved_arg;
        return std::nullopt;
    }

    std::string_view number_str = arg_.substr(0, dot_pos);
    std::string_view rest = arg_.substr(dot_pos + 1);

    // If either part is missing, this is not a valid number.item format
    if (number_str.empty() || rest.empty()) {
        this->arg_ = saved_arg;
        return std::nullopt;
    }

    // If the number is 'all', return the maximum number of items.
    if (number_str == "all") {
        return std::make_pair(MAX_ITEMS, rest);
    }

    // If the number part is not an integer, this is not a valid format
    if (!is_integer(number_str)) {
        this->arg_ = saved_arg;
        return std::nullopt;
    }

    auto number = svtoi(number_str);
    return std::make_pair(number, rest);
}

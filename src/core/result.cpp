#include "result.hpp"
#include <magic_enum/magic_enum.hpp>

Error::Error(ErrorCode error_code, std::string_view msg)
    : code(error_code), message(msg) {}

Error::Error(ErrorCode error_code, std::string_view msg, std::string_view ctx)
    : code(error_code), message(msg), context(ctx) {}

std::string Error::code_name() const {
    return std::string(magic_enum::enum_name(code));
}

std::string Error::to_string() const {
    if (context.empty()) {
        return fmt::format("[{}] {}", code_name(), message);
    }
    return fmt::format("[{}] {} (context: {})", code_name(), message, context);
}

bool Error::is(ErrorCode error_code) const {
    return code == error_code;
}

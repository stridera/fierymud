// Error handling with std::expected and typed error codes

#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <variant>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

/**
 * Modern error handling system using std::expected.
 * 
 * Replaces legacy error patterns with type-safe error handling:
 * - Return codes become Result<T>
 * - NULL returns become optional or Result
 * - Error messages are structured and contextual
 */

/** Error categories for different types of failures */
enum class ErrorCode {
    // General errors
    Success = 0,
    NotFound,
    AlreadyExists, 
    InvalidState,
    InvalidArgument,
    
    // Permission and access errors
    PermissionDenied,
    AccessDenied,
    InsufficientPrivileges,
    
    // Resource errors
    OutOfMemory,
    ResourceExhausted,
    Timeout,
    
    // I/O and persistence errors
    FileNotFound,
    FileAccessError,
    ParseError,
    SerializationError,
    
    // Game logic errors
    InvalidCommand,
    InvalidTarget,
    NotInCorrectState,
    CooldownActive,
    InsufficientResources,
    
    // Network errors
    NetworkError,
    ConnectionLost,
    ProtocolError,
    
    // Scripting errors  
    ScriptError,
    ScriptTimeout,
    
    // Database errors
    DatabaseError,

    // Internal errors
    InternalError,
    NotImplemented,
    
    // Testing errors
    TestFailure
};

/** Rich error type with code, message, and context */
struct Error {
    ErrorCode code;
    std::string message;
    std::string context; // Additional context (file, function, etc.)
    
    /** Construct error with code and message */
    Error(ErrorCode error_code, std::string_view msg) 
        : code(error_code), message(msg) {}
        
    /** Construct error with code, message, and context */
    Error(ErrorCode error_code, std::string_view msg, std::string_view ctx)
        : code(error_code), message(msg), context(ctx) {}
    
    /** Get error code name as string */
    std::string code_name() const {
        return std::string(magic_enum::enum_name(code));
    }
    
    /** Get formatted error string */
    std::string to_string() const {
        if (context.empty()) {
            return fmt::format("[{}] {}", code_name(), message);
        }
        return fmt::format("[{}] {} (context: {})", code_name(), message, context);
    }
    
    /** Check if error is of specific type */
    bool is(ErrorCode error_code) const {
        return code == error_code;
    }
};

/** Result type alias for std::expected<T, Error> */
template<typename T>
using Result = std::expected<T, Error>;

/** Void result type for operations that don't return values */
using VoidResult = Result<void>;

/** Helper functions for creating common errors */
namespace Errors {
    inline Error NotFound(std::string_view what) {
        return Error{ErrorCode::NotFound, fmt::format("{} not found", what)};
    }
    
    inline Error AlreadyExists(std::string_view what) {
        return Error{ErrorCode::AlreadyExists, fmt::format("{} already exists", what)};
    }
    
    inline Error InvalidArgument(std::string_view arg, std::string_view reason = "") {
        if (reason.empty()) {
            return Error{ErrorCode::InvalidArgument, fmt::format("Invalid argument: {}", arg)};
        }
        return Error{ErrorCode::InvalidArgument, fmt::format("Invalid argument: {} ({})", arg, reason)};
    }
    
    inline Error PermissionDenied(std::string_view action) {
        return Error{ErrorCode::PermissionDenied, fmt::format("Permission denied: {}", action)};
    }
    
    inline Error InvalidState(std::string_view current_state) {
        return Error{ErrorCode::InvalidState, fmt::format("Invalid state: {}", current_state)};
    }
    
    inline Error ParseError(std::string_view what, std::string_view details = "") {
        if (details.empty()) {
            return Error{ErrorCode::ParseError, fmt::format("Parse error: {}", what)};
        }
        return Error{ErrorCode::ParseError, fmt::format("Parse error: {} ({})", what, details)};
    }
    
    inline Error ScriptError(std::string_view script_name, std::string_view details) {
        return Error{ErrorCode::ScriptError, fmt::format("Script error in {}: {}", script_name, details)};
    }

    inline Error DatabaseError(std::string_view details) {
        return Error{ErrorCode::DatabaseError, fmt::format("Database error: {}", details)};
    }
    
    inline Error InternalError(std::string_view function, std::string_view details) {
        return Error{ErrorCode::InternalError, details, function};
    }
    
    inline Error NotImplemented(std::string_view feature) {
        return Error{ErrorCode::NotImplemented, fmt::format("Not implemented: {}", feature)};
    }
    
    inline Error FileNotFound(std::string_view filename) {
        return Error{ErrorCode::FileNotFound, fmt::format("File not found: {}", filename)};
    }
    
    inline Error FileAccessError(std::string_view details) {
        return Error{ErrorCode::FileAccessError, fmt::format("File access error: {}", details)};
    }
    
    inline Error SerializationError(std::string_view context, std::string_view details = "") {
        if (details.empty()) {
            return Error{ErrorCode::SerializationError, fmt::format("Serialization error: {}", context)};
        }
        return Error{ErrorCode::SerializationError, fmt::format("Serialization error in {}: {}", context, details)};
    }
    
    inline Error SystemError(std::string_view details) {
        return Error{ErrorCode::InternalError, fmt::format("System error: {}", details)};
    }
    
    inline Error FileSystem(std::string_view details) {
        return Error{ErrorCode::FileAccessError, fmt::format("File system error: {}", details)};
    }
    
    inline Error InvalidFormat(std::string_view details) {
        return Error{ErrorCode::ParseError, fmt::format("Invalid format: {}", details)};
    }
}

/** Helper functions for creating successful results */
inline Result<void> Success() {
    return Result<void>{};
}

/** Macros for convenient error checking */
#define TRY(expr) \
    do { \
        if (auto result = (expr); !result) { \
            return std::unexpected(result.error()); \
        } \
    } while(0)

#define TRY_ASSIGN(var, expr) \
    ({ \
        auto result = (expr); \
        if (!result) { \
            return std::unexpected(result.error()); \
        } \
        var = std::move(result.value()); \
    })

/** Formatting support for Error */
template<>
struct fmt::formatter<Error> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(const Error& err, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", err.to_string());
    }
};

/** Stream output support for Error */
#include <ostream>
inline std::ostream& operator<<(std::ostream& os, const Error& error) {
    return os << error.to_string();
}
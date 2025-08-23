/***************************************************************************
 *   File: src/core/logging.hpp                           Part of FieryMUD *
 *  Usage: Modern structured logging with spdlog integration               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "ids.hpp"
#include "result.hpp"

#include <memory>
#include <source_location>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <unordered_map>

/**
 * Modern structured logging system.
 *
 * Features:
 * - Structured logging with contextual metadata
 * - Multiple log levels with filtering
 * - File rotation and multiple sinks
 * - Performance-optimized with compile-time optimization
 * - Integration with Error system for rich error context
 */

/** Log levels matching spdlog levels */
enum class LogLevel { Trace = 0, Debug = 1, Info = 2, Warning = 3, Error = 4, Critical = 5, Off = 6 };

/** Context information for structured logging */
struct LogContext {
    std::string component;                                 // System component (combat, movement, etc.)
    std::string player_name;                               // Associated player if any
    EntityId entity_id = INVALID_ENTITY_ID;                // Associated entity if any
    std::string room_vnum;                                 // Associated room if any
    std::unordered_map<std::string, std::string> metadata; // Additional key-value pairs

    // Constructors to avoid aggregate missing-initializer warnings and allow direct initialization
    LogContext() = default;
    explicit LogContext(std::string_view component_name) : component(component_name) {}

    /** Add metadata entry */
    LogContext &add(std::string_view key, std::string_view value) {
        metadata[std::string(key)] = std::string(value);
        return *this;
    }

    /** Add metadata entry with formatting */
    template <typename... Args> LogContext &add(std::string_view key, std::string_view format, Args &&...args) {
        metadata[std::string(key)] = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
        return *this;
    }
};

/** Logger wrapper for structured logging */
class Logger {
  public:
    /** Initialize logging system */
    static void initialize(const std::string &log_file = "fierymud.log", LogLevel level = LogLevel::Info,
                           bool console_output = true);

    /** Get logger instance for component */
    static std::shared_ptr<Logger> get(std::string_view component);

    /** Log with context */
    template <typename... Args> void trace(const LogContext &ctx, std::string_view format, Args &&...args) {
        log_with_context(LogLevel::Trace, ctx, format, std::forward<Args>(args)...);
    }

    template <typename... Args> void debug(const LogContext &ctx, std::string_view format, Args &&...args) {
        log_with_context(LogLevel::Debug, ctx, format, std::forward<Args>(args)...);
    }

    template <typename... Args> void info(const LogContext &ctx, std::string_view format, Args &&...args) {
        log_with_context(LogLevel::Info, ctx, format, std::forward<Args>(args)...);
    }

    template <typename... Args> void warn(const LogContext &ctx, std::string_view format, Args &&...args) {
        log_with_context(LogLevel::Warning, ctx, format, std::forward<Args>(args)...);
    }

    template <typename... Args> void error(const LogContext &ctx, std::string_view format, Args &&...args) {
        log_with_context(LogLevel::Error, ctx, format, std::forward<Args>(args)...);
    }

    template <typename... Args> void critical(const LogContext &ctx, std::string_view format, Args &&...args) {
        log_with_context(LogLevel::Critical, ctx, format, std::forward<Args>(args)...);
    }

    /** Simple logging without context */
    template <typename... Args> void trace(std::string_view format, Args &&...args) {
        logger_->trace(fmt::runtime(format), std::forward<Args>(args)...);
    }

    template <typename... Args> void debug(std::string_view format, Args &&...args) {
        logger_->debug(fmt::runtime(format), std::forward<Args>(args)...);
    }

    template <typename... Args> void info(std::string_view format, Args &&...args) {
        logger_->info(fmt::runtime(format), std::forward<Args>(args)...);
    }

    template <typename... Args> void warn(std::string_view format, Args &&...args) {
        logger_->warn(fmt::runtime(format), std::forward<Args>(args)...);
    }

    template <typename... Args> void error(std::string_view format, Args &&...args) {
        logger_->error(fmt::runtime(format), std::forward<Args>(args)...);
    }

    template <typename... Args> void critical(std::string_view format, Args &&...args) {
        logger_->critical(fmt::runtime(format), std::forward<Args>(args)...);
    }

    /** Log Error objects with rich context */
    void log_error(const Error &error, const LogContext &ctx = {});

  private:
    explicit Logger(std::shared_ptr<spdlog::logger> logger) : logger_(std::move(logger)) {}

    template <typename... Args>
    void log_with_context(LogLevel level, const LogContext &ctx, std::string_view format, Args &&...args) {
        std::string full_message = format_with_context(ctx, format, std::forward<Args>(args)...);

        switch (level) {
        case LogLevel::Trace:
            logger_->trace(full_message);
            break;
        case LogLevel::Debug:
            logger_->debug(full_message);
            break;
        case LogLevel::Info:
            logger_->info(full_message);
            break;
        case LogLevel::Warning:
            logger_->warn(full_message);
            break;
        case LogLevel::Error:
            logger_->error(full_message);
            break;
        case LogLevel::Critical:
            logger_->critical(full_message);
            break;
        case LogLevel::Off:
            break;
        }
    }

    template <typename... Args>
    std::string format_with_context(const LogContext &ctx, std::string_view format, Args &&...args) {
        std::string base_message = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);

        if (ctx.player_name.empty() && ctx.entity_id == INVALID_ENTITY_ID && ctx.room_vnum.empty() &&
            ctx.metadata.empty()) {
            return base_message;
        }

        std::string context_parts;

        if (!ctx.player_name.empty()) {
            context_parts += fmt::format("player={}", ctx.player_name);
        }

        if (ctx.entity_id.is_valid()) {
            if (!context_parts.empty())
                context_parts += ", ";
            context_parts += fmt::format("entity={}", ctx.entity_id);
        }

        if (!ctx.room_vnum.empty()) {
            if (!context_parts.empty())
                context_parts += ", ";
            context_parts += fmt::format("room={}", ctx.room_vnum);
        }

        for (const auto &[key, value] : ctx.metadata) {
            if (!context_parts.empty())
                context_parts += ", ";
            context_parts += fmt::format("{}={}", key, value);
        }

        return fmt::format("{} [{}]", base_message, context_parts);
    }

    std::shared_ptr<spdlog::logger> logger_;
    static std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
};

/** Global logging functions for common use cases */
namespace Log {
/** Get game system logger */
std::shared_ptr<Logger> game();

/** Get combat system logger */
std::shared_ptr<Logger> combat();

/** Get movement system logger */
std::shared_ptr<Logger> movement();

/** Get command system logger */
std::shared_ptr<Logger> commands();

/** Get networking logger */
std::shared_ptr<Logger> network();

/** Get persistence logger */
std::shared_ptr<Logger> persistence();

/** Get scripting logger */
std::shared_ptr<Logger> scripting();

/** Quick error logging with source location */
template <typename... Args> void error(std::string_view format, Args &&...args) {
    LogContext ctx("global");
    game()->error(ctx, format, std::forward<Args>(args)...);
}

/** Quick warning logging */
template <typename... Args> void warn(std::string_view format, Args &&...args) {
    LogContext ctx("global");
    game()->warn(ctx, format, std::forward<Args>(args)...);
}

/** Quick info logging */
template <typename... Args> void info(std::string_view format, Args &&...args) {
    game()->info(format, std::forward<Args>(args)...);
}

/** Quick debug logging */
template <typename... Args> void debug(std::string_view format, Args &&...args) {
    game()->debug(format, std::forward<Args>(args)...);
}

/** Log game event with player context */
template <typename... Args> void player_event(std::string_view player_name, std::string_view event, Args &&...args) {
    LogContext ctx;
    ctx.player_name = player_name;
    game()->info(ctx, fmt::format("{}: {}", event, fmt::format(std::forward<Args>(args)...)));
}
} // namespace Log

/** Convenience macros for common logging patterns */
#define LOG_TRACE(logger, ...)                                                                                         \
    do {                                                                                                               \
        if (logger)                                                                                                    \
            (logger)->trace(__VA_ARGS__);                                                                              \
    } while (0)
#define LOG_DEBUG(logger, ...)                                                                                         \
    do {                                                                                                               \
        if (logger)                                                                                                    \
            (logger)->debug(__VA_ARGS__);                                                                              \
    } while (0)
#define LOG_INFO(logger, ...)                                                                                          \
    do {                                                                                                               \
        if (logger)                                                                                                    \
            (logger)->info(__VA_ARGS__);                                                                               \
    } while (0)
#define LOG_WARN(logger, ...)                                                                                          \
    do {                                                                                                               \
        if (logger)                                                                                                    \
            (logger)->warn(__VA_ARGS__);                                                                               \
    } while (0)
#define LOG_ERROR(logger, ...)                                                                                         \
    do {                                                                                                               \
        if (logger)                                                                                                    \
            (logger)->error(__VA_ARGS__);                                                                              \
    } while (0)

/** Performance-optimized logging macros that compile out in release builds */
#ifdef NDEBUG
#define LOG_TRACE_FAST(logger, ...)                                                                                    \
    do {                                                                                                               \
    } while (0)
#define LOG_DEBUG_FAST(logger, ...)                                                                                    \
    do {                                                                                                               \
    } while (0)
#else
#define LOG_TRACE_FAST(logger, ...) LOG_TRACE(logger, __VA_ARGS__)
#define LOG_DEBUG_FAST(logger, ...) LOG_DEBUG(logger, __VA_ARGS__)
#endif
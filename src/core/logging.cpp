#include "logging.hpp"
#include "log_subscriber.hpp"

#include <spdlog/pattern_formatter.h>
#include <atomic>
#include <filesystem>

// Static member definition
std::unordered_map<std::string, std::shared_ptr<Logger>> Logger::loggers_;

// Static member for shared sinks
std::vector<spdlog::sink_ptr> Logger::sinks_;

// Shutdown flag to prevent use-after-free during static destruction
static std::atomic<bool> logging_shutdown{false};

void Logger::initialize(const std::string& log_file, LogLevel level, bool console_output) {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::path log_path(log_file);
        if (log_path.has_parent_path()) {
            std::filesystem::create_directories(log_path.parent_path());
        }

        // Clear existing sinks and loggers
        sinks_.clear();
        loggers_.clear();
        spdlog::drop_all();

        // Set up sinks
        if (console_output) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(static_cast<spdlog::level::level_enum>(level));
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] %v");
            sinks_.push_back(console_sink);
        }

        // Rotating file sink (10MB max, 5 files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 10 * 1024 * 1024, 5);
        file_sink->set_level(static_cast<spdlog::level::level_enum>(level));
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] %v");
        sinks_.push_back(file_sink);

        // Player log sink for in-game syslog viewing
        auto player_sink = PlayerLogSink::instance();
        player_sink->set_level(static_cast<spdlog::level::level_enum>(level));
        sinks_.push_back(player_sink);

        // Create and register default logger with our sinks
        auto default_logger = std::make_shared<spdlog::logger>("fierymud", sinks_.begin(), sinks_.end());
        default_logger->set_level(static_cast<spdlog::level::level_enum>(level));
        spdlog::set_default_logger(default_logger);

        // Set global log level
        spdlog::set_level(static_cast<spdlog::level::level_enum>(level));

        // Flush policy - flush on debug or higher (ensures file writes)
        spdlog::flush_on(spdlog::level::debug);

        // Also set periodic flush every 1 second
        spdlog::flush_every(std::chrono::seconds(1));

        // Set pattern for all loggers
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%n] [%l] %v");

    } catch (const std::exception& ex) {
        // Fallback to console-only logging if file setup fails
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(static_cast<spdlog::level::level_enum>(level));

        auto logger = std::make_shared<spdlog::logger>("fierymud", console_sink);
        logger->error("Failed to initialize file logging: {}. Using console only.", ex.what());
        spdlog::set_default_logger(logger);
    }
}

void Logger::shutdown() {
    // Set shutdown flag first to prevent new logger access
    logging_shutdown.store(true, std::memory_order_release);

    // Flush all loggers before shutdown
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger) {
        logger->flush();
    });

    // Clear our cached loggers
    loggers_.clear();
    sinks_.clear();

    // Shutdown spdlog
    spdlog::shutdown();
}

// Null logger singleton - heap-allocated to survive static destruction
// Uses leaky singleton pattern: intentionally never destroyed to avoid use-after-free
std::shared_ptr<Logger> Logger::null_logger() {
    static auto* logger = new std::shared_ptr<Logger>([]{
        std::vector<spdlog::sink_ptr> empty_sinks;
        auto null_spdlog = std::make_shared<spdlog::logger>("null", empty_sinks.begin(), empty_sinks.end());
        null_spdlog->set_level(spdlog::level::off);
        return std::shared_ptr<Logger>(new Logger(null_spdlog));
    }());
    return *logger;
}

std::shared_ptr<Logger> Logger::get(std::string_view component) {
    // If logging has been shut down, return null logger to avoid use-after-free
    if (logging_shutdown.load(std::memory_order_acquire)) {
        return Logger::null_logger();
    }

    std::string comp_str(component);

    auto it = loggers_.find(comp_str);
    if (it != loggers_.end()) {
        return it->second;
    }

    // Create new logger for this component
    try {
        auto spdlog_logger = spdlog::get(comp_str);
        if (!spdlog_logger) {
            // Create new spdlog logger with same sinks as default
            auto default_logger = spdlog::default_logger();
            if (default_logger) {
                spdlog_logger = std::make_shared<spdlog::logger>(comp_str,
                    default_logger->sinks().begin(), default_logger->sinks().end());
                spdlog_logger->set_level(default_logger->level());
                spdlog::register_logger(spdlog_logger);
            } else {
                // Fallback: create console-only logger
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                spdlog_logger = std::make_shared<spdlog::logger>(comp_str, console_sink);
                spdlog::register_logger(spdlog_logger);
            }
        }

        auto logger = std::shared_ptr<Logger>(new Logger(spdlog_logger));
        loggers_[comp_str] = logger;
        return logger;

    } catch (const std::exception& ex) {
        // Return a null logger wrapper that safely ignores all calls
        return Logger::null_logger();
    }
}

void Logger::log_error(const Error& error, const LogContext& ctx) {
    LogContext error_ctx = ctx;
    error_ctx.add("error_code", error.code_name());
    
    if (!error.context.empty()) {
        error_ctx.add("error_context", error.context);
    }
    
    this->error(error_ctx, "{}", error.message);
}

namespace Log {
    // Use Logger::get() directly instead of caching in static locals.
    // This ensures proper shutdown handling (Logger::get returns null_logger after shutdown).
    // The Logger::get() function itself caches loggers in Logger::loggers_.

    std::shared_ptr<Logger> game() {
        return Logger::get("game");
    }

    std::shared_ptr<Logger> combat() {
        return Logger::get("combat");
    }

    std::shared_ptr<Logger> movement() {
        return Logger::get("movement");
    }

    std::shared_ptr<Logger> commands() {
        return Logger::get("commands");
    }

    std::shared_ptr<Logger> network() {
        return Logger::get("network");
    }

    std::shared_ptr<Logger> persistence() {
        return Logger::get("persistence");
    }

    std::shared_ptr<Logger> scripting() {
        return Logger::get("scripting");
    }

    std::shared_ptr<Logger> database() {
        return Logger::get("database");
    }
}
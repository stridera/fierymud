#include "logging.hpp"

#include <spdlog/pattern_formatter.h>
#include <filesystem>

// Static member definition
std::unordered_map<std::string, std::shared_ptr<Logger>> Logger::loggers_;

// Static member for shared sinks
std::vector<spdlog::sink_ptr> Logger::sinks_;

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

std::shared_ptr<Logger> Logger::get(std::string_view component) {
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
        std::vector<spdlog::sink_ptr> empty_sinks;
        auto null_spdlog = std::make_shared<spdlog::logger>("null", empty_sinks.begin(), empty_sinks.end());
        null_spdlog->set_level(spdlog::level::off);
        auto logger = std::shared_ptr<Logger>(new Logger(null_spdlog));
        loggers_[comp_str] = logger;
        return logger;
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
    std::shared_ptr<Logger> game() {
        static auto logger = Logger::get("game");
        return logger;
    }
    
    std::shared_ptr<Logger> combat() {
        static auto logger = Logger::get("combat");
        return logger;
    }
    
    std::shared_ptr<Logger> movement() {
        static auto logger = Logger::get("movement");
        return logger;
    }
    
    std::shared_ptr<Logger> commands() {
        static auto logger = Logger::get("commands");
        return logger;
    }
    
    std::shared_ptr<Logger> network() {
        static auto logger = Logger::get("network");
        return logger;
    }
    
    std::shared_ptr<Logger> persistence() {
        static auto logger = Logger::get("persistence");
        return logger;
    }
    
    std::shared_ptr<Logger> scripting() {
        static auto logger = Logger::get("scripting");
        return logger;
    }

    std::shared_ptr<Logger> database() {
        static auto logger = Logger::get("database");
        return logger;
    }
}
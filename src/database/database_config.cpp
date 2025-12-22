#include "database/database_config.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"
#include <fmt/format.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

std::string DatabaseConfig::connection_string() const {
    return fmt::format("host={} port={} dbname={} user={} password={}",
                      host, port, dbname, user, password);
}

Result<DatabaseConfig> DatabaseConfig::from_env(const std::string& env_path) {
    auto logger = Log::database();
    logger->debug("Loading database configuration from: {}", env_path);

    DatabaseConfig config;

    // Check if file exists
    if (!std::filesystem::exists(env_path)) {
        logger->warn(".env file not found at: {}, falling back to environment variables", env_path);
        return from_environment();
    }

    // Parse .env file
    std::ifstream file(env_path);
    if (!file.is_open()) {
        return std::unexpected(Errors::FileNotFound(env_path));
    }

    std::string line;
    int line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse KEY=VALUE format
        auto equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            continue; // Skip malformed lines
        }

        std::string key{trim(line.substr(0, equals_pos))};
        std::string value{trim(line.substr(equals_pos + 1))};

        // Remove quotes if present
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        // Map to config fields
        if (key == "POSTGRES_HOST") {
            config.host = value;
        } else if (key == "POSTGRES_PORT") {
            try {
                config.port = std::stoi(value);
            } catch (const std::exception& e) {
                logger->warn("Invalid POSTGRES_PORT value '{}', using default: {}", value, config.port);
            }
        } else if (key == "POSTGRES_DB") {
            config.dbname = value;
        } else if (key == "POSTGRES_USER") {
            config.user = value;
        } else if (key == "POSTGRES_PASSWORD") {
            config.password = value;
        }
    }

    // Validate configuration
    if (!config.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("POSTGRES_PASSWORD not set"));
    }

    logger->info("Database configuration loaded: host={} port={} db={} user={}",
                config.host, config.port, config.dbname, config.user);

    return config;
}

Result<DatabaseConfig> DatabaseConfig::from_environment() {
    auto logger = Log::database();
    logger->debug("Loading database configuration from environment variables");

    DatabaseConfig config;

    // Read from environment variables with defaults
    if (const char* host = std::getenv("POSTGRES_HOST")) {
        config.host = host;
    }

    if (const char* port_str = std::getenv("POSTGRES_PORT")) {
        try {
            config.port = std::stoi(port_str);
        } catch (const std::exception& e) {
            logger->warn("Invalid POSTGRES_PORT environment variable, using default: {}", config.port);
        }
    }

    if (const char* dbname = std::getenv("POSTGRES_DB")) {
        config.dbname = dbname;
    }

    if (const char* user = std::getenv("POSTGRES_USER")) {
        config.user = user;
    }

    if (const char* password = std::getenv("POSTGRES_PASSWORD")) {
        config.password = password;
    }

    // Validate configuration
    if (!config.is_valid()) {
        return std::unexpected(Errors::InvalidArgument("POSTGRES_PASSWORD environment variable not set"));
    }

    logger->info("Database configuration loaded from environment: host={} port={} db={} user={}",
                config.host, config.port, config.dbname, config.user);

    return config;
}

bool DatabaseConfig::is_valid() const {
    return !password.empty() && !host.empty() && !dbname.empty() && !user.empty() && port > 0;
}

#pragma once

#include <string>

#include "core/result.hpp"

/**
 * Database configuration for PostgreSQL connection.
 *
 * Parses connection settings from .env file or environment variables.
 * Supports standard PostgreSQL connection parameters:
 * - POSTGRES_HOST (default: localhost)
 * - POSTGRES_PORT (default: 5432)
 * - POSTGRES_DB (default: fierydev)
 * - POSTGRES_USER (default: muditor)
 * - POSTGRES_PASSWORD (required)
 */
struct DatabaseConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string dbname = "fierydev";
    std::string user = "muditor";
    std::string password;

    /**
     * Generate libpqxx connection string.
     * Format: "host=... port=... dbname=... user=... password=..."
     */
    std::string connection_string() const;

    /**
     * Load configuration from .env file.
     *
     * @param env_path Path to .env file (relative to executable)
     * @return DatabaseConfig on success, Error on failure
     *
     * Example .env format:
     *   POSTGRES_HOST=localhost
     *   POSTGRES_PORT=5432
     *   POSTGRES_DB=fierydev
     *   POSTGRES_USER=muditor
     *   POSTGRES_PASSWORD=password
     */
    static Result<DatabaseConfig> from_env(const std::string &env_path = ".env");

    /**
     * Load configuration from environment variables.
     * Falls back to defaults if variables not set.
     */
    static Result<DatabaseConfig> from_environment();

    /**
     * Validate configuration completeness.
     * Ensures required fields (password) are set.
     */
    bool is_valid() const;
};

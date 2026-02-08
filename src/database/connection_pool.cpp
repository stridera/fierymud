#include "database/connection_pool.hpp"
#include "database/database_config.hpp"
#include "core/logging.hpp"
#include <fmt/format.h>

ConnectionPool& ConnectionPool::instance() {
    static ConnectionPool instance;
    return instance;
}

Result<void> ConnectionPool::initialize(const DatabaseConfig& config, std::size_t pool_size) {
    auto logger = Log::database();

    std::unique_lock lock(pool_mutex_);

    if (initialized_) {
        return std::unexpected(Error{ErrorCode::InvalidState,
            "ConnectionPool already initialized"});
    }

    if (!config.is_valid()) {
        return std::unexpected(Error{ErrorCode::InvalidArgument,
            "Invalid database configuration"});
    }

    connection_string_ = config.connection_string();
    pool_size_ = pool_size;

    logger->info("Initializing connection pool with {} connections", pool_size_);

    // Create initial connections
    for (std::size_t i = 0; i < pool_size_; ++i) {
        try {
            auto conn = std::make_unique<pqxx::connection>(connection_string_);

            if (!is_connection_healthy(*conn)) {
                logger->error("Connection {} failed health check during initialization", i);
                return std::unexpected(Error{ErrorCode::NetworkError,
                    fmt::format("Connection {} unhealthy", i)});
            }

            all_connections_.push_back(std::move(conn));
            logger->debug("Created database connection {}/{}", i + 1, pool_size_);

        } catch (const pqxx::broken_connection& e) {
            logger->error("Failed to connect to database: {}", e.what());
            // Clean up any connections we created
            all_connections_.clear();
            available_connections_ = {};
            return std::unexpected(Error{ErrorCode::NetworkError,
                fmt::format("Database connection failed: {}", e.what())});
        } catch (const std::exception& e) {
            logger->error("Unexpected error creating connection: {}", e.what());
            all_connections_.clear();
            available_connections_ = {};
            return std::unexpected(Error{ErrorCode::InternalError,
                fmt::format("Connection creation failed: {}", e.what())});
        }
    }

    // Make all connections available
    for (auto& conn : all_connections_) {
        available_connections_.push(std::move(conn));
    }

    initialized_ = true;
    logger->info("Connection pool initialized successfully: {} connections ready",
                available_connections_.size());

    return Success();
}

void ConnectionPool::shutdown() {
    auto logger = Log::database();
    std::unique_lock lock(pool_mutex_);

    if (!initialized_) {
        return;
    }

    logger->info("Shutting down connection pool");

    // Clear available connections
    while (!available_connections_.empty()) {
        available_connections_.pop();
    }

    // Close and clear all connections
    all_connections_.clear();

    initialized_ = false;
    logger->info("Connection pool shutdown complete");

    // Wake up any waiting threads
    pool_cv_.notify_all();
}

bool ConnectionPool::is_initialized() const {
    std::unique_lock lock(pool_mutex_);
    return initialized_;
}

std::unique_ptr<pqxx::connection> ConnectionPool::acquire_connection() {
    std::unique_lock lock(pool_mutex_);

    if (!initialized_) {
        Log::database()->error("Attempted to acquire connection from uninitialized pool");
        return nullptr;
    }

    // Wait for a connection to become available
    while (available_connections_.empty()) {
        Log::database()->debug("Waiting for available connection...");
        pool_cv_.wait(lock);

        if (!initialized_) {
            // Pool was shutdown while waiting
            return nullptr;
        }
    }

    // Get connection from queue
    auto conn = std::move(available_connections_.front());
    available_connections_.pop();

    Log::database()->trace("Acquired connection: {} available, {} active",
                          available_connections_.size(),
                          pool_size_ - available_connections_.size());

    return conn;
}

void ConnectionPool::release_connection(std::unique_ptr<pqxx::connection> conn) {
    if (!conn) {
        return;
    }

    std::unique_lock lock(pool_mutex_);

    if (!initialized_) {
        // Pool shutdown, just let connection be destroyed
        return;
    }

    // Check if connection is still healthy
    if (!is_connection_healthy(*conn)) {
        Log::database()->warn("Releasing unhealthy connection, creating replacement");

        try {
            // Create a new connection to replace the unhealthy one
            conn = std::make_unique<pqxx::connection>(connection_string_);
        } catch (const std::exception& e) {
            Log::database()->error("Failed to replace unhealthy connection: {}", e.what());
            // Don't add it back to the pool
            return;
        }
    }

    // Return connection to pool
    available_connections_.push(std::move(conn));

    Log::database()->trace("Released connection: {} available, {} active",
                          available_connections_.size(),
                          pool_size_ - available_connections_.size());

    // Notify waiting threads
    pool_cv_.notify_one();
}

bool ConnectionPool::is_connection_healthy(pqxx::connection& conn) {
    try {
        // Simple ping query
        pqxx::nontransaction txn(conn);
        txn.exec("SELECT 1");
        return true;
    } catch (const std::exception& e) {
        Log::database()->debug("Connection health check failed: {}", e.what());
        return false;
    }
}

ConnectionPool::PoolStats ConnectionPool::get_stats() const {
    std::unique_lock lock(pool_mutex_);

    return PoolStats{
        .total_connections = pool_size_,
        .available_connections = available_connections_.size(),
        .active_connections = pool_size_ - available_connections_.size()
    };
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

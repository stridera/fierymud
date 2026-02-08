// Thread-safe PostgreSQL connection pool via libpqxx

#pragma once

#include "core/result.hpp"
#include "database_config.hpp"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <shared_mutex>
#include <vector>

/**
 * Thread-safe connection pool for PostgreSQL.
 *
 * Features:
 * - Automatic connection pooling and reuse
 * - Thread-safe acquire/release operations
 * - Transaction support with RAII
 * - Graceful shutdown and cleanup
 * - Connection health checking
 *
 * Usage:
 *   auto result = ConnectionPool::instance().execute([](pqxx::work& txn) {
 *       auto rows = txn.exec("SELECT * FROM \"Room\" WHERE \"zoneId\" = 30");
 *       return parse_rooms(rows);
 *   });
 */
class ConnectionPool {
  public:
    /** Get singleton instance */
    static ConnectionPool &instance();

    /** Initialize connection pool with configuration */
    Result<void> initialize(const DatabaseConfig &config, std::size_t pool_size = 10);

    /** Shutdown connection pool and release all connections */
    void shutdown();

    /** Check if pool is initialized */
    bool is_initialized() const;

    /**
     * Execute a function with a database transaction.
     *
     * Automatically acquires a connection, creates a transaction,
     * executes the function, commits on success, and releases the connection.
     *
     * @param func Function taking pqxx::work& and returning Result<T>
     * @return Result<T> from function execution
     *
     * Example:
     *   auto zones = ConnectionPool::instance().execute([](pqxx::work& txn) {
     *       return WorldQueries::load_all_zones(txn);
     *   });
     */
    template <typename Func> auto execute(Func &&func) -> decltype(func(std::declval<pqxx::work &>())) {
        auto conn = acquire_connection();
        if (!conn) {
            return std::unexpected(Error{ErrorCode::ResourceExhausted, "Failed to acquire database connection"});
        }

        try {
            decltype(func(std::declval<pqxx::work &>())) result;
            {
                // transaction must be destroyed before releasing connection
                pqxx::work txn(*conn);
                result = func(txn);
                txn.commit();
            }
            release_connection(std::move(conn));
            return result;
        } catch (const pqxx::sql_error &e) {
            release_connection(std::move(conn));
            return std::unexpected(Error{ErrorCode::InternalError, fmt::format("SQL error: {}", e.what())});
        } catch (const std::exception &e) {
            release_connection(std::move(conn));
            return std::unexpected(Error{ErrorCode::InternalError, fmt::format("Database error: {}", e.what())});
        }
    }

    /**
     * Execute a read-only query (no transaction needed).
     * More efficient for simple queries that don't modify data.
     */
    template <typename Func>
    auto execute_read_only(Func &&func) -> decltype(func(std::declval<pqxx::nontransaction &>())) {
        auto conn = acquire_connection();
        if (!conn) {
            return std::unexpected(Error{ErrorCode::ResourceExhausted, "Failed to acquire database connection"});
        }

        try {
            decltype(func(std::declval<pqxx::nontransaction &>())) result;
            {
                // nontransaction must be destroyed before releasing connection
                pqxx::nontransaction txn(*conn);
                result = func(txn);
            }
            release_connection(std::move(conn));
            return result;
        } catch (const pqxx::sql_error &e) {
            release_connection(std::move(conn));
            return std::unexpected(Error{ErrorCode::InternalError, fmt::format("SQL error: {}", e.what())});
        } catch (const std::exception &e) {
            release_connection(std::move(conn));
            return std::unexpected(Error{ErrorCode::InternalError, fmt::format("Database error: {}", e.what())});
        }
    }

    /** Get current pool statistics */
    struct PoolStats {
        std::size_t total_connections;
        std::size_t available_connections;
        std::size_t active_connections;
    };
    PoolStats get_stats() const;

  private:
    ConnectionPool() = default;
    ~ConnectionPool();

    // Prevent copying
    ConnectionPool(const ConnectionPool &) = delete;
    ConnectionPool &operator=(const ConnectionPool &) = delete;

    /** Acquire a connection from the pool (blocks if none available) */
    std::unique_ptr<pqxx::connection> acquire_connection();

    /** Release a connection back to the pool */
    void release_connection(std::unique_ptr<pqxx::connection> conn);

    /** Check if a connection is still healthy */
    bool is_connection_healthy(pqxx::connection &conn);

    std::string connection_string_;
    std::size_t pool_size_ = 0;
    bool initialized_ = false;

    std::vector<std::unique_ptr<pqxx::connection>> all_connections_;
    std::queue<std::unique_ptr<pqxx::connection>> available_connections_;

    mutable std::mutex pool_mutex_;
    std::condition_variable pool_cv_;
};

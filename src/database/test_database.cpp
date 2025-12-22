#include "database/database_config.hpp"
#include "database/connection_pool.hpp"
#include "database/world_queries.hpp"
#include "core/logging.hpp"
#include <iostream>

int main() {
    std::cout << "=== FieryMUD Database Integration Test ===\n\n";

    // Initialize logging
    Logger::initialize("test_database.log", LogLevel::Debug, true);

    // Step 1: Load database configuration
    std::cout << "1. Loading database configuration from ../.env\n";
    auto db_config_result = DatabaseConfig::from_env("../.env");
    if (!db_config_result) {
        std::cerr << "   FAILED: " << db_config_result.error().message << "\n";
        return 1;
    }
    std::cout << "   SUCCESS: Connected to " << db_config_result->host
              << ":" << db_config_result->port << "\n\n";

    // Step 2: Initialize connection pool
    std::cout << "2. Initializing connection pool (10 connections)\n";
    auto pool_init = ConnectionPool::instance().initialize(*db_config_result, 10);
    if (!pool_init) {
        std::cerr << "   FAILED: " << pool_init.error().message << "\n";
        return 1;
    }
    std::cout << "   SUCCESS: Connection pool ready\n\n";

    // Step 3: Load all zones
    std::cout << "3. Loading zones from database\n";
    auto zones_result = ConnectionPool::instance().execute([](pqxx::work& txn) {
        return WorldQueries::load_all_zones(txn);
    });

    if (!zones_result) {
        std::cerr << "   FAILED: " << zones_result.error().message << "\n";
        return 1;
    }

    std::cout << "   SUCCESS: Loaded " << zones_result->size() << " zones\n";

    // Display first 5 zones
    std::cout << "   First 5 zones:\n";
    for (size_t i = 0; i < std::min(size_t(5), zones_result->size()); ++i) {
        const auto& zone = (*zones_result)[i];
        std::cout << "     - Zone " << zone->id().zone_id()
                  << ": " << zone->name() << "\n";
    }
    std::cout << "\n";

    // Step 4: Load rooms for first zone
    if (!zones_result->empty()) {
        int first_zone_id = (*zones_result)[0]->id().zone_id();
        std::cout << "4. Loading rooms for zone " << first_zone_id << "\n";

        auto rooms_result = ConnectionPool::instance().execute([first_zone_id](pqxx::work& txn) {
            return WorldQueries::load_rooms_in_zone(txn, first_zone_id);
        });

        if (!rooms_result) {
            std::cerr << "   FAILED: " << rooms_result.error().message << "\n";
            return 1;
        }

        std::cout << "   SUCCESS: Loaded " << rooms_result->size() << " rooms\n";

        // Display first 5 rooms
        std::cout << "   First 5 rooms:\n";
        for (size_t i = 0; i < std::min(size_t(5), rooms_result->size()); ++i) {
            const auto& room = (*rooms_result)[i];
            std::cout << "     - Room (" << room->id().zone_id()
                      << ", " << room->id().local_id() << "): "
                      << room->name() << "\n";
        }
        std::cout << "\n";
    }

    // Step 5: Check connection pool stats
    std::cout << "5. Connection pool statistics\n";
    auto stats = ConnectionPool::instance().get_stats();
    std::cout << "   Total connections: " << stats.total_connections << "\n";
    std::cout << "   Available: " << stats.available_connections << "\n";
    std::cout << "   Active: " << stats.active_connections << "\n\n";

    // Cleanup
    ConnectionPool::instance().shutdown();
    std::cout << "=== All Tests Passed! ===\n";

    return 0;
}

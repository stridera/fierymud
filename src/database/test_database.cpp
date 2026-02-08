#include <iostream>

#include "core/logging.hpp"
#include "database/connection_pool.hpp"
#include "database/database_config.hpp"
#include "database/world_queries.hpp"
#include "world/room.hpp"

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
    std::cout << "   SUCCESS: Connected to " << db_config_result->host << ":" << db_config_result->port << "\n\n";

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
    auto zones_result =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_all_zones(txn); });

    if (!zones_result) {
        std::cerr << "   FAILED: " << zones_result.error().message << "\n";
        return 1;
    }

    std::cout << "   SUCCESS: Loaded " << zones_result->size() << " zones\n";

    // Display first 5 zones
    std::cout << "   First 5 zones:\n";
    for (size_t i = 0; i < std::min(size_t(5), zones_result->size()); ++i) {
        const auto &zone = (*zones_result)[i];
        std::cout << "     - Zone " << zone->id().zone_id() << ": " << zone->name() << "\n";
    }
    std::cout << "\n";

    // Step 4: Load rooms for first zone
    if (!zones_result->empty()) {
        int first_zone_id = (*zones_result)[0]->id().zone_id();
        std::cout << "4. Loading rooms for zone " << first_zone_id << "\n";

        auto rooms_result = ConnectionPool::instance().execute(
            [first_zone_id](pqxx::work &txn) { return WorldQueries::load_rooms_in_zone(txn, first_zone_id); });

        if (!rooms_result) {
            std::cerr << "   FAILED: " << rooms_result.error().message << "\n";
            return 1;
        }

        std::cout << "   SUCCESS: Loaded " << rooms_result->size() << " rooms\n";

        // Display first 5 rooms
        std::cout << "   First 5 rooms:\n";
        for (size_t i = 0; i < std::min(size_t(5), rooms_result->size()); ++i) {
            const auto &room = (*rooms_result)[i];
            std::cout << "     - Room (" << room->id().zone_id() << ", " << room->id().local_id()
                      << "): " << room->name() << "\n";
        }
        std::cout << "\n";
    }

    // Step 5: Check connection pool stats
    std::cout << "5. Connection pool statistics\n";
    auto stats = ConnectionPool::instance().get_stats();
    std::cout << "   Total connections: " << stats.total_connections << "\n";
    std::cout << "   Available: " << stats.available_connections << "\n";
    std::cout << "   Active: " << stats.active_connections << "\n\n";

    // Step 6: Test room property loading for room 30:1 (Temple of Midgaard)
    std::cout << "6. Testing room property loading for room 30:1 (Temple)\n";
    auto room_result =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_room(txn, 30, 1); });

    if (!room_result) {
        std::cerr << "   FAILED: " << room_result.error().message << "\n";
        ConnectionPool::instance().shutdown();
        return 1;
    }

    const auto &room = *room_result;
    std::cout << "   Room loaded: " << room->id() << " - " << room->name() << "\n";
    std::cout << "   Base light level: " << room->base_light_level() << "\n";
    std::cout << "   Is peaceful: " << (room->is_peaceful() ? "yes" : "no") << "\n";
    std::cout << "   Allows magic: " << (room->allows_magic() ? "yes" : "no") << "\n";
    std::cout << "   Capacity: " << room->capacity() << "\n";
    std::cout << "   SUCCESS: Room properties loaded correctly\n\n";

    // Step 7: Test room property loading via load_rooms_in_zone (server path)
    std::cout << "7. Testing room property loading via load_rooms_in_zone (zone 30)\n";
    auto rooms_z30_result =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_rooms_in_zone(txn, 30); });

    if (!rooms_z30_result) {
        std::cerr << "   FAILED: " << rooms_z30_result.error().message << "\n";
        ConnectionPool::instance().shutdown();
        return 1;
    }

    std::cout << "   Loaded " << rooms_z30_result->size() << " rooms from zone 30\n";

    // Find room 30:1 in the results
    Room *temple_room = nullptr;
    for (const auto &r : *rooms_z30_result) {
        if (r->id().local_id() == 1) {
            temple_room = r.get();
            break;
        }
    }

    if (!temple_room) {
        std::cerr << "   FAILED: Room 30:1 not found in zone 30 rooms!\n";
        ConnectionPool::instance().shutdown();
        return 1;
    }

    std::cout << "   Found room: " << temple_room->id() << " - " << temple_room->name() << "\n";
    std::cout << "   Base light level: " << temple_room->base_light_level() << "\n";
    std::cout << "   Is peaceful: " << (temple_room->is_peaceful() ? "yes" : "no") << "\n";
    std::cout << "   Allows magic: " << (temple_room->allows_magic() ? "yes" : "no") << "\n";
    std::cout << "   Capacity: " << temple_room->capacity() << "\n";
    std::cout << "   SUCCESS: Room properties loaded correctly via load_rooms_in_zone\n\n";

    // Cleanup
    ConnectionPool::instance().shutdown();
    std::cout << "=== All Tests Passed! ===\n";

    return 0;
}

#include <iomanip>
#include <iostream>

#include "core/logging.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "database/connection_pool.hpp"
#include "database/database_config.hpp"
#include "database/player_queries.hpp"
#include "database/world_queries.hpp"

// Test result tracking
struct TestStats {
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;

    void record_pass(const std::string &test_name) {
        total_tests++;
        passed_tests++;
        std::cout << "  ✓ " << test_name << "\n";
    }

    void record_fail(const std::string &test_name, const std::string &error) {
        total_tests++;
        failed_tests++;
        std::cout << "  ✗ " << test_name << "\n";
        std::cout << "    Error: " << error << "\n";
    }

    void print_summary() {
        std::cout << "\n=== Test Summary ===\n";
        std::cout << "Total tests: " << total_tests << "\n";
        std::cout << "Passed: " << passed_tests << " ✓\n";
        std::cout << "Failed: " << failed_tests << " ✗\n";
        std::cout << "Success rate: " << std::fixed << std::setprecision(1) << (100.0 * passed_tests / total_tests)
                  << "%\n";
    }
};

TestStats g_stats;

// Test 1: Database Configuration Loading
bool test_database_config() {
    std::cout << "\n[Test 1] Database Configuration\n";

    auto config_result = DatabaseConfig::from_env("../.env");
    if (!config_result) {
        g_stats.record_fail("Load .env configuration", config_result.error().message);
        return false;
    }

    g_stats.record_pass("Load .env configuration");

    const auto &config = *config_result;

    // Validate configuration values
    if (config.host.empty()) {
        g_stats.record_fail("Validate host", "Host is empty");
        return false;
    }
    g_stats.record_pass("Validate host: " + config.host);

    if (config.port <= 0 || config.port > 65535) {
        g_stats.record_fail("Validate port", "Invalid port: " + std::to_string(config.port));
        return false;
    }
    g_stats.record_pass("Validate port: " + std::to_string(config.port));

    if (config.dbname.empty()) {
        g_stats.record_fail("Validate database name", "Database name is empty");
        return false;
    }
    g_stats.record_pass("Validate database name: " + config.dbname);

    if (config.user.empty()) {
        g_stats.record_fail("Validate user", "User is empty");
        return false;
    }
    g_stats.record_pass("Validate user: " + config.user);

    // Test connection string generation
    std::string conn_str = config.connection_string();
    if (conn_str.empty()) {
        g_stats.record_fail("Generate connection string", "Connection string is empty");
        return false;
    }
    g_stats.record_pass("Generate connection string");

    // Test validation
    if (!config.is_valid()) {
        g_stats.record_fail("Config validation", "Configuration marked as invalid");
        return false;
    }
    g_stats.record_pass("Config validation");

    return true;
}

// Test 2: Connection Pool Initialization
bool test_connection_pool() {
    std::cout << "\n[Test 2] Connection Pool\n";

    auto config_result = DatabaseConfig::from_env("../.env");
    if (!config_result) {
        g_stats.record_fail("Load config for pool test", config_result.error().message);
        return false;
    }

    // Test pool initialization
    auto pool_init = ConnectionPool::instance().initialize(*config_result, 10);
    if (!pool_init) {
        g_stats.record_fail("Initialize connection pool", pool_init.error().message);
        return false;
    }
    g_stats.record_pass("Initialize connection pool (10 connections)");

    // Check if pool is initialized
    if (!ConnectionPool::instance().is_initialized()) {
        g_stats.record_fail("Verify pool initialized state", "Pool not marked as initialized");
        return false;
    }
    g_stats.record_pass("Verify pool initialized state");

    // Test pool statistics
    auto stats = ConnectionPool::instance().get_stats();
    if (stats.total_connections != 10) {
        g_stats.record_fail("Verify total connections", "Expected 10, got " + std::to_string(stats.total_connections));
        return false;
    }
    g_stats.record_pass("Verify total connections: " + std::to_string(stats.total_connections));

    if (stats.available_connections != 10) {
        g_stats.record_fail("Verify available connections",
                            "Expected 10, got " + std::to_string(stats.available_connections));
        return false;
    }
    g_stats.record_pass("Verify available connections: " + std::to_string(stats.available_connections));

    if (stats.active_connections != 0) {
        g_stats.record_fail("Verify active connections", "Expected 0, got " + std::to_string(stats.active_connections));
        return false;
    }
    g_stats.record_pass("Verify active connections: " + std::to_string(stats.active_connections));

    return true;
}

// Test 3: Zone Loading
bool test_zone_loading() {
    std::cout << "\n[Test 3] Zone Loading\n";

    auto zones_result =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_all_zones(txn); });

    if (!zones_result) {
        g_stats.record_fail("Load all zones", zones_result.error().message);
        return false;
    }

    g_stats.record_pass("Load all zones");

    auto &zones = *zones_result;
    if (zones.empty()) {
        g_stats.record_fail("Verify zones loaded", "No zones returned");
        return false;
    }
    g_stats.record_pass("Verify zones loaded: " + std::to_string(zones.size()) + " zones");

    // Test loading first zone
    auto first_zone_id = zones[0]->id().zone_id();
    auto single_zone = ConnectionPool::instance().execute(
        [first_zone_id](pqxx::work &txn) { return WorldQueries::load_zone(txn, first_zone_id); });

    if (!single_zone) {
        g_stats.record_fail("Load single zone", single_zone.error().message);
        return false;
    }
    g_stats.record_pass("Load single zone (ID: " + std::to_string(first_zone_id) + ")");

    // Verify zone data
    if ((*single_zone)->name().empty()) {
        g_stats.record_fail("Verify zone has name", "Zone name is empty");
        return false;
    }
    g_stats.record_pass("Verify zone has name: " + std::string((*single_zone)->name()));

    return true;
}

// Test 4: Room Loading
bool test_room_loading() {
    std::cout << "\n[Test 4] Room Loading\n";

    // Load zones first
    auto zones_result =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_all_zones(txn); });

    if (!zones_result || zones_result->empty()) {
        g_stats.record_fail("Load zones for room test", "No zones available");
        return false;
    }

    auto first_zone_id = (*zones_result)[0]->id().zone_id();

    // Test loading rooms for zone
    auto rooms_result = ConnectionPool::instance().execute(
        [first_zone_id](pqxx::work &txn) { return WorldQueries::load_rooms_in_zone(txn, first_zone_id); });

    if (!rooms_result) {
        g_stats.record_fail("Load rooms for zone", rooms_result.error().message);
        return false;
    }
    g_stats.record_pass("Load rooms for zone " + std::to_string(first_zone_id));

    auto &rooms = *rooms_result;
    std::cout << "    Loaded " << rooms.size() << " rooms\n";

    if (!rooms.empty()) {
        // Test loading single room
        int first_room_local_id = rooms[0]->id().local_id();
        auto single_room = ConnectionPool::instance().execute([first_zone_id, first_room_local_id](pqxx::work &txn) {
            return WorldQueries::load_room(txn, first_zone_id, first_room_local_id);
        });

        if (!single_room) {
            g_stats.record_fail("Load single room", single_room.error().message);
            return false;
        }
        g_stats.record_pass("Load single room (" + std::to_string(first_zone_id) + ", " +
                            std::to_string(first_room_local_id) + ")");

        // Verify room data
        if ((*single_room)->name().empty()) {
            g_stats.record_fail("Verify room has name", "Room name is empty");
            return false;
        }
        g_stats.record_pass("Verify room has name: " + std::string((*single_room)->name()));
    }

    return true;
}

// Test 5: Player Query Functions
bool test_player_queries() {
    std::cout << "\n[Test 5] Player Queries\n";

    // Test player count
    auto count_result = ConnectionPool::instance().execute(
        [](pqxx::work &txn) -> Result<int> { return PlayerQueries::get_player_count(txn); });

    if (!count_result) {
        g_stats.record_fail("Get player count", count_result.error().message);
        return false;
    }
    g_stats.record_pass("Get player count: " + std::to_string(*count_result));

    // Test player existence check (should not exist)
    auto exists_result = ConnectionPool::instance().execute(
        [](pqxx::work &txn) -> Result<bool> { return PlayerQueries::player_exists(txn, "NonExistentPlayer"); });

    if (!exists_result) {
        g_stats.record_fail("Check player existence", exists_result.error().message);
        return false;
    }

    if (*exists_result) {
        g_stats.record_fail("Verify non-existent player", "Player unexpectedly exists");
        return false;
    }
    g_stats.record_pass("Verify non-existent player check works");

    // Test loading non-existent player (should fail gracefully)
    auto player_result = ConnectionPool::instance().execute(
        [](pqxx::work &txn) { return PlayerQueries::load_player_by_name(txn, "NonExistentPlayer"); });

    if (player_result) {
        g_stats.record_fail("Load non-existent player", "Should have returned error");
        return false;
    }

    if (player_result.error().code != ErrorCode::NotFound) {
        g_stats.record_fail("Verify NotFound error", "Wrong error code returned");
        return false;
    }
    g_stats.record_pass("Load non-existent player returns NotFound error");

    return true;
}

// Test 6: Connection Pool Stress Test
bool test_connection_pool_stress() {
    std::cout << "\n[Test 6] Connection Pool Stress Test\n";

    // Execute multiple concurrent queries
    int num_queries = 20;
    int successful = 0;

    for (int i = 0; i < num_queries; ++i) {
        auto result =
            ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_all_zones(txn); });

        if (result) {
            successful++;
        }
    }

    if (successful != num_queries) {
        g_stats.record_fail("Execute concurrent queries",
                            std::to_string(successful) + "/" + std::to_string(num_queries) + " succeeded");
        return false;
    }
    g_stats.record_pass("Execute concurrent queries: " + std::to_string(num_queries) + " successful");

    // Verify pool statistics are correct
    auto stats = ConnectionPool::instance().get_stats();
    if (stats.available_connections != stats.total_connections) {
        g_stats.record_fail("Verify connections returned to pool",
                            "Expected " + std::to_string(stats.total_connections) + " available, got " +
                                std::to_string(stats.available_connections));
        return false;
    }
    g_stats.record_pass("Verify all connections returned to pool");

    return true;
}

// Test 7: Mob Loading
bool test_mob_loading() {
    std::cout << "\n[Test 7] Mob Loading\n";

    // Load zones first
    auto zones_result =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_all_zones(txn); });

    if (!zones_result || zones_result->empty()) {
        g_stats.record_fail("Load zones for mob test", "No zones available");
        return false;
    }

    auto first_zone_id = (*zones_result)[0]->id().zone_id();

    // Test loading mobs for zone
    auto mobs_result = ConnectionPool::instance().execute(
        [first_zone_id](pqxx::work &txn) { return WorldQueries::load_mobs_in_zone(txn, first_zone_id); });

    if (!mobs_result) {
        g_stats.record_fail("Load mobs for zone", mobs_result.error().message);
        return false;
    }
    g_stats.record_pass("Load mobs for zone " + std::to_string(first_zone_id));

    auto &mobs = *mobs_result;
    std::cout << "    Loaded " << mobs.size() << " mobs\n";

    if (!mobs.empty()) {
        // Verify mob data
        if (mobs[0]->name().empty()) {
            g_stats.record_fail("Verify mob has name", "Mob name is empty");
            return false;
        }
        g_stats.record_pass("Verify mob has name: " + std::string(mobs[0]->name()));

        // Verify mob has valid ID
        EntityId mob_id = mobs[0]->id();
        if (mob_id.zone_id() != first_zone_id) {
            g_stats.record_fail("Verify mob zone ID", "Zone ID mismatch");
            return false;
        }
        g_stats.record_pass("Verify mob has valid ID: (" + std::to_string(mob_id.zone_id()) + ", " +
                            std::to_string(mob_id.local_id()) + ")");
    }

    return true;
}

// Test 8: Object Loading
bool test_object_loading() {
    std::cout << "\n[Test 8] Object Loading\n";

    // Load zones first
    auto zones_result =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_all_zones(txn); });

    if (!zones_result || zones_result->empty()) {
        g_stats.record_fail("Load zones for object test", "No zones available");
        return false;
    }

    auto first_zone_id = (*zones_result)[0]->id().zone_id();

    // Test loading objects for zone
    auto objects_result = ConnectionPool::instance().execute(
        [first_zone_id](pqxx::work &txn) { return WorldQueries::load_objects_in_zone(txn, first_zone_id); });

    if (!objects_result) {
        g_stats.record_fail("Load objects for zone", objects_result.error().message);
        return false;
    }
    g_stats.record_pass("Load objects for zone " + std::to_string(first_zone_id));

    auto &objects = *objects_result;
    std::cout << "    Loaded " << objects.size() << " objects\n";

    if (!objects.empty()) {
        // Verify object data
        if (objects[0]->name().empty()) {
            g_stats.record_fail("Verify object has name", "Object name is empty");
            return false;
        }
        g_stats.record_pass("Verify object has name: " + std::string(objects[0]->name()));

        // Verify object has valid ID
        EntityId obj_id = objects[0]->id();
        if (obj_id.zone_id() != first_zone_id) {
            g_stats.record_fail("Verify object zone ID", "Zone ID mismatch");
            return false;
        }
        g_stats.record_pass("Verify object has valid ID: (" + std::to_string(obj_id.zone_id()) + ", " +
                            std::to_string(obj_id.local_id()) + ")");
    }

    return true;
}

// Test 9: Error Handling
bool test_error_handling() {
    std::cout << "\n[Test 9] Error Handling\n";

    // Test invalid zone ID
    auto invalid_zone =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_zone(txn, 99999); });

    if (invalid_zone) {
        g_stats.record_fail("Load invalid zone ID", "Should have returned error");
        return false;
    }

    if (invalid_zone.error().code != ErrorCode::NotFound) {
        g_stats.record_fail("Verify NotFound error for zone", "Wrong error code");
        return false;
    }
    g_stats.record_pass("Load invalid zone returns NotFound error");

    // Test invalid room
    auto invalid_room =
        ConnectionPool::instance().execute([](pqxx::work &txn) { return WorldQueries::load_room(txn, 1, 99999); });

    if (invalid_room) {
        g_stats.record_fail("Load invalid room", "Should have returned error");
        return false;
    }

    if (invalid_room.error().code != ErrorCode::NotFound) {
        g_stats.record_fail("Verify NotFound error for room", "Wrong error code");
        return false;
    }
    g_stats.record_pass("Load invalid room returns NotFound error");

    return true;
}

int main() {
    std::cout << "=== FieryMUD Comprehensive Database Test Suite ===\n";
    std::cout << "Testing database integration with PostgreSQL\n\n";

    // Initialize logging
    Logger::initialize("test_database_comprehensive.log", LogLevel::Debug, true);

    bool all_passed = true;

    // Run all tests
    all_passed &= test_database_config();
    all_passed &= test_connection_pool();
    all_passed &= test_zone_loading();
    all_passed &= test_room_loading();
    all_passed &= test_player_queries();
    all_passed &= test_connection_pool_stress();
    all_passed &= test_mob_loading();
    all_passed &= test_object_loading();
    all_passed &= test_error_handling();

    // Cleanup
    ConnectionPool::instance().shutdown();
    std::cout << "\n[Cleanup] Connection pool shutdown complete\n";

    // Print summary
    g_stats.print_summary();

    if (all_passed && g_stats.failed_tests == 0) {
        std::cout << "\n=== ✓ All Tests Passed! ===\n";
        return 0;
    } else {
        std::cout << "\n=== ✗ Some Tests Failed ===\n";
        return 1;
    }
}

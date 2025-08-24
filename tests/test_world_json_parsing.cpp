/***************************************************************************
 *   File: tests/test_world_json_parsing.cpp              Part of FieryMUD *
 *  Usage: Unit tests for world manager JSON parsing functionality         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "../src/world/world_manager.hpp"
#include "../src/world/room.hpp"
#include "../src/world/zone.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Helper class for testing world manager room parsing
class TestWorldManager {
public:
    static Result<void> test_load_zone_json(const json& zone_json, const std::string& zone_name) {
        // Create temporary file for testing
        fs::path temp_file = fs::temp_directory_path() / ("test_zone_" + zone_name + ".json");
        std::ofstream file(temp_file);
        file << zone_json.dump(2);
        file.close();
        
        auto& wm = WorldManager::instance();
        auto result = wm.load_zone_file(temp_file.string());
        
        // Cleanup
        fs::remove(temp_file);
        
        return result;
    }
};

TEST_CASE("WorldManager room parsing with objects and ID references", "[world][manager][rooms]") {
    
    SECTION("Parse zone with room objects") {
        auto& world_manager = WorldManager::instance();
        world_manager.clear_state();
        
        json zone_json = {
            {"zone", {
                {"id", "1000"},
                {"name", "Room Object Zone"},
                {"top", 1099},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"rooms", {
                {
                    {"id", "1001"},
                    {"name", "Test Room 1"},
                    {"description", "A test room with full data."},
                    {"flags", ""},
                    {"sector", "CITY"},
                    {"exits", {}}
                },
                {
                    {"id", "1002"},
                    {"name", "Test Room 2"},
                    {"description", "Another test room."},
                    {"flags", ""},
                    {"sector", "FOREST"},
                    {"exits", {}}
                }
            }}
        };
        
        auto result = TestWorldManager::test_load_zone_json(zone_json, "1000");
        REQUIRE(result.has_value());
        
        // Verify rooms were loaded
        auto room1 = world_manager.get_room(EntityId{1001});
        auto room2 = world_manager.get_room(EntityId{1002});
        
        REQUIRE(room1 != nullptr);
        REQUIRE(room2 != nullptr);
        REQUIRE(room1->name() == "Test Room 1");
        REQUIRE(room2->name() == "Test Room 2");
    }
    
    SECTION("Parse zone with room ID references") {
        auto& world_manager = WorldManager::instance();
        world_manager.clear_state();
        
        json zone_json = {
            {"zone", {
                {"id", "2000"},
                {"name", "Room ID Zone"},
                {"top", 2099},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"rooms", {2001, 2002, 2003}}  // Just room IDs, not objects
        };
        
        auto result = TestWorldManager::test_load_zone_json(zone_json, "2000");
        REQUIRE(result.has_value());
        
        // Zone should be created and room IDs should be referenced
        auto zone = world_manager.get_zone(EntityId{2000});
        REQUIRE(zone != nullptr);
        REQUIRE(zone->name() == "Room ID Zone");
    }
    
    SECTION("Parse zone with mixed room objects and ID references") {
        auto& world_manager = WorldManager::instance();
        world_manager.clear_state();
        
        json zone_json = {
            {"zone", {
                {"id", "3000"},
                {"name", "Mixed Room Zone"},
                {"top", 3099},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"rooms", {
                {
                    {"id", "3001"},
                    {"name", "Full Room Object"},
                    {"description", "A room with complete data."},
                    {"flags", ""},
                    {"sector", "PLAINS"},
                    {"exits", {}}
                },
                3002,  // Just an ID reference
                {
                    {"id", "3003"},
                    {"name", "Another Full Room"},
                    {"description", "Another room with complete data."},
                    {"flags", ""},
                    {"sector", "MOUNTAIN"},
                    {"exits", {}}
                }
            }}
        };
        
        auto result = TestWorldManager::test_load_zone_json(zone_json, "3000");
        REQUIRE(result.has_value());
        
        // Verify the room objects were loaded
        auto room1 = world_manager.get_room(EntityId{3001});
        auto room3 = world_manager.get_room(EntityId{3003});
        
        REQUIRE(room1 != nullptr);
        REQUIRE(room3 != nullptr);
        REQUIRE(room1->name() == "Full Room Object");
        REQUIRE(room3->name() == "Another Full Room");
        
        // Room 3002 should be referenced but not loaded (since it's just an ID)
        auto room2 = world_manager.get_room(EntityId{3002});
        REQUIRE(room2 == nullptr);  // Not loaded, just referenced
    }
    
    SECTION("Parse zone with empty rooms array") {
        auto& world_manager = WorldManager::instance();
        world_manager.clear_state();
        
        json zone_json = {
            {"zone", {
                {"id", "4000"},
                {"name", "Empty Rooms Zone"},
                {"top", 4099},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"rooms", {}}  // Empty array
        };
        
        auto result = TestWorldManager::test_load_zone_json(zone_json, "4000");
        REQUIRE(result.has_value());
        
        auto zone = world_manager.get_zone(EntityId{4000});
        REQUIRE(zone != nullptr);
        REQUIRE(zone->name() == "Empty Rooms Zone");
    }
    
    SECTION("Parse zone without rooms array") {
        auto& world_manager = WorldManager::instance();
        world_manager.clear_state();
        
        json zone_json = {
            {"zone", {
                {"id", "5000"},
                {"name", "No Rooms Zone"},
                {"top", 5099},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }}
            // No "rooms" field at all
        };
        
        auto result = TestWorldManager::test_load_zone_json(zone_json, "5000");
        REQUIRE(result.has_value());
        
        auto zone = world_manager.get_zone(EntityId{5000});
        REQUIRE(zone != nullptr);
        REQUIRE(zone->name() == "No Rooms Zone");
    }
}

TEST_CASE("WorldManager room parsing error handling", "[world][manager][rooms][errors]") {
    
    SECTION("Handle invalid room entry types gracefully") {
        auto& world_manager = WorldManager::instance();
        world_manager.clear_state();
        
        json zone_json = {
            {"zone", {
                {"id", "6000"},
                {"name", "Invalid Entry Zone"},
                {"top", 6099},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"rooms", {
                {
                    {"id", "6001"},
                    {"name", "Valid Room"},
                    {"description", "A valid room."},
                    {"flags", ""},
                    {"sector", "CITY"},
                    {"exits", {}}
                },
                "invalid_string_entry",  // Invalid - string that's not parseable as room ID
                {
                    {"id", "6002"},
                    {"name", "Another Valid Room"},
                    {"description", "Another valid room."},
                    {"flags", ""},
                    {"sector", "FOREST"},
                    {"exits", {}}
                }
            }}
        };
        
        auto result = TestWorldManager::test_load_zone_json(zone_json, "6000");
        REQUIRE(result.has_value());  // Should succeed despite invalid entry
        
        // Valid rooms should still be loaded
        auto room1 = world_manager.get_room(EntityId{6001});
        auto room2 = world_manager.get_room(EntityId{6002});
        
        REQUIRE(room1 != nullptr);
        REQUIRE(room2 != nullptr);
        REQUIRE(room1->name() == "Valid Room");
        REQUIRE(room2->name() == "Another Valid Room");
    }
    
    SECTION("Handle room object parsing errors gracefully") {
        auto& world_manager = WorldManager::instance();
        world_manager.clear_state();
        
        json zone_json = {
            {"zone", {
                {"id", "7000"},
                {"name", "Room Error Zone"},
                {"top", 7099},
                {"lifespan", 30},
                {"reset_mode", "Normal"}
            }},
            {"rooms", {
                {
                    {"id", "7001"},
                    {"name", "Valid Room"},
                    {"description", "A valid room."},
                    {"flags", ""},
                    {"sector", "CITY"},
                    {"exits", {}}
                },
                {
                    // Missing required "id" field - should cause parsing error
                    {"name", "Invalid Room"},
                    {"description", "A room without ID."},
                    {"flags", ""},
                    {"sector", "FOREST"},
                    {"exits", {}}
                }
            }}
        };
        
        auto result = TestWorldManager::test_load_zone_json(zone_json, "7000");
        REQUIRE(result.has_value());  // Should succeed despite room parsing error
        
        // Valid room should still be loaded
        auto room1 = world_manager.get_room(EntityId{7001});
        REQUIRE(room1 != nullptr);
        REQUIRE(room1->name() == "Valid Room");
    }
}
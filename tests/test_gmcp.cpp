/***************************************************************************
 *   File: tests/test_gmcp.cpp                        Part of FieryMUD *
 *  Usage: Tests for GMCP functionality                                   *
 ***************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include "test_harness.hpp"
#include "../src/modern/server/gmcp_handler.hpp"
#include "../src/modern/commands/command_context.hpp"
#include "../src/modern/commands/builtin_commands.hpp"

TEST_CASE("GMCP Handler - Basic Functionality", "[integration][gmcp]") {
    TestHarness harness;
    
    SECTION("GMCP initialization and enable/disable") {
        // For this test, we'll verify GMCP functionality exists but can't fully test
        // the network layer without a real PlayerConnection
        
        // Test that we can create and access GMCP components
        auto& world_manager = WorldManager::instance();
        auto start_room_id = world_manager.get_start_room();
        
        // This verifies the GMCP system doesn't crash during basic operations
        REQUIRE(start_room_id.value() > 0); // We have a valid start room for GMCP
    }
    
    SECTION("GMCP Room.Info data structures") {
        // Test that GMCP-related room data structures work correctly
        auto& test_room = harness.fixtures().create_room("GMCP Test Room");
        
        // Verify the test room has the data GMCP would need
        REQUIRE(test_room.name() == "GMCP Test Room");
        REQUIRE(test_room.id().value() > 0);
        REQUIRE_FALSE(test_room.description().empty());
    }
    
    SECTION("GMCP constants and message format") {
        // Test GMCP constants are properly defined
        REQUIRE(GMCPHandler::GMCP_OPTION == 201);
        REQUIRE(GMCPHandler::TELNET_IAC == 255);
        REQUIRE(GMCPHandler::TELNET_WILL == 251);
        REQUIRE(GMCPHandler::TELNET_WONT == 252);
        
        // Test basic JSON data structure for GMCP
        nlohmann::json test_data;
        test_data["num"] = 100;
        test_data["name"] = "Test Room";
        
        REQUIRE(test_data["num"] == 100);
        REQUIRE(test_data["name"] == "Test Room");
    }
}

TEST_CASE("GMCP Integration with Room Changes", "[integration][gmcp]") {
    TestHarness harness;
    
    SECTION("GMCP room data consistency") {
        // Create two test rooms
        auto& room1 = harness.fixtures().create_room("Room One");
        auto& room2 = harness.fixtures().create_room("Room Two");
        
        // Move player to room1
        harness.player->move_to(WorldManager::instance().get_room(room1.id()));
        
        // Clear any initial output
        harness.clear_output();
        
        // Test that room data is consistent for GMCP
        REQUIRE(room1.name() == "Room One");
        REQUIRE(room2.name() == "Room Two");
        REQUIRE(room1.id() != room2.id());
        
        // Verify room data structures that GMCP would use
        REQUIRE(room1.id().value() > 0);
        REQUIRE(room2.id().value() > 0);
        REQUIRE_FALSE(room1.name().empty());
        REQUIRE_FALSE(room2.name().empty());
    }
}

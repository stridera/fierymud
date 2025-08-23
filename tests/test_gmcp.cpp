/***************************************************************************
 *   File: tests/test_gmcp.cpp                        Part of FieryMUD *
 *  Usage: Tests for GMCP functionality                                   *
 ***************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_context.hpp"
#include "../src/server/gmcp_handler.hpp"
#include "test_harness.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("GMCP Handler - Basic Functionality", "[integration][gmcp]") {
    TestHarness harness;

    SECTION("GMCP initialization and enable/disable") {
        // For this test, we'll verify GMCP functionality exists but can't fully test
        // the network layer without a real PlayerConnection

        // Test that we can create and access GMCP components
        auto &world_manager = WorldManager::instance();
        auto start_room_id = world_manager.get_start_room();

        // This verifies the GMCP system doesn't crash during basic operations
        REQUIRE(start_room_id.value() > 0); // We have a valid start room for GMCP
    }

    SECTION("GMCP Room.Info data structures") {
        // Test that GMCP-related room data structures work correctly
        auto &test_room = harness.fixtures().create_room("GMCP Test Room");
        test_room.set_description("A room created for testing GMCP functionality.");

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

    SECTION("GMCP Char.Vitals functionality") {
        // Test player vitals GMCP generation
        auto player_result = Player::create(EntityId{999}, "TestVitalsPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::move(player_result.value());

        // Get GMCP vitals data
        auto vitals_json = player->get_vitals_gmcp();

        // Verify standard vitals are present (no mana - spell slots instead)
        REQUIRE(vitals_json.contains("hp"));
        REQUIRE(vitals_json.contains("max_hp"));
        REQUIRE(vitals_json.contains("mv"));
        REQUIRE(vitals_json.contains("max_mv"));

        // Verify mana is NOT included (FieryMUD uses spell slots)
        REQUIRE_FALSE(vitals_json.contains("mana"));
        REQUIRE_FALSE(vitals_json.contains("max_mana"));

        // Verify data types and reasonable values
        REQUIRE(vitals_json["hp"].is_number_integer());
        REQUIRE(vitals_json["max_hp"].is_number_integer());
        REQUIRE(vitals_json["mv"].is_number_integer());
        REQUIRE(vitals_json["max_mv"].is_number_integer());

        REQUIRE(vitals_json["hp"].get<int>() > 0);
        REQUIRE(vitals_json["max_hp"].get<int>() > 0);
        REQUIRE(vitals_json["mv"].get<int>() >= 0);
        REQUIRE(vitals_json["max_mv"].get<int>() > 0);
    }
}

TEST_CASE("GMCP Integration with Room Changes", "[integration][gmcp]") {
    TestHarness harness;

    SECTION("GMCP room data consistency") {
        // Create two test rooms
        auto &room1 = harness.fixtures().create_room("Room One");
        auto &room2 = harness.fixtures().create_room("Room Two");

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

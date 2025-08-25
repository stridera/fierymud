/***************************************************************************
 *   File: tests/test_world.cpp                       Part of FieryMUD *
 *  Usage: Integration tests for world management and interaction        *
 ***************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_system.hpp"
#include "../src/core/ids.hpp"
#include "../src/core/object.hpp"
#include "mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("World: Room and Actor Integration", "[world][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();

        auto connect_result = session->connect("WorldTester");
        REQUIRE(connect_result.has_value());

        // Test room access through session
        auto starting_room = session->get_room(EntityId{100UL});
        REQUIRE(starting_room != nullptr);
        REQUIRE(starting_room->name() == "Starting Room");

        // Test actor presence in room
        auto actors = session->get_actors_in_room(EntityId{100UL});
        REQUIRE_FALSE(actors.empty());

        // Should have player and NPCs
        bool found_player = false;
        bool found_npc = false;

        for (const auto &actor : actors) {
            if (actor->name() == "WorldTester") {
                found_player = true;
            } else if (actor->name().find("guard") != std::string::npos) {
                found_npc = true;
            }
        }

        REQUIRE(found_player);
        REQUIRE(found_npc);

        session->disconnect();
    });
}

TEST_CASE("World: Object short_description Display", "[world][object][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();
        
        auto connect_result = session->connect("ObjectTester");
        REQUIRE(connect_result.has_value());
        
        // Get the starting room
        auto current_room_id = session->current_room();
        auto room = session->get_room(current_room_id);
        REQUIRE(room != nullptr);
        
        // Create a test object with specific short_description that differs from name
        nlohmann::json object_json = {
            {"id", "9999"},
            {"type", "OTHER"},
            {"name_list", "test sculpture artifact"},
            {"short_description", "a mysterious glowing sculpture"}
        };
        
        auto object_result = Object::from_json(object_json);
        REQUIRE(object_result.has_value());
        
        auto test_object = std::move(object_result.value());
        REQUIRE(test_object->name() == "test sculpture artifact");
        REQUIRE(test_object->short_description() == "a mysterious glowing sculpture");
        
        // Add the object to the room
        room->add_object(std::shared_ptr<Object>(test_object.release()));
        
        // Use the look command to see room contents
        session->send_input("look");
        
        // Wait for and get the output
        bool found_output = session->wait_for_output_containing("mysterious glowing sculpture", std::chrono::milliseconds(1000));
        REQUIRE(found_output);
        
        auto output = session->get_all_output();
        
        // Verify that we see the short_description, not the name_list
        REQUIRE(output.find("a mysterious glowing sculpture") != std::string::npos);
        REQUIRE(output.find("test sculpture artifact") == std::string::npos); // Should NOT see the raw name_list
        
        session->disconnect();
    });
}
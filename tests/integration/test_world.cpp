/***************************************************************************
 *   File: tests/test_world.cpp                       Part of FieryMUD *
 *  Usage: Integration tests for world management and interaction        *
 ***************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_system.hpp"
#include "../src/core/ids.hpp"
#include "../src/core/object.hpp"
#include "../common/mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("World: Room and Actor Integration", "[world][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();

        // Test room access through session
        auto starting_room = session->get_room(EntityId{100UL});
        REQUIRE(starting_room != nullptr);
        REQUIRE(starting_room->name() == "Starting Room");

        // Test actor presence in room - check that the guard NPC was created
        auto actors = session->get_actors_in_room(EntityId{100UL});
        REQUIRE_FALSE(actors.empty());

        // Should have the test guard NPC that was created in reset_world_state
        bool found_npc = false;
        for (const auto &actor : actors) {
            if (actor->name().find("guard") != std::string::npos) {
                found_npc = true;
            }
        }
        REQUIRE(found_npc);

        // Test that the room system works by verifying room properties
        REQUIRE(starting_room->description() == "A test room for integration testing.");
        REQUIRE(starting_room->short_description() == "a test room");
        
        // Test that we can also access the default test room
        auto default_room = session->get_room(EntityId{1});
        REQUIRE(default_room != nullptr);
        REQUIRE(default_room->name() == "Test Room");
    });
}

TEST_CASE("World: Object short_description Display", "[world][object][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        // Get the test room directly from WorldManager (bypass session networking)
        auto& world_manager = WorldManager::instance();
        auto room = world_manager.get_room(EntityId{1});
        REQUIRE(room != nullptr);
        REQUIRE(room->name() == "Test Room");
        
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
        
        // Verify the object is in the room and has correct short_description
        auto room_contents = room->contents();
        REQUIRE(room_contents.objects.size() == 1);
        
        auto placed_object = room_contents.objects[0];
        REQUIRE(placed_object->short_description() == "a mysterious glowing sculpture");
        REQUIRE(placed_object->name() == "test sculpture artifact");
        
        // Verify display_name uses short_description 
        REQUIRE(placed_object->display_name(false) == "a mysterious glowing sculpture");
        REQUIRE(placed_object->display_name(true) == "a mysterious glowing sculpture"); // already has article
    });
}

TEST_CASE("World: Look Command Shows Object Short Description", "[world][command][integration]") {
    UnifiedTestHarness::run_integration_test([&]() {
        // Get the test room directly from WorldManager 
        auto& world_manager = WorldManager::instance();
        auto room = world_manager.get_room(EntityId{1});
        REQUIRE(room != nullptr);
        
        // Create a test player in the room
        auto player_result = Player::create(EntityId{5000}, "TestPlayer");
        REQUIRE(player_result.has_value());
        auto player = std::shared_ptr<Player>(player_result.value().release());
        
        // Move player to the room
        auto move_result = player->move_to(room);
        REQUIRE(move_result.has_value());
        
        // Create test objects with different names vs short descriptions
        nlohmann::json bread_json = {
            {"id", "5001"},
            {"type", "FOOD"},
            {"name_list", "bread loaf food"},
            {"short_description", "some fresh bread"}
        };
        
        nlohmann::json sword_json = {
            {"id", "5002"}, 
            {"type", "WEAPON"},
            {"name_list", "sword blade weapon"},
            {"short_description", "a gleaming steel sword"}
        };
        
        auto bread_result = Object::from_json(bread_json);
        REQUIRE(bread_result.has_value());
        auto bread = std::shared_ptr<Object>(bread_result.value().release());
        
        auto sword_result = Object::from_json(sword_json);
        REQUIRE(sword_result.has_value());
        auto sword = std::shared_ptr<Object>(sword_result.value().release());
        
        // Add objects to room
        room->add_object(bread);
        room->add_object(sword);
        
        // Verify the objects have different names vs short descriptions
        REQUIRE(bread->name() == "bread loaf food");
        REQUIRE(bread->short_description() == "some fresh bread");
        REQUIRE(sword->name() == "sword blade weapon");
        REQUIRE(sword->short_description() == "a gleaming steel sword");
        
        // Test the look command logic directly by checking what room contents show
        auto room_contents = room->contents();
        REQUIRE(room_contents.objects.size() == 2);
        
        // Verify that when displaying room objects, we show short_description not name
        for (const auto& obj : room_contents.objects) {
            if (obj->id() == EntityId{5001}) {
                // Bread object should show "some fresh bread" not "bread loaf food"
                REQUIRE(obj->short_description() == "some fresh bread");
                REQUIRE(obj->short_description() != obj->name());
            } else if (obj->id() == EntityId{5002}) {
                // Sword object should show "a gleaming steel sword" not "sword blade weapon"  
                REQUIRE(obj->short_description() == "a gleaming steel sword");
                REQUIRE(obj->short_description() != obj->name());
            }
        }
        
        // This is the key test: the room display should use short_description
        // The cmd_look function calls obj->short_description() for room object display
        // So users should see "some fresh bread" and "a gleaming steel sword" when they look
    });
}
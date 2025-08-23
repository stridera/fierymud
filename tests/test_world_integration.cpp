/***************************************************************************
 *   File: tests/test_world_integration.cpp           Part of FieryMUD *
 *  Usage: Integration tests for world management components              *
 ***************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_system.hpp"
#include "../src/core/ids.hpp"
#include "mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>

// World management integration test
TEST_CASE("World Management - Room and Actor Integration", "[integration][world]") {
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

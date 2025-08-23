/***************************************************************************
 *   File: tests/test_multiplayer_interaction.cpp     Part of FieryMUD *
 *  Usage: Integration tests for multi-player interaction                 *
 ***************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_system.hpp"
#include "../src/core/ids.hpp"
#include "mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>

// Multi-session integration test
TEST_CASE("Multi-Session - Player Interaction", "[integration][multiplayer]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto sessions = UnifiedTestHarness::create_multiple_sessions(3);

        // Connect all sessions
        REQUIRE(sessions[0]->connect("Alpha").has_value());
        REQUIRE(sessions[1]->connect("Beta").has_value());
        REQUIRE(sessions[2]->connect("Gamma").has_value());

        // All should be in starting room
        for (const auto &session : sessions) {
            REQUIRE(session->current_room() == EntityId{100UL});
        }

        // Test visibility between players
        sessions[0]->clear_output();
        sessions[0]->send_input("look");

        auto alpha_output = sessions[0]->get_all_output();
        REQUIRE(alpha_output.find("Beta") != std::string::npos);
        REQUIRE(alpha_output.find("Gamma") != std::string::npos);

        // Move one player and test separation
        sessions[1]->send_input("north");
        REQUIRE(sessions[1]->current_room() == EntityId{101UL});

        sessions[0]->clear_output();
        sessions[0]->send_input("look");
        alpha_output = sessions[0]->get_all_output();

        // Alpha should still see Gamma but not Beta
        REQUIRE(alpha_output.find("Gamma") != std::string::npos);
        REQUIRE(alpha_output.find("Beta") == std::string::npos);

        // Disconnect all
        for (auto &session : sessions) {
            session->disconnect();
        }
    });
}

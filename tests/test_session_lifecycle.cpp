/***************************************************************************
 *   File: tests/test_session_lifecycle.cpp           Part of FieryMUD *
 *  Usage: Integration tests for session lifecycle and state consistency *
 ***************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_system.hpp"
#include "../src/core/ids.hpp"
#include "mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>

// State consistency integration test
TEST_CASE("State Consistency - Session Lifecycle", "[integration][session_lifecycle]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();

        // Test connection state progression
        REQUIRE(session->connection_state() == ConnectionState::Connected);
        REQUIRE_FALSE(session->is_connected());

        auto connect_result = session->connect("StateTester");
        REQUIRE(connect_result.has_value());
        REQUIRE(session->is_connected());
        REQUIRE(session->connection_state() == ConnectionState::Playing);

        // Test player state
        REQUIRE(session->player_name() == "StateTester");
        REQUIRE(session->current_room() == EntityId{100UL});

        // Test state persistence through commands
        session->send_input("north");
        REQUIRE(session->current_room() == EntityId{101UL});
        REQUIRE(session->player_name() == "StateTester");                 // Name should persist
        REQUIRE(session->connection_state() == ConnectionState::Playing); // State should persist

        // Test disconnection state
        session->disconnect();
        REQUIRE_FALSE(session->is_connected());
    });
}

/***************************************************************************
 *   File: tests/test_error_handling.cpp              Part of FieryMUD *
 *  Usage: Integration tests for error handling and system resilience   *
 ***************************************************************************/

#include "../src/commands/builtin_commands.hpp"
#include "../src/commands/command_system.hpp"
#include "../src/core/ids.hpp"
#include "mock_game_session.hpp"

#include <catch2/catch_test_macros.hpp>

// Error handling integration test
TEST_CASE("Error Handling - Command System Resilience", "[integration][error_handling]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();

        auto connect_result = session->connect("ErrorTester");
        REQUIRE(connect_result.has_value());

        session->clear_output();

        // Test invalid commands
        session->send_input("invalidcommand");
        auto output = session->get_all_output();
        REQUIRE((output.find("Unknown command") != std::string::npos || output.find("not found") != std::string::npos ||
                 output.find("don't understand") != std::string::npos));

        session->clear_output();

        // Test invalid movement
        session->send_input("southwest"); // Invalid direction
        output = session->get_all_output();
        // Should get some kind of error message
        REQUIRE_FALSE(output.empty());

        session->clear_output();

        // Verify system still works after errors
        session->send_input("look");
        output = session->get_all_output();
        REQUIRE(output.find("Starting Room") != std::string::npos);

        session->disconnect();
    });
}

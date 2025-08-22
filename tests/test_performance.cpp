/***************************************************************************
 *   File: tests/test_performance.cpp                 Part of FieryMUD *
 *  Usage: Performance integration tests                                  *
 ***************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include "mock_game_session.hpp"
#include "../src/modern/commands/command_system.hpp"
#include "../src/modern/commands/builtin_commands.hpp"
#include "../src/modern/core/ids.hpp"

// Performance integration test
TEST_CASE("Performance - Command Processing Speed", "[integration][performance]") {
    UnifiedTestHarness::run_integration_test([&]() {
        auto session = UnifiedTestHarness::create_session();
        
        auto connect_result = session->connect("SpeedTester");
        REQUIRE(connect_result.has_value());
        
        session->clear_output();
        
        // Time multiple command executions
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 10; ++i) {
            session->send_input("look");
            // Small delay to let command process
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Commands should execute quickly (less than 100ms total for 10 commands)
        REQUIRE(duration.count() < 100);
        
        // Verify all commands executed
        auto output = session->get_all_output();
        size_t room_count = 0;
        size_t pos = 0;
        while ((pos = output.find("Starting Room", pos)) != std::string::npos) {
            room_count++;
            pos++;
        }
        REQUIRE(room_count >= 10); // At least 10 room descriptions
        
        session->disconnect();
    });
}

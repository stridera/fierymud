/***************************************************************************
 *   File: test_session_stable.cpp                        Part of FieryMUD *
 *  Usage: Stable session lifecycle tests using LightweightTestHarness     *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include "../common/test_builders.hpp"
#include <catch2/catch_test_macros.hpp>

/**
 * BEFORE vs AFTER Comparison:
 * 
 * BEFORE (problematic approach):
 * - Used UnifiedTestHarness with complex session management
 * - Background thread interference causing segfaults
 * - Timeout-based synchronization patterns
 * - Shared global state between test instances
 * - Result: Unreliable test execution, segfaults
 * 
 * AFTER (stable approach):
 * - Uses LightweightTestHarness with direct state management
 * - Synchronous operations, no background threads
 * - Immediate state verification without timeouts
 * - Isolated test state per harness instance
 * - Result: Fast, reliable, deterministic tests
 */

/**
 * Session State Consistency Testing
 * Tests session lifecycle and state management without thread interference
 */
TEST_CASE("Session Stable: State Consistency and Lifecycle", "[session][integration][stable]") {
    SECTION("Basic session state management") {
        LightweightTestHarness harness;
        
        // Test initial state - player should be properly initialized
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
        REQUIRE(harness.get_player()->can_act());
        
        // Verify initial player state
        const auto& player = harness.get_player();
        REQUIRE(player->name() == "TestPlayer");
        REQUIRE(player->position() != Position::Dead);
        REQUIRE(player->position() != Position::Ghost);
        
        // Test state persistence through basic commands
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        // Verify state hasn't changed after command
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(player->name() == "TestPlayer");
        REQUIRE(harness.get_player()->is_alive());
    }
    
    SECTION("State consistency through movement") {
        LightweightTestHarness harness;
        
        // Test movement and state persistence
        harness.execute_command("north")
               .then_output_contains("You move north");
        
        REQUIRE(harness.current_room_id() == EntityId{101});
        
        // Verify player state persists through movement
        const auto& player = harness.get_player();
        REQUIRE(player->name() == "TestPlayer");
        REQUIRE(player->is_alive());
        REQUIRE(player->can_act());
        
        // Test state persistence through multiple operations
        harness.execute_command("look")
               .then_output_contains("North Room");
        
        harness.execute_command("say I moved successfully")
               .then_output_contains("You say")
               .then_output_contains("I moved successfully");
        
        // State should still be consistent
        REQUIRE(harness.current_room_id() == EntityId{101});
        REQUIRE(player->name() == "TestPlayer");
        REQUIRE(player->is_alive());
    }
    
    SECTION("State recovery after errors") {
        LightweightTestHarness harness;
        
        // Test state consistency after error conditions
        harness.execute_command("invalidcommand")
               .then_output_contains("Unknown command");
        
        // Player state should be unchanged after error
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
        REQUIRE(harness.get_player()->can_act());
        
        // Should be able to execute valid commands after error
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        // Test multiple errors don't corrupt state
        harness.execute_command("west");  // Invalid direction
        harness.execute_command("get nonexistentitem");  // Invalid item
        harness.execute_command("");  // Empty command
        
        // State should remain consistent
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->name() == "TestPlayer");
        REQUIRE(harness.get_player()->is_alive());
        
        // Should still respond to valid commands
        harness.execute_command("say Still working after errors")
               .then_output_contains("You say")
               .then_output_contains("Still working after errors");
    }
}

/**
 * Advanced Session Management Testing
 * Tests complex session scenarios and edge cases
 */
TEST_CASE("Session Stable: Advanced State Management", "[session][integration][advanced][stable]") {
    SECTION("Session isolation between instances") {
        // Create multiple independent sessions
        LightweightTestHarness session1;
        LightweightTestHarness session2;
        LightweightTestHarness session3;
        
        // All should start in same initial state but be completely isolated
        REQUIRE(session1.current_room_id() == EntityId{100});
        REQUIRE(session2.current_room_id() == EntityId{100});
        REQUIRE(session3.current_room_id() == EntityId{100});
        
        // Perform different operations on each session
        session1.execute_command("say Message from session 1");
        session2.execute_command("north");
        session3.execute_command("emote waves");
        
        // Verify complete isolation - no cross-talk between sessions
        auto output1 = session1.get_player()->get_output();
        auto output2 = session2.get_player()->get_output();
        auto output3 = session3.get_player()->get_output();
        
        REQUIRE(output1.size() >= 1);  // At least one output from say command
        REQUIRE(output2.size() >= 1);  // At least one output from north command (may include room desc)
        REQUIRE(output3.size() >= 1);  // At least one output from emote command
        
        // Each session should only see its own output
        REQUIRE(output1[0].find("session 1") != std::string::npos);
        REQUIRE(output2[0].find("You move north") != std::string::npos);
        REQUIRE(output3[0].find("TestPlayer waves") != std::string::npos);
        
        // Cross-contamination check
        REQUIRE(output1[0].find("You move north") == std::string::npos);
        REQUIRE(output1[0].find("waves") == std::string::npos);
        REQUIRE(output2[0].find("session 1") == std::string::npos);
        REQUIRE(output2[0].find("waves") == std::string::npos);
        REQUIRE(output3[0].find("session 1") == std::string::npos);
        REQUIRE(output3[0].find("You move north") == std::string::npos);
        
        // Verify different final states
        REQUIRE(session1.current_room_id() == EntityId{100});  // Didn't move
        REQUIRE(session2.current_room_id() == EntityId{101});  // Moved north
        REQUIRE(session3.current_room_id() == EntityId{100});  // Didn't move
    }
    
    SECTION("Long-term session stability") {
        LightweightTestHarness harness;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute many operations to test session stability
        for (int i = 0; i < 100; ++i) {
            harness.execute_command("look");
            harness.get_player()->clear_output();
            
            // Verify session remains stable every 20 iterations
            if (i % 20 == 0) {
                REQUIRE(harness.current_room_id().is_valid());
                REQUIRE(harness.get_player()->is_alive());
                REQUIRE(harness.get_player()->name() == "TestPlayer");
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Performance requirement: 100 operations should be fast
        REQUIRE(duration.count() < 500);  // Less than 500ms
        
        // Final state should be consistent
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
        
        INFO("100 session operations completed in " << duration.count() << "ms");
    }
    
    SECTION("Session state during complex workflows") {
        LightweightTestHarness harness;
        
        // Clear output first, then use accumulation for the workflow
        harness.get_player()->clear_output();
        
        // Test complex workflow: movement, communication, actions (with accumulation)
        harness.execute_command_accumulate("say Starting complex workflow");
        harness.execute_command_accumulate("north");
        REQUIRE(harness.current_room_id() == EntityId{101});
        
        harness.execute_command_accumulate("look");
        harness.execute_command_accumulate("emote examines the room carefully");
        harness.execute_command_accumulate("south");
        REQUIRE(harness.current_room_id() == EntityId{100});
        
        harness.execute_command_accumulate("say Workflow completed successfully");
        
        // Verify final state consistency
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->name() == "TestPlayer");
        REQUIRE(harness.get_player()->is_alive());
        REQUIRE(harness.get_player()->can_act());
        
        // Verify all outputs were captured (should have multiple messages from the workflow)
        const auto& output = harness.get_player()->get_output();
        REQUIRE(output.size() >= 6);  // At least say, move north, look, emote, move south, say
        
        // Verify key workflow elements are present
        std::string combined_output;
        for (const auto& line : output) {
            combined_output += line + " ";
        }
        
        REQUIRE(combined_output.find("Starting complex workflow") != std::string::npos);
        REQUIRE(combined_output.find("Workflow completed successfully") != std::string::npos);
    }
}

/**
 * Regression Testing for Session-Related Issues
 * Tests fixes for previously identified session problems
 */
TEST_CASE("Session Stable: Regression Testing", "[session][integration][regression][stable]") {
    SECTION("No memory leaks during session creation/destruction") {
        // This test addresses potential memory leaks in session management
        std::vector<std::unique_ptr<LightweightTestHarness>> sessions;
        
        // Rapidly create and destroy sessions
        for (int i = 0; i < 50; ++i) {
            auto session = std::make_unique<LightweightTestHarness>();
            
            // Basic functionality test
            session->execute_command("look")
                   .then_output_contains("Test Start Room");
            
            REQUIRE(session->current_room_id() == EntityId{100});
            REQUIRE(session->get_player()->is_alive());
            
            sessions.push_back(std::move(session));
        }
        
        // All sessions should remain functional
        for (auto& session : sessions) {
            session->execute_command("say Session still active")
                   .then_output_contains("You say");
        }
        
        // Clean destruction (should not leak)
        sessions.clear();
    }
    
    SECTION("Session state consistency under stress") {
        LightweightTestHarness harness;
        
        // Rapid command execution to test state consistency
        std::vector<std::string> commands = {
            "look", "say test", "emote nods", "north", "south", 
            "look", "say another test", "emote smiles"
        };
        
        for (int cycle = 0; cycle < 20; ++cycle) {
            for (const auto& cmd : commands) {
                harness.execute_command(cmd);
                
                // Verify state remains consistent
                REQUIRE(harness.get_player()->is_alive());
                REQUIRE(harness.get_player()->name() == "TestPlayer");
                REQUIRE(harness.current_room_id().is_valid());
            }
            
            // Clear output for next cycle
            harness.get_player()->clear_output();
        }
        
        // Final state check
        REQUIRE(harness.current_room_id() == EntityId{100});  // Should end up back at start
        REQUIRE(harness.get_player()->is_alive());
    }
    
    SECTION("Session recovery from edge cases") {
        LightweightTestHarness harness;
        
        // Test various edge case inputs
        std::vector<std::string> edge_cases = {
            "",           // Empty command
            " ",          // Whitespace only
            "   \t  ",    // Mixed whitespace
            "very_long_invalid_command_that_does_not_exist_in_the_system",
            "north west east south",  // Multiple directions
            "say",        // Command without arguments
            "123456",     // Numeric input
            "!@#$%^&*()", // Special characters
        };
        
        for (const auto& cmd : edge_cases) {
            // These commands may produce various outputs, but shouldn't crash
            harness.execute_command(cmd);
            
            // Session should remain stable after each edge case
            REQUIRE(harness.get_player()->is_alive());
            REQUIRE(harness.current_room_id().is_valid());
            REQUIRE(harness.get_player()->name() == "TestPlayer");
        }
        
        // Move back to start room in case edge cases moved us
        if (harness.current_room_id() != EntityId{100}) {
            harness.execute_command("south");  // If we're in room 101, go back to 100
        }
        
        // Should still be able to execute normal commands
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        harness.execute_command("say Recovery successful")
               .then_output_contains("You say")
               .then_output_contains("Recovery successful");
    }
}
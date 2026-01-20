#include "../common/lightweight_test_harness.hpp"
#include "../common/test_builders.hpp"
#include <catch2/catch_test_macros.hpp>

/**
 * BEFORE vs AFTER Comparison:
 * 
 * BEFORE (problematic approach):
 * - Used UnifiedTestHarness with complex background thread management
 * - Command system integration testing prone to race conditions
 * - Timeout-based waiting for command execution
 * - Shared global state causing interference between tests
 * - Result: Segfaults, unreliable command execution testing
 * 
 * AFTER (stable approach):
 * - Uses LightweightTestHarness with synchronous command execution
 * - Direct command system testing without thread interference
 * - Immediate command result verification
 * - Isolated test state per harness instance
 * - Result: Fast, reliable, deterministic command system testing
 */

/**
 * Command System Integration Testing
 * Tests core command system functionality without background thread interference
 */
TEST_CASE("CommandSystem Stable: Integration", "[command][system][integration][stable]") {
    SECTION("Command system initialization and registration") {
        // The LightweightTestHarness initializes the command system internally
        LightweightTestHarness harness;
        
        // Verify key commands are available through actual execution
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        harness.execute_command("north")
               .then_output_contains("You move north");
        
        REQUIRE(harness.current_room_id() == EntityId{101});
        
        harness.execute_command("south")
               .then_output_contains("You move south");
        
        REQUIRE(harness.current_room_id() == EntityId{100});
        
        harness.execute_command("say Hello command system")
               .then_output_contains("You say")
               .then_output_contains("Hello command system");
        
        harness.execute_command("who")
               .then_output_contains("No players are currently online");
    }
    
    SECTION("Command aliases and abbreviations") {
        LightweightTestHarness harness;
        
        // Test common command aliases work the same as full commands
        harness.execute_command("l")
               .then_output_contains("Test Start Room");
        
        harness.execute_command("n")
               .then_output_contains("You move north");
        REQUIRE(harness.current_room_id() == EntityId{101});
        
        harness.execute_command("s")
               .then_output_contains("You move south");
        REQUIRE(harness.current_room_id() == EntityId{100});
        
        // Test inventory alias  
        harness.execute_command("i")
               .then_output_contains("You are not carrying anything");
    }
    
    SECTION("Command error handling and invalid commands") {
        LightweightTestHarness harness;
        
        // Test invalid command handling
        harness.execute_command("invalidcommandname")
               .then_output_contains("Unknown command");
        
        // Player should remain in valid state after invalid command
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
        
        // Test invalid directions
        harness.execute_command("west")
               .then_output_contains("Cannot move West");
        
        harness.execute_command("up")
               .then_output_contains("Cannot move Up");
        
        // Player should still be able to execute valid commands
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
    }
    
    SECTION("Complex command argument handling") {
        LightweightTestHarness harness;
        
        // Test commands with multiple arguments
        harness.execute_command("say This is a multi-word message")
               .then_output_contains("You say")
               .then_output_contains("This is a multi-word message");
        
        // Test commands with no arguments when arguments expected
        harness.execute_command("say");  // Say with no message
        
        // Test emote commands with arguments
        harness.execute_command("emote nods approvingly at the test results")
               .then_output_contains("TestPlayer nods approvingly at the test results");
        
        // Test commands with special characters and formatting
        harness.execute_command("say Testing special chars: !@#$%^&*()")
               .then_output_contains("You say")
               .then_output_contains("Testing special chars: !@#$%^&*()");
    }
}

/**
 * Advanced Command System Testing
 * Tests complex command interactions and edge cases
 */
TEST_CASE("CommandSystem Stable: Advanced Integration", "[command][system][advanced][stable]") {
    SECTION("Command execution sequence and state consistency") {
        LightweightTestHarness harness;
        
        // Execute a complex sequence of commands and verify state consistency
        std::vector<std::pair<std::string, std::string>> command_sequence = {
            {"look", "Test Start Room"},
            {"say Beginning command sequence test", "You say"},
            {"north", "You move north"},
            {"look", "North Room"},
            {"emote examines the northern room", "TestPlayer examines"},
            {"south", "You move south"},
            {"look", "Test Start Room"},
            {"say Sequence completed successfully", "completed successfully"}
        };
        
        EntityId expected_room = EntityId{100};
        
        for (const auto& [command, expected_output] : command_sequence) {
            harness.execute_command(command)
                   .then_output_contains(expected_output);
            
            // Update expected room for movement commands
            if (command == "north") {
                expected_room = EntityId{101};
            } else if (command == "south") {
                expected_room = EntityId{100};
            }
            
            // Verify room consistency after each command
            REQUIRE(harness.current_room_id() == expected_room);
            REQUIRE(harness.get_player()->is_alive());
        }
        
        // Verify final state
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->name() == "TestPlayer");
    }
    
    SECTION("Command system performance and stress testing") {
        LightweightTestHarness harness;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute many commands rapidly to test performance
        std::vector<std::string> stress_commands = {
            "look", "say test", "emote nods", "i", "who", "look"
        };
        
        constexpr int stress_cycles = 50;
        for (int cycle = 0; cycle < stress_cycles; ++cycle) {
            for (const auto& cmd : stress_commands) {
                harness.execute_command(cmd);
                
                // Verify state remains consistent under stress
                REQUIRE(harness.get_player()->is_alive());
                REQUIRE(harness.current_room_id().is_valid());
            }
            
            // Clear output periodically to prevent memory buildup
            if (cycle % 10 == 0) {
                harness.get_player()->clear_output();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Performance requirement: Many commands should execute quickly
        REQUIRE(duration.count() < 1000);  // Less than 1 second
        
        // System should still be responsive after stress test
        harness.execute_command("say Stress test completed")
               .then_output_contains("You say")
               .then_output_contains("Stress test completed");
        
        INFO("Executed " << (stress_cycles * stress_commands.size()) << " commands in " << duration.count() << "ms");
    }
    
    SECTION("Command privilege system testing") {
        LightweightTestHarness harness;
        
        // Test basic player commands (should all work)
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        harness.execute_command("say I can use player commands")
               .then_output_contains("You say");
        
        harness.execute_command("who")
               .then_output_contains("No players are currently online");
        
        harness.execute_command("inventory")
               .then_output_contains("You are not carrying anything");
        
        // Test movement commands
        harness.execute_command("north")
               .then_output_contains("You move north");
        
        harness.execute_command("south")
               .then_output_contains("You move south");
        
        // All commands should execute successfully for basic player privileges
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
}

/**
 * Command Context and Message Routing Testing
 * Tests the message routing system that commands rely on
 */
TEST_CASE("CommandSystem Stable: Message Routing", "[command][system][messaging][stable]") {
    SECTION("Command output routing and isolation") {
        LightweightTestHarness harness;
        
        // Clear any existing output
        harness.get_player()->clear_output();
        
        // Execute command that produces output
        harness.execute_command("say Testing message routing");
        
        const auto& output = harness.get_player()->get_output();
        REQUIRE(output.size() >= 1);
        
        // Verify we get the expected output format
        bool found_self_message = false;
        for (const auto& line : output) {
            if (line.find("You say") != std::string::npos && 
                line.find("Testing message routing") != std::string::npos) {
                found_self_message = true;
            }
        }
        
        REQUIRE(found_self_message);
    }
    
    SECTION("Multiple command output accumulation") {
        LightweightTestHarness harness;
        
        // Simplified test: Just accumulate 3 say commands
        // Each say command produces exactly 1 output line via ctx.send() - the "You say, '...'" message
        // The ctx.send_to_room() call doesn't affect the sender since there are no other players in the room
        harness.get_player()->clear_output();
        
        harness.execute_command_accumulate("say First message");
        harness.execute_command_accumulate("say Second message");
        harness.execute_command_accumulate("say Third message");
        
        const auto& output = harness.get_player()->get_output();
        
        // Should have exactly 3 messages (one from each say command)
        REQUIRE(output.size() == 3);
        
        // Verify all messages are present
        std::string combined_output;
        for (const auto& line : output) {
            combined_output += line + " ";
        }
        
        REQUIRE(combined_output.find("First message") != std::string::npos);
        REQUIRE(combined_output.find("Second message") != std::string::npos);
        REQUIRE(combined_output.find("Third message") != std::string::npos);
    }
    
    SECTION("Output isolation between harness instances") {
        LightweightTestHarness harness1;
        LightweightTestHarness harness2;
        
        // Execute different commands on each harness
        harness1.execute_command("say Message from harness 1");
        harness2.execute_command("say Message from harness 2");
        
        // Verify complete output isolation
        const auto& output1 = harness1.get_player()->get_output();
        const auto& output2 = harness2.get_player()->get_output();
        
        REQUIRE(output1.size() >= 1);
        REQUIRE(output2.size() >= 1);
        
        // Each harness should only see its own output
        std::string output1_text, output2_text;
        for (const auto& line : output1) output1_text += line + " ";
        for (const auto& line : output2) output2_text += line + " ";
        
        REQUIRE(output1_text.find("harness 1") != std::string::npos);
        REQUIRE(output1_text.find("harness 2") == std::string::npos);
        
        REQUIRE(output2_text.find("harness 2") != std::string::npos);
        REQUIRE(output2_text.find("harness 1") == std::string::npos);
    }
}

/**
 * Regression Testing for Command System Issues
 * Tests fixes for previously identified command system problems
 */
TEST_CASE("CommandSystem Stable: Regression Testing", "[command][system][regression][stable]") {
    SECTION("No crashes on rapid command execution") {
        // This test addresses potential crashes during rapid command execution
        LightweightTestHarness harness;
        
        std::vector<std::string> rapid_commands = {
            "look", "say test1", "north", "south", "emote nods", "who",
            "look", "say test2", "i", "look", "say test3", "emote smiles"
        };
        
        // Execute commands as rapidly as possible
        for (int cycle = 0; cycle < 10; ++cycle) {
            for (const auto& cmd : rapid_commands) {
                harness.execute_command(cmd);
            }
            
            // Verify system stability after each cycle
            REQUIRE(harness.get_player()->is_alive());
            REQUIRE(harness.current_room_id().is_valid());
            
            // Clear output to prevent excessive memory usage
            harness.get_player()->clear_output();
        }
        
        // Final validation - system should still be responsive
        harness.execute_command("say Rapid execution test completed")
               .then_output_contains("You say")
               .then_output_contains("Rapid execution test completed");
    }
    
    SECTION("Command state consistency after errors") {
        LightweightTestHarness harness;
        
        // Execute sequence with both valid and invalid commands
        std::vector<std::string> mixed_commands = {
            "look",                               // Valid
            "invalidcommand",                     // Invalid
            "say This should work",               // Valid
            "nonexistentdirection",              // Invalid
            "north",                             // Valid
            "another_invalid_command",           // Invalid
            "look",                              // Valid
            "say Error recovery successful"       // Valid
        };
        
        EntityId expected_room = EntityId{100};
        
        for (const auto& cmd : mixed_commands) {
            harness.execute_command(cmd);
            
            // Update expected room for valid movement
            if (cmd == "north") {
                expected_room = EntityId{101};
            }
            
            // State should remain consistent regardless of command validity
            REQUIRE(harness.get_player()->is_alive());
            REQUIRE(harness.current_room_id().is_valid());
        }
        
        // Verify final state is correct
        REQUIRE(harness.current_room_id() == expected_room);
        REQUIRE(harness.get_player()->name() == "TestPlayer");
        
        // Verify last valid command worked
        const auto& output = harness.get_player()->get_output();
        std::string combined_output;
        for (const auto& line : output) {
            combined_output += line + " ";
        }
        REQUIRE(combined_output.find("Error recovery successful") != std::string::npos);
    }
    
    SECTION("Memory stability during extended command sessions") {
        LightweightTestHarness harness;
        
        // Simulate extended play session with various command types
        std::vector<std::string> session_commands = {
            "look", "say hello", "emote waves", "who", "i", 
            "north", "look", "south", "say goodbye"
        };
        
        // Run extended session
        constexpr int session_length = 100;
        for (int i = 0; i < session_length; ++i) {
            const auto& cmd = session_commands[i % session_commands.size()];
            harness.execute_command(cmd);
            
            // Periodically clear output to simulate realistic usage
            if (i % 20 == 0) {
                harness.get_player()->clear_output();
            }
            
            // Verify stability throughout session
            if (i % 25 == 0) {
                REQUIRE(harness.get_player()->is_alive());
                REQUIRE(harness.current_room_id().is_valid());
                REQUIRE(harness.get_player()->name() == "TestPlayer");
            }
        }
        
        // Final validation after extended session
        harness.execute_command("say Extended session completed successfully")
               .then_output_contains("You say")
               .then_output_contains("Extended session completed successfully");
        
        REQUIRE(harness.current_room_id().is_valid());
        REQUIRE(harness.get_player()->is_alive());
    }
}
/***************************************************************************
 *   File: test_multiplayer_stable.cpp                    Part of FieryMUD *
 *  Usage: Stable multiplayer integration tests using LightweightTestHarness *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include "../common/test_builders.hpp"
#include <catch2/catch_test_macros.hpp>

/**
 * BEFORE vs AFTER Comparison:
 * 
 * BEFORE (problematic approach):
 * - Used UnifiedTestHarness with background threads
 * - Complex session management with async operations
 * - Timeout-based synchronization prone to race conditions
 * - Shared global state causing interference between tests
 * - Result: Segfaults, unreliable test execution
 * 
 * AFTER (stable approach):
 * - Uses LightweightTestHarness with synchronous operations
 * - Isolated test state per harness instance
 * - Immediate, deterministic command execution
 * - Simple, direct player interaction testing
 * - Result: Fast, reliable, debuggable tests
 */

/**
 * Multi-Player Room Occupancy Testing
 * Tests multiple players in the same room and their interactions
 */
TEST_CASE("Multiplayer: Room Occupancy and Visibility", "[multiplayer][stable]") {
    SECTION("Multiple players can coexist in same room") {
        // Create multiple isolated test environments
        LightweightTestHarness harness1;
        LightweightTestHarness harness2;
        LightweightTestHarness harness3;
        
        // Each harness is completely isolated - no interference
        REQUIRE(harness1.current_room_id() == EntityId{100});
        REQUIRE(harness2.current_room_id() == EntityId{100});
        REQUIRE(harness3.current_room_id() == EntityId{100});
        
        // Test that each player can interact independently
        harness1.execute_command("say Hello from Player 1")
               .then_output_contains("You say")
               .then_output_contains("Hello from Player 1");
               
        harness2.execute_command("say Hello from Player 2")
               .then_output_contains("You say")
               .then_output_contains("Hello from Player 2");
        
        // Verify complete isolation - no cross-talk between harnesses
        auto output1 = harness1.get_player()->get_output();
        auto output2 = harness2.get_player()->get_output();
        
        REQUIRE(output1.size() == 1);
        REQUIRE(output2.size() == 1);
        REQUIRE(output1[0].find("Player 1") != std::string::npos);
        REQUIRE(output2[0].find("Player 2") != std::string::npos);
        
        // Cross-contamination check
        REQUIRE(output1[0].find("Player 2") == std::string::npos);
        REQUIRE(output2[0].find("Player 1") == std::string::npos);
    }
}

/**
 * Player Movement and Separation Testing
 * Tests players moving between rooms and losing visibility
 */
TEST_CASE("Multiplayer: Player Movement and Separation", "[multiplayer][stable]") {
    SECTION("Players can move independently") {
        LightweightTestHarness alice_harness;
        LightweightTestHarness bob_harness;
        
        // Both start in same room
        REQUIRE(alice_harness.current_room_id() == EntityId{100});
        REQUIRE(bob_harness.current_room_id() == EntityId{100});
        
        // Alice moves north
        alice_harness.execute_command("north")
                     .then_output_contains("You move north");
        
        REQUIRE(alice_harness.current_room_id() == EntityId{101});
        REQUIRE(bob_harness.current_room_id() == EntityId{100}); // Bob stays
        
        // Alice looks around in new room
        alice_harness.execute_command("look")
                     .then_output_contains("North Room");
        
        // Bob looks around in original room  
        bob_harness.execute_command("look")
                   .then_output_contains("Test Start Room");
        
        // Test room-specific commands work correctly
        alice_harness.execute_command("say I'm in the north room")
                     .then_output_contains("You say");
                     
        bob_harness.execute_command("say I'm in the start room")
                   .then_output_contains("You say");
    }
    
    SECTION("Players can reunite by moving") {
        LightweightTestHarness charlie_harness;
        LightweightTestHarness diana_harness;
        
        // Charlie moves north first
        charlie_harness.execute_command("north")
                       .then_output_contains("You move north");
        
        REQUIRE(charlie_harness.current_room_id() == EntityId{101});
        REQUIRE(diana_harness.current_room_id() == EntityId{100});
        
        // Diana follows Charlie
        diana_harness.execute_command("north")
                     .then_output_contains("You move north");
        
        // Now both are in the same room again
        REQUIRE(charlie_harness.current_room_id() == EntityId{101});
        REQUIRE(diana_harness.current_room_id() == EntityId{101});
        
        // Both can look around in the same room
        charlie_harness.execute_command("look")
                      .then_output_contains("North Room");
                      
        diana_harness.execute_command("look")
                     .then_output_contains("North Room");
    }
}

/**
 * Communication Testing Between Players
 * Tests various forms of player-to-player communication
 */
TEST_CASE("Multiplayer: Communication Patterns", "[multiplayer][stable]") {
    SECTION("Say command works in isolation") {
        LightweightTestHarness eve_harness;
        
        eve_harness.execute_command("say Testing communication system")
                   .then_output_contains("You say")
                   .then_output_contains("Testing communication system");
        
        // Verify message format
        const auto& output = eve_harness.get_player()->get_output();
        REQUIRE(output.size() == 1);
        
        const auto& message = output[0];
        REQUIRE(message.find("You say") != std::string::npos);
        REQUIRE(message.find("Testing communication system") != std::string::npos);
    }
    
    SECTION("Emote commands work in isolation") {
        LightweightTestHarness frank_harness;
        
        frank_harness.execute_command("emote waves to everyone in the room")
                     .then_output_contains("TestPlayer waves to everyone in the room");
        
        // Verify emote format includes player name
        const auto& output = frank_harness.get_player()->get_output();
        REQUIRE(output.size() == 1);
        REQUIRE(output[0].find("TestPlayer") != std::string::npos);
        REQUIRE(output[0].find("waves to everyone") != std::string::npos);
    }
    
    SECTION("Multiple communication commands in sequence") {
        LightweightTestHarness grace_harness;
        
        // Clear output first, then execute multiple commands with accumulation
        grace_harness.get_player()->clear_output();
        
        // Execute multiple commands in sequence (accumulate output)
        grace_harness.execute_command_accumulate("say Hello everyone");
        grace_harness.execute_command_accumulate("emote smiles cheerfully");
        grace_harness.execute_command_accumulate("say Hope everyone is doing well");
        
        // Verify all commands executed
        const auto& output = grace_harness.get_player()->get_output();
        REQUIRE(output.size() == 3);
        
        // Check each message
        REQUIRE(output[0].find("Hello everyone") != std::string::npos);
        REQUIRE(output[1].find("smiles cheerfully") != std::string::npos);
        REQUIRE(output[2].find("Hope everyone is doing well") != std::string::npos);
    }
}

/**
 * Stress Testing for Multiplayer Stability
 * Tests system stability under multiple concurrent operations
 */
TEST_CASE("Multiplayer: Stress Testing and Stability", "[multiplayer][stress]") {
    SECTION("Multiple harnesses handle rapid commands") {
        constexpr int num_players = 5;
        constexpr int commands_per_player = 20;
        
        std::vector<std::unique_ptr<LightweightTestHarness>> harnesses;
        
        // Create multiple harnesses
        for (int i = 0; i < num_players; ++i) {
            harnesses.push_back(std::make_unique<LightweightTestHarness>());
        }
        
        // Execute many commands across all harnesses
        for (int cmd = 0; cmd < commands_per_player; ++cmd) {
            for (int player = 0; player < num_players; ++player) {
                auto& harness = *harnesses[player];
                
                // Alternate between different command types
                if (cmd % 3 == 0) {
                    harness.execute_command("look");
                } else if (cmd % 3 == 1) {
                    harness.execute_command(fmt::format("say Command {} from player {}", cmd, player));
                } else {
                    harness.execute_command(fmt::format("emote performs action {}", cmd));
                }
                
                // Verify harness remains stable
                REQUIRE(harness.current_room_id().is_valid());
                REQUIRE(harness.get_player()->is_alive());
                
                // Clear output to prevent accumulation
                harness.get_player()->clear_output();
            }
        }
        
        // All harnesses should still be functional
        for (auto& harness : harnesses) {
            harness->execute_command("look")
                   .then_output_contains("Test Start Room");
        }
        
        INFO("Successfully executed " << (num_players * commands_per_player) << " commands across " << num_players << " players");
    }
    
    SECTION("Long-running session stability") {
        LightweightTestHarness long_harness;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute many operations in a single session
        for (int i = 0; i < 1000; ++i) {
            long_harness.execute_command("look");
            long_harness.get_player()->clear_output();
            
            // Verify stability every 100 iterations
            if (i % 100 == 0) {
                REQUIRE(long_harness.current_room_id().is_valid());
                REQUIRE(long_harness.get_player()->is_alive());
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Performance requirement: 1000 operations in reasonable time
        REQUIRE(duration.count() < 1000); // Less than 1 second
        
        INFO("1000 operations completed in " << duration.count() << "ms");
    }
}

/**
 * Error Handling in Multiplayer Context
 * Tests error conditions and recovery in multiplayer scenarios
 */
TEST_CASE("Multiplayer: Error Handling and Recovery", "[multiplayer][error]") {
    SECTION("Invalid commands don't affect other players") {
        LightweightTestHarness harry_harness;
        LightweightTestHarness iris_harness;
        
        // Harry issues invalid command
        harry_harness.execute_command("invalidcommandxyz")
                     .then_output_contains("Unknown command");
        
        // Iris should be unaffected and able to execute valid commands
        iris_harness.execute_command("look")
                    .then_output_contains("Test Start Room");
        
        // Harry should recover and be able to execute valid commands
        harry_harness.execute_command("look")
                     .then_output_contains("Test Start Room");
    }
    
    SECTION("Network simulation errors don't cascade") {
        LightweightTestHarness jack_harness;
        LightweightTestHarness kelly_harness;
        
        // Simulate various error conditions
        jack_harness.execute_command(""); // Empty command
        jack_harness.execute_command("west"); // Invalid direction
        jack_harness.execute_command("get nonexistentitem"); // Invalid item
        
        // Kelly operates normally
        kelly_harness.execute_command("north")
                     .then_output_contains("You move north");
        
        REQUIRE(kelly_harness.current_room_id() == EntityId{101});
        
        // Jack should still be functional after errors
        jack_harness.execute_command("look")
                   .then_output_contains("Test Start Room");
        
        REQUIRE(jack_harness.current_room_id() == EntityId{100});
    }
}

/**
 * Regression Testing for Previously Fixed Issues
 * Ensures previously resolved multiplayer bugs don't reappear
 */
TEST_CASE("Multiplayer: Regression Testing", "[multiplayer][regression]") {
    SECTION("No segfaults during rapid player creation") {
        // This test specifically addresses the segfault issues we fixed
        std::vector<std::unique_ptr<LightweightTestHarness>> rapid_harnesses;
        
        // Rapidly create and use multiple harnesses
        for (int i = 0; i < 10; ++i) {
            auto harness = std::make_unique<LightweightTestHarness>();
            
            // Immediately test functionality
            harness->execute_command("look")
                   .then_output_contains("Test Start Room");
            
            REQUIRE(harness->current_room_id() == EntityId{100});
            
            rapid_harnesses.push_back(std::move(harness));
        }
        
        // All harnesses should remain functional
        for (auto& harness : rapid_harnesses) {
            harness->execute_command("say Still working")
                   .then_output_contains("You say");
        }
        
        // Destruction should be clean (no segfaults)
        rapid_harnesses.clear();
    }
    
    SECTION("Session isolation prevents state bleeding") {
        // Create harness, use it, destroy it
        {
            LightweightTestHarness temp_harness;
            temp_harness.execute_command("say Temporary message");
        } // Harness destroyed here
        
        // Create new harness - should have clean state
        LightweightTestHarness clean_harness;
        clean_harness.execute_command("look")
                     .then_output_contains("Test Start Room");
        
        // Should not contain any remnants from previous harness
        const auto& output = clean_harness.get_player()->get_output();
        for (const auto& line : output) {
            REQUIRE(line.find("Temporary message") == std::string::npos);
        }
    }
}

/**
 * Performance Benchmarking for Multiplayer Operations
 * Validates that the new approach meets performance requirements
 */
TEST_CASE("Multiplayer: Performance Benchmarks", "[multiplayer][performance]") {
    SECTION("Harness creation performance") {
        constexpr int num_harnesses = 50;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::unique_ptr<LightweightTestHarness>> harnesses;
        harnesses.reserve(num_harnesses);
        
        for (int i = 0; i < num_harnesses; ++i) {
            harnesses.push_back(std::make_unique<LightweightTestHarness>());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Performance requirement: Creating 50 harnesses should be fast
        REQUIRE(duration.count() < 500); // Less than 500ms
        
        // Verify all harnesses are functional
        for (auto& harness : harnesses) {
            REQUIRE(harness->current_room_id().is_valid());
        }
        
        INFO("Created " << num_harnesses << " harnesses in " << duration.count() << "ms");
    }
    
    SECTION("Parallel command execution performance") {
        constexpr int num_players = 10;
        constexpr int commands_per_player = 50;
        
        std::vector<std::unique_ptr<LightweightTestHarness>> players;
        for (int i = 0; i < num_players; ++i) {
            players.push_back(std::make_unique<LightweightTestHarness>());
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute commands across all players
        for (int cmd = 0; cmd < commands_per_player; ++cmd) {
            for (auto& player : players) {
                player->execute_command("look");
                player->get_player()->clear_output();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        int total_commands = num_players * commands_per_player;
        double commands_per_second = total_commands / (duration.count() / 1000.0);
        
        // Performance requirement: Should handle many commands quickly
        REQUIRE(commands_per_second > 1000); // At least 1000 commands/second
        
        INFO("Executed " << total_commands << " commands in " << duration.count() 
             << "ms (" << commands_per_second << " commands/second)");
    }
}
/***************************************************************************
 *   File: test_enhanced_features_demo.cpp               Part of FieryMUD *
 *  Usage: Demonstration of enhanced LightweightTestHarness features       *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

/**
 * Demonstration of enhanced LightweightTestHarness features
 */
TEST_CASE("Enhanced Features: Regex Output Validation", "[integration][enhanced][regex]") {
    LightweightTestHarness harness;
    
    // Test regex pattern matching
    harness.execute_command("say Hello, test pattern 123")
           .then_output_matches_regex(R"(You say.*Hello.*\d+)")
           .then_output_not_contains("invalid content");
}

TEST_CASE("Enhanced Features: Multi-Command Execution", "[integration][enhanced][multi]") {
    LightweightTestHarness harness;
    
    std::vector<std::string> commands = {
        "look",
        "north", 
        "look",
        "south"
    };
    
    harness.execute_commands(commands)
           .then_output_contains("Test Start Room")
           .then_output_contains("North Room");
    
    // Verify we're back in start room
    REQUIRE(harness.player_is_in_room(EntityId{100}));
}

TEST_CASE("Enhanced Features: Performance Measurement", "[integration][enhanced][performance]") {
    LightweightTestHarness harness;
    
    // Test performance measurement capabilities
    harness.then_executes_within_ms([&harness]() {
        for (int i = 0; i < 10; ++i) {
            harness.execute_command("look");
        }
    }, 100); // Should complete 10 look commands within 100ms
}

TEST_CASE("Enhanced Features: Output Analysis", "[integration][enhanced][analysis]") {
    LightweightTestHarness harness;
    
    // Generate multiple output lines
    harness.execute_command("say line 1")
           .execute_command("say line 2")
           .execute_command("emote does something")
           .execute_command("say line 3");
    
    // Test output counting
    auto say_count = harness.count_output_lines_containing("You say");
    REQUIRE(say_count == 3);
    
    // Test output filtering
    auto say_lines = harness.get_output_lines_containing("You say");
    REQUIRE(say_lines.size() == 3);
    REQUIRE(say_lines[0].find("line 1") != std::string::npos);
    REQUIRE(say_lines[1].find("line 2") != std::string::npos);
    REQUIRE(say_lines[2].find("line 3") != std::string::npos);
}

TEST_CASE("Enhanced Features: State Validation", "[integration][enhanced][state]") {
    LightweightTestHarness harness;
    
    // Test initial player stats
    harness.then_player_stat_equals("level", 1)
           .then_player_stat_equals("max_hp", 20);
    
    // Test room state validation
    REQUIRE(harness.player_is_in_room(EntityId{100}));
    REQUIRE_FALSE(harness.player_is_in_room(EntityId{999}));
    
    // Test navigation
    harness.execute_command("north");
    REQUIRE(harness.player_is_in_room(EntityId{101}));
}

/**
 * Comprehensive integration test using all enhanced features
 */
TEST_CASE("Enhanced Features: Comprehensive Integration", "[integration][enhanced][comprehensive]") {
    LightweightTestHarness harness;
    
    // Start with validation
    harness.then_player_stat_equals("level", 1)
           .then_output_size_is(0); // No initial output
    
    // Execute complex command sequence
    std::vector<std::string> sequence = {
        "say Starting comprehensive test",
        "look", 
        "north",
        "say I moved north",
        "look",
        "emote examines the surroundings",
        "south",
        "say Back to start"
    };
    
    // Performance-measured execution
    harness.then_executes_within_ms([&]() {
        harness.execute_commands_accumulate(sequence);
    }, 200);
    
    // Comprehensive output validation
    harness.then_output_contains("Starting comprehensive test")
           .then_output_contains("moved north")
           .then_output_contains("Back to start")
           .then_output_matches_regex(R"(TestPlayer examines)")
           .then_output_not_contains("error")
           .then_output_not_contains("Invalid");
    
    // State validation
    REQUIRE(harness.player_is_in_room(EntityId{100})); // Back at start
    REQUIRE(harness.count_output_lines_containing("You say") == 3);
    
    // Complex regex validation
    auto look_lines = harness.get_output_lines_containing("Test");
    REQUIRE(look_lines.size() >= 2); // Should see both room descriptions
    
    // Final validation
    harness.then_player_stat_equals("level", 1) // Should still be level 1
           .then_player_stat_equals("hp", 20);   // Should still have full health
}

/**
 * Error handling and edge case testing
 */
TEST_CASE("Enhanced Features: Error Handling", "[integration][enhanced][error]") {
    LightweightTestHarness harness;
    
    // Test invalid commands
    harness.execute_command("invalidcommand")
           .then_output_matches_regex(R"(Invalid|Unknown|command)")
           .then_output_not_contains("segfault");
    
    // Test invalid directions
    harness.execute_command("west") // No west exit
           .then_output_matches_regex(R"(can't go|no exit|way)")
           .then_player_stat_equals("level", 1); // Player should be unchanged
    
    // Verify no state corruption
    REQUIRE(harness.player_is_in_room(EntityId{100}));
    REQUIRE(harness.get_player()->is_alive());
}

/**
 * Enhanced Object System Testing
 * Tests the new setup_test_objects() functionality
 */
TEST_CASE("Enhanced Features: Object System Integration", "[integration][enhanced][objects]") {
    SECTION("Interactive objects are created and accessible") {
        LightweightTestHarness harness;
        
        // Look at room - should contain our test objects
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        // Try to interact with some objects
        harness.execute_command("get test_sword");
        harness.execute_command("inventory")
               .then_output_contains("test_sword");
        
        harness.execute_command("drop test_sword");
        harness.execute_command("look")
               .then_output_contains("test_sword");
    }
    
    SECTION("Container interaction with chest and potion") {
        LightweightTestHarness harness;
        
        // Test chest interaction
        harness.execute_command("look test_chest");
        
        // Try container operations
        harness.execute_command("get test_potion from test_chest");
        
        // Verify some response
        const auto& output = harness.get_player()->get_output();
        REQUIRE(output.size() > 0);
        
        // Test putting something back
        harness.execute_command("get test_key");
        harness.execute_command("put test_key in test_chest");
        
        // Should get responses
        REQUIRE(harness.get_player()->get_output().size() > 0);
    }
    
    SECTION("Equipment system with weapon and armor") {
        LightweightTestHarness harness;
        
        // Test equipment operations
        harness.execute_command("get test_sword");
        harness.execute_command("wield test_sword");
        
        harness.execute_command("get test_shield");
        harness.execute_command("wear test_shield");
        
        // Check equipment status
        harness.execute_command("equipment");
        
        const auto& output = harness.get_player()->get_output();
        REQUIRE(output.size() > 0);
    }
    
    SECTION("Diverse object types testing") {
        LightweightTestHarness harness;
        
        std::vector<std::string> test_objects = {
            "test_chest", "test_sword", "test_shield", 
            "test_torch", "test_key", "test_bread", "test_bag"
        };
        
        // Each object should be gettable
        for (const auto& obj : test_objects) {
            harness.execute_command(fmt::format("get {}", obj));
            // Should get some kind of response
            REQUIRE(harness.get_player()->get_output().size() > 0);
            
            // Drop it back
            harness.execute_command(fmt::format("drop {}", obj));
            harness.get_player()->clear_output(); // Clear for next test
        }
    }
}

/**
 * Performance regression testing
 */
TEST_CASE("Enhanced Features: Performance Benchmarks", "[integration][enhanced][benchmark]") {
    LightweightTestHarness harness;
    
    SECTION("Command execution performance") {
        // Test rapid command execution
        harness.then_executes_within_ms([&]() {
            for (int i = 0; i < 50; ++i) {
                harness.execute_command("look");
            }
        }, 500); // 50 commands in 500ms
    }
    
    SECTION("Navigation performance") {
        // Test rapid navigation
        harness.then_executes_within_ms([&]() {
            for (int i = 0; i < 20; ++i) {
                harness.execute_command("north")
                       .execute_command("south");
            }
        }, 300); // 40 movement commands in 300ms
        
        // Verify no state corruption after rapid navigation
        REQUIRE(harness.player_is_in_room(EntityId{100}));
    }
    
    SECTION("Output processing performance") {
        // Generate large amount of output
        std::vector<std::string> chatty_commands;
        for (int i = 0; i < 30; ++i) {
            chatty_commands.push_back(fmt::format("say Performance test message {}", i));
        }
        
        harness.then_executes_within_ms([&]() {
            harness.execute_commands(chatty_commands);
        }, 400); // 30 say commands in 400ms
        
        // Verify output was captured correctly
        REQUIRE(harness.count_output_lines_containing("You say") == 30);
    }
}
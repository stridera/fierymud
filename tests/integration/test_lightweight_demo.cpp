/***************************************************************************
 *   File: test_lightweight_demo.cpp                      Part of FieryMUD *
 *  Usage: Demonstration of improved integration testing without segfaults  *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include "../common/test_builders.hpp"
#include "../../src/core/combat.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace FieryMUD;

/**
 * Demonstration: How to replace problematic TestHarness with stable approach
 * This shows the same functionality without background threads or segfaults
 */
TEST_CASE("Lightweight Integration: Basic Command Flow", "[integration][stable]") {
    // Old approach (causes segfaults):
    // TestHarness harness; // Background threads, shared state, timeouts
    
    // New approach (stable):
    LightweightTestHarness harness; // Synchronous, isolated, deterministic
    
    SECTION("Movement commands work reliably") {
        // Test initial state
        REQUIRE(harness.current_room_id() == EntityId{100});
        
        // Execute commands synchronously - no timeouts or async issues
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        harness.execute_command("north")
               .then_output_contains("You move north");
        
        REQUIRE(harness.current_room_id() == EntityId{101});
        
        // Multiple commands in sequence - stable execution
        harness.execute_command("look")
               .then_output_contains("North Room");
               
        harness.execute_command("south")
               .then_output_contains("You move south");
               
        REQUIRE(harness.current_room_id() == EntityId{100});
    }
    
    SECTION("Social commands execute without interference") {
        harness.execute_command("say Hello, integration testing!")
               .then_output_contains("You say")
               .then_output_contains("Hello, integration testing!");
        
        harness.execute_command("emote celebrates successful testing")
               .then_output_contains("TestPlayer celebrates successful testing");
    }
    
    SECTION("Error conditions handled gracefully") {
        harness.execute_command("invalidcommand")
               .then_output_contains("Unknown command");
        
        harness.execute_command("west")  // Invalid direction
               .then_output_contains("Cannot move West");
        
        // Player should remain in same room after errors
        REQUIRE(harness.current_room_id() == EntityId{100});
    }
}

/**
 * Demonstration: Using test builders for complex scenarios
 */
TEST_CASE("Modern Test Patterns: Data Builders", "[integration][builders]") {
    SECTION("Combat scenario with builders") {
        auto scenario = ScenarioBuilder::create_basic_combat();
        
        // Verify scenario setup
        REQUIRE(scenario.attacker->name() == "Attacker");
        REQUIRE(scenario.attacker->player_class() == "warrior");
        REQUIRE(scenario.attacker->stats().level == 10);
        
        REQUIRE(scenario.target->name() == "Target");
        REQUIRE(scenario.target->player_class() == "sorcerer");
        REQUIRE(scenario.target->stats().level == 8);
        
        REQUIRE(scenario.combat_room->name() == "Combat Arena");
    }
    
    SECTION("Treasure hunting scenario") {
        auto scenario = ScenarioBuilder::create_treasure_hunt();
        
        REQUIRE(scenario.player->name() == "TreasureHunter");
        REQUIRE(scenario.player->player_class() == "rogue");
        REQUIRE(scenario.treasures.size() == 3);
        
        // Verify treasure objects
        bool found_crown = false;
        bool found_chalice = false;
        bool found_dagger = false;
        
        for (const auto& treasure : scenario.treasures) {
            if (treasure->name() == "Golden Crown") found_crown = true;
            if (treasure->name() == "Silver Chalice") found_chalice = true;
            if (treasure->name() == "Gem-encrusted Dagger") found_dagger = true;
        }
        
        REQUIRE(found_crown);
        REQUIRE(found_chalice);
        REQUIRE(found_dagger);
    }
}

/**
 * Demonstration: Property-based testing for game mechanics
 */
TEST_CASE("Property-Based Testing: Combat Mechanics", "[integration][property]") {
    SECTION("All character classes can be created and have valid stats") {
        std::vector<std::string> classes = {"warrior", "cleric", "sorcerer", "rogue"};
        std::vector<std::string> races = {"human", "elf", "dwarf", "halfling"};
        
        for (const auto& cls : classes) {
            for (const auto& race : races) {
                auto player = PlayerBuilder()
                    .named(fmt::format("Test{}{}", cls, race))
                    .of_class(cls)
                    .of_race(race)
                    .at_level(5)
                    .build();
                
                // Properties that should hold for all class/race combinations
                REQUIRE(player->stats().level == 5);
                REQUIRE(player->stats().max_hit_points > 0);
                REQUIRE(player->stats().strength >= 3);
                REQUIRE(player->stats().strength <= 25);
                REQUIRE(player->player_class() == cls);
                REQUIRE(player->race() == race);
                REQUIRE(player->is_alive());
                
                INFO("Testing class: " << cls << ", race: " << race);
            }
        }
    }
    
    SECTION("Combat modifiers scale correctly with level") {
        for (int level = 1; level <= 20; ++level) {
            auto warrior = PlayerBuilder()
                .as_warrior()
                .at_level(level)
                .build();
            
            auto sorcerer = PlayerBuilder()
                .as_sorcerer()
                .at_level(level)
                .build();
            
            // Properties: Warriors should always have better physical combat stats
            auto warrior_mods = CombatSystem::calculate_combat_modifiers(*warrior);
            auto sorcerer_mods = CombatSystem::calculate_combat_modifiers(*sorcerer);
            
            REQUIRE(warrior_mods.hit_bonus >= sorcerer_mods.hit_bonus);
            REQUIRE(warrior_mods.damage_bonus >= sorcerer_mods.damage_bonus);
            REQUIRE(warrior_mods.armor_class_bonus >= sorcerer_mods.armor_class_bonus);
            
            INFO("Testing level: " << level);
        }
    }
}

/**
 * Demonstration: Performance testing with benchmarks
 */
TEST_CASE("Performance Benchmarking: Critical Paths", "[integration][performance]") {
    SECTION("Command execution performance") {
        LightweightTestHarness harness;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute multiple commands to test performance
        constexpr int iterations = 50;
        for (int i = 0; i < iterations; ++i) {
            harness.execute_command("look");
            harness.get_player()->clear_output(); // Clear for next iteration
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Performance requirement: 50 commands should execute quickly
        REQUIRE(duration.count() < 100); // Less than 100ms
        
        INFO("50 command executions took " << duration.count() << "ms");
    }
    
    SECTION("Player creation performance") {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::unique_ptr<Player>> players;
        constexpr int player_count = 100;
        
        for (int i = 0; i < player_count; ++i) {
            players.push_back(PlayerBuilder()
                .named(fmt::format("Player{}", i))
                .at_level(i % 20 + 1)
                .build());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Performance requirement: Creating 100 players should be fast
        REQUIRE(duration.count() < 50); // Less than 50ms
        REQUIRE(players.size() == player_count);
        
        INFO("Creating " << player_count << " players took " << duration.count() << "ms");
    }
}

/**
 * Demonstration: Regression testing for known issues
 */
TEST_CASE("Regression Testing: Previously Fixed Issues", "[integration][regression]") {
    SECTION("Player start room handling") {
        // This tests the fix for player revival system
        auto player = PlayerBuilder()
            .named("RevivalTestPlayer")
            .build();
        
        // Initially no start room
        REQUIRE(!player->start_room().is_valid());
        
        // Set start room
        EntityId test_room{3001};
        player->set_start_room(test_room);
        REQUIRE(player->start_room() == test_room);
        
        // Test position changes (related to revival system)
        player->set_position(Position::Dead);
        REQUIRE(!player->is_alive());
        REQUIRE(!player->can_act());
        
        player->set_position(Position::Ghost);
        REQUIRE(!player->is_alive());
        REQUIRE(!player->can_act());
        
        // Simulate revival
        player->set_position(Position::Standing);
        player->stats().hit_points = player->stats().max_hit_points;
        
        REQUIRE(player->is_alive());
        REQUIRE(player->can_act());
        REQUIRE(player->stats().hit_points == player->stats().max_hit_points);
    }
    
    SECTION("Combat system stability") {
        // This tests the fix for combat system segfaults
        auto [attacker, target] = CombatTestFixture::create_combat_pair(10, 8);
        CombatTestFixture::setup_warrior_vs_sorcerer(*attacker, *target);
        
        // Convert to shared_ptr as expected by combat system
        auto attacker_ptr = std::static_pointer_cast<Actor>(attacker);
        auto target_ptr = std::static_pointer_cast<Actor>(target);
        
        // This should not segfault
        CombatManager::start_combat(attacker_ptr, target_ptr);
        
        REQUIRE(attacker_ptr->is_fighting());
        REQUIRE(target_ptr->is_fighting());
        
        // Perform multiple combat rounds without crashes
        for (int round = 0; round < 10; ++round) {
            auto result = CombatSystem::perform_attack(attacker_ptr, target_ptr);
            REQUIRE(result.type != CombatResult::Type::Miss);
            
            // Stop if target dies
            if (!target_ptr->is_alive()) break;
        }
        
        // Clean up combat
        CombatManager::end_combat(attacker_ptr);
        
        REQUIRE(!attacker_ptr->is_fighting());
        REQUIRE(!target_ptr->is_fighting());
    }
}

/**
 * Demonstration: Error boundary testing
 */
TEST_CASE("Error Boundary Testing: Edge Cases", "[integration][edge]") {
    SECTION("Invalid entity operations") {
        // Test creating entities with invalid IDs
        auto invalid_player = Player::create(INVALID_ENTITY_ID, "InvalidPlayer");
        REQUIRE(!invalid_player.has_value());
        REQUIRE(invalid_player.error().code == ErrorCode::InvalidArgument);
    }
    
    SECTION("Resource exhaustion simulation") {
        LightweightTestHarness harness;
        
        // Test many rapid commands (simulate high load)
        for (int i = 0; i < 1000; ++i) {
            harness.execute_command("look");
            harness.get_player()->clear_output();
            
            // System should remain stable
            REQUIRE(harness.get_player()->is_alive());
            REQUIRE(harness.current_room_id().is_valid());
        }
    }
    
    SECTION("Concurrent state access") {
        // Test that our lightweight approach handles state correctly
        LightweightTestHarness harness1;
        LightweightTestHarness harness2; // Should be completely isolated
        
        harness1.execute_command("say From harness 1");
        harness2.execute_command("say From harness 2");
        
        // Each harness should have its own isolated state
        auto output1 = harness1.get_player()->get_output();
        auto output2 = harness2.get_player()->get_output();
        
        REQUIRE(output1.size() == 1);
        REQUIRE(output2.size() == 1);
        REQUIRE(output1[0].find("From harness 1") != std::string::npos);
        REQUIRE(output2[0].find("From harness 2") != std::string::npos);
    }
}
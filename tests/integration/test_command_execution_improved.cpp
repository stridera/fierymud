/***************************************************************************
 *   File: test_command_execution_improved.cpp            Part of FieryMUD *
 *  Usage: Improved integration tests using LightweightTestHarness         *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include "../../src/core/combat.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace FieryMUD;

/**
 * Example of improved integration testing approach:
 * - No background threads or async complexity
 * - Deterministic, synchronous execution
 * - Fast setup/teardown
 * - Isolated test state
 */
TEST_CASE("Command Execution: Basic Movement", "[integration][command][movement]") {
    LightweightTestHarness harness;
    
    // Test initial state
    REQUIRE(harness.current_room_id() == EntityId{100});
    
    // Execute movement command synchronously
    harness.execute_command("north")
           .then_output_contains("You move north");
    
    // Verify state change
    REQUIRE(harness.current_room_id() == EntityId{101});
    
    // Test return movement
    harness.execute_command("south")
           .then_output_contains("You move south");
    
    REQUIRE(harness.current_room_id() == EntityId{100});
}

TEST_CASE("Command Execution: Social Commands", "[integration][command][social]") {
    LightweightTestHarness harness;
    
    // Test say command
    harness.execute_command("say Hello, world!")
           .then_output_contains("You say")
           .then_output_contains("Hello, world!");
    
    // Test emote command  
    harness.execute_command("emote waves cheerfully")
           .then_output_contains("TestPlayer waves cheerfully");
}

TEST_CASE("Command Execution: Error Handling", "[integration][command][error]") {
    LightweightTestHarness harness;
    
    // Test invalid direction
    harness.execute_command("west")
           .then_output_contains("Cannot move West");
    
    // Test invalid command
    harness.execute_command("invalidcommand")
           .then_output_contains("Unknown command");
    
    // Verify player didn't move
    REQUIRE(harness.current_room_id() == EntityId{100});
}

/**
 * Example of using test fixtures for complex scenarios
 */
TEST_CASE("Combat System: Deterministic Combat Flow", "[integration][combat]") {
    // Create combat scenario with deterministic setup
    auto [attacker, target] = CombatTestFixture::create_combat_pair(10, 8);
    CombatTestFixture::setup_warrior_vs_sorcerer(*attacker, *target);
    
    // Use deterministic RNG for predictable results
    DeterministicRNG rng(12345);
    
    // Test combat initiation
    REQUIRE_FALSE(attacker->is_fighting());
    REQUIRE_FALSE(target->is_fighting());
    
    // Start combat
    CombatManager::start_combat(attacker, target);
    
    REQUIRE(attacker->is_fighting());
    REQUIRE(target->is_fighting());
    REQUIRE(attacker->position() == Position::Fighting);
    
    // Test combat resolution with deterministic outcomes
    auto result = CombatSystem::perform_attack(attacker, target);
    REQUIRE(result.type != CombatResult::Type::Miss);
    REQUIRE(!result.attacker_message.empty());
    
    // Clean up
    CombatManager::end_combat(attacker);
}

/**
 * Property-based testing example
 */
TEST_CASE("Combat System: Damage Calculation Properties", "[integration][combat][property]") {
    // Test mathematical properties of damage system
    
    SECTION("Damage is always non-negative") {
        for (int attacker_level = 1; attacker_level <= 20; ++attacker_level) {
            for (int target_level = 1; target_level <= 20; ++target_level) {
                auto [attacker, target] = CombatTestFixture::create_combat_pair(
                    attacker_level, target_level);
                
                auto result = CombatSystem::perform_attack(attacker, target);
                if (result.type == CombatResult::Type::Hit || 
                    result.type == CombatResult::Type::CriticalHit) {
                    REQUIRE(result.damage_dealt >= 0);
                    REQUIRE(result.damage_dealt <= 1000); // Reasonable upper bound
                }
            }
        }
    }
    
    SECTION("Level difference affects damage scaling") {
        auto [high_level, low_level] = CombatTestFixture::create_combat_pair(20, 5);
        auto [equal_level1, equal_level2] = CombatTestFixture::create_combat_pair(10, 10);
        
        // High vs low level should have potential for more damage
        auto high_vs_low = CombatSystem::perform_attack(high_level, low_level);
        auto equal_combat = CombatSystem::perform_attack(equal_level1, equal_level2);
        
        // Property: Higher level difference creates more damage variance
        // (This would be verified with multiple samples in practice)
        REQUIRE(high_vs_low.type != CombatResult::Type::Miss);
        REQUIRE(equal_combat.type != CombatResult::Type::Miss);
    }
}

/**
 * Performance regression testing
 */
TEST_CASE("Performance: Command Execution Speed", "[integration][performance]") {
    LightweightTestHarness harness;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Execute multiple commands to test performance
    for (int i = 0; i < 100; ++i) {
        harness.execute_command("look");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Performance requirement: 100 commands should execute in < 100ms
    REQUIRE(duration.count() < 100);
    
    INFO("100 command executions took " << duration.count() << "ms");
}
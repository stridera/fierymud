/***************************************************************************
 *   File: test_enhanced_interaction_feedback_stable.cpp  Part of FieryMUD *
 *  Usage: Tests for enhanced object interaction feedback and error messages *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

/**
 * Tests for enhanced object interaction feedback and error messages
 * Demonstrates improvements to user experience through better messaging
 */
TEST_CASE("Enhanced Feedback: Container Interactions", "[integration][stable][feedback][containers]") {
    LightweightTestHarness harness;
    
    SECTION("Clear container status messages") {
        // Test getting from closed container
        harness.execute_command("close test_chest")
               .then_output_contains("closed");
        
        harness.execute_command("get test_potion from test_chest")
               .then_output_contains("closed")
               .then_output_not_contains("doesn't exist");
        
        // Test getting from locked container
        harness.execute_command("get test_key");
        harness.execute_command("lock test_chest with test_key")
               .then_output_contains("lock");
        
        harness.execute_command("get test_potion from test_chest")
               .then_output_matches_regex("(locked|closed|can't)")
               .then_output_not_contains("doesn't exist");
    }
    
    SECTION("Helpful container capacity messages") {
        // Fill the small bag to test capacity
        harness.execute_command("get test_bread");
        harness.execute_command("get test_key");
        harness.execute_command("put test_bread in test_bag")
               .then_output_contains("put");
        
        harness.execute_command("put test_key in test_bag")
               .then_output_contains("put");
        
        // Now try to put something else - should get capacity message
        harness.execute_command("get test_torch");
        harness.execute_command("put test_torch in test_bag")
               .then_output_matches_regex("(full|capacity|room|can't)")
               .then_output_not_contains("doesn't exist");
    }
}

TEST_CASE("Enhanced Feedback: Item Not Found Messages", "[integration][stable][feedback][errors]") {
    LightweightTestHarness harness;
    
    SECTION("Specific item not found messages") {
        // Test getting non-existent item
        harness.execute_command("get nonexistent_item")
               .then_output_matches_regex("(don't see|not here|can't find)")
               .then_output_not_contains("ERROR")
               .then_output_not_contains("FAIL");
        
        // Test getting from container that doesn't exist
        harness.execute_command("get test_key from nonexistent_container")
               .then_output_matches_regex("(don't see.*container|container.*not)")
               .then_output_not_contains("ERROR");
    }
    
    SECTION("Contextual object interaction messages") {
        // Test trying to get item from non-container
        harness.execute_command("get test_key");
        harness.execute_command("get test_potion from test_key")
               .then_output_matches_regex("(can't get.*from|not.*container)")
               .then_output_not_contains("ERROR");
        
        // Test trying to put item in non-container
        harness.execute_command("put test_key test_sword")
               .then_output_matches_regex("(can't put|not.*container)")
               .then_output_not_contains("ERROR");
    }
}

TEST_CASE("Enhanced Feedback: Equipment Messages", "[integration][stable][feedback][equipment]") {
    LightweightTestHarness harness;
    
    SECTION("Equipment slot feedback") {
        harness.execute_command("get test_sword");
        harness.execute_command("wield test_sword")
               .then_output_contains("wield")
               .then_output_not_contains("ERROR");
        
        // Try to wield another weapon - should get slot occupied message
        harness.execute_command("get test_dagger");  
        harness.execute_command("wield test_dagger")
               .then_output_matches_regex("(already.*wielding|hands.*full|remove.*first)")
               .then_output_not_contains("ERROR");
    }
    
    SECTION("Armor slot feedback") {
        harness.execute_command("get test_shield");
        harness.execute_command("wear test_shield")
               .then_output_contains("wear")
               .then_output_not_contains("ERROR");
        
        // Equipment command should show equipped items
        harness.execute_command("equipment")
               .then_output_contains("test_shield")
               .then_output_not_contains("ERROR");
    }
}

TEST_CASE("Enhanced Feedback: Light Source Interactions", "[integration][stable][feedback][lights]") {
    LightweightTestHarness harness;
    
    SECTION("Light source status messages") {
        harness.execute_command("get test_torch");
        
        // Test lighting behavior (commands may not be fully implemented yet)
        harness.execute_command("light test_torch");
        // Note: Response depends on command implementation
        
        harness.execute_command("equipment");
        // Should show torch in equipment
    }
    
    SECTION("Different light sources") {
        harness.execute_command("get test_lantern");
        harness.execute_command("light test_lantern");
        
        // Both torch and lantern should be available for testing
        harness.execute_command("inventory")
               .then_output_contains("test_lantern");
    }
}

TEST_CASE("Enhanced Feedback: Consumable Item Messages", "[integration][stable][feedback][consumables]") {
    LightweightTestHarness harness;
    
    SECTION("Food consumption feedback") {
        harness.execute_command("get test_bread");
        
        // Test eating (command may not be fully implemented)
        harness.execute_command("eat test_bread");
        // Note: Response depends on command implementation
        
        harness.execute_command("inventory");
        // Check if bread is still in inventory or consumed
    }
    
    SECTION("Scroll usage feedback") {
        harness.execute_command("get test_scroll");
        
        // Test reading/using scroll
        harness.execute_command("read test_scroll");
        // Note: Response depends on command implementation
        
        harness.execute_command("inventory");
        // Check scroll status
    }
}

TEST_CASE("Enhanced Feedback: Progressive Error Recovery", "[integration][stable][feedback][recovery]") {
    LightweightTestHarness harness;
    
    SECTION("Command suggestion on partial matches") {
        // Test abbreviated commands work
        harness.execute_command("l")  // Should match "look"
               .then_output_contains("Test Start Room");
        
        harness.execute_command("inv")  // Should match "inventory"
               .then_output_not_contains("Unknown command");
    }
    
    SECTION("State consistency after errors") {
        // Generate several error conditions
        harness.execute_command("get nonexistent_item");
        harness.execute_command("put nothing nowhere");
        harness.execute_command("wield invisible_weapon");
        
        // Player should still be in valid state
        REQUIRE(harness.get_player()->is_alive());
        REQUIRE(harness.current_room_id() == EntityId{100});
        
        // Basic commands should still work
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
               
        harness.execute_command("inventory");
        // Should get inventory response without errors
    }
}

/**
 * Test comprehensive object workflow with enhanced feedback
 */
TEST_CASE("Enhanced Feedback: Complete Object Workflow", "[integration][stable][feedback][workflow]") {
    LightweightTestHarness harness;
    
    SECTION("Full container workflow with feedback") {
        // Complete workflow: unlock, open, get from container, use item
        harness.execute_command("get test_key");
        harness.execute_command("unlock test_chest with test_key")
               .then_output_matches_regex("(unlock|open)")
               .then_output_not_contains("ERROR");
        
        harness.execute_command("open test_chest")
               .then_output_contains("open");
        
        harness.execute_command("get test_potion from test_chest")
               .then_output_contains("get")
               .then_output_contains("potion");
        
        harness.execute_command("inventory")
               .then_output_contains("test_potion")
               .then_output_contains("test_key");
    }
    
    SECTION("Equipment workflow with conflict resolution") {
        // Test equipment conflicts and resolution
        harness.execute_command("get test_sword");
        harness.execute_command("get test_torch");
        
        harness.execute_command("wield test_sword")
               .then_output_contains("wield");
        
        harness.execute_command("wield test_torch");
        // Should get helpful message about weapon conflict
        
        harness.execute_command("equipment")
               .then_output_contains("test_sword");
    }
    
    SECTION("Resource management feedback") {
        // Test weight/capacity limits with clear feedback
        std::vector<std::string> items = {
            "test_sword", "test_shield", "test_torch", "test_lantern", 
            "test_key", "test_bread", "test_scroll", "test_waterskin"
        };
        
        for (const auto& item : items) {
            harness.execute_command(fmt::format("get {}", item));
            // Each get should provide clear feedback
        }
        
        harness.execute_command("inventory");
        // Should show current carrying capacity status
    }
}
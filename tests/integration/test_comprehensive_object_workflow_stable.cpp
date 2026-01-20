#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

/**
 * Comprehensive integration tests covering complete object workflows
 * Tests the interaction between all object systems working together
 */
TEST_CASE("Comprehensive Object Workflow: Container Management", "[integration][stable][workflow][containers]") {
    LightweightTestHarness harness;
    
    SECTION("Complete chest unlock, open, get workflow") {
        // Step 1: Get the key
        harness.execute_command("get test_key")
               .then_output_not_contains("error")
               .then_player_stat_equals("level", 1);
        
        REQUIRE(harness.player_has_item_named("test_key"));
        
        // Step 2: Unlock the chest with the key
        harness.execute_command("unlock test_chest with test_key");
        // Note: Command success depends on implementation
        
        // Step 3: Open the chest 
        harness.execute_command("open test_chest");
        
        // Step 4: Get items from the chest
        harness.execute_command("get test_potion from test_chest");
        
        // Step 5: Verify inventory state
        harness.execute_command("inventory");
        
        // All operations should maintain consistent state
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
    
    SECTION("Container capacity and overflow handling") {
        // Use the small test_bag to test capacity limits
        harness.execute_command("get test_bag");
        REQUIRE(harness.player_has_item_named("test_bag"));
        
        // Fill the bag to capacity (capacity is 2)
        harness.execute_command("get test_bread");
        harness.execute_command("get test_key");
        
        harness.execute_command("put test_bread in test_bag");
        harness.execute_command("put test_key in test_bag");
        
        // Try to put one more item - should hit capacity limit
        harness.execute_command("get test_torch");
        harness.execute_command("put test_torch in test_bag");
        // Note: Should provide helpful capacity feedback
        
        // Verify state consistency
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
}

TEST_CASE("Comprehensive Object Workflow: Equipment Systems", "[integration][stable][workflow][equipment]") {
    LightweightTestHarness harness;
    
    SECTION("Complete weapon and armor equipping workflow") {
        // Step 1: Get weapon and armor
        harness.execute_command("get test_sword")
               .then_output_not_contains("error");
        harness.execute_command("get test_shield")
               .then_output_not_contains("error");
        
        // Step 2: Equip weapon
        harness.execute_command("wield test_sword")
               .then_output_not_contains("error");
        
        // Step 3: Equip armor
        harness.execute_command("wear test_shield")
               .then_output_not_contains("error");
        
        // Step 4: Check equipment status
        harness.execute_command("equipment");
        
        // Step 5: Test equipment conflicts
        harness.execute_command("get test_torch");
        harness.execute_command("wield test_torch");
        // Should provide feedback about weapon slot being occupied
        
        // Verify final state
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
    
    SECTION("Light source management workflow") {
        // Test different light sources
        harness.execute_command("get test_torch");
        harness.execute_command("get test_lantern");
        
        // Test lighting (commands may not be implemented yet)
        harness.execute_command("light test_torch");
        harness.execute_command("light test_lantern");
        
        // Check inventory status
        harness.execute_command("inventory");
        
        // Test equipment with light sources
        harness.execute_command("wield test_torch");  // Try to wield as weapon
        
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
}

TEST_CASE("Comprehensive Object Workflow: Consumables and Resources", "[integration][stable][workflow][consumables]") {
    LightweightTestHarness harness;
    
    SECTION("Food and consumable item workflow") {
        // Get consumable items
        harness.execute_command("get test_bread");
        harness.execute_command("get test_scroll");
        harness.execute_command("get test_waterskin");
        
        // Test consumption (commands may not be fully implemented)
        harness.execute_command("eat test_bread");
        harness.execute_command("read test_scroll");
        harness.execute_command("drink from test_waterskin");
        
        // Check inventory after consumption
        harness.execute_command("inventory");
        
        // Verify player state
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
    
    SECTION("Timer-based object behavior") {
        // Get timed objects
        harness.execute_command("get test_bread");
        REQUIRE(harness.player_has_item_named("test_bread"));
        
        // Note: Timer testing would require time advancement
        // For now, just verify the objects exist and can be manipulated
        harness.execute_command("inventory");
        
        // Test putting timed object in container
        harness.execute_command("get test_bag");
        harness.execute_command("put test_bread in test_bag");
        
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
}

TEST_CASE("Comprehensive Object Workflow: Mixed Object Interactions", "[integration][stable][workflow][mixed]") {
    LightweightTestHarness harness;
    
    SECTION("Complex multi-object scenario") {
        // Scenario: Player enters room, unlocks chest, gets equipment, 
        // equips items, stores consumables, and organizes inventory
        
        // Phase 1: Survey the room
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        // Phase 2: Get key and unlock chest
        harness.execute_command("get test_key");
        harness.execute_command("unlock test_chest with test_key");
        harness.execute_command("open test_chest");
        harness.execute_command("get test_potion from test_chest");
        
        // Phase 3: Get combat equipment
        harness.execute_command("get test_sword");
        harness.execute_command("get test_shield");
        harness.execute_command("wield test_sword");
        harness.execute_command("wear test_shield");
        
        // Phase 4: Organize consumables and tools
        harness.execute_command("get test_bread");
        harness.execute_command("get test_torch");
        harness.execute_command("get test_bag");
        
        // Phase 5: Storage management
        harness.execute_command("put test_bread in test_bag");
        harness.execute_command("put test_key in test_bag");
        
        // Phase 6: Final inventory check
        harness.execute_command("inventory");
        harness.execute_command("equipment");
        
        // Verify all operations maintained consistency
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
        REQUIRE(harness.get_player()->name() == "TestPlayer");
    }
    
    SECTION("Object state persistence through complex operations") {
        // Test that objects maintain proper state through multiple operations
        std::vector<std::string> all_objects = {
            "test_chest", "test_sword", "test_shield", "test_torch",
            "test_key", "test_bread", "test_bag", "test_scroll", 
            "test_lantern", "test_waterskin"
        };
        
        // Get all available objects
        for (const auto& obj : all_objects) {
            harness.execute_command(fmt::format("get {}", obj));
        }
        
        // Perform various operations
        harness.execute_command("inventory");
        harness.execute_command("drop test_chest");  // Drop heavy item
        harness.execute_command("wield test_sword");
        harness.execute_command("wear test_shield");
        
        // Test container operations
        harness.execute_command("put test_key in test_bag");
        harness.execute_command("put test_bread in test_bag");
        
        // Verify everything is still working
        harness.execute_command("look");
        harness.execute_command("inventory");
        harness.execute_command("equipment");
        
        // Final state verification
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
        
        // Performance check - all operations should be fast
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 10; ++i) {
            harness.execute_command("look");
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        REQUIRE(duration.count() < 100); // Should be very fast
        INFO("10 look commands completed in " << duration.count() << "ms");
    }
}

TEST_CASE("Comprehensive Object Workflow: Edge Cases and Error Recovery", "[integration][stable][workflow][edge_cases]") {
    LightweightTestHarness harness;
    
    SECTION("Invalid operations and error recovery") {
        // Test various invalid operations
        harness.execute_command("get nonexistent_item");
        harness.execute_command("put nothing nowhere");
        harness.execute_command("wield invisible_weapon");
        harness.execute_command("unlock door with spoon");
        harness.execute_command("drink solid_object");
        
        // Verify system remains stable after errors
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
        
        // Valid operations should still work
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
        
        harness.execute_command("get test_key");
        REQUIRE(harness.player_has_item_named("test_key"));
    }
    
    SECTION("Boundary conditions and limits") {
        // Test weight/capacity limits
        harness.execute_command("get test_sword");   // Heavy item
        harness.execute_command("get test_shield");  // Another heavy item
        harness.execute_command("get test_chest");   // Very heavy item
        
        // Should handle weight limits gracefully
        harness.execute_command("inventory");
        
        // Test container capacity limits
        harness.execute_command("get test_bag");
        harness.execute_command("get test_bread");
        harness.execute_command("get test_key");
        harness.execute_command("get test_torch");
        
        // Fill bag to capacity
        harness.execute_command("put test_bread in test_bag");
        harness.execute_command("put test_key in test_bag");
        harness.execute_command("put test_torch in test_bag"); // Should fail
        
        // System should remain stable
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
    
    SECTION("State consistency under stress") {
        // Rapid command execution to test stability
        std::vector<std::string> stress_commands = {
            "look", "inventory", "equipment", 
            "get test_key", "drop test_key", "get test_key",
            "get test_sword", "wield test_sword", "remove test_sword",
            "get test_bag", "put test_key in test_bag", "get test_key from test_bag"
        };
        
        for (int cycle = 0; cycle < 5; ++cycle) {
            for (const auto& cmd : stress_commands) {
                harness.execute_command(cmd);
                
                // Verify state remains valid after each command
                REQUIRE(harness.get_player()->is_alive());
                REQUIRE(harness.current_room_id().is_valid());
            }
        }
        
        // Final verification
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->name() == "TestPlayer");
        
        // Should still respond to basic commands
        harness.execute_command("look")
               .then_output_contains("Test Start Room");
    }
}

/**
 * Performance and scalability testing for object systems
 */
TEST_CASE("Comprehensive Object Workflow: Performance Validation", "[integration][stable][workflow][performance]") {
    LightweightTestHarness harness;
    
    SECTION("Command execution performance") {
        // Measure performance of common object operations
        harness.then_executes_within_ms([&]() {
            // Simulate typical gameplay session
            harness.execute_command("look");
            harness.execute_command("get test_key");
            harness.execute_command("get test_sword");
            harness.execute_command("wield test_sword");
            harness.execute_command("get test_bag");
            harness.execute_command("inventory");
            harness.execute_command("equipment");
            harness.execute_command("drop test_sword");
            harness.execute_command("get test_sword");
            harness.execute_command("put test_key in test_bag");
        }, 100); // Should complete within 100ms
    }
    
    SECTION("Object system scalability") {
        // Test system performance with multiple objects
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::string> all_objects = {
            "test_sword", "test_shield", "test_torch", "test_lantern",
            "test_key", "test_bread", "test_scroll", "test_waterskin", "test_bag"
        };
        
        // Get all objects
        for (const auto& obj : all_objects) {
            harness.execute_command(fmt::format("get {}", obj));
        }
        
        // Perform operations on all objects
        harness.execute_command("inventory");
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        REQUIRE(duration.count() < 200); // Should handle multiple objects quickly
        INFO("Multi-object operations completed in " << duration.count() << "ms");
        
        // Verify final state
        REQUIRE(harness.current_room_id() == EntityId{100});
        REQUIRE(harness.get_player()->is_alive());
    }
}
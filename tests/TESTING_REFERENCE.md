# FieryMUD Testing Quick Reference

Quick reference card for developers using the FieryMUD test framework.

## Test Framework at a Glance

### Recommended Usage
```bash
# Run stable tests (recommended)
./run_tests.sh stable

# Run all recommended tests  
./run_tests.sh                  # Default
```

### File Structure
```
tests/
├── unit/test_*.cpp                    # Unit tests
├── integration/test_*_stable.cpp      # Stable integration tests ← USE THESE
├── integration/test_*.cpp             # Legacy integration tests (avoid)
└── common/lightweight_test_harness.hpp # Modern test framework
```

## LightweightTestHarness Quick API

### Setup
```cpp
#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test Name", "[integration][stable]") {
    LightweightTestHarness harness;  // Auto-initialized with test world
    // Test code here
}
```

### Command Execution
```cpp
// Basic execution with fluent assertions
harness.execute_command("look")
       .then_output_contains("Test Start Room")
       .then_output_not_contains("error");

// Multiple commands
harness.execute_command("get sword")
       .execute_command("wield sword");

// Accumulate output across commands
harness.execute_command_accumulate("say first");
harness.execute_command_accumulate("say second");
// Output now contains both messages
```

### Assertions
```cpp
// Output validation
.then_output_contains("text")              // Must contain text
.then_output_not_contains("error")         // Must NOT contain text  
.then_output_matches_regex(R"(\d+)")       // Must match regex pattern
.then_output_size_is(2)                    // Exact line count

// State validation  
REQUIRE(harness.current_room_id() == EntityId{100});
REQUIRE(harness.player_is_in_room(EntityId{100}));
REQUIRE(harness.player_has_item_named("sword"));
REQUIRE(harness.get_player()->is_alive());

// Player stats
harness.then_player_stat_equals("level", 1);
harness.then_player_stat_equals("hp", 20);
```

### Output Analysis
```cpp
// Count matching lines
auto count = harness.count_output_lines_containing("You say");

// Get specific lines
auto lines = harness.get_output_lines_containing("You say");

// Direct output access
const auto& output = harness.get_player()->get_output();
```

### Performance Testing
```cpp
// Measure execution time
harness.then_executes_within_ms([&]() {
    for (int i = 0; i < 100; ++i) {
        harness.execute_command("look");
    }
}, 1000); // Must complete within 1000ms
```

## Available Test Objects

| Object | Type | Usage | Special Features |
|--------|------|-------|------------------|
| `test_chest` | Container | `get potion from chest`, `open chest`, `lock chest with key` | Lockable, closeable, contains potion |
| `test_sword` | Weapon | `get sword`, `wield sword`, `equipment` | 2d6+2 damage, reach 1, speed 2 |
| `test_shield` | Armor | `get shield`, `wear shield`, `equipment` | AC 3, steel material |
| `test_torch` | Light | `get torch`, `light torch`, `equipment` | Duration 100, brightness 2, starts unlit |
| `test_key` | Key | `get key`, `unlock chest with key` | Unlocks test_chest specifically |
| `test_bread` | Food | `get bread`, `eat bread` | 24-hour spoilage timer |
| `test_bag` | Container | `put item in bag`, capacity testing | Capacity 2, always open |
| `test_scroll` | Scroll | `get scroll`, `read scroll` | Single-use consumable |
| `test_lantern` | Light | `get lantern`, `light lantern` | Duration 500, brightness 4, more powerful |
| `test_waterskin` | Liquid Container | `get waterskin`, `drink from waterskin` | For liquid consumption testing |

## Test Patterns

### Basic Command Test
```cpp
TEST_CASE("Command Test", "[integration][stable]") {
    LightweightTestHarness harness;
    harness.execute_command("look")
           .then_output_contains("Test Start Room");
}
```

### Multi-Step Workflow
```cpp
TEST_CASE("Equipment Workflow", "[integration][stable]") {
    LightweightTestHarness harness;
    
    harness.execute_command("get test_sword")
           .then_output_contains("You get");
           
    harness.execute_command("wield test_sword")
           .then_output_contains("You wield");
           
    harness.execute_command("equipment")
           .then_output_contains("test_sword");
}
```

### Error Handling
```cpp
TEST_CASE("Error Handling", "[integration][stable]") {
    LightweightTestHarness harness;
    
    harness.execute_command("invalidcommand")
           .then_output_contains("Unknown command");
    
    // State should remain consistent
    REQUIRE(harness.get_player()->is_alive());
}
```

### Container Testing
```cpp
TEST_CASE("Container Test", "[integration][stable]") {
    LightweightTestHarness harness;
    
    // Get item from container
    harness.execute_command("get test_potion from test_chest");
    
    // Test capacity limits
    harness.execute_command("put test_bread in test_bag");
    harness.execute_command("put test_key in test_bag");     // Should work
    harness.execute_command("put test_torch in test_bag");   // Should fail
}
```

### Performance Test
```cpp
TEST_CASE("Performance", "[integration][stable][performance]") {
    LightweightTestHarness harness;
    
    harness.then_executes_within_ms([&]() {
        for (int i = 0; i < 50; ++i) {
            harness.execute_command("look");
        }
    }, 500);
    
    // Verify stability
    REQUIRE(harness.get_player()->is_alive());
}
```

## Debugging Tests

### Print Output for Debugging
```cpp
const auto& output = harness.get_player()->get_output();
for (size_t i = 0; i < output.size(); ++i) {
    std::cout << "Output[" << i << "]: '" << output[i] << "'" << std::endl;
}
```

### Common Issues

| Problem | Solution |
|---------|----------|
| Test not found | File must end with `_stable.cpp` |
| Compilation error | Include `lightweight_test_harness.hpp` and `catch2/catch_test_macros.hpp` |
| Assertion failed | Use `.then_output_contains()` with exact expected text |
| Slow tests | Use `--durations yes` to identify slow tests |

## Build Commands

```bash
# Build specific targets
cmake --build build --target stable_tests
cmake --build build --target unit_tests

# Run with filters
./build/stable_tests "[objects]"           # Object tests only
./build/stable_tests "[enhanced]"          # Enhanced features  
./build/stable_tests --list-tests          # List available tests

# Performance information
./build/stable_tests --durations yes       # Show test timing
```

## Best Practices

### ✅ DO
- Use `LightweightTestHarness` for integration tests
- Name files `test_*_stable.cpp` 
- Include `[stable]` tag in test cases
- Test both success and error cases
- Use available test objects
- Verify state changes after commands

### ❌ DON'T  
- Use legacy `TestHarness` (unstable)
- Write tests without `[stable]` tag
- Share state between test cases
- Ignore command execution results
- Write overly complex tests

## File Template

```cpp
/***************************************************************************
 *   File: test_my_feature_stable.cpp                Part of FieryMUD      *
 *  Usage: Stable integration tests for MyFeature                          *
 ***************************************************************************/

#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MyFeature: Basic Functionality", "[integration][stable][myfeature]") {
    LightweightTestHarness harness;
    
    // Test basic functionality
    harness.execute_command("mycommand")
           .then_output_contains("expected output")
           .then_output_not_contains("error");
    
    // Verify state changes
    REQUIRE(harness.current_room_id().is_valid());
    REQUIRE(harness.get_player()->is_alive());
}

TEST_CASE("MyFeature: Error Handling", "[integration][stable][myfeature]") {
    LightweightTestHarness harness;
    
    // Test error conditions
    harness.execute_command("mycommand invalid_args")
           .then_output_contains("Invalid");
    
    // State should remain consistent
    REQUIRE(harness.get_player()->is_alive());
}
```

---

*Quick Reference v1.0 - Last Updated: 2024-08-27*
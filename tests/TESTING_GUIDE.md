# FieryMUD Testing Framework Guide

This guide provides comprehensive documentation for developers working with the FieryMUD testing infrastructure.

## Table of Contents

1. [Overview](#overview)
2. [Test Framework Architecture](#test-framework-architecture)
3. [Running Tests](#running-tests)
4. [Writing Tests](#writing-tests)
5. [LightweightTestHarness Usage](#lightweighttestharness-usage)
6. [Available Test Objects](#available-test-objects)
7. [Testing Patterns](#testing-patterns)
8. [Troubleshooting](#troubleshooting)

## Overview

FieryMUD uses a modern test framework built on **Catch2** with custom test harnesses designed for MUD-specific functionality. The framework provides:

- **Reliable** stable tests (no segfaults)
- **Fast execution** (synchronous, no threads)
- **Rich test objects** for comprehensive integration testing
- **Isolated test environments** (no shared state)

## Test Framework Architecture

### Test Categories

| Category | Executable | Description |
|----------|------------|-------------|
| **Unit Tests** | `unit_tests` | Fast, isolated component tests |
| **Integration Tests** | `stable_tests` | Reliable integration tests with LightweightTestHarness |

### Directory Structure

```
tests/
├── unit/                    # Unit tests using simple fixtures
│   ├── test_core.cpp       # Core system tests
│   ├── test_actor.cpp      # Actor system tests
│   └── ...
├── integration/             # Integration tests
│   └── test_*_stable.cpp   # Integration tests with LightweightTestHarness
├── common/                  # Shared test infrastructure
│   ├── lightweight_test_harness.hpp  # Modern test harness
│   └── test_builders.hpp   # Test object builders
└── TESTING_GUIDE.md        # This guide
```

### Build Targets

```bash
cmake --build build --target unit_tests      # Unit tests only
cmake --build build --target stable_tests    # Integration tests
cmake --build build --target tests           # Combined (unit + integration)
```

## Running Tests

### Quick Start

```bash
# Run all tests (unit + integration)
./run_tests.sh                    # Default
./run_tests.sh all                # Same as above

# Run specific test categories
./run_tests.sh unit               # Unit tests only
./run_tests.sh stable             # Integration tests only
./run_tests.sh verbose            # Verbose output
```

### Direct Test Execution

```bash
# Run integration tests with specific filters
./build/stable_tests "[objects]"           # Object system tests
./build/stable_tests "[command][stable]"   # Command system tests

# Unit tests
./build/unit_tests "[unit][containers]"    # Container unit tests
./build/unit_tests --list-tests            # See available tests
```

### CTest Integration

```bash
# Run via CTest (integrates with CI/CD)
ctest --test-dir build -L unit             # Unit tests
ctest --test-dir build -L stable           # Integration tests
ctest --test-dir build --verbose           # Verbose output
```

## Writing Tests

### Choosing the Right Framework

| Test Type | Use | Framework | Location |
|-----------|-----|-----------|----------|
| **Unit Tests** | Test individual classes/functions in isolation | Catch2 + Simple fixtures | `tests/unit/` |
| **Integration Tests** | Test component interactions, commands, gameplay | LightweightTestHarness | `tests/integration/*_stable.cpp` |

### Unit Test Example

```cpp
// tests/unit/test_my_feature.cpp
#include <catch2/catch_test_macros.hpp>
#include "core/object.hpp"

TEST_CASE("Object Creation", "[unit][objects]") {
    SECTION("Container creation") {
        auto result = Container::create(EntityId{1001}, "test_chest", 10);
        REQUIRE(result.has_value());

        auto container = std::move(result.value());
        REQUIRE(container->is_container());
        REQUIRE(container->is_empty());
    }
}
```

### Integration Test Example

```cpp
// tests/integration/test_my_feature_stable.cpp  <- Note "_stable" suffix
#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Feature Integration", "[integration][stable][my_feature]") {
    SECTION("Basic functionality") {
        LightweightTestHarness harness;

        // Execute commands and verify results
        harness.execute_command("get test_sword")
               .then_output_contains("You get")
               .then_output_not_contains("error");

        // Verify state changes
        REQUIRE(harness.player_has_item_named("test_sword"));
        REQUIRE(harness.current_room_id() == EntityId{100});
    }
}
```

## LightweightTestHarness Usage

The `LightweightTestHarness` is the framework for integration testing.

### Key Features

- **Synchronous execution** (no threading issues)
- **Isolated test environments** (no shared state)
- **Rich test objects** automatically created
- **Fluent assertion API** for readable tests
- **Performance measurement** capabilities

### Basic Usage

```cpp
#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Command Testing", "[integration][stable]") {
    LightweightTestHarness harness;

    // Execute commands with fluent assertions
    harness.execute_command("look")
           .then_output_contains("Test Start Room")
           .then_output_size_is(1);

    // Access player and world state
    auto player = harness.get_player();
    auto& world = harness.get_world();

    // Verify state
    REQUIRE(harness.current_room_id() == EntityId{100});
    REQUIRE(player->is_alive());
}
```

### Command Execution Methods

```cpp
// Basic command execution (clears previous output)
harness.execute_command("say hello");

// Command execution with output accumulation
harness.execute_command_accumulate("say first");
harness.execute_command_accumulate("say second");
// Output now contains both messages

// Multi-command execution
std::vector<std::string> commands = {"look", "north", "south"};
harness.execute_commands(commands);           // Clears between commands
harness.execute_commands_accumulate(commands); // Accumulates all output
```

### Fluent Assertion API

```cpp
harness.execute_command("say Hello World")
       .then_output_contains("You say")          // Must contain text
       .then_output_not_contains("error")        // Must NOT contain text
       .then_output_matches_regex(R"(\d+)")      // Must match regex
       .then_output_size_is(1)                   // Exact output line count
       .then_player_stat_equals("level", 5);     // Player stat validation
```

### State Validation

```cpp
// Room and location testing
REQUIRE(harness.player_is_in_room(EntityId{100}));
REQUIRE(harness.current_room_id().is_valid());

// Inventory testing
REQUIRE(harness.player_has_item_named("sword"));

// Output analysis
auto say_count = harness.count_output_lines_containing("You say");
auto say_lines = harness.get_output_lines_containing("You say");
```

### Performance Testing

```cpp
// Measure execution time
harness.then_executes_within_ms([&]() {
    for (int i = 0; i < 100; ++i) {
        harness.execute_command("look");
    }
}, 500); // Must complete within 500ms
```

## Available Test Objects

The LightweightTestHarness automatically creates diverse test objects in the start room:

### Interactive Objects

| Object | Type | Properties | Use Cases |
|--------|------|------------|-----------|
| `test_chest` | Container | Closeable, lockable, capacity 5, contains `test_potion` | Container testing, lock/unlock mechanics |
| `test_sword` | Weapon | Damage 2d6+2, reach 1, speed 2 | Equipment, combat, wielding |
| `test_shield` | Armor | AC 3, steel material | Equipment, wearing, protection |
| `test_torch` | Light | Duration 100, brightness 2, unlit | Light sources, illumination |
| `test_key` | Key | Unlocks `test_chest` specifically | Lock/unlock mechanics |
| `test_bread` | Food | Timer 24hrs, consumable | Consumption, spoilage |
| `test_bag` | Container | Capacity 2, always open | Capacity limits, overflow testing |
| `test_scroll` | Scroll | Single-use consumable | Magic item usage, consumption |
| `test_lantern` | Light | Duration 500, brightness 4, more powerful | Advanced light source testing |
| `test_waterskin` | Liquid Container | Holds liquids | Drink mechanics, liquid containers |

### Testing Scenarios

```cpp
// Container interaction
harness.execute_command("get test_potion from test_chest")
       .then_output_contains("You get");

// Equipment testing
harness.execute_command("get test_sword")
       .execute_command("wield test_sword")
       .execute_command("equipment");

// Capacity limits
harness.execute_command("put test_bread in test_bag");
harness.execute_command("put test_key in test_bag");   // Should work
harness.execute_command("put test_torch in test_bag"); // Should fail (capacity)

// Lock/unlock mechanics
harness.execute_command("close test_chest")
       .execute_command("lock test_chest with test_key")
       .execute_command("get test_potion from test_chest"); // Should fail
```

## Testing Patterns

### Pattern 1: Command Response Testing

```cpp
TEST_CASE("Command Responses", "[integration][stable]") {
    LightweightTestHarness harness;

    SECTION("Valid command") {
        harness.execute_command("look")
               .then_output_contains("Test Start Room")
               .then_output_not_contains("error");
    }

    SECTION("Invalid command") {
        harness.execute_command("invalidcommand")
               .then_output_contains("Unknown command");
    }
}
```

### Pattern 2: State Change Validation

```cpp
TEST_CASE("State Changes", "[integration][stable]") {
    LightweightTestHarness harness;

    // Initial state
    REQUIRE(harness.current_room_id() == EntityId{100});

    // Action
    harness.execute_command("north");

    // Verify state change
    REQUIRE(harness.current_room_id() == EntityId{101});
}
```

### Pattern 3: Multi-Step Workflows

```cpp
TEST_CASE("Complete Workflows", "[integration][stable]") {
    LightweightTestHarness harness;

    // Multi-step equipment workflow
    harness.execute_command("get test_sword")
           .then_output_contains("You get");

    harness.execute_command("wield test_sword")
           .then_output_contains("You wield");

    harness.execute_command("equipment")
           .then_output_contains("test_sword");
}
```

### Pattern 4: Error Condition Testing

```cpp
TEST_CASE("Error Handling", "[integration][stable]") {
    LightweightTestHarness harness;

    // Test capacity limits
    harness.execute_command("get test_bread");
    harness.execute_command("get test_key");
    harness.execute_command("put test_bread in test_bag");
    harness.execute_command("put test_key in test_bag");

    // This should fail due to capacity
    harness.execute_command("get test_torch");
    harness.execute_command("put test_torch in test_bag")
           .then_output_matches_regex(R"(full|capacity|room)");
}
```

### Pattern 5: Performance Testing

```cpp
TEST_CASE("Performance", "[integration][stable][performance]") {
    LightweightTestHarness harness;

    // Rapid command execution test
    harness.then_executes_within_ms([&]() {
        for (int i = 0; i < 50; ++i) {
            harness.execute_command("look");
        }
    }, 500);

    // System should remain stable
    REQUIRE(harness.get_player()->is_alive());
}
```

## Troubleshooting

### Common Issues

#### Tests Not Found
```bash
# Problem: Test file not being compiled
# Solution: Ensure filename ends with "_stable.cpp" for integration tests
mv test_my_feature.cpp test_my_feature_stable.cpp
```

#### Compilation Errors
```cpp
// Problem: Missing includes
#include "../common/lightweight_test_harness.hpp"  // Required
#include <catch2/catch_test_macros.hpp>           // Required

// Problem: Wrong test tags
TEST_CASE("My Test", "[integration][stable]") {  // <- Include [stable] tag
```

#### Test Failures
```cpp
// Problem: Output not found
harness.execute_command("look")
       .then_output_contains("Test Start Room");  // Check exact text

// Problem: State not updated
harness.execute_command("north");
REQUIRE(harness.current_room_id() == EntityId{101});
```

#### Performance Issues
```bash
# Problem: Slow tests
./build/stable_tests --durations yes  # Identify slow tests

# Use lighter test patterns
harness.execute_command("look");  # Instead of complex multi-command sequences
```

### Debugging Tests

```cpp
// Enable debug output
TEST_CASE("Debug Test", "[integration][stable]") {
    LightweightTestHarness harness;

    harness.execute_command("look");

    // Print actual output for debugging
    const auto& output = harness.get_player()->get_output();
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "Output[" << i << "]: '" << output[i] << "'" << std::endl;
    }
}
```

### Test Isolation

```cpp
// Each test case gets a fresh harness
TEST_CASE("Isolated Test 1", "[integration][stable]") {
    LightweightTestHarness harness1;  // Fresh environment
    // ... test code
}

TEST_CASE("Isolated Test 2", "[integration][stable]") {
    LightweightTestHarness harness2;  // Completely separate
    // ... test code
}
```

## Best Practices

### DO

- Use **LightweightTestHarness** for integration tests
- Name integration test files with **"_stable.cpp"** suffix
- Include **"[stable]"** tag in integration test cases
- Use **fluent assertions** for readable tests
- Test **both success and failure cases**
- Verify **state changes** after commands
- Use **available test objects** for comprehensive testing

### DON'T

- Write tests without proper tags
- Share state between test cases
- Ignore command execution results
- Skip error condition testing
- Write overly complex tests

### Checklist for New Tests

- [ ] File named appropriately (`test_*.cpp` for unit, `*_stable.cpp` for integration)
- [ ] Includes `LightweightTestHarness` for integration tests
- [ ] Test case has appropriate tags (`[unit]` or `[integration][stable]`)
- [ ] Tests both success and error cases
- [ ] Verifies command output
- [ ] Checks state changes
- [ ] Runs within performance budget

## Contributing

When adding new test functionality:

1. **Follow existing patterns** from existing tests
2. **Document new test objects** in this guide
3. **Update examples** if adding new harness features
4. **Run full test suite** before committing

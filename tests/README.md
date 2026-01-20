# FieryMUD Test Suite

This directory contains the test suite for FieryMUD, built using the Catch2 testing framework.

## Test Organization

Tests are organized by functionality:

### Unit Tests (`tests/unit/`)

Fast, isolated component tests:

- **test_core.cpp** - Core system functionality (Entity, Result, Error handling)
- **test_core_arguments.cpp** - Command argument parsing and handling
- **test_combat_system.cpp** - Combat mechanics, class abilities, damage calculation
- **test_actor.cpp** - Actor/Mobile JSON parsing, stats validation
- **test_container_system_simple.cpp** - Object containers and inventory
- **test_container_inheritance.cpp** - Container inheritance hierarchies
- **test_object_descriptions.cpp** - Object description system
- **test_object_interactions.cpp** - Complex object interactions
- **test_actual_bag_loading.cpp** - Bag loading and management
- **test_world_parsing.cpp** - World file parsing
- **test_zone_parsing.cpp** - Zone file parsing
- **test_world_utils.cpp** - World management utilities
- **test_player_revival.cpp** - Player revival and resurrection
- **test_command_ability_link.cpp** - Command-to-ability binding
- **test_quest_system.cpp** - Quest system functionality
- **test_scripting.cpp** - Lua scripting integration
- **test_db_parsing.cpp** - Database parsing utilities
- **test_text_format.cpp** - Text formatting utilities
- **test_modern_patterns.cpp** - Modern C++23 patterns

### Integration Tests (`tests/integration/`)

Comprehensive integration tests using `LightweightTestHarness`:

- **test_command_system_stable.cpp** - Command registration, parsing, execution
- **test_session_stable.cpp** - Session lifecycle and character creation
- **test_multiplayer_stable.cpp** - Multi-player interactions
- **test_comprehensive_object_workflow_stable.cpp** - Complex object workflows
- **test_enhanced_features_demo_stable.cpp** - Enhanced MUD features
- **test_enhanced_interaction_feedback_stable.cpp** - Interaction feedback systems

### Test Infrastructure (`tests/common/`)

- **lightweight_test_harness.hpp** - Modern test harness for integration testing
- **test_builders.hpp** - Fluent builder pattern for test data creation
- **mock_game_session.hpp/.cpp** - Mock objects for testing game sessions

## Running Tests

### Quick Start
```bash
./run_tests.sh              # Run all tests (unit + integration)
./run_tests.sh verbose      # Run with verbose output
```

### By Category
```bash
./run_tests.sh unit         # Unit tests only
./run_tests.sh stable       # Integration tests only
./build/tests "[unit]"      # Unit tests via Catch2
./build/tests "[stable]"    # Integration tests via Catch2
```

### By Component
```bash
./build/tests "[combat]"    # Combat system tests
./build/tests "[actor]"     # Actor/Mobile tests
./build/tests "[objects]"   # Object system tests
./build/tests "[session]"   # Session tests
```

## Test Coverage

### Well-Covered Systems
- Core systems (Entity, Result, Arguments)
- Combat system (class abilities, damage calculation)
- Command system (parsing, execution, permissions)
- Actor/Mobile creation and management
- Object containers and interactions
- Session lifecycle and character creation
- World and room management
- Lua scripting integration
- Quest system

### Areas Needing More Tests
- Advanced combat scenarios
- Complex multi-player interactions
- Error recovery and edge cases

## Adding New Tests

### Unit Tests
1. Create file: `tests/unit/test_[component].cpp`
2. Use Catch2 with simple fixtures
3. Include appropriate tags: `[unit][component_name]`

```cpp
#include <catch2/catch_test_macros.hpp>
#include "core/object.hpp"

TEST_CASE("Component: Basic Functionality", "[unit][component]") {
    // Test implementation
    REQUIRE(condition);
}
```

### Integration Tests
1. Create file: `tests/integration/test_[feature]_stable.cpp` (note the `_stable` suffix)
2. Use `LightweightTestHarness`
3. Include tags: `[integration][stable][feature_name]`

```cpp
#include "../common/lightweight_test_harness.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Feature: Description", "[integration][stable][feature]") {
    LightweightTestHarness harness;

    harness.execute_command("look")
           .then_output_contains("Test Start Room")
           .then_output_not_contains("error");
}
```

## Build Targets

```bash
cmake --build build --target unit_tests     # Unit tests
cmake --build build --target stable_tests   # Integration tests
cmake --build build --target tests          # Combined (unit + integration)
```

## Continuous Integration

Tests are run automatically on:
- Code changes via CMake/CTest integration
- Manual execution via `./run_tests.sh`
- Build verification before deployment

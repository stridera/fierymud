# FieryMUD Test Suite

This directory contains the test suite for FieryMUD, built using the Catch2 testing framework.

## Test Organization

Tests are organized by functionality and tagged for easy filtering:

### Test Categories

#### Unit Tests `[unit]`
- **test_core.cpp** - Core system functionality (Entity, Result, Error handling)
- **test_core_arguments.cpp** - Command argument parsing and handling  
- **test_combat_system.cpp** - Combat mechanics, class abilities, damage calculation
- **test_actor.cpp** - Actor/Mobile JSON parsing, stats validation, level handling
- **test_container_system.cpp** & **test_container_system_simple.cpp** - Object containers and inventory
- **test_object_descriptions.cpp** - Object description system
- **test_mobile_keywords.cpp** - Mobile keyword parsing and matching

#### Integration Tests `[integration]`  
- **test_session_integration.cpp** - Full session lifecycle, character creation, movement
- **test_command_system.cpp** - Command registration, parsing, execution
- **test_world.cpp** - Room and actor integration, object interaction
- **test_object_interactions.cpp** - Complex object command interactions
- **test_server_resilience.cpp** - Server error handling and recovery
- **test_multiplayer.cpp** - Multi-player interactions
- **test_performance.cpp** - Performance benchmarks
- **test_emotes.cpp** - Emote command integration
- **test_gmcp.cpp** - GMCP protocol integration
- **test_session.cpp** - Basic session management

#### Specialized Tests
- **test_world_utils.cpp** - World management utilities 
- **test_world_parsing.cpp** - World file parsing
- **test_zone_parsing.cpp** - Zone file parsing
- **test_test_harness.cpp** - Test framework itself

### Test Infrastructure

- **test_harness.hpp** - Core testing infrastructure and utilities
- **mock_game_session.hpp/.cpp** - Mock objects for testing game sessions

## Running Tests

### All Tests
```bash
./run_tests.sh                  # All tests
./build/tests                   # Direct execution
```

### By Category  
```bash
./run_tests.sh unit             # Unit tests only
./run_tests.sh integration      # Integration tests only
./run_tests.sh session          # Session tests only
./build/tests "[unit]"          # Unit tests via Catch2
./build/tests "[integration]"   # Integration tests via Catch2
```

### By Component
```bash
./build/tests "[combat]"        # Combat system tests
./build/tests "[actor]"         # Actor/Mobile tests  
./build/tests "[emotes]"        # Emote tests
./build/tests "[gmcp]"          # GMCP protocol tests
```

### Verbose Output
```bash
./run_tests.sh verbose          # Verbose test output
./build/tests --verbosity high  # High verbosity
```

## Test Coverage

### Well-Covered Systems
- ✅ Core systems (Entity, Result, Arguments)
- ✅ Combat system (class abilities, damage calculation)
- ✅ Command system (parsing, execution, permissions)
- ✅ Actor/Mobile creation and management
- ✅ Object containers and interactions
- ✅ Session lifecycle and character creation
- ✅ World and room management
- ✅ GMCP protocol implementation

### Recently Fixed/Implemented
- ✅ Player start room initialization and revival (room 3001 default)
- ✅ Dead player command restrictions (comprehensive whitelist)
- ✅ Prompt system with combat status (HP/Move/Enemy condition)

### Areas Needing More Tests
- ⚠️ Container system (some test failures)
- ⚠️ Clan system functionality
- ⚠️ Advanced combat scenarios
- ⚠️ Complex multi-player interactions
- ⚠️ Error recovery and edge cases

## Adding New Tests

1. **Create test file**: `test_[component].cpp`
2. **Use appropriate tags**: `[unit]`, `[integration]`, `[component_name]`
3. **Include test harness**: `#include "test_harness.hpp"`
4. **Follow naming convention**: `TEST_CASE("Component: Feature Description", "[tags]")`

### Example Test Structure
```cpp
#include <catch2/catch_test_macros.hpp>
#include "test_harness.hpp"

TEST_CASE("Component: Basic Functionality", "[unit][component]") {
    TestHarness harness;
    // Test implementation
    REQUIRE(condition);
}
```

## Continuous Integration

Tests are run automatically on:
- Code changes via CMake/CTest integration
- Manual execution via `./run_tests.sh`
- Build verification before deployment
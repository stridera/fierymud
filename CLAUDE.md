# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Conversation Guidelines
- Primary Objective: Engage in honest, insight-driven dialogue that advances understanding.

### Core Principles
- Intellectual honesty: Share genuine insights without unnecessary flattery or dismissiveness
- Critical engagement: Push on important considerations rather than accepting ideas at face value
- Balanced evaluation: Present both positive and negative opinions only when well-reasoned and warranted
- Directional clarity: Focus on whether ideas move us forward or lead us astray

### What to Avoid
- Sycophantic responses or unwarranted positivity
- Dismissing ideas without proper consideration
- Superficial agreement or disagreement
- Flattery that doesn't serve the conversation

### Success Metric
The only currency that matters: Does this advance or halt productive thinking? If we're heading down an unproductive path, point it out directly.

### BEST PRACTICES:

#### Development Workflow
1. Always run tests before committing.  They should all be passing.
2. Use semantic versioning for releases
3. Follow existing code style and conventions
4. Document all significant changes
5. Review changes before finalizing

#### Error Handling
1. Catch and log all errors appropriately
2. Never suppress error messages
3. Fail gracefully with helpful error messages
4. Create rollback plans for risky operations

#### Communication
1. Log all significant actions
2. Create summary reports after task completion
3. Highlight any issues or concerns
4. Ask for clarification if tasks are ambiguous

### ALLOWED ACTIONS:

#### Code Operations
- Read and analyze source code
- Create new files in the project directory
- Modify existing project files
- Run linters and formatters
- Execute test suites
- Build and compile code

#### Git Operations
- Stay within the current branch unless told otherwise.
- Stage and commit changes
- View git history and diffs
- Create pull requests
- Tag releases

#### Documentation
- Generate documentation
- Update README files
- Create changelog entries
- Add code comments
- Write tutorials or guides

#### Dependency Management
- Install project dependencies
- Update package versions (following semver)
- Audit for security vulnerabilities
- Generate dependency graphs

### FORBIDDEN ACTIONS:

1. Accessing user's personal files
2. Modifying IDE or editor configurations
3. Changing system settings
4. Installing global packages without permission
5. Accessing credentials or secrets
6. Making external API calls without explicit permission
7. Modifying production configurations
8. Deleting user data or backups

Remember: When in doubt, choose the safer option or log the concern for human review.

## C++23 Modernization Guidelines

When working on the MUD codebase, follow these modern C++23 practices:

### Required Modern Features
- **Strings**: Use `fmt::format` instead of printf family, `std::string_view` for parameters, `std::string` for owned strings
- **Containers**: Use `std::vector`, `std::array`, `std::span` instead of C arrays, `std::unordered_map` for hash tables
- **Memory**: Smart pointers (`std::unique_ptr`, `std::shared_ptr`) instead of raw pointers, RAII for all resources
- **Algorithms**: `std::ranges` and `std::views` instead of manual loops, range-based for loops
- **Error Handling**: `std::expected` instead of error codes, `std::optional` for nullable values
- **Enums**: `magic_enum` for enum-to-string conversion
- **JSON**: `nlohmann/json` for all JSON processing
- **CLI**: `cxxopts` for command line argument parsing
- **Testing**: `Catch2` for unit tests and test-driven development
- **Constants**: Named constants (`constexpr`/`constinit`) instead of bare numbers (magic numbers)

### Forbidden Legacy Practices
- No `printf`, `sprintf`, `char*` manipulation, `malloc`/`free`, `NULL`, C-style casts, `#define` constants
- No manual memory management or C-style arrays
- No bare numbers/magic numbers - use named constants

### Terminology Changes from Legacy
- **IDs not VNUMs**: The modern codebase uses `EntityId` (composite of zone_id + local_id), NOT legacy "vnums". Never use the term "vnum" in modern code - use "ID" or "entity ID" instead.
- **Actors not Characters**: Use `Actor` for living beings, not `CharData` or "character"
- **Objects not Items**: Use `Object` class, not `ObjData`
- **Rooms**: Use `Room` class, not `RoomData`

### Code Cleanup Requirements
- Remove unused `#include` headers from legacy code
- Replace magic numbers with named constants
- Clean up commented-out code and obsolete functions

### Data-Driven Design Principles
- **Database over hard-coding**: Store messages, descriptions, and configurable values in the database rather than hard-coding them in C++. This allows content to be edited via Muditor without recompiling.
- **Effect messages**: Spell effect descriptions (what you see when looking at someone affected) should come from the Effects table, not hard-coded strings.
- **Ability messages**: All ability/spell messages (cast, success, failure, wear-off) should come from the AbilityMessages table.
- **Room/mob/object descriptions**: All world content comes from database tables, never hard-coded.
- **Configuration**: Game constants and tunable values should be stored in configuration tables when they might need adjustment.
- **Template system**: Use `{actor}`, `{target}`, `{damage}` placeholders in database messages for dynamic substitution at runtime.

### Code Style Examples
```cpp
// ✅ Modern string handling
void send_message(std::string_view msg, std::span<const Player> recipients) {
    auto formatted = fmt::format("Message: {}", msg);
    for (const auto& player : recipients) {
        player.send(formatted);
    }
}

// ✅ Named constants instead of magic numbers
constexpr int MAX_PLAYERS_PER_ROOM = 50;
constexpr std::chrono::seconds SAVE_INTERVAL{300};

// ✅ Modern error handling  
auto parse_command(std::string_view input) -> std::expected<Command, ParseError> {
    if (input.empty()) return std::unexpected(ParseError::EmptyInput);
    // ... parsing logic
    return Command{cmd_name, args};
}

// ✅ Modern ranges
auto online_players = all_players 
    | std::views::filter(&Player::is_online)
    | std::views::transform(&Player::get_name);
```

### Required Libraries
- Standard: `<ranges>`, `<span>`, `<expected>`, `<string_view>`  
- External: `libfmt`, `nlohmann/json`, `magic_enum`, `cxxopts`, `spdlog`, `Catch2`

## Project Build System

This project uses CMake with Ninja generator for faster builds. Key commands:

### Building the MUD

**Build both servers**:
```bash
cmake -B build -G Ninja .
cmake --build build
```

**Build specific server**:
```bash
# Modern server only
cmake --build build --target fierymud

# Legacy server only  
cmake --build build --target fierymud_legacy
```

### Running Tests

**Quick Test Script (Recommended)**:
```bash
./run_tests.sh                  # Run recommended tests (unit + stable) [DEFAULT]
./run_tests.sh unit              # Run only unit tests
./run_tests.sh stable            # Run only stable integration tests
./run_tests.sh integration       # Run legacy integration tests (may segfault)
./run_tests.sh session           # Run only session tests
./run_tests.sh verbose           # Run recommended tests with verbose output
./run_tests.sh all               # Run all tests including legacy (not recommended)
```

**Manual CTest Commands**:
```bash
# Recommended for development (100% success rate)
ctest --test-dir build -L unit            # Unit tests
ctest --test-dir build -L stable          # Stable integration tests

# Legacy tests (may segfault - use with caution)
ctest --test-dir build -L integration     # Legacy integration tests 
ctest --test-dir build -L session         # Session tests
ctest --test-dir build                     # All tests (includes legacy)
```

**Direct Test Execution**:
```bash
# Build and run stable tests (recommended)
cmake --build build --target stable_tests unit_tests
./build/unit_tests                        # Unit tests (fast, reliable)
./build/stable_tests                      # Stable integration tests (reliable)

# Legacy test executable (may segfault)
cmake --build build --target tests
./build/tests "[unit]"                    # Unit tests only
./build/tests "[stable]"                  # Stable integration tests only
```

### Development Build
```bash
cmake -B build -G Ninja .
cmake --build build
```

### Running the MUD

**Modern Server (recommended)**:
```bash
# First time setup - copy default library files
cp -r lib.default lib

# Run the modern FieryMUD server
./build/fierymud
```

**Legacy Server**:
```bash
# First time setup - copy default library files  
cp -r lib.default lib

# Run the legacy FieryMUD server
./build/fierymud_legacy
```

## Architecture Overview

FieryMUD is a modern C++ MUD (Multi-User Dungeon) evolved from CircleMUD. Key architectural components:

### Core Systems (Modern)
- **Main Loop**: `src/main.cpp` - Entry point and initialization
- **Server Systems**: `src/server/` - Network, world, and persistence management
- **Command Processing**: `src/commands/` - Command dispatch and execution
- **World Management**: `src/world/` - World loading, zones, and room management
- **Game Logic**: `src/game/` - Player interactions and game systems

### Legacy Systems
- **Legacy Code**: All legacy code now lives in `legacy/src/` directory
- **Legacy Files**: Original CircleMUD-derived files like `legacy/src/comm.cpp`, `legacy/src/interpreter.cpp`, `legacy/src/db.cpp`, `legacy/src/handler.cpp`
- **Legacy Build**: Use `cmake --build build --target fierymud_legacy` to build legacy server

### Key Data Structures
- **Modern**: Core classes in `src/core/` (Actor, Object, Entity)
- **Legacy**: Traditional structs in `legacy/src/structs.hpp` (CharData, ObjData, RoomData, DescriptorData)

### Magic System
FieryMUD uses a **circle-based spell memorization system** (similar to D&D), NOT a mana system:
- Spellcasters memorize spells by circle (1st through 9th)
- Each class/level grants a number of spell slots per circle
- Once a spell is cast, it must be re-memorized
- **Focus** stat determines memorization speed
- **Meditate** skill improves focus, accelerating spell memorization
- There is no mana pool or mana regeneration in this game

### Game Systems
- **Modern Combat**: Turn-based combat in `src/core/combat.cpp`
- **Legacy Systems**: Classes, magic, skills in `legacy/src/` (class.cpp, warrior.cpp, spell_mem.cpp, fight.cpp, skills.cpp)
- **Legacy Scripting**: DG Scripts in `legacy/src/dg_*.cpp`

### OLC (Online Creation)
In-game content editing system (legacy):
- `legacy/src/medit.cpp` - Mobile editing
- `legacy/src/oedit.cpp` - Object editing  
- `legacy/src/redit.cpp` - Room editing
- `legacy/src/zedit.cpp` - Zone editing

### File Organization
- `src/` - Modern source code
- `legacy/src/` - Legacy source code (archived)
- `data/` or `data_test/` - Runtime data (player files, world data, configuration)
- `data/world/` or `data_test/world/` - Zone files in JSON format
- `data/players/` or `data_test/players/` - Player save files
- `scripts/` - Python utilities for data conversion
- `docs/WORLD_JSON_FORMAT.md` - JSON world format documentation

## Development Guidelines

### Code Style
- Modern C++23 features encouraged
- Uses STL containers (`std::string`, `std::vector`, etc.)
- Extensive use of `fmt` library for string formatting
- JSON for data serialization with `nlohmann/json`

### Testing
- Uses Catch2 testing framework
- Test files in `tests/` directory
- Run tests with `./build/tests`
- **IMPORTANT**: Changes should be tested using Catch2 - add appropriate unit tests for new functionality

### Common Development Tasks
- **Adding new commands**: Modern commands in `src/commands/`, legacy in `legacy/src/interpreter.cpp`
- **New spells**: Legacy system in `legacy/src/spells.cpp` and spell tables
- **New classes**: Follow legacy patterns in `legacy/src/class.cpp`, `legacy/src/warrior.cpp`, etc.
- **World content**: Use OLC system in-game or edit JSON files directly (see `docs/WORLD_JSON_FORMAT.md` for format)
- **Database changes**: Update save/load functions in relevant files

### Important Constants
- Modern constants in `src/core/` files
- Legacy constants in `legacy/src/constants.cpp`
- Legacy flags in `legacy/src/structs.hpp`
- Skill/spell definitions in respective legacy system files

## Configuration

### Required Setup
1. Copy `lib.default/` to `lib/` on first run
2. Edit `lib/etc/` files for game configuration
3. World data in `data/world/` or `data_test/world/` as JSON files (format documented in `docs/WORLD_JSON_FORMAT.md`)

### VSCode Debugging
```json
{
    "type": "lldb",
    "request": "launch", 
    "name": "Debug",
    "program": "${workspaceFolder}/build/fierymud",
    "args": [],
    "cwd": "${workspaceFolder}"
}
```

## Testing Guidelines

### Test Framework Migration Status ✅ **COMPLETED**

The FieryMUD test framework has been **successfully migrated** from legacy segfaulting tests to stable, reliable testing infrastructure:

**Current Status**:
- **Unit Tests**: ✅ 100% reliable (0.02s execution time)
- **Stable Integration Tests**: ✅ 100% reliable (0.03s execution time) 
- **Legacy Integration Tests**: ❌ Segfaulting (preserved for reference, not recommended)

**Key Achievements**:
- **Zero Segfaults**: Stable tests use `LightweightTestHarness` approach with synchronous execution
- **Fast Execution**: Total recommended test time under 0.1 seconds
- **100% Success Rate**: Unit + Stable tests consistently pass
- **Comprehensive Coverage**: Command system, sessions, multiplayer, combat, and core functionality

**Migration Approach**:
- **Before**: Problematic `TestHarness` with background threads causing race conditions and segfaults
- **After**: `LightweightTestHarness` with direct command system integration and isolated test state

**Recommended Usage**: Use `./run_tests.sh` (defaults to stable tests) for all development work.

### Manual Testing for Game Features
When implementing game features (especially clan system, commands, and user interactions):

1. **Build and Run**: Always build and run the MUD for manual testing of user-facing features
   ```bash
   cmake --build build
   ./build/fierymud
   ```

2. **Test User Scenarios**: Test both regular user and god-level access patterns
   - Regular clan members should be able to use basic clan commands
   - Gods should have access to all clan administrative commands
   - Test permission boundaries and error conditions

3. **Test Room-Based Features**: For features requiring specific room locations:
   - Verify room-based validation works correctly (clan bank/chest rooms)
   - Test VNUM/RNUM conversion issues if room numbers are involved
   - Test from both correct and incorrect room locations

4. **Interactive Testing**: The user can perform comprehensive testing of implemented features
   - User has access to god-level characters for testing administrative functions
   - User can test clan membership scenarios and permission systems
   - User can verify command abbreviation and fuzzy matching functionality

### Important Testing Notes
- **Automated Tests**: Run `./build/tests` for unit tests, but game features require manual testing
- **Integration Testing**: Manual testing is essential for command parsing, permission systems, and user interactions
- **User Feedback**: The user can provide immediate feedback on functionality, UI/UX, and edge cases that automated tests might miss
- Once we complete a milestone (Everything tested, all todos handled, etc) we should commit everything to the current branch.
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

### Code Cleanup Requirements
- Remove unused `#include` headers from legacy code
- Replace magic numbers with named constants
- Clean up commented-out code and obsolete functions

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
```bash
cmake -B build -G Ninja --install-prefix /opt/MUD/ .
cmake --build build --target install
```

### Running Tests
```bash
cmake --build build --target tests
./build/tests
```

### Development Build
```bash
cmake -B build -G Ninja .
cmake --build build
```

### Running the MUD
```bash
# First time setup - copy default library files
cp -r lib.default lib

# Run the MUD
./build/fierymud
```

## Architecture Overview

FieryMUD is a modern C++ MUD (Multi-User Dungeon) evolved from CircleMUD. Key architectural components:

### Core Systems
- **Main Loop**: `src/main.cpp` - Entry point and initialization
- **Networking**: `src/comm.cpp` - Socket handling, game loop, telnet protocol
- **Command Processing**: `src/interpreter.cpp` - Command dispatch and privilege system
- **Database**: `src/db.cpp` - World loading, player persistence, zone management
- **Object Management**: `src/handler.cpp` - Game object lifecycle and manipulation

### Key Data Structures (`src/structs.hpp`)
- `CharData` - Players and NPCs with abilities, equipment, effects
- `ObjData` - Game objects (weapons, armor, containers, etc.)
- `RoomData` - Locations with exits and contents
- `DescriptorData` - Network connection management

### Game Systems
- **Classes**: Warrior, Cleric, Sorcerer, Rogue in `src/class.cpp`, `src/warrior.cpp`, etc.
- **Magic System**: Circle-based spell memorization in `src/spell_mem.cpp`
- **Combat**: Turn-based combat in `src/fight.cpp`
- **Skills**: Comprehensive skill system in `src/skills.cpp`
- **Scripting**: DG Scripts for dynamic content (`src/dg_*.cpp`)

### OLC (Online Creation)
In-game content editing system:
- `src/medit.cpp` - Mobile editing
- `src/oedit.cpp` - Object editing  
- `src/redit.cpp` - Room editing
- `src/zedit.cpp` - Zone editing

### File Organization
- `src/` - All source code
- `lib/` - Runtime data (player files, world data, configuration)
- `lib/world/` - Zone files in JSON format
- `lib/players/` - Player save files
- `scripts/` - Python utilities for data conversion

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
- **Adding new commands**: Add to `src/interpreter.cpp` command table
- **New spells**: Add to `src/spells.cpp` and spell tables
- **New classes**: Follow pattern of existing class files
- **World content**: Use OLC system in-game or edit JSON files directly
- **Database changes**: Update save/load functions in relevant files

### Important Constants
- Game constants in `src/constants.cpp`
- Magic numbers and flags in `src/structs.hpp`
- Skill/spell definitions in respective system files

## Configuration

### Required Setup
1. Copy `lib.default/` to `lib/` on first run
2. Edit `lib/etc/` files for game configuration
3. World data in `lib/world/` as JSON files

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
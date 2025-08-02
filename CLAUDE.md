# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

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

## Dependencies
- fmt - String formatting
- nlohmann/json - JSON processing
- magic_enum - Enum utilities
- cxxopts - Command line parsing
- Catch2 - Testing framework

The codebase represents a mature, well-structured MUD with modern C++ practices while maintaining compatibility with traditional MUD conventions and file formats.
# FieryMUD Project Overview

## Purpose
FieryMUD is a modern C++ Multi-User Dungeon (MUD) evolved from CircleMUD. It aims to create a challenging MUD for advanced players, offering complex gameplay that 3D worlds cannot provide. The project is actively developed and playable at telnet://fierymud.org:4000.

## Tech Stack
- **Language**: C++23 (modern standard with extensive STL usage)
- **Build System**: CMake with Ninja generator 
- **Testing Framework**: Catch2
- **Key Libraries**:
  - fmt - String formatting
  - nlohmann/json - JSON processing  
  - magic_enum - Enum utilities
  - cxxopts - Command line parsing
- **Development**: VSCode debugging support with LLDB

## Architecture
- **Core Systems**: Networking (comm.cpp), command processing (interpreter.cpp), database (db.cpp), object management (handler.cpp)
- **Game Systems**: Classes, magic system, combat, skills, scripting (DG Scripts)
- **Data Structures**: CharData (players/NPCs), ObjData (objects), RoomData (locations), DescriptorData (network connections)
- **Content Creation**: OLC (Online Creation) system for in-game editing

## File Structure  
- `src/` - All source code (main implementation)
- `lib/` - Runtime data (player files, world data, configuration)
- `tests/` - Catch2 unit tests
- `scripts/` - Python utilities for data conversion
- `cmake/` - Build system configuration

## Recent Development Focus
Currently undergoing major clan system improvements with modern C++23 patterns, permission decorators, and enhanced maintainability.
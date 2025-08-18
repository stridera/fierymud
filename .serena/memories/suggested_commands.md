# Suggested Commands for FieryMUD Development

## Essential Development Commands

### Building
```bash
# Quick development build
cmake -B build -G Ninja . && cmake --build build

# Full build with install
cmake -B build -G Ninja --install-prefix /opt/MUD/ . && cmake --build build --target install
```

### Testing
```bash
# Build and run all tests
cmake --build build --target tests && ./build/tests

# Run specific test
./build/tests --test-name "Basic Clan Test"
```

### Running the MUD
```bash
# First time setup
cp -r lib.default lib

# Run the MUD server
./build/fierymud
```

### Code Quality
```bash
# Format code (automatic with .clang-format)
clang-format -i src/*.cpp src/*.hpp

# Check compilation warnings
cmake --build build 2>&1 | grep -i warning
```

### Development Workflow
```bash
# Clean build from scratch
rm -rf build && cmake -B build -G Ninja . && cmake --build build

# Quick test cycle
cmake --build build && ./build/tests
```

## System Commands (Linux/WSL)
- `ls` - List files
- `grep` - Text search (prefer rg/ripgrep if available)
- `find` - File search
- `git` - Version control
- `cd` - Change directory

## MUD-Specific Commands
- Check `src/interpreter.cpp` for in-game commands
- OLC system commands: `medit`, `oedit`, `redit`, `zedit`
- Clan commands: `clan`, various subcommands
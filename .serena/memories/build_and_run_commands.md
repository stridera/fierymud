# Build and Run Commands

## Building the MUD
```bash
# Development build
cmake -B build -G Ninja .
cmake --build build

# Production build with install
cmake -B build -G Ninja --install-prefix /opt/MUD/ .
cmake --build build --target install
```

## Testing
```bash
# Build and run tests
cmake --build build --target tests
./build/tests
```

## Running the MUD
```bash
# First time setup - copy default library files
cp -r lib.default lib

# Run the MUD
./build/fierymud
```

## Development Tools
- **CMake**: Uses Ninja generator for faster builds
- **Catch2**: Unit testing framework  
- **clang-format**: Code formatting with LLVM style (120 char limit)
- **VSCode**: Debugging support with LLDB configuration

## Key Directories
- `lib.default/` → `lib/` (runtime data, copy on first run)
- `build/` - Build output directory
- `src/` - All source code
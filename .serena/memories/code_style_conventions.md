# Code Style and Conventions

## C++ Style (LLVM-based)
- **Standard**: C++23 with modern features encouraged
- **Line Limit**: 120 characters
- **Indentation**: 4 spaces, no tabs
- **Braces**: Attach style (opening brace on same line)
- **Pointers**: Right-aligned (`int *ptr`)
- **Access Modifiers**: -2 offset from class indentation

## Modern C++ Patterns
- **STL Containers**: `std::string`, `std::vector`, `std::unordered_map`
- **Smart Pointers**: `std::shared_ptr`, `std::weak_ptr`, `std::unique_ptr` 
- **Error Handling**: `std::expected` for modern error handling
- **String Formatting**: `fmt` library extensively used
- **JSON**: `nlohmann/json` for serialization
- **Enums**: `magic_enum` for enum utilities

## Naming Conventions
- **Classes**: PascalCase (`CharData`, `ClanRank`)
- **Functions**: snake_case (`has_permission`, `get_clan_member`)
- **Variables**: snake_case with descriptive names
- **Constants**: UPPER_SNAKE_CASE
- **Member Variables**: trailing underscore (`name_`, `id_`)

## Code Organization
- **Headers**: `.hpp` extension
- **Includes**: System headers first, then local headers
- **Namespaces**: Used for logical grouping (e.g., `clan_permissions`)
- **Forward Declarations**: Used extensively to reduce compilation dependencies
- **Comments**: Minimal, code should be self-documenting

## Error Handling
- **Modern**: `std::expected` for error returns
- **Defensive**: Null checks and bounds validation
- **Explicit**: Fail fast with meaningful error messages
- **Context**: Preserve error context for debugging
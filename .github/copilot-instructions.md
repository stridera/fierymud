## Code Guidelines

This file provides guidance when working with code in this repository.

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

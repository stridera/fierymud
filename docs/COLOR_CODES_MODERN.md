# FieryMUD Modern Color System Reference

Complete reference for the modern RichText color and formatting system in FieryMUD.

**Location**: `src/commands/rich_text.{hpp,cpp}`, `src/commands/command_context.{hpp,cpp}`, `src/commands/terminal_capabilities.{hpp,cpp}`

---

## Table of Contents

1. [Overview](#overview)
2. [Standard Colors](#standard-colors)
3. [Extended Color Support](#extended-color-support)
4. [Text Styles](#text-styles)
5. [Predefined Semantic Colors](#predefined-semantic-colors)
6. [RichText Builder API](#richtext-builder-api)
7. [Helper Functions](#helper-functions)
8. [CommandContext Methods](#commandcontext-methods)
9. [MessageType Color Mappings](#messagetype-color-mappings)
10. [Terminal Capability Detection](#terminal-capability-detection)
11. [Usage Examples](#usage-examples)
12. [Best Practices](#best-practices)

---

## Overview

The modern color system uses a **type-safe builder pattern** with comprehensive ANSI escape code support:

- **16 standard colors** + **16 bright variants**
- **256-color palette** support
- **24-bit RGB/TrueColor** support
- **Text styling**: Bold, Italic, Underline, Strikethrough, etc.
- **Background colors**
- **Terminal capability detection** with automatic fallback
- **Semantic color names** for game elements
- **Helper functions** for common UI patterns

---

## Standard Colors

### Foreground Colors (enum Color)

**Basic Colors (30-37)**:

| Color | Value | ANSI Code | Visual |
|-------|-------|-----------|--------|
| `Color::Black` | 30 | `\033[30m` | Black |
| `Color::Red` | 31 | `\033[31m` | Red |
| `Color::Green` | 32 | `\033[32m` | Green |
| `Color::Yellow` | 33 | `\033[33m` | Yellow |
| `Color::Blue` | 34 | `\033[34m` | Blue |
| `Color::Magenta` | 35 | `\033[35m` | Magenta |
| `Color::Cyan` | 36 | `\033[36m` | Cyan |
| `Color::White` | 37 | `\033[37m` | White/Light Gray |

**Bright Colors (90-97)**:

| Color | Value | ANSI Code | Visual |
|-------|-------|-----------|--------|
| `Color::BrightBlack` | 90 | `\033[90m` | Dark Gray |
| `Color::BrightRed` | 91 | `\033[91m` | Bright Red |
| `Color::BrightGreen` | 92 | `\033[92m` | Bright Green |
| `Color::BrightYellow` | 93 | `\033[93m` | Bright Yellow |
| `Color::BrightBlue` | 94 | `\033[94m` | Bright Blue |
| `Color::BrightMagenta` | 95 | `\033[95m` | Bright Magenta |
| `Color::BrightCyan` | 96 | `\033[96m` | Bright Cyan |
| `Color::BrightWhite` | 97 | `\033[97m` | Bright White |

**Special Colors**:

| Color | Value | ANSI Code | Usage |
|-------|-------|-----------|-------|
| `Color::Reset` | 0 | `\033[0m` | Reset all formatting |
| `Color::Default` | 39 | `\033[39m` | Default terminal foreground |

### Background Colors (enum BackgroundColor)

**Basic Backgrounds (40-47)**:

| Color | Value | ANSI Code |
|-------|-------|-----------|
| `BackgroundColor::Black` | 40 | `\033[40m` |
| `BackgroundColor::Red` | 41 | `\033[41m` |
| `BackgroundColor::Green` | 42 | `\033[42m` |
| `BackgroundColor::Yellow` | 43 | `\033[43m` |
| `BackgroundColor::Blue` | 44 | `\033[44m` |
| `BackgroundColor::Magenta` | 45 | `\033[45m` |
| `BackgroundColor::Cyan` | 46 | `\033[46m` |
| `BackgroundColor::White` | 47 | `\033[47m` |

**Bright Backgrounds (100-107)**:

| Color | Value | ANSI Code |
|-------|-------|-----------|
| `BackgroundColor::BrightBlack` | 100 | `\033[100m` |
| `BackgroundColor::BrightRed` | 101 | `\033[101m` |
| `BackgroundColor::BrightGreen` | 102 | `\033[102m` |
| `BackgroundColor::BrightYellow` | 103 | `\033[103m` |
| `BackgroundColor::BrightBlue` | 104 | `\033[104m` |
| `BackgroundColor::BrightMagenta` | 105 | `\033[105m` |
| `BackgroundColor::BrightCyan` | 106 | `\033[106m` |
| `BackgroundColor::BrightWhite` | 107 | `\033[107m` |

**Special**:

| Color | Value | ANSI Code |
|-------|-------|-----------|
| `BackgroundColor::Default` | 49 | `\033[49m` |

---

## Extended Color Support

### 256-Color Palette (Color256)

Support for extended 256-color palette (not all terminals support this).

```cpp
Color256(int value);  // value: 0-255
```

**Color Ranges**:
- **0-7**: Standard ANSI colors (same as basic 16)
- **8-15**: Bright ANSI colors
- **16-231**: 216 colors in 6×6×6 RGB cube
- **232-255**: 24 grayscale shades

**Methods**:
```cpp
Color256 color(208);  // Orange-ish
color.to_foreground();  // Returns "\033[38;5;208m"
color.to_background();  // Returns "\033[48;5;208m"
```

**Usage**:
```cpp
RichText msg;
msg.color256("Level 50!", Color256(226));  // Bright yellow
```

### RGB/TrueColor (ColorRGB)

Support for 24-bit RGB colors (16.7 million colors - modern terminals only).

```cpp
ColorRGB(int r, int g, int b);  // r, g, b: 0-255 each
```

**Methods**:
```cpp
ColorRGB color(255, 128, 64);  // Orange
color.to_foreground();  // Returns "\033[38;2;255;128;64m"
color.to_background();  // Returns "\033[48;2;255;128;64m"
```

**Usage**:
```cpp
RichText msg;
msg.rgb("Critical Hit!", ColorRGB(255, 20, 147));  // Deep pink
```

---

## Text Styles

### Available Styles (enum TextStyle)

| Style | Value | ANSI Code | Reset Code | Description |
|-------|-------|-----------|------------|-------------|
| `TextStyle::Normal` | 0 | `\033[0m` | - | Reset all formatting |
| `TextStyle::Bold` | 1 | `\033[1m` | `\033[22m` | Bold/bright text |
| `TextStyle::Dim` | 2 | `\033[2m` | `\033[22m` | Dimmed text |
| `TextStyle::Italic` | 3 | `\033[3m` | `\033[23m` | Italic (limited support) |
| `TextStyle::Underline` | 4 | `\033[4m` | `\033[24m` | Underlined text |
| `TextStyle::Blink` | 5 | `\033[5m` | `\033[25m` | Blinking text |
| `TextStyle::Reverse` | 7 | `\033[7m` | `\033[27m` | Reverse video (swap fg/bg) |
| `TextStyle::Strikethrough` | 9 | `\033[9m` | `\033[29m` | Strikethrough text |

**Reset Styles**:
```cpp
TextStyle::NoBold          // Same as NoDim (22)
TextStyle::NoDim           // 22
TextStyle::NoItalic        // 23
TextStyle::NoUnderline     // 24
TextStyle::NoBlink         // 25
TextStyle::NoReverse       // 27
TextStyle::NoStrikethrough // 29
```

---

## Predefined Semantic Colors

Located in `namespace Colors` (src/commands/rich_text.hpp:212-248).

### Game Status Colors

```cpp
Colors::Health      // RGB(220, 20, 60)   - Crimson red for HP
Colors::Mana        // RGB(30, 144, 255)  - Dodger blue for mana
Colors::Movement    // RGB(50, 205, 50)   - Lime green for stamina
Colors::Experience  // RGB(255, 215, 0)   - Gold for XP
```

### Combat Colors

```cpp
Colors::Damage      // RGB(255, 69, 0)    - Orange-red for damage
Colors::Healing     // RGB(50, 205, 50)   - Lime green for healing
Colors::Miss        // RGB(128, 128, 128) - Gray for misses
Colors::Critical    // RGB(255, 20, 147)  - Deep pink for crits
```

### Object Quality Colors

```cpp
Colors::Common      // RGB(192, 192, 192) - Silver
Colors::Uncommon    // RGB(30, 255, 0)    - Lime
Colors::Rare        // RGB(0, 112, 221)   - Blue
Colors::Epic        // RGB(163, 53, 238)  - Purple
Colors::Legendary   // RGB(255, 128, 0)   - Orange
```

### UI Element Colors

```cpp
Colors::Border      // Color::BrightBlack   - UI borders
Colors::Header      // Color::BrightWhite   - Headers
Colors::Accent      // Color::BrightCyan    - Accent elements
Colors::Success     // Color::BrightGreen   - Success messages
Colors::Warning     // Color::BrightYellow  - Warnings
Colors::Error       // Color::BrightRed     - Errors
Colors::Info        // Color::BrightBlue    - Info messages
```

### Environment Colors

```cpp
Colors::Grass       // RGB(34, 139, 34)   - Forest green
Colors::Water       // RGB(0, 191, 255)   - Deep sky blue
Colors::Fire        // RGB(255, 69, 0)    - Orange-red
Colors::Stone       // RGB(105, 105, 105) - Dim gray
Colors::Wood        // RGB(139, 69, 19)   - Saddle brown
Colors::Metal       // RGB(192, 192, 192) - Silver
```

---

## RichText Builder API

### Construction

```cpp
RichText();                                          // Empty
RichText(std::string_view text);                    // With initial text
RichText(const TerminalCapabilities::Capabilities& caps);  // With capabilities
RichText(std::string_view text, const Capabilities& caps); // Both
```

### Text Methods

```cpp
RichText& text(std::string_view str);               // Append plain text
```

### Color Methods

```cpp
RichText& colored(std::string_view str, Color color);
RichText& rgb(std::string_view str, const ColorRGB& color);
RichText& color256(std::string_view str, const Color256& color);
```

### Style Methods

```cpp
RichText& bold(std::string_view str);
RichText& italic(std::string_view str);
RichText& underline(std::string_view str);
RichText& strikethrough(std::string_view str);
```

### Combined Formatting

```cpp
RichText& highlight(std::string_view str, Color fg, BackgroundColor bg);
RichText& formatted(std::string_view str, const TextFormat& format);
```

### Special Formatting

```cpp
// Progress bar (adaptive: Unicode █░ or ASCII #-)
RichText& progress_bar(float percentage, int width = 20);
RichText& progress_bar(float percentage, int width,
                      std::string_view filled, std::string_view empty);

// Table row
RichText& table_row(const std::vector<std::string>& columns,
                   const std::vector<int>& widths = {});

// Horizontal separator
RichText& separator(char ch = '-', int width = 60,
                   Color color = Color::BrightBlack);

// Code block with syntax highlighting hint
RichText& code_block(std::string_view code, std::string_view language = "");
```

### Output Methods

```cpp
std::string to_ansi() const;        // Generate ANSI-formatted string
std::string to_plain() const;       // Strip all formatting
size_t plain_length() const;        // Character count (no formatting)
bool empty() const;                 // Check if empty
void clear();                       // Clear all content
```

### Capability Management

```cpp
void set_capabilities(const TerminalCapabilities::Capabilities& caps);
```

---

## Helper Functions

Located in `namespace Format` (src/commands/rich_text.hpp:251-281).

### Health/Status Displays

```cpp
RichText health_bar(int current, int max, int width = 20);
// Creates colored progress bar: green (>60%), yellow (30-60%), red (<30%)
```

### Combat Messages

```cpp
RichText damage_text(int amount);
// Returns amount in Colors::Damage (orange-red)

RichText healing_text(int amount);
// Returns "+amount" in Colors::Healing (lime green)
```

### Object Display

```cpp
RichText object_name(std::string_view name, std::string_view quality = "common");
// Qualities: "common", "uncommon", "rare", "epic", "legendary"
```

### Tables

```cpp
RichText table(const std::vector<std::string>& headers,
              const std::vector<std::vector<std::string>>& rows);
// Auto-detects terminal capabilities

RichText table(const std::vector<std::string>& headers,
              const std::vector<std::vector<std::string>>& rows,
              const TerminalCapabilities::Capabilities& caps);
// Uses specified capabilities
```

### Command Help

```cpp
RichText command_syntax(std::string_view command, std::string_view syntax);
// Format: "Usage: <command> <syntax>"
```

### Messages

```cpp
RichText error_message(std::string_view message);
// Prefixes with red "Error: "

RichText success_message(std::string_view message);
// Prefixes with green "✓ "
```

---

## CommandContext Methods

Located in `src/commands/command_context.{hpp,cpp}`.

### Rich Text Methods

```cpp
void send_rich(const RichText& rich_text) const;
void send_colored(std::string_view message, Color color) const;
void send_progress_bar(std::string_view label, float percentage,
                      int width = 20) const;
void send_table(const std::vector<std::string>& headers,
               const std::vector<std::vector<std::string>>& rows) const;
```

### Semantic Message Methods

```cpp
void send(std::string_view message) const;           // Plain
void send_line(std::string_view message) const;      // Plain + newline
void send_error(std::string_view message) const;     // Red
void send_success(std::string_view message) const;   // Green
void send_info(std::string_view message) const;      // Blue
void send_usage(std::string_view usage) const;       // Error colored
```

---

## MessageType Color Mappings

Located in `src/commands/command_context.cpp:692-724`.

| MessageType | Color | ANSI Code | Usage |
|-------------|-------|-----------|-------|
| `MessageType::Normal` | None | - | Default text |
| `MessageType::Error` | Red | `\033[31m` | Error messages |
| `MessageType::Success` | Green | `\033[32m` | Success confirmations |
| `MessageType::Warning` | Yellow | `\033[33m` | Warning messages |
| `MessageType::Info` | Blue | `\033[34m` | Informational messages |
| `MessageType::System` | Cyan | `\033[36m` | System messages |
| `MessageType::Debug` | White | `\033[37m` | Debug output |
| `MessageType::Combat` | Magenta | `\033[35m` | Combat messages |
| `MessageType::Social` | Yellow | `\033[33m` | Social actions |
| `MessageType::Tell` | Cyan | `\033[36m` | Private tells |
| `MessageType::Say` | White | `\033[37m` | Speech |
| `MessageType::Emote` | Yellow | `\033[33m` | Emotes |
| `MessageType::Channel` | Magenta | `\033[35m` | Channel communication |
| `MessageType::Broadcast` | Red | `\033[31m` | System broadcasts |

**Usage**:
```cpp
auto formatted = CommandContextUtils::format_message(message, MessageType::Error);
// Returns: "\033[31m{message}\033[0m"
```

---

## Terminal Capability Detection

Located in `src/commands/terminal_capabilities.{hpp,cpp}`.

### Support Levels (enum SupportLevel)

| Level | Features |
|-------|----------|
| `SupportLevel::None` | No color support (plain text) |
| `SupportLevel::Basic` | Basic ANSI (16 colors only) |
| `SupportLevel::Standard` | 16 colors + basic formatting |
| `SupportLevel::Extended` | 256 colors + advanced formatting |
| `SupportLevel::Full` | TrueColor (24-bit) + Unicode + all features |

### Detection Methods (enum DetectionMethod)

| Method | Description |
|--------|-------------|
| `DetectionMethod::Environment` | Server-side environment variables |
| `DetectionMethod::MTTS` | Mud Terminal Type Standard |
| `DetectionMethod::GMCP` | Generic Mud Communication Protocol |
| `DetectionMethod::NewEnviron` | NEW-ENVIRON telnet protocol |

### Capabilities Structure

```cpp
struct Capabilities {
    // Display support
    bool supports_color = false;
    bool supports_256_color = false;
    bool supports_true_color = false;
    bool supports_unicode = false;
    bool supports_bold = false;
    bool supports_italic = false;
    bool supports_underline = false;

    // Client information
    std::string terminal_name = "unknown";
    std::string client_name = "unknown";
    std::string client_version = "unknown";

    // Advanced capabilities
    bool supports_gmcp = false;
    bool supports_screen_reader = false;
    bool supports_tls = false;
    bool supports_mouse = false;
    bool supports_hyperlinks = false;

    // Source information
    DetectionMethod detection_method = DetectionMethod::Environment;
    SupportLevel overall_level = SupportLevel::None;
    uint32_t mtts_bitvector = 0;
};
```

### MTTS Bitvector Constants

```cpp
namespace MTTS {
    constexpr uint32_t ANSI = 1;               // ANSI color codes
    constexpr uint32_t VT100 = 2;              // VT100 sequences
    constexpr uint32_t UTF8 = 4;               // UTF-8 encoding
    constexpr uint32_t COLOR_256 = 8;          // 256 color support
    constexpr uint32_t MOUSE = 16;             // Mouse tracking
    constexpr uint32_t OSC_COLOR_PALETTE = 32; // OSC color palette
    constexpr uint32_t SCREEN_READER = 64;     // Screen reader
    constexpr uint32_t PROXY = 128;            // Behind proxy
    constexpr uint32_t TRUECOLOR = 256;        // 24-bit truecolor
    constexpr uint32_t MNES = 512;             // Mud New Env Standard
    constexpr uint32_t TLS = 1024;             // TLS encryption
}
```

### Detection Functions

```cpp
Capabilities detect_capabilities_from_mtts(uint32_t bitvector,
                                          std::string_view terminal_type = "");

Capabilities detect_capabilities_from_gmcp(const nlohmann::json& client_info);

Capabilities detect_capabilities_from_new_environ(
    const std::unordered_map<std::string, std::string>& env_vars);

Capabilities detect_capabilities();  // Default/fallback

Capabilities get_capabilities_for_terminal(std::string_view terminal_name);
```

### Known Terminal Capabilities

**Full Support (SupportLevel::Full)**:
- Mudlet, Alacritty, Kitty, WezTerm, iTerm2

**Extended Support (SupportLevel::Extended)**:
- MUSHclient, TinTin++, xterm-256, screen-256, tmux-256, gnome-terminal, konsole

**Standard Support (SupportLevel::Standard)**:
- Basic xterm, rxvt, screen, tmux

**Basic Support (SupportLevel::Basic)**:
- Linux console

### Helper Functions

```cpp
ProgressChars get_progress_chars(const Capabilities& caps);
// Returns: {filled: "█", empty: "░"} or {filled: "#", empty: "-"}

TableChars get_table_chars(const Capabilities& caps);
// Returns Unicode box drawing or ASCII equivalents
```

---

## Usage Examples

### Basic Colored Text

```cpp
RichText msg;
msg.colored("Error: ", Color::BrightRed)
   .text("Invalid command!");
ctx.send_rich(msg);
```

### RGB Colors

```cpp
RichText health;
health.text("Health: ")
      .rgb("1250", Colors::Health)
      .text("/1500");
ctx.send_rich(health);
```

### Text Styling

```cpp
RichText title;
title.bold("=== FIERYMUD ===");
ctx.send_rich(title);

RichText emphasis;
emphasis.text("This is ")
        .underline("very important")
        .text(" information!");
ctx.send_rich(emphasis);
```

### Progress Bars

```cpp
// Adaptive progress bar
ctx.send_progress_bar("Loading", 0.75f, 30);

// Custom with RichText
RichText hp_bar;
hp_bar.text("HP: ")
      .progress_bar(0.85f, 20);  // 85% health, 20 chars
ctx.send_rich(hp_bar);
```

### Tables

```cpp
std::vector<std::string> headers = {"Name", "Level", "Class"};
std::vector<std::vector<std::string>> rows = {
    {"Gandalf", "50", "Wizard"},
    {"Aragorn", "40", "Ranger"}
};

ctx.send_table(headers, rows);
// Or with RichText
auto table = Format::table(headers, rows);
ctx.send_rich(table);
```

### Helper Functions

```cpp
// Health bar with auto-coloring
auto hp_bar = Format::health_bar(850, 1000, 20);
ctx.send_rich(hp_bar);

// Damage/healing
auto dmg = Format::damage_text(47);
auto heal = Format::healing_text(123);

// Quality-colored items
auto item = Format::object_name("Excalibur", "legendary");
ctx.send_rich(item);

// Command help
auto help = Format::command_syntax("cast", "<spell> [target]");
ctx.send_rich(help);

// Messages
auto err = Format::error_message("File not found!");
auto success = Format::success_message("Quest completed!");
```

### Adaptive Output Based on Capabilities

```cpp
auto caps = TerminalCapabilities::detect_capabilities();
RichText msg(caps);  // Set capabilities

if (caps.supports_true_color) {
    msg.rgb("Advanced Graphics", ColorRGB{255, 128, 64});
} else if (caps.supports_256_color) {
    msg.color256("256 Colors", Color256{208});
} else if (caps.supports_color) {
    msg.colored("Basic Colors", Color::BrightYellow);
} else {
    msg.text("Plain Text");
}

ctx.send_rich(msg);
```

### Combined Formatting

```cpp
RichText highlight;
highlight.highlight("IMPORTANT", Color::BrightYellow, BackgroundColor::Red);
ctx.send_rich(highlight);

// Custom format
TextFormat fmt;
fmt.foreground = Color::BrightCyan;
fmt.background = BackgroundColor::Blue;
fmt.styles.push_back(TextStyle::Bold);
fmt.styles.push_back(TextStyle::Underline);

RichText custom;
custom.formatted("Special Text", fmt);
ctx.send_rich(custom);
```

### CommandContext Shortcuts

```cpp
// Semantic messages (auto-colored)
ctx.send_error("Invalid target!");        // Red
ctx.send_success("Character saved!");     // Green
ctx.send_info("Type 'help' for info.");   // Blue

// Direct colored output
ctx.send_colored("Warning!", Color::BrightYellow);

// Progress bar
ctx.send_progress_bar("Casting", 0.65f, 25);
```

---

## Best Practices

### 1. Use Semantic MessageTypes

```cpp
// Good
ctx.send_error("Command failed!");
ctx.send_success("Quest completed!");

// Avoid
RichText msg;
msg.colored("Command failed!", Color::Red);
ctx.send_rich(msg);
```

### 2. Use Predefined Semantic Colors

```cpp
// Good
msg.rgb("Critical Hit!", Colors::Critical);

// Avoid
msg.rgb("Critical Hit!", ColorRGB{255, 20, 147});
```

### 3. Always Reset Colors

```cpp
// RichText automatically resets after each colored segment
msg.colored("Red text", Color::Red)
   .text("Normal text");  // Automatically normal

// Manual ANSI: always reset
std::string manual = "\033[31mRed\033[0m Normal";
```

### 4. Detect Capabilities for Adaptive Output

```cpp
auto caps = TerminalCapabilities::detect_capabilities();
RichText msg(caps);  // Automatically uses appropriate features

// Progress bars automatically adapt
msg.progress_bar(0.75f, 20);  // Uses █░ or #- based on caps
```

### 5. Don't Rely Solely on Color

```cpp
// Bad (color-only differentiation)
msg.colored("Success", Color::Green);

// Good (color + text/symbols)
msg.colored("✓ Success", Color::Green);
msg.colored("[OK] Success", Color::Green);
```

### 6. Test with Different Terminal Types

Test your output with:
- Basic terminal (no Unicode, 16 colors only)
- Modern terminal (TrueColor, Unicode)
- Screen reader friendly mode

### 7. Use Helper Functions

```cpp
// Good (consistent, tested)
auto table = Format::table(headers, rows);

// Avoid (reinventing the wheel)
RichText custom_table;
// ... manual table building ...
```

### 8. Keep Accessibility in Mind

```cpp
// Provide text alternatives
msg.text("Health: ");
msg.progress_bar(0.50f, 20);
msg.text(" 500/1000 (50%)");  // Numeric alternative

// Not just: progress_bar(0.50f, 20);
```

---

## Quick Reference

### Most Common Colors

```cpp
Color::BrightRed        // Errors
Color::BrightGreen      // Success
Color::BrightYellow     // Warnings
Color::BrightCyan       // Info/highlights
Color::BrightWhite      // Headers/emphasis
Color::White            // Normal text
Color::BrightBlack      // Dimmed/borders
```

### Common Patterns

```cpp
// Error message
ctx.send_error("message");
// or
auto err = Format::error_message("message");

// Success message
ctx.send_success("message");
// or
auto msg = Format::success_message("message");

// Colored text
msg.colored("text", Color::BrightYellow);

// RGB text
msg.rgb("text", Colors::Health);

// Bold text
msg.bold("text");

// Progress bar
msg.progress_bar(0.75f, 20);

// Table
auto tbl = Format::table(headers, rows);
```

---

## ANSI Escape Sequence Quick Reference

### Basic Format

```
\033[{code}m        - Single code
\033[{code};{code}m - Multiple codes
```

### Common Codes

```cpp
0   - Reset all
1   - Bold
4   - Underline
30-37   - Foreground colors
40-47   - Background colors
90-97   - Bright foreground
100-107 - Bright background

38;5;{n}       - 256-color foreground (n = 0-255)
48;5;{n}       - 256-color background (n = 0-255)
38;2;{r};{g};{b} - RGB foreground
48;2;{r};{g};{b} - RGB background
```

---

*Document Version: 1.0*
*Last Updated: 2025-12-10*
*For legacy color codes, see: COLOR_CODES_LEGACY.md*

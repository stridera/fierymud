# FieryMUD Legacy Color System Reference

Reference documentation for the legacy inline color code system used in older FieryMUD code.

**Location**: `legacy/src/screen.{hpp,cpp}`

**Status**: Legacy/deprecated - use modern RichText API for new code (see COLOR_CODES_MODERN.md)

---

## Table of Contents

1. [Overview](#overview)
2. [Color Code Characters](#color-code-characters)
3. [Relative Color Codes](#relative-color-codes)
4. [Absolute Color Codes](#absolute-color-codes)
5. [ANSI Code Macros](#ansi-code-macros)
6. [Processing Functions](#processing-functions)
7. [Character Preference Macros](#character-preference-macros)
8. [Usage Examples](#usage-examples)
9. [Migration to Modern API](#migration-to-modern-api)

---

## Overview

The legacy color system uses **inline color codes** embedded directly in strings:

- **Relative codes** (`&` prefix): Modify current formatting without reset
- **Absolute codes** (`@` prefix): Reset all formatting, then apply color

**Example**:
```cpp
"&1Warning:&0 Low health!"           // Relative: red then reset
"@Y== @WFIERY@RMUD @Y=="              // Absolute: yellow, white, red, yellow
```

These codes are **processed at output time** by `process_colors()` function.

---

## Color Code Characters

### Prefix Characters

| Character | Macro | Value | Description |
|-----------|-------|-------|-------------|
| `&` | `CREL` / `AREL` | `'&'` / `"&"` | Relative color change |
| `@` | `CABS` / `AABS` | `'@'` / `"@"` | Absolute color change |

### Processing Modes

| Mode | Value | Description |
|------|-------|-------------|
| `CLR_PARSE` | 0 | Convert codes to ANSI escape sequences |
| `CLR_ESCAPE` | 1 | Escape color codes (double prefix) |
| `CLR_STRIP` | 2 | Remove all color codes |

---

## Relative Color Codes

**Prefix**: `&`

Relative codes **preserve current formatting** and add/change colors on top.

**Source**: `legacy/src/screen.cpp:25-28`

### Foreground Colors

| Code | Color | ANSI Macro | ANSI Code | Description |
|------|-------|------------|-----------|-------------|
| `&0` | Reset | `ANRM` | `\033[0m` | Reset to normal |
| `&1` | Red | `FRED` | `\033[31m` | Foreground red |
| `&2` | Green | `FGRN` | `\033[32m` | Foreground green |
| `&3` | Yellow | `FYEL` | `\033[33m` | Foreground yellow |
| `&4` | Blue | `FBLU` | `\033[34m` | Foreground blue |
| `&5` | Magenta | `FMAG` | `\033[35m` | Foreground magenta |
| `&6` | Cyan | `FCYN` | `\033[36m` | Foreground cyan |
| `&7` | White | `FWHT` | `\033[37m` | Foreground white/gray |
| `&9` | Black | `FBLK` | `\033[30m` | Foreground black |

### Background Colors

| Code | Color | ANSI Macro | ANSI Code | Description |
|------|-------|------------|-----------|-------------|
| `&R` | Red | `BRED` | `\033[41m` | Background red |
| `&G` | Green | `BGRN` | `\033[42m` | Background green |
| `&Y` | Yellow | `BYEL` | `\033[43m` | Background yellow |
| `&B` | Blue | `BBLU` | `\033[44m` | Background blue |
| `&M` | Magenta | `BMAG` | `\033[45m` | Background magenta |
| `&C` | Cyan | `BCYN` | `\033[46m` | Background cyan |
| `&W` | White | `BWHT` | `\033[47m` | Background white |
| `&L` | Black | `BBLK` | `\033[40m` | Background black |
| `&K` | Light Black | `BLBK` | `\033[100m` | Background light black |

### Text Styles

| Code | Style | ANSI Macro | ANSI Code | Description |
|------|-------|------------|-----------|-------------|
| `&u` | Underline | `AUND` | `\033[4m` | Underline text |
| `&b` | Bold | `ABLD` | `\033[1m` | Bold/bright text |
| `&d` | Dim | `ADAR` | `\033[2m` | Dimmed text |

### Special Codes

| Code | Effect | Description |
|------|--------|-------------|
| `&&` | Literal `&` | Escape the & character |
| `&_` | Newline | Insert newline character (`\n`) |

---

## Absolute Color Codes

**Prefix**: `@`

Absolute codes **reset all formatting first**, then apply the color.

**Source**: `legacy/src/screen.cpp:30-34`

### Basic Foreground Colors (Lowercase)

| Code | Color | ANSI Macro | ANSI Code | Description |
|------|-------|------------|-----------|-------------|
| `@0` | Reset | `ANRM` | `\033[0m` | Reset to normal |
| `@1` | Red | `AFRED` | `\033[0;31m` | Absolute red |
| `@2` | Green | `AFGRN` | `\033[0;32m` | Absolute green |
| `@3` | Yellow | `AFYEL` | `\033[0;33m` | Absolute yellow |
| `@4` | Blue | `AFBLU` | `\033[0;34m` | Absolute blue |
| `@5` | Magenta | `AFMAG` | `\033[0;35m` | Absolute magenta |
| `@6` | Cyan | `AFCYN` | `\033[0;36m` | Absolute cyan |
| `@7` | White | `AFWHT` | `\033[0;37m` | Absolute white |
| `@9` | Black | `AFBLK` | `\033[0;30m` | Absolute black |

### Alternative Lowercase Colors

| Code | Color | ANSI Macro | ANSI Code |
|------|-------|------------|-----------|
| `@r` | Red | `AFRED` | `\033[0;31m` |
| `@g` | Green | `AFGRN` | `\033[0;32m` |
| `@y` | Yellow | `AFYEL` | `\033[0;33m` |
| `@b` | Blue | `AFBLU` | `\033[0;34m` |
| `@m` | Magenta | `AFMAG` | `\033[0;35m` |
| `@c` | Cyan | `AFCYN` | `\033[0;36m` |
| `@w` | White | `AFWHT` | `\033[0;37m` |
| `@d` | Black | `AFBLK` | `\033[0;30m` |

### Bright Colors (Uppercase)

| Code | Color | ANSI Macro | ANSI Code | Description |
|------|-------|------------|-----------|-------------|
| `@R` | Bright Red | `AHRED` | `\033[0;1;31m` | Absolute bright red |
| `@G` | Bright Green | `AHGRN` | `\033[0;1;32m` | Absolute bright green |
| `@Y` | Bright Yellow | `AHYEL` | `\033[0;1;33m` | Absolute bright yellow |
| `@B` | Bright Blue | `AHBLU` | `\033[0;1;34m` | Absolute bright blue |
| `@M` | Bright Magenta | `AHMAG` | `\033[0;1;35m` | Absolute bright magenta |
| `@C` | Bright Cyan | `AHCYN` | `\033[0;1;36m` | Absolute bright cyan |
| `@W` | Bright White | `AHWHT` | `\033[0;1;37m` | Absolute bright white |
| `@L` | Bright Black | `AHBLK` | `\033[0;1;30m` | Absolute bright black (dark gray) |

### Special Codes

| Code | Effect | Description |
|------|--------|-------------|
| `@@` | Literal `@` | Escape the @ character |

---

## ANSI Code Macros

**Source**: `legacy/src/screen.hpp`

These macros define the actual ANSI escape sequences used by the color system.

### General Control Codes

| Macro | Value | Description |
|-------|-------|-------------|
| `ANUL` | `""` | No effect (empty string) |
| `ANRM` | `\x1B[0m` | Reset to normal |
| `ABLD` | `\x1B[1m` | Bold/brighten colors |
| `ADAR` | `\x1B[2m` | Dim colors |
| `AEMP` | `\x1B[3m` | Italicize (not widely supported) |
| `AUND` | `\x1B[4m` | Underline |
| `AFSH` | `\x1B[5m` | Flashing text (slow blink) |
| `AFST` | `\x1B[6m` | Flashing text (fast blink) |
| `ARVS` | `\x1B[7m` | Reverse video |
| `AHID` | `\x1B[8m` | Hide text |
| `ADUN` | `\x1B[21m` | Double underline |
| `ANRI` | `\x1B[22m` | Normal intensity (not dim/bright) |
| `ANUN` | `\x1B[24m` | Turn off underline |
| `ANFS` | `\x1B[25m` | Turn off flashing |
| `APOS` | `\x1B[27m` | Positive image (normal) |
| `AUNH` | `\x1B[28m` | Unhide text |

### Special Control Codes

| Macro | Value | Description |
|-------|-------|-------------|
| `ACLR` | `\x1B[2J` | Clear screen |
| `AALM` | `\x07` | Alarm (beep) |

### Foreground Colors (Relative)

| Macro | Value | Description |
|-------|-------|-------------|
| `FBLK` | `\x1B[30m` | Foreground black |
| `FRED` | `\x1B[31m` | Foreground red |
| `FGRN` | `\x1B[32m` | Foreground green |
| `FYEL` | `\x1B[33m` | Foreground yellow |
| `FBLU` | `\x1B[34m` | Foreground blue |
| `FMAG` | `\x1B[35m` | Foreground magenta |
| `FCYN` | `\x1B[36m` | Foreground cyan |
| `FWHT` | `\x1B[37m` | Foreground white (light gray) |

### Foreground Bright Colors (Relative)

| Macro | Value | Description |
|-------|-------|-------------|
| `HBLK` | `\x1B[1;30m` | Foreground bright black (dark gray) |
| `HRED` | `\x1B[1;31m` | Foreground bright red |
| `HGRN` | `\x1B[1;32m` | Foreground bright green |
| `HYEL` | `\x1B[1;33m` | Foreground bright yellow |
| `HBLU` | `\x1B[1;34m` | Foreground bright blue |
| `HMAG` | `\x1B[1;35m` | Foreground bright magenta |
| `HCYN` | `\x1B[1;36m` | Foreground bright cyan |
| `HWHT` | `\x1B[1;37m` | Foreground bright white |

### Background Colors

| Macro | Value | Description |
|-------|-------|-------------|
| `BBLK` | `\x1B[40m` | Background black |
| `BRED` | `\x1B[41m` | Background red |
| `BGRN` | `\x1B[42m` | Background green |
| `BYEL` | `\x1B[43m` | Background yellow |
| `BBLU` | `\x1B[44m` | Background blue |
| `BMAG` | `\x1B[45m` | Background magenta |
| `BCYN` | `\x1B[46m` | Background cyan |
| `BWHT` | `\x1B[47m` | Background white |
| `BLBK` | `\x1B[100m` | Background light black |

### Absolute Foreground Colors

| Macro | Value | Description |
|-------|-------|-------------|
| `AFBLK` | `\x1B[0;30m` | Absolute foreground black |
| `AFRED` | `\x1B[0;31m` | Absolute foreground red |
| `AFGRN` | `\x1B[0;32m` | Absolute foreground green |
| `AFYEL` | `\x1B[0;33m` | Absolute foreground yellow |
| `AFBLU` | `\x1B[0;34m` | Absolute foreground blue |
| `AFMAG` | `\x1B[0;35m` | Absolute foreground magenta |
| `AFCYN` | `\x1B[0;36m` | Absolute foreground cyan |
| `AFWHT` | `\x1B[0;37m` | Absolute foreground white |

### Absolute Bright Foreground Colors

| Macro | Value | Description |
|-------|-------|-------------|
| `AHBLK` | `\x1B[0;1;30m` | Absolute bright black (dark gray) |
| `AHRED` | `\x1B[0;1;31m` | Absolute bright red |
| `AHGRN` | `\x1B[0;1;32m` | Absolute bright green |
| `AHYEL` | `\x1B[0;1;33m` | Absolute bright yellow |
| `AHBLU` | `\x1B[0;1;34m` | Absolute bright blue |
| `AHMAG` | `\x1B[0;1;35m` | Absolute bright magenta |
| `AHCYN` | `\x1B[0;1;36m` | Absolute bright cyan |
| `AHWHT` | `\x1B[0;1;37m` | Absolute bright white |

### Absolute Blinking Colors

| Macro | Value | Description |
|-------|-------|-------------|
| `ABBLK` | `\x1B[0;5;30m` | Blinking black |
| `ABRED` | `\x1B[0;5;31m` | Blinking red |
| `ABGRN` | `\x1B[0;5;32m` | Blinking green |
| `ABYEL` | `\x1B[0;5;33m` | Blinking yellow |
| `ABBLU` | `\x1B[0;5;34m` | Blinking blue |
| `ABMAG` | `\x1B[0;5;35m` | Blinking magenta |
| `ABCYN` | `\x1B[0;5;36m` | Blinking cyan |
| `ABWHT` | `\x1B[0;5;37m` | Blinking white |

### Absolute Bright Blinking Colors

| Macro | Value | Description |
|-------|-------|-------------|
| `ABHBLK` | `\x1B[1;5;30m` | Blinking bright black |
| `ABHRED` | `\x1B[1;5;31m` | Blinking bright red |
| `ABHGRN` | `\x1B[1;5;32m` | Blinking bright green |
| `ABHYEL` | `\x1B[1;5;33m` | Blinking bright yellow |
| `ABHBLU` | `\x1B[1;5;34m` | Blinking bright blue |
| `ABHMAG` | `\x1B[1;5;35m` | Blinking bright magenta |
| `ABHCYN` | `\x1B[1;5;36m` | Blinking bright cyan |
| `ABHWHT` | `\x1B[1;5;37m` | Blinking bright white |

---

## Processing Functions

**Source**: `legacy/src/screen.cpp`

### process_colors()

Main function for processing inline color codes.

```cpp
std::string process_colors(std::string_view str, int mode);
```

**Parameters**:
- `str`: String containing inline color codes
- `mode`: Processing mode (CLR_PARSE, CLR_ESCAPE, or CLR_STRIP)

**Modes**:
- `CLR_PARSE` (0): Convert `&` and `@` codes to ANSI escape sequences
- `CLR_ESCAPE` (1): Escape color codes by doubling the prefix (`&` → `&&`, `@` → `@@`)
- `CLR_STRIP` (2): Remove all inline color codes, leaving plain text

**Example**:
```cpp
std::string msg = "&1Warning:&0 Low health!";
std::string ansi = process_colors(msg, CLR_PARSE);
// Returns: "\033[31mWarning:\033[0m Low health!"

std::string plain = process_colors(msg, CLR_STRIP);
// Returns: "Warning: Low health!"

std::string escaped = process_colors(msg, CLR_ESCAPE);
// Returns: "&&1Warning:&&0 Low health!" (literal display)
```

### count_color_chars()

Count the number of color code characters in a string.

```cpp
int count_color_chars(std::string_view str) noexcept;
```

**Returns**: Count of `&` and `@` characters (color code prefixes)

### ansi_strlen()

Get the visual length of a string (excluding color codes).

```cpp
int ansi_strlen(std::string_view str);
```

**Returns**: String length minus color codes (each code = 2 chars: prefix + color)

**Example**:
```cpp
std::string msg = "&1Red&0 Text";  // 10 chars total
int visual_len = ansi_strlen(msg);  // Returns 8 ("Red Text")
```

### ansi_strnlen()

Get the byte length needed to display n visible characters.

```cpp
int ansi_strnlen(std::string_view str, int n);
```

**Parameters**:
- `str`: String with color codes
- `n`: Number of visible characters desired

**Returns**: Byte offset in string to get n visible characters

**Example**:
```cpp
std::string msg = "&1Hello&0 World";
int offset = ansi_strnlen(msg, 5);  // Get offset for "Hello"
// Returns: 8 (includes "&1Hello" bytes)
```

### strip_ansi()

Remove all inline color codes from a string.

```cpp
std::string strip_ansi(std::string_view string);
```

**Equivalent to**: `process_colors(string, CLR_STRIP)`

### escape_ansi()

Escape all inline color codes (double the prefixes).

```cpp
std::string escape_ansi(std::string_view string);
```

**Equivalent to**: `process_colors(string, CLR_ESCAPE)`

---

## Character Preference Macros

**Source**: `legacy/src/screen.hpp:128-157`

These macros allow color output to respect player color preferences.

### Color Preference Levels

| Constant | Value | Description |
|----------|-------|-------------|
| `C_OFF` | 0 | No color |
| `C_SPR` | 1 | Sparse color |
| `C_NRM` | 2 | Normal color |
| `C_CMP` | 3 | Complete color |

### Preference Detection

```cpp
COLOR_LEV(ch)
```

Returns player's color preference level (0-3) based on flags:
- `PRF_COLOR_1` flag: +1
- `PRF_COLOR_2` flag: +2

### Conditional Color Macros

```cpp
CLRLV(ch, clr, lev)  // Color if player's level >= lev
CLR(ch, clr)         // Color if player's level >= C_NRM (2)
```

**Example**:
```cpp
// Only show color if player has C_NRM or higher
send_to_char(CLR(ch, FRED) "Error!" QNRM, ch);
```

### Quick Color Macros (Q-prefix)

These macros use `CLR(ch, ...)` to respect player preferences.

**Reset**:
```cpp
QNRM  // CLR(ch, ANRM) - Reset to normal
```

**Normal Colors**:
```cpp
QBLK  // CLR(ch, AFBLK) - Black
QRED  // CLR(ch, AFRED) - Red
QGRN  // CLR(ch, AFGRN) - Green
QYEL  // CLR(ch, AFYEL) - Yellow
QBLU  // CLR(ch, AFBLU) - Blue
QMAG  // CLR(ch, AFMAG) - Magenta
QCYN  // CLR(ch, AFCYN) - Cyan
QWHT  // CLR(ch, AFWHT) - White
```

**Bright Colors**:
```cpp
QHBLK  // CLR(ch, AHBLK) - Bright black
QHRED  // CLR(ch, AHRED) - Bright red
QHGRN  // CLR(ch, AHGRN) - Bright green
QHYEL  // CLR(ch, AHYEL) - Bright yellow
QHBLU  // CLR(ch, AHBLU) - Bright blue
QHMAG  // CLR(ch, AHMAG) - Bright magenta
QHCYN  // CLR(ch, AHCYN) - Bright cyan
QHWHT  // CLR(ch, AHWHT) - Bright white
```

**Usage**:
```cpp
sprintf(buf, "%sYou hit %s for %d damage!%s\n",
        QHRED, GET_NAME(victim), damage, QNRM);
send_to_char(buf, ch);
```

---

## Usage Examples

### Basic Relative Codes

```cpp
// Simple colored text
std::string msg = "&1Error:&0 Invalid command!";
send_to_char(process_colors(msg, CLR_PARSE).c_str(), ch);
// Output: Red "Error:" then normal "Invalid command!"

// Multiple colors
std::string status = "Health: &2High&0 Mana: &3Medium&0";
// Output: Green "High", Yellow "Medium"

// Background color
std::string warning = "&R&7 DANGER &0";
// Output: White text on red background
```

### Absolute Codes

```cpp
// Title with multiple colors
std::string title = "@Y== @WFIERY@RMUD @Y==";
// Output: Yellow "== ", bright white "FIERY", bright red "MUD ", yellow "=="

// Ensuring clean state
std::string msg = "@0Some text @GBright green@0 Normal";
// Each @0 ensures reset to normal state
```

### Combined Relative and Absolute

```cpp
// Relative codes build on each other
std::string msg1 = "&b&1Bold Red&0";  // Bold + Red, then reset both

// Absolute codes reset first
std::string msg2 = "&b&1Bold Red@GBright Green";  // @G resets bold
```

### Escaping Color Codes

```cpp
// Display literal color codes
std::string help = "Use &&1 for red text";
std::string output = process_colors(help, CLR_PARSE);
// Output: "Use &1 for red text" (shows the code, doesn't color it)

std::string escaped = process_colors("&1Red", CLR_ESCAPE);
// Returns: "&&1Red" (escaped for display)
```

### String Length Calculation

```cpp
std::string colored = "&1Hello&0 World";
int total_len = colored.length();        // 15 (includes codes)
int color_chars = count_color_chars(colored);  // 2 (two &'s)
int visual_len = ansi_strlen(colored);   // 11 ("Hello World")

// For formatting with colors
int padding = 20 - visual_len;  // Calculate padding correctly
```

### Legacy Character-Aware Output

```cpp
// Old-style with character color preferences
sprintf(buf, "%sYou gain %d experience!%s\n",
        QHGRN, exp_gained, QNRM);
send_to_char(buf, ch);
// Only shows color if ch has color enabled
```

---

## Migration to Modern API

### Before (Legacy)

```cpp
// Inline color codes
sprintf(buf, "&1Error:&0 %s not found!\n", name);
send_to_char(process_colors(buf, CLR_PARSE).c_str(), ch);
```

### After (Modern)

```cpp
// RichText API
RichText msg;
msg.colored("Error: ", Color::BrightRed)
   .text(fmt::format("{} not found!", name));
ctx.send_rich(msg);

// Or using CommandContext helper
ctx.send_error(fmt::format("{} not found!", name));
```

### Before (Legacy)

```cpp
sprintf(buf, "@G%s@0 says, '@Y%s@0'\n",
        GET_NAME(ch), argument);
send_to_room(buf, ch->in_room);
```

### After (Modern)

```cpp
RichText msg;
msg.colored(actor->name(), Color::BrightGreen)
   .text(" says, '")
   .colored(argument, Color::BrightYellow)
   .text("'");
ctx.send_to_room(msg.to_ansi());
```

### Before (Legacy)

```cpp
sprintf(buf, "%s[%sHP:%d/%d%s %sMP:%d/%d%s]%s\n",
        QHCYN, QHRED, hp, maxhp, QHCYN,
        QHBLU, mana, maxmana, QHCYN, QNRM);
```

### After (Modern)

```cpp
RichText status;
status.colored("[", Color::BrightCyan)
      .colored("HP:", Color::BrightRed)
      .text(fmt::format("{}/{}", hp, maxhp))
      .colored(" ", Color::BrightCyan)
      .colored("MP:", Color::BrightBlue)
      .text(fmt::format("{}/{}", mana, maxmana))
      .colored("]", Color::BrightCyan);
ctx.send_rich(status);
```

---

## Key Differences: Legacy vs Modern

| Feature | Legacy | Modern |
|---------|--------|--------|
| **Syntax** | Inline codes (`&1`, `@R`) | Builder API (`.colored()`) |
| **Type Safety** | String-based, no checking | Type-safe enums |
| **Processing** | Runtime string parsing | Compile-time safe |
| **Color Support** | 16 colors only | 16 + 256 + RGB (24-bit) |
| **Terminal Adaptation** | None | Automatic fallback |
| **Readability** | Hard to read in code | Clear, self-documenting |
| **Error Handling** | Silent failures | Compile-time errors |

---

## Common Pitfalls

### 1. Forgetting to Reset Colors

```cpp
// Bad (color bleeds)
send_to_char("&1Error: something failed", ch);
send_to_char("This will also be red!", ch);

// Good
send_to_char("&1Error: something failed&0", ch);
send_to_char("This is normal.", ch);
```

### 2. Mixing Relative and Absolute Without Understanding

```cpp
// Confusing behavior
std::string msg = "&b&1Bold Red @2Green";
// @2 resets bold, then applies green (not bold green)

// Clearer
std::string msg = "&b&1Bold Red&0 @G&bBold Green";
```

### 3. Incorrect Length Calculations

```cpp
// Wrong
std::string colored = "&1Hello&0";
int width = colored.length();  // 9, not 5!

// Correct
int width = ansi_strlen(colored);  // 5 (visual length)
```

### 4. Not Processing Before Output

```cpp
// Wrong (shows literal codes)
send_to_char("&1Red text", ch);

// Correct
send_to_char(process_colors("&1Red text", CLR_PARSE).c_str(), ch);
```

---

## Quick Reference

### Most Common Codes

**Relative**:
```
&0 - Reset
&1 - Red
&2 - Green
&3 - Yellow
&4 - Blue
&b - Bold
&u - Underline
```

**Absolute**:
```
@0 - Reset
@R - Bright Red
@G - Bright Green
@Y - Bright Yellow
@W - Bright White
```

### Processing

```cpp
process_colors(str, CLR_PARSE);   // Convert to ANSI
process_colors(str, CLR_STRIP);   // Remove codes
ansi_strlen(str);                 // Visual length
```

### Character Preferences

```cpp
QHRED  // Bright red (respects preferences)
QNRM   // Reset (respects preferences)
CLR(ch, AHRED)  // Manual preference check
```

---

## Deprecation Notice

**This legacy color system is deprecated.**

For all new code, use the modern RichText API documented in `COLOR_CODES_MODERN.md`.

**Benefits of Modern API**:
- Type safety (compile-time checks)
- Better readability
- Extended color support (256-color, RGB)
- Terminal capability detection
- Automatic color reset
- No string parsing overhead
- Self-documenting code

**Legacy code maintenance**:
- Keep legacy code for backward compatibility with existing world files, help texts, etc.
- When modifying legacy code, consider migrating to modern API
- Do not add new inline color codes to new features

---

*Document Version: 1.0*
*Last Updated: 2025-12-10*
*For modern color API, see: COLOR_CODES_MODERN.md*

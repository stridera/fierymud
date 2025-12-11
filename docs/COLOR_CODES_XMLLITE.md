# FieryMUD XML-Lite Markup System

Modern XML-like color and style markup for FieryMUD player-authored content (names, descriptions, etc.).

**Status**: Proposed/In Development
**Target**: Player-facing content (object names, descriptions, room descriptions, character names)

---

## Table of Contents

1. [Overview](#overview)
2. [Tag Syntax](#tag-syntax)
3. [Modifiers Reference](#modifiers-reference)
4. [Color System](#color-system)
5. [Style Stack Behavior](#style-stack-behavior)
6. [Complete Examples](#complete-examples)
7. [Semantic Style Guidelines](#semantic-style-guidelines)
8. [Implementation Specification](#implementation-specification)
9. [Migration from Legacy](#migration-from-legacy)
10. [Security and Validation](#security-and-validation)

---

## Overview

The XML-Lite markup system provides a clean, nestable, and powerful way to style text:

### Key Features

✅ **XML-like syntax**: Familiar, readable tags
✅ **Nestable**: Proper tag nesting with stack-based rendering
✅ **Flexible colors**: Named colors, 256-color palette, or 24-bit RGB
✅ **Text attributes**: Bold, underline, strikethrough
✅ **Semantic tags**: Name your styles (e.g., `<npc-name>`, `<danger>`)
✅ **Full reset**: `</>` clears all active styles
✅ **No escaping needed**: Natural text flow

### Design Principles

1. **Readability**: Code should be human-readable in source form
2. **Nesting**: Proper tag hierarchy, not inline code chaos
3. **Flexibility**: From simple named colors to full RGB control
4. **Safety**: Validation prevents malformed markup from breaking output

---

## Tag Syntax

### Basic Tag Forms

```
<name>                  → Open named style with default modifiers
<name:mod:mod>          → Open named style with modifiers
<mod:mod>               → Anonymous style (no name)
</name>                 → Close specific named tag
</>                     → FULL RESET (clear all styles)
```

### Syntax Rules

1. **No whitespace inside tags**: `<red:blue>` ✅ `<red : blue>` ❌
2. **Colon-separated modifiers**: `<b:red:bg-blue>`
3. **Case-sensitive**: `<red>` ≠ `<Red>` (recommend lowercase)
4. **Closing behavior**:
   - `</name>` closes only the most recent `<name>` tag
   - `</>` clears entire style stack (full reset)
5. **Nesting allowed**: `<b><red>text</red></b>` ✅
6. **Newlines**: Use literal `\n` in markup text (no special tag needed)

### Tag Examples

```xml
<!-- Simple named tag -->
<red>Red text</red>

<!-- Named tag with modifiers -->
<danger:red:b>DANGER!</danger>

<!-- Anonymous style (closes with </>) -->
<b:u:red>Bold underlined red</> back to normal

<!-- Full reset -->
<b><red>Bold red</> plain</b> still bold
```

---

## Modifiers Reference

Modifiers are tokens separated by colons (`:`) that define style attributes.

### Text Attributes

| Modifier | Effect | ANSI Code | Support | Notes |
|----------|--------|-----------|---------|-------|
| `b` | Bold/bright | `\033[1m` | Universal | Makes text prominent (brighter colors + bold font) |
| `dim` | Dim/faint | `\033[2m` | Common | Reduces intensity (50-70% brightness) |
| `u` | Underline | `\033[4m` | Universal | Single underline |
| `i` | Italic | `\033[3m` | Limited | Not all terminals support |
| `s` | Strikethrough | `\033[9m` | Modern | Modern terminals only |
| `blink` | Slow blink | `\033[5m` | Limited | Often disabled, use sparingly |
| `reverse` | Reverse video | `\033[7m` | Common | Swaps foreground/background colors |
| `hide` | Hidden text | `\033[8m` | Limited | Text invisible (for passwords, etc.) |

**Intensity Levels**:
- `<dim:red>` → Dark/muted red (50-70% intensity)
- `<red>` → Normal red (100% intensity)
- `<b:red>` → Bright/vivid red (150% intensity, same as `\033[91m`)

**Mutual Exclusivity**:
- `b` (bold) and `dim` are mutually exclusive in most terminals
- Using both in same tag: last one wins or they cancel out

**Examples**:
```xml
<b>Bold text</b>
<dim>Dimmed/faint text</dim>
<u>Underlined text</u>
<i>Italic text</i>
<s>Strikethrough text</s>
<b:u>Bold and underlined</b:u>
<blink>Blinking text (use sparingly!)</blink>
<reverse>Reversed colors</reverse>
<hide>Hidden password</hide>

<!-- Intensity examples -->
<dim:red>A faint red glow</dim:red>
<red>The dragon's scales are red</red>
<b:red>CRITICAL HIT!</b:red>
```

### Named Foreground Colors

Standard 16-color ANSI palette with friendly names:

| Modifier | Color | ANSI Code | Visual |
|----------|-------|-----------|--------|
| `black` | Black | `\033[30m` | Black |
| `red` | Red | `\033[31m` | Red |
| `green` | Green | `\033[32m` | Green |
| `yellow` | Yellow | `\033[33m` | Yellow |
| `blue` | Blue | `\033[34m` | Blue |
| `purple` | Magenta/Purple | `\033[35m` | Magenta |
| `magenta` | Magenta (alias) | `\033[35m` | Magenta |
| `cyan` | Cyan | `\033[36m` | Cyan |
| `teal` | Cyan (alias) | `\033[36m` | Cyan |
| `white` | White/Gray | `\033[37m` | White |
| `brown` | Dark Yellow | `\033[33m` | Yellow |
| `orange` | Bright Yellow | `\033[93m` | Bright Yellow |

**Bright variants** (when used with `b` modifier or terminal support):
```xml
<b:red>Bright red</b>      <!-- Bold makes it bright -->
<b:blue>Bright blue</b>
```

**Examples**:
```xml
<red>Red text</red>
<green>Green text</green>
<cyan>Cyan text</cyan>
<b:yellow>Bright yellow</b>
```

### Named Background Colors

Background colors use `bg-` prefix:

| Modifier | Color | ANSI Code |
|----------|-------|-----------|
| `bg-black` | Black background | `\033[40m` |
| `bg-red` | Red background | `\033[41m` |
| `bg-green` | Green background | `\033[42m` |
| `bg-yellow` | Yellow background | `\033[43m` |
| `bg-blue` | Blue background | `\033[44m` |
| `bg-purple` | Magenta background | `\033[45m` |
| `bg-magenta` | Magenta background | `\033[45m` |
| `bg-cyan` | Cyan background | `\033[46m` |
| `bg-teal` | Cyan background | `\033[46m` |
| `bg-white` | White background | `\033[47m` |

**Examples**:
```xml
<white:bg-red>White on red</white>
<black:bg-yellow>Black on yellow (warning style)</black>
<bg-blue>Default foreground on blue background</bg-blue>
```

---

## Color System

The XML-Lite system supports three color modes with automatic terminal capability detection.

### 1. Named Colors (16-color ANSI)

**Best for**: Broad compatibility, semantic meaning

```xml
<red>Error message</red>
<green>Success!</green>
<yellow:bg-blue>Yellow on blue</yellow>
```

**Terminal Support**: Universal (all terminals)

### 2. Indexed Colors (256-color palette)

**Syntax**:
- `cN` → Foreground color index N (0-255)
- `bgcN` → Background color index N (0-255)

**Color Ranges**:
- 0-15: Standard ANSI colors
- 16-231: 6×6×6 RGB cube (216 colors)
- 232-255: Grayscale (24 shades)

**Examples**:
```xml
<c196>Bright red (index 196)</c196>
<c33>Deep blue (index 33)</c33>
<c33:bgc235>Blue on gray background</c33>
<bgc17>Background only (index 17)</bgc17>
```

**ANSI Output**:
- Foreground: `\033[38;5;Nm`
- Background: `\033[48;5;Nm`

**Terminal Support**: Modern terminals (xterm-256color, screen-256color, etc.)

### 3. TrueColor / RGB (24-bit)

**Syntax**:
- `#RRGGBB` → Foreground RGB color (hex)
- `bg#RRGGBB` → Background RGB color (hex)

**Examples**:
```xml
<#FF0000>Pure red</FF0000>
<#00FF00>Pure green</00FF00>
<#FFAA00>Orange</FFAA00>
<#FF1493>Deep pink</FF1493>
<#00FF00:bg#002200>Matrix green on dark green</00FF00>
```

**ANSI Output**:
- Foreground: `\033[38;2;R;G;Bm`
- Background: `\033[48;2;R;G;Bm`

**Terminal Support**: Modern terminals (iTerm2, Alacritty, Kitty, WezTerm, Mudlet)

**Fallback**: Renderer should downsample to 256-color or 16-color if unsupported

### Foreground/Background Resolution Rules

When multiple color modifiers appear without explicit `bg-` or `bgc` prefix:

**Rule 1**: First color-like token = foreground, second = background

```xml
<red:blue>          → fg: red,    bg: blue
<c196:c17>          → fg: c196,   bg: c17
<#FF0000:#0000FF>   → fg: #FF0000, bg: #0000FF
```

**Rule 2**: Explicit background prefix always wins

```xml
<red:bg-blue>       → fg: red,    bg: blue (explicit)
<c196:bgc17>        → fg: c196,   bg: c17 (explicit)
<#FF0000:bg#0000FF> → fg: #FF0000, bg: #0000FF (explicit)
```

**Rule 3**: Mixing indexed and named colors

```xml
<red:bgc235>        → fg: red (named), bg: c235 (indexed)
<c196:bg-blue>      → fg: c196 (indexed), bg: blue (named)
<#FF0000:bg-red>    → fg: #FF0000 (RGB), bg: red (named)
```

### Color Modifier Priority

When parsing modifiers, apply in this order:

1. **Text attributes** (b, u, s)
2. **Foreground color** (first color token or explicit fg)
3. **Background color** (second color token or explicit bg)

```xml
<b:u:red:bg-blue>
   │ │  │    └─── background: blue
   │ │  └──────── foreground: red
   │ └─────────── underline
   └───────────── bold
```

---

## Style Stack Behavior

The renderer maintains a **style stack** that tracks all active styles.

### Stack Operations

1. **Opening tag** (`<name>` or `<name:mod>`): Push style to stack
2. **Closing tag** (`</name>`): Pop only that named style
3. **Full reset** (`</>`): Clear entire stack

### Nesting Example

```xml
<b>Bold text
  <red>Bold red text
    <u>Bold red underlined</u>
  </red> still bold
</b> normal
```

**Stack evolution**:
```
"Bold text"               → [b]
"Bold red text"           → [b, red]
"Bold red underlined"     → [b, red, u]
After </u>                → [b, red]
"still bold"              → [b]
After </b>                → []
```

### Full Reset Behavior

```xml
<b><red>Bold red</> plain</b> still bold
```

**Stack evolution**:
```
"Bold red"     → [b, red]
After </>      → []              (full reset clears stack)
"plain"        → []
"still bold"   → [b]             (outer <b> still active)
```

### Closing Specific Tags

```xml
<b><red>Bold red</red> just bold</b>
```

**Stack evolution**:
```
"Bold red"     → [b, red]
After </red>   → [b]             (only red removed)
"just bold"    → [b]
After </b>     → []
```

### Unclosed Tags

**Behavior**: Unclosed tags remain on stack until:
1. Explicitly closed with `</name>` or `</>`
2. End of string reached (auto-close all)

**Example**:
```xml
<b>Bold text that never closes
```

**Rendering**: Applies bold until end of string, then auto-resets

**Best practice**: Always close tags for clarity

---

## Complete Examples

### Basic Text Styling

```xml
<b>Bold text</b>
<u>Underlined text</u>
<i>Italic text</i>
<s>Strikethrough text</s>
<d>Dimmed text</d>
<blink>Blinking text</blink>
<reverse>Reversed video</reverse>
<b:u>Bold and underlined</b:u>
<b:u:s>All attributes combined</b:u:s>
```

### Named Colors

```xml
<red>Red text</red>
<green>Green text</green>
<blue>Blue text</blue>
<b:yellow>Bright yellow</b:yellow>
```

### Foreground + Background

```xml
<red:blue>Red on blue</red:blue>
<yellow:bg-purple>Yellow on purple</yellow:bg-purple>
<white:bg-red>White on red (warning)</white:bg-red>
<black:bg-yellow>Black on yellow (caution)</black:bg-yellow>
```

### Indexed Colors (256-color)

```xml
<c196>Bright red (index 196)</c196>
<c33>Deep blue (index 33)</c33>
<c220>Golden yellow (index 220)</c220>
<c33:bgc235>Blue on gray</c33:bgc235>
<c196:c17>Red on dark blue</c196:c17>
```

### TrueColor (24-bit RGB)

```xml
<#FF0000>Pure red</#FF0000>
<#00FF00>Pure green</#00FF00>
<#0000FF>Pure blue</#0000FF>
<#FFAA00>Orange</#FFAA00>
<#FF1493>Deep pink</#FF1493>
<#00FF00:bg#002200>Matrix green</#00FF00>
```

### Nesting Styles

```xml
<b>This is bold. <red>This is bold red. <u>Bold red underlined</u></red> Still bold.</b>

<yellow>Yellow <b>yellow bold <red>red bold</red> yellow bold</b> yellow</yellow>

<b>
  <red>R</red>
  <green>A</green>
  <blue>I</blue>
  <yellow>N</yellow>
  <purple>B</purple>
  <cyan>O</cyan>
  <white>W</white>
</b>
```

### Anonymous Styles

```xml
<u:c33>Underlined blue indexed</> now plain

<b:red:bg-yellow>Bold red on yellow</> reset

<#FF00FF:bg#000000>Magenta on black</> normal
```

### Rainbow Text (Bold Wrapper)

```xml
<b>
  <#FF0000>R</#FF0000>
  <#FF7F00>A</#FF7F00>
  <#FFFF00>I</#FFFF00>
  <#00FF00>N</#00FF00>
  <#0000FF>B</#0000FF>
  <#4B0082>O</#4B0082>
  <#8B00FF>W</#8B00FF>
</b>
```

Output: **RAINBOW** (each letter in its color, all bold)

### Semantic Tags

```xml
<npc-name>Goblin Chieftain</npc-name> attacks you!

<danger:red:b>WARNING: Low health!</danger>

<item-legendary:#FF8C00:b>Excalibur</item-legendary>

<quest-complete:green:b>Quest completed!</quest-complete>

<room-name:cyan:b>The Throne Room</room-name>
```

### Object Descriptions

```xml
A <item-rare:blue>shimmering blue cloak</item-rare> lies here.

<item-legendary:#FFD700:b>The Sword of Kings</item-legendary> glows with <#FF4500>fiery</#FF4500> power.

A <npc-name:red>red dragon</npc-name> guards a pile of <#FFD700>gold</#FFD700>.
```

### Room Descriptions

```xml
<room-name:cyan:b>The Mystic Forest</room-name>

You stand in a dense forest. <#228B22>Emerald trees</#228B22> tower above you,
their leaves rustling in the wind. A <#8B4513>dirt path</#8B4513> leads <cyan>north</cyan>
and <yellow>east</yellow>. Strange <purple:b>purple mushrooms</purple> glow faintly
in the shadows.

Exits: <cyan:b>north</cyan>, <yellow:b>east</yellow>
```

### Using Intensity Levels

```xml
<room-name:cyan:b>The Ancient Library</room-name>

<dim:yellow>Dusty golden light<dim:yellow> filters through <dim:blue>cobweb-covered
windows</dim:blue>. Rows of <b:red>ancient tomes</b:red> line the shelves, their
<dim:red>faded red bindings</dim:red> barely visible in the gloom. A <green>green
marble desk</green> sits in the center, and a <b:green>brilliant emerald gem</b:green>
rests upon it, casting <dim:green>eerie green shadows</dim:green> across the room.

The <b:yellow>title page</b:yellow> of an open book catches your eye, while
<dim>forgotten scrolls</dim> gather dust in the corners.
```

---

## Semantic Style Guidelines

### Recommended Semantic Tags

Define consistent semantic tags for game elements:

#### NPCs and Creatures

```xml
<npc-name>                     → NPC/mob names
<npc-friendly:green>           → Friendly NPCs
<npc-hostile:red>              → Hostile NPCs
<npc-boss:#FF4500:b>           → Boss monsters
```

#### Items and Objects

```xml
<item-common:#C0C0C0>          → Common items (silver)
<item-uncommon:#1EFF00>        → Uncommon items (lime)
<item-rare:#0070DD>            → Rare items (blue)
<item-epic:#A335EE>            → Epic items (purple)
<item-legendary:#FF8C00:b>     → Legendary items (orange/bold)
```

#### UI Elements

```xml
<danger:red:b>                 → Danger/error messages
<warning:yellow>               → Warning messages
<success:green>                → Success messages
<info:cyan>                    → Informational messages
<highlight:yellow:b>           → Emphasis/highlights
```

#### World Elements

```xml
<room-name:cyan:b>             → Room names
<exit:yellow>                  → Exit directions
<quest:green:b>                → Quest-related text
<skill:blue>                   → Skill/ability names
<damage:#FF0000>               → Damage numbers
<healing:#00FF00>              → Healing numbers
```

### Usage in Content

**Object short description**:
```xml
<item-rare:blue>a shimmering blue cloak</item-rare>
```

**Object long description**:
```xml
A <item-rare:blue>shimmering blue cloak</item-rare> of <#4169E1>royal silk</#4169E1>
lies neatly folded here. Intricate <#FFD700>golden</#FFD700> embroidery depicts
ancient <purple>magical<purple> runes along the hem.
```

**Room description**:
```xml
<room-name:cyan:b>The Crystal Cavern</room-name>

Massive <#87CEEB>crystal formations</#87CEEB> jut from the walls, refracting light
into <#FF69B4>rainbow patterns</#FF69B4>. A <#B22222>dark pool</#B22222> of water
sits in the center, its surface perfectly still. The air hums with
<purple:b>magical energy</purple>.
```

---

## Implementation Specification

### Parser Requirements

The renderer must implement a **tag parser** and **style stack manager**.

#### Parser Algorithm

1. **Scan for tags**: Find `<` and matching `>`
2. **Classify tag type**:
   - Closing: `</name>` or `</>`
   - Opening: `<name>` or `<name:mod:mod>`
3. **Parse modifiers**: Split by `:`, classify each token
4. **Update stack**:
   - Opening: Push style to stack
   - Closing named: Pop that named style
   - Closing `</>`: Clear entire stack
5. **Generate ANSI**: Convert current stack state to ANSI codes
6. **Output**: Text with ANSI codes inserted

#### Modifier Classification

```cpp
enum class ModifierType {
    Attribute,          // b, u, i, s, d, blink, reverse, hide
    NamedForeground,    // red, green, blue, etc.
    NamedBackground,    // bg-red, bg-green, etc.
    IndexedForeground,  // cN (0-255)
    IndexedBackground,  // bgcN (0-255)
    RGBForeground,      // #RRGGBB
    RGBBackground,      // bg#RRGGBB
    Unknown
};
```

**Classification rules**:
```cpp
if (mod == "b" || mod == "u" || mod == "i" || mod == "s" ||
    mod == "d" || mod == "blink" || mod == "reverse" || mod == "hide")
    → Attribute
else if (mod.starts_with("bg-"))
    → NamedBackground
else if (mod.starts_with("bgc") && is_number(mod.substr(3)))
    → IndexedBackground
else if (mod.starts_with("bg#") && is_hex_color(mod.substr(3)))
    → RGBBackground
else if (mod.starts_with("c") && is_number(mod.substr(1)))
    → IndexedForeground
else if (mod.starts_with("#") && is_hex_color(mod.substr(1)))
    → RGBForeground
else if (is_named_color(mod))
    → NamedForeground
else
    → Unknown (ignore or error)
```

#### Style Stack Structure

```cpp
struct StyleLayer {
    std::string tag_name;              // Empty for anonymous
    bool bold = false;
    bool underline = false;
    bool italic = false;
    bool strikethrough = false;
    bool dim = false;
    bool blink = false;
    bool reverse = false;
    bool hidden = false;
    std::optional<Color> foreground;   // Named, indexed, or RGB
    std::optional<Color> background;
};

std::vector<StyleLayer> style_stack;
```

#### ANSI Generation

**From current stack state**:
1. Combine all attributes (bold, underline, strikethrough)
2. Take most recent foreground color (if any)
3. Take most recent background color (if any)
4. Generate ANSI escape sequence

**Example**:
```
Stack: [b], [red], [u]
→ Attributes: bold, underline
→ Foreground: red
→ Background: none
→ ANSI: \033[1;4;31m
```

#### Terminal Capability Adaptation

**Fallback strategy**:

1. **Detect terminal capabilities** (from TerminalCapabilities)
2. **Downsample colors** if needed:
   - RGB → 256-color (nearest color)
   - RGB → 16-color (nearest ANSI color)
   - 256-color → 16-color
3. **Skip unsupported attributes** (e.g., strikethrough on basic terminals)

**Capability levels**:
```cpp
if (caps.supports_true_color) {
    // Use RGB as-is
} else if (caps.supports_256_color) {
    // Downsample RGB to 256-color
    color = nearest_256_color(rgb);
} else if (caps.supports_color) {
    // Downsample to basic 16 colors
    color = nearest_ansi_color(rgb);
} else {
    // Plain text only
}
```

### Processing Modes

**Input**: Player-authored string with XML-Lite markup
**Output**: ANSI-escaped string for terminal display

**Modes**:
1. **Parse**: Convert markup to ANSI (default)
2. **Strip**: Remove all markup, plain text only
3. **Validate**: Check markup syntax without rendering

### Error Handling

**Malformed markup**:
- **Unclosed tags**: Auto-close at end of string
- **Invalid modifiers**: Ignore unknown modifiers
- **Mismatched closing**: Log warning, attempt recovery
- **Invalid hex colors**: Ignore or use fallback

**Security**:
- **Length limits**: Cap total string length (prevent DoS)
- **Nesting depth**: Limit stack depth (prevent recursion attacks)
- **Tag name length**: Max 32 chars for tag names

---

## Special Characters and Escaping

### Newlines

**Legacy system**: `&_` → newline character
**XML-Lite**: Use literal `\n` in markup text

```xml
<!-- Just use normal newlines -->
Line 1
Line 2

<!-- Or literal \n in code -->
"Line 1\nLine 2"

<!-- No special tag needed -->
<red>Red text
on multiple lines</red>
```

### Escaping Special Characters

**Tag characters** (`<`, `>`):
- Use HTML entities: `&lt;` for `<`, `&gt;` for `>`
- Or escape function: `escape_markup(text)`

```xml
<!-- Display literal angle brackets -->
The formula is: &lt;damage&gt; = strength * 2

<!-- Or escape the entire string -->
escape_markup("<example>") → "&lt;example&gt;"
```

**Colon in text** (not in tags):
- No escaping needed, only modifiers inside `<...>` are parsed

```xml
<red>Time: 12:34:56</red>  <!-- Colons in text are fine -->
```

---

## Migration from Legacy

### Legacy Inline → XML-Lite

| Legacy | XML-Lite | Notes |
|--------|----------|-------|
| `&1Red&0` | `<red>Red</red>` | Named tag |
| `&b&1Bold red&0` | `<b:red>Bold red</b:red>` | Combined |
| `@RBright red@0` | `<b:red>Bright red</b:red>` | Bold = bright |
| `&R&7White on red&0` | `<white:bg-red>White on red</white:bg-red>` | Fg + bg |
| `&_` | `\n` | Just use normal newline |
| `&d` | `<dim>text</dim>` | Dim/faint intensity |

### Migration Strategy

1. **Preserve legacy**: Keep `&` and `@` codes for backward compatibility
2. **New content**: Use XML-Lite for all new descriptions, names
3. **Gradual migration**: Convert legacy content over time
4. **Dual parser**: Support both systems during transition
5. **Builder tools**: Provide helpers for builders to use XML-Lite

---

## Security and Validation

### Input Validation

**Length limits**:
```cpp
constexpr size_t MAX_MARKUP_LENGTH = 4096;      // Total string
constexpr size_t MAX_TAG_NAME_LENGTH = 32;      // Tag names
constexpr size_t MAX_NESTING_DEPTH = 16;        // Stack depth
constexpr size_t MAX_MODIFIER_COUNT = 8;        // Modifiers per tag
```

**Character restrictions**:
- Tag names: `[a-zA-Z0-9_-]` only
- Hex colors: `[0-9A-Fa-f]{6}` exactly
- Indexed colors: `[0-9]{1,3}` (0-255)

### Sanitization

**Builder input**:
```cpp
std::string sanitize_markup(std::string_view input) {
    // 1. Validate syntax
    // 2. Check nesting depth
    // 3. Verify color codes
    // 4. Enforce length limits
    // 5. Remove/escape invalid content
}
```

**Display to players**:
```cpp
std::string render_markup(std::string_view markup,
                         const TerminalCapabilities& caps) {
    // 1. Parse markup
    // 2. Build style stack
    // 3. Generate ANSI with capability fallback
    // 4. Return escaped string
}
```

### Denial of Service Prevention

**Prevent**:
1. **Excessive nesting**: Limit stack depth to 16
2. **Long strings**: Cap at 4KB
3. **Tag bombs**: Limit tags per string (e.g., 100 tags max)
4. **Processing time**: Timeout after N milliseconds

---

## Quick Reference

### Syntax Cheat Sheet

```xml
<!-- Text attributes -->
<b>Bold</b>
<u>Underline</u>
<s>Strikethrough</s>

<!-- Named colors -->
<red>Red</red>
<green>Green</green>
<blue>Blue</blue>

<!-- Foreground + background -->
<red:blue>Red on blue</red>
<yellow:bg-purple>Yellow on purple</yellow>

<!-- Indexed colors (256) -->
<c196>Indexed color 196</c196>
<c33:bgc235>Fg 33, bg 235</c33>

<!-- TrueColor (RGB) -->
<#FF0000>Pure red</#FF0000>
<#00FF00:bg#002200>Green on dark green</#00FF00>

<!-- Combined -->
<b:u:red:bg-blue>Bold underlined red on blue</b:u:red:bg-blue>

<!-- Nesting -->
<b>Bold <red>red</red> still bold</b>

<!-- Full reset -->
<b:red>Bold red</> plain

<!-- Semantic -->
<npc-name:red>Goblin</npc-name>
<item-legendary:#FFD700:b>Excalibur</item-legendary>
```

### Modifier Quick Reference

| Type | Syntax | Example |
|------|--------|---------|
| Bold | `b` | `<b>text</b>` |
| Underline | `u` | `<u>text</u>` |
| Italic | `i` | `<i>text</i>` |
| Strikethrough | `s` | `<s>text</s>` |
| Dim | `d` | `<d>text</d>` |
| Blink | `blink` | `<blink>text</blink>` |
| Reverse | `reverse` | `<reverse>text</reverse>` |
| Hidden | `hide` | `<hide>text</hide>` |
| Named color | `red`, `green`, etc. | `<red>text</red>` |
| Named bg | `bg-red`, `bg-green` | `<bg-red>text</bg-red>` |
| Indexed fg | `cN` (0-255) | `<c196>text</c196>` |
| Indexed bg | `bgcN` (0-255) | `<bgc235>text</bgc235>` |
| RGB fg | `#RRGGBB` | `<#FF0000>text</#FF0000>` |
| RGB bg | `bg#RRGGBB` | `<bg#FF0000>text</bg#FF0000>` |

---

## Next Steps

### Implementation Tasks

1. **Parser**: Implement XML-Lite tag parser
2. **Style Stack**: Implement stack-based style management
3. **ANSI Generator**: Convert styles to ANSI codes
4. **Terminal Adaptation**: Integrate with TerminalCapabilities
5. **Validation**: Input sanitization and security
6. **Testing**: Unit tests for parser, edge cases
7. **Builder Tools**: In-game markup preview/validator
8. **Documentation**: In-game help files for builders

### Integration Points

- **Object system**: Short/long descriptions
- **Room system**: Room names, descriptions, extras
- **Character system**: Player names, titles
- **Social system**: Emote formatting
- **Help system**: Colored help files
- **OLC system**: Live preview of markup

---

*Document Version: 1.0*
*Last Updated: 2025-12-10*
*Status: Specification/Proposed*

# FieryMUD Color System Comparison

Quick comparison of all three color systems in FieryMUD.

---

## System Overview

| Feature | Legacy | Modern RichText | XML-Lite |
|---------|--------|-----------------|----------|
| **Target Use** | Old code | New C++ code | Player content |
| **Syntax** | `&1text&0` | `msg.colored("text", Color::Red)` | `<red>text</red>` |
| **Colors** | 16 ANSI | 16 + 256 + RGB | 16 + 256 + RGB |
| **Nesting** | ❌ No | ✅ Yes (builder) | ✅ Yes (tags) |
| **Type Safety** | ❌ Strings | ✅ C++ types | ⚠️ Runtime parsed |
| **Terminal Adapt** | ❌ No | ✅ Yes | ✅ Yes |
| **Status** | Deprecated | Recommended | Proposed |

---

## Text Attributes

| Attribute | Legacy | Modern | XML-Lite |
|-----------|--------|--------|----------|
| Bold/Bright | `&b` | `msg.bold("text")` | `<b>text</b>` |
| Dim/Faint | `&d` | `TextStyle::Dim` | `<dim>text</dim>` |
| Underline | `&u` | `msg.underline("text")` | `<u>text</u>` |
| Italic | N/A | `msg.italic("text")` | `<i>text</i>` |
| Strikethrough | N/A | `msg.strikethrough("text")` | `<s>text</s>` |
| Blink | N/A | `TextStyle::Blink` | `<blink>text</blink>` |
| Reverse | N/A | `TextStyle::Reverse` | `<reverse>text</reverse>` |
| Hidden | N/A | N/A | `<hide>text</hide>` |
| Newline | `&_` | `\n` | `\n` |

---

## Color Examples

### Simple Red Text

**Legacy**:
```
&1Red text&0
```

**Modern**:
```cpp
RichText msg;
msg.colored("Red text", Color::Red);
ctx.send_rich(msg);
```

**XML-Lite**:
```xml
<red>Red text</red>
```

---

### Bold Red Text

**Legacy**:
```
&b&1Bold red&0
```

**Modern**:
```cpp
TextFormat fmt;
fmt.foreground = Color::Red;
fmt.styles.push_back(TextStyle::Bold);
msg.formatted("Bold red", fmt);
// or simpler:
msg.colored("Bold red", Color::BrightRed);
```

**XML-Lite**:
```xml
<b:red>Bold red</b:red>
```

---

### Foreground + Background

**Legacy**:
```
&R&7White on red&0
```

**Modern**:
```cpp
msg.highlight("White on red", Color::White, BackgroundColor::Red);
```

**XML-Lite**:
```xml
<white:bg-red>White on red</white:bg-red>
```

---

### RGB Colors

**Legacy**:
```
Not supported (16 colors only)
```

**Modern**:
```cpp
msg.rgb("Orange text", ColorRGB{255, 165, 0});
```

**XML-Lite**:
```xml
<#FFA500>Orange text</#FFA500>
```

---

### 256-Color Palette

**Legacy**:
```
Not supported
```

**Modern**:
```cpp
msg.color256("Text", Color256{208});
```

**XML-Lite**:
```xml
<c208>Text</c208>
```

---

### Intensity Levels

**Legacy**:
```
&dDim red&0
&1Normal red&0
&b&1Bright red&0
```

**Modern**:
```cpp
// Dim
TextFormat dim_fmt;
dim_fmt.foreground = Color::Red;
dim_fmt.styles.push_back(TextStyle::Dim);
msg.formatted("Dim red", dim_fmt);

// Normal
msg.colored("Normal red", Color::Red);

// Bright
msg.colored("Bright red", Color::BrightRed);
```

**XML-Lite**:
```xml
<dim:red>Dim red</dim:red>
<red>Normal red</red>
<b:red>Bright red</b:red>
```

---

### Nesting

**Legacy**:
```
Not supported (tags don't nest)
&b&1Bold red &2green&0
Result: Bold red [reset] green [confusing]
```

**Modern**:
```cpp
RichText msg;
msg.bold("Bold ")
   .colored("red", Color::Red)
   .text(" ")
   .colored("green", Color::Green);
```

**XML-Lite**:
```xml
<b>Bold <red>red</red> <green>green</green></b>
```

---

## Use Cases

### Legacy System

**Use for**:
- Maintaining old code
- Existing world files
- Backward compatibility

**Don't use for**:
- New features
- Player-facing content

---

### Modern RichText API

**Use for**:
- New C++ code
- Server-side formatting
- Command output
- Combat messages
- System messages
- Progress bars, tables

**Example**:
```cpp
auto hp_bar = Format::health_bar(850, 1000, 20);
ctx.send_rich(hp_bar);

auto table = Format::table(headers, rows);
ctx.send_rich(table);
```

---

### XML-Lite Markup

**Use for**:
- Player-authored content
- Object descriptions
- Room descriptions
- Character names/titles
- OLC content
- Help files

**Example**:
```xml
A <item-legendary:#FFD700:b>legendary sword</item-legendary> lies here.
Its <dim:blue>faint blue glow</dim:blue> pulses with ancient magic.
```

---

## Migration Path

### Phase 1: Current State
- Legacy: `&` and `@` codes (deprecated)
- Modern: RichText API (recommended for code)

### Phase 2: XML-Lite Implementation
- Implement XML-Lite parser
- Add OLC support
- Builder documentation

### Phase 3: Content Migration
- Convert room descriptions
- Convert object descriptions
- Update help files
- Preserve legacy for old content

### Phase 4: Dual Support
- Both legacy and XML-Lite work
- Gradual migration
- Legacy eventually deprecated

---

## Recommendations

### For Developers
✅ Use **Modern RichText API** for all new C++ code
✅ Avoid legacy `&` and `@` codes
✅ Use semantic colors from `Colors::` namespace

### For Builders
✅ Use **XML-Lite** for descriptions and content
✅ Take advantage of RGB colors
✅ Use semantic tags (`<npc-name>`, `<item-rare>`)
✅ Experiment with intensity levels (`dim`, `b`)

### For Players
- See colored output based on terminal capabilities
- Automatic fallback for older terminals
- Consistent experience across all content

---

## Complete Feature Matrix

| Feature | Legacy | Modern | XML-Lite |
|---------|--------|--------|----------|
| **Basic 16 colors** | ✅ | ✅ | ✅ |
| **Bright colors** | ✅ | ✅ | ✅ (via `b`) |
| **256-color palette** | ❌ | ✅ | ✅ |
| **RGB/TrueColor** | ❌ | ✅ | ✅ |
| **Bold** | ✅ | ✅ | ✅ |
| **Dim** | ✅ | ✅ | ✅ |
| **Underline** | ✅ | ✅ | ✅ |
| **Italic** | ❌ | ✅ | ✅ |
| **Strikethrough** | ❌ | ✅ | ✅ |
| **Blink** | ❌ | ✅ | ✅ |
| **Reverse** | ❌ | ✅ | ✅ |
| **Hidden** | ❌ | ❌ | ✅ |
| **Background colors** | ✅ | ✅ | ✅ |
| **Nesting** | ❌ | ✅ | ✅ |
| **Semantic names** | ❌ | ✅ | ✅ |
| **Terminal adapt** | ❌ | ✅ | ✅ |
| **Type safety** | ❌ | ✅ | ❌ |
| **Player-editable** | ⚠️ | ❌ | ✅ |
| **Newline support** | ✅ `&_` | ✅ `\n` | ✅ `\n` |

---

*Document Version: 1.0*
*Last Updated: 2025-12-10*

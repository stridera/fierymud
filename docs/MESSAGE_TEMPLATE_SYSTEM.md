# Message Template System

This document describes FieryMUD's template system for creating dynamic, personalized messages. This system is used throughout the game for socials, combat messages, spell effects, and system notifications.

## Overview

The template system allows message authors to write templates with placeholders that get replaced at runtime with actual values. This enables:

- **Gender-aware pronouns** - Automatically uses he/she/they based on character gender
- **Dynamic names** - Inserts actor and target names
- **Rich color formatting** - XML-style color tags for visual variety
- **Custom variables** - Damage amounts, weapon names, spell names, etc.

## Template Variables

Variables are enclosed in curly braces: `{variable.property}`

### Entity Names

| Variable | Description |
|----------|-------------|
| `{actor}` | The name of the person performing the action |
| `{target}` | The name of the person receiving the action |
| `{object}` | The name of an item/object involved |

**Note:** `{actor}`, `{target}`, and `{object}` are shorthand for `{actor.name}`, `{target.name}`, and `{object.name}`. Both forms work identically.

**Entity Aliases**: The system recognizes multiple names for the same entity:

| Primary | Aliases |
|---------|---------|
| `actor` | `char`, `self`, `attacker`, `caster` |
| `target` | `vict`, `victim`, `defender` |
| `object` | `obj`, `item`, `weapon` |

### Pronouns

Pronouns automatically match the character's gender (Male, Female, or Neutral/Other).

#### Full Pronoun Syntax

```
{entity.pronoun.type}
```

| Type | Male | Female | Neutral |
|------|------|--------|---------|
| `subjective` | he | she | they |
| `objective` | him | her | them |
| `possessive` | his | her | their |
| `reflexive` | himself | herself | themselves |

**Examples:**
```
{actor.pronoun.subjective} smiles warmly.     -> "He smiles warmly." / "She smiles warmly."
You punch {target.pronoun.objective} hard.    -> "You punch him hard." / "You punch her hard."
{target.pronoun.possessive} eyes go wide.     -> "His eyes go wide." / "Her eyes go wide."
```

#### Pronoun Shortcuts

For convenience, shorter forms are available:

| Full Form | Shortcuts |
|-----------|-----------|
| `{actor.pronoun.subjective}` | `{actor.he}`, `{actor.she}`, `{actor.they}`, `{actor.subjective}`, `{actor.sub}` |
| `{actor.pronoun.objective}` | `{actor.him}`, `{actor.her}`, `{actor.them}`, `{actor.objective}`, `{actor.obj}` |
| `{actor.pronoun.possessive}` | `{actor.his}`, `{actor.hers}`, `{actor.their}`, `{actor.possessive}`, `{actor.pos}` |
| `{actor.pronoun.reflexive}` | `{actor.himself}`, `{actor.herself}`, `{actor.themselves}`, `{actor.reflexive}`, `{actor.ref}` |

The same shortcuts work for `{target.*}`.

**Note:** The shortcut you use (e.g., `{actor.he}`) doesn't determine the output - the character's actual gender does. Using `{actor.he}` on a female character will output "she".

### Custom Variables

These are set programmatically and vary by message type:

| Variable | Description | Example |
|----------|-------------|---------|
| `{damage}` | Numeric damage dealt | 42 |
| `{amount}` | Generic numeric value | 100 |
| `{weapon}` | Weapon name | "a sharp sword" |
| `{spell}` | Spell name | "fireball" |

Custom variables are defined by the code calling the template system and may vary based on context.

## Color Tags

Color tags use XML-style syntax: `<color>text</color>` or `<color>text</>`

### Standard Colors

```
<black>   <red>      <green>    <yellow>
<blue>    <magenta>  <cyan>     <white>
<gray>    <grey>
```

### Bright Colors

Prefix with `b:` for bright/bold variants:

```
<b:black>   <b:red>      <b:green>    <b:yellow>
<b:blue>    <b:magenta>  <b:cyan>     <b:white>
```

### Text Styles

```
<b>text</b>              Bold
<bold>text</bold>        Bold (alias)
<i>text</i>              Italic
<italic>text</italic>    Italic (alias)
<u>text</u>              Underline
<underline>text</>       Underline (alias)
<s>text</s>              Strikethrough
<strikethrough>text</>   Strikethrough (alias)
<dim>text</dim>          Dimmed
<blink>text</blink>      Blinking
<reverse>text</reverse>  Reverse video
```

### Semantic Colors

Use these for consistent game theming:

```
<damage>25 damage</>       Bright red - for damage numbers
<healing>+50 HP</>         Bright green - for healing
<info>You learn...</>      Bright blue - for information
<warning>Caution!</>       Bright yellow - for warnings
<error>Failed!</>          Bright red - for errors
<success>Success!</>       Bright green - for success messages
<mana>-10 mana</>          Bright cyan - for mana costs
<stamina>-5 stamina</>     Bright yellow - for stamina costs
<experience>+100 XP</>     Bright magenta - for experience gains
```

### Advanced Color Options

#### Hex Colors (24-bit true color)

```
<#FF0000>Red text</>
<#00FF00>Green text</>
<#0088FF>Custom blue</>
```

#### Indexed Colors (256-color palette)

```
<c196>Foreground color 196</>
<bgc21>Background color 21</>
```

#### Combined Styles

```
<b:red>Bold red text</>
<b:c196>Bold with indexed color</>
```

### Closing Tags

You can close tags in multiple ways:

```
<red>text</red>      Full closing tag
<red>text</>         Short closing tag (resets all formatting)
<red>text</red>      Named closing tag
```

The short form `</>` resets all formatting and is recommended for simplicity.

## Message Examples

### Social Messages

**Smile (no target):**
```
char_no_arg:    "You smile happily."
others_no_arg:  "{actor.name} smiles happily."
```

**Smile (with target):**
```
char_found:     "You smile at {target.name}."
vict_found:     "{actor.name} smiles at you."
others_found:   "{actor.name} smiles at {target.name}."
```

**Hug:**
```
char_found:     "You hug {target.name} warmly."
vict_found:     "{actor.name} hugs you warmly."
others_found:   "{actor.name} hugs {target.name} warmly."
```

### Combat Messages

**Hit message:**
```
to_actor:  "You hit {target.name} for <damage>{damage}</> damage!"
to_target: "{actor.name} hits you for <damage>{damage}</> damage!"
to_room:   "{actor.name} hits {target.name} with {actor.pos} {weapon}."
```

**Miss message:**
```
to_actor:  "You swing at {target.name} but miss!"
to_target: "{actor.name} swings at you but misses!"
to_room:   "{actor.name} swings at {target.name} but misses!"
```

**Critical hit:**
```
to_actor:  "<b:yellow>CRITICAL!</> You devastate {target.name} for <damage>{damage}</> damage!"
to_target: "<b:yellow>CRITICAL!</> {actor.name} devastates you for <damage>{damage}</> damage!"
to_room:   "<b:yellow>CRITICAL!</> {actor.name} devastates {target.name}!"
```

### Spell Messages

**Healing spell:**
```
to_actor:  "You cast cure light wounds on {target.name}, healing {target.him} for <healing>{amount}</> HP."
to_target: "{actor.name} casts cure light wounds on you. You feel <healing>{amount}</> HP restored!"
to_room:   "{actor.name} casts cure light wounds on {target.name}."
```

**Offensive spell:**
```
to_actor:  "Your fireball engulfs {target.name} in flames for <damage>{damage}</> damage!"
to_target: "{actor.name}'s fireball engulfs you in flames for <damage>{damage}</> damage!"
to_room:   "{actor.name}'s fireball engulfs {target.name} in flames!"
```

### System Messages

**Level up:**
```
"<b:yellow>Congratulations!</> You have advanced to level {level}!"
```

**Item pickup:**
```
"You pick up {object.name}."
```

**Death:**
```
to_actor:  "<b:red>You have been KILLED by {target.name}!</>"
to_killer: "<b:red>You have KILLED {target.name}!</>"
to_room:   "<b:red>{target.name} has been KILLED by {actor.name}!</>"
```

## Writing Guidelines for Builders

### Do's

1. **Always use template variables for names** - Never hardcode names
   ```
   Good: "{actor.name} waves."
   Bad:  "Someone waves."
   ```

2. **Use pronouns for natural language flow**
   ```
   Good: "{actor.name} draws {actor.pos} sword."
   Bad:  "{actor.name} draws the sword."
   ```

3. **Use semantic colors for game information**
   ```
   Good: "You deal <damage>{damage}</> damage."
   Bad:  "You deal <red>{damage}</red> damage."
   ```

4. **Keep messages concise but descriptive**

5. **Write three versions for targeted actions:**
   - `to_actor` - What the person doing the action sees
   - `to_target` - What the target sees
   - `to_room` - What everyone else sees

### Don'ts

1. **Don't assume gender**
   ```
   Good: "{target.name} looks at {target.ref} in the mirror."
   Bad:  "{target.name} looks at himself in the mirror."
   ```

2. **Don't nest color tags** - Close one before opening another
   ```
   Good: "<red>Fire</> and <blue>ice</>"
   Bad:  "<red>Fire <blue>ice</></>"
   ```

3. **Don't use colors excessively** - Reserve them for important information

4. **Don't include terminal ANSI codes directly** - Always use the tag system

## Technical Reference

### Code Usage

For developers integrating with the template system:

```cpp
#include "text/text_format.hpp"

// Simple formatting with actor and target
std::string msg = TextFormat::format(
    "{actor.name} attacks {target.name}!",
    attacker_ptr,
    victim_ptr
);

// Builder pattern for complex messages
auto msg = TextFormat::Message()
    .actor(attacker)
    .target(victim)
    .set("damage", 42)
    .set("weapon", "longsword")
    .format("You hit {target.name} with your {weapon} for <damage>{damage}</> damage!");

// Color-only formatting (no variables)
std::string colored = TextFormat::format_colors("<red>Warning!</> System message.");

// Plain text (no colors)
std::string plain = TextFormat::Message()
    .actor(player)
    .format_plain("{actor.name} has logged in.");
```

### Utility Functions

```cpp
// Check if a message has template variables
bool has_vars = TextFormat::has_variables(msg);  // looks for { and }

// Check if a message has color tags
bool has_cols = TextFormat::has_colors(msg);     // looks for < >

// Strip all color tags
std::string plain = TextFormat::strip_colors(msg);

// Capitalize first letter (handles ANSI codes)
std::string capped = TextFormat::capitalize(msg);
```

### Fallback Behavior

- **Unknown variables** remain as-is: `{unknown}` stays `{unknown}`
- **Missing actor** produces "someone" for name, neutral pronouns
- **Missing target** produces "someone" for name, neutral pronouns
- **Missing object** produces "something"

## Database Integration

Messages stored in the database should use this template format. When loaded, the game engine processes them through `TextFormat::format()` with the appropriate context.

**Database message columns typically include:**
- `to_actor` - Message shown to the person performing the action
- `to_target` - Message shown to the target of the action
- `to_room` - Message shown to others in the room

**Example database entry (AbilityMessages table):**
```
ability_id: 101
message_type: "hit"
to_actor: "Your {spell} strikes {target.name} for <damage>{damage}</> damage!"
to_target: "{actor.name}'s {spell} strikes you for <damage>{damage}</> damage!"
to_room: "{actor.name}'s {spell} strikes {target.name}!"
```

---

*Last updated: 2026-01-19*

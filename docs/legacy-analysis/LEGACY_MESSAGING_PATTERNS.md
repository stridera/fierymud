# Legacy Messaging Patterns Analysis

This document analyzes the messaging patterns from the legacy FieryMUD codebase to inform modern implementation improvements.

## Core Messaging System

### act() Function Pattern
The legacy system uses a sophisticated `act()` function for room-wide messaging:

```cpp
void act(std::string_view str, int hide_invisible, const CharData *ch, ActArg obj, ActArg vict_obj, int type);
```

**Message Types:**
- `TO_CHAR`: Message to the actor performing the action
- `TO_VICT`: Message to the target/victim of the action  
- `TO_ROOM`: Message to everyone else in the room

**Variable Substitution:**
- `$n`: Actor's name (subject)
- `$N`: Target's name (object)
- `$s`: Actor's possessive (his/her/its)
- `$S`: Target's possessive
- Color codes: `&0-&9` for colors, `&2&b` for bold green, etc.

### Example Legacy Spell Messages

```cpp
// Spell: Anthem of the Apocalypse  
char_printf(ch, "You let the anthem of the apocalypse ring!\n");
act("$n chants an anthem of demise and fatality!", false, ch, 0, 0, TO_ROOM);

// Spell: Armor (with target)
act("&6$n&6 calls upon &2&bGaia&0&6 to guard $N...&0", true, ch, 0, victim, TO_ROOM);
act("&6You call upon &2&bGaia&0&6 to guard $S...&0", false, ch, 0, victim, TO_CHAR);

// Spell: Confusion effect
act("$N turns on you in hatred and confusion!", false, ch, 0, tch, TO_CHAR);
act("You turn on $n in hatred and confusion!", false, ch, 0, tch, TO_VICT);
act("$N turns on $n in hatred and confusion!", false, ch, 0, tch, TO_ROOM);
```

## Social Command Structure

Legacy socials have a sophisticated message structure:

```cpp
struct social_messg {
    int act_nr;
    int hide;
    int min_victim_position;
    
    // No argument supplied
    char *char_no_arg;    // "You smile happily."
    char *others_no_arg;  // "$n smiles happily."
    
    // Target found
    char *char_found;     // "You smile at $N."
    char *others_found;   // "$n smiles at $N."
    char *vict_found;     // "$n smiles at you."
    
    // Target not found  
    char *not_found;      // "Smile at whom?"
};
```

## Modern vs Legacy Comparison

### Current Modern Implementation (Verbose)
```cpp
ctx.send("You smile happily.");
ctx.send_to_room(fmt::format("{} smiles happily.", ctx.actor->name()), true);

// With target
ctx.send(fmt::format("You smile at {}.", target->name()));
ctx.send_to_actor(target, fmt::format("{} smiles at you.", ctx.actor->name()));
ctx.send_to_room(fmt::format("{} smiles at {}.", ctx.actor->name(), target->name()), true);
```

### Legacy Implementation (Concise)
```cpp
act(soc_mess_list[act_nr].char_no_arg, false, ch, 0, 0, TO_CHAR);
act(soc_mess_list[act_nr].others_no_arg, false, ch, 0, 0, TO_ROOM);

// With target  
act(soc_mess_list[act_nr].char_found, false, ch, 0, vict, TO_CHAR);
act(soc_mess_list[act_nr].others_found, false, ch, 0, vict, TO_ROOM);
act(soc_mess_list[act_nr].vict_found, false, ch, 0, vict, TO_VICT);
```

## Architectural Recommendations

### 1. Enhanced CommandContext 
Add act()-style messaging methods to CommandContext:

```cpp
// Proposed additions to CommandContext
void act(std::string_view message, ActTarget target) const;
void act_to_char(std::string_view message) const;
void act_to_room(std::string_view message, bool exclude_actor = true) const;
void act_to_target(std::shared_ptr<Actor> target, std::string_view message) const;
```

### 2. Message Template System
Create a message template system for consistent formatting:

```cpp
struct SocialMessage {
    std::string to_actor;      // "You smile happily."
    std::string to_room;       // "$n smiles happily."  
    std::string to_target;     // "$n smiles at you."
    std::string to_room_with_target; // "$n smiles at $N."
};
```

### 3. Variable Substitution
Implement variable substitution similar to legacy:
- `$n` → actor name
- `$N` → target name  
- `$s/$S` → possessive forms
- Color code support

### 4. Spell/Ability Enhancement
Enhance spell and ability output with:
- Rich color coding for visual appeal
- Contextual messages (different for self vs others)
- Room-wide atmospheric effects
- Target-specific feedback

## Implementation Priority

1. **High**: Enhanced CommandContext with act() methods
2. **Medium**: Social message template system  
3. **Medium**: Variable substitution in messages
4. **Low**: Color code system (can use existing fmt colors)

## Examples for Enhancement

### Current Social Command
```cpp  
Result<CommandResult> cmd_smile(const CommandContext& ctx) {
    ctx.send("You smile happily.");
    ctx.send_to_room(fmt::format("{} smiles happily.", ctx.actor->name()), true);
    return CommandResult::Success;
}
```

### Enhanced Social Command
```cpp
Result<CommandResult> cmd_smile(const CommandContext& ctx) {
    ctx.act_to_char("You smile happily.");
    ctx.act_to_room("$n smiles happily.");
    return CommandResult::Success;
}
```

The enhanced version is more concise, consistent, and easier to maintain while providing the same functionality as the legacy system.
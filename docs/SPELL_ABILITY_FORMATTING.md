# Spell and Ability Output Formatting Reference

Based on analysis of legacy FieryMUD spell/ability systems, this document outlines the formatting patterns that should be used in the modern system for consistency.

## Legacy System Patterns

### Message Distribution Pattern
The legacy system uses the `act()` function with different targets to provide appropriate messages to different participants:

```cpp
// Spell casting example from legacy spells.cpp
act("&6$n&6 calls upon &2&bGaia&0&6 to guard $s body...&0", true, ch, 0, 0, TO_ROOM);
act("&6You call upon &2&bGaia&0&6 to guard your body...&0", false, ch, 0, 0, TO_CHAR);

// Banishment spell example
act("You look at $N&0 and shout, '&1&bI banish thee!&0'", false, ch, 0, victim, TO_CHAR);
act("$n&0 looks at $N&0 and shouts, '&1&bI banish thee!&0'", false, ch, 0, victim, TO_NOTVICT);
act("$n&0 looks at you and shouts, '&1&bI banish thee!&0'", false, ch, 0, victim, TO_VICT);
```

### Message Target Types
- **TO_CHAR**: Message sent to the caster/actor
- **TO_VICT**: Message sent to the target/victim
- **TO_ROOM**: Message sent to everyone in the room except caster
- **TO_NOTVICT**: Message sent to everyone except caster and victim

### Color Code System
The legacy system uses `&` codes for colors:
- `&0` - Normal/reset color
- `&1` - Red
- `&2` - Green  
- `&6` - Brown/yellow
- `&9` - Blue
- `&b` - Bold/bright modifier

Examples:
- `&2&b` - Bright green (for nature/earth spells)
- `&6` - Brown (for earth/nature descriptions)
- `&1&b` - Bright red (for dramatic effects)
- `&9&b` - Bright blue (for banishment effects)

### Formatting Patterns

#### Spell Casting
```cpp
// Self-cast spell
act("&6You call upon &2&bGaia&0&6 to guard your body...&0", false, ch, 0, 0, TO_CHAR);
act("&6$n&6 calls upon &2&bGaia&0&6 to guard $s body...&0", true, ch, 0, 0, TO_ROOM);

// Target spell
act("&6You call upon &2&bGaia&0&6 to guard $S...&0", false, ch, 0, victim, TO_CHAR);
act("&6$n&6 calls upon &2&bGaia&0&6 to guard $N...&0", true, ch, 0, victim, TO_ROOM);
```

#### Combat Actions
```cpp
act("$n's blow shatters the magic paralyzing you!", false, ch, 0, victim, TO_VICT);
act("Your blow disrupts the magic keeping $N frozen.", false, ch, 0, victim, TO_CHAR);
act("$n's attack frees $N from magic which held $M motionless.", false, ch, 0, victim, TO_NOTVICT);
```

#### Area Effects
```cpp
act("&2&bYou conjure a thick corrosive fog!&0", false, ch, 0, victim, TO_CHAR);
act("&2&b$n conjures a thick corrosive fog!&0", false, ch, 0, victim, TO_ROOM);
```

## Modern System Recommendations

### Command Context Integration
The modern system should use the `CommandContext` API for consistent message distribution:

```cpp
// Self-targeted spell
ctx.send("&6You call upon &2&bGaia&0&6 to guard your body...&0");
ctx.send_to_room(fmt::format("&6{} calls upon &2&bGaia&0&6 to guard {} body...&0", 
                            ctx.actor->name(), ctx.actor->pronoun_possessive()), true);

// Targeted spell
ctx.send(fmt::format("&6You call upon &2&bGaia&0&6 to guard {}...&0", target->name()));
ctx.send_to_actor(target, fmt::format("&6{} calls upon &2&bGaia&0&6 to guard you...&0", 
                                     ctx.actor->name()));
ctx.send_to_room(fmt::format("&6{} calls upon &2&bGaia&0&6 to guard {}...&0", 
                            ctx.actor->name(), target->name()), true);
```

### Color System Modernization
Consider implementing a modern color system that maintains compatibility with legacy codes while adding modern features:

```cpp
namespace Colors {
    constexpr std::string_view RESET = "&0";
    constexpr std::string_view RED = "&1";
    constexpr std::string_view GREEN = "&2";
    constexpr std::string_view BROWN = "&6";
    constexpr std::string_view BLUE = "&9";
    constexpr std::string_view BOLD = "&b";
    
    // Nature/Earth theme
    constexpr std::string_view EARTH = "&6";
    constexpr std::string_view NATURE = "&2&b";
    
    // Magic themes
    constexpr std::string_view ARCANE = "&9&b";
    constexpr std::string_view DIVINE = "&6";
    constexpr std::string_view DRAMATIC = "&1&b";
}
```

### Spell/Ability Template Functions
Create template functions for common spell patterns:

```cpp
// Self-enhancement spell pattern
void cast_self_enhancement(const CommandContext& ctx, std::string_view spell_name, 
                          std::string_view color_theme = Colors::NATURE) {
    ctx.send(fmt::format("{}You cast {} on yourself.{}", 
                        color_theme, spell_name, Colors::RESET));
    ctx.send_to_room(fmt::format("{}{} casts {} on {}.{}", 
                                color_theme, ctx.actor->name(), spell_name, 
                                ctx.actor->pronoun_reflexive(), Colors::RESET), true);
}

// Offensive spell pattern  
void cast_offensive_spell(const CommandContext& ctx, std::shared_ptr<Actor> target,
                         std::string_view spell_name, std::string_view incantation = "",
                         std::string_view color_theme = Colors::ARCANE) {
    if (!incantation.empty()) {
        ctx.send(fmt::format("You shout, '{}{}{}'{}", 
                           Colors::DRAMATIC, incantation, Colors::RESET));
        ctx.send_to_room(fmt::format("{} shouts, '{}{}{}'{}", 
                                   ctx.actor->name(), Colors::DRAMATIC, 
                                   incantation, Colors::RESET), false);
    }
    
    ctx.send(fmt::format("{}You cast {} at {}.{}", 
                        color_theme, spell_name, target->name(), Colors::RESET));
    ctx.send_to_actor(target, fmt::format("{}{} casts {} at you!{}", 
                                         color_theme, ctx.actor->name(), 
                                         spell_name, Colors::RESET));
    ctx.send_to_room(fmt::format("{}{} casts {} at {}.{}", 
                                color_theme, ctx.actor->name(), spell_name, 
                                target->name(), Colors::RESET), true);
}
```

## Implementation Guidelines

1. **Consistency**: All spells/abilities should follow the same messaging patterns
2. **Color Themes**: Use consistent color themes for similar types of magic
3. **Perspective**: Always provide appropriate messages for caster, target, and observers
4. **Immersion**: Use descriptive, flavorful text that enhances the fantasy atmosphere
5. **Legacy Compatibility**: Maintain compatibility with legacy color codes
6. **Modern Integration**: Use the CommandContext API for all message distribution

This approach ensures consistency with the legacy system while leveraging modern C++ patterns and the enhanced command context system.
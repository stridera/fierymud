# FieryMUD Effect Composition System - Summary

> **DEPRECATION NOTICE**: This document describes the legacy 15-effect system.
> The effect system is being consolidated to 18 effects with a cleaner, more orthogonal design.
> See the Effect System Consolidation Plan for the new system design.
> This document is retained for historical reference only.

This document summarizes the composable effect system extracted from the FieryMUD C++ codebase and applied to ability documentation.

## What Was Done

### 1. Effect Type System Created

Defined 15 core effect types that can be composed to describe any ability:

1. **DAMAGE** - Apply damage by formula, respecting resistances
2. **HEAL** - Restore HP by formula
3. **APPLY_AFFECT** - Apply a timed aura (buff/debuff)
4. **REMOVE_AFFECT** - Remove one or more auras
5. **SUMMON** - Spawn mob(s) in same room
6. **TELEPORT** - Move target to another room
7. **MESSAGE** - Send custom messages
8. **RESOURCE_CHANGE** - Affect mana/stamina/movement
9. **DISPEL** - Remove auras selectively
10. **CONDITION** - Scripted check that gates other effects
11. **MULTI_HIT** - Multiple damage instances
12. **AREA_EFFECT** - Affects multiple targets in room
13. **CREATE_OBJECT** - Create and place/equip object
14. **MODIFY_OBJECT** - Change object properties
15. **DELAYED_EFFECT** - Event-based recurring/delayed effects

### 2. Documentation Enhanced

Created **ABILITY_EFFECTS_ENHANCEMENT.md** with detailed effect compositions for representative abilities:

**Damage Spells**: Fireball, Magic Missile, Harm
- Extracted actual damage formulas from `magic.cpp::sorcerer_single_target()`
- Documented saving throw mechanics
- Described resistance/susceptibility system (0-200%)
- Detailed globe evasion and immunity checks

**Buff Spells**: Bless, Stone Skin
- Documented effect flags (EFF_BLESS, EFF_STONE_SKIN)
- Extracted duration formulas
- Described stat modifiers (hitroll, AC, saves)
- Explained wear-off messages

**Healing Spells**: Cure Light, Cure Critic, Heal
- Documented healing formulas (1d8+5, 3d8+15, 100+3d8)
- Described additional effects (poison/disease removal)
- Noted HP caps

**Debuff Spells**: Blindness, Sleep
- Documented effect flags and durations
- Extracted penalty modifiers
- Described saving throw mechanics

**Utility Spells**: Cure Blind, Remove Poison, Dispel Magic
- Documented removal mechanics
- Described dispel logic for violent spells

**Teleport Spells**: Word of Recall, Teleport, Summon
- Documented destination calculation
- Described room restriction checks
- Noted combat/fighting restrictions

**Summon Spells**: Animate Dead, Summon
- Documented mob VNUM selection
- Described follower mechanics
- Noted corpse consumption

**Area Effect Spells**: Earthquake, Chain Lightning
- Documented target selection (all enemies)
- Extracted damage formulas per target
- Described special mechanics (flying, arcing)

**Object Creation**: Flame Blade, Create Food, Create Water
- Documented object VNUMs
- Described auto-equip mechanics
- Noted object property modification

**Delayed Effects**: Acid Fog, Pyre
- Documented recurring damage mechanics
- Described event queue system
- Noted round-by-round progression

**Skills**: Backstab, Bash, Kick, Disarm, Hide, Sneak, Rescue, Parry, Dodge
- Extracted damage multipliers (backstab 2-5×)
- Documented knockdown mechanics (bash)
- Described stealth flags (hide/sneak)
- Noted defensive passive checks

### 3. CSV Enhancement

Enhanced **abilities.csv** with new `effectTypes` column:

- **368 abilities processed**
- Automatic effect type assignment based on:
  - Category (DAMAGE, BUFF, UTILITY, etc.)
  - Damage type (FIRE, COLD, SHOCK, etc.)
  - Target type (SINGLE, AREA, etc.)
  - Name patterns (heal, cure, summon, teleport, etc.)

**Example Entries**:
```csv
26,SPELL,fireball,Fireball,DAMAGE,SINGLE_ENEMY,SAME_ROOM,FIRE,sorcerer_single_target(...),STANDING,true,true,,DAMAGE
32,SPELL,magic missile,Magic Missile,UTILITY,SINGLE_ENEMY,SAME_ROOM,PHYSICAL_BLUNT,roll_dice(4,21),STANDING,true,true,,MULTI_HIT,DAMAGE
27,SPELL,harm,Harm,DAMAGE,SINGLE_ENEMY,TOUCH,NONE,pow(skill,2)*41/5000,STANDING,true,true,,DAMAGE,CONDITION,HEAL,DAMAGE
3,SPELL,bless,Bless,BUFF,SINGLE_CHARACTER,SAME_ROOM,NONE,,SITTING,false,false,,APPLY_AFFECT
401,SKILL,backstab,Backstab,STEALTH,NONE,TOUCH,NONE,,STANDING,true,true,ASSASSIN:1;...,CONDITION,DAMAGE
```

### 4. Implementation References

Documented key implementation files and functions:

**Core Magic System** (`magic.cpp`):
- `mag_damage()` - All damage spells
- `mag_affect()` - All buff/debuff spells
- `mag_point()` - Healing and resource changes
- `mag_unaffect()` - Effect removal
- `mag_summon()` - Summon mechanics
- `evades_spell()` - Globe/immunity checks
- `mag_savingthrow()` - Saving throw resolution
- `susceptibility()` - Resistance calculation

**Complex Spells** (`spells.cpp`):
- `ASPELL(spell_name)` - Custom multi-effect spells
- Event-based recurring effects (acid_fog, pyre, soul_tap)

**Combat Skills** (`act.offensive.cpp`):
- `do_backstab()`, `do_bash()`, `do_kick()`, `do_disarm()`

**Utility Skills** (`act.other.cpp`):
- `do_hide()`, `do_steal()`, `do_bandage()`

**Movement Skills** (`act.movement.cpp`):
- `do_mount()`, `do_tame()`

## Key Formulas Extracted

### Damage Formulas

**Sorcerer Single-Target** (by circle):
```cpp
// Circle 1: roll_dice(4, 19) + pow(power, exponent)
// Circle 3: roll_dice(4, 24) + pow(power, exponent)
// Circle 6: roll_dice(10, 24) + pow(power, exponent)
// Circle 9: roll_dice(10, 35) + pow(power, exponent)

// Exponent scales with level:
exponent = 1.2 + 0.3 * minlevel/100.0 + (power - minlevel) * (0.004 * minlevel - 0.2)/100.0
```

**Scaling Damage**:
```cpp
// Earthquake: (pow(skill, 2) * 7) / 400
// Chain Lightning: (pow(skill, 2) * 7) / 500
// Harm: (pow(skill, 2) * 41) / 5000
```

### Healing Formulas

```cpp
// Cure Light: 1d8 + 5
// Cure Serious: 2d8 + 10
// Cure Critic: 3d8 + 15
// Heal: 100 + 3d8
```

### Resistance Modifiers

```cpp
// susceptibility(victim, damage_type):
// 0%: Complete immunity (spell evaded)
// 50%: Resistant (half damage)
// 100%: Normal damage
// 150%: Vulnerable (1.5× damage)
// 200%: Highly vulnerable (2× damage)
```

### Backstab Multipliers

```cpp
// Skill-based damage multipliers:
// 0-200: 2× weapon damage
// 201-400: 3× weapon damage
// 401-600: 4× weapon damage
// 601+: 5× weapon damage
```

## Usage Examples

### Query Abilities by Effect Type

```bash
# All damage spells
grep "DAMAGE" abilities.csv

# All multi-hit abilities
grep "MULTI_HIT" abilities.csv

# All teleport effects
grep "TELEPORT" abilities.csv

# Complex abilities with conditions
grep "CONDITION" abilities.csv
```

### Filter Composite Effects

```csv
# Harm (heals undead, damages living):
27,SPELL,harm,Harm,DAMAGE,SINGLE_ENEMY,TOUCH,NONE,...,DAMAGE,CONDITION,HEAL,DAMAGE

# Magic Missile (multiple damage instances):
32,SPELL,magic missile,Magic Missile,UTILITY,SINGLE_ENEMY,SAME_ROOM,PHYSICAL_BLUNT,...,MULTI_HIT,DAMAGE
```

## Integration with Game Systems

### Globe Protection

- **Minor Globe**: Blocks spells ≤ Circle 3
- **Major Globe**: Blocks spells ≤ Circle 6
- Implementation: `evades_spell()` checks globe flags before damage

### Saving Throws

- **SAVING_SPELL**: Most magical effects
- **SAVING_PARA**: Paralysis effects
- **SAVING_BREATH**: Breath weapons
- **SAVING_ROD**: Staff/wand effects
- **SAVING_PETRI**: Petrification
- Formula: `max(1, base_save + modifiers) < random_number(0, 99)`

### Effect Duration

```cpp
// Bless: 6 hours
// Stone Skin: 2 hours
// Blindness: 2-4 rounds
// Sleep: 4 + level/4 rounds
// Hide/Sneak: Until detected or movement
```

### Event System

Delayed effects use event queue:
```cpp
event_create(EVENT_SPELL, spell_recur_func, victim, 4 SECS);
// Repeats every 4 seconds for specified rounds
```

## Files Created/Modified

1. **ABILITY_EFFECTS_ENHANCEMENT.md** (NEW)
   - Detailed effect compositions for 30+ representative abilities
   - Actual formulas extracted from C++ code
   - Implementation file references

2. **abilities.csv** (ENHANCED)
   - Added `effectTypes` column
   - 368 abilities with effect type annotations
   - Backup saved as `abilities_original.csv`

3. **add_effect_types.py** (NEW)
   - Python script for automatic effect type assignment
   - Rule-based logic using category, damage type, and name patterns
   - Located in `/home/strider/Code/mud/fierymud/scripts/`

4. **EFFECT_SYSTEM_SUMMARY.md** (NEW - this file)
   - Overview of effect composition system
   - Key formulas and mechanics
   - Usage examples and integration notes

## Next Steps

### For Game Development

1. **Effect Templates**: Create reusable effect templates for common patterns
2. **Effect Builder**: Develop tools to compose new abilities from effect types
3. **Balance Tool**: Analyze damage/healing formulas across all abilities
4. **Documentation Generator**: Auto-generate ability descriptions from effect compositions

### For Documentation

1. **Complete Coverage**: Add detailed effect compositions for all 368 abilities
2. **Visual Diagrams**: Create flowcharts for complex multi-effect abilities
3. **Formula Analysis**: Statistical analysis of damage scaling across levels
4. **Effect Interactions**: Document how effects combine and interact

### For Database Integration

1. **Prisma Schema**: Add `effectTypes` field to Abilities model
2. **GraphQL API**: Expose effect type filtering in ability queries
3. **Muditor UI**: Display effect compositions in visual editor
4. **Effect Search**: Filter abilities by effect type in admin interface

## Conclusion

This effect composition system provides a structured, composable framework for understanding and documenting FieryMUD abilities. By extracting actual implementation details from C++ code, we've created accurate, maintainable documentation that can be used for:

- **Game balance**: Understanding damage/healing formulas
- **Player documentation**: Clear ability descriptions
- **Development**: Templates for new abilities
- **Database integration**: Structured ability metadata

All effect types are grounded in actual code implementation, with references to specific C++ functions and formulas. This ensures documentation accuracy and provides a foundation for future enhancements.

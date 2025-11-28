# FieryMUD Mob Stat Calculation Documentation

This document outlines how mob statistics are calculated during the loading process in `legacy/src/db.cpp`. This information is intended to help migrate from dice-based calculations to hard-coded values in the Python mob parser.

## Overview

When a mob is loaded, the parser reads basic dice specifications from the mob file and then calculates final statistics using complex formulas that consider:
- Mob level
- Race factors  
- Class factors
- Zone factors
- Additional modifiers

The goal is to pre-calculate these values in Python and store them directly instead of relying on dice rolls.

## File Format Parse Flow

The mob loading process follows this sequence in `parse_mobile()`:

1. Read string data (keywords, descriptions)
2. Read flags, alignment, and mob type ('S' for simple)
3. Call `parse_simple_mob()` which reads:
   - **First line**: `level hitroll ac hp_dice damage_dice`
     - Format: `level hitroll ac #d#+# #d#+#`
     - Example: `25 5 -10 8d10+200 3d6+5`
   - **Second line**: Money values 
   - **Third line**: Position, default_pos, gender, class, race, race_align, size

## Stat Calculation Functions

### Hit Points Calculation

**Function**: `get_set_hit(level, race, class_num, state)`

**Base HP Calculation by Level**:
```cpp
if (level < 20)
    xmain = 3 * (level * (level / 1.25))
else if (level < 35)  
    xmain = 3 * (level * (level / 1.35))
else if (level < 50)
    xmain = 3 * (level * level / 1.25)
else if (level >= 50)
    xmain = 3 * (level * level / 1.25)
```

**Level-based Adjustments**:
```cpp
if (level <= 5)
    face = 1
else if (level <= 10) {
    xmain -= 25
    face = 5
} else if (level <= 20) {
    xmain -= 100  
    face = 10
} else if (level <= 30) {
    xmain -= 200
    face = 20
} else {
    xmain -= 2000
    face = 200
}
```

**Factor Application**:
```cpp
cfactor = CLASS_HITFACTOR(class_num)  // Class modifier (percentage)
sfactor = RACE_HITFACTOR(race)        // Race modifier (percentage)
combined_factor = (sfactor + cfactor) / 2
xmain = (combined_factor * xmain) / 100
```

**Final HP Calculation**:
```cpp
final_hp = xmain / (2 - (level / 100.0))
```

**Current Implementation in parse_simple_mob**:
```cpp
mob_proto[i].points.hit = 7 + mob_proto[i].mob_specials.ex_no_dice
GET_EX_MAIN_HP(mob_proto + i) = get_set_hit(level, race, class, 1)
```

### Hit Roll Calculation

**Function**: `get_set_hd(level, race, class_num, state=0)` for hit roll

**Base Hit Roll by Level**:
```cpp
if (level < 10)
    hit = level / 2.0
else if (level < 24)
    hit = level / 2.4  
else if (level < 32)
    hit = level / 2.6
else if (level < 50)
    hit = level / 2.8
else if (level < 62)
    hit = level / 3.0
else if (level < 75)
    hit = level / 3.2
else if (level < 82)
    hit = level / 3.4
else if (level >= 90)
    hit = level / 3.6
```

**Factor Application**:
```cpp
cfactor = CLASS_HDFACTOR(class_num)
sfactor = RACE_HDFACTOR(race)
combined_factor = (sfactor + cfactor) / 2
hit = (combined_factor * hit) / 100
```

**Current Implementation**:
```cpp
mob_proto[i].points.hitroll = clamp(get_set_hd(level, race, class, 0), 10, 80)
mob_proto[i].points.hitroll += mob_proto[i].mob_specials.ex_hitroll
```

### Damage Roll Calculation

**Function**: `get_set_hd(level, race, class_num, state=1)` for damage roll

**Base Damage Roll by Level**:
```cpp
if (level < 10)
    dam = level / 4.0
else if (level < 20)
    dam = level / 4.0
else if (level < 35)
    dam = level / 4.3
else if (level < 50)
    dam = level / 4.6
else if (level >= 50)
    dam = level / 4.4
```

**Factor Application**: Same as hit roll calculation

**Current Implementation**:
```cpp
mob_proto[i].points.damroll = get_set_hd(level, race, class, 1)
mob_proto[i].points.damroll += mob_proto[i].mob_specials.ex_damroll
```

### Damage Dice Calculation

**Function**: `get_set_dice(level, race, class_num, state)`

**Number of Dice (state=0)**:
```cpp
if (level < 10)
    dice = max(1, (level / 3) + 0.5)
else if (level < 30)
    dice = (level / 3) + 0.5
else if (level <= 50)
    dice = (level / 3) + 0.5
else if (level > 50)
    dice = (level / 2.5) + 0.5
```

**Dice Size (state=1)**:

```cpp
if (level < 10)
    face = 3
else if (level < 26)
    face = 4
else if (level < 36)
    face = 4
else if (level <= 50)
    face = 5
else if (level > 50 && level <= 60)
    face = 8
else if (level > 60)
    face = 10
```

**Current Implementation**:
```cpp
mob_proto[i].mob_specials.damnodice = get_set_dice(level, race, class, 0) + mob_proto[i].mob_specials.ex_damnodice
mob_proto[i].mob_specials.damsizedice = get_set_dice(level, race, class, 1) + mob_proto[i].mob_specials.ex_damsizedice
```

### Armor Class Calculation

**Function**: `get_ac(level, race, class_num)`

**AC Calculation**:
```cpp
sfactor = RACE_ACFACTOR(race)  // Class factor currently disabled
ac = 90 - (2 * level * (sfactor / 100.0))
ac = clamp(ac, -100, 100)
```

**Current Implementation**:
```cpp
if (mob_proto[i].mob_specials.ex_armor != 100)
    mob_proto[i].points.armor = mob_proto[i].mob_specials.ex_armor + get_ac(level, race, class)
else
    mob_proto[i].points.armor = get_ac(level, race, class)
```

### Experience Points Calculation

**Function**: `get_set_exp(level, race, class_num, zone)`

**Base Experience by Level**:
```cpp
if (level < 50)
    exp = (level * level * level) + 1000
else if (level >= 50)
    exp = (level * level * 50) + 1000
```

**Factor Application**:
```cpp
cfactor = CLASS_EXPFACTOR(class_num)
sfactor = RACE_EXPFACTOR(race)
zfactor = zone_table[zone_index].zone_factor
combined_factor = (sfactor + zfactor + cfactor) / 3
exp = (combined_factor * exp) / 100
```

## Factor Constants

The calculation functions reference various factor constants that need to be available:

- `CLASS_HITFACTOR(class)` - Class hit point modifier
- `CLASS_HDFACTOR(class)` - Class hit/damage roll modifier  
- `CLASS_DICEFACTOR(class)` - Class damage dice modifier
- `CLASS_EXPFACTOR(class)` - Class experience modifier
- `CLASS_COPPERFACTOR(class)` - Class money modifier
- `RACE_HITFACTOR(race)` - Race hit point modifier
- `RACE_HDFACTOR(race)` - Race hit/damage roll modifier
- `RACE_DICEFACTOR(race)` - Race damage dice modifier
- `RACE_EXPFACTOR(race)` - Race experience modifier
- `RACE_COPPERFACTOR(race)` - Race money modifier
- `RACE_ACFACTOR(race)` - Race armor class modifier

## Current Python Parser Issues

Looking at `scripts/mud/types/mob.py`, the current implementation:

1. **Line 78**: Parses HP dice but doesn't calculate final HP values
2. **Line 84**: Parses damage dice but doesn't calculate final damage values
3. **Missing**: No calculation of hit roll, damage roll, AC, or experience
4. **Missing**: No access to race/class factor constants

## Recommended Migration Steps

To replace dice with calculated values:

1. **Extract Factor Constants**: Create Python constants or lookup tables for all the `*_FACTOR` macros
2. **Implement Calculation Functions**: Port the C++ calculation functions to Python
3. **Modify Mob Dataclass**: Replace `hp_dice` and `damage_dice` with calculated values:
   - `max_hp: int` (calculated hit points)
   - `hit_roll: int` (calculated hit roll)
   - `damage_roll: int` (calculated damage roll)  
   - `damage_dice_num: int` (calculated damage dice number)
   - `damage_dice_size: int` (calculated damage dice size)
   - `armor_class: int` (calculated AC)
   - `experience: int` (calculated XP)

4. **Update Parser**: Modify the `parse()` method to call calculation functions instead of storing raw dice

This will eliminate the need for runtime dice rolling and ensure consistent mob statistics.
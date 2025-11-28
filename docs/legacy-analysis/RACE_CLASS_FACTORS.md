# FieryMUD Race and Class Factor Reference

This document provides a complete reference of all race and class factor values used in mob stat calculations in FieryMUD. These factors are applied to base calculations to modify mob statistics based on their race and class.

## Race Factors

All races from `legacy/src/races.cpp` with their modifier factors:

| ID | Race Name | exp_factor | hit_factor | hd_factor | dice_factor | copper_factor | ac_factor |
|----|-----------|------------|------------|-----------|-------------|---------------|-----------|
| 0 | HUMAN | 100 | 100 | 100 | 100 | 75 | 100 |
| 1 | ELF | 100 | 100 | 100 | 100 | 75 | 100 |
| 2 | GNOME | 100 | 100 | 100 | 100 | 75 | 100 |
| 3 | DWARF | 100 | 100 | 100 | 100 | 75 | 100 |
| 4 | TROLL | 100 | 130 | 110 | 110 | 75 | 100 |
| 5 | DROW | 100 | 100 | 100 | 100 | 75 | 100 |
| 6 | DUERGAR | 100 | 100 | 100 | 100 | 75 | 100 |
| 7 | OGRE | 100 | 110 | 100 | 120 | 75 | 85 |
| 8 | ORC | 100 | 100 | 100 | 100 | 75 | 100 |
| 9 | HALF_ELF | 100 | 100 | 100 | 100 | 75 | 100 |
| 10 | BARBARIAN | 100 | 100 | 100 | 100 | 75 | 100 |
| 11 | HALFLING | 100 | 100 | 100 | 100 | 75 | 100 |
| 12 | PLANT | 100 | 100 | 100 | 100 | 75 | 120 |
| 13 | HUMANOID | 100 | 100 | 100 | 100 | 100 | 60 |
| 14 | ANIMAL | 100 | 100 | 100 | 100 | 0 | 65 |
| 15 | DRAGON_GENERAL | 130 | 130 | 140 | 140 | 500 | 140 |
| 16 | GIANT | 110 | 120 | 120 | 100 | 125 | 120 |
| 17 | OTHER | 80 | 110 | 120 | 80 | 75 | 105 |
| 18 | GOBLIN | 60 | 60 | 60 | 60 | 75 | 90 |
| 19 | DEMON | 120 | 120 | 120 | 120 | 150 | 120 |
| 20 | BROWNIE | 100 | 100 | 100 | 100 | 75 | 100 |
| 21 | DRAGON_FIRE | 130 | 130 | 140 | 140 | 500 | 140 |
| 22 | DRAGON_FROST | 130 | 130 | 140 | 140 | 500 | 140 |
| 23 | DRAGON_ACID | 130 | 130 | 140 | 140 | 500 | 140 |
| 24 | DRAGON_LIGHTNING | 130 | 130 | 140 | 140 | 500 | 140 |
| 25 | DRAGON_GAS | 130 | 130 | 140 | 140 | 500 | 140 |
| 26 | DRAGONBORN_FIRE | 100 | 100 | 100 | 100 | 75 | 100 |
| 27 | DRAGONBORN_FROST | 100 | 100 | 100 | 100 | 75 | 100 |
| 28 | DRAGONBORN_ACID | 100 | 100 | 100 | 100 | 75 | 100 |
| 29 | DRAGONBORN_LIGHTNING | 100 | 100 | 100 | 100 | 75 | 100 |
| 30 | DRAGONBORN_GAS | 100 | 100 | 100 | 100 | 75 | 100 |
| 31 | SVERFNEBLIN | 100 | 100 | 100 | 100 | 75 | 100 |
| 32 | FAERIE_SEELIE | 100 | 80 | 100 | 100 | 75 | 100 |
| 33 | FAERIE_UNSEELIE | 100 | 80 | 100 | 100 | 75 | 100 |
| 34 | NYMPH | 100 | 100 | 100 | 100 | 75 | 100 |
| 35 | ARBOREAN | 100 | 100 | 100 | 100 | 75 | 100 |

## Class Factors

All classes from `legacy/src/class.cpp` with their modifier factors:

| ID | Class Name     | exp_factor | hit_factor | hd_factor | dice_factor | copper_factor | ac_factor |
|----|----------------|------------|------------|-----------|-------------|---------------|-----------|
| 0  | SORCERER       | 120        | 80         | 80        | 60          | 100           | 75        |
| 1  | CLERIC         | 100        | 80         | 80        | 70          | 100           | 100       |
| 2  | THIEF          | 100        | 90         | 100       | 100         | 100           | 80        |
| 3  | WARRIOR        | 100        | 120        | 120       | 120         | 100           | 120       |
| 4  | PALADIN        | 100        | 120        | 120       | 120         | 100           | 120       |
| 5  | ANTI_PALADIN   | 100        | 120        | 120       | 120         | 100           | 120       |
| 6  | RANGER         | 100        | 120        | 120       | 120         | 100           | 120       |
| 7  | DRUID          | 100        | 80         | 80        | 70          | 100           | 100       |
| 8  | SHAMAN         | 100        | 100        | 100       | 100         | 100           | 100       |
| 9  | ASSASSIN       | 100        | 90         | 100       | 100         | 100           | 80        |
| 10 | MERCENARY      | 100        | 90         | 100       | 100         | 100           | 80        |
| 11 | NECROMANCER    | 120        | 80         | 80        | 60          | 100           | 75        |
| 12 | CONJURER       | 120        | 80         | 80        | 60          | 100           | 75        |
| 13 | MONK           | 100        | 120        | 120       | 120         | 100           | 120       |
| 14 | BERSERKER      | 100        | 120        | 120       | 120         | 100           | 120       |
| 15 | PRIEST         | 100        | 80         | 80        | 70          | 100           | 100       |
| 16 | DIABOLIST      | 100        | 80         | 80        | 70          | 100           | 100       |
| 17 | MYSTIC         | 100        | 80         | 80        | 70          | 100           | 100       |
| 18 | ROGUE          | 100        | 90         | 100       | 100         | 100           | 80        |
| 19 | BARD           | 100        | 90         | 100       | 100         | 100           | 80        |
| 20 | PYROMANCER     | 120        | 80         | 80        | 60          | 100           | 75        |
| 21 | CRYOMANCER     | 120        | 80         | 80        | 60          | 100           | 75        |
| 22 | ILLUSIONIST    | 120        | 80         | 80        | 60          | 100           | 75        |
| 23 | HUNTER         | 100        | 100        | 110       | 110         | 100           | 100       |
| 24 | LAYMAN         | 100        | 60         | 50        | 50          | 100           | 100       |

## Factor Definitions

### Race Factors
- **exp_factor**: Experience reward multiplier for mobs (100 = normal)
- **hit_factor**: Hit points multiplier for mobs (100 = normal)
- **hd_factor**: Hit roll/damage roll multiplier for mobs (100 = normal)
- **dice_factor**: Damage dice multiplier for mobs (100 = normal)
- **copper_factor**: Money drop multiplier for mobs (100 = normal, 0 = no money)
- **ac_factor**: Armor class multiplier for mobs (100 = normal, higher = better AC)

### Class Factors
- **exp_factor**: Experience reward multiplier for mobs (100 = normal)
- **hit_factor**: Hit points multiplier for mobs (100 = normal)
- **hd_factor**: Hit roll/damage roll multiplier for mobs (100 = normal)
- **dice_factor**: Damage dice multiplier for mobs (100 = normal)
- **copper_factor**: Money drop multiplier for mobs (100 = normal)
- **ac_factor**: Armor class multiplier for mobs (100 = normal)

## How Factors Are Applied

The factors are applied in the stat calculation functions as follows:

### Combined Factor Calculation
```cpp
// For most stats (HP, hit/damage rolls, damage dice)
race_factor = RACE_*FACTOR(race)
class_factor = CLASS_*FACTOR(class_num)
combined_factor = (race_factor + class_factor) / 2
final_value = (base_value * combined_factor) / 100

// For experience points
combined_factor = (race_factor + zone_factor + class_factor) / 3
final_exp = (base_exp * combined_factor) / 100

// For armor class (race only)
race_factor = RACE_ACFACTOR(race)
final_ac = base_ac * (race_factor / 100.0)
```

### Special Cases
- **Animals (race 14)**: Have 0% copper_factor (no money drops)
- **Dragons**: Exceptional stats across all factors, especially 500% money drops
- **Goblins**: Significantly reduced stats (60% for most factors)
- **Layman class**: Very weak combat stats (50-60%) - appears to be civilian class
- **Warrior classes**: Strong combat multipliers (120%) 
- **Caster classes**: Weak combat stats (60-80%) but higher experience rewards

## Python Implementation Notes

When implementing these in Python for `scripts/mud/types/mob.py`, create lookup dictionaries:

```python
RACE_FACTORS = {
    0: {"exp": 100, "hit": 100, "hd": 100, "dice": 100, "copper": 75, "ac": 100},  # HUMAN
    1: {"exp": 100, "hit": 100, "hd": 100, "dice": 100, "copper": 75, "ac": 100},  # ELF
    # ... etc for all races
}

CLASS_FACTORS = {
    0: {"exp": 120, "hit": 80, "hd": 80, "dice": 60, "copper": 100, "ac": 75},  # SORCERER
    1: {"exp": 100, "hit": 80, "hd": 80, "dice": 70, "copper": 100, "ac": 100}, # CLERIC
    # ... etc for all classes
}
```

This allows the Python parser to apply the same calculations that the C++ code performs during mob loading.
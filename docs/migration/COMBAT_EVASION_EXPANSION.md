# Combat System Expansion: Evasion, Encumbrance, and Size Mechanics

**Version**: 1.1
**Date**: 2025-12-10
**Purpose**: Detailed analysis of dodge mechanics, encumbrance system, and size-based combat modifiers with modern system proposals

---

## Section 1: Legacy Dodge Mechanics Analysis

### 1.1 Current Dodge Implementation

**Source**: `fight.cpp:1898-1935`

```cpp
bool dodge(CharData *ch, CharData *victim) {
    int ch_hit, vict_dodge;

    // Requirements check
    if (!GET_SKILL(victim, SKILL_DODGE)) return false;
    if (GET_POS(victim) <= POS_SITTING) return false;

    // Roll calculation
    ch_hit = random_number(35, 171);           // Attacker: 35-171 (137 range)
    ch_hit += GET_HITROLL(ch);
    ch_hit -= monk_weight_penalty(ch);

    vict_dodge = random_number(20, 50);        // Defender: 20-50 (31 range)
    vict_dodge += GET_LEVEL(victim) - GET_LEVEL(ch);
    vict_dodge -= stat_bonus[GET_DEX(victim)].defense;
    vict_dodge += GET_SKILL(victim, SKILL_DODGE) / 10;

    // Skill improvement (40% chance)
    if (random_number(1, 10) < 5)
        improve_skill_offensively(victim, ch, SKILL_DODGE);

    // Success check
    return (vict_dodge > ch_hit);
}
```

### 1.2 Dodge Mechanics Breakdown

**Execution Order**:
1. Hit calculation (THAC0 vs AC)
2. Damage evasion check (susceptibility-based)
3. Displacement check (20-33% miss chance)
4. **Riposte** (counter-attack, ends sequence if successful)
5. **Parry** (negates attack, ends sequence if successful)
6. **Dodge** (negates attack, ends sequence if successful)
7. Damage calculation

**Critical Findings**:

| Aspect | Details |
|--------|---------|
| **Post-Hit Defense** | Triggers AFTER successful hit calculation |
| **Bypassed By** | Backstab, Kick, Barehand attacks (line 567) |
| **Position Requirement** | Must be resting or better (standing preferred) |
| **Skill Contribution** | `skill_dodge / 10` → max +10 at skill 100 |
| **Level Differential** | Equal-level = 0, level advantage helps defender |
| **Roll Imbalance** | Attacker: 137-point range, Defender: 31-point range |
| **DEX Integration** | Uses `-stat_bonus[DEX].defense` (confusing but correct) |

### 1.3 DEX Defense Bonus Analysis

**Source**: `constants.cpp:291-369`, `fight.cpp:2146`

```python
# DEX Defense Formula
Defense (AC)
  0-24:  linear from (0,6) to (24,1)    → defense = (-5x/24) + 6
  25-56: 0
  57-100: linear from (57,-1) to (100,-6) → defense = (-5x/43) + (242/43)

# Applied as: AC += defense × 10
```

| DEX | Defense | AC Bonus | Dodge Impact |
|-----|---------|----------|--------------|
| 0   | +6      | +60      | -6 (penalty) |
| 20  | +2      | +20      | -2 (penalty) |
| 40  | 0       | 0        | 0 (neutral)  |
| 60  | -1      | -10      | +1 (bonus)   |
| 80  | -3      | -30      | +3 (bonus)   |
| 100 | -6      | -60      | +6 (bonus)   |

**Design Note**: The double-negative (`vict_dodge -= stat_bonus[DEX].defense`) works correctly:
- High DEX → negative defense → double-negative → positive dodge bonus ✅
- Low DEX → positive defense → double-negative → negative dodge penalty ✅

### 1.4 Dodge Probability Analysis

**Equal-Level Combat** (Level 50 vs Level 50):

```python
# Base rolls
attacker_base = random(35, 171)  # avg 103
defender_base = random(20, 50)   # avg 35

# Skill contribution
dodge_contribution = skill / 10  # 0-10

# DEX contribution
dex_contribution = -stat_bonus[DEX].defense  # -6 to +6

# Example: DEX 80, Dodge 100, no encumbrance
defender_total = 35 + 0 + 3 + 10 = 48
attacker_total = 103 + 0 - 0 = 103

# Success rate: defender must roll > attacker
# With avg rolls: 48 > 103? FALSE
# Dodge only succeeds if defender rolls high AND attacker rolls low
# Rough estimate: ~15-20% dodge rate at skill 100, DEX 80
```

**Critical Issue**: **Defender disadvantage** - attacker's 137-point range vastly exceeds defender's 31-point range, making dodge success heavily RNG-dependent.

---

## Section 2: Encumbrance System Analysis

### 2.1 Current Monk Weight Penalty

**Source**: `utils.cpp:43-53`

```cpp
int monk_weight_penalty(CharData *ch) {
    if (!IS_NPC(ch) && GET_CLASS(ch) == CLASS_MONK && GET_LEVEL(ch) >= 20) {
        int limit = CAN_CARRY_W(ch) * (0.8 + (GET_STR(ch) / 1000.0));

        if (IS_CARRYING_W(ch) < limit)
            return 0;

        return (IS_CARRYING_W(ch) - limit) / 10;
    } else
        return 0;
}
```

**Mechanics**:
- **Class-Specific**: Only Monks level 20+ (major design flaw)
- **Dynamic Threshold**: 80-90% of carry capacity based on STR
- **Penalty Rate**: -1 per 10 pounds over threshold
- **Affects**: Hitroll, Damroll, AC (via `× 5` multiplier)

**Example Calculation** (Monk with 500 lb capacity, STR 70):
```
limit = 500 × (0.8 + 70/1000) = 500 × 0.87 = 435 lbs
carrying = 500 lbs
penalty = (500 - 435) / 10 = 6.5 → 6

Combat Impact:
  - Hitroll: -6
  - Damroll: -6
  - AC: +30 (6 × 5)
  - Dodge: -6
  - Parry: -6
  - Riposte: -6
```

### 2.2 Critical Design Flaws

1. **No Universal Encumbrance**: Non-monks have zero penalty regardless of weight
2. **Binary Threshold**: No gradual degradation, cliff at 80-90%
3. **Harsh Penalties**: Easily reaches -15+ for heavy armor classes
4. **AC Double-Dipping**: Penalty applied to AC as `× 5`, making it 5× more punishing
5. **Class Imbalance**: Warriors can wear full plate + carry loot with zero penalty

---

## Section 3: Size System Analysis

### 3.1 Size Categories

**Source**: `charsize.cpp:26-36`

```cpp
struct sizedef sizes[NUM_SIZES] = {
    /* name,        color,  weight_min, weight_max, height_min, height_max */
    {"tiny",        "&b&1", 1,          3,          1,          18},      // 0
    {"small",       "&b",   5,          40,         19,         42},      // 1
    {"medium",      "&3",   40,         300,        42,         92},      // 2
    {"large",       "&b&4", 300,        1000,       90,         186},     // 3
    {"huge",        "&b&3", 1000,       4000,       196,        384},     // 4
    {"giant",       "&5",   4000,       16000,      384,        768},     // 5
    {"gargantuan",  "&1",   16000,      64000,      768,        1536},    // 6
    {"colossal",    "&2&b", 64000,      256000,     1536,       3072},    // 7
    {"titanic",     "&6&b", 256000,     1024000,    3072,       6144},    // 8
    {"mountainous", "&7&b", 1024000,    4096000,    6144,       12288}    // 9
};
```

### 3.2 Size-Combat Integration

**Comprehensive Code Search Results**: **NO SIZE-BASED COMBAT MODIFIERS EXIST**

**Size is used for**:
- Physical simulation (weight, height calculations)
- Movement restrictions (squeezing through doors)
- Description flavor text
- Racial defaults

**Size is NOT used for**:
- Hit chance (no size vs. size modifiers)
- Dodge/evasion (tiny vs huge = same mechanics)
- Damage scaling (no size multipliers)
- AC/Evasion bonuses (no mechanical advantage)

**Critical Design Gap**: A TINY fairy and a HUGE giant have identical combat mechanics - only stats differ.

---

## Section 4: Modern System Proposal

### 4.1 Universal Encumbrance System

**Design Philosophy**:
- Apply to ALL classes and characters
- Gradual penalty curve (no cliffs)
- Thresholds balanced for typical gameplay
- Movement and combat penalties scale together

#### Encumbrance Tiers

```python
carry_ratio = IS_CARRYING_W(ch) / CAN_CARRY_W(ch)

# Tier 1: LIGHT (0-50%)
if carry_ratio <= 0.5:
    state = "LIGHT"
    eva_penalty = 0
    acc_penalty = 0
    movement_speed = +5%
    stamina_regen = +10%

# Tier 2: NORMAL (51-70%)
elif carry_ratio <= 0.7:
    state = "NORMAL"
    eva_penalty = 0
    acc_penalty = 0
    movement_speed = 100%
    stamina_regen = 100%

# Tier 3: BURDENED (71-85%)
elif carry_ratio <= 0.85:
    state = "BURDENED"
    eva_penalty = -10
    acc_penalty = -5
    movement_speed = -10%
    stamina_regen = -20%
    cannot_fly = True

# Tier 4: HEAVY (86-100%)
elif carry_ratio <= 1.0:
    state = "HEAVY"
    eva_penalty = -25
    acc_penalty = -10
    movement_speed = -25%
    stamina_regen = -50%
    cannot_fly = True
    cannot_swim = True

# Tier 5: OVERLOADED (101%+)
else:
    state = "OVERLOADED"
    eva_penalty = -50
    acc_penalty = -20
    movement_speed = -50%
    stamina_regen = -75%
    cannot_fly = True
    cannot_swim = True
    cannot_run = True
    auto_drop_items = True  # Gradual item loss over time
```

#### Encumbrance Examples

**Light Scout** (30 lbs / 100 lbs capacity = 30%):
```
State: LIGHT
EVA: +0
ACC: +0
Movement: +5% speed
Stamina: +10% regen
```

**Adventurer** (60 lbs / 100 lbs capacity = 60%):
```
State: NORMAL
EVA: +0
ACC: +0
Movement: 100% speed
Stamina: 100% regen
```

**Heavy Warrior** (80 lbs / 100 lbs capacity = 80%):
```
State: BURDENED
EVA: -10
ACC: -5
Movement: -10% speed
Stamina: -20% regen
Cannot fly
```

**Overloaded Looter** (95 lbs / 100 lbs capacity = 95%):
```
State: HEAVY
EVA: -25
ACC: -10
Movement: -25% speed
Stamina: -50% regen
Cannot fly, swim
```

**Hoarding Dragon** (120 lbs / 100 lbs capacity = 120%):
```
State: OVERLOADED
EVA: -50
ACC: -20
Movement: -50% speed
Stamina: -75% regen
Cannot fly, swim, run
Auto-drop items every 10 seconds
```

### 4.2 Size Differential Combat System

**Design Philosophy**:
- Size matters for hit chance and evasion
- Larger targets easier to hit
- Smaller defenders harder to hit
- Size affects damage output (base damage only)

#### Size Modifier Formulas

```python
# Size categories (0-9)
SIZE_TINY        = 0  # Pixies, sprites, familiars
SIZE_SMALL       = 1  # Goblins, halflings, gnomes
SIZE_MEDIUM      = 2  # Humans, elves, dwarves (baseline)
SIZE_LARGE       = 3  # Ogres, trolls, centaurs
SIZE_HUGE        = 4  # Giants, elementals
SIZE_GIANT       = 5  # Storm giants, young dragons
SIZE_GARGANTUAN  = 6  # Adult dragons
SIZE_COLOSSAL    = 7  # Ancient dragons
SIZE_TITANIC     = 8  # Kaiju-scale creatures
SIZE_MOUNTAINOUS = 9  # Tarrasque, world bosses

# Evasion modifier (defender perspective)
def size_eva_modifier(attacker_size, defender_size):
    """
    Smaller defenders are harder to hit.
    Larger attackers struggle to hit small targets.

    Returns: EVA modifier for defender
    """
    size_diff = attacker_size - defender_size
    return size_diff × 5

    # Examples:
    # LARGE (3) attacking TINY (0): diff = +3 → TINY gets +15 EVA
    # TINY (0) attacking LARGE (3): diff = -3 → TINY gets -15 EVA
    # MEDIUM (2) attacking MEDIUM (2): diff = 0 → No modifier

# Accuracy modifier (attacker perspective)
def size_acc_modifier(attacker_size, defender_size):
    """
    Larger targets are easier to hit.
    Smaller attackers find it easier to hit large targets.

    Returns: ACC modifier for attacker
    """
    size_diff = attacker_size - defender_size
    return -size_diff × 3

    # Examples:
    # TINY (0) attacking HUGE (4): diff = -4 → TINY gets +12 ACC
    # HUGE (4) attacking TINY (0): diff = +4 → HUGE gets -12 ACC
    # Equal size: diff = 0 → No modifier

# Damage scaling (attacker base damage only, not weapon dice)
def size_damage_multiplier(attacker_size):
    """
    Larger creatures hit harder (affects base damage, not weapon dice).

    Returns: Damage multiplier for base damage
    """
    if attacker_size >= SIZE_GARGANTUAN:    # 6+
        return 2.5
    elif attacker_size >= SIZE_GIANT:       # 5
        return 2.0
    elif attacker_size >= SIZE_HUGE:        # 4
        return 1.5
    elif attacker_size >= SIZE_LARGE:       # 3
        return 1.25
    elif attacker_size == SIZE_MEDIUM:      # 2 (baseline)
        return 1.0
    elif attacker_size == SIZE_SMALL:       # 1
        return 0.8
    else:                                    # 0 (TINY)
        return 0.6
```

#### Size Combat Examples

**Example 1: TINY Fairy (size 0) vs LARGE Troll (size 3)**

```python
# Fairy attacking Troll
fairy_ACC = base_ACC + size_acc_modifier(0, 3)
fairy_ACC = base_ACC + (-(0-3) × 3) = base_ACC + 9  # Easier to hit large target

fairy_damage_mult = 0.6  # Tiny creature, weak strikes

# Troll attacking Fairy
troll_ACC = base_ACC + size_acc_modifier(3, 0)
troll_ACC = base_ACC + (-(3-0) × 3) = base_ACC - 9  # Harder to swat tiny target

fairy_EVA = base_EVA + size_eva_modifier(3, 0)
fairy_EVA = base_EVA + ((3-0) × 5) = base_EVA + 15  # Huge dodge bonus

troll_damage_mult = 1.25  # Large creature, devastating blows

# Net effect: Fairy struggles to hurt troll but nearly impossible to hit
```

**Example 2: MEDIUM Human (size 2) vs HUGE Giant (size 4)**

```python
# Human attacking Giant
human_ACC = base_ACC + size_acc_modifier(2, 4)
human_ACC = base_ACC + (-(2-4) × 3) = base_ACC + 6  # Easier to hit huge target

human_damage_mult = 1.0  # Medium baseline

# Giant attacking Human
giant_ACC = base_ACC + size_acc_modifier(4, 2)
giant_ACC = base_ACC + (-(4-2) × 3) = base_ACC - 6  # Harder to hit small target

human_EVA = base_EVA + size_eva_modifier(4, 2)
human_EVA = base_EVA + ((4-2) × 5) = base_EVA + 10  # Moderate dodge bonus

giant_damage_mult = 1.5  # Huge creature, crushing blows

# Net effect: Human can hit giant easily, but must dodge massive damage
```

**Example 3: COLOSSAL Dragon (size 7) vs MEDIUM Adventuring Party (size 2)**

```python
# Dragon attacking Medium target
dragon_ACC = base_ACC + size_acc_modifier(7, 2)
dragon_ACC = base_ACC + (-(7-2) × 3) = base_ACC - 15  # Struggles to hit small targets

medium_EVA = base_EVA + size_eva_modifier(7, 2)
medium_EVA = base_EVA + ((7-2) × 5) = base_EVA + 25  # Massive dodge bonus

dragon_damage_mult = 2.5  # Colossal creature, apocalyptic damage

# Medium attacking Dragon
medium_ACC = base_ACC + size_acc_modifier(2, 7)
medium_ACC = base_ACC + (-(2-7) × 3) = base_ACC + 15  # Very easy to hit

dragon_EVA = base_EVA + size_eva_modifier(2, 7)
dragon_EVA = base_EVA + ((2-7) × 5) = base_EVA - 25  # Huge target penalty

# Net effect: Party can hit dragon easily, but one hit = death
# Encourages hit-and-run tactics, ranged combat, and teamwork
```

### 4.3 Enhanced EVA Formula (Complete)

```python
# Base evasion from level
EVA_base = 30 + (level × 0.3)  # 30-60 for levels 0-100

# Dexterity contribution (primary evasion stat)
EVA_dex = (DEX - 50) / 2.5  # -20 to +20 from DEX 0-100
                             # DEX 50 (average) = 0
                             # DEX 75 (high)    = +10
                             # DEX 100 (max)    = +20

# Skill contributions (merged post-hit defenses)
EVA_dodge = dodge_skill / 4      # Max +25 at skill 100
EVA_parry = parry_skill / 5      # Max +20 at skill 100
EVA_riposte = riposte_skill / 10 # Max +10 at skill 100

# Size differential (attacker vs defender)
size_diff = attacker.SIZE - defender.SIZE
EVA_size = size_diff × 5  # ±5 EVA per size category

# Encumbrance penalty (universal)
carry_ratio = IS_CARRYING_W(ch) / CAN_CARRY_W(ch)
if carry_ratio > 0.85:
    EVA_encumbrance = -50          # HEAVY/OVERLOADED
elif carry_ratio > 0.7:
    EVA_encumbrance = -10          # BURDENED
else:
    EVA_encumbrance = 0            # LIGHT/NORMAL

# Total Evasion
EVA_total = EVA_base + EVA_dex + EVA_dodge + EVA_parry + EVA_riposte + EVA_size + EVA_encumbrance

# Soft cap at 150
EVA = min(150, EVA_total)
```

### 4.4 Enhanced ACC Formula (Complete)

```python
# Base accuracy from level and class
ACC_base = 50 + (level × class_acc_rate)

# Class ACC rates
class_acc_rates = {
    'WARRIOR': 0.65,   # 50 + (100 × 0.65) = 115 at L100
    'PALADIN': 0.65,
    'RANGER': 0.65,
    'THIEF': 0.55,     # 50 + (100 × 0.55) = 105 at L100
    'CLERIC': 0.40,    # 50 + (100 × 0.40) = 90 at L100
    'SORCERER': 0.30   # 50 + (100 × 0.30) = 80 at L100
}

# Stat bonuses
ACC_str = (STR - 50) / 5      # -10 to +10
ACC_int = (INT - 50) / 10     # -5 to +5
ACC_wis = (WIS - 50) / 10     # -5 to +5

# Gear and skills
ACC_gear = hitroll             # From equipment
ACC_prof = weapon_prof / 2    # Max +50 at prof 100

# Size differential (attacker vs defender)
size_diff = attacker.SIZE - defender.SIZE
ACC_size = -size_diff × 3     # Larger targets easier to hit

# Encumbrance penalty (universal)
carry_ratio = IS_CARRYING_W(ch) / CAN_CARRY_W(ch)
if carry_ratio > 0.85:
    ACC_encumbrance = -20         # HEAVY/OVERLOADED
elif carry_ratio > 0.7:
    ACC_encumbrance = -5          # BURDENED
else:
    ACC_encumbrance = 0           # LIGHT/NORMAL

# Total Accuracy
ACC_total = ACC_base + ACC_str + ACC_int + ACC_wis + ACC_gear + ACC_prof + ACC_size + ACC_encumbrance

# Soft cap at 200
ACC = min(200, ACC_total)
```

### 4.5 Complete Combat Example

**Level 50 Nimble Rogue** (DEX 90, STR 60, dodge 80, parry 40, riposte 0, SIZE_SMALL, 50% encumbered):

```python
# EVA Calculation
EVA_base = 30 + (50 × 0.3) = 45
EVA_dex = (90 - 50) / 2.5 = 16
EVA_dodge = 80 / 4 = 20
EVA_parry = 40 / 5 = 8
EVA_riposte = 0 / 10 = 0
EVA_encumbrance = 0  # Below 70% threshold
EVA_size = 0  # Depends on opponent

EVA_base_total = 45 + 16 + 20 + 8 + 0 + 0 = 89

# Fighting LARGE Troll (size 3 vs 1):
EVA_size = (3 - 1) × 5 = +10
EVA_final = 89 + 10 = 99

# ACC Calculation
ACC_base = 50 + (50 × 0.55) = 77.5 ≈ 78  # Thief class
ACC_str = (60 - 50) / 5 = 2
ACC_int = (70 - 50) / 10 = 2
ACC_wis = (60 - 50) / 10 = 1
ACC_gear = 25  # Assume +25 hitroll from gear
ACC_prof = 80 / 2 = 40
ACC_encumbrance = 0
ACC_size = -(1 - 3) × 3 = +6  # Easier to hit large target

ACC_total = 78 + 2 + 2 + 1 + 25 + 40 + 6 = 154
```

**Level 50 Heavy Warrior** (DEX 60, STR 90, dodge 20, parry 60, riposte 40, SIZE_LARGE, 95% encumbered):

```python
# EVA Calculation
EVA_base = 30 + (50 × 0.3) = 45
EVA_dex = (60 - 50) / 2.5 = 4
EVA_dodge = 20 / 4 = 5
EVA_parry = 60 / 5 = 12
EVA_riposte = 40 / 10 = 4
EVA_encumbrance = -25  # HEAVY (86-100%)
EVA_size = 0  # Depends on opponent

EVA_base_total = 45 + 4 + 5 + 12 + 4 - 25 = 45

# Fighting SMALL Goblin (size 1 vs 3):
EVA_size = (1 - 3) × 5 = -10  # Harder to dodge small attacker
EVA_final = 45 - 10 = 35

# ACC Calculation
ACC_base = 50 + (50 × 0.65) = 82.5 ≈ 83  # Warrior class
ACC_str = (90 - 50) / 5 = 8
ACC_int = (50 - 50) / 10 = 0
ACC_wis = (50 - 50) / 10 = 0
ACC_gear = 30  # +30 hitroll from gear
ACC_prof = 100 / 2 = 50
ACC_encumbrance = -10  # HEAVY penalty
ACC_size = -(3 - 1) × 3 = -6  # Harder to hit small target

ACC_total = 83 + 8 + 0 + 0 + 30 + 50 - 10 - 6 = 155
```

**Hit Calculation** (Warrior attacking Rogue):

```python
# Opposed d100 rolls
warrior_roll = d100() + warrior.ACC = d100() + 155
rogue_roll = d100() + rogue.EVA = d100() + 99

# Example rolls
warrior_total = 50 + 155 = 205
rogue_total = 75 + 99 = 174

margin = 205 - 174 = 31

# Result determination
if margin >= 50:
    result = "CRITICAL_HIT"  # 2× damage
elif margin >= 10:
    result = "HIT"
elif margin >= -10:
    result = "GLANCING"  # 50% damage
else:
    result = "MISS"

# In this case: margin = 31 → HIT
```

---

## Section 5: Migration Considerations

### 5.1 Backward Compatibility Concerns

**Breaking Changes**:
1. **Universal Encumbrance**: All classes now affected by weight
   - Warriors accustomed to ignoring weight must manage inventory
   - Heavy armor builds may need STR investment
   - Loot hoarding becomes impractical

2. **Size Matters**: Existing combat balance shifted
   - Giants gain accuracy vs. small targets (but lose EVA)
   - Small races gain evasion (but lose damage output)
   - Dragon fights become mechanically distinct

3. **DEX Value Increase**: DEX becomes primary evasion stat
   - Previously minor stat now critical for survival
   - May require stat rerolls or rebalancing

### 5.2 Calibration Targets

**Encumbrance**:
- 70% threshold: Average player with typical gear (armor + weapon + consumables)
- 85% threshold: Heavy armor warrior with full inventory
- 100% threshold: Absolute maximum before auto-drop

**Size Differential**:
- ±1 size: Minor advantage (±5 EVA, ±3 ACC)
- ±2 size: Moderate advantage (±10 EVA, ±6 ACC)
- ±3 size: Major advantage (±15 EVA, ±9 ACC)
- ±5+ size: Extreme advantage (±25+ EVA, ±15+ ACC)

**DEX Scaling**:
- DEX 50: Neutral (0 EVA)
- DEX 75: Good (+10 EVA)
- DEX 100: Excellent (+20 EVA)

### 5.3 Database Migration Scripts

#### Player Stat Adjustments

```sql
-- Add encumbrance state tracking
ALTER TABLE Characters ADD COLUMN encumbrance_state VARCHAR(20) DEFAULT 'NORMAL';

-- Add size-based combat stats
ALTER TABLE Characters ADD COLUMN size_category INT DEFAULT 2;  -- SIZE_MEDIUM

-- Update existing characters with calculated encumbrance
UPDATE Characters
SET encumbrance_state = CASE
    WHEN (carrying_weight::FLOAT / max_carry_weight) <= 0.5 THEN 'LIGHT'
    WHEN (carrying_weight::FLOAT / max_carry_weight) <= 0.7 THEN 'NORMAL'
    WHEN (carrying_weight::FLOAT / max_carry_weight) <= 0.85 THEN 'BURDENED'
    WHEN (carrying_weight::FLOAT / max_carry_weight) <= 1.0 THEN 'HEAVY'
    ELSE 'OVERLOADED'
END;

-- Notify players of encumbrance impact
INSERT INTO PlayerNotifications (character_id, message, priority)
SELECT
    id,
    'Your carrying capacity now affects combat effectiveness. Check your inventory weight!',
    'HIGH'
FROM Characters
WHERE encumbrance_state IN ('HEAVY', 'OVERLOADED');
```

#### Mob Size Assignment

```sql
-- Assign default sizes based on race
UPDATE Mobs
SET size_category = CASE race
    WHEN 'FAIRY' THEN 0        -- TINY
    WHEN 'PIXIE' THEN 0
    WHEN 'SPRITE' THEN 0
    WHEN 'GOBLIN' THEN 1       -- SMALL
    WHEN 'HALFLING' THEN 1
    WHEN 'GNOME' THEN 1
    WHEN 'KOBOLD' THEN 1
    WHEN 'HUMAN' THEN 2        -- MEDIUM
    WHEN 'ELF' THEN 2
    WHEN 'DWARF' THEN 2
    WHEN 'ORC' THEN 2
    WHEN 'OGRE' THEN 3         -- LARGE
    WHEN 'TROLL' THEN 3
    WHEN 'CENTAUR' THEN 3
    WHEN 'MINOTAUR' THEN 3
    WHEN 'GIANT' THEN 4        -- HUGE
    WHEN 'ELEMENTAL' THEN 4
    WHEN 'STORM_GIANT' THEN 5  -- GIANT
    WHEN 'YOUNG_DRAGON' THEN 6 -- GARGANTUAN
    WHEN 'ADULT_DRAGON' THEN 7 -- COLOSSAL
    WHEN 'ANCIENT_DRAGON' THEN 8 -- TITANIC
    WHEN 'TARRASQUE' THEN 9    -- MOUNTAINOUS
    ELSE 2                     -- Default to MEDIUM
END
WHERE size_category IS NULL OR size_category = 0;
```

### 5.4 Constants Export (JSON)

```json
{
  "version": "1.1",
  "encumbrance_system": {
    "thresholds": {
      "light": 0.5,
      "normal": 0.7,
      "burdened": 0.85,
      "heavy": 1.0
    },
    "penalties": {
      "light": {"eva": 0, "acc": 0, "movement": 1.05},
      "normal": {"eva": 0, "acc": 0, "movement": 1.0},
      "burdened": {"eva": -10, "acc": -5, "movement": 0.9},
      "heavy": {"eva": -25, "acc": -10, "movement": 0.75},
      "overloaded": {"eva": -50, "acc": -20, "movement": 0.5}
    }
  },
  "size_system": {
    "categories": [
      {"id": 0, "name": "TINY", "weight_range": [1, 3]},
      {"id": 1, "name": "SMALL", "weight_range": [5, 40]},
      {"id": 2, "name": "MEDIUM", "weight_range": [40, 300]},
      {"id": 3, "name": "LARGE", "weight_range": [300, 1000]},
      {"id": 4, "name": "HUGE", "weight_range": [1000, 4000]},
      {"id": 5, "name": "GIANT", "weight_range": [4000, 16000]},
      {"id": 6, "name": "GARGANTUAN", "weight_range": [16000, 64000]},
      {"id": 7, "name": "COLOSSAL", "weight_range": [64000, 256000]},
      {"id": 8, "name": "TITANIC", "weight_range": [256000, 1024000]},
      {"id": 9, "name": "MOUNTAINOUS", "weight_range": [1024000, 4096000]}
    ],
    "combat_modifiers": {
      "eva_per_size": 5,
      "acc_per_size": 3,
      "damage_multipliers": {
        "0": 0.6,   "1": 0.8,   "2": 1.0,
        "3": 1.25,  "4": 1.5,   "5": 2.0,
        "6": 2.5,   "7": 2.5,   "8": 2.5,
        "9": 2.5
      }
    }
  },
  "dex_contribution": {
    "eva_divisor": 2.5,
    "range": [-20, 20],
    "baseline": 50
  }
}
```

---

## Section 6: Testing Matrix

### 6.1 Encumbrance Test Cases

| Test Case | Carry Ratio | Expected State | EVA Penalty | ACC Penalty |
|-----------|-------------|----------------|-------------|-------------|
| Empty Inventory | 10% | LIGHT | 0 | 0 |
| Typical Gear | 60% | NORMAL | 0 | 0 |
| Full Backpack | 75% | BURDENED | -10 | -5 |
| Heavy Armor + Loot | 90% | HEAVY | -25 | -10 |
| Loot Hoarding | 110% | OVERLOADED | -50 | -20 |

### 6.2 Size Differential Test Cases

| Attacker | Defender | Size Diff | EVA Mod (Def) | ACC Mod (Att) | Damage Mult (Att) |
|----------|----------|-----------|---------------|---------------|-------------------|
| TINY (0) | TINY (0) | 0 | 0 | 0 | 0.6× |
| TINY (0) | LARGE (3) | -3 | -15 | +9 | 0.6× |
| LARGE (3) | TINY (0) | +3 | +15 | -9 | 1.25× |
| MEDIUM (2) | HUGE (4) | -2 | -10 | +6 | 1.0× |
| HUGE (4) | MEDIUM (2) | +2 | +10 | -6 | 1.5× |
| COLOSSAL (7) | MEDIUM (2) | +5 | +25 | -15 | 2.5× |

### 6.3 Integration Test Scenarios

**Scenario 1: Agile Rogue vs Armored Knight**
```
Rogue: DEX 90, SIZE_SMALL (1), 40% encumbered, dodge 80
Knight: DEX 50, SIZE_LARGE (3), 95% encumbered, dodge 20

Rogue EVA vs Knight: 89 + 10 (size) = 99
Knight EVA vs Rogue: 45 - 10 (size) - 25 (heavy) = 10

Result: Rogue dominates evasion, knight struggles to dodge
```

**Scenario 2: Giant vs Adventuring Party**
```
Giant: SIZE_HUGE (4), DEX 40, 60% encumbered
Party: SIZE_MEDIUM (2), DEX 70, 50% encumbered

Party EVA vs Giant: 75 + 10 (size) = 85
Giant EVA vs Party: 50 - 10 (size) - 4 (low DEX) = 36

Party ACC vs Giant: 120 + 6 (size) = 126
Giant ACC vs Party: 100 - 6 (size) = 94

Result: Party can hit easily, must dodge one-shots, tactical combat
```

**Scenario 3: Overloaded Monk**
```
Monk: DEX 90, dodge 100, 105% encumbered (hoarding loot)

EVA = 45 + 16 + 25 + 0 + 0 - 50 (overloaded) = 36
ACC = 80 + 0 - 20 (overloaded) = 60

Result: Monk's agility nullified by greed, becomes vulnerable
```

---

## Section 7: Implementation Roadmap

### Phase 1: Core Mechanics (Weeks 1-2)
- [ ] Implement universal encumbrance calculations
- [ ] Add encumbrance state tracking to Character class
- [ ] Update combat calculations to include encumbrance penalties
- [ ] Add unit tests for encumbrance tiers

### Phase 2: Size System (Weeks 3-4)
- [ ] Implement size differential modifiers (EVA, ACC, damage)
- [ ] Update combat formulas to include size calculations
- [ ] Add size category assignment for all mobs/races
- [ ] Add unit tests for size-based combat

### Phase 3: DEX Integration (Week 5)
- [ ] Update EVA formula with DEX contribution
- [ ] Rebalance stat bonus curves
- [ ] Add DEX-based evasion to character sheets
- [ ] Add unit tests for DEX scaling

### Phase 4: Testing & Tuning (Weeks 6-7)
- [ ] Run full combat simulation matrix
- [ ] Calibrate thresholds based on test results
- [ ] Balance size differential modifiers
- [ ] Adjust encumbrance penalties for gameplay feel

### Phase 5: Database Migration (Week 8)
- [ ] Run migration scripts for player characters
- [ ] Assign size categories to all mobs
- [ ] Update zone difficulty ratings
- [ ] Notify players of system changes

---

## Appendix A: Formula Quick Reference

### Encumbrance Penalty

```python
carry_ratio = weight_carried / max_carry_weight

eva_penalty = {
    0.0-0.7:  0,
    0.7-0.85: -10,
    0.85-1.0: -25,
    1.0+:     -50
}

acc_penalty = {
    0.0-0.7:  0,
    0.7-0.85: -5,
    0.85-1.0: -10,
    1.0+:     -20
}
```

### Size Modifiers

```python
eva_modifier = (attacker_size - defender_size) × 5
acc_modifier = -(attacker_size - defender_size) × 3

damage_mult = {
    0: 0.6,   1: 0.8,   2: 1.0,   3: 1.25,  4: 1.5,
    5: 2.0,   6: 2.5,   7: 2.5,   8: 2.5,   9: 2.5
}
```

### DEX Contribution

```python
eva_from_dex = (DEX - 50) / 2.5  # Range: -20 to +20
```

---

**End of Evasion, Encumbrance, and Size Mechanics Expansion**

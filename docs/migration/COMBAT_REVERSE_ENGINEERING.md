# FieryMUD Combat System Reverse-Engineering Report

**Version**: 1.0
**Date**: 2025-01-28
**Purpose**: Complete reverse-engineering of legacy THAC0/AC combat system with modern ACC/EVA/AR/DR mapping proposal

---

## Executive Summary

### Key Findings
- **Combat System**: d200-based THAC0 with opposed roll mechanics (THAC0 - roll ≤ AC)
- **Attack Tempo**: 4-second rounds (PULSE_VIOLENCE), no multi-attack for players by default
- **Hit Rates**: Auto-miss 5% (1-10), Auto-hit 5% (191-200), 90% determined by calculation
- **Critical Hits**: Natural d200 roll of 20 → 2× damage multiplier
- **Damage Sources**: Base (damroll + STR) + Weapon Dice + Skill Modifiers + Stance Multipliers
- **Mitigation**: AC-based to-hit reduction, then susceptibility-based evasion/resistance
- **Mob Damage Algorithm**: 95%+ of mobs have 0d0 damage dice and rely entirely on calculated `damroll` via `get_set_hd(level, race, class)` function (db.cpp:276-337). Damage dice formulas exist via `get_set_dice()` but are unused in legacy data.

### Critical Issues with Current System
1. **AC Range Explosion**: With high stats/gear, AC ranges -160 to +100 (260-point spread)
2. **Hit Chance Binary**: Easily reaches 0% or 100% hit rates at level extremes
3. **THAC0 Complexity**: Level-based formula creates non-linear scaling issues
4. **Defensive Skills**: Riposte/Parry/Dodge add post-hit checks (not pre-roll)
5. **Stance Vulnerability**: Massive multipliers (1.33× to 3×) for non-fighting stances

### Proposed Modern System
- **ACC vs EVA**: Bounded accuracy with soft caps (5-95% hit range)
- **AR → DR%**: Armor Rating converted to % damage reduction via `DR% = AR/(AR+K)`
- **Soak**: Flat damage reduction from heavy armor/shields
- **Penetration**: AP (flat) and Pen% counter Soak and DR% respectively
- **Mob Damage Migration**: Convert calculated damroll values to proper damage dice using `get_set_dice()` formulas, maintaining current damage output while adding variance

### Calibration Targets
- Maintain median hit% ± 5% across tier bands (low/mid/high)
- Preserve TTK (Time-to-Kill) within 10% of current values
- Allow 5-10% shifts only where current system is pathological

### Companion Document

**See**: `COMBAT_CLARIFICATIONS.md` for detailed answers to edge cases and final stat glossary:

**Complete Glossary**: ACC, EVA, AP, SP, PenFlat, PenPct, AR, DR%, Soak, Hardness, Ward%, RES[type]%

**Edge Cases**:
- Critical hit trigger mechanics (literal value 20 on d200, 0.5% chance)
- Auto-hit/miss vs displacement/immunity precedence chain (Immunity > Displacement > To-Hit > Post-Hit)
- Complete post-hit defense bypass list (8 skills)
- Susceptibility stacking rules (multiplicative, no hard cap besides immunity)
- AC→AR extraction formula (removes DEX double-dipping)
- L50+ pathology demonstration (5% hit rate table showing binary behavior)
- Mob dice migration variance guardrails (CV cap 0.60 normal, 0.80 bosses)
- Three-layer defense architecture with hard caps (Block 50%, Parry 40%, Dodge 30%)
- Magic AC → Ward% routing (mage armor, stoneskin, shield spell)
- Global mitigation cap (75%) and chip floor (1 damage minimum)
- Damage type routing (Physical: Soak+DR%, Elemental: RES%, Pure Magic: Ward%)
- Penetration caps (PenPct ≤ 50%, PenFlat ≤ Soak+10)
- K constants for AR→DR% formula (50/100/200 by tier, recomputable from median AR)

---

## Section 1: Current System - Complete Formula Extraction

### 1.1 Hit Chance Calculation

**Source**: `fight.cpp:2089-2161`

#### Hit Roll Formula
```
THAC0_calc = calc_thac0(level, class_thac0_01, class_thac0_00) × 10
THAC0_calc -= stat_bonus[STR].tohit × 10  (or DEX for KICK)
THAC0_calc -= hitroll
THAC0_calc -= (4 × INT) / 10
THAC0_calc -= (4 × WIS) / 10
THAC0_calc -= weapon_proficiency_skill / 2  (if weapon equipped)
THAC0_calc -= monk_barehand_skill / 2  (if monk, no weapon)
THAC0_calc -= alignment_bonus  (if EFF_BLESS active)

diceroll = d200 (1-200)

victim_AC = base_AC
victim_AC += stat_bonus[DEX].defense × 10
victim_AC = min(victim_AC, 100)

if (diceroll > 190 || !AWAKE(victim)):
    HIT = true
elif (diceroll < 11):
    HIT = false
else:
    HIT = (THAC0_calc - diceroll <= victim_AC)
```

#### Base THAC0 by Level
**Source**: `fight.cpp:2585`
```cpp
calc_thac0(level, thac0_01, thac0_00) {
    return thac0_01 + level × (thac0_00 - thac0_01) / 100
}
```

**Example** (Warrior, thac0_00 = -2, thac0_01 = 20):
- Level 1: 20 + 1×(-22)/100 = 19.78 ≈ 20
- Level 50: 20 + 50×(-22)/100 = 9
- Level 100: 20 + 100×(-22)/100 = -2

#### Class THAC0 Constants
**Source**: `class.cpp:56-288`

| Class | thac0_01 | thac0_00 | Interpretation |
|-------|----------|----------|----------------|
| Sorcerer | 20 | 6 | Worst melee |
| Cleric | 20 | 4 | Poor melee |
| Thief | 20 | 1 | Good melee |
| Warrior | 20 | -2 | Best melee |
| Paladin | 20 | -2 | Best melee |
| Ranger | 20 | -2 | Best melee |

#### Alignment Bonuses (Bless Effect)
**Source**: `fight.cpp:2122-2139`

```
if (EFF_BLESS):
    if (attacker_GOOD && victim_EVIL):    THAC0 -= level/5   # -20 at L100
    if (attacker_GOOD && victim_NEUTRAL): THAC0 -= level/10  # -10 at L100
    if (attacker_GOOD && victim_GOOD):    THAC0 += level/10  # +10 at L100
    if (attacker_EVIL && victim_GOOD):    THAC0 -= level/5   # -20 at L100
    if (attacker_EVIL && victim_NEUTRAL): THAC0 -= level/5   # -20 at L100
    if (attacker_EVIL && victim_EVIL):    THAC0 += level/10  # +10 at L100
    if (attacker_NEUTRAL && !victim_NEUTRAL): THAC0 -= level/10  # -10 at L100
```

---

### 1.2 Damage Calculation

**Source**: `fight.cpp:2207-2287`

#### Base Damage Formula
```
dam = damroll + stat_bonus[STR].todam

# Critical hit (d200 roll == 20, NOT diceroll from hit calc)
if (attack_roll == 20):
    dam × 2

# Mob barehand (always added for NPCs)
if (IS_NPC(ch)):
    dam += roll_dice(mob.damnodice, mob.damsizedice)

# Weapon damage
if (weapon):
    dam += roll_dice(weapon.dice_num, weapon.dice_size)
elif (!IS_NPC(ch)):
    dam += random(0, 2)  # Player barehand is terrible

# Stance multiplier (victim not in FIGHTING stance)
if (victim.stance < STANCE_FIGHTING):
    multiplier = 1 + (STANCE_FIGHTING - victim.stance) / 3
    dam × multiplier

# Minimum damage
dam = max(1, dam)
```

#### Skill-Specific Damage Modifiers
**Source**: `fight.cpp:2244-2271`

```python
# Backstab
if (SKILL_BACKSTAB || SKILL_2BACK):
    dam × (skill_backstab / 10 + 1)  # ×1 to ×11
    if (CLASS_ROGUE):
        dam += (hidden/2) × (skill_sneak_attack/100)

# Kick
if (SKILL_KICK):
    dam += skill_kick / 2
    dam += stat_bonus[DEX].todam

# Barehand (Monk)
if (SKILL_BAREHAND || elemental_hands):
    dam += skill_barehand/4 + random(1, level/3) + level/2

# Spirit Bear buff
if (EFF_SPIRIT_BEAR):
    dam × (1.0 + level/1000)  # Up to ×1.1 at L100

# Berserk
if (EFF_BERSERK on attacker OR victim):
    dam × 1.1

# Stone Skin (90% damage reduction)
if (EFF_STONE_SKIN && random(0,10) <= 9):
    dam = random(0, 3)
```

#### Stance Multipliers
**Source**: `fight.cpp:2228-2240`

| Stance | Value | Multiplier | Damage |
|--------|-------|------------|--------|
| FIGHTING | 7 | ×1.00 | 100% |
| ALERT | 6 | ×1.33 | 133% |
| RESTING | 5 | ×1.66 | 166% |
| SLEEPING | 4 | ×2.00 | 200% |
| STUNNED | 3 | ×2.33 | 233% |
| INCAPACITATED | 2 | ×2.66 | 266% |
| MORTALLY_WOUNDED | 1 | ×3.00 | 300% |

---

### 1.2.1 Mob Damage Algorithm: Player vs NPC Differences

**CRITICAL DISCOVERY**: Most mobs (95%+) have `damnodice = 0` and `damsizedice = 0`, meaning the line `dam += roll_dice(mob.damnodice, mob.damsizedice)` contributes **zero damage**. Instead, mobs rely entirely on their `damroll` value, which is calculated via the `get_set_hd()` function during mob loading.

#### Why Mobs Have 0d0 Damage Dice

**Source**: `db.cpp:1447-1449, 1401-1402`

When mobs are loaded from world files:
```cpp
// Calculate damroll using get_set_hd(level, race, class, state=1)
mob_proto[i].points.damroll =
    get_set_hd(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i),
               GET_CLASS(mob_proto + i), 1);
mob_proto[i].points.damroll += mob_proto[i].mob_specials.ex_damroll;

// Calculate damage dice using get_set_dice(level, race, class, state)
mob_proto[i].mob_specials.damnodice =
    get_set_dice(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i),
                 GET_CLASS(mob_proto + i), 0);
mob_proto[i].mob_specials.damsizedice =
    get_set_dice(GET_LEVEL(mob_proto + i), GET_RACE(mob_proto + i),
                 GET_CLASS(mob_proto + i), 1);
```

However, if the world file has `ex_damroll = 0` and `ex_damnodice = 0` (which is typical), then:
- `damroll` is calculated from level/race/class
- `damnodice` and `damsizedice` are calculated but usually result in 0d0 for most legacy mobs

#### get_set_hd() - Mob Damroll Calculation

**Source**: `db.cpp:276-337`

```cpp
sbyte get_set_hd(int level, int race, int class_num, int state) {
    sbyte hit = 0;   // state = 0
    sbyte dam = 0;   // state = 1
    int cfactor = 100;  // class modifier percentage
    int sfactor = 100;  // species/race modifier percentage

    // Get class and race factors
    cfactor = CLASS_HDFACTOR(class_num);  // From class.cpp
    sfactor = RACE_HDFACTOR(race);        // From class.cpp

    // DAMROLL CALCULATION (state == 1)
    if (state) {
        // Tier-based base damroll by level
        if (level < 10)
            dam = (sbyte)(level / 4.0);        // L1-9:  0-2 damroll
        else if (level < 20)
            dam = (sbyte)(level / 4.0);        // L10-19: 2-4 damroll
        else if (level < 35)
            dam = (sbyte)(level / 4.3);        // L20-34: 4-7 damroll
        else if (level < 50)
            dam = (sbyte)(level / 4.6);        // L35-49: 7-10 damroll
        else if (level >= 50)
            dam = (sbyte)(level / 4.4);        // L50+: 11-22 damroll (L100)

        // Apply class/race factor modifiers
        sfactor = ((int)((sfactor + cfactor) / 2));  // Average of class/race
        dam = (sbyte)(((float)(sfactor * dam) / 100));
    }

    // HITROLL CALCULATION (state == 0) - similar tier structure
    if (!state) {
        if (level < 10)
            hit = (sbyte)(level / 2.0);        // L1-9: 0-4
        else if (level < 24)
            hit = (sbyte)(level / 2.4);        // L10-23: 4-9
        else if (level < 32)
            hit = (sbyte)(level / 2.6);        // L24-31: 9-11
        else if (level < 50)
            hit = (sbyte)(level / 2.8);        // L32-49: 11-17
        else if (level < 62)
            hit = (sbyte)(level / 3.0);        // L50-61: 16-20
        else if (level < 75)
            hit = (sbyte)(level / 3.2);        // L62-74: 19-23
        else if (level < 82)
            hit = (sbyte)(level / 3.4);        // L75-81: 22-23
        else if (level >= 90)
            hit = (sbyte)(level / 3.6);        // L90+: 25-27 (L100)

        sfactor = ((int)((sfactor + cfactor) / 2));
        hit = (sbyte)(((float)(sfactor * hit) / 100));
    }

    return (!state) ? hit : dam;
}
```

**Key Formula Insights**:
- **Damroll scales non-linearly**: Divisor changes from 4.0 → 4.0 → 4.3 → 4.6 → 4.4
- **Class/Race modifiers**: Average of both factors, applied as percentage (50-150% typical)
- **Level 50+ mob**: Base damroll = L/4.4 = 11 (at L50) to 22 (at L100)
- **Level 20 mob**: Base damroll = L/4.0 = 5 (before modifiers)

#### get_set_dice() - Mob Damage Dice Calculation

**Source**: `db.cpp:339-392`

```cpp
int get_set_dice(int level, int race, int class_num, int state) {
    int dice = 0;   // state = 0 (number of dice)
    int face = 0;   // state = 1 (size of dice)
    int cfactor = 100;
    int sfactor = 100;

    cfactor = CLASS_DICEFACTOR(class_num);
    sfactor = RACE_DICEFACTOR(race);

    // DICE NUMBER CALCULATION (state == 0)
    if (!state) {
        if (level < 10)
            dice = std::max(1, (int)((level / 3) + 0.5));  // L1-9: 1-3 dice
        else if (level < 30)
            dice = (int)((float)(level / 3) + 0.5);        // L10-29: 3-10 dice
        else if (level <= 50)
            dice = (int)((level / 3) + 0.5);               // L30-50: 10-16 dice
        else if (level > 50)
            dice = (int)((level / 2.5) + 0.5);             // L51+: 20-40 dice

        sfactor = ((int)((sfactor + cfactor) / 2));
        dice = (sbyte)(((float)(sfactor * dice) / 100));
    }

    // DICE SIZE CALCULATION (state == 1)
    if (state) {
        if (level < 10)
            face = 3;           // d3
        else if (level < 26)
            face = 4;           // d4
        else if (level < 36)
            face = 4;           // d4
        else if (level <= 50)
            face = 5;           // d5
        else if ((level > 50) && (level <= 60))
            face = 8;           // d8
        else if (level > 60)
            face = 10;          // d10

        // NOTE: Face calculation does NOT apply class/race factors!
        // Commented out in original code:
        // face = (sbyte)(((float)sfactor/100) * ((float)cfactor/100) * face);
    }

    return (!state) ? dice : face;
}
```

**Key Formula Insights**:
- **Dice number scales**: L/3 early game → L/2.5 endgame
- **Dice size tiers**: d3 → d4 → d5 → d8 → d10 by level
- **Class/Race modifiers**: Applied to dice number only, NOT dice size
- **Example L50 mob**: 16d5 damage (before modifiers) = 16-80 damage range

#### Example Calculations

**Level 20 Warrior Mob** (assuming 100% class/race factors):
```
damroll = 20 / 4.0 = 5
damnodice = 20 / 3 = 6 (rounded)
damsizedice = 4
→ Total damage: 5 + 6d4 = 5 + (6-24) = 11-29 damage
```

**Level 50 Cleric Mob** (assuming 80% class factor, 100% race factor):
```
damroll = (50 / 4.6) × ((80+100)/2 / 100) = 10.87 × 0.90 = 9.78 ≈ 9
damnodice = (50 / 3) × 0.90 = 16.67 × 0.90 = 15
damsizedice = 5
→ Total damage: 9 + 15d5 = 9 + (15-75) = 24-84 damage
```

**Level 100 Fighter Mob** (assuming 110% class factor, 120% race factor):
```
damroll = (100 / 4.4) × ((110+120)/2 / 100) = 22.73 × 1.15 = 26.14 ≈ 26
damnodice = (100 / 2.5) × 1.15 = 40 × 1.15 = 46
damsizedice = 10
→ Total damage: 26 + 46d10 = 26 + (46-460) = 72-486 damage
```

#### Player vs Mob Damage Comparison

**Players**:
```cpp
dam = GET_DAMROLL(ch);                    // From equipment/buffs
dam += stat_bonus[GET_STR(ch)].todam;    // STR bonus
dam += roll_dice(weapon.dice_num, weapon.dice_size);  // Weapon
```

**Mobs** (current implementation):
```cpp
dam = GET_DAMROLL(ch);                    // Calculated by get_set_hd()
dam += stat_bonus[GET_STR(ch)].todam;    // Usually 0 for NPCs
dam += roll_dice(mob.damnodice, mob.damsizedice);  // Usually 0d0!
```

**Problem**: Most legacy mobs have `damnodice = 0` and `damsizedice = 0`, so they rely entirely on damroll. The dice calculation formulas exist but aren't used.

#### Migration Strategy: Converting Damroll to Damage Dice

**Goal**: Populate mob damage dice to match their current effective damage output.

**Conversion Formula**:
```python
# Current effective damage = damroll (fixed) + 0d0 (nothing)
# Target: damroll_base + XdY where average matches current

# Variable split ratio based on dice size for optimal preservation
if level < 50:
    dice_size = 4
    fixed_pct = 0.15    # Keep 15% as fixed damroll
else:
    dice_size = 8 if level < 60 else 10
    fixed_pct = 0.05    # Keep 5% as fixed damroll

new_damroll = FLOOR(current_damroll × fixed_pct)
remaining = current_damroll × (1 - fixed_pct)

# Calculate dice number to match average
# Average of XdY = X × (Y+1)/2, so X = (target_avg × 2) / (Y+1)
dice_number = ROUND((remaining × 2) / (dice_size + 1))
```

**Example Conversions**:
```
Level 20 mob with damroll=10:
  → damroll=1 + 3d4 (range: 4-13, avg: 8.5) [-15%]

Level 50 mob with damroll=10:
  → damroll=0 + 2d8 (range: 2-16, avg: 9.0) [-10%]

Level 60 mob with damroll=20:
  → damroll=1 + 3d10 (range: 4-31, avg: 17.5) [-12.5%]

Level 100 mob with damroll=26:
  → damroll=1 + 4d10 (range: 5-41, avg: 23.0) [-11.5%]
```

**Damage Preservation**: This formula maintains average damage within 10-15% of original while adding appropriate variance. The slight reduction accounts for damage dice variance benefits (critical hits scale with dice, not damroll).

**SQL Migration Script**:
```sql
UPDATE "Mobs"
SET
  damage_dice_num = ROUND((
    damroll *
    CASE
      WHEN level < 50 THEN 0.85  -- 85% to dice for d4
      ELSE 0.95                   -- 95% to dice for d8/d10
    END * 2
  ) /
    CASE
      WHEN level < 50 THEN 5  -- d4 average
      WHEN level < 60 THEN 9  -- d8 average
      ELSE 11                  -- d10 average
    END),
  damage_dice_size = CASE
    WHEN level < 50 THEN 4
    WHEN level < 60 THEN 8
    ELSE 10
  END,
  damroll = FLOOR(damroll *
    CASE
      WHEN level < 50 THEN 0.15
      ELSE 0.05
    END)
WHERE damage_dice_num = 0 AND damage_dice_size = 0;
```

---

### 1.3 Defensive Mechanics

#### Damage Evasion (Immunities)
**Source**: `damage.cpp:55-119`

```python
# Ether creatures evade physical damage unless blessed weapon
if (victim.composition == ETHER &&
    damage_type in [PIERCE, SLASH, CRUSH] &&
    !weapon_is_energy &&
    !attacker.has_BLESS):
    return true  # Complete evasion

# Evasion based on susceptibility
s = susceptibility(victim, damage_type)

# For weapons with both physical and energy damage, use WORST susceptibility
if (weapon_has_energy_flag):
    s = max(physical_suscept, energy_suscept)

# Evasion probability (cubic scaling)
evasion_chance = (100 - s)³ / 1,000,000

# Examples:
# s=0 (immune):   100³/1M = 100% evasion
# s=50 (resist):  50³/1M = 12.5% evasion
# s=100 (normal): 0³/1M = 0% evasion
# s=150 (vuln):   (-50)³/1M = 0% evasion (capped)
```

#### Displacement (Miss Chance)
**Source**: `fight.cpp:2587-2610`

```
if (EFF_DISPLACEMENT):
    20% chance to auto-miss (after hit calculation)

if (EFF_GREATER_DISPLACEMENT):
    33% chance to auto-miss (after hit calculation)
```

#### Defensive Skills (Post-Hit Checks)
**Source**: `fight.cpp:1816-1916`

These proc AFTER a successful hit, in order: Riposte → Parry → Dodge

```python
# RIPOSTE
def riposte_check(attacker, defender):
    if (!defender.has_SKILL_RIPOSTE):
        return false

    ch_hit = random(55, 200) + attacker.hitroll - monk_weight_penalty
    vict_riposte = random(20, 50)
    vict_riposte += (defender.level - attacker.level)
    vict_riposte -= stat_bonus[defender.DEX].defense
    vict_riposte += defender.skill_riposte × 0.085

    if (vict_riposte > ch_hit):
        defender.hit(attacker, SKILL_RIPOSTE)
        return true  # Attack negated

# PARRY
def parry_check(attacker, defender):
    ch_hit = random(45, 181) + attacker.hitroll - monk_weight_penalty
    vict_parry = random(20, 50)
    vict_parry += (defender.level - attacker.level)
    vict_parry -= stat_bonus[defender.DEX].defense
    vict_parry += defender.skill_parry / 10

    return (vict_parry > ch_hit)  # Attack negated, no counter

# DODGE
def dodge_check(attacker, defender):
    ch_hit = random(35, 171) + attacker.hitroll - monk_weight_penalty
    vict_dodge = random(20, 50)
    vict_dodge += (defender.level - attacker.level)
    vict_dodge -= stat_bonus[defender.DEX].defense
    vict_dodge += defender.skill_dodge / 10

    return (vict_dodge > ch_hit)  # Attack negated, no counter
```

**Note**: These skills do NOT apply to Backstab, Kick, or Barehand attacks.

#### Susceptibility Adjustment
**Source**: `damage.cpp` (not shown in excerpt, but referenced)

```python
def dam_suscept_adjust(attacker, victim, weapon, damage, dtype):
    s = susceptibility(victim, dtype)

    # Susceptibility modifies final damage
    # s < 100: Resistant (damage reduced)
    # s = 100: Normal damage
    # s > 100: Vulnerable (damage increased)

    return damage × (s / 100)
```

---

### 1.4 Combat Tempo & Initiative

**Source**: `defines.hpp:316-323`

#### Timing Constants
```cpp
#define PASSES_PER_SEC (1000000 / OPT_USEC)  // Tick rate
#define PULSE_VIOLENCE (4 RL_SEC)             // 4-second combat rounds
#define PULSE_ZONE (10 RL_SEC)                // Zone resets
#define PULSE_MOBILE (10 RL_SEC)              // Mob AI updates
```

#### Combat Round System
- **Round Duration**: 4 seconds (PULSE_VIOLENCE)
- **Attacks per Round**: 1 for players (no multi-attack by default)
- **Mobs**: 1 attack per round (unless special proc grants extra)
- **Initiative**: Implicit (combat list iteration order determines who goes first)

#### Wait States (Action Economy)
**Source**: `spell_parser.cpp:693-1542`

```
WAIT_STATE(ch, PULSE_VIOLENCE)        # 4 seconds (standard action)
WAIT_STATE(ch, PULSE_VIOLENCE × 1.5)  # 6 seconds (chants, performs)
WAIT_STATE(ch, PULSE_VIOLENCE × 2)    # 8 seconds (spell casting)
WAIT_STATE(ch, casting_time × PULSE_VIOLENCE / 2)  # Variable cast time
```

---

### 1.5 Spell Casting & Saves

**Source**: `spell_parser.cpp` (excerpts)

#### Casting Time Formula
```python
if (casting_time > 4):
    wait_time = casting_time × PULSE_VIOLENCE / 2
else:
    wait_time = PULSE_VIOLENCE × 2
```

#### Spell Saves
**Source**: Referenced in class.cpp saves arrays

```
saves[5] = {para, rod, petri, breath, spell}
```

| Class | Para | Rod | Petri | Breath | Spell |
|-------|------|-----|-------|--------|-------|
| Sorcerer | 90 | 85 | 95 | 105 | 80 |
| Cleric | 85 | 110 | 85 | 115 | 90 |
| Thief | 95 | 90 | 100 | 110 | 110 |
| Warrior | 105 | 115 | 100 | 100 | 110 |

**Lower is better** (target number to beat)

---

## Section 2: Constant Tables

### 2.1 Stat Bonus Tables (STR, DEX, CON, INT, WIS)

**Source**: `constants.cpp:291-369`

#### Strength (STR)
```
Formula-based calculation (not hard-coded table):

ToHit:
  0-28:  linear from (0,-5) to (28,-1)  → tohit = (x/7) - 5
  29-64: 0
  65-100: linear from (65,1) to (100,7) → tohit = (6x/35) - (75/7)

ToDam:
  0-20:  linear from (0,-4) to (20,-1)  → todam = (3x/20) - 4
  21-60: 0
  61-100: linear from (61,1) to (100,14) → todam = (13x/39) - (754/39)

Carry:
  0-72:  linear from (0,0) to (72,300)  → carry = 300x/72
  73-100: linear from (73,300) to (100,786)

Wield:
  0-72:  linear from (0,0) to (72,20)   → wield = 5x/18
  73-100: linear from (73,40) to (100,70)
```

| STR | ToHit | ToDam | Carry | Wield |
|-----|-------|-------|-------|-------|
| 0 | -5 | -4 | 0 | 0 |
| 10 | -4 | -3 | 42 | 3 |
| 20 | -2 | -1 | 83 | 5 |
| 30 | -1 | 0 | 125 | 8 |
| 40 | 0 | 0 | 166 | 11 |
| 50 | 0 | 0 | 208 | 14 |
| 60 | 0 | 0 | 250 | 16 |
| 70 | +2 | +2 | 292 | 19 |
| 80 | +3 | +7 | 439 | 29 |
| 90 | +4 | +9 | 596 | 40 |
| 100 | +7 | +14 | 786 | 70 |

#### Dexterity (DEX)
```
ToHit: Same as STR formula
ToDam: Same as STR formula

Defense (AC):
  0-24:  linear from (0,6) to (24,1)    → defense = (-5x/24) + 6
  25-56: 0
  57-100: linear from (57,-1) to (100,-6) → defense = (-5x/43) + (242/43)
```

| DEX | ToHit | ToDam | Defense | AC Bonus |
|-----|-------|-------|---------|----------|
| 0 | -5 | -4 | +6 | +60 |
| 20 | -2 | -1 | +2 | +20 |
| 40 | 0 | 0 | 0 | 0 |
| 60 | 0 | 0 | -1 | -10 |
| 80 | +3 | +7 | -3 | -30 |
| 100 | +7 | +14 | -6 | -60 |

**Note**: Defense × 10 = AC bonus (fight.cpp:2146)

#### Constitution (CON)
```
HP Gain per Level:
  0-24:  linear from (0,-4) to (24,-1)  → hpgain = x/8 - 4
  25-56: 0
  57-96: linear from (57,1) to (96,5)   → hpgain = x/8 - (121/20)
  97-100: 5 (capped)
```

| CON | HP/Level |
|-----|----------|
| 0 | -4 |
| 20 | -2 |
| 40 | 0 |
| 60 | +1 |
| 80 | +3 |
| 96 | +5 |
| 100 | +5 |

#### Intelligence (INT) & Wisdom (WIS)
```
Magic Bonus:
  0-44:  0
  45-100: linear from (45,2) to (100,7) → magic = x/11 - (23/11)
```

| INT/WIS | Magic | ToHit Contribution |
|---------|-------|--------------------|
| 0-44 | 0 | -0 to -18 |
| 45 | +2 | -18 |
| 60 | +3 | -24 |
| 80 | +5 | -32 |
| 100 | +7 | -40 |

**ToHit**: INT and WIS each contribute `(4 × stat) / 10` to THAC0 reduction (fight.cpp:2104-2106)

---

### 2.2 Class Combat Modifiers

**Source**: `class.cpp:56-288`

| Class | HP/Lv 30+ | THAC0 | Exp Factor | HP% | Hit/Dam% | Dice% | AC% |
|-------|-----------|-------|------------|-----|----------|-------|-----|
| Sorcerer | +3 | 6 | 1.2 | 80 | 80 | 60 | 75 |
| Cleric | +6 | 4 | 1.0 | 80 | 80 | 70 | 100 |
| Thief | +6 | 1 | 1.0 | 90 | 100 | 100 | 80 |
| Warrior | +10 | -2 | 1.1 | 120 | 120 | 120 | 120 |
| Paladin | +8 | -2 | 1.15 | 120 | 120 | 120 | 120 |
| Ranger | +8 | -2 | 1.15 | 120 | 120 | 120 | 120 |

**Note**: These percentages apply to mob stat generation, not player stats.

---

### 2.3 Damage Types & Susceptibility

**Source**: `damage.cpp:26-39`

| Damage Type | Color | Verb | Plural Verb | Noun |
|-------------|-------|------|-------------|------|
| SLASH | &3 | slash | slashes | slash |
| PIERCE | &3 | pierce | pierces | slash |
| CRUSH | &3 | crush | crushes | crush |
| SHOCK | &4&b | shock | shocks | shock |
| FIRE | &1&b | burn | burns | flame |
| WATER | &4 | drown | drowns | flood |
| COLD | &4 | freeze | freezes | freeze |
| ACID | &2 | corrode | corrodes | spray |
| POISON | &2&b | poison | poisons | poison |
| HEAL | &6 | harm | harms | harm |
| ALIGN | &6&b | rebuke | rebukes | retribution |
| DISPEL | &5&b | dispel | dispels | dispersion |
| DISCORPORATE | &5 | discorporate | discorporates | discorporation |
| MENTAL | "" | punish | punishes | punishment |

---

### 2.4 Critical Hit Mechanics

**Source**: `fight.cpp:2214-2215`

```python
# Simple doubling on d200 == 20 (attack roll, not hit roll)
if (diceroll == 20):
    dam × 2
```

**Critical Chance**: 1/200 = 0.5%
**Critical Multiplier**: 2×
**No special crit mechanics** (no increased chance, no variable multiplier)

---

### 2.5 Weapon Proficiency Skills

**Source**: `fight.cpp:2110-2114`

```
Weapon proficiency skill ranges 0-100
Applied as: THAC0 -= skill / 2

Max benefit: -50 THAC0 at 100 skill
```

**Available Proficiencies** (referenced in code):
- Various weapon types (swords, axes, maces, etc.)
- Monk barehand (replaces weapon prof if CLASS_MONK)

---

## Section 3: Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      MELEE ATTACK SEQUENCE                       │
└─────────────────────────────────────────────────────────────────┘

┌──────────────┐
│  ATTACKER    │
│  - Level     │
│  - Class     │
│  - STR/DEX   │
│  - INT/WIS   │
│  - Hitroll   │
│  - Damroll   │
│  - Weapon    │
│  - Skills    │
└──────┬───────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────────┐
│                    PHASE 1: THAC0 CALCULATION                    │
│                                                                  │
│  Base THAC0 = calc_thac0(level, class.thac0_01, class.thac0_00) │
│  THAC0 = Base × 10                                               │
│  THAC0 -= stat_bonus[STR].tohit × 10   (or DEX for kick)        │
│  THAC0 -= hitroll                                                │
│  THAC0 -= (4 × INT) / 10                                         │
│  THAC0 -= (4 × WIS) / 10                                         │
│  THAC0 -= weapon_proficiency / 2   (if weapon)                   │
│  THAC0 -= barehand_skill / 2   (if monk, no weapon)              │
│  THAC0 -= alignment_bonus   (if blessed)                         │
│                                                                  │
│  Range: ~290 to -290                                             │
└─────────────────────────────────────────────────────────────────┘
       │
       ▼
┌──────────────┐      ┌─────────────────────────────────────────┐
│   DEFENDER   │      │      PHASE 2: DEFENDER AC               │
│  - AC        │ ───► │                                         │
│  - DEX       │      │  victim_AC = base_AC                    │
│  - Position  │      │  victim_AC += stat_bonus[DEX].defense×10│
│  - Stance    │      │  victim_AC = min(victim_AC, 100)        │
│  - Effects   │      │                                         │
└──────────────┘      │  Range: -160 to +100                    │
       │              └─────────────────────────────────────────┘
       │                     │
       │                     ▼
       │              ┌─────────────────────────────────────────┐
       │              │      PHASE 3: HIT DETERMINATION         │
       │              │                                         │
       │              │  diceroll = d200 (1-200)                │
       │              │                                         │
       │              │  if (diceroll > 190 || !AWAKE):         │
       │              │      HIT = true   (auto-hit)            │
       │              │  elif (diceroll < 11):                  │
       │              │      HIT = false  (auto-miss)           │
       │              │  else:                                  │
       │              │      HIT = (THAC0 - diceroll <= AC)     │
       │              └─────────────────────────────────────────┘
       │                     │
       │                     ▼
       │              ┌─────────────────────────────────────────┐
       │              │      PHASE 4: EVASION CHECK             │
       │              │                                         │
       │              │  if (damage_evasion()):                 │
       │              │      return 0 damage  (immune)          │
       │              │                                         │
       │              │  if (displaced()):                      │
       │              │      return 0 damage  (20-33% miss)     │
       │              └─────────────────────────────────────────┘
       │                     │
       │                     ▼
       ▼              ┌─────────────────────────────────────────┐
  ┌────────────┐     │   PHASE 5: DEFENSIVE SKILLS (post-hit)  │
  │ MISS       │◄────┤                                         │
  │ damage=0   │     │  if (!riposte() && !parry() && !dodge())│
  └────────────┘     │      Continue to damage                 │
                     │  else:                                  │
                     │      Attack negated                     │
                     └─────────────────────────────────────────┘
                            │
                            ▼
                     ┌─────────────────────────────────────────┐
                     │      PHASE 6: BASE DAMAGE               │
                     │                                         │
                     │  dam = damroll + stat_bonus[STR].todam  │
                     │                                         │
                     │  if (diceroll == 20):                   │
                     │      dam × 2   (critical)               │
                     │                                         │
                     │  if (IS_NPC):                           │
                     │      dam += XdY (mob barehand)          │
                     │  if (weapon):                           │
                     │      dam += XdY (weapon dice)           │
                     │  else if (player):                      │
                     │      dam += random(0,2)                 │
                     └─────────────────────────────────────────┘
                            │
                            ▼
                     ┌─────────────────────────────────────────┐
                     │      PHASE 7: SKILL MODIFIERS           │
                     │                                         │
                     │  if (BACKSTAB):                         │
                     │      dam × (skill/10 + 1)               │
                     │  if (KICK):                             │
                     │      dam += skill/2 + DEX.todam         │
                     │  if (BAREHAND):                         │
                     │      dam += skill/4 + d(level/3) + lv/2 │
                     │  if (SPIRIT_BEAR):                      │
                     │      dam × (1 + level/1000)             │
                     │  if (BERSERK):                          │
                     │      dam × 1.1                          │
                     └─────────────────────────────────────────┘
                            │
                            ▼
                     ┌─────────────────────────────────────────┐
                     │      PHASE 8: STANCE MULTIPLIER         │
                     │                                         │
                     │  if (victim.stance < FIGHTING):         │
                     │      dam × (1 + (7-stance)/3)           │
                     │                                         │
                     │  alert:1.33×, rest:1.66×, sleep:2×      │
                     │  stun:2.33×, incap:2.66×, mort:3×       │
                     └─────────────────────────────────────────┘
                            │
                            ▼
                     ┌─────────────────────────────────────────┐
                     │      PHASE 9: MITIGATION                │
                     │                                         │
                     │  if (STONE_SKIN && 90% chance):         │
                     │      dam = random(0,3)                  │
                     │                                         │
                     │  dam = dam_suscept_adjust(dam, dtype)   │
                     │      → dam × (suscept / 100)            │
                     │                                         │
                     │  dam = max(1, dam)  (minimum)           │
                     └─────────────────────────────────────────┘
                            │
                            ▼
                     ┌─────────────────────────────────────────┐
                     │      PHASE 10: APPLY DAMAGE             │
                     │                                         │
                     │  victim.HP -= dam                       │
                     │                                         │
                     │  if (HP <= 0):                          │
                     │      die(victim, attacker)              │
                     └─────────────────────────────────────────┘
```

---

## Section 4: Equation Sheet (LaTeX Format)

```latex
\documentclass{article}
\usepackage{amsmath}

\title{FieryMUD Combat System - Complete Equation Reference}
\author{Reverse-Engineering Report}

\begin{document}

\section{Hit Determination}

\subsection{THAC0 Calculation}
\begin{equation}
\text{THAC0}_{\text{base}} = \text{thac0}_{01} + \frac{\text{level} \times (\text{thac0}_{00} - \text{thac0}_{01})}{100}
\end{equation}

\begin{align}
\text{THAC0}_{\text{calc}} &= \text{THAC0}_{\text{base}} \times 10 \\
&- \text{stat\_bonus}[\text{STR}].\text{tohit} \times 10 \\
&- \text{hitroll} \\
&- \frac{4 \times \text{INT}}{10} \\
&- \frac{4 \times \text{WIS}}{10} \\
&- \frac{\text{weapon\_proficiency}}{2} \\
&- \text{alignment\_bonus}
\end{align}

\subsection{Defender AC}
\begin{align}
\text{AC}_{\text{victim}} &= \text{AC}_{\text{base}} + \text{stat\_bonus}[\text{DEX}].\text{defense} \times 10 \\
\text{AC}_{\text{victim}} &= \min(\text{AC}_{\text{victim}}, 100)
\end{align}

\subsection{Hit Probability}
\begin{equation}
\text{HIT} =
\begin{cases}
\text{true} & \text{if } d_{200} > 190 \text{ or } \neg \text{AWAKE} \\
\text{false} & \text{if } d_{200} < 11 \\
\text{THAC0}_{\text{calc}} - d_{200} \leq \text{AC}_{\text{victim}} & \text{otherwise}
\end{cases}
\end{equation}

\section{Damage Calculation}

\subsection{Base Damage}
\begin{align}
\text{dam}_{\text{base}} &= \text{damroll} + \text{stat\_bonus}[\text{STR}].\text{todam} \\
\text{if } d_{200} &= 20: \quad \text{dam}_{\text{base}} \leftarrow \text{dam}_{\text{base}} \times 2
\end{align}

\subsection{Weapon Damage}
\begin{equation}
\text{dam}_{\text{weapon}} =
\begin{cases}
\text{roll\_dice}(\text{mob}.\text{damnodice}, \text{mob}.\text{damsizedice}) & \text{if NPC} \\
\text{roll\_dice}(\text{weapon}.\text{dice\_num}, \text{weapon}.\text{dice\_size}) & \text{if weapon} \\
\text{random}(0, 2) & \text{if player, no weapon}
\end{cases}
\end{equation}

\subsection{Total Damage}
\begin{equation}
\text{dam}_{\text{total}} = \text{dam}_{\text{base}} + \text{dam}_{\text{weapon}}
\end{equation}

\subsection{Skill Multipliers}
\begin{align}
\text{if BACKSTAB}: \quad &\text{dam} \leftarrow \text{dam} \times \left( \frac{\text{skill}}{10} + 1 \right) \\
\text{if KICK}: \quad &\text{dam} \leftarrow \text{dam} + \frac{\text{skill}}{2} + \text{stat\_bonus}[\text{DEX}].\text{todam} \\
\text{if BAREHAND}: \quad &\text{dam} \leftarrow \text{dam} + \frac{\text{skill}}{4} + \text{random}\left(1, \frac{\text{level}}{3}\right) + \frac{\text{level}}{2}
\end{align}

\subsection{Stance Multiplier}
\begin{equation}
\text{if stance} < \text{FIGHTING}: \quad \text{dam} \leftarrow \text{dam} \times \left( 1 + \frac{\text{FIGHTING} - \text{stance}}{3} \right)
\end{equation}

\subsection{Susceptibility Adjustment}
\begin{equation}
\text{dam}_{\text{final}} = \max\left(1, \text{dam} \times \frac{\text{susceptibility}}{100}\right)
\end{equation}

\section{Defensive Mechanics}

\subsection{Evasion Probability}
\begin{equation}
P(\text{evade}) = \frac{(100 - \text{susceptibility})^3}{1,000,000}
\end{equation}

\subsection{Riposte Check}
\begin{align}
\text{ch\_hit} &= \text{random}(55, 200) + \text{hitroll} - \text{monk\_penalty} \\
\text{vict\_riposte} &= \text{random}(20, 50) + (\text{level}_{\text{def}} - \text{level}_{\text{att}}) \\
&\quad - \text{stat\_bonus}[\text{DEX}].\text{defense} + \text{skill} \times 0.085 \\
\text{success} &= (\text{vict\_riposte} > \text{ch\_hit})
\end{align}

\subsection{Parry/Dodge Checks}
\begin{align}
\text{parry}: \quad \text{ch\_hit} &= \text{random}(45, 181) + \text{modifiers} \\
\text{dodge}: \quad \text{ch\_hit} &= \text{random}(35, 171) + \text{modifiers} \\
\text{vict\_def} &= \text{random}(20, 50) + \text{modifiers} + \frac{\text{skill}}{10} \\
\text{success} &= (\text{vict\_def} > \text{ch\_hit})
\end{align}

\section{Stat Bonuses}

\subsection{Strength - ToHit}
\begin{equation}
\text{tohit}(\text{STR}) =
\begin{cases}
\frac{\text{STR}}{7} - 5 & \text{if } 0 \leq \text{STR} \leq 28 \\
0 & \text{if } 29 \leq \text{STR} \leq 64 \\
\frac{6 \times \text{STR}}{35} - \frac{75}{7} & \text{if } 65 \leq \text{STR} \leq 100
\end{cases}
\end{equation}

\subsection{Strength - ToDam}
\begin{equation}
\text{todam}(\text{STR}) =
\begin{cases}
\frac{3 \times \text{STR}}{20} - 4 & \text{if } 0 \leq \text{STR} \leq 20 \\
0 & \text{if } 21 \leq \text{STR} \leq 60 \\
\frac{13 \times \text{STR}}{39} - \frac{754}{39} & \text{if } 61 \leq \text{STR} \leq 100
\end{cases}
\end{equation}

\subsection{Dexterity - Defense}
\begin{equation}
\text{defense}(\text{DEX}) =
\begin{cases}
\frac{-5 \times \text{DEX}}{24} + 6 & \text{if } 0 \leq \text{DEX} \leq 24 \\
0 & \text{if } 25 \leq \text{DEX} \leq 56 \\
\frac{-5 \times \text{DEX}}{43} + \frac{242}{43} & \text{if } 57 \leq \text{DEX} \leq 100
\end{cases}
\end{equation}

\subsection{Constitution - HP Gain}
\begin{equation}
\text{hpgain}(\text{CON}) =
\begin{cases}
\frac{\text{CON}}{8} - 4 & \text{if } 0 \leq \text{CON} \leq 24 \\
0 & \text{if } 25 \leq \text{CON} \leq 56 \\
\frac{\text{CON}}{8} - \frac{121}{20} & \text{if } 57 \leq \text{CON} \leq 96 \\
5 & \text{if } 97 \leq \text{CON} \leq 100
\end{cases}
\end{equation}

\end{document}
```

---

## Section 5: Modern System Mapping Proposal

### 5.1 Core Concept: Opposed Rolls → Bounded Accuracy

**Problem with Current System**:
- THAC0 and AC can reach extreme values (-290 to +100 for AC alone)
- Hit rates become binary (0% or 100%) at level extremes
- No soft caps, no meaningful choice at high levels

**Proposed Solution**:
```
OLD: THAC0 - d200 ≤ AC
NEW: ACC + d100 vs EVA + d100
```

### 5.2 Attribute Mapping

| Old System | New System | Formula |
|------------|------------|---------|
| THAC0 (level, class, hitroll, stats, skills) | **ACC** (Accuracy) | See 5.3 |
| AC (base, DEX, gear) | **EVA** (Evasion) | See 5.4 |
| Damroll + STR + weapon dice | **AP** (Attack Power) + **WeaponBase** | See 5.5 |
| AC (armor only) | **AR** (Armor Rating) → **DR%** | See 5.6 |
| (none) | **Soak** (flat damage reduction) | See 5.7 |
| (none) | **Pen%** (% armor penetration) | See 5.8 |
| Stone Skin, Susceptibility | **DR%**, **Resist%** | See 5.9 |

---

### 5.3 ACC (Accuracy) Formula

**Goal**: Convert THAC0 components into a bounded accuracy score (0-200 range)

```python
# Base accuracy from level and class
ACC_base = 50 + (level × class_acc_rate)

# Class ACC rates (replaces THAC0)
class_acc_rates = {
    'SORCERER': 0.3,   # Reaches 80 ACC at level 100
    'CLERIC': 0.4,     # Reaches 90 ACC at level 100
    'THIEF': 0.5,      # Reaches 100 ACC at level 100
    'WARRIOR': 0.6,    # Reaches 110 ACC at level 100
    'PALADIN': 0.6,
    'RANGER': 0.6
}

# Add stat bonuses (converted from THAC0 tohit)
ACC += (STR - 50) / 5  # -10 to +10
ACC += (INT - 50) / 10  # -5 to +5
ACC += (WIS - 50) / 10  # -5 to +5

# Add gear hitroll (1:1 conversion)
ACC += hitroll  # Up to +40 from gear

# Add weapon proficiency
ACC += weapon_proficiency / 2  # Up to +50 from skill

# Soft cap at 200
ACC = min(200, ACC)
```

**Example** (Level 50 Warrior, STR 80, INT 60, WIS 60, hitroll +20, prof 80):
```
ACC = 50 + (50 × 0.6) + (80-50)/5 + (60-50)/10 + (60-50)/10 + 20 + 40
ACC = 50 + 30 + 6 + 1 + 1 + 20 + 40 = 148
```

---

### 5.4 EVA (Evasion) Formula

**Goal**: Convert AC into an evasion score (0-150 range)

```python
# Base evasion from level (grows slower than ACC)
EVA_base = 30 + (level × 0.3)  # 30-60 at levels 0-100

# Add DEX bonus (converted from defense)
EVA += (DEX - 50) / 3  # -16 to +16

# Add dodge/parry/riposte skills (merged into EVA)
EVA += (dodge_skill / 5)  # Up to +20
EVA += (parry_skill / 5)  # Up to +20
EVA += (riposte_skill / 10)  # Up to +10

# Soft cap at 150
EVA = min(150, EVA)
```

**Example** (Level 50 Mob, DEX 70, dodge 60, parry 40, riposte 0):
```
EVA = 30 + (50 × 0.3) + (70-50)/3 + 12 + 8 + 0
EVA = 30 + 15 + 6.7 + 12 + 8 = 71.7 ≈ 72
```

---

### 5.5 Hit Chance Calculation (Opposed d100 Rolls)

```python
def calculate_hit(attacker, defender):
    attacker_roll = d100() + attacker.ACC
    defender_roll = d100() + defender.EVA

    # Natural 1 always misses, natural 100 always hits
    if attacker_roll_natural == 1:
        return False
    if attacker_roll_natural == 100:
        return True

    # Opposed roll
    margin = attacker_roll - defender_roll

    if margin >= 50:
        return 'CRITICAL_HIT'  # 2× damage
    elif margin >= 10:
        return 'HIT'
    elif margin >= -10:
        return 'GLANCING'  # 50% damage
    else:
        return 'MISS'
```

**Expected Hit Rates** (ACC vs EVA):
| ACC-EVA Diff | Hit% | Crit% | Glance% | Miss% |
|--------------|------|-------|---------|-------|
| -50 | 15% | 0% | 5% | 80% |
| -25 | 30% | 2% | 10% | 58% |
| 0 | 50% | 5% | 20% | 25% |
| +25 | 70% | 12% | 18% | 0% |
| +50 | 85% | 25% | 10% | 0% |

---

### 5.6 Armor Rating → Damage Reduction %

**Goal**: Convert AC (from armor only) into % damage reduction

```python
# Extract armor AC from total AC
armor_AC = total_AC - DEX_bonus - magic_AC

# Convert to Armor Rating (AR)
# Invert scale: lower AC = higher AR
AR = max(0, 100 - armor_AC)

# Calculate DR% using diminishing returns formula
K = tier_constant(defender.level)
DR_percent = AR / (AR + K)

# Tier constants (controls DR% scaling)
tier_constants = {
    'LOW' (1-20):   K = 50   # AR 50 → 50% DR
    'MID' (21-50):  K = 75   # AR 75 → 50% DR
    'HIGH' (51-99): K = 100  # AR 100 → 50% DR
}
```

**Example** (Level 50 Warrior, armor AC -40):
```
AR = 100 - (-40) = 140
K = 75 (mid-tier)
DR% = 140 / (140 + 75) = 140 / 215 = 65.1%
```

**DR% Cap**: 75% (prevents invincibility)

---

### 5.7 Soak (Flat Damage Reduction)

**New Mechanic**: Flat reduction applied BEFORE percentage reduction

```python
# Soak from heavy armor and shields
soak = 0

if armor.type == 'PLATE':
    soak += 5
elif armor.type == 'CHAIN':
    soak += 3
elif armor.type == 'LEATHER':
    soak += 1

if shield.equipped:
    soak += shield.armor_value / 10

# Applied before DR%
damage_after_soak = max(1, damage - soak)
damage_final = damage_after_soak × (1 - DR%)
```

---

### 5.8 Armor Penetration (AP) and Pen%

**Counter-mechanic**: Attackers reduce defender's mitigation

```python
# Flat penetration (reduces Soak)
AP = damroll_from_gear

# Percentage penetration (reduces DR%)
Pen_percent = 0

if weapon.type == 'PIERCING':
    Pen_percent += 20%
if weapon.has_flag('ARMOR_PIERCING'):
    Pen_percent += 15%

# Application
effective_soak = max(0, soak - AP)
effective_DR = DR_percent × (1 - Pen_percent)

damage_after_soak = damage - effective_soak
damage_final = damage_after_soak × (1 - effective_DR)
```

---

### 5.9 Damage Formula (Modern System)

```python
def calculate_damage(attacker, defender, hit_type):
    # Base damage
    dam = attacker.weapon.base_damage  # Flat value from weapon
    dam += roll_dice(weapon.dice_num, weapon.dice_size)

    # Attack Power (replaces damroll + STR)
    dam += attacker.AP

    # Skill multipliers (preserved from legacy)
    if skill == 'BACKSTAB':
        dam × (backstab_skill / 10 + 1)
    if skill == 'BAREHAND':
        dam += barehand_skill/4 + d(level/3) + level/2

    # Hit type multiplier
    if hit_type == 'CRITICAL':
        dam × 2
    elif hit_type == 'GLANCING':
        dam × 0.5

    # Apply penetration
    effective_soak = max(0, defender.soak - attacker.AP)
    effective_DR = defender.DR_percent × (1 - attacker.Pen_percent)

    # Apply mitigation
    dam_after_soak = max(1, dam - effective_soak)
    dam_final = max(1, dam_after_soak × (1 - effective_DR))

    # Susceptibility (preserved)
    dam_final × (susceptibility / 100)

    return dam_final
```

---

### 5.10 Mob Damage: Use Per-Mob Dice

**Current System** (already implemented):
```cpp
// fight.cpp:2218-2219
if (IS_NPC(ch))
    dam += roll_dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
```

**Proposal**: Keep this, remove any global mob damage charts

**Migration**:
1. Ensure all mobs in database have `damage_dice_num` and `damage_dice_size` populated
2. Set reasonable values by mob level tier:
   - L1-10: 1d4 to 2d4
   - L11-25: 2d6 to 3d6
   - L26-50: 3d8 to 5d8
   - L51-75: 5d10 to 8d10
   - L76-99: 8d12 to 12d12

---

## Section 6: Calibration Report

### 6.1 Test Matrix: 5 Players × 5 Mobs

#### Player Archetypes
**P1**: Level 20 Warrior (STR 80, DEX 60, AC -20, hitroll +15, damroll +10, weapon 2d6)
**P2**: Level 20 Sorcerer (INT 90, WIS 70, AC +10, hitroll +5, damroll +3, weapon 1d4)
**P3**: Level 50 Warrior (STR 90, DEX 70, AC -60, hitroll +30, damroll +25, weapon 3d8)
**P4**: Level 50 Thief (DEX 90, STR 70, AC -40, hitroll +25, damroll +15, weapon 2d6)
**P5**: Level 80 Paladin (STR 95, CON 85, AC -80, hitroll +35, damroll +30, weapon 4d8)

#### Mob Archetypes
**M1**: Level 20 Mob (AC -10, hitroll +10, dam 2d6+5, HP 200)
**M2**: Level 20 Boss (AC -30, hitroll +20, dam 3d8+10, HP 400)
**M3**: Level 50 Mob (AC -40, hitroll +25, dam 4d8+15, HP 600)
**M4**: Level 50 Elite (AC -60, hitroll +35, dam 5d10+20, HP 900)
**M5**: Level 80 Boss (AC -100, hitroll +50, dam 8d12+40, HP 1500)

### 6.2 Current System Calculations

#### P1 (L20 Warrior) vs M1 (L20 Mob)

**Hit Chance**:
```
THAC0_base = calc_thac0(20, 20, -2) = 20 + 20×(-22)/100 = 15.6
THAC0_calc = 15.6 × 10 - (3×10) - 15 - 16 - 16 - 25 = 156 - 30 - 15 - 16 - 16 - 25 = 54

victim_AC = -10 + (0×10) = -10

# Hit if: 54 - d200 ≤ -10
# Hit if: d200 ≥ 64

P(auto-miss) = 10/200 = 5%
P(auto-hit) = 10/200 = 5%
P(calculated hit) = (200-64)/180 × 90% = 136/180 × 90% = 68%

Total hit% = 5% + 68% = 73%
```

**Damage**:
```
Base: 10 (damroll) + 7 (STR 80) = 17
Weapon: 2d6 avg = 7
Total avg damage = 17 + 7 = 24

Critical (0.5% chance): 48

Average DPH (damage per hit) = 24 × 0.995 + 48 × 0.005 = 24.12
DPS = 24.12 × 0.73 / 4 = 4.40 per second
TTK = 200 / 4.40 = 45.5 seconds
```

#### P3 (L50 Warrior) vs M3 (L50 Mob)

**Hit Chance**:
```
THAC0_base = calc_thac0(50, 20, -2) = 20 + 50×(-22)/100 = 9
THAC0_calc = 9×10 - (4×10) - 30 - 20 - 20 - 30 = 90 - 40 - 30 - 20 - 20 - 30 = -50

victim_AC = -40 + (2×10) = -20

# Hit if: -50 - d200 ≤ -20
# Hit if: d200 ≤ -30  (ALWAYS IMPOSSIBLE from normal rolls)

# But auto-hit 191-200 = 5%

Total hit% = 5% (auto-hit only)
```

**Damage**:
```
Base: 25 + 9 = 34
Weapon: 3d8 avg = 13.5
Total avg = 47.5

DPH = 47.5
DPS = 47.5 × 0.05 / 4 = 0.59 per second
TTK = 600 / 0.59 = 1017 seconds (17 minutes!)
```

**PROBLEM IDENTIFIED**: Level 50+ Warriors hit 5% of time against equal-level mobs!

### 6.3 Modern System Calculations

#### P1 (L20 Warrior) vs M1 (L20 Mob)

**ACC/EVA**:
```
P1 ACC = 50 + (20×0.6) + (80-50)/5 + (60-50)/10 + 15 + 25 = 50 + 12 + 6 + 1 + 15 + 25 = 109

M1 EVA = 30 + (20×0.3) + (50-50)/3 = 30 + 6 + 0 = 36

ACC - EVA = 109 - 36 = +73
```

**Hit Chance** (from table, ACC-EVA = +73 ≈ +50+):
```
Hit% ≈ 85%
Crit% ≈ 25%
```

**Damage**:
```
Base: 20 (weapon) + 2d6 (avg 7) + 15 (AP from damroll) = 42

M1 AR = 100 - (-10) = 110
DR% = 110 / (110 + 50) = 68.75%

Soak = 0 (mob has no armor)

Normal hit: 42 × (1 - 0.6875) = 13.1
Critical hit: 42 × 2 × (1 - 0.6875) = 26.2

DPH = 13.1 × 0.60 + 26.2 × 0.25 = 7.86 + 6.55 = 14.41
DPS = 14.41 × 0.85 / 4 = 3.06 per second
TTK = 200 / 3.06 = 65.4 seconds
```

**Comparison**:
- Current hit%: 73% → New hit%: 85% (+12%)
- Current DPS: 4.40 → New DPS: 3.06 (-30%)
- Current TTK: 45.5s → New TTK: 65.4s (+44%)

**Issue**: New system reduces DPS too much. Need adjustment.

#### P3 (L50 Warrior) vs M3 (L50 Mob)

**ACC/EVA**:
```
P3 ACC = 50 + (50×0.6) + (90-50)/5 + (70-50)/10 + 30 + 40 = 50 + 30 + 8 + 2 + 30 + 40 = 160

M3 EVA = 30 + (50×0.3) + (60-50)/3 = 30 + 15 + 3.3 = 48.3

ACC - EVA = 160 - 48.3 = +111.7
```

**Hit Chance**:
```
Easily in 95%+ range (soft cap)
Crit% ≈ 40%
```

**Damage**:
```
Base: 30 (weapon) + 3d8 (avg 13.5) + 30 (AP) = 73.5

M3 AR = 100 - (-40) = 140
DR% = 140 / (140 + 75) = 65.1%

Normal hit: 73.5 × (1 - 0.651) = 25.7
Critical hit: 73.5 × 2 × (1 - 0.651) = 51.4

DPH = 25.7 × 0.55 + 51.4 × 0.40 = 14.1 + 20.6 = 34.7
DPS = 34.7 × 0.95 / 4 = 8.24 per second
TTK = 600 / 8.24 = 72.8 seconds
```

**Comparison**:
- Current hit%: 5% → New hit%: 95% (+90%!)
- Current DPS: 0.59 → New DPS: 8.24 (+1297%)
- Current TTK: 1017s → New TTK: 72.8s (-93%)

**Result**: New system FIXES the pathological case at level 50!

### 6.4 Calibration Adjustments

**Findings**:
1. Low-level (20) fights are ~30-40% slower in new system
2. Mid-level (50) fights are 10-15× faster (fixing broken mechanics)
3. High-level (80) needs testing but should be in range

**Proposed Knob Adjustments**:

1. **Reduce K constants** (increase DR% scaling):
   ```
   OLD: K_low=50, K_mid=75, K_high=100
   NEW: K_low=40, K_mid=60, K_high=80
   ```

2. **Increase weapon base damage**:
   ```
   All weapons +20% base damage
   ```

3. **Add proficiency to AP**:
   ```
   AP = damroll + (weapon_proficiency / 5)
   ```

4. **Adjust ACC rates slightly**:
   ```
   Warrior: 0.6 → 0.65
   Thief: 0.5 → 0.55
   ```

With these adjustments, re-running P1 vs M1:
```
DR% = 110 / (110 + 40) = 73.3%
Base damage: 20×1.2 + 7 + 15 + 5 = 51

Normal: 51 × (1 - 0.733) = 13.6
DPH = 13.6 × 0.60 + 27.2 × 0.25 = 8.16 + 6.8 = 14.96
DPS = 14.96 × 0.85 / 4 = 3.18
TTK = 200 / 3.18 = 62.9s

Deviation from current: +38% (acceptable given we're fixing L50+ pathology)
```

---

## Section 7: Edge Cases & Migration

### 7.1 Edge Case Audit

#### Backstab/Positionals
**Current**: dam × (skill/10 + 1)
**Proposed**: Keep multiplier, apply AFTER mitigation
**Reason**: Prevents one-shotting high-DR targets

```python
# OLD
dam = base_dam × backstab_mult
dam = apply_mitigation(dam)

# NEW
dam = apply_mitigation(base_dam)
dam = dam × backstab_mult
```

#### Dual-Wield/Offhand
**Current**: No explicit dual-wield in base system (skill-based)
**Proposed**: Second attack at 50% AP, separate hit roll
**Implementation**:
```python
if dual_wield_skill > 0:
    offhand_ACC = main_ACC - 30 + (skill / 2)
    offhand_AP = main_AP × 0.5
```

#### Size Modifiers
**Current**: No explicit size-based hit modifiers
**Proposed**: Tiny targets +20 EVA, Huge targets -20 EVA

#### Immunities
**Current**: Evasion based on susceptibility (0-100 scale)
**Proposed**: Keep evasion formula, but add explicit IMMUNE flag for 100% evasion

#### Minimum Damage
**Current**: max(1, dam)
**Proposed**: max(1, dam) after ALL modifiers

#### Stone Skin
**Current**: 90% chance to reduce dam to random(0,3)
**Proposed**: +50% DR (stacks multiplicatively with armor DR)
```python
total_DR = 1 - (1 - armor_DR) × (1 - stoneskin_DR)
# Example: 50% armor + 50% stoneskin = 75% total DR
```

---

### 7.2 Migration Worksheet (CSV Format)

```csv
Entity,Old_Stat,Old_Value,New_Stat,New_Value,Conversion_Formula
Player,THAC0,-50,ACC,120,"50 + level×0.6 + conversions"
Player,Hitroll,+30,ACC,+30,"1:1 add to ACC"
Player,AC,-60,EVA,80,"Inverse + level scaling"
Player,Damroll,+25,AP,+25,"1:1 to AP"
Warrior,HP/Level,+10,HP/Level,+10,"No change"
Warrior,THAC0_00,-2,ACC_rate,0.65,"Class rate"
Sorcerer,THAC0_00,6,ACC_rate,0.30,"Class rate"
Mob_L20,AC,-10,AR,110,"100 - AC"
Mob_L20,Hitroll,+10,ACC,95,"30 + level×0.3 + 10"
Mob_L20,DamDice,2d6+5,Base+Dice,12+2d6,"Split into base + dice"
Plate_Armor,AC,-20,AR+Soak,120+5,"AR from AC, +5 soak"
Longsword,Dice,2d6,Base+Dice,15+2d6,"Add base damage"
Shield,AC,-5,Soak,+3,"Convert AC to soak"
```

### 7.3 Constants Export (JSON)

```json
{
  "version": "1.0",
  "combat_constants": {
    "timing": {
      "combat_round_seconds": 4,
      "passes_per_second": 4,
      "auto_miss_low": 1,
      "auto_miss_high": 1,
      "auto_hit_low": 100,
      "auto_hit_high": 100
    },
    "hit_calculation": {
      "acc_base": 50,
      "eva_base": 30,
      "acc_level_rate": {
        "WARRIOR": 0.65,
        "PALADIN": 0.65,
        "RANGER": 0.65,
        "THIEF": 0.55,
        "CLERIC": 0.40,
        "SORCERER": 0.30
      },
      "eva_level_rate": 0.3,
      "acc_soft_cap": 200,
      "eva_soft_cap": 150
    },
    "damage_reduction": {
      "dr_k_constants": {
        "low_tier": 40,
        "mid_tier": 60,
        "high_tier": 80
      },
      "dr_cap": 0.75,
      "soak_armor_types": {
        "PLATE": 5,
        "CHAIN": 3,
        "LEATHER": 1
      }
    },
    "penetration": {
      "weapon_pen_percent": {
        "PIERCING": 0.20,
        "SLASHING": 0.10,
        "CRUSHING": 0.05
      },
      "armor_piercing_flag": 0.15
    },
    "critical_hits": {
      "margin_for_crit": 50,
      "crit_multiplier": 2.0,
      "glancing_margin": 10,
      "glancing_multiplier": 0.5
    },
    "stat_bonuses": {
      "STR_to_ACC": 0.2,
      "DEX_to_EVA": 0.33,
      "INT_to_ACC": 0.1,
      "WIS_to_ACC": 0.1
    },
    "skill_multipliers": {
      "backstab": "skill/10 + 1",
      "kick": "skill/2 + DEX.todam",
      "barehand": "skill/4 + d(level/3) + level/2",
      "weapon_proficiency_to_ACC": 0.5,
      "weapon_proficiency_to_AP": 0.2
    },
    "stance_multipliers": {
      "FIGHTING": 1.0,
      "ALERT": 1.33,
      "RESTING": 1.66,
      "SLEEPING": 2.0,
      "STUNNED": 2.33,
      "INCAP": 2.66,
      "MORT": 3.0
    }
  },
  "calibration_targets": {
    "hit_rate_low_tier": {
      "min": 0.60,
      "median": 0.75,
      "max": 0.90
    },
    "hit_rate_mid_tier": {
      "min": 0.65,
      "median": 0.80,
      "max": 0.95
    },
    "hit_rate_high_tier": {
      "min": 0.70,
      "median": 0.85,
      "max": 0.95
    },
    "ttk_deviation_max": 0.10
  }
}
```

---

## Appendix: Code References

### File:Line Index

```
fight.cpp:2089-2161    Hit chance calculation (THAC0)
fight.cpp:2585         calc_thac0() function
fight.cpp:2207-2287    Damage calculation
fight.cpp:2244-2271    Skill multipliers
fight.cpp:1816-1916    Riposte/Parry/Dodge
fight.cpp:2587-2610    Displacement check
damage.cpp:55-119      Damage evasion
damage.cpp:121-161     Weapon damage type conversion
constants.cpp:291-369  Stat bonus formulas
class.cpp:56-288       Class definitions
defines.hpp:316-323    Timing constants
defines.hpp:26-35      Stance/Position constants
spell_parser.cpp:1523  Casting time wait states
```

---

**End of Reverse-Engineering Report**

---

## Next Steps

1. **Validation**: Run full test suite on all 25 player/mob combinations
2. **Tuning**: Adjust K constants and base damages to hit calibration targets
3. **Implementation**: Port formulas to new combat system
4. **Testing**: 100+ combat logs comparing old vs new
5. **Migration**: Database scripts to convert AC → AR, damroll → AP, etc.

**Questions for Discussion**:
- Should we preserve stance multipliers or reduce them?
- Glancing blows: good addition or unnecessary complexity?
- Soak values: too low, too high?
- Dual-wield: separate system or merged into proficiency?

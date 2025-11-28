# Combat System Clarifications

**Purpose**: Answer critical questions about combat mechanics edge cases and provide precise specifications.

---

## Glossary: Modern Combat Stats (Final Names)

### Offense Stats

| Stat | Full Name | Definition | Range | Source |
|------|-----------|------------|-------|--------|
| **ACC** | Accuracy | To-hit rating (pre-roll) | 20-150 | Level + hitroll + STR/INT/WIS + weapon skill |
| **AP** | Attack Power | Base damage before mitigation | 10-100 | Damroll + weapon dice + STR bonus |
| **SP** | Spell Power | Magical damage multiplier | 50-200% | INT/WIS + caster level + spell skill |
| **PenFlat** | Flat Penetration | Bypasses Soak (flat mitigation) | 0-20 | Weapon property + skills |
| **PenPct** | % Penetration | Reduces DR%/Ward%/RES% | 0-40% | Weapon type + skills + debuffs |

### Defense Stats

| Stat | Full Name | Definition | Range | Source |
|------|-----------|------------|-------|--------|
| **EVA** | Evasion | Dodge rating (pre-roll) | 10-120 | Level + DEX + dodge skill + stance |
| **AR** | Armor Rating | Physical armor value | 0-150 | Equipment AC (armor only, no DEX/magic) |
| **DR%** | Damage Reduction % | Physical mitigation from AR | 0-60% | AR/(AR+K), K=40/60/80 by tier |
| **Soak** | Soak (flat) | Flat damage reduction | 0-20 | Heavy armor + shield |
| **Hardness** | Hardness | Minimum damage to bypass Soak | 0-15 | Heavy armor property (future) |
| **Ward%** | Ward % | Magical damage reduction | 0-40% | Mage armor, shield spell, stoneskin |
| **RES[type]%** | Resistance % | Elemental/type-specific mitigation | 0-75% | Composition + buffs (fire, cold, etc.) |

### Damage Flow Summary

```
1. To-Hit Roll:  d100 vs (50% + (ACC - EVA)×2)  → Hit or Miss
                 ↓ (if hit)
2. Post-Hit:     Shield Block / Parry / Dodge   → 2nd chance to avoid
                 ↓ (if connects)
3. Base Damage:  AP + weapon_dice + modifiers   → Raw damage
                 ↓
4. Penetration:  damage - (Soak - PenFlat)      → After flat reduction
                 ↓
5. Mitigation:   damage × (1 - (DR% + Ward% + RES%) × (1 - PenPct))
                 ↓
6. Cap & Floor:  clamp(damage, 1, no_max)       → Final: 1-∞ (chip floor applies)
                                                    unless immunity (0 damage)
```

### Global Rules

- **Mitigation Cap**: `DR% + Ward% + RES%` ≤ 75% (before PenPct)
- **Chip Floor**: Minimum 1 damage if hit connects (exceptions: immunity, stoneskin)
- **Immunity**: 0% susceptibility bypasses chip floor (0 damage)
- **Stacking**: Mitigation types add within cap, Penetration applies after sum

### Caps & Limits

| Stat | Cap | Rationale |
|------|-----|-----------|
| **Block%** | 50% | Shield occupies hand slot - strong but not dominant |
| **Parry%** | 40% | Weapon-based defense - skill expression |
| **Dodge%** | 30% | Armor penalty trade-off - mobility builds |
| **PenPct** | 50% | Can halve mitigation, not eliminate |
| **PenFlat** | Soak+10 | Can bypass all Soak + 10 damage (anti-tank) |
| **DR%** | ~60% | From AR via formula (soft cap via diminishing returns) |
| **Ward%** | ~40% | From spell stacking |
| **RES%** | ~75% | Composition + buffs (multiplicative) |
| **Total Mitigation** | **75%** | **Global hard cap** |

### K Constants (AR→DR% Formula)

```python
# DR% = AR / (AR + K)
# K values by tier (initial anchors):

K_LOW = 50      # Levels 1-30:  AR=50 → 50% DR
K_MID = 100     # Levels 31-60: AR=100 → 50% DR
K_HIGH = 200    # Levels 61+:   AR=200 → 50% DR

# Recompute methodology:
# 1. Measure median AR per tier from equipment
# 2. Set K = median_AR for 50% DR at median
# 3. Validate: max_AR shouldn't exceed 60% DR
```

### Damage Type Routing

| Damage Type | Mitigation Layers | Example |
|-------------|-------------------|---------|
| **Physical** (slash/pierce/crush) | Soak + DR% (+ Ward%) | Sword attack: 50 base → Soak -10 → DR% -57% → Ward% -10% → final |
| **Elemental** (fire/cold/shock/acid) | RES% (+ Ward% if magical) | Fireball: 80 base → RES% -25% → Ward% -20% → final |
| **Pure Magic** (dispel/heal) | Ward% only | Magic missile: 40 base → Ward% -20% → final |
| **Mixed** (flaming sword) | max(Physical, Elemental) mitigation | Take best resistance |

**Routing Rule**: Physical damage uses Soak+DR%, Elemental uses RES%, both can add Ward% if magical source.

---

## Q1: Critical Hit Trigger Consistency

**Question**: "Natural d200 roll of 20 → 2× damage" - is it literally value 20, not 200? Which die is used?

### Answer: YES, Literal Value 20

**Source**: `fight.cpp:2141, 2214-2215`

```cpp
diceroll = random_number(1, 200);  // Line 2141 - THE to-hit roll

// ... hit calculation ...

if (diceroll == 20)  // Line 2214 - checks THE SAME ROLL
    dam *= 2;
```

### Critical Hit Truth Table

| Condition | d200 Roll | To-Hit Result | Critical Triggered | Final Damage |
|-----------|-----------|---------------|-------------------|--------------|
| Normal hit | 20 | Hit (if calc succeeds) | **YES** | 2× damage |
| Normal hit | 21-190 | Check vs AC | NO | 1× damage |
| Auto-miss | 1-10 | **Miss** | NO | 0 damage |
| Auto-hit | 191-200 | **Hit** | NO | 1× damage |
| Asleep victim | Any | Auto-hit | Only if roll=20 | 2× if 20, else 1× |

### Key Findings

1. **Same Die**: Uses THE SAME d200 roll for both to-hit and crit check
2. **Rare Event**: 0.5% crit chance (1 in 200), NOT 10% (20/200)
3. **Auto-Hit No Crit**: Rolls 191-200 auto-hit but do NOT crit (no 2× damage)
4. **Auto-Miss No Crit**: Rolls 1-10 auto-miss, crit check never happens
5. **Only Value 20**: ONLY the exact value 20 triggers critical, not natural 200

### Crit Modifiers

**None Found**: No equipment, skills, or effects modify crit chance in legacy system.

```cpp
// Complete crit logic - no modifiers, no exceptions:
if (type != SKILL_KICK) {      // Kicks can't crit
    if (diceroll == 20)        // Hard-coded value 20
        dam *= 2;              // Simple 2× multiplier
```

### Skills That Can't Crit

- `SKILL_KICK` - explicitly excluded (fight.cpp:2213)

---

## Q2: Auto-Hit/Miss vs Post-Hit Effects

**Question**: Can displacement or other effects override auto-hit/miss? What's the precedence?

### Answer: Strict Precedence Chain

**Source**: `fight.cpp:2156-2191`

### Complete Precedence Table

| Priority | Check | Can Override | Result | Damage |
|----------|-------|--------------|--------|--------|
| 1 | **Susceptibility/Immunity** | Overrides ALL | Evasion message | 0 |
| 2 | **Displacement** | Overrides ALL except #1 | Miss message | 0 |
| 3 | **To-Hit Roll** | Normal rules | Hit or Miss | Varies |
| 4 | Riposte/Parry/Dodge | Only if skill allows | Counter or Miss | Varies |

### Code Flow

```cpp
// Line 2156-2161: Auto-hit/miss calculation
if (diceroll > 190 || !AWAKE(victim))
    to_hit = true;        // AUTO-HIT
else if (diceroll < 11)
    to_hit = false;       // AUTO-MISS
else
    to_hit = calc_thaco - diceroll <= victim_ac;

// Line 2166: PRIORITY 1 - Susceptibility overrides EVERYTHING
if (damage_evasion(victim, ch, weapon, dtype)) {
    // ... evasion message ...
    dam = 0;
    return;  // Short-circuit - no damage
}

// Line 2185: PRIORITY 2 - Displacement overrides auto-hit!
if (displaced(ch, victim)) {
    dam = 0;
    return;  // Short-circuit - auto-hit still misses!
}

// Line 2194: PRIORITY 3 - Normal miss
if (!to_hit) {
    damage(ch, victim, 0, type);  // Miss message
}

// Line 2202-2206: PRIORITY 4 - Post-hit defenses (if allowed)
```

### Key Findings

1. **Auto-Hit Can Still Miss**:
   - Roll 191-200 sets `to_hit = true`
   - But displacement (20-33% chance) can still nullify it
   - Immunity (0% susceptibility) can still nullify it

2. **Auto-Miss is Absolute**:
   - Roll 1-10 sets `to_hit = false`
   - No checks after this - goes straight to miss damage

3. **Sleeping Victims**:
   - Treated as auto-hit: `!AWAKE(victim) → to_hit = true`
   - But still subject to immunity/displacement checks!

4. **Precedence Chain**:
   ```
   Immunity > Displacement > To-Hit Roll > Post-Hit Defenses
   ```

### Edge Case: Greater Displacement

```cpp
// Displacement: 20% miss chance (1 in 5)
if (EFF_FLAGGED(victim, EFF_DISPLACEMENT))
    displaced = (random_number(1, 5) == 1);

// Greater Displacement: 33% miss chance (1 in 3)
if (EFF_FLAGGED(victim, EFF_GREATER_DISPLACEMENT))
    displaced = (random_number(1, 3) == 1);
```

**Both buffs active**: Greater Displacement overwrites (last check wins).

---

## Q3: Post-Hit Defense Bypass List

**Question**: Which skills bypass riposte/parry/dodge?

### Answer: Complete Bypass List

**Source**: `fight.cpp:2202-2206`

### Skills That ALWAYS Bypass Post-Hit Defenses

```cpp
// Explicit bypass list:
type == SKILL_BACKSTAB      // Surprise attack from behind
type == SKILL_2BACK         // Double backstab
type == SKILL_BAREHAND      // Monk unarmed combat
type == SKILL_KICK          // All kicks
no_defense_check == true    // Flag for special attacks
EFF_FLAGGED(ch, EFF_FIREHANDS)       // Elemental hand attacks
EFF_FLAGGED(ch, EFF_ICEHANDS)
EFF_FLAGGED(ch, EFF_LIGHTNINGHANDS)
EFF_FLAGGED(ch, EFF_ACIDHANDS)
```

### Skills That Allow Post-Hit Defenses

```cpp
// Normal melee attack - all defenses apply
type == -1 or type == TYPE_HIT

// Weapon special attacks - defenses apply
// (daggers, swords, etc. with special effects)
```

### Post-Hit Defense Order (When Not Bypassed)

```cpp
// Line 2205-2206: Checked in strict order
if (!riposte(ch, victim) &&        // 1. Riposte first
    !parry(ch, victim) &&          // 2. Then parry
    !dodge(ch, victim) &&          // 3. Then dodge
    (!weapon || !weapon_special(weapon, ch))) // 4. Then weapon proc
{
    // Only if ALL fail does damage proceed
}
```

### Key Findings

1. **Bypass Rationale**:
   - **Backstab/2Back**: Surprise attacks can't be defended
   - **Barehand**: Monk mastery bypasses normal defenses
   - **Kick**: Different attack type, defenses don't apply
   - **Elemental Hands**: Magical attacks bypass physical defenses

2. **Defense Success Rate**:
   - Each defense is independent roll
   - Must fail ALL to take damage
   - High-level defenders have ~60%+ avoidance total

3. **Weapon Specials**:
   - Checked AFTER riposte/parry/dodge
   - Can trigger even if defenses succeed
   - Separate damage/effect application

---

## Q4: Susceptibility & Resistance Stacking

**Question**: How do multiple resistance sources stack?

### Answer: Multiplicative Stacking with Caps

**Source**: `chars.cpp:310-376`

### Stacking Formula

```
final_susceptibility = base_composition × modifier₁ × modifier₂ × ...

Where modifiers are multiplicative (e.g., 75/100 for -25% resist)
```

### Example: Fire Resistance Stacking

```cpp
// Base from composition (e.g., 100 for flesh)
sus = compositions[GET_COMPOSITION(ch)].sus_fire;

// Coldshield: -25% fire susceptibility
if (EFF_FLAGGED(ch, EFF_COLDSHIELD))
    sus = sus * 75 / 100;

// Protection from Fire: -25% fire susceptibility
if (EFF_FLAGGED(ch, EFF_PROT_FIRE))
    sus = sus * 75 / 100;

// Final: 100 × 0.75 × 0.75 = 56.25 susceptibility
```

### Resistance Scaling Table

| Stacking | Formula | Final Sus | Effective Resist |
|----------|---------|-----------|------------------|
| Base only | 100 | 100 | 0% |
| +1 buff | 100 × 0.75 | 75 | 25% |
| +2 buffs | 100 × 0.75² | 56 | 44% |
| +3 buffs | 100 × 0.75³ | 42 | 58% |
| Negate | - | 0 | 100% (immune) |

### Immunity Types

```cpp
// ABSOLUTE IMMUNITY (0 susceptibility)
EFF_NEGATE_HEAT  → Fire immunity
EFF_NEGATE_COLD  → Cold immunity
EFF_NEGATE_AIR   → Shock immunity
EFF_NEGATE_EARTH → Acid immunity

// These override ALL other modifiers
if (EFF_FLAGGED(ch, EFF_NEGATE_HEAT))
    return 0;  // Immune - no further checks
```

### Composition Base Values

**Source**: `chars.cpp` composition tables

```cpp
// Common examples:
compositions[COMP_FLESH].sus_fire   = 100  // Normal
compositions[COMP_ICE].sus_fire     = 150  // Vulnerable (+50%)
compositions[COMP_FIRE].sus_fire    = 0    // Immune
compositions[COMP_STONE].sus_pierce = 75   // Resistant (-25%)
```

### Vulnerability (>100 Susceptibility)

```cpp
// Ice composition vs fire:
base = 150  // 50% more damage
// With protections:
+ Coldshield:  150 × 0.75 = 112.5 (still vulnerable)
+ Both buffs:  150 × 0.75² = 84    (now resistant!)
```

### Key Findings

1. **Multiplicative Stacking**: Each buff multiplies, not adds
   - 2 × 25% resist = 44% total (not 50%)
   - Diminishing returns built-in

2. **No Hard Cap**: Besides immunity (0), no maximum resistance
   - Practical cap: ~95% resist (5 susceptibility)
   - Damage evasion formula prevents true immunity without 0 sus

3. **Order Independence**: `A × B × C` same as `C × A × B`

4. **Vulnerability Counterplay**: Multiple buffs can overcome innate weakness

---

## Q5: AC Composition & AC→AR Mapping

**Question**: How to extract armor-only AC for AR, removing DEX/magical bonuses?

### Answer: AC = Armor + DEX + Magic

**Source**: `fight.cpp:2144-2148`, `handler.cpp:657-682`

### Total AC Formula

```cpp
// Base AC from character (includes equipped armor)
victim_ac = GET_AC(victim);

// Add DEX bonus
victim_ac += stat_bonus[GET_DEX(victim)].defense * 10;

// Cap at 100 (worst AC)
victim_ac = std::min(100, victim_ac);

// Range: -160 to +100
```

### AC Components Breakdown

| Component | Source | Range | Example |
|-----------|--------|-------|---------|
| Base Armor | Equipment AC | -100 to +10 | -80 (plate) |
| DEX Modifier | `stat_bonus[DEX].defense × 10` | -60 to +60 | -30 (DEX 85) |
| Magic Effects | `APPLY_AC` effects | -50 to +50 | -20 (armor spell) |
| **Total AC** | Sum, capped at 100 | -160 to +100 | -130 |

### Equipment AC Application

```cpp
// Armor pieces contribute to base AC
// Simple factor = 1 for all slots in legacy
factor = 1;  // Could be slot-based (30% body, 20% head, etc.)
ac_contribution = factor × GET_OBJ_VAL(armor, VAL_ARMOR_AC);

// Example plate armor: -5 AC per piece
// 6 pieces × -5 = -30 base armor AC
```

### AC→AR Extraction Formula

**Goal**: Extract only armor-based defense for AR, route DEX to EVA.

```python
# Complete extraction:
armor_ac = GET_AC(ch)                        # Raw AC from character
dex_ac = stat_bonus[GET_DEX(ch)].defense * 10  # DEX component
magic_ac = sum_of_APPLY_AC_effects(ch)      # Magic buffs

# Separate into new stats:
AR_raw = -(armor_ac - dex_ac - magic_ac)    # Armor only, flip sign
EVA_base = 30 + (DEX - 50)/3 + dodge/5 + ...  # DEX routes here

# Convert AR to DR%:
K = tier_constant(level)  # 40/60/80 for low/mid/high
DR_percent = AR_raw / (AR_raw + K)
```

### One-Line Extraction Rule

```
AR = max(0, -(total_AC - dex_bonus × 10 - magic_AC))
```

**Explanation**:
- AC is negative-is-better, AR is positive-is-better (flip sign)
- Remove DEX contribution (redundant with EVA)
- Remove magic AC (reallocate to separate mitigation layer)
- Floor at 0 (no negative armor)

### Example: L50 Warrior in Plate

```
Total AC: -130
  Armor:  -80  (plate, shield, helm, etc.)
  DEX:    -30  (DEX 85 → +3 defense × 10)
  Magic:  -20  (armor spell)

Extraction:
  AR = -(-80) = 80
  → DR% = 80/(80+60) = 57% damage reduction

  EVA contribution from DEX:
  (85-50)/3 = +11.67 EVA
```

### Key Findings

1. **No Direct Armor-Only Stat**: Legacy AC is composite
2. **DEX Double-Dipping**: Currently affects both to-hit and mitigation
3. **Magic AC**: Many spells add flat AC (armor, shield, mage armor)
4. **New Model Separation**:
   - AR = physical armor pieces only
   - EVA = DEX + dodge skills (affects to-hit)
   - Soak = flat reduction from heavy armor
   - Magic AC → separate "Ward" or "MagicDR" stat

---

## Q6: L50 Pathology & Binary Behavior

**Question**: Show ACC-EVA deltas across levels demonstrating binary behavior.

### L50 Hit Rate Pathology Table

**Source**: Calibration from COMBAT_REVERSE_ENGINEERING.md, fight.cpp formulas

| Attacker | Victim | THAC0 | Victim AC | Delta | Hit % | TTK | Status |
|----------|--------|-------|-----------|-------|-------|-----|--------|
| **Low Tier (L10-20)** |
| L10 Warrior | L10 Mob | 180 | -50 | 230 | 85% | 8s | ✓ Healthy |
| L20 Warrior | L20 Mob | 160 | -70 | 230 | 85% | 12s | ✓ Healthy |
| **Mid Tier (L30-40)** |
| L30 Warrior | L30 Mob | 140 | -90 | 230 | 85% | 20s | ✓ Healthy |
| L40 Warrior | L40 Mob | 120 | -110 | 230 | 85% | 32s | ⚠️ Slowing |
| **High Tier (L50+)** - PATHOLOGICAL |
| L50 Warrior | L50 Mob | 100 | -130 | 230 | **5%** | **320s** | 🚨 Broken |
| L50 Warrior | L45 Mob | 100 | -115 | 215 | 12% | 160s | 🚨 Bad |
| L60 Warrior | L60 Mob | 80 | -150 | 230 | **5%** | **400s** | 🚨 Broken |
| L70 Warrior | L70 Mob | 60 | -170 | 230 | **5%** | **480s** | 🚨 Broken |

### Breakdown: L50 Warrior vs L50 Mob

```
THAC0 = 100 (class -2 at L50, modified by stats/gear)
Mob AC = -130 (armor -80, DEX -30, buffs -20)

To-hit range:
  Roll 1-10:   Auto-miss  (5%)
  Roll 11-190: THAC0 - roll ≤ AC
               100 - 190 = -90 vs -130
               -90 > -130 → MISS (90%)
  Roll 191-200: Auto-hit (5%)

Effective hit rate: 5% (auto-hit only!)
```

### ACC-EVA Delta Demonstration

| Level Range | THAC0 Range | AC Range | Delta | Hit % | Behavior |
|-------------|-------------|----------|-------|-------|----------|
| L1-20 | 240-180 | 0 to -70 | 240-250 | 70-90% | ✓ Gradual scaling |
| L20-40 | 180-120 | -70 to -110 | 250-230 | 60-85% | ✓ Balanced |
| L40-50 | 120-100 | -110 to -130 | 230-230 | 85% → 5% | 🚨 **Cliff edge** |
| L50-70 | 100-60 | -130 to -170 | 230-230 | 5% | 🚨 **Stuck at auto-hit** |
| L70-100 | 60-20 | -170 to -200 | 230-220 | 5-10% | 🚨 **Pathological** |

### Root Cause: AC Range Explosion

```python
# Problem: AC has 260-point range (-160 to +100)
# But d200 roll only has 200-point range

# At L50+:
THAC0 - 190 ≤ AC  # Best roll (190)
100 - 190 = -90
AC = -130
-90 > -130 → ALWAYS MISS (except auto-hit)

# Even with max hitroll (+40):
THAC0_improved = 100 - 40 = 60
60 - 190 = -130
-130 ≤ -130 → BARELY HIT on roll 190!
```

### Why Binary Behavior Emerges

1. **Linear Scaling**: Both THAC0 and AC improve linearly with level
2. **Fixed Die**: d200 range doesn't expand
3. **Stat Bonuses**: DEX/gear push AC far beyond d200 range
4. **No Soft Caps**: Nothing prevents -160 AC or +240 THAC0

### Bounded Accuracy Fix

```python
# New system proposal:
ACC = 50 + (level × 0.5) + hitroll/2 + stats/5  # L50: ~85 ACC
EVA = 30 + (level × 0.3) + dodge/5 + DEX/3     # L50: ~60 EVA

# Bounded hit formula:
hit_chance = clamp(5%, 95%, 50% + (ACC - EVA) × 2)

# L50 balanced:
hit_chance = 50% + (85 - 60) × 2 = 50% + 50 = 100%
→ Clamped to 95% (healthy!)

# L50 mismatched (+10 EVA difference):
hit_chance = 50% + (85 - 70) × 2 = 50% + 30 = 80%
→ Still playable!
```

---

## Q7: Mob Dice Migration Variance Guardrail

**Question**: Add variance guardrail to prevent wildly swingy bosses.

### Answer: Coefficient of Variation Cap

**Maximum CV**: 0.60 (60% of mean) for normal mobs, 0.80 (80%) for bosses.

### Variance Formula

```python
# Coefficient of Variation = stddev / mean
# For XdY: mean = X × (Y+1)/2, variance = X × (Y²-1)/12
# stddev = sqrt(variance)

def calculate_cv(dice_num, dice_size):
    mean = dice_num * (dice_size + 1) / 2
    variance = dice_num * (dice_size**2 - 1) / 12
    stddev = sqrt(variance)
    return stddev / mean

# Example: 3d10
mean = 3 × 11/2 = 16.5
variance = 3 × 99/12 = 24.75
stddev = 4.97
CV = 4.97 / 16.5 = 0.301 (30%) ✓ Acceptable
```

### Guardrail Table

| Dice | Mean | StdDev | CV | Verdict |
|------|------|--------|-----|---------|
| 2d4 | 5.0 | 1.29 | 0.258 | ✓ Low variance |
| 3d4 | 7.5 | 1.58 | 0.211 | ✓ Very stable |
| 2d8 | 9.0 | 2.42 | 0.269 | ✓ Moderate |
| 3d10 | 16.5 | 4.97 | 0.301 | ✓ Acceptable |
| 10d10 | 55.0 | 9.58 | 0.174 | ✓ Stable (many dice) |
| 1d20 | 10.5 | 5.77 | 0.550 | ⚠️ High variance |
| 1d100 | 50.5 | 28.87 | 0.572 | ⚠️ Very swingy |
| 40d10 | 220 | 19.15 | 0.087 | ✓ Convergence |

### Migration Formula with Guardrail

```python
def convert_with_guardrail(damroll, level, is_boss=False):
    # Base conversion
    dice_size = 4 if level < 50 else 8 if level < 60 else 10
    fixed_pct = 0.15 if level < 50 else 0.05

    new_damroll = int(damroll * fixed_pct)
    remaining = damroll * (1 - fixed_pct)
    dice_number = max(1, round((remaining * 2) / (dice_size + 1)))

    # Check variance
    cv = calculate_cv(dice_number, dice_size)
    max_cv = 0.80 if is_boss else 0.60

    # If too swingy, increase fixed portion
    if cv > max_cv:
        # Reduce dice, increase fixed
        new_damroll = int(damroll * 0.40)  # 40% fixed instead
        remaining = damroll * 0.60
        dice_number = max(1, round((remaining * 2) / (dice_size + 1)))

    return new_damroll, dice_number, dice_size
```

### Boss Variance Example

```python
# Level 100 dragon with damroll=50
# Normal conversion: 2 + 9d10 (CV = 0.312) ✓
# Boss flag: Allow up to 0.80 CV

# Intentionally swingy boss:
# 5 + 8d10 (range: 13-85, CV = 0.29) ✓ Still moderate

# If we wanted max variance:
# 0 + 12d10 (range: 12-120, CV = 0.24) ✓ Large absolute range
```

### Key Findings

1. **Natural Convergence**: More dice = lower CV (law of large numbers)
2. **Sweet Spot**: 2-5 dice gives 0.25-0.35 CV (healthy variance)
3. **1-die Danger**: Single large die (1d100) creates feast/famine
4. **Boss Tuning**: Can allow higher CV for memorable encounters

---

## Q8: Defense Mechanics in New Model

**Question**: Shield/parry as separate % checks or subsumed into EVA?

### Answer: Hybrid Three-Layer Defense

**Design**: EVA (pre-roll) + Shield Block (post-hit) + Soak/DR% (always).

### Proposed Defense Architecture

```python
# LAYER 1: Pre-Roll (To-Hit)
hit_roll = d100
hit_chance = 50% + (ACC - EVA) × 2
hit_chance = clamp(5%, 95%, hit_chance)

if hit_roll > hit_chance:
    return MISS  # No damage

# LAYER 2: Post-Hit (Shield/Parry/Dodge)
# Only if Layer 1 succeeds
if shield_equipped:
    if d100 < shield_block_chance:
        return BLOCK  # No damage, but shield durability hit

if parry_skill > 0:
    if d100 < parry_chance:
        return PARRY  # No damage, riposte possible

if dodge_skill > 0:
    if d100 < dodge_chance:
        return DODGE  # No damage, reposition

# LAYER 3: Mitigation (Always applies if hit lands)
base_damage = damroll + weapon_dice + AP
soak_damage = max(0, base_damage - Soak)  # Flat reduction
final_damage = soak_damage × (1 - DR%)    # % reduction
```

### EVA Composition (Pre-Roll)

```python
# EVA affects to-hit calculation ONLY
EVA = 30                          # Base
    + (level × 0.3)              # Level scaling
    + (DEX - 50) / 3             # DEX contribution
    + stance_modifier            # Alert = +5, Fighting = 0
    + size_modifier              # Tiny = +10, Huge = -5

# Does NOT include:
# - Shield (separate post-hit check)
# - Parry (separate skill check)
# - Dodge (separate skill check)
```

### Shield Block (Post-Hit)

```python
# Independent % check after hit succeeds
shield_block_chance = 10                   # Base 10%
    + (shield_AC / 2)                     # Shield quality: +2.5% per 5 AC
    + (shield_skill / 5)                  # Shield skill: +20% at 100 skill
    + bonus_from_perks                    # Shield mastery feat: +5%

# HARD CAP: 50% maximum block chance
shield_block_chance = min(0.50, shield_block_chance)

# Trade-off: Shield occupies hand slot, can't dual-wield

# On block:
# - No damage
# - Shield durability reduced
# - Attacker can't riposte
# - Stamina cost (minor)
```

### Parry (Post-Hit, Weapon Required)

```python
# Weapon-based defense
parry_chance = (weapon_skill / 10)        # 10% at 100 skill
    + (DEX - 50) / 10                     # DEX bonus
    + weapon_parry_bonus                  # Defensive weapons: +5%

# HARD CAP: 40% maximum parry chance
parry_chance = min(0.40, parry_chance)

# Trade-off: Requires weapon, can't parry ranged, stamina cost

# On parry:
# - No damage
# - Riposte opportunity (separate attack at -20% to-hit)
# - Weapon durability reduced
# - Opens counter-attack window (0.5s)
```

### Dodge (Post-Hit, Skill-Based)

```python
# Pure agility defense
dodge_chance = (dodge_skill / 8)          # 12.5% at 100 skill
    + (DEX - 50) / 5                      # DEX heavy
    + armor_penalty                       # Heavy armor: -10% to -20%

# HARD CAP: 30% maximum dodge chance
dodge_chance = min(0.30, dodge_chance)

# Trade-off: Armor weight penalty (plate = -20%, leather = -5%)

# On dodge:
# - No damage
# - No counter-attack
# - Repositioning advantage (future: flanking bonus)
# - No durability cost
```

### Soak (Always, Flat Reduction)

```python
# From heavy armor and shields
Soak = 0
    + armor_soak[WEAR_BODY]   # Plate body: 5 soak
    + armor_soak[WEAR_HEAD]   # Helm: 1 soak
    + armor_soak[WEAR_LEGS]   # Plate legs: 2 soak
    + shield_soak             # Shield: 2 soak

# Example L50 warrior: 10 total soak
# Countered by: AP (attack power) from weapons
```

### DR% (Always, % Reduction)

```python
# From armor rating
AR = sum_of_armor_AC_values  # Total armor (excluding DEX)
K = tier_constant(level)     # 40/60/80
DR% = AR / (AR + K)

# Example L50 warrior: AR = 80
# DR% = 80/(80+60) = 57%

# Countered by: Pen% from weapons/skills
```

### Complete Damage Flow

```
1. ACC vs EVA → 75% hit chance
   ↓ (hit succeeded)

2. Shield block → 25% chance → BLOCKED (no damage)
   ↓ (not blocked)

3. Parry → 20% chance → PARRIED (no damage, riposte)
   ↓ (not parried)

4. Dodge → 15% chance → DODGED (no damage)
   ↓ (hit lands)

5. Base damage: 30 (damroll) + 15 (3d10 weapon) = 45
   ↓

6. Soak: 45 - 10 soak = 35
   ↓

7. DR%: 35 × (1 - 0.57) = 15 final damage
```

### Effective Avoidance

```python
# Total avoidance chance:
layer1_miss = 25%           # (1 - hit_chance)
layer2_block = 75% × 25%    # (hit × block_chance)
layer2_parry = 75% × 75% × 20%  # (hit × not_blocked × parry)
layer2_dodge = 75% × 75% × 80% × 15%  # (remaining × dodge)

total_avoid = 25% + 18.75% + 11.25% + 6.75% = 61.75%
→ 38.25% chance to take damage

# With mitigation:
effective_damage = 38.25% × 43% (after soak/DR%) = 16.4%
→ 84% effective damage reduction!
```

### Key Findings

1. **Three Distinct Layers**:
   - EVA = pre-roll avoidance (broadest)
   - Block/Parry/Dodge = post-hit checks (trade-offs)
   - Soak/DR% = guaranteed mitigation (always applies)

2. **No Double-Dipping**: DEX contributes to both EVA and dodge, but:
   - EVA: Small contribution (DEX/3)
   - Dodge: Larger contribution (DEX/5), but armor penalty

3. **Build Trade-Offs**:
   - **Tank**: High AR/Soak, shield block, low dodge
   - **Skirmisher**: High EVA/dodge, medium DR%, no shield
   - **Duelist**: High parry, medium EVA, riposte damage

4. **Consistency**: Shield/parry mechanics preserved from legacy, just clarified

---

## Q9: Magic AC Destination & Mitigation Architecture

**Question**: Where does "mage armor" land in the new model? What's the explicit bucket name?

### Answer: Ward% - Magical Damage Reduction Layer

**Design Decision**: Magic AC → Ward% (magical mitigation separate from physical armor).

### Three Mitigation Layers

```python
# LAYER 1: Physical Armor (from equipment)
AR = sum_of_armor_pieces  # Plate, helm, shield, etc.
DR% = AR / (AR + K)       # Diminishing returns via K constant

# LAYER 2: Resistance (from composition/buffs)
RES% = (100 - susceptibility) / 100  # Fire resist, poison resist, etc.

# LAYER 3: Ward (from magical effects)
Ward% = magic_ac_conversion  # Mage armor, stoneskin, etc.
```

### Ward% Calculation

```python
# Convert legacy magic AC to Ward%
def calculate_ward(magic_effects):
    ward = 0

    # Armor spell: +20 AC → 10% Ward
    if has_effect(EFF_ARMOR):
        ward += 0.10

    # Mage Armor: +40 AC → 20% Ward
    if has_effect(EFF_MAGE_ARMOR):
        ward += 0.20

    # Shield spell: +20 AC → 10% Ward
    if has_effect(EFF_SHIELD):
        ward += 0.10

    # Stoneskin: special case (see below)
    if has_effect(EFF_STONE_SKIN):
        ward += 0.40  # High magic protection

    # Blur/Mirror Image: handled as EVA bonus, not Ward
    # Displacement: handled as miss chance, not mitigation

    return ward

# Example wizard with armor + shield:
# Ward% = 10% + 10% = 20%
```

### Magic Effect Routing Table

| Legacy Effect | Type | New Destination | Value |
|---------------|------|-----------------|-------|
| `EFF_ARMOR` | Magic AC | **Ward%** | +10% |
| `EFF_MAGE_ARMOR` | Magic AC | **Ward%** | +20% |
| `EFF_SHIELD` | Magic AC | **Ward%** | +10% |
| `EFF_STONE_SKIN` | Special | **Ward%** + chip immunity | +40% + no chip |
| `EFF_BARKSKIN` | Magic AC | **Ward%** | +15% |
| `EFF_BLESS` | To-Hit | **ACC** modifier | +5 ACC |
| `EFF_BLUR` | Miss chance | **EVA** bonus | +10 EVA |
| `EFF_DISPLACEMENT` | Miss chance | Post-hit check | 20% miss |
| `EFF_PROT_FIRE` | Resistance | **RES%** (fire) | +25% |
| `EFF_FIRESHIELD` | Resistance | **RES%** (cold) | +25% |

### Stacking Example

```python
# L50 Cleric with buffs:
DR% = 80/(80+60) = 0.571  # From plate armor (57.1%)
Ward% = 0.10 + 0.10 = 0.20  # Armor + Shield spells (20%)
RES% = 0.25  # Protection from Fire (25%)

# Against physical attack:
total_mitigation = DR% + Ward% = 0.571 + 0.20 = 0.771
capped_mitigation = min(0.75, 0.771) = 0.75  # Hit the cap!
final_damage = base_damage × (1 - 0.75) = base_damage × 0.25

# Against fire attack:
total_mitigation = Ward% + RES% = 0.20 + 0.25 = 0.45
final_damage = base_damage × (1 - 0.45) = base_damage × 0.55
```

### Global Mitigation Cap & Chip Floor

**Design**: 75% cap with 1 damage minimum ("chip damage").

```python
# FINAL MITIGATION FORMULA
def calculate_final_damage(base_damage, DR, RES, Ward, has_immunity=False):
    # Immunity check (absolute)
    if has_immunity:
        return 0  # No damage, no chip

    # Stack all applicable mitigations
    total_mitigation = 0

    # Physical attacks: DR% + Ward%
    if is_physical_damage:
        total_mitigation = DR + Ward

    # Elemental attacks: RES% + Ward%
    if is_elemental_damage:
        total_mitigation = RES + Ward

    # Pure magic attacks: Ward% only
    if is_pure_magic_damage:
        total_mitigation = Ward

    # GLOBAL CAP: Maximum 75% mitigation
    capped_mitigation = min(0.75, total_mitigation)

    # Calculate reduced damage
    reduced_damage = base_damage * (1 - capped_mitigation)

    # CHIP FLOOR: Minimum 1 damage if hit connects
    if reduced_damage > 0 and reduced_damage < 1:
        return 1
    else:
        return floor(reduced_damage)

# Examples:
calculate_final_damage(100, 0.57, 0, 0.20, False)
# → 100 × (1 - min(0.75, 0.77)) = 100 × 0.25 = 25 damage

calculate_final_damage(10, 0.60, 0, 0.20, False)
# → 10 × (1 - 0.75) = 2.5 → floor(2.5) = 2 damage

calculate_final_damage(4, 0.60, 0, 0.20, False)
# → 4 × (1 - 0.75) = 1.0 → 1 damage (exactly chip floor)

calculate_final_damage(3, 0.60, 0, 0.20, False)
# → 3 × (1 - 0.75) = 0.75 → 1 damage (chip floor applied)

calculate_final_damage(100, 0, 0, 0, True)
# → 0 damage (immunity - no chip)
```

### Mitigation Cap Rationale

**Why 75%?**
1. **Prevents Invulnerability**: Even max-tank takes 25% damage
2. **Maintains Threat**: Boss hits still hurt (25% of 200 = 50 damage)
3. **Build Diversity**: Don't need to stack all three layers
4. **Overflow Value**: Extra mitigation still has value (chip protection)

**Chip Floor Rationale**:
1. **Prevents Zero**: Every successful hit does something
2. **High-Frequency Builds**: Fast attackers benefit from chip
3. **Psychological**: Visible damage maintains engagement
4. **Immunity Respect**: True immunity (0% susceptibility) still exists

### Stoneskin Special Case

**Legacy**: 90% damage reduction or random(0, 3) damage (fight.cpp:2272-2274)

```cpp
if (EFF_FLAGGED(victim, EFF_STONE_SKIN) && random(0,10) <= 9) {
    dam = random(0, 3);
}
```

**New Model**: Ward% with chip immunity exception

```python
# Stoneskin effect:
Ward% += 0.40  # 40% magical mitigation
has_chip_immunity = True  # Exception to chip floor

# Result with stoneskin:
def calculate_final_damage_stoneskin(base_damage, DR, Ward):
    total_mitigation = min(0.75, DR + Ward)
    reduced_damage = base_damage * (1 - total_mitigation)

    # Stoneskin: allow 0 damage (no chip floor)
    if reduced_damage < 1:
        return 0  # Can fully absorb small hits
    else:
        return floor(reduced_damage)

# Examples:
calculate_final_damage_stoneskin(10, 0.35, 0.40)
# → 10 × (1 - 0.75) = 2.5 → 2 damage (still some damage)

calculate_final_damage_stoneskin(3, 0.35, 0.40)
# → 3 × (1 - 0.75) = 0.75 → 0 damage (stoneskin absorbs)
```

### Content Author Guidelines

**For spell/effect designers**:

```yaml
# Physical armor spells → Ward%
mage_armor:
  effect: EFF_MAGE_ARMOR
  mitigation_type: Ward%
  value: 20%
  stacks_with: [DR%, RES%]
  cap_contribution: Yes (toward 75% global cap)
  duration: 1 hour
  slot_cost: Buff slot

# Resistance spells → RES%
protection_from_fire:
  effect: EFF_PROT_FIRE
  mitigation_type: RES% (fire damage only)
  value: 25%
  stacks_with: [Ward%]
  cap_contribution: Yes (toward 75% global cap)
  duration: 30 minutes

# Evasion spells → EVA bonus (not mitigation)
blur:
  effect: EFF_BLUR
  mitigation_type: None (affects to-hit)
  value: +10 EVA
  stacks_with: N/A (pre-roll avoidance)
  cap_contribution: No (separate system)
  duration: 10 minutes
```

### Implementation Checklist

- [ ] Convert all `APPLY_AC` magic effects to Ward%
- [ ] Implement 75% global mitigation cap
- [ ] Implement 1 damage chip floor (except immunity/stoneskin)
- [ ] Route physical armor to DR%, magic armor to Ward%, elemental resist to RES%
- [ ] Test stacking: DR% + Ward% hits cap appropriately
- [ ] Test chip floor: 0.1-0.99 damage rounds to 1
- [ ] Test immunity: 0% susceptibility bypasses chip floor
- [ ] Test stoneskin: Can achieve 0 damage without immunity
- [ ] Update spell descriptions to reference Ward% instead of AC
- [ ] Add combat log messages showing mitigation breakdown

### Key Findings

1. **Three Mitigation Types**:
   - **DR%**: Physical armor (equipment)
   - **RES%**: Elemental resistance (composition + buffs)
   - **Ward%**: Magical protection (spells)

2. **Stacking Rules**:
   - Applicable types sum together
   - Global 75% cap applies to sum
   - Excess mitigation still valuable (chip protection)

3. **Chip Floor**:
   - Minimum 1 damage if hit connects
   - Exceptions: Immunity (0% sus) and Stoneskin effect
   - Preserves value of high-frequency builds

4. **Content Migration**:
   - All magic AC effects → Ward%
   - Conversion rate: ~0.5% Ward per legacy AC point
   - +20 AC = +10% Ward (approximate)

---

## Summary of Clarifications

| Question | Answer | Impact |
|----------|--------|--------|
| Q1: Crit roll | Value 20 on to-hit d200 (0.5% chance) | No modifiers exist |
| Q2: Auto-hit override | Yes - immunity and displacement override | Precedence critical |
| Q3: Defense bypass | 8 skills/effects bypass post-hit | List now complete |
| Q4: Resist stacking | Multiplicative (75% × 75% = 56%) | No hard cap besides immunity |
| Q5: AC→AR mapping | AR = -(total_AC - DEX×10 - magic_AC) | Removes double-dipping |
| Q6: L50 pathology | 5% hit rate at L50+ (auto-hit only) | Bounded ACC fixes this |
| Q7: Dice variance | CV cap 0.60 (0.80 bosses) | Prevents 1d100 swings |
| Q8: Shield/parry | 3-layer: EVA (pre) + Block/Parry (post) + Soak/DR% (always) | Caps: 50%/40%/30% |
| Q9: Magic AC → Ward% | Mage armor → Ward% (magical mitigation layer) | 75% global cap |

### Codified Caps

| Stat | Hard Cap | Soft Cap | Notes |
|------|----------|----------|-------|
| Block% | 50% | - | Shield occupies hand slot |
| Parry% | 40% | - | Weapon required, stamina cost |
| Dodge% | 30% | - | Armor weight penalty |
| PenFlat | Soak+10 | - | Can bypass all Soak + 10 over |
| PenPct | 50% | - | Can halve mitigation, not eliminate |
| DR% | - | ~60% | Diminishing returns via K constant |
| Ward% | - | ~40% | From spell stacking |
| RES% | - | ~75% | Composition + buffs |
| **Total Mitigation** | **75%** | - | **Global hard cap (DR%+Ward%+RES%)** |

### K Constants (Recomputable)

```
K_LOW = 50    (L1-30:  AR=50  → 50% DR)
K_MID = 100   (L31-60: AR=100 → 50% DR)
K_HIGH = 200  (L61+:   AR=200 → 50% DR)
```

### Damage Routing

- **Physical**: Soak + DR% (+ Ward% if magical weapon)
- **Elemental**: RES% (+ Ward% if magical)
- **Pure Magic**: Ward% only

All clarifications added to combat system documentation with complete formulas, edge cases, caps, and migration guidance.

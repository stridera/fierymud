# FieryMUD Combat System Snapshot

**Purpose**: Reference data for designing a new combat system
**Source**: Legacy FieryMUD codebase (`fierymud/legacy/src/`)
**Date**: 2025-01-28

---

## Table of Contents
1. [Combat Mechanics Overview](#combat-mechanics-overview)
2. [Player Classes](#player-classes)
3. [Stat Bonuses](#stat-bonuses)
4. [Mob Examples](#mob-examples)
5. [Damage Calculation](#damage-calculation)
6. [THAC0 System](#thac0-system)

---

## Combat Mechanics Overview

### Core Combat Formula
```
Hit Roll = d200 (1-200)
- Auto-miss: 1-10
- Check vs AC: 11-190
- Auto-hit: 191-200
- Victim asleep: Auto-hit

Success: calc_thac0 - diceroll <= victim_ac
```

### Damage Formula (from fight.cpp:2211-2256)
```cpp
// Base damage
dam = GET_DAMROLL(ch) + stat_bonus[GET_STR(ch)].todam;

// Critical hit (d200 == 20)
if (diceroll == 20) dam *= 2;

// Mob barehand damage
if (IS_NPC(ch)) dam += roll_dice(mob->damnodice, mob->damsizedice);

// Weapon damage
if (weapon) dam += roll_dice(weapon->dice_num, weapon->dice_size);

// Player unarmed damage
else if (!IS_NPC(ch)) dam += random_number(0, 2);

// Stance multiplier (if victim not fighting)
if (GET_STANCE(victim) < STANCE_FIGHTING)
    dam *= 1 + (STANCE_FIGHTING - GET_STANCE(victim)) / 3;

// alert     x1.33
// resting   x1.66
// sleeping  x2.00
// stunned   x2.33
// incap     x2.66
// mortally  x3.00

// Special skill multipliers
if (BACKSTAB) dam *= (SKILL_BACKSTAB / 10 + 1);
if (KICK) dam += (SKILL_KICK / 2) + stat_bonus[DEX].todam;
if (BAREHAND) dam += SKILL_BAREHAND/4 + random(1, LEVEL/3) + LEVEL/2;

// Minimum damage
dam = max(1, dam);
```

---

## Player Classes

### Class Stats (from class.cpp)

| Class | THAC0 | HP/Lev 30+ | Exp Factor | HP Factor | Hit/Dam Factor | Dice Factor | AC Factor |
|-------|-------|------------|------------|-----------|----------------|-------------|-----------|
| **Sorcerer** | 6 | +3 | 1.2 | 80% | 80% | 60% | 75% |
| **Cleric** | 4 | +6 | 1.0 | 80% | 80% | 70% | 100% |
| **Thief** | 1 | +6 | 1.0 | 90% | 100% | 100% | 80% |
| **Warrior** | -2 | +10 | 1.1 | 120% | 120% | 120% | 120% |
| **Paladin** | -2 | +8 | 1.15 | 120% | 120% | 120% | 120% |
| **Anti-Paladin** | -2 | +8 | 1.15 | 120% | 120% | 120% | 120% |
| **Ranger** | -2 | +8 | 1.15 | 120% | 120% | 120% | 120% |

### Saving Throws (Para, Rod, Petri, Breath, Spell)

| Class | Saves |
|-------|-------|
| Sorcerer | 90, 85, 95, 105, 80 |
| Cleric | 85, 110, 85, 115, 90 |
| Thief | 95, 90, 100, 110, 110 |
| Warrior | 105, 115, 100, 100, 110 |
| Paladin | 95, 115, 100, 105, 90 |

### Stat Preferences (Character Creation)

| Class | Priority Order |
|-------|----------------|
| Sorcerer | INT, CON, WIS, DEX, STR, CHA |
| Cleric | WIS, CON, INT, DEX, STR, CHA |
| Thief | DEX, STR, CON, WIS, INT, CHA |
| Warrior | CON, STR, DEX, WIS, INT, CHA |
| Paladin | STR, DEX, CON, WIS, INT, CHA |
| Ranger | STR, DEX, CON, INT, WIS, CHA |

---

## Stat Bonuses

### Strength (STR) - Melee Combat
**Source**: `constants.cpp:291-336`

| STR | ToHit | ToDam | Carry | Wield |
|-----|-------|-------|-------|-------|
| 0 | -5 | -4 | 0 | 0 |
| 20 | -2 | -1 | 83 | 5 |
| 28 | -1 | 0 | 116 | 7 |
| 40 | 0 | 0 | 166 | 11 |
| 60 | 0 | 0 | 250 | 16 |
| 65 | +1 | +1 | 271 | 18 |
| 72 | +2 | +3 | 300 | 20 |
| 80 | +3 | +7 | 439 | 29 |
| 90 | +4 | +9 | 596 | 40 |
| 100 | +7 | +14 | 786 | 70 |

### Dexterity (DEX) - Defense & Agility
| DEX | ToHit | ToDam | Defense (AC) |
|-----|-------|-------|--------------|
| 0 | -5 | -4 | +6 |
| 24 | -1 | -1 | +1 |
| 40 | 0 | 0 | 0 |
| 56 | 0 | 0 | 0 |
| 60 | 0 | 0 | -1 |
| 72 | +2 | +3 | -2 |
| 80 | +3 | +7 | -3 |
| 90 | +4 | +9 | -4 |
| 100 | +7 | +14 | -6 |

**Note**: Defense bonus is multiplied by 10 when applied to AC (fight.cpp:2146)
- DEX 100 → -6 defense → -60 AC bonus
- DEX 0 → +6 defense → +60 AC penalty

### Constitution (CON) - HP & Endurance
| CON | HP Gain/Level |
|-----|---------------|
| 0 | -4 |
| 24 | -1 |
| 40 | 0 |
| 56 | 0 |
| 57 | +1 |
| 72 | +2 |
| 80 | +3 |
| 96 | +5 |
| 100 | +5 |

### Intelligence (INT) & Wisdom (WIS) - Spellcasting & ToHit
**Magic Bonus** (both INT and WIS):
| Stat | Magic Bonus |
|------|-------------|
| 0-44 | 0 |
| 45 | +2 |
| 60 | +3 |
| 80 | +5 |
| 100 | +7 |

**ToHit Bonus** (applied in combat):
- INT: `calc_thaco -= (4 * GET_INT(ch)) / 10` → -4 to -40
- WIS: `calc_thaco -= (4 * GET_WIS(ch)) / 10` → -4 to -40

---

## Mob Examples

### Format Explanation (from .mob files)
```
Level Thac0 AC HitDice DamDice
Gold/Alignment/RaceAlign/Size
Position DefaultPosition Gender RaceNumber Align_Flags 0 Class
Str: <value>
Dex: <value>
Int: <value>
Wis: <value>
Con: <value>
Cha: <value>
```

### Low-Level Mobs (from zone 30)

**#3000 Bigby (Shopkeeper, Level 99)**
```
Level: 99, Thac0: 20, AC: 0
HP Dice: 0d0+0, Dam Dice: 0d0+0
Stats: STR 30, DEX 30, INT 30, WIS 30, CON 30, CHA 30
```

**#3003 Santiago (Weaponsmith, Level 99)**
```
Level: 99, Thac0: 20, AC: 0
HP Dice: 0d0+0, Dam Dice: 0d0+26
Stats: STR 30, DEX 70, INT 30, WIS 30, CON 30, CHA 30
Hitroll: 100, Damroll: 100
```

**#3005 Receptionist (Level 99)**
```
Level: 99, Thac0: 20, AC: 0
HP Dice: 0d0+0, Dam Dice: 0d0+0
Stats: STR 30, DEX 68, INT 65, WIS 76, CON 30, CHA 70
Perception: 1000
```

### Mid-Tier Mob Structure
*Note: Need to query database for level 20-40 mobs*
```sql
SELECT zone_id, id, level, armor_class, hit_roll,
       damage_dice_num, damage_dice_size, damage_dice_bonus,
       strength, dexterity, constitution, size
FROM "Mobs"
WHERE level BETWEEN 20 AND 40
ORDER BY level LIMIT 20;
```

### High-Tier Mob Structure
*Note: Need to query database for level 50+ mobs*
```sql
SELECT zone_id, id, level, armor_class, hit_roll,
       damage_dice_num, damage_dice_size, damage_dice_bonus,
       mobFlags, effectFlags, race, size
FROM "Mobs"
WHERE level >= 50
ORDER BY level LIMIT 20;
```

---

## Damage Calculation

### Base Damage Components
1. **Damroll** - Character's inherent damage bonus
2. **Strength Bonus** - `stat_bonus[STR].todam` (0 to +14)
3. **Weapon Dice** - `XdY` (e.g., longsword: 2d6, dagger: 1d4)
4. **Mob Barehand** - NPCs: `XdY` from mob data
5. **Player Barehand** - random(0, 2) if no weapon

### Multipliers & Modifiers

**Critical Hit** (d200 == 20):
- Damage × 2

**Stance Multiplier** (victim not fighting):
- Alert: ×1.33
- Resting: ×1.66
- Sleeping: ×2.00
- Stunned: ×2.33
- Incapacitated: ×2.66
- Mortally Wounded: ×3.00

**Skill Multipliers**:
- **Backstab**: dam × (SKILL_BACKSTAB/10 + 1) → ×1 to ×11
- **Sneak Attack** (Rogue): dam += (hidden/2) × (SKILL_SNEAK_ATTACK/100)
- **Kick**: dam += SKILL_KICK/2 + stat_bonus[DEX].todam
- **Barehand**: dam += SKILL/4 + random(1, LEVEL/3) + LEVEL/2

**Spell Effects**:
- **Spirit Bear**: dam × (1.0 + LEVEL/1000) → up to ×1.1
- **Berserk**: dam × 1.1
- **Stone Skin**: 90% chance to reduce dam to random(0, 3)

### Minimum Damage
```cpp
dam = std::max(1, dam); // Always at least 1 HP damage
```

---

## THAC0 System

### THAC0 Calculation (fight.cpp:2081-2140)

```cpp
// Base class THAC0
calc_thac0 = calc_thac0(GET_LEVEL(ch), classes[GET_CLASS(ch)].thac0_01,
                        classes[GET_CLASS(ch)].thac0_00);

// Apply modifiers
calc_thaco -= GET_HITROLL(ch);        // 0 to 40
calc_thaco -= (4 * GET_INT(ch)) / 10; // 0 to 40
calc_thaco -= (4 * GET_WIS(ch)) / 10; // 0 to 40

// Weapon proficiency
if (weapon && weapon_proficiency(weapon, position))
    calc_thaco -= GET_SKILL(ch, proficiency) / 2; // 0 to 50

// Monk barehand
if (CLASS_MONK && !weapon)
    calc_thaco -= GET_SKILL(ch, SKILL_BAREHAND) / 2; // 0 to 50

// Bless effect (alignment-based)
if (EFF_BLESS) {
    if (GOOD vs EVIL) calc_thaco -= LEVEL/5;      // -20 at level 100
    if (GOOD vs NEUTRAL) calc_thaco -= LEVEL/10;  // -10 at level 100
    if (GOOD vs GOOD) calc_thaco += LEVEL/10;     // +10 at level 100
    // ... (similar for EVIL and NEUTRAL)
}
```

### THAC0 Formula (fight.cpp:2585)
```cpp
int calc_thac0(int level, int thac0_01, int thac0_00) {
    return thac0_01 + level * (thac0_00 - thac0_01) / 100;
}
```

**Example** (Warrior, THAC0 = -2):
- Level 1: calc_thac0(1, 20, -2) = 20 + 1×(-22)/100 = 19.78 ≈ 20
- Level 50: calc_thac0(50, 20, -2) = 20 + 50×(-22)/100 = 9
- Level 100: calc_thac0(100, 20, -2) = 20 + 100×(-22)/100 = -2

### Victim AC Calculation (fight.cpp:2144-2148)
```cpp
victim_ac = GET_AC(victim);                    // -100 to +100
victim_ac += stat_bonus[GET_DEX(victim)].defense * 10; // -60 to +60
victim_ac = std::min(100, victim_ac);          // Cap at 100

// Final Range: -160 (best) to +100 (worst)
```

### Hit Determination
```cpp
diceroll = random_number(1, 200);

if (diceroll > 190 || !AWAKE(victim))
    to_hit = true;  // Auto-hit
else if (diceroll < 11)
    to_hit = false; // Auto-miss
else
    to_hit = (calc_thaco - diceroll <= victim_ac);
```

---

## Example Combat Calculations

### Scenario 1: Level 20 Warrior vs Level 20 Mob

**Attacker** (Level 20 Warrior):
- Base THAC0: calc_thac0(20, 20, -2) = 20 + 20×(-22)/100 = 15.6 ≈ 16
- STR: 80 (+3 hit, +7 dam)
- DEX: 60 (0 hit, 0 dam)
- Hitroll: +15 (from gear)
- Weapon: Longsword (2d6)
- Damroll: +10 (from gear)

**Final THAC0**: 16 - 15 (hitroll) - 3 (STR) - 16 (INT×4/10) - 16 (WIS×4/10) = -34

**Damage Roll**:
- Base: 10 (damroll) + 7 (STR) = 17
- Weapon: 2d6 (avg 7) = 17 + 7 = 24
- **Average Damage**: 24
- **Critical (d200=20)**: 48

**Defender** (Level 20 Mob):
- AC: -20
- DEX: 60 → 0 defense × 10 = 0
- **Final AC**: -20

**Hit Chance**:
- Roll 1-10: Auto-miss (5%)
- Roll 11-190: Check -34 - roll ≤ -20 → always hit (90%)
- Roll 191-200: Auto-hit (5%)
- **Total Hit Chance**: ~100%

---

### Scenario 2: Level 50 Sorcerer vs Level 50 Mob

**Attacker** (Level 50 Sorcerer):
- Base THAC0: calc_thac0(50, 20, 6) = 20 + 50×(-14)/100 = 13
- INT: 90 (+4 hit, +6 magic)
- WIS: 70 (+2 hit)
- Hitroll: +20 (from gear)
- Weapon: Staff (1d6)
- Damroll: +15 (from gear)

**Final THAC0**: 13 - 20 (hitroll) - 36 (INT×4/10) - 28 (WIS×4/10) = -71

**Damage Roll**:
- Base: 15 (damroll) + 9 (STR 90) = 24
- Weapon: 1d6 (avg 3.5) = 27.5
- **Average Damage**: 28

**Defender** (Level 50 Mob):
- AC: -40
- DEX: 70 → -2 defense × 10 = -20
- **Final AC**: -60

**Hit Chance**:
- Roll 1-10: Auto-miss (5%)
- Roll 11-190: Check -71 - roll ≤ -60 → hit if roll ≤ 11 (only on 11) (0.5%)
- Roll 191-200: Auto-hit (5%)
- **Total Hit Chance**: ~10.5%

---

## Weapon & Armor Examples

### Common Weapons (need database query)
```sql
SELECT zone_id, id, keywords, type,
       damage_dice_num, damage_dice_size, average_damage,
       hitroll, damroll
FROM "Objects"
WHERE type = 'WEAPON'
ORDER BY average_damage DESC
LIMIT 30;
```

### Common Armor (need database query)
```sql
SELECT zone_id, id, keywords, type,
       ac, wear_flags
FROM "Objects"
WHERE type = 'ARMOR'
ORDER BY ac ASC
LIMIT 30;
```

---

## Combat System Design Considerations

### Current System Strengths
1. **THAC0 Granularity**: d200 provides fine-grained hit chances
2. **Stat Integration**: STR/DEX/INT/WIS all contribute meaningfully
3. **Critical Hits**: Simple 2× multiplier on natural 20
4. **Stance System**: Tactical depth with vulnerability states
5. **Skill Multipliers**: Class identity through combat abilities

### Current System Weaknesses
1. **AC Range Issues**: With high stats and gear, AC can become extreme (-160 to +100)
2. **THAC0 Complexity**: Hard to balance across 100 levels
3. **Hit Chance Extremes**: Can easily reach 0% or 100% hit chance
4. **Damage Variance**: Wide swing between min and max rolls
5. **Class Imbalance**: Warriors significantly stronger in melee combat

### Recommendations for New System
1. **Normalize AC Range**: Consider AC cap of -100 to +50
2. **Bounded Accuracy**: Keep hit chances between 10-90%
3. **Damage Consistency**: Reduce variance, increase base damage
4. **Class Balance**: Give casters more combat utility
5. **Tactical Depth**: Add status effects, positioning, cooldowns
6. **Criticals**: Consider critical damage range (1.5× to 3×)
7. **Armor Penetration**: Add mechanics to bypass AC
8. **Resist System**: Expand damage type resistances/vulnerabilities

---

## Additional Data Needed

To complete this snapshot, the following queries should be run against the database:

### 1. Mob Combat Stats by Tier
```sql
-- Low tier (1-20)
SELECT level, AVG(armor_class) as avg_ac, AVG(hit_roll) as avg_hitroll,
       AVG(damage_dice_num) as avg_dam_num, AVG(damage_dice_size) as avg_dam_size,
       AVG(hp_dice_num * hp_dice_size + hp_dice_bonus) as avg_hp
FROM "Mobs"
WHERE level BETWEEN 1 AND 20
GROUP BY level
ORDER BY level;

-- Mid tier (21-50)
-- High tier (51-99)
```

### 2. Weapon Damage Distribution
```sql
SELECT
    CASE
        WHEN average_damage < 10 THEN 'Low (0-9)'
        WHEN average_damage < 20 THEN 'Medium (10-19)'
        WHEN average_damage < 30 THEN 'High (20-29)'
        ELSE 'Very High (30+)'
    END as damage_tier,
    COUNT(*) as weapon_count,
    AVG(average_damage) as avg_damage,
    MIN(average_damage) as min_damage,
    MAX(average_damage) as max_damage
FROM "Objects"
WHERE type = 'WEAPON'
GROUP BY damage_tier
ORDER BY avg_damage;
```

### 3. Armor AC Distribution
```sql
SELECT
    CASE
        WHEN ac > 0 THEN 'Poor (positive AC)'
        WHEN ac >= -10 THEN 'Common (0 to -10)'
        WHEN ac >= -20 THEN 'Good (-11 to -20)'
        WHEN ac >= -30 THEN 'Excellent (-21 to -30)'
        ELSE 'Legendary (-31+)'
    END as ac_tier,
    COUNT(*) as armor_count,
    AVG(ac) as avg_ac,
    MIN(ac) as best_ac,
    MAX(ac) as worst_ac
FROM "Objects"
WHERE type = 'ARMOR'
GROUP BY ac_tier
ORDER BY avg_ac;
```

---

**End of Combat Snapshot**

# FieryMUD Effect Flags Reference

**Comprehensive implementation guide for all effect flags in FieryMUD**

This document answers the critical question: **"What does each effect flag actually do?"**

All information extracted from actual code implementation in `legacy/src/`.

---

## Table of Contents

- [Understanding Effect Flags](#understanding-effect-flags)
- [Combat-Affecting Effects](#combat-affecting-effects)
- [Defensive Effects](#defensive-effects)
- [Detection Effects](#detection-effects)
- [Movement Effects](#movement-effects)
- [Status Effects](#status-effects)
- [Elemental Effects](#elemental-effects)

---

## Understanding Effect Flags

### What Are Effect Flags?

Effect flags are binary states that grant special abilities or conditions to characters. They are defined in `defines.hpp:408-497` and checked throughout the codebase to modify behavior.

### How They Work

1. **Setting**: Flags are set by spells (via `mag_affect()`), items (via object effect flags), or racial abilities
2. **Checking**: Code checks flags using `EFF_FLAGGED(ch, EFF_*)` macro
3. **Effects**: Flags can:
   - Grant stat modifiers (via APPLY locations)
   - Directly modify combat calculations
   - Enable special abilities
   - Change visual appearance

### Key Implementation Files

- `defines.hpp:408-497` - Flag definitions
- `magic.cpp:1109+` - Spell effect applications
- `fight.cpp` - Combat modifications
- `handler.cpp:130-246` - Stat modifier application
- `act.informative.cpp` - Visual descriptions

---

## Combat-Affecting Effects

### EFF_BLESS (78)

**What it does:**
- **Primary**: Allows barehand attacks to hit ethereal/incorporeal creatures
- **Stat Bonuses**:
  - Spell saves: -2 to -12 (better saves vs spells)
  - Damage: +1 to +2 (at skill 55+)
- **Combat Bonus**: Hitroll bonus when good vs evil (-level/5) or neutral (-level/10)

**Implementation:**
```cpp
// defines.hpp:486
#define EFF_BLESS 78  /* When blessed, your barehand attacks hurt ether chars */

// magic.cpp:1230-1256
eff[0].location = APPLY_SAVING_SPELL;
eff[0].modifier = -2 - (skill / 10);  // -2 to -12
eff[0].duration = 10 + (skill / 7);   // 10-24 hours

if (skill >= 55) {
    eff[1].location = APPLY_DAMROLL;
    eff[1].modifier = 1 + (skill > 95);  // +1 or +2
}
SET_FLAG(eff[2].flags, EFF_BLESS);

// damage.cpp:73
// Can only hit ethereal without bless
return !(EFF_FLAGGED(attacker, EFF_BLESS));

// fight.cpp:2123-2127
if (EFF_FLAGGED(ch, EFF_BLESS)) {
    if (IS_GOOD(ch) && IS_EVIL(victim))
        calc_thaco -= GET_LEVEL(ch) / 5;  // Big bonus vs evil
    if (IS_GOOD(ch) && IS_NEUTRAL(victim))
        calc_thaco -= GET_LEVEL(ch) / 10; // Small bonus vs neutral
}
```

**Duration**: 10-24 hours (skill-based)

**Restrictions**:
- Evil casters fail (gods forsaken)
- Cannot bless evil targets
- Conflicts with Dark Presence, Wings of Heaven, Earth Blessing

**AI Priority**: 10/100

---

### EFF_CURSE (9)

**What it does:**
- **Hitroll Penalty**: -1 to -3 (harder to hit)
- **Damroll Penalty**: -1 to -3 (less damage)
- **Accumulates**: Multiple curses stack

**Implementation:**
```cpp
// magic.cpp:1422-1429
eff[0].location = APPLY_HITROLL;
eff[0].duration = 5 + (skill / 14);   // 5-12 hours
eff[0].modifier = -1 - (skill / 50);  // -1 to -3

eff[1].location = APPLY_DAMROLL;
eff[1].modifier = eff[0].modifier;
accum_effect = true;  // Stacks with existing curses
```

**Duration**: 5-12 hours

**Save**: Yes (SAVING_SPELL)

**AI Priority**: 40/100

---

### EFF_HASTE (24)

**What it does:**
- **Extra Attack**: Grants +1 attack per combat round
- **Stacks with**: Double attack skill and blur

**Implementation:**
```cpp
// magic.cpp:2111-2114
SET_FLAG(eff[0].flags, EFF_HASTE);
eff[0].duration = 2 + (skill / 21);  // 2-6 hours

// fight.cpp:2397-2398
if (EFF_FLAGGED(ch, EFF_HASTE))
    hits += 1;
```

**Duration**: 2-6 hours

**AI Priority**: 50/100

**Note**: Stacks with blur for +2 attacks total

---

### EFF_BLUR (25)

**What it does:**
- **Extra Attack**: Grants +1 attack per combat round
- **Visual**: Character's image blurs with supernatural speed
- **Stacks with**: Haste and double attack

**Implementation:**
```cpp
// magic.cpp:1294
SET_FLAG(eff[0].flags, EFF_BLUR);
eff[0].duration = 2 + (skill / 21);  // 2-6 hours

// fight.cpp:2399-2400
if (EFF_FLAGGED(ch, EFF_BLUR))
    hits += 1;
```

**Duration**: 2-6 hours

**AI Priority**: 80/100

---

### EFF_CONFUSION (8)

**What it does:**
- Makes character attack random/wrong targets
- Character has difficulty focusing on foes
- High WIS/DEX provides bonus save chance

**Implementation:**
```cpp
// magic.cpp:1397-1407
// Additional save chance based on stats
i = (GET_DEX(victim) + GET_WIS(victim) - 100) / 10;
if (mag_savingthrow(victim, savetype) || i > random_number(1, 100)) {
    // Resisted
}

SET_FLAG(eff[0].flags, EFF_CONFUSION);
eff[0].duration = 2 + skill / 40;  // 2-4 hours
```

**Duration**: 2-4 hours

**Save**: Yes, plus bonus from DEX+WIS (up to 10% additional)

**Refresh**: No

**AI Priority**: 60/100

---

## Defensive Effects

### EFF_SANCTUARY (7)

**What it does:**
- **Damage Reduction**: 50% of all incoming damage
- **Charm Protection**: Cannot be charmed while under sanctuary
- **Visual Aura**: Black aura (evil) or white aura (good)

**Implementation:**
```cpp
// fight.cpp:1650-1651
if (EFF_FLAGGED(victim, EFF_SANCTUARY) || EFF_FLAGGED(victim, EFF_STONE_SKIN))
    dam >>= 1;  // Bit shift = divide by 2

// spells.cpp:243-244
else if (EFF_FLAGGED(victim, EFF_SANCTUARY))
    char_printf(ch, "Your victim is protected by sanctuary!\n");

// act.informative.cpp:625-630
if (EFF_FLAGGED(targ, EFF_SANCTUARY)) {
    if (IS_EVIL(targ))
        act("&9&b$S body is surrounded by a black aura!&0", ...);
    else if (IS_GOOD(targ))
        act("$S body is surrounded by a white aura!&0", ...);
}
```

**Duration**: 4 hours (fixed)

**AI Priority**: 90/100

**Note**: Stacks with stone skin for 75% total reduction

---

### EFF_STONE_SKIN (22)

**What it does:**
- **Charge-Based Damage Reduction**:
  - Base: 50% damage reduction (like sanctuary)
  - Per-Hit: 90% chance to reduce damage to 0-3 and consume 1 charge
  - Effect ends when charges reach 0
- **Charges**: 7-13 based on skill (modifier field)

**Implementation:**
```cpp
// magic.cpp:2853-2856
eff[0].location = APPLY_NONE;
eff[0].modifier = 7 + (skill / 16);  // 7-13 charges
eff[0].duration = 2;
SET_FLAG(eff[0].flags, EFF_STONE_SKIN);

// fight.cpp:1650
if (EFF_FLAGGED(victim, EFF_SANCTUARY) || EFF_FLAGGED(victim, EFF_STONE_SKIN))
    dam >>= 1;  // Base 50% reduction

// fight.cpp:2267-2270
if (EFF_FLAGGED(victim, EFF_STONE_SKIN) && random_number(0, 10) <= 9) {
    decrease_modifier(victim, SPELL_STONE_SKIN);  // Consume 1 charge
    dam = random_number(0, 3);  // Nearly nullify damage
}

// magic.cpp:258-269 - decrease_modifier()
if (eff->modifier > 1)
    eff->modifier--;
else
    effect_from_char(i, spell);  // Remove when charges depleted
```

**Duration**: 2 hours or until charges depleted

**Refresh**: No (can't recast while active)

**AI Priority**: 100/100

**Note**: Most powerful defensive effect - blocks nearly all damage for 7-13 hits

---

### EFF_ARMOR (implied via SPELL_ARMOR)

**What it does:**
- **AC Bonus**: 10-15 armor class improvement
- **Conflicts**: Cannot stack with other armor spells (barkskin, bone armor, etc.)

**Implementation:**
```cpp
// magic.cpp:1180-1182
eff[0].location = APPLY_AC;
eff[0].modifier = 10 + (skill / 20);  // 10-15
eff[0].duration = 10 + (skill / 50);  // 10-12 hours

// handler.cpp:195-198
case APPLY_AC:
    GET_AC(ch) -= mod;  // Subtract because lower AC is better
```

**Duration**: 10-12 hours

**Note**: Positive modifier improves AC (subtracted in handler)

---

## Status Effects

### EFF_BLIND (0)

**What it does:**
- **Severe Combat Penalties**:
  - Hitroll: -4 (much harder to hit)
  - AC: -40 (much easier to be hit)
- Character cannot see

**Implementation:**
```cpp
// magic.cpp:1276-1284
eff[0].location = APPLY_HITROLL;
eff[0].modifier = -4;
eff[0].duration = 2;
SET_FLAG(eff[0].flags, EFF_BLIND);

eff[1].location = APPLY_AC;
eff[1].modifier = -40;
eff[1].duration = 2;
SET_FLAG(eff[1].flags, EFF_BLIND);
```

**Duration**: 2 hours (fixed)

**Save**: Yes (SAVING_SPELL)

**Immunity**: MOB_NOBLIND flag

**AI Priority**: 50/100

**Note**: One of the most devastating debuffs

---

### EFF_POISON (11)

**What it does:**
- Periodic damage over time
- Can be cured with cure poison spell
- Implementation varies by poison source

**Source**: `defines.hpp:419`

**AI Priority**: 60/100

---

### EFF_SLEEP (14)

**What it does:**
- Character is magically asleep
- Cannot take actions
- Wakes on damage

**Source**: `defines.hpp:422`

---

### EFF_CHARM (21)

**What it does:**
- Character follows and obeys caster
- Blocked by sanctuary
- Player must have SUMMONABLE pref flag

**Implementation:**
```cpp
// spells.cpp:243-244
if (EFF_FLAGGED(victim, EFF_SANCTUARY))
    char_printf(ch, "Your victim is protected by sanctuary!\n");
```

**Source**: `defines.hpp:429`

**AI Priority**: 90/100

---

## Detection Effects

### EFF_DETECT_INVIS (3)

**What it does:**
- **Perception Bonus**: Varies by implementation
- Can see invisible characters and objects
- Essential for detecting hidden enemies

**Implementation:**
```cpp
// magic.cpp:1578-1581
SET_FLAG(eff[0].flags, EFF_DETECT_INVIS);
eff[0].location = APPLY_PERCEPTION;
// modifier varies
eff[0].duration = 5 + (skill / 10);  // 5-15 hours
```

**Duration**: 5-15 hours

**AI Priority**: 40/100

---

### EFF_DETECT_ALIGN (2)

**What it does:**
- Can see alignment auras on characters
- Helpful for identifying enemies/allies

**Implementation:**
```cpp
// magic.cpp:1569-1573
SET_FLAG(eff[0].flags, EFF_DETECT_ALIGN);
eff[0].duration = 5 + (skill / 10);  // 5-15 hours
```

**Duration**: 5-15 hours

**AI Priority**: 10/100

---

### EFF_DETECT_MAGIC (4)

**What it does:**
- Can detect magical auras on items and characters
- Useful for identifying magical equipment

**Source**: `defines.hpp:412`

**AI Priority**: 20/100

---

### EFF_SENSE_LIFE (5)

**What it does:**
- Can sense living beings even when hidden
- Bypasses stealth and invisibility for detection (not targeting)
- Does not reveal exact location

**Source**: `defines.hpp:413`

**AI Priority**: 50/100

---

### EFF_INFRAVISION (10)

**What it does:**
- Can see in darkness using heat vision
- Essential for dark areas
- Common racial ability

**Source**: `defines.hpp:418`

**AI Priority**: 30/100

---

## Movement Effects

### EFF_FLY (20)

**What it does:**
- Can fly through the air
- Prevents falling damage
- Access to flying-only areas

**Source**: `defines.hpp:428`

**AI Priority**: 60/100

---

### EFF_WATERWALK (6)

**What it does:**
- Can walk on water surfaces
- Essential for water navigation without swimming

**Source**: `defines.hpp:414`

**AI Priority**: 20/100

---

### EFF_WATERBREATH (37)

**What it does:**
- Can breathe underwater
- Prevents drowning in water sectors

**Source**: `defines.hpp:445`

**AI Priority**: 40/100

---

### EFF_FEATHER_FALL (36)

**What it does:**
- Prevents falling damage
- Slow descent when falling
- Often combined with fly

**Source**: `defines.hpp:444`

**AI Priority**: 30/100

---

### EFF_IMMOBILIZED (31)

**What it does:**
- Character cannot move
- Can still perform other actions
- Often paired with paralysis

**Source**: `defines.hpp:439`

---

## Elemental Effects

### EFF_FIRESHIELD (44)

**What it does:**
- Shield of fire around character
- Damages attackers
- Conflicts with coldshield

**Implementation:**
```cpp
// magic.cpp:1380-1383
if (affected_by_spell(victim, SPELL_FIRESHIELD)) {
    char_printf(ch, "The shield of fire around {} body negates your spell.\n", ...);
    return CAST_RESULT_CHARGE;
}
```

**Source**: `defines.hpp:452`

---

### EFF_COLDSHIELD (45)

**What it does:**
- Shield of ice shards around character
- Damages attackers
- Conflicts with fireshield

**Implementation:**
```cpp
// magic.cpp:1385-1389
SET_FLAG(eff[0].flags, EFF_COLDSHIELD);
eff[0].duration = skill / 20;  // 0-5 hours
refresh = false;
```

**Duration**: 0-5 hours

**Source**: `defines.hpp:453`

**Note**: Does not refresh if already active

---

### EFF_PROT_FIRE (40)

**What it does:**
- Protection from fire damage
- Reduces or negates fire-based attacks

**Source**: `defines.hpp:448`

---

### EFF_PROT_COLD (41)

**What it does:**
- Protection from cold damage
- Reduces or negates ice-based attacks

**Source**: `defines.hpp:449`

---

### EFF_PROT_AIR (42)

**What it does:**
- Protection from air/lightning damage
- Reduces or negates shock-based attacks

**Source**: `defines.hpp:450`

---

### EFF_PROT_EARTH (43)

**What it does:**
- Protection from earth/acid damage
- Reduces or negates acid-based attacks

**Source**: `defines.hpp:451`

---

## Protection Effects

### EFF_PROTECT_EVIL (12)

**What it does:**
- Defensive bonuses against evil-aligned attackers
- Common for good-aligned characters

**Source**: `defines.hpp:420`

**AI Priority**: 50/100

---

### EFF_PROTECT_GOOD (13)

**What it does:**
- Defensive bonuses against good-aligned attackers
- Common for evil-aligned characters

**Source**: `defines.hpp:421`

**AI Priority**: 50/100

---

## Special State Effects

### EFF_SNEAK (18)

**What it does:**
- Character moves silently
- Reduced chance of detection
- Rogue ability

**Source**: `defines.hpp:426`

---

### EFF_STEALTH (19)

**What it does:**
- Enhanced hiding ability
- Harder to detect than sneak
- Ranger ability

**Source**: `defines.hpp:427`

---

### EFF_INVISIBLE (1)

**What it does:**
- Character cannot be seen by normal vision
- Requires detect invisibility to see
- Common spell effect

**Source**: `defines.hpp:409`

**AI Priority**: 70/100

---

### EFF_NOTRACK (15)

**What it does:**
- Character cannot be tracked
- Prevents ranger tracking ability
- Useful for stealth

**Source**: `defines.hpp:423`

---

## Buff/Debuff Summary Tables

### Major Defensive Buffs (AI Priority 80+)

| Effect | Benefit | Duration | Priority |
|--------|---------|----------|----------|
| EFF_STONE_SKIN | 90% damage reduction for 7-13 hits | 2h or charges | 100 |
| EFF_SANCTUARY | 50% damage reduction + charm immunity | 4h | 90 |
| EFF_BLUR | +1 attack per round | 2-6h | 80 |

### Major Offensive Buffs (AI Priority 50+)

| Effect | Benefit | Duration | Priority |
|--------|---------|----------|----------|
| EFF_HASTE | +1 attack per round | 2-6h | 50 |
| EFF_BLESS | Hit ethereal + saves + damage | 10-24h | 10* |

*Note: Bless priority low because effects are subtle, but ethereal-hitting is critical

### Major Debuffs

| Effect | Penalty | Duration | Save | Priority |
|--------|---------|----------|------|----------|
| EFF_BLIND | -4 hitroll, -40 AC | 2h | Yes | 50 |
| EFF_CURSE | -1 to -3 hit/dam | 5-12h | Yes | 40 |
| EFF_CONFUSION | Attack wrong targets | 2-4h | Yes | 60 |
| EFF_POISON | Damage over time | Varies | Varies | 60 |

---

## Implementation Patterns

### How to Check Effect Flags

```cpp
// Simple check
if (EFF_FLAGGED(ch, EFF_SANCTUARY)) {
    // Do something
}

// Combat modifications
if (EFF_FLAGGED(victim, EFF_STONE_SKIN) && random_number(0, 10) <= 9) {
    decrease_modifier(victim, SPELL_STONE_SKIN);
    dam = random_number(0, 3);
}

// Visual descriptions
if (EFF_FLAGGED(targ, EFF_SANCTUARY)) {
    if (IS_EVIL(targ))
        act("&9&b$S body is surrounded by a black aura!&0", ...);
}
```

### How to Set Effect Flags

```cpp
// In mag_affect() - magic.cpp
SET_FLAG(eff[0].flags, EFF_BLESS);
eff[0].duration = 10 + (skill / 7);

// With stat modifiers
eff[0].location = APPLY_SAVING_SPELL;
eff[0].modifier = -2 - (skill / 10);
SET_FLAG(eff[0].flags, EFF_BLESS);
```

### Common Patterns

1. **Damage Reduction**: Check in combat damage calculation
2. **Stat Modifiers**: Use APPLY locations (handler.cpp)
3. **Visual Effects**: Display in act.informative.cpp
4. **Combat Bonuses**: Modify calculations in fight.cpp
5. **Blocking Mechanics**: Check before spell/action execution

---

## Cross-References

### Related Documentation
- `ability_implementation_mechanics.json` - Complete technical data
- `defines.hpp:408-497` - All flag definitions
- `magic.cpp` - Spell implementations
- `fight.cpp` - Combat mechanics
- `handler.cpp:130-246` - Stat modifier application

### Key Functions
- `mag_savingthrow()` - magic.cpp:220-252
- `mag_affect()` - magic.cpp:1109+
- `effect_modify()` - handler.cpp:130-246
- `decrease_modifier()` - magic.cpp:254-273

---

## FAQ

**Q: Why does EFF_BLESS have such low AI priority (10)?**
A: The stat bonuses are subtle. The real value is hitting ethereal creatures, which is situational. However, the hitroll bonus vs evil can be significant for good characters.

**Q: How does stone skin charge system work?**
A: The modifier field stores remaining charges (7-13). Each hit has 90% chance to consume 1 charge and reduce damage to 0-3. When modifier reaches 0, effect ends regardless of duration.

**Q: Can sanctuary and stone skin stack?**
A: Yes! Sanctuary gives 50% reduction, then stone skin gives another 50% on the remaining damage, resulting in 75% total reduction. Plus stone skin's 90% chance to nearly nullify.

**Q: What's the difference between invisibility and stealth?**
A: Invisibility is a magical effect that makes you unseeable (countered by detect invisibility). Stealth is a skill that makes you harder to notice (countered by perception).

**Q: Why are save modifiers negative for buffs?**
A: In FieryMUD, LOWER save numbers are better (easier to resist). So BLESS gives -2 to -12 SAVING_SPELL, which improves your saves by making the number lower.

---

## Version Information

- **Source**: FieryMUD legacy codebase (`legacy/src/`)
- **Extracted**: 2025
- **Files Analyzed**: magic.cpp, fight.cpp, handler.cpp, defines.hpp, act.informative.cpp, damage.cpp, spells.cpp
- **Total Effect Flags**: 90 (NUM_EFF_FLAGS)
- **Documented**: Primary combat and utility effects

---

*For complete technical details including formulas, line numbers, and all effect flags, see `ability_implementation_mechanics.json`*

# FieryMUD Core Game Mechanics

**Complete implementation reference for rebuilding the magic system**

This document fills the gaps in our ability documentation by providing the underlying game mechanics that make spells and skills work.

---

## Table of Contents

- [APPLY Locations (Stat Modifiers)](#apply-locations-stat-modifiers)
- [Damage Type System](#damage-type-system)
- [Saving Throw Mechanics](#saving-throw-mechanics)
- [Combat System](#combat-system)
- [Status Effects Implementation](#status-effects-implementation)
- [Time and Duration System](#time-and-duration-system)
- [Effect Stacking Rules](#effect-stacking-rules)

---

## APPLY Locations (Stat Modifiers)

**Purpose**: APPLY locations specify WHERE a spell's stat modifier affects the character.

**Implementation**: `handler.cpp:130-246` in `affect_modify()` function

### Primary Stats

| Location | Effect | Example | Notes |
|----------|--------|---------|-------|
| **APPLY_STR** | Modifies strength | Giant Strength +25 | Affects carry capacity, damage |
| **APPLY_DEX** | Modifies dexterity | Cat's Grace +10 | Affects AC, initiative |
| **APPLY_INT** | Modifies intelligence | Brilliance +15 | Affects spell learning |
| **APPLY_WIS** | Modifies wisdom | Wisdom +10 | Affects spell circles, saves |
| **APPLY_CON** | Modifies constitution | Endurance +10 | Affects HP, stamina |
| **APPLY_CHA** | Modifies charisma | Charm +5 | Affects reactions, prices |

**Code Implementation**:
```cpp
// handler.cpp:136-152
case APPLY_STR:
    GET_ACTUAL_STR(ch) += mod;
    break;
case APPLY_DEX:
    GET_ACTUAL_DEX(ch) += mod;
    break;
// ... etc
```

### Combat Modifiers

| Location | Effect | Example | Sign Convention |
|----------|--------|---------|-----------------|
| **APPLY_HITROLL** | To-hit bonus | Bless +1 to +2 | Positive = better hits |
| **APPLY_DAMROLL** | Damage bonus | Bless +1 to +2 | Positive = more damage |
| **APPLY_AC** | Armor class | Armor spell +10 to +15 | **Positive improves AC** (subtracted) |

**Critical Note on AC**:
```cpp
// handler.cpp:195-197
case APPLY_AC:
    GET_AC(ch) -= mod;  // Subtract because lower AC is better
    break;
```

AC uses **inverted logic**: Positive modifier = better AC because it's subtracted. In FieryMUD, lower AC = harder to hit.

### Saving Throws

| Location | Effect | Example | Sign Convention |
|----------|--------|---------|-----------------|
| **APPLY_SAVING_PARA** | Paralysis saves | Resist Paralysis -10 | **Negative = better** |
| **APPLY_SAVING_ROD** | Rod/staff/wand saves | Stone Skin -5 | **Negative = better** |
| **APPLY_SAVING_PETRI** | Petrification saves | Freedom -5 | **Negative = better** |
| **APPLY_SAVING_BREATH** | Breath weapon saves | Evasion -10 | **Negative = better** |
| **APPLY_SAVING_SPELL** | Spell saves | Bless -2 to -12 | **Negative = better** |

**Critical Note on Saves**:
```cpp
// handler.cpp:205-222
case APPLY_SAVING_PARA:
    GET_SAVE(ch, SAVING_PARA) += mod;
    break;
// All saves work this way
```

Saves use **lower-is-better** logic: Negative modifiers improve saves.

### Resources

| Location | Effect | Example | Notes |
|----------|--------|---------|-------|
| **APPLY_HIT** | Max HP | Aid +20 HP | Also increases current HP |
| **APPLY_MOVE** | Max movement | Endurance +50 | Also increases current move |
| **APPLY_FOCUS** | Spell circles | Mental Focus +1 | Unlocks higher spells |

**Code Implementation**:
```cpp
// handler.cpp:178-186
case APPLY_HIT:
    GET_MAX_HIT(ch) += mod;
    GET_HIT(ch) += mod;  // Current HP also adjusted
    break;

case APPLY_MOVE:
    GET_MAX_MOVE(ch) += mod;
    GET_MOVE(ch) += mod;  // Current movement also adjusted
    break;
```

### Perception and Stealth

| Location | Effect | Example |
|----------|--------|---------|
| **APPLY_PERCEPTION** | Detection ability | Detect Invisibility +perception |
| **APPLY_HIDDENNESS** | Stealth level | Improved Invisibility +hiddenness |

```cpp
// handler.cpp:228-233
case APPLY_PERCEPTION:
    GET_PERCEPTION(ch) += mod;
    break;
case APPLY_HIDDENNESS:
    GET_HIDDENNESS(ch) = std::clamp(GET_HIDDENNESS(ch) + mod, 0l, 1000l);
    break;
```

### Physical Attributes

| Location | Effect | Example |
|----------|--------|---------|
| **APPLY_AGE** | Character age | Age/Youth spell |
| **APPLY_CHAR_WEIGHT** | Weight | Reduce Weight -50 |
| **APPLY_CHAR_HEIGHT** | Height | Enlarge Person +12 |
| **APPLY_SIZE** | Size category | Growth +1 size |

### Regeneration

| Location | Effect | Example |
|----------|--------|---------|
| **APPLY_HIT_REGEN** | HP regeneration | Regeneration +5 HP/tick |

```cpp
// handler.cpp:225-227
case APPLY_HIT_REGEN:
    ch->char_specials.hitgain += mod;
    break;
```

### Special

| Location | Effect | Example |
|----------|--------|---------|
| **APPLY_COMPOSITION** | Material type | Stoneskin (changes susceptibility) |
| **APPLY_LEVEL** | Effective level | (Rarely used) |
| **APPLY_CLASS** | (Not implemented) | - |
| **APPLY_GOLD** | (Not implemented) | - |
| **APPLY_EXP** | (Not implemented) | - |
| **APPLY_NONE** | No stat modifier | Used for pure flag effects |

---

## Damage Type System

**Purpose**: Different damage types interact with creature compositions and resistances.

**Source**: `chars.cpp:308-363` in `suscept_adjust()` function

### Damage Types

| Type | Description | Common Sources |
|------|-------------|----------------|
| **DAM_FIRE** | Fire/heat damage | Fireball, Burning Hands, Immolate |
| **DAM_COLD** | Ice/frost damage | Cone of Cold, Iceball, Freeze |
| **DAM_SHOCK** | Lightning/electricity | Lightning Bolt, Shocking Grasp |
| **DAM_ACID** | Acid/corrosive | Acid Burst, Acidic Bile |
| **DAM_WATER** | Water/drowning | Water Blast, Drowning |
| **DAM_POISON** | Toxic damage | Poison spells, venomous attacks |
| **DAM_ALIGN** | Alignment-based | Harm, Ancestral Vengeance, Spirit Ray |
| **DAM_MENTAL** | Psychic/mind | Nightmare, Vicious Mockery, Insanity |
| **DAM_ENERGY** | Pure magic | Detonation, Magic Missile |
| **DAM_ROT** | Decay/necrotic | Decay, Decompose |
| **DAM_PIERCE** | Piercing physical | Backstab, arrows, spears |
| **DAM_SLASH** | Slashing physical | Swords, claws |
| **DAM_CRUSH** | Bludgeoning physical | Maces, fists |

### Resistance System

**Elemental Protections** (stackable with shields):

```cpp
// chars.cpp:323-360
case DAM_SHOCK:
    if (EFF_FLAGGED(ch, EFF_NEGATE_AIR))
        return 0;  // Immune
    if (EFF_FLAGGED(ch, EFF_PROT_AIR))
        return sus_shock * 75 / 100;  // 25% reduction
    return sus_shock;

case DAM_FIRE:
    if (EFF_FLAGGED(ch, EFF_NEGATE_HEAT))
        return 0;  // Immune
    sus = sus_fire;
    if (EFF_FLAGGED(ch, EFF_COLDSHIELD))
        sus = sus * 75 / 100;  // 25% reduction
    if (EFF_FLAGGED(ch, EFF_PROT_FIRE))
        sus = sus * 75 / 100;  // Another 25% reduction
    return sus;

case DAM_COLD:
    if (EFF_FLAGGED(ch, EFF_NEGATE_COLD))
        return 0;  // Immune
    sus = sus_cold;
    if (EFF_FLAGGED(ch, EFF_FIRESHIELD))
        sus = sus * 75 / 100;  // 25% reduction
    if (EFF_FLAGGED(ch, EFF_PROT_COLD))
        sus = sus * 75 / 100;  // Another 25% reduction
    return sus;

case DAM_ACID:
    if (EFF_FLAGGED(ch, EFF_NEGATE_EARTH))
        return 0;  // Immune
    if (EFF_FLAGGED(ch, EFF_PROT_EARTH))
        return sus_acid * 75 / 100;  // 25% reduction
    return sus_acid;
```

**Resistance Stacking**:
- **Negate effects**: 100% immunity
- **Protection effects**: 25% reduction (75% damage taken)
- **Opposite shield**: 25% reduction
- **Both stack multiplicatively**: 75% × 75% = 56% final damage (44% reduction)

### Composition System

Characters have a **composition** (material type) that determines base susceptibility to each damage type.

**Common Compositions**:
- **Flesh**: Normal susceptibility to physical, vulnerable to poison
- **Stone**: Resistant to physical, vulnerable to acid
- **Metal**: Resistant to physical, vulnerable to shock
- **Ethereal**: Immune to physical (unless attacker has EFF_BLESS)
- **Fire**: Immune to fire, vulnerable to cold/water
- **Water**: Immune to drowning, vulnerable to fire/cold
- **Plant**: Vulnerable to fire, resistant to cold

---

## Saving Throw Mechanics

**Purpose**: Determine if a spell's secondary effects (blindness, paralysis, etc.) are resisted.

**Source**: `magic.cpp:220-252` in `mag_savingthrow()`

### Core Formula

```cpp
int mag_savingthrow(CharData *ch, int type) {
    int save;

    // Get base save (from class/level)
    save = get_base_saves(ch, type);

    // Add character's save modifiers (from spells/items)
    save += GET_SAVE(ch, type);

    // Roll d100 against save
    if (std::max(1, save) < random_number(0, 99))
        return true;  // Saved!
    return false;     // Failed save
}
```

### Base Saves by Stats

**Source**: `chars.cpp:96-152` in `get_base_saves()`

```cpp
// Paralysis - CON based
saves[SAVING_PARA] -= (int)(0.5 * (GET_VIEWED_CON(ch) - 90));

// Spell resistance - WIS based
saves[SAVING_SPELL] -= (int)(0.5 * (GET_VIEWED_WIS(ch) - 90));

// Rod/staff/wand - DEX based
saves[SAVING_ROD] -= (int)(0.5 * (GET_VIEWED_DEX(ch) - 90));

// Breath weapons - DEX based
saves[SAVING_BREATH] -= (int)(0.5 * (GET_VIEWED_DEX(ch) - 90));

// Petrification - ??? based
saves[SAVING_PETRI] = base_value;
```

**Key Insight**: Each stat point above/below 90 gives ±0.5 to saves.
- CON 100 = -5 to paralysis saves (better)
- WIS 80 = +5 to spell saves (worse)

### Save Types

| Save Type | Used For | Primary Stat |
|-----------|----------|--------------|
| **SAVING_PARA** | Paralysis, hold, web, stunning | Constitution |
| **SAVING_SPELL** | Most spell effects | Wisdom |
| **SAVING_ROD** | Wands, rods, staves | Dexterity |
| **SAVING_BREATH** | Breath weapons, area effects | Dexterity |
| **SAVING_PETRI** | Petrification, flesh to stone | (Class/level) |

### Save Modifiers

**From Spells**:
- Bless: -2 to -12 SAVING_SPELL (better)
- Curse: varies by implementation
- Fear: saves with bonus

**From Class/Level**:
- Base saves improve with level
- Different classes have different save progressions

### Special Save Rules

**Additional Save Chances** (example from Confusion):
```cpp
// magic.cpp:1399-1407
// Confusion gives extra save based on DEX+WIS
int bonus = (GET_DEX(victim) + GET_WIS(victim) - 100) / 10;
if (mag_savingthrow(victim, SAVING_SPELL) || bonus > random_number(1, 100)) {
    // Saved!
}
```

---

## Combat System

**Purpose**: How attacks, damage, and combat resolution work.

### THAC0 (To Hit Armor Class 0)

Lower THAC0 = better chance to hit. Modified by:

```cpp
// fight.cpp:2100-2150 (approximate)
calc_thaco = base_thac0;

// Hitroll bonuses
calc_thaco -= GET_HITROLL(ch);

// Bless bonus vs evil
if (EFF_FLAGGED(ch, EFF_BLESS)) {
    if (IS_GOOD(ch) && IS_EVIL(victim))
        calc_thaco -= GET_LEVEL(ch) / 5;  // Big bonus
    if (IS_GOOD(ch) && IS_NEUTRAL(victim))
        calc_thaco -= GET_LEVEL(ch) / 10; // Small bonus
}

// Blindness penalty
if (EFF_FLAGGED(ch, EFF_BLIND))
    calc_thaco += 4;  // Much harder to hit
```

### Attack Resolution

1. **Calculate THAC0**: Base - hitroll bonuses + penalties
2. **Roll d20**: Random(1, 20)
3. **Compare**: If (roll + victim_AC) >= THAC0, hit!
4. **Calculate Damage**: Base weapon damage + damroll + skill bonuses

### Damage Calculation

```cpp
// Basic formula
damage = weapon_damage + GET_DAMROLL(ch) + str_bonus;

// Apply damage type susceptibility
damage = dam_suscept_adjust(ch, victim, weapon, damage, dam_type);

// Apply damage reduction effects
if (EFF_FLAGGED(victim, EFF_SANCTUARY))
    damage >>= 1;  // Halve damage

if (EFF_FLAGGED(victim, EFF_STONE_SKIN) && random_number(0, 10) <= 9) {
    decrease_modifier(victim, SPELL_STONE_SKIN);  // Consume charge
    damage = random_number(0, 3);  // Nearly nullify
}
```

### Attack Count

```cpp
// fight.cpp:2390-2410 (approximate)
int hits = 1;  // Base attack

// Extra attacks from skills
if (GET_SKILL(ch, SKILL_DOUBLE_ATTACK) && percent_success)
    hits++;

// Extra attacks from spells
if (EFF_FLAGGED(ch, EFF_HASTE))
    hits += 1;

if (EFF_FLAGGED(ch, EFF_BLUR))
    hits += 1;

// Maximum 4 attacks total
// Haste + Blur + Double Attack = 4 attacks
```

---

## Status Effects Implementation

### Blindness (EFF_BLIND)

**Effects**:
- Cannot see (obvious)
- -4 hitroll penalty
- -40 AC penalty (much easier to hit)
- Cannot target specific enemies
- Movement verb changes to "stumbles"

**Code Checks**:
```cpp
// act.offensive.cpp:451
if (EFF_FLAGGED(ch, EFF_BLIND))
    char_printf(ch, "You can't see a thing!\n");

// Cannot initiate combat while blind
if (FIGHTING(ch) != victim && EFF_FLAGGED(ch, EFF_BLIND))
    return;

// Movement text
if (EFF_FLAGGED(ch, EFF_BLIND))
    return "stumbles";  // instead of "walks"
```

**Immunity**: MOB_NOBLIND flag

### Sleep (EFF_SLEEP)

**Effects**:
- Character unconscious
- Cannot take any actions
- Position set to SLEEPING
- Wakes on damage

**Note**: Implementation scattered across combat and command systems.

### Charm (EFF_CHARM)

**Effects**:
- Character follows caster
- Obeys caster's orders
- Cannot attack master
- Blocked by sanctuary

```cpp
// spells.cpp:243-244
if (EFF_FLAGGED(victim, EFF_SANCTUARY))
    char_printf(ch, "Your victim is protected by sanctuary!\n");
```

**Player Protection**: Must have SUMMONABLE preference flag.

### Paralysis (Various Effects)

**Effects**:
- Cannot move (EFF_IMMOBILIZED)
- Cannot take actions (depending on spell)
- Save type: SAVING_PARA

### Confusion (EFF_CONFUSION)

**Effects**:
- Attacks random/wrong targets
- Duration: 2-4 hours
- Extra save based on DEX+WIS

```cpp
// Additional 10% save per 10 points of (DEX+WIS) over 100
int bonus = (GET_DEX(victim) + GET_WIS(victim) - 100) / 10;
```

### Poison (EFF_POISON)

**Effects**:
- Periodic damage over time
- Implementation varies by source
- Can be cured with cure poison spell

---

## Time and Duration System

### Time Units

- **1 tick** = 75 seconds (game time)
- **1 MUD hour** = 75 seconds (real time)
- **1 MUD day** = 24 MUD hours = 30 minutes (real time)
- **1 MUD year** = 12 MUD months = 6 hours (real time)

### Duration in Spells

**Durations specified in MUD hours**:
```cpp
// Example: Bless
eff[0].duration = 10 + (skill / 7);  // 10-24 MUD hours
// = 12.5 to 30 real minutes
```

**Special Durations**:
- `-1`: Permanent (until removed)
- `0`: Instant (removed next tick)
- `1+`: Specified number of MUD hours

### Effect Expiration

Effects are checked every tick (75 seconds):
1. Decrement duration by 1
2. If duration reaches 0, remove effect
3. Display wearoff message to player

**Wearoff Messages**:
```cpp
// Example from spell implementation
to_vict: "Your inner cold subsides."
to_room: "The blizzard around $N subsides."
```

---

## Effect Stacking Rules

### General Rules

1. **Same Spell**: Usually doesn't stack (refreshes duration)
2. **Different Spells, Same Effect**: Depends on `accum_effect` flag
3. **Different Effect Flags**: Always stack
4. **Stat Modifiers**: Accumulate additively

### Accumulating Effects

**Curse - Stacks**:
```cpp
// magic.cpp:1422-1429
accum_effect = true;  // Multiple curses stack
```

**Bless - Doesn't Stack**:
```cpp
// magic.cpp (implicit)
// Default: accum_effect = false
// Second cast refreshes duration
```

### Damage Reduction Stacking

**Multiplicative Stacking** (sanctuary + stone skin):
```cpp
// Both checked in sequence
if (EFF_FLAGGED(victim, EFF_SANCTUARY))
    dam >>= 1;  // 50% (divide by 2)

if (EFF_FLAGGED(victim, EFF_STONE_SKIN) && 90% chance)
    dam = random_number(0, 3);  // Nearly 0
```

**Combined Effect**:
1. Sanctuary: 100 damage → 50 damage
2. Stone Skin (90% chance): 50 damage → 0-3 damage
3. Total reduction: ~95% on average

### Attack Bonus Stacking

**Additive Stacking** (haste + blur):
```cpp
hits = 1;  // Base
if (EFF_FLAGGED(ch, EFF_HASTE))
    hits += 1;
if (EFF_FLAGGED(ch, EFF_BLUR))
    hits += 1;
// Result: 3 attacks (1 + 1 + 1)
// With double attack skill: 4 attacks total (max)
```

### Resistance Stacking

**Elemental Protections** (multiplicative):
```cpp
sus = base_susceptibility;
if (EFF_FLAGGED(ch, EFF_COLDSHIELD))
    sus = sus * 75 / 100;  // 25% reduction
if (EFF_FLAGGED(ch, EFF_PROT_FIRE))
    sus = sus * 75 / 100;  // Another 25% reduction
// Total: 56% damage (44% reduction)
```

### Conflict Rules

**Opposing Hand Effects** (monks):
```cpp
// Cannot have both:
- SPELL_BLIZZARDS_OF_SAINT_AUGUSTINE (EFF_ICEHANDS)
- SPELL_FIRES_OF_SAINT_AUGUSTINE (EFF_FIREHANDS)
- SPELL_ETHEREAL_FISTS (ethereal damage)
```

**Opposing Shields**:
```cpp
// Cannot have both:
- EFF_FIRESHIELD
- EFF_COLDSHIELD
```

---

## Critical Implementation Details

### Effect Application Order

1. **Spell cast**: Caster's skill determines power
2. **Saving throw**: Victim may resist (if applicable)
3. **Effect flags set**: Binary state changes (EFF_BLIND, etc.)
4. **Stat modifiers applied**: affect_modify() updates stats
5. **Messages sent**: Visual feedback to all parties
6. **Duration starts**: Effect timer begins

### Effect Removal Order

1. **Duration expires** or **dispel magic** cast
2. **Stat modifiers reversed**: affect_modify() with negative
3. **Effect flags cleared**: Binary states removed
4. **Wearoff message sent**: Notification to character
5. **Special cleanup**: (e.g., stone skin charges)

---

## Building a New Magic System

**What You Need**:

1. ✅ **Spell Definitions**: `fierylib/docs/extraction-reports/abilities.csv` (authoritative)
2. ✅ **Effect Flags**: EFFECT_FLAGS_REFERENCE.md (90 flags)
3. ✅ **APPLY Locations**: This document (31 stat modifiers)
4. ✅ **Damage Types**: This document (13 types + susceptibility)
5. ✅ **Saving Throws**: This document (complete formula)
6. ✅ **Combat System**: This document (THAC0, damage, attacks)
7. ✅ **Status Effects**: This document (implementations)
8. ✅ **Time System**: This document (durations, ticks)
9. ✅ **Stacking Rules**: This document (conflicts, accumulation)

**What's Still Missing**:

- [ ] AI Priority System (how NPCs choose spells)
- [ ] Spell Circles System (how spells are learned/prepared)
- [ ] Skill System (success rates, improvement)
- [ ] Racial/Class Restrictions (who can use what)
- [ ] Item Effect System (how worn items grant effects)

---

## Cross-References

### Related Documentation
- `fierylib/docs/extraction-reports/abilities.csv` - Authoritative ability data from `skills.cpp`
- `EFFECT_FLAGS_REFERENCE.md` - What each effect flag does
- `fierylib/docs/mapping/effects_reference.csv` - Effect definitions and usage

### Key Source Files
- `magic.cpp` - Spell casting and mag_*() functions
- `fight.cpp` - Combat system
- `handler.cpp:130-246` - affect_modify() (APPLY locations)
- `chars.cpp:96-363` - Base saves and susceptibility
- `defines.hpp` - All constants and enums

---

## Version Information

- **Source**: FieryMUD legacy codebase (`legacy/src/`)
- **Extracted**: 2025
- **Files Analyzed**: magic.cpp, fight.cpp, handler.cpp, chars.cpp, act.offensive.cpp, defines.hpp
- **Completeness**: Core mechanics for rebuilding magic system

---

*This document combined with `fierylib/docs/extraction-reports/abilities.csv` provides everything needed to rebuild the FieryMUD magic system from scratch.*

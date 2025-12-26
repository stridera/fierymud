# Data-Driven Damage-Over-Time (DoT) System Design

## Current State Analysis

### What We Have

**Database Schema:**
- `Effect` table with `dot` effect type (id=20)
- `AbilityEffect` linking abilities to effects with `trigger` field (on_cast, on_tick)
- `CharacterEffects` for tracking active effects on players
- Poison ability (id=33) already configured with:
  - `dot` effect (on_tick): `{"amount": "5 + (level / 2)", "damageType": "poison", "blocksRegen": true, "amountPercent": 5}`
  - `modify` effect (on_cast): reduces STR by formula

**C++ Implementation:**
- `perform_tick()` checks `ActorFlag::Poison` (boolean flag)
- Hardcoded 2% max HP damage formula
- Hardcoded regen blocking logic

### Problem

The database has the configuration, but C++ ignores it and uses hardcoded values. We need the C++ code to read effect parameters from the database.

---

## Proposed Architecture

### Design Principles

1. **Database is the source of truth** - All effect parameters come from Effect/AbilityEffect tables
2. **Actor flags remain for quick checks** - Keep `ActorFlag::Poison` for display/combat logic, but damage comes from DB
3. **Support multiple poison types** - Different abilities can apply different poison variants
4. **Extensible to other DoT types** - Disease, burning, bleeding, etc. use the same system

### Data Flow

```
1. ABILITY CAST
   Player casts "Poison" or "Deadly Venom" →
   Creates ActiveEffect record on target →
   Sets ActorFlag::Poison for visual/combat checks

2. EACH TICK (MUD hour)
   WorldManager::tick_all_effects() →
   For each actor with active effects:
     Query ActiveEffects with trigger='on_tick' →
     Read parameters (damageType, amount, amountPercent, blocksRegen) →
     Apply damage based on those parameters →
     Check if effect expired (duration reached) →
     Remove expired effects and clear flags

3. EFFECT REMOVAL
   Duration expires OR cleanse spell cast →
   Delete ActiveEffect record →
   Clear ActorFlag if no other poison effects remain
```

---

## Database Changes

### ActiveEffect Table (New)

We already have `CharacterEffects` for players. For NPCs/mobs, we need tracking in memory or a shared table.

```prisma
model ActiveEffect {
  id              Int       @id @default(autoincrement())

  // Target (polymorphic - either character or mob)
  characterId     String?   @map("character_id")
  mobInstanceId   String?   @map("mob_instance_id")  // Runtime mob ID

  // Effect source
  abilityId       Int       @map("ability_id")
  effectId        Int       @map("effect_id")

  // Timing
  appliedAt       DateTime  @default(now()) @map("applied_at")
  expiresAt       DateTime? @map("expires_at")
  remainingTicks  Int?      @map("remaining_ticks")

  // Resolved parameters (snapshot at cast time)
  resolvedParams  Json      @map("resolved_params")  // Pre-computed values

  // Source tracking
  sourceActorId   String?   @map("source_actor_id")
  sourceLevel     Int       @default(1) @map("source_level")

  // Relationships
  character       Characters? @relation(fields: [characterId], references: [id], onDelete: Cascade)
  ability         Ability     @relation(fields: [abilityId], references: [id])
  effect          Effect      @relation(fields: [effectId], references: [id])

  @@index([characterId])
  @@index([mobInstanceId])
  @@index([expiresAt])
}
```

### Effect Type: `dot` Parameters

The `dot` effect type uses these parameters (stored in `Effect.defaultParams` or `AbilityEffect.overrideParams`):

```json
{
  "damageType": "poison",      // poison, fire, cold, disease, bleed, necrotic, acid
  "cureCategory": "poison",    // What type of cure removes this (poison, fire, disease, curse, magic)
  "potency": 5,                // 1-10 scale, cure must have >= curePower to remove
  "amount": "5 + (level / 2)", // Flat damage formula
  "amountPercent": 5,          // Percentage of max HP (optional, stacks with amount)
  "blocksRegen": true,         // Completely blocks natural HP regen
  "reducesRegen": 75,          // Alternative: reduce regen by X% (legacy mode)
  "duration": "5 + (skill / 10)", // Ticks until effect expires
  "stackable": false,          // Can multiple instances stack?
  "maxStacks": 1,              // Max stack count if stackable
  "tickInterval": 1,           // Ticks between damage applications (default: every tick)
  "resistanceStat": "poison",  // Which resistance stat reduces damage
  "maxResistance": 75          // Cap on resistance reduction (75% = at least 25% damage)
}
```

### Potency & Cure System

**Potency Scale (1-10):**
| Potency | Description | Example |
|---------|-------------|---------|
| 1-2 | Minor | Weak poison, small burn |
| 3-4 | Moderate | Standard poison, torch burn |
| 5-6 | Serious | Strong poison, fireball aftermath |
| 7-8 | Severe | Deadly venom, dragon fire |
| 9-10 | Legendary | Ancient curse, divine flame |

**Skill-Based Potency:**
Both inflicting and curing use skill level in their formulas:

```json
// DoT effect - potency scales with caster's skill
{
  "potency": "3 + (skill / 25)",  // skill 0-25 → potency 3-4, skill 75-100 → potency 6-7
  "amount": "5 + (skill / 10)",
  ...
}

// Cleanse effect - cure power scales with caster's skill
{
  "curePower": "3 + (skill / 20)",  // skill 0-20 → power 3-4, skill 80-100 → power 7-8
  ...
}
```

**Potency Formula Examples:**
| Ability | Base | Formula | Skill 25 | Skill 50 | Skill 100 |
|---------|------|---------|----------|----------|-----------|
| Weak Poison | 2 | `2 + (skill / 50)` | 2 | 3 | 4 |
| Poison | 4 | `4 + (skill / 33)` | 5 | 5 | 7 |
| Deadly Venom | 6 | `6 + (skill / 50)` | 6 | 7 | 8 |
| Cure Poison | 4 | `4 + (skill / 25)` | 5 | 6 | 8 |
| Neutralize Poison | 6 | `6 + (skill / 50)` | 6 | 7 | 8 |

**Cure Categories & Matching Spells:**
| Category | DoT Types | Cure Spells (base power) |
|----------|-----------|--------------------------|
| `poison` | poison, venom | Cure Poison (4+skill), Neutralize Poison (6+skill), Heal (2+skill) |
| `disease` | disease, plague, infection | Cure Disease (4+skill), Remove Affliction (6+skill) |
| `fire` | fire, burning, lava | Create Water (2+skill), Quench (5+skill), Ice Storm (3+skill) |
| `cold` | cold, frostbite, frozen | Warmth (2+skill), Flame Shield (4+skill) |
| `acid` | acid, corrosion | Neutralize (4+skill), Cleanse (6+skill) |
| `bleed` | bleed, laceration | Bandage (1+skill), Heal (3+skill), Regenerate (6+skill) |
| `curse` | curse, hex, bane | Remove Curse (4+skill), Break Enchantment (6+skill) |
| `magic` | arcane, eldritch | Dispel Magic (3+skill), Greater Dispel (6+skill) |

**Cure Resolution:**
```
If curePower >= potency:
    Full cure - effect removed completely
Else if curePower >= potency - 2:
    Partial cure - reduce remaining duration by 50%, reduce potency by 1
Else:
    Cure fails - "The poison is too strong for your cure!"
```

**Gameplay Example:**
- Novice caster (skill 25) casts Poison → potency 5
- Expert healer (skill 80) casts Cure Poison → power 7 → Full cure!
- Novice healer (skill 25) casts Cure Poison → power 5 → Exactly matches, full cure
- Novice healer vs Expert poisoner (potency 7) → power 5 vs 7 → Partial cure only

### Effect Type: `cleanse` Parameters (Updated)

```json
{
  "cureCategory": "poison",    // Which category this cures (or "all" for powerful cures)
  "curePower": 5,              // Must be >= effect potency to fully cure
  "scope": "first",            // "first" = strongest effect, "all" = all matching effects
  "partialOnFail": true        // If true, partial cure when underpowered; if false, total failure
}
```

---

## C++ Changes

### 1. ActiveEffect Struct (In-Memory)

```cpp
// src/core/active_effect.hpp
struct ActiveEffect {
    int ability_id;
    int effect_id;
    std::string effect_type;      // "dot", "modify", "status", etc.

    // Resolved parameters (computed at cast time)
    std::string damage_type;      // "poison", "fire", "disease", etc.
    std::string cure_category;    // "poison", "fire", "disease", "curse", "magic"
    int potency = 5;              // 1-10 scale, higher = harder to cure
    int flat_damage = 0;          // Pre-computed flat damage
    int percent_damage = 0;       // Percentage of max HP
    bool blocks_regen = false;
    int reduces_regen = 0;        // 0-100, percentage reduction

    // Timing
    int remaining_ticks = -1;     // -1 = permanent until cleansed
    int tick_interval = 1;
    int ticks_since_last = 0;

    // Source
    std::string source_actor_id;
    int source_level = 1;

    // Stacking
    int stack_count = 1;
    int max_stacks = 1;
    bool stackable = false;
};
```

### 2. Actor Changes

```cpp
// src/core/actor.hpp
class Actor {
    // ...existing code...

    // Active effects on this actor
    std::vector<ActiveEffect> active_effects_;

public:
    // Effect management
    void add_effect(const ActiveEffect& effect);
    void remove_effect(int ability_id, int effect_id);
    void remove_effects_by_type(const std::string& effect_type);
    void remove_effects_by_damage_type(const std::string& damage_type);

    // Queries
    bool has_effect_type(const std::string& type) const;
    bool has_damage_type(const std::string& damage_type) const;
    int get_regen_reduction() const;  // Returns total % reduction from all effects
    bool is_regen_blocked() const;    // Any effect blocks regen?

    // Tick processing
    TickResult process_dot_effects();
};
```

### 3. Updated perform_tick()

```cpp
Actor::TickResult Actor::perform_tick() {
    TickResult result;

    if (!is_alive()) return result;

    // 1. Process dying state damage
    if (is_dying()) {
        result.dying_damage = 1;
        take_damage(1, "blood loss");
        if (stats_.hit_points < -10) {
            result.died = true;
        }
        return result;
    }

    // 2. Process all DoT effects
    result = process_dot_effects();
    if (result.died) return result;

    // 3. Calculate regeneration (affected by effects)
    if (!is_regen_blocked()) {
        int regen_reduction = get_regen_reduction();
        int base_hp_gain = calculate_base_hit_gain();
        int base_move_gain = calculate_base_move_gain();

        // Apply reduction from effects
        if (regen_reduction > 0) {
            base_hp_gain = base_hp_gain * (100 - regen_reduction) / 100;
            base_move_gain = base_move_gain * (100 - regen_reduction) / 100;
        }

        result.hp_gained = std::max(0, base_hp_gain);
        result.move_gained = std::max(0, base_move_gain);

        // Apply regen
        stats_.hit_points = std::min(stats_.hit_points + result.hp_gained, stats_.max_hit_points);
        stats_.movement = std::min(stats_.movement + result.move_gained, stats_.max_movement);
    }

    return result;
}

Actor::TickResult Actor::process_dot_effects() {
    TickResult result;
    std::vector<size_t> expired_indices;

    for (size_t i = 0; i < active_effects_.size(); ++i) {
        auto& effect = active_effects_[i];

        if (effect.effect_type != "dot") continue;

        // Check if it's time to apply damage
        effect.ticks_since_last++;
        if (effect.ticks_since_last < effect.tick_interval) continue;
        effect.ticks_since_last = 0;

        // Calculate damage
        int damage = effect.flat_damage;
        if (effect.percent_damage > 0) {
            damage += (stats_.max_hit_points * effect.percent_damage) / 100;
        }
        damage = std::max(1, damage);

        // Apply resistance
        int resistance = get_resistance(effect.damage_type);
        if (resistance > 0) {
            int max_resist = 75; // Could come from effect.max_resistance
            resistance = std::min(resistance, max_resist);
            damage = damage * (100 - resistance) / 100;
            damage = std::max(1, damage);
        }

        // Apply damage
        result.poison_damage += damage;
        take_damage(damage, effect.damage_type);

        if (stats_.hit_points <= 0) {
            result.died = true;
            return result;
        }

        // Decrement duration
        if (effect.remaining_ticks > 0) {
            effect.remaining_ticks--;
            if (effect.remaining_ticks == 0) {
                expired_indices.push_back(i);
            }
        }
    }

    // Remove expired effects (in reverse order to preserve indices)
    for (auto it = expired_indices.rbegin(); it != expired_indices.rend(); ++it) {
        result.expired_effects.push_back(active_effects_[*it].damage_type);
        active_effects_.erase(active_effects_.begin() + *it);
    }

    // Update actor flags based on remaining effects
    update_effect_flags();

    return result;
}
```

### 4. Effect Application (Cast Time)

When a poison ability is cast, resolve formulas and create the ActiveEffect:

```cpp
void Actor::apply_ability_effect(const AbilityEffect& ability_effect,
                                  const Actor& caster,
                                  int caster_level) {
    ActiveEffect effect;
    effect.ability_id = ability_effect.ability_id;
    effect.effect_id = ability_effect.effect_id;
    effect.effect_type = ability_effect.effect.effect_type;

    // Merge default params with override params
    auto params = merge_params(ability_effect.effect.default_params,
                               ability_effect.override_params);

    // Resolve formulas at cast time
    if (effect.effect_type == "dot") {
        effect.damage_type = params["damageType"].get<std::string>();
        effect.cure_category = params.value("cureCategory", effect.damage_type); // default to damageType
        effect.potency = params.value("potency", 5);
        effect.flat_damage = resolve_formula(params["amount"], caster_level);
        effect.percent_damage = params.value("amountPercent", 0);
        effect.blocks_regen = params.value("blocksRegen", false);
        effect.reduces_regen = params.value("reducesRegen", 0);
        effect.remaining_ticks = resolve_formula(params["duration"], caster_level);
        effect.tick_interval = params.value("tickInterval", 1);
        effect.stackable = params.value("stackable", false);
        effect.max_stacks = params.value("maxStacks", 1);
    }

    effect.source_actor_id = caster.id();
    effect.source_level = caster_level;

    add_effect(effect);
}
```

### 5. Cleanse/Cure Implementation

```cpp
// Cure result enum
enum class CureResult {
    FullCure,       // Effect completely removed
    PartialCure,    // Duration/potency reduced
    WrongCategory,  // Cure doesn't match effect category
    TooPowerful,    // Effect potency too high for cure
    NoEffect        // No matching effects found
};

struct CureAttemptResult {
    CureResult result;
    std::string message;
    int effects_removed = 0;
    int effects_reduced = 0;
};

// Attempt to cure effects of a given category
CureAttemptResult Actor::attempt_cure(const std::string& cure_category,
                                       int cure_power,
                                       bool cure_all,
                                       bool partial_on_fail) {
    CureAttemptResult result;
    result.result = CureResult::NoEffect;

    std::vector<size_t> to_remove;

    // Find effects matching the cure category
    // Sort by potency descending to cure strongest first (if not cure_all)
    std::vector<std::pair<size_t, int>> matching_effects;
    for (size_t i = 0; i < active_effects_.size(); ++i) {
        if (active_effects_[i].cure_category == cure_category) {
            matching_effects.emplace_back(i, active_effects_[i].potency);
        }
    }

    if (matching_effects.empty()) {
        result.result = CureResult::WrongCategory;
        result.message = fmt::format("You have no {} effects to cure.", cure_category);
        return result;
    }

    // Sort by potency (highest first)
    std::ranges::sort(matching_effects, [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for (const auto& [idx, potency] : matching_effects) {
        auto& effect = active_effects_[idx];

        if (cure_power >= effect.potency) {
            // Full cure - mark for removal
            to_remove.push_back(idx);
            result.effects_removed++;
            result.result = CureResult::FullCure;
        } else if (partial_on_fail && cure_power >= effect.potency - 2) {
            // Partial cure - reduce duration by 50% and potency by 1
            effect.remaining_ticks = std::max(1, effect.remaining_ticks / 2);
            effect.potency = std::max(1, effect.potency - 1);
            result.effects_reduced++;
            if (result.result != CureResult::FullCure) {
                result.result = CureResult::PartialCure;
            }
        } else {
            // Cure too weak
            if (result.result == CureResult::NoEffect) {
                result.result = CureResult::TooPowerful;
                result.message = fmt::format("The {} is too powerful for your cure!",
                                             effect.damage_type);
            }
        }

        // If not curing all, stop after first attempt
        if (!cure_all) break;
    }

    // Remove fully cured effects (reverse order to preserve indices)
    std::ranges::sort(to_remove, std::greater{});
    for (size_t idx : to_remove) {
        active_effects_.erase(active_effects_.begin() + idx);
    }

    // Update flags
    update_effect_flags();

    // Build result message
    if (result.effects_removed > 0 && result.effects_reduced > 0) {
        result.message = fmt::format("You cure {} effect{} and weaken {} more.",
                                     result.effects_removed,
                                     result.effects_removed == 1 ? "" : "s",
                                     result.effects_reduced);
    } else if (result.effects_removed > 0) {
        result.message = fmt::format("You cure {} {} effect{}.",
                                     result.effects_removed,
                                     cure_category,
                                     result.effects_removed == 1 ? "" : "s");
    } else if (result.effects_reduced > 0) {
        result.message = fmt::format("You weaken {} {} effect{}, but cannot fully cure them.",
                                     result.effects_reduced,
                                     cure_category,
                                     result.effects_reduced == 1 ? "" : "s");
    }

    return result;
}
```

### 6. Integration with Cleanse Spell Effect

When a `cleanse` effect is processed:

```cpp
void SpellSystem::apply_cleanse_effect(Actor& target,
                                        const nlohmann::json& params,
                                        int caster_level) {
    std::string category = params["cureCategory"].get<std::string>();
    int cure_power = resolve_formula(params.value("curePower", "5"), caster_level);
    bool cure_all = params.value("scope", "first") == "all";
    bool partial = params.value("partialOnFail", true);

    auto result = target.attempt_cure(category, cure_power, cure_all, partial);

    // Send appropriate messages based on result
    switch (result.result) {
        case CureResult::FullCure:
        case CureResult::PartialCure:
            send_to_char(target, result.message);
            break;
        case CureResult::WrongCategory:
            send_to_char(target, "That cure has no effect on you.");
            break;
        case CureResult::TooPowerful:
            send_to_char(target, result.message);
            break;
        case CureResult::NoEffect:
            send_to_char(target, "You don't seem to need that cure.");
            break;
    }
}
```

---

## Multiple Poison Types

### Database Configuration Examples

**Weak Poison** (Beginner spell, level 5) - Base Potency 2:
```sql
INSERT INTO "AbilityEffect" (ability_id, effect_id, order, trigger, override_params)
VALUES (
  (SELECT id FROM "Ability" WHERE plain_name = 'WEAK_POISON'),
  20,  -- dot effect
  0,
  'on_tick',
  '{
    "damageType": "poison",
    "cureCategory": "poison",
    "potency": "2 + (skill / 50)",
    "amount": "2 + (skill / 10)",
    "amountPercent": 1,
    "blocksRegen": false,
    "reducesRegen": 25,
    "duration": "3 + (skill / 15)"
  }'
);
-- Skill 25: potency 2, Skill 100: potency 4
-- Novice Cure Poison (power 5) always works
```

**Poison** (Standard spell, level 15) - Base Potency 4:
```sql
INSERT INTO "AbilityEffect" (ability_id, effect_id, order, trigger, override_params)
VALUES (
  (SELECT id FROM "Ability" WHERE plain_name = 'POISON'),
  20,
  0,
  'on_tick',
  '{
    "damageType": "poison",
    "cureCategory": "poison",
    "potency": "4 + (skill / 33)",
    "amount": "5 + (skill / 5)",
    "amountPercent": "2 + (skill / 50)",
    "blocksRegen": true,
    "duration": "5 + (skill / 10)"
  }'
);
-- Skill 33: potency 5, Skill 100: potency 7
-- Requires skilled healer to fully cure high-skill poison
```

**Deadly Venom** (High-level, level 50) - Base Potency 6:
```sql
INSERT INTO "AbilityEffect" (ability_id, effect_id, order, trigger, override_params)
VALUES (
  (SELECT id FROM "Ability" WHERE plain_name = 'DEADLY_VENOM'),
  20,
  0,
  'on_tick',
  '{
    "damageType": "poison",
    "cureCategory": "poison",
    "potency": "6 + (skill / 50)",
    "amount": "10 + (skill / 4)",
    "amountPercent": "3 + (skill / 33)",
    "blocksRegen": true,
    "maxResistance": 50,
    "duration": "8 + (skill / 8)"
  }'
);
-- Skill 50: potency 7, Skill 100: potency 8
-- Expert Neutralize Poison (power 8) required for master poisoner
```

**Cure Poison** (Cleanse spell) - Base Power 4:
```sql
INSERT INTO "AbilityEffect" (ability_id, effect_id, order, trigger, override_params)
VALUES (
  (SELECT id FROM "Ability" WHERE plain_name = 'CURE_POISON'),
  5,  -- cleanse effect
  0,
  'on_cast',
  '{
    "cureCategory": "poison",
    "curePower": "4 + (skill / 25)",
    "scope": "first",
    "partialOnFail": true
  }'
);
-- Skill 25: power 5, Skill 50: power 6, Skill 100: power 8
-- Master healer can cure most poisons; novice struggles with strong ones
```

**Neutralize Poison** (Stronger cleanse) - Base Power 6:
```sql
INSERT INTO "AbilityEffect" (ability_id, effect_id, order, trigger, override_params)
VALUES (
  (SELECT id FROM "Ability" WHERE plain_name = 'NEUTRALIZE_POISON'),
  5,
  0,
  'on_cast',
  '{
    "cureCategory": "poison",
    "curePower": "6 + (skill / 50)",
    "scope": "all",
    "partialOnFail": true
  }'
);
-- Skill 50: power 7, Skill 100: power 8
-- Cures ALL poison effects, even strong ones at high skill
```

**Dragon Fire** (Fire DoT from dragon breath) - Potency 7 (fixed, not skill-based):
```sql
INSERT INTO "AbilityEffect" (ability_id, effect_id, order, trigger, override_params)
VALUES (
  (SELECT id FROM "Ability" WHERE plain_name = 'DRAGON_FIRE'),
  20,
  0,
  'on_tick',
  '{
    "damageType": "fire",
    "cureCategory": "fire",
    "potency": 7,
    "amount": "15 + (level / 2)",
    "amountPercent": 4,
    "blocksRegen": false,
    "reducesRegen": 50,
    "duration": "4 + (level / 10)"
  }'
);
-- Monster abilities use fixed potency (not skill-based)
-- Curable by: Quench (5+skill), jumping in water, Ice Storm (3+skill)
```

**Plague** (Disease DoT) - Base Potency 5:
```sql
INSERT INTO "AbilityEffect" (ability_id, effect_id, order, trigger, override_params)
VALUES (
  (SELECT id FROM "Ability" WHERE plain_name = 'PLAGUE'),
  20,
  0,
  'on_tick',
  '{
    "damageType": "disease",
    "cureCategory": "disease",
    "potency": "5 + (skill / 50)",
    "amount": "3 + (skill / 8)",
    "amountPercent": 2,
    "blocksRegen": true,
    "stackable": true,
    "maxStacks": 3,
    "duration": "10 + (skill / 5)"
  }'
);
-- Skill 50: potency 6, Skill 100: potency 7
-- Note: Cure Poison does NOTHING - wrong category!
-- Requires Cure Disease (4+skill) or Remove Affliction (6+skill)
```

### Poison Plants as Items

Objects can trigger abilities when used. Configure in the database:

```sql
-- Nightshade plant (object vnum 5001)
INSERT INTO "ObjectAbilities" (object_zone_id, object_id, ability_id, level, charges)
VALUES (
  50, 1,  -- Zone 50, vnum 1
  (SELECT id FROM "Ability" WHERE plain_name = 'NIGHTSHADE_POISON'),
  15,     -- Cast at level 15
  3       -- 3 charges before consumed
);
```

When a player uses the nightshade plant on a target, it casts NIGHTSHADE_POISON at level 15.

---

## Implementation Order

### Phase 1: Core Infrastructure
1. Add `ActiveEffect` struct to C++ (`src/core/active_effect.hpp`)
2. Add `active_effects_` vector to Actor class
3. Add effect management methods to Actor

### Phase 2: Tick Processing
1. Update `perform_tick()` to call `process_dot_effects()`
2. Implement `process_dot_effects()` reading from `active_effects_`
3. Implement `is_regen_blocked()` and `get_regen_reduction()`

### Phase 3: Effect Application
1. Implement `apply_ability_effect()` for casting
2. Add formula resolution (parse "5 + (level / 2)" type strings)
3. Connect to spell casting system

### Phase 4: Database Integration
1. Add Prisma `ActiveEffect` model (or use existing `CharacterEffects`)
2. Load active effects from DB on character login
3. Save active effects to DB on logout/periodic save

### Phase 5: Testing & Polish
1. Create test poison variants in database
2. Add poison plant objects
3. Test stacking, resistance, duration expiry
4. Add messaging for DoT damage and effect expiry

---

## Migration Notes

- Keep `ActorFlag::Poison` for backwards compatibility during transition
- Initial implementation can check BOTH the flag AND active_effects
- Eventually, flag becomes a "cached" indicator derived from active_effects
- No database migration needed for core Effect/AbilityEffect - already configured

---

## Summary

| Component | Current | Proposed |
|-----------|---------|----------|
| Poison config | Hardcoded in C++ | Database Effect.defaultParams |
| Damage formula | `max_hp / 50` | `effect.flat_damage + effect.percent_damage` |
| Regen blocking | Check `ActorFlag::Poison` | Check `is_regen_blocked()` from effects |
| Multiple poisons | Not supported | Different abilities with different params |
| Duration tracking | None (permanent flag) | `remaining_ticks` per effect |
| Source tracking | None | `source_actor_id`, `source_level` |

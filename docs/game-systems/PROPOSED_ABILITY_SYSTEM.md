# Proposed Data-Driven Ability System

This document outlines a new, flexible, and data-driven system for handling all abilities in the game, including spells, skills, songs, and chants.

## 1. Overview & Goals

The primary goal of this system is to move away from hard-coded ability logic and towards a model where abilities are defined almost entirely by data. This empowers builders to create and modify abilities without requiring changes to the C++ engine.

The core principles are:
- **Data-Driven:** The behavior of an ability is defined in the database, not in the code.
- **Composable:** Complex abilities are built by combining simple, reusable "effects."
- **Flexible:** The system provides "escape hatches" (like Lua scripting) for unique mechanics without sacrificing performance or safety in common cases.
- **Clear & Readable:** The data structures are designed to be as human-readable and self-documenting as possible.

## 2. Core Concepts

The system is built on three main models: `Ability`, `Effect`, and `AbilityEffect`.

- **`Ability`**: This is the central container for any action a character can perform. It holds metadata like the name, type (spell, skill, etc.), casting time, and resource costs.
- **`Effect`**: This is a reusable, atomic game action, like "deal damage," "apply a buff," or "teleport." These are the fundamental "Lego bricks" of the system.
- **`AbilityEffect`**: This is a join table that links an `Ability` to one or more `Effect`s. It specifies the order in which effects are executed and allows for overriding the default parameters of an effect to create unique variations.

## 3. Schema Definition

The following Prisma schema will be implemented. It replaces the old `Spells` and related tables.

```prisma
// ============================================================================ 
// CORE ABILITY SYSTEM
// ============================================================================ 

model Ability {
  id                   Int      @id @default(autoincrement())
  name                 String   @unique
  description          String?
  gameId               String?  @unique @map("game_id") // e.g., "ability.fireball", "skill.backstab"
  abilityType          String   @default("SPELL") // Type: SPELL, SKILL, SONG, CHANT
  schoolId             Int?     @map("school_id")
  minPosition          Position @default(STANDING)
  violent              Boolean  @default(false)
  castTimeRounds       Int      @map("cast_time_rounds") @default(1)
  cooldownMs           Int      @map("cooldown_ms") @default(0)
  inCombatOnly         Boolean  @map("in_combat_only") @default(false)
  isArea               Boolean  @map("is_area") @default(false)
  notes                String?
  tags                 String[] @default([])
  luaScript            String?  @map("lua_script") @db.Text

  // --- Relationships ---
  effects              AbilityEffect[]
  school               AbilitySchool?       @relation(fields: [schoolId], references: [id])
  characterAbilities   CharacterAbilities[]
  mobAbilities         MobAbilities[]
  classAbilities       ClassAbilities[]
  components           AbilityComponent[]
  messages             AbilityMessages?
  restrictions         AbilityRestrictions?
  savingThrows         AbilitySavingThrow[]
  targeting            AbilityTargeting?

  createdAt            DateTime @map("created_at") @default(now())
  updatedAt            DateTime @map("updated_at") @updatedAt

  @@index([gameId])
}

model Effect {
  id            Int      @id @default(autoincrement())
  name          String   @unique
  description   String?
  effectType    String   // e.g., "damage", "heal", "apply_aura", "teleport"
  defaultParams Json     @map("default_params") @default("{}")
  abilities     AbilityEffect[]
}

model AbilityEffect {
  abilityId      Int      @map("ability_id")
  ability        Ability  @relation(fields: [abilityId], references: [id], onDelete: Cascade)
  effectId       Int      @map("effect_id")
  effect         Effect   @relation(fields: [effectId], references: [id], onDelete: Cascade)
  overrideParams Json?    @map("override_params")
  order          Int      @default(0)
  trigger        String?  // e.g., "on_cast", "on_hit", "on_tick"
  chancePct      Int      @map("chance_pct") @default(100)
  condition      String?  // Lua expression: "target.isUndead"

  @@id([abilityId, effectId])
}

// ============================================================================ 
// SUPPORTING MODELS (IMPROVED & RENAMED)
// ============================================================================ 

model AbilitySchool {
  id          Int       @id @default(autoincrement())
  name        String    @unique
  description String?
  abilities   Ability[]
}

model AbilityRestrictions {
  id                   Int     @id @default(autoincrement())
  abilityId            Int     @map("ability_id") @unique
  ability              Ability @relation(fields: [abilityId], references: [id], onDelete: Cascade)
  
  // Structured, performant requirements for 99% of cases.
  requirements         Json[]  @default([])
  
  // Optional "escape hatch" for complex logic.
  customRequirementLua String? @map("custom_requirement_lua")
}

model AbilitySavingThrow {
  id           Int      @id @default(autoincrement())
  abilityId    Int      @map("ability_id")
  ability      Ability  @relation(fields: [abilityId], references: [id], onDelete: Cascade)
  saveType     SaveType @default(SPELL)
  dcFormula    String   @map("dc_formula")
  onSaveAction Json     @map("on_save_action") @default("\"NEGATE\"")
}

model AbilityTargeting {
  id             Int          @id @default(autoincrement())
  abilityId      Int          @map("ability_id") @unique
  ability        Ability      @relation(fields: [abilityId], references: [id], onDelete: Cascade)
  validTargets   TargetType[] @map("valid_targets") @default([SELF])
  scope          TargetScope  @default(SINGLE)
  scopePattern   String?      @map("scope_pattern")
  maxTargets     Int          @map("max_targets") @default(1)
  range          Int          @default(0)
  requireLos     Boolean      @map("require_los") @default(false)
}

model AbilityMessages {
  id              Int     @id @default(autoincrement())
  abilityId       Int     @map("ability_id") @unique
  startToCaster   String? @map("start_to_caster")
  startToVictim   String? @map("start_to_victim")
  startToRoom     String? @map("start_to_room")
  successToCaster String? @map("success_to_caster")
  successToVictim String? @map("success_to_victim")
  successToRoom   String? @map("success_to_room")
  failToCaster    String? @map("fail_to_caster")
  failToVictim    String? @map("fail_to_victim")
  failToRoom      String? @map("fail_to_room")
  wearoffToTarget String? @map("wearoff_to_target")
  wearoffToRoom   String? @map("wearoff_to_room")
  ability         Ability @relation(fields: [abilityId], references: [id], onDelete: Cascade)
}

// Other related tables (ClassAbilities, CharacterAbilities, etc.) will also be renamed.
```

## 4. The Effect Runner (Engine Logic)

The C++ game engine will contain an "Effect Runner" system. When an ability is used, this system will:
1.  Fetch the `Ability` and its related data from the database.
2.  Verify all conditions in `AbilityRestrictions` (both the structured `requirements` and the `customRequirementLua`).
3.  Identify all valid targets based on the `AbilityTargeting` rules.
4.  For each target, iterate through the `AbilityEffect` records associated with the ability, sorted by the `order` field.
5.  For each `AbilityEffect`, it will check the `chancePct` and `condition`.
6.  It will then look up the `effectType` string in a central C++ **Effect Registry** (a map of strings to functions).
7.  Finally, it will execute the corresponding C++ function, passing the merged `defaultParams` and `overrideParams` to it.

## 5. JSON and Lua Usage

The power of this system comes from the intelligent use of structured JSON for common cases and powerful Lua scripts for complex ones.

### Structured Data (JSON)

JSON is the preferred method for defining requirements and outcomes, as it is safe, performant, and easy for builders to work with.

**Example: `AbilityRestrictions.requirements`**
- **Outdoors only:** `[{ "type": "location", "is": "outdoors" }]`
- **Requires target to be undead:** `[{ "type": "target_flag", "requires": ["UNDEAD"] }]`
- **Complex example:**
  ```json
  [
    { "type": "sector", "not_in": ["CITY", "STRUCTURE"] },
    { "type": "caster_stat", "stat": "mana", "gte": 100 },
    { "type": "time_of_day", "is": "night" }
  ]
  ```

**Example: `AbilitySavingThrow.onSaveAction`**
- **Negate the effect entirely:** `"NEGATE"` (Default)
- **Take half damage:** `"HALF_DAMAGE"`
- **Apply a weaker, shorter debuff:**
  ```json
  {
    "effectType": "apply_aura",
    "params": { "name": "Slowed", "duration": "1" }
  }
  ```

### Lua for Dynamic Logic

Lua is used when the logic is too complex to be represented by simple JSON structures. The C++ engine is responsible for providing the correct context (variables like `caster`, `target`, `ability`, `circle`) to the Lua environment before execution.

**Use Case 1: Simple Formulas**
For simple dynamic values, a one-line Lua script is sufficient.
- **`Effect.params.duration`**: `"caster.level / 2 + 10"`

**Use Case 2: Complex, Conditional Formulas**
For logic that depends on multiple runtime variables (like spell circle), a multi-line Lua script is used. This entire script is stored as a string in the database. This allows us to take complex C++ `switch` statements and make them fully data-driven.

- **Example: `sorcerer_single_target` damage calculation stored in an `Effect`'s `defaultParams`:**
  ```json
  {
    "damageType": "varies",
    "amount": "
      -- This entire script is the 'amount'.
      -- The engine provides 'caster', 'target', 'ability', and 'circle' to Lua.

      local minlevel = get_min_level_for_ability(caster.class, ability.id) or 1
      local power = caster:get_proficiency(ability.id)
      local exponent = 1.2 + 0.3 * minlevel / 100.0 + (power - minlevel) * (0.004 * minlevel - 0.2) / 100.0

      local base_damage = 0
      if circle == 1 then
        base_damage = dice(4, 19)
      elseif circle == 2 then
        base_damage = dice(5, 16)
      elseif circle == 3 then
        base_damage = dice(4, 24)
      else -- etc...
        base_damage = dice(10, 35)
      end

      return base_damage + (power ^ exponent)
    "
  }
  ```

**Use Case 3: The "Escape Hatch"**
- **`AbilityRestrictions.customRequirementLua`**: For exceptionally complex targeting or state conditions that the structured JSON can't handle.
- **`effectType: "script"`**: For completely unique ability mechanics that don't fit any other predefined `effectType`.

This hybrid approach provides safety and performance for the majority of cases while still allowing for near-infinite flexibility when required.

# Spell Effect Params Schema

**Version**: 1.0
**Last Updated**: 2025-11-08

## Overview

The `params` field in `SpellEffects` is a JSON/JSONB column that stores all game mechanics data in a structured, queryable format. This document defines the schemas for different effect types.

## Design Philosophy

**Declarative Data > Imperative Code**

- **Lua formulas** contain only math expressions (damage calculations, duration formulas)
- **Params** contain all game mechanics (restrictions, bonuses, scaling, conditions)
- Game engine interprets params to apply mechanics
- Enables visual editing in Muditor
- Queryable via PostgreSQL JSON operators

---

## Common Base Schema

All params objects share these optional base fields:

```typescript
interface BaseParams {
  // Effect-specific notes/description
  notes?: string;

  // Maximum value cap (for damage, healing, etc.)
  maxValue?: number;

  // Minimum value floor
  minValue?: number;
}
```

---

## Damage Effect Params

### Alignment Damage

**Used by**: Dispel Evil, Dispel Good, Divine Bolt, Stygian Eruption

```typescript
interface AlignmentDamageParams extends BaseParams {
  damageType: 'ALIGN';

  // Maximum damage cap (e.g., 135 for Dispel Evil)
  maxDamage?: number;

  // Alignment-based restrictions (spell fails if not met)
  restrictions?: {
    // Spell only damages evil targets (fails vs good/neutral)
    requireEvil?: boolean;

    // Spell only damages good targets (fails vs evil/neutral)
    requireGood?: boolean;

    // Spell fails against neutral targets
    failOnNeutral?: boolean;

    // Custom alignment range
    minAlignment?: number;  // e.g., -1000 to 1000
    maxAlignment?: number;
  };

  // Caster class/alignment bonuses
  alignmentBonuses?: AlignmentBonus[];

  // Victim alignment scaling (damage varies by victim alignment)
  victimScaling?: {
    // Lua formula using victim.alignment
    // Example: "target.alignment * -0.0007 + 0.8"
    formula: string;

    // Whether more evil = more damage (true) or more good = more damage (false)
    penalizesEvil?: boolean;
  };

  // Caster alignment scaling (damage varies by caster alignment)
  casterScaling?: {
    // Lua formula using caster.alignment
    // Example: "caster.alignment * -0.0022 - 0.7"
    formula: string;
  };
}

interface AlignmentBonus {
  // Class requirement (null = any class)
  class?: 'PRIEST' | 'DIABOLIST' | 'CLERIC' | 'PALADIN' | string;

  // Minimum alignment threshold
  minAlignment?: number;  // e.g., 750 for very good

  // Maximum alignment threshold
  maxAlignment?: number;  // e.g., -750 for very evil

  // Damage multiplier (e.g., 1.25 = +25% damage)
  multiplier: number;
}
```

**Example - Dispel Evil**:
```json
{
  "damageType": "ALIGN",
  "maxDamage": 135,
  "restrictions": {
    "requireEvil": true,
    "failOnNeutral": true
  },
  "alignmentBonuses": [
    {
      "class": "PRIEST",
      "minAlignment": 750,
      "multiplier": 1.25
    },
    {
      "class": "PRIEST",
      "minAlignment": 0,
      "maxAlignment": 749,
      "multiplier": 1.15
    },
    {
      "minAlignment": 750,
      "multiplier": 1.10
    }
  ]
}
```

**Example - Divine Bolt**:
```json
{
  "damageType": "ALIGN",
  "maxDamage": 110,
  "victimScaling": {
    "formula": "target.alignment * -0.0007 + 0.8",
    "penalizesEvil": true
  }
}
```

**Example - Stygian Eruption**:
```json
{
  "damageType": "ALIGN",
  "casterScaling": {
    "formula": "caster.alignment * -0.0022 - 0.7"
  }
}
```

---

### Elemental Damage

**Used by**: Fireball, Lightning Bolt, Cone of Cold, Acid Arrow, etc.

```typescript
interface ElementalDamageParams extends BaseParams {
  damageType: 'FIRE' | 'COLD' | 'ACID' | 'SHOCK' | 'POISON';

  // Maximum damage cap
  maxDamage?: number;

  // Area of effect properties
  aoe?: {
    // Affects all enemies in room
    roomWide?: boolean;

    // Damage falloff at range
    falloff?: {
      // Damage at epicenter = 100%
      // Damage at max range = this value
      edgeMultiplier: number;  // e.g., 0.5 = 50% at edges
    };
  };

  // Damage over time (burning, poison, etc.)
  dot?: {
    // Damage per tick
    tickDamage: string;  // Lua formula

    // Ticks per round
    tickRate: number;

    // Total duration in rounds
    duration: string;  // Lua formula
  };

  // Racial/composition modifiers
  racialModifiers?: {
    // Races that take extra damage
    vulnerable?: string[];  // e.g., ["TROLL"] for fire

    // Races that take reduced damage
    resistant?: string[];  // e.g., ["FIRE_GIANT"] for fire

    // Races immune to this damage
    immune?: string[];
  };
}
```

**Example - Fireball**:
```json
{
  "damageType": "FIRE",
  "maxDamage": 200,
  "aoe": {
    "roomWide": true
  },
  "racialModifiers": {
    "vulnerable": ["TROLL"],
    "resistant": ["FIRE_GIANT", "RED_DRAGON"],
    "immune": ["FIRE_ELEMENTAL"]
  }
}
```

---

### Physical Damage

**Used by**: Magic Missile, Shocking Grasp, melee attacks

```typescript
interface PhysicalDamageParams extends BaseParams {
  damageType: 'HIT' | 'SLASH' | 'PIERCE' | 'BLUDGEON' | 'CRUSH' |
              'POUND' | 'CLAW' | 'BITE' | 'STING' | 'WHIP' |
              'MAUL' | 'THRASH' | 'BLAST' | 'PUNCH' | 'STAB';

  // Maximum damage cap
  maxDamage?: number;

  // Armor penetration
  armorPenetration?: {
    // Percentage of armor ignored (0-100)
    percentage?: number;

    // Flat armor reduction
    flat?: number;
  };

  // Critical hit mechanics
  critical?: {
    // Base crit chance percentage
    baseChance: number;

    // Crit damage multiplier
    multiplier: number;  // e.g., 2.0 = double damage
  };
}
```

---

## Healing Effect Params

```typescript
interface HealingParams extends BaseParams {
  // Healing type
  healingType: 'HP' | 'MANA' | 'STAMINA' | 'ALL';

  // Maximum healing cap
  maxHealing?: number;

  // Healing over time
  hot?: {
    // Healing per tick
    tickHealing: string;  // Lua formula

    // Ticks per round
    tickRate: number;

    // Total duration in rounds
    duration: string;  // Lua formula
  };

  // Overheal mechanics
  overheal?: {
    // Can heal above max HP
    allowed: boolean;

    // Maximum overheal percentage
    maxPercentage?: number;  // e.g., 10 = can heal to 110% max HP

    // Overheal decay rate per round
    decayRate?: number;
  };

  // Class/alignment bonuses
  healingBonuses?: {
    // Class requirement
    class?: string;

    // Alignment requirement
    minAlignment?: number;
    maxAlignment?: number;

    // Healing multiplier
    multiplier: number;
  }[];
}
```

**Example - Cure Light Wounds**:
```json
{
  "healingType": "HP",
  "maxHealing": 50,
  "healingBonuses": [
    {
      "class": "CLERIC",
      "multiplier": 1.15
    }
  ]
}
```

---

## Apply Aura Effect Params

```typescript
interface ApplyAuraParams extends BaseParams {
  // Reference to Aura by gameId
  auraId: string;  // e.g., "aura.bless"

  // Duration override (if not using durationFormula)
  duration?: number;

  // Stacking behavior override
  stackingOverride?: 'REFRESH' | 'STACK' | 'IGNORE' | 'MAX_ONLY';

  // Dispel mechanics
  dispel?: {
    // Dispel category (for dispel magic)
    category: 'magic' | 'disease' | 'poison' | 'curse';

    // Dispel resistance
    resistance?: number;  // Percentage chance to resist dispel
  };

  // Conditional application
  conditions?: {
    // Requires specific target state
    requiresState?: string[];  // e.g., ["STANDING", "FIGHTING"]

    // Requires target race/class
    requiresRace?: string[];
    requiresClass?: string[];

    // Alignment restrictions
    minAlignment?: number;
    maxAlignment?: number;
  };
}
```

**Example - Bless**:
```json
{
  "auraId": "aura.bless",
  "dispel": {
    "category": "magic",
    "resistance": 0
  }
}
```

---

## Remove Aura Effect Params

```typescript
interface RemoveAuraParams extends BaseParams {
  // Removal type
  removeType: 'SPECIFIC' | 'CATEGORY' | 'ALL_NEGATIVE' | 'ALL_POSITIVE';

  // Specific aura IDs to remove
  auraIds?: string[];  // e.g., ["aura.bless", "aura.curse"]

  // Remove by dispel category
  category?: 'magic' | 'disease' | 'poison' | 'curse';

  // Maximum number of auras to remove (null = all matching)
  maxRemove?: number;

  // Removal chance
  chance?: {
    // Base chance percentage
    baseChance: number;

    // Bonus per skill level
    skillBonus?: number;
  };
}
```

**Example - Cure Disease**:
```json
{
  "removeType": "CATEGORY",
  "category": "disease"
}
```

**Example - Dispel Magic**:
```json
{
  "removeType": "CATEGORY",
  "category": "magic",
  "maxRemove": 3,
  "chance": {
    "baseChance": 50,
    "skillBonus": 0.5
  }
}
```

---

## Summon Effect Params

```typescript
interface SummonParams extends BaseParams {
  // Mob to summon (by gameId or vnum)
  mobId?: string;  // e.g., "mob.fire_elemental"

  // Zone and vnum (legacy format)
  zoneId?: number;
  vnum?: number;

  // Number of summons
  quantity?: {
    // Base quantity
    base: number;

    // Bonus per skill level
    skillBonus?: number;  // e.g., 0.1 = +1 per 10 skill
  };

  // Summon duration
  duration?: string;  // Lua formula

  // Summon behavior
  behavior?: {
    // Follows caster
    follows: boolean;

    // Assists caster in combat
    assists: boolean;

    // Obeys commands
    obeyable: boolean;
  };

  // Power scaling
  scaling?: {
    // HP multiplier based on caster level
    hpScale?: string;  // Lua formula

    // Damage multiplier
    damageScale?: string;  // Lua formula
  };
}
```

---

## Teleport Effect Params

```typescript
interface TeleportParams extends BaseParams {
  // Teleport type
  teleportType: 'RANDOM' | 'MEMORIZED' | 'RECALL' | 'SPECIFIC';

  // Specific destination
  destination?: {
    zoneId: number;
    roomId: number;
  };

  // Restrictions
  restrictions?: {
    // Can't teleport from no-recall rooms
    noRecallBlocks?: boolean;

    // Can't teleport into no-teleport rooms
    respectNoTeleport?: boolean;

    // Can't teleport in combat
    noCombat?: boolean;
  };

  // Range limits (for random teleport)
  range?: {
    // Zone restrictions
    sameZone?: boolean;
    allowedZones?: number[];

    // Maximum distance in rooms
    maxDistance?: number;
  };
}
```

---

## Usage in C++ Engine

The game engine should parse params and apply mechanics accordingly:

```cpp
// Example pseudocode for damage calculation
int calculateSpellDamage(SpellEffect* effect, Character* caster, Character* victim, int skill) {
    // 1. Calculate base damage from Lua formula
    int baseDamage = luaEval(effect->luaFormula, caster, victim, skill);

    // 2. Parse params based on damage type
    auto params = parseParams(effect->params);

    if (params.damageType == "ALIGN") {
        auto alignParams = parseAlignmentParams(params);

        // Check restrictions
        if (alignParams.restrictions.requireEvil && victim->alignment >= 0) {
            return 0;  // Spell fails vs non-evil
        }

        // Apply caster bonuses
        float multiplier = 1.0;
        for (auto& bonus : alignParams.alignmentBonuses) {
            if (matchesBonus(caster, bonus)) {
                multiplier = bonus.multiplier;
                break;
            }
        }

        // Apply victim scaling
        if (alignParams.victimScaling) {
            multiplier *= evalVictimScaling(alignParams.victimScaling, victim);
        }

        baseDamage *= multiplier;

        // Apply max damage cap
        if (alignParams.maxDamage) {
            baseDamage = min(baseDamage, alignParams.maxDamage);
        }
    }

    return baseDamage;
}
```

---

## Muditor Integration

Muditor can provide type-safe editors for each params type:

```tsx
// Example React component
function AlignmentDamageEditor({ value, onChange }: Props) {
  const params = value as AlignmentDamageParams;

  return (
    <div>
      <NumberInput
        label="Max Damage"
        value={params.maxDamage}
        onChange={(v) => onChange({ ...params, maxDamage: v })}
      />

      <Checkbox
        label="Requires Evil Target"
        checked={params.restrictions?.requireEvil}
        onChange={(v) => onChange({
          ...params,
          restrictions: { ...params.restrictions, requireEvil: v }
        })}
      />

      <AlignmentBonusEditor
        bonuses={params.alignmentBonuses}
        onChange={(bonuses) => onChange({ ...params, alignmentBonuses: bonuses })}
      />
    </div>
  );
}
```

---

## TypeScript Type Definitions

Complete TypeScript definitions should be placed in `packages/types/src/spell-params.ts`:

```typescript
export type SpellEffectParams =
  | AlignmentDamageParams
  | ElementalDamageParams
  | PhysicalDamageParams
  | HealingParams
  | ApplyAuraParams
  | RemoveAuraParams
  | SummonParams
  | TeleportParams;

export function getParamsSchema(effectType: EffectType, damageType?: string): JSONSchema {
  // Return appropriate JSON schema for validation
}
```

---

## Migration Notes

When updating from old format to new:

**Old format**:
```json
{
  "school": "align"
}
```

**New format**:
```json
{
  "damageType": "ALIGN",
  "maxDamage": 135,
  "restrictions": {
    "requireEvil": true
  },
  "alignmentBonuses": [...]
}
```

Migration script should:
1. Parse `all_spell_implementations.json` for rich data
2. Extract bonus strings and parse into structured bonuses
3. Extract requirement strings and parse into restrictions
4. Store in new params format

# Element Types System Design

This document defines the element/damage type system used across FieryMUD, Muditor, and FieryLib for combat, abilities, resistances, and environmental effects.

## Overview

The element type system provides a unified framework for:
- Damage classification and calculation
- Resistance, immunity, and vulnerability tracking
- Ability/spell multi-component damage
- Environmental interactions and synergies
- Race, class, and creature type definitions

## Element Types (18 Total)

### Physical Types
| Element | Description | Use Cases |
|---------|-------------|-----------|
| `PHYSICAL` | Weapon damage (slash, pierce, bludgeon) | Melee/ranged attacks, blocked by armor |
| `FORCE` | Magical kinetic energy | Spell concussion, hits ethereal creatures |
| `SONIC` | Sound/thunder damage | Ignores physical armor, can deafen |
| `BLEED` | Physical damage over time | Wounds, bypasses some armor, CON resist |

### Classical Elements
| Element | Opposite | Description |
|---------|----------|-------------|
| `FIRE` | `COLD` | Heat, burning, melts ice |
| `COLD` | `FIRE` | Freezing, slowing, shatters |
| `WATER` | — | Drowning, pressure, conducts electricity |
| `EARTH` | `AIR` | Crushing, grounding, tremors |
| `AIR` | `EARTH` | Suffocation, cutting winds, lifts |
| `SHOCK` | — | Electricity, chains in water, grounded by earth |

### Chemical Types
| Element | Description | Use Cases |
|---------|-------------|-----------|
| `ACID` | Corrosion damage | DoT, destroys items, ignores some armor |
| `POISON` | Toxin damage | DoT, CON-based resistance, undead immune |

### Light/Dark
| Element | Opposite | Description |
|---------|----------|-------------|
| `RADIANT` | `SHADOW` | Pure light energy, blinds, dispels darkness |
| `SHADOW` | `RADIANT` | Darkness/void, stronger at night |

### Divine
| Element | Opposite | Description |
|---------|----------|-------------|
| `HOLY` | `UNHOLY` | Divine positive energy, harms evil/undead |
| `UNHOLY` | `HOLY` | Divine negative energy, harms good/celestials |

### Life/Death
| Element | Opposite | Description |
|---------|----------|-------------|
| `HEAL` | `NECROTIC` | Restoration energy, inverts on undead |
| `NECROTIC` | `HEAL` | Life drain, withering, decay |

### Other
| Element | Description | Use Cases |
|---------|-------------|-----------|
| `MENTAL` | Psychic/psionic damage | Ignores physical armor, mindless immune |
| `NATURE` | Druidic/primal energy | Thorns, vines, heals outdoors |

## Resistance System

### Single Scale Model

Resistances use a single percentage scale from -100% to 200%:

```
-100% ─────── 0% ─────── 100% ─────── 200%
  │           │           │            │
 2x dmg    normal      immune       absorbs
(vulnerable)                        (heals)
```

| Value | Effect | Example |
|-------|--------|---------|
| -100% | Takes 2x damage (maximum vulnerability) | Ice elemental vs FIRE |
| -50% | Takes 1.5x damage | Undead vs RADIANT |
| 0% | Normal damage | Standard creature |
| 50% | Takes 0.5x damage | Fire resistant armor |
| 75% | Takes 0.25x damage | Red dragon vs FIRE |
| 100% | Immune (no damage, unpenetratable) | Fire elemental vs FIRE |
| 150% | Absorbs, heals for 50% of damage | Special items only |

### Resistance Rules

1. **Immunity Threshold**: 100%+ resistance = immune, penetration has no effect
2. **Absorption Cap**: Values >100% only allowed via explicit item/spell effects
3. **Stacking Cap**: Multiplicative stacking caps at 100% (cannot stack to immunity naturally)

### Multiplicative Stacking

Resistances from multiple sources stack multiplicatively, not additively:

```
Formula: effective = 1 - ((1 - race) × (1 - class) × (1 - gear) × (1 - buffs))

Example: Three 25% fire resistance sources
├── Additive:       25 + 25 + 25 = 75% (too easy to reach immunity)
├── Multiplicative: 1 - (0.75 × 0.75 × 0.75) = 57.8%
└── Result: Much harder to reach immunity through stacking
```

### Opposition Mechanics

When a creature has resistance to an element, they gain vulnerability to its opposite:

```
Formula: vulnerability_to_opposite = -(source_resistance / 2)

Example: Fire Elemental
├── FIRE resistance: 100% (immune)
├── Implied COLD resistance: -50% (vulnerable, takes 1.5x damage)
└── Can be overridden by explicit COLD resistance value
```

**Opposed Pairs:**
- `FIRE` ↔ `COLD`
- `RADIANT` ↔ `SHADOW`
- `HOLY` ↔ `UNHOLY`
- `HEAL` ↔ `NECROTIC`
- `EARTH` ↔ `AIR`

## Multi-Component Damage

Abilities can deal multiple damage types simultaneously, allowing partial resistance bypass:

### Example Spell Compositions

```
Magic Missile
└── 100% FORCE

Fireball
├── 70% FIRE (main elemental damage)
└── 30% FORCE (concussive blast)

Lightning Bolt
├── 85% SHOCK (electrical)
└── 15% SONIC (thunder crack)

Cone of Cold
├── 90% COLD (freezing)
└── 10% FORCE (wind pressure)

Flame Strike (divine)
├── 50% FIRE
└── 50% HOLY (bypasses fire-immune demons)

Sunburst
├── 50% RADIANT
└── 50% FIRE

Psychic Scream
├── 70% MENTAL
└── 30% SONIC

Heal (on undead)
└── 100% HEAL (inverts to damage)

Shadow Bolt
└── 100% SHADOW

Word of Power
├── 60% SONIC
└── 40% HOLY
```

### Damage Calculation

For multi-component damage against a target:

```
total_damage = Σ (component_damage × (1 - effective_resistance[component_element]))

Example: Fireball (100 base damage) vs Fire-immune creature
├── FIRE component:  70 × (1 - 1.0) = 0 damage
├── FORCE component: 30 × (1 - 0.0) = 30 damage
└── Total: 30 damage (creature still takes concussive blast)
```

## Environmental Interactions

### Synergies (Bonus Effects)

| Condition | Effect |
|-----------|--------|
| `WET` target + `SHOCK` damage | +50% damage, chains to adjacent wet targets |
| `UNDERWATER` room + `SHOCK` damage | +50% damage, hits all creatures in water |
| `FIRE` + `NATURE` target | +25% damage (burns vegetation) |
| `AIR` + `FIRE` | +25% damage (fans flames) |
| `AIR` + `POISON` | Wider AoE (spreads gas) |
| `WATER` + `COLD` | Creates ice formations |

### Counters (Reduced Effects)

| Condition | Effect |
|-----------|--------|
| `UNDERWATER` room + `FIRE` damage | -50% damage |
| `EARTH` grounding + `SHOCK` damage | -50% damage |
| `WATER` + `FIRE` | Extinguishes, creates steam (obscures vision) |
| `RADIANT` + `SHADOW` | Cancel out, reduced damage both |
| `EARTH` + `AIR` | Creates dust cloud (obscures vision) |

## Data Model

### Prisma Schema

```prisma
enum ElementType {
  // Physical
  PHYSICAL
  FORCE
  SONIC
  BLEED

  // Classical
  FIRE
  COLD
  WATER
  EARTH
  AIR
  SHOCK

  // Chemical
  ACID
  POISON

  // Light/Dark
  RADIANT
  SHADOW

  // Divine
  HOLY
  UNHOLY

  // Life/Death
  HEAL
  NECROTIC

  // Other
  MENTAL
  NATURE
}
```

### Resistance Storage

**Option A: JSON field (simpler)**
```prisma
model Race {
  id          Int     @id @default(autoincrement())
  name        String
  resistances Json?   // { "FIRE": 25, "COLD": -25, "HOLY": 100 }
}

model CharacterClass {
  id          Int     @id @default(autoincrement())
  name        String
  resistances Json?   // { "MENTAL": 25 }
}
```

**Option B: Junction table (more queryable)**
```prisma
model RaceResistance {
  raceId      Int
  element     ElementType
  value       Int           // -100 to 100 (200 only via items)

  race        Race @relation(fields: [raceId], references: [id])

  @@id([raceId, element])
}
```

### Ability Effect Components

```prisma
model AbilityDamageComponent {
  id              Int         @id @default(autoincrement())
  abilityId       Int
  element         ElementType
  damageFormula   String      // "8d6", "2d6+level", etc.
  percentage      Int         // % of total damage (must sum to 100)
  sequence        Int         @default(0)

  ability         Ability     @relation(fields: [abilityId], references: [id])

  @@index([abilityId])
}
```

### Item Resistance Modifiers

```prisma
model ObjectResistance {
  id              Int         @id @default(autoincrement())
  objectZoneId    Int
  objectVnum      Int
  element         ElementType
  value           Int         // Can be >100 for absorption items
  allowAbsorption Boolean     @default(false)

  object          Objects     @relation(fields: [objectZoneId, objectVnum], references: [zoneId, vnum])

  @@index([objectZoneId, objectVnum])
}
```

## Usage Examples

### Creature Definitions

```yaml
Fire Elemental:
  lifeForce: ELEMENTAL
  composition: FIRE
  resistances:
    FIRE: 100      # Immune
    COLD: -50      # Vulnerable (or auto-calculated from opposition)
    WATER: -25     # Weak to water
    PHYSICAL: 50   # Partially resistant (incorporeal)

Vampire:
  lifeForce: UNDEAD
  resistances:
    NECROTIC: 50   # Resistant to death magic
    HEAL: -100     # Healing damages (2x)
    RADIANT: -50   # Vulnerable to light
    HOLY: -50      # Vulnerable to divine
    FIRE: -25      # Weak to fire

Stone Golem:
  lifeForce: MAGIC
  composition: STONE
  resistances:
    PHYSICAL: 50   # Hard to damage physically
    FIRE: 25       # Stone resists heat
    COLD: 25       # Stone resists cold
    SHOCK: -25     # Conducts poorly but cracks
    ACID: -50      # Corrodes stone
    MENTAL: 100    # Mindless, immune
    POISON: 100    # No biology, immune
```

### Combat Example

```
Scenario: Mage casts Fireball at Fire Elemental

Fireball composition:
├── FIRE:  70% of 50 damage = 35 FIRE damage
└── FORCE: 30% of 50 damage = 15 FORCE damage

Fire Elemental resistances:
├── FIRE:  100% (immune)
└── FORCE: 0% (normal)

Damage calculation:
├── FIRE:  35 × (1 - 1.0) = 0 damage
├── FORCE: 15 × (1 - 0.0) = 15 damage
└── Total: 15 damage

Result: Fire Elemental takes 15 force damage from the concussive blast,
        but is unharmed by the flames.
```

## Integration Points

### FieryMUD (C++)
- Combat system uses ElementType for damage calculation
- Creature templates define resistances
- Spell definitions include damage components
- Room sector types affect elemental damage

### Muditor (TypeScript)
- Visual resistance editor for races/classes/mobs
- Spell component editor with percentage allocation
- Resistance preview showing effective values
- Environmental modifier configuration

### FieryLib (Python)
- Imports legacy damage types to new ElementType system
- Converts old resistance flags to percentage values
- Validates resistance consistency during import

## Migration Notes

### From Old DamageType System

The legacy `DamageType` enum (HIT, SLASH, PIERCE, etc.) maps to the new system:

| Old DamageType | New ElementType | Notes |
|----------------|-----------------|-------|
| HIT, STING, WHIP, SLASH, BITE, BLUDGEON, CRUSH, POUND, CLAW, MAUL, THRASH, PIERCE, PUNCH, STAB | PHYSICAL | Physical weapon damage consolidated |
| FIRE | FIRE | Direct mapping |
| COLD | COLD | Direct mapping |
| ACID | ACID | Direct mapping |
| SHOCK | SHOCK | Direct mapping |
| POISON | POISON | Direct mapping |
| BLAST, ENERGY | FORCE | Magical kinetic |
| ALIGN | HOLY or UNHOLY | Based on alignment context |
| MENTAL | MENTAL | Direct mapping |
| ROT | NECROTIC | Decay/death damage |
| WATER | WATER | Direct mapping |

### New Elements Without Legacy Equivalent
- `SONIC` - New, no legacy mapping
- `BLEED` - New, no legacy mapping
- `RADIANT` - New, no legacy mapping
- `SHADOW` - New, no legacy mapping
- `EARTH` - New, no legacy mapping
- `AIR` - New, no legacy mapping
- `NATURE` - New, no legacy mapping
- `HEAL` - Existed as effect, now explicit element

## Future Considerations

1. **Elemental Affinity**: Bonus damage dealt with specific elements (Fire mage +25% FIRE damage)
2. **Penetration**: Ability to reduce target's resistance before calculation
3. **Temporary Resistances**: Buff spells that grant resistance
4. **Resistance Auras**: Room/zone effects that modify resistances
5. **Creature Type Defaults**: Standard resistance packages for LifeForce types

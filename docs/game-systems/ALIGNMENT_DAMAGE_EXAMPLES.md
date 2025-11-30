# Alignment Damage Implementation Examples

**Last Updated**: 2025-11-08

This document shows real examples of alignment-based damage spells from the database with enhanced params.

---

## Example 1: Dispel Evil (Binary Targeting)

**Spell**: `spell.dispel_evil`
**Pattern**: Only damages evil targets, fails vs good/neutral

### Database Data

```sql
SELECT lua_formula, params FROM "SpellEffects"
WHERE spell_id = (SELECT id FROM "Spells" WHERE game_id = 'spell.dispel_evil');
```

**Lua Formula** (base damage only):
```lua
dam + ((skill^2) * 7) / 1000
```

**Params** (structured mechanics):
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
      "multiplier": 1.25,
      "minAlignment": 750
    },
    {
      "class": "PRIEST",
      "multiplier": 1.15,
      "maxAlignment": 749,
      "minAlignment": 0
    },
    {
      "multiplier": 1.1,
      "minAlignment": 750
    }
  ]
}
```

> **Note**: Legacy reference data (like `source: "magic.cpp:670-693"`) is kept in documentation but NOT in the database. The game engine only needs actual mechanics.

### C++ Implementation Logic

```cpp
int calculateDispelEvil(Character* caster, Character* victim, int skill) {
    auto params = parseAlignmentParams(effect->params);

    // 1. Check restrictions
    if (params.restrictions.requireEvil && victim->alignment >= 0) {
        sendToChar(caster, "Your spell has no effect on %s!\n", victim->getName());
        return 0;  // Spell fails vs non-evil
    }

    // 2. Calculate base damage from Lua
    int baseDamage = luaEval("dam + ((skill^2) * 7) / 1000", caster, victim, skill);

    // 3. Apply caster alignment bonuses
    float multiplier = 1.0;
    for (auto& bonus : params.alignmentBonuses) {
        // Check class match
        if (bonus.class && caster->getClass() != bonus.class) {
            continue;
        }

        // Check alignment thresholds
        if (bonus.minAlignment && caster->alignment < bonus.minAlignment) {
            continue;
        }
        if (bonus.maxAlignment && caster->alignment > bonus.maxAlignment) {
            continue;
        }

        // Match found!
        multiplier = bonus.multiplier;
        break;
    }

    baseDamage *= multiplier;

    // 4. Apply max damage cap
    if (params.maxDamage) {
        baseDamage = std::min(baseDamage, params.maxDamage);
    }

    return baseDamage;
}
```

### Example Calculations

**Scenario 1**: PRIEST with 800 alignment, skill 95, vs evil victim (-500 alignment)
```
Base damage = dam + ((95^2) * 7) / 1000 = 10 + 63.175 = 73
Multiplier = 1.25 (PRIEST with align >= 750)
Final damage = 73 * 1.25 = 91 damage
```

**Scenario 2**: PRIEST with 400 alignment, skill 95, vs evil victim
```
Base damage = 73
Multiplier = 1.15 (PRIEST with align 0-749)
Final damage = 73 * 1.15 = 84 damage
```

**Scenario 3**: WARRIOR with 800 alignment, skill 95, vs evil victim
```
Base damage = 73
Multiplier = 1.1 (non-priest with align >= 750)
Final damage = 73 * 1.1 = 80 damage
```

**Scenario 4**: Any class, any alignment, vs good victim (200 alignment)
```
Spell fails immediately (requireEvil restriction)
Damage = 0
```

---

## Example 2: Divine Bolt (Victim Scaling)

**Spell**: `spell.divine_bolt`
**Pattern**: Damage varies smoothly by victim alignment (more vs evil)

### Database Data

**Lua Formula**:
```lua
dam + ((skill^2) * 53) / 10000
```

**Params**:
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

### C++ Implementation Logic

```cpp
int calculateDivineBolt(Character* caster, Character* victim, int skill) {
    auto params = parseAlignmentParams(effect->params);

    // 1. Calculate base damage
    int baseDamage = luaEval("dam + ((skill^2) * 53) / 10000", caster, victim, skill);

    // 2. Apply victim alignment scaling
    if (params.victimScaling) {
        // Evaluate scaling formula with victim's alignment
        float scalingMultiplier = luaEval(
            params.victimScaling.formula,
            caster,
            victim,
            skill
        );

        baseDamage *= scalingMultiplier;
    }

    // 3. Apply max damage cap
    if (params.maxDamage) {
        baseDamage = std::min(baseDamage, params.maxDamage);
    }

    return baseDamage;
}
```

### Example Calculations

**Base damage** (skill 100):
```
dam + ((100^2) * 53) / 10000 = 10 + 53 = 63
```

**Victim alignment: -1000 (very evil)**:
```
Scaling = -1000 * -0.0007 + 0.8 = 0.7 + 0.8 = 1.5
Damage = 63 * 1.5 = 94 damage ⚡ (High damage vs evil)
```

**Victim alignment: 0 (neutral)**:
```
Scaling = 0 * -0.0007 + 0.8 = 0.8
Damage = 63 * 0.8 = 50 damage
```

**Victim alignment: +1000 (very good)**:
```
Scaling = 1000 * -0.0007 + 0.8 = -0.7 + 0.8 = 0.1
Damage = 63 * 0.1 = 6 damage (Very low vs good)
```

**Damage by Alignment Graph**:
```
Victim      Scaling    Damage
-1000 (evil)   1.5      94 ████████████████
-750           1.325    83 ██████████████
-500           1.15     72 ████████████
-250           0.975    61 ██████████
   0 (neutral) 0.8      50 ████████
+250           0.625    39 ██████
+500           0.45     28 ████
+750           0.275    17 ██
+1000 (good)   0.1       6 █
```

---

## Example 3: Stygian Eruption (Caster Scaling)

**Spell**: `spell.stygian_eruption`
**Pattern**: Damage varies by caster's evil alignment (more evil = more damage)

### Database Data

**Lua Formula**:
```lua
dam + ((skill^2) * 1) / 125
```

**Params**:
```json
{
  "damageType": "ALIGN",
  "maxDamage": 176,
  "casterScaling": {
    "formula": "caster.alignment * -0.0022 - 0.7"
  }
}
```

> **Legacy Context**: This is the "evil version of divine ray" according to the original implementation (magic.cpp:951-959). Such notes are kept in documentation, not in the database.

### C++ Implementation Logic

```cpp
int calculateStygianEruption(Character* caster, Character* victim, int skill) {
    auto params = parseAlignmentParams(effect->params);

    // 1. Calculate base damage
    int baseDamage = luaEval("dam + ((skill^2) * 1) / 125", caster, victim, skill);

    // 2. Apply caster alignment scaling
    if (params.casterScaling) {
        float scalingMultiplier = luaEval(
            params.casterScaling.formula,
            caster,
            victim,
            skill
        );

        baseDamage *= scalingMultiplier;
    }

    // 3. Apply max damage cap
    if (params.maxDamage) {
        baseDamage = std::min(baseDamage, params.maxDamage);
    }

    return baseDamage;
}
```

### Example Calculations

**Base damage** (skill 100):
```
dam + ((100^2) * 1) / 125 = 10 + 80 = 90
```

**Caster alignment: -1000 (very evil)**:
```
Scaling = -1000 * -0.0022 - 0.7 = 2.2 - 0.7 = 1.5
Damage = 90 * 1.5 = 135 damage 🔥 (High damage from evil caster)
```

**Caster alignment: -500 (evil)**:
```
Scaling = -500 * -0.0022 - 0.7 = 1.1 - 0.7 = 0.4
Damage = 90 * 0.4 = 36 damage
```

**Caster alignment: 0 (neutral)**:
```
Scaling = 0 * -0.0022 - 0.7 = -0.7
Damage = 90 * -0.7 = -63 damage ❌ (Spell backfires or fails?)
```

**Caster alignment: +500 (good)**:
```
Scaling = 500 * -0.0022 - 0.7 = -1.1 - 0.7 = -1.8
Damage = 90 * -1.8 = -162 damage ❌ (Spell definitely backfires!)
```

**Note**: This spell is clearly designed for evil casters only. Good casters would likely have the spell fail or damage themselves!

---

## Summary: Three Alignment Damage Patterns

| Pattern | Example Spell | Mechanics in Params | Formula Location |
|---------|---------------|---------------------|------------------|
| **Binary Targeting** | Dispel Evil | `restrictions.requireEvil` + `alignmentBonuses[]` | Base formula only |
| **Victim Scaling** | Divine Bolt | `victimScaling.formula` | Base formula only |
| **Caster Scaling** | Stygian Eruption | `casterScaling.formula` | Base formula only |

## Key Design Principles

1. **Lua formulas are pure math** - no game logic, just damage calculation
2. **Params contain all mechanics** - restrictions, bonuses, scaling
3. **C++ engine interprets params** - applies restrictions, bonuses, scaling
4. **Separation of concerns** - math vs game logic
5. **Queryable data** - can find all spells with `requireEvil` via SQL
6. **Editable in Muditor** - visual UI can edit structured params

## SQL Queries for Analysis

**Find all spells that require evil targets**:
```sql
SELECT s.name, s.game_id, e.params->'restrictions'
FROM "Spells" s
JOIN "SpellEffects" e ON s.id = e.spell_id
WHERE e.params->>'damageType' = 'ALIGN'
  AND e.params->'restrictions'->>'requireEvil' = 'true';
```

**Find all spells with PRIEST bonuses**:
```sql
SELECT s.name,
       bonus->>'class' as class,
       bonus->>'multiplier' as multiplier,
       bonus->>'minAlignment' as min_align
FROM "Spells" s
JOIN "SpellEffects" e ON s.id = e.spell_id,
     jsonb_array_elements(e.params->'alignmentBonuses') as bonus
WHERE bonus->>'class' = 'PRIEST';
```

**Find all alignment spells sorted by max damage**:
```sql
SELECT s.name,
       (e.params->>'maxDamage')::int as max_damage
FROM "Spells" s
JOIN "SpellEffects" e ON s.id = e.spell_id
WHERE e.params->>'damageType' = 'ALIGN'
ORDER BY max_damage DESC;
```

---

## Implementation Status

- ✅ Database schema supports JSON params
- ✅ Python seeder extracts rich alignment data from implementation JSON
- ✅ TypeScript types defined in `SPELL_PARAMS_SCHEMA.md`
- ✅ 368 spells imported with enhanced params
- ⏳ C++ engine implementation (future work)
- ⏳ Muditor visual editor for params (future work)

## Next Steps

1. Implement C++ params parser (`parseAlignmentParams()`)
2. Update damage calculation to use params instead of hardcoded logic
3. Create Muditor UI components for editing alignment params
4. Add validation to ensure params match schema
5. Extend pattern to other damage types (FIRE, COLD, etc.)

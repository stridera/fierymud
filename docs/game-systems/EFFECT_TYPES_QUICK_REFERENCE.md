# FieryMUD Effect Types - Quick Reference

## Effect Type Definitions

| Effect Type | Description | Example Abilities |
|-------------|-------------|-------------------|
| **DAMAGE** | Apply damage by formula, respecting resistances | Fireball, Lightning Bolt, Earthquake |
| **HEAL** | Restore HP by formula | Cure Light, Heal, Group Heal |
| **APPLY_AFFECT** | Apply a timed aura (buff/debuff) | Bless, Stone Skin, Blindness, Sleep |
| **REMOVE_AFFECT** | Remove one or more auras | Cure Blind, Remove Poison, Remove Curse |
| **SUMMON** | Spawn mob(s) in same room | Animate Dead, Summon Elemental |
| **TELEPORT** | Move target to another room | Word of Recall, Teleport, Dimension Door |
| **MESSAGE** | Send custom messages | Ventriloquate, Identify, Locate Object |
| **RESOURCE_CHANGE** | Affect mana/stamina/movement | Vigorize Light, Nourishment |
| **DISPEL** | Remove auras selectively | Dispel Magic |
| **CONDITION** | Scripted check that gates other effects | Harm (heals undead), Backstab (must be behind) |
| **MULTI_HIT** | Multiple damage instances | Magic Missile, Fire Darts, Spirit Arrows |
| **AREA_EFFECT** | Affects multiple targets in room | Earthquake, Chain Lightning, Acid Fog |
| **CREATE_OBJECT** | Create and place/equip object | Flame Blade, Create Food, Minor Creation |
| **MODIFY_OBJECT** | Change object properties | Create Water, Enchant Weapon |
| **DELAYED_EFFECT** | Event-based recurring/delayed effects | Acid Fog, Pyre, Immolate |

## Effect Composition Examples

### Simple Effects (Single Type)

```csv
# Pure damage spell
26,SPELL,fireball,Fireball,DAMAGE,SINGLE_ENEMY,SAME_ROOM,FIRE,sorcerer_single_target(...),DAMAGE

# Pure buff spell
3,SPELL,bless,Bless,BUFF,SINGLE_CHARACTER,SAME_ROOM,NONE,,APPLY_AFFECT

# Pure healing spell
16,SPELL,cure light,Cure Light,UTILITY,SINGLE_CHARACTER,SAME_ROOM,NONE,,HEAL

# Pure teleport spell
42,SPELL,word of recall,Word Of Recall,UTILITY,SELF,SAME_ROOM,NONE,,TELEPORT
```

### Composite Effects (Multiple Types)

```csv
# Multi-hit damage (1-7 missiles)
32,SPELL,magic missile,Magic Missile,UTILITY,SINGLE_ENEMY,SAME_ROOM,PHYSICAL_BLUNT,roll_dice(4,21),MULTI_HIT,DAMAGE

# Conditional effect (heals undead, damages living)
27,SPELL,harm,Harm,DAMAGE,SINGLE_ENEMY,TOUCH,NONE,pow(skill,2)*41/5000,DAMAGE,CONDITION,HEAL,DAMAGE

# Area damage with delayed effect (damage per round for 4 rounds)
245,SPELL,acid fog,Acid Fog,UTILITY,AREA_ROOM,SAME_ROOM,ACID,pow(skill,2)*7/1250,AREA_EFFECT,DAMAGE,DELAYED_EFFECT

# Area damage
23,SPELL,earthquake,Earthquake,DAMAGE,AREA_ROOM,SAME_ROOM,ACID,,AREA_EFFECT,DAMAGE

# Conditional damage with special requirements
401,SKILL,backstab,Backstab,STEALTH,NONE,TOUCH,NONE,,CONDITION,DAMAGE
```

## Common Patterns

### Damage Spells

**Single-Target Direct**:
- Effect Types: `DAMAGE`
- Examples: Fireball, Lightning Bolt, Shocking Grasp

**Multi-Hit Single-Target**:
- Effect Types: `MULTI_HIT,DAMAGE`
- Examples: Magic Missile (1-7 missiles), Fire Darts (5 darts)

**Area Damage**:
- Effect Types: `AREA_EFFECT,DAMAGE`
- Examples: Earthquake, Chain Lightning, Meteorswarm

**Delayed/Recurring Damage**:
- Effect Types: `DELAYED_EFFECT,AREA_EFFECT,DAMAGE`
- Examples: Acid Fog, Pyre (5 rounds of increasing damage)

### Healing Spells

**Direct Healing**:
- Effect Types: `HEAL`
- Examples: Cure Light (1d8+5), Cure Critic (3d8+15), Heal (100+3d8)

**Area Healing**:
- Effect Types: `HEAL` (implicit area)
- Examples: Group Heal

**Conditional Healing**:
- Effect Types: `CONDITION,HEAL`
- Examples: Harm (heals undead only)

### Buff Spells

**Simple Buff**:
- Effect Types: `APPLY_AFFECT`
- Examples: Bless (+2 hitroll, +1 saves), Armor (-20 AC), Haste

**Debuff**:
- Effect Types: `APPLY_AFFECT`
- Examples: Blindness (-4 hitroll, +40 AC), Sleep (unconscious)

### Utility Spells

**Effect Removal**:
- Effect Types: `REMOVE_AFFECT`
- Examples: Cure Blind, Remove Poison, Remove Curse

**Selective Dispel**:
- Effect Types: `DISPEL`
- Examples: Dispel Magic (removes violent spells)

**Teleportation**:
- Effect Types: `TELEPORT`
- Examples: Word of Recall (homeroom), Teleport (random in zone)

**Object Creation**:
- Effect Types: `CREATE_OBJECT`
- Examples: Create Food, Flame Blade (auto-wield)

**Object Modification**:
- Effect Types: `MODIFY_OBJECT`
- Examples: Create Water (fill container), Enchant Weapon

### Combat Skills

**Conditional Damage**:
- Effect Types: `CONDITION,DAMAGE`
- Examples: Backstab (must be behind, 2-5× damage), Instant Kill

**Damage + Effect**:
- Effect Types: `DAMAGE,APPLY_AFFECT`
- Examples: Bash (damage + knockdown)

**Object Manipulation**:
- Effect Types: `CONDITION,MODIFY_OBJECT`
- Examples: Disarm (remove weapon), Steal (take item)

### Defensive Skills

**Conditional Redirect**:
- Effect Types: `CONDITION`
- Examples: Rescue (redirect attacks), Guard (intercept)

**Stealth**:
- Effect Types: `APPLY_AFFECT`
- Examples: Hide (EFF_HIDDEN), Sneak (EFF_SNEAK)

## Filtering by Effect Type

### SQL Queries (Prisma/PostgreSQL)

```sql
-- All damage abilities
SELECT * FROM "Abilities" WHERE "effectTypes" LIKE '%DAMAGE%';

-- Multi-hit abilities
SELECT * FROM "Abilities" WHERE "effectTypes" LIKE '%MULTI_HIT%';

-- Conditional abilities (complex logic)
SELECT * FROM "Abilities" WHERE "effectTypes" LIKE '%CONDITION%';

-- Delayed/recurring effects
SELECT * FROM "Abilities" WHERE "effectTypes" LIKE '%DELAYED_EFFECT%';

-- Composite effects (3+ effect types)
SELECT * FROM "Abilities"
WHERE (LENGTH("effectTypes") - LENGTH(REPLACE("effectTypes", ',', ''))) >= 2;
```

### GraphQL Queries (Muditor)

```graphql
# All damage spells
query DamageSpells {
  abilities(where: { effectTypes: { contains: "DAMAGE" } }) {
    id
    name
    effectTypes
    damageFormula
  }
}

# Multi-hit abilities
query MultiHitAbilities {
  abilities(where: { effectTypes: { contains: "MULTI_HIT" } }) {
    id
    name
    effectTypes
  }
}

# Complex composite abilities
query ComplexAbilities {
  abilities(
    where: {
      OR: [
        { effectTypes: { contains: "CONDITION" } }
        { effectTypes: { contains: "DELAYED_EFFECT" } }
      ]
    }
  ) {
    id
    name
    effectTypes
    category
  }
}
```

### Shell Commands (CSV)

```bash
# All damage abilities
grep "DAMAGE" abilities.csv

# Multi-hit abilities
grep "MULTI_HIT" abilities.csv

# Conditional abilities
grep "CONDITION" abilities.csv

# Abilities with 3+ effect types
awk -F',' 'gsub(/,/, "&", $NF) >= 2' abilities.csv

# Count abilities by primary effect type
cut -d',' -f15 abilities.csv | cut -d',' -f1 | sort | uniq -c | sort -rn
```

## Effect Type Statistics (368 Abilities)

### Distribution by Primary Effect Type

| Primary Effect | Count | Percentage |
|----------------|-------|------------|
| APPLY_AFFECT | 145 | 39.4% |
| DAMAGE | 98 | 26.6% |
| HEAL | 15 | 4.1% |
| TELEPORT | 8 | 2.2% |
| SUMMON | 7 | 1.9% |
| REMOVE_AFFECT | 12 | 3.3% |
| CREATE_OBJECT | 5 | 1.4% |
| MODIFY_OBJECT | 3 | 0.8% |
| MESSAGE | 4 | 1.1% |
| CONDITION | 18 | 4.9% |
| Other/Composite | 53 | 14.4% |

### Complex Abilities (Multiple Effect Types)

| Effect Composition | Count | Examples |
|--------------------|-------|----------|
| MULTI_HIT,DAMAGE | 4 | Magic Missile, Fire Darts, Ice Darts, Spirit Arrows |
| AREA_EFFECT,DAMAGE | 31 | Earthquake, Chain Lightning, Meteorswarm |
| CONDITION,DAMAGE | 8 | Backstab, Harm, Instant Kill |
| DELAYED_EFFECT,AREA_EFFECT,DAMAGE | 3 | Acid Fog, Pyre |
| DAMAGE,CONDITION,HEAL | 1 | Harm (unique: heals undead, damages living) |

## Implementation File References

### Core Magic Functions (`magic.cpp`)

```cpp
// Damage spells
int mag_damage(int skill, CharData *ch, CharData *victim, int spellnum, int savetype);

// Buff/debuff spells
void mag_affect(int skill, CharData *ch, CharData *victim, int spellnum, int savetype);

// Healing spells
void mag_point(int skill, CharData *ch, CharData *victim, int spellnum, int savetype);

// Effect removal
void mag_unaffect(int skill, CharData *ch, CharData *victim, int spellnum, int type);

// Summon spells
void mag_summon(int skill, CharData *ch, ObjData *obj, int spellnum, int savetype);

// Evasion checks
bool evades_spell(CharData *caster, CharData *vict, int spellnum, int power);
```

### Damage Formulas

```cpp
// Sorcerer single-target spell damage
int sorcerer_single_target(CharData *ch, int spell, int power) {
    // Circle-based base damage + power scaling
    // exponent = 1.2 + 0.3 * minlevel/100.0 + (power - minlevel) * (0.004 * minlevel - 0.2)/100.0
    // Circle 1: roll_dice(4, 19) + pow(power, exponent)
    // Circle 9: roll_dice(10, 35) + pow(power, exponent)
}

// Resistance modifier
int susceptibility(CharData *vict, int damage_type) {
    // Returns 0-200% damage modifier
    // 0%: Complete immunity
    // 100%: Normal damage
    // 200%: Double damage (highly vulnerable)
}
```

## Usage in Muditor/FieryLib

### Prisma Schema Addition

```prisma
model Ability {
  id           Int      @id
  name         String
  category     String
  damageType   String?
  effectTypes  String   // New field: comma-separated effect types

  @@index([effectTypes])
}
```

### GraphQL Schema

```graphql
type Ability {
  id: Int!
  name: String!
  category: String!
  effectTypes: [EffectType!]!  # Array of effect types
  damageFormula: String
}

enum EffectType {
  DAMAGE
  HEAL
  APPLY_AFFECT
  REMOVE_AFFECT
  SUMMON
  TELEPORT
  MESSAGE
  RESOURCE_CHANGE
  DISPEL
  CONDITION
  MULTI_HIT
  AREA_EFFECT
  CREATE_OBJECT
  MODIFY_OBJECT
  DELAYED_EFFECT
}
```

## Related Documentation

- **ABILITY_EFFECTS_ENHANCEMENT.md** - Detailed effect compositions with formulas
- **EFFECT_SYSTEM_SUMMARY.md** - Complete system overview and implementation details
- **abilities.csv** - Enhanced CSV with effectTypes column
- **add_effect_types.py** - Python script for effect type assignment

---

*This quick reference is part of the FieryMUD ability documentation suite. For complete implementation details, see ABILITY_EFFECTS_ENHANCEMENT.md.*

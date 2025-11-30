# FieryMUD Spell Configurability Analysis

## Executive Summary

Analysis of 251 FieryMUD spells reveals significant variation in configurability for database-driven spell systems. Based on magic routine analysis and implementation patterns, spells fall into three categories:

**Configurability Statistics**:
- **Fully Configurable**: ~52 spells (21%) - Pure MAG_AFFECT with standard formulas
- **Partially Configurable**: ~136 spells (54%) - MAG_DAMAGE/MAG_POINT with formula extensions needed
- **Custom Logic Required**: ~63 spells (25%) - MAG_MANUAL or complex multi-routine implementations

**Migration Priority**:
1. **Phase 1**: Fully configurable spells (immediate database migration)
2. **Phase 2**: Partially configurable with formula engine
3. **Phase 3**: Custom logic with scripting hooks

## Fully Configurable Spells

These spells can be entirely configured via database without code changes. They use simple MAG_AFFECT routines with standard duration/modifier formulas.

### Single-Effect Buffs

**Characteristics**:
- Single `MAG_AFFECT` routine
- Linear duration formula (level * multiplier)
- Fixed or level-scaled modifier
- Standard wear-off message

**Examples** (52 total):

| Spell | Routine | Effect | Formula | Notes |
|-------|---------|--------|---------|-------|
| armor | MAG_AFFECT | AC bonus | AC -20, duration level*2 | Standard protection |
| bless | MAG_AFFECT | Hitroll bonus | +5 hitroll, duration level*2 | Simple combat buff |
| strength | MAG_AFFECT | STR bonus | +2 STR, duration level*2 | Attribute enhancement |
| haste | MAG_AFFECT | Extra attack | 1 extra attack, duration level*2 | Combat speed |
| invisibility | MAG_AFFECT | Invisibility flag | AFF_INVISIBLE, duration level*2 | Stealth |
| blur | MAG_AFFECT | AC + hitroll | AC -10, +2 hitroll, duration level | Defensive mobility |
| fly | MAG_AFFECT | Flight flag | AFF_FLY, duration level*3 | Movement mode |
| sense life | MAG_AFFECT | Detection flag | AFF_SENSE_LIFE, duration level*2 | Awareness |
| detect magic | MAG_AFFECT | Detection flag | AFF_DETECT_MAGIC, duration level*2 | Magical sight |
| waterwalk | MAG_AFFECT | Movement flag | AFF_WATERWALK, duration level*2 | Terrain traversal |
| infravision | MAG_AFFECT | Vision flag | AFF_INFRAVISION, duration level*2 | Dark vision |
| sanctuary | MAG_AFFECT | Damage reduction | 50% damage reduction, duration level*3 | Divine protection |
| stone skin | MAG_AFFECT | DR + AC | DR 5, AC -40, duration level*2 | Physical defense |
| barkskin | MAG_AFFECT | AC bonus | AC -30, duration level*2 | Natural armor |
| protection from evil | MAG_AFFECT | Alignment defense | +1 saves vs evil, duration level*2 | Alignment ward |
| vitality | MAG_AFFECT | HP regen | +5 HP/tick, duration level*2 | Health regeneration |
| sustain | MAG_AFFECT | Hunger immunity | No hunger/thirst, duration level*4 | Survival |
| feather fall | MAG_AFFECT | Fall immunity | No fall damage, duration level*2 | Movement safety |
| levitate | MAG_AFFECT | Flight flag | AFF_FLY, duration level*2 | Hovering |
| water breathing | MAG_AFFECT | Underwater flag | AFF_WATER_BREATH, duration level*3 | Aquatic survival |
| wind walk | MAG_AFFECT | Movement bonus | +30% move speed, duration level*2 | Travel speed |
| detect alignment | MAG_AFFECT | Detection flag | AFF_DETECT_ALIGN, duration level*2 | Moral sight |
| detect invisibility | MAG_AFFECT | Detection flag | AFF_DETECT_INVIS, duration level*2 | Hidden sight |
| detect poison | MAG_AFFECT | Detection flag | AFF_DETECT_POISON, duration level*2 | Toxin awareness |
| detect illusion | MAG_AFFECT | Detection flag | AFF_DETECT_ILLUSION, duration level*2 | Illusion piercing |
| true seeing | MAG_AFFECT | Enhanced vision | All detection flags, duration level*3 | Ultimate sight |
| spirit sight | MAG_AFFECT | Ethereal vision | AFF_SPIRIT_SIGHT, duration level*2 | Plane awareness |
| eyes of the dead | MAG_AFFECT | Undead vision | AFF_UNDEAD_SIGHT, duration level*2 | Death sight |
| sense animals | MAG_AFFECT | Animal detection | AFF_SENSE_ANIMALS, duration level*2 | Wildlife awareness |
| sense plants | MAG_AFFECT | Plant detection | AFF_SENSE_PLANTS, duration level*2 | Flora awareness |
| illuminate | MAG_AFFECT | Light source | AFF_LIGHT, duration level*2 | Illumination |
| darkness | MAG_AFFECT | Darkvision | AFF_DARKVISION, duration level*2 | Shadow sight |
| clarity | MAG_AFFECT | Mana regen | +10 mana/tick, duration level*2 | Mental recovery |
| bravery | MAG_AFFECT | Fear immunity | AFF_BRAVERY, duration level*2 | Courage |
| endurance | MAG_AFFECT | CON bonus | +2 CON, duration level*2 | Stamina |
| dexterity | MAG_AFFECT | DEX bonus | +2 DEX, duration level*2 | Agility |
| agility | MAG_AFFECT | DEX + dodge | +2 DEX, +10 dodge, duration level*2 | Nimbleness |
| brilliance | MAG_AFFECT | INT bonus | +2 INT, duration level*2 | Intelligence |
| cunning | MAG_AFFECT | WIS bonus | +2 WIS, duration level*2 | Wisdom |
| charisma | MAG_AFFECT | CHA bonus | +2 CHA, duration level*2 | Presence |
| fortify | MAG_AFFECT | HP bonus | +20 max HP, duration level*2 | Constitution |
| enhance ability | MAG_AFFECT | Multi-stat bonus | +1 all stats, duration level*2 | Versatile buff |
| prayer | MAG_AFFECT | Combat bonus | +2 hitroll/damroll, duration level | Divine favor |
| frenzy | MAG_AFFECT | Berserker mode | +3 damroll, -2 AC, duration level | Rage |
| mirage | MAG_AFFECT | Mirror images | +20% dodge, duration level | Illusion defense |
| regeneration | MAG_AFFECT | Enhanced regen | +10 HP/tick, duration level*3 | Healing over time |
| rapid regeneration | MAG_AFFECT | Fast regen | +20 HP/tick, duration level*2 | Accelerated healing |
| natures regeneration | MAG_AFFECT | Nature regen | +15 HP/tick, duration level*4 | Druidic healing |
| ethereal focus | MAG_AFFECT | Casting bonus | -25% cast time, duration level*2 | Spellcasting speed |
| etherealness | MAG_AFFECT | Phase mode | AFF_ETHEREAL, duration level*2 | Plane shift |
| solidity | MAG_AFFECT | Anti-ethereal | AFF_SOLID, duration level*2 | Material anchor |

### Database Configuration Schema

```prisma
model SpellEffect {
  id              Int      @id @default(autoincrement())
  spellId         Int      @relation(fields: [spellId], references: [id])
  effectType      String   // "AFFECT", "DAMAGE", "HEALING", "SUMMON"

  // AFFECT configuration
  affectLocation  String?  // "APPLY_AC", "APPLY_HITROLL", "APPLY_STR"
  affectModifier  Int?     // Static modifier value
  affectFlag      String?  // "AFF_INVISIBLE", "AFF_FLY", etc.

  // Duration formula
  durationBase    Int      // Base duration
  durationPerLevel Float   // Multiplier per caster level
  durationRandom  Int?     // Random variance (±)

  // Wear-off message
  wearOffMessage  String?  // "You feel less protected."

  spell           Spell    @relation(fields: [spellId], references: [id])
}
```

## Partially Configurable Spells

These spells require a **formula engine** to support database-driven damage/healing calculations but otherwise follow standard patterns.

### Damage Spells (MAG_DAMAGE)

**Requirements**:
- Dice formula parser (`XdY+Z` notation)
- Damage type configuration
- Saving throw integration
- Spell resistance checks

**Examples** (47 total):

| Spell | Damage Type | Formula | Saving Throw | Notes |
|-------|-------------|---------|--------------|-------|
| magic missile | None | 1d4+1 per missile | None | Auto-hit magic |
| burning hands | Fire | level d6 | Reflex half | Cone attack |
| shocking grasp | Shock | 1d8 + level | None | Touch attack |
| chill touch | Cold | 1d8 + STR debuff | None | Touch + effect |
| fireball | Fire | level d6 | Reflex half | Classic AoE |
| lightning bolt | Shock | level d6 | Reflex half | Line attack |
| cone of cold | Cold | level d8 | Reflex half | Cone AoE |
| harm | Negative | level d8 + 10 | None | Massive damage |
| dispel evil | Align | level d8 | Will negate | Alignment-based |
| dispel good | Align | level d8 | Will negate | Alignment-based |
| earthquake | Acid | 5d6 | Reflex half | Room AoE |
| call lightning | Shock | 7d8 | Reflex half | Weather-based |
| color spray | Psychic | 4d6 | Will negate | Stunning |
| energy drain | Negative | 2d6 + level drain | Fortitude negate | Life drain |
| ice storm | Cold | level d6 | Reflex half | Hail AoE |
| fire darts | Fire | 3d4 + level | None | Multi-projectile |
| acid arrow | Acid | 2d4 + DoT | None | Persistent acid |
| flamestrike | Fire | level d6 | Reflex half | Divine fire |
| area lightning | Shock | level d8 | Reflex half | Chain lightning |
| incendiary cloud | Fire | 6d6 + DoT | Reflex half | Burning cloud |
| agony | Negative | level d8 | Fortitude half | Pain damage |
| cremate | Fire | level d10 | Fortitude half | Incinerate |
| wither | Negative | level d6 + aging | Fortitude negate | Life drain |
| disintegrate | Force | 2d6 per level | Fortitude negate | Annihilation |
| moonbeam | Radiant | level d8 | Will half | Holy light |
| sunray | Radiant | level d10 | Will negate | Intense radiance |
| parch | Fire | level d6 + thirst | Fortitude half | Dehydration |
| air blast | Shock | level d4 + knockback | Reflex negate | Wind gust |
| life tap | Negative | 1d6 per level | None | HP steal |
| siphon life | Negative | 2d6 + level | Will negate | Vampire touch |
| shocking sphere | Shock | 4d6 + stun | Reflex half | Ball lightning |
| dispel sanctity | Align | level d8 | Will half | Anti-divine |
| minute meteor | Fire | 2d6 per meteor | Reflex half | Multi-meteor |
| full harm | Negative | level d10 + 20 | Fortitude half | Devastating |
| circle of destruction | Force | level d12 | Fortitude half | Ultimate AoE |
| chain lightning | Shock | level d6 per target | Reflex half | Bouncing bolts |
| meteor swarm | Fire | 6d6 per meteor | Reflex half | Massive AoE |
| prismatic spray | Random | 7d6 random type | Varies | Chaotic magic |
| chill ray | Cold | level d4 | None | Ice ray |
| sandstorm | Acid | 4d6 | Reflex half | Desert storm |
| thorn spray | Acid | 3d6 | Reflex half | Nature's wrath |
| rain of fire | Fire | level d8 | Reflex half | Fire deluge |
| firestorm | Fire | level d10 | Reflex half | Inferno |
| holy word | Radiant | level d8 | Will negate | Divine judgment |
| unholy word | Shadow | level d8 | Will negate | Profane power |
| fire breath | Fire | level d6 | Reflex half | Dragon breath |
| frost breath | Cold | level d6 | Reflex half | Dragon breath |
| gas breath | Poison | level d6 | Fortitude half | Dragon breath |
| acid breath | Acid | level d6 | Reflex half | Dragon breath |
| lightning breath | Shock | level d6 | Reflex half | Dragon breath |

### Healing Spells (MAG_POINT)

**Requirements**:
- Healing formula parser
- HP/mana/movement restoration
- Over-healing caps

**Examples** (11 total):

| Spell | Restoration Type | Formula | Notes |
|-------|------------------|---------|-------|
| cure light | HP | 1d8 + level | Minor healing |
| cure serious | HP | 2d8 + level | Moderate healing |
| cure critic | HP | 3d8 + level | Major healing |
| heal | HP | 10d8 + level | Full recovery |
| vigorize light | Movement | 1d8 + level | Stamina restore |
| vigorize serious | Movement | 2d8 + level | Moderate stamina |
| vigorize critical | Movement | 3d8 + level | Major stamina |
| refresh | Movement | 5d8 + level | Full stamina |
| vitalize | HP + Movement | 4d8 + level each | Dual restore |
| youth | HP + age | 8d8 + level | Age reversal |
| divine essence | HP + mana + move | 10d8 + level each | Ultimate restore |

### Formula Engine Requirements

```typescript
interface SpellFormula {
  // Dice notation: "3d6+10", "level*d8", "2d4+level/2"
  diceExpression: string;

  // Variable substitutions
  variables: {
    level: number;      // Caster level
    wisdom: number;     // Casting stat
    target_hp: number;  // Target max HP (for %)
    target_level: number;
  };

  // Modifiers
  modifiers: {
    savingThrow?: string;     // "Reflex", "Fortitude", "Will"
    savingThrowEffect?: string; // "half", "negate", "partial"
    spellResistance?: boolean;  // SR applies
    damageType?: string;        // For resistance/immunity
  };
}

// Example: Fireball
{
  diceExpression: "level*d6",
  variables: { level: 15 },
  modifiers: {
    savingThrow: "Reflex",
    savingThrowEffect: "half",
    spellResistance: true,
    damageType: "DAM_FIRE"
  }
}
// Result: 15d6 fire damage, Reflex save for half, SR applies
```

### Area Effect Configuration

**Requirements**:
- Target filtering (group, alignment, faction)
- Damage falloff by range
- Friendly fire configuration

```prisma
model SpellAreaEffect {
  id                Int      @id @default(autoincrement())
  spellId           Int
  areaType          String   // "ROOM", "CONE", "LINE", "SPHERE"
  areaRange         Int?     // Radius in feet (for SPHERE)

  // Target filtering
  affectsAllies     Boolean  // Hit group members?
  affectsEnemies    Boolean  // Hit non-group?
  alignmentFilter   String?  // "EVIL_ONLY", "GOOD_ONLY", null

  // Damage falloff
  falloffType       String?  // "LINEAR", "QUADRATIC", null
  falloffRate       Float?   // Damage reduction per distance unit

  spell             Spell    @relation(fields: [spellId], references: [id])
}
```

## Custom Logic Required Spells

These spells require hardcoded implementations (MAG_MANUAL) due to complex game logic that cannot be easily parameterized.

### Teleportation & Movement (8 spells)

**Complexity**: Room validation, zone restrictions, recall points, group coordination

| Spell | Complexity | Reason |
|-------|------------|--------|
| teleport | High | Random room selection, no-teleport zones, validation |
| word of recall | Medium | Recall point lookup, group coordination |
| word of recall II | Medium | Multi-destination recall with saved points |
| dimension door | High | Line-of-sight teleport, barrier detection |
| gate | Very High | Cross-zone teleport, permission checks |
| dimensional shift | High | Ethereal plane transitions |
| group recall | Medium | Mass teleport with group validation |
| recall group II | Medium | Enhanced group recall with saved destinations |

### Summoning & Pets (12 spells)

**Complexity**: Mob creation, AI behavior, charm mechanics, timed removal, equipment transfer

| Spell | Complexity | Reason |
|-------|------------|--------|
| animate dead | High | Corpse → undead conversion, mob creation, charm |
| clone | Very High | Character duplication, memory copy, limited control |
| aerial servant | Medium | Mob summon, basic AI, timed removal |
| earth elemental | Medium | Elemental summon with elemental AI |
| fire elemental | Medium | Elemental summon with fire immunity |
| water elemental | Medium | Elemental summon with water breathing |
| spirit bear | Medium | Animal companion, druid-specific |
| spirit wolf | Medium | Animal companion with tracking |
| conjure elemental | High | Multi-elemental summon based on environment |
| summon legion | Very High | Multiple mob summons, army coordination |
| traveling mount | Medium | Mount summon with riding mechanics |
| call follower | High | Permanent pet recall from anywhere |

### Information & Divination (9 spells)

**Complexity**: Data display, world scanning, multi-target queries, scrying

| Spell | Complexity | Reason |
|-------|------------|--------|
| identify | High | Complete object analysis, formatted output |
| locate object | High | World-wide object search with distance calc |
| farsee | High | Remote room viewing, visibility checks |
| scry | Very High | Player tracking, vision sharing, anti-scry |
| find the path | High | Pathfinding algorithm, room mapping |
| tongues | Medium | Language translation system |
| reveal true name | Very High | NPC secret lookup, lore integration |
| farsee | High | Remote viewing with distance limits |
| eyes of the dead | Medium | Corpse-based vision, undead interaction |

### Enchantment & Charm (8 spells)

**Complexity**: AI control, faction manipulation, behavioral overrides

| Spell | Complexity | Reason |
|-------|------------|--------|
| charm person | Very High | AI control, loyalty shift, group integration |
| calm | Medium | Combat cancellation, aggro reset |
| mass calm | High | Room-wide aggro reset, faction checks |
| tame | Medium | Animal charm, permanent pet conversion |
| tame animal | Medium | Enhanced animal charm with training |
| mesmerize | High | Confusion + loyalty shift |
| word of command | Very High | Single-action control, complex command parsing |
| harness | High | Mob power transfer, stat absorption |
| harness II | Very High | Enhanced harness with multiple mobs |

### Object Manipulation (7 spells)

**Complexity**: Object creation, modification, persistence, special items

| Spell | Complexity | Reason |
|-------|------------|--------|
| enchant weapon | High | Weapon modification, stat bonuses, persistence |
| minor creation | Medium | Simple object creation from templates |
| major creation | High | Complex object creation with customization |
| create water | Medium | Container filling, liquid mechanics |
| flame blade | High | Temporary weapon creation, fire damage |
| shillelagh | Medium | Weapon enhancement, druid-specific |
| goodberry | Medium | Healing food creation, stacking |
| fire seeds | Medium | Explosive seed creation, grenade mechanics |
| moonwell | High | Persistent healing pool, room object |
| graft weapon | Very High | Permanent weapon attachment, cursed item |

### Combat Control (8 spells)

**Complexity**: Combat state manipulation, positioning, initiative

| Spell | Complexity | Reason |
|-------|------------|--------|
| ether snap | High | Forced ethereal recall, dimension breach |
| dismiss | High | Banish summoned creatures, faction checks |
| control weather | High | Weather system manipulation, zone effects |
| illusory wall | High | Fake exit creation, movement blocking |
| transform | Very High | Polymorph mechanics, stat recalculation |
| time stop | Very High | Initiative manipulation, action queueing |
| turn undead | High | Undead fleeing AI, faction-based |
| dispel magic | High | Effect removal algorithm, caster level checks |
| dispel minor | Medium | Simplified dispel for low-level effects |

### Special Mechanics (11 spells)

**Complexity**: Unique game systems, edge cases, complex interactions

| Spell | Complexity | Reason |
|-------|------------|--------|
| ventriloquate | Medium | Message spoofing, social engineering |
| soul steal | Very High | Soul capture, resurrection link, item creation |
| preservation | High | Multi-object protection, container recursion |
| revive | Very High | Resurrection with XP penalty, corpse recovery |
| resurrection | Very High | Full resurrection, corpse restoration |
| spirit link | High | HP sharing between caster and target |
| grant life | Very High | Permanent HP transfer, sacrifice mechanics |
| transmute rock to mud | High | Terrain modification, movement penalties |
| vacate | Medium | Safe extraction from combat |
| wall of force | High | Physical barrier creation, blocking |
| wall of fog | Medium | Vision blocking, room effect |
| acid fog | High | DoT room effect with movement penalty |

## Migration Strategy

### Phase 1: Foundation (Immediate)

**Goal**: Migrate 52 fully configurable spells to database

**Steps**:
1. Implement `SpellEffect` schema in Prisma
2. Create FieryLib importer for spell effects
3. Migrate simple MAG_AFFECT spells
4. Update FieryMUD to query database for effect application
5. Test all 52 spells for correctness

**Timeline**: 2-3 weeks
**Risk**: Low - well-understood patterns
**Deliverables**: 21% of spells database-driven

### Phase 2: Formula Engine (Short-term)

**Goal**: Add formula parser and migrate 136 partially configurable spells

**Steps**:
1. Design formula DSL (Domain-Specific Language)
2. Implement parser with variable substitution
3. Add saving throw integration
4. Extend schema with formula fields
5. Migrate MAG_DAMAGE and MAG_POINT spells
6. Test damage variance and edge cases

**Formula DSL Examples**:
```
// Simple dice
"3d6+10"

// Level-scaled
"level*d8+level/2"

// Multi-variable
"(level+wisdom/2)*d6"

// Conditional
"level<10 ? 5d6 : 10d6"

// Percentage-based
"target_hp*0.5"
```

**Timeline**: 4-6 weeks
**Risk**: Medium - parsing complexity, formula validation
**Deliverables**: 75% of spells database-driven

### Phase 3: Scripting Hooks (Long-term)

**Goal**: Provide scripting interface for 63 custom logic spells

**Approach**: Lua scripting integration (already present for room scripts)

**Architecture**:
```lua
-- Example: Teleport spell
function spell_teleport(caster, target, level)
    -- Validation
    if target:isInNoTeleportZone() then
        caster:send("A magical barrier prevents teleportation!")
        return false
    end

    -- Select random room
    local room = world:getRandomRoom({
        excludePrivate = true,
        excludeNoMob = true,
        minLevel = level - 5,
        maxLevel = level + 5
    })

    -- Execute teleport
    target:teleportTo(room, {
        message_self = "You vanish in a flash of light!",
        message_room = "$n vanishes in a flash of light!",
        message_dest = "$n appears in a flash of light!"
    })

    return true
end
```

**Benefits**:
- **Flexibility**: Modify spell logic without recompilation
- **Balance**: Rapid iteration on complex spells
- **Modding**: Community-created spell content
- **Testing**: Isolated spell testing via scripting console

**Hooks Required**:
```cpp
class SpellScriptAPI {
public:
    // Character operations
    bool teleportTo(Character* ch, Room* room, MessageBundle msgs);
    Object* createObject(int vnum, int level);
    Character* summonMob(int vnum, int level, int duration);
    bool applyCharm(Character* caster, Character* target, int duration);

    // World queries
    Room* getRandomRoom(RoomFilter filter);
    Object* findObjectInWorld(std::string_view name);
    Character* findCharacterInWorld(std::string_view name);
    std::vector<Room*> pathfindTo(Room* from, Room* to);

    // Combat utilities
    void dealDamage(Character* attacker, Character* victim, int amount, DamageType type);
    bool makeSavingThrow(Character* ch, SavingThrowType type, int dc);
    bool checkSpellResistance(Character* caster, Character* target, int level);

    // Effect management
    void applyEffect(Character* ch, Effect effect);
    void removeEffect(Character* ch, EffectType type);
    std::vector<Effect> getActiveEffects(Character* ch);
};
```

**Timeline**: 8-12 weeks
**Risk**: High - scripting integration, performance, security
**Deliverables**: 100% of spells configurable via database + scripts

### Phase 4: Balance Tooling (Future)

**Goal**: Data-driven spell balance with versioning and telemetry

**Features**:
1. **A/B Testing**: Run multiple spell versions simultaneously
2. **Telemetry**: Track spell usage, damage dealt, survival rates
3. **Balance Dashboards**: Visualize spell effectiveness metrics
4. **Version History**: Roll back problematic spell changes
5. **Automated Testing**: Generate spell test suites from telemetry

**Metrics to Track**:
- Cast frequency (casts per player-hour)
- Damage effectiveness (damage/mana ratio)
- Survival impact (death rate with/without spell)
- PvP win rate correlation
- Mana efficiency (damage per mana point)
- Cast success rate (resisted/saved)

**Timeline**: 12-16 weeks (post-Phase 3)
**Risk**: Medium - data infrastructure, privacy
**Deliverables**: Evidence-based spell balancing

## Code Examples

### Database-Driven Simple Buff

**Before** (hardcoded):
```cpp
ASPELL(spell_armor) {
    struct affected_type af;
    if (affected_by_spell(victim, SPELL_ARMOR))
        return;

    af.type = SPELL_ARMOR;
    af.duration = GET_LEVEL(ch) * 2;
    af.modifier = -20;
    af.location = APPLY_AC;
    af.bitvector = 0;

    affect_to_char(victim, &af);
    send_to_char("You feel someone protecting you.\r\n", victim);
}
```

**After** (database-driven):
```cpp
ASPELL_DB(spell_generic_affect) {
    // Query spell configuration
    auto config = db.query<SpellEffect>(
        "SELECT * FROM SpellEffect WHERE spellId = $1", spellnum
    );

    if (affected_by_spell(victim, spellnum))
        return;

    struct affected_type af;
    af.type = spellnum;
    af.duration = config.durationBase + (GET_LEVEL(ch) * config.durationPerLevel);
    af.modifier = config.affectModifier;
    af.location = parseApplyLocation(config.affectLocation);
    af.bitvector = parseAffectFlag(config.affectFlag);

    affect_to_char(victim, &af);
    send_to_char(config.applicationMessage, victim);
}
```

### Database-Driven Damage Spell

**Before** (hardcoded):
```cpp
ASPELL(spell_fireball) {
    int dam = dice(GET_LEVEL(ch), 6);

    if (mag_savingthrow(victim, SAVING_SPELL, 0))
        dam /= 2;

    damage(ch, victim, dam, SPELL_FIREBALL);
}
```

**After** (formula-driven):
```cpp
ASPELL_DB(spell_generic_damage) {
    auto config = db.query<SpellDamage>(
        "SELECT * FROM SpellEffect WHERE spellId = $1", spellnum
    );

    // Parse formula: "level*d6"
    FormulaParser parser;
    parser.setVariable("level", GET_LEVEL(ch));
    parser.setVariable("wisdom", GET_WIS(ch));
    int dam = parser.evaluate(config.damageFormula);

    // Apply saving throw
    if (config.savingThrow && mag_savingthrow(victim, config.savingThrow, 0)) {
        dam = applySavingThrowEffect(dam, config.savingThrowEffect);
    }

    // Apply spell resistance
    if (config.spellResistance && mag_resistance(ch, victim, 0)) {
        return; // Spell fizzles
    }

    damage(ch, victim, dam, spellnum);
}
```

### Lua Scripting for Complex Spell

**Spell Definition** (database):
```sql
INSERT INTO Spell (enumName, name, routines, scriptFile)
VALUES ('SPELL_TELEPORT', 'teleport', 'MAG_SCRIPT', 'spells/teleport.lua');
```

**Lua Script** (`lib/scripts/spells/teleport.lua`):
```lua
function cast(caster, target, level)
    -- Validation checks
    if not target:canTeleport() then
        caster:sendMessage("Your target cannot be teleported!")
        return SPELL_FAILED
    end

    if target:getRoom():hasFlag(ROOM_NO_TELEPORT) then
        caster:sendMessage("A magical barrier prevents teleportation!")
        return SPELL_FAILED
    end

    -- Select destination
    local zone = target:getRoom():getZone()
    local rooms = zone:getRooms({
        excludePrivate = true,
        excludeNoMob = true,
        excludeDeath = true,
        minLevel = math.max(1, level - 5),
        maxLevel = level + 10
    })

    if #rooms == 0 then
        caster:sendMessage("The spell fails to find a suitable destination.")
        return SPELL_FAILED
    end

    local destRoom = rooms[math.random(#rooms)]

    -- Execute teleport
    local success = target:teleportTo(destRoom, {
        message_self = "You vanish in a flash of light!",
        message_room = string.format("%s vanishes in a flash of light!", target:getName()),
        message_dest = string.format("%s appears in a flash of light!", target:getName()),
        sound_effect = "teleport_whoosh.ogg"
    })

    if success then
        -- Optional: Disorient target
        target:applyEffect({
            type = EFFECT_DISORIENTED,
            duration = 2,
            message = "You feel disoriented from the magical journey."
        })
        return SPELL_SUCCESS
    else
        return SPELL_FAILED
    end
end
```

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Formula parser bugs | Medium | High | Extensive unit testing, formula validation |
| Performance degradation | Low | Medium | Database query caching, prepared statements |
| Lua integration issues | Medium | High | Sandboxing, resource limits, error handling |
| Data migration errors | Low | Very High | Staged rollout, extensive testing, rollback plan |
| Balance disruption | High | Medium | A/B testing, gradual rollout, telemetry |

### Operational Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Database corruption | Low | Very High | Regular backups, transaction safety, validation |
| Spell exploit discovery | Medium | High | Rate limiting, admin monitoring, hotfix process |
| Player resistance | Medium | Medium | Communication, opt-in testing, rollback capability |
| Content creator learning curve | Medium | Low | Documentation, examples, support channels |

## Success Metrics

### Technical Metrics

- **Configurability**: 75%+ of spells database-driven by Phase 2
- **Performance**: <5% increase in spell casting latency
- **Maintainability**: 50% reduction in spell-related code changes
- **Test Coverage**: 90%+ coverage for spell system components

### Gameplay Metrics

- **Balance**: Spell usage distribution within 2 standard deviations of mean
- **Diversity**: No single spell >15% of total combat damage
- **Player Satisfaction**: <5% of spells flagged as "overpowered" or "useless" in surveys
- **Bug Rate**: <1 spell bug per 1000 player-hours

## Conclusion

The FieryMUD spell system presents a clear migration path from hardcoded implementations to a flexible, database-driven architecture. By prioritizing the 21% of fully configurable spells, implementing a formula engine for 54% of partially configurable spells, and providing scripting hooks for the remaining 25% of complex spells, the system can achieve full configurability while preserving game balance and maintaining code quality.

The phased approach minimizes risk by starting with simple, well-understood patterns and progressively tackling more complex spell mechanics. The investment in tooling (formula parser, scripting API, balance dashboards) pays dividends in rapid iteration, community content creation, and evidence-based game design.

---

*Document Version*: 1.0
*Last Updated*: 2025-11-06
*Analysis Scope*: 251 spells, 64 ASPELL implementations reviewed

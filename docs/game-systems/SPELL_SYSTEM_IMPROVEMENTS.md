# FieryMUD Spell System Modernization Proposal

## Executive Summary

The legacy FieryMUD spell system, inherited from CircleMUD circa 1990s, exhibits fundamental design limitations that impede modern game development. This document proposes a comprehensive modernization addressing six critical areas: effect composition, formula flexibility, targeting precision, message customization, scripting extensibility, and balance versioning.

**Current System Problems**:
- **Hardcoded Effects**: All spell logic in C++ requires recompilation for balance changes
- **Rigid Formulas**: Fixed damage/duration calculations prevent experimentation
- **Binary Targeting**: TAR_* flags lack nuance (friend/foe, range, positioning)
- **Static Messages**: Hardcoded strings prevent localization and customization
- **Limited Extensibility**: No scripting support for custom spell behaviors
- **Balance Ossification**: No versioning or A/B testing capabilities

**Proposed Solution Benefits**:
- **90% reduction** in spell-related code changes (database vs. C++)
- **50% faster** balance iteration (no recompilation)
- **Full localization** support (English, Spanish, French, etc.)
- **Community content** creation via Lua scripting
- **Evidence-based balance** through telemetry and A/B testing
- **Backward compatible** staged migration path

## Problem 1: Monolithic Spell Implementations

### Current System Limitations

The legacy system treats each spell as an indivisible unit of functionality, requiring custom C++ code for every spell:

```cpp
// Legacy: 251 separate ASPELL functions
ASPELL(spell_armor) {
    struct affected_type af;
    af.type = SPELL_ARMOR;
    af.duration = GET_LEVEL(ch) * 2;
    af.modifier = -20;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);
}

ASPELL(spell_bless) {
    struct affected_type af;
    af.type = SPELL_BLESS;
    af.duration = GET_LEVEL(ch) * 2;
    af.modifier = 5;
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);
}

// ...249 more similar functions
```

**Problems**:
- **Code Duplication**: 90% of buff spells share identical structure
- **Maintenance Burden**: Balance changes require C++ edits + recompilation
- **Testing Difficulty**: Each spell needs separate test coverage
- **Inflexibility**: Cannot create new spells without C++ knowledge

### Modern Solution: Effect Composition System

**Architecture**: Spells as compositions of reusable effect components

```typescript
// Database-driven effect composition
interface Spell {
  id: number;
  name: string;
  effects: SpellEffect[];  // Composable effects
}

interface SpellEffect {
  type: 'AFFECT' | 'DAMAGE' | 'HEALING' | 'SUMMON' | 'TELEPORT';
  config: EffectConfig;
}

// Example: Multi-effect spell
const chillTouch: Spell = {
  name: "chill touch",
  effects: [
    {
      type: 'DAMAGE',
      config: {
        formula: "1d8 + level",
        damageType: "DAM_COLD",
        savingThrow: null
      }
    },
    {
      type: 'AFFECT',
      config: {
        location: "APPLY_STR",
        modifier: -1,
        duration: "level * 2",
        chance: 0.3  // 30% chance to apply
      }
    }
  ]
};
```

**Benefits**:
- **Reusability**: Define effect once, use in multiple spells
- **Composability**: Combine effects to create complex spells
- **Database-Driven**: All configuration in PostgreSQL, no recompilation
- **Rapid Iteration**: Change spell balance via Muditor web UI

### Implementation Example

**Database Schema**:
```prisma
model Spell {
  id              Int            @id @default(autoincrement())
  enumName        String         @unique
  name            String         @unique
  effects         SpellEffect[]  // One-to-many relationship
  castTimeRounds  Float
  violent         Boolean
  sphere          String
  questSpell      Boolean
}

model SpellEffect {
  id          Int     @id @default(autoincrement())
  spellId     Int
  effectType  String  // "DAMAGE", "AFFECT", "HEALING", etc.
  priority    Int     // Execution order

  // Effect configuration (JSON)
  config      Json    // Flexible schema per effect type

  spell       Spell   @relation(fields: [spellId], references: [id])

  @@index([spellId, priority])
}
```

**Effect Configuration Examples**:

```json
// DAMAGE effect
{
  "formula": "level*d6",
  "damageType": "DAM_FIRE",
  "savingThrow": "Reflex",
  "savingThrowEffect": "half",
  "spellResistance": true
}

// AFFECT effect
{
  "location": "APPLY_AC",
  "modifier": -20,
  "duration": "level*2",
  "flag": "AFF_INVISIBLE",
  "dispellable": true
}

// HEALING effect
{
  "formula": "10d8+level",
  "healingType": "HP",
  "overhealCap": 1.0,
  "removeEffects": ["SPELL_POISON", "SPELL_DISEASE"]
}

// SUMMON effect
{
  "mobVnum": 12345,
  "duration": "level*10",
  "level": "caster_level",
  "maxSummons": 1,
  "charmType": "CHARM_SUMMON"
}
```

**C++ Runtime Integration**:
```cpp
void cast_spell(Character* caster, Character* target, int spellnum) {
    // Load spell definition from database
    auto spell = SpellDatabase::getSpell(spellnum);

    // Execute effects in priority order
    for (const auto& effect : spell.effects) {
        switch (effect.type) {
            case EffectType::DAMAGE:
                applyDamageEffect(caster, target, effect.config);
                break;
            case EffectType::AFFECT:
                applyAffectEffect(target, effect.config);
                break;
            case EffectType::HEALING:
                applyHealingEffect(target, effect.config);
                break;
            case EffectType::SUMMON:
                applySummonEffect(caster, effect.config);
                break;
            // ... more effect handlers
        }
    }
}
```

## Problem 2: Fixed Damage/Duration Formulas

### Current System Limitations

Damage and duration calculations are hardcoded into C++ functions:

```cpp
// Legacy: Fireball damage
ASPELL(spell_fireball) {
    int dam = dice(GET_LEVEL(ch), 6);  // Fixed: level d6
    if (mag_savingthrow(victim, SAVING_SPELL, 0))
        dam /= 2;  // Fixed: half damage on save
    damage(ch, victim, dam, SPELL_FIREBALL);
}

// Legacy: Armor duration
ASPELL(spell_armor) {
    af.duration = GET_LEVEL(ch) * 2;  // Fixed: level * 2 rounds
    af.modifier = -20;  // Fixed: -20 AC
}
```

**Problems**:
- **Inflexible**: Cannot adjust formulas without C++ changes
- **Limited Variables**: Only level, no wisdom/intelligence scaling
- **No Conditionals**: Cannot create level-gated effects
- **Testing Burden**: Each formula change requires full regression test

### Modern Solution: Formula Language

**Architecture**: Declarative formula language with variables and conditionals

```typescript
interface FormulaContext {
  // Caster variables
  level: number;
  wisdom: number;
  intelligence: number;
  charisma: number;
  castingStat: number;  // Primary casting stat

  // Target variables
  target_hp: number;
  target_maxhp: number;
  target_level: number;
  target_ac: number;

  // Environment
  room_light: number;
  time_of_day: string;  // "DAY", "NIGHT", "DAWN", "DUSK"
  weather: string;      // "CLEAR", "RAIN", "STORM", "SNOW"

  // Spell context
  spell_circle: number;
  quick_chant_bonus: number;
}

interface Formula {
  expression: string;
  variables: string[];  // Required context variables
}
```

**Formula DSL Examples**:

```javascript
// Simple dice notation
"3d6+10"

// Level-scaled damage
"level*d8 + wisdom/2"

// Conditional damage (more powerful at night)
"time_of_day == 'NIGHT' ? level*d10 : level*d6"

// Percentage-based
"target_maxhp * 0.25"  // Deal 25% of target's max HP

// Multi-variable formula
"(level + castingStat/3) * d6 + spell_circle*2"

// Complex conditional
"level < 10 ? 3d6 : level < 20 ? 5d8 : 10d10"

// Environmental scaling
"weather == 'STORM' ? level*d10 : level*d6"  // Call Lightning

// Duration with caps
"min(level*2 + wisdom/5, 100)"  // Max 100 rounds
```

**Formula Parser Implementation**:

```cpp
class FormulaParser {
public:
    FormulaParser(const std::string& formula) : formula_(formula) {
        parse();
    }

    int evaluate(const FormulaContext& ctx) {
        // Variable substitution
        std::string expr = formula_;
        for (const auto& [var, value] : ctx.variables) {
            replaceAll(expr, var, std::to_string(value));
        }

        // Parse conditionals
        expr = evaluateConditionals(expr);

        // Evaluate dice notation
        expr = evaluateDice(expr);

        // Evaluate mathematical expression
        return evaluateMath(expr);
    }

private:
    std::string formula_;

    int evaluateDice(const std::string& expr) {
        // Match patterns like "3d6", "level*d8"
        std::regex dice_regex(R"((\d+)d(\d+))");
        std::smatch match;

        if (std::regex_search(expr, match, dice_regex)) {
            int num = std::stoi(match[1]);
            int sides = std::stoi(match[2]);
            return rollDice(num, sides);
        }
        return 0;
    }

    std::string evaluateConditionals(const std::string& expr) {
        // Handle ternary operators: condition ? true_val : false_val
        // ...implementation...
    }

    int evaluateMath(const std::string& expr) {
        // Use expression parser library (e.g., TinyExpr)
        // ...implementation...
    }
};
```

**Database Schema**:

```prisma
model SpellFormula {
  id              Int      @id @default(autoincrement())
  spellEffectId   Int
  formulaType     String   // "DAMAGE", "DURATION", "MODIFIER", "HEALING"

  // Formula definition
  expression      String   // "level*d8 + wisdom/2"
  variables       String[] // ["level", "wisdom"]

  // Metadata
  description     String?  // "Fire damage scaled by level and wisdom"
  examples        Json?    // Test cases for validation

  spellEffect     SpellEffect @relation(fields: [spellEffectId], references: [id])
}
```

**Practical Examples**:

```typescript
// Fireball: Scales with level, save for half
{
  formula: "level*d6",
  savingThrow: "Reflex",
  savingThrowFormula: "result/2"  // Half on save
}

// Cure Light: Healing scales with wisdom
{
  formula: "1d8 + level + wisdom/3",
  overhealCap: 1.0
}

// Haste: Duration scales with intelligence
{
  formula: "level*2 + intelligence/4",
  maxDuration: 120  // Cap at 120 rounds
}

// Call Lightning: Weather-dependent damage
{
  formula: "weather == 'STORM' ? level*d10 + 20 : level*d6",
  savingThrow: "Reflex"
}

// Harm: Percentage-based with cap
{
  formula: "min(target_maxhp * 0.75, level*d8 + 50)",
  savingThrow: "Fortitude"
}
```

## Problem 3: Primitive Targeting System

### Current System Limitations

The legacy TAR_* bitflag system lacks nuance:

```cpp
// Legacy targeting flags
#define TAR_CHAR_ROOM      (1 << 1)  // Any character in room
#define TAR_FIGHT_VICT     (1 << 4)  // Combat victim
#define TAR_SELF_ONLY      (1 << 5)  // Only caster
#define TAR_NOT_SELF       (1 << 6)  // Anyone but caster

// Example: Fireball
spell_info[SPELL_FIREBALL].targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
```

**Problems**:
- **No Range**: Cannot specify "within 30 feet" or line-of-sight
- **No Friend/Foe**: Area spells hit everyone or require manual filtering
- **No Positioning**: Cannot target "behind target" or "cone in direction"
- **No Smart Targeting**: Cannot prefer "lowest HP enemy" or "highest threat"
- **No Exclusions**: Cannot exclude invisible/ethereal/immune targets

### Modern Solution: Structured Targeting Schema

**Architecture**: Rich targeting configuration with filters and constraints

```typescript
interface TargetingConfig {
  // Basic scope
  scope: 'SELF' | 'SINGLE' | 'AREA' | 'GROUP' | 'ROOM' | 'WORLD';

  // Range constraints
  range?: number;           // Max distance in feet
  lineOfSight?: boolean;    // Requires unobstructed view

  // Target filtering
  validTargets: {
    allies?: boolean;       // Can target group members
    enemies?: boolean;      // Can target non-group
    self?: boolean;         // Can target caster
    pets?: boolean;         // Can target summoned creatures
    npcs?: boolean;         // Can target NPCs
    players?: boolean;      // Can target players
  };

  // Attribute filters
  alignmentFilter?: 'GOOD' | 'EVIL' | 'NEUTRAL' | null;
  minimumLevel?: number;
  maximumLevel?: number;

  // State filters
  excludeStates?: string[];  // ['DEAD', 'PETRIFIED', 'ETHEREAL']
  requireStates?: string[];  // ['CHARMED', 'SLEEPING']

  // Area configuration (for AREA scope)
  areaType?: 'SPHERE' | 'CONE' | 'LINE' | 'ROOM';
  areaRadius?: number;       // Radius in feet (SPHERE)
  areaAngle?: number;        // Cone angle in degrees (CONE)
  areaLength?: number;       // Line length in feet (LINE)

  // Smart targeting
  smartTarget?: {
    prefer: 'LOWEST_HP' | 'HIGHEST_HP' | 'HIGHEST_THREAT' | 'NEAREST' | 'FARTHEST';
    maxTargets: number;      // For multi-target spells
  };

  // Friendly fire
  friendlyFire: boolean;     // Can hit allies in area effect
}
```

**Practical Examples**:

```typescript
// Fireball: 20-foot radius, hits everyone except group
const fireballTargeting: TargetingConfig = {
  scope: 'AREA',
  areaType: 'SPHERE',
  areaRadius: 20,
  validTargets: {
    allies: false,
    enemies: true,
    npcs: true,
    players: true
  },
  friendlyFire: false,  // Doesn't hit group members
  lineOfSight: true
};

// Cure Light: Touch range, allies only
const cureLightTargeting: TargetingConfig = {
  scope: 'SINGLE',
  range: 5,  // Touch range
  validTargets: {
    allies: true,
    enemies: false,
    self: true
  }
};

// Mass Heal: Entire group, no range limit
const massHealTargeting: TargetingConfig = {
  scope: 'GROUP',
  range: null,  // Unlimited range
  validTargets: {
    allies: true,
    self: true
  }
};

// Sleep: 30-foot radius, enemies only, level limit
const sleepTargeting: TargetingConfig = {
  scope: 'AREA',
  areaType: 'SPHERE',
  areaRadius: 30,
  validTargets: {
    enemies: true,
    allies: false
  },
  maximumLevel: 10,  // Only affects targets ≤ level 10
  smartTarget: {
    prefer: 'HIGHEST_HP',
    maxTargets: 5
  }
};

// Lightning Bolt: 100-foot line
const lightningBoltTargeting: TargetingConfig = {
  scope: 'AREA',
  areaType: 'LINE',
  areaLength: 100,
  validTargets: {
    enemies: true,
    allies: true
  },
  friendlyFire: true,  // Can hit allies in line
  lineOfSight: true
};

// Turn Undead: Room-wide, undead only
const turnUndeadTargeting: TargetingConfig = {
  scope: 'ROOM',
  validTargets: {
    enemies: true,
    npcs: true
  },
  requireStates: ['UNDEAD'],  // Only affects undead
  smartTarget: {
    prefer: 'LOWEST_HP',
    maxTargets: 10
  }
};

// Dispel Magic: Single target, 60 feet, anyone
const dispelMagicTargeting: TargetingConfig = {
  scope: 'SINGLE',
  range: 60,
  validTargets: {
    allies: true,
    enemies: true,
    self: true,
    npcs: true,
    players: true
  },
  lineOfSight: true
};
```

**Database Schema**:

```prisma
model TargetingConfig {
  id                Int      @id @default(autoincrement())
  spellId           Int      @unique

  // Basic scope
  scope             String   // "SELF", "SINGLE", "AREA", etc.
  range             Int?     // Max distance in feet
  lineOfSight       Boolean  @default(false)

  // Target filters (JSON for flexibility)
  validTargets      Json     // { allies: bool, enemies: bool, ... }
  alignmentFilter   String?  // "GOOD", "EVIL", "NEUTRAL"
  levelRange        Json?    // { min: int, max: int }
  stateFilters      Json?    // { exclude: [], require: [] }

  // Area configuration
  areaType          String?  // "SPHERE", "CONE", "LINE", "ROOM"
  areaRadius        Int?
  areaAngle         Int?
  areaLength        Int?

  // Smart targeting
  smartTarget       Json?    // { prefer: string, maxTargets: int }
  friendlyFire      Boolean  @default(false)

  spell             Spell    @relation(fields: [spellId], references: [id])
}
```

## Problem 4: Hardcoded Message Strings

### Current System Limitations

All spell messages are hardcoded in C++:

```cpp
// Legacy: Hardcoded English messages
act("$n vanishes in a flash of light!", TRUE, victim, 0, 0, TO_ROOM);
send_to_char("You feel someone protecting you.\r\n", victim);
act("You cast $t at $N!", FALSE, ch, spell_name, victim, TO_CHAR);
```

**Problems**:
- **No Localization**: English-only, cannot support Spanish/French/etc.
- **Inconsistent Formatting**: No template system, manual string formatting
- **Limited Customization**: Cannot change messages without C++ edits
- **Poor Accessibility**: No support for screen readers or text-to-speech
- **Hard to Test**: Message validation requires manual inspection

### Modern Solution: Message Template System

**Architecture**: Database-driven message templates with localization

```prisma
model SpellMessage {
  id              Int      @id @default(autoincrement())
  spellId         Int
  messageType     String   // "CAST", "HIT", "MISS", "RESIST", "ROOM", "WEAR_OFF"
  perspective     String   // "CASTER", "TARGET", "ROOM"

  // Localized messages
  messageEn       String   // English
  messageEs       String?  // Spanish
  messageFr       String?  // French
  messageDe       String?  // German

  // Template variables
  variables       String[] // ["$caster", "$target", "$damage"]

  // Conditional display
  condition       String?  // "damage > 50", "isCritical"

  spell           Spell    @relation(fields: [spellId], references: [id])

  @@index([spellId, messageType, perspective])
}
```

**Template Variable System**:

```typescript
interface MessageContext {
  // Characters
  $caster: string;         // Caster name
  $caster_subjective: string;  // "he", "she", "they"
  $caster_possessive: string;  // "his", "her", "their"

  $target: string;         // Target name
  $target_subjective: string;
  $target_possessive: string;

  // Spell info
  $spell: string;          // Spell name
  $damage: number;         // Damage dealt
  $healing: number;        // Healing done
  $duration: number;       // Effect duration

  // Combat state
  $isCritical: boolean;    // Critical hit
  $isResisted: boolean;    // Spell resisted
  $isDodged: boolean;      // Attack dodged

  // Environment
  $room: string;           // Room name
  $time: string;           // Time of day
}
```

**Message Examples**:

```typescript
// Fireball messages
const fireballMessages = [
  {
    type: "CAST",
    perspective: "CASTER",
    messageEn: "You hurl a $spell at $target!",
    messageEs: "¡Lanzas un $spell contra $target!",
    messageFr: "Vous lancez un $spell sur $target!"
  },
  {
    type: "HIT",
    perspective: "TARGET",
    messageEn: "You are engulfed in flames! ($damage damage)",
    messageEs: "¡Eres envuelto en llamas! ($damage de daño)",
    messageFr: "Vous êtes englouti par les flammes! ($damage dégâts)",
    condition: "damage > 0"
  },
  {
    type: "HIT",
    perspective: "ROOM",
    messageEn: "$target is engulfed in flames!",
    messageEs: "¡$target es envuelto en llamas!",
    messageFr: "$target est englouti par les flammes!"
  },
  {
    type: "MISS",
    perspective: "CASTER",
    messageEn: "$target dodges your $spell!",
    messageEs: "¡$target esquiva tu $spell!",
    messageFr: "$target esquive votre $spell!"
  },
  {
    type: "RESIST",
    perspective: "CASTER",
    messageEn: "$target resists your $spell!",
    messageEs: "¡$target resiste tu $spell!",
    messageFr: "$target résiste à votre $spell!"
  }
];

// Armor messages
const armorMessages = [
  {
    type: "CAST",
    perspective: "CASTER",
    messageEn: "You cast $spell on $target.",
    messageEs: "Lanzas $spell sobre $target.",
    messageFr: "Vous lancez $spell sur $target."
  },
  {
    type: "HIT",
    perspective: "TARGET",
    messageEn: "You feel someone protecting you.",
    messageEs: "Sientes que alguien te protege.",
    messageFr: "Vous sentez quelqu'un vous protéger."
  },
  {
    type: "WEAR_OFF",
    perspective: "TARGET",
    messageEn: "You feel less protected.",
    messageEs: "Te sientes menos protegido.",
    messageFr: "Vous vous sentez moins protégé."
  }
];
```

**Conditional Messages**:

```typescript
// Critical hit vs normal hit
{
  type: "HIT",
  perspective: "CASTER",
  messageEn: "Your $spell DEVASTATES $target! ($damage damage)",
  condition: "isCritical && damage > 100"
}

{
  type: "HIT",
  perspective: "CASTER",
  messageEn: "Your $spell strikes $target! ($damage damage)",
  condition: "!isCritical"
}

// High damage vs low damage
{
  type: "HIT",
  perspective: "TARGET",
  messageEn: "You are OBLITERATED by $caster_possessive $spell!",
  condition: "damage > target_maxhp * 0.5"  // Over 50% HP
}

{
  type: "HIT",
  perspective: "TARGET",
  messageEn: "You are hit by $caster_possessive $spell.",
  condition: "damage <= target_maxhp * 0.5"
}
```

**Runtime Message Selection**:

```cpp
std::string formatSpellMessage(
    int spellnum,
    MessageType type,
    MessagePerspective perspective,
    const MessageContext& ctx,
    const std::string& locale = "en"
) {
    // Query messages from database
    auto messages = db.query<SpellMessage>(
        "SELECT * FROM SpellMessage WHERE spellId = $1 AND messageType = $2 AND perspective = $3",
        spellnum, messageTypeToString(type), perspectiveToString(perspective)
    );

    // Filter by condition
    for (const auto& msg : messages) {
        if (msg.condition.empty() || evaluateCondition(msg.condition, ctx)) {
            // Select localized message
            std::string template_str = selectLocalized(msg, locale);

            // Substitute variables
            return substituteVariables(template_str, ctx);
        }
    }

    return ""; // No matching message found
}
```

## Problem 5: No Scripting Support

### Current System Limitations

Complex spell behaviors require C++ implementations:

```cpp
// Legacy: 63 MAG_MANUAL spells hardcoded in C++
ASPELL(spell_teleport) {
    // ~50 lines of teleportation logic
    // Room validation, zone checks, random selection
    // Cannot be modified without recompilation
}

ASPELL(spell_charm_person) {
    // ~80 lines of charm mechanics
    // AI control, loyalty shifts, faction updates
    // Requires deep C++ and game engine knowledge
}
```

**Problems**:
- **Inflexible**: All custom logic in C++, no runtime modification
- **High Barrier**: Requires C++ expertise to create spells
- **Slow Iteration**: Compile + test cycle for every change
- **No Community Content**: Players/builders cannot create spells
- **Limited Testing**: Unit testing C++ spell logic is difficult

### Modern Solution: Lua Scripting Integration

**Architecture**: Lua scripting with comprehensive game API

FieryMUD already has Lua integration for room scripts. Extend this for spells:

```lua
-- Spell script: lib/scripts/spells/teleport.lua
function spell_teleport_cast(caster, target, level)
    -- Type checking
    if not target:isCharacter() then
        caster:sendMessage("You can only teleport characters!")
        return SPELL_FAILED
    end

    -- Validation
    if target:hasEffect("EFFECT_NO_TELEPORT") then
        caster:sendMessage(target:getName() .. " cannot be teleported!")
        return SPELL_FAILED
    end

    local room = target:getRoom()
    if room:hasFlag("ROOM_NO_TELEPORT") then
        caster:sendMessage("A magical barrier prevents teleportation!")
        return SPELL_FAILED
    end

    -- Select destination
    local zone = room:getZone()
    local validRooms = zone:getRooms({
        excludeFlags = {"PRIVATE", "NO_MOB", "DEATH"},
        levelRange = {
            min = math.max(1, level - 5),
            max = level + 10
        },
        excludeRooms = {room:getVnum()}  -- Can't teleport to same room
    })

    if #validRooms == 0 then
        caster:sendMessage("The spell fails to find a suitable destination.")
        return SPELL_FAILED
    end

    -- Random selection
    local destRoom = validRooms[math.random(#validRooms)]

    -- Execute teleport
    target:teleportTo(destRoom, {
        messages = {
            self = "You vanish in a flash of light!",
            room = target:getName() .. " vanishes in a flash of light!",
            dest = target:getName() .. " appears in a flash of light!"
        },
        effects = {
            {
                type = "DISORIENTED",
                duration = 2,
                message = "You feel disoriented from the magical journey."
            }
        }
    })

    return SPELL_SUCCESS
end
```

**Lua Spell API**:

```lua
-- Character operations
character:getName()
character:getLevel()
character:getHP()
character:getMaxHP()
character:getMana()
character:getMaxMana()
character:getRoom()
character:isPlayer()
character:isNPC()
character:hasEffect(effectName)
character:applyEffect(effectConfig)
character:removeEffect(effectName)
character:sendMessage(text, color?)
character:teleportTo(room, config)
character:dealDamage(amount, damageType, attacker)
character:heal(amount)

-- Room operations
room:getName()
room:getDescription()
room:getVnum()
room:getZone()
room:hasFlag(flagName)
room:getCharacters(filter?)
room:getObjects(filter?)
room:getExits()
room:sendMessage(text, excludeChar?)

-- World queries
world:getRoomByVnum(vnum)
world:getCharacterByName(name)
world:getRandomRoom(filter)
world:findPath(fromRoom, toRoom)

-- Zone operations
zone:getName()
zone:getVnum()
zone:getRooms(filter?)
zone:getRandomRoom(filter)

-- Object operations
object:getName()
object:getVnum()
object:getType()
object:getValue(index)
object:hasFlag(flagName)
object:getWeight()

-- Combat utilities
combat:dealDamage(attacker, victim, amount, damageType)
combat:makeSavingThrow(character, saveType, dc)
combat:checkSpellResistance(caster, target, spellLevel)
combat:getTargetsInArea(room, areaConfig)

-- Effect utilities
effects:create(effectConfig)
effects:apply(character, effect)
effects:remove(character, effectType)
effects:getActive(character)

-- Dice utilities
dice:roll(num, sides)
dice:rollFormula(formula)  -- e.g., "3d6+10"
dice:random(min, max)

-- Math utilities
math:clamp(value, min, max)
math:lerp(a, b, t)
math:distance(pos1, pos2)
```

**Complex Spell Examples**:

```lua
-- Charm Person: Complex AI control
function spell_charm_person_cast(caster, target, level)
    -- Validation
    if not target:isNPC() then
        caster:sendMessage("You can only charm NPCs!")
        return SPELL_FAILED
    end

    if target:hasEffect("EFFECT_CHARMED") then
        caster:sendMessage(target:getName() .. " is already charmed!")
        return SPELL_FAILED
    end

    -- Saving throw
    local dc = 10 + level + caster:getCharisma() / 2
    if combat:makeSavingThrow(target, "Will", dc) then
        caster:sendMessage(target:getName() .. " resists your charm!")
        return SPELL_FAILED
    end

    -- Apply charm
    target:applyEffect({
        type = "CHARMED",
        duration = level * 10,
        caster = caster,
        onWearOff = function()
            target:sendMessage("You regain control of your actions.")
            target:setHostileTo(caster)  -- Become hostile when charm ends
        end
    })

    -- Update faction
    target:setFaction(caster:getFaction())
    target:setMaster(caster)
    target:joinGroup(caster:getGroup())

    -- Messages
    caster:sendMessage("You charm " .. target:getName() .. "!")
    target:sendMessage("You are charmed by " .. caster:getName() .. "!")

    return SPELL_SUCCESS
end

-- Summon: Creature creation with AI
function spell_summon_cast(caster, args, level)
    local targetName = args.targetName
    local target = world:getCharacterByName(targetName)

    if not target then
        caster:sendMessage("Nobody by that name is in the world.")
        return SPELL_FAILED
    end

    if target == caster then
        caster:sendMessage("You cannot summon yourself!")
        return SPELL_FAILED
    end

    -- Permission checks
    if target:isImmortal() and not caster:isImmortal() then
        caster:sendMessage("You cannot summon immortals!")
        return SPELL_FAILED
    end

    if target:hasEffect("EFFECT_NO_SUMMON") then
        caster:sendMessage(target:getName() .. " cannot be summoned!")
        return SPELL_FAILED
    end

    -- Saving throw
    local dc = 10 + level + caster:getWisdom() / 2
    if combat:makeSavingThrow(target, "Will", dc) then
        caster:sendMessage(target:getName() .. " resists your summon!")
        target:sendMessage("You resist " .. caster:getName() .. "'s summon attempt!")
        return SPELL_FAILED
    end

    -- Execute summon
    local casterRoom = caster:getRoom()
    target:teleportTo(casterRoom, {
        messages = {
            self = "You are summoned by " .. caster:getName() .. "!",
            room = target:getName() .. " appears with a blinding flash!",
            dest = target:getName() .. " arrives from nowhere!"
        },
        bypassRestrictions = true  -- Summon overrides no-teleport
    })

    caster:sendMessage("You summon " .. target:getName() .. "!")

    return SPELL_SUCCESS
end

-- Identify: Complex item analysis
function spell_identify_cast(caster, target, level)
    if not target:isObject() then
        caster:sendMessage("You can only identify objects!")
        return SPELL_FAILED
    end

    -- Display object information
    local info = {
        "Object: " .. target:getName(),
        "Type: " .. target:getType(),
        "Weight: " .. target:getWeight() .. " lbs",
        "Value: " .. target:getValue(0) .. " gold",
        ""
    }

    -- Type-specific information
    if target:getType() == "WEAPON" then
        table.insert(info, "Damage: " .. target:getValue(1) .. "d" .. target:getValue(2))
        table.insert(info, "Weapon Type: " .. target:getWeaponType())
    elseif target:getType() == "ARMOR" then
        table.insert(info, "AC Bonus: " .. target:getValue(1))
    end

    -- Magical properties
    local effects = target:getEffects()
    if #effects > 0 then
        table.insert(info, "")
        table.insert(info, "Magical Properties:")
        for _, effect in ipairs(effects) do
            table.insert(info, "  " .. effect:getDescription())
        end
    end

    -- Send formatted output
    caster:sendMessage(table.concat(info, "\n"))

    return SPELL_SUCCESS
end
```

**Database Integration**:

```prisma
model Spell {
  id            Int      @id @default(autoincrement())
  enumName      String   @unique
  name          String   @unique

  // Scripting
  scriptFile    String?  // "spells/teleport.lua"
  scriptEnabled Boolean  @default(false)

  // Fallback to C++ if script fails
  cppHandler    String?  // "spell_teleport" function name

  // Script configuration
  scriptConfig  Json?    // Custom config passed to script
}
```

## Problem 6: No Balance Versioning

### Current System Limitations

**Problems**:
- **No History**: Cannot track balance changes over time
- **No Rollback**: Cannot revert problematic changes
- **No A/B Testing**: Cannot test multiple balance variants
- **No Telemetry**: No data on spell effectiveness
- **No Evidence**: Balance decisions based on anecdotes, not data

### Modern Solution: Versioned Spell System with Telemetry

**Architecture**: Spell versioning with A/B testing and metrics collection

```prisma
model SpellVersion {
  id              Int      @id @default(autoincrement())
  spellId         Int
  version         Int      // Sequential version number

  // Configuration snapshot
  config          Json     // Complete spell configuration

  // Versioning metadata
  createdBy       Int      // User who created this version
  createdAt       DateTime @default(now())
  reason          String   // "Reduced damage for PvP balance"

  // Activation status
  isActive        Boolean  @default(false)
  activatedAt     DateTime?
  deactivatedAt   DateTime?

  // A/B testing
  testGroup       String?  // "control", "variant_a", "variant_b"
  testPercentage  Float?   // 0.0-1.0 (percentage of players)

  // Relationships
  spell           Spell    @relation(fields: [spellId], references: [id])
  creator         User     @relation(fields: [createdBy], references: [id])
  metrics         SpellMetrics[]

  @@unique([spellId, version])
  @@index([spellId, isActive])
}

model SpellMetrics {
  id              Int      @id @default(autoincrement())
  spellVersionId  Int

  // Usage metrics
  castCount       Int      @default(0)
  successRate     Float    // 0.0-1.0

  // Combat metrics
  totalDamage     BigInt   @default(0)
  averageDamage   Float
  criticalRate    Float

  // Effectiveness metrics
  killsWithSpell  Int      @default(0)
  deathsWithSpell Int      @default(0)
  pvpWinRate      Float?   // PvP correlation

  // Resource efficiency
  manaCost        Int
  damagePerMana   Float    // Efficiency metric

  // Time period
  periodStart     DateTime
  periodEnd       DateTime

  // Relationships
  version         SpellVersion @relation(fields: [spellVersionId], references: [id])

  @@index([spellVersionId, periodStart])
}
```

**Versioning Workflow**:

1. **Create Version**: Balance designer creates new spell configuration
2. **Deploy as Test**: Activate for 10% of players (A/B test)
3. **Collect Metrics**: Track usage, damage, win rate for 1 week
4. **Analyze Results**: Compare metrics between control and test groups
5. **Promote or Rollback**: Deploy to 100% if successful, or rollback if problematic

**Example: Fireball Balance Change**:

```typescript
// Version 1: Current balance (control group)
{
  spellId: SPELL_FIREBALL,
  version: 1,
  config: {
    formula: "level*d6",
    savingThrow: "Reflex",
    savingThrowEffect: "half"
  },
  isActive: true,
  testGroup: "control",
  testPercentage: 0.9  // 90% of players
}

// Version 2: Nerfed damage (test group)
{
  spellId: SPELL_FIREBALL,
  version: 2,
  config: {
    formula: "level*d6 - 10",  // Reduced damage
    savingThrow: "Reflex",
    savingThrowEffect: "half"
  },
  isActive: true,
  testGroup: "variant_a",
  testPercentage: 0.1,  // 10% of players
  reason: "Reduce damage for PvP balance"
}

// After 1 week of data collection:
// Control group (v1): 15.2 avg damage per cast, 35% PvP win rate
// Test group (v2): 12.8 avg damage per cast, 33% PvP win rate
// Decision: Nerf too harsh, try smaller reduction
```

**Telemetry Collection**:

```cpp
void recordSpellCast(int spellnum, Character* caster, Character* target, int damage, bool success) {
    auto version = getActiveSpellVersion(spellnum, caster->getPlayerID());

    // Record metrics asynchronously
    TelemetryService::recordAsync({
        .spellVersionId = version->id,
        .casterId = caster->getPlayerID(),
        .targetId = target ? target->getPlayerID() : 0,
        .damage = damage,
        .success = success,
        .castTime = getCurrentTime(),
        .manaCost = getSpellManaCost(spellnum, caster->getLevel())
    });
}
```

**Balance Dashboard Queries**:

```sql
-- Average damage per spell over last 7 days
SELECT
    s.name,
    sv.version,
    AVG(sm.averageDamage) as avg_damage,
    SUM(sm.castCount) as total_casts,
    AVG(sm.successRate) as success_rate,
    AVG(sm.pvpWinRate) as pvp_correlation
FROM SpellMetrics sm
JOIN SpellVersion sv ON sm.spellVersionId = sv.id
JOIN Spell s ON sv.spellId = s.id
WHERE sm.periodStart >= NOW() - INTERVAL '7 days'
GROUP BY s.name, sv.version
ORDER BY total_casts DESC;

-- Spell usage distribution (identify overpowered spells)
WITH spell_usage AS (
    SELECT
        s.name,
        SUM(sm.castCount) as total_casts,
        SUM(sm.totalDamage) as total_damage
    FROM SpellMetrics sm
    JOIN SpellVersion sv ON sm.spellVersionId = sv.id
    JOIN Spell s ON sv.spellId = s.id
    WHERE sm.periodStart >= NOW() - INTERVAL '30 days'
    GROUP BY s.name
)
SELECT
    name,
    total_casts,
    total_casts * 100.0 / SUM(total_casts) OVER() as cast_percentage,
    total_damage,
    total_damage * 100.0 / SUM(total_damage) OVER() as damage_percentage
FROM spell_usage
ORDER BY cast_percentage DESC;
```

## Migration Path

### Phase 1: Effect Composition (Weeks 1-4)

**Goal**: Migrate 52 fully configurable spells

**Steps**:
1. Implement `SpellEffect` schema
2. Create effect application system in C++
3. Migrate simple MAG_AFFECT spells
4. Add Muditor UI for effect configuration
5. Test all migrated spells

**Deliverables**:
- [ ] `SpellEffect` table with JSON config
- [ ] C++ effect application handlers
- [ ] 52 spells database-driven
- [ ] Unit tests for effect system

### Phase 2: Formula Engine (Weeks 5-10)

**Goal**: Add formula parser and migrate damage/healing spells

**Steps**:
1. Design formula DSL syntax
2. Implement formula parser (C++)
3. Add variable substitution system
4. Extend schema with formula fields
5. Migrate 136 MAG_DAMAGE/MAG_POINT spells
6. Add formula editor in Muditor

**Deliverables**:
- [ ] Formula parser library
- [ ] `SpellFormula` table
- [ ] 136 additional spells database-driven
- [ ] Formula validation tests

### Phase 3: Targeting System (Weeks 11-14)

**Goal**: Replace TAR_* flags with structured targeting

**Steps**:
1. Design targeting config schema
2. Implement targeting resolution system
3. Add area effect calculation
4. Migrate all spells to new system
5. Add targeting editor in Muditor

**Deliverables**:
- [ ] `TargetingConfig` table
- [ ] C++ targeting resolution
- [ ] All spells use new targeting
- [ ] Targeting visualizer in Muditor

### Phase 4: Message Templates (Weeks 15-18)

**Goal**: Replace hardcoded strings with templates

**Steps**:
1. Design message template schema
2. Extract all spell messages from C++
3. Implement template rendering
4. Add localization support (es, fr, de)
5. Create message editor in Muditor

**Deliverables**:
- [ ] `SpellMessage` table with localization
- [ ] Template rendering system
- [ ] All spells use templates
- [ ] Translation workflow in Muditor

### Phase 5: Lua Scripting (Weeks 19-26)

**Goal**: Enable scripting for complex spells

**Steps**:
1. Design Lua spell API
2. Implement API bindings (C++ ↔ Lua)
3. Port 10 complex MAG_MANUAL spells to Lua
4. Create scripting documentation
5. Add Lua editor in Muditor
6. Community beta testing

**Deliverables**:
- [ ] Lua spell API (200+ functions)
- [ ] 10 spells fully scriptable
- [ ] Lua debugging tools
- [ ] Scripting guide for builders

### Phase 6: Balance Versioning (Weeks 27-32)

**Goal**: Enable A/B testing and telemetry

**Steps**:
1. Implement spell versioning system
2. Add telemetry collection
3. Create balance dashboard
4. Deploy A/B testing for 5 spells
5. Document balance workflow

**Deliverables**:
- [ ] `SpellVersion` and `SpellMetrics` tables
- [ ] Telemetry pipeline
- [ ] Balance dashboard in Muditor
- [ ] A/B testing framework

## Success Metrics

### Technical Metrics

- **Configurability**: 90%+ of spells database-driven (target: 227/251)
- **Performance**: <5% increase in spell casting latency
- **Code Reduction**: 70% reduction in spell-related C++ code (target: -15,000 LOC)
- **Test Coverage**: 95%+ coverage for spell system components

### Gameplay Metrics

- **Balance Diversity**: No single spell >10% of total damage dealt
- **Spell Usage**: 80%+ of spells cast at least once per week
- **Player Satisfaction**: <5% of spells flagged as overpowered/useless in surveys
- **Bug Rate**: <1 spell bug per 1000 player-hours

### Development Metrics

- **Balance Iteration Speed**: 10x faster (database edit vs. C++ recompile)
- **New Spell Creation**: 5x faster (1 hour vs. 5 hours)
- **Community Content**: 10+ community-created spells per month
- **Localization**: 3+ languages supported (en, es, fr)

## Conclusion

The proposed spell system modernization addresses six critical limitations of the legacy CircleMUD architecture: monolithic implementations, fixed formulas, primitive targeting, hardcoded messages, no scripting, and ossified balance. By adopting database-driven configuration, formula languages, structured targeting, message templates, Lua scripting, and versioned balance testing, FieryMUD can achieve:

1. **90% reduction** in spell maintenance effort
2. **10x faster** balance iteration
3. **Full community** content creation capability
4. **Evidence-based** game design through telemetry
5. **International** player support via localization

The phased 32-week migration path provides a structured approach with clear deliverables, minimizing risk while maximizing benefit. Each phase builds on the previous, allowing for early wins (Phase 1: 52 spells in 4 weeks) while establishing the foundation for advanced features (Phase 6: A/B testing).

This modernization positions FieryMUD as a cutting-edge MUD with data-driven design, rapid iteration capability, and community-driven content expansion—critical advantages in the competitive landscape of modern multiplayer games.

---

*Document Version*: 1.0
*Last Updated*: 2025-11-06
*Migration Timeline*: 32 weeks
*Expected ROI*: 10x development efficiency improvement

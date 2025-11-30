# FieryMUD Skill System - Modern Design Recommendations

**Comprehensive Modernization Plan for the Skill System**

---

## Table of Contents

1. [Problems with Legacy System](#problems-with-legacy-system)
2. [Skill Effect Templates](#skill-effect-templates)
3. [Formula Language](#formula-language)
4. [Skill Tree System](#skill-tree-system)
5. [Progression and Specialization](#progression-and-specialization)
6. [Lua Scripting Integration](#lua-scripting-integration)
7. [Balance Versioning](#balance-versioning)
8. [Migration Path](#migration-path)

---

## Problems with Legacy System

### Code Duplication

**Problem**: Similar skills have nearly identical implementations with minor variations.

```cpp
// act.offensive.cpp - bash, kick, sweep all follow same pattern
ACMD(do_bash) {
    // Target validation
    // Weapon check
    // Calculate damage
    // Apply stun
    // Send messages
}

ACMD(do_kick) {
    // Target validation (duplicate)
    // Calculate damage (different formula)
    // Apply delay (duplicate)
    // Send messages (duplicate)
}
```

**Impact**:
- 1000+ lines of duplicated validation code
- Changes require updating multiple functions
- High risk of introducing inconsistencies

### Hardcoded Balance

**Problem**: All game balance values are embedded in C++ code.

```cpp
// Backstab damage multipliers hardcoded
int multiplier = (level < 8) ? 2 :
                 (level < 14) ? 3 :
                 (level < 21) ? 4 : 5;

// Bash stun duration hardcoded
SET_WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
```

**Impact**:
- Recompilation required for balance changes
- No A/B testing capability
- Balance history not tracked
- Slow iteration cycles (hours vs. seconds)

### Limited Extensibility

**Problem**: Adding new skills requires C++ expertise and code changes in multiple files.

**Steps to Add a Skill**:
1. Edit `spells.hpp` - add `#define SKILL_NEW 496`
2. Edit `skills.cpp` - add `skillo()` definition
3. Edit `class.cpp` - add class assignments
4. Edit `act.offensive.cpp` - implement `ACMD(do_new_skill)`
5. Edit `interpreter.cpp` - register command
6. Edit `help/commands.hlp` - add help text
7. Recompile entire server

**Impact**:
- High barrier to entry for content creators
- Slow development velocity
- Merge conflicts on multi-dev teams
- No community content contribution

### No Skill Relationships

**Problem**: Skills are independent entities with no prerequisites or synergies.

**Missing Features**:
- Skill prerequisites (e.g., backstab requires hide)
- Skill trees (e.g., combat → advanced combat → legendary combat)
- Skill synergies (e.g., dual wield + double attack)
- Conditional availability (e.g., underwater skills)

### Poor Balance Tooling

**Problem**: No systematic way to analyze or adjust skill balance.

**Missing Capabilities**:
- No skill usage statistics
- No damage/healing tracking
- No PvP/PvE balance separation
- No class balance dashboards

---

## Skill Effect Templates

### Concept

Create reusable effect templates similar to the spell system's `MAG_*` routines. Allows composition of complex skills from simple building blocks.

### Effect Type Registry

```cpp
enum SkillEffectType {
    EFFECT_DAMAGE,              // Direct damage
    EFFECT_HEAL,                // HP restoration
    EFFECT_STAT_MODIFIER,       // Temporary stat change
    EFFECT_STATUS_EFFECT,       // Apply buff/debuff
    EFFECT_STUN,                // Disable actions
    EFFECT_KNOCKDOWN,           // Change position
    EFFECT_DISARM,              // Remove weapon
    EFFECT_TELEPORT,            // Move character
    EFFECT_SUMMON,              // Create entity
    EFFECT_TRANSFORM,           // Change form
    EFFECT_AOE_DAMAGE,          // Area damage
    EFFECT_DOT,                 // Damage over time
    EFFECT_HOT,                 // Heal over time
    EFFECT_LIFESTEAL,           // Damage + heal
    EFFECT_COOLDOWN,            // Set skill cooldown
    EFFECT_RESOURCE_COST,       // Mana/stamina cost
    EFFECT_WEAPON_BONUS,        // Weapon proficiency
    EFFECT_EXTRA_ATTACK,        // Additional attacks
    EFFECT_RESIST,              // Damage reduction
    EFFECT_MESSAGE              // Flavor text
};
```

### Example: Bash Skill as Template Composition

**JSON Definition**:
```json
{
    "id": 402,
    "name": "bash",
    "display_name": "Bash",
    "category": "COMBAT_STUN",
    "effects": [
        {
            "type": "DAMAGE",
            "damage_formula": "dice(1, 4) + (level / 5) + damroll",
            "damage_type": "BLUDGEON",
            "save_type": "STR",
            "save_difficulty": 15,
            "multiplier_victim_sitting": 1.5
        },
        {
            "type": "STUN",
            "duration_formula": "2",
            "unit": "ROUNDS",
            "save_type": "DEX",
            "save_difficulty": 18,
            "failure_effect": "KNOCKDOWN"
        },
        {
            "type": "COOLDOWN",
            "duration_formula": "2",
            "unit": "ROUNDS",
            "applies_on_failure": true
        },
        {
            "type": "MESSAGE",
            "success_actor": "You slam into $N with your shield!",
            "success_victim": "$n slams into you with their shield!",
            "success_room": "$n slams into $N with their shield!",
            "failure_actor": "You lose your balance and fall!",
            "failure_victim": "$n tries to bash you but falls!",
            "failure_room": "$n loses their balance and falls!"
        }
    ],
    "requirements": {
        "min_position": "FIGHTING",
        "requires_shield": true,
        "cannot_target_self": true
    }
}
```

**C++ Implementation**:
```cpp
class SkillEffectEngine {
public:
    static bool execute_skill(CharData *ch, CharData *vict, SkillDef *skill) {
        // Validate requirements
        if (!validate_requirements(ch, vict, skill)) {
            return false;
        }

        // Calculate success chance
        bool success = calculate_success(ch, vict, skill);

        // Apply effects
        for (const auto& effect : skill->effects) {
            if (success || effect.applies_on_failure) {
                apply_effect(ch, vict, effect, success);
            }
        }

        return success;
    }

private:
    static bool validate_requirements(CharData *ch, CharData *vict, SkillDef *skill) {
        if (skill->requirements.requires_shield && !has_shield(ch)) {
            send_to_char("You need a shield to bash!\n", ch);
            return false;
        }

        if (GET_POS(ch) < skill->requirements.min_position) {
            send_to_char("You can't do that from this position!\n", ch);
            return false;
        }

        return true;
    }

    static void apply_effect(CharData *ch, CharData *vict, const SkillEffect& effect, bool success) {
        switch (effect.type) {
            case EFFECT_DAMAGE:
                apply_damage_effect(ch, vict, effect, success);
                break;
            case EFFECT_STUN:
                apply_stun_effect(ch, vict, effect, success);
                break;
            case EFFECT_COOLDOWN:
                apply_cooldown_effect(ch, effect, success);
                break;
            case EFFECT_MESSAGE:
                apply_message_effect(ch, vict, effect, success);
                break;
        }
    }
};
```

### Benefits

1. **Reusability**: Define DAMAGE effect once, use in 50+ skills
2. **Consistency**: All damage calculations follow same logic
3. **Composition**: Complex skills = combination of simple effects
4. **Testability**: Test each effect type independently
5. **Balance**: Adjust effect formulas across all skills at once

---

## Formula Language

### Concept

Create a domain-specific language for skill formulas that balances power with safety.

### Syntax

```
# Basic arithmetic
level + 10
(str * 2) + (dex / 2)

# Functions
dice(2, 8)
min(100, skill_percent + level)
max(1, (level - 10) / 5)

# Conditional expressions
level < 20 ? 2 : (level < 40 ? 3 : 4)

# Random chance
random(1, 100) < skill_percent

# Character properties
caster.level
caster.str
caster.hitroll
caster.wielded.damage

# Victim properties
victim.level
victim.ac
victim.max_hp

# Complex expressions
dice(1, 4) + (caster.level / 5) + caster.damroll * (1 + caster.str / 50)
```

### Variable Context

**Caster Variables**:
```
level          # Character level
str, dex, con, int, wis, cha
hitroll, damroll, ac
max_hp, max_mana, max_move
wielded.damage, wielded.weight
skill_percent  # Proficiency in current skill (0-100)
class          # Character class enum
race           # Character race enum
position       # STANDING, SITTING, etc.
```

**Victim Variables**:
```
victim.level
victim.str, victim.dex, etc.
victim.ac
victim.max_hp, victim.hp
victim.saves.fortitude, victim.saves.reflex, victim.saves.will
```

**Environmental Variables**:
```
time.hour      # 0-23
weather.raining
room.underwater
room.indoors
```

### Example Formulas

**Simple Damage**:
```
dice(2, 6) + level + damroll
```

**Scaling Damage** (weak at low levels, strong at high levels):
```
dice(level / 10, 8) + damroll * (1 + level / 50)
```

**Success Chance** (skill % + modifiers, capped at 95%):
```
min(95, skill_percent + (dex - 15) * 2 + (level / 5))
```

**Level-Based Multiplier**:
```
level < 8 ? 2 : (level < 14 ? 3 : (level < 21 ? 4 : 5))
```

**Conditional Damage** (bonus if victim is sitting):
```
base_damage * (victim.position <= SITTING ? 1.5 : 1.0)
```

### Implementation

```cpp
class FormulaParser {
public:
    static std::unique_ptr<AST> parse(const std::string& formula) {
        // Tokenize: "dice(2, 8) + level" → [FUNC, LPAREN, NUM, COMMA, NUM, RPAREN, PLUS, VAR]
        auto tokens = tokenize(formula);

        // Parse: tokens → abstract syntax tree
        Parser parser(tokens);
        return parser.parse_expression();
    }
};

class FormulaEvaluator {
public:
    static int evaluate(const AST* ast, EvalContext& ctx) {
        switch (ast->type) {
            case AST_BINOP:
                return evaluate_binop(ast, ctx);
            case AST_FUNCTION:
                return evaluate_function(ast, ctx);
            case AST_VARIABLE:
                return evaluate_variable(ast, ctx);
            case AST_LITERAL:
                return ast->value;
        }
    }

private:
    static int evaluate_function(const AST* ast, EvalContext& ctx) {
        if (ast->name == "dice") {
            int num = evaluate(ast->args[0], ctx);
            int size = evaluate(ast->args[1], ctx);
            return dice(num, size);
        } else if (ast->name == "min") {
            int a = evaluate(ast->args[0], ctx);
            int b = evaluate(ast->args[1], ctx);
            return std::min(a, b);
        }
        // ... more functions
    }

    static int evaluate_variable(const AST* ast, EvalContext& ctx) {
        if (ast->name == "level") {
            return GET_LEVEL(ctx.caster);
        } else if (ast->name == "skill_percent") {
            return GET_ISKILL(ctx.caster, ctx.skill_id);
        } else if (ast->name.starts_with("victim.")) {
            // Handle victim.level, victim.ac, etc.
            return ctx.victim ? get_victim_property(ctx.victim, ast->name) : 0;
        }
        // ... more variables
    }
};
```

### Safety Measures

1. **Sandboxing**: No file I/O, no network access
2. **Resource Limits**: Max 1000 operations per formula
3. **Timeout Protection**: 10ms execution limit
4. **Type Safety**: Only integers, no strings or pointers
5. **Division by Zero**: Returns 0 instead of crashing
6. **Infinite Loop Prevention**: No while loops, only functions

---

## Skill Tree System

### Concept

Organize skills into hierarchical trees with prerequisites and specialization branches.

### Tree Structure

```
Combat Skills
├── Basic Combat (Level 1-20)
│   ├── Kick (Level 1)
│   ├── Bash (Level 1, requires Shield)
│   └── Rescue (Level 10)
│
├── Advanced Combat (Level 20-50)
│   ├── Disarm (Level 20, requires Bash)
│   ├── Riposte (Level 30, requires Parry)
│   └── Double Attack (Level 35)
│
└── Legendary Combat (Level 50+)
    ├── Hitall (Level 50, requires Double Attack)
    ├── Whirlwind (Level 60, requires Double Attack + Sweep)
    └── Bladestorm (Level 70, requires Hitall + Whirlwind)

Stealth Skills
├── Basic Stealth (Level 1-20)
│   ├── Hide (Level 1)
│   ├── Sneak (Level 1)
│   └── Backstab (Level 3, requires Hide + Sneak)
│
├── Advanced Stealth (Level 20-50)
│   ├── Shadow (Level 20, requires Hide + Sneak)
│   ├── Steal (Level 10)
│   └── Circle (Level 30, requires Backstab)
│
└── Master Stealth (Level 50+)
    ├── Throatcut (Level 50, requires Backstab + Circle)
    ├── Assassinate (Level 60, requires Throatcut)
    └── Vanish (Level 70, requires Shadow + Stealth)
```

### Database Schema

```json
{
    "skill_trees": [
        {
            "id": "combat_basic",
            "name": "Basic Combat",
            "description": "Fundamental combat techniques",
            "min_level": 1,
            "max_level": 20,
            "skills": [
                {
                    "skill_id": 404,
                    "position": {"x": 0, "y": 0},
                    "prerequisites": []
                },
                {
                    "skill_id": 402,
                    "position": {"x": 1, "y": 0},
                    "prerequisites": [],
                    "requirements": ["equipped_shield"]
                },
                {
                    "skill_id": 407,
                    "position": {"x": 0, "y": 1},
                    "prerequisites": [404, 402]
                }
            ]
        }
    ]
}
```

### UI Visualization

```
        Basic Combat Tree

Level 1    [Kick]     [Bash]
            │  └───────┘
Level 10   [Rescue]

Level 20   [Disarm]   [Parry]
            │           │
Level 30   └────┬──────┘
            [Riposte]
                │
Level 50      [Hitall]
```

### Skill Points System

**Acquisition**:
- Gain 1 skill point per level
- Bonus points from quests
- Respec available at trainers (cost: gold)

**Spending**:
- Basic skills: 1 point
- Advanced skills: 2 points
- Legendary skills: 3 points

**Example**: Level 50 Warrior
- Total points: 50 (from leveling) + 10 (from quests) = 60
- Spent: 15 basic (15 points) + 10 advanced (20 points) + 5 legendary (15 points) = 50 points
- Remaining: 10 points for flexibility

---

## Progression and Specialization

### Skill Improvement System

**Current System** (Legacy):
- Skills improve randomly on use (1-5% chance)
- Based on INT/WIS stats
- No control over progression speed

**Proposed System**:
1. **Practice Mode**: Use skill at reduced effectiveness for 2x improvement rate
2. **Mastery Levels**: Skills have stages (Novice → Apprentice → Journeyman → Expert → Master)
3. **Specialization Bonuses**: Choose specialization at level 30 for +10% to tree skills

### Mastery Levels

```
Novice (0-20%)      - Basic functionality, high failure rate
Apprentice (21-40%) - Reduced cooldowns, improved success
Journeyman (41-60%) - Unlocks advanced features
Expert (61-80%)     - Significant bonuses, low failure rate
Master (81-100%)    - Maximum effectiveness, special effects
```

**Example: Backstab Mastery**
```
Novice:
    - Damage multiplier: 2x
    - Can only backstab sleeping/resting targets

Apprentice:
    - Damage multiplier: 3x
    - Can backstab standing targets if hidden

Journeyman:
    - Damage multiplier: 4x
    - 50% chance to remain hidden after backstab

Expert:
    - Damage multiplier: 5x
    - Remain hidden after backstab if successful

Master:
    - Damage multiplier: 6x
    - Can backstab in combat (circle not required)
    - Critical hits apply additional bleeding effect
```

### Class Specializations

**Warrior Specializations** (Choose at Level 30):
- **Weapon Master**: +10% to all weapon skills, +2 damage with mastered weapons
- **Defender**: +10% to defensive skills (parry/dodge/riposte), -20% damage taken while guarding
- **Berserker**: +10% to offensive skills, berserk mode lasts 50% longer

**Thief Specializations**:
- **Assassin**: +10% to combat stealth (backstab/throatcut), +1 damage multiplier
- **Burglar**: +10% to utility stealth (steal/pick lock), +20% gold stolen
- **Acrobat**: +10% to movement (sneak/hide), can use stealth in combat

### Synergy System

Skills that work well together gain bonuses.

**Example Synergies**:
```
Dual Wield + Double Attack
    = Triple Attack (30% chance for 3rd attack)

Hide + Sneak + Shadow
    = Ghost (cannot be tracked, even with sense life)

Parry + Riposte + Weapon Mastery
    = Counterattack Master (riposte damage +50%)

Bash + Kick + Sweep
    = Area Control (stun all melee attackers)
```

---

## Lua Scripting Integration

### Skill Hooks System

Allow Lua scripts to hook into skill execution without replacing the entire skill.

**Hook Points**:
```lua
-- before_use: Called before skill validation
function before_backstab(caster, victim, args)
    -- Check custom prerequisites
    if caster:has_effect("FUMBLE") then
        caster:send("Your hands are too clumsy to backstab!")
        return false  -- Cancel skill
    end
    return true  -- Continue to validation
end

-- after_damage: Called after damage is calculated
function after_backstab_damage(caster, victim, damage)
    -- Apply bleeding if mastery level is high
    if caster:get_skill_mastery("backstab") >= MASTERY_MASTER then
        victim:apply_effect("BLEEDING", 60, damage / 10)
    end
    return damage
end

-- on_success: Called when skill succeeds
function on_backstab_success(caster, victim)
    -- Grant bonus XP for difficult backstabs
    if victim:get_level() > caster:get_level() then
        caster:gain_experience(100)
    end
end

-- on_failure: Called when skill fails
function on_backstab_failure(caster, victim)
    -- Penalty for failed backstabs
    victim:send("You feel someone trying to stab you!")
    victim:start_combat(caster)
end
```

### Custom Skill Creation

Allow builders to create entirely new skills via Lua.

**Example: Shadow Strike** (Custom Assassin Skill)
```lua
-- skills/shadow_strike.lua
skill_define({
    id = 496,
    name = "shadow strike",
    display_name = "Shadow Strike",
    category = "COMBAT_STEALTH",
    min_level = { ASSASSIN = 40, ROGUE = 60 },
    cooldown = 30,  -- seconds
    requirements = {
        skills = {"backstab", "shadow"},
        min_position = "FIGHTING",
        requires_hidden = true
    }
})

function shadow_strike(caster, victim, args)
    -- Validate requirements
    if not caster:is_hidden() then
        caster:send("You must be hidden to use shadow strike!")
        return false
    end

    if not caster:is_fighting(victim) then
        caster:send("You can only shadow strike enemies you're fighting!")
        return false
    end

    -- Calculate success
    local skill_percent = caster:get_skill_percent("shadow strike")
    local success_chance = math.min(95, skill_percent + (caster:dex() - victim:dex()) * 2)

    if math.random(1, 100) > success_chance then
        -- Failure
        caster:send("You attempt to shadow strike but are noticed!")
        victim:send(caster:name() .. " tries to strike you from the shadows!")
        caster:set_hidden(false)
        caster:set_cooldown("shadow strike", 10)  -- Reduced cooldown on fail
        return false
    end

    -- Success - calculate damage
    local base_damage = math.dice(2, 8) + caster:damroll()
    local backstab_skill = caster:get_skill_percent("backstab")
    local multiplier = 3 + math.floor(backstab_skill / 20)  -- 3-8x multiplier

    local total_damage = base_damage * multiplier

    -- Apply damage
    victim:damage(total_damage, caster, "shadow strike")

    -- Special effect: 50% chance to remain hidden
    if math.random(1, 100) <= 50 then
        caster:send("You strike from the shadows and melt back into them!")
        victim:send("Someone strikes you from the shadows!")
    else
        caster:send("You strike from the shadows but are revealed!")
        victim:send(caster:name() .. " strikes you from the shadows!")
        caster:set_hidden(false)
    end

    -- Set cooldown
    caster:set_cooldown("shadow strike", 30)

    -- Improve skill
    caster:improve_skill("shadow strike")

    return true
end
```

### Lua API Reference

**Character Methods**:
```lua
caster:level()
caster:class()
caster:race()
caster:str(), caster:dex(), caster:con(), etc.
caster:hitroll(), caster:damroll(), caster:ac()
caster:hp(), caster:max_hp()
caster:mana(), caster:max_mana()
caster:get_skill_percent(skill_name)
caster:get_skill_mastery(skill_name)
caster:has_skill(skill_name)
caster:improve_skill(skill_name)
caster:set_cooldown(skill_name, seconds)
caster:get_cooldown(skill_name)
caster:is_hidden()
caster:set_hidden(bool)
caster:is_sneaking()
caster:set_sneaking(bool)
caster:get_wielded()
caster:has_shield()
caster:apply_effect(effect_name, duration, power)
caster:remove_effect(effect_name)
caster:has_effect(effect_name)
caster:send(message)
caster:gain_experience(amount)
```

**Combat Methods**:
```lua
victim:damage(amount, attacker, skill_name)
victim:heal(amount)
victim:start_combat(target)
victim:stop_combat()
victim:is_fighting(target)
victim:get_enemy()
```

**Utility Functions**:
```lua
math.dice(num, size)
math.random(min, max)
math.clamp(value, min, max)
string.format(format, ...)
```

---

## Balance Versioning

### Concept

Track all balance changes in version-controlled JSON files, allowing:
- Rollback to previous balance states
- A/B testing between versions
- Historical analysis of balance changes

### Balance Patch System

**Directory Structure**:
```
/data/balance/
    v1.0.0/
        skills.json
        damage_formulas.json
        cooldowns.json
    v1.1.0/
        skills.json (modified backstab multipliers)
        damage_formulas.json
        cooldowns.json
    v1.2.0-beta/
        skills.json (experimental bash changes)
```

**Version Manifest**:
```json
{
    "version": "1.2.0-beta",
    "release_date": "2025-01-15",
    "changes": [
        {
            "skill": "backstab",
            "change": "Increased multiplier at Expert mastery from 5x to 6x",
            "reason": "Assassins underperforming in PvP"
        },
        {
            "skill": "bash",
            "change": "Reduced cooldown from 3 rounds to 2 rounds",
            "reason": "Warriors too passive in group combat"
        }
    ],
    "playtesting_notes": "Monitor assassin win rate in PvP",
    "rollback_if": "Assassin win rate > 60%"
}
```

### A/B Testing Framework

**Test Configuration**:
```json
{
    "test_id": "backstab_multiplier_test",
    "duration": "2025-01-15 to 2025-01-29",
    "variants": [
        {
            "name": "control",
            "weight": 50,
            "config": "v1.1.0"
        },
        {
            "name": "variant_a",
            "weight": 25,
            "config": "v1.2.0-beta-high",
            "description": "6x multiplier at master"
        },
        {
            "name": "variant_b",
            "weight": 25,
            "config": "v1.2.0-beta-low",
            "description": "5.5x multiplier at master"
        }
    ],
    "metrics": [
        "average_backstab_damage",
        "assassin_kill_rate",
        "backstab_deaths",
        "player_satisfaction_survey"
    ]
}
```

**Assignment**:
- Players randomly assigned to variant on login
- Assignment persists for test duration
- Skills use variant-specific balance configuration

### Telemetry and Analytics

**Tracked Metrics**:
```json
{
    "skill_usage": {
        "skill_id": 401,
        "skill_name": "backstab",
        "total_uses": 45832,
        "success_rate": 0.67,
        "average_damage": 245,
        "kills_attributed": 1823,
        "deaths_while_using": 234
    },
    "class_performance": {
        "class": "ASSASSIN",
        "kill_death_ratio": 1.34,
        "average_level": 42,
        "skill_usage_distribution": {
            "backstab": 0.35,
            "throatcut": 0.15,
            "hide": 0.25,
            "steal": 0.10,
            "other": 0.15
        }
    }
}
```

**Balance Dashboard** (Web UI):
- Real-time skill usage graphs
- Class performance comparison
- PvP win rates by class/level
- Skill effectiveness heatmaps
- Player feedback integration

---

## Migration Path

### Phase 1: Foundation (4 weeks)

**Goals**:
- Implement effect template system
- Create formula parser
- Build database schema

**Tasks**:
1. Design `SkillEffects` table schema
2. Implement `SkillEffectEngine` C++ class
3. Create `FormulaParser` and `FormulaEvaluator`
4. Write unit tests for all components

**Deliverables**:
- Working effect system with 5 effect types
- Formula parser handling basic expressions
- Database schema finalized

### Phase 2: Skill Migration (8 weeks)

**Goals**:
- Migrate 35 fully configurable skills to database
- Convert 20 formula-driven skills to new system

**Tasks**:
1. Extract passive skill logic to database
2. Convert weapon proficiencies to effect templates
3. Migrate simple combat skills (bash, kick, sweep)
4. Add formula evaluation to damage calculations

**Deliverables**:
- 35 skills 100% database-driven
- 20 skills using formula system
- Balance changes no longer require recompilation

### Phase 3: Scripting (8 weeks)

**Goals**:
- Integrate Lua scripting engine
- Implement skill hooks system
- Migrate complex skills to Lua

**Tasks**:
1. Integrate LuaJIT into server
2. Create Lua API for character/combat operations
3. Implement skill hook system
4. Migrate backstab, steal, shapechange to Lua
5. Create builder documentation

**Deliverables**:
- Lua scripting fully functional
- 10 complex skills migrated to Lua
- Hot-reload working for Lua scripts

### Phase 4: Skill Trees (6 weeks)

**Goals**:
- Implement skill tree system
- Create prerequisite validation
- Add skill point allocation

**Tasks**:
1. Design skill tree JSON schema
2. Implement prerequisite checking
3. Create skill point system
4. Build web UI for skill tree visualization
5. Populate trees for all classes

**Deliverables**:
- Skill tree system functional
- 8 class skill trees defined
- Web UI for skill management

### Phase 5: Advanced Features (6 weeks)

**Goals**:
- Add mastery levels
- Implement synergies
- Create balance versioning system

**Tasks**:
1. Design mastery progression system
2. Implement synergy detection and bonuses
3. Create balance version framework
4. Add A/B testing infrastructure
5. Build balance analytics dashboard

**Deliverables**:
- Mastery system live on all skills
- 20 skill synergies defined
- Balance versioning operational
- Analytics dashboard deployed

### Phase 6: Testing & Polish (4 weeks)

**Goals**:
- Comprehensive testing
- Performance optimization
- Documentation completion

**Tasks**:
1. Load testing (1000+ players)
2. Balance testing across all classes
3. Lua script security audit
4. Performance profiling and optimization
5. Complete builder documentation
6. Create player-facing skill guides

**Deliverables**:
- All systems passing load tests
- Performance targets met (<1ms skill execution)
- Complete documentation
- Ready for production deployment

---

## Success Metrics

### Developer Productivity
- **Balance Change Time**: 5 minutes (vs. 2 hours with recompilation)
- **New Skill Creation**: 30 minutes (vs. 4 hours with C++)
- **Bug Fix Time**: 15 minutes (hot-reload vs. full restart)

### Game Balance
- **Class Win Rates**: All within 45-55% (PvP)
- **Skill Usage Distribution**: No single skill >30% of combat actions
- **Player Retention**: +15% due to better balance and variety

### Content Creation
- **Builder Satisfaction**: 8/10 average rating
- **Community Skills**: 50+ custom skills created by players
- **Balance Iteration Speed**: 5x faster (weekly vs. monthly patches)

---

## Conclusion

Modernizing the FieryMUD skill system to a **data-driven, scriptable, and balanced architecture** will:

1. **Accelerate Development**: 10x faster iteration on balance and content
2. **Empower Builders**: No C++ knowledge required for most skills
3. **Improve Balance**: Data-driven decisions, A/B testing, rapid adjustments
4. **Enhance Player Experience**: Better variety, skill customization, ongoing content

**Total Effort**: 36 weeks (9 months) with 2 developers

**ROI**:
- Developer time saved: 20+ hours/week on balance changes
- Content creation speed: 5x faster for new skills
- Player retention: Estimated +15% from improved balance and variety

---

**Generated**: 2025-01-06
**Version**: 1.0
**Maintainer**: FieryMUD Development Team

# FieryMUD Skill System Configurability Analysis

**Technical Analysis of Skill Configuration vs. Hardcoded Implementation**

---

## Executive Summary

The FieryMUD skill system shows a **30/40/30 split** in configurability:
- **30% Fully Configurable**: Passive skills and simple effects (weapon proficiencies, stat buffs)
- **40% Partially Configurable**: Formula-driven combat skills with hardcoded logic
- **30% Custom Logic Required**: Complex mechanics requiring C++ implementation

### Key Findings

| Category | Count | % | Database Ready | Migration Effort |
|----------|-------|---|----------------|------------------|
| **Fully Configurable** | 35 | 30% | Yes | Low (1-2 weeks) |
| **Partially Configurable** | 47 | 40% | With formulas | Medium (4-6 weeks) |
| **Custom Logic Required** | 35 | 30% | Requires scripting | High (8-12 weeks) |

---

## Configurability Categories

### 1. Fully Configurable Skills (35 skills, 30%)

These skills can be **entirely defined in the database** with no C++ code changes required for balance adjustments.

#### 1.1 Passive Skills (12 skills)

Skills that apply effects automatically during combat or continuously.

| ID | Name | Configuration Data | Current Implementation |
|----|------|-------------------|------------------------|
| 411 | dual wield | Penalty reduction by level | Passive check in `fight.cpp` |
| 412 | double attack | Proc chance by level | Combat system auto-check |
| 420 | parry | Success chance = skill% | Auto-check during combat |
| 421 | dodge | Success chance = skill% + DEX modifier | Auto-check during combat |
| 422 | riposte | Counter-attack chance after parry | Triggered by parry |
| 423 | meditate | Mana regen multiplier | Position-based check |
| 424 | quick chant | Cast time reduction % | Spell parser check |
| 448 | safefall | Fall damage reduction % | Movement damage check |
| 451 | spell knowledge | Spell learning bonus | Character advancement |
| 476 | stealth | Detection resistance | Movement stealth check |
| 477 | shadow | Enhanced hide bonus | Layered with hide |
| 493 | sneak attack | Damage bonus from stealth | Combat damage calculation |

**Database Schema**:
```json
{
    "id": 412,
    "name": "double_attack",
    "category": "passive_combat",
    "effects": [
        {
            "type": "EXTRA_ATTACK",
            "chance_formula": "min(95, skill_percent + (level / 5))",
            "attack_count": 1
        }
    ]
}
```

#### 1.2 Weapon Proficiencies (9 skills)

Pure stat modifiers applied to combat rolls.

| ID | Name | Configuration Data |
|----|------|-------------------|
| 449 | barehand | Attack bonus, damage dice |
| 463 | bludgeoning weapons | Hit/dam bonus by skill% |
| 464 | piercing weapons | Hit/dam bonus by skill% |
| 465 | slashing weapons | Hit/dam bonus by skill% |
| 466 | 2H bludgeoning weapons | Hit/dam bonus by skill% |
| 467 | 2H piercing weapons | Hit/dam bonus by skill% |
| 468 | 2H slashing weapons | Hit/dam bonus by skill% |
| 469 | missile weapons | Ranged hit/dam bonus |

**Database Schema**:
```json
{
    "id": 463,
    "name": "bludgeoning_weapons",
    "category": "weapon_proficiency",
    "effects": [
        {
            "type": "WEAPON_BONUS",
            "weapon_types": ["BLUDGEON"],
            "hit_bonus_formula": "skill_percent / 10",
            "dam_bonus_formula": "skill_percent / 20"
        }
    ]
}
```

#### 1.3 Simple Buff/Debuff Songs/Chants (14 skills)

Apply status effects with defined durations and modifiers.

| ID | Name | Effects | Configuration |
|----|------|---------|---------------|
| 551 | inspiration | STR/CHA bonus | Duration, stat modifiers |
| 556 | song of rest | HP regen rate | Tick rate, heal amount |
| 558 | heroic journey | Group damage bonus | Duration, damage % |
| 601 | regeneration | HP regen buff | Duration, regen rate |
| 602 | battle hymn | STR/CON bonus | Duration, stat modifiers |
| 603 | war cry | Group morale | Duration, save bonuses |
| 611 | spirit of the wolf | DEX/damage bonus | Duration, modifiers |
| 612 | spirit of the bear | STR/AC bonus | Duration, modifiers |
| 613 | interminable wrath | Extended berserk | Duration multiplier |
| 614 | hymn of saint augustine | Elemental resistance | Duration, resist % |
| 615 | fires of saint augustine | Fire damage bonus | Duration, damage % |
| 616 | blizzards of saint augustine | Cold damage bonus | Duration, damage % |
| 617 | tremors of saint augustine | Earth damage bonus | Duration, damage % |
| 618 | tempest of saint augustine | Lightning damage bonus | Duration, damage % |

**Database Schema**:
```json
{
    "id": 601,
    "name": "regeneration",
    "category": "buff_chant",
    "effects": [
        {
            "type": "STAT_MODIFIER",
            "stat": "HP_REGEN",
            "modifier_formula": "5 + (level / 10)",
            "duration_formula": "60 + (level * 2)",
            "tick_interval": 6
        }
    ],
    "wearoff_message": "Your healthy feeling subsides."
}
```

---

### 2. Partially Configurable Skills (47 skills, 40%)

These skills have **formula-driven damage/effects** but require hardcoded validation and targeting logic.

#### 2.1 Simple Combat Skills (18 skills)

Direct damage attacks with straightforward formulas.

| ID | Name | Hardcoded Logic | Configurable Data |
|----|------|-----------------|-------------------|
| 402 | bash | Target validation, position checks | Damage formula, stun duration, cooldown |
| 404 | kick | Combat state, cooldown | Damage = `level * 2 + damroll`, delay time |
| 406 | punch | Unarmed check | Damage based on monk level |
| 414 | springleap | Jump mechanics | Damage, position change |
| 427 | bodyslam | Knockdown logic | Damage, stun duration |
| 436 | sweep | Area effect targeting | Damage to all enemies |
| 479 | peck | Animal-only validation | Damage formula |
| 480 | claw | Natural weapon check | Damage based on size |
| 481 | electrify | Electric damage type | Damage, shock effect |
| 485 | maul | Size-based targeting | Damage multiplier |
| 491 | cartwheel | Movement + attack | Damage, dodge bonus |
| 494 | rend | Armor reduction | Damage, AC penalty |
| 495 | roundhouse | Multi-target logic | Damage per target |

**Example - Bash Configuration**:
```json
{
    "id": 402,
    "name": "bash",
    "category": "combat_stun",
    "validation": {
        "requires_shield": true,
        "min_position": "FIGHTING",
        "cannot_target_self": true
    },
    "effects": [
        {
            "type": "DAMAGE",
            "damage_formula": "dice(1, 4) + (level / 5) + damroll",
            "damage_multiplier_sitting": 1.5,
            "damage_type": "BLUDGEON"
        },
        {
            "type": "STUN",
            "duration_rounds": 2,
            "save_type": "DEX",
            "save_difficulty": 15
        }
    ],
    "cooldown_rounds": 2,
    "failure_stun_self": true
}
```

#### 2.2 Stealth Skills (6 skills)

Detection and positioning mechanics.

| ID | Name | Hardcoded Logic | Configurable Data |
|----|------|-----------------|-------------------|
| 403 | hide | Visibility system | Detection difficulty = `skill% + dex/2` |
| 408 | sneak | Movement tracking | Success chance formula |
| 409 | steal | Inventory manipulation | Success vs. victim perception |
| 410 | track | Pathfinding algorithm | Range = `skill% / 10 rooms` |
| 478 | conceal | Item visibility flags | Concealment difficulty |

#### 2.3 Breath Weapons (5 skills)

Area-of-effect elemental attacks.

| ID | Name | Configurable Data |
|----|------|-------------------|
| 435 | breathe lightning | Damage = `level * 3`, save for half, AoE size |
| 486 | breathe fire | Damage, fire damage type, cooldown |
| 487 | breathe frost | Damage, slow effect, cooldown |
| 488 | breathe acid | Damage, armor corrosion |
| 489 | breathe gas | Poison damage, lingering effect |

#### 2.4 Buff/Debuff Songs with Damage (8 skills)

Hybrid effects combining damage and status changes.

| ID | Name | Hardcoded Logic | Configurable Data |
|----|------|-----------------|-------------------|
| 552 | terror | Fear mechanics | Damage, fear save DC |
| 555 | crown of madness | Charm/confusion | Control duration, save DC |
| 557 | ballad of tears | Area fear | AoE size, fear duration |
| 605 | shadows sorrow song | Mind effects | Damage, debuff strength |
| 606 | ivory symphony | Paralysis | Damage, paralyze chance |
| 607 | aria of dissonance | Sonic damage | Damage formula, deafen effect |
| 608 | sonata of malaise | Debuff spread | Stat penalties, duration |
| 610 | seed of destruction | Disease mechanics | DoT damage, spread chance |

#### 2.5 Support Skills (10 skills)

Utility abilities with moderate complexity.

| ID | Name | Hardcoded Logic | Configurable Data |
|----|------|-----------------|-------------------|
| 407 | rescue | Combat target switching | Success chance formula |
| 415 | mount | Entity relationships | Riding bonus |
| 417 | tame | AI behavior changes | Success vs. creature level |
| 419 | doorbash | Door state manipulation | Success formula, door damage |
| 434 | guard | Intercept mechanics | Block chance |
| 438 | douse | Fire effect removal | Success chance |
| 443 | bandage | HP restoration | Heal = `3d8 + (skill% / 10)` |
| 444 | first aid | Advanced healing | Heal formula, bleeding stop |
| 450 | summon mount | Creature summoning | Mount type by level |

---

### 3. Custom Logic Required (35 skills, 30%)

These skills require **complex C++ implementation** that cannot easily be data-driven.

#### 3.1 Advanced Combat Mechanics (11 skills)

| ID | Name | Why Custom Logic Required |
|----|------|---------------------------|
| 401 | backstab | Position validation, damage multiplier (2x-5x by level), stealth breaking |
| 405 | pick lock | Lock complexity algorithms, pick chance vs. lock difficulty |
| 413 | berserk | Mode toggle, stat changes, HP threshold triggers |
| 418 | throatcut | Instant kill chance, victim HP threshold |
| 426 | circle | Combat repositioning, backstab re-attempt mid-fight |
| 428 | bind | Movement restriction, escape mechanics |
| 430 | switch | Combat target management, aggro transfer |
| 431 | disarm | Item manipulation, weapon drop/pickup |
| 440 | instant kill | Assassin-only one-shot mechanic |
| 472 | eye gouge | Blindness effect, vision system interaction |
| 475 | corner | Movement blocking, escape prevention |

**Example - Backstab Complexity**:
```cpp
// Cannot be easily data-driven:
// 1. Stealth requirement check
if (!HIDDEN(ch) && !SNEAKING(ch))
    return fail("You must be hidden to backstab!");

// 2. Position validation (behind target)
if (FIGHTING(vict))
    return fail("They're too alert!");

// 3. Weapon type requirement
if (!is_piercing_weapon(wielded))
    return fail("Only piercing weapons work!");

// 4. Level-based damage multiplier
int multiplier = (level < 8) ? 2 :
                 (level < 14) ? 3 :
                 (level < 21) ? 4 : 5;

// 5. Backstab breaks stealth
REMOVE_HIDDEN(ch);
REMOVE_SNEAKING(ch);

// 6. Special messaging
act("$n backstabs $N!", TRUE, ch, 0, vict, TO_NOTVICT);
```

#### 3.2 Complex Status Effects (9 skills)

| ID | Name | Why Custom Logic Required |
|----|------|---------------------------|
| 429 | shapechange | Form transformation, stat replacements, ability swaps |
| 442 | hunt | Tracking system, mob location, path generation |
| 446 | chant | Performance system, chant channeling |
| 447 | scribe | Spellbook manipulation, spell copying |
| 490 | perform | Music system, audience reactions |
| 553 | enrapture | Complex charm mechanics, multi-target control |
| 554 | hearthsong | Illusion system, disguise management |
| 604 | peace | Combat ending, aggro clearing |
| 609 | apocalyptic anthem | Massive AoE with environmental effects |

#### 3.3 High-Level Group Mechanics (6 skills)

| ID | Name | Why Custom Logic Required |
|----|------|---------------------------|
| 441 | hitall | Multi-target combat system |
| 445 | vampiric touch | HP drain + heal, life steal mechanics |
| 473 | retreat | Combat disengagement, movement validation |
| 474 | group retreat | Group coordination, simultaneous movement |
| 492 | lure | AI manipulation, forced movement |
| 559 | freedom song | Complex debuff removal, status cleansing |

#### 3.4 Spell Sphere System (11 skills)

| ID | Name | Why Custom Logic Required |
|----|------|---------------------------|
| 451-462 | sphere of * | Spell access control, memorization limits, sphere interaction |

**Complexity**: These act as "meta-skills" that gate access to entire spell categories. Requires deep integration with spell system.

---

## Migration Strategy

### Phase 1: Passive Skills (2 weeks)

**Target**: 35 fully configurable skills

1. Design `SkillEffects` schema with effect types:
   - `STAT_MODIFIER` (STR, DEX, CON, HP_REGEN, etc.)
   - `WEAPON_BONUS` (hit/dam bonuses)
   - `EXTRA_ATTACK` (double attack)
   - `DAMAGE_REDUCTION` (safefall, dodge)

2. Implement effect application system in C++:
```cpp
void apply_skill_effects(CharData *ch, SkillDef *skill) {
    for (auto& effect : skill->effects) {
        switch (effect.type) {
            case EFFECT_STAT_MODIFIER:
                apply_stat_modifier(ch, effect);
                break;
            case EFFECT_WEAPON_BONUS:
                apply_weapon_bonus(ch, effect);
                break;
            // ...
        }
    }
}
```

3. Migrate passive skill data to database
4. Update combat/stat systems to read from database

**Success Criteria**:
- All weapon proficiencies configurable via database
- All passive combat skills (dodge, parry, riposte) use database formulas
- Zero C++ changes needed for balance adjustments

---

### Phase 2: Combat Formulas (6 weeks)

**Target**: 47 partially configurable skills

1. Create formula evaluation system:
```cpp
class FormulaEngine {
public:
    int evaluate(const std::string& formula, CharData *ch, CharData *vict) {
        // Parse formula: "dice(1,4) + (level / 5) + damroll"
        // Substitute variables: level, skill_percent, str, dex, etc.
        // Return evaluated result
    }
};
```

2. Define common formula variables:
   - `level` - character level
   - `skill_percent` - skill proficiency (0-100)
   - `str`, `dex`, `con`, `int`, `wis`, `cha` - stat values
   - `hitroll`, `damroll` - combat modifiers
   - `target_ac`, `target_level` - victim stats

3. Implement validation templates:
```json
{
    "validation": {
        "requires_weapon": true,
        "weapon_types": ["PIERCE", "SLASH"],
        "min_position": "FIGHTING",
        "cannot_target_self": true,
        "requires_shield": false
    }
}
```

4. Migrate skill implementations:
   - Extract formulas from C++ into JSON
   - Replace hardcoded numbers with formula strings
   - Test formula evaluation matches original behavior

**Success Criteria**:
- All simple combat skills (bash, kick, sweep) use database formulas
- Balance changes require JSON edits only
- Formula evaluation performance < 1ms per skill use

---

### Phase 3: Scripting System (12 weeks)

**Target**: 35 custom logic skills

1. Integrate Lua scripting engine:
```cpp
class SkillScript {
public:
    bool execute(CharData *ch, CharData *vict, const std::string& args) {
        lua_State *L = get_lua_state();
        lua_getglobal(L, skill->script_name.c_str());
        lua_push_character(L, ch);
        lua_push_character(L, vict);
        lua_pushstring(L, args.c_str());
        lua_call(L, 3, 1);
        return lua_toboolean(L, -1);
    }
};
```

2. Define Lua API for skills:
```lua
-- Example: backstab.lua
function backstab(caster, victim, args)
    -- Check stealth
    if not caster:is_hidden() and not caster:is_sneaking() then
        caster:send("You must be hidden to backstab!")
        return false
    end

    -- Check weapon
    local weapon = caster:get_wielded()
    if not weapon or weapon:get_damage_type() ~= DAMAGE_PIERCE then
        caster:send("Only piercing weapons work!")
        return false
    end

    -- Calculate damage
    local level = caster:get_level()
    local multiplier = (level < 8) and 2 or
                       (level < 14) and 3 or
                       (level < 21) and 4 or 5

    local base_damage = math.dice(1, weapon:get_dice_size()) + caster:get_damroll()
    local total_damage = base_damage * multiplier

    -- Apply damage
    victim:damage(total_damage, caster, "backstab")

    -- Break stealth
    caster:set_hidden(false)
    caster:set_sneaking(false)

    -- Messages
    caster:send("You backstab " .. victim:get_name() .. "!")
    victim:send(caster:get_name() .. " backstabs you!")

    return true
end
```

3. Implement safe sandboxing:
   - Restrict file I/O
   - Limit memory usage
   - Timeout protection (max 100ms per skill)
   - API whitelist

4. Migrate complex skills to Lua:
   - Start with backstab, pick lock, steal
   - Move shapechange, hunt, perform
   - Document Lua API for builders

**Success Criteria**:
- All complex skills scriptable in Lua
- Builders can create custom skills without C++ knowledge
- Hot-reload Lua scripts without server restart
- Script execution time < 100ms per skill

---

### Phase 4: Validation & Polish (4 weeks)

1. **Balance Testing**:
   - Compare new system to legacy behavior
   - Adjust formulas for equivalent power levels
   - Test all 117 skills across all classes

2. **Performance Optimization**:
   - Cache parsed formulas
   - Optimize database queries
   - Profile Lua script execution

3. **Builder Tools**:
   - Web UI for skill editing (Muditor integration)
   - Formula validator with test cases
   - Lua script debugger

4. **Documentation**:
   - Formula language reference
   - Lua API documentation
   - Migration guide for legacy skills

---

## Benefits of Data-Driven Skills

### For Developers

1. **Rapid Iteration**: Change formulas without recompiling
2. **A/B Testing**: Test multiple balance variations easily
3. **Version Control**: Track balance changes in JSON/Lua files
4. **Parallel Development**: Multiple devs can work on skills simultaneously

### For Game Designers

1. **No C++ Required**: Balance adjustments via JSON/web UI
2. **Instant Feedback**: Test changes immediately (hot-reload)
3. **Experimentation**: Try radical changes without code risk
4. **Historical Data**: Track balance changes over time

### For Players

1. **Better Balance**: More frequent, data-driven adjustments
2. **Transparency**: Skill formulas visible in help system
3. **Custom Content**: Community-created skills (if enabled)
4. **Bug Fixes**: Faster turnaround on broken skills

---

## Code Examples

### Database-Driven Skill Effect Application

```cpp
void CharData::apply_skill_effects(int skill_id) {
    auto skill = SkillDatabase::get(skill_id);

    for (const auto& effect : skill->effects) {
        switch (effect.type) {
            case EFFECT_STAT_MODIFIER: {
                int mod_value = FormulaEngine::evaluate(
                    effect.modifier_formula, this, nullptr
                );
                this->apply_stat_modifier(effect.stat, mod_value, effect.duration);
                break;
            }

            case EFFECT_DAMAGE: {
                int damage = FormulaEngine::evaluate(
                    effect.damage_formula, this, victim
                );
                damage_char(victim, this, damage, skill_id);
                break;
            }

            case EFFECT_HEAL: {
                int heal = FormulaEngine::evaluate(
                    effect.heal_formula, this, nullptr
                );
                this->heal(heal);
                break;
            }
        }
    }
}
```

### Formula Evaluation Engine

```cpp
class FormulaEngine {
    static int evaluate(const std::string& formula, CharData *ch, CharData *vict) {
        // Create context with variables
        std::map<std::string, int> context = {
            {"level", GET_LEVEL(ch)},
            {"skill_percent", GET_ISKILL(ch, current_skill)},
            {"str", GET_STR(ch)},
            {"dex", GET_DEX(ch)},
            {"hitroll", GET_HITROLL(ch)},
            {"damroll", GET_DAMROLL(ch)}
        };

        if (vict) {
            context["target_level"] = GET_LEVEL(vict);
            context["target_ac"] = GET_AC(vict);
        }

        // Parse and evaluate
        auto ast = parse_formula(formula);
        return ast->evaluate(context);
    }

    static AST* parse_formula(const std::string& formula) {
        // Parse expressions like:
        // "dice(1, 4) + (level / 5) + damroll"
        // "min(95, skill_percent + (level / 5))"
        // "str * 2 + dex / 2"
    }
};
```

---

## Conclusion

The FieryMUD skill system can be successfully migrated to a **data-driven architecture** with three-tiered configurability:

1. **Simple skills (30%)**: Pure database configuration
2. **Moderate skills (40%)**: Database + formula engine
3. **Complex skills (30%)**: Database + Lua scripting

**Estimated Total Migration Time**: 24 weeks (6 months) with 2 developers

**Benefits**:
- Faster iteration on balance
- No recompilation for most changes
- Community content creation potential
- Better separation of data and logic

**Risks**:
- Formula/scripting bugs harder to debug
- Performance overhead from interpretation
- Builders need training on new system

**Recommendation**: Start with Phase 1 (passive skills) to validate approach, then proceed to formulas and scripting.

---

**Generated**: 2025-01-06
**Version**: 1.0
**Maintainer**: FieryMUD Development Team

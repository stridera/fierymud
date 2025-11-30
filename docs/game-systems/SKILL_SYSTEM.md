# FieryMUD Skill System - Developer Reference

**Complete Reference for the FieryMUD Skill System**

---

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Complete Skill Reference](#complete-skill-reference)
4. [Class Skill Assignments](#class-skill-assignments)
5. [Implementation Patterns](#implementation-patterns)
6. [Database Schema Mapping](#database-schema-mapping)
7. [Development Guidelines](#development-guidelines)

---

## Overview

The FieryMUD skill system consists of **117 unique skills, songs, and chants** distributed across combat, utility, movement, stealth, and class-specific abilities.

### Key Statistics

- **Total Skills**: 89 combat/utility skills (IDs 401-495)
- **Bard Songs**: 10 songs (IDs 551-560)
- **Monk Chants**: 18 chants (IDs 601-618)
- **Implemented**: 54/117 (46.2%) have explicit C++ implementations
- **Class-Assigned**: 56/117 (47.9%) have class-level requirements
- **Humanoid-Only**: 24/117 (20.5%) restricted to humanoid characters

### Skill Categories

| Category | Count | Description |
|----------|-------|-------------|
| Combat | 37 | Active combat abilities including attacks and defensive maneuvers |
| Weapon Proficiencies | 9 | Passive skills governing weapon effectiveness |
| Movement | 11 | Travel, positioning, and tactical abilities |
| Stealth | 8 | Covert operations and subterfuge |
| Passive | 3 | Always-active or toggle abilities |
| Utility | 9 | Miscellaneous non-combat abilities |
| Spell Spheres | 12 | Meta-skills representing spell schools |
| Songs | 10 | Bard-specific musical abilities |
| Chants | 18 | Monk/Berserker spiritual abilities |

---

## System Architecture

### Data Structure

```cpp
struct SkillDef {
    char *name;                    // Skill name (e.g., "backstab")
    int min_level[NUM_CLASSES];    // Min level per class
    int min_race_level[NUM_RACES]; // Min level per race
    int lowest_level;              // Earliest availability

    int mana_max;                  // Max mana cost (spells)
    int mana_min;                  // Min mana cost
    int mana_change;               // Mana change per level

    int minpos;                    // Minimum position required
    bool fighting_ok;              // Can use in combat
    int targets;                   // Valid targets (bitfield)
    bool violent;                  // Triggers combat
    bool humanoid;                 // Humanoid-only restriction

    int routines;                  // Magic routines (for songs/chants)
    int damage_type;               // Damage type
    int sphere;                    // Spell sphere
    int pages;                     // Spellbook pages
    bool quest;                    // Quest-required
    char *wearoff;                 // Wearoff message

    int addl_mem_time;             // Additional memorization time
    int cast_time;                 // Casting time
};
```

### Initialization Flow

1. **init_skills()** - Called on boot (`skills.cpp:430`)
   - Clears all skill definitions
   - Defines all skills via `skillo()`, `chanto()`, `songo()` macros
   - Sets base properties (name, targets, violent flag)

2. **Class Assignments** - In `class.cpp`
   - `skill_assign(skill, class, level)` sets minimum level per class
   - Common skills assigned to all classes in loops
   - Class-specific skills assigned individually

3. **Race Assignments** - In `races.cpp`
   - Similar to class assignments but for racial skills
   - Natural abilities, racial bonuses

### Skill Usage Flow

1. **Command Parsing** - `interpreter.cpp`
   - Player enters command (e.g., "backstab goblin")
   - Command table maps to `ACMD(do_backstab)`

2. **Validation** - In skill implementation
   - Check if player has skill
   - Check skill level (`GET_ISKILL`)
   - Validate position, fighting status
   - Check cooldowns

3. **Execution** - Skill-specific logic
   - Calculate success (skill % vs target AC/saves)
   - Apply effects (damage, status changes)
   - Send messages to player/room
   - Set cooldowns
   - Improve skill

4. **Improvement** - `improve_skill()`
   - Random chance based on INT/WIS
   - Increase skill percentage
   - Faster improvement for certain skills

---

## Complete Skill Reference

### Combat Skills (37 skills)

#### Attack Skills

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 401 | backstab | 1-15 | Assassin(1), Thief(1), Rogue(1), Bard(1), Illusionist(15), Mercenary(10) | Stealth attack from behind with damage multiplier (2x-5x based on level) |
| 402 | bash | 1 | Warrior(1), Paladin(1), Anti-Paladin(1), Hunter(1), Ranger(1), Mercenary(1) | Shield bash to stun opponents; inflicts 1.5x damage if victim is sitting/resting |
| 404 | kick | 1-10 | Warrior(1), Monk(1), multiple classes | Basic melee kick attack; 3 round delay after execution |
| 406 | punch | - | Monk | Unarmed strike attack |
| 414 | springleap | 50 | Monk(50) | Acrobatic jump attack |
| 418 | throatcut | 30 | Assassin(30) | Lethal throat attack for assassination |
| 426 | circle | - | Assassin, Rogue | Re-position for backstab mid-combat |
| 427 | bodyslam | - | Mercenary, Berserker | Full-body tackle |
| 431 | disarm | 20-50 | Warrior(20), Mercenary(20), Paladin(50), Anti-Paladin(50), Thief(40) | Remove opponent's weapon |
| 436 | sweep | - | Monk | Leg sweep to knock down enemies |
| 441 | hitall | 50-80 | Warrior(50), Paladin(80), Anti-Paladin(80) | Attack all enemies in room |
| 472 | eye gouge | 10 | Thief(10) | Blinds opponent temporarily with wearoff: "Your vision returns." |
| 479 | peck | - | NPC/Animals | Avian attack |
| 480 | claw | - | NPC/Animals | Natural claw attack |
| 481 | electrify | - | NPC/Animals | Electric shock attack |
| 485 | maul | 65 | Berserker(65), Animal forms | Vicious melee attack |
| 491 | cartwheel | 10 | Rogue(10) | Acrobatic combat maneuver |
| 492 | lure | 15 | Rogue(15) | Attract creatures into combat |
| 494 | rend | 30 | Thief(30) | Tears enemy defenses with wearoff: "You patch your defenses." |
| 495 | roundhouse | 65 | Monk(65) | Spinning kick attack |

#### Defensive Skills

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 420 | parry | 20-40 | Warrior(30), Paladin(20), Anti-Paladin(20), Hunter(1), Ranger(30), Berserker(15), multiple classes | Deflect incoming attacks with weapon |
| 421 | dodge | 1-20 | Monk(1), Rogue(1), Thief(1), Assassin(1), Hunter(1), Warrior(1), multiple classes | Avoid attacks through agility |
| 422 | riposte | 20-60 | Paladin(40), Anti-Paladin(40), Warrior(40), Hunter(1), Ranger(40), Monk(20), Berserker(50), Mercenary(60) | Counter-attack after successful parry |
| 434 | guard | 1-50 | Paladin(10), Anti-Paladin(10), Warrior(25), Hunter(1), Ranger(50), Berserker(25), Mercenary(10) | Protect group members from attacks |

#### Special Combat

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 407 | rescue | 10-35 | Warrior(15), Paladin(10), Anti-Paladin(10), Hunter(1), Ranger(35), Berserker(35) | Intervene to protect allies |
| 413 | berserk | 10 | Berserker(10) | Rage mode for increased damage |
| 428 | bind | 16 | Mercenary(16) | Restrain target |
| 430 | switch | 1-40 | Anti-Paladin(10), Paladin(10), Warrior(10), Berserker(1), Hunter(1), Ranger(10), Monk(40), Mercenary(40) | Change combat targets |
| 440 | instant kill | 1 | Assassin(1) | One-shot kill ability for assassins |
| 475 | corner | 60-80 | Rogue(60), Monk(80) | Trap opponent |

#### Breath Weapons (Draconic)

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 435 | breathe lightning | - | NPC/Draconic | Lightning breath weapon |
| 486 | breathe fire | - | NPC/Draconic | Fire breath weapon |
| 487 | breathe frost | - | NPC/Draconic | Frost breath weapon |
| 488 | breathe acid | - | NPC/Draconic | Acid breath weapon |
| 489 | breathe gas | - | NPC/Draconic | Gas breath weapon |

#### Support/Recovery

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 417 | tame | 1-7 | Paladin(1), Anti-Paladin(1), Ranger(1), Druid(1), Hunter(7), Shaman(7) | Befriend animals |
| 419 | doorbash | - | Multiple | Bash down doors |
| 438 | douse | - | All | Extinguish flames |
| 443 | bandage | - | All | Basic wound treatment |
| 444 | first aid | - | Multiple | Advanced healing |

### Weapon Proficiencies (9 skills)

Passive skills governing weapon effectiveness. All humanoid-only except barehand.

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 449 | barehand | 1 | Monk(1) | Unarmed combat expertise |
| 463 | bludgeoning weapons | 1 | All melee classes | Clubs, maces, hammers |
| 464 | piercing weapons | 1 | Most classes | Daggers, spears, rapiers |
| 465 | slashing weapons | 1 | Most classes | Swords, axes, scimitars |
| 466 | 2H bludgeoning weapons | 1 | Heavy melee classes | Two-handed blunt weapons |
| 467 | 2H piercing weapons | 1 | Warrior classes | Two-handed piercing weapons |
| 468 | 2H slashing weapons | 1 | Warrior classes | Two-handed slashing weapons |
| 469 | missile weapons | - | Ranger/Hunter | Bows, crossbows, slings |
| 411 | dual wield | 1-70 | Monk(1), Hunter(1), Ranger(1), Rogue(15), Thief(15), Assassin(15), Mercenary(15), multiple classes | Fight with two weapons |

### Movement Skills (11 skills)

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 403 | hide | 1-20 | Thief(1), Rogue(1), Assassin(1), Bard(10), Illusionist(20), Mercenary(20) | Conceal presence; only detectable by sense life |
| 408 | sneak | 1-10 | Thief(1), Rogue(1), Assassin(1), Bard(10) | Move silently without notice |
| 410 | track | 1-50 | Ranger(1), Hunter(36), Assassin(10), Mercenary(30), Rogue(30), Thief(40), Bard(50) | Follow trails |
| 414 | springleap | 50 | Monk(50) | Acrobatic jump attack |
| 415 | mount | - | All | Ride creatures |
| 416 | riding | - | All | Mounted combat |
| 448 | safefall | 1 | Monk(1) | Reduce fall damage |
| 450 | summon mount | 15 | Paladin(15), Anti-Paladin(15) | Call personal mount |
| 473 | retreat | 40-60 | Warrior(60), Paladin(60), Anti-Paladin(60), Mercenary(40) | Tactical withdrawal |
| 474 | group retreat | 80 | Mercenary(80) | Coordinated escape |
| 493 | sneak attack | 1 | Rogue(1) | Bonus damage from stealth |

### Stealth Skills (8 skills)

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 401 | backstab | 1-15 | See Combat Skills | Primary assassination technique |
| 405 | pick lock | 1-10 | Thief(1), Rogue(5), Assassin(5), Bard(10) | Open locked doors/containers |
| 409 | steal | 10 | Thief(10), Bard(10) | Pickpocket items/gold |
| 426 | circle | - | Assassin, Rogue | Re-position for backstab mid-combat |
| 476 | stealth | 50 | Thief(50), Rogue(50) | Master stealth ability |
| 477 | shadow | 40-60 | Assassin(40), Rogue(60) | Enhanced concealment |
| 478 | conceal | 10-25 | Thief(10), Illusionist(10), Rogue(25) | Hide objects/weapons |

### Passive Skills (3 skills)

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 412 | double attack | 30-85 | Warrior(35), Monk(30), Hunter(1), Ranger(60), multiple classes (50-85) | Extra attack per round |
| 423 | meditate | 1-50 | Bard(1), Berserker(50) | Improve spell memorization |
| 424 | quick chant | - | Caster classes | Faster spell casting |

### Utility Skills (9 skills)

| ID | Name | Level | Classes | Description |
|----|------|-------|---------|-------------|
| 429 | shapechange | 1 | Druid(1) | Transform into creatures |
| 442 | hunt | 41 | Hunter(41) | Advanced tracking |
| 446 | chant | 15-25 | Monk(15), Berserker(25) | Monk chanting ability |
| 447 | scribe | 1 | Bard(1) | Copy spells to spellbook |
| 445 | vampiric touch | 45 | Anti-Paladin(45) | Drain life force |
| 482 | tantrum | 45 | Berserker(45) | Rage ability |
| 483 | ground shaker | 75 | Berserker(75) | Area earthquake |
| 484 | battle howl | 30 | Berserker(30) | Rallying cry |
| 490 | perform | 1 | Bard(1) | Bard performance for effects |

### Spell Spheres (12 skills)

Meta-skills representing spell schools for casters.

| ID | Name | Level | Description |
|----|------|-------|-------------|
| 451 | spell knowledge | - | General spellcasting ability |
| 452 | sphere of generic | - | Universal magic |
| 453 | sphere of fire | - | Fire magic specialization |
| 454 | sphere of water | - | Water/ice magic |
| 455 | sphere of earth | - | Earth/stone magic |
| 456 | sphere of air | - | Air/lightning magic |
| 457 | sphere of healing | - | Restoration magic |
| 458 | sphere of protection | - | Defensive magic |
| 459 | sphere of enchantment | - | Enhancement magic |
| 460 | sphere of summoning | - | Conjuration magic |
| 461 | sphere of death | - | Necromancy |
| 462 | sphere of divination | - | Information magic |

### Bard Songs (10 songs, IDs 551-560)

Musical abilities unique to the Bard class. All use perform skill.

| ID | Name | Quest | Violent | Description |
|----|------|-------|---------|-------------|
| 551 | inspiration | No | No | Buff allies with courage |
| 552 | terror | No | Yes | Frighten enemies; wearoff: "Your nerves settle down as the terror leaves you." |
| 553 | enrapture | Yes | No | Charm and distract; wearoff: "You regain your senses as the illusions subside." |
| 554 | hearthsong | Yes | No | Create illusions; wearoff: "Your familiar disguise melts away." |
| 555 | crown of madness | Yes | Yes | Confuse enemies; wearoff: "Your mind returns to reality." |
| 556 | song of rest | No | No | Healing over time; wearoff: "The restful song fades from your memory." |
| 557 | ballad of tears | No | Yes | Area fear effect; wearoff: "Your nerves settle down as the terror leaves you." |
| 558 | heroic journey | No | No | Group inspiration buff; wearoff: "Your inspiration fades." |
| 559 | freedom song | No | Yes | Remove fear/paralysis |
| 560 | joyful noise | No | No | Remove negative effects |

### Monk Chants (18 chants, IDs 601-618)

Spiritual abilities for Monk and Berserker classes.

#### Basic Chants

| ID | Name | Quest | Description |
|----|------|-------|-------------|
| 601 | regeneration | No | Health regeneration buff; wearoff: "Your healthy feeling subsides." |
| 602 | battle hymn | No | Combat enhancement; wearoff: "Your rage fades away." |
| 603 | war cry | No | Group determination buff; wearoff: "Your determination level returns to normal." |
| 604 | peace | No | End combat peacefully |

#### Offensive Chants

| ID | Name | Quest | Description |
|----|------|-------|-------------|
| 605 | shadows sorrow song | No | Mental debuff on enemies; wearoff: "The shadows in your mind clear up." |
| 606 | ivory symphony | No | Paralyze foes; wearoff: "Feeling returns to your limbs." |
| 607 | aria of dissonance | Yes | Sound-based attack; wearoff: "The dissonance stops ringing in your ears." |
| 608 | sonata of malaise | No | Debuff enemies; wearoff: "The sonata of malaise stops echoing in your ears." |
| 609 | apocalyptic anthem | Yes | Massive destruction |
| 610 | seed of destruction | Yes | Disease attack; wearoff: "The disease leaves you." |

#### Spirit Chants

| ID | Name | Quest | Description |
|----|------|-------|-------------|
| 611 | spirit of the wolf | No | Wolf spirit enhancement; wearoff: "Your fangs recede and you lose your wolf-like spirit." |
| 612 | spirit of the bear | No | Bear spirit enhancement; wearoff: "Your claws become decidedly less bear-like." |
| 613 | interminable wrath | No | Sustained rage |

#### Elemental Chants (Saint Augustine Series)

| ID | Name | Quest | Description |
|----|------|-------|-------------|
| 614 | hymn of saint augustine | Yes | Elemental mastery; wearoff: "Your inner elements subside." |
| 615 | fires of saint augustine | Yes | Inner fire; wearoff: "Your inner fire subsides." |
| 616 | blizzards of saint augustine | Yes | Inner cold; wearoff: "Your inner cold subsides." |
| 617 | tremors of saint augustine | Yes | Inner earth; wearoff: "Your inner earth subsides." |
| 618 | tempest of saint augustine | Yes | Inner storm; wearoff: "Your inner storm subsides." |

---

## Class Skill Assignments

### Top Classes by Skill Count

| Class | Skill Count | Focus |
|-------|-------------|-------|
| Anti-Paladin | 22 | Heavy combat focus with dark abilities |
| Mercenary | 22 | Versatile warrior with many weapon proficiencies |
| Berserker | 22 | Rage-based combat with chants |
| Paladin | 21 | Holy warrior with mount abilities |
| Warrior | 19 | Core combat skills |
| Hunter | 18 | Wilderness tracking and archery |
| Ranger | 16 | Nature-based combat and utility |
| Rogue | 15 | Stealth and subterfuge |
| Monk | 14 | Unarmed combat and chants |
| Thief | 13 | Stealth and thievery |
| Assassin | 11 | Lethal stealth attacks |
| Bard | 10+ | Performance and social skills |

### Class-Specific Patterns

**Warrior Classes** (Warrior, Paladin, Anti-Paladin, Mercenary):
- All weapon proficiencies (levels 1-20)
- Core combat skills: bash, kick, rescue, disarm
- Defensive skills: parry, dodge, riposte
- Advanced: double attack, hitall, guard

**Rogue Classes** (Thief, Rogue, Assassin):
- Stealth skills: sneak, hide, backstab, steal
- Utility: pick lock, conceal
- Advanced: circle, throatcut, eye gouge, sneak attack
- Light weapon proficiencies

**Ranger/Hunter**:
- Tracking and wilderness: track, hunt, tame
- Dual wield, archery (missile weapons)
- First aid, bandage
- Some druid-like abilities

**Monk/Berserker**:
- Barehand combat expertise
- Chant system (18 chants available)
- Defensive: dodge, safefall
- Special: meditate, double attack

**Bard**:
- Performance and songs (10 songs)
- Some rogue skills: backstab, hide, steal
- Social/utility focus
- Dual wield, light weapons

**Caster Classes** (Sorcerer, Cleric, Druid, etc.):
- Spell sphere skills (determine available spells)
- Meditate, quick chant, know spell
- Scribe (for wizards)
- Limited combat skills

---

## Implementation Patterns

### ACMD (Command-based) - 47 skills

Standard command implementation using `ACMD(do_skillname)` macro located primarily in:
- `act.offensive.cpp` - Combat skills (backstab, bash, kick, etc.)
- `act.movement.cpp` - Movement skills (sneak, hide, mount)
- `act.other.cpp` - Utility skills (steal, bandage, guard)

**Example Implementation**:
```cpp
ACMD(do_backstab)
{
    char arg[MAX_INPUT_LENGTH];
    CharData *vict;

    one_argument(argument, arg);

    if (!*arg)
        char_printf(ch, "Backstab who?\n");
    else if (!(vict = get_char_room_vis(ch, arg)))
        char_printf(ch, "They aren't here.\n");
    else if (vict == ch)
        char_printf(ch, "How can you sneak up on yourself?\n");
    else if (!GET_EQ(ch, WEAR_WIELD))
        char_printf(ch, "You need to wield a weapon to make it a success.\n");
    else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), VAL_WEAPON_DAM_TYPE) != TYPE_PIERCE - TYPE_HIT)
        char_printf(ch, "Only piercing weapons can be used for backstabbing.\n");
    else if (FIGHTING(vict))
        char_printf(ch, "You can't backstab a fighting person -- they're too alert!\n");
    else {
        if (GET_ISKILL(ch, SKILL_BACKSTAB) > random_number(1, 101)) {
            // Successful backstab - apply damage multiplier
            hit(ch, vict, SKILL_BACKSTAB);
        } else {
            // Failed backstab
            damage(ch, vict, 0, SKILL_BACKSTAB);
        }
    }
}
```

### Switch Case - 9 skills

Implemented in switch statements (often in `spell_parser.cpp` for songs/chants). Shared implementation logic with spell system.

**Example**:
```cpp
case CHANT_PEACE:
    spell_peace(level, ch, vict, obj);
    break;
```

### Passive/Unimplemented - 63 skills (53.8%)

Defined but no clear implementation found. May be:
- Passive (checked automatically during combat)
- NPC-only abilities
- Legacy definitions awaiting implementation
- Weapon proficiencies (affect hit/damage rolls passively)

---

## Database Schema Mapping

### Skills Table (Proposed)

```sql
CREATE TABLE "Skills" (
    id INTEGER PRIMARY KEY,
    enum_name VARCHAR(50) UNIQUE NOT NULL,  -- e.g., "SKILL_BACKSTAB"
    name VARCHAR(50) NOT NULL,              -- e.g., "backstab"
    display_name VARCHAR(50) NOT NULL,      -- e.g., "Backstab"
    category VARCHAR(30) NOT NULL,          -- e.g., "COMBAT_STEALTH"

    -- Properties
    violent BOOLEAN DEFAULT false,
    humanoid_only BOOLEAN DEFAULT false,
    fighting_ok BOOLEAN DEFAULT true,
    targets VARCHAR(50),                    -- e.g., "TAR_CONTACT"
    quest_required BOOLEAN DEFAULT false,

    -- Implementation
    implementation_file VARCHAR(100),       -- e.g., "act.offensive.cpp"
    implementation_function VARCHAR(50),    -- e.g., "do_backstab"
    implementation_pattern VARCHAR(20),     -- e.g., "ACMD", "SWITCH_CASE"

    -- Messages
    wearoff_message TEXT,
    help_text TEXT,

    -- Metadata
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "SkillClassAssignments" (
    id SERIAL PRIMARY KEY,
    skill_id INTEGER NOT NULL REFERENCES "Skills"(id),
    class_name VARCHAR(30) NOT NULL,        -- e.g., "WARRIOR"
    min_level INTEGER NOT NULL,             -- e.g., 1

    UNIQUE(skill_id, class_name)
);

CREATE INDEX idx_skill_class ON "SkillClassAssignments"(skill_id);
CREATE INDEX idx_class_name ON "SkillClassAssignments"(class_name);
```

### Example Queries

**Get all skills for Thief class:**
```sql
SELECT s.*, sca.min_level
FROM "Skills" s
JOIN "SkillClassAssignments" sca ON s.id = sca.skill_id
WHERE sca.class_name = 'THIEF'
ORDER BY sca.min_level, s.name;
```

**Get all stealth skills:**
```sql
SELECT * FROM "Skills"
WHERE category LIKE '%STEALTH%'
ORDER BY name;
```

**Get all implemented combat skills:**
```sql
SELECT * FROM "Skills"
WHERE category LIKE '%COMBAT%'
  AND implementation_function IS NOT NULL
ORDER BY name;
```

---

## Development Guidelines

### Adding a New Skill

1. **Define skill in `skills.cpp`**:
```cpp
skillo(SKILL_NEW_SKILL, "new skill", TAR_CONTACT, true, false);
```

2. **Add enum to header** (`structs.hpp` or `spells.hpp`):
```cpp
#define SKILL_NEW_SKILL 496
```

3. **Implement command** (e.g., in `act.offensive.cpp`):
```cpp
ACMD(do_new_skill) {
    // Implementation
}
```

4. **Register command** in `interpreter.cpp`:
```cpp
{"newskill", POS_FIGHTING, do_new_skill, 0, 0},
```

5. **Assign to classes** in `class.cpp`:
```cpp
skill_assign(SKILL_NEW_SKILL, CLASS_WARRIOR, 10);
skill_assign(SKILL_NEW_SKILL, CLASS_MERCENARY, 15);
```

6. **Add help text** to `lib/text/help/commands.hlp`

### Best Practices

1. **Validation**: Always validate skill prerequisites before execution
2. **Messages**: Provide clear feedback for success, failure, and invalid usage
3. **Balance**: Consider cooldowns, mana costs, and damage scaling
4. **Documentation**: Update help files and this reference document
5. **Testing**: Test with multiple classes and edge cases

### Common Patterns

**Skill Check**:
```cpp
if (GET_ISKILL(ch, SKILL_NAME) < random_number(1, 101)) {
    // Skill failed
} else {
    // Skill succeeded
}
```

**Cooldown**:
```cpp
if (GET_SKILL_COOLDOWN(ch, SKILL_NAME) > 0) {
    char_printf(ch, "You must wait before using that skill again.\n");
    return;
}
SET_SKILL_COOLDOWN(ch, SKILL_NAME, cooldown_time);
```

**Damage Calculation**:
```cpp
int dam = dice(GET_LEVEL(ch) / 5, 8) + GET_DAMROLL(ch);
dam = dam * GET_ISKILL(ch, SKILL_NAME) / 100;
```

---

## References

- **Source Files**: `legacy/src/skills.cpp`, `legacy/src/class.cpp`, `legacy/src/act.offensive.cpp`
- **Data Files**: `/home/strider/Code/mud/fierymud/skill_metadata_comprehensive.json`
- **Analysis**: `/home/strider/Code/mud/fierymud/SKILL_SYSTEM_ANALYSIS.md`
- **Spell Documentation**: `/home/strider/Code/mud/docs/SPELL_SYSTEM.md` (for comparison)

---

**Generated**: 2025-01-06
**Version**: 1.0
**Maintainer**: FieryMUD Development Team

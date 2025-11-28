# FieryMUD Skill System - Comprehensive Analysis

## Executive Summary

The FieryMUD skill system consists of **117 unique skills, songs, and chants** distributed across combat, utility, movement, stealth, and class-specific abilities. This document provides a complete analysis of skill definitions, implementations, and class assignments extracted from the legacy C++ codebase.

### Key Statistics
- **Total Skills**: 89 combat/utility skills (IDs 401-495)
- **Bard Songs**: 10 songs (IDs 551-560)
- **Monk Chants**: 18 chants (IDs 601-618)
- **Implemented**: 54/117 (46.2%) have explicit implementations
- **Class-Assigned**: 56/117 (47.9%) have class-level assignments
- **Humanoid-Only**: 24/117 (20.5%) restricted to humanoid characters

---

## 1. Skill Categories

### Combat Skills (37 skills)
Active combat abilities including attacks, defensive maneuvers, and special techniques.

**Attack Skills**:
- `backstab` (401) - Stealth attack from behind [Assassin/Rogue/Thief]
- `bash` (402) - Shield bash to stun opponents [Warrior/Paladin/Anti-Paladin]
- `kick` (404) - Basic melee kick attack [Multiple classes]
- `punch` (406) - Unarmed strike [Monk/Thief]
- `bodyslam` (427) - Full-body tackle [Mercenary/Berserker]
- `sweep` (436) - Leg sweep to knock down enemies [Monk]
- `roundhouse` (495) - Spinning kick attack [Monk/Berserker]
- `maul` (485) - Vicious melee attack [Animal forms]
- `throatcut` (418) - Lethal throat attack [Assassin/Rogue]
- `eye gouge` (472) - Blinds opponent temporarily [Rogue]
- `rend` (494) - Tears enemy defenses [Druid forms]

**Defensive Skills**:
- `parry` (420) - Deflect incoming attacks [Warrior/Paladin/Anti-Paladin]
- `dodge` (421) - Avoid attacks through agility [Monk/Rogue/Ranger]
- `riposte` (422) - Counter-attack after parry [Paladin/Anti-Paladin]
- `guard` (434) - Protect group members [Paladin/Anti-Paladin]

**Special Combat**:
- `berserk` (413) - Rage mode for increased damage [Berserker]
- `disarm` (431) - Remove opponent's weapon [Warrior classes]
- `rescue` (407) - Intervene to protect allies [Warrior/Paladin]
- `switch` (430) - Change combat targets [Anti-Paladin]
- `corner` (475) - Trap opponent [Rogue]
- `bind` (428) - Restrain target [Multiple classes]
- `hitall` (441) - Attack all enemies in room [High-level warriors]
- `instant kill` (440) - One-shot kill ability [NPC/special]

**Breath Weapons** (Draconic):
- `breathe acid` (488)
- `breathe fire` (486)
- `breathe frost` (487)
- `breathe gas` (489)
- `breathe lightning` (435)

**Support/Recovery**:
- `bandage` (443) - Basic wound treatment [All classes]
- `first aid` (444) - Advanced healing [Ranger/Druid]
- `douse` (438) - Extinguish flames [All classes]
- `tame` (417) - Befriend animals [Ranger/Druid]
- `lure` (492) - Attract creatures [Bard]

### Weapon Proficiencies (9 skills)
Passive skills governing weapon effectiveness. All humanoid-only.

- `bludgeoning weapons` (463) - Clubs, maces, hammers
- `piercing weapons` (464) - Daggers, spears, rapiers
- `slashing weapons` (465) - Swords, axes, scimitars
- `2H bludgeoning weapons` (466) - Two-handed blunt weapons
- `2H piercing weapons` (467) - Two-handed piercing weapons
- `2H slashing weapons` (468) - Two-handed slashing weapons
- `missile weapons` (469) - Bows, crossbows, slings
- `barehand` (449) - Unarmed combat (Monk specialty)
- `dual wield` (411) - Fight with two weapons [Ranger/Rogue/Bard]

### Movement Skills (11 skills)
Abilities for travel, positioning, and tactical retreat.

- `sneak` (408) - Move silently [Thief/Rogue/Assassin]
- `hide` (403) - Conceal presence [Thief/Rogue/Ranger]
- `track` (410) - Follow trails [Ranger/Hunter/Druid]
- `mount` (415) - Ride creatures [All classes]
- `riding` (416) - Mounted combat [All classes]
- `retreat` (473) - Tactical withdrawal [Anti-Paladin/Mercenary]
- `group retreat` (474) - Coordinated escape [Ranger/Bard]
- `safefall` (448) - Reduce fall damage [Monk/Thief]
- `springleap` (414) - Acrobatic jump attack [Monk/Rogue]
- `summon mount` (450) - Call personal mount [Paladin/Anti-Paladin]
- `sneak attack` (493) - Bonus damage from stealth [Rogue/Assassin]

### Stealth Skills (8 skills)
Covert operations and subterfuge.

- `backstab` (401) - Primary assassination technique
- `circle` (426) - Re-position for backstab mid-combat [Assassin/Rogue]
- `steal` (409) - Pickpocket items/gold [Thief/Rogue]
- `pick lock` (405) - Open locked doors/containers [Thief/Rogue]
- `shadow` (477) - Enhanced concealment [Assassin]
- `conceal` (478) - Hide objects/weapons [Rogue]
- `stealth` (476) - Master stealth ability [Rogue]
- `throatcut` (418) - Silent assassination

### Passive Skills (3 skills)
Always-active or toggle abilities.

- `double attack` (412) - Extra attack per round [High-level warriors]
- `meditate` (423) - Improve spell memorization [Caster classes]
- `quick chant` (424) - Faster spell casting [Caster classes]

### Utility Skills (9 skills)
Miscellaneous non-combat abilities.

- `perform` (490) - Bard performance for effects
- `chant` (446) - Monk chanting ability
- `scribe` (447) - Copy spells to spellbook [Wizard classes]
- `vampiric touch` (445) - Drain life force [Anti-Paladin]
- `shapechange` (429) - Transform into creatures [Druid]
- `hunt` (442) - Advanced tracking [Ranger/Hunter]
- `switch` (430) - Combat target switching
- `battle howl` (484) - Rallying cry [Berserker]
- `ground shaker` (483) - Area earthquake [Large creatures]
- `tantrum` (482) - Rage ability [NPCs]

### Spell Spheres (12 skills)
Meta-skills representing spell schools for casters.

- `sphere of generic` (452) - Universal magic
- `sphere of fire` (453) - Fire magic specialization
- `sphere of water` (454) - Water/ice magic
- `sphere of earth` (455) - Earth/stone magic
- `sphere of air` (456) - Air/lightning magic
- `sphere of healing` (457) - Restoration magic
- `sphere of protection` (458) - Defensive magic
- `sphere of enchantment` (459) - Enhancement magic
- `sphere of summoning` (460) - Conjuration magic
- `sphere of death` (461) - Necromancy
- `sphere of divination` (462) - Information magic
- `spell knowledge` (451) - General spellcasting ability

### Bard Songs (10 songs, IDs 551-560)
Musical abilities unique to the Bard class. All use performance skill.

- `inspiration` (551) - Buff allies with courage
- `terror` (552) - Frighten enemies
- `enrapture` (553) - Charm and distract [Quest]
- `hearthsong` (554) - Create illusions [Quest]
- `crown of madness` (555) - Confuse enemies [Quest]
- `song of rest` (556) - Healing over time
- `ballad of tears` (557) - Area fear effect
- `heroic journey` (558) - Group inspiration buff
- `freedom song` (559) - Remove fear/paralysis
- `joyful noise` (560) - Remove negative effects

### Monk Chants (18 chants, IDs 601-618)
Spiritual abilities for Monk and Berserker classes.

**Basic Chants**:
- `regeneration` (601) - Health regeneration buff
- `battle hymn` (602) - Combat enhancement
- `war cry` (603) - Group determination buff
- `peace` (604) - End combat peacefully

**Offensive Chants**:
- `aria of dissonance` (607) - Sound-based attack [Quest]
- `sonata of malaise` (608) - Debuff enemies
- `apocalyptic anthem` (609) - Massive destruction [Quest]
- `seed of destruction` (610) - Disease attack [Quest]

**Defensive Chants**:
- `shadows sorrow song` (605) - Mental debuff on enemies
- `ivory symphony` (606) - Paralyze foes

**Spirit Chants**:
- `spirit of the wolf` (611) - Wolf spirit enhancement
- `spirit of the bear` (612) - Bear spirit enhancement
- `interminable wrath` (613) - Sustained rage

**Elemental Chants** (Saint Augustine series, all Quest):
- `hymn of saint augustine` (614) - Elemental mastery
- `fires of saint augustine` (615) - Inner fire
- `blizzards of saint augustine` (616) - Inner cold
- `tremors of saint augustine` (617) - Inner earth
- `tempest of saint augustine` (618) - Inner storm

---

## 2. Implementation Analysis

### Implementation Patterns

**ACMD (Command-based)**: 47 skills
- Standard command implementation using `ACMD(do_skillname)` macro
- Located primarily in `act.offensive.cpp`, `act.movement.cpp`, `act.other.cpp`
- Examples: `do_backstab`, `do_bash`, `do_kick`, `do_hide`, `do_sneak`

**Switch Case**: 9 skills
- Implemented in switch statements (often in `spell_parser.cpp` for songs/chants)
- Shared implementation logic with spell system
- Examples: Most monk chants and bard songs

**Unimplemented**: 63 skills (53.8%)
- Defined but no clear implementation found
- May be passive, NPC-only, or legacy definitions
- Includes: Most weapon proficiencies, spell spheres, some special abilities

### Implementation Files

| File | Skills Implemented | Primary Focus |
|------|-------------------|---------------|
| `act.offensive.cpp` | 15+ | Combat attacks (backstab, bash, kick) |
| `act.movement.cpp` | 8+ | Movement (sneak, hide, track) |
| `act.other.cpp` | 12+ | Utility (bandage, pick lock, steal) |
| `warrior.cpp` | 6+ | Warrior-specific (bash, rescue, disarm) |
| `rogue.cpp` | 7+ | Rogue-specific (backstab, throatcut) |
| `fight.cpp` | Various | Passive combat skills (parry, dodge) |
| `spell_parser.cpp` | 9+ | Songs and chants |

### Configurability Assessment

**Highly Configurable** (Data-driven):
- All skill metadata (name, humanoid flag, targets, violent flag)
- Class-level assignments via `skill_assign()` calls
- Minimum position, fighting allowed, quest status
- Wearoff messages

**Hardcoded Implementation**:
- Damage calculations
- Success/failure logic
- Special effects and conditions
- Cooldown timers
- Skill improvement rates

**Modernization Opportunity**:
- Move implementation logic to configuration data
- Create skill effect templates (similar to spell system)
- Extract damage formulas to configuration
- Implement skill scripting system (Lua integration)

---

## 3. Class Assignments

### Skills by Class

**Top 5 Classes by Skill Count**:
1. **Anti-Paladin**: 22 skills - Heavy combat focus with dark abilities
2. **Mercenary**: 22 skills - Versatile warrior with many weapon proficiencies
3. **Berserker**: 22 skills - Rage-based combat with chants
4. **Paladin**: 21 skills - Holy warrior with mount abilities
5. **Warrior**: 19 skills - Core combat skills

### Class-Specific Patterns

**Warrior Classes** (Warrior, Paladin, Anti-Paladin, Mercenary):
- All weapon proficiencies (levels 1-20)
- Core combat skills: bash, kick, rescue, disarm
- Defensive skills: parry, dodge, riposte
- Advanced: double attack, hitall, guard

**Rogue Classes** (Thief, Rogue, Assassin):
- Stealth skills: sneak, hide, backstab, steal
- Utility: pick lock, steal
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

## 4. Skill Properties

### Targeting Flags

- **TAR_CONTACT**: Direct melee contact (can be blocked by guard)
- **TAR_DIRECT**: Ranged/directed attack (cannot be blocked by guard)
- **TAR_IGNORE**: No specific target (area effect)
- **TAR_CHAR_ROOM**: Target character in same room
- **TAR_SELF_ONLY**: Can only target self

### Violence Flags

- **Violent Skills** (43): Combat-oriented, trigger combat mode
  - All contact/direct attacks
  - Offensive songs/chants
  - Aggressive utility (steal, disarm)

- **Non-Violent Skills** (74): Utility, passive, or supportive
  - Movement skills
  - Defensive abilities
  - Healing/support
  - Utility skills

### Humanoid Restriction

**Humanoid-Only Skills** (24):
- All weapon proficiencies (require hands)
- Fine manipulation: pick lock, steal, scribe
- Complex combat: backstab, throatcut, eye gouge
- Social: perform

**Universal Skills** (93):
- Natural attacks: kick, bite, claw
- Animal skills: sneak, hide, track
- Innate abilities: dodge, berserk
- All songs and chants

### Quest Skills (11)

Special skills requiring quest completion:
- Songs: enrapture, hearthsong, crown of madness
- Chants: apocalyptic anthem, aria of dissonance, seed of destruction
- Elemental chants: All 5 Saint Augustine chants

---

## 5. Skill System Architecture

### Data Structure (SkillDef)

```cpp
struct SkillDef {
    char *name;                    // Skill name
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

1. **init_skills()** - Called on boot (skills.cpp:430)
   - Clears all skill definitions
   - Defines all skills via `skillo()`, `chanto()`, `songo()` macros
   - Sets base properties (name, targets, violent flag)

2. **Class Assignments** - In class.cpp
   - `skill_assign(skill, class, level)` sets minimum level per class
   - Common skills assigned to all classes in loops
   - Class-specific skills assigned individually

3. **Race Assignments** - In races.cpp
   - Similar to class assignments but for racial skills
   - Natural abilities, racial bonuses

### Skill Usage Flow

1. **Command Parsing** - `interpreter.cpp`
   - Player enters command (e.g., "backstab goblin")
   - Command table maps to `ACMD(do_backstab)`

2. **Validation** - In skill implementation
   - Check if player has skill
   - Check skill level (GET_ISKILL)
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

## 6. Modernization Recommendations

### High Priority

1. **Configuration Data Migration**
   - Extract skill formulas to JSON/config files
   - Move damage calculations out of hardcoded logic
   - Create skill effect templates

2. **Implementation Standardization**
   - Unify skill execution through common framework
   - Reduce code duplication across similar skills
   - Template-based skill effects

3. **Database Integration**
   - Store skill assignments in database
   - Dynamic class/race skill assignments
   - Runtime configuration changes

### Medium Priority

4. **Skill Scripting System**
   - Lua integration for skill effects
   - Allow builders to create custom skills
   - Hot-reload skill definitions

5. **Enhanced Skill System**
   - Skill trees and prerequisites
   - Skill point allocation system
   - Specialization branches

6. **Balance Framework**
   - Centralized damage/effect calculations
   - Automatic balance testing
   - Data-driven tuning

### Low Priority

7. **Advanced Features**
   - Combo system (skill chains)
   - Contextual skill availability
   - Dynamic skill cooldowns
   - Skill evolution/upgrades

---

## 7. Data Files Generated

### skill_metadata.json
Basic skill metadata from skills.cpp definitions.

**Fields**: id, name, type, humanoid, targets, wearoff, fighting_ok, violent, implementations, help_text

### skill_metadata_comprehensive.json
Complete skill data including class assignments.

**Additional Fields**: class_levels (dict of class -> level), categories

### skill_implementation_analysis.json
Implementation pattern analysis and categorization.

**Sections**: categories, implementation_patterns, class_coverage

### skill_summary.md
Human-readable summary with categorized skill lists.

---

## 8. Notable Findings

### Strengths
- Clear separation of concerns (definition vs implementation)
- Flexible class assignment system
- Rich variety of skills across classes
- Well-documented skill properties

### Weaknesses
- 54% of skills lack clear implementations
- Heavy code duplication in skill implementations
- Hardcoded formulas make balancing difficult
- No centralized skill effect system

### Opportunities
- Modernize to data-driven architecture
- Implement skill scripting system
- Create skill builder tools
- Add dynamic skill systems (trees, evolution)

### Legacy Debt
- Many unused/incomplete skill definitions
- Inconsistent implementation patterns
- Mixed ACMD vs switch-case approaches
- Passive skills poorly documented

---

## Appendix: Complete Skill List by ID

### Skills (401-495)
```
401  backstab              426  circle                451  spell knowledge       476  stealth
402  bash                  427  bodyslam              452  sphere of generic     477  shadow
403  hide                  428  bind                  453  sphere of fire        478  conceal
404  kick                  429  shapechange           454  sphere of water       479  peck
405  pick lock             430  switch                455  sphere of earth       480  claw
406  punch                 431  disarm                456  sphere of air         481  electrify
407  rescue                432  disarm_fumbling_weap  457  sphere of healing     482  tantrum
408  sneak                 433  disarm_dropped_weap   458  sphere of prot        483  ground shaker
409  steal                 434  guard                 459  sphere of enchant     484  battle howl
410  track                 435  breathe lightning     460  sphere of summon      485  maul
411  dual wield            436  sweep                 461  sphere of death       486  breathe fire
412  double attack         437  roar                  462  sphere of divin       487  breathe frost
413  berserk               438  douse                 463  bludgeoning           488  breathe acid
414  springleap            439  aware                 464  piercing              489  breathe gas
415  mount                 440  instant kill          465  slashing              490  perform
416  riding                441  hitall                466  2H bludgeoning        491  cartwheel
417  tame                  442  hunt                  467  2H piercing           492  lure
418  throatcut             443  bandage               468  2H slashing           493  sneak attack
419  doorbash              444  first aid             469  missile               494  rend
420  parry                 445  vampiric touch        470  [unused]              495  roundhouse
421  dodge                 446  chant                 471  lay hands
422  riposte               447  scribe                472  eye gouge
423  meditate              448  safefall              473  retreat
424  quick chant           449  barehand              474  group retreat
425  2back [unused]        450  summon mount          475  corner
```

### Songs (551-560)
```
551  inspiration           555  crown of madness      559  freedom song
552  terror                556  song of rest          560  joyful noise
553  enrapture             557  ballad of tears
554  hearthsong            558  heroic journey
```

### Chants (601-618)
```
601  regeneration          608  sonata of malaise     615  fires of saint augustine
602  battle hymn           609  apocalyptic anthem    616  blizzards of saint augustine
603  war cry               610  seed of destruction   617  tremors of saint augustine
604  peace                 611  spirit of the wolf    618  tempest of saint augustine
605  shadows sorrow song   612  spirit of the bear
606  ivory symphony        613  interminable wrath
607  aria of dissonance    614  hymn of saint augustine
```

---

**Generated**: 2025-01-06
**Source**: FieryMUD legacy codebase (legacy/src/)
**Extraction Tools**: extract_skill_metadata.py, extract_comprehensive_skill_data.py

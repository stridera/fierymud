# FieryMUD Ability Effect Composition Enhancement

This document provides detailed effect composition for FieryMUD abilities extracted from actual C++ implementation.

## Effect Type System

### Core Effect Types

1. **DAMAGE** - Apply damage by formula, respecting resistances
2. **HEAL** - Restore HP by formula
3. **APPLY_AFFECT** - Apply a timed aura (buff/debuff)
4. **REMOVE_AFFECT** - Remove one or more auras
5. **SUMMON** - Spawn mob(s) in same room
6. **TELEPORT** - Move target to another room
7. **MESSAGE** - Send custom messages
8. **RESOURCE_CHANGE** - Affect mana/stamina/movement
9. **DISPEL** - Remove auras selectively
10. **CONDITION** - Scripted check that gates other effects
11. **MULTI_HIT** - Multiple damage instances
12. **AREA_EFFECT** - Affects multiple targets in room
13. **CREATE_OBJECT** - Create and place/equip object
14. **MODIFY_OBJECT** - Change object properties
15. **DELAYED_EFFECT** - Event-based recurring/delayed effects

## Sample Effect Compositions

### Damage Spells

#### Fireball (ID 26)

- **Type**: SPELL
- **Command**: `fireball`
- **Category**: DAMAGE
- **Target**: SINGLE_ENEMY (LINE_OF_SIGHT required)

**Effects:**
1. **DAMAGE**
   - Type: FIRE
   - Formula: `sorcerer_single_target(ch, spellnum, skill)`
     - Circle-based dice: Circle 3 = `roll_dice(4, 24) + pow(power, exponent)`
     - Exponent: `1.2 + 0.3 * minlevel/100.0 + (power - minlevel) * (0.004 * minlevel - 0.2)/100.0`
   - Saving Throw: SAVING_SPELL (half damage on save)
   - Respects: Fire resistance/susceptibility (0-200%)
   - Globe Evasion: Blocked by Minor Globe (≤Circle 3) or Major Globe (≤Circle 6)

**Implementation**: `magic.cpp::mag_damage()` (case SPELL_FIREBALL)
**Key Functions**:
- `evades_spell()` - Checks globe protection and elemental immunity
- `mag_savingthrow()` - Determines save success
- `susceptibility()` - Modifies damage by 0-200% based on resistance

---

#### Magic Missile (ID 32)

- **Type**: SPELL
- **Command**: `magic missile`
- **Category**: DAMAGE
- **Target**: SINGLE_ENEMY

**Effects:**
1. **MULTI_HIT**
   - Missiles: 1-7 (based on skill thresholds: 5/14/24/34/44/74)
   - Per missile: **DAMAGE**
     - Type: MAGICAL (force damage, DAM_PHYSICAL_BLUNT)
     - Formula: `mag_damage()` call per missile with `roll_dice(4, 21)` base
     - Saving Throw: Yes (SAVING_SPELL per missile)
   - Checks: `ALIVE(victim)` between missiles
   - Special: Each missile is a separate attack event

**Implementation**: `spells.cpp::spell_magic_missile()` (ASPELL)
**Loop Logic**:
```cpp
for (missiles = 1; missiles <= num_missiles; missiles++) {
    if (!ALIVE(victim)) break;
    mag_damage(skill, ch, victim, SPELL_MAGIC_MISSILE, SAVING_SPELL);
}
```

---

#### Harm (ID 27)

- **Type**: SPELL
- **Command**: `harm`
- **Category**: DAMAGE
- **Target**: SINGLE_ENEMY (TOUCH)

**Effects:**
1. **CONDITION** (if target is undead)
   - **HEAL**
     - Formula: `1d8 + 50`
     - Implementation: Calls `mag_point(SPELL_HEAL, ...)`
     - Message: "Divine energy flows into $N, healing $M!"

2. **DAMAGE** (if target is living)
   - Type: ALIGN (alignment-based damage)
   - Formula: `dam += (pow(skill, 2) * 41) / 5000`
   - Maximum: ~170 at skill 1000
   - Saving Throw: SAVING_SPELL (half damage)
   - Special: `ROOM_PEACEFUL` check prevents casting

**Implementation**: `spells.cpp::spell_degeneration()` (ASPELL)
**Condition Logic**:
```cpp
if (IS_UNDEAD(victim)) {
    // Heals undead
    mag_point(skill, ch, victim, SPELL_HEAL, 0);
} else {
    // Damages living
    mag_damage(skill, ch, victim, SPELL_HARM, SAVING_SPELL);
}
```

---

### Buff Spells

#### Bless (ID 3)

- **Type**: SPELL
- **Command**: `bless`
- **Category**: BUFF
- **Target**: SINGLE_ALLY or OBJECT

**Effects:**
1. **APPLY_AFFECT** (on character)
   - Effect Flag: `EFF_BLESS`
   - Duration: 6 hours (`get_spell_duration(ch, SPELL_BLESS)`)
   - Modifiers:
     - Hitroll: +2 (`af.modifier = 2; af.location = APPLY_HITROLL`)
     - Saving throw vs spells: +1 (`af.modifier = 1; af.location = APPLY_SAVE_SPELL`)
   - Wear-off: "You feel less righteous."
   - No-stack: Removes existing BLESS before applying

2. **APPLY_AFFECT** (on object)
   - Flag: `ITEM_BLESS`
   - Duration: Permanent
   - Requirements:
     - Non-evil item (!ITEM_ANTI_GOOD)
     - Weight ≤ 5 lbs per caster level
   - Message: "$p glows briefly."

**Implementation**: `magic.cpp::mag_affect()` (case SPELL_BLESS)
**Data Structure**:
```cpp
effect af;
af.type = SPELL_BLESS;
af.duration = get_spell_duration(ch, SPELL_BLESS);  // 6 hours
af.modifier = 2;
af.location = APPLY_HITROLL;
af.bitvector = EFF_BLESS;
```

---

#### Stone Skin (ID 52)

- **Type**: SPELL
- **Command**: `stone skin`
- **Category**: BUFF
- **Target**: SINGLE_CHARACTER

**Effects:**
1. **APPLY_AFFECT**
   - Effect Flag: `EFF_STONE_SKIN`
   - Duration: 2 hours
   - Modifiers:
     - AC: -40 (`af.modifier = -40; af.location = APPLY_AC`)
     - Absorb counter: Initial value based on skill
   - Wear-off: "Your skin feels soft and vulnerable again."
   - Damage Absorption: `decrease_modifier()` reduces counter on each hit

**Implementation**: `magic.cpp::mag_affect()` (case SPELL_STONE_SKIN)
**Special Mechanic**:
- Each physical hit reduces `modifier` by 1
- When modifier reaches 0, effect is removed
- Visual: "Your stone skin absorbs the blow!"

---

### Healing Spells

#### Cure Light (ID 16)

- **Type**: SPELL
- **Command**: `cure light`
- **Category**: UTILITY (HEALING)
- **Target**: SINGLE_CHARACTER

**Effects:**
1. **HEAL**
   - Formula: `1d8 + 5`
   - Implementation: `mag_point()` with `roll_dice(1, 8) + 5`
   - Cap: Cannot exceed max HP
   - Message: "You feel better."

**Implementation**: `magic.cpp::mag_point()` (case SPELL_CURE_LIGHT)

---

#### Cure Critic (ID 15)

- **Type**: SPELL
- **Command**: `cure critic`
- **Category**: UTILITY (HEALING)
- **Target**: SINGLE_CHARACTER

**Effects:**
1. **HEAL**
   - Formula: `3d8 + 15`
   - Implementation: `mag_point()` with `roll_dice(3, 8) + 15`
   - Cap: Cannot exceed max HP
   - Message: "You feel a lot better!"

**Implementation**: `magic.cpp::mag_point()` (case SPELL_CURE_CRITIC)

---

#### Heal (ID 28)

- **Type**: SPELL
- **Command**: `heal`
- **Category**: UTILITY (HEALING)
- **Target**: SINGLE_CHARACTER

**Effects:**
1. **HEAL**
   - Formula: `100 + roll_dice(3, 8)`
   - Implementation: `mag_point()` with `100 + roll_dice(3, 8)`
   - Cap: Cannot exceed max HP
   - Message: "A warm feeling floods your body."
   - Special: Removes `EFF_POISON` and `EFF_DISEASE` flags

**Implementation**: `magic.cpp::mag_point()` (case SPELL_HEAL)
**Additional Effects**:
```cpp
if (EFF_FLAGGED(victim, EFF_POISON) || EFF_FLAGGED(victim, EFF_DISEASE)) {
    remove_char_spell(victim, SPELL_POISON);
    remove_char_spell(victim, SPELL_DISEASE);
    char_printf(victim, "A warm feeling purges the poison from your body.\n");
}
```

---

### Debuff Spells

#### Blindness (ID 4)

- **Type**: SPELL
- **Command**: `blindness`
- **Category**: BUFF (DEBUFF)
- **Target**: SINGLE_ENEMY

**Effects:**
1. **APPLY_AFFECT**
   - Effect Flag: `EFF_BLIND`
   - Duration: 2-4 rounds
   - Modifiers:
     - Hitroll: -4 (`af.modifier = -4; af.location = APPLY_HITROLL`)
     - AC: +40 (worse AC) (`af.modifier = 40; af.location = APPLY_AC`)
   - Wear-off: "You can see again."
   - Saving Throw: SAVING_SPELL (negates entirely on save)
   - Immunity: Cannot blind if already blind

**Implementation**: `magic.cpp::mag_affect()` (case SPELL_BLINDNESS)

---

#### Sleep (ID 38)

- **Type**: SPELL
- **Command**: `sleep`
- **Category**: BUFF (DEBUFF)
- **Target**: SINGLE_CHARACTER

**Effects:**
1. **APPLY_AFFECT**
   - Effect Flag: `EFF_SLEEP`
   - Duration: 4 + level/4 rounds
   - Position Change: Sets `GET_POS(victim) = POS_SLEEPING`
   - Wear-off: (victim wakes up automatically)
   - Saving Throw: SAVING_SPELL (negates entirely on save)
   - Restrictions:
     - Cannot affect mobs with `MOB_NOSLEEP`
     - Cannot affect immortals
     - Reduced effectiveness vs high-level targets

**Implementation**: `magic.cpp::mag_affect()` (case SPELL_SLEEP)

---

### Utility Spells

#### Cure Blind (ID 14)

- **Type**: SPELL
- **Command**: `cure blind`
- **Category**: UTILITY
- **Target**: SINGLE_CHARACTER

**Effects:**
1. **REMOVE_AFFECT**
   - Removes: `EFF_BLIND`
   - Implementation: `mag_unaffect()` removes SPELL_BLINDNESS
   - Message: "Your vision returns!"
   - Failure: "There is no impairment to cure."

**Implementation**: `magic.cpp::mag_unaffect()` (case SPELL_CURE_BLIND)

---

#### Remove Poison (ID 43)

- **Type**: SPELL
- **Command**: `remove poison`
- **Category**: UTILITY
- **Target**: SINGLE_CHARACTER

**Effects:**
1. **REMOVE_AFFECT**
   - Removes: `EFF_POISON`
   - Implementation: `mag_unaffect()` removes SPELL_POISON
   - Message: "A warm feeling runs through your body."
   - Failure: "There is no poison to remove."

**Implementation**: `magic.cpp::mag_unaffect()` (case SPELL_REMOVE_POISON)

---

#### Dispel Magic (ID 99)

- **Type**: SPELL
- **Command**: `dispel magic`
- **Category**: UTILITY
- **Target**: SINGLE_ENEMY

**Effects:**
1. **DISPEL** (violent spells)
   - Removal Logic: Removes spells with `VIOLENT` flag
   - Saving Throw: SAVING_SPELL per effect (partial resistance)
   - Special Cases:
     - Always removes charm effects
     - Checks each effect individually
   - Message: "$N's magical auras flicker and fade."

**Implementation**: `spells.cpp::spell_dispel_magic()` (ASPELL)
**Dispel Logic**:
```cpp
void dispel_harmful_magic(CharData *ch) {
    for (eff = ch->effects; eff; eff = next) {
        next = eff->next;
        if (skills[eff->type].violent) {
            if (!mag_savingthrow(ch, SAVING_SPELL)) {
                active_effect_remove(ch, eff);
            }
        }
    }
}
```

---

### Teleport Spells

#### Word of Recall (ID 42)

- **Type**: SPELL
- **Command**: `word of recall`
- **Category**: UTILITY (TELEPORT)
- **Target**: SELF

**Effects:**
1. **TELEPORT**
   - Destination: `GET_HOMEROOM(ch)` (player's set recall point)
   - Restrictions:
     - Cannot use in `ROOM_NOTELEPORT` rooms
     - Cannot use while fighting (combat check)
   - Message: "$n disappears." / "You are yanked to safety!"
   - Post-teleport: `check_new_surroundings()`, `WAIT_STATE(ch, PULSE_VIOLENCE)`

**Implementation**: `spells.cpp::spell_recall()` (ASPELL)

---

#### Teleport (ID 2)

- **Type**: SPELL
- **Command**: `teleport`
- **Category**: TELEPORT
- **Target**: SELF

**Effects:**
1. **TELEPORT**
   - Destination: Random room in same zone
   - Restrictions:
     - Cannot teleport into `ROOM_NOTELEPORT` rooms
     - Cannot teleport into `ROOM_PRIVATE` rooms
     - Cannot teleport into death traps
   - Saving Throw: SAVING_SPELL (negates teleport)
   - Message: "$n slowly fades out of existence and is gone."

**Implementation**: `spells.cpp::spell_teleport()` (ASPELL)

---

### Summon Spells

#### Animate Dead (ID 45)

- **Type**: SPELL
- **Command**: `animate dead`
- **Category**: SUMMON
- **Target**: CORPSE (in room)

**Effects:**
1. **CONDITION** (requires corpse in room)
   - **SUMMON**
     - Mob VNUM: Varies based on corpse level
       - Level 1-10: Skeleton (VNUM varies)
       - Level 11-20: Zombie
       - Level 21+: Wight/Ghoul/Wraith
     - Count: 1
     - Duration: Permanent (until killed or spell wears off)
     - Follows: Yes (`add_follower()`)
     - Flags: `MOB_ANIMATED`, `EFF_ANIMATED`
   - **MODIFY_OBJECT** (consumes corpse)
     - Removes corpse from room
     - `extract_obj(corpse)`

**Implementation**: `magic.cpp::mag_summon()` (case SPELL_ANIMATE_DEAD)

---

#### Summon (ID 40)

- **Type**: SPELL
- **Command**: `summon`
- **Category**: UTILITY (SUMMON)
- **Target**: SINGLE_CHARACTER (ANYWHERE)

**Effects:**
1. **TELEPORT** (victim to caster)
   - Destination: Caster's room
   - Restrictions:
     - Cannot summon from `ROOM_NOSUMMON` rooms
     - Cannot summon into `ROOM_NOSUMMON` rooms
     - Cannot summon immortals
     - Cannot summon mobs with `MOB_NOSUMMON`
   - Saving Throw: SAVING_SPELL (negates entirely)
   - Message: "$n disappears suddenly." / "$N arrives suddenly."
   - Aggro: Sets `set_fighting(victim, ch, false)` if save fails

**Implementation**: `spells.cpp::spell_summon()` (ASPELL)

---

### Area Effect Spells

#### Earthquake (ID 23)

- **Type**: SPELL
- **Command**: `earthquake`
- **Category**: DAMAGE
- **Target**: AREA_ROOM (all enemies)

**Effects:**
1. **AREA_EFFECT**
   - Target Selection: All fighting enemies in room
   - Max Targets: Unlimited
   - Per Target: **DAMAGE**
     - Type: ACID (earth/crushing damage)
     - Formula: `dam += (pow(skill, 2) * 7) / 400`
     - Saving Throw: SAVING_SPELL (half damage)
   - Special: Flying characters take reduced damage
   - Message: "The earth trembles beneath your feet!"

**Implementation**: `magic.cpp::mag_area()` (case SPELL_EARTHQUAKE)

---

#### Chain Lightning (ID 111)

- **Type**: SPELL
- **Command**: `chain lightning`
- **Category**: DAMAGE
- **Target**: AREA_ROOM (all enemies)

**Effects:**
1. **AREA_EFFECT**
   - Target Selection: All fighting enemies in room
   - Max Targets: Unlimited
   - Per Target: **DAMAGE**
     - Type: SHOCK
     - Formula: `dam += (pow(skill, 2) * 7) / 500`
     - Saving Throw: SAVING_SPELL (half damage)
   - Special: Arc effect visual ("Lightning arcs between targets!")

**Implementation**: `magic.cpp::mag_area()` (case SPELL_CHAIN_LIGHTNING)

---

### Object Creation Spells

#### Flame Blade (ID 161)

- **Type**: SPELL
- **Command**: `flame blade`
- **Category**: UTILITY
- **Target**: AREA_ROOM

**Effects:**
1. **CREATE_OBJECT**
   - Object VNUM: 1043 (flame blade weapon)
   - Auto-equip: WIELD slot
   - Duration: Temporary (object has timer)
   - Properties:
     - Weapon type: Slashing
     - Damage: Fire-based
     - Flags: `ITEM_MAGIC`, `ITEM_NODONATE`
   - Message: "A blade of fire appears in your hand!"

**Implementation**: `spells.cpp::spell_flame_blade()` (ASPELL)

---

#### Create Food (ID 12)

- **Type**: SPELL
- **Command**: `create food`
- **Category**: UTILITY
- **Target**: AREA_ROOM

**Effects:**
1. **CREATE_OBJECT**
   - Object VNUM: 10 (mushroom food item)
   - Placement: Room floor
   - Duration: Temporary (object has timer)
   - Properties:
     - Type: FOOD
     - Nutrition: Fills stomach
   - Message: "A magic mushroom suddenly appears."

**Implementation**: `magic.cpp::mag_create_obj()` (case SPELL_CREATE_FOOD)

---

#### Create Water (ID 13)

- **Type**: SPELL
- **Command**: `create water`
- **Category**: UTILITY
- **Target**: DRINKCON object

**Effects:**
1. **MODIFY_OBJECT**
   - Target: Drink container in inventory
   - Modification: Adds water charges
   - Formula: `water_added = level`
   - Cap: Cannot exceed container max capacity
   - Message: "$p is filled."
   - Requirements: Must target a valid drink container

**Implementation**: `spells.cpp::spell_create_water()` (ASPELL)

---

### Delayed Effect Spells

#### Acid Fog (ID 245)

- **Type**: SPELL
- **Command**: `acid fog`
- **Category**: UTILITY (AREA DAMAGE)
- **Target**: AREA_ROOM

**Effects:**
1. **DELAYED_EFFECT**
   - Rounds: 4
   - Interval: 4 seconds (1 combat round)
   - Per Round: **AREA_EFFECT**
     - Target Selection: All enemies in room
     - **DAMAGE**
       - Type: ACID
       - Formula: `dam += (pow(skill, 2) * 7) / 1250`
       - Saving Throw: SAVING_SPELL (half damage)
   - Message: "A cloud of acid fog fills the room!"
   - Implementation: Event queue system (`event_create()`)

**Implementation**: `spells.cpp::spell_acid_fog()` + `spell_acid_fog_recur()` (ASPELL)

---

#### Pyre (ID 222)

- **Type**: SPELL
- **Command**: `pyre`
- **Category**: UTILITY (DELAYED DAMAGE)
- **Target**: AREA_ROOM

**Effects:**
1. **DELAYED_EFFECT**
   - Rounds: 5
   - Interval: 4 seconds
   - Per Round: **AREA_EFFECT**
     - Target Selection: All enemies in room
     - **DAMAGE**
       - Type: FIRE
       - Formula: Varies per round (increases over time)
       - Saving Throw: SAVING_BREATH (half damage)
   - Message: "A pillar of fire erupts from the ground!"
   - Special: Damage increases each round

**Implementation**: `spells.cpp::spell_pyre()` + `spell_pyre_recur()` (ASPELL)

---

## Skills

### Combat Skills

#### Backstab (ID 401)

- **Type**: SKILL
- **Command**: `backstab`
- **Category**: STEALTH
- **Target**: SINGLE_ENEMY (TOUCH)

**Effects:**
1. **CONDITION** (must be behind target, not fighting)
   - **DAMAGE**
     - Type: PIERCE (weapon-based)
     - Formula: Weapon damage × multiplier (2-5× based on skill)
       - Skill 0-200: 2× damage
       - Skill 201-400: 3× damage
       - Skill 401-600: 4× damage
       - Skill 601+: 5× damage
     - Backstab bonus: `(skill / 10)` added to damage
     - Saving Throw: None (but requires hit roll)
   - Requirements:
     - Must not be fighting
     - Must have piercing weapon equipped
     - Victim must not see attacker (stealth check)
   - **MESSAGE**
     - Success: "$n places $p in the back of $N!"
     - Failure: "$n tries to stab $N in the back, but fails!"
   - Initiates combat

**Implementation**: `act.offensive.cpp::do_backstab()`

---

#### Bash (ID 402)

- **Type**: SKILL
- **Command**: `bash`
- **Category**: COMBAT
- **Target**: SINGLE_ENEMY (TOUCH)

**Effects:**
1. **DAMAGE**
   - Type: PHYSICAL_BLUNT
   - Formula: `dam = (skill / 20)` (minimal damage, primarily for knockdown)
   - Saving Throw: None (but requires skill check vs target's dexterity)

2. **APPLY_AFFECT** (on success)
   - Position Change: `GET_POS(victim) = POS_SITTING`
   - Duration: 1 round (victim must stand up)
   - Message: "$n sends $N sprawling with a powerful bash!"
   - Wait State: `WAIT_STATE(victim, PULSE_VIOLENCE)`

**Implementation**: `act.offensive.cpp::do_bash()`

---

#### Kick (ID 404)

- **Type**: SKILL
- **Command**: `kick`
- **Category**: COMBAT
- **Target**: SINGLE_ENEMY (TOUCH)

**Effects:**
1. **DAMAGE**
   - Type: PHYSICAL_BLUNT
   - Formula: `GET_LEVEL(ch) / 2 + (skill / 20)`
   - Saving Throw: None (but requires hit roll)
   - Special: Monks get bonus damage from barefoot kicks
   - Message: "$n kicks $N in the stomach!"

**Implementation**: `act.offensive.cpp::do_kick()`

---

#### Disarm (ID 431)

- **Type**: SKILL
- **Command**: `disarm`
- **Category**: COMBAT
- **Target**: SINGLE_ENEMY (TOUCH)

**Effects:**
1. **CONDITION** (skill check vs target's strength/dexterity)
   - **MODIFY_OBJECT** (on success)
     - Removes weapon from victim's WIELD slot
     - Drops weapon to room floor
     - Message: "$n disarms $N, sending $s weapon flying!"
   - Failure: "$n tries to disarm $N, but fails!"
   - Requirements:
     - Victim must be wielding a weapon
     - Attacker must have weapon skill

**Implementation**: `act.offensive.cpp::do_disarm()`

---

### Movement Skills

#### Hide (ID 403)

- **Type**: SKILL
- **Command**: `hide`
- **Category**: MOVEMENT
- **Target**: SELF

**Effects:**
1. **APPLY_AFFECT**
   - Effect Flag: `EFF_HIDDEN`
   - Duration: Until detected or movement
   - Modifiers: None (purely visibility flag)
   - Detection: Perception check by others
   - Broken by: Movement, combat, failed skill check
   - Message: "You attempt to hide yourself."

**Implementation**: `act.other.cpp::do_hide()`

---

#### Sneak (ID 408)

- **Type**: SKILL
- **Command**: `sneak`
- **Category**: MOVEMENT
- **Target**: SELF

**Effects:**
1. **APPLY_AFFECT**
   - Effect Flag: `EFF_SNEAK`
   - Duration: Until detected or skill wears off
   - Modifiers: Reduces movement noise
   - Detection: Perception check per room entered
   - Message: "Okay, you'll try to move silently for a while."

**Implementation**: `act.other.cpp::do_sneak()` (assumed)

---

#### Track (ID 410)

- **Type**: SKILL
- **Command**: `track`
- **Category**: MOVEMENT
- **Target**: CHARACTER (anywhere in zone)

**Effects:**
1. **MESSAGE**
   - Success: "You sense a trail leading [direction]..."
   - Failure: "You can't sense a trail from here."
   - Implementation: Pathfinding algorithm to target
   - Requirements: Target must be in same zone

**Implementation**: `act.other.cpp::do_track()` (assumed)

---

### Defensive Skills

#### Rescue (ID 407)

- **Type**: SKILL
- **Command**: `rescue`
- **Category**: DEFENSIVE
- **Target**: ALLY (being attacked)

**Effects:**
1. **CONDITION** (ally must be in combat)
   - Changes combat target: Attacker now targets rescuer instead of ally
   - Message: "You bravely rescue $N!"
   - Requirements:
     - Target must be in same room
     - Target must be fighting
     - Rescuer must have clear path
   - Wait State: `WAIT_STATE(ch, PULSE_VIOLENCE)`

**Implementation**: `act.offensive.cpp::do_rescue()`

---

#### Parry (ID 420)

- **Type**: SKILL
- **Command**: (passive)
- **Category**: DEFENSIVE
- **Target**: SELF (automatic)

**Effects:**
1. **CONDITION** (automatic check on being attacked)
   - Negates incoming attack entirely
   - Skill Check: `skill > random_number(1, 1000)`
   - Requirements: Must be wielding weapon
   - Message: "You skillfully parry $n's attack!"
   - No resource cost

**Implementation**: Combat system (passive check)

---

#### Dodge (ID 421)

- **Type**: SKILL
- **Command**: (passive)
- **Category**: DEFENSIVE
- **Target**: SELF (automatic)

**Effects:**
1. **CONDITION** (automatic check on being attacked)
   - Negates incoming attack entirely
   - Skill Check: `skill + dex_bonus > random_number(1, 1000)`
   - Message: "You dodge $n's attack!"
   - No resource cost
   - Higher priority than parry

**Implementation**: Combat system (passive check)

---

## Effect Formulas Summary

### Damage Formulas by Circle

**Sorcerer Single-Target** (`sorcerer_single_target()`):
- Circle 1: `roll_dice(4, 19) + pow(power, exponent)`
- Circle 2: `roll_dice(5, 16) + pow(power, exponent)`
- Circle 3: `roll_dice(4, 24) + pow(power, exponent)`
- Circle 4: `roll_dice(6, 20) + pow(power, exponent)`
- Circle 5: `roll_dice(8, 25) + pow(power, exponent)`
- Circle 6: `roll_dice(10, 24) + pow(power, exponent)`
- Circle 7: `roll_dice(15, 17) + pow(power, exponent)`
- Circle 8: `roll_dice(15, 18) + pow(power, exponent)`
- Circle 9: `roll_dice(10, 35) + pow(power, exponent)`

**Exponent Calculation**:
```
exponent = 1.2 + 0.3 * minlevel/100.0 + (power - minlevel) * (0.004 * minlevel - 0.2)/100.0
```

### Healing Formulas

- Cure Light: `1d8 + 5`
- Cure Serious: `2d8 + 10`
- Cure Critic: `3d8 + 15`
- Heal: `100 + 3d8`
- Full Heal: Restores to max HP

### Duration Formulas

**Spell Duration** (`get_spell_duration()`):
- Varies by spell
- Bless: 6 hours
- Stone Skin: 2 hours
- Blindness: 2-4 rounds
- Sleep: 4 + level/4 rounds

### Resistance Modifiers

**Susceptibility** (`susceptibility()`):
- 0%: Complete immunity (spell evaded)
- 50%: Resistant (half damage)
- 100%: Normal damage
- 150%: Vulnerable (50% extra damage)
- 200%: Highly vulnerable (double damage)

## Implementation Files Reference

- **magic.cpp**: Core magic system (mag_damage, mag_affect, mag_point, mag_unaffect, mag_summon)
- **spells.cpp**: ASPELL implementations (complex multi-effect spells)
- **act.offensive.cpp**: Combat skills (backstab, bash, kick, disarm)
- **act.other.cpp**: Utility skills (hide, steal, bandage)
- **act.movement.cpp**: Movement skills (mount, tame)
- **casting.cpp**: Spell casting system and target resolution
- **damage.cpp**: Damage calculation and resistance application
- **skills.hpp**: Skill/spell data structures and definitions

---

This document provides a composable effect system extracted from actual FieryMUD C++ implementation. Each ability is broken down into its constituent effects with formulas, conditions, and implementation details.

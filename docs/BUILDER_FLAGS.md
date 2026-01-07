# FieryMUD Builder Flag Reference Guide

This document is a comprehensive reference for builders creating mobs, objects, rooms, and exits in FieryMUD.

## Overview

FieryMUD uses a modern flag system organized by entity type. Flags define **intrinsic properties** (what something IS), while temporary states use the **effects system** (what something is experiencing).

**Key Principles**:

- Flags = permanent properties that don't change during gameplay
- Effects = temporary states with duration, charges, or conditions
- Structured fields = typed data replacing flag bloat (restrictions, immunities)

---

## Table of Contents

1. [Mob Flags & Properties](#mob-flags--properties)
2. [Object Flags & Properties](#object-flags--properties)
3. [Room Flags & Properties](#room-flags--properties)
4. [Exit Flags & Properties](#exit-flags--properties)
5. [Effects System](#effects-system)
6. [Quick Reference Checklists](#quick-reference)

---

## Mob Flags & Properties

### Traits (What the mob IS)

Traits define the fundamental nature of a mobile.

| Trait           | Description                                                          |
| --------------- | -------------------------------------------------------------------- |
| ILLUSION        | Mob is an illusion, can be dispelled with true sight or dispel magic |
| ANIMATED        | Mob is animated (skeleton, golem), dies if animation effect removed  |
| PLAYER_PHANTASM | Illusion of a player; other mobs are aggressive toward it            |
| AQUATIC         | Can only exist in water sector rooms, dies if moved to land          |
| MOUNT           | Can be ridden as a mount by players                                  |
| SUMMONED        | Was magically summoned, can be banished or dismissed                 |
| PET             | Was purchased from a shop or tamed, follows owner                    |

**Examples**:

```yaml
# Animated skeleton
traits: [ANIMATED]

# Summoned mount (horse from spell)
traits: [SUMMONED, MOUNT]

# Illusory guard (decoy)
traits: [ILLUSION, PLAYER_PHANTASM]
```

---

### Behaviors (How the mob ACTS)

Behaviors define how the mob interacts with the world.

| Behavior                | Description                                        |
| ----------------------- | -------------------------------------------------- |
| **Movement**            |
| SENTINEL                | Won't wander from spawn room, stays put            |
| STAY_ZONE               | Won't leave its home zone boundaries               |
| SCAVENGER               | Picks up items from the ground automatically       |
| **Tracking**            |
| TRACK                   | Will track enemies who flee from combat            |
| SLOW_TRACK              | Tracks but with delays between moves               |
| FAST_TRACK              | Tracks quickly and persistently                    |
| **Combat Stance**       |
| WIMPY                   | Flees when health drops below threshold            |
| AWARE                   | Cannot be backstabbed or pickpocketed              |
| HELPER                  | Assists other mobs of same type in combat          |
| PROTECTOR               | Protects specified NPCs when they're attacked      |
| PEACEKEEPER             | Attacks anyone who initiates combat in the room    |
| **Combat Restrictions** |
| NO_BASH                 | Cannot be knocked down with bash                   |
| NO_SUMMON               | Cannot be summoned via gate/summon spells          |
| NO_VICIOUS              | Ignores vicious stance damage bonus                |
| PEACEFUL                | Cannot be attacked by players                      |
| NO_KILL                 | Fights but won't deliver the killing blow          |
| **Social**              |
| MEMORY                  | Remembers attackers, hostile on their return       |
| TEACHER                 | Can teach skills to players (guildmaster)          |
| MEDITATE                | Meditates to recover (for spell memorization NPCs) |
| **Special**             |
| NO_SCRIPT               | Won't run Lua script triggers                      |
| NO_CLASS_AI             | Doesn't use class-based AI behaviors               |

**Examples**:

```yaml
# City guard
behaviors: [SENTINEL, HELPER, PEACEKEEPER, MEMORY]

# Wandering merchant
behaviors: [STAY_ZONE, AWARE]

# Training dummy
behaviors: [SENTINEL, PEACEFUL, NO_KILL]
```

---

### Mob Roles (Service NPCs)

Mobs can have **multiple roles** that enable player services. Services are tied to **mobs**, not rooms.

| Role         | Service          | Description                                         |
| ------------ | ---------------- | --------------------------------------------------- |
| SHOPKEEPER   | Buy/Sell         | Sells items from inventory, buys items from players |
| BANKER       | Deposit/Withdraw | Manages player gold storage                         |
| RECEPTIONIST | Rent/Checkout    | Provides inn services (save on logout)              |
| POSTMASTER   | Mail             | Sends and receives player mail                      |
| GUILDMASTER  | Training         | Teaches skills/spells to qualifying players         |
| STABLEMASTER | Mounts           | Stores and retrieves player mounts                  |

**Design Note**: Services are mob-based because:

- NPCs can be placed anywhere (no special room required)
- If NPC dies/leaves, service becomes unavailable (logical)
- Avoids redundant room flags
- Roles can be combined for multi-function NPCs

**Examples**:

```yaml
# Basic shopkeeper
roles: [SHOPKEEPER]
inventory: [items...]

# Horse trader (sells AND stores mounts)
roles: [SHOPKEEPER, STABLEMASTER]
inventory: [horses, saddles, bridles...]

# Temple priest (sells resurrection)
roles: [SHOPKEEPER]
inventory: [{ability: "raise dead", price: 10000}]

# Warrior guildmaster (teaches warrior skills based on mob's class)
roles: [GUILDMASTER]
class: WARRIOR

# Banker in any room
roles: [BANKER]

# All-in-one town center NPC
roles: [SHOPKEEPER, BANKER, POSTMASTER]
```

**Note**: Guildmasters teach skills based on their own class assignment. A mob with `class: WARRIOR` and `roles: [GUILDMASTER]` teaches warrior skills.

---

### Aggression Targets

Defines who the mob will attack on sight. Multiple targets can be combined.

| Target            | Description                                         |
| ----------------- | --------------------------------------------------- |
| ALL               | Attacks everyone on sight (fully aggressive)        |
| EVIL_ALIGNMENT    | Attacks players with evil alignment (score <= -350) |
| GOOD_ALIGNMENT    | Attacks players with good alignment (score >= 350)  |
| NEUTRAL_ALIGNMENT | Attacks neutrally-aligned players (-349 to 349)     |
| EVIL_RACE         | Attacks inherently evil races (drow, demons, etc.)  |
| GOOD_RACE         | Attacks inherently good races (elves, etc.)         |
| NEUTRAL_RACE      | Attacks neutrally-aligned races                     |

**Examples**:

```yaml
# City guard - attacks evil players
aggroTargets: [EVIL_ALIGNMENT]

# Paladin NPC - attacks evil players and evil races
aggroTargets: [EVIL_ALIGNMENT, EVIL_RACE]

# Demon - attacks good races
aggroTargets: [GOOD_RACE]

# Berserk monster - attacks everyone
aggroTargets: [ALL]

# Passive shopkeeper - no aggression
aggroTargets: []
```

---

### Immunities

Two-layer immunity system for flexible mob design.

#### Effect Immunities (`immuneEffects: String[]`)

List of effect type names the mob is immune to:

| Effect      | Blocks                             |
| ----------- | ---------------------------------- |
| "charm"     | Charm person, dominate             |
| "sleep"     | Sleep spells                       |
| "fear"      | Fear, terror                       |
| "blind"     | Blindness                          |
| "silence"   | Silence spells                     |
| "knockdown" | Bash, trip                         |
| "stun"      | Stunning effects                   |
| "poison"    | Poison effects (not poison damage) |
| "disease"   | Disease effects                    |
| "summon"    | Being summoned/gated               |

#### Damage Type Immunities (`immuneDamageTypes: DamageType[]`)

List of damage types the mob takes no damage from:

| Type      | Examples                                     |
| --------- | -------------------------------------------- |
| FIRE      | Fire spells, burning                         |
| COLD      | Ice spells, freezing                         |
| ACID      | Acid attacks                                 |
| LIGHTNING | Lightning bolt, shocking                     |
| POISON    | Poison damage (different from poison effect) |
| SLASH     | Slashing weapons                             |
| PIERCE    | Piercing weapons                             |
| BLUDGEON  | Bludgeoning weapons                          |
| HOLY      | Holy damage                                  |
| UNHOLY    | Unholy/negative damage                       |

**Examples**:

```yaml
# Mindless golem - immune to mental effects
immuneEffects: ["charm", "sleep", "fear", "knockdown"]
immuneDamageTypes: [POISON]

# Fire elemental - only immune to fire damage
immuneEffects: []
immuneDamageTypes: [FIRE]

# Ethereal spirit - immune to physical
immuneEffects: ["summon"]
immuneDamageTypes: [SLASH, PIERCE, BLUDGEON]

# Undead - immune to many effects
immuneEffects: ["charm", "sleep", "fear", "poison", "disease"]
immuneDamageTypes: [POISON, COLD]
```

---

### Activity Restrictions

Lua expressions that control when the mob is active. If the expression evaluates to `false`, the mob becomes dormant or weakened.

| Expression                          | Use Case                            |
| ----------------------------------- | ----------------------------------- |
| `not world.is_sunny()`              | Vampire - hates sunlight            |
| `time.hour >= 20 or time.hour <= 6` | Nocturnal creature (active 8pm-6am) |
| `room.is_indoors()`                 | Indoor-only creature                |
| `not world.is_raining()`            | Weather-sensitive                   |
| `world.is_full_moon()`              | Werewolf - active during full moon  |

**Example**:

```yaml
# Vampire - weakened in sunlight
activityRestrictions: "not world.is_sunny() or room.is_indoors()"

# Nocturnal predator
activityRestrictions: "time.hour >= 20 or time.hour <= 6"
```

---

## Object Flags & Properties

### Object Flags

Intrinsic properties of items.

| Flag                      | Description                                                                                   |
| ------------------------- | --------------------------------------------------------------------------------------------- |
| **Visual/Audio**          |
| GLOW                      | Emits light, illuminates room when equipped/held                                              |
| HUM                       | Makes noise, can be found when invisible via sound                                            |
| **Permanence**            |
| PERMANENT                 | Cannot be destroyed by any means                                                              |
| UNIQUE                    | A character can only hold one of this item (world instance limits use maxInstances in resets) |
| SOULBOUND                 | Kept in inventory on death, cannot trade or drop                                              |
| **Physics & Transport**   |
| FLOAT                     | Doesn't fall in air/gravity rooms (feathers, etc.)                                            |
| BUOYANT                   | Doesn't sink in water (corks, boats)                                                          |
| VEHICLE                   | Can carry passengers (combine with FLOAT/BUOYANT for terrain)                                 |
| **Behavior Restrictions** |
| NO_TAKE                   | Cannot be picked up (furniture, fixtures)                                                     |
| NO_DONATE                 | Cannot be donated                                                                             |
| NO_INVISIBLE              | Cannot be made invisible via spells                                                           |
| NO_DROP                   | Cannot be dropped (but CAN be traded/given)                                                   |
| NO_SELL                   | Cannot be sold to shops                                                                       |
| NO_LOCATE                 | Cannot be found with locate object spell                                                      |
| **Source Flags**          |
| TRUSTED_SOURCE            | Fountain/keg - liquids drawn are auto-identified                                              |
| INHERENTLY_MAGICAL        | Always detects as magical (even without enchantments)                                         |

---

### Equipment Restrictions

Structured fields replace the old Anti-\* flags for cleaner configuration.

| Field                  | Type        | Description                                    |
| ---------------------- | ----------- | ---------------------------------------------- |
| `restrictedClasses`    | Class[]     | Classes that CANNOT use this item              |
| `restrictedAlignments` | Alignment[] | Alignments that CANNOT use this item           |
| `minSize`              | Size        | Minimum character size to use                  |
| `maxSize`              | Size        | Maximum character size to use                  |
| `allowedRaces`         | Race[]      | Only these races can use (empty = all allowed) |

**Examples**:

```yaml
# Holy sword - no evil users or evil classes
restrictedClasses: [NECROMANCER, ANTI_PALADIN]
restrictedAlignments: [EVIL]

# Giant's belt - large creatures only
minSize: LARGE

# Elven bow - elves and half-elves only
allowedRaces: [ELF, HALF_ELF]
# Thieves' tools - thieves and assassins only (via allowedClasses)
# Note: Use allowedClasses if you want a whitelist instead of blacklist
```

---

### Weapon Grip System

Controls how weapons are held and wielded.

| Grip       | Description             | Damage           | Offhand Available |
| ---------- | ----------------------- | ---------------- | ----------------- |
| ONE_HANDED | Single hand only        | Normal           | Yes               |
| TWO_HANDED | Requires both hands     | Higher           | No                |
| VERSATILE  | Player chooses 1H or 2H | Normal or +bonus | Depends on choice |

**Player Commands**:

- `wield <weapon>` - Equip weapon (defaults to 1H for versatile)
- `grip` or `changegrip` - Toggle between 1H and 2H grip for versatile weapons

**Configuration Examples**:

```yaml
# Dagger - can be mainhand or offhand
wearSlots: [MAINHAND, OFFHAND]
weaponGrip: ONE_HANDED

# Longsword - versatile, player chooses
wearSlots: [MAINHAND]
weaponGrip: VERSATILE

# Greatsword - two-handed only
wearSlots: [MAINHAND]
weaponGrip: TWO_HANDED

# Staff - two-handed
wearSlots: [MAINHAND]
weaponGrip: TWO_HANDED
```

---

### Wear Slots

Equipment positions on a character.

| Slot                  | Location        | Examples                                   |
| --------------------- | --------------- | ------------------------------------------ |
| **Head**              |
| HEAD                  | Top of head     | Helmets, hats, crowns                      |
| FACE                  | Face            | Masks, veils                               |
| EYES                  | Eyes            | Goggles, glasses, blindfolds               |
| EAR                   | Ears            | Earrings, ear cuffs                        |
| NECK                  | Neck            | Necklaces, amulets, collars                |
| THROAT                | Throat          | Throat protection, chokers                 |
| **Torso**             |
| BODY                  | Torso           | Armor, robes, shirts                       |
| ABOUT                 | Over body       | Cloaks, capes                              |
| **Arms & Hands**      |
| ARMS                  | Arms            | Arm guards, sleeves                        |
| WRIST                 | Wrists          | Bracelets, bracers                         |
| HANDS                 | Hands           | Gloves, gauntlets                          |
| FINGER                | Fingers         | Rings (system tracks which finger)         |
| **Lower Body**        |
| WAIST                 | Waist           | Belts, sashes, girdles                     |
| BELT                  | On belt         | Pouches, scabbards, tools (requires WAIST) |
| LEGS                  | Legs            | Pants, greaves, leggings                   |
| FEET                  | Feet            | Boots, shoes                               |
| **Special**           |
| TAIL                  | Tail            | Decorations (tailed races only)            |
| BADGE                 | Visible badge   | Insignia, emblems                          |
| FOCUS                 | Magic focus     | Spell foci, holy symbols                   |
| HOVER                 | Floating nearby | Ioun stones, familiars                     |
| WINGS                 | Back/wings      | Wings, jetpacks                            |
| DISGUISE              | Disguise slot   | Disguise items                             |
| **Hands (Equipment)** |
| MAINHAND              | Primary hand    | Weapons, tools                             |
| OFFHAND               | Secondary hand  | Shields, torches, offhand weapons          |

**Multi-Slot Items**:

```yaml
# Dagger - either hand
wearSlots: [MAINHAND, OFFHAND]

# Shield - offhand only
wearSlots: [OFFHAND]

# Spiked shield - can be used as weapon too
wearSlots: [MAINHAND, OFFHAND]
weaponGrip: ONE_HANDED

# Ring - system tracks which finger
wearSlots: [FINGER]
```

**Slot Dependencies**:

- **BELT requires WAIST**: You must have something worn on WAIST to use BELT slot. If WAIST item is removed, BELT items fall to ground/inventory.

**Disguise Slot**: The DISGUISE slot is for items that alter how other players/NPCs perceive the wearer:

Common Mechanics:

- Wearing a disguise changes your visible name/description to others
- look <player> shows the disguise description instead of the real one
- NPCs might treat you differently based on the disguise (enemy faction uniform, guard disguise, etc.)
- Usually revealed by certain skills (true sight, perception checks) or actions (combat, speaking)

Example Items:

- Cultist robes (appear as a cultist to faction NPCs)
- Guard uniform (access restricted areas)
- Glamour mask (appear as a different race)
- Cloak of shadows (appear as "a shadowy figure")

---

### Vehicle Objects

Combine physics flags to create different vehicle types.

| Object Type   | BUOYANT | FLOAT | VEHICLE | passengerCapacity | Terrain               |
| ------------- | ------- | ----- | ------- | ----------------- | --------------------- |
| Cork          | Yes     |       |         | -                 | Floats (not rideable) |
| Rowboat       | Yes     |       | Yes     | 4                 | Water only            |
| Flying carpet |         | Yes   | Yes     | 2                 | Air only              |
| Airship       | Yes     | Yes   | Yes     | 10                | Air + water landing   |
| Horse (mount) |         |       | Yes     | 2                 | Ground                |

**Example Configurations**:

```yaml
# Small rowboat
flags: [BUOYANT, VEHICLE]
passengerCapacity: 2

# Flying carpet
flags: [FLOAT, VEHICLE]
passengerCapacity: 2

# Large airship
flags: [BUOYANT, FLOAT, VEHICLE]
passengerCapacity: 10
```

---

## Room Flags & Properties

### Room Flags

| Flag                       | Description                                |
| -------------------------- | ------------------------------------------ |
| **Lighting**               |
| DARK                       | Always dark without a light source         |
| ALWAYS_LIT                 | Always illuminated regardless of time      |
| **Danger**                 |
| DEATH                      | Instant death trap - entering kills        |
| TIMED_DT                   | Timed death trap - kills after duration    |
| **Access Restrictions**    |
| NO_MOB                     | NPCs cannot enter this room                |
| GODROOM                    | Gods/immortals only                        |
| INDOORS                    | Indoor room - protected from weather       |
| UNDERGROUND                | Below ground level                         |
| UNDERDARK                  | Deep underground - special rules apply     |
| **Combat/Magic**           |
| PEACEFUL                   | No combat allowed in this room             |
| NO_MAGIC                   | Magic doesn't work here                    |
| SOUNDPROOF                 | Sound doesn't carry in/out                 |
| **Transport Restrictions** |
| NO_TRACK                   | Cannot be tracked to or from               |
| NO_RECALL                  | Cannot recall from or to                   |
| NO_SUMMON                  | Cannot summon to or from                   |
| NO_TELEPORT                | Cannot teleport to or from                 |
| NO_SHIFT                   | Cannot plane shift                         |
| NO_SCAN                    | Cannot scan into                           |
| NO_WELL                    | Cannot use well of worlds                  |
| **Services**               |
| CAMPSITE                   | Valid camping/resting location             |
| GUILDHALL                  | Guild hall                                 |
| ARENA                      | PvP arena room                             |
| **Housing**                |
| HOUSE                      | Player house room                          |
| ATRIUM                     | Multi-person private area                  |
| **Special**                |
| VEHICLE                    | Interior of a vehicle (moves with vehicle) |
| CURRENT                    | Has water current - forced movement        |
| CLAN_ENTRANCE              | Clan entrance room                         |
| CLAN_STORAGE               | Clan storage room                          |

---

### Room Capacity

Use the `capacity` field (integer) instead of size flags.

| Value | Meaning     | Use Case                    |
| ----- | ----------- | --------------------------- |
| 0     | Unlimited   | Large outdoor areas, plazas |
| 1     | Single file | Tunnels, narrow passages    |
| 2     | Private     | Small rooms, cells          |
| 10    | Default     | Normal rooms                |
| 20+   | Large       | Arenas, throne rooms        |

**Example**:

```yaml
# Narrow tunnel
capacity: 1

# Private bedroom
capacity: 2

# Grand hall
capacity: 50
```

---

### Magic Affinity

Rooms can have a magical affinity that affects spell power.

| Affinity          | Effect                                                      |
| ----------------- | ----------------------------------------------------------- |
| **Elemental**     |
| FIRE              | Fire spells +%, cold spells -%, fire elementals comfortable |
| WATER             | Water spells +%, aquatic creatures thrive                   |
| COLD              | Cold/ice spells +%, fire spells -%, frozen terrain          |
| EARTH             | Earth spells +%, stone creatures empowered                  |
| AIR               | Air/lightning spells +%, flying creatures benefit           |
| **Divine/Planar** |
| HOLY              | Holy spells +%, unholy -%, undead weakened                  |
| UNHOLY            | Unholy spells +%, holy -%, undead empowered                 |
| SHADOW            | Shadow spells +%, radiant -%, darkness persists             |
| DEATH             | Necromancy +%, close to death plane                         |
| ASTRAL            | Summoning/teleport +%, planar barriers weak                 |
| **Other**         |
| NATURE            | Nature/druid spells +%, fey welcome                         |
| ARCANE            | All magic +% or spell regen bonus                           |
| CHAOS             | Wild magic - spells have unpredictable side effects         |

**Example**:

```yaml
# Fire temple inner sanctum
magicAffinity: FIRE

# Graveyard
magicAffinity: DEATH

# Fey grove
magicAffinity: NATURE
```

---

### Room Effects (Environmental Hazards)

Use `activeEffects` for environmental dangers that affect characters in the room.

```yaml
# Poison swamp - periodic poison damage
activeEffects: [{type: "poison", params: {damage: "1d4", interval: 30}}]

# Lava chamber - heavy fire damage
activeEffects: [{type: "damage", params: {damageType: "fire", amount: "3d6", interval: 10}}]

# Magical darkness - overcomes light sources
activeEffects: [{type: "darkness", params: {level: 5}}]

# Disease pit - chance of disease on entry
activeEffects: [{type: "disease", params: {chance: 0.1, severity: "moderate"}}]

# Freezing cold - cold damage
activeEffects: [{type: "damage", params: {damageType: "cold", amount: "2d4", interval: 20}}]

# Acid pool
activeEffects: [{type: "damage", params: {damageType: "acid", amount: "2d6", interval: 5}}]

# Healing spring - periodic healing
activeEffects: [{type: "heal", params: {amount: "1d8", interval: 60}}]
```

---

## Exit Flags & Properties

### Exit Flags

Intrinsic properties of exits/doors.

| Flag       | Description                                      |
| ---------- | ------------------------------------------------ |
| IS_DOOR    | Has a door (enables open/close/lock/unlock/pick) |
| PICKPROOF  | Cannot be picked with lock picks                 |
| HIDDEN     | Not visible until searched/found                 |
| BASHABLE   | Can be broken down with bash skill               |
| MAGICPROOF | Immune to knock spell and dispel magic           |

**Examples**:

```yaml
# Regular door
flags: [IS_DOOR]
defaultState: CLOSED

# Locked reinforced door
flags: [IS_DOOR, PICKPROOF]
defaultState: LOCKED
keyId: 12345

# Secret passage
flags: [IS_DOOR, HIDDEN]
defaultState: CLOSED

# Wooden door that can be bashed
flags: [IS_DOOR, BASHABLE]
defaultState: CLOSED
hitPoints: 50

# Magically sealed door
flags: [IS_DOOR, MAGICPROOF, PICKPROOF]
defaultState: LOCKED
```

---

### Default State

The state the exit resets to on zone reset.

| State  | Description              |
| ------ | ------------------------ |
| OPEN   | Door open or no door     |
| CLOSED | Door closed but unlocked |
| LOCKED | Door closed and locked   |

**Note**: Runtime door state (open/closed/locked during gameplay) is managed by the game engine and resets to `defaultState` on zone reset.

---

### Breakable Doors

For bashable doors, use the `hitPoints` field.

| HP Value  | Door Type              |
| --------- | ---------------------- |
| 0 or null | Not breakable          |
| 50        | Wooden door            |
| 100       | Reinforced door        |
| 200       | Iron door              |
| 500+      | Magical/legendary door |

---

## Effects System

Instead of permanent flags, use `activeEffects` for temporary states on mobs, objects, and rooms.

### Mob Spawn Effects

Effects applied when a mob spawns. Use `permanent: true` for always-on effects.

```yaml
# Always hasted mob
spawnEffects: [{type: "haste", permanent: true}]

# Always blurred (hard to hit)
spawnEffects: [{type: "blur", permanent: true}]

# Invisible until detected
spawnEffects: [{type: "invisible", permanent: true}]

# Flying creature
spawnEffects: [{type: "fly", permanent: true}]

# Sanctuary (damage reduction)
spawnEffects: [{type: "sanctuary", permanent: true}]
```

---

### Object Enchantments

Effects on items. Can be permanent, duration-based, or charge-based.

```yaml
# Blessed item (permanent until removed)
activeEffects: [{type: "bless", permanent: true}]

# Cursed ring (requires remove_curse)
activeEffects: [{type: "curse", permanent: true, params: {removable: "remove_curse"}}]

# Poisoned dagger (5 uses)
activeEffects: [{type: "poison", charges: 5, params: {damage: "1d4", onHit: true}}]

# Flaming sword
activeEffects: [{type: "fire_damage", permanent: true, params: {bonusDamage: "1d6"}}]

# Temporary enchantment (fades over time)
activeEffects: [{type: "sharpness", duration: 3600, params: {bonusDamage: "1d4"}}]
```

---

### Common Effect Types

| Effect               | Description                        |
| -------------------- | ---------------------------------- |
| **Control Effects**  |
| charm                | Under control of another character |
| sleep                | Unconscious, cannot act            |
| fear                 | Fleeing in terror                  |
| confusion            | Random actions                     |
| **Debuffs**          |
| blind                | Cannot see, major combat penalty   |
| silence              | Cannot speak or cast spells        |
| slow                 | Reduced action speed               |
| weakness             | Reduced strength                   |
| **Combat Effects**   |
| stun                 | Dazed, cannot act but aware        |
| knockdown            | Prone on ground, must stand up     |
| paralyzed            | Cannot move or act (magical)       |
| **Buffs**            |
| haste                | Faster actions, extra attacks      |
| blur                 | Harder to hit, dodge bonus         |
| invisible            | Cannot be seen                     |
| fly                  | Can fly/float                      |
| sanctuary            | Damage reduction                   |
| shield               | AC bonus                           |
| strength             | Increased strength                 |
| **Damage Over Time** |
| poison               | Ongoing poison damage              |
| disease              | Ongoing disease effects            |
| bleed                | Ongoing physical damage            |
| burn                 | Ongoing fire damage                |
| **Detection**        |
| detect_invis         | Can see invisible                  |
| detect_magic         | Can see magical auras              |
| infravision          | Can see in dark via heat           |
| sense_life           | Can detect hidden living creatures |

---

## Quick Reference

### Mob Creation Checklist

1. **Basic Stats**: Set level, HP, class, race, alignment
2. **Traits**: Choose what the mob IS (ILLUSION, ANIMATED, MOUNT, etc.)
3. **Behaviors**: Choose how the mob ACTS (SENTINEL, HELPER, TRACK, etc.)
4. **Aggression**: Set `aggroTargets` if aggressive (ALL, EVIL_ALIGNMENT, etc.)
5. **Immunities**: Set `immuneEffects` and `immuneDamageTypes`
6. **Restrictions**: Add `activityRestrictions` if needed (vampires, nocturnal)
7. **Effects**: Configure `spawnEffects` for buffs (haste, blur, fly)
8. **Equipment**: Define equipment loadout via resets

---

### Object Creation Checklist

1. **Basic Properties**: Set type, weight, value, material
2. **Wear Slots**: Set `wearSlots` for equipment (HEAD, BODY, MAINHAND, etc.)
3. **Weapon Grip**: Set `weaponGrip` for weapons (ONE_HANDED, TWO_HANDED, VERSATILE)
4. **Restrictions**: Add class/alignment/size/race restrictions
5. **Flags**: Choose object flags (GLOW, SOULBOUND, NO_DROP, etc.)
6. **Enchantments**: Add `activeEffects` for magical properties

---

### Room Creation Checklist

1. **Basic Info**: Name, description, sector type
2. **Capacity**: Set `capacity` if not default (10)
3. **Magic Affinity**: Set `magicAffinity` for magical areas
4. **Flags**: Choose room flags (DARK, PEACEFUL, NO_MAGIC, etc.)
5. **Hazards**: Add `activeEffects` for environmental dangers
6. **Exits**: Configure exits with flags, `defaultState`, keys

---

### Exit Creation Checklist

1. **Direction**: North, south, east, west, up, down
2. **Destination**: Target room
3. **Door**: Add IS_DOOR flag if it has a door
4. **State**: Set `defaultState` (OPEN, CLOSED, LOCKED)
5. **Key**: Set `keyId` if door requires a key
6. **Special Flags**: HIDDEN, PICKPROOF, BASHABLE, MAGICPROOF
7. **Breakable**: Set `hitPoints` for bashable doors

---

## See Also

- [Effects Reference](EFFECTS.md) - Complete list of effect types
- [Damage Types](DAMAGE_TYPES.md) - All damage type categories
- [Classes Reference](CLASSES.md) - Player and NPC classes
- [Races Reference](RACES.md) - Playable and NPC races

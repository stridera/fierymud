# FieryMUD Lua Scripting API Reference

This document describes the Lua scripting API available for writing triggers in FieryMUD.

## Table of Contents

- [Core Concepts](#core-concepts)
- [Global Functions](#global-functions)
- [Namespaces](#namespaces)
  - [combat](#combat)
  - [spells](#spells)
  - [skills](#skills)
  - [world](#world)
  - [zone](#zone)
  - [doors](#doors)
  - [effects](#effects)
  - [timers](#timers)
  - [vars](#vars)
  - [quest](#quest)
  - [objects / mobiles](#objects--mobiles)
- [Entity Types](#entity-types)
  - [Actor](#actor)
  - [Room](#room)
  - [Object](#object)
- [Enums](#enums)
- [Error Handling](#error-handling)
- [Migration from DG Script](#migration-from-dg-script)

## Core Concepts

### Return Value Convention

Most functions return `(result, error)` tuples for scriptable error handling:

```lua
local success, err = combat.engage(self, target)
if not success then
    self:say("I cannot attack: " .. (err or "unknown error"))
end
```

Functions that find entities return `nil` on failure:

```lua
local player = world.find_player("Bob")
if player then
    player:send("Hello!")
end
```

### Nil Safety

Calling methods on nil produces clear error messages rather than cryptic crashes. Always check return values when searching for entities.

### Composite IDs

All world entities use composite IDs `(zone_id, local_id)`. Legacy vnums like `3045` convert to `(30, 45)`. Zone 0 maps to zone 1000.

```lua
-- Spawning with composite ID
local mob, err = room:spawn_mobile(30, 45)

-- Getting room by ID
local room = get_room("30:45")  -- Modern format
local room = get_room(3045)     -- Legacy format (auto-converted)
```

---

## Global Functions

### Dice and Random

```lua
-- Roll dice: dice(count, sides)
local damage = dice(2, 6)  -- 2d6, returns 2-12

-- Random integer in range [min, max]
local pick = random(1, 100)

-- Probability check: returns true with N% chance
if percent_chance(30) then
    self:say("30% chance happened!")
end
```

### Room Lookup

```lua
-- Get room by various formats
local room = get_room(3045)       -- Legacy vnum
local room = get_room("30:45")    -- Modern zone:id format
local room = get_room(my_room)    -- Pass-through if already Room
```

### Async / Timing

```lua
-- Suspend execution (uses coroutines)
self:say("Wait for it...")
wait(3)  -- 3 seconds
self:say("Here it comes!")

-- Non-blocking timer (script continues immediately)
local timer_id = timers.after(5, function()
    self:say("5 seconds have passed!")
end)
```

### Logging

```lua
echo("Debug message")           -- Shorthand for log_debug
log_debug("Detailed info")
log_warn("Something odd")
log_error("Something wrong")
```

### Game Time

```lua
local time = mud_time()
-- time.hour (0-23)
-- time.day (1-35)
-- time.month (1-16)
-- time.year
```

---

## Namespaces

### combat

Combat system management.

```lua
-- Start combat between two actors
local success, err = combat.engage(self, target)

-- Rescue: rescuer intervenes to protect target
local success, err = combat.rescue(self, victim)

-- Stop fighting
local success, err = combat.disengage(self)

-- Check combat status
if combat.is_fighting(target) then
    self:say("They're busy!")
end
```

### spells

Spell casting system.

```lua
-- Cast a spell
local success, err = spells.cast(self, "fireball", target)

-- Cast with specific level
local success, err = spells.cast(self, "heal", self, 30)

-- Cast without target (self-only or area spells)
local success, err = spells.cast(self, "sanctuary")

-- Check if spell exists
if spells.exists("meteor swarm") then
    spells.cast(self, "meteor swarm", target)
end
```

### skills

Data-driven skill execution (looks up skills from database).

```lua
-- Execute a skill
local success, err = skills.execute(self, "bash", target)
local success, err = skills.execute(self, "backstab", target)
local success, err = skills.execute(self, "kick", target)

-- Check skill existence
if skills.exists("disarm") then
    skills.execute(self, "disarm", target)
end

-- Query/modify skill levels
local level = skills.get_level(actor, "sword")
skills.set_level(actor, "sword", 85)
```

### world

Global world operations.

```lua
-- Find player by name (case-insensitive)
local player = world.find_player("Bob")
if player then
    player:send("The gods are watching you.")
end

-- Find mobile by name
local guard = world.find_mobile("cityguard")

-- Destroy an entity (mobile or object)
local success, err = world.destroy(target_mob)
local success, err = world.destroy(the_object)
```

### zone

Zone-wide operations.

```lua
-- Send message to all players in zone (no speaker attribution)
zone.echo(30, "The ground trembles beneath your feet!")

-- Trigger zone reset
zone.reset(30)
```

### doors

Door and exit manipulation.

```lua
-- Basic door operations
local success, err = doors.open(room, "north")
local success, err = doors.close(room, "north")
local success, err = doors.lock(room, "east")
local success, err = doors.unlock(room, "east")

-- Set door state directly
doors.set_state(room, "north", {
    closed = true,
    locked = true,
    hidden = false
})
```

### effects

Status effect management.

```lua
-- Apply effect with options
local success, err = effects.apply(target, "poison", {
    duration = 60,  -- seconds
    power = 5
})

-- Simple effect application
effects.apply(target, "blind")

-- Remove effect
local success, err = effects.remove(target, "poison")

-- Check for effect
if effects.has(target, "sanctuary") then
    self:say("They are protected!")
end

-- Get remaining duration
local remaining = effects.duration(target, "haste")
```

### timers

Non-blocking delayed execution.

```lua
-- Execute after delay
local timer_id = timers.after(10, function()
    room:send("A loud explosion rocks the area!")
end)

-- Cancel a timer
timers.cancel(timer_id)
```

### vars

Entity variable storage (persistent key-value store).

```lua
-- Set variable on any entity
vars.set(self, "greeting_count", 0)
vars.set(actor, "last_visited", mud_time().hour)
vars.set(room, "trap_armed", true)

-- Get variable
local count = vars.get(self, "greeting_count") or 0
count = count + 1
vars.set(self, "greeting_count", count)

-- Check existence
if vars.has(actor, "quest_started") then
    self:say("You've been here before!")
end

-- Remove variable
vars.clear(actor, "temporary_flag")

-- Get all variables
local all_vars = vars.all(self)
for key, value in pairs(all_vars) do
    echo(key .. " = " .. tostring(value))
end
```

### quest

Quest system management.

```lua
-- Start a quest (zone_id, quest_id)
quest.start(actor, 30, 5)

-- Check quest state
if quest.has_quest(actor, 30, 5) then
    local status = quest.status(actor, 30, 5)  -- "active", "complete", etc.
end

if quest.is_available(actor, 30, 5) then
    self:say("I have a task for you!")
end

if quest.is_completed(actor, 30, 5) then
    self:say("Thank you for your help!")
end

-- Quest progression
quest.advance_objective(actor, 30, 5, 1)  -- Advance objective 1
quest.complete(actor, 30, 5)
quest.abandon(actor, 30, 5)

-- Quest variables (for tracking state)
quest.set_variable(actor, 30, 5, "items_collected", 3)
local items = quest.get_variable(actor, 30, 5, "items_collected")
```

### objects / mobiles

Template access for reading prototype data.

```lua
-- Get object template (read-only)
local template = objects.template(30, 45)
if template then
    self:say("That item weighs " .. template.weight .. " pounds.")
end

-- Get mobile template
local mob_template = mobiles.template(30, 12)
if mob_template then
    self:say("That creature is level " .. mob_template.level)
end
```

---

## Entity Types

### Actor

Base type for all characters (Player and Mobile).

#### Properties (read-only)

```lua
actor.name           -- Display name
actor.level          -- Character level
actor.room           -- Current Room
actor.position       -- Position enum
actor.alignment      -- Alignment value
actor.wealth         -- Gold carried
actor.is_npc         -- true for mobiles
actor.is_player      -- true for players
actor.is_fighting    -- true if in combat
actor.fighting       -- Actor they're fighting (or nil)
actor.sex            -- "male", "female", "neutral"
actor.race           -- Race name
actor.class          -- Class name
actor.hp             -- Current hit points
actor.max_hp         -- Maximum hit points
actor.mana           -- Current mana
actor.max_mana       -- Maximum mana
actor.move           -- Current movement
actor.max_move       -- Maximum movement
```

#### Methods

```lua
-- Communication
actor:send("Private message")
actor:say("I say this out loud")
actor:emote("waves hello")
actor:shout("ZONE-WIDE MESSAGE!")
actor:whisper(target, "Secret message")

-- Health
actor:damage(50)      -- Deal 50 damage
actor:heal(25)        -- Heal 25 HP

-- Movement
actor:teleport(room)  -- Instant move to room
local success, err = actor:move("north")  -- Walk in direction
actor:follow(leader)  -- Follow another actor

-- Economy
actor:give_wealth(100)  -- Give gold
actor:take_wealth(50)   -- Take gold (returns false if not enough)
if actor:has_wealth(200) then ... end

-- Effects and Flags
if actor:has_effect(Effect.Invisible) then ... end
if actor:has_effect_named("poison") then ... end
if actor:has_flag(ActorFlag.Aggressive) then ... end

-- Experience
actor:award_exp(500)

-- Execute game command
actor:command("drop sword")
actor:command("say Hello everyone!")

-- Spawn object to inventory
local obj, err = actor:spawn_object(30, 45)
if obj then
    actor:send("You received " .. obj.name)
end
```

#### Inventory

```lua
-- Check for items
if actor.inventory:has("sword") then ... end

-- Count matching items
local count = actor.inventory:count("potion")

-- Find specific item
local item = actor.inventory:find("key")

-- Drop item
local success, err = actor.inventory:drop("sword")

-- Give to another actor
local success, err = actor.inventory:give(target, "gold coin")

-- Iterate over inventory
for item in actor.inventory:items() do
    echo(item.name)
end
```

### Room

Game location/room.

#### Properties (read-only)

```lua
room.name        -- Room title
room.id          -- String "zone:local" format
room.zone_id     -- Zone number
room.sector      -- SectorType enum
room.light       -- Light level
room.actors      -- Table of actors in room
room.objects     -- Table of objects in room
room.exits       -- Table of available Direction enums
room.is_dark     -- true if dark
room.is_peaceful -- true if no-fight zone
```

#### Methods

```lua
-- Communication
room:send("Everyone in room sees this")
room:send_except("Everyone except one person", excluded_actor)

-- Finding entities
local guard = room:find_actor("guard")
local guards = room:find_actors("guard")  -- Returns table
local chest = room:find_object("chest")
local chests = room:find_objects("chest")  -- Returns table

-- Exit information
if room:has_exit(Direction.North) then
    local next_room = room:get_exit_room(Direction.North)
end
if room:exit_is_open(Direction.East) then ... end
if room:exit_is_locked(Direction.West) then ... end

-- Spawning
local mob, err = room:spawn_mobile(30, 45)
local obj, err = room:spawn_object(30, 45)

-- Mass operations
room:teleport_all(destination_room)
room:purge_all()  -- Remove all non-player entities
```

### Object

Game item/object.

#### Properties (read-only)

```lua
obj.name          -- Keywords
obj.display_name  -- Full display name
obj.id            -- String "zone:local" format
obj.type          -- ObjectType enum
obj.weight        -- Weight in pounds
obj.value         -- Gold value
obj.level         -- Level requirement
obj.is_weapon     -- Type checks
obj.is_armor
obj.is_container
obj.is_light_source
obj.is_potion
```

---

## Enums

Use enum tables for type-safe value access:

```lua
-- Position
Position.Dead
Position.Standing
Position.Sleeping
Position.Fighting
-- etc.

-- Direction
Direction.North
Direction.East
Direction.South
Direction.West
Direction.Up
Direction.Down

-- Effect (for has_effect)
Effect.Invisible
Effect.Sanctuary
Effect.Blind
Effect.Poison
Effect.Flying
-- etc.

-- SectorType
SectorType.Inside
SectorType.City
SectorType.Forest
SectorType.Water_Swim
-- etc.

-- ObjectType
ObjectType.Weapon
ObjectType.Armor
ObjectType.Container
ObjectType.Potion
-- etc.
```

---

## Error Handling

### Checking Results

```lua
local success, err = combat.engage(self, target)
if not success then
    if err == "already_in_combat" then
        self:say("I'm already fighting!")
    elseif err == "invalid_target" then
        self:say("I can't attack that!")
    else
        log_error("Combat failed: " .. (err or "unknown"))
    end
end
```

### Common Error Codes

| Code | Meaning |
|------|---------|
| `invalid_target` | Target is nil or invalid |
| `not_in_combat` | Actor not fighting |
| `already_in_combat` | Already in combat |
| `spell_not_found` | Unknown spell name |
| `skill_not_found` | Unknown skill name |
| `insufficient_mana` | Not enough mana |
| `on_cooldown` | Ability on cooldown |
| `invalid_id` | Bad zone/local ID |
| `not_found` | Entity not found |
| `too_heavy` | Too heavy to carry |
| `inventory_full` | No inventory space |
| `no_exit` | No exit in direction |
| `exit_blocked` | Exit exists but blocked |
| `no_key` | Missing required key |

---

## Migration from DG Script

### Variable Syntax

| DG Script | Lua |
|-----------|-----|
| `%actor%` | `actor` |
| `%self%` | `self` |
| `%actor.name%` | `actor.name` |
| `%actor.vnum%` | `actor.id` |
| `%actor.gold%` | `actor.wealth` |
| `%random.100%` | `random(1, 100)` |

### Command Conversions

| DG Script | Lua |
|-----------|-----|
| `msend %actor% text` | `actor:send("text")` |
| `mecho text` | `room:send("text")` |
| `rescue %actor%` | `combat.rescue(self, actor)` |
| `mkill %actor%` | `combat.engage(self, actor)` |
| `mcast 'spell' %actor%` | `spells.cast(self, "spell", actor)` |
| `mload mob 3045` | `room:spawn_mobile(30, 45)` |
| `mload obj 3045` | `room:spawn_object(30, 45)` |
| `mjunk %obj%` | `world.destroy(obj)` |
| `zoneecho 30 text` | `zone.echo(30, "text")` |
| `backstab %actor%` | `skills.execute(self, "backstab", actor)` |
| `wait 2 s` | `wait(2)` |

### Control Flow

| DG Script | Lua |
|-----------|-----|
| `if %actor.level% > 10` | `if actor.level > 10 then` |
| `elseif` | `elseif ... then` |
| `else` | `else` |
| `end` | `end` |
| `while %condition%` | `while condition do` |
| `done` | `end` |
| `return 0` | `_return_value = false` |
| `return 1` | `_return_value = true` |

### Example Migration

**DG Script:**
```
if %actor.level% > 10
  msend %actor% You are worthy!
  rescue %actor%
else
  mecho The guard ignores the newcomer.
end
```

**Lua:**
```lua
if actor.level > 10 then
    actor:send("You are worthy!")
    combat.rescue(self, actor)
else
    room:send("The guard ignores the newcomer.")
end
```

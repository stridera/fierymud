# FieryMUD Lua Scripting System

This document provides comprehensive documentation for the Lua scripting system used in FieryMUD. Scripts are stored in the PostgreSQL database and edited via Muditor.

## Table of Contents

1. [Overview](#overview)
2. [Trigger Types](#trigger-types)
3. [Context Variables](#context-variables)
4. [Lua API Reference](#lua-api-reference)
5. [Example Scripts](#example-scripts)
6. [Admin Commands](#admin-commands)
7. [Best Practices](#best-practices)

---

## Overview

The FieryMUD scripting system uses **Lua 5.4** with **Sol3** bindings. Scripts are attached to entities (mobs, objects, or rooms) and fire based on trigger conditions.

### Key Concepts

- **Triggers**: Scripts that fire when specific events occur
- **Owner (`self`)**: The entity the trigger is attached to (mob, object, or room)
- **Actor**: The player or mob that caused the trigger to fire
- **Script Types**: `MOB`, `OBJECT`, or `WORLD` (attached to zones/rooms)

### Security Sandbox

For safety, the following Lua features are **disabled**:
- File I/O (`io` library)
- OS access (`os` library)
- Debug hooks (`debug` library)
- Dynamic code loading (`load`, `loadfile`, `dofile`)
- Module loading (`package` library)
- Metatable manipulation (`getmetatable`, `setmetatable`)
- Raw table access (`rawget`, `rawset`)

---

## Trigger Types

Triggers are defined by **flags** that determine when they fire. A trigger can have multiple flags.

### Player Interaction Triggers

| Flag | Description | Context Variables |
|------|-------------|-------------------|
| `COMMAND` | Fires when a command is typed near the owner | `cmd`, `arg`, `actor` |
| `SPEECH` | Fires when someone speaks in the room | `speech`, `actor` |
| `SPEECH_TO` | Fires when directly addressed (say to <mob>) | `speech`, `actor` |
| `GREET` | Fires when a visible actor enters the room | `actor`, `direction` |
| `GREET_ALL` | Fires when any actor enters (even invisible) | `actor`, `direction` |
| `LOOK` | Fires when the entity is looked at | `actor` |

### Movement Triggers

| Flag | Description | Context Variables |
|------|-------------|-------------------|
| `ENTRY` | Fires when the mob enters a room | `room`, `direction` |
| `LEAVE` | Fires when an actor leaves the room | `actor`, `direction` |
| `DOOR` | Fires on door interaction (open/close/lock) | `actor`, `direction` |

### Combat Triggers

| Flag | Description | Context Variables |
|------|-------------|-------------------|
| `FIGHT` | Fires each combat round | `actor` (opponent) |
| `DEATH` | Fires when the entity dies | `actor` (killer, may be nil) |
| `HIT_PERCENT` | Fires when HP drops below threshold | `actor`, `amount` (HP %) |

### Object/Economy Triggers

| Flag | Description | Context Variables |
|------|-------------|-------------------|
| `RECEIVE` | Fires when the mob receives an object | `actor`, `object` |
| `BRIBE` | Fires when gold is given to the mob | `actor`, `amount` |

### Magic Triggers

| Flag | Description | Context Variables |
|------|-------------|-------------------|
| `CAST` | Fires when a spell is cast on the entity | `actor`, `speech` (spell name) |

### Timer/Random Triggers

| Flag | Description | Context Variables |
|------|-------------|-------------------|
| `RANDOM` | Fires randomly on game pulse | (none) |
| `TIME` | Fires at specific MUD time (numeric_arg = hour) | `amount` (hour) |
| `LOAD` | Fires when the entity is spawned | `room` |
| `AUTO` | Fires automatically on a timer | (none) |

### Modifier Flags

| Flag | Description |
|------|-------------|
| `GLOBAL` | Trigger is checked everywhere, not just in mob's room |
| `MEMORY` | Trigger fires when a remembered actor is seen |
| `ACT` | Trigger fires on action messages |

---

## Context Variables

When a trigger fires, these global variables are set in the Lua environment:

### Always Available

| Variable | Type | Description |
|----------|------|-------------|
| `self` | Actor/Object/Room | The entity that owns the trigger |
| `room` | Room | The room where the event occurred |

### Event-Specific

| Variable | Type | Available In | Description |
|----------|------|--------------|-------------|
| `actor` | Actor | Most triggers | The actor who caused the event |
| `target` | Actor | Some triggers | Target of an action |
| `object` | Object | RECEIVE | The object being given/received |
| `cmd` | string | COMMAND | The command word typed |
| `arg` | string | COMMAND | Arguments after the command |
| `speech` | string | SPEECH, CAST | What was said / spell name |
| `direction` | Direction | GREET, LEAVE, ENTRY | Direction of movement |
| `amount` | number | BRIBE, HIT_PERCENT, TIME | Numeric value |

---

## Lua API Reference

### Utility Functions

```lua
-- Random number generation
dice(count, sides)        -- Roll dice: dice(2, 6) returns 2-12
random(min, max)          -- Random integer in range [min, max]
percent_chance(n)         -- Returns true n% of the time

-- Timing
wait(seconds)             -- Pause script execution (1-300 seconds)
                          -- Note: Script resumes after delay

-- Logging (for debugging)
echo(message)             -- Log to server console (info level)
log_debug(message)        -- Debug level log
log_warn(message)         -- Warning level log
log_error(message)        -- Error level log

-- Direction helpers
reverse_direction(dir)    -- Get opposite direction
direction_name(dir)       -- Get direction as string ("north", etc.)
```

### Actor API

Actors include both players (Player) and NPCs (Mobile).

#### Properties (Read-Only)

```lua
-- Identity
actor.name                -- Internal name
actor.display_name        -- Name shown to players
actor.level               -- Character level

-- Vitals
actor.hp                  -- Current hit points
actor.max_hp              -- Maximum hit points
actor.mana                -- Current mana
actor.max_mana            -- Maximum mana
actor.move                -- Current movement
actor.max_move            -- Maximum movement

-- Stats
actor.strength
actor.dexterity
actor.intelligence
actor.wisdom
actor.constitution
actor.charisma
actor.alignment           -- -1000 (evil) to 1000 (good)

-- Economy
actor.wealth              -- Gold carried

-- State
actor.position            -- Position enum (Standing, Sitting, etc.)
actor.position_name       -- Position as string
actor.room                -- Current room (Room object)

-- Type Checks
actor.is_npc              -- true if NPC, false if player
actor.is_player           -- true if player
actor.is_fighting         -- true if in combat
actor.is_alive            -- true if alive
actor.can_act             -- true if can take actions
actor.is_afk              -- true if player is AFK

-- Identity
actor.gender              -- "male", "female", "neutral"
actor.race                -- Race name
actor.size                -- Size category
```

#### Methods

```lua
-- Communication
actor:send(message)       -- Send message to this actor only
actor:say(message)        -- Speak in the room
actor:emote(message)      -- Perform an emote

-- Combat/Stats
actor:damage(amount)      -- Deal damage (reduces HP)
actor:heal(amount)        -- Heal (increases HP up to max)

-- Inventory
actor:has_item(keyword)   -- Check if carrying item (returns bool)
actor:item_count(keyword) -- Count items matching keyword

-- Economy
actor:give_wealth(amount) -- Give gold to actor
actor:take_wealth(amount) -- Take gold from actor
actor:has_wealth(amount)  -- Check if actor can afford amount

-- Effects
actor:has_effect(name)    -- Check for active effect
actor:has_flag(flag)      -- Check for actor flag

-- Movement
actor:teleport(room)      -- Move actor to room (returns success bool)
```

### Mobile-Specific (NPCs)

```lua
mobile.is_aggressive      -- true if will attack on sight
mobile.is_shopkeeper      -- true if shop NPC
mobile.is_banker          -- true if bank NPC
mobile.is_teacher         -- true if skill teacher
mobile.prototype_id       -- "zone:id" format
mobile.description        -- Long description
mobile.damage_dice_num    -- Base damage dice count
mobile.damage_dice_size   -- Base damage dice size
mobile:has_mob_flag(flag) -- Check mob flag
```

### Player-Specific

```lua
player.is_god             -- true if immortal
player.god_level          -- Immortal level (0 for mortals)
player.is_online          -- true if connected
player.is_linkdead        -- true if disconnected but in game
player.class              -- Class name
player.title              -- Player title
player.in_clan            -- true if clan member
player.clan_name          -- Clan name (or empty)
player.clan_rank          -- Clan rank title
player.is_brief           -- Brief mode enabled
player.is_autoloot        -- Autoloot enabled
player.is_pk_enabled      -- PK flag enabled
```

### Room API

#### Properties

```lua
room.name                 -- Room title
room.id                   -- "zone:id" format
room.zone_id              -- Zone number

-- Environment
room.sector               -- SectorType enum
room.sector_name          -- Sector as string
room.light_level          -- Light level (number)

-- State Checks
room.is_dark              -- true if dark
room.is_peaceful          -- true if no-combat
room.allows_magic         -- true if magic allowed
room.allows_recall        -- true if recall allowed
room.allows_summon        -- true if summon allowed
room.allows_teleport      -- true if teleport allowed
room.is_private           -- true if private room
room.is_death_trap        -- true if DT
room.is_full              -- true if at capacity

-- Contents
room.actors               -- Table of all actors in room
room.actor_count          -- Number of actors
room.objects              -- Table of all objects in room
room.object_count         -- Number of objects
room.exits                -- Table of available exit directions
```

#### Methods

```lua
-- Communication
room:send(message)                    -- Send to all in room
room:send_except(actor, message)      -- Send to all except one actor

-- Exit Checks
room:has_exit(direction)              -- Check if exit exists
room:exit_is_open(direction)          -- Check if exit passable
room:exit_is_locked(direction)        -- Check if exit locked

-- Finding Entities
room:find_actor(keyword)              -- Find first matching actor
room:find_actors(keyword)             -- Find all matching actors
room:find_object(keyword)             -- Find first matching object
room:find_objects(keyword)            -- Find all matching objects

-- Flag Check
room:has_flag(flag)                   -- Check room flag
```

### Object API

#### Properties

```lua
object.name               -- Internal name
object.display_name       -- Name shown to players
object.short_desc         -- Short description
object.id                 -- "zone:id" format

-- Type Info
object.type               -- ObjectType enum
object.equip_slot         -- EquipSlot enum

-- Physical
object.weight             -- Current weight
object.base_weight        -- Base weight (before contents)
object.value              -- Gold value
object.level              -- Level requirement

-- Type Checks
object.is_weapon
object.is_armor
object.is_container
object.is_light_source
object.is_wearable
object.is_magic_item
object.is_potion
object.is_scroll
object.is_wand
object.is_staff

-- Weapon Properties (if weapon)
object.damage_dice_num    -- Damage dice count
object.damage_dice_size   -- Damage dice size
object.damage_bonus       -- Damage bonus
object.average_damage     -- Average damage per hit
object.spell_level        -- Magic item spell level
```

#### Methods

```lua
object:is_type(type)      -- Check against ObjectType
object:has_flag(flag)     -- Check object flag
```

### Container API (extends Object)

```lua
container.capacity        -- Maximum items
container.item_count      -- Current item count
container.is_empty        -- true if empty
container.is_full         -- true if at capacity
container.is_closed       -- true if closed
container.is_locked       -- true if locked
container.items           -- Table of contained objects

container:find_item(keyword)  -- Find item inside
container:has_item(keyword)   -- Check for item inside
```

### Enums

#### Position
```lua
Position.Dead, Position.Ghost, Position.MortallyWounded,
Position.Incapacitated, Position.Stunned, Position.Sleeping,
Position.Resting, Position.Sitting, Position.Prone,
Position.Fighting, Position.Standing, Position.Flying
```

#### Direction
```lua
Direction.North, Direction.East, Direction.South, Direction.West,
Direction.Up, Direction.Down, Direction.Northeast, Direction.Northwest,
Direction.Southeast, Direction.Southwest, Direction.In, Direction.Out,
Direction.Portal
```

#### SectorType
```lua
SectorType.Inside, SectorType.City, SectorType.Field, SectorType.Forest,
SectorType.Hills, SectorType.Mountains, SectorType.WaterSwim,
SectorType.WaterNoswim, SectorType.Underwater, SectorType.Flying,
SectorType.Desert, SectorType.Swamp, SectorType.Beach, SectorType.Road,
SectorType.Underground, SectorType.Lava, SectorType.Ice, SectorType.Astral,
SectorType.Fire, SectorType.Lightning, SectorType.Spirit,
SectorType.Badlands, SectorType.Void
```

#### ObjectType
```lua
ObjectType.Light, ObjectType.Scroll, ObjectType.Wand, ObjectType.Staff,
ObjectType.Weapon, ObjectType.Fireweapon, ObjectType.Missile,
ObjectType.Treasure, ObjectType.Armor, ObjectType.Potion, ObjectType.Worn,
ObjectType.Other, ObjectType.Container, ObjectType.Note,
ObjectType.LiquidContainer, ObjectType.Key, ObjectType.Food,
ObjectType.Money, ObjectType.Fountain, ObjectType.Portal,
ObjectType.Spellbook, ObjectType.Corpse
```

---

## Example Scripts

### COMMAND Trigger - Quest NPC

```lua
-- Trigger: COMMAND flag, attached to quest-giver mob
-- Listens for "quest" command

if cmd ~= "quest" then
    return true  -- Not our command, continue normal processing
end

if actor.level < 10 then
    self:say("Come back when you have more experience, young one.")
    return false  -- Halt command processing
end

if actor:has_item("ancient tome") then
    self:say("Excellent! You found the tome!")
    -- Would need item transfer API to actually take it
    actor:give_wealth(5000)
    self:say("Here is your reward of 5000 gold!")
    return false
end

self:say("I seek an ancient tome lost in the Haunted Forest.")
self:say("Bring it to me and I shall reward you handsomely.")
return false
```

### GREET Trigger - Guard with Wait

```lua
-- Trigger: GREET flag, attached to city guard
-- Challenges evil characters

if actor.alignment < -500 then
    self:say("Halt, evildoer! Your kind is not welcome here!")
    wait(2)
    self:emote("draws their sword menacingly.")
    wait(1)
    self:say("Leave now or face the consequences!")
end
-- Return true to allow normal greet behavior
return true
```

### SPEECH Trigger - Keyword Response

```lua
-- Trigger: SPEECH flag
-- Responds to keywords in conversation

local text = string.lower(speech)

if string.find(text, "help") then
    self:say("I can assist with many things. Ask about 'quests', 'directions', or 'rumors'.")
elseif string.find(text, "quest") then
    self:say("The mayor is looking for adventurers. You'll find him in the town hall.")
elseif string.find(text, "direction") then
    self:say("The temple is to the north, the market to the east.")
elseif string.find(text, "rumor") then
    if percent_chance(50) then
        self:say("I heard strange noises from the old mine last night...")
    else
        self:say("They say a dragon was spotted near the mountains.")
    end
end
```

### DEATH Trigger - Boss Loot

```lua
-- Trigger: DEATH flag, attached to boss mob
-- Special death effects

room:send("The demon lord lets out a terrible shriek as it falls!")
wait(2)
room:send("Dark energy swirls around the corpse...")
wait(2)
room:send("A treasure chest materializes from the shadows!")
-- Would need spawn_obj API to create the chest
```

### RECEIVE Trigger - Item Quest

```lua
-- Trigger: RECEIVE flag
-- Reacts to receiving specific items

if object.name == "dragon scale" then
    self:say("A dragon scale! This is exactly what I needed!")
    wait(1)
    actor:give_wealth(2000)
    self:say("Take this gold as thanks, brave adventurer.")
    -- Would need API to destroy the object
elseif object.is_weapon then
    self:say("I have no use for weapons. Here, take it back.")
    -- Would need API to give back
else
    self:say("Thank you for the gift.")
end
```

### BRIBE Trigger - Corrupt Guard

```lua
-- Trigger: BRIBE flag
-- Guard accepts bribes

if amount < 100 then
    self:say("That's insulting. Get lost.")
    return true  -- Return the money
elseif amount < 500 then
    self:say("Hmm... I suppose I could look the other way for a moment.")
    wait(1)
    self:emote("steps aside briefly.")
elseif amount >= 500 then
    self:say("A generous gift! You may pass freely, friend.")
    wait(1)
    self:emote("opens the gate and gestures you through.")
end
return false  -- Keep the money
```

### FIGHT Trigger - Combat Special

```lua
-- Trigger: FIGHT flag
-- Special combat abilities

if percent_chance(15) then
    self:emote("raises their staff and mutters an incantation!")
    wait(1)
    room:send("Lightning crackles through the air!")
    actor:damage(dice(3, 10))
    actor:send("You are struck by lightning!")
end
```

### HIT_PERCENT Trigger - Flee Behavior

```lua
-- Trigger: HIT_PERCENT flag, numeric_arg = 25
-- Fires when HP drops below 25%

self:say("You'll pay for this! I'll be back!")
-- Would need flee API
echo("Mob " .. self.name .. " attempting to flee at " .. amount .. "% HP")
```

### TIME Trigger - Scheduled Event

```lua
-- Trigger: TIME flag, numeric_arg = 6
-- Fires at 6 AM game time

room:send("The town crier calls out: 'Six o'clock and all is well!'")
```

### RANDOM Trigger - Ambient Behavior

```lua
-- Trigger: RANDOM flag
-- Random ambient actions

local action = random(1, 5)

if action == 1 then
    self:emote("scratches their head thoughtfully.")
elseif action == 2 then
    self:emote("glances around nervously.")
elseif action == 3 then
    self:say("Nice weather we're having...")
elseif action == 4 then
    self:emote("stretches and yawns.")
else
    self:emote("hums a quiet tune.")
end
```

### CAST Trigger - Spell Immunity

```lua
-- Trigger: CAST flag
-- React to spells cast on mob

local spell = string.lower(speech)

if string.find(spell, "charm") then
    self:say("Your feeble magic has no effect on me!")
    room:send(self.display_name .. " resists the spell!")
    return false  -- Block the spell
elseif string.find(spell, "fireball") or string.find(spell, "fire") then
    self:say("I am immune to fire, fool!")
    return false  -- Block fire spells
end

return true  -- Allow other spells
```

---

## Admin Commands

### tstat - Trigger Status

Shows trigger system status and statistics.

```
> tstat
--- Trigger System Status ---
Total triggers cached: 42
  MOB triggers: 28
  OBJECT triggers: 10
  WORLD triggers: 4
Bytecode cache size: 35

Execution Statistics:
  Total executions: 156
  Successful: 142
  Halted: 8
  Yielded (wait): 4
  Failed: 2

Last error: (none)
```

### tlist - List Triggers

List all triggers for a zone.

```
> tlist 30
--- Triggers in Zone 30 ---
ID 1: "guard_greet" [MOB 30:5] - GREET COMMAND
ID 2: "shopkeeper_speech" [MOB 30:12] - SPEECH BRIBE
ID 3: "fountain_random" [OBJECT 30:8] - RANDOM
```

### treload - Reload Triggers

Reload triggers from the database.

```
> treload 30
Reloading triggers for zone 30...
Loaded 15 triggers for zone 30.

> treload all
Reloading all triggers...
Cleared all cached triggers.
Loaded 142 triggers from 12 zones.
```

---

## Best Practices

### Performance

1. **Use `return false` early** to halt processing when the trigger handles the event
2. **Avoid long `wait()` chains** - each wait consumes server resources
3. **Cache repeated lookups** - store `room:find_actor()` results in local variables
4. **Use `percent_chance()` for random triggers** to reduce script execution

### Safety

1. **Always check for nil** before accessing properties
   ```lua
   if actor and actor.level > 10 then
       -- safe to use actor
   end
   ```

2. **Validate input** in COMMAND triggers
   ```lua
   if not arg or arg == "" then
       self:say("Usage: quest <accept|decline>")
       return false
   end
   ```

3. **Limit damage/healing** to reasonable values
   ```lua
   local heal_amount = math.min(100, actor.max_hp - actor.hp)
   actor:heal(heal_amount)
   ```

### Debugging

1. **Use `echo()` for debugging** - output appears in server logs
   ```lua
   echo("Trigger fired: actor=" .. actor.name .. " cmd=" .. cmd)
   ```

2. **Check `tstat`** for execution statistics and errors

3. **Use descriptive trigger names** for easier identification in logs

### Code Style

1. **Return values matter**:
   - `return true` or no return = continue normal processing
   - `return false` = halt processing (command was handled)

2. **Use meaningful variable names**:
   ```lua
   local quest_item = object
   local quest_giver = self
   local adventurer = actor
   ```

3. **Comment complex logic**:
   ```lua
   -- Only trigger at night (hours 20-6)
   local hour = mud_time().hour
   if hour >= 20 or hour < 6 then
       -- Night behavior
   end
   ```

---

## Database Schema

Triggers are stored in the `Triggers` table:

| Column | Type | Description |
|--------|------|-------------|
| id | SERIAL | Primary key |
| name | TEXT | Trigger name for identification |
| attach_type | TEXT | 'MOB', 'OBJECT', or 'WORLD' |
| flags | TEXT[] | Array of TriggerFlag names |
| commands | TEXT | Lua script code |
| variables | JSONB | Script-local variables |
| num_args | INT | Number of expected arguments |
| arg_list | TEXT[] | Argument names/descriptions |
| numeric_arg | INT | Numeric arg (e.g., HIT_PERCENT threshold, TIME hour) |
| zone_id | INT | Zone attachment (for WORLD) |
| mob_zone_id, mob_id | INT | Mob attachment |
| object_zone_id, object_id | INT | Object attachment |

---

## Version History

- **Phase 1-4**: Core scripting engine, bindings, trigger dispatch, coroutines
- **Phase 5**: Combat triggers (FIGHT, DEATH, HIT_PERCENT), economy (RECEIVE, BRIBE), magic (CAST), time (TIME)
- **Phase 6**: Admin commands (tstat, tlist, treload), bytecode caching, execution statistics

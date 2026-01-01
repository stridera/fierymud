# Minion/Pet System Design

## Overview

Pets and minions are followers that players can acquire through purchase, taming, or quests. This document outlines potential features for making pets more than just cosmetic accessories.

## Current Implementation

- Pets can be purchased from pet shops (zone 30, shop 91)
- `set_master()` / `add_follower()` establishes the relationship
- Followers automatically move with their master
- Basic spawn and follow mechanics working

## Proposed Commands

### `minion` - Core Command

```
minion                      - List all your current pets/followers
minion name <pet> <name>    - Give your pet a custom name
minion dismiss <pet>        - Release a pet permanently
minion stay                 - All pets stay in current room
minion follow               - All pets resume following
minion status <pet>         - Show pet stats, loyalty, abilities
```

### `mount` - Riding System

```
mount <horse>               - Mount a rideable pet
dismount                    - Dismount
```

## Pet Types & Abilities

### Combat Pets
| Type | Combat Style | Special Ability |
|------|--------------|-----------------|
| Dog | Loyal tank, moderate damage | Guard (attacks anyone who attacks master) |
| Cat | Fast, high dodge, low damage | Ambush (bonus damage from stealth) |
| Wolf | Pack hunter, scales with other wolves | Howl (intimidate enemies) |
| Bear | High HP, slow, high damage | Maul (bleed effect) |

### Utility Pets
| Type | Primary Use | Special Ability |
|------|-------------|-----------------|
| Cat | Detection | Sense hidden/invisible creatures |
| Dog | Tracking | Track command - follow scent trails |
| Bird | Scouting | Scout - peek into adjacent rooms |
| Horse | Transport | Faster travel, +carry capacity |
| Mule | Hauling | Large inventory expansion |

### Exotic Pets (Rare/Quest rewards)
| Type | Special Ability |
|------|-----------------|
| Fire Lizard | Permanent light source, fire resistance aura |
| Shadow Cat | Grants minor stealth bonus |
| War Dog | Trained combat pet, can learn tricks |
| Familiar (mage) | Spell focus, mana regen bonus |

## Progression System

### Loyalty Levels
1. **Wary** (0-100) - Pet may not always obey, might flee in combat
2. **Friendly** (101-500) - Reliable following, basic commands work
3. **Loyal** (501-1000) - Combat assistance, special abilities unlock
4. **Bonded** (1001+) - Full ability access, won't flee, death protection?

### Loyalty Gain
- Time spent together: +1 per hour
- Feeding: +5-20 depending on food quality
- Combat together: +2 per fight
- Petting/interaction: +1 per interaction (cooldown)

### Loyalty Loss
- Ignoring hunger: -5 per day
- Pet death: -100
- Abandonment (dismiss): Permanent

## Combat Integration

### Auto-Assist
- Pets automatically attack master's combat target
- Pet damage scales with pet level and type
- Pets have their own HP pool

### Pet Death
- Pets can die in combat
- Options for revival:
  - Respawn at pet shop (fee)
  - Healer NPC
  - Player resurrection spell
  - Timer-based respawn (ghost follows master)

### Pet Stats
```
Level: 1-50 (matches master level cap)
HP: Base by type, scales with level
Damage: Base by type, scales with level
Armor: Base by type
Speed: Determines attack frequency
```

## Care System (Optional)

### Hunger
- Pets get hungry over real-time
- Fed via `feed <pet> <food>`
- Hungry pets: reduced loyalty gain, may not obey
- Starving pets: loyalty loss, eventual abandonment

### Happiness
- Affected by: hunger, combat deaths, interactions
- Happy pets: bonus loyalty gain, better performance
- Unhappy pets: reduced abilities, may disobey

## Mount System

### Rideable Pets
- Horses, mules, exotic mounts
- Requires `mount` command
- While mounted:
  - Faster room-to-room travel (reduced movement delay)
  - Can't be knocked down
  - Some skills unavailable
  - Mount takes some damage instead of rider

### Mount Types
| Mount | Speed Bonus | Carry Bonus | Special |
|-------|-------------|-------------|---------|
| Horse | +50% | +20 lbs | None |
| War Horse | +30% | +10 lbs | Combat trained |
| Mule | +10% | +100 lbs | Sure-footed (no fall) |
| Exotic | Varies | Varies | Unique abilities |

## Database Schema Additions

```sql
-- Pet instances (beyond mob spawns)
CREATE TABLE PetInstances (
    id SERIAL PRIMARY KEY,
    owner_character_id UUID REFERENCES Characters(id),
    mob_prototype_zone_id INT,
    mob_prototype_id INT,
    custom_name TEXT,
    loyalty INT DEFAULT 0,
    level INT DEFAULT 1,
    experience INT DEFAULT 0,
    current_hp INT,
    max_hp INT,
    hunger INT DEFAULT 100,
    happiness INT DEFAULT 100,
    created_at TIMESTAMP DEFAULT NOW(),
    last_fed_at TIMESTAMP,
    is_alive BOOLEAN DEFAULT TRUE,
    FOREIGN KEY (mob_prototype_zone_id, mob_prototype_id)
        REFERENCES Mobs(zone_id, id)
);

-- Pet abilities unlocked
CREATE TABLE PetAbilities (
    pet_id INT REFERENCES PetInstances(id),
    ability_name TEXT,
    unlocked_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (pet_id, ability_name)
);
```

## Implementation Phases

### Phase 1: Basic Minion Command
- [ ] `minion` list command
- [ ] `minion name` for custom names
- [ ] `minion dismiss` to release pets
- [ ] `minion stay/follow` toggle
- [ ] Display custom names: "A kitten named Whiskers"

### Phase 2: Combat Integration
- [ ] Pets auto-attack master's target
- [ ] Pet HP and death
- [ ] Basic pet stats (damage, HP)
- [ ] Pet respawn mechanism

### Phase 3: Mount System
- [ ] `mount`/`dismount` commands
- [ ] Movement speed bonus while mounted
- [ ] Riding skill check?

### Phase 4: Utility Abilities
- [ ] Per-pet-type special abilities
- [ ] Loyalty-gated ability unlocks
- [ ] Scout, track, detect commands

### Phase 5: Care & Progression
- [ ] Hunger/happiness system
- [ ] Loyalty progression
- [ ] Pet leveling
- [ ] Persistent pet storage (database)

## Open Questions

1. Should pets persist across sessions (database) or be session-only?
2. Pet limit per player? (1? 3? Unlimited?)
3. Can pets die permanently, or always recoverable?
4. PvP implications - pets attack other players?
5. Pet trading between players?
6. Taming wild mobs as alternative to buying?

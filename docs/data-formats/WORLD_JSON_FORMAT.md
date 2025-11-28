# FieryMUD World JSON Format

This document describes the JSON format used for FieryMUD world files. These files are stored in `data/world/` directory.

## Top-Level Structure

Each zone file (e.g., `30.json`) contains a nested structure with four main sections:

```json
{
    "zone": { ... },
    "mobs": [ ... ],
    "objects": [ ... ],
    "rooms": [ ... ],
    "shops": [ ... ],
    "triggers": [ ... ]
}
```

## Zone Section

The `zone` object contains zone metadata and reset configuration:

```json
"zone": {
    "id": "30",                    // Zone ID (string)
    "name": "Mielikki",            // Zone display name
    "top": 3499,                   // Highest room number in zone
    "lifespan": 30,                // Minutes between resets
    "reset_mode": "Normal",        // Reset behavior (see ResetModes enum)
    "hemisphere": "NORTHWEST",     // Weather system (see Hemispheres enum)
    "climate": "OCEANIC",          // Climate type (see Climates enum)
    "resets": {                    // Zone reset configuration
        "mob": [ ... ]             // Mobile spawning definitions
    }
}
```

### Mobile Reset Configuration

The new format uses inline equipment and inventory specification:

```json
"mob": [
    {
        "id": 3100,                        // Mobile ID to spawn
        "max": 1,                          // Maximum instances
        "room": 3052,                      // Room to spawn in
        "name": "(a half-elven maid)",     // Human-readable name
        "carrying": [                      // Objects mob carries in inventory
            {
                "id": 3100,
                "max": 500,
                "name": "(a cup)"
            },
            {
                "id": 3101, 
                "max": 500,
                "name": "(a cup)"
            }
        ],
        "equipped": [                      // Objects mob wears/wields
            {
                "id": 3077,
                "max": 99,
                "location": "Head",        // Equipment slot name (see EquipSlot enum)
                "name": "(a green hair-ribbon)"
            },
            {
                "id": 3076,
                "max": 99,
                "location": "Feet",       // Equipment slot name
                "name": "(light brown leather shoes)"
            },
            {
                "id": 3075,
                "max": 99,
                "location": "Body",       // Equipment slot name
                "name": "(an apple-green linen dress)"
            }
        ]
    }
]
```

## Mobs Section

The `mobs` array contains mobile (NPC) definitions:

```json
"mobs": [
    {
        "id": 3000,                           // Unique mobile ID
        "name_list": "bigby wizard shopkeeper", // Keywords for targeting
        "short_description": "Bigby",         // Name shown in room
        "long_description": "Bigby walks around...", // Description when in room
        "description": "Bigby looks to be very old...", // Examine description
        "mob_flags": ["SENTINEL", "ISNPC"],   // See MobFlags enum
        "effect_flags": ["DETECT_INVIS"],     // See EffectFlags enum
        "alignment": 900,                     // Good/evil alignment (-1000 to 1000)
        "level": 99,                         // Character level
        "hp_dice": { "num": 0, "size": 0, "bonus": 0 }, // Hit point calculation
        "move": 0,                           // Movement points
        "ac": 0,                             // Armor class
        "hit_roll": 0,                       // Attack bonus
        "damage_dice": { "num": 0, "size": 0, "bonus": 0 }, // Damage calculation
        "money": { "copper": 0, "silver": 0, "gold": 0, "platinum": 0 },
        "position": "STANDING",              // See Position enum
        "default_position": "STANDING",      // See Position enum
        "gender": "NEUTRAL",                 // See Gender enum
        "mob_class": "Sorcerer",             // See Class enum
        "race": "HUMAN",                     // See Race enum
        "race_align": 0,                     // Racial alignment modifier
        "size": "MEDIUM",                    // See Size enum
        "stats": { "strength": 18, "intelligence": 25, "wisdom": 20, "dexterity": 15, "constitution": 18, "charisma": 16 },
        "perception": 50,                    // Perception skill
        "concealment": 0,                    // Stealth skill
        "life_force": "LIFE",                // See LifeForce enum
        "composition": "FLESH",              // See Composition enum
        "stance": "ALERT",                   // See Stance enum
        "damage_type": "HIT"                 // See DamageType enum
    }
]
```

## Objects Section

The `objects` array contains item definitions:

```json
"objects": [
    {
        "id": "3000",                        // Unique object ID (string)
        "type": "DRINKCON",                  // See ObjectType enum
        "name_list": "beer barrel beer-barrel", // Keywords for targeting
        "short_description": "a barrel",     // Name shown in inventory
        "description": "A beer barrel has been left here.", // Description when on ground
        "action_description": "",            // Special action text
        "extra_descriptions": [ { "keyword": "barrel", "desc": "..." } ],
        "values": {                          // Type-specific properties
            "Capacity": "512",
            "Remaining": "512",
            "Liquid": "BEER",                // See Liquids enum
            "Poisoned": "0"
        },
        "flags": ["FLOAT"],                  // See ObjectFlags enum
        "weight": "8.00",
        "cost": "300",
        "timer": "0",
        "decompose_timer": "0",
        "level": "1",
        "effect_flags": [],                  // See EffectFlags enum
        "wear_flags": ["TAKE", "HOLD"],      // See WearFlags enum
        "concealment": 0,
        "affects": [ { "location": "STRENGTH", "modifier": 1 } ], // See ApplyTypes enum
        "applies": {},
        "spells": [],
        "triggers": [],
        "script_variables": {},
        "effects": []
    }
]
```

## Rooms Section

The `rooms` object contains an array with room definitions:

```json
"rooms": [
    {
        "id": "3001",
        "name": "The Forest Temple of Mielikki",
        "description": "You are standing in the ancient wooded grove...",
        "sector": "STRUCTURE",           // See Sectors enum
        "flags": ["NOMOB", "INDOORS"],   // See RoomFlags enum
        "exits": {
            "North": {
                "description": "You see the altar...",
                "keyword": "",
                "key": "-1",
                "destination": "3002"
            }
        },
        "extra_descriptions": {
            "altar": "The altar is made of living wood..."
        }
    }
]
```

## Shops Section

The `shops` array contains shop definitions:

```json
"shops": [
    {
        "id": 3000,
        "selling": { "3001": 0 }, // item_id: amount (0 for infinite)
        "buy_profit": 1.0,
        "sell_profit": 1.0,
        "accepts": [ { "type": "WEAPON", "keywords": "sword" } ],
        "no_such_item1": "%s I don't see one of those.",
        "no_such_item2": "You don't seem to have that.",
        "do_not_buy": "I don't buy those.",
        "missing_cash1": "You can't afford it!",
        "missing_cash2": "You don't have enough money.",
        "message_buy": "$n buys $p.",
        "message_sell": "$n sells $p.",
        "temper1": 0,
        "flags": ["WILL_BUY_SAME_ITEM"], // See ShopFlags enum
        "keeper": 3000, // mob_id of the shopkeeper
        "trades_with": ["TRADES_WITH_ANYONE"], // See ShopTradesWith enum
        "rooms": [3001],
        "hours": [ { "open": 0, "close": 23 } ]
    }
]
```

## Triggers Section

The `triggers` array contains script trigger definitions:

```json
"triggers": [
    {
        "id": "1",
        "name": "my_trigger",
        "attach_type": "Object", // See ScriptType enum
        "flags": ["DEATH_TRIGGER"], // See TriggerFlags enum
        "number_of_arguments": "0",
        "argument_list": "",
        "commands": "say Hello World!"
    }
]
```

## Data Types and Enums

This section details the various enums and data structures used throughout the JSON files.

### `ApplyTypes`

Used in object `affects` to specify what character attribute is modified.

- `Str`, `Dex`, `Int`, `Wis`, `Con`, `Cha`
- `Class`, `Level`, `Age`, `CharacterWeight`, `CharacterHeight`
- `Mana`, `Hit`, `Move`, `Gold`, `Exp`
- `AC`, `HitRoll`, `DamRoll`
- `SavingParalysis`, `SavingRod`, `SavingPetrification`, `SavingBreath`, `SavingSpell`
- `Size`, `Regeneration`, `Focus`, `Perception`, `Concealment`, `Composition`

### `Class`

Character classes for mobs and players.

- `Sorcerer`, `Cleric`, `Thief`, `Warrior`, `Paladin`, `AntiPaladin`, `Ranger`, `Druid`, `Shaman`, `Assassin`, `Mercenary`, `Necromancer`, `Conjurer`, `Monk`, `Berserker`, `Priest`, `Diabolist`, `Mystic`, `Rogue`, `Bard`, `Pyromancer`, `Cryomancer`, `Illusionist`, `Hunter`, `Layman`

### `Climates`

Zone climate types.

- `NONE`, `SEMIARID`, `ARID`, `OCEANIC`, `TEMPERATE`, `SUBTROPICAL`, `TROPICAL`, `SUBARCTIC`, `ARCTIC`, `ALPINE`

### `Composition`

The physical makeup of a mob.

- `Flesh`, `Earth`, `Air`, `Fire`, `Water`, `Ice`, `Mist`, `Ether`, `Metal`, `Stone`, `Bone`, `Lava`, `Plant`

### `DamageType`

Types of physical damage.

- `HIT`, `STING`, `WHIP`, `SLASH`, `BITE`, `BLUDGEON`, `CRUSH`, `POUND`, `CLAW`, `MAUL`, `THRASH`, `PIERCE`, `BLAST`, `PUNCH`, `STAB`, `FIRE`, `COLD`, `ACID`, `SHOCK`, `POISON`, `ALIGN`

### `Direction`

Standard directions for room exits.

- `North`, `East`, `South`, `West`, `Up`, `Down`

### `EquipSlot`

Equipment slot names used in the `equipped` section. These correspond to the EquipSlot enum values:

- `Light` (0) - Light source slot
- `FingerRight` (1) - Right finger
- `FingerLeft` (2) - Left finger  
- `Neck1` (3) - First neck slot
- `Neck2` (4) - Second neck slot
- `Body` (5) - Main body armor
- `Head` (6) - Helmet/hat
- `Legs` (7) - Leg armor
- `Feet` (8) - Boots/shoes
- `Hands` (9) - Gloves
- `Arms` (10) - Arm protection
- `Shield` (11) - Shield
- `About` (12) - Cloak/cape
- `Waist` (13) - Belt
- `WristRight` (14) - Right wrist
- `WristLeft` (15) - Left wrist
- `Wield` (16) - Main weapon
- `Hold` (17) - Held item
- `Float` (18) - Floating item
- `Eyes` (21) - Eye slot (glasses, etc.)
- `Face` (22) - Face slot
- `Ear` (23) - Ear slot
- `Belt` (25) - Belt slot

### `Gender`

- `Neutral`, `Male`, `Female`, `NonBinary`

### `Hemispheres`

Zone weather hemispheres.

- `NORTHWEST`, `NORTHEAST`, `SOUTHWEST`, `SOUTHEAST`

### `LifeForce`

The nature of a mob's life force.

- `Life`, `Undead`, `Magic`, `Celestial`, `Demonic`, `Elemental`

### `ObjectFlags`

Flags that can be set on objects.

- `Glow`, `Hum`, `NoRent`, `AntiBerserker`, `NoInvisible`, `Invisible`, `Magic`, `NoDrop`, `Permanent`, `AntiGood`, `AntiEvil`, `AntiNeutral`, `AntiSorcerer`, `AntiCleric`, `AntiRogue`, `AntiWarrior`, `NoSell`, `AntiPaladin`, `AntiAntiPaladin`, `AntiRanger`, `AntiDruid`, `AntiShaman`, `AntiAssassin`, `AntiMercenary`, `AntiNecromancer`, `AntiConjurer`, `NoBurn`, `NoLocate`, `Decomposing`, `Float`, `NoFall`, `WasDisarmed`, `AntiMonk`, `AntiBard`, `Elven`, `Dwarven`, `AntiThief`, `AntiPyromancer`, `AntiCryomancer`, `AntiIllusionist`, `AntiPriest`, `AntiDiabolist`, `AntiTiny`, `AntiSmall`, `AntiMedium`, `AntiLarge`, `AntiHuge`, `AntiGiant`, `AntiGargantuan`, `AntiColossal`, `AntiTitanic`, `AntiMountainous`, `AntiArborean`

### `ObjectType`

The type of an object, which determines its `values`.

- `Nothing`, `Light`, `Scroll`, `Wand`, `Staff`, `Weapon`, `FireWeapon`, `Missile`, `Treasure`, `Armor`, `Potion`, `Worn`, `Other`, `Trash`, `Trap`, `Container`, `Note`, `DrinkContainer`, `Key`, `Food`, `Money`, `Pen`, `Boat`, `Fountain`, `Portal`, `Rope`, `SpellBook`, `Wall`, `Touchstone`, `Board`, `Instrument`

### `Position`

A mob's physical position.

- `Prone`, `Sitting`, `Kneeling`, `Standing`, `Flying`

### `Race`

- `Human`, `Elf`, `Gnome`, `Dwarf`, `Troll`, `Drow`, `Duergar`, `Ogre`, `Orc`, `HalfElf`, `Barbarian`, `Halfling`, `Plant`, `Humanoid`, `Animal`, `DragonGeneral`, `Giant`, `Other`, `Goblin`, `Demon`, `Brownie`, `DragonFire`, `DragonFrost`, `DragonAcid`, `DragonLightning`, `DragonGas`, `DragonbornFire`, `DragonbornFrost`, `DragonbornAcid`, `DragonbornLightning`, `DragonbornGas`, `Sverfneblin`, `FaerieSeelie`, `FaerieUnseelie`, `Nymph`, `Arborean`

### `ResetModes`

Zone reset behavior.

- `Never`, `Empty`, `Normal`

### `RoomFlags`

Flags that can be set on rooms.

- `DARK`, `DEATH`, `NOMOB`, `INDOORS`, `PEACEFUL`, `SOUNDPROOF`, `NOTRACK`, `NOMAGIC`, `TUNNEL`, `PRIVATE`, `GODROOM`, `HOUSE`, `HOUSECRASH`, `ATRIUM`, `OLC`, `BFS_MARK`

### `Sectors`

The terrain type of a room.

- `STRUCTURE`, `CITY`, `FIELD`, `FOREST`, `HILLS`, `MOUNTAIN`, `SHALLOWS`, `WATER`, `UNDERWATER`, `AIR`, `ROAD`, `GRASSLANDS`, `CAVE`, `RUINS`, `SWAMP`, `BEACH`, `UNDERDARK`, `ASTRALPLANE`, `AIRPLANE`, `FIREPLANE`, `EARTHPLANE`, `ETHEREALPLANE`, `AVERNUS`

### `ScriptType`

The type of entity a trigger is attached to.

- `Mob`, `Object`, `World`

### `Size`

- `Tiny`, `Small`, `Medium`, `Large`, `Huge`, `Giant`, `Gargantuan`, `Colossal`, `Titanic`, `Mountainous`

### `Stance`

A mob's combat stance.

- `Dead`, `Mort`, `Incapacitated`, `Stunned`, `Sleeping`, `Resting`, `Alert`, `Fighting`

### `WearFlags`

Determines where an object can be worn.

- `Take`, `Finger`, `Neck`, `Body`, `Head`, `Legs`, `Feet`, `Hands`, `Arms`, `Shield`, `About`, `Waist`, `Wrist`, `Wield`, `Hold`, `TwoHandWield`, `Eyes`, `Face`, `Ear`, `Badge`, `Belt`, `Hover`

## Field Mappings

Key differences between legacy and modern field names:

| JSON Field | Modern Field | Notes |
|------------|--------------|-------|
| `name_list` | `name` | Keywords for targeting |
| `short_description` | `short_desc` | Brief description |
| `long_description` | `long_desc` | Room presence description |
| `action_description` | `action_desc` | Special action text |
| `extra_descriptions` | `extra_descs` | Array vs object structure |
| `mob_flags` | `flags` | Comma-separated string vs. array of strings |
| `effect_flags` | `effects` | Magical effects |

## ID Handling

- Zone IDs are strings in JSON but converted to integers
- Zone 0 is converted to ID 1000 (modern system rejects ID 0)
- Room and object IDs are strings in JSON
- Mobile IDs are integers in JSON

## Usage in Code

The modern FieryMUD system loads these files via:

1. `WorldManager::load_world()` reads zone files from `data/world/`
2. `Zone::from_json()` parses the zone section
3. Entity `from_json()` methods handle mobs, objects, and rooms
4. Field mapping handles differences between JSON and internal representation
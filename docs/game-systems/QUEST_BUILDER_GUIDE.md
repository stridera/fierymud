# Quest Builder Guide

A hands-on guide for builders creating quests in Muditor. For technical details, see [QUEST_SYSTEM.md](QUEST_SYSTEM.md).

## Quick Start

1. Open Muditor at `http://localhost:3000`
2. Log in with an IMMORTAL (or higher) account
3. Navigate to **Quests** in the sidebar
4. Click **Create New Quest**

## Key Concepts

Before building, understand how quests are structured:

```
Quest "The Stolen Relic"
├── Phase 1: "The Plea" (must complete before Phase 2 unlocks)
│   ├── Objective: Talk to NPC "Elder Helena"
│   └── Reward: 200 XP
├── Phase 2: "Gather Evidence" (all objectives can be done in any order)
│   ├── Objective: Collect 3x Torn Cloth
│   ├── Objective: Visit the Ruined Shrine
│   └── Reward: 500 XP, 100 gold
├── Phase 3: "Confront the Thief"
│   ├── Objective: Kill Shadow Thief
│   ├── Objective: Collect the Stolen Relic
│   └── Reward: 2000 XP, 500 gold
└── Phase 4: "Return the Relic"
    ├── Objective: Talk to NPC "Elder Helena"
    └── Reward: 5000 XP, 1000 gold, choice of item reward
```

**Phases** run in order — players must finish all objectives in Phase 1 before Phase 2 begins. **Objectives** within a phase can be completed in any order.

## Walkthrough: Building "The Stolen Relic"

This example creates a 4-phase quest with talk, collect, visit, kill, and return objectives.

### Step 1: Create the Quest (Basic Info tab)

Click **Create New Quest** from the quest list page. You'll land on the **Basic Info** tab.

| Field | Value | Notes |
|-------|-------|-------|
| Zone ID | `30` | The zone this quest belongs to |
| Quest ID | `5` | Unique within the zone (check existing quests first) |
| Name | `The Stolen Relic` | Supports color codes (e.g., `&1The &3Stolen &1Relic&0`) |
| Description | `Elder Helena's sacred relic has been stolen by a shadowy figure...` | Shown when players inspect the quest |
| Min Level | `10` | Lowest level that can receive this quest |
| Max Level | `30` | Highest level that can receive this quest |
| Repeatable | unchecked | Check if players can redo it |
| Hidden | unchecked | Hidden quests don't appear in quest lists |

**Trigger Configuration:**

| Field | Value | Notes |
|-------|-------|-------|
| Trigger Type | **Mob Encounter** | Player talks to an NPC to get the quest |
| Quest Giver Mob | Search for `helena` or enter `30:5` | Use the autocomplete to find the NPC |
| Time Limit | *(empty)* | Leave blank for no timer |

Click **Create Quest** to save. The page reloads in edit mode.

### Step 2: Build the Phases (Phases & Objectives tab)

Switch to the **Phases & Objectives** tab.

#### Phase 1: "The Plea"

1. Click **+ Add Phase**
2. Rename it from "Phase 1" to `The Plea`
3. Optionally add a description: `Elder Helena asks you to investigate the theft.`

**Add the objective:**
1. Click **+ Add Objective** inside Phase 1
2. Set the fields:

| Field | Value |
|-------|-------|
| Type | **Talk to NPC** |
| Count | `1` |
| Player Description | `Speak with Elder Helena about the stolen relic` |
| Target Mob | Search `helena` or `30:5` |
| Show Progress | checked |

**Add the reward:**
1. Click **+ Add Reward** inside Phase 1
2. Set Type to **Experience**, Amount to `200`

#### Phase 2: "Gather Evidence"

1. Click **+ Add Phase**
2. Rename to `Gather Evidence`

**Add objectives** (click + Add Objective for each):

| # | Type | Description | Target | Count |
|---|------|-------------|--------|-------|
| 1 | **Collect Item** | `Collect torn cloth from the crime scene` | Search `torn cloth` or `30:50` | `3` |
| 2 | **Visit Room** | `Investigate the ruined shrine` | Search `ruined shrine` or `30:25` | `1` |

Both objectives can be completed in any order — the player might visit the shrine first, or collect cloth first.

**Add rewards:**
1. **Experience**: `500`
2. **Gold**: `100`

#### Phase 3: "Confront the Thief"

1. Click **+ Add Phase**
2. Rename to `Confront the Thief`

**Add objectives:**

| # | Type | Description | Target | Count |
|---|------|-------------|--------|-------|
| 1 | **Kill Mob** | `Defeat the Shadow Thief` | Search `shadow thief` or `30:60` | `1` |
| 2 | **Collect Item** | `Take the stolen relic from the thief` | Search `stolen relic` or `30:101` | `1` |

**Add rewards:**
1. **Experience**: `2000`
2. **Gold**: `500`

#### Phase 4: "Return the Relic"

1. Click **+ Add Phase**
2. Rename to `Return the Relic`

**Add objective:**

| Type | Description | Target | Count |
|------|-------------|--------|-------|
| **Talk to NPC** | `Return the relic to Elder Helena` | `30:5` (Helena) | `1` |

**Add rewards:**
1. **Experience**: `5000`
2. **Gold**: `1000`
3. **Item**: Search for the reward item (e.g., `blessed amulet` or `30:200`) — set **Choice Group** to `1`
4. **Item**: Search for an alternate reward (e.g., `enchanted ring` or `30:201`) — also set **Choice Group** to `1`

Rewards sharing the same **Choice Group** number let the player pick one. In this case, the player chooses between the amulet and the ring.

### Step 3: Set Requirements (Requirements tab)

Switch to the **Requirements** tab if this quest has class, race, or other prerequisites.

**Example: Warriors and Paladins only:**
```lua
character.class == 'WARRIOR' or character.class == 'PALADIN'
```

**Example: Must have completed another quest first:**
```lua
character:hasCompletedQuest(30, 0)
```

**Example: Elves who are at least level 15:**
```lua
character.race == 'ELF' and character.level >= 15
```

Leave this blank if the quest is open to everyone (level requirements on the Basic Info tab still apply).

### Step 4: Verify in the Quest List

Go back to the **Quests** page. Your quest should appear in the list for Zone 30. Click it to expand and verify all phases, objectives, and rewards look correct.

## Objective Type Reference

| Type | What the Player Does | You Configure |
|------|---------------------|---------------|
| **Kill Mob** | Kill a specific mob | Target mob + count |
| **Collect Item** | Pick up / loot an item | Target item + count |
| **Deliver Item** | Give an item to an NPC | Target item + delivery NPC |
| **Visit Room** | Walk into a room | Target room |
| **Talk to NPC** | Interact with an NPC | Target mob |
| **Use Skill** | Cast a spell or use ability | Ability ID |
| **Custom (Lua)** | Any custom condition | Lua expression |

### Target Fields by Objective Type

Each objective type shows different target fields in the editor:

- **Kill Mob / Talk to NPC** — mob autocomplete (search by name or `zone:id`)
- **Collect Item / Deliver Item** — item autocomplete
- **Deliver Item** also shows a second mob autocomplete for the delivery recipient
- **Visit Room** — room autocomplete
- **Use Skill** — ability ID number field
- **Custom (Lua)** — free-form Lua code editor

## Reward Types

| Type | Fields | Notes |
|------|--------|-------|
| **Experience** | Amount | XP granted |
| **Gold** | Amount | Gold granted |
| **Item** | Item (autocomplete) | Item given to player |
| **Ability** | Ability ID | Teaches a skill/spell |

### Choice Groups

Give the player a choice between rewards by assigning the same **Choice Group** number:

```
Reward 1: Item "Fire Sword"     → Choice Group: 1
Reward 2: Item "Ice Staff"      → Choice Group: 1
Reward 3: 1000 Gold             → Choice Group: (empty)
```

Result: The player receives 1000 gold automatically, plus chooses either the Fire Sword or Ice Staff.

## Trigger Types

How the quest becomes available to players:

| Trigger | How It Works | When to Use |
|---------|-------------|-------------|
| **Mob Encounter** | Player talks to a specific NPC | Standard quest givers |
| **Level Reached** | Auto-offered when player hits a level | Tutorial/milestone quests |
| **Item Obtained** | Triggered by picking up an item | Discovery quests (find a mysterious note) |
| **Room Entered** | Triggered by entering a room | Exploration quests |
| **Skill Used** | Triggered by using an ability | Class-specific skill quests |
| **Event Active** | Only during server events | Holiday/seasonal quests |
| **Auto-Start** | Always available when requirements met | Tutorial quests |
| **Manual** | Only given by GM command or script | Debug/testing, scripted events |

## Common Patterns

### Linear Story Quest

Phases that tell a story in sequence. Each phase has one or two objectives.

```
Phase 1: Talk to quest giver       →  Phase 2: Go to location
Phase 3: Kill the boss             →  Phase 4: Return to quest giver
```

### Hub-and-Spoke (Gather Phase)

A phase with many objectives the player completes in any order:

```
Phase 1: Talk to quest giver
Phase 2: (all at once)
  - Collect 5 wolf pelts
  - Collect 3 bear claws
  - Visit the hunter's lodge
Phase 3: Return to quest giver
```

### Escort / Deliver

Use DELIVER_ITEM to require giving an item to a specific NPC:

```
Phase 1: Collect the letter (COLLECT_ITEM)
Phase 2: Deliver the letter to Captain Voss (DELIVER_ITEM → target item + target NPC)
```

### Branching Quests (Exclusive Groups)

Create two quests with the same **Exclusive Group** string on the Basic Info tab. Once a player accepts one, the other disappears.

```
Quest A: "Join the Light" → Exclusive Group: "faction-choice"
Quest B: "Join the Dark"  → Exclusive Group: "faction-choice"
```

### Repeatable Daily Quest

Check **Repeatable** on the Basic Info tab. Optionally set a **Cooldown** (minutes) to prevent spam. The game tracks how many times each player has completed it.

### Class-Restricted Quest

On the Requirements tab:
```lua
character.class == 'THIEF'
```

### Quest Chains

Use the prerequisite system. If quest `30:5` requires completing `30:4` first, add a prerequisite via the GraphQL API (prerequisite management UI is on the quest list page).

## Tips

- **Check your Zone ID and Quest ID** before creating — these can't be changed after creation.
- **Name phases clearly** — players see phase names in their quest log.
- **Write good player descriptions** — these appear in the quest tracker (e.g., "Defeat the Shadow Thief" not "kill mob 30:60").
- **Use internal notes** for builder-only comments on objectives (e.g., "mob spawns in room 30:42 after midnight").
- **Test with `qgive`** — an admin can use `qgive <player> 30:5` in-game to give the quest for testing without needing the trigger.
- **Use `qstat`** — check a player's quest progress with `qstat <player>` in-game.
- **Reload after changes** — use `qreload 30` in-game to pick up editor changes without restarting the server.
- **Show Progress** — leave this checked for kill/collect objectives so players see "3/5 wolf pelts". Uncheck it for talk/visit objectives where a counter doesn't make sense.

## In-Game Testing Commands

| Command | What It Does |
|---------|-------------|
| `qlist 30` | List all quests loaded for zone 30 |
| `qgive <player> 30:5` | Give quest 30:5 to a player |
| `qstat <player>` | View a player's quest progress |
| `qcomplete <player> 30:5` | Force-complete a quest |
| `qreload 30` | Reload zone 30 quests from the database |
| `quest accept 30:5` | (As a player) Accept a quest |
| `quests` | (As a player) View quest log |
| `questinfo 30:5` | (As a player) View quest details |

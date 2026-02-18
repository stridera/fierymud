# Quest System

The quest system is a full-stack feature spanning all three repositories: quests are defined and edited in **Muditor**, stored in **PostgreSQL**, loaded and executed by the **FieryMUD** game server, and optionally seeded from legacy data via **FieryLib**.

## Architecture Overview

```
Muditor (Web Editor)                PostgreSQL                  FieryMUD (Game Server)
┌──────────────────┐          ┌──────────────────┐          ┌──────────────────┐
│ Quest List UI    │──CRUD───▶│ Quest            │◀──load──│ QuestManager     │
│ Quest Editor UI  │  via     │ QuestPhase       │  per    │  (singleton)     │
│ GraphQL API      │  GraphQL │ QuestObjective   │  zone   │                  │
│                  │          │ QuestReward      │         │ QuestQueries     │
│                  │          │ QuestPrerequisite│         │  (SQL layer)     │
│                  │          │ QuestDialogue    │         │                  │
│                  │          │                  │         │ Quest Commands   │
│                  │          │ CharacterQuest   │◀──r/w──│ Lua Bindings     │
│                  │          │ CharacterQuest-  │         │ Objective Hooks  │
│                  │          │   Objective      │         │                  │
└──────────────────┘          └──────────────────┘          └──────────────────┘
```

## Data Model

All quest entities use **composite primary keys** (`zoneId`, `id`) consistent with the rest of the codebase.

### Core Tables

| Table | Primary Key | Description |
|-------|-------------|-------------|
| `Quest` | `(zoneId, id)` | Quest definition (name, level range, trigger config) |
| `QuestPhase` | `(questZoneId, questId, id)` | Sequential phases within a quest |
| `QuestObjective` | `(questZoneId, questId, phaseId, id)` | Tasks within a phase (any order) |
| `QuestReward` | `id` (auto) | Rewards granted per-phase |
| `QuestPrerequisite` | `id` (auto) | Quest chains (must complete X before Y) |
| `QuestDialogue` | `id` (auto) | NPC dialogue for TALK_TO_NPC objectives |

### Player Progress Tables

| Table | Primary Key | Description |
|-------|-------------|-------------|
| `CharacterQuest` | `id` (UUID) | Player's quest status, current phase, variables |
| `CharacterQuestObjective` | `id` (UUID) | Per-objective progress counters |

### Quest Structure

```
Quest (30:1) "The Lost Artifact"
├── Phase 1: "Find the Map" (order: 0)
│   ├── Objective 1: COLLECT_ITEM "Find the ancient map" (1x item 30:100)
│   └── Rewards: 500 XP
├── Phase 2: "Retrieve the Artifact" (order: 1)
│   ├── Objective 1: KILL_MOB "Defeat the guardian" (1x mob 30:60)
│   ├── Objective 2: COLLECT_ITEM "Take the artifact" (1x item 30:101)
│   └── Rewards: 5000 XP, 500 gold, item 30:200
└── Prerequisites: [Quest 30:0 must be COMPLETED]
```

Phases execute **sequentially** (in `order`). Objectives within a phase can be completed in **any order**.

## Enums

### QuestStatus
| Value | Description |
|-------|-------------|
| `IN_PROGRESS` | Player has accepted and is working on the quest |
| `COMPLETED` | Quest finished successfully |
| `FAILED` | Quest failed (time expired, etc.) |
| `ABANDONED` | Player abandoned the quest |

### QuestObjectiveType
| Value | Description | Target Fields |
|-------|-------------|---------------|
| `KILL_MOB` | Kill specific mob(s) | `targetMobZoneId`, `targetMobId` |
| `COLLECT_ITEM` | Pick up specific item(s) | `targetObjectZoneId`, `targetObjectId` |
| `DELIVER_ITEM` | Give item to NPC | `targetObjectZoneId/Id`, `deliverToMobZoneId/Id` |
| `VISIT_ROOM` | Enter a specific room | `targetRoomZoneId`, `targetRoomId` |
| `TALK_TO_NPC` | Talk to a specific NPC | `targetMobZoneId`, `targetMobId` |
| `USE_SKILL` | Use a specific ability | `targetAbilityId` |
| `CUSTOM_LUA` | Custom Lua expression | `luaExpression` |

### QuestTriggerType
Controls how a quest becomes available/starts:

| Value | Description | Trigger Field |
|-------|-------------|---------------|
| `MOB` | Talk to NPC quest giver | `triggerMobZoneId/Id` |
| `LEVEL` | Auto-granted at level | `triggerLevel` |
| `ITEM` | Triggered by picking up item | `triggerItemZoneId/Id` |
| `ROOM` | Triggered by entering room | `triggerRoomZoneId/Id` |
| `SKILL` | Triggered by using ability | `triggerAbilityId` |
| `EVENT` | Available during game event | `triggerEventId` |
| `AUTO` | Auto-granted (tutorial quests) | *(none)* |
| `MANUAL` | Only via GM command or script | *(none)* |

### QuestRewardType
| Value | Description | Fields |
|-------|-------------|--------|
| `EXPERIENCE` | XP reward | `amount` |
| `GOLD` | Gold reward | `amount` |
| `ITEM` | Item reward | `objectZoneId/Id` |
| `ABILITY` | Learn ability | `abilityId` |

Rewards support **choice groups**: rewards sharing the same non-null `choiceGroup` value let the player pick one.

## FieryMUD Game Server (C++)

### Key Files

| File | Purpose |
|------|---------|
| `src/quests/quest_manager.hpp/.cpp` | Central singleton managing all quest state |
| `src/commands/quest_commands.hpp/.cpp` | Player and admin in-game commands |
| `src/database/quest_queries.hpp/.cpp` | SQL query layer (prepared statements) |
| `src/scripting/bindings/lua_quest.hpp/.cpp` | Lua scripting API |
| `tests/unit/test_quest_system.cpp` | Catch2 unit tests |

### QuestManager

Singleton accessed via `FieryMUD::QuestManager::instance()`. NOT thread-safe; must be called from the game loop strand.

**Lifecycle:**
1. `initialize()` — called at server startup
2. `load_zone_quests(zone_id)` — loads quests from DB into cache when a zone is loaded
3. Game events trigger objective tracking via `on_*` hooks
4. `shutdown()` — logs statistics and cleans up

**Quest Cache:** Quests are cached in memory per-zone. Trigger indexes provide fast lookup:
- `level_triggered_quests_` — level -> quests
- `item_triggered_quests_` — item EntityId -> quests
- `room_triggered_quests_` — room EntityId -> quests
- `skill_triggered_quests_` — ability ID -> quests
- `event_triggered_quests_` — event ID -> quests
- `auto_triggered_quests_` — always-available quests

### Objective Progress Hooks

The QuestManager provides event hooks that game systems call to automatically update quest progress:

```cpp
// Called from combat system when a mob dies
quest_manager.on_mob_killed(killer, killed_mob_id);

// Called from item pickup handler
quest_manager.on_item_collected(collector, item_id);

// Called when giving item to NPC
quest_manager.on_item_delivered(deliverer, recipient_mob_id, item_id);

// Called from say/tell handler for NPC dialogue
quest_manager.on_npc_talked(talker, npc_id);

// Called from room entry handler
quest_manager.on_room_visited(visitor, room_id);

// Called from ability use handler
quest_manager.on_skill_used(user, ability_id);
```

Each hook iterates the player's active quests, checks if any current-phase objectives match the event, and updates the progress count in the database.

### Player Commands

| Command | Privilege | Description |
|---------|-----------|-------------|
| `quest accept <name\|zone:id>` | Player | Accept an available quest |
| `quest abandon <name\|zone:id>` | Player | Abandon an active quest |
| `quests [completed\|all]` | Player | View quest log |
| `questinfo <zone:id>` | Player | View detailed quest info |
| `qstat <player>` | God | View player's quest progress |
| `qgive <player> <zone:id>` | God | Give quest to player |
| `qcomplete <player> <zone:id>` | God | Force-complete quest |
| `qlist [zone]` | God | List loaded quests / system stats |
| `qload <zone>` | God | Load quests for zone |
| `qreload <zone>` | God | Reload quests for zone |

Quest identifiers can be specified as `zone:id` (e.g., `30:1`) or by name prefix (e.g., `lost art`).

### Lua Scripting API

Scripts can interact with the quest system through the `quest` table:

```lua
-- Quest lifecycle
quest.start(actor, zone_id, quest_id)        -- Start a quest
quest.complete(actor, zone_id, quest_id)     -- Complete a quest
quest.abandon(actor, zone_id, quest_id)      -- Abandon a quest

-- Status queries (returns: "NONE", "AVAILABLE", "IN_PROGRESS", "COMPLETED", "FAILED", "ABANDONED")
quest.status(actor, zone_id, quest_id)
quest.has_quest(actor, zone_id, quest_id)     -- true if IN_PROGRESS
quest.is_available(actor, zone_id, quest_id)  -- true if can accept
quest.is_completed(actor, zone_id, quest_id)  -- true if ever completed

-- Objective progress
quest.advance_objective(actor, zone_id, quest_id, objective_id, count)  -- count defaults to 1

-- Quest variables (JSON-backed, stored in CharacterQuest.variables)
quest.set_variable(actor, zone_id, quest_id, name, value)
quest.get_variable(actor, zone_id, quest_id, name)  -- returns nil if not found
```

**Quest variables** are arbitrary key-value pairs stored as JSON. They allow scripts to track custom state that doesn't fit the standard objective model (e.g., player choices, dialogue flags, NPC reputation).

## Muditor Web Editor (TypeScript)

### Key Files

| File | Purpose |
|------|---------|
| `apps/api/src/quests/quests.resolver.ts` | GraphQL resolver (queries + mutations) |
| `apps/api/src/quests/quests.service.ts` | Prisma-based CRUD service |
| `apps/api/src/quests/quest.dto.ts` | DTOs, inputs, enums |
| `apps/web/src/app/dashboard/quests/page.tsx` | Quest list page |
| `apps/web/src/app/dashboard/quests/editor/page.tsx` | Quest editor (create/edit) |
| `apps/api/src/__tests__/quest-data-integrity.spec.ts` | Data integrity tests |

### GraphQL API

**Queries:**
- `quests(filter, skip, take)` — List quests with optional filtering
- `questsByZone(zoneId)` — List quests in a zone
- `quest(zoneId, id)` — Get single quest with all relations
- `questsCount(zoneId?)` — Count quests
- `characterQuests(characterId)` — View player's quest progress
- `availableQuests(characterId, level)` — Quests available to a character

**Mutations** (all require JWT auth):
- `createQuest / updateQuest / deleteQuest`
- `createQuestPhase / updateQuestPhase / deleteQuestPhase`
- `createQuestObjective / updateQuestObjective / deleteQuestObjective`
- `createQuestDialogue / updateQuestDialogue / deleteQuestDialogue`
- `createQuestReward / updateQuestReward / deleteQuestReward`
- `createQuestPrerequisite / deleteQuestPrerequisite`

### Web Editor Features

The quest editor at `/dashboard/quests/editor` has three tabs:

1. **Basic Info** — Name, description, level range, repeatable/hidden flags, trigger type configuration, exclusive groups
2. **Requirements** — Lua availability expressions for class/race restrictions
3. **Phases & Objectives** — Add/remove phases, objectives (with entity autocomplete), and per-phase rewards

Access requires IMMORTAL role or higher.

## Advanced Features

### Exclusive Groups (Branching Paths)

Quests with the same non-null `exclusiveGroup` string are mutually exclusive. Once a player accepts one quest in a group, others become unavailable. Use for class specializations, faction choices, etc.

### Availability Requirements

A Lua expression evaluated to determine if a character can receive the quest, checked in addition to level requirements:

```lua
-- Warriors only
character.class == 'WARRIOR'

-- Elves who completed quest 0:5
character.race == 'ELF' and character:hasCompletedQuest(0, 5)
```

### Time Limits

Quests can have a `timeLimitMinutes` and `cooldownMinutes`. The `CharacterQuest.expiresAt` field tracks when a timed quest expires.

### Repeatable Quests

When `repeatable = true`, the `completionCount` on `CharacterQuest` tracks how many times the quest has been completed. The quest remains available after completion.

### Quest Variables

Stored as a JSON column on `CharacterQuest`, variables provide arbitrary per-player per-quest state accessible from both C++ and Lua:

```cpp
// C++ - set variable
manager.set_quest_variable(actor, 30, 1, "chose_path", json("dark"));

// C++ - get variable
auto result = manager.get_quest_variable(actor, 30, 1, "chose_path");
```

```lua
-- Lua equivalent
quest.set_variable(actor, 30, 1, "chose_path", "dark")
local path = quest.get_variable(actor, 30, 1, "chose_path")
```

### NPC Dialogue

TALK_TO_NPC objectives can have attached `QuestDialogue` with:
- `npcMessage` — What the NPC says (supports markup)
- `matchType` — How player responses are matched (`CONTAINS`, `EXACT`, `ANY_RESPONSE`)
- `matchKeywords` — Keywords to match against player input
- `dialogueTreeId` — Optional link to a full `DialogueTree` for complex conversations

## FieryLib Legacy Import

The `quest_seeder.py` imports legacy quests from `fierymud/lib/misc/quests`:

```bash
cd fierylib
poetry run fierylib import-legacy  # imports quests along with other data
```

Legacy format is simple: `quest_name quest_id max_stages`. The seeder creates a `Quest` with phases for each stage, using `triggerType: MANUAL`.

## Testing

```bash
# C++ unit tests (data structures, manager lifecycle, no DB required)
cd fierymud
cmake --build build --target tests && ctest --test-dir build -R quest

# Muditor data integrity tests
cd muditor
pnpm test:e2e  # includes quest-data-integrity.spec.ts
```

## Known TODOs

- **Item rewards**: `qcomplete` logs "item loading not implemented" — item instantiation from quest rewards not yet wired
- **Ability rewards**: ability teaching on quest completion not yet implemented
- **`questinfo` name lookup**: currently only supports `zone:id` format, not name prefix
- **Dialogue trees**: `DialogueTree`/`DialogueNode`/`DialogueResponse` schema exists but runtime integration is not yet implemented

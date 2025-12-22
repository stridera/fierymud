# Database Requirements for Command Implementation

This document tracks which commands require database changes and what already exists.

## Characters Table - Already Exists

The following fields are **already in the schema** (fierylib/prisma/schema.prisma):

| Field | Type | Used By | Notes |
|-------|------|---------|-------|
| `title` | String? | `title` command | ✅ Ready to use |
| `description` | String? | `description` command | ✅ Ready to use |
| `prompt` | String | `prompt` command | ✅ Already implemented |
| `wimpyThreshold` | Int | `wimpy` command | ✅ Ready to use |
| `playerFlags` | String[] | Toggle commands | Use for BRIEF, COMPACT, AUTOLOOT, etc. |
| `skillPoints` | Int | `practice`, `train` | For practice sessions |
| `gold`, `copper`, `silver`, `platinum` | Int | `split` command | ✅ Ready to use |

## Existing Related Models

| Model | Used By | Notes |
|-------|---------|-------|
| `CharacterEffects` | `affects` command | Stores active spell effects with duration, strength, modifiers |
| `CharacterAbilities` | `skills`, `practice` | Stores known abilities with proficiency |
| `CharacterAliases` | Future `alias` command | User-defined command shortcuts |

## Recommended: Use playerFlags for Toggles

The `playerFlags` String[] field can store toggle states. Recommended flags:

```
BRIEF       - Brief room descriptions
COMPACT     - Reduce blank lines
AUTOLOOT    - Auto-loot corpses
AUTOGOLD    - Auto-take gold from corpses
AUTOSPLIT   - Auto-split gold with group
AFK         - Away from keyboard
NOSHOUT     - Block shout channel
NOGOSSIP    - Block gossip channel
NOTELL      - Block tells
COLOR       - Enable color (ON by default)
PK_ENABLED  - PK mode active
```

## Missing Fields - Need to Add

These fields should be added to the Characters model:

```prisma
// Add to Characters model in schema.prisma:

  // AFK system
  afkMessage         String?              @map("afk_message")

  // PK system
  pkEnabledAt        DateTime?            @map("pk_enabled_at")  // For 24hr lockout

  // Consent system (alternative: separate CharacterConsent model)
  consentList        String[]             @default([]) @map("consent_list")  // Player IDs with consent
  consentAll         Boolean              @default(false) @map("consent_all")
```

## Alternative: CharacterConsent Model

For more complex consent tracking:

```prisma
model CharacterConsent {
  id            Int        @id @default(autoincrement())
  characterId   String     @map("character_id")
  targetId      String     @map("target_id")      // Who has consent
  consentType   String     @default("ALL")        // ALL, PVP, STEAL, etc.
  grantedAt     DateTime   @default(now()) @map("granted_at")
  expiresAt     DateTime?  @map("expires_at")
  character     Characters @relation(fields: [characterId], references: [id], onDelete: Cascade)

  @@unique([characterId, targetId, consentType])
}
```

## Commands by Database Status

### ✅ Ready (fields exist)
- `title` - uses Characters.title
- `description` - uses Characters.description
- `prompt` - uses Characters.prompt (already implemented)
- `wimpy` - uses Characters.wimpyThreshold
- `split` - uses Characters.gold/copper/silver/platinum
- `affects` - uses CharacterEffects model
- `skills` - uses CharacterAbilities model

### ⚠️ Use playerFlags (no schema change needed)
- `brief` - add "BRIEF" to playerFlags
- `compact` - add "COMPACT" to playerFlags
- `autoloot` - add "AUTOLOOT" to playerFlags
- `autogold` - add "AUTOGOLD" to playerFlags
- `autosplit` - add "AUTOSPLIT" to playerFlags
- `afk` - add "AFK" to playerFlags
- `toggle` - reads/writes playerFlags
- `pk` - add "PK_ENABLED" to playerFlags

### 🔧 Needs Schema Addition
- `afk` (message) - needs afkMessage field
- `pk` (lockout) - needs pkEnabledAt timestamp
- `consent` - needs consentList[] or CharacterConsent model

### 🏗️ Needs Infrastructure (not just schema)
- `practice` - needs skill definitions loaded from Ability table
- `train` - needs stat training logic and limits
- `alias` - CharacterAliases exists but needs command processing

## Group System - Runtime Only

Group membership is **not persisted** to database. These commands work with in-memory state:
- `group`, `follow`, `unfollow`, `report`
- `split`, `disband`, `abandon`
- `gtell`, `gsay`

Groups dissolve on logout/crash. This is traditional MUD behavior.

## Combat Commands - Runtime Only

Combat state is not persisted:
- `kill`, `hit`, `flee`, `release`
- `rescue`, `assist`, `kick`, `bash`

## Implementation Priority

1. **High**: Hook up existing fields (title, description, wimpy)
2. **Medium**: Implement playerFlags toggle system
3. **Low**: Add missing schema fields (afkMessage, pkEnabledAt, consent)
4. **Future**: Full skill/ability/effect systems

---
*Last updated: Phase 3 completion*

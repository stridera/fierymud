# Player Data Model

This document describes the `Player` data model defined in `scripts/mud/types/player.py`. It is intended to help developers, data migration scripts, and tooling understand each field's purpose, type, and typical semantics.

## Overview
A `Player` instance represents a persistent player character including account/session metadata, character progression, physical attributes, status effects, economy, social history, administrative privileges, and engine‐level runtime hints (e.g., paging length, load room).

Instances are constructed via the classmethod `parse_player`, which delegates to `Parser(MudTypes.PLAYER)` to translate a `MudData` record (legacy MUD file format) into keyword arguments for the dataclass.

```python
@classmethod
def parse_player(cls, player_file: MudData):
    parser = Parser(MudTypes.PLAYER)
    return cls(**parser.parse(player_file))
```

## Enumerated / Complex Types Referenced
| Type | Meaning |
|------|---------|
| `MudTypes` | Top-level discriminator for parser / serialization domain (here always `PLAYER`). |
| `Gender` | Character gender identity enum. |
| `Class` | Primary character class (e.g., Warrior, Mage). |
| `Race` | Racial lineage; may influence stats / size / abilities. |
| `LifeForce` | Broad life / power source category (e.g., mortal, undead, elemental). |
| `Composition` | Bodily composition (flesh, ethereal, construct, etc.). |
| `SavingThrows` | Structured saving throw modifiers vs. different effect schools. |
| `Stats` | Core attribute block (STR, INT, WIS, DEX, CON, etc.). |
| `Cooldown` | Enum/identifier set for named cooldown categories. |
| `QuitReason` | Enumerated reasons for last disconnect / quit. |
| `Money` | Structured currency object (coins by denomination, or unified value). |

> See their respective definitions for field-level semantics; only high-level context is captured here.

## Field Reference
Fields are grouped logically. Unless otherwise stated, fields are persisted and loaded from the legacy data format. Optional fields may be `None` if absent or not yet initialized.

### Identity & Account

| Field | Type | Description |
|-------|------|-------------|
| `id` | int | Unique numeric player identifier. Stable primary key. |
| `name` | str | Player's canonical character name. |
| `password` | str | Hashed password (NEVER store plaintext; tooling must ensure secure hash). |
| `host` | ipaddress.IPv4Address | Last known IPv4 host at login. |
| `birth_time` | datetime | Timestamp of character creation. |
| `last_login_time` | datetime | Timestamp of most recent successful login. |
| `time_played` | int | Total accumulated play time in (likely) seconds. |
| `bad_passwords` | int or None | Count of failed password attempts since last success / reset. |
| `quit_reason` | QuitReason or None | Reason code for last session end. |

### Presentation & Messaging

| Field | Type | Description |
|-------|------|-------------|
| `prompt` | str | Player's active prompt template. |
| `description` | str or None | Long-form in-game character description. |
| `title` | str or None | Clan-specific or global title (see also `current_title`). |
| `current_title` | str or None | Title currently displayed (may reflect promotion or dynamic title). |
| `wiz_title` | str or None | Wizard/immortal styled title (staff-only). |
| `poof_in` | str or None | Custom arrival message (builder/staff flair). |
| `poof_out` | str or None | Custom departure message. |
| `page_length` | int or None | Output pagination height (# lines) for page breaks. |
| `log_view` | int or None | Log verbosity / viewing preference setting. |

### Progression & Leveling

| Field | Type | Description |
|-------|------|-------------|
| `level` | int | Current character level. |
| `last_level` | int or None | Previous level (used in level-loss or checkpoint logic). |
| `experience` | int or None | Total accumulated experience points. |
| `freeze_level` | int or None | Staff-imposed level freeze threshold (no advancement beyond). |
| `player_class` | Class or None | Primary class selection. |
| `races` | Race or None | Race (single or possibly multi-race encoded). |
| `life_force` | LifeForce or None | Life force classification. |
| `composition` | Composition or None | Physical composition (affects damage types / spells). |
| `skills` | dict[str,int] or None | Learned skills/spells with proficiency percentages. |
| `spell_casts` | dict[str,int] or None | Counters for spell casts (usage stats / progression). |
| `cooldowns` | dict[Cooldown, dict[str,str]] or None | Structured active cooldown timers keyed by category. |
| `trophy` | list[dict[str,int]] or None | Kill trophy / achievement tracking (mob id => count). |


### Physical & Biological

| Field | Type | Description |
|-------|------|-------------|
| `height` | int or None | Current (possibly modified) height. |
| `weight` | int or None | Current weight. |
| `base_height` | int or None | Baseline (unmodified) height. |
| `base_weight` | int or None | Baseline weight. |
| `base_size` | int or None | Baseline size category. |
| `natural_size` | int or None | Natural size (pre-morph / polymorph). |
| `size` | int or None | Current size (active). |
| `alignment` | int or None | Moral/ethical alignment scale (e.g., -1000 evil to +1000 good). |
| `gender` | Gender or None | Gender identity. |
| `hunger` | int or None | Hunger meter (0=satiated or inverse depending on convention). |
| `thirst` | int or None | Thirst meter. |
| `drunkenness` | int or None | Intoxication level. |
| `wimpy` | int or None | Auto-flee threshold (HP percentage or absolute HP). |
| `auto_invis` | int or None | Auto invisibility trigger / preference bit. |

### Location & Persistence

| Field | Type | Description |
|-------|------|-------------|
| `home` | str | Home location (may be a room tag / starting area). |
| `load_room` | int or None | Room vnum to load into on login (overrides default). |
| `save_room` | int or None | Last saved room (for crash / linkdead recovery). |
| `olc_zones` | list[int] or None | Builder-editable zone identifiers. |
| `clan` | int or None | Clan identifier (numeric id). |

### Combat & Survivability

| Field | Type | Description |
|-------|------|-------------|
| `hit_points` | int or None | Current (or saved) hit points. |
| `move` | int or None | Current movement points (stamina). |
| `damage_roll` | int or None | Damage roll modifier. |
| `hit_roll` | int or None | Hit roll / accuracy modifier. |
| `armor_class` | int or None | Armor Class (lower usually better in many MUD systems). |
| `aggression` | int or None | Aggression / auto-attack tendency metric. |
| `saving_throws` | SavingThrows or None | Resistances / saves vs. magical or status effects. |
| `stats` | Stats or None | Core stats block. |

### Economy

| Field | Type | Description |
|-------|------|-------------|
| `money` | Money or None | Carried currency. |
| `bank` | Money or None | Banked currency. |

### Status & Effects

| Field | Type | Description |
|-------|------|-------------|
| `effects` | list[dict] or None | Active effect instances (buffs/debuffs; structure is legacy). |
| `effect_flags` | list[str] or None | Flag list summarizing active effects (derived quick checks). |
| `player_flags` | list[str] or None | Persistent character state flags (e.g., PK, AFK, Deaf). |
| `preference_flags` | list[str] or None | Player-configured preferences (color on/off, auto-loot). |
| `privilege_flags` | list[str] or None | Elevated ability flags (e.g., build, wiz commands). |
| `grant_groups` | list[dict[str,str]] or None | Group-based permission grants. |
| `grants` | list[dict[str,str]] or None | Individual permission overrides. |
| `invis_level` | int or None | Staff invisibility level (0=visible). |

### Communication & Social History

| Field | Type | Description |
|-------|------|-------------|
| `tells` | list[str] or None | Recent private tells (history buffer). |
| `gossips` | list[str] or None | Recent gossip channel messages (history buffer). |
| `aliases` | dict[str,str] or None | Command alias mappings. |

### Miscellaneous / Engine

| Field | Type | Description |
|-------|------|-------------|
| `KILL_TYPE_FLAGS` | list[str] | Static mapping of kill types (index-based) for legacy parsing. |
|

## Parsing & Data Flow

1. `MudData` provides raw tokenized sections from legacy player file.
2. `Parser(MudTypes.PLAYER)` selects a ruleset for translating tokens into Python-native structures.
3. `parser.parse(player_file)` returns a dict mapping to dataclass field names.
4. `Player(**data)` constructs the dataclass instance.

Downstream systems (conversion scripts, JSON exporters, analytics) should treat absent optional values as semantically "unknown / not yet initialized" rather than zero unless the field domain defines a natural zero (e.g., `experience=0`).

## Reserved / Potential Refactors

| Concern | Note |
|---------|------|
| Duplicate `title` field comment | Present once with different contextual meaning (clan). Unify if possible. |
| `effects` schema | Raw dicts reduce type safety; consider structured dataclass. |
| `cooldowns` nested dict[str,str] | Should likely become a typed model with expiry timestamps. |
| `trophy` structure | Clarify meaning of dict keys (mob id?) and counts; maybe map[int,int]. |
| Alignment scale | Define explicit constant bounds for clarity. |
| Flags as `list[str]` | Replace with `Enum` + `EnumSet` or bitset for validation & storage. |

## Usage Guidelines

- Treat all collections as owned by the `Player` instance; copy before mutating in shared contexts.
- When generating outward-facing JSON, omit `password` and any sensitive administrative flags unless explicitly required for an internal tool.
- Enforce validation on: `level >= 1`, non-negative currency, and alignment bounds.
- Normalize absent numeric fields to domain defaults at load time if required by downstream combat logic (e.g., set `hit_points` to max if `None`).

## Security Considerations

- `password` must already be hashed; never re-emit in logs.
- IP address (`host`) may be considered PII; restrict export.
- Permission / privilege flags must be validated against an allowlist before granting runtime capabilities.

## Future Modernization Ideas

- Replace free-form flag lists with strongly-typed enums + bit masks or sets.
- Introduce a `Permissions` aggregate object encapsulating grants / groups / privilege flags.
- Convert effect dictionaries into a `StatusEffect` dataclass with: id, source, modifier, duration, stacks.
- Provide a serialization schema version tag to support migrations.

## Quick Field Count

Total declared data attributes (excluding class constants & method): 70+ (subject to consolidation during refactor).

## Contact

For questions about extending the player schema, see `docs/ARCHITECTURE_ANALYSIS.md` and open a design discussion before adding new top-level fields.

# Spells data model (no mana; circle-based)

This document defines a data-driven schema for the magic system (see `src/spells.cpp`) with no mana. Spells are gated by class circles and controlled by cooldowns, cast times, components, and restrictions. Effects are unified into a single JSONB-backed table for Postgres.

## Goals

- Declarative spells (targets, effects, saves, messages)
- Support damage/heal, buffs/debuffs, teleport, summon, object/room effects
- Multi-effect spells with explicit order
- Circles per class replace mana costs
- Escape hatch for special/hard-coded logic

## Core concepts

- Spell: Named action with targeting rules, optional save(s), one or more effects, messages, and restrictions.
- Circle gating: Each class learns a spell at a given circle (1..N). Circles govern availability (no mana).
- Effects: A spell may have multiple effects that resolve in order.
- Formulas: Simple expressions using variables like `level`, `circle`, `caster_level`.

## Tables (overview)

The schema uses a core spell row, one targeting row, optional saving throw(s), messages, restrictions, and a unified `spell_effects` table with JSONB parameters.

### spells

- id (PK)
- name (unique)
- school_id (FK -> spell_schools) optional
- min_position (enum: sleeping, resting, standing, fighting)
- violent (bool) [affects safe-room/combat rules]
- cast_time_rounds (int)
- cooldown_ms (int)
- in_combat_only (bool)
- is_area (bool)
- notes (text)

### spell_schools (optional)

- id, name (e.g., evocation, necromancy)

### classes and circles

- classes: id, name (cleric, mage, ranger, rogue, etc.)
- spell_class_circles:
  - id (PK), spell_id (FK), class_id (FK)
  - circle (int) [1..N]
  - min_level (int) optional
  - proficiency_gain (int) optional

### spell_targeting

- id (PK), spell_id (FK)
- allowed_targets_mask (bitmask): self|ally|enemy|npc|pc|object|room|corpse
- target_scope (enum): single|room|group|area|chain|cone|line
- max_targets (int)
- range (enum): self|touch|room|adjacent_room|world
- require_los (bool)
- filters_mask (bitmask): undead_only|living_only|summoned|same_group|...

### spell_saving_throws

- id (PK), spell_id (FK)
- save_type (enum): spell|poison|breath|paralysis|wand
- on_save (enum): none|half|negate|reduce25|custom
- dc_formula (text) e.g., `10 + caster_level/2`
- save_modifier_vs_mask (bitmask) optional

### spell_messages

- id (PK), spell_id (FK)
- start_to_caster (text)
- start_to_victim (text)
- start_to_room (text)
- success_to_caster (text)
- success_to_victim (text)
- success_to_room (text)
- fail_to_caster (text)
- fail_to_victim (text)
- fail_to_room (text)
- wearoff_to_target (text)
- wearoff_to_room (text) optional

### spell_components (optional)

- id (PK), spell_id (FK)
- object_vnum (int)
- consumed (bool)
- required (bool)

### spell_restrictions (optional)

- id (PK), spell_id (FK)
- indoors_only (bool), outdoors_only (bool)
- no_safe_rooms (bool)
- no_teleport_flags_mask (bitmask)
- terrain_mask (bitmask)
- disallow_states_mask (bitmask)

## Unified effects (JSONB)

A spell has N effects in `spell_effects`. Each row declares a type and provides type-specific parameters in a JSONB `params` column. This replaces per-type tables.

### spell_effects

- id (PK), spell_id (FK)
- effect_type (enum): damage|heal|stat_mod|affect_flag|dispel|teleport|summon|create_object|resource|obj_affect|room_affect|cleanse|remove_curse|script
- order (int)
- chance_pct (int)
- trigger (enum): on_cast|on_hit|on_kill|on_save|on_fail optional
- duration_formula (text)
- stacking_rule (enum): refresh|stack|ignore|max_only
- condition_filter (jsonb)
- params (jsonb)

Recommended Postgres constraints:

- CHECK (effect_type IN ('damage','heal','stat_mod','affect_flag','dispel','teleport','summon','create_object','resource','obj_affect','room_affect','cleanse','remove_curse','script'))
- CHECK (order >= 0)
- CHECK (chance_pct BETWEEN 0 AND 100)
- CHECK (effect_type <> 'damage' OR (params ? 'dice' AND params ? 'damage_type_id'))
- CHECK (effect_type <> 'heal' OR (params ? 'dice'))
- CHECK (effect_type <> 'affect_flag' OR (params ? 'flags_mask'))

Indexes (Postgres):

- CREATE INDEX ON spell_effects (spell_id, order);
- CREATE INDEX spell_effects_params_gin ON spell_effects USING GIN (params);
- Optional: CREATE INDEX spell_effects_condition_gin ON spell_effects USING GIN (condition_filter);

Params examples (JSONB):

- damage: { "damage_type_id": 1, "dice": "6d6", "bonus": "level", "cap": "12d6", "splash_pct": 0 }
- heal: { "dice": "1d8", "bonus": "level/2", "remove_bleed": true }
- stat_mod: { "location": "hitroll", "modifier": "+(level/5)" }
- affect_flag: { "flags_mask": "AFF_INVIS|AFF_DETECT_INVIS", "duration": "(level/2)+10" }
- dispel: { "remove_flags_mask": "AFF_CURSE|AFF_POISON", "max_spell_level": 4 }
- teleport: { "destination_type": "recall", "destination_arg": null }
- summon: { "mob_id": 1234, "count": 2, "charm": false }
- create_object: { "obj_id": 5678, "count": 1 }
- resource: { "resource": "lifeforce", "amount": "-10" }
- obj_affect: { "apply_flags_mask": "ENCHANT|BLESS", "duration": "level" }
- room_affect: { "room_flags_mask": "DARKNESS", "duration": "10" }
- script: { "script_ref": "spell_specials::weird_spell", "payload": { "key": "value" } }

## Enums/aux

- damage_types: id, name (fire, cold, acid, lightning, holy, untyped, etc.)
- positions: id, name
- target_flags: id, name
- terrain_types: id, name

## Formulas

String expressions evaluated by the engine. Variables available: `level`, `caster_level`, `victim_level`, `circle`, `intmod`, `wismod`, `strmod`, `skill`, `room_level`. Supported ops: +, -, *, /, floor, min, max, clamp. Dice strings like `XdY` are permitted in `dice` fields.

## Examples

### Fireball

- spells: name=Fireball, school=Evocation, violent=true, cast_time_rounds=1, cooldown_ms=6000
- targeting: enemy, scope=room (AoE), range=room
- save: type=spell, on_save=half, dc="10 + caster_level/2"
- effects: damage params above; optional script DoT 20%
- gating: spell_class_circles: mage circle=3 (example)

### Cure Light Wounds

- targeting: ally|self, scope=single, range=touch|room
- effects: heal dice="1d8", bonus="level/2"
- gating: cleric circle=1 (example)

### Teleport

- targeting: self|ally, scope=single
- restrictions: no_safe_rooms=true, no_teleport_flags_mask includes NO_RECALL
- effects: teleport destination_type=recall
- save: none
- gating: mage circle=4 (example)

### Detect Invisibility

- targeting: self|ally
- effects: affect_flag flags=DETECT_INVIS, duration="(level/2)+10"
- stacking: refresh
- gating: mage circle=1 (example)

## Mapping from current codebase

- Map target bitvectors to `spell_targeting.allowed_targets_mask`.
- Set `spells.violent`, `min_position`, `cast_time_rounds`, `cooldown_ms` from current behavior.
- Derive circles from per-class spell lists; populate `spell_class_circles`.
- Map affects to flags masks; damage types and saves to existing enums.
- Copy cast/wear-off strings into `spell_messages`.
- Use `effect_type=script` for hard-coded specials initially; refactor later.

## Execution pipeline

1) Validate circle/class access and context
2) Resolve target(s) from `spell_targeting` and filters
3) Compute save(s) and outcomes
4) Apply effects in `order` with `chance_pct`, `trigger`, `stacking_rule`
5) Emit messages (start/success/fail/wearoff)
6) Start cooldowns and durations

## Edge cases

- Resist/immunity handled in damage resolution; add per-spell overrides later if needed
- Stacking controlled per-effect via `stacking_rule`
- Room/object targets supported via masks and params
- Peaceful rooms: `spell_restrictions.no_safe_rooms` + `spells.violent`

## Migration plan

1) Create tables and seed enums
2) Populate `classes` and `spell_class_circles`
3) Migrate simple spells (damage/heal/buffs)
4) Add AoE/teleport/summon/object/room effects
5) Map remaining specials to `script` effects

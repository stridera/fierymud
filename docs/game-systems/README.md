# FieryMUD Game Systems Documentation

This directory contains comprehensive documentation of FieryMUD's game mechanics and systems. These documents are essential references for implementing and maintaining the game.

## Core Game Mechanics

- **[CORE_MECHANICS.md](CORE_MECHANICS.md)** - Fundamental game systems
  - APPLY locations (stat modifiers)
  - Damage type system
  - Saving throw mechanics
  - Combat system
  - Status effects implementation
  - Time and duration system
  - Effect stacking rules

- **[EFFECT_FLAGS_REFERENCE.md](EFFECT_FLAGS_REFERENCE.md)** - Complete reference for all effect flags (AFF_*)

## Magic & Spell Systems

- **[SPELL_SYSTEM.md](SPELL_SYSTEM.md)** - Complete spell system reference (251 spells)
  - Spell database and metadata
  - Magic routines (MAG_* handlers)
  - Sphere system (11 magical spheres)
  - Targeting system
  - Cast time mechanics
  - Effect system

- **[ALIGNMENT_DAMAGE_EXAMPLES.md](ALIGNMENT_DAMAGE_EXAMPLES.md)** - Alignment-based damage mechanics
- **[SPELL_PARAMS_SCHEMA.md](SPELL_PARAMS_SCHEMA.md)** - Spell parameter definitions
- **[MAGIC_SYSTEM_COMPLETENESS.md](MAGIC_SYSTEM_COMPLETENESS.md)** - Magic system implementation status

### Spell System Design

- **[SPELL_CONFIGURABILITY.md](SPELL_CONFIGURABILITY.md)** - Data-driven spell configuration design
- **[SPELL_SYSTEM_IMPROVEMENTS.md](SPELL_SYSTEM_IMPROVEMENTS.md)** - Proposed spell system enhancements

## Skill Systems

- **[SKILL_SYSTEM.md](SKILL_SYSTEM.md)** - Complete skill system reference
  - Skill mechanics
  - Proficiency system
  - Skill categories
  - Class-specific skills

### Skill System Design

- **[SKILL_CONFIGURABILITY.md](SKILL_CONFIGURABILITY.md)** - Data-driven skill configuration design
- **[SKILL_SYSTEM_IMPROVEMENTS.md](SKILL_SYSTEM_IMPROVEMENTS.md)** - Proposed skill system enhancements

## Ability System

- **[PROPOSED_ABILITY_SYSTEM.md](PROPOSED_ABILITY_SYSTEM.md)** - Future ability system design

> **Note**: Previous ability extraction files were removed due to incorrect damage type mappings.
> For authoritative ability data, see [fierylib/docs/extraction-reports/abilities.csv](../../fierylib/docs/extraction-reports/abilities.csv)

## Related Documentation

For legacy data extraction and import documentation, see:
- [fierylib/docs/extraction-reports/](../../fierylib/docs/extraction-reports/) - Spell/skill extraction from legacy code
- [fierymud/docs/MOB_DATA_MAPPING.md](../MOB_DATA_MAPPING.md) - Mob stat conversion and combat formulas

## Usage

These documents are maintained alongside the FieryMUD codebase and should be updated when:
- Game mechanics are modified or enhanced
- New spells, skills, or abilities are added
- Combat formulas or damage calculations change
- Effect flags or status effects are added/modified

**Status**: Most documents reflect the legacy CircleMUD-based system. As FieryMUD is modernized, these docs should be updated to reflect the new data-driven approaches.

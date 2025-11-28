# FieryMUD Documentation

**Complete documentation for FieryMUD C++ MUD server**

---

## 📚 Documentation Structure

### `/data-formats` - World Data File Formats

Documentation for world data file formats and serialization.

- **`MUD_FILE_FORMATS.md`** - Legacy CircleMUD text file formats (.wld, .mob, .obj, .zon, .shp)
- **`WORLD_JSON_FORMAT.md`** - Modern JSON world file format specification

### `/development` - Development Guides

Development workflow, task tracking, and coding standards.

- **`PLAN.md`** - FieryMUD modernization roadmap (legacy → C++23)
- **`TASKS.md`** - Current development task list (high-level)
- **`task.md`** - Detailed task tracking and status updates
- **`TESTING_ROADMAP.md`** - Testing strategy and test framework migration
- **`TEST_PLAN.md`** - Comprehensive test plan for all systems
- **`rules.md`** - Development safety rules and best practices

### `/game-systems` - Game Mechanics Documentation

Core game system documentation and enhancement proposals.

- **`ABILITIES.md`** - Complete ability system reference (spells and skills)
- **`ABILITY_EFFECTS_ENHANCEMENT.md`** - Proposed ability system improvements
- **`EFFECT_SYSTEM_SUMMARY.md`** - Effect system architecture and usage
- **`EFFECT_TYPES_QUICK_REFERENCE.md`** - Quick reference for all effect types
- **`CLAN_SCRIPTING.md`** - Clan scripting system documentation
- **`PLAYER.md`** - Player data structure and character systems
- **`DESIGN.md`** - Web interface (Muditor) system design

### `/legacy-analysis` - Legacy System Analysis

Reverse-engineering and analysis of legacy CircleMUD systems.

- **`LEGACY_MESSAGING_PATTERNS.md`** - Communication and messaging patterns
- **`LEGACY_SPELLS.md`** - Legacy spell system analysis
- **`MOB_HP_FORMULA.md`** - Mob hit point calculation formulas
- **`MOB_STAT_CALCULATION.md`** - Mob stat generation algorithms
- **`RACE_CLASS_FACTORS.md`** - Race and class modifiers for mobs
- **`SPELL_ABILITY_FORMATTING.md`** - Spell/ability message formatting

### `/migration` - Combat System Migration ✨ NEW

**Complete combat system reverse-engineering and modernization documentation.**

- **`COMBAT_REVERSE_ENGINEERING.md`** (59KB) - Complete legacy combat system analysis
  - THAC0/AC hit calculation formulas
  - Damage calculation (player vs mob differences)
  - Mob damage algorithms (`get_set_hd`, `get_set_dice`)
  - Defensive mechanics (riposte, parry, dodge, susceptibility)
  - Modern ACC/EVA/AR/DR% mapping proposal
  - Calibration reports (5×5 player/mob matrix)
  - L50+ pathology analysis (5% hit rate demonstration)
  - Migration worksheets and SQL scripts

- **`COMBAT_CLARIFICATIONS.md`** (36KB) - Edge cases and final specifications
  - **Complete glossary**: ACC, EVA, AP, SP, PenFlat, PenPct, AR, DR%, Soak, Hardness, Ward%, RES[type]%
  - Critical hit mechanics (literal value 20, 0.5% chance)
  - Auto-hit/miss precedence chain (Immunity > Displacement > To-Hit > Post-Hit)
  - Post-hit defense bypass list (8 skills)
  - Susceptibility stacking rules (multiplicative)
  - AC→AR extraction formula
  - Magic AC → Ward% routing
  - Global mitigation cap (75%) and chip floor (1 damage)
  - Damage type routing (Physical/Elemental/Magic)
  - All caps codified (Block 50%, Parry 40%, Dodge 30%, PenPct 50%, PenFlat Soak+10)
  - K constants (50/100/200 by tier, recomputable)
  - Stoneskin special case handling
  - Content author guidelines

- **`COMBAT_SNAPSHOT.md`** (14KB) - Current combat data snapshots
  - Player class stat tables (7 classes)
  - Stat bonus formulas (STR, DEX, CON, INT, WIS)
  - Mob examples from zone 30
  - Current damage calculations
  - Recommendations for new system

### `/reference` - Data Reference Files

Extracted metadata and reference data from the codebase.

**Markdown Documentation**:
- **`SKILL_ANALYSIS_README.md`** - Skill system analysis overview
- **`SKILL_SYSTEM_ANALYSIS.md`** - Detailed skill implementation analysis
- **`skill_summary.md`** - Skill summary and statistics
- **`spell_metadata_summary.md`** - Spell metadata summary

**JSON Metadata** (extracted from codebase):
- **`skill_metadata.json`** (43KB) - All skill definitions and properties
- **`skill_metadata_comprehensive.json`** (53KB) - Extended skill metadata
- **`spell_metadata.json`** (127KB) - All spell definitions and properties
- **`skill_analysis_comprehensive.json`** (3.2KB) - Skill analysis results
- **`skill_implementation_analysis.json`** (15KB) - Skill implementation status

**CSV Data**:
- **`abilities.csv`** - Processed ability data
- **`abilities_original.csv`** - Original ability data export

**TypeScript Interfaces**:
- **`conceptual_spell_idl.ts`** - Spell interface definitions

### Root Documentation

- **`ARCHITECTURE_ANALYSIS.md`** - High-level codebase architecture analysis and modernization assessment

---

## 🎯 Quick Navigation

### I want to...

**Understand the combat system**:
→ Start with `/migration/COMBAT_SNAPSHOT.md`
→ Then read `/migration/COMBAT_REVERSE_ENGINEERING.md`
→ Reference `/migration/COMBAT_CLARIFICATIONS.md` for edge cases

**Understand mob stat generation**:
→ `/legacy-analysis/MOB_STAT_CALCULATION.md`
→ `/legacy-analysis/MOB_HP_FORMULA.md`
→ `/legacy-analysis/RACE_CLASS_FACTORS.md`

**Work with world data**:
→ `/data-formats/WORLD_JSON_FORMAT.md` (modern)
→ `/data-formats/MUD_FILE_FORMATS.md` (legacy)

**Understand the ability system**:
→ `/game-systems/ABILITIES.md`
→ `/game-systems/EFFECT_SYSTEM_SUMMARY.md`
→ `/reference/spell_metadata.json`

**Start development**:
→ `/development/PLAN.md` (roadmap)
→ `/development/rules.md` (safety rules)
→ `/development/TESTING_ROADMAP.md` (testing strategy)

**Review the architecture**:
→ `ARCHITECTURE_ANALYSIS.md` (root)

---

## 📊 Documentation Statistics

- **Total Docs**: 47 files
- **Total Size**: ~800KB
- **Categories**: 7 (data-formats, development, game-systems, legacy-analysis, migration, reference, root)
- **Combat System**: 3 comprehensive documents (109KB)
- **JSON Metadata**: 5 files (241KB extracted game data)
- **CSV Data**: 2 files (ability exports)

---

## 🔗 Related Projects

- **FieryLib**: Python import tool (`../fierylib/`) - imports legacy data to PostgreSQL
- **Muditor**: Web editor (`../muditor/`) - TypeScript/React visual world editor
- **Parent CLAUDE.md**: `../CLAUDE.md` - Project integration documentation

---

## 📝 Documentation Standards

### File Naming

- **ALL_CAPS.md**: Major system documentation
- **lowercase.md**: Utility/reference documentation
- **PascalCase.json**: Metadata exports
- **lowercase.csv**: Data exports

### Organization Principles

1. **Separation of Concerns**: Development vs. game systems vs. legacy analysis
2. **Progressive Disclosure**: Snapshots → detailed analysis → edge cases
3. **Findability**: Clear directory names, comprehensive README
4. **Maintainability**: Remove redundancy, consolidate related docs

### Content Guidelines

- Include code references with file:line citations
- Provide examples for all formulas
- Document edge cases and exceptions
- Add migration/implementation checklists
- Cross-reference related documents

---

**Last Updated**: 2025-11-28
**Maintained By**: FieryMUD Development Team

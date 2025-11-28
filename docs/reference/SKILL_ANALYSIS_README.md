# FieryMUD Skill System Analysis - Quick Reference

## Overview

Complete extraction and analysis of the FieryMUD skill system from legacy C++ source code.

**Analysis Date**: 2025-01-06
**Total Skills Analyzed**: 117 (89 skills + 10 songs + 18 chants)
**Implementation Coverage**: 54/117 (46.2%)
**Class Assignment Coverage**: 56/117 (47.9%)

---

## Generated Files

### 1. SKILL_SYSTEM_ANALYSIS.md (21 KB)
**Comprehensive human-readable report**

**Contents**:
- Executive summary with key statistics
- Complete skill categorization (combat, movement, stealth, etc.)
- Implementation analysis (ACMD vs switch-case patterns)
- Class assignment breakdown
- Skill properties (targeting, violence, humanoid restrictions)
- System architecture documentation
- Modernization recommendations
- Complete skill listing by ID

**Use When**: You need to understand the overall skill system design, find specific skills, or plan modernization efforts.

### 2. skill_metadata_comprehensive.json (53 KB)
**Complete machine-readable skill database**

**Structure** (per skill):
```json
{
  "SKILL_NAME": {
    "id": 401,
    "name": "backstab",
    "type": "skill",
    "humanoid": true,
    "targets": "TAR_CONTACT",
    "wearoff": null,
    "fighting_ok": true,
    "violent": true,
    "class_levels": {
      "ASSASSIN": 1,
      "ROGUE": 1,
      "THIEF": 1
    },
    "implementations": [
      {
        "file": "act.offensive.cpp",
        "type": "ACMD",
        "function": "do_backstab",
        "line": 581
      }
    ],
    "categories": ["stealth"]
  }
}
```

**Use When**: Building tools, importing to database, programmatic analysis.

### 3. skill_analysis_comprehensive.json (3.2 KB)
**High-level analysis summary**

**Contents**:
- Categories with skill names
- Implementation pattern counts
- Class coverage statistics

**Use When**: Quick overview, dashboard generation, comparison analysis.

### 4. skill_metadata.json (43 KB)
**Basic skill metadata (original extraction)**

Similar to comprehensive version but without class assignments. Kept for reference.

### 5. skill_implementation_analysis.json (15 KB)
**Implementation pattern deep-dive**

Detailed breakdown of where and how each skill is implemented in source code.

### 6. skill_summary.md (3.5 KB)
**Categorized skill list (original extraction)**

Quick reference list organized by category. Simpler than comprehensive analysis.

---

## Key Findings

### Skills by Category
- **Combat**: 37 skills (31.6%) - Attacks, defense, special maneuvers
- **Weapon Proficiencies**: 9 skills (7.7%) - Weapon type expertise
- **Movement**: 11 skills (9.4%) - Travel, positioning, retreat
- **Stealth**: 8 skills (6.8%) - Covert operations
- **Passive**: 3 skills (2.6%) - Always-active abilities
- **Utility**: 9 skills (7.7%) - Miscellaneous non-combat
- **Spell Spheres**: 12 skills (10.3%) - Caster meta-skills
- **Bard Songs**: 10 songs (8.5%) - Musical abilities
- **Monk Chants**: 18 chants (15.4%) - Spiritual abilities

### Implementation Patterns
- **ACMD-based**: 47 skills (40.2%) - Standard command implementation
- **Switch-case**: 9 skills (7.7%) - Shared with spell system
- **Unimplemented**: 63 skills (53.8%) - Defined but no clear implementation

### Top Classes by Skill Count
1. Anti-Paladin: 22 skills
2. Mercenary: 22 skills
3. Berserker: 22 skills
4. Paladin: 21 skills
5. Warrior: 19 skills

---

## Common Use Cases

### Find all skills for a specific class
```bash
# Using jq
jq 'to_entries[] | select(.value.class_levels.ROGUE != null) | {name: .value.name, level: .value.class_levels.ROGUE}' skill_metadata_comprehensive.json
```

### Find implementation location for a skill
```bash
# Using jq
jq '.SKILL_BACKSTAB.implementations' skill_metadata_comprehensive.json
```

### Count skills by violence flag
```bash
# Using jq
jq '[.[] | select(.violent == true)] | length' skill_metadata_comprehensive.json
```

### Export to CSV
```bash
# Using jq
jq -r '.[] | [.id, .name, .type, .violent, .humanoid] | @csv' skill_metadata_comprehensive.json > skills.csv
```

---

## Extraction Scripts

### scripts/extract_skill_metadata.py
**Original extraction script**

Generates:
- skill_metadata.json
- skill_implementation_analysis.json
- skill_summary.md

### scripts/extract_comprehensive_skill_data.py
**Enhanced extraction with class assignments**

Generates:
- skill_metadata_comprehensive.json
- skill_analysis_comprehensive.json

**Run**: `python3 scripts/extract_comprehensive_skill_data.py`

---

## Skill ID Ranges

- **401-495**: Skills (combat, utility, movement, stealth)
- **551-560**: Bard Songs
- **601-618**: Monk/Berserker Chants

**Note**: Some IDs in the 401-495 range are unused (e.g., 425, 470).

---

## Source Files Analyzed

### Core Skill System
- `legacy/src/defines.hpp` - Skill ID definitions
- `legacy/src/skills.cpp` - Skill metadata (skillo/chanto/songo macros)
- `legacy/src/skills.hpp` - Skill system declarations
- `legacy/src/class.cpp` - Class skill assignments

### Implementation Files
- `legacy/src/act.offensive.cpp` - Combat skills (backstab, bash, kick)
- `legacy/src/act.movement.cpp` - Movement skills (sneak, hide, track)
- `legacy/src/act.other.cpp` - Utility skills (bandage, steal, pick lock)
- `legacy/src/warrior.cpp` - Warrior-specific skills
- `legacy/src/rogue.cpp` - Rogue-specific skills
- `legacy/src/fight.cpp` - Passive combat skills
- `legacy/src/spell_parser.cpp` - Songs and chants

---

## Modernization Priorities

### High Priority
1. **Data-Driven Configuration**: Move skill formulas to JSON/config
2. **Unified Framework**: Standardize skill execution patterns
3. **Database Integration**: Store assignments in PostgreSQL

### Medium Priority
4. **Scripting System**: Lua integration for custom skills
5. **Skill Trees**: Prerequisites and specialization branches
6. **Balance Framework**: Centralized tuning system

### Low Priority
7. **Advanced Features**: Combos, contextual skills, evolution

---

## Related Documentation

- `SPELL_SYSTEM_ANALYSIS.md` - Similar analysis for spell system
- `docs/WORLD_JSON_FORMAT.md` - World data format
- `CLAUDE.md` - Project development guidelines

---

## Questions?

For detailed information on specific aspects:

1. **Skill Categories**: See SKILL_SYSTEM_ANALYSIS.md Section 1
2. **Implementation Details**: See SKILL_SYSTEM_ANALYSIS.md Section 2
3. **Class Assignments**: See SKILL_SYSTEM_ANALYSIS.md Section 3
4. **System Architecture**: See SKILL_SYSTEM_ANALYSIS.md Section 5
5. **Raw Data**: Use skill_metadata_comprehensive.json

---

**Analysis Tools**: Python 3.x, regex parsing, static code analysis
**Confidence Level**: High (extracted directly from source code)
**Known Limitations**: Help text extraction incomplete (no help files found in lib/)

# ABILITIES_COMPLETE.md - Implementation Format Examples

This document shows examples of how different types of ability implementations are formatted in ABILITIES_COMPLETE.md.

---

## Damage Spell Example

**Harm** (ID 27) - Shows damage formula and type

```markdown
**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `(pow(skill,2)*41)/5000`
```

---

## Buff Spell with Duration & Effects

**Bless** (ID 3) - Shows duration, effects, requirements, conflicts

```markdown
**Implementation**:
- **Duration**: `10 + (skill / 7)`
- **Effects**:
  - **APPLY_SAVING_SPELL**: `-2 - (skill / 10)`
  - **APPLY_DAMROLL**: `1 + (skill > 95)`
  - **EFF_BLESS**
- **Special Mechanics**:
  - Conflicts with other blessing spells (check_bless_spells)
- **Requirements**: Alignment: Cannot target evil, Alignment: Caster cannot be evil
- **Messages**:
  - **to_char**: "$N is inspired by your gods."
  - **to_room**: "$n is inspired to do good."
  - **to_vict**: "Your inner angel is inspired.\r\nYou feel righteous."
- **Source**: magic.cpp:1212-1257
```

---

## Debuff Spell with Duration

**Chill Touch** (ID 8) - Shows duration and requirements

```markdown
**Implementation**:
- **Duration**: `3 + (skill / 20)`
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "$N is withered by your cold!"
  - **to_room**: "$N withers slightly from $n's cold!"
  - **to_vict**: "You feel your strength wither!"
- **Source**: magic.cpp:1350-1367
```

---

## Skill with Mechanics

**Backstab** (ID 401) - Shows skill mechanics and requirements

```markdown
**Implementation**:
- **Mechanics**: Attack from behind for massive damage
- **Requirements**: piercing weapon, target not fighting, hidden or behind
```

---

## Mind Control Spell with Complex Mechanics

**Charm Person** (ID 7) - Shows comprehensive mechanics

```markdown
**Implementation**:
- **Mechanics**: Charms victim to follow and obey caster
- **Duration**: skill / 5 hours
- **Requirements**: Caster cannot be charmed (unless animated)
- **Immunities**: MOB_NOCHARM, EFF_SANCTUARY, SUMMON protection
- **Max Followers**: Limited by CHA
- **Special**: Circle 1-6 max, victim level vs caster check
- **Source**: spells.cpp:221-299
```

---

## Buff with Effects and Messages

**Waterwalk** (ID 51) - Shows duration, effect flag, and messages

```markdown
**Implementation**:
- **Duration**: `35 + (skill / 4)`
- **Effects**:
  - **EFF_WATERWALK**
- **Messages**:
  - **to_room**: "$N sprouts webbing between $S toes!"
  - **to_vict**: "You feel webbing between your toes."
- **Source**: magic.cpp:2928-2934
```

---

## Protective Spell with Special Mechanics

**Stone Skin** (ID 52) - Shows duration, effects, special mechanics

```markdown
**Implementation**:
- **Duration**: `2`
- **Effects**:
  - **APPLY_NONE**: `7 + (skill / 16)`
  - **EFF_STONE_SKIN**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_char**: "&9&b$N's skin hardens and turns to stone!&0"
  - **to_room**: "&9&b$N's skin hardens and turns to stone!&0"
  - **to_vict**: "&9&bYour skin hardens and turns to stone!&0"
- **Source**: magic.cpp:2846-2862
```

---

## Chant with Duration and Messages

**Aria of Dissonance** (ID 607) - Chant-type ability

```markdown
**Implementation**:
- **Duration**: `5 + (skill / 30)`
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "Your song of dissonance confuses $N!"
  - **to_room**: "$N winces as $n's dissonant song fills $S ears."
  - **to_vict**: "$n fills your ears with an aria of dissonance, causing confusion!"
- **Source**: magic.cpp:3057-3068
```

---

## Song Ability

**Hearthsong** (ID 554) - Song-type ability

```markdown
**Implementation**:
- **Mechanics**: Song of home and comfort
- **Source**: defines.hpp:554
```

---

## Utility Spell

**Locate Object** (ID 31) - Simple utility spell

```markdown
**Implementation**:
- **Mechanics**: Locates specific object in world
- **Source**: spells.cpp
```

---

## Summary of Implementation Fields

Based on ability type and characteristics, entries may include:

### Common Fields (Most Abilities)
- **Source**: Source code file and line numbers

### Duration-Based Abilities (Buffs/Debuffs)
- **Duration**: Formula calculating how long effect lasts
- **Effects**: List of APPLY_* modifiers and EFF_* flags
- **Messages**: to_char, to_room, to_vict flavor text

### Damage Abilities (Offensive Spells/Skills)
- **Damage Type**: DAM_FIRE, DAM_COLD, etc.
- **Damage Formula**: Mathematical formula or function name

### Skill Abilities
- **Mechanics**: Description of how the skill works
- **Requirements**: Weapon types, conditions, etc.

### Special Mechanics
- **Special Mechanics**: Additional notes about behavior
- **Requirements**: Alignment, target restrictions, etc.
- **Conflicts**: Other spells that conflict
- **Immunities**: What makes targets immune
- **Max Followers**: For charm/summon spells

### Other Fields
- **Healing Amount**: For healing spells
- **Save Type**: SAVING_SPELL, SAVING_ROD, etc.

---

## Format Notes

1. **Formulas**: Always enclosed in backticks: `` `10 + (skill / 7)` ``
2. **Effect Modifiers**: Format is `APPLY_CONSTANT`: `` `formula` ``
3. **Effect Flags**: Just the flag name: `EFF_BLESS`
4. **Messages**: Use exact text including color codes
5. **Duration**: Cleaned up (whitespace normalized)
6. **Source**: File:line-range format when available

---

## Using ABILITIES_COMPLETE.md

### Quick Searches

Find all damage spells:
```bash
grep -A5 "**Damage Formula**:" ABILITIES_COMPLETE.md
```

Find all buffs with duration:
```bash
grep -A3 "**Duration**:" ABILITIES_COMPLETE.md
```

Find specific spell:
```bash
grep -A30 "#### Bless" ABILITIES_COMPLETE.md
```

Find by enum:
```bash
grep -A30 "SPELL_HARM" ABILITIES_COMPLETE.md
```

### Statistics

- **Total Abilities**: 368 (251 spells, 89 skills, 10 songs, 18 chants)
- **With Duration Formulas**: 122 (33%)
- **With Damage Formulas**: 106 (29%)
- **With Effects**: 99 (27%)
- **With Mechanics**: 168 (46%)
- **With Source References**: 209 (57%)

---

Generated from ABILITIES_COMPLETE.md (7,441 lines, 172 KB)

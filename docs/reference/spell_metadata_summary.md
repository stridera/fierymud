# FieryMUD Spell Metadata Extraction Summary

## Overview
Successfully extracted comprehensive spell metadata from `/home/strider/Code/mud/fierymud/legacy/src/skills.cpp`.

## Statistics
- **Total Spells Parsed**: 251
- **Spell ID Range**: 1-267 (some IDs are unused)
- **Output File**: `/home/strider/Code/mud/fierymud/spell_metadata.json`

## Extracted Fields per Spell
1. **enumName** - C++ enum constant (e.g., SPELL_ARMOR)
2. **name** - Display name (e.g., "armor")
3. **castTimeSpeed** - Cast speed constant (e.g., CAST_SPEED8)
4. **castTimeRounds** - Rounds to cast (0.5-4.5)
5. **minPosition** - Minimum position required (POS_STANDING, POS_SITTING, etc.)
6. **fightingOk** - Boolean: can cast while fighting
7. **targets** - Array of target flags (TAR_CHAR_ROOM, TAR_FIGHT_VICT, etc.)
8. **violent** - Boolean: is spell violent
9. **routines** - Array of routine flags (MAG_DAMAGE, MAG_AFFECT, etc.)
10. **additionalMemTime** - Integer: extra memorization time
11. **damageType** - Damage type constant (DAM_FIRE, DAM_COLD, etc.)
12. **sphere** - Skill sphere (SKILL_SPHERE_FIRE, SKILL_SPHERE_EARTH, etc.)
13. **pages** - Integer: spellbook pages required
14. **questSpell** - Boolean: is this a quest-only spell
15. **wearOffMessage** - String or null: message when spell effect ends

## Cast Speed to Rounds Mapping
- CAST_SPEED1/2: 0.5 rounds
- CAST_SPEED4: 1.0 rounds
- CAST_SPEED6: 1.5 rounds
- CAST_SPEED8: 2.0 rounds
- CAST_SPEED10: 2.5 rounds
- CAST_SPEED12: 3.0 rounds
- CAST_SPEED14: 3.5 rounds
- CAST_SPEED16: 4.0 rounds
- CAST_SPEED18: 4.5 rounds

## Example Entry
```json
{
  "1": {
    "enumName": "SPELL_ARMOR",
    "name": "armor",
    "castTimeSpeed": "CAST_SPEED8",
    "castTimeRounds": 2.0,
    "minPosition": "POS_STANDING",
    "fightingOk": true,
    "targets": ["TAR_CHAR_ROOM"],
    "violent": false,
    "routines": ["MAG_AFFECT"],
    "additionalMemTime": 0,
    "damageType": "DAM_UNDEFINED",
    "sphere": "SKILL_SPHERE_PROT",
    "pages": 5,
    "questSpell": false,
    "wearOffMessage": "You feel less protected."
  }
}
```

## Sphere Distribution

## Damage Type Distribution

## Quest Spells

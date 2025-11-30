# FieryMUD Spell System Documentation

## Overview

The FieryMUD spell system is a comprehensive magic framework inherited from CircleMUD with extensive FieryMUD-specific enhancements. The system manages 251 fully defined spells across 11 magical spheres, supporting diverse targeting modes, casting mechanics, and effect application patterns.

**Key Components:**
- **Spell Database**: 251 spells with metadata (cast time, targeting, damage type, sphere)
- **Magic Routines**: 13 distinct MAG_* handler types for spell effects
- **Sphere System**: 11 magical spheres organizing spells by theme and access
- **Targeting System**: Flexible TAR_* flag combinations for spell targets
- **Cast Time Mechanics**: Variable casting speed (CAST_SPEED0-12) with Quick Chant interaction
- **Effect System**: Temporary and permanent effects via affect system

**System Statistics:**
- Total Spells: 251
- Violent Spells: 89 (35%)
- Quest-Only Spells: 19 (8%)
- MAG_MANUAL Implementations: 63 (25%)
- Average Casting Time: 1.75 rounds

## Complete Spell Reference Table

| ID | Spell Name | Cast Time | Violent | Damage Type | Sphere | Quest | Routines |
|----|------------|-----------|---------|-------------|--------|-------|----------|
| 1 | armor | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 2 | teleport | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 3 | bless | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT, MAG_ALTER_OBJ |
| 4 | blindness | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 5 | burning hands | 1.0 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 6 | call lightning | 2.0 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_DAMAGE |
| 7 | charm person | 1.5 | false | DAM_MENTAL | SKILL_SPHERE_ENCHANT | true | MAG_MANUAL |
| 8 | chill touch | 1.0 | true | DAM_COLD | SKILL_SPHERE_WATER | false | MAG_DAMAGE, MAG_AFFECT |
| 9 | clone | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_SUMMON |
| 10 | color spray | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_DAMAGE |
| 11 | control weather | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 12 | create food | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_CREATION |
| 13 | create water | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 14 | cure blind | 1.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_UNAFFECT |
| 15 | cure critic | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 16 | cure light | 1.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 17 | detect alignment | 1.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 18 | detect invisibility | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 19 | detect magic | 1.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 20 | detect poison | 1.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 21 | dispel evil | 1.5 | true | DAM_ALIGN | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 22 | earthquake | 2.0 | true | DAM_ACID | SKILL_SPHERE_EARTH | false | MAG_AREA |
| 23 | enchant weapon | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 24 | energy drain | 1.5 | true | DAM_HEAL | SKILL_SPHERE_DEATH | false | MAG_DAMAGE, MAG_MANUAL |
| 25 | fireball | 2.0 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_AREA |
| 26 | harm | 1.5 | true | DAM_HEAL | SKILL_SPHERE_HEALING | false | MAG_DAMAGE |
| 27 | heal | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 28 | invisibility | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 29 | lightning bolt | 1.5 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_DAMAGE |
| 30 | locate object | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_MANUAL |
| 31 | magic missile | 1.0 | true | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_DAMAGE |
| 32 | poison | 1.5 | true | DAM_POISON | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 33 | protection from evil | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 34 | remove curse | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_UNAFFECT |
| 35 | sanctuary | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 36 | shocking grasp | 1.0 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_DAMAGE |
| 37 | sleep | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 38 | strength | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 39 | summon | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 40 | ventriloquate | 1.0 | false | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_MANUAL |
| 41 | word of recall | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 42 | remove poison | 1.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_UNAFFECT |
| 43 | sense life | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 44 | animate dead | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_SUMMON |
| 45 | dispel good | 1.5 | true | DAM_ALIGN | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 46 | group armor | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_GROUP |
| 47 | group heal | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_GROUP |
| 48 | group recall | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 49 | infravision | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 50 | waterwalk | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 51 | fly | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 52 | haste | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 53 | stone skin | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 54 | barkskin | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 55 | cone of cold | 2.0 | true | DAM_COLD | SKILL_SPHERE_WATER | false | MAG_AREA |
| 56 | wall of fog | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_WATER | false | MAG_ROOM |
| 57 | wall of force | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_ROOM |
| 58 | blazeward | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 59 | coldshield | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 60 | spirit bear | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 61 | minor creation | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 62 | major creation | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 63 | identify | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_MANUAL |
| 64 | calm | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 65 | feeblemind | 2.0 | true | DAM_MENTAL | SKILL_SPHERE_ENCHANT | true | MAG_AFFECT |
| 66 | confusion | 1.5 | true | DAM_MENTAL | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 67 | blur | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 68 | haze | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 69 | farsee | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_MANUAL |
| 70 | ethereal focus | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 71 | illusory wall | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 72 | chillshield | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 73 | silence | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 74 | word of command | 1.5 | true | DAM_MENTAL | SKILL_SPHERE_ENCHANT | true | MAG_MANUAL |
| 75 | mass calm | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 76 | mesmerize | 2.0 | true | DAM_MENTAL | SKILL_SPHERE_ENCHANT | true | MAG_AFFECT |
| 77 | shrink | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 78 | enlarge | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 79 | vigorize critical | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 80 | vigorize light | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 81 | vigorize serious | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 82 | cure serious | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 83 | goodberry | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 84 | minor paralysis | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 85 | major paralysis | 2.0 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 86 | dismiss | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 87 | true seeing | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 88 | harness | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 89 | demonskin | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 90 | vitality | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 91 | illuminate | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_AFFECT |
| 92 | darkness | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_AFFECT |
| 93 | ice storm | 2.5 | true | DAM_COLD | SKILL_SPHERE_WATER | false | MAG_AREA |
| 94 | fire darts | 1.5 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 95 | acid arrow | 1.5 | true | DAM_ACID | SKILL_SPHERE_EARTH | false | MAG_DAMAGE |
| 96 | refresh | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 97 | stasis | 2.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | true | MAG_AFFECT |
| 98 | find the path | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_MANUAL |
| 99 | curse | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 100 | tongues | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_MANUAL |
| 101 | flame blade | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_FIRE | false | MAG_MANUAL |
| 102 | aerial servant | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_SUMMON |
| 103 | earth elemental | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_SUMMON |
| 104 | fire elemental | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_SUMMON |
| 105 | water elemental | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_SUMMON |
| 106 | preservation | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_MANUAL |
| 107 | sandstorm | 2.5 | true | DAM_ACID | SKILL_SPHERE_EARTH | false | MAG_AREA |
| 108 | flamestrike | 2.0 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 109 | ether snap | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 110 | life tap | 1.5 | true | DAM_HEAL | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 111 | stone to flesh | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_UNAFFECT |
| 112 | petrify | 2.5 | true | DAM_UNDEFINED | SKILL_SPHERE_EARTH | true | MAG_AFFECT |
| 113 | shillelagh | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 114 | natures embrace | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 115 | regeneration | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_AFFECT |
| 116 | incendiary cloud | 2.5 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_AREA |
| 117 | area lightning | 2.5 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_AREA |
| 118 | transform | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 119 | air blast | 1.5 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_DAMAGE |
| 120 | agony | 2.0 | true | DAM_HEAL | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 121 | deaths door | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 122 | soul steal | 2.5 | true | DAM_HEAL | SKILL_SPHERE_DEATH | true | MAG_MANUAL |
| 123 | malison | 2.0 | true | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 124 | heroes feast | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_GROUP |
| 125 | entangle | 2.0 | true | DAM_UNDEFINED | SKILL_SPHERE_EARTH | false | MAG_AFFECT |
| 126 | thornflesh | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 127 | thorn spray | 2.0 | true | DAM_ACID | SKILL_SPHERE_EARTH | false | MAG_AREA |
| 128 | bone armor | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 129 | tame | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 130 | disease | 1.5 | true | DAM_POISON | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 131 | cure disease | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_UNAFFECT |
| 132 | slow | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 133 | sustain | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 134 | fear | 1.5 | true | DAM_MENTAL | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 135 | youth | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 136 | cremate | 2.0 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 137 | peace | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 138 | soulshield | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 139 | word of recall II | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 140 | divine essence | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 141 | rain of fire | 2.5 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_AREA |
| 142 | arcane guard | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 143 | spirit sight | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 144 | feather fall | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 145 | degeneration | 2.0 | true | DAM_POISON | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 146 | dimension door | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 147 | armor of gaia | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 148 | mantle of gaia | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 149 | elemental shield | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 150 | spirit wolf | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 151 | wither | 1.5 | true | DAM_POISON | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 152 | reveal true name | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | true | MAG_MANUAL |
| 153 | prayer | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 154 | scry | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_MANUAL |
| 155 | disintegrate | 2.5 | true | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | true | MAG_DAMAGE |
| 156 | dispel magic | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_MANUAL |
| 157 | frenzy | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 158 | negate heat | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 159 | negate cold | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 160 | harness II | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 161 | enhance ability | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 162 | cone of silence | 2.0 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AREA |
| 163 | mass refresh | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_GROUP |
| 164 | detect illusion | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 165 | fumble | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 166 | clumsiness | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 167 | weakness | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 168 | reduce | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 169 | dimensional shift | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 170 | minute meteor | 2.0 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 171 | full harm | 2.0 | true | DAM_HEAL | SKILL_SPHERE_HEALING | false | MAG_DAMAGE |
| 173 | gate | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 174 | prismatic spray | 2.5 | true | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_AREA |
| 175 | time stop | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | true | MAG_MANUAL |
| 176 | solidity | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 177 | spirit link | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 178 | etherealness | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 179 | graft weapon | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 180 | mirage | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 181 | bravery | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 182 | clarity | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 183 | group clarity | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_GROUP |
| 184 | eyes of the dead | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 185 | conjure elemental | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 186 | traveling mount | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 187 | mass levitate | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MASS |
| 188 | silence group | 2.0 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AREA |
| 189 | sickness | 1.5 | true | DAM_POISON | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 190 | sense animals | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 191 | sense plants | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DIVIN | false | MAG_AFFECT |
| 192 | vacate | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 193 | recall group II | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 194 | tame animal | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_MANUAL |
| 195 | vitalize | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_POINT |
| 196 | wind walk | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 197 | fortify | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 198 | mass blur | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AREA |
| 199 | moonbeam | 2.0 | true | DAM_ALIGN | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 200 | endure heat | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 201 | endure cold | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 202 | armor of righteousness | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 203 | acid fog | 2.5 | true | DAM_ACID | SKILL_SPHERE_EARTH | false | MAG_ROOM |
| 204 | transmute rock to mud | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_EARTH | false | MAG_MANUAL |
| 205 | sunray | 2.5 | true | DAM_ALIGN | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 206 | dispel minor | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_MANUAL |
| 207 | levitate | 1.5 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 208 | chill ray | 1.5 | true | DAM_COLD | SKILL_SPHERE_WATER | false | MAG_DAMAGE |
| 209 | firestorm | 2.5 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_AREA |
| 210 | turn undead | 2.0 | true | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_MANUAL |
| 211 | siphon life | 2.0 | true | DAM_HEAL | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 212 | revive | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_MANUAL |
| 213 | resurrection | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_MANUAL |
| 214 | fire breath | 1.5 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 215 | gas breath | 1.5 | true | DAM_POISON | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 216 | frost breath | 1.5 | true | DAM_COLD | SKILL_SPHERE_WATER | false | MAG_DAMAGE |
| 217 | acid breath | 1.5 | true | DAM_ACID | SKILL_SPHERE_EARTH | false | MAG_DAMAGE |
| 218 | lightning breath | 1.5 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_DAMAGE |
| 219 | magic armor | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 220 | water breathing | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 221 | spirit armor | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 222 | summon legion | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_SUMMON |
| 223 | blade barrier | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 224 | hex | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 225 | faerie fire | 1.5 | true | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 226 | natures regeneration | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_AFFECT |
| 227 | hold undead | 2.0 | true | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 228 | fire seeds | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 229 | moonwell | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 230 | parch | 2.0 | true | DAM_HEAL | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 231 | flame arrow | 1.5 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_DAMAGE |
| 232 | call follower | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_SUMMON | false | MAG_MANUAL |
| 233 | plague | 2.5 | true | DAM_POISON | SKILL_SPHERE_DEATH | false | MAG_AREA |
| 234 | steelskin | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 235 | negate air | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_PROT | false | MAG_AFFECT |
| 236 | shocking sphere | 2.0 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_DAMAGE |
| 237 | vampiric aura | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_DEATH | false | MAG_AFFECT |
| 238 | chain lightning | 2.5 | true | DAM_SHOCK | SKILL_SPHERE_AIR | false | MAG_AREA |
| 239 | meteor swarm | 2.5 | true | DAM_FIRE | SKILL_SPHERE_FIRE | false | MAG_AREA |
| 240 | dispel sanctity | 1.5 | true | DAM_ALIGN | SKILL_SPHERE_DEATH | false | MAG_DAMAGE |
| 241 | holy word | 2.5 | true | DAM_ALIGN | SKILL_SPHERE_DEATH | false | MAG_AREA |
| 242 | unholy word | 2.5 | true | DAM_ALIGN | SKILL_SPHERE_DEATH | false | MAG_AREA |
| 243 | endurance | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 244 | dexterity | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 245 | agility | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 246 | brilliance | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 247 | cunning | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 248 | charisma | 2.0 | false | DAM_UNDEFINED | SKILL_SPHERE_ENCHANT | false | MAG_AFFECT |
| 249 | rapid regeneration | 2.5 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_AFFECT |
| 250 | grant life | 3.0 | false | DAM_UNDEFINED | SKILL_SPHERE_HEALING | false | MAG_MANUAL |
| 251 | circle of destruction | 2.5 | true | DAM_UNDEFINED | SKILL_SPHERE_GENERIC | false | MAG_AREA |

## Cast Time Mechanics

### CAST_SPEED System

Spells use a `CAST_SPEED` rating (0-12) that determines base casting time in rounds:

```cpp
enum CastSpeed {
    CAST_SPEED0,   // 0.0 rounds - instant
    CAST_SPEED1,   // 0.25 rounds
    CAST_SPEED2,   // 0.5 rounds
    CAST_SPEED3,   // 0.75 rounds
    CAST_SPEED4,   // 1.0 rounds
    CAST_SPEED5,   // 1.25 rounds
    CAST_SPEED6,   // 1.5 rounds
    CAST_SPEED7,   // 1.75 rounds
    CAST_SPEED8,   // 2.0 rounds
    CAST_SPEED9,   // 2.25 rounds
    CAST_SPEED10,  // 2.5 rounds
    CAST_SPEED11,  // 2.75 rounds
    CAST_SPEED12   // 3.0 rounds
};
```

**Formula**: `actualCastTime = baseCastTime * (1 - quickChantBonus)`

### Quick Chant Skill Interaction

The Quick Chant skill reduces casting time based on skill proficiency:

```cpp
// Quick Chant reduces cast time by 0-25% based on skill level
float quickChantBonus = (skillLevel / 100.0f) * 0.25f;
```

**Example**:
- Spell: Fireball (CAST_SPEED8 = 2.0 rounds)
- Quick Chant: 80% proficiency
- Reduction: 80% * 25% = 20%
- Final Time: 2.0 * 0.8 = 1.6 rounds

### Cast Time Distribution

| Speed Range | Count | Examples |
|-------------|-------|----------|
| Instant (0.0) | 0 | None |
| Fast (0.25-1.0) | 42 | magic missile, cure light, detect magic |
| Moderate (1.25-2.0) | 156 | fireball, heal, teleport, bless |
| Slow (2.25-2.75) | 49 | summon elemental, gate, resurrection |
| Very Slow (3.0) | 4 | gate, conjure elemental, earth/fire/water elemental |

## Spell Circle System

**Note**: The spell circle/memorization system is referenced in the codebase but not fully implemented in the extracted metadata. Legacy CircleMUD used a Vancian magic system where spells were memorized in "slots" and consumed upon casting.

**Current Implementation Status**:
- Circle data present in SkillDef structure (`pages` field indicates circle complexity)
- Memorization mechanics not fully active in modern codebase
- Most spells have `additionalMemTime: 0` indicating instant memorization

**Circle Complexity (Pages)**:
- Low Circles (1-5 pages): Utility spells, basic combat magic
- Mid Circles (6-10 pages): Powerful combat, summoning
- High Circles (11-15 pages): Teleportation, high-level summons

## Targeting System

### TAR_* Flags

Spells use bitwise flags to define valid targets:

```cpp
#define TAR_IGNORE         (1 << 0)  // No target required
#define TAR_CHAR_ROOM      (1 << 1)  // Character in same room
#define TAR_CHAR_WORLD     (1 << 2)  // Character anywhere in world
#define TAR_FIGHT_SELF     (1 << 3)  // Self during combat
#define TAR_FIGHT_VICT     (1 << 4)  // Combat victim
#define TAR_SELF_ONLY      (1 << 5)  // Only caster
#define TAR_NOT_SELF       (1 << 6)  // Anyone but caster
#define TAR_OBJ_INV        (1 << 7)  // Object in inventory
#define TAR_OBJ_ROOM       (1 << 8)  // Object in room
#define TAR_OBJ_WORLD      (1 << 9)  // Object anywhere
#define TAR_OBJ_EQUIP      (1 << 10) // Equipped object
#define TAR_DIRECT         (1 << 11) // Direct line of sight required
#define TAR_UNPLEASANT     (1 << 12) // Hostile spell (triggers aggro)
```

**Common Combinations**:
- `TAR_CHAR_ROOM`: Buff spells (bless, strength, haste)
- `TAR_CHAR_ROOM | TAR_SELF_ONLY`: Self-buffs (teleport, word of recall)
- `TAR_CHAR_ROOM | TAR_FIGHT_VICT`: Combat spells (fireball, harm)
- `TAR_CHAR_ROOM | TAR_OBJ_INV`: Versatile spells (bless weapon or person)
- `TAR_IGNORE`: Area effects (earthquake, mass calm)

### Targeting Examples

```cpp
// Armor - buff any character in room
targets: [TAR_CHAR_ROOM]

// Teleport - only self
targets: [TAR_CHAR_ROOM, TAR_SELF_ONLY]

// Harm - combat target, hostile
targets: [TAR_CHAR_ROOM, TAR_FIGHT_VICT, TAR_UNPLEASANT]

// Identify - object in inventory
targets: [TAR_OBJ_INV]

// Earthquake - no specific target (area effect)
targets: [TAR_IGNORE, TAR_UNPLEASANT]
```

## Magic Routines

### MAG_* Handler Types

Magic routines determine how spell effects are applied:

| Routine | Count | Purpose | Examples |
|---------|-------|---------|----------|
| MAG_AFFECT | 98 | Apply temporary effect | armor, bless, haste, invisibility |
| MAG_MANUAL | 63 | Custom handler required | teleport, summon, identify, charm |
| MAG_DAMAGE | 47 | Deal damage | fireball, harm, lightning bolt |
| MAG_AREA | 18 | Area-of-effect damage | earthquake, ice storm, meteor swarm |
| MAG_POINT | 11 | Healing/restoration | cure light, heal, refresh |
| MAG_UNAFFECT | 8 | Remove specific effect | cure blind, remove poison |
| MAG_SUMMON | 7 | Summon creature | animate dead, aerial servant |
| MAG_ALTER_OBJ | 6 | Modify object | bless weapon, enchant weapon |
| MAG_GROUP | 5 | Affect entire group | group armor, group heal |
| MAG_ROOM | 3 | Affect entire room | wall of fog, acid fog |
| MAG_CREATION | 2 | Create object | create food |
| MAG_MASS | 1 | Mass targeting | mass levitate |
| MAG_BULK_OBJS | 1 | Multiple objects | preservation (assumed) |

### Routine Details

**MAG_AFFECT**: Applies temporary magical effects using the affect system:
```cpp
// Example: armor spell
struct affected_type af = {
    .type = SPELL_ARMOR,
    .duration = level * 2,
    .modifier = -20,  // AC bonus
    .location = APPLY_AC
};
affect_to_char(victim, &af);
```

**MAG_DAMAGE**: Calculates and applies damage:
```cpp
// Example: fireball damage
int dam = dice(level, 8) + level;
damage(ch, victim, dam, SPELL_FIREBALL);
```

**MAG_MANUAL**: Requires custom implementation in `spell_handler.cpp`:
```cpp
ASPELL(spell_teleport) {
    // Custom teleportation logic
    char_from_room(victim);
    char_to_room(victim, target_room);
    act("$n vanishes in a flash of light!", TRUE, victim, 0, 0, TO_ROOM);
}
```

**MAG_AREA**: Affects all valid targets in room:
```cpp
// Automatically iterates through room occupants
// Filters by alignment, faction, group membership
```

**MAG_POINT**: Heals or restores points (HP/mana/movement):
```cpp
int healing = dice(level, 8) + level;
GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
```

## Spell Spheres

### Sphere Distribution

| Sphere | Count | Theme | Access |
|--------|-------|-------|--------|
| SKILL_SPHERE_ENCHANT | 68 | Buffs, debuffs, mental effects | Mages, Clerics |
| SKILL_SPHERE_SUMMON | 31 | Transportation, creatures | Mages, Clerics |
| SKILL_SPHERE_PROT | 28 | Defensive magic, wards | Clerics, Paladins |
| SKILL_SPHERE_FIRE | 21 | Fire damage and effects | Mages, Pyromancers |
| SKILL_SPHERE_GENERIC | 20 | Universal magic | All casters |
| SKILL_SPHERE_HEALING | 19 | Restoration, curing | Clerics, Druids |
| SKILL_SPHERE_DEATH | 19 | Necromancy, energy drain | Necromancers, Evil Clerics |
| SKILL_SPHERE_WATER | 16 | Cold damage, water effects | Mages, Druids |
| SKILL_SPHERE_DIVIN | 13 | Detection, scrying | Clerics, All casters |
| SKILL_SPHERE_EARTH | 9 | Acid damage, stone effects | Druids, Earth casters |
| SKILL_SPHERE_AIR | 7 | Lightning, wind effects | Mages, Storm casters |

### Sphere Access by Class

**Clerics**: PROT, HEALING, DEATH, DIVIN, ENCHANT
**Mages**: FIRE, WATER, AIR, EARTH, ENCHANT, SUMMON, GENERIC
**Druids**: HEALING, EARTH, WATER, SUMMON, ENCHANT
**Necromancers**: DEATH, SUMMON, ENCHANT

## Implementation Patterns

Based on analysis of ASPELL implementations, spells follow 10 distinct patterns:

### 1. Simple Buff/Debuff (MAG_AFFECT)

**Characteristics**: Single affect applied, configurable duration/modifier
**Count**: ~40 spells
**Examples**: armor, bless, strength, haste

```cpp
// Fully configurable via SkillDef
struct affected_type af = {
    .type = spellnum,
    .duration = duration,
    .modifier = modifier,
    .location = apply_location
};
```

### 2. Direct Damage (MAG_DAMAGE)

**Characteristics**: Damage calculation with dice rolls
**Count**: ~25 spells
**Examples**: magic missile, fireball, lightning bolt

```cpp
int dam = dice(level, damage_dice) + damage_bonus;
damage(ch, victim, dam, spellnum);
```

### 3. Complex Teleportation (MAG_MANUAL)

**Characteristics**: Room validation, movement, special messages
**Count**: ~8 spells
**Examples**: teleport, word of recall, dimension door, gate

```cpp
ASPELL(spell_teleport) {
    // Validate target room
    // Check restrictions (no-teleport zones, etc.)
    // Move character with char_from_room/char_to_room
    // Send appropriate messages
}
```

### 4. Creature Summoning (MAG_SUMMON + MAG_MANUAL)

**Characteristics**: Mob creation, charm application, timed removal
**Count**: ~12 spells
**Examples**: animate dead, aerial servant, elementals

```cpp
ASPELL(spell_animate_dead) {
    // Find corpse object
    // Create mob from template
    // Apply charm effect
    // Set timed extraction
}
```

### 5. Multi-Effect Spells (Multiple Routines)

**Characteristics**: Combines damage + affect or multiple affects
**Count**: ~6 spells
**Examples**: chill touch (damage + STR debuff), energy drain (damage + XP drain)

```cpp
// Apply damage via MAG_DAMAGE
// Apply secondary effect via MAG_AFFECT or custom logic
```

### 6. Area Effects (MAG_AREA)

**Characteristics**: Affects all valid targets in room
**Count**: ~18 spells
**Examples**: earthquake, ice storm, meteor swarm

```cpp
// System automatically iterates room occupants
// Filters by group membership, alignment
// Applies effect to each valid target
```

### 7. Healing/Restoration (MAG_POINT)

**Characteristics**: Restores HP/mana/movement
**Count**: ~11 spells
**Examples**: cure light, heal, refresh

```cpp
int healing = dice(level, heal_dice) + heal_bonus;
GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
```

### 8. Object Manipulation (MAG_ALTER_OBJ + MAG_MANUAL)

**Characteristics**: Modifies object properties
**Count**: ~5 spells
**Examples**: enchant weapon, bless (on object), identify

```cpp
ASPELL(spell_enchant_weapon) {
    // Validate object type
    // Modify object stats (hitroll, damroll)
    // Set magical flag
    // Apply wear-off timer
}
```

### 9. Detection/Divination (MAG_AFFECT + MAG_MANUAL)

**Characteristics**: Grants vision modes or provides information
**Count**: ~13 spells
**Examples**: detect magic, sense life, identify, farsee

```cpp
// Detection: Apply AFF_DETECT_* flag via MAG_AFFECT
// Divination: Custom information display via MAG_MANUAL
```

### 10. Room Effects (MAG_ROOM)

**Characteristics**: Modifies room state or creates barriers
**Count**: ~3 spells
**Examples**: wall of fog, wall of force, acid fog

```cpp
ASPELL(spell_wall_of_fog) {
    // Apply room flag or create barrier object
    // Set duration timer
    // Affect movement/combat in room
}
```

## Database Schema Mapping

### Legacy SkillDef to Prisma Schema

The legacy C++ `SkillDef` structure maps to the modern Prisma database schema:

**Legacy Structure** (`src/magic.cpp`):
```cpp
struct SkillDef {
    const char *name;          // "fireball"
    int castTimeSpeed;         // CAST_SPEED8
    float castTimeRounds;      // 2.0
    int minPosition;           // POS_STANDING
    bool fightingOk;           // true
    int targets;               // TAR_CHAR_ROOM | TAR_FIGHT_VICT
    bool violent;              // true
    int routines;              // MAG_AREA
    int additionalMemTime;     // 0
    int damageType;            // DAM_FIRE
    int sphere;                // SKILL_SPHERE_FIRE
    int pages;                 // 8
    bool questSpell;           // false
    const char *wearOffMsg;    // "The flames subside."
};
```

**Modern Prisma Schema** (muditor `schema.prisma`):
```prisma
model Spell {
  id                Int       @id @default(autoincrement())
  enumName          String    @unique
  name              String    @unique
  castTimeSpeed     String    // "CAST_SPEED8"
  castTimeRounds    Float     // 2.0
  minPosition       String    // "POS_STANDING"
  fightingOk        Boolean   // true
  targets           String[]  // ["TAR_CHAR_ROOM", "TAR_FIGHT_VICT"]
  violent           Boolean   // true
  routines          String[]  // ["MAG_AREA"]
  additionalMemTime Int       // 0
  damageType        String    // "DAM_FIRE"
  sphere            String    // "SKILL_SPHERE_FIRE"
  pages             Int       // 8
  questSpell        Boolean   // false
  wearOffMessage    String?   // "The flames subside."

  // Additional fields for modern system
  baseDamage        Int?      // Damage dice configuration
  duration          Int?      // Effect duration formula
  savingThrow       String?   // Saving throw type
  spellResistance   Boolean?  // Spell resistance applicable

  createdAt         DateTime  @default(now())
  updatedAt         DateTime  @updatedAt
}
```

### Migration Considerations

1. **Enum to String Conversion**: C++ enums (`CAST_SPEED8`, `DAM_FIRE`) stored as strings for flexibility
2. **Bitwise Flags to Arrays**: `targets` and `routines` converted from bitfields to string arrays
3. **Extended Metadata**: Additional fields for damage formulas, saving throws, spell resistance
4. **Audit Trail**: `createdAt`/`updatedAt` timestamps for change tracking

### Database Integration Points

**FieryLib Import**:
```python
# Import spell definitions from legacy files
spell = prisma.spell.create({
    "enumName": "SPELL_FIREBALL",
    "name": "fireball",
    "castTimeRounds": 2.0,
    "violent": True,
    "damageType": "DAM_FIRE",
    # ... additional fields
})
```

**Muditor GraphQL API**:
```graphql
query GetSpell($id: Int!) {
  spell(id: $id) {
    id
    name
    castTimeRounds
    violent
    damageType
    sphere
    routines
  }
}
```

**FieryMUD Runtime** (C++ libpqxx):
```cpp
// Query spell at runtime
pqxx::result r = txn.exec_params(
    "SELECT * FROM \"Spell\" WHERE \"enumName\" = $1",
    "SPELL_FIREBALL"
);
```

## Development Guidelines

### Adding New Spells

1. **Define Metadata**: Add entry to Prisma schema via Muditor
2. **Assign Routine**: Choose appropriate MAG_* handler(s)
3. **Implement Logic**:
   - MAG_AFFECT/MAG_DAMAGE: Configure via database
   - MAG_MANUAL: Implement ASPELL function in `spell_handler.cpp`
4. **Test Thoroughly**: Validate targeting, damage, duration, messages
5. **Document**: Update help text and balance notes

### Balancing Considerations

- **Cast Time**: Longer cast times for powerful effects
- **Damage Scaling**: Linear scaling appropriate for most combat spells
- **Duration Formulas**: Level-based duration for buffs (level * 2 rounds typical)
- **Sphere Access**: Restrict powerful spells to appropriate classes
- **Quest Flags**: Use for story-critical or overpowered spells

### Testing Checklist

- [ ] Spell casts successfully with correct target
- [ ] Damage/healing values within expected range
- [ ] Effects apply and wear off correctly
- [ ] Messages display to caster, victim, room
- [ ] Violent flag triggers appropriate aggro
- [ ] Quest flag prevents inappropriate access
- [ ] Works with Quick Chant skill
- [ ] Compatible with spell resistance/saving throws

## Future Enhancements

1. **Full Circle Implementation**: Restore Vancian memorization system
2. **Dynamic Spell Formulas**: Database-driven damage/duration calculations
3. **Spell Combinations**: Synergy effects for multiple active buffs
4. **Metamagic**: Quicken, Empower, Extend spell modifications
5. **Counterspelling**: Interrupt enemy casting with dispel magic
6. **Spell Resistance**: Full SR calculation and bypass mechanics
7. **Visual Effects**: Enhanced combat messages and ASCII art
8. **Spell Scrolls/Wands**: Object-based spell storage and casting

## References

- **Source Code**: `src/magic.cpp`, `src/spell_handler.cpp`, `src/spell_parser.cpp`
- **Data Files**: `spell_metadata.json` (generated), `lib/text/spells/*` (help files)
- **Prisma Schema**: `muditor/packages/db/prisma/schema.prisma`
- **Legacy Docs**: `doc/spell_info.txt` (CircleMUD original)

---

*Document Version*: 1.0
*Last Updated*: 2025-11-06
*Spell Count*: 251 spells analyzed

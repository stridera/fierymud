# Damage Spell Formulas

Complete reference for all damage spells in FieryMUD.

**Total Damage Spells**: 79

## Damage Types

- **DAM_FIRE**: Fire damage
- **DAM_COLD**: Cold/ice damage
- **DAM_SHOCK**: Lightning/electricity damage
- **DAM_ACID**: Acid/corrosive damage
- **DAM_POISON**: Poison/toxic damage
- **DAM_ALIGN**: Alignment-based (good/evil) damage
- **DAM_MENTAL**: Psychic/mental damage
- **DAM_ENERGY**: Pure magical energy
- **DAM_ROT**: Decay/rot damage
- **DAM_WATER**: Water damage
- **DAM_PIERCE/SLASH/CRUSH**: Physical damage types

## Damage Formulas

### Sorcerer Single-Target Spells (Balanced)

These spells use `sorcerer_single_target(ch, spellnum, skill)` and are calibrated to warrior damage:


### Acid Burst
- **ID**: `SPELL_ACID_BURST`
- **Damage Type**: DAM_ACID
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:568

### Ancestral Vengeance
- **ID**: `SPELL_ANCESTRAL_VENGEANCE`
- **Damage Type**: DAM_ALIGN
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:562

### Baleful Polymorph
- **ID**: `SPELL_BALEFUL_POLYMORPH`
- **Damage Type**: DAM_ALIGN
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:565

### Bigby's Clenched Fist
- **ID**: `SPELL_BIGBYS_CLENCHED_FIST`
- **Damage Type**: DAM_CRUSH
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:563

### Burning Hands
- **ID**: `SPELL_BURNING_HANDS`
- **Damage Type**: DAM_FIRE
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:552

### Cone of Cold
- **ID**: `SPELL_CONE_OF_COLD`
- **Damage Type**: DAM_COLD
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:558

### Decay
- **ID**: `SPELL_DECAY`
- **Damage Type**: DAM_ROT
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:551

### Detonation
- **ID**: `SPELL_DETONATION`
- **Damage Type**: DAM_ENERGY
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:553

### Disintegrate
- **ID**: `SPELL_DISINTEGRATE`
- **Damage Type**: DAM_ALIGN
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:569

### Dispel Magic
- **ID**: `SPELL_DISPEL_MAGIC`
- **Damage Type**: varies
- **Formula**: sorcerer_single_target(ch, spellnum, skill)
- **Notes**: Special case - evades normal immunity checks, reduction=yes
- **Source**: magic.cpp:718-726

### Fireball
- **ID**: `SPELL_FIREBALL`
- **Damage Type**: DAM_FIRE
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:560

### Fracture
- **ID**: `SPELL_FRACTURE`
- **Damage Type**: DAM_ALIGN
- **Formula**: sorcerer_single_target / 3, x2 if charmed by PC master
- **Notes**: Spawns SPELL_FRACTURE_SHRAPNEL AoE
- **Source**: magic.cpp:817-821

### Fracture Shrapnel
- **ID**: `SPELL_FRACTURE_SHRAPNEL`
- **Damage Type**: DAM_PIERCE
- **Formula**: sorcerer_single_target(SPELL_FRACTURE) / 2, x2 if charmed by PC master
- **Notes**: AoE splash from SPELL_FRACTURE
- **Source**: magic.cpp:822-826

### Freeze
- **ID**: `SPELL_FREEZE`
- **Damage Type**: DAM_COLD
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:567

### Iceball
- **ID**: `SPELL_ICEBALL`
- **Damage Type**: DAM_COLD
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:570

### Immolate
- **ID**: `SPELL_IMMOLATE`
- **Damage Type**: DAM_FIRE
- **Formula**: sorcerer_single_target / 4
- **Notes**: Hits 5 times, reduction=yes
- **Source**: magic.cpp:867-871

### Iron Maiden
- **ID**: `SPELL_IRON_MAIDEN`
- **Damage Type**: DAM_PIERCE
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:566

### Lightning Bolt
- **ID**: `SPELL_LIGHTNING_BOLT`
- **Damage Type**: DAM_SHOCK
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:556

### Melt
- **ID**: `SPELL_MELT`
- **Damage Type**: DAM_FIRE
- **Formula**: sorcerer_single_target(ch, spellnum, skill)
- **Source**: magic.cpp:908-919

### Nightmare
- **ID**: `SPELL_NIGHTMARE`
- **Damage Type**: DAM_MENTAL
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:557

### Phosphoric Embers
- **ID**: `SPELL_PHOSPHORIC_EMBERS`
- **Damage Type**: DAM_FIRE
- **Formula**: sorcerer_single_target / 3
- **Notes**: Hits 4 times, reduction=yes
- **Source**: magic.cpp:927-931

### Positive Field
- **ID**: `SPELL_POSITIVE_FIELD`
- **Damage Type**: DAM_ALIGN
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:559

### Pyre
- **ID**: `SPELL_PYRE`
- **Damage Type**: DAM_FIRE
- **Formula**: sorcerer_single_target / 3, x2 if charmed by PC master
- **Notes**: Hits 4 times, reduction=yes
- **Source**: magic.cpp:932-938

### Pyre Recoil
- **ID**: `SPELL_PYRE_RECOIL`
- **Damage Type**: DAM_FIRE
- **Formula**: sorcerer_single_target(SPELL_PYRE) / 6
- **Notes**: Uses SPELL_ON_FIRE damage message, reduction=yes
- **Source**: magic.cpp:939-944

### Shocking Grasp
- **ID**: `SPELL_SHOCKING_GRASP`
- **Damage Type**: DAM_SHOCK
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:555

### Spirit Ray
- **ID**: `SPELL_SPIRIT_RAY`
- **Damage Type**: DAM_ALIGN
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:564

### Vicious Mockery
- **ID**: `SPELL_VICIOUS_MOCKERY`
- **Damage Type**: DAM_MENTAL
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:561

### Water Blast
- **ID**: `SPELL_WATER_BLAST`
- **Damage Type**: DAM_WATER
- **Formula**: sorcerer_single_target
- **Source**: magic.cpp:571

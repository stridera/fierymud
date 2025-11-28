# FieryMUD Ability System - Complete Reference

**Source**: Extracted from actual C++ implementation code

## Overview

- **Total abilities**: 368
  - Chants: 18
  - Skills: 89
  - Songs: 10
  - Spells: 251

### Ability Categories

- **UTILITY**: 134 abilities
- **BUFF**: 93 abilities
- **DAMAGE**: 65 abilities
- **DEFENSIVE**: 20 abilities
- **COMBAT**: 18 abilities
- **MOVEMENT**: 11 abilities
- **WEAPON_PROFICIENCY**: 9 abilities
- **STEALTH**: 8 abilities
- **SUMMON**: 7 abilities
- **TELEPORT**: 2 abilities
- **HEALING**: 1 abilities

---

## Implementation Mechanics

This section provides comprehensive implementation details extracted from the FieryMUD C++ codebase.

### Quick Links

- [Saving Throw System](#saving-throw-system)
- [Effect Flags - What They Actually Do](#effect-flags-reference)
- [Duration Formulas](#duration-formulas)
- [Stat Modifiers (APPLY Locations)](#stat-modifiers-apply-locations)
- [Combat Mechanics](#combat-mechanics)
- [Detailed Effect Flag Reference](EFFECT_FLAGS_REFERENCE.md)
- [Complete Implementation Data (JSON)](../docs/ability_implementation_mechanics.json)

### Saving Throw System

**Formula**: `save = get_base_saves(ch, type) + GET_SAVE(ch, type); success if max(1, save) < random(0,99)`

**Five Save Types**:
1. **SAVING_PARA** (Paralysis) - Modified by CON
2. **SAVING_ROD** (Rods/Staves/Wands) - Modified by DEX
3. **SAVING_PETRI** (Petrification) - No stat bonus
4. **SAVING_BREATH** (Breath Weapons) - Modified by DEX
5. **SAVING_SPELL** (Magical Spells) - Modified by WIS

**How Saves Work**:
- Lower save number = better (easier to resist)
- Base save starts at 100-115 (class-dependent)
- Decreases by 0.5 per level (better saves at higher level)
- Stats above 90 provide -0.5 bonus per point (e.g., 100 CON = -5 to PARA saves)
- Rolling 0 always fails (formula uses `max(1, save)`)

**Racial Bonuses**:
- **Dwarves/Duergar**: -20% CON to PARA, -10% CON to ROD, -15% CON to SPELL
- **Dragonborn**: -12.5% CON to PARA, -10% CON to ROD/SPELL
- **Gnomes/Sverfneblin**: -10% CON to ROD/SPELL
- **Halflings**: -10% CON to PARA/ROD/SPELL

**Source**: `chars.cpp:96-152`, `magic.cpp:220-252`

### Effect Flags Reference

**What Effect Flags Actually Do** (see [EFFECT_FLAGS_REFERENCE.md](EFFECT_FLAGS_REFERENCE.md) for complete details):

#### Combat Buffs

- **EFF_BLESS** (78): Primary purpose is allowing barehand attacks to hit ethereal creatures
  - Saves: -2 to -12 spell resistance
  - Damage: +1 to +2 damroll (skill 55+)
  - Combat: -level/5 THAC0 vs evil (good vs evil bonus)
  - Duration: 10-24 hours
  - Source: `magic.cpp:1230-1256`, `fight.cpp:2123-2127`

- **EFF_HASTE** (24): +1 attack per combat round
  - Stacks with Double Attack skill and EFF_BLUR
  - Duration: 2-6 hours
  - Source: `magic.cpp:2111-2114`, `fight.cpp:2397-2398`

- **EFF_BLUR** (25): +1 attack per combat round
  - Stacks with Haste and Double Attack
  - Duration: 2-6 hours
  - Source: `magic.cpp:1294`, `fight.cpp:2399-2400`

#### Defensive Effects

- **EFF_SANCTUARY** (7): 50% damage reduction on ALL incoming damage
  - Formula: `dam >>= 1` (bitwise divide by 2)
  - Also grants charm immunity
  - Visual: Black aura (evil) or white aura (good)
  - Source: `fight.cpp:1650`, `spells.cpp:243`

- **EFF_STONE_SKIN** (22): Charge-based damage absorption
  - Charges: 7-13 based on skill
  - Per-hit: 90% chance to reduce damage to 0-3 AND consume 1 charge
  - Base reduction: 50% (like sanctuary)
  - Effect ends when charges reach 0 (not duration-based)
  - Source: `fight.cpp:2267-2270`, `magic.cpp:2853-2856`

- **EFF_MINOR_GLOBE** (46): 30% physical damage reduction
  - Physical attacks only (not spells)
  - Source: `fight.cpp:2242-2244`

- **EFF_MAJOR_GLOBE** (47): 50% physical damage reduction + 30% spell damage reduction
  - Stacks multiplicatively with sanctuary
  - Source: `fight.cpp:2249-2258`

#### Debuffs

- **EFF_BLIND** (0): Severe combat penalties
  - Hitroll: -4 (much harder to hit enemies)
  - AC: -40 (much easier for enemies to hit you)
  - Duration: 2 hours fixed
  - Source: `magic.cpp:1276-1284`

- **EFF_CURSE** (9): Stacking combat penalties
  - Hitroll: -1 to -3
  - Damroll: -1 to -3
  - Accumulates: Multiple curses stack
  - Duration: 5-12 hours
  - Source: `magic.cpp:1422-1429`

- **EFF_CONFUSION** (8): Severe debuff with stat-based save bonus
  - AC: -50 penalty
  - Save bonus: (DEX + WIS - 100) / 10
  - Duration: Variable
  - Source: `fight.cpp:2201-2215`

### Duration Formulas

Common patterns for spell durations (extracted from `mag_affect()` in `magic.cpp`):

- **ARMOR**: `10 + (skill / 50)` hours (max 12)
- **BLESS**: `10 + (skill / 7)` hours (max 24)
- **BARKSKIN**: `5 + (skill / 10)` hours (max 15)
- **BLINDNESS**: `2` hours (fixed)
- **BLUR**: `2 + (skill / 21)` hours (max 6)
- **HASTE**: `2 + (skill / 21)` hours (max 6)
- **CURSE**: `5 + (skill / 14)` hours (max 12)
- **SANCTUARY**: `12` hours (fixed)
- **STONE_SKIN**: Charge-based, not duration-based (ends at 0 charges)

**Note**: Skill ranges from 0-100, so max values shown are at skill 100.

### Stat Modifiers (APPLY Locations)

Effect flags apply stat modifiers via APPLY locations. All 31 types (from `handler.cpp:130-246`):

**Attributes**:
- APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_WIS, APPLY_CON, APPLY_CHA
- APPLY_SIZE, APPLY_AGE, APPLY_CHAR_WEIGHT, APPLY_CHAR_HEIGHT

**Combat Stats**:
- **APPLY_AC**: Subtracted from AC (positive mod = better AC, since lower is better)
- **APPLY_HITROLL**: Added to hit chance (positive = better to-hit)
- **APPLY_DAMROLL**: Added to damage (positive = more damage)

**Resources**:
- APPLY_HIT (max HP), APPLY_MOVE (max stamina), APPLY_MANA (max mana)
- APPLY_HIT_REGEN (HP regen rate)
- APPLY_GOLD, APPLY_EXP (usually reserved)

**Saving Throws**:
- **APPLY_SAVING_PARA**, **APPLY_SAVING_ROD**, **APPLY_SAVING_PETRI**
- **APPLY_SAVING_BREATH**, **APPLY_SAVING_SPELL**
- Note: Positive modifiers WORSEN saves (since lower is better)

**Special Stats**:
- APPLY_FOCUS (spellcasting stat)
- APPLY_PERCEPTION (awareness)
- APPLY_HIDDENNESS (stealth bonus)
- APPLY_COMPOSITION (elemental composition)

**Critical Implementation Note**:
- AC modifier is **SUBTRACTED**: `GET_AC(ch) -= mod` (positive mod improves AC)
- Save modifiers are **ADDED**: `GET_SAVE(ch, type) += mod` (negative mod improves saves)
- This is why BLESS gives -2 to saves (improvement) and why positive save bonuses are actually penalties

### Combat Mechanics

**Attack Frequency**:
- Base: 1 attack per round
- Haste: +1 attack
- Blur: +1 attack
- Double Attack skill: ×2 base attacks
- **Maximum**: (1 × 2) + 1 + 1 = 4 attacks per round

**Damage Reduction Stacking** (multiplicative):
- Sanctuary alone: 50%
- Stone Skin alone: 50% base + 90% chance for near-total block
- Sanctuary + Stone Skin: 75% (50% then 50% of remainder)
- Major Globe + Sanctuary: 75% reduction (50% then 50%)

**Alignment Bonuses**:
- Good + Bless vs Evil: -level/5 THAC0 (major bonus)
- Good + Bless vs Neutral: -level/10 THAC0 (minor bonus)

**Special Cases**:
- Blessed barehand attacks can hit ethereal creatures (critical for some encounters)
- Stone Skin uses modifier field for charges, ends at 0 regardless of duration
- Confusion save bonus scales with stats: (DEX + WIS - 100) / 10

---

## Complete Ability Reference

### BUFF

#### Armor (ID 1)

- **Type**: SPELL
- **Enum**: `SPELL_ARMOR`
- **Command**: `armor`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less protected."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No


**Implementation**:
- **Duration**: `10 + (skill / 50)` (10-12 hours)
- **Effects**:
  - **APPLY_AC**: `10 + (skill / 20)` (10-15) - Positive modifier improves AC (subtracted in handler)
- **Source**: magic.cpp:1182, magic.cpp:1180-1181
#### Barkskin (ID 144)

- **Type**: SPELL
- **Enum**: `SPELL_BARKSKIN`
- **Command**: `barkskin`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your skin softens back to its original texture."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No


**Implementation**:
- **Duration**: `5 + (skill / 10)` (5-15 hours)
- **Effects**:
  - **APPLY_AC**: `7 + (skill / 9)` (7-18)
- **Source**: magic.cpp:1205, magic.cpp:1203-1204
#### Bless (ID 3)

- **Type**: SPELL
- **Enum**: `SPELL_BLESS`
- **Command**: `bless`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less righteous."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No


**Implementation**:
- **Duration**: `10 + (skill / 7)` (10-24 hours)
- **Effects**:
  - **APPLY_SAVING_SPELL**: `-2 - (skill / 10)` (-2 to -12)
  - **APPLY_DAMROLL**: `1 + (skill > 95)` (+1 to +2) - skill >= 55
- **Special Mechanics**:
  - Barehand attacks can hurt ethereal characters (defines.hpp:486)
  - Cannot hit ethereal targets without bless (damage.cpp:73)
- **Restrictions**:
  - Evil casters: Gods forsake them (CAST_SPELL only) (magic.cpp:1213-1217)
  - Evil targets: Cannot bless evil characters (magic.cpp:1223-1228)
  - Conflicts with SPELL_DARK_PRESENCE, SPELL_WINGS_OF_HEAVEN, SPELL_EARTH_BLESSING
- **Source**: magic.cpp:1232, magic.cpp:1230-1231, magic.cpp:1243-1244, magic.cpp:1255, fight.cpp:386, fight.cpp:2123, damage.cpp:73
#### Blindness (ID 4)

- **Type**: SPELL
- **Enum**: `SPELL_BLINDNESS`
- **Command**: `blindness`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel a cloak of blindness dissolve."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes


**Implementation**:
- **Duration**: `2` (2 hours fixed)
- **Effects**:
  - **APPLY_AC**: -40 - Negative modifier worsens AC
  - **APPLY_HITROLL**: -4
- **Special Mechanics**:
  - Character cannot see
  - Severe combat penalties
- **Immunities**:
  - MOB_NOBLIND flag prevents blindness (magic.cpp:1264-1267)
- **Save Allowed**: SAVING_SPELL
- **Source**: magic.cpp:1278, magic.cpp:1281-1282, magic.cpp:1276-1277, magic.cpp:1279-1284
#### Blizzards Of St. Augustine (ID 259)

- **Type**: SPELL
- **Enum**: `SPELL_BLIZZARDS_OF_SAINT_AUGUSTINE`
- **Command**: `blizzards of st. augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner cold subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Bone Armor (ID 187)

- **Type**: SPELL
- **Enum**: `SPELL_BONE_ARMOR`
- **Command**: `bone armor`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "&3Your skin returns to normal.&0"
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No


**Implementation**:
- **Duration**: `8 + 2 * (skill / 5)` (8-48 hours)
- **Source**: magic.cpp:1313
#### Bone Cage (ID 226)

- **Type**: SPELL
- **Enum**: `SPELL_BONE_CAGE`
- **Command**: `bone cage`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: PHYSICAL_SLASH
- **Wearoff Message**: "The bones holding you down crumble to dust."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Circle Of Light (ID 115)

- **Type**: SPELL
- **Enum**: `SPELL_CIRCLE_OF_LIGHT`
- **Command**: `circle of light`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "The circle of light above you fades out."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Clarity (ID 267)

- **Type**: SPELL
- **Enum**: `SPELL_CLARITY`
- **Command**: `clarity`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your mind returns to its normal state."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Cloak Of Gaia (ID 149)

- **Type**: SPELL
- **Enum**: `SPELL_GAIAS_CLOAK`
- **Command**: `cloak of gaia`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your shroud of nature dissolves."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Coldshield (ID 106)

- **Type**: SPELL
- **Enum**: `SPELL_COLDSHIELD`
- **Command**: `coldshield`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Damage Type**: COLD
- **Wearoff Message**: "The ice formation around your body melts."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No


**Implementation**:
- **Duration**: `skill / 20` (0-5 hours)
- **Source**: magic.cpp:1386
#### Confusion (ID 219)

- **Type**: SPELL
- **Enum**: `SPELL_CONFUSION`
- **Command**: `confusion`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "You no longer feel so confused."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes


**Implementation**:
- **Duration**: `2 + skill / 40` (2-4 hours)
- **Special Mechanics**:
  - Character has difficulty focusing on foes
  - May attack wrong targets
- **Save Allowed**: unknown
  - Bonus: `(GET_DEX(victim) + GET_WIS(victim) - 100) / 10`
- **Source**: magic.cpp:1405, magic.cpp:1404
#### Dark Presence (ID 131)

- **Type**: SPELL
- **Enum**: `SPELL_DARK_PRESENCE`
- **Command**: `dark presence`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel the dark presence leave you."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No


**Implementation**:
- **Duration**: `10 + (skill / 7)` (10-24 hours)
- **Source**: magic.cpp:1465
#### Demonic Aspect (ID 137)

- **Type**: SPELL
- **Enum**: `SPELL_DEMONIC_ASPECT`
- **Command**: `demonic aspect`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "The demon within you fades away."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No


**Implementation**:
- **Duration**: `5 + (skill / 20)` (5-10 hours)
- **Source**: magic.cpp:1513
#### Demonic Mutation (ID 140)

- **Type**: SPELL
- **Enum**: `SPELL_DEMONIC_MUTATION`
- **Command**: `demonic mutation`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "You mutate back to your original form."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Demonskin (ID 132)

- **Type**: SPELL
- **Enum**: `SPELL_DEMONSKIN`
- **Command**: `demonskin`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your skin reverts back to normal."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Detect Alignment (ID 18)

- **Type**: SPELL
- **Enum**: `SPELL_DETECT_ALIGN`
- **Command**: `detect alignment`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less aware."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No


**Implementation**:
- **Duration**: `5 + (skill / 10)` (5-15 hours)
- **Special Mechanics**:
  - Can see alignment auras on characters
- **Source**: magic.cpp:1570, magic.cpp:1569
#### Detect Invisibility (ID 19)

- **Type**: SPELL
- **Enum**: `SPELL_DETECT_INVIS`
- **Command**: `detect invisibility`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your eyes stop tingling."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No


**Implementation**:
- **Duration**: `5 + (skill / 10)` (5-15 hours)
- **Effects**:
  - **APPLY_PERCEPTION**: varies
- **Special Mechanics**:
  - Can see invisible characters and objects
- **Source**: magic.cpp:1581, magic.cpp:1579, magic.cpp:1578
#### Detect Magic (ID 20)

- **Type**: SPELL
- **Enum**: `SPELL_DETECT_MAGIC`
- **Command**: `detect magic`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "The detect magic wears off."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Detect Poison (ID 21)

- **Type**: SPELL
- **Enum**: `SPELL_DETECT_POISON`
- **Command**: `detect poison`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel slightly less aware."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Disease (ID 135)

- **Type**: SPELL
- **Enum**: `SPELL_DISEASE`
- **Command**: `disease`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Wearoff Message**: "You are cured of your disease!"
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Displacement (ID 264)

- **Type**: SPELL
- **Enum**: `SPELL_DISPLACEMENT`
- **Command**: `displacement`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Damage Type**: MENTAL
- **Wearoff Message**: "&9&bYour image solidifies.&0"
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Earth Blessing (ID 247)

- **Type**: SPELL
- **Enum**: `SPELL_EARTH_BLESSING`
- **Command**: `earth blessing`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less righteous."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Elemental Warding (ID 118)

- **Type**: SPELL
- **Enum**: `SPELL_ELEMENTAL_WARDING`
- **Command**: `elemental warding`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less safe from the elements."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Endurance (ID 207)

- **Type**: SPELL
- **Enum**: `SPELL_ENDURANCE`
- **Command**: `endurance`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your endurance returns to normal."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enhance Ability (ID 39)

- **Type**: SPELL
- **Enum**: `SPELL_ENHANCE_ABILITY`
- **Command**: `enhance ability`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less enhanced."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enhance Charisma (ID 257)

- **Type**: SPELL
- **Enum**: `SPELL_ENHANCE_CHA`
- **Command**: `enhance charisma`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel uglier."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enhance Constitution (ID 254)

- **Type**: SPELL
- **Enum**: `SPELL_ENHANCE_CON`
- **Command**: `enhance constitution`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less healthy."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enhance Dexterity (ID 253)

- **Type**: SPELL
- **Enum**: `SPELL_ENHANCE_DEX`
- **Command**: `enhance dexterity`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel slower."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enhance Intelligence (ID 255)

- **Type**: SPELL
- **Enum**: `SPELL_ENHANCE_INT`
- **Command**: `enhance intelligence`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel dumber."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enhance Strength (ID 252)

- **Type**: SPELL
- **Enum**: `SPELL_ENHANCE_STR`
- **Command**: `enhance strength`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel weaker."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enhance Wisdom (ID 256)

- **Type**: SPELL
- **Enum**: `SPELL_ENHANCE_WIS`
- **Command**: `enhance wisdom`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less witty."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Enlarge (ID 185)

- **Type**: SPELL
- **Enum**: `SPELL_ENLARGE`
- **Command**: `enlarge`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You return to your normal size.&0"
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Entangle (ID 151)

- **Type**: SPELL
- **Enum**: `SPELL_ENTANGLE`
- **Command**: `entangle`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "You break free of the vines."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Familiarity (ID 195)

- **Type**: SPELL
- **Enum**: `SPELL_FAMILIARITY`
- **Command**: `familiarity`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your familiar disguise melts away."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Farsee (ID 67)

- **Type**: SPELL
- **Enum**: `SPELL_FARSEE`
- **Command**: `farsee`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your pupils dilate as your vision returns to normal."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Feather Fall (ID 103)

- **Type**: SPELL
- **Enum**: `SPELL_FEATHER_FALL`
- **Command**: `feather fall`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You float back to the ground."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Fires Of St. Augustine (ID 258)

- **Type**: SPELL
- **Enum**: `SPELL_FIRES_OF_SAINT_AUGUSTINE`
- **Command**: `fires of st. augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner fire subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Fireshield (ID 105)

- **Type**: SPELL
- **Enum**: `SPELL_FIRESHIELD`
- **Command**: `fireshield`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Damage Type**: FIRE
- **Wearoff Message**: "The flames around your body dissipate."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Fly (ID 57)

- **Type**: SPELL
- **Enum**: `SPELL_FLY`
- **Command**: `fly`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel the weight of your body return."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Glory (ID 190)

- **Type**: SPELL
- **Enum**: `SPELL_GLORY`
- **Command**: `glory`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your visage becomes plain once again."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Greater Displacement (ID 265)

- **Type**: SPELL
- **Enum**: `SPELL_GREATER_DISPLACEMENT`
- **Command**: `greater displacement`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Damage Type**: MENTAL
- **Wearoff Message**: "&9&bYour image solidifies.&0"
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Greater Endurance (ID 70)

- **Type**: SPELL
- **Enum**: `SPELL_GREATER_ENDURANCE`
- **Command**: `greater endurance`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your endurance returns to normal."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Greater Vitality (ID 209)

- **Type**: SPELL
- **Enum**: `SPELL_GREATER_VITALITY`
- **Command**: `greater vitality`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your magical vitality drains away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Harness (ID 110)

- **Type**: SPELL
- **Enum**: `SPELL_HARNESS`
- **Command**: `harness`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "&4The harnessed power in your body fades.&0"
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Haste (ID 68)

- **Type**: SPELL
- **Enum**: `SPELL_HASTE`
- **Command**: `haste`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your pulse returns to normal."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No


**Implementation**:
- **Duration**: `2 + (skill / 21)` (2-6 hours)
- **Special Mechanics**:
  - Grants +1 attack per round
  - Stacks with double attack skill
- **Attack Bonus**: `hits += 1`
- **Source**: magic.cpp:2112, fight.cpp:2397-2398, fight.cpp:2397
#### Ice Armor (ID 172)

- **Type**: SPELL
- **Enum**: `SPELL_ICE_ARMOR`
- **Command**: `ice armor`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your iced encasing melts away, leaving you vulnerable again."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Infravision (ID 50)

- **Type**: SPELL
- **Enum**: `SPELL_INFRAVISION`
- **Command**: `infravision`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your night vision seems to fade."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Insanity (ID 136)

- **Type**: SPELL
- **Enum**: `SPELL_INSANITY`
- **Command**: `insanity`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "Your mind returns to reality."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Invisibility (ID 29)

- **Type**: SPELL
- **Enum**: `SPELL_INVISIBLE`
- **Command**: `invisibility`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You fade back into view."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Lesser Endurance (ID 206)

- **Type**: SPELL
- **Enum**: `SPELL_LESSER_ENDURANCE`
- **Command**: `lesser endurance`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your endurance returns to normal."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Magic Torch (ID 158)

- **Type**: SPELL
- **Enum**: `SPELL_MAGIC_TORCH`
- **Command**: `magic torch`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your magic torch peters out."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Major Globe (ID 108)

- **Type**: SPELL
- **Enum**: `SPELL_MAJOR_GLOBE`
- **Command**: `major globe`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "The globe of force surrounding you dissipates."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Mesmerize (ID 197)

- **Type**: SPELL
- **Enum**: `SPELL_MESMERIZE`
- **Command**: `mesmerize`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "You regain your senses."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: Yes

#### Minor Globe (ID 107)

- **Type**: SPELL
- **Enum**: `SPELL_MINOR_GLOBE`
- **Command**: `minor globe`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "The globe around your body fades out."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Minor Paralysis (ID 83)

- **Type**: SPELL
- **Enum**: `SPELL_MINOR_PARALYSIS`
- **Command**: `minor paralysis`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "Your muscles regain feeling."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Mirage (ID 160)

- **Type**: SPELL
- **Enum**: `SPELL_MIRAGE`
- **Command**: `mirage`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "You become more visible as the heat around your body dies out."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Misdirection (ID 218)

- **Type**: SPELL
- **Enum**: `SPELL_MISDIRECTION`
- **Command**: `misdirection`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Damage Type**: MENTAL
- **Wearoff Message**: "You no longer feel like you're going every which way at once."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Natures Embrace (ID 150)

- **Type**: SPELL
- **Enum**: `SPELL_NATURES_EMBRACE`
- **Command**: `natures embrace`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Nature releases you from her embrace."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Natures Guidance (ID 214)

- **Type**: SPELL
- **Enum**: `SPELL_NATURES_GUIDANCE`
- **Command**: `natures guidance`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "You suddenly feel a little unguided."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Negate Cold (ID 180)

- **Type**: SPELL
- **Enum**: `SPELL_NEGATE_COLD`
- **Command**: `negate cold`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "You feel vulnerable to the cold again."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Negate Heat (ID 169)

- **Type**: SPELL
- **Enum**: `SPELL_NEGATE_HEAT`
- **Command**: `negate heat`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your immunity to heat has passed."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Night Vision (ID 145)

- **Type**: SPELL
- **Enum**: `SPELL_NIGHT_VISION`
- **Command**: `night vision`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your night vision fades out."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No

#### Nimble (ID 266)

- **Type**: SPELL
- **Enum**: `SPELL_NIMBLE`
- **Command**: `nimble`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your movements slow to normal."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Poison (ID 33)

- **Type**: SPELL
- **Enum**: `SPELL_POISON`
- **Command**: `poison`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less sick."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Prayer (ID 117)

- **Type**: SPELL
- **Enum**: `SPELL_PRAYER`
- **Command**: `prayer`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your holy prayer fades."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Protection From Air (ID 251)

- **Type**: SPELL
- **Enum**: `SPELL_PROTECT_SHOCK`
- **Command**: `protection from air`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less protected from air."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Protection From Cold (ID 249)

- **Type**: SPELL
- **Enum**: `SPELL_PROTECT_COLD`
- **Command**: `protection from cold`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less protected from cold."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Protection From Earth (ID 250)

- **Type**: SPELL
- **Enum**: `SPELL_PROTECT_ACID`
- **Command**: `protection from earth`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less protected from earth."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Protection From Evil (ID 34)

- **Type**: SPELL
- **Enum**: `SPELL_PROT_FROM_EVIL`
- **Command**: `protection from evil`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less protected."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Protection From Fire (ID 248)

- **Type**: SPELL
- **Enum**: `SPELL_PROTECT_FIRE`
- **Command**: `protection from fire`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less protected from fire."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Protection From Good (ID 235)

- **Type**: SPELL
- **Enum**: `SPELL_PROT_FROM_GOOD`
- **Command**: `protection from good`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel less protected."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Ray Of Enfeeblement (ID 102)

- **Type**: SPELL
- **Enum**: `SPELL_RAY_OF_ENFEEB`
- **Command**: `ray of enfeeblement`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your strength returns to you."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Rebuke Undead (ID 211)

- **Type**: SPELL
- **Enum**: `SPELL_REBUKE_UNDEAD`
- **Command**: `rebuke undead`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Reduce (ID 184)

- **Type**: SPELL
- **Enum**: `SPELL_REDUCE`
- **Command**: `reduce`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "You return to your normal size.&0"
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Sanctuary (ID 36)

- **Type**: SPELL
- **Enum**: `SPELL_SANCTUARY`
- **Command**: `sanctuary`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "The white aura around your body fades."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No


**Implementation**:
- **Special Mechanics**:
  - Damage reduction: 50% (dam >>= 1)
  - Prevents charming (spells.cpp:243-244)
  - Visual aura: black for evil, white for good (act.informative.cpp:625-630)
- **Damage Reduction**: 50% (`dam >>= 1`)
- **Source**: fight.cpp:1650-1651, fight.cpp:1650, spells.cpp:243, act.informative.cpp:625
#### Sense Life (ID 44)

- **Type**: SPELL
- **Enum**: `SPELL_SENSE_LIFE`
- **Command**: `sense life`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "You feel less aware of your surroundings."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Silence (ID 94)

- **Type**: SPELL
- **Enum**: `SPELL_SILENCE`
- **Command**: `silence`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Wearoff Message**: "You can speak again."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Sleep (ID 38)

- **Type**: SPELL
- **Enum**: `SPELL_SLEEP`
- **Command**: `sleep`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "You feel less tired."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Smoke (ID 159)

- **Type**: SPELL
- **Enum**: `SPELL_SMOKE`
- **Command**: `smoke`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Wearoff Message**: "As the smoke clears, your vision returns."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Soulshield (ID 92)

- **Type**: SPELL
- **Enum**: `SPELL_SOULSHIELD`
- **Command**: `soulshield`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "The aura guarding your body fades away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Speak In Tongues (ID 122)

- **Type**: SPELL
- **Enum**: `SPELL_SPEAK_IN_TONGUES`
- **Command**: `speak in tongues`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your vocabulary diminishes drastically."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Spinechiller (ID 125)

- **Type**: SPELL
- **Enum**: `SPELL_SPINECHILLER`
- **Command**: `spinechiller`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Wearoff Message**: "The tingling in your spine subsides."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Stone Skin (ID 52)

- **Type**: SPELL
- **Enum**: `SPELL_STONE_SKIN`
- **Command**: `stone skin`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "&3&dYour skin softens and returns to normal.&0"
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No


**Implementation**:
- **Duration**: `2` (2 hours fixed)
- **Effects**:
  - **APPLY_NONE**: `7 + (skill / 16)` (7-13 (modifier tracks charges)) - Modifier decreases by 1 each hit until 0
- **Special Mechanics**:
  - Damage reduction: 50% (dam >>= 1)
  - Charge-based: 90% chance per hit to reduce modifier by 1
  - When modifier reaches 0, effect ends early
  - Only 10% chance hit damages normally (fight.cpp:2267-2270)
- **Damage Reduction**:  (`dam >>= 1 (if stone skin active)`)
- **Source**: magic.cpp:2855, magic.cpp:2853-2854, fight.cpp:1650, fight.cpp:2267-2270, fight.cpp:1650, fight.cpp:2267, act.informative.cpp:592
#### Tempest Of St. Augustine (ID 261)

- **Type**: SPELL
- **Enum**: `SPELL_TEMPEST_OF_SAINT_AUGUSTINE`
- **Command**: `tempest of st. augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner storm subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Tremors Of St. Augustine (ID 260)

- **Type**: SPELL
- **Enum**: `SPELL_TREMORS_OF_SAINT_AUGUSTINE`
- **Command**: `tremors of st. augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner earth subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Vaporform (ID 179)

- **Type**: SPELL
- **Enum**: `SPELL_VAPORFORM`
- **Command**: `vaporform`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your form condenses into flesh once again."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Vitality (ID 208)

- **Type**: SPELL
- **Enum**: `SPELL_VITALITY`
- **Command**: `vitality`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your magical vitality drains away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Waterform (ID 181)

- **Type**: SPELL
- **Enum**: `SPELL_WATERFORM`
- **Command**: `waterform`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your form solidifies into flesh once again."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Waterwalk (ID 51)

- **Type**: SPELL
- **Enum**: `SPELL_WATERWALK`
- **Command**: `waterwalk`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your feet seem less buoyant."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Web (ID 246)

- **Type**: SPELL
- **Enum**: `SPELL_WEB`
- **Command**: `web`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "The webs holding you in place dissolve."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Wings Of Heaven (ID 126)

- **Type**: SPELL
- **Enum**: `SPELL_WINGS_OF_HEAVEN`
- **Command**: `wings of heaven`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your wings gently fold back and fade away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Wings Of Hell (ID 141)

- **Type**: SPELL
- **Enum**: `SPELL_WINGS_OF_HELL`
- **Command**: `wings of hell`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your giant bat-like wings fold up and vanish."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No


### COMBAT

#### Bash (ID 402)

- **Type**: SKILL
- **Enum**: `SKILL_BASH`
- **Command**: `bash`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - ANTI_PALADIN: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - PALADIN: level 1
  - RANGER: level 1
  - WARRIOR: level 1
- **Implementation**: `act.offensive.cpp::do_bash`

#### Bind (ID 428)

- **Type**: SKILL
- **Enum**: `SKILL_BIND`
- **Command**: `bind`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - MERCENARY: level 16
- **Implementation**: `act.other.cpp::do_bind`

#### Bodyslam (ID 427)

- **Type**: SKILL
- **Enum**: `SKILL_BODYSLAM`
- **Command**: `bodyslam`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Implementation**: `act.offensive.cpp::unknown`

#### Cartwheel (ID 491)

- **Type**: SKILL
- **Enum**: `SKILL_CARTWHEEL`
- **Command**: `cartwheel`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - ROGUE: level 10
- **Implementation**: `act.offensive.cpp::do_cartwheel`

#### Claw (ID 480)

- **Type**: SKILL
- **Enum**: `SKILL_CLAW`
- **Command**: `claw`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Implementation**: `act.offensive.cpp::do_claw`

#### Disarm (ID 431)

- **Type**: SKILL
- **Enum**: `SKILL_DISARM`
- **Command**: `disarm`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 50
  - MERCENARY: level 20
  - PALADIN: level 50
  - THIEF: level 40
  - WARRIOR: level 20
- **Implementation**: `act.offensive.cpp::do_disarm`

#### Electrify (ID 481)

- **Type**: SKILL
- **Enum**: `SKILL_ELECTRIFY`
- **Command**: `electrify`
- **Target**: NONE
  - Scope: SAME_ROOM
  - Requires line of sight
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Implementation**: `act.offensive.cpp::do_electrify`

#### Eye Gouge (ID 472)

- **Type**: SKILL
- **Enum**: `SKILL_EYE_GOUGE`
- **Command**: `eye gouge`
- **Target**: NONE
  - Scope: TOUCH
- **Wearoff Message**: "Your vision returns."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - THIEF: level 10
- **Implementation**: `act.offensive.cpp::do_eye_gouge`

#### Hitall (ID 441)

- **Type**: SKILL
- **Enum**: `SKILL_HITALL`
- **Command**: `hitall`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - ANTI_PALADIN: level 80
  - PALADIN: level 80
  - WARRIOR: level 50
- **Implementation**: `act.offensive.cpp::do_hit`

#### Instant Kill (ID 440)

- **Type**: SKILL
- **Enum**: `SKILL_INSTANT_KILL`
- **Command**: `instant kill`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ASSASSIN: level 1
- **Implementation**: `act.offensive.cpp::do_kill`

#### Kick (ID 404)

- **Type**: SKILL
- **Enum**: `SKILL_KICK`
- **Command**: `kick`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 1
  - ASSASSIN: level 10
  - BERSERKER: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - MONK: level 1
  - PALADIN: level 1
  - RANGER: level 1
  - WARRIOR: level 1
- **Implementation**: `act.offensive.cpp::do_kick`

#### Maul (ID 485)

- **Type**: SKILL
- **Enum**: `SKILL_MAUL`
- **Command**: `maul`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - BERSERKER: level 65
- **Implementation**: `act.offensive.cpp::unknown`

#### Peck (ID 479)

- **Type**: SKILL
- **Enum**: `SKILL_PECK`
- **Command**: `peck`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Implementation**: `act.offensive.cpp::do_peck`

#### Punch (ID 406)

- **Type**: SKILL
- **Enum**: `SKILL_PUNCH`
- **Command**: `punch`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only

#### Rend (ID 494)

- **Type**: SKILL
- **Enum**: `SKILL_REND`
- **Command**: `rend`
- **Target**: NONE
  - Scope: TOUCH
- **Wearoff Message**: "You patch your defenses."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - THIEF: level 30
- **Implementation**: `act.offensive.cpp::do_rend`

#### Roundhouse (ID 495)

- **Type**: SKILL
- **Enum**: `SKILL_ROUNDHOUSE`
- **Command**: `roundhouse`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - MONK: level 65
- **Implementation**: `act.offensive.cpp::do_roundhouse`

#### Sweep (ID 436)

- **Type**: SKILL
- **Enum**: `SKILL_SWEEP`
- **Command**: `sweep`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Implementation**: `act.offensive.cpp::do_sweep`

#### Vampiric Touch (ID 445)

- **Type**: SKILL
- **Enum**: `SKILL_VAMP_TOUCH`
- **Command**: `vampiric touch`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - ANTI_PALADIN: level 45
- **Implementation**: `act.item.cpp::do_touch`


### DAMAGE

#### Acid Breath (ID 204)

- **Type**: SPELL
- **Enum**: `SPELL_ACID_BREATH`
- **Command**: `acid breath`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ACID
- **Damage Formula**: `skill + random_number(1, skill * 2)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Acid Burst (ID 170)

- **Type**: SPELL
- **Enum**: `SPELL_ACID_BURST`
- **Command**: `acid burst`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ACID
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Ancestral Vengeance (ID 236)

- **Type**: SPELL
- **Enum**: `SPELL_ANCESTRAL_VENGEANCE`
- **Command**: `ancestral vengeance`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ALIGN
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Baleful Polymorph (ID 238)

- **Type**: SPELL
- **Enum**: `SPELL_BALEFUL_POLYMORPH`
- **Command**: `baleful polymorph`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Bigbys Clenched Fist (ID 66)

- **Type**: SPELL
- **Enum**: `SPELL_BIGBYS_CLENCHED_FIST`
- **Command**: `bigbys clenched fist`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: PHYSICAL_BLUNT
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Blinding Beauty (ID 244)

- **Type**: SPELL
- **Enum**: `SPELL_BLINDING_BEAUTY`
- **Command**: `blinding beauty`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "You feel a cloak of blindness dissolve."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Burning Hands (ID 5)

- **Type**: SPELL
- **Enum**: `SPELL_BURNING_HANDS`
- **Command**: `burning hands`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Type**: FIRE
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Call Lightning (ID 6)

- **Type**: SPELL
- **Enum**: `SPELL_CALL_LIGHTNING`
- **Command**: `call lightning`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: SHOCK
- **Damage Formula**: `dam += (pow(skill, 2) * 7) / 400`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Cause Critical (ID 86)

- **Type**: SPELL
- **Enum**: `SPELL_CAUSE_CRITIC`
- **Command**: `cause critical`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Formula**: `dam += (pow(skill, 2) * 53) / 10000`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Cause Light (ID 84)

- **Type**: SPELL
- **Enum**: `SPELL_CAUSE_LIGHT`
- **Command**: `cause light`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Formula**: `dam += (pow(skill, 2) * 2) / 625`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Cause Serious (ID 85)

- **Type**: SPELL
- **Enum**: `SPELL_CAUSE_SERIOUS`
- **Command**: `cause serious`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Formula**: `dam += (pow(skill, 2) * 9) / 2000`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Chain Lightning (ID 111)

- **Type**: SPELL
- **Enum**: `SPELL_CHAIN_LIGHTNING`
- **Command**: `chain lightning`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: SHOCK
- **Damage Formula**: `dam += (pow(skill, 2) * 7) / 500`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Chill Touch (ID 8)

- **Type**: SPELL
- **Enum**: `SPELL_CHILL_TOUCH`
- **Command**: `chill touch`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Type**: COLD
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Wearoff Message**: "You feel your strength return."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Circle Of Death (ID 237)

- **Type**: SPELL
- **Enum**: `SPELL_CIRCLE_OF_DEATH`
- **Command**: `circle of death`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: MENTAL
- **Damage Formula**: `dam += (pow(skill, 2) * 7) / 500`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Cone Of Cold (ID 76)

- **Type**: SPELL
- **Enum**: `SPELL_CONE_OF_COLD`
- **Command**: `cone of cold`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: COLD
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Cremate (ID 168)

- **Type**: SPELL
- **Enum**: `SPELL_CREMATE`
- **Command**: `cremate`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Decay (ID 121)

- **Type**: SPELL
- **Enum**: `SPELL_DECAY`
- **Command**: `decay`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Destroy Undead (ID 93)

- **Type**: SPELL
- **Enum**: `SPELL_DESTROY_UNDEAD`
- **Command**: `destroy undead`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Detonation (ID 200)

- **Type**: SPELL
- **Enum**: `SPELL_DETONATION`
- **Command**: `detonation`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: PHYSICAL_BLUNT
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Discorporate (ID 193)

- **Type**: SPELL
- **Enum**: `SPELL_DISCORPORATE`
- **Command**: `discorporate`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Disintegrate (ID 109)

- **Type**: SPELL
- **Enum**: `SPELL_DISINTEGRATE`
- **Command**: `disintegrate`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ACID
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Dispel Evil (ID 22)

- **Type**: SPELL
- **Enum**: `SPELL_DISPEL_EVIL`
- **Command**: `dispel evil`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Dispel Good (ID 46)

- **Type**: SPELL
- **Enum**: `SPELL_DISPEL_GOOD`
- **Command**: `dispel good`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Divine Bolt (ID 116)

- **Type**: SPELL
- **Enum**: `SPELL_DIVINE_BOLT`
- **Command**: `divine bolt`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Divine Ray (ID 119)

- **Type**: SPELL
- **Enum**: `SPELL_DIVINE_RAY`
- **Command**: `divine ray`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Doom (ID 64)

- **Type**: SPELL
- **Enum**: `SPELL_DOOM`
- **Command**: `doom`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: ACID
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Earthquake (ID 23)

- **Type**: SPELL
- **Enum**: `SPELL_EARTHQUAKE`
- **Command**: `earthquake`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: ACID
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Exorcism (ID 124)

- **Type**: SPELL
- **Enum**: `SPELL_EXORCISM`
- **Command**: `exorcism`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Fire Breath (ID 201)

- **Type**: SPELL
- **Enum**: `SPELL_FIRE_BREATH`
- **Command**: `fire breath`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Damage Formula**: `skill + random_number(1, skill * 2)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Fireball (ID 26)

- **Type**: SPELL
- **Enum**: `SPELL_FIREBALL`
- **Command**: `fireball`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: FIRE
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Firestorm (ID 163)

- **Type**: SPELL
- **Enum**: `SPELL_FIRESTORM`
- **Command**: `firestorm`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Flamestrike (ID 95)

- **Type**: SPELL
- **Enum**: `SPELL_FLAMESTRIKE`
- **Command**: `flamestrike`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Freeze (ID 175)

- **Type**: SPELL
- **Enum**: `SPELL_FREEZE`
- **Command**: `freeze`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: COLD
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Freezing Wind (ID 174)

- **Type**: SPELL
- **Enum**: `SPELL_FREEZING_WIND`
- **Command**: `freezing wind`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: COLD
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Frost Breath (ID 203)

- **Type**: SPELL
- **Enum**: `SPELL_FROST_BREATH`
- **Command**: `frost breath`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: COLD
- **Damage Formula**: `skill + random_number(1, skill * 2)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Full Harm (ID 54)

- **Type**: SPELL
- **Enum**: `SPELL_FULL_HARM`
- **Command**: `full harm`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Formula**: `dam += (pow(skill, 2) * 1) / 50`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Gas Breath (ID 202)

- **Type**: SPELL
- **Enum**: `SPELL_GAS_BREATH`
- **Command**: `gas breath`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: POISON
- **Damage Formula**: `skill + random_number(1, skill * 2)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Harm (ID 27)

- **Type**: SPELL
- **Enum**: `SPELL_HARM`
- **Command**: `harm`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Formula**: `dam += (pow(skill, 2) * 41) / 5000`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Hell Bolt (ID 134)

- **Type**: SPELL
- **Enum**: `SPELL_HELL_BOLT`
- **Command**: `hell bolt`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Hellfire And Brimstone (ID 138)

- **Type**: SPELL
- **Enum**: `SPELL_HELLFIRE_BRIMSTONE`
- **Command**: `hellfire and brimstone`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Holy Word (ID 97)

- **Type**: SPELL
- **Enum**: `SPELL_HOLY_WORD`
- **Command**: `holy word`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Ice Shards (ID 78)

- **Type**: SPELL
- **Enum**: `SPELL_ICE_SHARDS`
- **Command**: `ice shards`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: COLD
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Ice Storm (ID 77)

- **Type**: SPELL
- **Enum**: `SPELL_ICE_STORM`
- **Command**: `ice storm`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: COLD
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Iceball (ID 177)

- **Type**: SPELL
- **Enum**: `SPELL_ICEBALL`
- **Command**: `iceball`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: COLD
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Incendiary Nebula (ID 82)

- **Type**: SPELL
- **Enum**: `SPELL_INCENDIARY_NEBULA`
- **Command**: `incendiary nebula`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Iron Maiden (ID 223)

- **Type**: SPELL
- **Enum**: `SPELL_IRON_MAIDEN`
- **Command**: `iron maiden`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: PHYSICAL_PIERCE
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Lesser Exorcism (ID 120)

- **Type**: SPELL
- **Enum**: `SPELL_LESSER_EXORCISM`
- **Command**: `lesser exorcism`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Lightning Bolt (ID 30)

- **Type**: SPELL
- **Enum**: `SPELL_LIGHTNING_BOLT`
- **Command**: `lightning bolt`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: SHOCK
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Lightning Breath (ID 205)

- **Type**: SPELL
- **Enum**: `SPELL_LIGHTNING_BREATH`
- **Command**: `lightning breath`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: SHOCK
- **Damage Formula**: `skill + random_number(1, skill * 2)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Melt (ID 164)

- **Type**: SPELL
- **Enum**: `SPELL_MELT`
- **Command**: `melt`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Meteorswarm (ID 65)

- **Type**: SPELL
- **Enum**: `SPELL_METEORSWARM`
- **Command**: `meteorswarm`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Nightmare (ID 192)

- **Type**: SPELL
- **Enum**: `SPELL_NIGHTMARE`
- **Command**: `nightmare`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: MENTAL
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Positive Field (ID 162)

- **Type**: SPELL
- **Enum**: `SPELL_POSITIVE_FIELD`
- **Command**: `positive field`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: SHOCK
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Severance (ID 198)

- **Type**: SPELL
- **Enum**: `SPELL_SEVERANCE`
- **Command**: `severance`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Shocking Grasp (ID 37)

- **Type**: SPELL
- **Enum**: `SPELL_SHOCKING_GRASP`
- **Command**: `shocking grasp`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Damage Type**: SHOCK
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Soul Reaver (ID 199)

- **Type**: SPELL
- **Enum**: `SPELL_SOUL_REAVER`
- **Command**: `soul reaver`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Spirit Ray (ID 239)

- **Type**: SPELL
- **Enum**: `SPELL_SPIRIT_RAY`
- **Command**: `spirit ray`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ALIGN
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Stygian Eruption (ID 139)

- **Type**: SPELL
- **Enum**: `SPELL_STYGIAN_ERUPTION`
- **Command**: `stygian eruption`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Sunray (ID 155)

- **Type**: SPELL
- **Enum**: `SPELL_SUNRAY`
- **Command**: `sunray`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: FIRE
- **Wearoff Message**: "Your vision has returned."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Supernova (ID 167)

- **Type**: SPELL
- **Enum**: `SPELL_SUPERNOVA`
- **Command**: `supernova`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Unholy Word (ID 96)

- **Type**: SPELL
- **Enum**: `SPELL_UNHOLY_WORD`
- **Command**: `unholy word`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Vampiric Breath (ID 80)

- **Type**: SPELL
- **Enum**: `SPELL_VAMPIRIC_BREATH`
- **Command**: `vampiric breath`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Vicious Mockery (ID 240)

- **Type**: SPELL
- **Enum**: `SPELL_VICIOUS_MOCKERY`
- **Command**: `vicious mockery`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: MENTAL
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Water Blast (ID 263)

- **Type**: SPELL
- **Enum**: `SPELL_WATER_BLAST`
- **Command**: `water blast`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Writhing Weeds (ID 146)

- **Type**: SPELL
- **Enum**: `SPELL_WRITHING_WEEDS`
- **Command**: `writhing weeds`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: ACID
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes


### DEFENSIVE

#### Bandage (ID 443)

- **Type**: SKILL
- **Enum**: `SKILL_BANDAGE`
- **Command**: `bandage`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Implementation**: `act.other.cpp::do_bandage`

#### Berserk (ID 413)

- **Type**: SKILL
- **Enum**: `SKILL_BERSERK`
- **Command**: `berserk`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - BERSERKER: level 10
- **Implementation**: `act.offensive.cpp::do_berserk`

#### Breathe Acid (ID 488)

- **Type**: SKILL
- **Enum**: `SKILL_BREATHE_ACID`
- **Command**: `breathe acid`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.offensive.cpp::do_breathe`

#### Breathe Fire (ID 486)

- **Type**: SKILL
- **Enum**: `SKILL_BREATHE_FIRE`
- **Command**: `breathe fire`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.offensive.cpp::do_breathe`

#### Breathe Frost (ID 487)

- **Type**: SKILL
- **Enum**: `SKILL_BREATHE_FROST`
- **Command**: `breathe frost`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.offensive.cpp::do_breathe`

#### Breathe Gas (ID 489)

- **Type**: SKILL
- **Enum**: `SKILL_BREATHE_GAS`
- **Command**: `breathe gas`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.offensive.cpp::do_breathe`

#### Breathe Lightning (ID 435)

- **Type**: SKILL
- **Enum**: `SKILL_BREATHE_LIGHTNING`
- **Command**: `breathe lightning`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.offensive.cpp::do_breathe`

#### Corner (ID 475)

- **Type**: SKILL
- **Enum**: `SKILL_CORNER`
- **Command**: `corner`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - MONK: level 80
  - ROGUE: level 60
- **Implementation**: `act.offensive.cpp::do_corner`

#### Dodge (ID 421)

- **Type**: SKILL
- **Enum**: `SKILL_DODGE`
- **Command**: `dodge`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ANTI_PALADIN: level 1
  - ASSASSIN: level 1
  - BARD: level 20
  - BERSERKER: level 1
  - CLERIC: level 20
  - DIABOLIST: level 20
  - DRUID: level 20
  - HUNTER: level 1
  - MERCENARY: level 1
  - MONK: level 1
  - MYSTIC: level 1
  - PALADIN: level 1
  - PRIEST: level 20
  - RANGER: level 10
  - ROGUE: level 1
  - SHAMAN: level 1
  - THIEF: level 1
  - WARRIOR: level 1

#### Doorbash (ID 419)

- **Type**: SKILL
- **Enum**: `SKILL_DOORBASH`
- **Command**: `doorbash`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.offensive.cpp::do_bash`

#### Douse (ID 438)

- **Type**: SKILL
- **Enum**: `SKILL_DOUSE`
- **Command**: `douse`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.other.cpp::do_douse`

#### First Aid (ID 444)

- **Type**: SKILL
- **Enum**: `SKILL_FIRST_AID`
- **Command**: `first aid`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Implementation**: `act.other.cpp::do_first_aid`

#### Guard (ID 434)

- **Type**: SKILL
- **Enum**: `SKILL_GUARD`
- **Command**: `guard`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ANTI_PALADIN: level 10
  - BERSERKER: level 25
  - HUNTER: level 1
  - MERCENARY: level 10
  - PALADIN: level 10
  - RANGER: level 50
  - WARRIOR: level 25
- **Implementation**: `act.other.cpp::do_guard`

#### Lure (ID 492)

- **Type**: SKILL
- **Enum**: `SKILL_LURE`
- **Command**: `lure`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ROGUE: level 15
- **Implementation**: `act.offensive.cpp::do_lure`

#### Parry (ID 420)

- **Type**: SKILL
- **Enum**: `SKILL_PARRY`
- **Command**: `parry`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 20
  - ASSASSIN: level 40
  - BARD: level 40
  - BERSERKER: level 15
  - HUNTER: level 1
  - MERCENARY: level 30
  - MONK: level 10
  - PALADIN: level 20
  - RANGER: level 30
  - ROGUE: level 40
  - THIEF: level 30
  - WARRIOR: level 30

#### Rescue (ID 407)

- **Type**: SKILL
- **Enum**: `SKILL_RESCUE`
- **Command**: `rescue`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ANTI_PALADIN: level 10
  - BERSERKER: level 35
  - HUNTER: level 1
  - PALADIN: level 10
  - RANGER: level 35
  - WARRIOR: level 15
- **Implementation**: `act.offensive.cpp::do_rescue`

#### Riposte (ID 422)

- **Type**: SKILL
- **Enum**: `SKILL_RIPOSTE`
- **Command**: `riposte`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 40
  - BERSERKER: level 50
  - HUNTER: level 1
  - MERCENARY: level 60
  - MONK: level 20
  - PALADIN: level 40
  - RANGER: level 40
  - WARRIOR: level 40

#### Roar (ID 437)

- **Type**: SKILL
- **Enum**: `SKILL_ROAR`
- **Command**: `roar`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.offensive.cpp::do_roar`

#### Shapechange (ID 429)

- **Type**: SKILL
- **Enum**: `SKILL_SHAPECHANGE`
- **Command**: `shapechange`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - DRUID: level 1
- **Implementation**: `act.other.cpp::do_shapechange`

#### Tame (ID 417)

- **Type**: SKILL
- **Enum**: `SKILL_TAME`
- **Command**: `tame`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ANTI_PALADIN: level 1
  - DRUID: level 1
  - HUNTER: level 7
  - PALADIN: level 1
  - RANGER: level 1
  - SHAMAN: level 7
- **Implementation**: `act.movement.cpp::do_tame`


### HEALING

#### Dragons Health (ID 210)

- **Type**: SPELL
- **Enum**: `SPELL_DRAGONS_HEALTH`
- **Command**: `dragons health`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your health returns to normal."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required


### MOVEMENT

#### Group Retreat (ID 474)

- **Type**: SKILL
- **Enum**: `SKILL_GROUP_RETREAT`
- **Command**: `group retreat`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - MERCENARY: level 80
- **Implementation**: `act.offensive.cpp::do_retreat`

#### Hide (ID 403)

- **Type**: SKILL
- **Enum**: `SKILL_HIDE`
- **Command**: `hide`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ASSASSIN: level 1
  - BARD: level 10
  - ILLUSIONIST: level 20
  - MERCENARY: level 20
  - ROGUE: level 1
  - THIEF: level 1
- **Implementation**: `act.other.cpp::do_hide`

#### Mount (ID 415)

- **Type**: SKILL
- **Enum**: `SKILL_MOUNT`
- **Command**: `mount`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.movement.cpp::do_mount`

#### Retreat (ID 473)

- **Type**: SKILL
- **Enum**: `SKILL_RETREAT`
- **Command**: `retreat`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ANTI_PALADIN: level 60
  - MERCENARY: level 40
  - PALADIN: level 60
  - WARRIOR: level 60
- **Implementation**: `act.offensive.cpp::do_retreat`

#### Riding (ID 416)

- **Type**: SKILL
- **Enum**: `SKILL_RIDING`
- **Command**: `riding`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Safefall (ID 448)

- **Type**: SKILL
- **Enum**: `SKILL_SAFEFALL`
- **Command**: `safefall`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - MONK: level 1

#### Sneak (ID 408)

- **Type**: SKILL
- **Enum**: `SKILL_SNEAK`
- **Command**: `sneak`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ASSASSIN: level 1
  - BARD: level 10
  - ROGUE: level 1
  - THIEF: level 1

#### Sneak Attack (ID 493)

- **Type**: SKILL
- **Enum**: `SKILL_SNEAK_ATTACK`
- **Command**: `sneak attack`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ROGUE: level 1

#### Springleap (ID 414)

- **Type**: SKILL
- **Enum**: `SKILL_SPRINGLEAP`
- **Command**: `springleap`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - MONK: level 50
- **Implementation**: `act.offensive.cpp::do_springleap`

#### Summon Mount (ID 450)

- **Type**: SKILL
- **Enum**: `SKILL_SUMMON_MOUNT`
- **Command**: `summon mount`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 15
  - PALADIN: level 15
- **Implementation**: `act.movement.cpp::do_mount`

#### Track (ID 410)

- **Type**: SKILL
- **Enum**: `SKILL_TRACK`
- **Command**: `track`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ASSASSIN: level 10
  - BARD: level 50
  - HUNTER: level 36
  - MERCENARY: level 30
  - RANGER: level 1
  - ROGUE: level 30
  - THIEF: level 40


### STEALTH

#### Backstab (ID 401)

- **Type**: SKILL
- **Enum**: `SKILL_BACKSTAB`
- **Command**: `backstab`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ASSASSIN: level 1
  - BARD: level 1
  - ILLUSIONIST: level 15
  - MERCENARY: level 10
  - ROGUE: level 1
  - THIEF: level 1
- **Implementation**: `act.offensive.cpp::do_backstab`

#### Circle (ID 426)

- **Type**: SKILL
- **Enum**: `SKILL_CIRCLE`
- **Command**: `circle`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Conceal (ID 478)

- **Type**: SKILL
- **Enum**: `SKILL_CONCEAL`
- **Command**: `conceal`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ILLUSIONIST: level 10
  - ROGUE: level 25
  - THIEF: level 10
- **Implementation**: `act.item.cpp::do_conceal`

#### Pick Lock (ID 405)

- **Type**: SKILL
- **Enum**: `SKILL_PICK_LOCK`
- **Command**: `pick lock`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Classes**:
  - ASSASSIN: level 5
  - BARD: level 10
  - ROGUE: level 5
  - THIEF: level 1

#### Shadow (ID 477)

- **Type**: SKILL
- **Enum**: `SKILL_SHADOW`
- **Command**: `shadow`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ASSASSIN: level 40
  - ROGUE: level 60

#### Steal (ID 409)

- **Type**: SKILL
- **Enum**: `SKILL_STEAL`
- **Command**: `steal`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - BARD: level 10
  - THIEF: level 10
- **Implementation**: `act.other.cpp::do_steal`

#### Stealth (ID 476)

- **Type**: SKILL
- **Enum**: `SKILL_STEALTH`
- **Command**: `stealth`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ROGUE: level 50
  - THIEF: level 50
- **Implementation**: `act.other.cpp::do_steal`

#### Throatcut (ID 418)

- **Type**: SKILL
- **Enum**: `SKILL_THROATCUT`
- **Command**: `throatcut`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ASSASSIN: level 30
- **Implementation**: `act.offensive.cpp::do_throatcut`


### SUMMON

#### Animate Dead (ID 45)

- **Type**: SPELL
- **Enum**: `SPELL_ANIMATE_DEAD`
- **Command**: `animate dead`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your undead creation crumbles to dust."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Clone (ID 9)

- **Type**: SPELL
- **Enum**: `SPELL_CLONE`
- **Command**: `clone`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Phantasm (ID 216)

- **Type**: SPELL
- **Enum**: `SPELL_PHANTASM`
- **Command**: `phantasm`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your illusion dissolves into tiny multicolored lights that float away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Simulacrum (ID 217)

- **Type**: SPELL
- **Enum**: `SPELL_SIMULACRUM`
- **Command**: `simulacrum`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Wearoff Message**: "Your illusion dissolves into tiny multicolored lights that float away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Summon Demon (ID 60)

- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_DEMON`
- **Command**: `summon demon`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Summon Elemental (ID 59)

- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_ELEMENTAL`
- **Command**: `summon elemental`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Summon Greater Demon (ID 61)

- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_GREATER_DEMON`
- **Command**: `summon greater demon`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No


### TELEPORT

#### Teleport (ID 2)

- **Type**: SPELL
- **Enum**: `SPELL_TELEPORT`
- **Command**: `teleport`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### World Teleport (ID 228)

- **Type**: SPELL
- **Enum**: `SPELL_WORLD_TELEPORT`
- **Command**: `world teleport`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No


### UTILITY

#### Acid Fog (ID 245)

- **Type**: SPELL
- **Enum**: `SPELL_ACID_FOG`
- **Command**: `acid fog`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ACID
- **Damage Formula**: `dam += (pow(skill, 2) * 7) / 1250`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Apocalyptic Anthem (ID 609)

- **Type**: CHANT
- **Enum**: `CHANT_APOCALYPTIC_ANTHEM`
- **Command**: `apocalyptic anthem`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required
- **Implementation**: `spell_parser.cpp::unknown`

#### Aria Of Dissonance (ID 607)

- **Type**: CHANT
- **Enum**: `CHANT_ARIA_OF_DISSONANCE`
- **Command**: `aria of dissonance`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Wearoff Message**: "The dissonance stops ringing in your ears."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Armor Of Gaia (ID 156)

- **Type**: SPELL
- **Enum**: `SPELL_ARMOR_OF_GAIA`
- **Command**: `armor of gaia`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Ballad Of Tears (ID 557)

- **Type**: SONG
- **Enum**: `SONG_BALLAD_OF_TEARS`
- **Command**: `ballad of tears`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your nerves settle down as the terror leaves you."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Banish (ID 127)

- **Type**: SPELL
- **Enum**: `SPELL_BANISH`
- **Command**: `banish`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Battle Howl (ID 484)

- **Type**: SKILL
- **Enum**: `SKILL_BATTLE_HOWL`
- **Command**: `battle howl`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - BERSERKER: level 30

#### Battle Hymn (ID 602)

- **Type**: CHANT
- **Enum**: `CHANT_BATTLE_HYMN`
- **Command**: `battle hymn`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your rage fades away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Blizzards Of Saint Augustine (ID 616)

- **Type**: CHANT
- **Enum**: `CHANT_BLIZZARDS_OF_SAINT_AUGUSTINE`
- **Command**: `blizzards of saint augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner cold subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Chant (ID 446)

- **Type**: SKILL
- **Enum**: `SKILL_CHANT`
- **Command**: `chant`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - BERSERKER: level 25
  - MONK: level 15

#### Charm Person (ID 7)

- **Type**: SPELL
- **Enum**: `SPELL_CHARM`
- **Command**: `charm person`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "You feel more self-confident."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Circle Of Fire (ID 165)

- **Type**: SPELL
- **Enum**: `SPELL_CIRCLE_OF_FIRE`
- **Command**: `circle of fire`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Damage Formula**: `(skill / 2) + roll_dice(2, 3)`
- **Wearoff Message**: "&1&bThe &1&bfl&3am&1es&0 &1surrounding &1the area &9&bdie out&0."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: Yes

#### Cloud Of Daggers (ID 242)

- **Type**: SPELL
- **Enum**: `SPELL_CLOUD_OF_DAGGERS`
- **Command**: `cloud of daggers`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: PHYSICAL_SLASH
- **Damage Formula**: `dam += (pow(skill, 2) * 7) / 1250`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Color Spray (ID 10)

- **Type**: SPELL
- **Enum**: `SPELL_COLOR_SPRAY`
- **Command**: `color spray`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Formula**: `dam += (pow(skill, 2) * 1) / 200`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Concealment (ID 101)

- **Type**: SPELL
- **Enum**: `SPELL_CONCEALMENT`
- **Command**: `concealment`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "You fade back into view."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Control Weather (ID 11)

- **Type**: SPELL
- **Enum**: `SPELL_CONTROL_WEATHER`
- **Command**: `control weather`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Create Food (ID 12)

- **Type**: SPELL
- **Enum**: `SPELL_CREATE_FOOD`
- **Command**: `create food`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Create Spring (ID 147)

- **Type**: SPELL
- **Enum**: `SPELL_CREATE_SPRING`
- **Command**: `create spring`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Create Water (ID 13)

- **Type**: SPELL
- **Enum**: `SPELL_CREATE_WATER`
- **Command**: `create water`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Creeping Doom (ID 63)

- **Type**: SPELL
- **Enum**: `SPELL_CREEPING_DOOM`
- **Command**: `creeping doom`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: ACID
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Crown Of Madness (ID 555)

- **Type**: SONG
- **Enum**: `SONG_CROWN_OF_MADNESS`
- **Command**: `crown of madness`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your mind returns to reality."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Cure Blind (ID 14)

- **Type**: SPELL
- **Enum**: `SPELL_CURE_BLIND`
- **Command**: `cure blind`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Cure Critic (ID 15)

- **Type**: SPELL
- **Enum**: `SPELL_CURE_CRITIC`
- **Command**: `cure critic`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Cure Light (ID 16)

- **Type**: SPELL
- **Enum**: `SPELL_CURE_LIGHT`
- **Command**: `cure light`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Cure Serious (ID 88)

- **Type**: SPELL
- **Enum**: `SPELL_CURE_SERIOUS`
- **Command**: `cure serious`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Dark Feast (ID 133)

- **Type**: SPELL
- **Enum**: `SPELL_DARK_FEAST`
- **Command**: `dark feast`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Darkness (ID 73)

- **Type**: SPELL
- **Enum**: `SPELL_DARKNESS`
- **Command**: `darkness`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "The magical darkness lifts."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Degeneration (ID 212)

- **Type**: SPELL
- **Enum**: `SPELL_DEGENERATION`
- **Command**: `degeneration`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Dimension Door (ID 62)

- **Type**: SPELL
- **Enum**: `SPELL_DIMENSION_DOOR`
- **Command**: `dimension door`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Dispel Magic (ID 99)

- **Type**: SPELL
- **Enum**: `SPELL_DISPEL_MAGIC`
- **Command**: `dispel magic`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: Yes

#### Divine Essence (ID 129)

- **Type**: SPELL
- **Enum**: `SPELL_DIVINE_ESSENCE`
- **Command**: `divine essence`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Double Attack (ID 412)

- **Type**: SKILL
- **Enum**: `SKILL_DOUBLE_ATTACK`
- **Command**: `double attack`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - ANTI_PALADIN: level 70
  - ASSASSIN: level 70
  - BARD: level 50
  - BERSERKER: level 85
  - HUNTER: level 1
  - MERCENARY: level 50
  - MONK: level 30
  - PALADIN: level 70
  - RANGER: level 60
  - ROGUE: level 70
  - THIEF: level 75
  - WARRIOR: level 35

#### Enchant Weapon (ID 24)

- **Type**: SPELL
- **Enum**: `SPELL_ENCHANT_WEAPON`
- **Command**: `enchant weapon`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Energy Drain (ID 25)

- **Type**: SPELL
- **Enum**: `SPELL_ENERGY_DRAIN`
- **Command**: `energy drain`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Enlightenment (ID 123)

- **Type**: SPELL
- **Enum**: `SPELL_ENLIGHTENMENT`
- **Command**: `enlightenment`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Enrapture (ID 553)

- **Type**: SONG
- **Enum**: `SONG_ENRAPTURE`
- **Command**: `enrapture`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "You regain your senses as the illusions subside."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Extinguish (ID 182)

- **Type**: SPELL
- **Enum**: `SPELL_EXTINGUISH`
- **Command**: `extinguish`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No

#### Fear (ID 114)

- **Type**: SPELL
- **Enum**: `SPELL_FEAR`
- **Command**: `fear`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "Your courage returns to you."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Fire Darts (ID 157)

- **Type**: SPELL
- **Enum**: `SPELL_FIRE_DARTS`
- **Command**: `fire darts`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: FIRE
- **Damage Formula**: `roll_dice(5, 18)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Fires Of Saint Augustine (ID 615)

- **Type**: CHANT
- **Enum**: `CHANT_FIRES_OF_SAINT_AUGUSTINE`
- **Command**: `fires of saint augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner fire subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Flame Blade (ID 161)

- **Type**: SPELL
- **Enum**: `SPELL_FLAME_BLADE`
- **Command**: `flame blade`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Flood (ID 178)

- **Type**: SPELL
- **Enum**: `SPELL_FLOOD`
- **Command**: `flood`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Fracture (ID 224)

- **Type**: SPELL
- **Enum**: `SPELL_FRACTURE`
- **Command**: `fracture`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: PHYSICAL_SLASH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Freedom Song (ID 559)

- **Type**: SONG
- **Enum**: `SONG_FREEDOM_SONG`
- **Command**: `freedom song`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your nerves settle down as the terror leaves you."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Full Heal (ID 53)

- **Type**: SPELL
- **Enum**: `SPELL_FULL_HEAL`
- **Command**: `full heal`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Ground Shaker (ID 483)

- **Type**: SKILL
- **Enum**: `SKILL_GROUND_SHAKER`
- **Command**: `ground shaker`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - BERSERKER: level 75

#### Group Armor (ID 47)

- **Type**: SPELL
- **Enum**: `SPELL_GROUP_ARMOR`
- **Command**: `group armor`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Group Heal (ID 48)

- **Type**: SPELL
- **Enum**: `SPELL_GROUP_HEAL`
- **Command**: `group heal`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Group Recall (ID 49)

- **Type**: SPELL
- **Enum**: `SPELL_GROUP_RECALL`
- **Command**: `group recall`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: ALIGN
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Heal (ID 28)

- **Type**: SPELL
- **Enum**: `SPELL_HEAL`
- **Command**: `heal`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Hearthsong (ID 554)

- **Type**: SONG
- **Enum**: `SONG_HEARTHSONG`
- **Command**: `hearthsong`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your familiar disguise melts away."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Heavens Gate (ID 130)

- **Type**: SPELL
- **Enum**: `SPELL_HEAVENS_GATE`
- **Command**: `heavens gate`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Hell Gate (ID 143)

- **Type**: SPELL
- **Enum**: `SPELL_HELLS_GATE`
- **Command**: `hell gate`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Heroic Journey (ID 558)

- **Type**: SONG
- **Enum**: `SONG_HEROIC_JOURNEY`
- **Command**: `heroic journey`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your inspiration fades."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Hunt (ID 442)

- **Type**: SKILL
- **Enum**: `SKILL_HUNT`
- **Command**: `hunt`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - HUNTER: level 41

#### Hymn Of Saint Augustine (ID 614)

- **Type**: CHANT
- **Enum**: `CHANT_HYMN_OF_SAINT_AUGUSTINE`
- **Command**: `hymn of saint augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner elements subside."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Hysteria (ID 196)

- **Type**: SPELL
- **Enum**: `SPELL_HYSTERIA`
- **Command**: `hysteria`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "Your courage returns to you."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Ice Dagger (ID 173)

- **Type**: SPELL
- **Enum**: `SPELL_ICE_DAGGER`
- **Command**: `ice dagger`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Ice Darts (ID 171)

- **Type**: SPELL
- **Enum**: `SPELL_ICE_DARTS`
- **Command**: `ice darts`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: COLD
- **Damage Formula**: `roll_dice(4, 21)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Identify (ID 186)

- **Type**: SPELL
- **Enum**: `SPELL_IDENTIFY`
- **Command**: `identify`
- **Target**: SINGLE_CHARACTER
  - Scope: TOUCH
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Illumination (ID 74)

- **Type**: SPELL
- **Enum**: `SPELL_ILLUMINATION`
- **Command**: `illumination`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "The magical light fades away.&0"
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Illusory Wall (ID 191)

- **Type**: SPELL
- **Enum**: `SPELL_ILLUSORY_WALL`
- **Command**: `illusory wall`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Wearoff Message**: "The wall dissolves into tiny motes of light..."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Immolate (ID 166)

- **Type**: SPELL
- **Enum**: `SPELL_IMMOLATE`
- **Command**: `immolate`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Inspiration (ID 551)

- **Type**: SONG
- **Enum**: `SONG_INSPIRATION`
- **Command**: `inspiration`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your inspiration fades."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Interminable Wrath (ID 613)

- **Type**: CHANT
- **Enum**: `CHANT_INTERMINABLE_WRATH`
- **Command**: `interminable wrath`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Invigorate (ID 152)

- **Type**: SPELL
- **Enum**: `SPELL_INVIGORATE`
- **Command**: `invigorate`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Isolation (ID 194)

- **Type**: SPELL
- **Enum**: `SPELL_ISOLATION`
- **Command**: `isolation`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "It is as if a veil has lifted from the surrounding area."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Ivory Symphony (ID 606)

- **Type**: CHANT
- **Enum**: `CHANT_IVORY_SYMPHONY`
- **Command**: `ivory symphony`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "Feeling returns to your limbs."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Implementation**: `spell_parser.cpp::unknown`

#### Joyful Noise (ID 560)

- **Type**: SONG
- **Enum**: `SONG_JOYFUL_NOISE`
- **Command**: `joyful noise`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Locate Object (ID 31)

- **Type**: SPELL
- **Enum**: `SPELL_LOCATE_OBJECT`
- **Command**: `locate object`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Magic Missile (ID 32)

- **Type**: SPELL
- **Enum**: `SPELL_MAGIC_MISSILE`
- **Command**: `magic missile`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: PHYSICAL_BLUNT
- **Damage Formula**: `roll_dice(4, 21)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Major Paralysis (ID 79)

- **Type**: SPELL
- **Enum**: `SPELL_MAJOR_PARALYSIS`
- **Command**: `major paralysis`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Wearoff Message**: "You can move again."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: Yes
- **Restrictions**: Quest required

#### Mass Invisibility (ID 112)

- **Type**: SPELL
- **Enum**: `SPELL_MASS_INVIS`
- **Command**: `mass invisibility`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Meditate (ID 423)

- **Type**: SKILL
- **Enum**: `SKILL_MEDITATE`
- **Command**: `meditate`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - BARD: level 1
  - BERSERKER: level 50

#### Minor Creation (ID 100)

- **Type**: SPELL
- **Enum**: `SPELL_MINOR_CREATION`
- **Command**: `minor creation`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Moonbeam (ID 215)

- **Type**: SPELL
- **Enum**: `SPELL_MOONBEAM`
- **Command**: `moonbeam`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Moonwell (ID 71)

- **Type**: SPELL
- **Enum**: `SPELL_MOONWELL`
- **Command**: `moonwell`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Nourishment (ID 148)

- **Type**: SPELL
- **Enum**: `SPELL_NOURISHMENT`
- **Command**: `nourishment`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Peace (ID 604)

- **Type**: CHANT
- **Enum**: `CHANT_PEACE`
- **Command**: `peace`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.other.cpp::do_peace`

#### Perform (ID 490)

- **Type**: SKILL
- **Enum**: `SKILL_PERFORM`
- **Command**: `perform`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Classes**:
  - BARD: level 1

#### Phosphoric Embers (ID 220)

- **Type**: SPELL
- **Enum**: `SPELL_PHOSPHORIC_EMBERS`
- **Command**: `phosphoric embers`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: FIRE
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Plane Shift (ID 98)

- **Type**: SPELL
- **Enum**: `SPELL_PLANE_SHIFT`
- **Command**: `plane shift`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Preserve (ID 87)

- **Type**: SPELL
- **Enum**: `SPELL_PRESERVE`
- **Command**: `preserve`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Pyre (ID 222)

- **Type**: SPELL
- **Enum**: `SPELL_PYRE`
- **Command**: `pyre`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Damage Type**: FIRE
- **Wearoff Message**: "The flames enveloping you die down."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Quick Chant (ID 424)

- **Type**: SKILL
- **Enum**: `SKILL_QUICK_CHANT`
- **Command**: `quick chant`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Rain (ID 183)

- **Type**: SPELL
- **Enum**: `SPELL_RAIN`
- **Command**: `rain`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Regeneration (ID 601)

- **Type**: CHANT
- **Enum**: `CHANT_REGENERATION`
- **Command**: `regeneration`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your healthy feeling subsides."
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Remove Curse (ID 35)

- **Type**: SPELL
- **Enum**: `SPELL_REMOVE_CURSE`
- **Command**: `remove curse`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Remove Paralysis (ID 241)

- **Type**: SPELL
- **Enum**: `SPELL_REMOVE_PARALYSIS`
- **Command**: `remove paralysis`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Remove Poison (ID 43)

- **Type**: SPELL
- **Enum**: `SPELL_REMOVE_POISON`
- **Command**: `remove poison`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No

#### Reveal Hidden (ID 243)

- **Type**: SPELL
- **Enum**: `SPELL_REVEAL_HIDDEN`
- **Command**: `reveal hidden`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Sane Mind (ID 142)

- **Type**: SPELL
- **Enum**: `SPELL_SANE_MIND`
- **Command**: `sane mind`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Scribe (ID 447)

- **Type**: SKILL
- **Enum**: `SKILL_SCRIBE`
- **Command**: `scribe`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Classes**:
  - BARD: level 1

#### Seed Of Destruction (ID 610)

- **Type**: CHANT
- **Enum**: `CHANT_SEED_OF_DESTRUCTION`
- **Command**: `seed of destruction`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Wearoff Message**: "The disease leaves you."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Quest required

#### Shadows Sorrow Song (ID 605)

- **Type**: CHANT
- **Enum**: `CHANT_SHADOWS_SORROW_SONG`
- **Command**: `shadows sorrow song`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "The shadows in your mind clear up."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Shift Corpse (ID 189)

- **Type**: SPELL
- **Enum**: `SPELL_SHIFT_CORPSE`
- **Command**: `shift corpse`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Sonata Of Malaise (ID 608)

- **Type**: CHANT
- **Enum**: `CHANT_SONATA_OF_MALAISE`
- **Command**: `sonata of malaise`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "The sonata of malaise stops echoing in your ears."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Song Of Rest (ID 556)

- **Type**: SONG
- **Enum**: `SONG_SONG_OF_REST`
- **Command**: `song of rest`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Wearoff Message**: "The restful song fades from your memory."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Implementation**: `act.movement.cpp::do_rest`

#### Soul Tap (ID 213)

- **Type**: SPELL
- **Enum**: `SPELL_SOUL_TAP`
- **Command**: `soul tap`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Spell Knowledge (ID 451)

- **Type**: SKILL
- **Enum**: `SKILL_KNOW_SPELL`
- **Command**: `spell knowledge`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Air (ID 456)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_AIR`
- **Command**: `sphere of air`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Death (ID 461)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_DEATH`
- **Command**: `sphere of death`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `act.item.cpp::do_eat`

#### Sphere Of Divination (ID 462)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_DIVIN`
- **Command**: `sphere of divination`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Earth (ID 455)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_EARTH`
- **Command**: `sphere of earth`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `spell_parser.cpp::unknown`

#### Sphere Of Enchantment (ID 459)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_ENCHANT`
- **Command**: `sphere of enchantment`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Fire (ID 453)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_FIRE`
- **Command**: `sphere of fire`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `spell_parser.cpp::unknown`

#### Sphere Of Generic (ID 452)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_GENERIC`
- **Command**: `sphere of generic`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Healing (ID 457)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_HEALING`
- **Command**: `sphere of healing`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Protection (ID 458)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_PROT`
- **Command**: `sphere of protection`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Summoning (ID 460)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_SUMMON`
- **Command**: `sphere of summoning`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Sphere Of Water (ID 454)

- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_WATER`
- **Command**: `sphere of water`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Implementation**: `spell_parser.cpp::unknown`

#### Spirit Arrows (ID 234)

- **Type**: SPELL
- **Enum**: `SPELL_SPIRIT_ARROWS`
- **Command**: `spirit arrows`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
  - Requires line of sight
- **Damage Type**: ALIGN
- **Damage Formula**: `roll_dice(4, 21)`
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Spirit Of The Bear (ID 612)

- **Type**: CHANT
- **Enum**: `CHANT_SPIRIT_BEAR`
- **Command**: `spirit of the bear`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your claws become decidedly less bear-like."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Spirit Of The Wolf (ID 611)

- **Type**: CHANT
- **Enum**: `CHANT_SPIRIT_WOLF`
- **Command**: `spirit of the wolf`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your fangs recede and you lose your wolf-like spirit."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

#### Summon (ID 40)

- **Type**: SPELL
- **Enum**: `SPELL_SUMMON`
- **Command**: `summon`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Summon Corpse (ID 188)

- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_CORPSE`
- **Command**: `summon corpse`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Switch (ID 430)

- **Type**: SKILL
- **Enum**: `SKILL_SWITCH`
- **Command**: `switch`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - ANTI_PALADIN: level 10
  - BERSERKER: level 1
  - HUNTER: level 1
  - MERCENARY: level 40
  - MONK: level 40
  - PALADIN: level 10
  - RANGER: level 10
  - WARRIOR: level 10

#### Tantrum (ID 482)

- **Type**: SKILL
- **Enum**: `SKILL_TANTRUM`
- **Command**: `tantrum`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - BERSERKER: level 45

#### Tempest Of Saint Augustine (ID 618)

- **Type**: CHANT
- **Enum**: `CHANT_TEMPEST_OF_SAINT_AUGUSTINE`
- **Command**: `tempest of saint augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner storm subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Terror (ID 552)

- **Type**: SONG
- **Enum**: `SONG_TERROR`
- **Command**: `terror`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your nerves settle down as the terror leaves you."
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

#### Tremors Of Saint Augustine (ID 617)

- **Type**: CHANT
- **Enum**: `CHANT_TREMORS_OF_SAINT_AUGUSTINE`
- **Command**: `tremors of saint augustine`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "Your inner earth subsides."
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Quest required

#### Urban Renewal (ID 154)

- **Type**: SPELL
- **Enum**: `SPELL_URBAN_RENEWAL`
- **Command**: `urban renewal`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "&2The woods in the surrounding area break apart and crumble.&0"
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Ventriloquate (ID 41)

- **Type**: SPELL
- **Enum**: `SPELL_VENTRILOQUATE`
- **Command**: `ventriloquate`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

#### Vigorize Critic (ID 91)

- **Type**: SPELL
- **Enum**: `SPELL_VIGORIZE_CRITIC`
- **Command**: `vigorize critic`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Vigorize Light (ID 89)

- **Type**: SPELL
- **Enum**: `SPELL_VIGORIZE_LIGHT`
- **Command**: `vigorize light`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Vigorize Serious (ID 90)

- **Type**: SPELL
- **Enum**: `SPELL_VIGORIZE_SERIOUS`
- **Command**: `vigorize serious`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Wall Of Fog (ID 55)

- **Type**: SPELL
- **Enum**: `SPELL_WALL_OF_FOG`
- **Command**: `wall of fog`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Wearoff Message**: "The fog seems to clear out."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Wall Of Ice (ID 176)

- **Type**: SPELL
- **Enum**: `SPELL_WALL_OF_ICE`
- **Command**: `wall of ice`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Damage Type**: COLD
- **Wearoff Message**: "The wall of ice melts away..."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Wall Of Stone (ID 56)

- **Type**: SPELL
- **Enum**: `SPELL_WALL_OF_STONE`
- **Command**: `wall of stone`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Wearoff Message**: "The wall of stone crumbles into dust."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Wandering Woods (ID 153)

- **Type**: SPELL
- **Enum**: `SPELL_WANDERING_WOODS`
- **Command**: `wandering woods`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "The woods around you shift back to their proper form."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### War Cry (ID 603)

- **Type**: CHANT
- **Enum**: `CHANT_WAR_CRY`
- **Command**: `war cry`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Wearoff Message**: "Your determination level returns to normal."
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

#### Wizard Eye (ID 104)

- **Type**: SPELL
- **Enum**: `SPELL_WIZARD_EYE`
- **Command**: `wizard eye`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Word Of Command (ID 128)

- **Type**: SPELL
- **Enum**: `SPELL_WORD_OF_COMMAND`
- **Command**: `word of command`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Damage Type**: MENTAL
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No
- **Restrictions**: Quest required

#### Word Of Recall (ID 42)

- **Type**: SPELL
- **Enum**: `SPELL_WORD_OF_RECALL`
- **Command**: `word of recall`
- **Target**: SELF
  - Scope: SAME_ROOM
  - Can target self
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No


### WEAPON_PROFICIENCY

#### 2H Bludgeoning Weapons (ID 466)

- **Type**: SKILL
- **Enum**: `SKILL_2H_BLUDGEONING`
- **Command**: `2H bludgeoning weapons`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 1
  - BERSERKER: level 1
  - CLERIC: level 1
  - DIABOLIST: level 1
  - DRUID: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - MYSTIC: level 1
  - PALADIN: level 1
  - PRIEST: level 1
  - RANGER: level 1
  - SHAMAN: level 1
  - WARRIOR: level 1

#### 2H Piercing Weapons (ID 467)

- **Type**: SKILL
- **Enum**: `SKILL_2H_PIERCING`
- **Command**: `2H piercing weapons`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 1
  - BERSERKER: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - PALADIN: level 1
  - RANGER: level 1
  - WARRIOR: level 1

#### 2H Slashing Weapons (ID 468)

- **Type**: SKILL
- **Enum**: `SKILL_2H_SLASHING`
- **Command**: `2H slashing weapons`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 1
  - BERSERKER: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - PALADIN: level 1
  - RANGER: level 1
  - WARRIOR: level 1

#### Barehand (ID 449)

- **Type**: SKILL
- **Enum**: `SKILL_BAREHAND`
- **Command**: `barehand`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Classes**:
  - MONK: level 1

#### Bludgeoning Weapons (ID 463)

- **Type**: SKILL
- **Enum**: `SKILL_BLUDGEONING`
- **Command**: `bludgeoning weapons`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 1
  - ASSASSIN: level 1
  - BARD: level 1
  - BERSERKER: level 1
  - CLERIC: level 1
  - DIABOLIST: level 1
  - DRUID: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - MONK: level 1
  - MYSTIC: level 1
  - PALADIN: level 1
  - PRIEST: level 1
  - RANGER: level 1
  - ROGUE: level 1
  - SHAMAN: level 1
  - THIEF: level 1
  - WARRIOR: level 1

#### Dual Wield (ID 411)

- **Type**: SKILL
- **Enum**: `SKILL_DUAL_WIELD`
- **Command**: `dual wield`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 20
  - ASSASSIN: level 15
  - BARD: level 70
  - BERSERKER: level 20
  - HUNTER: level 1
  - MERCENARY: level 15
  - MONK: level 1
  - PALADIN: level 20
  - RANGER: level 1
  - ROGUE: level 15
  - THIEF: level 15
  - WARRIOR: level 25
- **Implementation**: `act.item.cpp::do_wield`

#### Missile Weapons (ID 469)

- **Type**: SKILL
- **Enum**: `SKILL_MISSILE`
- **Command**: `missile weapons`
- **Target**: NONE
  - Scope: SAME_ROOM
  - Requires line of sight
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only

#### Piercing Weapons (ID 464)

- **Type**: SKILL
- **Enum**: `SKILL_PIERCING`
- **Command**: `piercing weapons`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 1
  - ASSASSIN: level 1
  - BARD: level 1
  - BERSERKER: level 1
  - DRUID: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - PALADIN: level 1
  - RANGER: level 1
  - ROGUE: level 1
  - SHAMAN: level 1
  - THIEF: level 1
  - WARRIOR: level 1

#### Slashing Weapons (ID 465)

- **Type**: SKILL
- **Enum**: `SKILL_SLASHING`
- **Command**: `slashing weapons`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes
- **Restrictions**: Humanoid only
- **Classes**:
  - ANTI_PALADIN: level 1
  - ASSASSIN: level 1
  - BARD: level 1
  - BERSERKER: level 1
  - DRUID: level 1
  - HUNTER: level 1
  - MERCENARY: level 1
  - MYSTIC: level 1
  - PALADIN: level 1
  - RANGER: level 1
  - ROGUE: level 1
  - SHAMAN: level 1
  - THIEF: level 1
  - WARRIOR: level 1


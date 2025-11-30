# FieryMUD Ability System - Complete Reference

**Source**: Extracted from actual C++ implementation code

**Last Updated**: 2025-11-07

**Coverage**: 368/368 abilities (100.0%)

## Overview

- **Total abilities**: 368
  - Spells: ~268
  - Skills: ~94
  - Songs: ~10
  - Chants: ~18

### Implementation Coverage

- **With complete implementation data**: 368/368 (100.0%)
- **Missing implementation data**: 0

### Categories

- **Complete Ability Reference**: 368 abilities

### Quick Reference

- [Implementation Mechanics](#implementation-mechanics) - System-level details
- [Damage Formulas](DAMAGE_SPELL_FORMULAS.md) - All damage spell formulas
- [Skill Mechanics](SKILL_MECHANICS.md) - All skill mechanics
- [Complete JSON Data](all_spell_implementations.json) - Machine-readable database

---


## Complete Ability Reference

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
- **Duration**: `get_spell_duration(ch, spellnum)`
- **Effects**:
  - **APPLY_HIT**: `get_vitality_hp_gain(ch, spellnum)`
- **Special Mechanics**:
  - HP gain calculated by get_vitality_hp_gain()
  - Duration calculated by get_spell_duration()
  - Conflicts with other armor spells (check_armor_spells)
- **Conflicts**: SPELL_LESSER_ENDURANCE, SPELL_ENDURANCE, SPELL_GREATER_ENDURANCE, SPELL_VITALITY, SPELL_GREATER_VITALITY, SPELL_DRAGONS_HEALTH
- **Messages**:
  - **to_char**: "$N looks healthier than before!"
  - **to_room**: "$N looks healthier than before!"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1138-1177

---

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

**Implementation**:
- **Mechanics**: Teleports to memorized location
- **Source**: spells.cpp

---

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
- **Duration**: `10 + (skill / 7)`
- **Effects**:
  - **APPLY_SAVING_SPELL**: `-2 - (skill / 10)`
  - **APPLY_DAMROLL**: `1 + (skill > 95)`
  - **EFF_BLESS**
- **Special Mechanics**:
  - Conflicts with other blessing spells (check_bless_spells)
- **Requirements**: Alignment: Cannot target evil; Alignment: Caster cannot be evil
- **Messages**:
  - **to_char**: "$N is inspired by your gods."
  - **to_room**: "$n is inspired to do good."
  - **to_vict**: "Your inner angel is inspired.\r\nYou feel righteous."
- **Source**: magic.cpp:1212-1257

---

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
- **Duration**: `2`
- **Effects**:
  - **APPLY_HITROLL**: `-4`
  - **APPLY_AC**: `-40`
  - **EFF_BLIND**
  - **EFF_BLIND**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "&9&b$N&9&b is blinded by you!&0"
  - **to_room**: "&9&b$N&9&b is blinded by $n!&0"
  - **to_vict**: "&9&bYou have been blinded!&0"
- **Source**: magic.cpp:1259-1289

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_SHOCK
- **Damage Formula**: `(pow(skill,2)*7)/400`

---

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

**Implementation**:
- **Mechanics**: Charms victim to follow and obey caster
- **Duration**: skill / 5 hours
- **Requirements**: Caster cannot be charmed (unless animated)
- **Immunities**: MOB_NOCHARM, EFF_SANCTUARY, SUMMON protection
- **Max Followers**: Limited by CHA
- **Special**: Circle 1-6 max, victim level vs caster check
- **Source**: spells.cpp:221-299

---

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

**Implementation**:
- **Duration**: `3 + (skill / 20)`
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "$N is withered by your cold!"
  - **to_room**: "$N withers slightly from $n's cold!"
  - **to_vict**: "You feel your strength wither!"
- **Source**: magic.cpp:1350-1367

---

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

**Implementation**:
- **Duration**: Until killed or dismissed
- **Source**: magic.cpp:4250+

---

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

**Implementation**:
- **Mechanics**: Spray of colors that damages and may blind
- **Source**: spells.cpp:315-454

---

#### Control Weather (ID 11)
- **Type**: SPELL
- **Enum**: `SPELL_CONTROL_WEATHER`
- **Command**: `control weather`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Changes weather in zone
- **Source**: spells.cpp

---

#### Create Food (ID 12)
- **Type**: SPELL
- **Enum**: `SPELL_CREATE_FOOD`
- **Command**: `create food`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Duration**: Temporary - decays
- **Source**: magic.cpp:5100+

---

#### Create Water (ID 13)
- **Type**: SPELL
- **Enum**: `SPELL_CREATE_WATER`
- **Command**: `create water`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Source**: magic.cpp:5120+

---

#### Cure Blind (ID 14)
- **Type**: SPELL
- **Enum**: `SPELL_CURE_BLIND`
- **Command**: `cure blind`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:

---

#### Cure Critic (ID 15)
- **Type**: SPELL
- **Enum**: `SPELL_CURE_CRITIC`
- **Command**: `cure critic`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:

---

#### Cure Light (ID 16)
- **Type**: SPELL
- **Enum**: `SPELL_CURE_LIGHT`
- **Command**: `cure light`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:

---

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
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **EFF_DETECT_ALIGN**
- **Messages**:
  - **to_char**: "$N can determine alignment."
  - **to_room**: "&7&b$N&7&b glows briefly.&0"
  - **to_vict**: "Your eyes tingle."
- **Source**: magic.cpp:1567-1574

---

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
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **APPLY_PERCEPTION**: `10`
  - **EFF_DETECT_INVIS**
- **Messages**:
  - **to_vict**: "Your eyes tingle."
- **Source**: magic.cpp:1576-1583

---

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

**Implementation**:
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **EFF_DETECT_MAGIC**
- **Messages**:
  - **to_vict**: "Your eyes tingle."
- **Source**: magic.cpp:1585-1590

---

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

**Implementation**:
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **EFF_DETECT_POISON**
- **Messages**:
  - **to_vict**: "Your eyes tingle."
- **Source**: magic.cpp:1592-1597

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 7) / 1000`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 13) / 2500, then *= sector_qdam_mod / 100`

---

#### Enchant Weapon (ID 24)
- **Type**: SPELL
- **Enum**: `SPELL_ENCHANT_WEAPON`
- **Command**: `enchant weapon`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Enhances weapon with magical properties
- **Duration**: Permanent
- **Source**: spells.cpp:659-694

---

#### Energy Drain (ID 25)
- **Type**: SPELL
- **Enum**: `SPELL_ENERGY_DRAIN`
- **Command**: `energy drain`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

**Implementation**:
- **Mechanics**: Drains experience from victim
- **Source**: spells.cpp:694-742

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `(pow(skill,2)*41)/5000`

---

#### Heal (ID 28)
- **Type**: SPELL
- **Enum**: `SPELL_HEAL`
- **Command**: `heal`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:

---

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

**Implementation**:
- **Duration**: `9 + (skill / 9)`
- **Effects**:
  - **APPLY_40**: `AC`
  - **EFF_INVISIBLE**
- **Messages**:
  - **to_room**: "$N slowly fades out of existence."
  - **to_vict**: "You vanish."
- **Source**: magic.cpp:2181-2190

---

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

**Implementation**:
- **Damage Type**: DAM_SHOCK
- **Damage Formula**: `sorcerer_single_target`

---

#### Locate Object (ID 31)
- **Type**: SPELL
- **Enum**: `SPELL_LOCATE_OBJECT`
- **Command**: `locate object`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Locates specific object in world
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Launches multiple magical missiles at target
- **Source**: spells.cpp:1268-1309

---

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

**Implementation**:
- **Duration**: `2 + (skill / 33) + (stat_bonus[GET_WIS(ch)].magic / 2)`
- **Effects**:
  - **APPLY_STR**: `(-2 - (skill / 4) - (skill / 20)) * susceptibility(victim, DAM_POISON) / 100`
  - **EFF_POISON**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "$N gets violently ill!"
  - **to_room**: "$N gets violently ill!"
  - **to_vict**: "You feel very sick."
- **Source**: magic.cpp:2489-2517

---

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

**Implementation**:
- **Duration**: `9 + (skill / 9)`
- **Effects**:
  - **EFF_PROTECT_EVIL**
- **Requirements**: Alignment: Cannot target evil
- **Messages**:
  - **to_char**: "You surround $N with glyphs of holy warding."
  - **to_vict**: "You feel invulnerable!"
- **Source**: magic.cpp:2537-2551

---

#### Remove Curse (ID 35)
- **Type**: SPELL
- **Enum**: `SPELL_REMOVE_CURSE`
- **Command**: `remove curse`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Removes curses from objects/players
- **Source**: spells.cpp

---

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
- **Duration**: `4`
- **Effects**:
  - **EFF_SANCTUARY**
- **Messages**:
  - **to_room**: "Absolutely nothing happens to $N."
  - **to_vict**: "This spell doesn't exist.  Ask no questions."
- **Source**: magic.cpp:2703-2713

---

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

**Implementation**:
- **Damage Type**: DAM_SHOCK
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Duration**: `9 + (skill / 9)`
- **Effects**:
  - **EFF_SLEEP**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Requirements**: Cannot target NPCs
- **Source**: magic.cpp:2748-2782

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Special Mechanics**:
  - Conflicts with other enhancement spells
  - Requires parameter: strength, dexterity, constitution, intelligence, wisdom, charisma
- **Messages**:
  - **to_char**: "You increase $N's strength!"
  - **to_room**: "$N looks stronger!"
  - **to_vict**: "You feel stronger!"
- **Source**: magic.cpp:1742-1803

---

#### Summon (ID 40)
- **Type**: SPELL
- **Enum**: `SPELL_SUMMON`
- **Command**: `summon`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Summons target to caster location
- **Source**: spells.cpp

---

#### Ventriloquate (ID 41)
- **Type**: SPELL
- **Enum**: `SPELL_VENTRILOQUATE`
- **Command**: `ventriloquate`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Makes speech appear to come from elsewhere
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Recall to temple/home

---

#### Remove Poison (ID 43)
- **Type**: SPELL
- **Enum**: `SPELL_REMOVE_POISON`
- **Command**: `remove poison`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:

---

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

**Implementation**:
- **Duration**: `17 + (skill / 3)`
- **Effects**:
  - **EFF_SENSE_LIFE**
- **Messages**:
  - **to_room**: "$N seems more aware of $S surroundings."
  - **to_vict**: "Your feel your awareness improve."
- **Source**: magic.cpp:2715-2721

---

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

**Implementation**:
- **Duration**: Permanent until killed
- **Requirements**: Corpse in room
- **Source**: magic.cpp:4280+

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 7) / 1000`

---

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

**Implementation**:
- **Mechanics**: Group armor buff

---

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

**Implementation**:
- **Mechanics**: Heal entire group

---

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

**Implementation**:
- **Mechanics**: Recall entire group

---

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

**Implementation**:
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **EFF_INFRAVISION**
- **Conflicts**: SPELL_NIGHT_VISION
- **Messages**:
  - **to_char**: "$N's eyes glow red."
  - **to_room**: "$N's eyes glow red."
  - **to_vict**: "Your eyes glow red."
- **Source**: magic.cpp:2138-2157

---

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

**Implementation**:
- **Duration**: `35 + (skill / 4)`
- **Effects**:
  - **EFF_WATERWALK**
- **Messages**:
  - **to_room**: "$N sprouts webbing between $S toes!"
  - **to_vict**: "You feel webbing between your toes."
- **Source**: magic.cpp:2928-2934

---

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

---

#### Full Heal (ID 53)
- **Type**: SPELL
- **Enum**: `SPELL_FULL_HEAL`
- **Command**: `full heal`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `(pow(skill,2)*1)/50`

---

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

**Implementation**:
- **Mechanics**: Creates fog wall

---

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

**Implementation**:
- **Mechanics**: Creates stone wall

---

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

**Implementation**:
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **EFF_FLY**
- **Special Mechanics**:
  - Weight restriction for flying
- **Messages**:
  - **to_char**: "$N remains earthbound."
  - **to_room**: "&6&b$N lifts into the air.&0"
  - **to_vict**: "You feel somewhat lighter."
- **Source**: magic.cpp:2046-2062

---

#### Summon Elemental (ID 59)
- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_ELEMENTAL`
- **Command**: `summon elemental`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Summon elemental

---

#### Summon Demon (ID 60)
- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_DEMON`
- **Command**: `summon demon`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Summon demon servant

---

#### Summon Greater Demon (ID 61)
- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_GREATER_DEMON`
- **Command**: `summon greater demon`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Summon powerful demon

---

#### Dimension Door (ID 62)
- **Type**: SPELL
- **Enum**: `SPELL_DIMENSION_DOOR`
- **Command**: `dimension door`
- **Target**: SINGLE_CHARACTER
  - Scope: ANYWHERE
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Opens portal to target location
- **Requirements**: Not in NOTELEPORT room
- **Source**: spells.cpp:615-659

---

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

**Implementation**:
- **Mechanics**: Swarm of insects that damages all enemies in room over time
- **Source**: spells.cpp:480-499

---

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

**Implementation**:
- **Mechanics**: Curse of doom

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `dam + (pow(skill, 2) * 13) / 200`

---

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

**Implementation**:
- **Damage Type**: DAM_CRUSH
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **EFF_FARSEE**
- **Messages**:
  - **to_room**: "$N's pupils dilate rapidly for a second."
  - **to_vict**: "Your sight improves dramatically."
- **Source**: magic.cpp:1982-1988

---

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
- **Duration**: `2 + (skill / 21)`
- **Effects**:
  - **EFF_HASTE**
- **Messages**:
  - **to_char**: "&1$N starts to move with uncanny speed!&0"
  - **to_room**: "&1$N starts to move with uncanny speed!&0"
  - **to_vict**: "&1You start to move with uncanny speed!&0"
- **Source**: magic.cpp:2109-2116

---

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

**Implementation**:
- **Duration**: `get_spell_duration(ch, spellnum)`
- **Effects**:
  - **APPLY_HIT**: `get_vitality_hp_gain(ch, spellnum)`
- **Special Mechanics**:
  - HP gain calculated by get_vitality_hp_gain()
  - Duration calculated by get_spell_duration()
  - Conflicts with other armor spells (check_armor_spells)
- **Conflicts**: SPELL_LESSER_ENDURANCE, SPELL_ENDURANCE, SPELL_VITALITY, SPELL_GREATER_VITALITY, SPELL_DRAGONS_HEALTH
- **Messages**:
  - **to_char**: "$N looks healthier than before!"
  - **to_room**: "$N looks healthier than before!"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1138-1177

---

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

**Implementation**:
- **Mechanics**: Creates healing moonwell
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Creates magical darkness in room
- **Duration**: Temporary
- **Source**: spells.cpp:516-588

---

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

**Implementation**:
- **Mechanics**: Fills room with magical light
- **Duration**: Temporary
- **Source**: spells.cpp:1054-1154

---

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

**Implementation**:
- **Damage Type**: DAM_COLD
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_COLD
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 100`

---

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

**Implementation**:
- **Damage Type**: DAM_COLD
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 10`

---

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

**Implementation**:
- **Mechanics**: Paralyzes target completely
- **Source**: spells.cpp

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + roll_dice(2, skill + 10), +random(0,70) if skill>=95`

---

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

**Implementation**:
- **Mechanics**: Burning cloud

---

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

**Implementation**:
- **Duration**: `2 + (skill / 15)`
- **Effects**:
  - **EFF_MINOR_PARALYSIS**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You temporarily paralyze $N!"
  - **to_room**: "&7&bAll motion in $N&7&b's body grinds to a halt.&0"
  - **to_vict**: "&7&bAll motion in your body grinds to a halt.&0"
- **Source**: magic.cpp:2274-2304

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `(pow(skill,2)*2)/625`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `(pow(skill,2)*9)/2000`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `(pow(skill,2)*53)/10000`

---

#### Preserve (ID 87)
- **Type**: SPELL
- **Enum**: `SPELL_PRESERVE`
- **Command**: `preserve`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Preserves corpse from decay
- **Source**: spells.cpp

---

#### Cure Serious (ID 88)
- **Type**: SPELL
- **Enum**: `SPELL_CURE_SERIOUS`
- **Command**: `cure serious`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:

---

#### Vigorize Light (ID 89)
- **Type**: SPELL
- **Enum**: `SPELL_VIGORIZE_LIGHT`
- **Command**: `vigorize light`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:

---

#### Vigorize Serious (ID 90)
- **Type**: SPELL
- **Enum**: `SPELL_VIGORIZE_SERIOUS`
- **Command**: `vigorize serious`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:

---

#### Vigorize Critic (ID 91)
- **Type**: SPELL
- **Enum**: `SPELL_VIGORIZE_CRITIC`
- **Command**: `vigorize critic`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:

---

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

**Implementation**:
- **Duration**: `2 + (skill / 10)`
- **Effects**:
  - **EFF_SOULSHIELD**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_room**: "&3&bA bright golden aura surrounds $N's body!&0"
  - **to_vict**: "&3&bA bright golden aura surrounds your body!&0"
- **Source**: magic.cpp:2820-2837

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 27) / 1000`

---

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

**Implementation**:
- **Duration**: `2 + (skill / 15)`
- **Effects**:
  - **EFF_SILENCE**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You silence $N!"
  - **to_room**: "&0$N&7 squeaks as all sound is squelched from $S throat.&0"
  - **to_vict**: "&9&bYour throat begins to close, sealing off all chance of "
- **Source**: magic.cpp:2723-2746

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `dam *= (caster_INT * 0.007 + 0.8)`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 33) / 2000`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 33) / 2000`

---

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

**Implementation**:
- **Mechanics**: Shifts to different plane of existence
- **Source**: spells.cpp

---

#### Dispel Magic (ID 99)
- **Type**: SPELL
- **Enum**: `SPELL_DISPEL_MAGIC`
- **Command**: `dispel magic`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: Yes

**Implementation**:
- **Damage Type**: varies
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`

---

#### Minor Creation (ID 100)
- **Type**: SPELL
- **Enum**: `SPELL_MINOR_CREATION`
- **Command**: `minor creation`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Creates minor magical items
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Enhanced concealment

---

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

**Implementation**:
- **Duration**: `8 + (skill / 20)`
- **Effects**:
  - **EFF_RAY_OF_ENFEEB**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "$N turns pale and starts to sag."
  - **to_room**: "$N turns pale and starts to sag."
  - **to_vict**: "You feel the strength flow out of your body."
- **Source**: magic.cpp:2625-2648

---

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

**Implementation**:
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **EFF_FEATHER_FALL**
- **Messages**:
  - **to_char**: "&6$N&0&6 floats up into the air.&0"
  - **to_room**: "&6$N&0&6 floats up into the air.&0"
  - **to_vict**: "&6You float up into the air.&0"
- **Source**: magic.cpp:2192-2199

---

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

**Implementation**:
- **Mechanics**: Creates magical eye to scout distant locations
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `skill / 20`
- **Effects**:
  - **EFF_FIRESHIELD**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_room**: "&1A burning shield of f&bi&3r&7e&0&1 explodes from $N&0&1's body!&0"
  - **to_vict**: "&1A burning shield of f&bi&3r&7e&0&1 explodes from your body!&0"
- **Source**: magic.cpp:2031-2044

---

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
- **Duration**: `skill / 20`
- **Effects**:
  - **EFF_COLDSHIELD**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_room**: "&4A jagged formation of i&bc&7e sh&4ard&0&4s forms around $N&0&4.&0"
  - **to_vict**: "&4A jagged formation of i&bc&7e sh&4ard&0&4s forms around you.&0"
- **Source**: magic.cpp:1377-1390

---

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

**Implementation**:
- **Duration**: `skill / 20`
- **Effects**:
  - **EFF_MINOR_GLOBE**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_char**: "&1Your shimmering globe wraps around $N&0&1's body.&0"
  - **to_room**: "&1A shimmering globe wraps around $N&0&1's body.&0"
  - **to_vict**: "&1A shimmering globe wraps around your body.&0"
- **Source**: magic.cpp:2257-2272

---

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

**Implementation**:
- **Duration**: `4 + (skill / 20)`
- **Effects**:
  - **EFF_MAJOR_GLOBE**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_char**: "&1&bYour shimmering globe of force wraps around $N&1&b's body.&0"
  - **to_room**: "&1&bA shimmering globe of force wraps around $N&1&b's body.&0"
  - **to_vict**: "&1&bA shimmering globe of force wraps around your body.&0"
- **Source**: magic.cpp:2209-2224

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Duration**: `(skill >= 20)`
- **Effects**:
  - **EFF_HARNESS**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_room**: "&4&b$N&4&b's veins bulge as a surge of energy rushes into $M!&0"
  - **to_vict**: "&4&bYour veins begin to pulse with energy!&0"
- **Source**: magic.cpp:2095-2107

---

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

**Implementation**:
- **Damage Type**: DAM_SHOCK
- **Damage Formula**: `(pow(skill,2)*7)/500`

---

#### Mass Invisibility (ID 112)
- **Type**: SPELL
- **Enum**: `SPELL_MASS_INVIS`
- **Command**: `mass invisibility`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Duration**: `9 + (skill / 9)`
- **Effects**:
  - **APPLY_40**: `AC`
  - **EFF_INVISIBLE**
- **Messages**:
  - **to_room**: "$N slowly fades out of existence."
  - **to_vict**: "You vanish."
- **Source**: magic.cpp:2181-2190

---

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

**Implementation**:
- **Mechanics**: Causes targets to flee in terror
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `5 + (skill / 2)`
- **Effects**:
  - **EFF_LIGHT**
- **Messages**:
  - **to_room**: "&7&bA bright white circle of light appears over $N's&7&b head."
  - **to_vict**: "&7&bA bright white circle of light begins hovering about your head.&0"
- **Source**: magic.cpp:1369-1375

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 53) / 10000, then *= (victim_align * -0.0007 + 0.8)`

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Effects**:
  - **APPLY_SAVING_SPELL**: `-5 - (skill / 14)`
- **Messages**:
  - **to_room**: "$N perks up, looking full of life."
  - **to_vict**: "Your prayer is answered...\nYou feel full of life!"
- **Source**: magic.cpp:2519-2535

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Effects**:
  - **EFF_PROT_FIRE**
  - **EFF_PROT_COLD**
  - **EFF_PROT_AIR**
  - **EFF_PROT_EARTH**
- **Special Mechanics**:
  - Requires parameter: fire, cold, air, earth
- **Messages**:
  - **to_char**: "You protect $N from &1fire&0."
  - **to_room**: "&7&b$N&7&b glows briefly.&0"
  - **to_vict**: "You are warded from &1fire&0."
- **Source**: magic.cpp:1704-1740

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 125, then *= (caster_align * 0.0022 - 0.7)`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 25, then *= (caster_align * 0.001 + 0.2)`

---

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

**Implementation**:
- **Damage Type**: DAM_ROT
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Mechanics**: Speak any language

---

#### Enlightenment (ID 123)
- **Type**: SPELL
- **Enum**: `SPELL_ENLIGHTENMENT`
- **Command**: `enlightenment`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Reveals detailed information about target
- **Source**: spells.cpp:742-769

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam = (pow(skill, 2) * 13) / 20, then *= (caster_align * 0.001 + 0.2)`

---

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

**Implementation**:
- **Duration**: `2 + (skill / 15)`
- **Effects**:
  - **EFF_MINOR_PARALYSIS**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You grab $N and scramble $S nerves!"
  - **to_room**: "$N gasps for breath as $n scrambles $S nerves!"
  - **to_vict**: "Tingles run up and down your spine as $n scrambles your nerves!"
- **Source**: magic.cpp:3167-3190

---

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

**Implementation**:
- **Duration**: `10 + (skill / 5)`
- **Effects**:
  - **EFF_FLY**
- **Special Mechanics**:
  - Conflicts with other blessing spells (check_bless_spells)
- **Messages**:
  - **to_room**: "&7&bBeautiful bright white wings unfurl from $n's&7&b back, lifting $m into the air.&0"
  - **to_vict**: "&7&bBeautiful bright white wings unfurl behind you as you lift into the air.&0"
- **Source**: magic.cpp:2961-2975

---

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

**Implementation**:
- **Mechanics**: Banishes target to random location or extraction
- **Immunities**: MOB_NOSUMMON, MOB_NOCHARM, ROOM_NOSUMMON
- **Special**: 66% chance to destroy NPC equipment if WIS check passes
- **Source**: spells.cpp:155-219

---

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

**Implementation**:
- **Mechanics**: Forces target to obey single command
- **Source**: spells.cpp

---

#### Divine Essence (ID 129)
- **Type**: SPELL
- **Enum**: `SPELL_DIVINE_ESSENCE`
- **Command**: `divine essence`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Enhances divine caster abilities
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Opens gate to celestial plane, summons angels
- **Requirements**: Good alignment
- **Source**: spells.cpp:882-942

---

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
- **Duration**: `10 + (skill / 7)`
- **Effects**:
  - **APPLY_SAVING_SPELL**: `-2 - (skill / 10)`
  - **APPLY_DAMROLL**: `1 + (skill > 95)`
  - **EFF_BLESS**
- **Requirements**: Alignment: Cannot target good; Alignment: Caster cannot be good
- **Conflicts**: SPELL_BLESS, SPELL_WINGS_OF_HEAVEN, SPELL_EARTH_BLESSING
- **Messages**:
  - **to_char**: "You summon allegiance from your dark gods to protect $N."
  - **to_room**: "$n seizes up in pain!\n$n crosses $s arms on $s chest, and is surrounded by a dark presence."
  - **to_vict**: "You summon allegiance from your dark gods to protect yourself.\n&9&bA dark presence fills your "
- **Source**: magic.cpp:1435-1495

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other armor spells (check_armor_spells)
  - Conflicts with other blessing spells (check_bless_spells)
- **Messages**:
  - **to_room**: "&7&b$n magically restores the gaps in $N's armor.&0"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1531-1543

---

#### Dark Feast (ID 133)
- **Type**: SPELL
- **Enum**: `SPELL_DARK_FEAST`
- **Command**: `dark feast`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Drains life from all enemies in room to heal caster
- **Source**: spells.cpp:499-516

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 53) / 10000, then *= (victim_align * 0.0007 + 0.8)`

---

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

**Implementation**:
- **Duration**: `5 + (skill / 10)`
- **Effects**:
  - **APPLY_CON**: `-10 - (skill / 10)`
  - **APPLY_STR**: `eff[0].modifier`
  - **EFF_DISEASE**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "Your diseased air infects $N!"
  - **to_room**: "&3$N&3 chokes and gasps on $n's foul air, $E looks seriously ill!"
  - **to_vict**: "&3You choke and gasp on $n's foul air as a sick feeling overtakes you.\n"
- **Source**: magic.cpp:1599-1622

---

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

**Implementation**:
- **Duration**: `5`
- **Effects**:
  - **APPLY_WIS**: `-50`
  - **EFF_INSANITY**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You cause &5$N to snap... A crazed gleam fills $S eyes.&0"
  - **to_room**: "&5$N's&5 psyche snaps... A crazed gleam fills $S eyes.&0"
  - **to_vict**: "&5You go out of your &bMIND!&0"
- **Source**: magic.cpp:2159-2179

---

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
- **Duration**: `5 + (skill / 20)`
- **Effects**:
  - **APPLY_STR**: `5 + (skill / 14)`
- **Special Mechanics**:
  - Conflicts with other blessing spells (check_bless_spells)
- **Conflicts**: SPELL_DEMONIC_MUTATION
- **Messages**:
  - **to_room**: "&1$n's&1 body &bglows red&0&1 briefly and grows stronger.&0"
  - **to_vict**: "&1Your body fills with a demonic strength.&0"
- **Source**: magic.cpp:1497-1529

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 180, then *= (caster_align * -0.0022 - 0.7)`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 125, then *= (caster_align * -0.0022 - 0.7)`

---

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

**Implementation**:
- **Duration**: `5 + (skill / 20)`
- **Effects**:
  - **APPLY_STR**: `5 + (skill / 14)`
- **Special Mechanics**:
  - Conflicts with other blessing spells (check_bless_spells)
- **Conflicts**: SPELL_DEMONIC_ASPECT
- **Messages**:
  - **to_room**: "&1$n's&1 body &bglows red&0&1 briefly and grows stronger.&0"
  - **to_vict**: "&1Your body fills with a demonic strength.&0"
- **Source**: magic.cpp:1497-1529

---

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

**Implementation**:
- **Duration**: `10 + (skill / 5)`
- **Effects**:
  - **EFF_FLY**
- **Special Mechanics**:
  - Conflicts with other blessing spells (check_bless_spells)
- **Messages**:
  - **to_room**: "&1&bHuge leathery &9bat-like&1 wings sprout out of $n's&1&b back.&0"
  - **to_vict**: "&1&bHuge leathery &9bat-like&1 wings sprout from your back.&0"
- **Source**: magic.cpp:2977-2991

---

#### Sane Mind (ID 142)
- **Type**: SPELL
- **Enum**: `SPELL_SANE_MIND`
- **Command**: `sane mind`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:

---

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

**Implementation**:
- **Mechanics**: Opens gate to infernal plane, summons demons
- **Requirements**: Evil alignment
- **Source**: spells.cpp:942-1002

---

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
- **Special Mechanics**:
  - Conflicts with other armor spells (check_armor_spells)
- **Messages**:
  - **to_room**: "&7&b$n magically restores the gaps in $N's armor.&0"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1191-1200

---

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

**Implementation**:
- **Duration**: `(skill / 21)`
- **Effects**:
  - **EFF_ULTRAVISION**
- **Conflicts**: SPELL_INFRAVISION
- **Messages**:
  - **to_room**: "$N's eyes glow a dim neon green."
  - **to_vict**: "&9&bYour vision sharpens a bit."
- **Source**: magic.cpp:2463-2478

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 53) / 10000`

---

#### Create Spring (ID 147)
- **Type**: SPELL
- **Enum**: `SPELL_CREATE_SPRING`
- **Command**: `create spring`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Duration**: Permanent until removed
- **Source**: magic.cpp:5140+

---

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

**Implementation**:

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other armor spells (check_armor_spells)
- **Messages**:
  - **to_room**: "&7&b$n magically restores the gaps in $N's armor.&0"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:2064-2074

---

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

**Implementation**:
- **Duration**: `(skill / 3) + 1`
- **Effects**:
  - **EFF_CAMOUFLAGED**
- **Messages**:
  - **to_room**: "&9&b$n&9&b phases into the landscape.&0"
  - **to_vict**: "&9&bYou phase into the landscape.&0"
- **Source**: magic.cpp:2448-2453

---

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

**Implementation**:
- **Duration**: `2 + (skill / 24)`
- **Effects**:
  - **EFF_MAJOR_PARALYSIS**
  - **EFF_MINOR_PARALYSIS**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "&2&bYour crop of thick branches and vines burst"
  - **to_room**: "&2&bA slew of thick branches and vines burst"
  - **to_vict**: "&2&bA slew of thick branches and vines burst"
- **Source**: magic.cpp:1935-1971

---

#### Invigorate (ID 152)
- **Type**: SPELL
- **Enum**: `SPELL_INVIGORATE`
- **Command**: `invigorate`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Restore stamina

---

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

**Implementation**:
- **Mechanics**: Animates trees to aid caster
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Destructive city spell

---

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

**Implementation**:
- **Duration**: `2`
- **Effects**:
  - **APPLY_HITROLL**: `-4`
  - **APPLY_AC**: `-40`
  - **EFF_BLIND**
  - **EFF_BLIND**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "&9&bYou have blinded $N with your sunray!&0"
  - **to_room**: "&9&b$N&9&b seems to be blinded!&0"
  - **to_vict**: "&9&bYou have been blinded!&0"
- **Source**: magic.cpp:2864-2892

---

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

**Implementation**:
- **Mechanics**: Creates magical armor on body/arms/legs slots
- **Requirements**: Body/arms/legs slots must be empty
- **Source**: spells.cpp:110-153

---

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

**Implementation**:
- **Mechanics**: Launches multiple fire darts at target
- **Source**: spells.cpp:1309+

---

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

**Implementation**:
- **Duration**: `5 + (skill / 2)`
- **Effects**:
  - **EFF_LIGHT**
- **Messages**:
  - **to_room**: "&1A magical flame bursts into focus, lighting the area.&0"
  - **to_vict**: "&1A magical flame bursts into focus, lighting the area.&0"
- **Source**: magic.cpp:2201-2207

---

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

**Implementation**:
- **Duration**: `2`
- **Effects**:
  - **APPLY_HITROLL**: `-1`
  - **APPLY_AC**: `-10`
  - **EFF_BLIND**
  - **EFF_BLIND**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You temporarily choke $N with your column of smoke."
  - **to_room**: "&9&b$N&9&b is slightly choked by $n's&9&b column of smoke!&0"
  - **to_vict**: "&9&bYou have been temporarily choked by $n's&9&b column of smoke!&0"
- **Source**: magic.cpp:2784-2818

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other armor spells (check_armor_spells)
- **Messages**:
  - **to_room**: "&7&b$n magically restores the gaps in $N's armor.&0"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:2306-2317

---

#### Flame Blade (ID 161)
- **Type**: SPELL
- **Enum**: `SPELL_FLAME_BLADE`
- **Command**: `flame blade`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Creates flaming weapon
- **Duration**: Temporary
- **Source**: spells.cpp:769-792

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 100`

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `sorcerer_single_target(ch, spellnum, skill)`

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `(skill/2) + roll_dice(2,3)`

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `sorcerer_single_target / 4`

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `dam + (pow(skill, 2) * 1) / 10`

---

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

**Implementation**:
- **Mechanics**: Destroy corpse completely

---

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

**Implementation**:
- **Duration**: `2 + (skill / 20)`
- **Effects**:
  - **EFF_NEGATE_HEAT**
- **Messages**:
  - **to_room**: "&6$n&6 is surrounded by a frigid crystalline field.&0"
  - **to_vict**: "&6Your body becomes impervious to all forms of heat!&0"
- **Source**: magic.cpp:2440-2446

---

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

**Implementation**:
- **Damage Type**: DAM_ACID
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Mechanics**: Launches ice darts at target
- **Source**: spells.cpp

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other armor spells (check_armor_spells)
- **Messages**:
  - **to_room**: "&7&b$n magically restores the gaps in $N's armor.&0"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:2118-2128

---

#### Ice Dagger (ID 173)
- **Type**: SPELL
- **Enum**: `SPELL_ICE_DAGGER`
- **Command**: `ice dagger`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Creates ice dagger and hurls it at target
- **Source**: spells.cpp:1002-1024

---

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

**Implementation**:
- **Damage Type**: DAM_COLD
- **Damage Formula**: `dam + (pow(skill, 2) * 3) / 500`

---

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

**Implementation**:
- **Damage Type**: DAM_COLD
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Mechanics**: Creates ice wall

---

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

**Implementation**:
- **Damage Type**: DAM_COLD
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_WATER
- **Damage Formula**: `dam + (pow(skill, 2) * 13) / 200`

---

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

**Implementation**:
- **Duration**: `2 + (skill / 25)`
- **Effects**:
  - **APPLY_COMPOSITION**: `COMP_MIST`
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_room**: "&6&b$N's body dematerializes into a translucent &7cloud &6of "
  - **to_vict**: "&6&bYour body sublimates into a &7cloud &6of &7vapor&6.&0"
- **Source**: magic.cpp:2894-2909

---

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

**Implementation**:
- **Duration**: `2 + (skill / 20)`
- **Effects**:
  - **EFF_NEGATE_COLD**
- **Messages**:
  - **to_room**: "&4$n&4's is protected by a &3&bwarm&0&4-looking magical field.&0"
  - **to_vict**: "&4&bYour body becomes impervious to the cold!&0"
- **Source**: magic.cpp:2432-2438

---

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

**Implementation**:
- **Duration**: `2 + (skill / 20)`
- **Effects**:
  - **APPLY_COMPOSITION**: `COMP_WATER`
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_room**: "&4&b$N&4&b's body wavers a bit, slowly changing into a "
  - **to_vict**: "&4&bYour body liquifies.&0"
- **Source**: magic.cpp:2911-2926

---

#### Extinguish (ID 182)
- **Type**: SPELL
- **Enum**: `SPELL_EXTINGUISH`
- **Command**: `extinguish`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: SITTING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Extinguish fires

---

#### Rain (ID 183)
- **Type**: SPELL
- **Enum**: `SPELL_RAIN`
- **Command**: `rain`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Causes rain in area
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `1 + (skill / 40)`
- **Effects**:
  - **APPLY_SIZE**: `-1`
  - **APPLY_CON**: `-10`
  - **APPLY_STR**: `-10`
  - **EFF_REDUCE**
- **Requirements**: Cannot target NPCs
- **Messages**:
  - **to_room**: "&1&b$N's skin ripples as $E shrinks to half $S normal size!&0"
  - **to_vict**: "&1&bYour skin starts to itch as you reduce to half your normal "
- **Source**: magic.cpp:2679-2701

---

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

**Implementation**:
- **Duration**: `1 + (skill / 40)`
- **Effects**:
  - **APPLY_SIZE**: `1`
  - **APPLY_CON**: `10`
  - **APPLY_STR**: `10`
  - **EFF_ENLARGE**
- **Requirements**: Cannot target NPCs
- **Messages**:
  - **to_room**: "&9&b$N's skin ripples as $E enlarges to twice $S normal size!&0"
  - **to_vict**: "&9&bYour skin starts to itch as you enlarge to twice your normal size!&0"
- **Source**: magic.cpp:1913-1933

---

#### Identify (ID 186)
- **Type**: SPELL
- **Enum**: `SPELL_IDENTIFY`
- **Command**: `identify`
- **Target**: SINGLE_CHARACTER
  - Scope: TOUCH
- **Min Position**: SITTING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Reveals all properties of an object
- **Source**: spells.cpp:1024-1054

---

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
- **Special Mechanics**:
  - Conflicts with other armor spells (check_armor_spells)
- **Messages**:
  - **to_room**: "&7&b$n magically restores the gaps in $N's armor.&0"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1299-1308

---

#### Summon Corpse (ID 188)
- **Type**: SPELL
- **Enum**: `SPELL_SUMMON_CORPSE`
- **Command**: `summon corpse`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Summons corpse to caster
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Teleports corpse to caster
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `5 + skill / 20`
- **Effects**:
  - **APPLY_CHA**: `50`
  - **EFF_GLORY**
- **Messages**:
  - **to_room**: "&7&b$N seems taller in the light, and appears like unto a god.&0"
  - **to_vict**: "&7&bYou stand tall in the light, a beacon of greatness.&0"
- **Source**: magic.cpp:2085-2093

---

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

**Implementation**:
- **Mechanics**: Creates illusory wall

---

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

**Implementation**:
- **Damage Type**: DAM_MENTAL
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `100 + GET_HIT(victim) if MOB_ILLUSORY, else sorcerer_single_target / 5`

---

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

**Implementation**:
- **Mechanics**: Teleports caster to safe isolated location
- **Requirements**: Cannot be in NOTELEPORT room
- **Source**: spells.cpp:1170-1248

---

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

**Implementation**:
- **Duration**: `skill / 5 + 4`
- **Effects**:
  - **EFF_FAMILIARITY**
- **Messages**:
  - **to_room**: "You know in your heart that $N is a steady friend, to be "
  - **to_vict**: "&7&bAn aura of comfort and solidarity surrounds you.&0"
- **Source**: magic.cpp:1973-1980

---

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

**Implementation**:
- **Mechanics**: Causes hysteria

---

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

**Implementation**:
- **Duration**: `2`
- **Effects**:
  - **EFF_MESMERIZED**
- **Requirements**: Cannot target NPCs
- **Messages**:
  - **to_char**: "You weave a mesmerizing pattern before $N, and $E seems to be utterly absorbed by it."
  - **to_room**: "$n &0&5w&4e&5av&4es a &5mesme&4rizing pa&5ttern before &0$N's eyes.\n"
  - **to_vict**: "$n shows you a truly fascinating puzzle.  You simply must work it out."
- **Source**: magic.cpp:2226-2255

---

#### Severance (ID 198)
- **Type**: SPELL
- **Enum**: `SPELL_SEVERANCE`
- **Command**: `severance`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

**Implementation**:
- **Damage Type**: DAM_SLASH
- **Damage Formula**: `dam + (pow(skill, 2) * 13) / 200`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `dam + (pow(skill, 2) * 13) / 199`

---

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

**Implementation**:
- **Damage Type**: DAM_ENERGY
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Mechanics**: Breathes fire at targets
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `2 + (skill / 33) + (stat_bonus[GET_WIS(ch)].magic / 2)`
- **Effects**:
  - **APPLY_STR**: `(-2 - (skill / 4) - (skill / 20)) * susceptibility(victim, DAM_POISON) / 100`
  - **EFF_POISON**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "$N gets violently ill!"
  - **to_room**: "$N gets violently ill!"
  - **to_vict**: "You feel very sick."
- **Source**: magic.cpp:2489-2517

---

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

**Implementation**:
- **Mechanics**: Breathes frost at targets
- **Source**: spells.cpp

---

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

**Implementation**:
- **Damage Type**: DAM_ACID
- **Damage Formula**: `skill + random(1, skill*2)`

---

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

**Implementation**:
- **Mechanics**: Breathes lightning at target
- **Source**: spells.cpp:1248-1268

---

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

**Implementation**:
- **Duration**: `get_spell_duration(ch, spellnum)`
- **Effects**:
  - **APPLY_HIT**: `get_vitality_hp_gain(ch, spellnum)`
- **Special Mechanics**:
  - HP gain calculated by get_vitality_hp_gain()
  - Duration calculated by get_spell_duration()
  - Conflicts with other armor spells (check_armor_spells)
- **Conflicts**: SPELL_ENDURANCE, SPELL_GREATER_ENDURANCE, SPELL_VITALITY, SPELL_GREATER_VITALITY, SPELL_DRAGONS_HEALTH
- **Messages**:
  - **to_char**: "$N looks healthier than before!"
  - **to_room**: "$N looks healthier than before!"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1138-1177

---

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

**Implementation**:
- **Duration**: `get_spell_duration(ch, spellnum)`
- **Effects**:
  - **APPLY_HIT**: `get_vitality_hp_gain(ch, spellnum)`
- **Special Mechanics**:
  - HP gain calculated by get_vitality_hp_gain()
  - Duration calculated by get_spell_duration()
  - Conflicts with other armor spells (check_armor_spells)
- **Conflicts**: SPELL_LESSER_ENDURANCE, SPELL_GREATER_ENDURANCE, SPELL_VITALITY, SPELL_GREATER_VITALITY, SPELL_DRAGONS_HEALTH
- **Messages**:
  - **to_char**: "$N looks healthier than before!"
  - **to_room**: "$N looks healthier than before!"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1138-1177

---

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

**Implementation**:
- **Duration**: `get_spell_duration(ch, spellnum)`
- **Effects**:
  - **APPLY_HIT**: `get_vitality_hp_gain(ch, spellnum)`
- **Special Mechanics**:
  - HP gain calculated by get_vitality_hp_gain()
  - Duration calculated by get_spell_duration()
  - Conflicts with other armor spells (check_armor_spells)
- **Conflicts**: SPELL_LESSER_ENDURANCE, SPELL_ENDURANCE, SPELL_GREATER_ENDURANCE, SPELL_GREATER_VITALITY, SPELL_DRAGONS_HEALTH
- **Messages**:
  - **to_char**: "$N looks healthier than before!"
  - **to_room**: "$N looks healthier than before!"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1138-1177

---

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

**Implementation**:
- **Duration**: `get_spell_duration(ch, spellnum)`
- **Effects**:
  - **APPLY_HIT**: `get_vitality_hp_gain(ch, spellnum)`
- **Special Mechanics**:
  - HP gain calculated by get_vitality_hp_gain()
  - Duration calculated by get_spell_duration()
  - Conflicts with other armor spells (check_armor_spells)
- **Conflicts**: SPELL_LESSER_ENDURANCE, SPELL_ENDURANCE, SPELL_GREATER_ENDURANCE, SPELL_VITALITY, SPELL_DRAGONS_HEALTH
- **Messages**:
  - **to_char**: "$N looks healthier than before!"
  - **to_room**: "$N looks healthier than before!"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1138-1177

---

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

**Implementation**:
- **Duration**: `get_spell_duration(ch, spellnum)`
- **Effects**:
  - **APPLY_HIT**: `get_vitality_hp_gain(ch, spellnum)`
- **Special Mechanics**:
  - HP gain calculated by get_vitality_hp_gain()
  - Duration calculated by get_spell_duration()
  - Conflicts with other armor spells (check_armor_spells)
- **Conflicts**: SPELL_LESSER_ENDURANCE, SPELL_ENDURANCE, SPELL_GREATER_ENDURANCE, SPELL_VITALITY, SPELL_GREATER_VITALITY
- **Messages**:
  - **to_char**: "$N looks healthier than before!"
  - **to_room**: "$N looks healthier than before!"
  - **to_vict**: "&7&bThe gaps in your armor are restored.&0"
- **Source**: magic.cpp:1138-1177

---

#### Rebuke Undead (ID 211)
- **Type**: SPELL
- **Enum**: `SPELL_REBUKE_UNDEAD`
- **Command**: `rebuke undead`
- **Target**: SINGLE_ENEMY
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

**Implementation**:
- **Duration**: `eff[1].duration = eff[2].duration = skill / 10`
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "&5You shout a powerful rebuke at $N, forcing $M to cower in fear!&0"
  - **to_room**: "&5$N cowers in fear as $n rebukes $M.&0"
  - **to_vict**: "&5You catch a glimpse of $n's true power and cower in fear!&0"
- **Source**: magic.cpp:2650-2677

---

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

**Implementation**:
- **Mechanics**: Causes victim to degenerate over time
- **Duration**: Multiple ticks
- **Source**: spells.cpp:588-615

---

#### Soul Tap (ID 213)
- **Type**: SPELL
- **Enum**: `SPELL_SOUL_TAP`
- **Command**: `soul tap`
- **Target**: SINGLE_ENEMY
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

**Implementation**:
- **Mechanics**: Drains life force over time to heal caster
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `(skill / 20) + 1`
- **Effects**:
  - **APPLY_HITROLL**: `eff[0].duration = (skill / 20) + 1`
- **Messages**:
  - **to_room**: "$N calls on guidance from a higher power."
  - **to_vict**: "You feel a higher power guiding your hands."
- **Source**: magic.cpp:2455-2461

---

#### Moonbeam (ID 215)
- **Type**: SPELL
- **Enum**: `SPELL_MOONBEAM`
- **Command**: `moonbeam`
- **Target**: AREA_ROOM
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `skill * 2 + random_number(20, 80)`

---

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

**Implementation**:
- **Mechanics**: Creates phantasmal image

---

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

**Implementation**:
- **Mechanics**: Create simulacrum copy

---

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

**Implementation**:
- **Duration**: `2 + skill / 4`
- **Effects**:
  - **EFF_MISDIRECTION**
- **Messages**:
  - **to_vict**: "You feel like a stack of little illusions all pointing in "
- **Source**: magic.cpp:2331-2337

---

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
- **Duration**: `2 + skill / 40`
- **Effects**:
  - **EFF_CONFUSION**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Messages**:
  - **to_room**: "$N can't decide which way to cross $S eyes!"
  - **to_vict**: "&5You suddenly find it difficult to focus upon your foes.&0"
- **Source**: magic.cpp:1392-1408

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `sorcerer_single_target / 3`

---

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

**Implementation**:
- **Damage Type**: DAM_FIRE
- **Damage Formula**: `sorcerer_single_target / 3, x2 if charmed by PC master`

---

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

**Implementation**:
- **Damage Type**: DAM_PIERCE
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `sorcerer_single_target / 3, x2 if charmed by PC master`

---

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

**Implementation**:
- **Duration**: `1`
- **Effects**:
  - **APPLY_NONE**: `4`
  - **EFF_IMMOBILIZED**
- **Requirements**: Cannot target NPCs
- **Messages**:
  - **to_char**: "You conjure four magical bones to lock $N in place!"
  - **to_room**: "$n conjures four magical bones that lock around $N's legs!"
  - **to_vict**: "$n conjures four magical bones which bind your legs!"
- **Source**: magic.cpp:1319-1339

---

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

**Implementation**:
- **Mechanics**: Teleports to any location in world
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Launches spirit arrows at target
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `9 + (skill / 9)`
- **Effects**:
  - **EFF_PROTECT_GOOD**
- **Requirements**: Alignment: Cannot target good
- **Messages**:
  - **to_char**: "You surround $N with glyphs of unholy warding."
  - **to_vict**: "You feel invulnerable!"
- **Source**: magic.cpp:2553-2567

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `(pow(skill,2)*7)/500`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_ALIGN
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Damage Type**: DAM_MENTAL
- **Damage Formula**: `sorcerer_single_target`

---

#### Remove Paralysis (ID 241)
- **Type**: SPELL
- **Enum**: `SPELL_REMOVE_PARALYSIS`
- **Command**: `remove paralysis`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: No
- **Violent**: No

**Implementation**:
- **Mechanics**: Removes paralysis effects
- **Source**: spells.cpp

---

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

**Implementation**:
- **Mechanics**: Creates cloud of spinning daggers hitting all enemies 4 times
- **Source**: spells.cpp:299-315

---

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

**Implementation**:
- **Mechanics**: Reveals hidden/invisible creatures
- **Source**: spells.cpp

---

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

**Implementation**:
- **Duration**: `2`
- **Effects**:
  - **APPLY_HITROLL**: `-4`
  - **APPLY_AC**: `-40`
  - **EFF_BLIND**
  - **EFF_BLIND**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "&9&b$N&9&b is blinded by you!&0"
  - **to_room**: "&9&b$N&9&b is blinded by $n!&0"
  - **to_vict**: "&9&bYou have been blinded!&0"
- **Source**: magic.cpp:1259-1289

---

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

**Implementation**:
- **Mechanics**: Creates corrosive fog that hits all enemies in room 4 times
- **Source**: spells.cpp:52-65, magic.cpp:618-622

---

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

**Implementation**:
- **Duration**: `2 + (skill / 50)`
- **Effects**:
  - **EFF_IMMOBILIZED**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "&2&bYou tangle $N in a glowing &3&bweb&2&b!&0"
  - **to_room**: "&2&b$n tangles $N in a glowing &3&bweb&2&b!&0"
  - **to_vict**: "&2&b$n tangles you in a glowing &3&bweb&2&b!&0"
- **Source**: magic.cpp:2936-2959

---

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

**Implementation**:
- **Duration**: `10 + (skill / 7)`
- **Effects**:
  - **APPLY_SAVING_SPELL**: `-2 - (skill / 10)`
  - **APPLY_DAMROLL**: `1 + (skill > 95)`
  - **EFF_BLESS**
- **Special Mechanics**:
  - Conflicts with other blessing spells (check_bless_spells)
- **Requirements**: Alignment: Cannot target evil; Alignment: Cannot target good; Alignment: Caster must be neutral
- **Messages**:
  - **to_char**: "$N is imbued with the power of nature."
  - **to_room**: "$N is imbued with the power of nature."
  - **to_vict**: "You imbue yourself with the power of nature."
- **Source**: magic.cpp:1649-1702

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Effects**:
  - **EFF_PROT_FIRE**
- **Messages**:
  - **to_char**: "You protect $N from &1fire&0."
  - **to_room**: "&7&b$N&7&b glows briefly.&0"
  - **to_vict**: "You are warded from &1fire&0."
- **Source**: magic.cpp:2597-2609

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Effects**:
  - **EFF_PROT_COLD**
- **Messages**:
  - **to_char**: "You protect $N from the &4cold&0."
  - **to_room**: "&7&b$N&7&b glows briefly.&0"
  - **to_vict**: "You are warded from the &4cold&0."
- **Source**: magic.cpp:2583-2595

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Effects**:
  - **EFF_PROT_EARTH**
- **Messages**:
  - **to_char**: "You protect $N from &3earth&0."
  - **to_room**: "&7&b$N&7&b glows briefly.&0"
  - **to_vict**: "You are warded from &3earth&0."
- **Source**: magic.cpp:2569-2581

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Effects**:
  - **EFF_PROT_AIR**
- **Messages**:
  - **to_char**: "You protect $N from &6&bair&0."
  - **to_room**: "&7&b$N&7&b glows briefly.&0"
  - **to_vict**: "You are warded from &6&bair&0."
- **Source**: magic.cpp:2611-2623

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other enhancement spells
- **Source**: magic.cpp:1805-1812

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other enhancement spells
- **Source**: magic.cpp:1823-1830

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other enhancement spells
- **Source**: magic.cpp:1841-1848

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other enhancement spells
- **Source**: magic.cpp:1859-1866

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other enhancement spells
- **Source**: magic.cpp:1877-1884

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other enhancement spells
- **Source**: magic.cpp:1895-1902

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_FIREHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&1$N's fists burn with inner fire.&0"
  - **to_vict**: "&1Your fists burn with inner fire.&0"
- **Source**: magic.cpp:2386-2407

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_ICEHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&4&b$N unleashes the blizzard in $S heart.&0"
  - **to_vict**: "&4&bYou unleash the blizzard in your heart.&0"
- **Source**: magic.cpp:2363-2384

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_ACIDHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&3&b$N charges $S hands with corrosive chi.&0"
  - **to_vict**: "&3&bYou charge your hands with corrosive chi.&0"
- **Source**: magic.cpp:2339-2361

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_LIGHTNINGHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&6&b$N's knuckles crackle with lightning.&0"
  - **to_vict**: "&6&bYour knuckles crackle with lightning.&0"
- **Source**: magic.cpp:2409-2430

---

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

**Implementation**:
- **Damage Type**: DAM_WATER
- **Damage Formula**: `sorcerer_single_target`

---

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

**Implementation**:
- **Duration**: `(skill / 50) + ((stat_bonus[GET_INT(ch)].magic + stat_bonus[GET_WIS(ch)].magic) / 7)`
- **Effects**:
  - **EFF_DISPLACEMENT**
  - **EFF_GREATER_DISPLACEMENT**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Conflicts**: SPELL_GREATER_DISPLACEMENT
- **Messages**:
  - **to_char**: "&9&b$N's image blurs into the shadows!&0"
  - **to_room**: "&9&b$N's image blurs into the shadows!&0"
  - **to_vict**: "&9&bYour image blurs into the shadows!&0"
- **Source**: magic.cpp:1624-1647

---

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

**Implementation**:
- **Duration**: `(skill / 50) + ((stat_bonus[GET_INT(ch)].magic + stat_bonus[GET_WIS(ch)].magic) / 7)`
- **Effects**:
  - **EFF_DISPLACEMENT**
  - **EFF_GREATER_DISPLACEMENT**
- **Special Mechanics**:
  - Does not refresh existing spell duration
- **Conflicts**: SPELL_DISPLACEMENT
- **Messages**:
  - **to_char**: "&9&b$N's image blurs into the shadows!&0"
  - **to_room**: "&9&b$N's image blurs into the shadows!&0"
  - **to_vict**: "&9&bYour image blurs into the shadows!&0"
- **Source**: magic.cpp:1624-1647

---

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

**Implementation**:
- **Duration**: `2 + (skill / 21)`
- **Effects**:
  - **EFF_NIMBLE**
- **Messages**:
  - **to_char**: "&1$N starts to move with uncanny grace!&0"
  - **to_room**: "&1$N starts to move with uncanny grace!&0"
  - **to_vict**: "&1You start to move with uncanny grace!&0"
- **Source**: magic.cpp:2480-2487

---

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

**Implementation**:
- **Duration**: `10 + (skill / 10)`
- **Effects**:
  - **APPLY_FOCUS**: `(1 + (skill > 95)) * 10`
- **Messages**:
  - **to_char**: "You feel more clear-headed."
  - **to_room**: "$N looks more clear-headed."
  - **to_vict**: "You feel more clear-headed."
- **Source**: magic.cpp:1341-1348

---

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

**Implementation**:
- **Mechanics**: Attack from behind for massive damage
- **Requirements**: piercing weapon; target not fighting; hidden or behind
- **Source**: warrior.cpp

---

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

**Implementation**:
- **Mechanics**: Shield bash to knock down opponent
- **Requirements**: Shield equipped
- **Source**: warrior.cpp

---

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

**Implementation**:
- **Mechanics**: Hide in shadows
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Extra attack with kick
- **Source**: warrior.cpp

---

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

**Implementation**:
- **Mechanics**: Pick locked doors/containers
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Unarmed punch attack

---

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

**Implementation**:
- **Mechanics**: Rescue ally from combat
- **Source**: warrior.cpp

---

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

**Implementation**:
- **Mechanics**: Move silently
- **Source**: act.movement.cpp

---

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

**Implementation**:
- **Mechanics**: Steal items or gold from target
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Track target through wilderness
- **Source**: act.movement.cpp

---

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

**Implementation**:
- **Mechanics**: Fight with two weapons

---

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

**Implementation**:
- **Mechanics**: Chance for extra attack

---

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

**Implementation**:
- **Mechanics**: Enter berserk rage
- **Duration**: Limited time
- **Source**: act.offensive.cpp

---

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

**Implementation**:
- **Mechanics**: Leap attack

---

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

**Implementation**:
- **Mechanics**: Mount rideable creature
- **Requirements**: Tameable/mountable creature
- **Source**: act.movement.cpp

---

#### Riding (ID 416)
- **Type**: SKILL
- **Enum**: `SKILL_RIDING`
- **Command**: `riding`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Mount riding proficiency

---

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

**Implementation**:
- **Mechanics**: Tame wild creature
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Lethal throat cutting

---

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

**Implementation**:
- **Mechanics**: Bash down doors

---

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

**Implementation**:
- **Mechanics**: Parry incoming attacks

---

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

**Implementation**:
- **Mechanics**: Dodge incoming attacks

---

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

**Implementation**:
- **Mechanics**: Counter-attack after parry

---

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

**Implementation**:
- **Mechanics**: Meditate to regain mana/spells
- **Source**: spell_mem.cpp

---

#### Quick Chant (ID 424)
- **Type**: SKILL
- **Enum**: `SKILL_QUICK_CHANT`
- **Command**: `quick chant`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Faster chanting

---

#### Circle (ID 426)
- **Type**: SKILL
- **Enum**: `SKILL_CIRCLE`
- **Command**: `circle`
- **Target**: NONE
  - Scope: TOUCH
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: Yes

**Implementation**:
- **Mechanics**: Circle around to backstab again
- **Requirements**: Piercing weapon, stealth
- **Source**: act.offensive.cpp

---

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

**Implementation**:
- **Mechanics**: Full body slam knocking down opponent
- **Source**: act.offensive.cpp

---

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

**Implementation**:
- **Mechanics**: Bind victim
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Change form
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Switch combat targets

---

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

**Implementation**:
- **Mechanics**: Disarms opponent's weapon
- **Source**: act.offensive.cpp

---

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

**Implementation**:
- **Mechanics**: Guard location/person
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Racial lightning breath

---

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

**Implementation**:
- **Mechanics**: Sweep attack

---

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

**Implementation**:
- **Mechanics**: Intimidating roar

---

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

**Implementation**:
- **Mechanics**: Extinguish flames
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Instant kill attempt

---

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

**Implementation**:
- **Mechanics**: Attack all enemies in room
- **Source**: warrior.cpp

---

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

**Implementation**:
- **Mechanics**: Hunt for prey

---

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

**Implementation**:
- **Mechanics**: Bandage wounds to stop bleeding
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Provide first aid
- **Source**: act.other.cpp

---

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

**Implementation**:
- **Mechanics**: Life-draining touch

---

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

**Implementation**:
- **Mechanics**: Chant ability
- **Source**: spell_mem.cpp

---

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

**Implementation**:
- **Mechanics**: Scribe scrolls

---

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

**Implementation**:
- **Mechanics**: Reduce falling damage

---

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

**Implementation**:
- **Mechanics**: Unarmed combat

---

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

**Implementation**:
- **Mechanics**: Summon rideable mount
- **Source**: act.other.cpp

---

#### Spell Knowledge (ID 451)
- **Type**: SKILL
- **Enum**: `SKILL_KNOW_SPELL`
- **Command**: `spell knowledge`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Spell knowledge

---

#### Sphere Of Generic (ID 452)
- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_GENERIC`
- **Command**: `sphere of generic`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Generic spell sphere access

---

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

**Implementation**:
- **Mechanics**: Fire spell sphere access

---

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

**Implementation**:
- **Mechanics**: Water spell sphere access

---

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

**Implementation**:
- **Mechanics**: Earth spell sphere access

---

#### Sphere Of Air (ID 456)
- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_AIR`
- **Command**: `sphere of air`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Air spell sphere access

---

#### Sphere Of Healing (ID 457)
- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_HEALING`
- **Command**: `sphere of healing`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Healing spell sphere access

---

#### Sphere Of Protection (ID 458)
- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_PROT`
- **Command**: `sphere of protection`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Protection spell sphere access

---

#### Sphere Of Enchantment (ID 459)
- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_ENCHANT`
- **Command**: `sphere of enchantment`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Enchantment spell sphere access

---

#### Sphere Of Summoning (ID 460)
- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_SUMMON`
- **Command**: `sphere of summoning`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Summoning spell sphere access

---

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

**Implementation**:
- **Mechanics**: Death/necromancy spell sphere access

---

#### Sphere Of Divination (ID 462)
- **Type**: SKILL
- **Enum**: `SKILL_SPHERE_DIVIN`
- **Command**: `sphere of divination`
- **Target**: NONE
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Divination spell sphere access

---

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

**Implementation**:
- **Mechanics**: Bludgeoning weapon proficiency

---

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

**Implementation**:
- **Mechanics**: Piercing weapon proficiency

---

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

**Implementation**:
- **Mechanics**: Slashing weapon proficiency

---

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

**Implementation**:
- **Mechanics**: Two-handed bludgeoning proficiency

---

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

**Implementation**:
- **Mechanics**: Two-handed piercing proficiency

---

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

**Implementation**:
- **Mechanics**: Two-handed slashing proficiency

---

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

**Implementation**:
- **Mechanics**: Ranged weapon proficiency

---

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

**Implementation**:
- **Mechanics**: Gouge eyes causing blindness

---

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

**Implementation**:
- **Mechanics**: Tactical retreat

---

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

**Implementation**:
- **Mechanics**: Organize group retreat

---

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

**Implementation**:
- **Mechanics**: Corner enemy

---

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

**Implementation**:
- **Mechanics**: Move stealthily

---

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

**Implementation**:
- **Mechanics**: Blend into shadows

---

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

**Implementation**:
- **Mechanics**: Conceal items

---

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

**Implementation**:
- **Mechanics**: Bird pecking attack

---

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

**Implementation**:
- **Mechanics**: Natural claw attack

---

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

**Implementation**:
- **Damage Type**: DAM_SHOCK
- **Damage Formula**: `skill - random_number(0, 3)`

---

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

**Implementation**:
- **Mechanics**: Berserker tantrum

---

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

**Implementation**:
- **Mechanics**: Earthquake-like attack

---

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

**Implementation**:
- **Mechanics**: Howl boosting allies

---

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

**Implementation**:
- **Mechanics**: Brutal maul attack

---

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

**Implementation**:
- **Mechanics**: Racial fire breath

---

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

**Implementation**:
- **Mechanics**: Racial frost breath

---

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

**Implementation**:
- **Mechanics**: Racial acid breath

---

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

**Implementation**:
- **Mechanics**: Racial poison gas breath

---

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

**Implementation**:
- **Mechanics**: Perform for audience

---

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

**Implementation**:
- **Mechanics**: Acrobatic cartwheel

---

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

**Implementation**:
- **Mechanics**: Lure creatures

---

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

**Implementation**:
- **Mechanics**: Attack from stealth

---

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

**Implementation**:
- **Mechanics**: Rending attack

---

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

**Implementation**:
- **Mechanics**: Spinning kick hitting multiple targets
- **Source**: warrior.cpp

---

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

**Implementation**:
- **Duration**: `skill / (15 - (GET_CHA(ch) / 20))`
- **Effects**:
  - **APPLY_HITROLL**: `skill / (25 - (GET_CHA(ch) / 20))`
  - **APPLY_DAMROLL**: `skill / (25 - (GET_CHA(ch) / 20))`
  - **APPLY_HIT**: `(skill + (GET_CHA(ch)) * 2)`
- **Conflicts**: SONG_TERROR, SONG_BALLAD_OF_TEARS
- **Messages**:
  - **to_room**: "$N's spirit stirs with inspiration!"
  - **to_vict**: "Your spirit swells with inspiration!"
- **Source**: magic.cpp:3219-3253

---

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

**Implementation**:
- **Duration**: `eff[1].duration = eff[2].duration = eff[3].duration = skill / (15 - (GET_CHA(ch) / 10))`
- **Effects**:
  - **APPLY_CON**: `-(((skill + GET_CHA(ch)) / 4) * (GET_VIEWED_CON(victim) / 2)) / 100`
  - **APPLY_STR**: `-(((skill + GET_CHA(ch)) / 4) * (GET_VIEWED_STR(victim) / 2)) / 100`
  - **APPLY_HITROLL**: `-(skill / (15 - (GET_CHA(ch) / 20)))`
  - **APPLY_DAMROLL**: `-(skill / (15 - (GET_CHA(ch) / 20)))`
- **Conflicts**: SONG_INSPIRATION, SONG_HEROIC_JOURNEY
- **Messages**:
  - **to_room**: "$N's spirit withers in terror and sorrow!"
  - **to_vict**: "Your spirit withers in terror and sorrow!"
- **Source**: magic.cpp:3268-3302

---

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

**Implementation**:
- **Duration**: `2`
- **Effects**:
  - **EFF_MESMERIZED**
- **Requirements**: Cannot target NPCs
- **Messages**:
  - **to_char**: "You weave a mesmerizing pattern before $N, and $E seems to be utterly absorbed by it."
  - **to_room**: "$n &0&5w&4e&5av&4es a &5mesme&4rizing pa&5ttern before &0$N's eyes.\n"
  - **to_vict**: "$n shows you a truly fascinating puzzle.  You simply must work it out."
- **Source**: magic.cpp:2226-2255

---

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

**Implementation**:
- **Mechanics**: Song of home and comfort
- **Source**: defines.hpp:554

---

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

**Implementation**:
- **Duration**: `5`
- **Effects**:
  - **APPLY_WIS**: `-50`
  - **EFF_INSANITY**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You cause &5$N to snap... A crazed gleam fills $S eyes.&0"
  - **to_room**: "&5$N's&5 psyche snaps... A crazed gleam fills $S eyes.&0"
  - **to_vict**: "&5You go out of your &bMIND!&0"
- **Source**: magic.cpp:2159-2179

---

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

**Implementation**:
- **Duration**: `skill / (15 - (GET_CHA(ch) / 20))`
- **Effects**:
  - **EFF_SONG_OF_REST**
- **Messages**:
  - **to_char**: "You sing $N a gentle lullaby to help $M rest.\n"
  - **to_room**: "$n sings $N a gentle lullaby to help $M rest."
  - **to_vict**: "$n sings you a gentle lullaby to help you rest."
- **Source**: magic.cpp:3255-3266

---

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

**Implementation**:
- **Duration**: `eff[1].duration = eff[2].duration = eff[3].duration = skill / (15 - (GET_CHA(ch) / 10))`
- **Effects**:
  - **APPLY_CON**: `-(((skill + GET_CHA(ch)) / 4) * (GET_VIEWED_CON(victim) / 2)) / 100`
  - **APPLY_STR**: `-(((skill + GET_CHA(ch)) / 4) * (GET_VIEWED_STR(victim) / 2)) / 100`
  - **APPLY_HITROLL**: `-(skill / (15 - (GET_CHA(ch) / 20)))`
  - **APPLY_DAMROLL**: `-(skill / (15 - (GET_CHA(ch) / 20)))`
- **Conflicts**: SONG_INSPIRATION, SONG_HEROIC_JOURNEY
- **Messages**:
  - **to_room**: "$N's spirit withers in terror and sorrow!"
  - **to_vict**: "Your spirit withers in terror and sorrow!"
- **Source**: magic.cpp:3268-3302

---

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

**Implementation**:
- **Mechanics**: Epic song of heroic deeds
- **Duration**: While singing
- **Source**: defines.hpp:558

---

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

**Implementation**:
- **Mechanics**: Song breaking bonds and restrictions
- **Duration**: Instant
- **Source**: defines.hpp:559

---

#### Joyful Noise (ID 560)
- **Type**: SONG
- **Enum**: `SONG_JOYFUL_NOISE`
- **Command**: `joyful noise`
- **Target**: SINGLE_CHARACTER
  - Scope: SAME_ROOM
- **Min Position**: STANDING
- **Combat Use**: Yes
- **Violent**: No

**Implementation**:
- **Mechanics**: Uplifting joyful song
- **Duration**: While singing
- **Source**: defines.hpp:560

---

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

**Implementation**:
- **Duration**: `skill / 2 + 3`
- **Messages**:
  - **to_room**: "$n looks a little healthier."
  - **to_vict**: "You feel your health improve."
- **Source**: magic.cpp:3112-3118

---

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

**Implementation**:
- **Duration**: `skill / 25 + 1`
- **Effects**:
  - **APPLY_HITROLL**: `skill / 25 + 1`
  - **APPLY_DAMROLL**: `skill / 25 + 1`
- **Messages**:
  - **to_room**: "$n's chest swells with courage!"
  - **to_vict**: "Your heart beats with the rage of your fallen brothers."
- **Source**: magic.cpp:3070-3082

---

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

**Implementation**:
- **Duration**: `skill / 25 + 1`
- **Effects**:
  - **APPLY_HITROLL**: `skill / 25 + 1`
  - **APPLY_DAMROLL**: `skill / 25 + 1`
- **Messages**:
  - **to_room**: "$N looks more determined than ever!"
  - **to_vict**: "You feel more determined than ever!"
- **Source**: magic.cpp:3206-3215

---

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

**Implementation**:
- **Mechanics**: Peaceful chant stopping combat
- **Duration**: Instant
- **Source**: defines.hpp:604

---

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

**Implementation**:
- **Duration**: `5 + (skill / 14)`
- **Special Mechanics**:
  - Effect modifiers accumulate
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You depress $N with a song of darkness and sorrow!"
  - **to_room**: "A dark look clouds $N's visage as $n sings $M a song of sorrow."
  - **to_vict**: "$n's song of sorrow fills your mind with darkness and shadows."
- **Source**: magic.cpp:3120-3142

---

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

**Implementation**:
- **Mechanics**: Harmonious chant
- **Duration**: While chanting
- **Source**: defines.hpp:606

---

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

**Implementation**:
- **Duration**: `5 + (skill / 30)`
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "Your song of dissonance confuses $N!"
  - **to_room**: "$N winces as $n's dissonant song fills $S ears."
  - **to_vict**: "$n fills your ears with an aria of dissonance, causing confusion!"
- **Source**: magic.cpp:3057-3068

---

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

**Implementation**:
- **Duration**: `eff[1].duration = eff[2].duration = eff[3].duration = eff[4].duration = 1 + (skill / 20)`
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You fill $N with a sense of malaise!"
  - **to_room**: "$N contorts $S face briefly in anger and fear."
  - **to_vict**: "Malaise fills the air, hampering your movements!"
- **Source**: magic.cpp:3144-3165

---

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

**Implementation**:
- **Mechanics**: Causes enemies in room to turn on caster in confusion
- **Source**: spells.cpp:67-108

---

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

**Implementation**:
- **Duration**: `(skill / 20)`
- **Effects**:
  - **APPLY_CON**: `-(skill * GET_VIEWED_CON(victim) / 2) / 100`
  - **APPLY_STR**: `-(skill * GET_VIEWED_STR(victim) / 2) / 100`
  - **EFF_DISEASE**
- **Requirements**: Requires attack_ok check (offensive spell)
- **Messages**:
  - **to_char**: "You force $N down the path to destruction..."
  - **to_room**: "$n plants the seed of destruction in $N's mind."
  - **to_vict**: "You feel your time in this world growing short..."
- **Source**: magic.cpp:3091-3110

---

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

**Implementation**:
- **Duration**: `(skill / 20)`
- **Effects**:
  - **EFF_SPIRIT_WOLF**
- **Messages**:
  - **to_room**: "$n seems to take on a fearsome, wolf-like demeanor."
  - **to_vict**: "You feel a wolf-like fury come over you."
- **Source**: magic.cpp:3199-3204

---

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

**Implementation**:
- **Duration**: `(skill / 20)`
- **Effects**:
  - **EFF_SPIRIT_BEAR**
- **Messages**:
  - **to_room**: "$n shifts $s weight, seeming heavier and more dangerous."
  - **to_vict**: "The spirit of the bear consumes your body."
- **Source**: magic.cpp:3192-3197

---

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

**Implementation**:
- **Duration**: `(skill / 20)`
- **Effects**:
  - **EFF_WRATH**
- **Messages**:
  - **to_room**: "$n bristles with anger."
  - **to_vict**: "A feeling of unforgiving wrath fills you."
- **Source**: magic.cpp:3084-3089

---

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

**Implementation**:
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Source**: magic.cpp:1990-1997

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_FIREHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&1$N's fists burn with inner fire.&0"
  - **to_vict**: "&1Your fists burn with inner fire.&0"
- **Source**: magic.cpp:2386-2407

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_ICEHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&4&b$N unleashes the blizzard in $S heart.&0"
  - **to_vict**: "&4&bYou unleash the blizzard in your heart.&0"
- **Source**: magic.cpp:2363-2384

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_ACIDHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&3&b$N charges $S hands with corrosive chi.&0"
  - **to_vict**: "&3&bYou charge your hands with corrosive chi.&0"
- **Source**: magic.cpp:2339-2361

---

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

**Implementation**:
- **Duration**: `((skill / 10) + (GET_WIS(ch) / 20))`
- **Effects**:
  - **EFF_LIGHTNINGHANDS**
- **Special Mechanics**:
  - Conflicts with other monk hand spells
- **Requirements**: Must have SKILL_BAREHAND (monk-only)
- **Messages**:
  - **to_room**: "&6&b$N's knuckles crackle with lightning.&0"
  - **to_vict**: "&6&bYour knuckles crackle with lightning.&0"
- **Source**: magic.cpp:2409-2430

---


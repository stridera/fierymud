# Effects System Review

This document catalogs all effects in the FieryMUD magic system for review and consolidation decisions.

---

## NEW: Consolidated Parameterized Effects Design

Based on review, we're consolidating ~142 legacy effects into **~25 parameterized effects** using the new combat system (Accuracy/Evasion).

### Final 33 Effect Types

#### Combat Effects (7)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **damage** | `{type, amount, scaling}` | All damage types | `{type: "fire", amount: "level * 5"}` |
| **heal** | `{resource, amount, scaling}` | All healing | `{resource: "hp", amount: "2d8 + level"}` |
| **dot** | `{type, amount, interval, duration}` | POISON, DISEASE, ON_FIRE | `{type: "poison", amount: "2%", interval: 1}` |
| **chain_damage** | `{type, amount, maxJumps, attenuation}` | CHAIN_LIGHTNING | `{type: "shock", amount: 100, maxJumps: 5, attenuation: 0.7}` |
| **lifesteal** | `{percent, duration}` | VAMP_TOUCH | `{percent: 25, duration: "level"}` |
| **reflect** | `{type, amount, duration}` | FIRESHIELD, COLDSHIELD | `{type: "fire", amount: 25}` |
| **damage_mod** | `{amount, duration}` | APPLY_DAMROLL | `{amount: 3, duration: "skill / 4"}` |

#### Defense/Combat Stats (4)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **accuracy_mod** | `{amount, duration}` | APPLY_HITROLL (new combat) | `{amount: 10, duration: "level * 2"}` |
| **evasion_mod** | `{amount, duration}` | APPLY_AC, BLUR, STONESKIN (new combat) | `{amount: 20, duration: "level * 2"}` |
| **protection** | `{type, amount, duration}` | PROT_FIRE, PROTECT_EVIL, etc. | `{type: "fire", amount: 50}` |
| **vulnerability** | `{type, amount, duration}` | (opposite of protection) | `{type: "cold", amount: 150}` |

#### Stat Modifiers (4)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **stat_mod** | `{stat, amount, duration}` | APPLY_STR, DEX, CON, INT, WIS, CHA | `{stat: "str", amount: 4}` |
| **resource_mod** | `{resource, amount, duration}` | APPLY_HIT, MANA, MOVE | `{resource: "hp", amount: 50}` |
| **saving_mod** | `{type, amount, duration}` | All APPLY_SAVING_* | `{type: "spell", amount: -3}` |
| **size_mod** | `{amount, duration}` | ENLARGE, REDUCE | `{amount: 1}` |

#### Status Effects (3)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **status** | `{flag, duration, breakable}` | BLIND, BLESS, SANCTUARY, etc. | `{flag: "sanctuary", duration: "level * 2"}` |
| **crowd_control** | `{type, duration, breakOnDamage}` | SLEEP, PARALYZE, CHARM, FEAR, etc. | `{type: "paralyze", duration: 5}` |
| **cure** | `{condition, scope}` | REMOVE_POISON, REMOVE_CURSE, etc. | `{condition: "poison", scope: "all"}` |

#### Utility Buffs (5)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **stealth** | `{type, level}` | INVISIBLE, CAMOUFLAGED | `{type: "invisible", level: "skill"}` |
| **detection** | `{type, duration}` | DETECT_INVIS, DETECT_MAGIC, etc. | `{type: "invisible", duration: "level * 2"}` |
| **movement** | `{type, duration}` | FLY, WATERWALK, HASTE, etc. | `{type: "fly", duration: "level * 3"}` |
| **elemental_hands** | `{element, duration}` | FIREHANDS, ICEHANDS, etc. | `{element: "fire", duration: "skill / 4"}` |
| **globe** | `{maxCircle, duration}` | MINOR_GLOBE, MAJOR_GLOBE | `{maxCircle: 3, duration: "level"}` |

#### Magic Manipulation (2)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **dispel** | `{filter, power, scope}` | DISPEL_MAGIC, DISPEL_EVIL/GOOD | `{filter: "magic", power: "level", scope: "all"}` |
| **reveal** | `{type, depth, autoDispel}` | IDENTIFY, LOCATE, REVEAL_HIDDEN | `{type: "object", depth: "world"}` |

#### Teleportation (3)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **teleport** | `{destination, restrictions}` | TELEPORT, WORD_OF_RECALL, DIMENSION_DOOR | `{destination: "home"}` |
| **banish** | `{successTiers, extractCondition}` | BANISH | `{successTiers: [50, 100], extractCondition: "npc"}` |
| **broadcast_teleport** | `{destination, groupFilter}` | GROUP_RECALL | `{destination: "home", groupFilter: "same_room"}` |

#### Creation/Summoning (3)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **summon** | `{mobType, duration, maxCount}` | SUMMON_ELEMENTAL, SUMMON_DEMON | `{mobType: "fire_elemental", duration: 100}` |
| **create_object** | `{objectType, decay}` | CREATE_FOOD, CREATE_WATER | `{objectType: "food", decay: 24}` |
| **resurrect** | `{expPenalty, equipTransfer}` | RESURRECT, RAISE_DEAD | `{expPenalty: 0.1, equipTransfer: true}` |

#### Room/Area Effects (2)

| Effect | Parameters | Replaces | Example |
|--------|------------|----------|---------|
| **room_effect** | `{type, duration, areaOfEffect}` | CIRCLE_OF_FIRE, DARKNESS | `{type: "darkness", duration: 10}` |
| **room_barrier** | `{type, traversalRules, hp}` | WALL_OF_FOG/STONE/ICE | `{type: "stone", traversalRules: "block", hp: 100}` |

### Parameter Types

| Type | Values | Notes |
|------|--------|-------|
| **element** | fire, cold, acid, shock, poison, magic, holy, unholy, physical | Damage/protection types |
| **stat** | str, dex, con, int, wis, cha | Ability scores |
| **resource** | hp, mana, move | Resource pools |
| **cc_type** | paralyze, sleep, charm, fear, confuse, silence, slow | Crowd control types |
| **detect_type** | invisible, magic, align, poison, life | Detection types |
| **move_type** | fly, waterwalk, waterbreath, haste, featherfall | Movement abilities |

### Amount Can Be:
- **Number**: `50` (flat value)
- **Percentage**: `"25%"` (of max HP, etc.)
- **Dice**: `"2d6 + 4"` (rolled)
- **Formula**: `"level * 2"`, `"skill / 4"`, `"int * 0.5"` (calculated)

---

## Consolidation Summary

| Old System | New System | Reduction |
|------------|------------|-----------|
| 90 EFF_* status flags | 33 parameterized effects | -63% |
| 31 APPLY_* modifiers | (included in 33) | -100% |
| 21 damage types | (1 effect with type param) | -95% |
| AC/THAC0 combat | Accuracy/Evasion combat | Modernized |
| **142 total effects** | **33 total effects** | **-77%** |

### Coverage

| Ability Type | Count | Coverage |
|--------------|-------|----------|
| Damage spells | ~80 | 100% via `damage` |
| Healing spells | ~20 | 100% via `heal` |
| Buffs/Protection | ~60 | 100% via `protection`, `stat_mod`, etc. |
| Crowd Control | ~25 | 100% via `crowd_control`, `status` |
| Utility spells | ~50 | 100% via various effects |
| Special mechanics | ~30 | 95% via new effects (chain, dispel, etc.) |
| **Total** | **~265** | **~95%** |

Remaining ~5% use generic `scripted` effect with Lua for unique mechanics.

---

## Legacy Reference (Below)

The following sections document the OLD effects for reference during migration.

---

## Status Effects (EFF_* Flags)

### Crowd Control (CC) Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **BLIND** | Target cannot see, -40 AC, cannot target | Blindness, Color Spray, Blinding Beauty | - | Basilisks, mage NPCs | Y |
| **SLEEP** | Target incapacitated until damaged | Sleep, Nightmare | - | - | Y |
| **PARALYSIS** (minor) | Short-duration cannot act | Color Spray, Moonbeam | - | - | Y |
| **PARALYSIS** (major) | Long-duration cannot act | Hold Person, Hold Monster, Bone Cage | - | Ghouls, Liches, Mind Flayers | Y |
| **CHARM** | Target follows caster's commands | Charm Person, Dominate, Mesmerize | - | Vampires, Succubi | Y |
| **FEAR** | Target flees in terror | Fear, Scare, Horror | - | Dragons, Demons, Undead Lords | Y |
| **CONFUSION** | Target acts randomly | Confusion, Insanity | - | - | Y |
| **SILENCE** | Target cannot cast spells | Silence | - | Anti-mage NPCs | Y |
| **SLOW** | Reduced attacks and movement | Slow | - | - | Y |
| **WEB** | Target immobilized in place | Web | - | Spiders | Y |
| **IMMOBILIZED** | Cannot move (different from paralysis) | Bone Cage | - | - | Merge with PARALYSIS? |
| **MESMERIZED** | Stunned state (breaks on damage) | Mesmerize | - | - | Merge with CHARM? |
| **INSANITY** | Severe confusion, chaotic behavior | Insanity | - | - | Merge with CONFUSION? |

**Consolidation Candidates:**
- IMMOBILIZED vs PARALYSIS - same mechanic?
- MESMERIZED vs CHARM - similar effect?
- INSANITY vs CONFUSION - just stronger version?

---

### Damage Over Time (DoT) Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **POISON** | 2% HP per tick, lasts 30 ticks | Poison, Envenom, Cloudkill | Envenom (skill) | Snakes, Spiders, Scorpions | Y |
| **DISEASE** | Periodic damage + stat drain | Disease, Contagion | - | Rats, Undead, Demons | Y |
| **ON_FIRE** | Burning, fire damage per tick | Immolate | - | Fire elementals | Y |
| **CURSE** | -hitroll, -damroll (not DoT but debuff) | Curse, Bestow Curse, Doom | - | Witches, Liches | Y |
| **EXPOSED** | Target takes increased damage | Expose Weakness | - | - | Rare, keep? |

---

### Buff Effects (Positive)

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **SANCTUARY** | 50% damage reduction | Sanctuary, Mass Sanctuary | - | High Priests, Angels | Y |
| **INVISIBLE** | Cannot be seen by normal sight | Invisibility, Mass Invis, Greater Invis | Hide (skill) | Rogues, Spirits, Shadows | Y |
| **HASTE** | Extra attacks, faster movement | Haste, Mass Haste | Battle Hymn (chant) | Elite Warriors, Demons | Y |
| **BLESS** | Holy attacks work vs undead | Bless, Prayer, Divine Favor | - | Clerics | Y |
| **FLY** | Flight capability | Fly, Mass Fly, Wings of Heaven | - | Flying creatures | Y |
| **STONESKIN** | Damage reduction (stacks with armor) | Stoneskin, Barkskin, Bone Armor | - | Stone golems | Y |
| **BLUR** | Evasion bonus, harder to hit | Blur, Displacement | - | - | Y |
| **GREATER_DISPLACEMENT** | High evasion bonus | Greater Displacement | - | - | Merge with BLUR? |
| **BERSERK** | Rage: +damage, +HP, reduced defense | Berserk | Berserk (skill) | Berserkers | Y |
| **AWARE** | Cannot be surprised, +perception | Awareness | Awareness (skill) | - | Rare |

**Consolidation Candidates:**
- BLUR vs GREATER_DISPLACEMENT - just different magnitudes?

---

### Protection Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **PROTECT_EVIL** | Protection vs evil-aligned creatures | Protection from Evil, Holy Word | - | Paladins, Angels | Y |
| **PROTECT_GOOD** | Protection vs good-aligned creatures | Protection from Good, Unholy Word | - | Anti-paladins, Demons | Y |
| **FIRESHIELD** | Reflect fire damage to attackers | Fire Shield | - | Fire Elementals | Y |
| **COLDSHIELD** | Reflect cold damage to attackers | Cold Shield | - | Ice Elementals | Y |
| **PROT_FIRE** | Fire resistance (reduces fire damage) | Resist Fire, Dragon spells | - | Fire-resistant creatures | Y |
| **PROT_COLD** | Cold resistance | Resist Cold | - | Cold-resistant creatures | Y |
| **PROT_ACID** | Acid resistance | Resist Acid | - | - | Rare |
| **PROT_SHOCK** | Shock/lightning resistance | Resist Shock | - | - | Rare |
| **MINOR_GLOBE** | Blocks spells circle 1-3 | Minor Globe of Invulnerability | - | - | Y |
| **MAJOR_GLOBE** | Blocks spells circle 1-6 | Major Globe of Invulnerability | - | Archmages | Y |

**Consolidation Candidates:**
- FIRESHIELD vs PROT_FIRE - reflect vs resist (different mechanics, keep both)
- PROT_ACID, PROT_SHOCK - rarely used, consolidate into generic "elemental_resist"?

---

### Detection/Sense Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **DETECT_INVIS** | Can see invisible creatures/objects | Detect Invisible, True Seeing | - | - | Y |
| **DETECT_MAGIC** | See magical auras on items/creatures | Detect Magic, Identify | - | - | Y |
| **DETECT_ALIGN** | See alignment auras (good/evil/neutral) | Detect Alignment, Know Alignment | - | - | Y |
| **DETECT_POISON** | See poison on items/creatures | Detect Poison | - | - | Rare |
| **SENSE_LIFE** | Detect living creatures (even hidden) | Sense Life | - | - | Y |
| **INFRAVISION** | See in darkness (heat vision) | Infravision, Night Vision | - | Many monster races | Y |
| **FARSEE** | Extended vision range | Farsee | - | - | Rare |

**Consolidation Candidates:**
- DETECT_POISON - rarely used, merge into SENSE_LIFE?
- FARSEE - rarely used, keep?

---

### Movement/Utility Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **FLY** | Flight capability | Fly, Wings of Heaven | - | Flying creatures | Y |
| **WATERWALK** | Walk on water surfaces | Waterwalk | - | Water striders | Y |
| **WATERBREATH** | Breathe underwater | Waterbreathing | - | Aquatic creatures | Y |
| **FEATHER_FALL** | Slow falling, no fall damage | Feather Fall | - | - | Rare |
| **FAMILIARITY** | Remember location for Word of Recall | Word of Recall prep | - | - | Rare, system use |
| **TETHERED** | Location tracking (Locate spells) | Locate Object, Locate Person | - | - | Rare, system use |

---

### Combat Enhancement Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **FIREHANDS** | Add fire damage to melee attacks | Fire Darts, Flame Blade | - | Fire Elementals, Fire Giants | Y |
| **ICEHANDS** | Add cold damage to melee attacks | Chill Touch, Ice Blade | - | Ice Elementals, Frost Giants | Y |
| **LIGHTNING_HANDS** | Add shock damage to melee attacks | Shocking Grasp | - | - | Rare |
| **ACID_HANDS** | Add acid damage to melee attacks | Acid Touch | - | - | Rare |
| **WRATH** | Holy damage bonus to attacks | Wrath | - | - | Y |
| **VAMP_TOUCH** | Life steal on melee attacks | Vampiric Touch | - | Vampires | Y |

**Consolidation Candidates:**
- FIREHANDS, ICEHANDS, LIGHTNING_HANDS, ACID_HANDS - consolidate into "elemental_hands" with damage type parameter?

---

### Weapon Enchantment Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **FIRE_WEAPON** | Weapon deals fire damage | Enchant Weapon (Fire) | - | - | Rare |
| **COLD_WEAPON** | Weapon deals cold damage | Enchant Weapon (Cold) | - | - | Rare |
| **LIGHT_WEAPON** | Weapon deals light/radiant damage | Enchant Weapon (Light) | - | - | Rare |
| **DARK_WEAPON** | Weapon deals dark/shadow damage | Enchant Weapon (Dark) | - | - | Rare |
| **ACID_WEAPON** | Weapon deals acid damage | - | - | - | UNUSED |
| **RADIANT_WEAPON** | Weapon deals radiant damage | - | - | - | UNUSED |

**Consolidation Candidates:**
- All weapon enchants could be one effect "weapon_enchant" with element type parameter
- ACID_WEAPON, RADIANT_WEAPON - completely unused, DELETE

---

### Transform/Shape Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **ENLARGE** | Increase creature size (+damage, -AC) | Enlarge | - | Giants | Rare |
| **REDUCE** | Decrease creature size (-damage, +AC) | Reduce | - | - | Rare |
| **SPIRIT_WOLF** | Transform into wolf form | Spirit Wolf | - | - | Rare |
| **SPIRIT_BEAR** | Transform into bear form | Spirit Bear | - | - | Rare |
| **ANIMATED** | Undead animated state | Animate Dead | - | Animated undead | Y |
| **CAMOUFLAGED** | Hidden in natural terrain | Camouflage | - | - | Rare |

---

### Special/System Effects

| Effect | What It Does | Spells | Skills | Mobs | Keep? |
|--------|--------------|--------|--------|------|-------|
| **GLORY** | Divine presence aura | Divine spells | - | Divine beings | Rare |
| **NOPAIN** | Ignore pain (continue fighting at low HP) | Numb | - | - | Rare |
| **HARNESS** | Mount capability | Mount spells | Ride (skill) | - | System |
| **REMOTE_AGGR** | Generate aggro from distance | Remote aggro spells | - | - | Rare |

---

### UNUSED Effects (Candidates for Deletion)

| Effect | What It Was For | Status | Recommendation |
|--------|-----------------|--------|----------------|
| **SONG_HEALING** | Bard healing song | Never implemented | DELETE |
| **SONG_VALOR** | Bard combat song | Never implemented | DELETE |
| **SONG_LULLABY** | Bard sleep song | Never implemented | DELETE |
| **SONG_FEAR** | Bard fear song | Never implemented | DELETE |
| **SONG_CHARM** | Bard charm song | Never implemented | DELETE |
| **FEIGNED_DEATH** | Play dead mechanic | Never implemented | DELETE |
| **REFLECT_DAMAGE** | Thorns-style effect | Never implemented | DELETE |
| **MISDIRECTION** | Threat redirect | Never implemented | DELETE |
| **ACID_WEAPON** | Acid weapon enchant | Never used | DELETE |
| **RADIANT_WEAPON** | Radiant weapon enchant | Never used | DELETE |
| **Reserved slots (72-74, 83-84, 86-87)** | Future expansion | Unused | DELETE |

---

## Stat Modifiers (APPLY_* Effects)

### Core Combat Modifiers (Essential)

| Modifier | What It Does | Spells | Skills | Keep? |
|----------|--------------|--------|--------|-------|
| **APPLY_AC** | Armor class improvement | Armor, Shield, Barkskin, Stoneskin | - | Y |
| **APPLY_HITROLL** | Attack bonus | Bless, Prayer, Battle Hymn | - | Y |
| **APPLY_DAMROLL** | Damage bonus | Bless, Dark Presence, Wrath | - | Y |
| **APPLY_SAVING_SPELL** | Spell resistance | Bless, Protection spells | - | Y |

### Stat Modifiers (Essential)

| Modifier | What It Does | Spells | Skills | Keep? |
|----------|--------------|--------|--------|-------|
| **APPLY_STR** | Strength bonus | Strength, Bull's Strength | - | Y |
| **APPLY_DEX** | Dexterity bonus | Dexterity, Cat's Grace | - | Y |
| **APPLY_CON** | Constitution bonus | Endurance, Bear's Endurance | - | Y |
| **APPLY_INT** | Intelligence bonus | Intelligence, Fox's Cunning | - | Y |
| **APPLY_WIS** | Wisdom bonus | Wisdom, Owl's Wisdom | - | Y |
| **APPLY_CHA** | Charisma bonus | Charisma, Eagle's Splendor | - | Y |

### Resource Modifiers (Essential)

| Modifier | What It Does | Spells | Skills | Keep? |
|----------|--------------|--------|--------|-------|
| **APPLY_HIT** | Max HP bonus | Endurance, Vitality, Dragon's Health | - | Y |
| **APPLY_MANA** | Max Mana bonus | - | - | Y |
| **APPLY_MOVE** | Max Movement bonus | - | - | Y |

### Other Saving Throws

| Modifier | What It Does | Spells | Skills | Keep? |
|----------|--------------|--------|--------|-------|
| **APPLY_SAVING_PARA** | vs Paralysis | Protection spells | - | Consolidate? |
| **APPLY_SAVING_ROD** | vs Rods/Staves | Protection spells | - | Consolidate? |
| **APPLY_SAVING_PETRI** | vs Petrification | Protection spells | - | Consolidate? |
| **APPLY_SAVING_BREATH** | vs Breath Weapons | Protection spells | - | Consolidate? |

**Consolidation Candidates:**
- All saving throws could be one "saving_modifier" with type parameter
- Or just use APPLY_SAVING_SPELL for everything

### Rarely Used Modifiers

| Modifier | What It Does | Spells | Skills | Keep? |
|----------|--------------|--------|--------|-------|
| **APPLY_SIZE** | Creature size modifier | Enlarge, Reduce | - | Rare |
| **APPLY_FOCUS** | Spell concentration | Clarity, Focus | - | Rare |
| **APPLY_PERCEPTION** | Detection rolls | Sense Life variants | - | Rare |
| **APPLY_HIDE** | Stealth bonus | Invisibility variants | - | Rare |
| **APPLY_SNEAK** | Move silently | Silence variants | - | Rare |

### UNUSED Modifiers (Delete)

| Modifier | What It Was For | Recommendation |
|----------|-----------------|----------------|
| **APPLY_AGE** | Age effects | DELETE |
| **APPLY_WEIGHT** | Carry capacity | DELETE |
| **APPLY_HEIGHT** | Size display | DELETE |
| **APPLY_RACE** | Race modifier | DELETE |
| **APPLY_CLASS** | Class modifier | DELETE |
| **APPLY_LEVEL** | Level modifier | DELETE |
| **APPLY_COMPOSITION** | Material type | DELETE or keep for objects? |
| **APPLY_ALIGN** | Alignment shift | DELETE or keep for curses? |

---

## Damage Types

### Physical Damage (Keep All)

| Type | Description | Used By |
|------|-------------|---------|
| HIT | Blunt impact | Unarmed, clubs, fists |
| SLASH | Cutting | Swords, axes, claws |
| PIERCE | Stabbing | Daggers, arrows, fangs |
| BITE | Animal bite | Beasts, undead |
| CLAW | Raking attack | Beasts, demons |
| CRUSH | Heavy impact | Giants, golems, hammers |
| POUND | Heavy blows | Hammers, mauls |

### Physical Damage (Consider Consolidating)

| Type | Description | Consolidate Into? |
|------|-------------|-------------------|
| STING | Insect attack | PIERCE? |
| WHIP | Lashing | SLASH? |
| MAUL | Savage attack | CLAW or CRUSH? |
| THRASH | Wild attack | CRUSH? |
| CLEAVE | Splitting | SLASH? |
| REND | Tearing | CLAW? |

### Elemental Damage (Keep All)

| Type | Spells | Resistance |
|------|--------|------------|
| FIRE | Fireball, Burning Hands, Flame Strike | PROT_FIRE |
| COLD | Cone of Cold, Ice Storm, Chill Touch | PROT_COLD |
| ACID | Acid Arrow, Acid Fog, Melt | PROT_ACID |
| SHOCK | Lightning Bolt, Chain Lightning | PROT_SHOCK |
| POISON | Poison, Cloudkill | PROT_POISON |

### Special Damage (Keep)

| Type | Description | Used By |
|------|-------------|---------|
| MAGIC | Pure arcane | Magic Missile, Dispel |
| ALIGN | Alignment-based | Holy/Unholy spells |
| BLAST | Explosive force | Area spells |

---

## Summary: Consolidation Recommendations

### Effects to DELETE (Unused)
1. SONG_* (all 5 bard songs)
2. FEIGNED_DEATH
3. REFLECT_DAMAGE
4. MISDIRECTION
5. ACID_WEAPON
6. RADIANT_WEAPON
7. Reserved slots (72-74, 83-84, 86-87)

### Effects to CONSOLIDATE

| Current Effects | Proposed Single Effect | Rationale |
|-----------------|------------------------|-----------|
| FIREHANDS, ICEHANDS, LIGHTNING_HANDS, ACID_HANDS | `elemental_hands` | Same mechanic, different element |
| FIRE_WEAPON, COLD_WEAPON, LIGHT_WEAPON, DARK_WEAPON | `weapon_enchant` | Same mechanic, different element |
| BLUR, GREATER_DISPLACEMENT | `evasion` | Just different magnitudes |
| PARALYSIS, MAJOR_PARALYSIS | `paralysis` | Use severity parameter |
| IMMOBILIZED | Merge into `paralysis` | Same mechanic |
| MESMERIZED | Merge into `charm` | Similar effect |
| INSANITY | Merge into `confusion` | Stronger version |
| All APPLY_SAVING_* | `saving_modifier` | Use type parameter |
| PROT_FIRE, PROT_COLD, PROT_ACID, PROT_SHOCK | `elemental_resist` | Use element parameter |

### Modifiers to DELETE (Unused)
1. APPLY_AGE
2. APPLY_WEIGHT
3. APPLY_HEIGHT
4. APPLY_RACE
5. APPLY_CLASS
6. APPLY_LEVEL

### Damage Types to CONSOLIDATE

| Current Types | Proposed Single Type | Rationale |
|---------------|---------------------|-----------|
| STING, PIERCE | `pierce` | Same damage category |
| WHIP, SLASH | `slash` | Same damage category |
| MAUL, THRASH, CRUSH | `crush` | Same damage category |
| CLEAVE, SLASH | `slash` | Same damage category |
| REND, CLAW | `claw` | Same damage category |

---

## Final Counts (After Consolidation)

| Category | Before | After | Reduction |
|----------|--------|-------|-----------|
| Status Effects | 90 | ~50 | -40 |
| Stat Modifiers | 31 | ~20 | -11 |
| Damage Types | 21 | ~12 | -9 |
| **Total** | **142** | **~82** | **-60 (42%)** |

---

## Questions for Review

1. **Elemental hands** - Combine FIREHANDS/ICEHANDS/etc into one effect with element param?
2. **Weapon enchants** - Combine all weapon enchants into one effect?
3. **Paralysis variants** - One effect with severity, or keep separate?
4. **Evasion effects** - BLUR vs GREATER_DISPLACEMENT, merge?
5. **Detection effects** - Keep all 7 or consolidate some?
6. **Saving throws** - One modifier or keep 5 separate?
7. **Physical damage types** - Keep 14 or consolidate to ~7?
8. **Bard songs** - Delete completely or implement later?

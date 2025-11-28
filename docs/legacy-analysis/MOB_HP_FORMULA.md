# Mobile (Mob) Hit Point Generation

This document explains exactly how mob prototype hit point related fields are parsed and how a spawned mob's final hit points are produced. It also identifies the class and race multiplier fields that influence the calculation.

## Parsing Overview

`parse_mobile` -> (`parse_simple_mob` OR `parse_enhanced_mob`) → optional `interpret_espec` (enhanced only) → prototype stored in `mob_proto[]`.

Instantiation uses `read_mobile` → copy prototype → `setup_mob()` to finalize derived stats (including rolling HP).

## Prototype Fields From First Numeric Line (in `parse_simple_mob`)

```text
<level> <ex_hitroll> <ex_ac/10> <hp_dice_num>d<hp_dice_face>+<hp_bonus> <dam_dice_num>d<dam_dice_face>+<ex_damroll>
```

Mapped:

- level                -> GET_LEVEL
- ex_hitroll (clamped 0–20) -> mob_specials.ex_hitroll
- ex_ac (multiplied by 10)  -> mob_specials.ex_armor
- hp_dice_num          -> mob_specials.ex_no_dice
- hp_dice_face         -> mob_specials.ex_face (legacy; not used in final HP roll)
- hp_bonus             -> points.move (temporarily; later also added flat to HP)
- dam_dice_num         -> mob_specials.ex_damnodice
- dam_dice_face        -> mob_specials.ex_damsizedice
- ex_damroll           -> mob_specials.ex_damroll

Prototype initialization then sets:

```text
points.max_hit = 0  (sentinel meaning: roll at spawn time)
points.hit     = 7 + ex_no_dice     (interpreted as HP Dice COUNT)
points.mana    = 10                 (temporarily used as HP Die SIZE)
GET_EX_MAIN_HP = get_set_hit(level, race, class, state=1)
```

`points.move` (t[5]) is retained and later added as a flat HP bonus during the final roll.

Enhanced mobs ('E') may override ability scores etc. via `interpret_espec` but do **not** directly alter HP dice here.

## Final HP Roll (During `setup_mob`)

If `points.max_hit == 0` (it is for prototypes):

```text
max_hit = clamp( roll_dice(points.hit, points.mana)
                 + GET_EX_MAIN_HP
                 + points.move,
                 0, 32000 )
points.hit = max_hit
```

Where (under default unmodified prototype):

- Number of dice = 7 + ex_no_dice
- Die size       = 10 (constant here)
- Flat additive  = get_set_hit(...) + points.move

After this roll, `points.mana` and `points.move` are repurposed to their real meanings (mana pool & movement) and the intermediate dice meaning disappears.

### Average HP Formula

Let:

- L = level
- N = ex_no_dice (extra HP dice count from prototype)
- B = hp_bonus = t[5] (the number placed into `points.move`)
- CF = CLASS_HITFACTOR(class)
- RF = RACE_HITFACTOR(race)
- ScalingFactor S = (CF + RF) / 200.0  (average of the two percent factors, expressed as multiplier)
- DiceAvg = 5.5 * (7 + N)  (average of (7+N)d10; each d10 averages 5.5)
- MainChunk(L) = base chunk from `get_set_hit(L, race, class, 1)`.

Then:

```text
Average_HP(L) ≈ DiceAvg + MainChunk(L) + B
```

### MainChunk(L) (from `get_set_hit`, state=1)

`get_set_hit` performs a level-dependent quadratic, applies race/class hit factors, then divides by `(2 - L/100.0)`.

In simplified form (after merging branching logic; see source for exact piecewise equation):

```text
RawMain(L) = 3 * L * (L / K(L))        // K(L) switches with level bands
ScaledMain(L) = RawMain(L) * ((CF + RF)/2) / 100
MainChunk(L) = ScaledMain(L) / (2 - L/100.0)
```

The code currently uses several piecewise constants (1.25, 1.35, etc.) to adjust curvature in level bands (<20, <35, <50, ≥50). Refer to `get_set_hit` for exact transitions.

## Class & Race Multipliers

Only two fields influence the HP main chunk here:

- `classes[].hit_factor` accessed via `CLASS_HITFACTOR(class)`
- `races[].hit_factor` accessed via `RACE_HITFACTOR(race)`

Both are stored as percentages (100 = neutral). They combine as an arithmetic mean ( (CF + RF)/2 ) before being applied.

### Extracting Values

Full authoritative values reside in:

- Classes: `legacy/src/class.cpp` → array `classes[NUM_CLASSES]`
- Races:   `legacy/src/races.cpp` → array `races[NUM_RACES]`

Look for the comment lines just after each definition entry; within each struct initializer, the sequence contains (among others):

```
... exp_factor, hit_factor, hd_factor, dice_factor, copper_factor, ac_factor, ...
```

`hit_factor` is the second of those six.

#### Example (Subset)

| Class        | hit_factor |
|--------------|------------|
| Sorcerer     | 80 |
| Cleric       | 80 |
| Thief        | 90 |
| Warrior      | 120 |
| Paladin      | 120 |
| Anti-Paladin | 120 |
| (Others)     | See `class.cpp` |

| Race   | hit_factor |
|--------|------------|
| Human  | 100 |
| Elf    | 100 |
| Gnome  | 100 |
| Dwarf  | 100 |
| Troll  | 130 |
| Drow   | 100 |
| (Others)| See `races.cpp` |

To extend the tables, continue scanning the initializer blocks; each entry places `hit_factor` in the same relative position (after `exp_factor`).

### Effective Scaling Examples

If CF=120 (Warrior) and RF=130 (Troll):

```text
S = (120 + 130) / 200 = 1.25
MainChunk multiplied by 1.25 compared to neutral (100/100).
```

If CF=80 (Sorcerer) and RF=100 (Human):

```text
S = (80 + 100) / 200 = 0.90
MainChunk reduced by 10%.
```

## Putting It Together (Worked Example)

Level 40 Warrior Troll (CF=120, RF=130), assume ex_no_dice=0, B=0.

1. DiceAvg = 7 * 5.5 = 38.5
2. Compute RawMain(40) per code path (see `get_set_hit`, level band 35–49). Suppose base neutral MainChunk40 ≈ 2400 (from previous analysis).
3. Apply scaling: 2400 * 1.25 = 3000
4. Average HP ≈ 3000 + 38.5 = 3038.5

## Notes & Caveats

- `ex_face` is parsed but unused in the final HP computation (legacy artifact).
- `ex_no_dice` increases both variance and average HP linearly ( +5.5 average per extra die ).
- `points.move` at parse time is a misused placeholder for HP flat bonus; after `setup_mob` it becomes real movement points.
- Clamping: final rolled HP is clamped to `[0, 32000]`.
- Zone factors affect experience (`get_set_exp`) but **not** HP.

## Quick Reference Formula (Average)

```text
HP_avg = 5.5 * (7 + ex_no_dice)
         + get_set_hit(L, R, C, 1)   // already includes (CF, RF)
         + hp_bonus
```

Where `get_set_hit` internally applies: `( (CF + RF)/2 )` percent scaling and the level band logic.

## Recommended Future Cleanup

1. Replace overloading of `points.hit`, `points.mana`, and `points.move` with a dedicated temporary HPDice struct (num, size, flat_bonus).
2. Remove or repurpose `ex_face` to avoid confusion.
3. Add explicit comments in `parse_simple_mob` explaining each numeric token.
4. Consider consolidating race/class scaling into a single helper returning a struct of multipliers.

---
Maintained for modernization work. Update this doc if `get_set_hit` or factor field ordering changes.

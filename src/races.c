/***************************************************************************
 *   File: races.c                                        Part of FieryMUD *
 *  Usage: Aligns race situations                                          *
 * Author: Brian Williams <bmw@efn.org>                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 98, 99               *
 ***************************************************************************/

#include "races.h"

#include "casting.h"
#include "charsize.h"
#include "class.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "lifeforce.h"
#include "math.h"
#include "regen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

#include <math.h>

/* Prototypes */
void set_init_height_weight(struct char_data *ch);

/* races[]
 *
 * The individual members of these struct definitions have been arranged
 * in a regular fashion, so that they can easily be located.  Please maintain
 * this arrangement:
 *
 *   name, names, displayname, fullname, plainname,
 *   playable, humanoid, racealign, def_size, def_align, \
 *     bonus_damroll, bonus_hitroll,
 *   def_lifeforce, def_composition,
 *   mweight_lo, mweight_hi, mheight_lo, mheight_hi, \
 *     fweight_lo, fweight_hi, fheight_lo, fheight_hi
 *   attrib_scales[], (str, dex, int, wis, con, cha)
 *   exp_factor, hit_factor, hd_factor, dice_factor, copper_factor, ac_factor,
 *   move_verb, leave_verb
 */

struct racedef races[NUM_RACES] = {
    /* HUMAN */
    {"human",
     "human",
     "&6Human&0",
     "&6Human&0",
     "Human",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     120,
     180,
     60,
     76,
     95,
     150,
     60,
     70,
     {76, 76, 76, 76, 76, 76},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},

    /* ELF */
    {"elf",
     "elf",
     "&8Elf&0",
     "&8Elf&0",
     "Elf",
     FALSE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     1000,
     2,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     90,
     160,
     60,
     70,
     90,
     160,
     59,
     68,
     {64, 80, 88, 86, 64, 78},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* GNOME */
    {"gnome",
     "gnome",
     "&1&d&bGnome&0",
     "&1&d&bGnome&0",
     "Gnome",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_SMALL,
     1000,
     2,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     40,
     90,
     26,
     38,
     40,
     90,
     26,
     38,
     {64, 76, 88, 84, 68, 80},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* DWARF */
    {"dwarf",
     "dwarf mountain",
     "&3Dwarf&0",
     "&3Mountain Dwarf&0",
     "Dwarf",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     1000,
     4,
     5,
     LIFE_LIFE,
     COMP_FLESH,
     170,
     200,
     38,
     50,
     150,
     190,
     38,
     50,
     {84, 76, 64, 86, 84, 68},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* TROLL */
    {"troll",
     "swamp troll",
     "&2&dTroll&0",
     "&2&dSwamp Troll&0",
     "Troll",
     TRUE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_LARGE,
     -1000,
     4,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     130,
     290,
     72,
     90,
     130,
     290,
     72,
     90,
     {80, 72, 56, 56, 100, 64},
     100,
     130,
     110,
     110,
     75,
     100,
     "prowls in",
     "prowls",
     {0, 0}},
    /* DROW */
    {"drow",
     "drow",
     "&5Drow&0",
     "&5Drow&0",
     "Drow",
     FALSE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_MEDIUM,
     -1000,
     2,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     90,
     160,
     60,
     70,
     90,
     160,
     60,
     70,
     {64, 80, 88, 80, 64, 72},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* DUERGAR */
    {"duergar",
     "duergar",
     "&1Duergar&0",
     "&1Duergar&0",
     "Duergar",
     TRUE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_MEDIUM,
     -1000,
     4,
     5,
     LIFE_LIFE,
     COMP_FLESH,
     170,
     200,
     38,
     50,
     150,
     190,
     38,
     50,
     {84, 76, 68, 72, 84, 64},
     100,
     100,
     100,
     100,
     75,
     100,
     "skulks in",
     "skulks",
     {0, 0}},
    /* OGRE */
    {"ogre",
     "ogre",
     "&4Ogre&0",
     "&4Ogre&0",
     "Ogre",
     TRUE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_LARGE,
     -1000,
     5,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     390,
     530,
     93,
     119,
     390,
     530,
     93,
     119,
     {100, 64, 52, 60, 80, 60},
     100,
     110,
     100,
     120,
     75,
     85,
     "lumbers in",
     "lumbers",
     {0, 0}},
    /* ORC */
    {"orc",
     "orc",
     "&9&bOrc&0",
     "&9&bOrc&0",
     "Orc",
     TRUE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_MEDIUM,
     -1000,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     90,
     150,
     58,
     68,
     90,
     150,
     58,
     68,
     {80, 72, 72, 72, 76, 68},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* HALF-ELF */
    {"half-elf",
     "half-elf half elf",
     "&6&bHalf-&0&6&dElf&0",
     "&6&bHalf-&0&6&dElf&0",
     "Half-Elf",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     1000,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     100,
     170,
     60,
     76,
     94,
     155,
     60,
     70,
     {68, 76, 76, 76, 68, 84},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* BARBARIAN */
    {"barbarian",
     "barbarian",
     "&4Barbarian&0",
     "&4Barbarian&0",
     "Barbarian",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_LARGE,
     0,
     5,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     170,
     260,
     69,
     88,
     130,
     210,
     69,
     80,
     {88, 68, 60, 60, 88, 64},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* HALFLING */
    {"halfling",
     "halfling",
     "&3&dHalfling&0",
     "&3&dHalfling&0",
     "Halfling",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_SMALL,
     1000,
     3,
     6,
     LIFE_LIFE,
     COMP_FLESH,
     90,
     160,
     35,
     42,
     90,
     160,
     35,
     42,
     {68, 96, 80, 80, 64, 76},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* PLANT */
    {"plant",
     "plant",
     "&2Plant&0",
     "&2Plant&0",
     "Plant",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_PLANT,
     80,
     180,
     40,
     96,
     80,
     180,
     40,
     96,
     {72, 52, 32, 72, 100, 72},
     100,
     100,
     100,
     100,
     0,
     120,
     NULL,
     NULL,
     {0, 0}},
    /* HUMANOID */
    {"humanoid",
     "humanoid",
     "&7Humanoid&0",
     "&7Humanoid&0",
     "Humanoid",
     FALSE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     120,
     180,
     60,
     76,
     95,
     150,
     60,
     70,
     {72, 72, 72, 72, 72, 72},
     100,
     100,
     100,
     100,
     100,
     60,
     NULL,
     NULL,
     {0, 0}},
    /* ANIMAL */
    {"animal",
     "animal",
     "&2Animal&0",
     "&2Animal&0",
     "Animal",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     120,
     180,
     60,
     76,
     95,
     150,
     60,
     70,
     {72, 72, 72, 72, 72, 72},
     100,
     100,
     100,
     100,
     0,
     65,
     NULL,
     NULL,
     {0, 0}},
    /*
    /* DRAGON
    {"dragon",
     "dragon",
     "&1&bDragon&0",
     "&1&bDragon&0",
     "Dragon",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_GARGANTUAN,
     0,
     10,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     16000,
     64000,
     768,
     1536,
     16000,
     64000,
     768,
     1536,
     {100, 72, 100, 72, 72, 100},
     130,
     130,
     140,
     140,
     500,
     140,
     "stomps in",
     "stomps",
     {0, 0}},
     */
    /* DRAGON - GENERAL */
    {"dragon_general",
     "dragon general",
     "&5&bDragon&0",
     "&5&bDragon&0",
     "General Dragon",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_GARGANTUAN,
     0,
     10,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     16000,
     64000,
     768,
     1536,
     16000,
     64000,
     768,
     1536,
     {100, 72, 100, 72, 72, 100},
     130,
     130,
     140,
     140,
     500,
     140,
     "stomps in",
     "stomps",
     {0, 0}},
    /* GIANT */
    {"giant",
     "giant",
     "&2&bGiant&0",
     "&2&bGiant&0",
     "Giant",
     FALSE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_HUGE,
     0,
     7,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     1000,
     4000,
     196,
     384,
     1000,
     4000,
     196,
     384,
     {100, 72, 44, 64, 80, 72},
     110,
     120,
     120,
     100,
     125,
     120,
     "lumbers in",
     "lumbers",
     {0, 0}},
    /* OTHER */
    {"other",
     "other",
     "&4&bOther&0",
     "&4&bOther&0",
     "Other",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     120,
     180,
     60,
     76,
     95,
     150,
     60,
     70,
     {72, 72, 72, 72, 72, 72},
     80,
     110,
     120,
     80,
     75,
     105,
     NULL,
     NULL,
     {0, 0}},
    /* GOBLIN */
    {"goblin",
     "goblin",
     "&4&bGoblin&0",
     "&4&bGoblin&0",
     "Goblin",
     FALSE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_SMALL,
     -500,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     60,
     90,
     30,
     38,
     55,
     80,
     30,
     35,
     {76, 72, 64, 72, 84, 64},
     60,
     60,
     60,
     60,
     75,
     90,
     NULL,
     NULL,
     {0, 0}},
    /* DEMON */
    {"demon",
     "demon",
     "&1&bDemon&0",
     "&1&bDemon&0",
     "Demon",
     FALSE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_LARGE,
     -1000,
     6,
     4,
     LIFE_DEMONIC,
     COMP_FLESH,
     130,
     290,
     72,
     90,
     130,
     290,
     72,
     90,
     {80, 100, 68, 68, 58, 58},
     120,
     120,
     120,
     120,
     150,
     120,
     "stalks in",
     "stalks",
     {0, 0}},
    /* BROWNIE */
    {"brownie",
     "brownie",
     "&3Brownie&0",
     "&3Brownie&0",
     "Brownie",
     FALSE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_SMALL,
     500,
     1,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     20,
     30,
     20,
     30,
     20,
     30,
     20,
     30,
     {60, 80, 60, 78, 70, 72},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* DRAGON - FIRE */
    {"dragon_fire",
     "dragon fire",
     "&1&bDragon&0",
     "&1&bDragon&0",
     "Fire Dragon",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_GARGANTUAN,
     0,
     10,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     16000,
     64000,
     768,
     1536,
     16000,
     64000,
     768,
     1536,
     {100, 72, 100, 72, 72, 100},
     130,
     130,
     140,
     140,
     500,
     140,
     "stomps in",
     "stomps",
     {0, 0}},
    /* DRAGON - ICE */
    {"dragon_frost",
     "dragon frost",
     "&7&bDragon&0",
     "&7&bDragon&0",
     "Frost Dragon",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_GARGANTUAN,
     0,
     10,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     16000,
     64000,
     768,
     1536,
     16000,
     64000,
     768,
     1536,
     {100, 72, 100, 72, 72, 100},
     130,
     130,
     140,
     140,
     500,
     140,
     "stomps in",
     "stomps",
     {0, 0}},
    /* DRAGON - ACID */
    {"dragon_acid",
     "dragon acid",
     "&9&bDragon&0",
     "&9&bDragon&0",
     "Acid Dragon",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_GARGANTUAN,
     0,
     10,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     16000,
     64000,
     768,
     1536,
     16000,
     64000,
     768,
     1536,
     {100, 72, 100, 72, 72, 100},
     130,
     130,
     140,
     140,
     500,
     140,
     "stomps in",
     "stomps",
     {0, 0}},
    /* DRAGON - LIGHTNING */
    {"dragon_lightning",
    "dragon lightning",
    "&4&bDragon&0",
    "&4&bDragon&0",
    "Lightning Dragon",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_GARGANTUAN,
     0,
     10,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     16000,
     64000,
     768,
     1536,
     16000,
     64000,
     768,
     1536,
     {100, 72, 100, 72, 72, 100},
     130,
     130,
     140,
     140,
     500,
     140,
     "stomps in",
     "stomps",
     {0, 0}},
    /* DRAGON - GAS */
    {"dragon_gas",
     "dragon gas",
     "&2&bDragon&0",
     "&2&bDragon&0",
     "Gas Dragon",
     FALSE,
     FALSE,
     RACE_ALIGN_GOOD,
     SIZE_GARGANTUAN,
     0,
     10,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     16000,
     64000,
     768,
     1536,
     16000,
     64000,
     768,
     1536,
     {100, 72, 100, 72, 72, 100},
     130,
     130,
     140,
     140,
     500,
     140,
     "stomps in",
     "stomps",
     {0, 0}},
    /* DRAGONBORN - FIRE */
    {"dragonborn_fire",
     "dragonborn fire",
     "&1Dr&ba&3g&1on&0&1b&1&bo&3r&1&bn&0",
     "&1Fire Dragonborn&0",
     "Fire Dragonborn",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     180,
     370,
     70,
     80,
     180,
     370,
     70,
     80,
     {78, 64, 76, 72, 78, 76},
     100,
     100,
     100,
     100,
     75,
     110,
     NULL,
     NULL,
     {0, 0}},
    /* DRAGONBORN - FROST */
    {"dragonborn_frost",
     "dragonborn frost",
     "&7&bDr&b&4ag&7&bonb&b&4or&7&bn&0",
     "&7&bFrost Dragonborn&0",
     "Frost Dragonborn",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     180,
     370,
     70,
     80,
     180,
     370,
     70,
     80,
     {78, 64, 76, 72, 78, 76},
     100,
     100,
     100,
     100,
     75,
     110,
     NULL,
     NULL,
     {0, 0}},
    /* DRAGONBORN - ACID */
    {"dragonborn_acid",
     "dragonborn acid",
     "&9&bDr&2a&0&2g&bo&9nb&2o&0&2r&b&9n&0",
     "&9&bAcid Dragonborn&0",
     "Acid Dragonborn",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     180,
     370,
     70,
     80,
     180,
     370,
     70,
     80,
     {78, 64, 76, 72, 78, 76},
     100, 100, 100, 100, 75, 110,
     NULL,
     NULL,
     {0, 0}},
    /* DRAGONBORN - LIGHTNING */
    {"dragonborn_lightning",
     "dragonborn lightning",
     "&b&4Dr&6a&4go&6n&4b&6or&4n&0",
     "&b&4Lightning Dragonborn&0",
     "Lightning Dragonborn",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     180,
     370,
     70,
     80,
     180,
     370,
     70,
     80,
     {78, 64, 76, 72, 78, 76},
     100,
     100,
     100,
     100,
     75,
     110,
     NULL,
     NULL,
     {0, 0}},
    /* DRAGONBORN - GAS */
    {"dragonborn_gas",
     "dragonborn gas",
     "&2&bDra&3g&2onb&3or&2n&0",
     "&2&bGas Dragonborn&0",
     "Gas Dragonborn",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     0,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     180,
     370,
     70,
     80,
     180,
     370,
     70,
     80,
     {78, 64, 76, 72, 78, 76},
     100,
     100,
     100,
     100,
     75,
     110,
     NULL,
     NULL,
     {0, 0}},
    /* SVERFNEBLIN */
    {"sverfneblin",
     "sverfneblin",
     "&9&d&bSverfneblin&0",
     "&9&d&bSverfneblin&0",
     "Sverfneblin",
     TRUE,
     TRUE,
     RACE_ALIGN_EVIL,
     SIZE_SMALL,
     -1000,
     2,
     4,
     LIFE_LIFE,
     COMP_FLESH,
     40,
     90,
     26,
     38,
     40,
     90,
     26,
     38,
     {64, 76, 88, 84, 68, 76},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}},
    /* NYMPH */
    {"nymph",
     "nymph",
     "&3&bN&0&2ym&3&bph&0",	
     "&3&bN&0&2ym&3&bph&0",	
     "Nymph",
     TRUE,
     TRUE,
     RACE_ALIGN_GOOD,
     SIZE_MEDIUM,
     1000,
     3,
     3,
     LIFE_LIFE,
     COMP_FLESH,
     90,
     160,
     59,
     68,
     90,	
     160,
     59,
     68,
     {65, 72, 80, 80, 65, 100},
     100,
     100,
     100,
     100,
     75,
     100,
     NULL,
     NULL,
     {0, 0}}
};
const char *race_align_abbrevs[] = {"&0&3&bGOOD&0", "&0&1&bEVIL&0"};

static flagvector race_effects_mask[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];

void init_races(void) {
#define PERM_EFF(r, f) SET_FLAG(races[(r)].effect_flags, (f))
#define ADD_SKILL(s, p)                                                                                                \
    do {                                                                                                               \
        races[race].skills[pos].skill = (s);                                                                           \
        races[race].skills[pos].proficiency = (p);                                                                     \
        ++pos;                                                                                                         \
    } while (0)

    int race, pos;

    /*
     * Add permanent effects to races here.
     */
    PERM_EFF(RACE_DROW, EFF_INFRAVISION);
    PERM_EFF(RACE_DROW, EFF_ULTRAVISION);
    PERM_EFF(RACE_ELF, EFF_INFRAVISION);
    PERM_EFF(RACE_DWARF, EFF_DETECT_POISON);
    PERM_EFF(RACE_DWARF, EFF_INFRAVISION);
    PERM_EFF(RACE_DWARF, EFF_ULTRAVISION);
    PERM_EFF(RACE_DUERGAR, EFF_INFRAVISION);
    PERM_EFF(RACE_DUERGAR, EFF_ULTRAVISION);
    PERM_EFF(RACE_HALFLING, EFF_INFRAVISION);
    PERM_EFF(RACE_HALFLING, EFF_SENSE_LIFE);
    PERM_EFF(RACE_TROLL, EFF_INFRAVISION);
    PERM_EFF(RACE_TROLL, EFF_ULTRAVISION);
    PERM_EFF(RACE_OGRE, EFF_INFRAVISION);
    PERM_EFF(RACE_OGRE, EFF_ULTRAVISION);
    PERM_EFF(RACE_HALF_ELF, EFF_INFRAVISION);
    PERM_EFF(RACE_GNOME, EFF_INFRAVISION);
    PERM_EFF(RACE_SVERFNEBLIN, EFF_INFRAVISION);
    PERM_EFF(RACE_SVERFNEBLIN, EFF_ULTRAVISION);
    PERM_EFF(RACE_BROWNIE, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_FIRE, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_FROST, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_ACID, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_LIGHTNING, EFF_INFRAVISION);
    PERM_EFF(RACE_DRAGONBORN_GAS, EFF_INFRAVISION);    

    /*
     * Add race skills to the switch below.
     * If a constant value is declared, the skill will always reset back to that value.
     * Use 'proficiency' or 'ROLL_SKILL_PROF' instead.
     */
    for (race = 0; race < NUM_RACES; ++race) {
        memset(races[race].skills, 0, sizeof(races[race].skills));
        pos = 0;
        switch (race) {
        case RACE_ELF:
            ADD_SKILL(SKILL_SLASHING, ROLL_SKILL_PROF);
            break;
        case RACE_TROLL:
            ADD_SKILL(SKILL_DOORBASH, 1000);
            ADD_SKILL(SKILL_BODYSLAM, 1000);
            break;
        case RACE_OGRE:
            ADD_SKILL(SKILL_DOORBASH, 1000);
            ADD_SKILL(SKILL_BODYSLAM, 1000);
            break;
        case RACE_BARBARIAN:
            ADD_SKILL(SKILL_DOORBASH, 1000);
            ADD_SKILL(SKILL_BODYSLAM, 1000);
            break;
        case RACE_DRAGON_GENERAL:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGON_FIRE:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_FIRE:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            break;
        case RACE_DRAGON_FROST:
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_FROST:
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            break;
        case RACE_DRAGON_ACID:
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_ACID:
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            break;
        case RACE_DRAGON_LIGHTNING:
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGONBORN_LIGHTNING:
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_DRAGON_GAS:
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_SWEEP, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            break; 
        case RACE_DRAGONBORN_GAS:
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            break; 
        case RACE_DEMON:
            ADD_SKILL(SKILL_BREATHE_FIRE, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_FROST, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_ACID, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_GAS, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_BREATHE_LIGHTNING, ROLL_SKILL_PROF);  
            ADD_SKILL(SKILL_ROAR, ROLL_SKILL_PROF);
            ADD_SKILL(SPELL_ACID_BREATH, 1000);
            ADD_SKILL(SPELL_FROST_BREATH, 1000);
            ADD_SKILL(SPELL_GAS_BREATH, 1000);
            ADD_SKILL(SPELL_FIRE_BREATH, 1000);
            ADD_SKILL(SPELL_LIGHTNING_BREATH, 1000);
            break;
        case RACE_BROWNIE:
            ADD_SKILL(SKILL_SNEAK, ROLL_SKILL_PROF);
            ADD_SKILL(SKILL_HIDE, ROLL_SKILL_PROF);
            break;
        }
        if (pos > NUM_RACE_SKILLS) {
            sprintf(buf,
                    "init_races: Too many skills assigned to race %s.  "
                    "Increase NUM_RACE_SKILLS in races.h to at least %d",
                    races[race].name, pos);
            log(buf);
            exit(1);
        }
    }

    CLEAR_FLAGS(race_effects_mask, NUM_EFF_FLAGS);
    for (race = 0; race < NUM_RACES; ++race)
        SET_FLAGS(race_effects_mask, races[race].effect_flags, NUM_EFF_FLAGS);

#undef ADD_SKILL
#undef PERM_EFF
}

/* parse_race
 *
 * Identifies a race from a string.  Will do partial matches.
 *
 * Code is present to prohibit a player from being set to the wrong
 * race, but it's disabled.  If it were enabled, it would only take
 * effect if "vict" were not null.
 *
 * ch is someone who's trying to change vict's race (e.g., a wizard
 * manually setting someone to a race due to a quest).
 *
 * If RACE_UNDEFINED is returned, this function will already have provided
 * feedback to ch (if specified) as to the reason for the failure.  Otherwise,
 * it does not provide feedback.
 */
int parse_race(struct char_data *ch, struct char_data *vict, char *arg) {
    int i, race = RACE_UNDEFINED, altname = RACE_UNDEFINED, best = RACE_UNDEFINED;

    if (!*arg) {
        if (ch)
            send_to_char("What race?\r\n", ch);
        return RACE_UNDEFINED;
    }

    for (i = 0; i < NUM_RACES; i++) {
        if (!strncasecmp(arg, races[i].name, strlen(arg))) {
            if (!strcasecmp(arg, races[i].name)) {
                race = i;
                break;
            }
            if (best == RACE_UNDEFINED)
                best = i;
        } else if (isname(arg, races[i].names)) {
            if (altname == RACE_UNDEFINED)
                altname = i;
        } else if (is_abbrev(arg, races[i].name)) {
            if (best == RACE_UNDEFINED)
                best = i;
        }
    }

    if (race == RACE_UNDEFINED)
        race = altname;
    if (race == RACE_UNDEFINED)
        race = best;
    if (race == RACE_UNDEFINED) {
        if (ch)
            send_to_char("There is no such race.\r\n", ch);
    }

    /* There are no validity checks. */
    return race;

    /* The following code could be used to prevent deities from assigning
     * a race racee to a player if:
     *
     *  - The race is not "playable"
     *  - The player's race does not allow the race
     *
     * It's currently not used. */

    /* Bypass validity checks for immortal victims (or no specified victim). */
    if (!vict || GET_LEVEL(vict) > LVL_MAX_MORT)
        return race;

    /* The race has been identified, and there is a mortal victim.
     * Make sure this race is available to the victim. */

    if (!races[race].playable) {
        if (ch) {
            sprintf(buf, "The %s race is not available to mortals.\r\n", races[race].name);
            send_to_char(buf, ch);
        }
        return RACE_UNDEFINED;
    }

    if (!class_ok_race[race][(int)GET_CLASS(vict)]) {
        if (ch) {
            sprintf(buf, "As %s, $n can't be %s.", with_indefinite_article(classes[(int)GET_CLASS(vict)].displayname),
                    with_indefinite_article(races[race].displayname));
            act(buf, FALSE, vict, 0, ch, TO_VICT);
        }
        return RACE_UNDEFINED;
    }

    return race;
}

/* Send a menu to someone who's creating a character, listing the available
 * races.  We assume that this function would not have been called if
 * "races_allowed" were false. */
void send_race_menu(struct descriptor_data *d) {
    extern int evil_races_allowed;
    char idx;
    int i;

    write_to_output("\r\nThe following races are available:\r\n", d);
    for (i = 0, idx = 'a'; i < NUM_RACES; i++) {
        if (races[i].playable && (evil_races_allowed || races[i].racealign == RACE_ALIGN_GOOD)) {
            sprintf(buf, "  &7%c)&0 %s\r\n", idx, races[i].fullname);
            write_to_output(buf, d);
            idx++;
        }
    }
}

/* Someone who's creating a character typed a letter to indicate which
 * race they wanted.  Determine which race they indicated, using the same
 * rules as send_race_menu() -- skip over inactive/unavailable races. */
int interpret_race_selection(char arg) {
    extern int evil_races_allowed;
    char idx;
    int i;

    for (i = 0, idx = 'a'; i < NUM_RACES; i++) {
        if (races[i].playable && (evil_races_allowed || races[i].racealign == RACE_ALIGN_GOOD)) {
            if (arg == idx)
                return i;
            idx++;
        }
    }
    return RACE_UNDEFINED;
}

/* Oddly enough, the base value for movement points is not stored
 * anywhere.  Thus, it would be impossible to increase the value
 * as a player advances in level.  Anyway, the same value gets set,
 * based on CON, whenever a player logs in.
 *
 * There are times when you want to see this unaffected value, so
 * here's the function to find it.  This is also used at character
 * creation time and when logging. */
int natural_move(struct char_data *ch) {
    if (IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0) {

        /* Mountable mobs will have their mv points set according to level.
         * The second parameter to pow (now 0.8) controls how the points
         * increase as the level increases. If it were 1, the points would
         * increase in a straight line as the level increases. If it were
         * greater than 1, the points would increase slowly at first, and
         * then sharply curve up to their maximum as the level got close
         * to the maximum level. When it's below 1, the points increase
         * quickly at first, and then slowly reach their maximum. */

        if (MOB_FLAGGED(ch, MOB_MOUNTABLE)) {
            if (GET_LEVEL(ch) > MAX_MOUNT_LEVEL)
                return MOUNT_MAXMOVE + 2 * (GET_LEVEL(ch) - MAX_MOUNT_LEVEL) + number(0, 9);
            else
                return (int) (MOUNT_MINMOVE + (MOUNT_MAXMOVE - MOUNT_MINMOVE) *
                                                pow((GET_LEVEL(ch) - 1) / (double) (MAX_MOUNT_LEVEL - 1), 0.8)) +
                       number(0, 9);
        } else
            return mob_proto[GET_MOB_RNUM(ch)].points.max_move;
    } else {
        return MAX(100, GET_CON(ch) * 2);
    }
}

/* init_proto_race()
 *
 * Sets beginning values on a mob prototype, according to race.
 */

void init_proto_race(struct char_data *ch) {
    set_base_size(ch, races[(int)GET_RACE(ch)].def_size);
    GET_LIFEFORCE(ch) = races[(int)GET_RACE(ch)].def_lifeforce;
    BASE_COMPOSITION(ch) = races[(int)GET_RACE(ch)].def_composition;
    GET_COMPOSITION(ch) = BASE_COMPOSITION(ch);
}

/* init_char_race()
 *
 * Sets beginning values that are appropriate for a brand-new character,
 * according to race. */

void init_char_race(struct char_data *ch) {
    if (!IS_NPC(ch) && VALID_RACE(ch)) {
        GET_BASE_DAMROLL(ch) = races[(int)GET_RACE(ch)].bonus_damroll;
        GET_BASE_HITROLL(ch) = races[(int)GET_RACE(ch)].bonus_hitroll;
    }

    /* NPCs will have their own align defined at build time,
     * and it might have been adjusted by the builder, too. */
    if (!IS_NPC(ch) && VALID_RACE(ch))
        GET_ALIGNMENT(ch) = races[(int)GET_RACE(ch)].def_align;
    set_init_height_weight(ch);

    GET_MAX_MOVE(ch) = natural_move(ch);
}

void update_char_race(struct char_data *ch) {
    if (!VALID_RACE(ch)) {
        log("update_char_race: %s doesn't have a valid race (%d).", GET_NAME(ch), GET_RACE(ch));
        return;
    }

    GET_RACE_ALIGN(ch) = races[(int)GET_RACE(ch)].racealign;

    /* Any bits that might get set below should be cleared here first. */
    REMOVE_FLAGS(EFF_FLAGS(ch), race_effects_mask, NUM_EFF_FLAGS);

    /* Reset effect flags for this race */
    SET_FLAGS(EFF_FLAGS(ch), races[(int)GET_RACE(ch)].effect_flags, NUM_EFF_FLAGS);
}

/*
 * Returns a positive value for skills that this race has.
 *
 * Doesn't disqualify any skills! Only enables them.
 */

int racial_skill_proficiency(int skill, int race, int level) {
    int i;

    for (i = 0; races[race].skills[i].skill > 0 && i < NUM_RACE_SKILLS; ++i)
        if (races[race].skills[i].skill == skill) {
            return races[race].skills[i].proficiency;
        }

    return 0;
}

/* convert_race does no checking.  It expects a valid race and ch.
 * This function changes a player's race and converts the skills/spells
 * accordingly, keeping the old values if they are better.
 * It also transfers quest spells. */
void convert_race(struct char_data *ch, int newrace) {
    int skill;
    sh_int old_skills[TOP_SKILL + 1];
    sh_int new_skills[TOP_SKILL + 1];

    /* read in the player's old skills */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        old_skills[skill] = GET_ISKILL(ch, skill);
    }

    /* set race/align */
    GET_RACE(ch) = newrace;

    /* Big changes occur here: */
    update_char(ch);

    /* read the new skills */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        new_skills[skill] = GET_ISKILL(ch, skill);
    }

    /* compare old and new */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        if (new_skills[skill]) {
            /* keep the value of the old skill if you still have the skill */
            if (old_skills[skill] > new_skills[skill]) {
                SET_SKILL(ch, skill, old_skills[skill]);
            }
        }

        /* keep any quest spells you might have earned */
        if ((old_skills[skill]) && (skills[skill].quest)) {
            SET_SKILL(ch, skill, old_skills[skill]);
        }
    }
    check_regen_rates(ch);
}

void scale_attribs(struct char_data *ch) {
    if (VALID_RACE(ch)) {
        GET_AFFECTED_STR(ch) = (GET_VIEWED_STR(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_STR - 1]) / 100;
        GET_AFFECTED_DEX(ch) = (GET_VIEWED_DEX(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_DEX - 1]) / 100;
        GET_AFFECTED_INT(ch) = (GET_VIEWED_INT(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_INT - 1]) / 100;
        GET_AFFECTED_WIS(ch) = (GET_VIEWED_WIS(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_WIS - 1]) / 100;
        GET_AFFECTED_CON(ch) = (GET_VIEWED_CON(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_CON - 1]) / 100;
        GET_AFFECTED_CHA(ch) = (GET_VIEWED_CHA(ch) * races[(int)GET_RACE(ch)].attrib_scales[APPLY_CHA - 1]) / 100;
    } else {
        GET_AFFECTED_STR(ch) = GET_VIEWED_STR(ch) * 72 / 100;
        GET_AFFECTED_DEX(ch) = GET_VIEWED_DEX(ch) * 72 / 100;
        GET_AFFECTED_INT(ch) = GET_VIEWED_INT(ch) * 72 / 100;
        GET_AFFECTED_WIS(ch) = GET_VIEWED_WIS(ch) * 72 / 100;
        GET_AFFECTED_CON(ch) = GET_VIEWED_CON(ch) * 72 / 100;
        GET_AFFECTED_CHA(ch) = GET_VIEWED_CHA(ch) * 72 / 100;
    }
}
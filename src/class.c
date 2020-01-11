/***************************************************************************
 * $Id: class.c,v 1.176 2009/07/18 01:17:23 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: class.c                                        Part of FieryMUD  *
 *  Usage: Source file for class-specific code                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */
#include "class.h"

#include "casting.h"
#include "clan.h"
#include "comm.h"
#include "conf.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "pfiles.h"
#include "players.h"
#include "races.h"
#include "regen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

const char *subclass_descrip = "\r\n"
                               "                    &1&bF&0&3&bi&0&1&be&0&3&br&0&1&by&0&3&bMUD Class "
                               "System&0\r\n"
                               "&1&bF&0&3&bi&0&1&be&0&3&br&0&1&by&0&7&bMUD &0has many various and complex "
                               "classes for you to play.  All players\r\n"
                               "originate from four basic classes. Each of the basic classes\r\n"
                               "posses unknown growth potential throughout gameplay. As you explore \r\n"
                               "and the realm and advance your experience you may learn of ways to\r\n"
                               "specialize your skills and spells into a new subclass.  If you manage\r\n";

const char *subclass_descrip2 = "to learn of these ways your new class will posses new and different\r\n"
                                "skills and spells thus altering your power within the realm.  Some\r\n"
                                "classes will also be required to choose a deity in the future as "
                                "well.&0\r\n";

/* The newbie equipment
 *
 * 18     a small leather bag           All
 * 19     some iron rations             All
 * 20     a leather water skin          All
 * 21     a pair of leather leggings    warrior, rogue, cleric
 * 22     a pair of leather sleeves     warrior, rogue, cleric
 * 23     a wooden torch                All
 * 24     a steel-studded helmet        warrior
 * 25     steel-toed boots              warrior
 * 26     a pair of leather sandals     rogue, cleric
 * 27     a small leather cap           rogue, cleric
 * 28     a steel mace                  cleric
 * 29     a steel longsword             warrior
 * 30     a steel dagger                rogue, mage
 * 38     black linen leggings          mage
 * 1029   a spellbook                   mage
 * 1154   a feather quill               mage
 * 3090   a small map of mielikki       All
 */

const int common_newbie_eq[] = {18, 19, 20, 23, 3090, -1};

/* classes[]
 *
 * The individual members of these struct definitions have been arranged
 * in a regular fashion, so that they can easily be located.  Please maintain
 * this arrangement:
 *
 *   name, altname, displayname, plainname,
 *   fmtname, abbrev, stars,
 *   magical, mem_mode, active, is_subclass, subclass_of, max_subclass_level,
 * homeroom statorder saves hp_lev, thac0, nowear_flag, hit_regen_factor,
 * mana_regen_factor, mv_regen_factor, exp_gain_factor, exp_factor, hit_factor,
 * hd_factor, dice_factor, copper_factor, ac_factor, newbie_eq[]
 *
 * The is_subclass value of inactive classes (such as mystic) should be taken
 * with a grain of salt.  If/when an inactive class is made active, its
 * subclass status should be reevaluated.
 */

struct classdef classes[NUM_CLASSES] = {

    /* SORCERER */
    {"sorcerer",
     "",
     "&5&bSorcerer&0",
     "Sorcerer",
     "&5&bSorcerer&0    ",
     "&5&bSor&0",
     "&5&b**&0",
     TRUE,
     MEMORIZE,
     TRUE,
     FALSE,
     CLASS_UNDEFINED,
     45,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     6,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1.2,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 27, 26, 30, -1}},

    /* CLERIC */
    {"cleric",
     "",
     "&6Cleric&0",
     "Cleric",
     "&6Cleric&0      ",
     "&6Cle&0",
     "&6**&0",
     TRUE,
     PRAY,
     TRUE,
     FALSE,
     CLASS_UNDEFINED,
     45,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     4,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {21, 22, 27, 26, 28, -1}},

    /* THIEF */
    {"thief",
     "",
     "&1&bThief&0",
     "Thief",
     "&1&bThief&0       ",
     "&1&bThi&0",
     "&1&b**&0",
     FALSE,
     MEM_NONE,
     TRUE,
     TRUE,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 90, 100, 110, 110},
     8,
     1,
     ITEM_ANTI_ROGUE,
     100,
     100,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {21, 22, 27, 26, 30, -1}},

    /* WARRIOR */
    {"warrior",
     "",
     "&4&bWarrior&0",
     "Warrior",
     "&4&bWarrior&0     ",
     "&4&bWar&0",
     "&4&b**&0",
     FALSE,
     MEM_NONE,
     TRUE,
     FALSE,
     CLASS_UNDEFINED,
     25,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     12,
     -5,
     ITEM_ANTI_WARRIOR,
     100,
     0,
     100,
     1.1,
     100,
     120,
     120,
     120,
     100,
     120,
     {21, 22, 24, 25, 29, -1}},

    /* PALADIN */
    {"paladin",
     "",
     "&8Paladin&0",
     "Paladin",
     "&8Paladin&0     ",
     "&8Pal&0",
     "&8**&0",
     TRUE,
     PRAY,
     TRUE,
     TRUE,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 115, 100, 105, 90},
     10,
     -4,
     ITEM_ANTI_PALADIN,
     100,
     50,
     100,
     1,
     100,
     120,
     120,
     120,
     100,
     120,
     {21, 22, 24, 25, 29, -1}},

    /* ANTI_PALADIN */
    {"anti-paladin",
     "antipaladin",
     "&1&bAnti-&9Paladin&0",
     "Anti-Paladin",
     "&1&bAnti-&9Paladin&0",
     "&1&bAnt&0",
     "&1&b**&0",
     TRUE,
     PRAY,
     TRUE,
     TRUE,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 115, 100, 105, 90},
     10,
     -4,
     ITEM_ANTI_ANTI_PALADIN,
     100,
     50,
     100,
     1,
     100,
     120,
     120,
     120,
     100,
     120,
     {21, 22, 24, 25, 29, -1}},

    /* RANGER */
    {"ranger",
     "",
     "&2&bRanger&0",
     "Ranger",
     "&2&d&bRanger&0      ",
     "&2&bRan&0",
     "&2&b**&0",
     TRUE,
     MEMORIZE,
     TRUE,
     TRUE,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_INT, STAT_WIS, STAT_CHA},
     {95, 115, 100, 105, 90},
     10,
     -4,
     ITEM_ANTI_RANGER,
     100,
     50,
     100,
     1.15,
     100,
     120,
     120,
     120,
     100,
     120,
     {21, 22, 24, 25, 29, -1}},

    /* DRUID */
    {"druid",
     "",
     "&2Druid&0",
     "Druid",
     "&2Druid&0       ",
     "&2Dru&0",
     "&2**&0",
     TRUE,
     PRAY,
     TRUE,
     TRUE,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     5,
     ITEM_ANTI_DRUID,
     85,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {21, 22, 27, 26, 28, -1}},

    /* SHAMAN */
    {"shaman",
     "",
     "&6&bShaman&0",
     "Shaman",
     "&6&bShaman&0      ",
     "&6&bSha&0",
     "&6&b**&0",
     TRUE,
     MEM_NONE,
     FALSE,
     FALSE,
     CLASS_UNDEFINED,
     -1,
     3001,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {105, 115, 105, 110, 110},
     8,
     6,
     ITEM_ANTI_SHAMAN,
     80,
     100,
     100,
     1,
     100,
     100,
     100,
     100,
     100,
     100,
     {21, 22, 27, 26, 28, -1}},

    /* ASSASSIN */
    {"assassin",
     "",
     "&1Assassin&0",
     "Assassin",
     "&1Assassin&0    ",
     "&1Ass&0",
     "&1**&0",
     FALSE,
     MEM_NONE,
     TRUE,
     TRUE,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {95, 90, 100, 110, 110},
     8,
     2,
     ITEM_ANTI_ASSASSIN,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {21, 22, 27, 26, 30, -1}},

    /* MERCENARY */
    {"mercenary",
     "",
     "&3Mercenary&0",
     "Mercenary",
     "&3Mercenary&0   ",
     "&3Mer&0",
     "&3**&0",
     FALSE,
     MEM_NONE,
     TRUE,
     TRUE,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_INT, STAT_WIS, STAT_CHA},
     {95, 90, 100, 110, 110},
     11,
     2,
     ITEM_ANTI_MERCENARY,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {21, 22, 24, 25, 30, 29, -1}},

    /* NECROMANCER */
    {"necromancer",
     "",
     "&5Necromancer&0",
     "Necromancer",
     "&5Necromancer&0 ",
     "&5Nec&0",
     "&5**&0",
     TRUE,
     MEMORIZE,
     TRUE,
     TRUE,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_NECROMANCER,
     80,
     100,
     100,
     1.3,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 27, 26, 30, -1}},

    /* CONJURER */
    {"conjurer",
     "",
     "&3&bConjurer&0",
     "Conjurer",
     "&3&bConjurer&0    ",
     "&3&bCon&0",
     "&3&b**&0",
     TRUE,
     MEMORIZE,
     FALSE,
     TRUE,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_CONJURER,
     80,
     100,
     100,
     1,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 27, 26, 30, -1}},

    /* MONK */
    {"monk",
     "",
     "&9&bM&0&7on&9&bk&0",
     "Monk",
     "&9&bM&0&7on&9&bk&0        ",
     "&9&bMon&0",
     "&9&b**&0",
     FALSE,
     MEM_NONE,
     TRUE,
     TRUE,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_CON, STAT_STR, STAT_DEX, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     10,
     -4,
     0,
     100,
     0,
     200,
     1.5,
     100,
     120,
     120,
     120,
     100,
     120,
     {21, 22, 26, 27, -1}},

    /* BERSERKER */
    {"berserker",
     "",
     "&9&bBer&1ser&9ker&0",
     "Berserker",
     "&9&bBer&1ser&9ker&0   ",
     "&9&bBe&1r&0",
     "&9&b**&0",
     FALSE,
     MEM_NONE,
     FALSE,
     TRUE,
     CLASS_WARRIOR,
     -1,
     3022,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     12,
     -4,
     0,
     100,
     0,
     100,
     1,
     100,
     120,
     120,
     120,
     100,
     120,
     {21, 22, 24, 25, 29, -1}},

    /* PRIEST */
    {"priest",
     "",
     "&6&bPr&7ie&6st&0",
     "Priest",
     "&6&bPr&7ie&6st&0      ",
     "&6&bPr&7i&0",
     "&6&b**&0",
     TRUE,
     PRAY,
     TRUE,
     TRUE,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     5,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {21, 22, 27, 26, 28, -1}},

    /* DIABOLIST */
    {"diabolist",
     "",
     "&5Dia&9&bbol&0&5ist&0",
     "Diabolist",
     "&5Dia&9&bbol&0&5ist&0   ",
     "&5Di&9&ba&0",
     "&5**&0",
     TRUE,
     PRAY,
     TRUE,
     TRUE,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     5,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {21, 22, 27, 26, 28, -1}},

    /* MYSTIC */
    {"mystic",
     "",
     "&7&bM&0&7ys&9&bti&7c&0",
     "Mystic",
     "&7&bM&0&7ys&9&bti&7c&0      ",
     "&7&bM&0&7ys&0",
     "&7&b**&0",
     TRUE,
     PRAY,
     FALSE,
     TRUE,
     CLASS_CLERIC,
     -1,
     3003,
     {STAT_WIS, STAT_INT, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {85, 110, 85, 115, 90},
     8,
     7,
     ITEM_ANTI_CLERIC,
     80,
     100,
     100,
     1,
     100,
     80,
     80,
     70,
     100,
     100,
     {21, 22, 27, 26, 28, -1}},

    /* ROGUE */
    {"rogue",
     "",
     "&9&bRogue&0",
     "Rogue",
     "&9&bRogue&0       ",
     "&9&bRog&0",
     "&9&b**&0",
     FALSE,
     MEM_NONE,
     TRUE,
     FALSE,
     CLASS_UNDEFINED,
     25,
     3038,
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 90, 100, 110, 110},
     8,
     2,
     ITEM_ANTI_ROGUE,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {21, 22, 27, 26, 30, -1}},

    /* BARD */
    {"bard",
     "",
     "&4B&9&bar&0&4d&0",
     "Bard",
     "&4B&9&bar&0&4d&0        ",
     "&4B&9&bar&0",
     "&4**&0",
     FALSE,
     MEM_NONE,
     FALSE,
     TRUE,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_DEX, STAT_STR, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {95, 90, 100, 110, 110},
     9,
     2,
     0,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {21, 22, 27, 26, 30, -1}},

    /* PYROMANCER */
    {"pyromancer",
     "",
     "&1P&byr&0&1o&9&bma&0&7nc&9&ber&0",
     "Pyromancer",
     "&1P&byr&0&1o&9&bma&0&7nc&9&ber&0  ",
     "&1P&byr&0",
     "&1**&0",
     TRUE,
     MEMORIZE,
     TRUE,
     TRUE,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 27, 26, 30, -1}},

    /* CRYOMANCER */
    {"cryomancer",
     "",
     "&4C&bry&0&4o&7ma&9&bnc&0&7er&0",
     "Cryomancer",
     "&4C&bry&0&4o&7ma&9&bnc&0&7er&0  ",
     "&4C&bry&0",
     "&4**&0",
     TRUE,
     MEMORIZE,
     TRUE,
     TRUE,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 27, 26, 30, -1}},

    /* ILLUSIONIST */
    {"illusionist",
     "",
     "&4I&5l&4l&5u&4s&5i&4o&5n&4i&5s&4t&0",
     "Illusionist",
     "&4I&5l&4l&5u&4s&5i&4o&5n&4i&5s&4t&0 ",
     "&4I&5ll&0",
     "&4**&0",
     TRUE,
     MEMORIZE,
     FALSE,
     TRUE,
     CLASS_SORCERER,
     -1,
     3046,
     {STAT_INT, STAT_WIS, STAT_STR, STAT_DEX, STAT_CON, STAT_CHA},
     {90, 85, 95, 105, 80},
     5,
     7,
     ITEM_ANTI_SORCERER,
     80,
     100,
     100,
     1,
     120,
     80,
     80,
     60,
     100,
     75,
     {1029, 1154, 38, 27, 26, 30, -1}},

    /* HUNTER */
    {"hunter",
     "",
     "&9&bHun&0&2te&9&br&0",
     "Hunter",
     "&9&bHun&0&2te&9&br&0      ",
     "&9&bHun&0",
     "&9&b**&0",
     FALSE,
     MEM_NONE,
     FALSE,
     TRUE,
     CLASS_ROGUE,
     -1,
     3038,
     {STAT_STR, STAT_DEX, STAT_CON, STAT_WIS, STAT_INT, STAT_CHA},
     {105, 115, 100, 100, 110},
     10,
     -4,
     0,
     100,
     0,
     100,
     1,
     100,
     90,
     100,
     100,
     100,
     80,
     {21, 22, 27, 26, 30, -1}},

    /* LAYMAN */
    {"layman",
     "",
     "Layman",
     "Layman",
     "Layman      ",
     "Lay",
     "**",
     FALSE,
     MEM_NONE,
     FALSE,
     FALSE,
     CLASS_UNDEFINED,
     -1,
     3001,
     {STAT_CHA, STAT_CON, STAT_DEX, STAT_STR, STAT_WIS, STAT_INT},
     {100, 100, 100, 100, 100},
     7,
     0,
     0,
     100,
     0,
     100,
     1,
     80,
     100,
     130,
     100,
     75,
     105,
     {26, 27, 30, 38, -1}}};

static flagvector class_effects_mask[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
void init_classes(void) {
#define PERM_EFF(c, f) SET_FLAG(classes[(c)].effect_flags, (f))

    PERM_EFF(CLASS_PRIEST, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_DIABOLIST, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_PALADIN, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_PALADIN, EFF_PROTECT_EVIL);
    PERM_EFF(CLASS_ANTI_PALADIN, EFF_DETECT_ALIGN);
    PERM_EFF(CLASS_ANTI_PALADIN, EFF_PROTECT_GOOD);
    PERM_EFF(CLASS_RANGER, EFF_FARSEE);

#undef PERM_EFF
}

/* parse_class
 *
 * Identifies a class from a string.  Will do partial matches.
 *
 * Code is present to prohibit a player from being set to the wrong
 * class, but it's disabled.  If it were enabled, it would only take
 * effect if "vict" were not null.
 *
 * ch is someone who's trying to change vict's class (e.g., a wizard
 * manually setting someone to a class due to a quest).
 *
 * If CLASS_UNDEFINED is returned, this function will already have provided
 * feedback to ch (if specified) as to the reason for the failure.  Otherwise,
 * it does not provide feedback.
 */
int parse_class(struct char_data *ch, struct char_data *vict, char *arg) {
    int i, class = CLASS_UNDEFINED, best = CLASS_UNDEFINED;

    if (!*arg) {
        if (ch)
            send_to_char("What class?\r\n", ch);
        return CLASS_UNDEFINED;
    }

    for (i = 0; i < NUM_CLASSES; i++) {
        if (!strncasecmp(arg, classes[i].name, strlen(arg))) {
            if (!strcasecmp(arg, classes[i].name)) {
                class = i;
                break;
            }
            if (best == CLASS_UNDEFINED)
                best = i;
        } else if (!strncasecmp(arg, classes[i].altname, strlen(arg))) {
            if (!strcasecmp(arg, classes[i].altname)) {
                class = i;
                break;
            }
            if (best == CLASS_UNDEFINED)
                best = i;
        }
    }

    if (class == CLASS_UNDEFINED)
        class = best;

    if (class == CLASS_UNDEFINED) {
        if (ch)
            send_to_char("There is no such class.\r\n", ch);
    }

    /* There are no validity checks. */
    return class;

    /* The following code could be used to prevent deities from assigning
     * a class classe to a player if:
     *
     *  - The class is not "active"
     *  - The player's race does not allow the class
     *
     * It's currently not used. */

    /* Bypass validity checks for immortal victims (or no specified victim). */
    if (!vict || GET_LEVEL(vict) > LVL_MAX_MORT)
        return class;

    /* The class has been identified, and there is a mortal victim.
     * Make sure this class is available to the victim. */

    if (!classes[class].active) {
        if (ch) {
            sprintf(buf, "The %s class is not available to mortals.\r\n", classes[class].name);
            send_to_char(buf, ch);
        }
        return CLASS_UNDEFINED;
    }

    if (!class_ok_race[(int)GET_RACE(vict)][class]) {
        if (ch) {
            sprintf(buf, "As %s, $n can't become %s.", with_indefinite_article(races[(int)GET_RACE(vict)].displayname),
                    with_indefinite_article(classes[class].name));
            act(buf, FALSE, vict, 0, ch, TO_VICT);
        }
        return CLASS_UNDEFINED;
    }

    return class;
}

void give_newbie_eq(struct char_data *ch) {
    struct obj_data *obj;
    int i;

    if (!VALID_CLASS(ch)) {
        sprintf(buf, "SYSERR: give_newbie_eq() called for char with invalid class of %d", GET_CLASS(ch));
        log(buf);
        return;
    }

    if (GET_LEVEL(ch) > 4)
        return;

    for (i = 0; common_newbie_eq[i] >= 0; i++) {
        obj = read_object(common_newbie_eq[i], VIRTUAL);
        obj_to_char(obj, ch);
    }

    for (i = 0; classes[(int)GET_CLASS(ch)].newbie_eq[i] >= 0; i++) {
        obj = read_object(classes[(int)GET_CLASS(ch)].newbie_eq[i], VIRTUAL);
        obj_to_char(obj, ch);
    }
}

/* GUILD GUARDS
 *
 * When a mob with the guild guard specproc is placed in a room, it will
 * by default do no blocking at all.
 *
 * When a guild guard is instructed to allow a class to go in a particular
 * direction, that direction becomes restricted.  The guard will now block
 * all other classes from passing that way.
 *
 * More than one class may be permitted to go in a particular direction.
 * This is useful in the case of base-class guildmasters, such as the
 * warrior guildmaster beyond room 3021.  All warrior classes should be
 * permitted to go there.  They must all be mentioned specifically,
 * however.
 */
int guild_info[][3] = {

    /* Mielikki */ /* Room    Direction */
    {CLASS_SORCERER, 3045, SCMD_EAST},
    {CLASS_PYROMANCER, 3045, SCMD_EAST},
    {CLASS_CRYOMANCER, 3045, SCMD_EAST},
    {CLASS_NECROMANCER, 3045, SCMD_EAST},
    {CLASS_ILLUSIONIST, 3045, SCMD_EAST},
    {CLASS_PYROMANCER, 3045, SCMD_UP},
    {CLASS_CRYOMANCER, 3045, SCMD_DOWN},
    {CLASS_ILLUSIONIST, 3207, SCMD_SOUTH},
    {CLASS_NECROMANCER, 16950, SCMD_UP}, /* Haunted House */

    {CLASS_CLERIC, 3004, SCMD_NORTH},
    {CLASS_DIABOLIST, 3004, SCMD_NORTH},
    {CLASS_DRUID, 3004, SCMD_NORTH},
    {CLASS_PRIEST, 3004, SCMD_NORTH},
    {CLASS_DRUID, 3004, SCMD_WEST},
    {CLASS_PRIEST, 3004, SCMD_UP},

    {CLASS_ROGUE, 3037, SCMD_DOWN},
    {CLASS_ASSASSIN, 3037, SCMD_DOWN},
    {CLASS_THIEF, 3037, SCMD_DOWN},
    {CLASS_MERCENARY, 3037, SCMD_DOWN},

    {CLASS_WARRIOR, 3021, SCMD_NORTH},
    {CLASS_ANTI_PALADIN, 3021, SCMD_NORTH},
    {CLASS_RANGER, 3021, SCMD_NORTH},
    {CLASS_MONK, 3021, SCMD_NORTH},
    {CLASS_PALADIN, 3021, SCMD_NORTH},
    {CLASS_BERSERKER, 3021, SCMD_NORTH},
    {CLASS_RANGER, 3549, SCMD_WEST},  /* Light Forest */
    {CLASS_MONK, 5307, SCMD_EAST},    /* Grey Castle */
    {CLASS_PALADIN, 5305, SCMD_EAST}, /* Grey Castle */

    /* Ogakh */ /* Room    Direction */
    {CLASS_SORCERER, 30072, SCMD_WEST},
    {CLASS_PYROMANCER, 30072, SCMD_WEST},
    {CLASS_CRYOMANCER, 30072, SCMD_WEST},
    {CLASS_NECROMANCER, 30072, SCMD_WEST},
    {CLASS_ILLUSIONIST, 30072, SCMD_WEST},
    {CLASS_PYROMANCER, 30102, SCMD_NORTH},
    {CLASS_CRYOMANCER, 30102, SCMD_WEST},
    {CLASS_NECROMANCER, 30112, SCMD_UP},
    {CLASS_ILLUSIONIST, 30120, SCMD_UP},

    {CLASS_CLERIC, 30069, SCMD_WEST},
    {CLASS_DIABOLIST, 30069, SCMD_WEST},
    {CLASS_DRUID, 30069, SCMD_WEST},
    {CLASS_PRIEST, 30069, SCMD_WEST},
    {CLASS_DIABOLIST, 30107, SCMD_SOUTH},

    {CLASS_ROGUE, 30018, SCMD_EAST},
    {CLASS_ASSASSIN, 30018, SCMD_EAST},
    {CLASS_THIEF, 30018, SCMD_EAST},
    {CLASS_MERCENARY, 30018, SCMD_EAST},

    {CLASS_WARRIOR, 30029, SCMD_WEST},
    {CLASS_ANTI_PALADIN, 30029, SCMD_WEST},
    {CLASS_RANGER, 30029, SCMD_WEST},
    {CLASS_MONK, 30029, SCMD_WEST},
    {CLASS_PALADIN, 30029, SCMD_WEST},
    {CLASS_BERSERKER, 30029, SCMD_WEST},
    {CLASS_ANTI_PALADIN, 30107, SCMD_NORTH},

    /* Anduin */
    {CLASS_ANTI_PALADIN, 6079, SCMD_EAST},

    {CLASS_WARRIOR, 6148, SCMD_SOUTH},
    {CLASS_MONK, 6148, SCMD_SOUTH},
    {CLASS_PALADIN, 6148, SCMD_SOUTH},
    {CLASS_RANGER, 6148, SCMD_SOUTH},
    {CLASS_BERSERKER, 6148, SCMD_SOUTH},

    {CLASS_ROGUE, 6067, SCMD_NORTH},
    {CLASS_THIEF, 6067, SCMD_NORTH},
    {CLASS_MERCENARY, 6106, SCMD_UP},
    {CLASS_ASSASSIN, 6083, SCMD_NORTH},

    {CLASS_CLERIC, 6217, SCMD_EAST},
    {CLASS_PRIEST, 6217, SCMD_EAST},
    {CLASS_DIABOLIST, 6075, SCMD_SOUTH},
    {CLASS_DRUID, 6217, SCMD_UP},

    {CLASS_SORCERER, 6219, SCMD_UP},
    {CLASS_NECROMANCER, 6219, SCMD_DOWN},
    {CLASS_PYROMANCER, 6219, SCMD_NORTH},
    {CLASS_CRYOMANCER, 6219, SCMD_SOUTH},
    {CLASS_ILLUSIONIST, 6233, SCMD_SOUTH},

    /* Ickle */
    {CLASS_ROGUE, 10047, SCMD_EAST},
    {CLASS_MERCENARY, 10047, SCMD_EAST},
    {CLASS_ASSASSIN, 10047, SCMD_EAST},
    {CLASS_THIEF, 10047, SCMD_EAST},

    {CLASS_WARRIOR, 10014, SCMD_WEST},
    {CLASS_ANTI_PALADIN, 10014, SCMD_WEST},
    {CLASS_PALADIN, 10014, SCMD_WEST},
    {CLASS_MONK, 10014, SCMD_WEST},
    {CLASS_RANGER, 10014, SCMD_WEST},
    {CLASS_BERSERKER, 10014, SCMD_WEST},

    {CLASS_CLERIC, 10004, SCMD_WEST},
    {CLASS_PRIEST, 10004, SCMD_WEST},
    {CLASS_DIABOLIST, 10004, SCMD_WEST},
    {CLASS_DRUID, 10004, SCMD_WEST},

    {CLASS_SORCERER, 10036, SCMD_NORTH},
    {CLASS_NECROMANCER, 10036, SCMD_NORTH},
    {CLASS_PYROMANCER, 10036, SCMD_NORTH},
    {CLASS_CRYOMANCER, 10036, SCMD_NORTH},
    {CLASS_ILLUSIONIST, 10036, SCMD_NORTH},

    /* Timun's Citadel of testing guards */
    {-999, 17200, SCMD_WEST},

    /* Example to block everyone:
      {-999<--all  ,     5065,   SCMD_WEST},
    */

    /* this must go last -- add new guards above! */
    {-1, -1, -1}};

/* This function is used to limit the skill a players class can get.
 * The defult is full skill 1000, so dont use it if it is that.
 * It actually limits skill accross the levels, ie:

 MAX SKILL = 600/50 * lvl.  eg if 1000 was max 1000/50 = 20 *lvl

 */

int level_max_skill(struct char_data *ch, int level, int skill) {
    int max_skill = 1000;

    /* The proficiency of individual spells comes from their related skills,
     * not the skill level of the spell itself.  Spells are set to a so-called
     * "proficiency" of 1000 when a quest spell is awarded. */
    if (IS_SPELL(skill))
        return 1000;

    switch (GET_RACE(ch)) {
    case RACE_OGRE:
        switch (skill) {
        case SKILL_PARRY:
        case SKILL_RIPOSTE:
            max_skill = 750;
            break;
        }
    case RACE_TROLL:
        switch (skill) {
        case SKILL_DODGE:
        case SKILL_PARRY:
            max_skill = 700;
            break;
        }
    }
    switch (GET_CLASS(ch)) {
    case CLASS_RANGER:
        switch (skill) {
        case SKILL_HUNT:
            max_skill = 750;
            break;
        }
        break;
    case CLASS_PRIEST:
        switch (skill) {
        case SKILL_DOUBLE_ATTACK:
            max_skill = 500;
            break;
        }
        break;
    case CLASS_HUNTER:
        switch (skill) {}
        break;
    case CLASS_WARRIOR:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 600;
            break;
        }
        break;
    case CLASS_MERCENARY:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 850;
            break;
        }
        break;
    case CLASS_ASSASSIN:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 750;
            break;
        }
        break;
    case CLASS_THIEF:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 600;
            break;
        }
        break;
    case CLASS_ROGUE:
        switch (skill) {
        case SKILL_TRACK:
            max_skill = 650;
            break;
        }
        break;
    case CLASS_ILLUSIONIST:
        switch (skill) {
        case SKILL_HIDE:
            max_skill = 500;
            break;
        }
        break;
    }
    return (MIN((10 * level) + 50, max_skill));
}

int return_max_skill(struct char_data *ch, int skill) { return level_max_skill(ch, GET_LEVEL(ch), skill); }

void init_char_class(struct char_data *ch) { /* Nothing much to do here. */
}

void update_char_class(struct char_data *ch) {
    if (!VALID_CLASS(ch)) {
        char buf[500];
        sprintf(buf, "update_char_class: %s doesn't have a valid class (%d).", GET_NAME(ch), GET_CLASS(ch));
        log(buf);
        return;
    }

    /* Any bits that might get set below should be cleared here first. */
    REMOVE_FLAGS(EFF_FLAGS(ch), class_effects_mask, NUM_EFF_FLAGS);

    /* Reset effect flags for this race */
    SET_FLAGS(EFF_FLAGS(ch), classes[(int)GET_CLASS(ch)].effect_flags, NUM_EFF_FLAGS);
}

/*
 * This function controls the change to maxmove, maxmana, and maxhp whenever
 * someone gains or loses a level. */

/*modified by proky so if lose level uses GET_LASTLEVEL(ch)*/

void advance_level(struct char_data *ch, enum level_action action) {
    int add_hp = 0, c = GET_CLASS(ch);

    /*
     * First determine how much hp the player should gain based on their
     * class under normal circumstances.
     */
    switch (c) {
    case CLASS_SORCERER:
    case CLASS_CRYOMANCER:
    case CLASS_ILLUSIONIST:
    case CLASS_PYROMANCER:
    case CLASS_NECROMANCER:
    case CLASS_CONJURER:
        if (action == LEVEL_GAIN)
            add_hp += number(3, 8);
        else
            add_hp -= 8;
        break;

    case CLASS_CLERIC:
    case CLASS_SHAMAN:
    case CLASS_PRIEST:
    case CLASS_DIABOLIST:
    case CLASS_MYSTIC:
        if (action == LEVEL_GAIN)
            add_hp += number(5, 10);
        else
            add_hp -= 10;
        break;

    case CLASS_DRUID:
        if (action == LEVEL_GAIN)
            add_hp += number(5, 11);
        else
            add_hp -= 11;
        break;

    case CLASS_ASSASSIN:
        if (action == LEVEL_GAIN)
            add_hp += number(7, 12);
        else
            add_hp -= 12;
        break;

    case CLASS_THIEF:
    case CLASS_ROGUE:
    case CLASS_BARD:
        if (action == LEVEL_GAIN)
            add_hp += number(7, 13);
        else
            add_hp -= 13;
        break;

    case CLASS_MERCENARY:
        if (action == LEVEL_GAIN)
            add_hp += number(7, 14);
        else
            add_hp -= 14;
        break;

    case CLASS_RANGER:
        if (action == LEVEL_GAIN)
            add_hp += number(9, 13);
        else
            add_hp -= 13;
        break;

    case CLASS_PALADIN:
    case CLASS_ANTI_PALADIN:
        if (action == LEVEL_GAIN)
            add_hp += number(9, 14);
        else
            add_hp -= 14;
        break;

    case CLASS_WARRIOR:
    case CLASS_MONK:
    case CLASS_BERSERKER:
    case CLASS_HUNTER:
        if (action == LEVEL_GAIN)
            add_hp += number(10, 15);
        else
            add_hp -= 14;
        break;

    default:
        if (action == LEVEL_GAIN)
            add_hp += number(5, 10);
        else
            add_hp -= 9;
        log("SYSERR: Unrecognized class %d in advance_char", c);
    }

    /*
     * If this is a magical class and the char is losing a level,
     * run init_mem_list to clear out spells they shouldn't know.
     */
    if (classes[c].magical && action == LEVEL_LOSE)
        init_mem_list(ch);

    /*
     * If losing a level, and there is a value in GET_LASTLEVEL,
     * use that instead of whatever we calculated above.
     */
    if (action == LEVEL_LOSE && GET_LASTLEVEL(ch) != 0)
        add_hp = 0 - (GET_LASTLEVEL(ch) + number(0, 1));

    /* If over level 30, use constants for hitpoints */
    if (GET_LEVEL(ch) > 30) {
        if (action == LEVEL_GAIN)
            add_hp = classes[c].hp_lev;
        else
            add_hp = 0 - classes[c].hp_lev;
    }

    /* Add to natural hitpoints and then recalculate other hitpoints */
    ch->points.max_hit += add_hp;
    GET_BASE_HIT(ch) += add_hp;
    effect_total(ch);

    /* Calculate mana */
    if (GET_LEVEL(ch) > 10)
        ch->points.max_mana = (GET_LEVEL(ch) * GET_LEVEL(ch)) / 10;
    else
        ch->points.max_mana = 0;

    /* Give immortals special treatment */
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        GET_COND(ch, FULL) = (char)-1;
        GET_COND(ch, THIRST) = (char)-1;
        GET_COND(ch, DRUNK) = (char)-1;
        SET_FLAG(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }

    /* Modify clan power */
    if (GET_CLAN(ch) && IS_CLAN_MEMBER(ch)) {
        if (action == LEVEL_GAIN)
            ++GET_CLAN(ch)->power;
        else
            --GET_CLAN(ch)->power;
    }

    check_regen_rates(ch); /* start regening new points */
    update_char(ch);       /* update skills/spells/innates/etc. for new level */
    save_player_char(ch);
}

/*
 * ASSIGNMENT OF SPELLS AND SKILLS.  Here we define which spells, skills,
 * chants, and songs are assigned to which classes, and the minimum level
 * a character must be to use them.
 */
#define spell_assign(spellnum, class, level) skill_assign(spellnum, class, level)
#define chant_assign(chantnum, class, level) skill_assign(chantnum, class, level)
void assign_class_skills(void) {
    int i;

    /* Set up some skills common to multiple classes */
    for (i = 0; i < NUM_CLASSES; ++i) {

        /* Skills that all classes get. */
        skill_assign(SKILL_MOUNT, i, 1);
        skill_assign(SKILL_RIDING, i, 1);
        skill_assign(SKILL_BANDAGE, i, 1);
        skill_assign(SKILL_DOUSE, i, 1);
        skill_assign(SKILL_FIRST_AID, i, 1);

        /* Magic only */
        if (classes[i].magical) {
            skill_assign(SKILL_MEDITATE, i, 1);
            skill_assign(SKILL_KNOW_SPELL, i, 1);
            skill_assign(SKILL_QUICK_CHANT, i, 1);
        }

        /* Scribe users */
        if (classes[i].mem_mode == MEMORIZE)
            skill_assign(SKILL_SCRIBE, i, 1);

        /* Arcane magic types */
        if (i == CLASS_SORCERER || classes[i].subclass_of == CLASS_SORCERER) {
            skill_assign(SKILL_PIERCING, i, 1);
            skill_assign(SKILL_DODGE, i, 20);
        }
    }

    /* From here on down, the classes are listed in alphabetical order.
     * Within each class, skills should be sorted by level and spells
     * tend to be grouped by circle. */

    /* ANTI_PALADIN */
    skill_assign(SKILL_BLUDGEONING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_PIERCING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_SLASHING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_KICK, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_BASH, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_DODGE, CLASS_ANTI_PALADIN, 1);
    skill_assign(SKILL_TAME, CLASS_ANTI_PALADIN, 7);
    skill_assign(SKILL_GUARD, CLASS_ANTI_PALADIN, 10);
    skill_assign(SKILL_SWITCH, CLASS_ANTI_PALADIN, 10);
    skill_assign(SKILL_RESCUE, CLASS_ANTI_PALADIN, 10);
    skill_assign(SKILL_SUMMON_MOUNT, CLASS_ANTI_PALADIN, 15);
    skill_assign(SKILL_DUAL_WIELD, CLASS_ANTI_PALADIN, 20);
    skill_assign(SKILL_PARRY, CLASS_ANTI_PALADIN, 20);
    skill_assign(SKILL_RIPOSTE, CLASS_ANTI_PALADIN, 40);
    skill_assign(SKILL_VAMP_TOUCH, CLASS_ANTI_PALADIN, 45);
    skill_assign(SKILL_RETREAT, CLASS_ANTI_PALADIN, 60);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_ANTI_PALADIN, 70);
    skill_assign(SKILL_HITALL, CLASS_ANTI_PALADIN, 80);
    skill_assign(SKILL_DISARM, CLASS_ANTI_PALADIN, 50);

    spell_assign(SPELL_CAUSE_LIGHT, CLASS_ANTI_PALADIN, CIRCLE_1);
    spell_assign(SPELL_DARK_PRESENCE, CLASS_ANTI_PALADIN, CIRCLE_2);
    spell_assign(SPELL_DEMONSKIN, CLASS_ANTI_PALADIN, CIRCLE_2);
    spell_assign(SPELL_CREATE_FOOD, CLASS_ANTI_PALADIN, CIRCLE_3);
    spell_assign(SPELL_CREATE_WATER, CLASS_ANTI_PALADIN, CIRCLE_3);
    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_ANTI_PALADIN, CIRCLE_3);
    spell_assign(SPELL_CURSE, CLASS_ANTI_PALADIN, CIRCLE_4);
    spell_assign(SPELL_CAUSE_CRITIC, CLASS_ANTI_PALADIN, CIRCLE_5);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_ANTI_PALADIN, CIRCLE_5);
    spell_assign(SPELL_POISON, CLASS_ANTI_PALADIN, CIRCLE_5);
    spell_assign(SPELL_CURE_BLIND, CLASS_ANTI_PALADIN, CIRCLE_6);
    spell_assign(SPELL_BLINDNESS, CLASS_ANTI_PALADIN, CIRCLE_6);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_ANTI_PALADIN, CIRCLE_7);
    spell_assign(SPELL_SUMMON_CORPSE, CLASS_ANTI_PALADIN, CIRCLE_8);
    spell_assign(SPELL_SOULSHIELD, CLASS_ANTI_PALADIN, CIRCLE_8);
    spell_assign(SPELL_HARM, CLASS_ANTI_PALADIN, CIRCLE_9);
    spell_assign(SPELL_UNHOLY_WORD, CLASS_ANTI_PALADIN, CIRCLE_11);

    /* ASSASSIN */
    skill_assign(SKILL_INSTANT_KILL, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_SNEAK, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_BACKSTAB, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_HIDE, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_TRACK, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_DODGE, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_PARRY, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_PIERCING, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_SLASHING, CLASS_ASSASSIN, 1);
    skill_assign(SKILL_PICK_LOCK, CLASS_ASSASSIN, 6);
    skill_assign(SKILL_DUAL_WIELD, CLASS_ASSASSIN, 15);
    skill_assign(SKILL_THROATCUT, CLASS_ASSASSIN, 31);
    skill_assign(SKILL_KICK, CLASS_ASSASSIN, 36);
    skill_assign(SKILL_SHADOW, CLASS_ASSASSIN, 40);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_ASSASSIN, 65);

    /* BARD */
    skill_assign(SKILL_BACKSTAB, CLASS_BARD, 1);
    skill_assign(SKILL_PIERCING, CLASS_BARD, 1);
    skill_assign(SKILL_SNEAK, CLASS_BARD, 10);
    skill_assign(SKILL_PICK_LOCK, CLASS_BARD, 10);
    skill_assign(SKILL_STEAL, CLASS_BARD, 10);
    skill_assign(SKILL_HIDE, CLASS_BARD, 10);
    skill_assign(SKILL_DODGE, CLASS_BARD, 20);
    skill_assign(SKILL_PARRY, CLASS_BARD, 40);
    skill_assign(SKILL_TRACK, CLASS_BARD, 50);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_BARD, 70);
    skill_assign(SKILL_DUAL_WIELD, CLASS_BARD, 90);

    /* BERSERKER */
    skill_assign(SKILL_BERSERK, CLASS_BERSERKER, 10);
    skill_assign(SKILL_BLUDGEONING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_PIERCING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_SLASHING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_BERSERKER, 1);
    skill_assign(SKILL_SWITCH, CLASS_BERSERKER, 1);
    skill_assign(SKILL_KICK, CLASS_BERSERKER, 1);
    skill_assign(SKILL_DODGE, CLASS_BERSERKER, 1);
    skill_assign(SKILL_PARRY, CLASS_BERSERKER, 15);
    skill_assign(SKILL_DUAL_WIELD, CLASS_BERSERKER, 20);
    skill_assign(SKILL_CHANT, CLASS_BERSERKER, 25);
    chant_assign(CHANT_SPIRIT_BEAR, CLASS_BERSERKER, 25);
    skill_assign(SKILL_MAUL, CLASS_BERSERKER, 30);
    skill_assign(SKILL_TANTRUM, CLASS_BERSERKER, 42);
    skill_assign(SKILL_MEDITATE, CLASS_BERSERKER, 50);
    skill_assign(SKILL_RIPOSTE, CLASS_BERSERKER, 50);
    chant_assign(CHANT_SPIRIT_WOLF, CLASS_BERSERKER, 60);
    skill_assign(SKILL_BATTLE_HOWL, CLASS_BERSERKER, 67);
    skill_assign(SKILL_GROUND_SHAKER, CLASS_BERSERKER, 75);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_BERSERKER, 83);
    chant_assign(CHANT_INTERMINABLE_WRATH, CLASS_BERSERKER, 90);

    /* CLERIC */
    skill_assign(SKILL_BLUDGEONING, CLASS_CLERIC, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_CLERIC, 1);
    skill_assign(SKILL_DODGE, CLASS_CLERIC, 20);

    spell_assign(SPELL_DETECT_MAGIC, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_ARMOR, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CAUSE_LIGHT, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_CLERIC, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_CLERIC, CIRCLE_1);

    spell_assign(SPELL_CURE_SERIOUS, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_CLERIC, CIRCLE_2);
    spell_assign(SPELL_BLESS, CLASS_CLERIC, CIRCLE_2);

    spell_assign(SPELL_ENDURANCE, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_CURE_BLIND, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_CAUSE_CRITIC, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_CLERIC, CIRCLE_3);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_CLERIC, CIRCLE_3);

    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_BLINDNESS, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_SENSE_LIFE, CLASS_CLERIC, CIRCLE_4);
    spell_assign(SPELL_FLAMESTRIKE, CLASS_CLERIC, CIRCLE_4);

    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_HEAL, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_HARM, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_DESTROY_UNDEAD, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_SOULSHIELD, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_EARTHQUAKE, CLASS_CLERIC, CIRCLE_5);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_CLERIC, CIRCLE_5);

    spell_assign(SPELL_DARKNESS, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_ILLUMINATION, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_SILENCE, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_UNHOLY_WORD, CLASS_CLERIC, CIRCLE_6);
    spell_assign(SPELL_HOLY_WORD, CLASS_CLERIC, CIRCLE_6);

    spell_assign(SPELL_VITALITY, CLASS_CLERIC, CIRCLE_7);
    spell_assign(SPELL_WATERWALK, CLASS_CLERIC, CIRCLE_7);
    spell_assign(SPELL_FULL_HEAL, CLASS_CLERIC, CIRCLE_7);
    spell_assign(SPELL_FULL_HARM, CLASS_CLERIC, CIRCLE_7);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_CLERIC, CIRCLE_8);
    spell_assign(SPELL_GROUP_HEAL, CLASS_CLERIC, CIRCLE_8);

    spell_assign(SPELL_GREATER_VITALITY, CLASS_CLERIC, CIRCLE_9);

    spell_assign(SPELL_GROUP_ARMOR, CLASS_CLERIC, CIRCLE_10);

    spell_assign(SPELL_RESURRECT, CLASS_CLERIC, CIRCLE_11);

    spell_assign(SPELL_DRAGONS_HEALTH, CLASS_CLERIC, CIRCLE_12);

    /* CONJURER */
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_CONJURER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_CONJURER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_CONJURER, CIRCLE_1);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_CONJURER, CIRCLE_2);
    spell_assign(SPELL_BURNING_HANDS, CLASS_CONJURER, CIRCLE_2);
    spell_assign(SPELL_STRENGTH, CLASS_CONJURER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_CONJURER, CIRCLE_2);
    spell_assign(SPELL_IDENTIFY, CLASS_CONJURER, CIRCLE_3);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_CONJURER, CIRCLE_3);
    spell_assign(SPELL_CONCEALMENT, CLASS_CONJURER, CIRCLE_3);
    spell_assign(SPELL_SUMMON_ELEMENTAL, CLASS_CONJURER, CIRCLE_3);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_CONJURER, CIRCLE_4);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_CONJURER, CIRCLE_4);
    spell_assign(SPELL_WALL_OF_STONE, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_INFRAVISION, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_COLOR_SPRAY, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_WALL_OF_FOG, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_CONE_OF_COLD, CLASS_CONJURER, CIRCLE_5);
    spell_assign(SPELL_STONE_SKIN, CLASS_CONJURER, CIRCLE_6);
    spell_assign(SPELL_DETECT_POISON, CLASS_CONJURER, CIRCLE_6);
    spell_assign(SPELL_FIREBALL, CLASS_CONJURER, CIRCLE_6);
    spell_assign(SPELL_FLY, CLASS_CONJURER, CIRCLE_6);
    spell_assign(SPELL_INVISIBLE, CLASS_CONJURER, CIRCLE_7);
    spell_assign(SPELL_HASTE, CLASS_CONJURER, CIRCLE_7);
    spell_assign(SPELL_FARSEE, CLASS_CONJURER, CIRCLE_7);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_CONJURER, CIRCLE_8);
    spell_assign(SPELL_ENCHANT_WEAPON, CLASS_CONJURER, CIRCLE_9);
    spell_assign(SPELL_SUMMON_DEMON, CLASS_CONJURER, CIRCLE_10);
    spell_assign(SPELL_SUMMON_GREATER_DEMON, CLASS_CONJURER, CIRCLE_11);

    /* CRYOMANCER */
    spell_assign(SPELL_ICE_DARTS, CLASS_CRYOMANCER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_CRYOMANCER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_CRYOMANCER, CIRCLE_1);
    spell_assign(SPELL_WATERWALK, CLASS_CRYOMANCER, CIRCLE_1);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_CRYOMANCER, CIRCLE_2);
    spell_assign(SPELL_STRENGTH, CLASS_CRYOMANCER, CIRCLE_2);
    spell_assign(SPELL_CONCEALMENT, CLASS_CRYOMANCER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_CRYOMANCER, CIRCLE_2);
    spell_assign(SPELL_ICE_ARMOR, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_EXTINGUISH, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_CRYOMANCER, CIRCLE_3);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_ICE_DAGGER, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_TELEPORT, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_WORLD_TELEPORT, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_RAY_OF_ENFEEB, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_LEVITATE, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_COLDSHIELD, CLASS_CRYOMANCER, CIRCLE_4);
    spell_assign(SPELL_MINOR_PARALYSIS, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_FREEZING_WIND, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_FARSEE, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_CONE_OF_COLD, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_INVISIBLE, CLASS_CRYOMANCER, CIRCLE_5);
    spell_assign(SPELL_ICE_STORM, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_HASTE, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_RAIN, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_MINOR_GLOBE, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_STONE_SKIN, CLASS_CRYOMANCER, CIRCLE_6);
    spell_assign(SPELL_FREEZE, CLASS_CRYOMANCER, CIRCLE_7);
    spell_assign(SPELL_WALL_OF_ICE, CLASS_CRYOMANCER, CIRCLE_8);
    spell_assign(SPELL_CHAIN_LIGHTNING, CLASS_CRYOMANCER, CIRCLE_8);
    spell_assign(SPELL_MAJOR_GLOBE, CLASS_CRYOMANCER, CIRCLE_8);
    spell_assign(SPELL_FLY, CLASS_CRYOMANCER, CIRCLE_8);
    spell_assign(SPELL_RELOCATE, CLASS_CRYOMANCER, CIRCLE_9);
    spell_assign(SPELL_MASS_INVIS, CLASS_CRYOMANCER, CIRCLE_9);
    spell_assign(SPELL_ICEBALL, CLASS_CRYOMANCER, CIRCLE_9);
    spell_assign(SPELL_NEGATE_COLD, CLASS_CRYOMANCER, CIRCLE_10);
    spell_assign(SPELL_WATERFORM, CLASS_CRYOMANCER, CIRCLE_10);
    spell_assign(SPELL_FLOOD, CLASS_CRYOMANCER, CIRCLE_11);
    spell_assign(SPELL_ICE_SHARDS, CLASS_CRYOMANCER, CIRCLE_12);

    /* DIABOLIST */
    skill_assign(SKILL_BLUDGEONING, CLASS_DIABOLIST, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_DIABOLIST, 1);
    skill_assign(SKILL_DODGE, CLASS_DIABOLIST, 20);

    spell_assign(SPELL_DEMONSKIN, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_CAUSE_LIGHT, CLASS_DIABOLIST, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_DIABOLIST, CIRCLE_1);

    spell_assign(SPELL_CURE_SERIOUS, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DARK_PRESENCE, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DARK_FEAST, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_DIABOLIST, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_DIABOLIST, CIRCLE_2);

    spell_assign(SPELL_HELL_BOLT, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_CAUSE_CRITIC, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_CURE_BLIND, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_DIABOLIST, CIRCLE_3);
    spell_assign(SPELL_ENDURANCE, CLASS_DIABOLIST, CIRCLE_3);

    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_DISEASE, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_DEMONIC_ASPECT, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_ELEMENTAL_WARDING, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_BLINDNESS, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_SENSE_LIFE, CLASS_DIABOLIST, CIRCLE_4);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_DIABOLIST, CIRCLE_4);

    spell_assign(SPELL_SOULSHIELD, CLASS_DIABOLIST, CIRCLE_5);
    spell_assign(SPELL_EARTHQUAKE, CLASS_DIABOLIST, CIRCLE_5);
    spell_assign(SPELL_HEAL, CLASS_DIABOLIST, CIRCLE_5);
    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_DIABOLIST, CIRCLE_5);

    spell_assign(SPELL_SANE_MIND, CLASS_DIABOLIST, CIRCLE_6);
    spell_assign(SPELL_STYGIAN_ERUPTION, CLASS_DIABOLIST, CIRCLE_6);
    spell_assign(SPELL_DARKNESS, CLASS_DIABOLIST, CIRCLE_6);
    spell_assign(SPELL_SILENCE, CLASS_DIABOLIST, CIRCLE_6);

    spell_assign(SPELL_INSANITY, CLASS_DIABOLIST, CIRCLE_7);
    spell_assign(SPELL_DEMONIC_MUTATION, CLASS_DIABOLIST, CIRCLE_7);
    spell_assign(SPELL_WATERWALK, CLASS_DIABOLIST, CIRCLE_7);
    spell_assign(SPELL_FULL_HEAL, CLASS_DIABOLIST, CIRCLE_7);

    spell_assign(SPELL_HELLFIRE_BRIMSTONE, CLASS_DIABOLIST, CIRCLE_8);
    spell_assign(SPELL_SPEAK_IN_TONGUES, CLASS_DIABOLIST, CIRCLE_8);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_DIABOLIST, CIRCLE_8);
    spell_assign(SPELL_GROUP_HEAL, CLASS_DIABOLIST, CIRCLE_8);

    spell_assign(SPELL_WINGS_OF_HELL, CLASS_DIABOLIST, CIRCLE_9);
    spell_assign(SPELL_BANISH, CLASS_DIABOLIST, CIRCLE_9);
    spell_assign(SPELL_UNHOLY_WORD, CLASS_DIABOLIST, CIRCLE_9);

    spell_assign(SPELL_WORD_OF_COMMAND, CLASS_DIABOLIST, CIRCLE_10);
    spell_assign(SPELL_FULL_HARM, CLASS_DIABOLIST, CIRCLE_10);

    spell_assign(SPELL_RESURRECT, CLASS_DIABOLIST, CIRCLE_11);
    spell_assign(SPELL_HELLS_GATE, CLASS_DIABOLIST, CIRCLE_11);

    /* DRUID */
    skill_assign(SKILL_BLUDGEONING, CLASS_DRUID, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_DRUID, 1);
    skill_assign(SKILL_SHAPECHANGE, CLASS_DRUID, 1);
    skill_assign(SKILL_TAME, CLASS_DRUID, 1);
    skill_assign(SKILL_DODGE, CLASS_DRUID, 20);

    spell_assign(SPELL_DETECT_MAGIC, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_BARKSKIN, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_DRUID, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_DRUID, CIRCLE_1);

    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_NIGHT_VISION, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_CURE_LIGHT, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_DETECT_POISON, CLASS_DRUID, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_DRUID, CIRCLE_2);

    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_WRITHING_WEEDS, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_ENDURANCE, CLASS_DRUID, CIRCLE_3);
    spell_assign(SPELL_MOONBEAM, CLASS_DRUID, CIRCLE_3);

    spell_assign(SPELL_CURE_CRITIC, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_CREATE_SPRING, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_CURE_BLIND, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_EARTHQUAKE, CLASS_DRUID, CIRCLE_4);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_DRUID, CIRCLE_4);

    spell_assign(SPELL_CONTROL_WEATHER, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_HARM, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_DRUID, CIRCLE_5);
    spell_assign(SPELL_NOURISHMENT, CLASS_DRUID, CIRCLE_5);

    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_HEAL, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_WATERWALK, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_DRUID, CIRCLE_6);
    spell_assign(SPELL_DARKNESS, CLASS_DRUID, CIRCLE_6);

    spell_assign(SPELL_ILLUMINATION, CLASS_DRUID, CIRCLE_7);
    spell_assign(SPELL_GAIAS_CLOAK, CLASS_DRUID, CIRCLE_7);
    spell_assign(SPELL_CALL_LIGHTNING, CLASS_DRUID, CIRCLE_7);

    spell_assign(SPELL_NATURES_EMBRACE, CLASS_DRUID, CIRCLE_8);
    spell_assign(SPELL_ENTANGLE, CLASS_DRUID, CIRCLE_8);
    spell_assign(SPELL_URBAN_RENEWAL, CLASS_DRUID, CIRCLE_8);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_DRUID, CIRCLE_8);

    spell_assign(SPELL_SUNRAY, CLASS_DRUID, CIRCLE_9);
    spell_assign(SPELL_ARMOR_OF_GAIA, CLASS_DRUID, CIRCLE_9);
    spell_assign(SPELL_INVIGORATE, CLASS_DRUID, CIRCLE_9);

    spell_assign(SPELL_WANDERING_WOODS, CLASS_DRUID, CIRCLE_10);
    spell_assign(SPELL_MOONWELL, CLASS_DRUID, CIRCLE_10);

    spell_assign(SPELL_CREEPING_DOOM, CLASS_DRUID, CIRCLE_11);

    /* HUNTER */
    skill_assign(SKILL_GUARD, CLASS_HUNTER, 1);
    skill_assign(SKILL_SWITCH, CLASS_HUNTER, 1);
    skill_assign(SKILL_KICK, CLASS_HUNTER, 1);
    skill_assign(SKILL_RESCUE, CLASS_HUNTER, 1);
    skill_assign(SKILL_BASH, CLASS_HUNTER, 1);
    skill_assign(SKILL_DUAL_WIELD, CLASS_HUNTER, 1);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_HUNTER, 1);
    skill_assign(SKILL_DODGE, CLASS_HUNTER, 1);
    skill_assign(SKILL_PARRY, CLASS_HUNTER, 1);
    skill_assign(SKILL_RIPOSTE, CLASS_HUNTER, 1);
    skill_assign(SKILL_BLUDGEONING, CLASS_HUNTER, 1);
    skill_assign(SKILL_PIERCING, CLASS_HUNTER, 1);
    skill_assign(SKILL_SLASHING, CLASS_HUNTER, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_HUNTER, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_HUNTER, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_HUNTER, 1);
    skill_assign(SKILL_TAME, CLASS_HUNTER, 7);
    skill_assign(SKILL_TRACK, CLASS_HUNTER, 36);
    skill_assign(SKILL_HUNT, CLASS_HUNTER, 41);

    /* ILLUSIONIST */
    skill_assign(SKILL_CONCEAL, CLASS_ILLUSIONIST, 10);
    skill_assign(SKILL_HIDE, CLASS_ILLUSIONIST, 20);
    skill_assign(SKILL_BACKSTAB, CLASS_ILLUSIONIST, 13);

    spell_assign(SPELL_MAGIC_MISSILE, CLASS_ILLUSIONIST, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_ILLUSIONIST, CIRCLE_1);
    spell_assign(SPELL_VENTRILOQUATE, CLASS_ILLUSIONIST, CIRCLE_1);

    spell_assign(SPELL_CHILL_TOUCH, CLASS_ILLUSIONIST, CIRCLE_2);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_ILLUSIONIST, CIRCLE_2);
    spell_assign(SPELL_MAGIC_TORCH, CLASS_ILLUSIONIST, CIRCLE_2);
    spell_assign(SPELL_PHANTASM, CLASS_ILLUSIONIST, CIRCLE_2);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_ILLUSIONIST, CIRCLE_3);
    spell_assign(SPELL_FEAR, CLASS_ILLUSIONIST, CIRCLE_3);
    spell_assign(SPELL_MESMERIZE, CLASS_ILLUSIONIST, CIRCLE_3);
    spell_assign(SPELL_CHARM, CLASS_ILLUSIONIST, CIRCLE_3);

    spell_assign(SPELL_BLINDNESS, CLASS_ILLUSIONIST, CIRCLE_4);
    spell_assign(SPELL_CONFUSION, CLASS_ILLUSIONIST, CIRCLE_4);
    spell_assign(SPELL_MISDIRECTION, CLASS_ILLUSIONIST, CIRCLE_4);
    spell_assign(SPELL_NIGHTMARE, CLASS_ILLUSIONIST, CIRCLE_4);

    spell_assign(SPELL_COLOR_SPRAY, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_DISCORPORATE, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_INFRAVISION, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_SIMULACRUM, CLASS_ILLUSIONIST, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_ILLUSIONIST, CIRCLE_5);

    spell_assign(SPELL_DETECT_INVIS, CLASS_ILLUSIONIST, CIRCLE_6);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_ILLUSIONIST, CIRCLE_6);
    spell_assign(SPELL_ILLUMINATION, CLASS_ILLUSIONIST, CIRCLE_6);
    spell_assign(SPELL_ISOLATION, CLASS_ILLUSIONIST, CIRCLE_6);

    spell_assign(SPELL_FARSEE, CLASS_ILLUSIONIST, CIRCLE_7);
    spell_assign(SPELL_INSANITY, CLASS_ILLUSIONIST, CIRCLE_7);
    spell_assign(SPELL_INVISIBLE, CLASS_ILLUSIONIST, CIRCLE_7);
    spell_assign(SPELL_HYSTERIA, CLASS_ILLUSIONIST, CIRCLE_7);

    spell_assign(SPELL_ILLUSORY_WALL, CLASS_ILLUSIONIST, CIRCLE_8);

    spell_assign(SPELL_FAMILIARITY, CLASS_ILLUSIONIST, CIRCLE_9);

    spell_assign(SPELL_SEVERANCE, CLASS_ILLUSIONIST, CIRCLE_10);

    spell_assign(SPELL_GLORY, CLASS_ILLUSIONIST, CIRCLE_11);

    spell_assign(SPELL_SOUL_REAVER, CLASS_ILLUSIONIST, CIRCLE_12);

    /* MERCENARY */
    skill_assign(SKILL_BLUDGEONING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_PIERCING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_SLASHING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_MERCENARY, 1);
    skill_assign(SKILL_KICK, CLASS_MERCENARY, 1);
    skill_assign(SKILL_BASH, CLASS_MERCENARY, 1);
    skill_assign(SKILL_DODGE, CLASS_MERCENARY, 1);
    skill_assign(SKILL_GUARD, CLASS_MERCENARY, 10);
    skill_assign(SKILL_BACKSTAB, CLASS_MERCENARY, 11);
    skill_assign(SKILL_DUAL_WIELD, CLASS_MERCENARY, 15);
    skill_assign(SKILL_BIND, CLASS_MERCENARY, 16);
    skill_assign(SKILL_HIDE, CLASS_MERCENARY, 20);
    skill_assign(SKILL_DISARM, CLASS_MERCENARY, 20);
    skill_assign(SKILL_TRACK, CLASS_MERCENARY, 30);
    skill_assign(SKILL_SWITCH, CLASS_MERCENARY, 40);
    skill_assign(SKILL_PARRY, CLASS_MERCENARY, 40);
    skill_assign(SKILL_RETREAT, CLASS_MERCENARY, 40);
    skill_assign(SKILL_RIPOSTE, CLASS_MERCENARY, 60);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_MERCENARY, 70);
    skill_assign(SKILL_GROUP_RETREAT, CLASS_MERCENARY, 80);

    /* MONKS */
    skill_assign(SKILL_CHANT, CLASS_MONK, 15);
    skill_assign(SKILL_SAFEFALL, CLASS_MONK, 1);
    skill_assign(SKILL_BAREHAND, CLASS_MONK, 1);
    skill_assign(SKILL_KICK, CLASS_MONK, 1);
    skill_assign(SKILL_DUAL_WIELD, CLASS_MONK, 1);
    skill_assign(SKILL_DODGE, CLASS_MONK, 1);
    skill_assign(SKILL_BLUDGEONING, CLASS_MONK, 1);
    skill_assign(SKILL_PARRY, CLASS_MONK, 10);
    skill_assign(SKILL_RIPOSTE, CLASS_MONK, 20);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_MONK, 30);
    skill_assign(SKILL_SWITCH, CLASS_MONK, 40);
    skill_assign(SKILL_SPRINGLEAP, CLASS_MONK, 50);
    skill_assign(SKILL_CORNER, CLASS_MONK, 80);

    chant_assign(CHANT_REGENERATION, CLASS_MONK, 15);
    chant_assign(CHANT_BATTLE_HYMN, CLASS_MONK, 30);
    chant_assign(CHANT_WAR_CRY, CLASS_MONK, 90);
    chant_assign(CHANT_PEACE, CLASS_MONK, 70);
    chant_assign(CHANT_SHADOWS_SORROW_SONG, CLASS_MONK, 30);
    chant_assign(CHANT_IVORY_SYMPHONY, CLASS_MONK, 45);
    chant_assign(CHANT_ARIA_OF_DISSONANCE, CLASS_MONK, 60);
    chant_assign(CHANT_SONATA_OF_MALAISE, CLASS_MONK, 60);
    chant_assign(CHANT_APOCALYPTIC_ANTHEM, CLASS_MONK, 75);
    chant_assign(CHANT_SEED_OF_DESTRUCTION, CLASS_MONK, 99);

    /* MYSTIC */
    skill_assign(SKILL_BLUDGEONING, CLASS_MYSTIC, 1);
    skill_assign(SKILL_SLASHING, CLASS_MYSTIC, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_MYSTIC, 1);
    skill_assign(SKILL_DODGE, CLASS_MYSTIC, 1);

    spell_assign(SPELL_CURE_LIGHT, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_ARMOR, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_BLESS, CLASS_MYSTIC, CIRCLE_1);
    spell_assign(SPELL_DETECT_POISON, CLASS_MYSTIC, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_MYSTIC, CIRCLE_2);
    spell_assign(SPELL_CURE_BLIND, CLASS_MYSTIC, CIRCLE_2);
    spell_assign(SPELL_BLINDNESS, CLASS_MYSTIC, CIRCLE_4);
    spell_assign(SPELL_INFRAVISION, CLASS_MYSTIC, CIRCLE_9);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_MYSTIC, CIRCLE_3);
    spell_assign(SPELL_GROUP_ARMOR, CLASS_MYSTIC, CIRCLE_10);
    spell_assign(SPELL_CURE_CRITIC, CLASS_MYSTIC, CIRCLE_2);
    spell_assign(SPELL_SUMMON, CLASS_MYSTIC, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_MYSTIC, CIRCLE_3);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_MYSTIC, CIRCLE_6);
    spell_assign(SPELL_EARTHQUAKE, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_MYSTIC, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_MYSTIC, CIRCLE_4);
    spell_assign(SPELL_HEAL, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_HARM, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_GROUP_HEAL, CLASS_MYSTIC, CIRCLE_8);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_SENSE_LIFE, CLASS_MYSTIC, CIRCLE_4);
    spell_assign(SPELL_FULL_HEAL, CLASS_MYSTIC, CIRCLE_7);
    spell_assign(SPELL_FULL_HARM, CLASS_MYSTIC, CIRCLE_9);
    spell_assign(SPELL_VITALITY, CLASS_MYSTIC, CIRCLE_5);
    spell_assign(SPELL_DARKNESS, CLASS_MYSTIC, CIRCLE_6);

    /* NECRO */
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_NECROMANCER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_NECROMANCER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_NECROMANCER, CIRCLE_1);
    spell_assign(SPELL_DECAY, CLASS_NECROMANCER, CIRCLE_1);
    spell_assign(SPELL_BURNING_HANDS, CLASS_NECROMANCER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_NECROMANCER, CIRCLE_2);
    spell_assign(SPELL_CONCEALMENT, CLASS_NECROMANCER, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_NECROMANCER, CIRCLE_2);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_NECROMANCER, CIRCLE_3);
    spell_assign(SPELL_POISON, CLASS_NECROMANCER, CIRCLE_3);
    spell_assign(SPELL_STRENGTH, CLASS_NECROMANCER, CIRCLE_3);
    spell_assign(SPELL_ANIMATE_DEAD, CLASS_NECROMANCER, CIRCLE_3);
    spell_assign(SPELL_COLDSHIELD, CLASS_NECROMANCER, CIRCLE_4);
    spell_assign(SPELL_BONE_ARMOR, CLASS_NECROMANCER, CIRCLE_4);
    spell_assign(SPELL_SPINECHILLER, CLASS_NECROMANCER, CIRCLE_4);
    spell_assign(SPELL_INFRAVISION, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_IDENTIFY, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_ENERGY_DRAIN, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_CONE_OF_COLD, CLASS_NECROMANCER, CIRCLE_5);
    spell_assign(SPELL_SUMMON_CORPSE, CLASS_NECROMANCER, CIRCLE_6);
    spell_assign(SPELL_DETECT_POISON, CLASS_NECROMANCER, CIRCLE_6);
    spell_assign(SPELL_INVISIBLE, CLASS_NECROMANCER, CIRCLE_6);
    spell_assign(SPELL_PYRE, CLASS_NECROMANCER, CIRCLE_6);
    spell_assign(SPELL_HASTE, CLASS_NECROMANCER, CIRCLE_7);
    spell_assign(SPELL_FARSEE, CLASS_NECROMANCER, CIRCLE_7);
    spell_assign(SPELL_IRON_MAIDEN, CLASS_NECROMANCER, CIRCLE_7);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_NECROMANCER, CIRCLE_8);
    spell_assign(SPELL_FRACTURE, CLASS_NECROMANCER, CIRCLE_8);
    spell_assign(SPELL_SOUL_TAP, CLASS_NECROMANCER, CIRCLE_9);
    spell_assign(SPELL_STONE_SKIN, CLASS_NECROMANCER, CIRCLE_9);
    spell_assign(SPELL_REBUKE_UNDEAD, CLASS_NECROMANCER, CIRCLE_10);
    spell_assign(SPELL_BONE_DRAW, CLASS_NECROMANCER, CIRCLE_10);
    spell_assign(SPELL_DEGENERATION, CLASS_NECROMANCER, CIRCLE_11);
    spell_assign(SPELL_STONE_SKIN, CLASS_NECROMANCER, CIRCLE_12);
    spell_assign(SPELL_SHIFT_CORPSE, CLASS_NECROMANCER, CIRCLE_13);

    /* PALADIN */
    skill_assign(SKILL_BLUDGEONING, CLASS_PALADIN, 1);
    skill_assign(SKILL_PIERCING, CLASS_PALADIN, 1);
    skill_assign(SKILL_SLASHING, CLASS_PALADIN, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_PALADIN, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_PALADIN, 1);
    skill_assign(SKILL_KICK, CLASS_PALADIN, 1);
    skill_assign(SKILL_BASH, CLASS_PALADIN, 1);
    skill_assign(SKILL_TAME, CLASS_PALADIN, 1);
    skill_assign(SKILL_DODGE, CLASS_PALADIN, 1);
    skill_assign(SKILL_GUARD, CLASS_PALADIN, 10);
    skill_assign(SKILL_SWITCH, CLASS_PALADIN, 10);
    skill_assign(SKILL_RESCUE, CLASS_PALADIN, 10);
    skill_assign(SKILL_SUMMON_MOUNT, CLASS_PALADIN, 15);
    skill_assign(SKILL_DUAL_WIELD, CLASS_PALADIN, 20);
    skill_assign(SKILL_PARRY, CLASS_PALADIN, 20);
    skill_assign(SKILL_RIPOSTE, CLASS_PALADIN, 40);
    skill_assign(SKILL_HITALL, CLASS_PALADIN, 80);
    skill_assign(SKILL_DISARM, CLASS_PALADIN, 50);
    skill_assign(SKILL_RETREAT, CLASS_PALADIN, 60);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_PALADIN, 70);

    spell_assign(SPELL_CURE_LIGHT, CLASS_PALADIN, CIRCLE_1);
    spell_assign(SPELL_BLESS, CLASS_PALADIN, CIRCLE_1);
    spell_assign(SPELL_ARMOR, CLASS_PALADIN, CIRCLE_2);
    spell_assign(SPELL_CREATE_FOOD, CLASS_PALADIN, CIRCLE_3);
    spell_assign(SPELL_CREATE_WATER, CLASS_PALADIN, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_PALADIN, CIRCLE_4);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_PALADIN, CIRCLE_4);
    spell_assign(SPELL_DETECT_POISON, CLASS_PALADIN, CIRCLE_5);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_PALADIN, CIRCLE_5);
    spell_assign(SPELL_CURE_BLIND, CLASS_PALADIN, CIRCLE_6);
    spell_assign(SPELL_CURE_CRITIC, CLASS_PALADIN, CIRCLE_6);
    spell_assign(SPELL_REMOVE_POISON, CLASS_PALADIN, CIRCLE_7);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_PALADIN, CIRCLE_7);
    spell_assign(SPELL_SOULSHIELD, CLASS_PALADIN, CIRCLE_8);
    spell_assign(SPELL_HEAL, CLASS_PALADIN, CIRCLE_9);
    spell_assign(SPELL_HOLY_WORD, CLASS_PALADIN, CIRCLE_10);

    /* PRIEST */
    skill_assign(SKILL_BLUDGEONING, CLASS_PRIEST, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_PRIEST, 1);
    skill_assign(SKILL_DODGE, CLASS_PRIEST, 20);

    spell_assign(SPELL_DETECT_MAGIC, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_ARMOR, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CREATE_FOOD, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CAUSE_LIGHT, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_PRIEST, CIRCLE_1);
    spell_assign(SPELL_LESSER_ENDURANCE, CLASS_PRIEST, CIRCLE_1);

    spell_assign(SPELL_DETECT_POISON, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_VIGORIZE_LIGHT, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_CIRCLE_OF_LIGHT, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_PRESERVE, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_CAUSE_SERIOUS, CLASS_PRIEST, CIRCLE_2);
    spell_assign(SPELL_BLESS, CLASS_PRIEST, CIRCLE_2);

    spell_assign(SPELL_DIVINE_BOLT, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_VIGORIZE_SERIOUS, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_CAUSE_CRITIC, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_CURE_BLIND, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_REMOVE_POISON, CLASS_PRIEST, CIRCLE_3);
    spell_assign(SPELL_ENDURANCE, CLASS_PRIEST, CIRCLE_3);

    spell_assign(SPELL_VIGORIZE_CRITIC, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_ELEMENTAL_WARDING, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_BLINDNESS, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_SUMMON, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_SENSE_LIFE, CLASS_PRIEST, CIRCLE_4);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_PRIEST, CIRCLE_4);

    spell_assign(SPELL_PRAYER, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_SOULSHIELD, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_DESTROY_UNDEAD, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_EARTHQUAKE, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_HEAL, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_SANE_MIND, CLASS_PRIEST, CIRCLE_5);
    spell_assign(SPELL_GREATER_ENDURANCE, CLASS_PRIEST, CIRCLE_5);

    spell_assign(SPELL_LESSER_EXORCISM, CLASS_PRIEST, CIRCLE_6);
    spell_assign(SPELL_DIVINE_RAY, CLASS_PRIEST, CIRCLE_6);
    spell_assign(SPELL_ILLUMINATION, CLASS_PRIEST, CIRCLE_6);
    spell_assign(SPELL_SILENCE, CLASS_PRIEST, CIRCLE_6);

    spell_assign(SPELL_VITALITY, CLASS_PRIEST, CIRCLE_7);
    spell_assign(SPELL_WATERWALK, CLASS_PRIEST, CIRCLE_7);
    spell_assign(SPELL_FULL_HEAL, CLASS_PRIEST, CIRCLE_7);

    spell_assign(SPELL_SPEAK_IN_TONGUES, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_ENLIGHTENMENT, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_GROUP_ARMOR, CLASS_PRIEST, CIRCLE_8);
    spell_assign(SPELL_GROUP_HEAL, CLASS_PRIEST, CIRCLE_8);

    spell_assign(SPELL_GREATER_VITALITY, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_EXORCISM, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_WINGS_OF_HEAVEN, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_BANISH, CLASS_PRIEST, CIRCLE_9);
    spell_assign(SPELL_HOLY_WORD, CLASS_PRIEST, CIRCLE_9);

    spell_assign(SPELL_WORD_OF_COMMAND, CLASS_PRIEST, CIRCLE_10);
    spell_assign(SPELL_FULL_HARM, CLASS_PRIEST, CIRCLE_10);
    spell_assign(SPELL_DIVINE_ESSENCE, CLASS_PRIEST, CIRCLE_10);

    spell_assign(SPELL_HEAVENS_GATE, CLASS_PRIEST, CIRCLE_11);
    spell_assign(SPELL_RESURRECT, CLASS_PRIEST, CIRCLE_11);

    spell_assign(SPELL_DRAGONS_HEALTH, CLASS_PRIEST, CIRCLE_12);

    /* PYROMANCER */
    spell_assign(SPELL_BURNING_HANDS, CLASS_PYROMANCER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_PYROMANCER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_PYROMANCER, CIRCLE_1);

    spell_assign(SPELL_CONCEALMENT, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_DETONATION, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_FIRE_DARTS, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_MAGIC_TORCH, CLASS_PYROMANCER, CIRCLE_2);
    spell_assign(SPELL_STRENGTH, CLASS_PYROMANCER, CIRCLE_2);

    spell_assign(SPELL_DISPEL_MAGIC, CLASS_PYROMANCER, CIRCLE_3);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_PYROMANCER, CIRCLE_3);
    spell_assign(SPELL_PHOSPHORIC_EMBERS, CLASS_PYROMANCER, CIRCLE_3);
    spell_assign(SPELL_SMOKE, CLASS_PYROMANCER, CIRCLE_3);

    spell_assign(SPELL_FIREBALL, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_FIRESHIELD, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_FLAME_BLADE, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_INFRAVISION, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_LEVITATE, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_MIRAGE, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_RAY_OF_ENFEEB, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_TELEPORT, CLASS_PYROMANCER, CIRCLE_4);
    spell_assign(SPELL_WORLD_TELEPORT, CLASS_PYROMANCER, CIRCLE_4);

    spell_assign(SPELL_DIMENSION_DOOR, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_FARSEE, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_INVISIBLE, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_MINOR_PARALYSIS, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_POSITIVE_FIELD, CLASS_PYROMANCER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_PYROMANCER, CIRCLE_5);

    spell_assign(SPELL_FIRESTORM, CLASS_PYROMANCER, CIRCLE_6);
    spell_assign(SPELL_HASTE, CLASS_PYROMANCER, CIRCLE_6);
    spell_assign(SPELL_MINOR_GLOBE, CLASS_PYROMANCER, CIRCLE_6);
    spell_assign(SPELL_STONE_SKIN, CLASS_PYROMANCER, CIRCLE_6);

    spell_assign(SPELL_ACID_BURST, CLASS_PYROMANCER, CIRCLE_7);

    spell_assign(SPELL_FLY, CLASS_PYROMANCER, CIRCLE_8);
    spell_assign(SPELL_MAJOR_GLOBE, CLASS_PYROMANCER, CIRCLE_8);
    spell_assign(SPELL_MELT, CLASS_PYROMANCER, CIRCLE_8);

    spell_assign(SPELL_CIRCLE_OF_FIRE, CLASS_PYROMANCER, CIRCLE_9);
    spell_assign(SPELL_IMMOLATE, CLASS_PYROMANCER, CIRCLE_9);
    spell_assign(SPELL_MASS_INVIS, CLASS_PYROMANCER, CIRCLE_9);
    spell_assign(SPELL_RELOCATE, CLASS_PYROMANCER, CIRCLE_9);

    spell_assign(SPELL_CREMATE, CLASS_PYROMANCER, CIRCLE_10);
    spell_assign(SPELL_NEGATE_HEAT, CLASS_PYROMANCER, CIRCLE_10);

    spell_assign(SPELL_METEORSWARM, CLASS_PYROMANCER, CIRCLE_11);

    spell_assign(SPELL_SUPERNOVA, CLASS_PYROMANCER, CIRCLE_12);

    /* RANGER */
    skill_assign(SKILL_BLUDGEONING, CLASS_RANGER, 1);
    skill_assign(SKILL_PIERCING, CLASS_RANGER, 1);
    skill_assign(SKILL_SLASHING, CLASS_RANGER, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_RANGER, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_RANGER, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_RANGER, 1);
    skill_assign(SKILL_KICK, CLASS_RANGER, 1);
    skill_assign(SKILL_TRACK, CLASS_RANGER, 1);
    skill_assign(SKILL_BASH, CLASS_RANGER, 1);
    skill_assign(SKILL_TAME, CLASS_RANGER, 1);
    skill_assign(SKILL_DUAL_WIELD, CLASS_RANGER, 1);
    skill_assign(SKILL_DODGE, CLASS_RANGER, 10);
    skill_assign(SKILL_SWITCH, CLASS_RANGER, 10);
    skill_assign(SKILL_RESCUE, CLASS_RANGER, 35);
    skill_assign(SKILL_PARRY, CLASS_RANGER, 30);
    skill_assign(SKILL_RIPOSTE, CLASS_RANGER, 40);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_RANGER, 60);

    spell_assign(SPELL_DETECT_ALIGN, CLASS_RANGER, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_RANGER, CIRCLE_1);
    spell_assign(SPELL_CURE_LIGHT, CLASS_RANGER, CIRCLE_2);
    spell_assign(SPELL_CHILL_TOUCH, CLASS_RANGER, CIRCLE_3);
    spell_assign(SPELL_BARKSKIN, CLASS_RANGER, CIRCLE_3);
    spell_assign(SPELL_CURE_SERIOUS, CLASS_RANGER, CIRCLE_4);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_RANGER, CIRCLE_5);
    spell_assign(SPELL_CURE_CRITIC, CLASS_RANGER, CIRCLE_6);
    spell_assign(SPELL_SENSE_LIFE, CLASS_RANGER, CIRCLE_7);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_RANGER, CIRCLE_8);
    spell_assign(SPELL_NATURES_GUIDANCE, CLASS_RANGER, CIRCLE_9);
    spell_assign(SPELL_BLUR, CLASS_RANGER, CIRCLE_11);

    /* ROGUE */
    skill_assign(SKILL_PIERCING, CLASS_ROGUE, 1);
    skill_assign(SKILL_SLASHING, CLASS_ROGUE, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_ROGUE, 1);
    skill_assign(SKILL_HIDE, CLASS_ROGUE, 1);
    skill_assign(SKILL_DODGE, CLASS_ROGUE, 1);
    skill_assign(SKILL_PICK_LOCK, CLASS_ROGUE, 6);
    skill_assign(SKILL_SNEAK, CLASS_ROGUE, 10);
    skill_assign(SKILL_BACKSTAB, CLASS_ROGUE, 10);
    skill_assign(SKILL_DUAL_WIELD, CLASS_ROGUE, 15);
    skill_assign(SKILL_EYE_GOUGE, CLASS_ROGUE, 15);
    skill_assign(SKILL_CONCEAL, CLASS_ROGUE, 25);
    skill_assign(SKILL_TRACK, CLASS_ROGUE, 30);
    skill_assign(SKILL_PARRY, CLASS_ROGUE, 40);
    skill_assign(SKILL_STEALTH, CLASS_ROGUE, 50);
    skill_assign(SKILL_CORNER, CLASS_ROGUE, 60);
    skill_assign(SKILL_SHADOW, CLASS_ROGUE, 60);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_ROGUE, 70);

    /* SHAMAN */
    skill_assign(SKILL_DODGE, CLASS_SHAMAN, 1);
    skill_assign(SKILL_BLUDGEONING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_PIERCING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_SLASHING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_SHAMAN, 1);
    skill_assign(SKILL_TAME, CLASS_SHAMAN, 7);

    spell_assign(SPELL_CURE_LIGHT, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_ARMOR, CLASS_SHAMAN, CIRCLE_2);
    spell_assign(SPELL_CREATE_FOOD, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_CREATE_WATER, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_DETECT_POISON, CLASS_SHAMAN, CIRCLE_1);
    spell_assign(SPELL_DETECT_ALIGN, CLASS_SHAMAN, CIRCLE_2);
    spell_assign(SPELL_CURE_BLIND, CLASS_SHAMAN, CIRCLE_4);
    spell_assign(SPELL_BLESS, CLASS_SHAMAN, CIRCLE_2);
    spell_assign(SPELL_BLINDNESS, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_PROT_FROM_EVIL, CLASS_SHAMAN, CIRCLE_3);
    spell_assign(SPELL_CURE_CRITIC, CLASS_SHAMAN, CIRCLE_3);
    spell_assign(SPELL_SUMMON, CLASS_SHAMAN, CIRCLE_5);
    spell_assign(SPELL_REMOVE_POISON, CLASS_SHAMAN, CIRCLE_3);
    spell_assign(SPELL_WORD_OF_RECALL, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_EARTHQUAKE, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_DISPEL_EVIL, CLASS_SHAMAN, CIRCLE_4);
    spell_assign(SPELL_DISPEL_GOOD, CLASS_SHAMAN, CIRCLE_4);
    spell_assign(SPELL_HEAL, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_HARM, CLASS_SHAMAN, CIRCLE_5);
    spell_assign(SPELL_GROUP_HEAL, CLASS_SHAMAN, CIRCLE_10);
    spell_assign(SPELL_REMOVE_CURSE, CLASS_SHAMAN, CIRCLE_5);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_SHAMAN, CIRCLE_7);
    spell_assign(SPELL_FIREBALL, CLASS_SHAMAN, CIRCLE_8);
    spell_assign(SPELL_DETECT_INVIS, CLASS_SHAMAN, CIRCLE_7);
    spell_assign(SPELL_HASTE, CLASS_SHAMAN, CIRCLE_7);
    spell_assign(SPELL_VITALITY, CLASS_SHAMAN, CIRCLE_6);
    spell_assign(SPELL_DARKNESS, CLASS_SHAMAN, CIRCLE_6);

    /* SORCERER */
    spell_assign(SPELL_BURNING_HANDS, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_MINOR_CREATION, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_MAGIC_MISSILE, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_DETECT_MAGIC, CLASS_SORCERER, CIRCLE_1);
    spell_assign(SPELL_CONCEALMENT, CLASS_SORCERER, CIRCLE_1);

    spell_assign(SPELL_CHILL_TOUCH, CLASS_SORCERER, CIRCLE_2);
    spell_assign(SPELL_STRENGTH, CLASS_SORCERER, CIRCLE_2);
    spell_assign(SPELL_DETECT_INVIS, CLASS_SORCERER, CIRCLE_2);

    spell_assign(SPELL_IDENTIFY, CLASS_SORCERER, CIRCLE_3);
    spell_assign(SPELL_DISPEL_MAGIC, CLASS_SORCERER, CIRCLE_3);
    spell_assign(SPELL_LOCATE_OBJECT, CLASS_SORCERER, CIRCLE_3);
    spell_assign(SPELL_SHOCKING_GRASP, CLASS_SORCERER, CIRCLE_3);

    spell_assign(SPELL_TELEPORT, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_WORLD_TELEPORT, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_MINOR_PARALYSIS, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_RAY_OF_ENFEEB, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_LEVITATE, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_INFRAVISION, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_LIGHTNING_BOLT, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_MINOR_GLOBE, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_COLDSHIELD, CLASS_SORCERER, CIRCLE_4);
    spell_assign(SPELL_FIRESHIELD, CLASS_SORCERER, CIRCLE_4);

    spell_assign(SPELL_FARSEE, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_COLOR_SPRAY, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_DIMENSION_DOOR, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_CONE_OF_COLD, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_SLEEP, CLASS_SORCERER, CIRCLE_5);
    spell_assign(SPELL_INVISIBLE, CLASS_SORCERER, CIRCLE_5);

    spell_assign(SPELL_FIREBALL, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_HASTE, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_ICE_STORM, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_FLY, CLASS_SORCERER, CIRCLE_6);
    spell_assign(SPELL_STONE_SKIN, CLASS_SORCERER, CIRCLE_6);

    spell_assign(SPELL_BIGBYS_CLENCHED_FIST, CLASS_SORCERER, CIRCLE_7);

    spell_assign(SPELL_CHAIN_LIGHTNING, CLASS_SORCERER, CIRCLE_8);
    spell_assign(SPELL_HARNESS, CLASS_SORCERER, CIRCLE_8);
    spell_assign(SPELL_MAJOR_GLOBE, CLASS_SORCERER, CIRCLE_8);

    spell_assign(SPELL_RELOCATE, CLASS_SORCERER, CIRCLE_9);
    spell_assign(SPELL_MASS_INVIS, CLASS_SORCERER, CIRCLE_9);
    spell_assign(SPELL_DISINTEGRATE, CLASS_SORCERER, CIRCLE_9);

    spell_assign(SPELL_METEORSWARM, CLASS_SORCERER, CIRCLE_10);

    spell_assign(SPELL_REDUCE, CLASS_SORCERER, CIRCLE_11);
    spell_assign(SPELL_ENLARGE, CLASS_SORCERER, CIRCLE_11);
    spell_assign(SPELL_WIZARD_EYE, CLASS_SORCERER, CIRCLE_11);

    spell_assign(SPELL_CHARM, CLASS_SORCERER, CIRCLE_12);

    /* THIEF */
    skill_assign(SKILL_PIERCING, CLASS_THIEF, 1);
    skill_assign(SKILL_SLASHING, CLASS_THIEF, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_THIEF, 1);
    skill_assign(SKILL_SNEAK, CLASS_THIEF, 1);
    skill_assign(SKILL_BACKSTAB, CLASS_THIEF, 1);
    skill_assign(SKILL_HIDE, CLASS_THIEF, 1);
    skill_assign(SKILL_DODGE, CLASS_THIEF, 1);
    skill_assign(SKILL_PICK_LOCK, CLASS_THIEF, 6);
    skill_assign(SKILL_STEAL, CLASS_THIEF, 6);
    skill_assign(SKILL_CONCEAL, CLASS_THIEF, 10);
    skill_assign(SKILL_DUAL_WIELD, CLASS_THIEF, 15);
    skill_assign(SKILL_PARRY, CLASS_THIEF, 30);
    skill_assign(SKILL_TRACK, CLASS_THIEF, 40);
    skill_assign(SKILL_STEALTH, CLASS_THIEF, 50);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_THIEF, 75);

    /* WARRIOR */
    skill_assign(SKILL_BLUDGEONING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_PIERCING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_SLASHING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_2H_BLUDGEONING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_2H_PIERCING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_2H_SLASHING, CLASS_WARRIOR, 1);
    skill_assign(SKILL_KICK, CLASS_WARRIOR, 1);
    skill_assign(SKILL_BASH, CLASS_WARRIOR, 1);
    skill_assign(SKILL_DODGE, CLASS_WARRIOR, 1);
    skill_assign(SKILL_SWITCH, CLASS_WARRIOR, 10);
    skill_assign(SKILL_RESCUE, CLASS_WARRIOR, 15);
    skill_assign(SKILL_DISARM, CLASS_WARRIOR, 20);
    skill_assign(SKILL_GUARD, CLASS_WARRIOR, 25);
    skill_assign(SKILL_DUAL_WIELD, CLASS_WARRIOR, 25);
    skill_assign(SKILL_PARRY, CLASS_WARRIOR, 30);
    skill_assign(SKILL_DOUBLE_ATTACK, CLASS_WARRIOR, 35);
    skill_assign(SKILL_RIPOSTE, CLASS_WARRIOR, 40);
    skill_assign(SKILL_HITALL, CLASS_WARRIOR, 50);
    skill_assign(SKILL_RETREAT, CLASS_WARRIOR, 60);
}

/* convert_class does no checking.  It expects a valid class and ch.
 *
 * This function changes a player's class and converts the skills/spells
 * accordingly, keeping the old values if they are better.
 * It also transfers quest spells. */
void convert_class(struct char_data *ch, int newclass) {
    int skill;
    sh_int old_skills[TOP_SKILL + 1];
    sh_int new_skills[TOP_SKILL + 1];

    /* read in the player's old skills */
    for (skill = 0; skill <= TOP_SKILL; skill++) {
        old_skills[skill] = GET_ISKILL(ch, skill);
    }
    /* set class */
    GET_CLASS(ch) = newclass;

    /* Big changes occur here: */
    update_char(ch);

    /* read the new skills */
    for (skill = 0; skill < MAX_SKILLS + 1; skill++) {
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

int getbaseclass(int class) {
    if (classes[class].is_subclass)
        return classes[class].subclass_of;
    return class;
}

/***************************************************************************
 * $Log: class.c,v $
 * Revision 1.176  2009/07/18 01:17:23  myc
 * Adding decay, iron maiden, spinechiller, and bone draw to necromancer
 * spell list.  Removing shocking grasp and lightning bolt.
 *
 * Revision 1.175  2009/06/14 18:11:10  myc
 * Fix innate effect for paladins.
 *
 * Revision 1.174  2009/06/09 21:48:21  myc
 * Remove usage of global buffer.
 *
 * Revision 1.173  2009/06/09 19:33:50  myc
 * Passing advance_level an enum for which action it should taking
 * rather than using a boolean, in order to reduce confusion.
 * Also took out log messages from advance_level and put them in
 * gain_exp.
 *
 * Revision 1.172  2009/06/09 05:35:39  myc
 * Modified advance_level to work with the new clan interface.
 *
 * Revision 1.171  2009/03/20 16:06:04  jps
 * Removed spells of lesser/greater invocation.
 *
 * Revision 1.170  2009/03/20 06:08:18  myc
 * Adding waterwalk to cryomancers, circle 1.  Adding detonation,
 * phosphoric embers, positive field, and acid burst to pyromancers,
 * but removing heatwave and combust.  Also alphabetized circles in
 * pyromancer spell assignment list.
 *
 * Revision 1.169  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.168  2009/03/04 08:59:55  jps
 * Fix some comments.
 *
 * Revision 1.167  2008/09/20 17:43:26  jps
 * Gave dark presence spell to antipaladins at circle 2.
 *
 * Revision 1.166  2008/09/20 08:02:27  jps
 * Removed comprehend language spell.
 *
 * Revision 1.165  2008/09/12 20:16:23  jps
 * Removed vaporform from cryomancers.
 *
 * Revision 1.164  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.163  2008/09/01 00:49:07  mud
 * The "max skill" of a spell is always 1000, since you either have it or you
 *don't.
 *
 * Revision 1.162  2008/09/01 00:12:37  jps
 * Assign charm person to illusionists at circle 3.
 *
 * Revision 1.161  2008/08/10 06:54:33  jps
 * Added illusionist guild guards and guild masters.
 *
 * Revision 1.160  2008/08/10 01:58:49  jps
 * Added spells severance and soul reaver for illusionists.
 *
 * Revision 1.159  2008/07/27 05:23:01  jps
 * Changed save_player to save_player_char, since it only saves the character.
 *
 * Revision 1.158  2008/05/22 15:40:25  myc
 * Tweaked berserker skill assignments.
 *
 * Revision 1.157  2008/05/19 05:47:31  jps
 * Remove sane mind from illusionists, and assign mesmerize at circle 3.
 *
 * Revision 1.156  2008/05/18 22:53:54  jps
 * Assigning hysteria spell to illusionists at circle 7.
 *
 * Revision 1.155  2008/05/18 17:59:03  jps
 * Assigning familiarity to illusionists at circle 9.
 *
 * Revision 1.154  2008/05/18 02:02:53  jps
 * Assigning isolation to illusionists in circle 6.
 *
 * Revision 1.153  2008/05/12 00:43:16  jps
 * Add nightmare and discorporate spells to illusionists.
 *
 * Revision 1.152  2008/05/11 05:41:49  jps
 * Using regen.h.
 *
 * Revision 1.151  2008/04/26 23:35:43  myc
 * Info about permanent effects and race skills are stored in the
 * class/race structs now, but need to be initialized at runtime
 * by the init_races and init_classes functions.
 *
 * Revision 1.150  2008/04/26 18:56:36  myc
 * Assigning Berserker skills/chants.
 *
 * Revision 1.149  2008/04/15 04:44:42  jps
 * Give dispel magic to illusionists at circle 3.
 *
 * Revision 1.148  2008/04/14 08:39:56  jps
 * Added illusory wall to illusionists in circle 8.
 *
 * Revision 1.147  2008/04/14 02:18:25  jps
 * Assigning glory to illusionists in circle 11.
 *
 * Revision 1.146  2008/04/13 18:52:18  jps
 * Actually, confusion is supposed to be in circle 4 for illusionists.
 *
 * Revision 1.145  2008/04/13 18:30:30  jps
 * Assign spell of confusion to illusionists at circle 5.
 *
 * Revision 1.144  2008/04/04 06:12:52  myc
 * Removed dieites/worship code.
 *
 * Revision 1.143  2008/03/30 15:37:36  jps
 * Fix spelling of riposte.
 *
 * Revision 1.142  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.141  2008/03/11 19:50:55  myc
 * Get rid of practice points.
 *
 * Revision 1.140  2008/03/10 18:01:17  myc
 * Adding berserker to guild info.  Adding new berserker skills and
 * chants to skill_assignments.
 *
 * Revision 1.139  2008/03/09 18:12:41  jps
 * Assigned spell of misdirection to illusionists in circle 4.
 *
 * Revision 1.138  2008/03/09 09:00:05  jps
 * Assign spell of fear to illusionists.
 *
 * Revision 1.137  2008/03/05 03:03:54  myc
 * The advance_level function doesn't require save_mem_list anymore
 * due to the elimination of redundant spell memory structures in
 * the player structure due to ascii pfiles.
 *
 * Revision 1.136  2008/02/23 01:03:54  myc
 * Cleaning up advance_level a lot.  Also reworked assign_class_skills
 * to use skill/spell/chant_assign instead of spell_level.
 *
 * Revision 1.135  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.134  2008/02/09 03:04:23  myc
 * A few spell_level calls for illusionist were causing boot-time
 * error messages.
 *
 * Revision 1.133  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.132  2008/01/27 21:09:12  myc
 * Giving berserkers meditate skill
 *
 * Revision 1.131  2008/01/27 12:10:29  jps
 * Added regen and exp-cost factors.
 *
 * Revision 1.130  2008/01/27 11:16:43  jps
 * Handle newbie eq assignment here.
 *
 * Revision 1.129  2008/01/27 09:44:12  jps
 * Add a plain-but-capitalized name field. Add several factors that are used
 * when creating mob prototypes.
 *
 * Revision 1.128  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.127  2008/01/07 11:55:48  jps
 * Added phantasm and simulacrum spells to illusionist class.
 *
 * Revision 1.126  2008/01/06 23:50:47  jps
 * Added spells project and simulacrum, and MOB2_ILLUSORY flag.
 *
 * Revision 1.125  2008/01/06 20:38:57  jps
 * Change illusionist spells.  Add ventriloquate to illusionists.
 *
 * Revision 1.124  2008/01/06 17:34:41  jps
 * Move thac0 values into classdef array.
 *
 * Revision 1.123  2008/01/06 05:36:37  jps
 * Add equipment restriction flags to classdef.
 * Clean up advance_level a bit.
 *
 * Revision 1.122  2008/01/05 23:05:45  jps
 * Added hp per level value to classdef.
 *
 * Revision 1.121  2008/01/05 21:58:27  jps
 * Added saving throw data to classdef structs.
 * Moved a lot of crap to chars.c.
 *
 * Revision 1.120  2008/01/05 20:31:52  jps
 * Added data to the classdef struct which controls how newly-generated
 * stats are ordered in a character.
 *
 * Revision 1.119  2008/01/05 05:33:22  jps
 * Started updating some Illusionist stuff.  Added init_char_class() and
 * update_char_class() functions.  Moved a lot of race-related stuff to races.c.
 * Moved some skill-related stuff to spells.c.  Removed redundant skill
 *assignments.
 *
 * Revision 1.118  2008/01/04 01:50:41  jps
 * Added races.h file.  Added homeroom to class definitions.  Removed
 * obsolete "practice" data.
 *
 * Revision 1.117  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.116  2008/01/02 01:26:51  jps
 * Made the assignment of sphere-related skills automatic.  They will be
 * assigned to a character if and only if there's a spell from that sphere
 * available to the character.
 *
 * Revision 1.115  2007/12/19 20:44:27  myc
 * advance_level() automatically modifies clan power and boots a player
 * from a clan if they are advanced to clan god status.  save_player()
 * no longer requires a save room (which wasn't being used anyway).
 *
 * Revision 1.114  2007/10/13 05:07:24  myc
 * Added new monk chants.
 *
 * Revision 1.113  2007/10/11 20:14:48  myc
 * Changed the skill defines slightly to support chants and songs as
 * slightly distinguished from spells and skills.  TOP_SKILL is the
 * old MAX_SKILLS.
 * Implemented monk chants using the magic system.  Added skill sets
 * to monks.
 *
 * Revision 1.112  2007/09/21 08:02:33  jps
 * Adjust direction of Mielikki priest guild guard.
 *
 * Revision 1.111  2007/09/11 16:34:24  myc
 * Added a roll_skill function that takes the level skill limits into
 * account.  It also takes up far less clock cyrcles than roll_mob_skill,
 * but the latter remains because it's used a loooot.
 *
 * Revision 1.110  2007/09/08 06:03:03  jps
 * Remove "plane shift" spell from various classes.
 *
 * Revision 1.109  2007/09/08 05:18:49  jps
 * Remove summon dracolich spell from necromancers.
 *
 * Revision 1.108  2007/09/07 18:56:04  jps
 * Thieves get double attack at level 75.
 * Mercenaries get dual wield at level 15.
 *
 * Revision 1.107  2007/09/04 06:49:19  myc
 * Re-assigning control weather spell to druids.
 *
 * Revision 1.106  2007/08/28 20:18:35  myc
 * Rangers get AFF_FARSEE innate now, instead of special-casing the ranger
 * class in do_farsee.
 *
 * Revision 1.105  2007/08/26 21:33:36  jps
 * Add level_max_skill() so you can see what the max skill is for
 * someone at a particular level - especially 99.
 *
 * Revision 1.104  2007/08/15 06:07:52  myc
 * Making conceal level 10 for thieves.
 *
 * Revision 1.103  2007/08/14 22:43:07  myc
 * Adding corner, conceal, stealth, and shadow skills.
 *
 * Revision 1.102  2007/08/05 22:19:17  myc
 * Fixed up springleap skill for monks.
 *
 * Revision 1.101  2007/08/05 20:21:51  myc
 * Added retreat and group retreat skills.
 *
 * Revision 1.100  2007/08/02 04:19:04  jps
 * Added "moonbeam" spell for Druids.
 *
 * Revision 1.99  2007/07/31 07:37:51  jps
 * Brought back the locate object spell.
 * Simplified update_skills().
 * Remove innate effects prior to applying them, so that any you had from
 * the prior race won't linger when you change to a new race.
 *
 * Revision 1.98  2007/07/04 02:21:58  myc
 * Renamed douse spell to extinguish.
 *
 * Revision 1.97  2007/06/16 00:32:55  myc
 * Removing 'circle' skill from assassins' skill lists.
 *
 * Revision 1.96  2007/06/16 00:15:49  myc
 * Three spells for necromancers: soul tap, rebuke undead,
 * and degeneration.  One spell for rangers: natures guidance.
 *
 * Revision 1.95  2007/05/29 20:16:32  jps
 * Abstracted getting base class.
 *
 * Revision 1.94  2007/05/28 22:36:26  jps
 * Reduce the <base-class>_subclass arrays to the subclasses that are live.
 *
 * Revision 1.93  2007/05/28 06:34:01  jps
 * Fix guild guard blocking to actually allow the right classes and subclasses
 * into the various guilds.  Looks like someone fixed this situation in the
 * mud simply by not loading the guards - once this change goes live, the
 * guards can be loaded again.  And once again, only warriors and thier
 * subclasses will be able to see the warrior coach (same goes for clerics
 * and sorcerers).
 *
 * Revision 1.92  2007/05/11 21:03:12  myc
 * New rogue skill, eye gouge, allows rogues to gouge out eyes.  A very
 * complicated skill.  :P  Fixed cure blind's logic, and made it support
 * eye gouge too.
 *
 * Revision 1.91  2007/05/11 20:13:28  myc
 * Vaporform is a new circle 13 spell for cryomancers.  It significantly
 * increases the caster's chance of dodging a hit.  It is a quest spell.
 *
 * Revision 1.90  2007/04/11 22:14:40  jps
 * Disabled "control weather" spell from druids, since it does nothing.
 * No mortals should have access to it now.
 *
 * Revision 1.89  2007/03/07 23:10:02  jps
 * Changed Anduin mercenary guild entrance direction to up.
 *
 * Revision 1.88  2007/02/14 03:54:53  myc
 * Changed base saves.  Sorcerers get harness circle 8.
 *
 * Revision 1.87  2007/01/25 17:05:51  myc
 * Diabolists won't forget spells when they level gain.
 *
 * Revision 1.86  2006/11/26 08:31:17  jps
 * Typo fixes for character creation process.
 *
 * Revision 1.85  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.84  2006/11/08 09:16:04  jps
 * Fixed some loose-lose typos.
 *
 * Revision 1.83  2006/11/08 07:55:17  jps
 * Change verbal instances of "breath" to "breathe"
 *
 * Revision 1.82  2006/10/12 01:35:47  dce
 * Minor fix for Ogakh guild guards.
 *
 * Revision 1.81  2006/10/07 02:07:07  dce
 * Updated for guild guards in Ogakh.
 *
 * Revision 1.80  2006/05/30 01:25:13  rls
 * Gave poison back to antis and necro/druids/clerics/diabs
 * also made sure some classes had rem poison, like pali, etc.
 * as well, set innate prot_good/evil on anti/pali
 *
 * Revision 1.79  2005/06/15 17:21:43  cjd
 * Adjust checks to make certain monks skills mastered on subclas
 * and make dragon innates stay on dragons with a designated class
 *
 * Revision 1.78  2004/11/11 23:24:06  rsd
 * split up and edited the subclass_description char star to
 * make it smaller so the compiler would stop complaining
 * about it being more than 509 bytes.
 *
 * Revision 1.77  2003/06/21 01:01:08  jjl
 * Modified rogues.  Removed circle - backstab is now circlicious.  Updated
 * damage on backstab to give a little more pop.  Throatcut is now a once a day.
 *
 * Revision 1.76  2003/01/26 22:08:54  jjl
 * Gave rangers cure serious, since they have light and crit.
 *
 * Revision 1.75  2002/10/14 02:16:08  jjl
 * An update to turn vitality into a set of 6 spells, lesser endurance,
 * endurance, greater endurance, vitality, greater vitality, and dragon's
 * health.  Greater endurance is what vitality was.  The rest are scaled
 * appropriately.    The higher end may need scaled down, or may not.
 *
 * Revision 1.74  2002/09/21 03:15:32  jjl
 * Fixed the backstab multiplier spread so it's now actually based on
 * 100 levels.  The lowbie-midbie backstabby folk probably aren't going to like
 *it.
 *
 * Revision 1.73  2002/09/15 03:55:51  jjl
 * Added skills for summon corpse and shift corpse
 *
 * Revision 1.72  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.71  2002/07/16 23:21:53  rls
 * Added in new necro spell, bone armor.. and gave them ident
 *
 * Revision 1.70  2002/06/06 00:36:09  rls
 * Adjusted THAC0s for each class (to be a bit more sane *hope*)
 *
 * Revision 1.69  2002/06/04 02:24:30  dce
 * Adjusted thac0's for initial testing.
 *
 * Revision 1.68  2002/05/23 00:20:26  rls
 * Added thac0 min-max table for each of the classes (thac0 Revamp)
 *
 * Revision 1.67  2001/12/16 20:07:53  dce
 * Fixed a problem where players could get the max dam and hitroll
 * possible by changing their alignment and getting "zapped" by
 * alignment sensitive equipment. The "zapped" they would not lose
 * the +hitroll or damroll effects because of silly Hubis code.
 *
 * Revision 1.66  2001/12/07 15:42:30  dce
 * Fixed a bug with object spell effects where if a player
 * was wearing an item and died, they would permanently
 * gain that ability.
 *
 * Revision 1.65  2001/03/31 19:58:34  dce
 * Player innates and quest spells should stick.
 *
 * Revision 1.64  2001/02/02 00:34:21  mtp
 * fixed bad room def for 10014
 *
 * Revision 1.63  2001/01/16 00:33:56  mtp
 * make sure spell/skill list is clean after subclass
 *
 * Revision 1.62  2001/01/08 00:39:18  rsd
 * added the proper direction for the mercenary guild in mielikki
 *
 * Revision 1.61  2001/01/04 22:49:24  mtp
 * added guard 6175 as a guild guard in 6148
 *
 * Revision 1.60  2000/12/15 02:18:46  rsd
 * fixed the spelling of Sorcerer in a freaking class
 * array, there is no telling what relies on this misspelling.
 *
 * Revision 1.59  2000/11/29 00:19:24  mtp
 * checking guild guards for ickle/anduin/mielikki
 *
 * Revision 1.58  2000/11/21 04:31:04  rsd
 * Well, Rangers didn't have quick chant and only today
 * someone pointed it out, sheez
 *
 * Revision 1.57  2000/11/20 19:17:22  rsd
 * Added back rlog messages from prior to the addition of
 * the $log$.
 *
 * Revision 1.56  2000/11/14 20:24:24  rsd
 * added it so half elfs could be priests.. boggle
 *
 * Revision 1.55  2000/11/13 23:46:45  rsd
 * Added tame for paladins and anti's properly.
 *
 * Revision 1.54  2000/11/12 23:53:35  rsd
 * Added more debug into existing debug messages to make
 * them useful.
 *
 * Revision 1.53  2000/11/12 08:04:53  rsd
 * added more debug to set_skills() default case in the
 * switch. Unknown Class
 *
 * Revision 1.52  2000/11/12 07:28:57  rsd
 * removed the number 50 from the spell_level() circle field.
 * Rangers Paladins and AntiPaladins were assigned wrong
 *
 * Revision 1.51  2000/11/07 01:32:05  mtp
 * moved CLASS_MERCENARY from warrior subclass to rogue
 *
 * Revision 1.50  2000/09/15 17:03:00  jimmy
 * added function racial_innates() that prevents the wiping out of innate
 * skills by update_skills() when a new player is created or a player levels.
 * Added BODYSLAM to set_skills() to the races that were supposed to
 * have it.
 *
 * Revision 1.49  2000/09/04 19:58:35  rsd
 * Gave innate detect alignment to Priests and Diabs.
 *
 * Revision 1.48  2000/05/14 05:21:49  rsd
 * let half-elfs be thiefs
 *
 * Revision 1.47  2000/04/23 08:38:32  rsd
 * Retabbed and braced sections of the code, also commented out
 * add_move from the player advancement functions.  I never wanted
 * players to gain mv when the leveled in the first place. Paladins
 * have 3000 - 4000 mv at high level, found out someone had typoed
 * number(1,32) into the add_move variable as opposed to
 * number(1,3) so paladins were crankin on MV.  Smells like PWIPE
 * to me. Also fixed the citadel guardian problem, I hope.
 *
 * Revision 1.46  2000/04/23 03:23:48  rsd
 * ok I really removed weapon proficiencies from classes that didn't
 * need them, no body saw nuthiun'
 *
 * Revision 1.45  2000/04/22 22:34:01  rsd
 * Fixed deity spelling in player output. Added a guarding
 * direction for the entrance of Timun's citadel. Removed
 * weapons proficiencies from classes to set them all to
 * the proper proficiencies. Set Paladins and Anti-Paladins
 * to have detect alignemnt innately. Would have prefered
 * detect evil and good accordingly but they don't seem to
 * exist.
 *
 * Revision 1.44  2000/04/15 23:11:33  rsd
 * moved some spells between classes and levels.
 *
 * Revision 1.43  2000/04/05 06:31:31  rsd
 * changed the comment deader, moved flamestrike to lower
 * circle and removed color spay from pyros
 *
 * Revision 1.42  2000/02/14 05:12:44  cso
 * added guildguards for anduin and khuzhadam, added berserker to
 * class comment immediately after the other stuff.
 *
 * Revision 1.41  2000/01/31 00:35:20  rsd
 * removed the spell poison from the game for players. Removed
 * it from anti paladins.
 *
 * Revision 1.40  1999/11/28 22:53:27  cso
 * removed unused choice_table arg from roll_natural_abils
 *
 * Revision 1.39  1999/11/23 17:56:25  rsd
 * fixed guild guards exit points so same guard can block multiple exits.
 *
 * Revision 1.38  1999/11/23 16:09:19  rsd
 * added the other mielikki guild masters
 *
 * Revision 1.37  1999/11/23 15:48:23  jimmy
 * Fixed the slashing weapon skill.  I had it erroneously as stabbing. Doh.
 * Reinstated dual wield.
 * Allowed mobs/players to pick up items while fighting.
 * Fixed a bug in the damage message that wrongfully indicated a miss
 * due to a rounding error in the math.
 * This was all done in order to facilitate the chance to sling your
 * weapon in combat.  Dex and proficiency checks are now made on any missed
 * attact and a failure of both causes the weapon to be slung.
 *
 * Revision 1.35  1999/11/19 05:12:55  cso
 * Added buncha anduin guildguards :)
 *
 * Revision 1.34  1999/11/19 04:32:51  rsd
 * Well now,
 * Added class guild guard room assignemtns for Mielikki and
 * Ickle, I suspect Anduin will soon follow.
 * Added spell_level(SKILL_SPHERE..) for all casting classes.
 * Added SET_SKILL(ch, SKILL_SPHERE..) for players and MOBS!
 * WOO!
 *
 * Revision 1.33  1999/11/17 20:03:20  jimmy
 * reformatted return_max_skill and changed the equation to
 * 10*LEVEL + 50 as the max skill
 * --gurlaek
 *
 * Revision 1.32  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.31  1999/08/20 15:58:42  mud
 * commented out dual wield from each class until
 * such time that the use of it is more blanced
 *
 * Revision 1.30  1999/08/18 18:57:16  mud
 * made double attack level 90 for thieves
 *
 * Revision 1.29  1999/08/15 19:42:04  mud
 * made double attack level 65 for assassins
 *
 * Revision 1.28  1999/08/15 19:21:39  mud
 * made double attack level 70 for rogues
 *
 * Revision 1.27  1999/08/12 20:42:01  dce
 * Level 99's can now reach **.
 *
 * Revision 1.26  1999/07/30 05:08:34  mud
 * Added rescue for rangers at level 35
 *
 * Revision 1.25  1999/07/22 17:43:59  jimmy
 * Gave the newly reimpleminted spell IDENTIFY to magic users and conjurers
 * at 3rd circle
 * --gurlaek
 *
 * Revision 1.24  1999/07/20 19:45:51  jimmy
 * This is the spanky New Spell recognition code.
 * This code allows mobs/players that have the KNOW_SPELL skill
 * to make a skill check to guess the spell.  A good roll will show both
 * the spell and the target.  A bad roll will show the spell garbled and
 * then an INT check for the target.  If a really bad roll is made, the spell
 * will be replaced by an incorrect one.  the heart of this system is
 * start_chant(), end_chant, and bad_guess().
 * --gurlaek 7/20/1999
 *
 * Revision 1.23  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their
 *race/class/level that exactly align with PC's.  PC's no longer have to rent to
 *use skills gained by leveling or when first creating a char.  Languages no
 *longer reset to defaults when a PC levels.  Discovered that languages have
 *been defined right in the middle of the spell area.  This needs to be fixed.
 *A conversion util neeDs to be run on the mob files to compensate for the 13 to
 *-1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.22  1999/06/30 18:25:04  jimmy
 * >> This is a major conversion from the 18 point attribute system to the
 * >> 100 point attribute system.  A few of the major changes are:
 * >> All attributes are now on a scale from 0-100
 * >> Everyone views attribs the same but, the attribs for one race
 * >>   may be differeent for that of another even if they are the
 * >>   same number.
 * >> Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * >> Mobs now have individual random attributes based on race/class.
 * >> The STR_ADD attrib has been completely removed.
 * >> All bonus tables for attribs in constants.c have been replaced by
 * >>   algorithims that closely duplicate the tables except on a 100 scale.
 * >> Some minor changes:
 * >> Race selection at char creation can now be toggled by using
 * >>   <world races off>
 * >> Lots of cleanup done to affected areas of code.
 * >> Setting attributes for mobs in the .mob file no longer functions
 * >>   but is still in the code for later use.
 * >> We now have a spare attribut structure in the pfile because the new
 * >>   system only used three instead of four.
 * >> --gurlaek 6/30/1999
 *
 * Revision 1.21  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list. This code compiles fine under both gcc RH5.2 and egcs RH6.0.
 * --Gurlaek 6/10/1999
 *
 * Revision 1.20  1999/05/26 02:11:22  mud
 * added SET_SKILL(ch, SKILL_SUMMON_MOUNT, 1000) and
 * spell_level(SKILL_SUMMON_MOUNT, CLASS_PALADIN, 15, 2)
 * for both paladin and anti's
 *
 * Revision 1.19  1999/04/29 19:00:02  mud
 * removed locate object from spell lists because it's
 * crashing the mud.
 *
 * Revision 1.18  1999/04/21 04:11:02  jimmy
 * fixed a crashbug related to display_class[] and parse_class().  someone
 * removed some classes from the menu but didn't match everything up.
 * I replaced the removed classes with blank entries so that do_help
 * for that class now works on the login menu. SCREAM
 * --gurlaek
 *
 * Revision 1.17  1999/04/16 00:50:23  mud
 * added preserve to necro's at 2nd circle.
 *
 * Revision 1.16  1999/03/24 23:30:59  mud
 * added/moved some spells for conjurers.
 * Commented out all quest spells per Fingon's
 * instructions.
 *
 * Revision 1.15  1999/03/21 16:36:55  mud
 * Altered who gets what spell and at what level, also
 * bumped up anti/pally hosemanship and set several spells
 * to skill 0 to make them quest spells.
 *
 * Revision 1.14  1999/03/20 18:54:39  tph
 * removed mystic, hunter and illusionist from parse_class().
 *
 * Revision 1.13  1999/03/15 04:44:42  mud
 * Ok, In line 17 I added something to make Fingon crazy
 * Ok, seriously I did rearrange and remove/add skills from
 * the various classes.  The purpose of this was to verify
 * the sound arrangement of the skills and how they were
 * dispursed among the classes.  This was accomplished by
 * altering SET_SKILL and spell_level fields where applicable.
 *
 * Revision 1.12  1999/03/14 00:53:03  mud
 * In class.c added a new line before the fiery mud class explanation
 * in config.c added the variable for name explanations and added the
 * text for the variable
 * in interpreter.c added the con_state stuff, whatever that was and
 * added the CON_NAME_CHECK affirmation section to the creation menu
 * loop or nanny.
 * In structs.h added the CON_NAME_CHECK define..
 * I also drove Jimmy absolutely insane with the deail in information
 * I put into our change control system.
 *
 * Revision 1.11  1999/03/11 23:37:12  mud
 * made Paladin and tla bright white instead of grey
 *
 * Revision 1.10  1999/03/10 00:03:37  dce
 * Monk semantics for dodge/parry/ripost/attack
 *
 * Revision 1.9  1999/03/09 22:26:35  mud
 * added scribe for rangers and removed it from paladins
 * antis and other classes that dfidn't need it.
 *
 * Revision 1.8  1999/03/08 23:24:48  dce
 * Added Springleap for monks
 *
 * Revision 1.7  1999/03/08 20:29:34  dce
 * Adds safefall skill for monks.
 *
 * Revision 1.6  1999/03/03 20:11:02  jimmy
 * Many enhancements to scribe and spellbooks.  Lots of checks added.
 * Scribe is now a skill.
 * Spellbooks now have to be held to scribe as well as a quill in the other
 *hand.
 *
 * -fingon
 *
 * Revision 1.5  1999/02/26 22:30:30  dce
 * Monk additions/fixes
 *
 * Revision 1.4  1999/02/03 18:14:16  jimmy
 * Fixed yet more bugs with spellcasters and leveling.
 * Hope this does it.
 *
 * Revision 1.3  1999/02/02 20:16:54  jimmy
 * all spellcasters now attain spells when they level.
 * no renting and returning necessary.
 *
 * Revision 1.2  1999/01/30 04:16:40  mud
 * Entire file indented with emcacs
 * Changed *subclass_descrip to be Fieryified
 * changed *class_abbrevs[] to be three letter abbreviations
 * Removed Shaman as a class choice for humans
 * Added some question about *class_menu in a comment
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

/***************************************************************************
 *  File: objects.h                                       Part of FieryMUD *
 *  Usage: header file for objects                                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "effects.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define IS_CORPSE(obj) (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) != NOT_CORPSE)
#define IS_PLR_CORPSE(obj)                                                                                             \
    (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) == CORPSE_PC)

/*[ STRUCTURES ]**************************************************************/

struct ObjectTypeDef {
    const char *name;
    const char *desc;
    struct {
        int min;
        int max;
    } value[NUM_VALUES];
};

struct LiquidDef {
    const char *alias;
    const char *name;
    const char *color_desc;
    int condition_effects[3];
};

/* object flags; used in ObjData */
struct ObjectFlagData {
    int value[NUM_VALUES]; /* Values of the item (see list)  */
    byte type_flag;        /* Type of item                     */
    long int wear_flags;   /* Where you can wear it            */
    /* If it hums, glows, etc.          */
    flagvector extra_flags[FLAGVECTOR_SIZE(NUM_ITEM_FLAGS)];
    float weight;           /* Weight what else                 */
    float effective_weight; /* Weight of contents + weight */
    int cost;               /* Value when sold (gp.)            */
    int level;              /* Level of the object              */
    int timer;              /* Timer for object                 */
    int decomp;             /* Decomposition timer              */
    /* Object Spell effects             */
    flagvector effect_flags[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
    long hiddenness; /* How difficult it is to see obj   */
};

struct SpellBookList {
    int spell;
    int length;
    SpellBookList *next;
};

struct ObjectApplyType {
    byte location;   /* Which ability to change (APPLY_XXX) */
    sh_int modifier; /* How much it changes by              */
};

struct ObjData {
    obj_num item_number;                      /* Where in data-base                        */
    room_num in_room;                         /* In what room -1 when conta/carr        */
    int mob_from;                             /* where the mob is from*/
    ObjectFlagData obj_flags;                 /* Object information               */
    ObjectApplyType applies[MAX_OBJ_APPLIES]; /* applies */
    char *name;                               /* Title of object :get etc.        */
    char *description;                        /* When in room                     */
    char *short_description;                  /* when worn/carry/in cont.         */
    char *action_description;                 /* What to write when used          */
    ExtraDescriptionData *ex_description;     /* extra descriptions     */
    CharData *carried_by;                     /* Carried by :NULL in room/conta   */
    CharData *worn_by;                        /* Worn by?                              */
    sh_int worn_on;                           /* Worn where?                      */
    ObjData *in_obj;                          /* In what object NULL when none    */
    ObjData *contains;                        /* Contains objects                 */
    long id;                                  /* used by DG triggers              */
    TriggerPrototypeList *proto_script;       /* list of default triggers  */
    ScriptData *script;                       /* script info for the object       */
    CharData *last_to_hold;                   /* If MOB forcibly loses item      */
    ObjData *next_content;                    /* For 'contains' lists             */
    ObjData *next;                            /* For the object list              */

    SpellBookList *spell_book; /* list of all spells in book if obj is spellbook */

    CharData *casters; /* Characters who are casting spells at this */
    Event *events;     /* List of events related to this object */
    int event_flags[EVENT_FLAG_FIELDS];
    /* Bitfield of events active on this object */
};

/*
 * OBJECT TYPES
 *
 * name, desc, value min/maxes
 */
const struct ObjectTypeDef item_types[NUM_ITEM_TYPES] = {

    {
        "UNDEFINED",
        "something strange",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "LIGHT",
        "a light",
        {{false, true},
         {LIGHT_PERMANENT, VAL_MAX},
         {LIGHT_PERMANENT, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "SCROLL",
        "a scroll",
        {{0, LVL_IMMORT},
         {0, MAX_SPELLS},
         {0, MAX_SPELLS},
         {0, MAX_SPELLS},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "WAND",
        "a wand",
        {{0, LVL_IMMORT},
         {0, 20},
         {0, 20},
         {0, MAX_SPELLS},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "STAFF",
        "a staff",
        {{0, LVL_IMMORT},
         {0, 20},
         {0, 20},
         {0, MAX_SPELLS},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "WEAPON",
        "a weapon",
        {{0, 40},
         {0, 20},
         {0, 20},
         {0, TYPE_ALIGN - TYPE_HIT},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "FIREWEAPON",
        "a ranged weapon",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "MISSILE",
        "a missile",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "TREASURE",
        "treasure",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "ARMOR",
        "armor",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "POTION",
        "a potion",
        {{0, LVL_IMMORT},
         {0, MAX_SPELLS},
         {0, MAX_SPELLS},
         {0, MAX_SPELLS},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "WORN",
        "clothing",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "OTHER",
        "an object",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "TRASH",
        "trash",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "TRAP",
        "a trap",
        {{0, MAX_SPELLS},
         {0, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "CONTAINER",
        "a container",
        {{0, VAL_MAX},
         {0, (1 << 4) - 1},
         {0, VAL_MAX},
         {NOT_CORPSE, CORPSE_NPC_NORAISE},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "NOTE",
        "a note",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "LIQCONTAINER",
        "a liquid container",
        {{0, VAL_MAX},
         {0, VAL_MAX},
         {0, NUM_LIQ_TYPES - 1},
         {false, true},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "KEY",
        "a key",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "FOOD",
        "food",
        {{0, VAL_MAX},
         {false, true},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "MONEY",
        "money",
        {{0, VAL_MAX},
         {0, VAL_MAX},
         {0, VAL_MAX},
         {0, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "PEN",
        "a writing instrument",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "BOAT",
        "a boat",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},

    },

    {
        "FOUNTAIN",
        "a fountain",
        {{0, VAL_MAX},
         {0, VAL_MAX},
         {0, NUM_LIQ_TYPES - 1},
         {false, true},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "PORTAL",
        "a portal",
        {{0, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "ROPE",
        "rope",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "SPELLBOOK",
        "a book",
        {{0, MAX_SPELLBOOK_PAGES},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "WALL",
        "a wall",
        {{0, NUM_OF_DIRS},
         {false, true},
         {0, VAL_MAX},
         {0, MAX_SPELLS},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "TOUCHSTONE",
        "a touchstone",
        {{VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "BOARD",
        "a board",
        {{0, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },

    {
        "INSTRUMENT",
        "an instrument",
        {{0, LVL_IMMORT},
         {0, 20},
         {0, 20},
         {0, MAX_SPELLS},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX},
         {VAL_MIN, VAL_MAX}},
    },
};

const struct LiquidDef liquid_types[NUM_LIQ_TYPES] = {
    {"water", "water", "clear", {0, 0, 10}},
    {"beer", "beer", "brown", {3, 2, 5}},
    {"wine", "wine", "clear", {5, 2, 5}},
    {"ale", "ale", "brown", {2, 2, 5}},
    {"dark-ale", "dark ale", "dark", {1, 2, 5}},
    {"whisky", "whisky", "golden", {6, 1, 4}},
    {"lemonade", "lemonade", "yellow", {0, 1, 8}},
    {"firebreather", "firebreather", "green", {10, 0, 0}},
    {"local-speciality", "local speciality", "clear", {3, 3, 3}},
    {"slime-mold-juice", "slime mold juice", "light green", {0, 4, -8}},
    {"milk", "milk", "white", {0, 3, 6}},
    {"black-tea", "black tea", "brown", {0, 1, 6}},
    {"coffee", "coffee", "black", {0, 1, 6}},
    {"blood", "blood", "red", {0, 2, -1}},
    {"salt-water", "salt water", "clear", {0, 1, -2}},
    {"rum", "rum", "light brown", {5, 2, 3}},
    {"nectar", "nectar", "yellow", {-1, 12, 12}},
    {"sake", "sake", "clearish", {6, 1, 4}},
    {"cider", "cider", "dark brown", {1, 1, 5}},
    {"tomato-soup", "tomato soup", "thick red", {0, 7, 3}},
    {"potato-soup", "potato soup", "thick, light brown", {0, 8, 4}},
    {"chai", "chai", "light brown", {0, 2, 5}},
    {"apple-juice", "apple juice", "dark yellow", {0, 2, 6}},
    {"orange-juice", "orange juice", "fruity orange", {0, 3, 5}},
    {"pineapple-juice", "pineapple juice", "thin yellow", {0, 2, 6}},
    {"grape-juice", "grape juice", "deep purple", {0, 2, 6}},
    {"pomegranate-juice", "pomegranate juice", "dark red", {0, 3, 6}},
    {"melonade", "melonade", "pink", {0, 1, 8}},
    {"cocoa", "cocoa", "thick brown", {0, 3, 5}},
    {"espresso", "espresso", "dark brown", {0, 1, 4}},
    {"cappuccino", "cappuccino", "light brown", {0, 2, 4}},
    {"mango-lassi", "mango lassi", "thick yellow", {0, 4, 4}},
    {"rosewater", "rosewater", "light pink", {0, 0, 0}},
    {"green-tea", "green tea", "pale green", {0, 1, 6}},
    {"chamomile-tea", "chamomile tea", "clearish", {0, 1, 7}},
    {"gin", "gin", "clear", {5, 0, 3}},
    {"brandy", "brandy", "amber", {5, 0, 3}},
    {"mead", "mead", "golden", {2, 0, 3}},
    {"champagne", "champagne", "sparkly", {7, 0, 1}},
    {"vodka", "vodka", "clear", {6, 0, 3}},
    {"tequila", "tequila", "gold", {5, 0, 3}},
    {"absinthe", "absinthe", "greenish", {8, 0, 2}},
};

/*[ MACROS ]******************************************************************/

#define VALID_OBJ_TYPE_NUM(type) ((type) >= 0 && (type) < NUM_ITEM_TYPES)
#define VALID_OBJ_TYPE(obj) VALID_OBJ_TYPE_NUM(GET_OBJ_TYPE(obj))
#define OBJ_TYPE_NAME(obj) (VALID_OBJ_TYPE(obj) ? item_types[(int)GET_OBJ_TYPE(obj)].name : "--")
#define OBJ_TYPE_DESC(obj) (VALID_OBJ_TYPE(obj) ? item_types[(int)GET_OBJ_TYPE(obj)].desc : "--")
#define OBJ_TYPE_VALUE_MIN(obj, val)                                                                                   \
    (VALID_OBJ_TYPE(obj) ? item_types[(int)GET_OBJ_TYPE(obj)].value[(val)].min : VAL_MIN)
#define OBJ_TYPE_VALUE_MAX(obj, val)                                                                                   \
    (VALID_OBJ_TYPE(obj) ? item_types[(int)GET_OBJ_TYPE(obj)].value[(val)].max : VAL_MAX)

#define VALID_LIQ_TYPE(type) ((type) >= 0 && (type) < NUM_LIQ_TYPES)
#define VALID_OBJ_LIQ(obj) (VALID_LIQ_TYPE(GET_OBJ_VAL((obj), VAL_DRINKCON_LIQUID)))
#define LIQ_NAME(liq) (liquid_types[VALID_LIQ_TYPE(liq) ? liq : LIQ_WATER].name)
#define LIQ_ALIAS(liq) (liquid_types[VALID_LIQ_TYPE(liq) ? liq : LIQ_WATER].alias)
#define LIQ_COLOR(liq) (liquid_types[VALID_LIQ_TYPE(liq) ? liq : LIQ_WATER].color_desc)
#define LIQ_COND(liq, cond)                                                                                            \
    (liquid_types[VALID_LIQ_TYPE(liq) ? liq : LIQ_WATER]                                                               \
         .condition_effects[(cond) == DRUNK || (cond) == FULL || (cond) == THIRST ? (cond) : THIRST])

#define GET_OBJ_NAME(obj) ((obj)->name)
#define GET_OBJ_LEVEL(obj) ((obj)->obj_flags.level)
#define GET_OBJ_TYPE(obj) ((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj) ((obj)->obj_flags.cost)
#define GET_OBJ_FLAGS(obj) ((obj)->obj_flags.extra_flags)
#define GET_OBJ_WEAR(obj) ((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val) ((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj) ((obj)->obj_flags.weight)
#define GET_OBJ_EFFECTIVE_WEIGHT(obj) ((obj)->obj_flags.effective_weight)
#define GET_OBJ_TIMER(obj) ((obj)->obj_flags.timer)
#define GET_OBJ_DECOMP(obj) ((obj)->obj_flags.decomp)
#define GET_OBJ_HIDDENNESS(obj) ((obj)->obj_flags.hiddenness)
#define GET_OBJ_MOB_FROM(obj) ((obj)->mob_from)
#define GET_OBJ_RNUM(obj) ((obj)->item_number)
#define GET_OBJ_VNUM(obj) (GET_OBJ_RNUM(obj) >= 0 ? obj_index[GET_OBJ_RNUM(obj)].vnum : -1)
#define GET_OBJ_EFF_FLAGS(obj) ((obj)->obj_flags.effect_flags)
#define OBJ_FLAGGED(obj, flag) (IS_FLAGGED(GET_OBJ_FLAGS(obj), (flag)))
#define OBJ_EFF_FLAGGED(obj, f) (IS_FLAGGED(GET_OBJ_EFF_FLAGS(obj), (f)))
#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? (obj_index[(obj)->item_number].func) : NULL)
#define CAN_WEAR(obj, part) (IS_SET(GET_OBJ_WEAR(obj), (part)))

/* Manipulation of highly visible objects draws attention. Even invisible
 * beings will be noticed if they mess with such things. For example, you
 * couldn't drag a giant treasure chest out of a room without being noticed. */
#define HIGHLY_VISIBLE(obj)                                                                                            \
    (GET_OBJ_EFFECTIVE_WEIGHT(obj) > 6 || (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL((obj), VAL_LIGHT_LIT)) ||    \
     OBJ_FLAGGED((obj), ITEM_GLOW))

#define WEAPON_AVERAGE(obj)                                                                                            \
    (((GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE) + 1) / 2.0) * GET_OBJ_VAL(obj, VAL_WEAPON_DICE_NUM))
#define IS_WEAPON_PIERCING(obj)                                                                                        \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_PIERCE - TYPE_HIT) ||                                              \
     (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_STING - TYPE_HIT) ||                                               \
     (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_STAB - TYPE_HIT))
#define IS_WEAPON_SLASHING(obj)                                                                                        \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_SLASH - TYPE_HIT) ||                                               \
     (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_CLAW - TYPE_HIT) ||                                                \
     (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_WHIP - TYPE_HIT))
#define IS_WEAPON_CRUSHING(obj)                                                                                        \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_BLUDGEON - TYPE_HIT) ||                                            \
     (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_MAUL - TYPE_HIT) ||                                                \
     (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_POUND - TYPE_HIT) ||                                               \
     (GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_CRUSH - TYPE_HIT))
#define IS_WEAPON_FIRE(obj) ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_FIRE - TYPE_HIT))
#define IS_WEAPON_COLD(obj) ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_COLD - TYPE_HIT))
#define IS_WEAPON_ACID(obj) ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_ACID - TYPE_HIT))
#define IS_WEAPON_SHOCK(obj) ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_SHOCK - TYPE_HIT))
#define IS_WEAPON_POISON(obj) ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_POISON - TYPE_HIT))
#define IS_WEAPON_ALIGN(obj) ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_ALIGN - TYPE_HIT))

#define OBJ_IS_OPENABLE(obj)                                                                                           \
    (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_CLOSEABLE))
#define OBJ_IS_OPEN(obj) (!IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_CLOSED))
#define OBJ_IS_UNLOCKED(obj) (!IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_LOCKED))
#define OBJ_IS_PICKPROOF(obj) (IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_PICKPROOF))
#define OBJ_KEY_VNUM(obj) (GET_OBJ_VAL(obj, VAL_CONTAINER_KEY))

#define IS_POISONED(obj)                                                                                               \
    ((GET_OBJ_TYPE(obj) == ITEM_FOOD || GET_OBJ_TYPE(obj) == ITEM_DRINKCON || GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)      \
         ? GET_OBJ_VAL((obj), VAL_DRINKCON_POISONED)                                                                   \
         : false)

#define MONEY_VALUE(obj)                                                                                               \
    (GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM) * 1000 + GET_OBJ_VAL(obj, VAL_MONEY_GOLD) * 100 +                            \
     GET_OBJ_VAL(obj, VAL_MONEY_SILVER) * 10 + GET_OBJ_VAL(obj, VAL_MONEY_COPPER))

#define IS_VIEWABLE_GATE(obj)                                                                                          \
    (GET_OBJ_VNUM(obj) == OBJ_VNUM_HEAVENSGATE || GET_OBJ_VNUM(obj) == OBJ_VNUM_MOONWELL ||                            \
     GET_OBJ_VNUM(obj) == OBJ_VNUM_HELLGATE)

/*
 * General object visibility accessors.
 */
#define OBJ_INVIS_TO_CHAR(obj, ch) (OBJ_FLAGGED((obj), ITEM_INVISIBLE) && !EFF_FLAGGED((ch), EFF_DETECT_INVIS))
#define OBJ_HIDDEN_TO_CHAR(obj, ch) (GET_OBJ_HIDDENNESS(obj) > GET_PERCEPTION(ch) && (obj)->last_to_hold != (ch))
#define MORT_CAN_SEE_OBJ(ch, obj) (LIGHT_OK(ch) && !OBJ_INVIS_TO_CHAR(obj, ch) && !OBJ_HIDDEN_TO_CHAR(obj, ch))
#define CAN_SEE_OBJ(ch, obj) (MORT_CAN_SEE_OBJ(ch, obj) || PRF_FLAGGED((ch), PRF_HOLYLIGHT))
#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? (obj)->short_description : "something")
#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? fname((obj)->name) : "something")

#define REPLACE_OBJ_STR(obj, strname, newstr)                                                                          \
    do {                                                                                                               \
        if (GET_OBJ_RNUM(obj) == NOTHING || (obj)->strname != obj_proto[GET_OBJ_RNUM(obj)].strname)                    \
            free((obj)->strname);                                                                                      \
        (obj)->strname = newstr;                                                                                       \
    } while (0)

/*[ FUNCTIONS ]***************************************************************/

// const obj_type_def item_types[];
// const liquid_def liquid_types[];

void init_objtypes(void);
bool delete_object(obj_num rnum);
void limit_obj_values(ObjData *obj);
int parse_obj_type(CharData *ch, char *arg);
void free_obj_strings(ObjData *obj);
void free_obj_strings_absolutely(ObjData *obj);
void free_prototyped_obj_strings(ObjData *obj);
void copy_object(ObjData *to, ObjData *from);

int parse_liquid(CharData *ch, char *arg);
void weight_change_object(ObjData *obj, float weight);
void setup_drinkcon(ObjData *obj, int newliq);
void name_from_drinkcon(ObjData *obj, int type);
void name_to_drinkcon(ObjData *obj, int type);
void liquid_from_container(ObjData *container, int amount);
void liquid_to_container(ObjData *container, int amount, int liquid_type, bool poisoned);
ObjData *carried_key(CharData *ch, int keyvnum);
float calculate_object_weight(ObjData *obj);

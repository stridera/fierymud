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

#ifndef __FIERY_OBJECTS_H
#define __FIERY_OBJECTS_H

#include "effects.h"
#include "structs.h"
#include "sysdep.h"

/*[ CONSTANTS ]***************************************************************/

/* Vnums for portal objects */
#define OBJ_VNUM_HEAVENSGATE 72
#define OBJ_VNUM_MOONWELL 33
#define OBJ_VNUM_HELLGATE 74

/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT 1       /* Item is a light source          */
#define ITEM_SCROLL 2      /* Item is a scroll                */
#define ITEM_WAND 3        /* Item is a wand                  */
#define ITEM_STAFF 4       /* Item is a staff                 */
#define ITEM_WEAPON 5      /* Item is a weapon                */
#define ITEM_FIREWEAPON 6  /* Unimplemented                   */
#define ITEM_MISSILE 7     /* Unimplemented                   */
#define ITEM_TREASURE 8    /* Item is a treasure, not gold    */
#define ITEM_ARMOR 9       /* Item is armor                   */
#define ITEM_POTION 10     /* Item is a potion                */
#define ITEM_WORN 11       /* Unimplemented                   */
#define ITEM_OTHER 12      /* Misc object                     */
#define ITEM_TRASH 13      /* Trash - shopkeeps won't buy     */
#define ITEM_TRAP 14       /* Unimplemented                   */
#define ITEM_CONTAINER 15  /* Item is a container             */
#define ITEM_NOTE 16       /* Item is note                    */
#define ITEM_DRINKCON 17   /* Item is a drink container       */
#define ITEM_KEY 18        /* Item is a key                   */
#define ITEM_FOOD 19       /* Item is food                    */
#define ITEM_MONEY 20      /* Item is money (gold)            */
#define ITEM_PEN 21        /* Item is a pen                   */
#define ITEM_BOAT 22       /* Item is a boat                  */
#define ITEM_FOUNTAIN 23   /* Item is a fountain              */
#define ITEM_PORTAL 24     /* Item teleports to another room  */
#define ITEM_ROPE 25       /* Item is used to bind chars      */
#define ITEM_SPELLBOOK 26  /* Spells can be scribed for mem   */
#define ITEM_WALL 27       /* Blocks passage in one direction */
#define ITEM_TOUCHSTONE 28 /* Item sets homeroom when touched */
#define ITEM_BOARD 29
#define NUM_ITEM_TYPES 30

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE (1 << 0)     /* Item can be takes         */
#define ITEM_WEAR_FINGER (1 << 1)   /* Can be worn on finger     */
#define ITEM_WEAR_NECK (1 << 2)     /* Can be worn around neck   */
#define ITEM_WEAR_BODY (1 << 3)     /* Can be worn on body       */
#define ITEM_WEAR_HEAD (1 << 4)     /* Can be worn on head       */
#define ITEM_WEAR_LEGS (1 << 5)     /* Can be worn on legs       */
#define ITEM_WEAR_FEET (1 << 6)     /* Can be worn on feet       */
#define ITEM_WEAR_HANDS (1 << 7)    /* Can be worn on hands      */
#define ITEM_WEAR_ARMS (1 << 8)     /* Can be worn on arms       */
#define ITEM_WEAR_SHIELD (1 << 9)   /* Can be used as a shield   */
#define ITEM_WEAR_ABOUT (1 << 10)   /* Can be worn about body    */
#define ITEM_WEAR_WAIST (1 << 11)   /* Can be worn around waist  */
#define ITEM_WEAR_WRIST (1 << 12)   /* Can be worn on wrist      */
#define ITEM_WEAR_WIELD (1 << 13)   /* Can be wielded            */
#define ITEM_WEAR_HOLD (1 << 14)    /* Can be held               */
#define ITEM_WEAR_2HWIELD (1 << 15) /* Can be wielded two handed */
#define ITEM_WEAR_EYES (1 << 16)    /* Can be worn on eyes       */
#define ITEM_WEAR_FACE (1 << 17)    /* Can be worn upon face     */
#define ITEM_WEAR_EAR (1 << 18)     /* Can be worn in ear        */
#define ITEM_WEAR_BADGE (1 << 19)   /* Can be worn as badge      */
#define ITEM_WEAR_OBELT (1 << 20)   /* Can be worn on belt       */
#define ITEM_WEAR_HOVER (1 << 21)   /* Will hover around you     */
#define NUM_ITEM_WEAR_FLAGS 22

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW 0               /* Item is glowing               */
#define ITEM_HUM 1                /* Item is humming               */
#define ITEM_NORENT 2             /* Item cannot be rented         */
#define ITEM_ANTI_BERSERKER 3     /* Not usable by berserkers      */
#define ITEM_NOINVIS 4            /* Item cannot be made invis     */
#define ITEM_INVISIBLE 5          /* Item is invisible             */
#define ITEM_MAGIC 6              /* Item is magical               */
#define ITEM_NODROP 7             /* Item can't be dropped         */
#define ITEM_PERMANENT 8          /* Item doesn't decompose        */
#define ITEM_ANTI_GOOD 9          /* Not usable by good people     */
#define ITEM_ANTI_EVIL 10         /* Not usable by evil people     */
#define ITEM_ANTI_NEUTRAL 11      /* Not usable by neutral people  */
#define ITEM_ANTI_SORCERER 12     /* Not usable by sorcerers       */
#define ITEM_ANTI_CLERIC 13       /* Not usable by clerics         */
#define ITEM_ANTI_ROGUE 14        /* Not usable by rogues          */
#define ITEM_ANTI_WARRIOR 15      /* Not usable by warriors        */
#define ITEM_NOSELL 16            /* Shopkeepers won't touch it    */
#define ITEM_ANTI_PALADIN 17      /* Not usable by paladins        */
#define ITEM_ANTI_ANTI_PALADIN 18 /* Not usable by anti-paladins   */
#define ITEM_ANTI_RANGER 19       /* Not usable by rangers         */
#define ITEM_ANTI_DRUID 20        /* Not usable by druids          */
#define ITEM_ANTI_SHAMAN 21       /* Not usable by shamans         */
#define ITEM_ANTI_ASSASSIN 22     /* Not usable by assassins       */
#define ITEM_ANTI_MERCENARY 23    /* Not usable by mercenaries     */
#define ITEM_ANTI_NECROMANCER 24  /* Not usable by necromancers    */
#define ITEM_ANTI_CONJURER 25     /* Not usable by conjurers       */
#define ITEM_NOBURN 26            /* Not destroyed by purge/fire   */
#define ITEM_NOLOCATE 27          /* Cannot be found by locate obj */
#define ITEM_DECOMP 28            /* Item is currently decomposing */
#define ITEM_FLOAT 29             /* Floats in water rooms         */
#define ITEM_NOFALL 30            /* Doesn't fall - unaffected by gravity */
#define ITEM_WAS_DISARMED 31      /* Disarmed from mob             */
#define ITEM_ANTI_MONK 32         /* Not usable by monks           */
#define NUM_ITEM_FLAGS 33

/* Modifier constants used with obj effects ('A' fields) */
#define APPLY_NONE 0           /* No effect                       */
#define APPLY_STR 1            /* Apply to strength               */
#define APPLY_DEX 2            /* Apply to dexterity              */
#define APPLY_INT 3            /* Apply to constitution           */
#define APPLY_WIS 4            /* Apply to wisdom                 */
#define APPLY_CON 5            /* Apply to constitution           */
#define APPLY_CHA 6            /* Apply to charisma               */
#define APPLY_CLASS 7          /* Reserved                        */
#define APPLY_LEVEL 8          /* Reserved                        */
#define APPLY_AGE 9            /* Apply to age                    */
#define APPLY_CHAR_WEIGHT 10   /* Apply to weight                 */
#define APPLY_CHAR_HEIGHT 11   /* Apply to height                 */
#define APPLY_MANA 12          /* Apply to max mana               */
#define APPLY_HIT 13           /* Apply to max hit points         */
#define APPLY_MOVE 14          /* Apply to max move points        */
#define APPLY_GOLD 15          /* Reserved                        */
#define APPLY_EXP 16           /* Reserved                        */
#define APPLY_AC 17            /* Apply to Armor Class            */
#define APPLY_HITROLL 18       /* Apply to hitroll                */
#define APPLY_DAMROLL 19       /* Apply to damage roll            */
#define APPLY_SAVING_PARA 20   /* Apply to save throw: paralz     */
#define APPLY_SAVING_ROD 21    /* Apply to save throw: rods       */
#define APPLY_SAVING_PETRI 22  /* Apply to save throw: petrif     */
#define APPLY_SAVING_BREATH 23 /* Apply to save throw: breath     */
#define APPLY_SAVING_SPELL 24  /* Apply to save throw: spells     */
#define APPLY_SIZE 25          /* Apply to size                   */
#define APPLY_HIT_REGEN 26
#define APPLY_MANA_REGEN 27
#define APPLY_PERCEPTION 28
#define APPLY_HIDDENNESS 29
#define APPLY_COMPOSITION 30
#define NUM_APPLY_TYPES 31

#define VAL_LIGHT_LIT 0
#define VAL_LIGHT_CAPACITY 1
#define VAL_LIGHT_REMAINING 2
#define LIGHT_PERMANENT (-1) /* For both light values 1 and 2 */

#define VAL_SCROLL_LEVEL 0
#define VAL_SCROLL_SPELL_1 1
#define VAL_SCROLL_SPELL_2 2
#define VAL_SCROLL_SPELL_3 3

#define VAL_WAND_LEVEL 0
#define VAL_WAND_MAX_CHARGES 1
#define VAL_WAND_CHARGES_LEFT 2
#define VAL_WAND_SPELL 3

#define VAL_STAFF_LEVEL 0
#define VAL_STAFF_MAX_CHARGES 1
#define VAL_STAFF_CHARGES_LEFT 2
#define VAL_STAFF_SPELL 3

#define VAL_WEAPON_HITROLL 0
#define VAL_WEAPON_DICE_NUM 1
#define VAL_WEAPON_DICE_SIZE 2
#define VAL_WEAPON_DAM_TYPE 3

#define VAL_ARMOR_AC 0

#define VAL_POTION_LEVEL 0
#define VAL_POTION_SPELL_1 1
#define VAL_POTION_SPELL_2 2
#define VAL_POTION_SPELL_3 3

#define VAL_TRAP_SPELL 0
#define VAL_TRAP_HITPOINTS 1

#define VAL_BLOOD_ROOM 0

#define VAL_CONTAINER_CAPACITY 0
#define VAL_CONTAINER_BITS 1
#define CONT_CLOSEABLE (1 << 0) /* Container can be closed       */
#define CONT_PICKPROOF (1 << 1) /* Container is pickproof        */
#define CONT_CLOSED (1 << 2)    /* Container is closed           */
#define CONT_LOCKED (1 << 3)    /* Container is locked           */
#define VAL_CONTAINER_KEY 2
#define VAL_CONTAINER_WEIGHT_REDUCTION                                                                                 \
    4 /* Used to allow bags of holding, which reduce the weight of items carried.                                      \
       */
#define VAL_CORPSE_ID 2
#define VAL_CONTAINER_CORPSE 3
#define NOT_CORPSE 0
#define CORPSE_PC 1
#define CORPSE_NPC 2
#define CORPSE_NPC_NORAISE 3
#define IS_CORPSE(obj) (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) != NOT_CORPSE)
#define IS_PLR_CORPSE(obj)                                                                                             \
    (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) == CORPSE_PC)

#define VAL_DRINKCON_CAPACITY 0
#define VAL_DRINKCON_REMAINING 1
#define VAL_DRINKCON_LIQUID 2
#define LIQ_WATER 0
#define LIQ_BEER 1
#define LIQ_WINE 2
#define LIQ_ALE 3
#define LIQ_DARKALE 4
#define LIQ_WHISKY 5
#define LIQ_LEMONADE 6
#define LIQ_FIREBRT 7
#define LIQ_LOCALSPC 8
#define LIQ_SLIME 9
#define LIQ_MILK 10
#define LIQ_TEA 11
#define LIQ_COFFEE 12
#define LIQ_BLOOD 13
#define LIQ_SALTWATER 14
#define LIQ_RUM 15
#define LIQ_NECTAR 16
#define LIQ_SAKE 17
#define LIQ_CIDER 18
#define LIQ_TOMATOSOUP 19
#define LIQ_POTATOSOUP 20
#define LIQ_CHAI 21
#define LIQ_APPLEJUICE 22
#define LIQ_ORNGJUICE 23
#define LIQ_PNAPLJUICE 24
#define LIQ_GRAPEJUICE 25
#define LIQ_POMJUICE 26
#define LIQ_MELONAE 27
#define LIQ_COCOA 28
#define LIQ_ESPRESSO 29
#define LIQ_CAPPUCCINO 30
#define LIQ_MANGOLASSI 31
#define LIQ_ROSEWATER 32
#define LIQ_GREENTEA 33
#define LIQ_CHAMOMILE 34
#define LIQ_GIN 35
#define LIQ_BRANDY 36
#define LIQ_MEAD 37
#define LIQ_CHAMPAGNE 38
#define LIQ_VODKA 39
#define LIQ_TEQUILA 40
#define LIQ_ABSINTHE 41
#define NUM_LIQ_TYPES 42
#define VAL_DRINKCON_POISONED 3 /* Must = fountain/food poisoned */

#define VAL_FOOD_FILLINGNESS 0
#define VAL_FOOD_POISONED 3 /* Must = drink/fountain poisoned*/

#define VAL_MONEY_PLATINUM 0
#define VAL_MONEY_GOLD 1
#define VAL_MONEY_SILVER 2
#define VAL_MONEY_COPPER 3

#define VAL_FOUNTAIN_CAPACITY 0
#define VAL_FOUNTAIN_REMAINING 1
#define VAL_FOUNTAIN_LIQUID 2   /* Use LIQ_ constants above      */
#define VAL_FOUNTAIN_POISONED 3 /* Must = drinkcon/food poisoned */

#define VAL_PORTAL_DESTINATION 0
#define VAL_PORTAL_ENTRY_MSG 1 /* Message to room portal is in  */
#define VAL_PORTAL_CHAR_MSG 2  /* Message to character entering */
#define VAL_PORTAL_EXIT_MSG 3  /* Message to portal target room */

#define VAL_SPELLBOOK_PAGES 0
#define DEF_SPELLBOOK_PAGES 100
#define MAX_SPELLBOOK_PAGES 2000 /* Kind of arbitrary             */

#define VAL_WALL_DIRECTION 0
#define VAL_WALL_DISPELABLE 1
#define VAL_WALL_HITPOINTS 2
#define VAL_WALL_SPELL 3

#define VAL_BOARD_NUMBER 0

#define VAL_MIN (-100000) /* Arbitrary default min value */
#define VAL_MAX 100000    /* Arbitrary default max value */

#define NUM_VALUES 7

#define MAX_OBJ_APPLIES 6

/*[ STRUCTURES ]**************************************************************/

struct obj_type_def {
    char *name;
    char *desc;
    struct {
        int min;
        int max;
    } value[NUM_VALUES];
};

struct liquid_def {
    char *alias;
    char *name;
    char *color_desc;
    int condition_effects[3];
};

/* object flags; used in obj_data */
struct obj_flag_data {
    int value[NUM_VALUES]; /* Values of the item (see list)  */
    byte type_flag;        /* Type of item                     */
    long int wear_flags;   /* Where you can wear it            */
    /* If it hums, glows, etc.          */
    flagvector extra_flags[FLAGVECTOR_SIZE(NUM_ITEM_FLAGS)];
    float weight; /* Weight what else                 */
    int cost;     /* Value when sold (gp.)            */
    int level;    /* Level of the object              */
    int timer;    /* Timer for object                 */
    int decomp;   /* Decomposition timer              */
    /* Object Spell effects             */
    flagvector effect_flags[FLAGVECTOR_SIZE(NUM_EFF_FLAGS)];
    long hiddenness; /* How difficult it is to see obj   */
};

struct spell_book_list {
    int spell;
    int length;
    struct spell_book_list *next;
};

struct obj_apply_type {
    byte location;   /* Which ability to change (APPLY_XXX) */
    sh_int modifier; /* How much it changes by              */
};

struct obj_data {
    obj_num item_number;                            /* Where in data-base                        */
    room_num in_room;                               /* In what room -1 when conta/carr        */
    int mob_from;                                   /* where the mob is from*/
    struct obj_flag_data obj_flags;                 /* Object information               */
    struct obj_apply_type applies[MAX_OBJ_APPLIES]; /* applies */
    char *name;                                     /* Title of object :get etc.        */
    char *description;                              /* When in room                     */
    char *short_description;                        /* when worn/carry/in cont.         */
    char *action_description;                       /* What to write when used          */
    struct extra_descr_data *ex_description;        /* extra descriptions     */
    struct char_data *carried_by;                   /* Carried by :NULL in room/conta   */
    struct char_data *worn_by;                      /* Worn by?                              */
    sh_int worn_on;                                 /* Worn where?                      */
    struct obj_data *in_obj;                        /* In what object NULL when none    */
    struct obj_data *contains;                      /* Contains objects                 */
    long id;                                        /* used by DG triggers              */
    struct trig_proto_list *proto_script;           /* list of default triggers  */
    struct script_data *script;                     /* script info for the object       */
    struct char_data *last_to_hold;                 /* If MOB forcibly loses item      */
    struct obj_data *next_content;                  /* For 'contains' lists             */
    struct obj_data *next;                          /* For the object list              */

    struct spell_book_list *spell_book; /* list of all spells in book if obj is spellbook */

    struct char_data *casters; /* Characters who are casting spells at this */
    struct event *events;      /* List of events related to this object */
    int event_flags[EVENT_FLAG_FIELDS];
    /* Bitfield of events active on this object */
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
#define GET_OBJ_TIMER(obj) ((obj)->obj_flags.timer)
#define GET_OBJ_DECOMP(obj) ((obj)->obj_flags.decomp)
#define GET_OBJ_HIDDENNESS(obj) ((obj)->obj_flags.hiddenness)
#define GET_OBJ_MOB_FROM(obj) ((obj)->mob_from)
#define GET_OBJ_RNUM(obj) ((obj)->item_number)
#define GET_OBJ_VNUM(obj) (GET_OBJ_RNUM(obj) >= 0 ? obj_index[GET_OBJ_RNUM(obj)].virtual : -1)
#define GET_OBJ_EFF_FLAGS(obj) ((obj)->obj_flags.effect_flags)
#define OBJ_FLAGGED(obj, flag) (IS_FLAGGED(GET_OBJ_FLAGS(obj), (flag)))
#define OBJ_EFF_FLAGGED(obj, f) (IS_FLAGGED(GET_OBJ_EFF_FLAGS(obj), (f)))
#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? (obj_index[(obj)->item_number].func) : NULL)
#define CAN_WEAR(obj, part) (IS_SET(GET_OBJ_WEAR(obj), (part)))

/* Manipulation of highly visible objects draws attention. Even invisible
 * beings will be noticed if they mess with such things. For example, you
 * couldn't drag a giant treasure chest out of a room without being noticed. */
#define HIGHLY_VISIBLE(obj)                                                                                            \
    (GET_OBJ_WEIGHT(obj) > 6 || (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL((obj), VAL_LIGHT_LIT)) ||              \
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
#define IS_WEAPON_FIRE(obj)                                                                                            \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_FIRE - TYPE_HIT))
#define IS_WEAPON_COLD(obj)                                                                                            \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_COLD - TYPE_HIT))
#define IS_WEAPON_ACID(obj)                                                                                            \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_ACID - TYPE_HIT))
#define IS_WEAPON_SHOCK(obj)                                                                                           \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_SHOCK - TYPE_HIT))
#define IS_WEAPON_POISON(obj)                                                                                          \
    ((GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) == TYPE_POISON - TYPE_HIT))

#define OBJ_IS_OPENABLE(obj)                                                                                           \
    (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_CLOSEABLE))
#define OBJ_IS_OPEN(obj) (!IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_CLOSED))
#define OBJ_IS_UNLOCKED(obj) (!IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_LOCKED))
#define OBJ_IS_PICKPROOF(obj) (IS_SET(GET_OBJ_VAL(obj, VAL_CONTAINER_BITS), CONT_PICKPROOF))
#define OBJ_KEY_VNUM(obj) (GET_OBJ_VAL(obj, VAL_CONTAINER_KEY))

#define IS_POISONED(obj)                                                                                               \
    ((GET_OBJ_TYPE(obj) == ITEM_FOOD || GET_OBJ_TYPE(obj) == ITEM_DRINKCON || GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)      \
         ? GET_OBJ_VAL((obj), VAL_DRINKCON_POISONED)                                                                   \
         : FALSE)

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

extern const struct obj_type_def item_types[];
extern const struct liquid_def liquid_types[];

extern void init_objtypes(void);
extern bool delete_object(obj_num rnum);
extern void limit_obj_values(struct obj_data *obj);
extern int parse_obj_type(struct char_data *ch, char *arg);
extern void free_obj_strings(struct obj_data *obj);
extern void free_obj_strings_absolutely(struct obj_data *obj);
extern void free_prototyped_obj_strings(struct obj_data *obj);
extern void copy_object(struct obj_data *to, struct obj_data *from);

extern int parse_liquid(struct char_data *ch, char *arg);
extern void weight_change_object(struct obj_data *obj, float weight);
extern void setup_drinkcon(struct obj_data *obj, int newliq);
extern void name_from_drinkcon(struct obj_data *obj, int type);
extern void name_to_drinkcon(struct obj_data *obj, int type);
extern void liquid_from_container(struct obj_data *container, int amount);
extern void liquid_to_container(struct obj_data *container, int amount, int liquid_type, bool poisoned);

#endif
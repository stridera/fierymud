/***************************************************************************
 *  File: objects.c                                       Part of FieryMUD *
 *  Usage: object handling routines                                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "board.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "dg_scripts.h"
#include "directions.h"
#include "genzon.h"
#include "handler.h"
#include "math.h"
#include "olc.h"
#include "shop.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

#include <math.h>

/* External variables */
extern const char *portal_entry_messages[];
extern const char *portal_character_messages[];
extern const char *portal_exit_messages[];

#define ZCMD(zone, no) (zone_table[zone].cmd[no])

/*
 * OBJECT TYPES
 *
 * name, desc, value min/maxes
 */
const struct obj_type_def item_types[NUM_ITEM_TYPES] = {

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
        {{FALSE, TRUE},
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
         {0, TYPE_POISON - TYPE_HIT},
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
         {FALSE, TRUE},
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
         {FALSE, TRUE},
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
         {FALSE, TRUE},
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
         {FALSE, TRUE},
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
};

void init_objtypes(void) {
    int i, j;
    const struct obj_type_def *type;

    for (i = 0; i < NUM_ITEM_TYPES; ++i) {
        type = &item_types[i];
        if (!type->name || !*type->name) {
            sprintf(buf, "SYSERR: No name for object type %d in obj_type_def in objects.c", i);
            log(buf);
        }
        if (!type->desc || !*type->desc) {
            sprintf(buf,
                    "SYSERR: No description for object type %d in obj_type_def in "
                    "objects.c",
                    i);
            log(buf);
        }
        for (j = 0; j < NUM_VALUES; ++j)
            if (type->value[j].max < type->value[j].min) {
                sprintf(buf,
                        "SYSERR: max less than min for %s value %d in obj_type_def in "
                        "objects.c",
                        type->name, j);
                log(buf);
            }
    }
}

const struct liquid_def liquid_types[NUM_LIQ_TYPES] = {
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

static int min_value(struct obj_data *obj, int val) {
    int min = item_types[(int)GET_OBJ_TYPE(obj)].value[val].min;

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WAND:
    case ITEM_STAFF:
        if (val == VAL_WAND_MAX_CHARGES)
            min = MAX(min, GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        if (val == VAL_DRINKCON_CAPACITY)
            min = MAX(min, GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING));
        break;
    }
    return min;
}

static int max_value(struct obj_data *obj, int val) {
    int max = item_types[(int)GET_OBJ_TYPE(obj)].value[val].max;

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_PORTAL:
        switch (val) {
        case VAL_PORTAL_ENTRY_MSG:
            for (max = 0; *portal_entry_messages[max] != '\n'; ++max)
                ;
            break;
        case VAL_PORTAL_CHAR_MSG:
            for (max = 0; *portal_character_messages[max] != '\n'; ++max)
                ;
            break;
        case VAL_PORTAL_EXIT_MSG:
            for (max = 0; *portal_exit_messages[max] != '\n'; ++max)
                ;
            break;
        }
        --max;
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        if (val == VAL_WAND_CHARGES_LEFT)
            max = MIN(max, GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        if (val == VAL_DRINKCON_REMAINING)
            max = MIN(max, GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY));
        break;
    case ITEM_BOARD:
        if (val == VAL_BOARD_NUMBER)
            max = board_count();
        break;
    }

    return max;
}

bool is_value_within_bounds(struct obj_data *obj, int val) {
    if (!obj || val < 0 || val >= NUM_VALUES)
        return FALSE;

    return (GET_OBJ_VAL(obj, val) == LIMIT(min_value(obj, val), GET_OBJ_VAL(obj, val), max_value(obj, val)));
}

void limit_obj_values(struct obj_data *obj) {
    int i;

    if (!obj)
        return;

    if (GET_OBJ_TYPE(obj) < 0 || GET_OBJ_TYPE(obj) >= NUM_ITEM_TYPES)
        return;

    /*
     * Traverse the values in reverse order: this is sort of a hack.
     * We do this because typically CAPACITY values come before REMAINING,
     * and we usually want REMAINING to be capped by CAPACITY, so we need
     * to check it first.
     */
    for (i = NUM_VALUES - 1; i >= 0; --i)
        GET_OBJ_VAL(obj, i) = LIMIT(min_value(obj, i), GET_OBJ_VAL(obj, i), max_value(obj, i));
}

int parse_obj_type(struct char_data *ch, char *arg) {
    return parse_obj_name(ch, arg, "obj type", NUM_ITEM_TYPES, (void *)item_types, sizeof(struct obj_type_def));
}

int parse_liquid(struct char_data *ch, char *arg) {
    return parse_obj_name(ch, arg, "liquid", NUM_LIQ_TYPES, (void *)liquid_types, sizeof(struct liquid_def));
}

bool delete_object(obj_num rnum) {
    int i, vnum;
    int zrnum;
    struct obj_data *obj, *tmp;
    int shop, j, zone, cmd_no;
    bool save_this_zone;

    if (rnum == NOTHING || rnum > top_of_objt) {
        sprintf(buf, "ERR: delete_object() rnum %d out of range", rnum);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return FALSE;
    }

    obj = &obj_proto[rnum];
    vnum = GET_OBJ_VNUM(obj);

    zrnum = find_real_zone_by_room(GET_OBJ_VNUM(obj));
    if (zrnum == -1) {
        sprintf(buf, "ERR: delete_object() can't identify zone for obj vnum %d", vnum);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return FALSE;
    }

    for (tmp = object_list; tmp; tmp = tmp->next) {
        if (tmp->item_number != obj->item_number)
            continue;

        /* extract_obj() will just axe contents. */
        if (tmp->contains) {
            struct obj_data *this_content, *next_content;
            for (this_content = tmp->contains; this_content; this_content = next_content) {
                next_content = this_content->next_content;
                if (IN_ROOM(tmp)) {
                    /* Transfer stuff from object to room. */
                    obj_from_obj(this_content);
                    obj_to_room(this_content, IN_ROOM(tmp));
                } else if (tmp->worn_by || tmp->carried_by) {
                    /* Transfer stuff from object to person inventory. */
                    obj_from_char(this_content);
                    obj_to_char(this_content, tmp->carried_by);
                } else if (tmp->in_obj) {
                    /* Transfer stuff from object to containing object. */
                    obj_from_obj(this_content);
                    obj_to_obj(this_content, tmp->in_obj);
                }
            }
        }
        /* Remove from object_list, etc. - handles weightchanges, and similar. */
        extract_obj(tmp);
    }

    /* Make sure all are removed. */
    assert(obj_index[rnum].number == 0);

    /* Adjust rnums of all other objects. */
    for (tmp = object_list; tmp; tmp = tmp->next)
        GET_OBJ_RNUM(tmp) -= (GET_OBJ_RNUM(tmp) != NOTHING && GET_OBJ_RNUM(tmp) > rnum);

    for (i = rnum; i < top_of_objt; i++) {
        obj_index[i] = obj_index[i + 1];
        obj_proto[i] = obj_proto[i + 1];
        obj_proto[i].item_number = i;
    }

    top_of_objt--;
    RECREATE(obj_index, struct index_data, top_of_objt + 1);
    RECREATE(obj_proto, struct obj_data, top_of_objt + 1);

    /* Renumber shop products. */
    printf("top_shop is %d\n", top_shop);
    for (shop = 0; shop < top_shop; shop++)
        for (j = 0; SHOP_PRODUCT(shop, j) != NOTHING; j++)
            SHOP_PRODUCT(shop, j) -= (SHOP_PRODUCT(shop, j) > rnum);

    /* Renumber zone table. */
    for (zone = 0; zone <= top_of_zone_table; zone++) {
        save_this_zone = FALSE;
        for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++) {
            switch (ZCMD(zone, cmd_no).command) {
            case 'P':
                if (ZCMD(zone, cmd_no).arg3 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                } else
                    ZCMD(zone, cmd_no).arg3 -= (ZCMD(zone, cmd_no).arg3 > rnum);
                break;
                save_this_zone = TRUE;
            case 'O':
            case 'G':
            case 'E':
                if (ZCMD(zone, cmd_no).arg1 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                } else
                    ZCMD(zone, cmd_no).arg1 -= (ZCMD(zone, cmd_no).arg1 > rnum);
                break;
                save_this_zone = TRUE;
            case 'R':
                if (ZCMD(zone, cmd_no).arg2 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                } else
                    ZCMD(zone, cmd_no).arg2 -= (ZCMD(zone, cmd_no).arg2 > rnum);
                break;
                save_this_zone = TRUE;
            }
        }
        if (save_this_zone) {
            olc_add_to_save_list(zone_table[zone].number, OLC_SAVE_ZONE);
        }
    }

    olc_add_to_save_list(zone_table[zrnum].number, OLC_SAVE_OBJ);

    return TRUE;
}

void copy_object(struct obj_data *to, struct obj_data *from) {
    free_obj_strings(to);
    *to = *from;
    to->name = from->name ? strdup(from->name) : NULL;
    to->description = from->description ? strdup(from->description) : NULL;
    to->short_description = from->short_description ? strdup(from->short_description) : NULL;
    to->action_description = from->action_description ? strdup(from->action_description) : NULL;

    if (from->ex_description)
        copy_extra_descriptions(&to->ex_description, from->ex_description);
    else
        to->ex_description = NULL;
}

#define FREE(var)                                                                                                      \
    do {                                                                                                               \
        free(var);                                                                                                     \
        var = NULL;                                                                                                    \
    } while (0)

void free_obj_strings_absolutely(struct obj_data *obj) {
    extern void free_extra_descriptions(struct extra_descr_data * edesc);

    if (obj->name)
        FREE(obj->name);
    if (obj->description)
        FREE(obj->description);
    if (obj->short_description)
        FREE(obj->short_description);
    if (obj->action_description)
        FREE(obj->action_description);
    if (obj->ex_description) {
        free_extra_descriptions(obj->ex_description);
        obj->ex_description = NULL;
    }
}

void free_prototyped_obj_strings(struct obj_data *obj) {
    struct obj_data *proto = &obj_proto[GET_OBJ_RNUM(obj)];

    if (obj->name && obj->name != proto->name)
        FREE(obj->name);
    if (obj->description && obj->description != proto->description)
        FREE(obj->description);
    if (obj->short_description && obj->short_description != proto->short_description)
        FREE(obj->short_description);
    if (obj->action_description && obj->action_description != proto->action_description)
        FREE(obj->action_description);
    if (obj->ex_description) {
        struct extra_descr_data *obj_ed, *proto_ed, *next_ed;
        bool ok_key, ok_desc, ok_item;
        /* O(horrible) */
        for (obj_ed = obj->ex_description; obj_ed; obj_ed = next_ed) {
            next_ed = obj_ed->next;
            /* If this obj_ed is in the proto's ex_desc list, don't free */
            for (ok_item = ok_key = ok_desc = TRUE, proto_ed = proto->ex_description; proto_ed;
                 proto_ed = proto_ed->next) {
                if (proto_ed->keyword == obj_ed->keyword)
                    ok_key = FALSE;
                if (proto_ed->description == obj_ed->description)
                    ok_desc = FALSE;
                if (proto_ed == obj_ed)
                    ok_item = FALSE;
            }
            if (obj_ed->keyword && ok_key)
                FREE(obj_ed->keyword);
            if (obj_ed->description && ok_desc)
                FREE(obj_ed->description);
            if (ok_item)
                FREE(obj_ed);
        }
    }
}

void free_obj_strings(struct obj_data *obj) {
    if (GET_OBJ_RNUM(obj) == NOTHING)
        free_obj_strings_absolutely(obj);
    else
        free_prototyped_obj_strings(obj);
}

#undef FREE

void weight_change_object(struct obj_data *obj, float weight) {
    struct obj_data *tmp_obj;
    struct char_data *tmp_ch;

    if (obj->in_room != NOWHERE) {
        GET_OBJ_WEIGHT(obj) += weight;
    } else if ((tmp_ch = obj->carried_by)) {
        obj_from_char(obj);
        GET_OBJ_WEIGHT(obj) += weight;
        obj_to_char(obj, tmp_ch);
    } else if ((tmp_obj = obj->in_obj)) {
        obj_from_obj(obj);
        GET_OBJ_WEIGHT(obj) += weight;
        obj_to_obj(obj, tmp_obj);
    } else {
        /* Object is not in the world yet. */
        GET_OBJ_WEIGHT(obj) += weight;
    }
}

/* NAME_FROM_DRINKCON
 *
 * Called when a liquid container becomes empty.
 *
 * It will remove the first name from the object's aliases, if it is
 * the same as the name of the liquid.
 */

void name_from_drinkcon(struct obj_data *obj, int type) {
    int aliaslen;
    char *new_name;

    if (!VALID_LIQ_TYPE(type))
        return;

    aliaslen = strlen(LIQ_ALIAS(type));

    if (strlen(obj->name) > aliaslen + 1 &&
        /* The aliases are long enough to have more than just the drink name */
        (obj->name)[aliaslen] == ' ' &&
        /* An alias terminates just where the drink name should */
        !strncmp(LIQ_ALIAS(type), obj->name, aliaslen))
    /* Same string: we are go for removal */
    {
        new_name = strdup((obj->name) + aliaslen + 1);
        REPLACE_OBJ_STR(obj, name, new_name);
    }
}

void name_to_drinkcon(struct obj_data *obj, int type) {
    char *new_name;

    if (!VALID_LIQ_TYPE(type) || isname(LIQ_ALIAS(type), obj->name))
        return;

    CREATE(new_name, char, strlen(obj->name) + strlen(LIQ_ALIAS(type)) + 2);
    sprintf(new_name, "%s %s", LIQ_ALIAS(type), obj->name);
    REPLACE_OBJ_STR(obj, name, new_name);
}

#define POUNDS_PER_OZ_WATER 0.0651984721
#define LIQUID_MASS(amount, type) (amount * POUNDS_PER_OZ_WATER)

/* SETUP_DRINKCON
 *
 * Call this when:
 *
 *   1. The object is first created. Pass (obj, -1)
 *   2. The object becomes empty.    Pass (obj, 0)
 *   3. The object's liquid changes. Pass (obj, liquid)
 *
 * This function must assume that the object has not been inserted into the
 * world yet!  - That it is not being carried or worn, or in a room or
 * other object.
 *
 * Parameter:
 *
 *     newliq      The liquid that the container's going to be
 *                 carrying.  Set this to -1 if the liquid is not
 *                 changing.
 *
 * It does the following:
 *
 * -- add the weight of the liquid to that of the container, if new
 * -- makes certain that the alias list begins with the liquid name
 * -- applies the special "action desc" settings, as follows:
 *
 *    *nl <string>  - string is processed to become obj->name (aliases)
 *    *sl <string>  - string is processed to become short description
 *    *ll <string>  - string is processed to become long description
 *
 *    *ne <string>  - same as *nl, but when the container is empty
 *    *se <string>  - as *sl, empty
 *    *le <string>  - as *ll, empty
 *
 * The format codes for the action desc settings are:
 *
 *    $l  - Replaced with the liquid name
 *
 * It always attempts to change the aliases (obj->name). If the action
 * desc specifies *nl and/or *ne, those will override the default aliases.
 */

void setup_drinkcon(struct obj_data *obj, int newliq) {
    char *line, *adesc, *newtext;

    /* A brand new liquid container. */
    if (newliq < 0) {
        /* Ensure that the namelist begins with the drink name.
         * This effort will be undone if the action desc so directs,
         * but that's ok. */
        newliq = GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID);
        /* Add the weight of any contained liquid to the object's apparent weight.
         */
        if (GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) > 0) {
            weight_change_object(
                obj, LIQUID_MASS(GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING), GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)));
        }
    }

    /* First get the old liquid's name out of the object's aliases. */
    /* This is done when the liquid is changing, or the container is empty. */
    if (newliq != GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID) || GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) == 0) {
        name_from_drinkcon(obj, GET_OBJ_VAL(obj, 2));
        GET_OBJ_VAL(obj, 2) = newliq;
    }

    /* Put the drink name only if there is some liquid inside. */
    if (GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) > 0)
        name_to_drinkcon(obj, GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID));

    /* Get ready to process the action description. */
    adesc = obj->action_description;
    if (!adesc || !*adesc)
        return;

    /* Loop to process each line of text in the action description */
    while (*adesc) {
        if (!(line = next_line(&adesc)))
            break;
        /* We must subsequently free line else cause a memory leak. */

        if (strlen(line) > 4 && line[0] == '*' && line[3] == ' ') {
            if (line[2] != 'e' && line[2] != 'l') {
                sprintf(buf,
                        "Error setting up drinkcon %d: expected 'l' or 'e' in adesc, "
                        "but got #%d",
                        GET_OBJ_VNUM(obj), line[2]);
                mudlog(buf, BRF, LVL_IMMORT, TRUE);
            } else {
                if (line[2] == 'l' && GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) > 0) {
                    /* Processing input line with liquid present */
                    newtext = strdup(line + 4);

                    /* Ignore the return value of replace_str, because it's ok
                     * for no replacements to occur. */
                    replace_str(&newtext, "$l", LIQ_NAME(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)), 1, 190);
                } else if (line[2] == 'e' && GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING) == 0) {
                    /* Processing input line when empty */
                    newtext = strdup(line + 4);

                    /* Just a copy, nothing else to do */
                } else {
                    /* Since l/e doesn't match current fill state, ignore this line */
                    goto ADESC_LINE;
                }

                /* newtext now contains the processed string. It is ready for
                 * insertion into the object. */
                switch (line[1]) {
                case 'n':
                    REPLACE_OBJ_STR(obj, name, newtext);
                    break;
                case 's':
                    REPLACE_OBJ_STR(obj, short_description, newtext);
                    break;
                case 'l':
                    REPLACE_OBJ_STR(obj, description, newtext);
                    break;
                default:
                    sprintf(buf,
                            "Error setting up drinkcon %d: expected 'n', 's', or 'l' in "
                            "adesc, but got #%d",
                            GET_OBJ_VNUM(obj), line[1]);
                    mudlog(buf, BRF, LVL_IMMORT, TRUE);
                    free(newtext);
                }
            }
        }
    ADESC_LINE:
        free(line);
    }
}

void liquid_from_container(struct obj_data *container, int amount) {
    int loss, remaining;
    float weight;

    if (GET_OBJ_TYPE(container) == ITEM_FOUNTAIN)
        return;

    if (GET_OBJ_TYPE(container) != ITEM_DRINKCON) {
        mprintf(L_ERROR, LVL_IMMORT, "liquid_from_container asked to remove %d oz from %s", amount,
                GET_OBJ_NAME(container));
        return;
    }

    loss = MIN(amount, GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING));
    remaining = GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING) - loss;

    if (loss == 0) {
        mprintf(L_ERROR, LVL_IMMORT,
                "liquid_from_container is not removing any liquid from %s "
                "(requested: %d)",
                GET_OBJ_NAME(container), amount);
        return;
    }

    GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING) = remaining;

    weight = LIQUID_MASS(loss, GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID));
    weight_change_object(container, -weight);

    if (remaining == 0) {
        GET_OBJ_VAL(container, VAL_DRINKCON_POISONED) = FALSE;
        /* Since the container is empty, its strings could be changed. */
        setup_drinkcon(container, 0);
    }
}

void liquid_to_container(struct obj_data *container, int amount, int liquid_type, bool poisoned) {
    int change, final_amount;
    float weight;

    if (GET_OBJ_TYPE(container) == ITEM_FOUNTAIN)
        return;

    final_amount =
        MIN(amount + GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING), GET_OBJ_VAL(container, VAL_DRINKCON_CAPACITY));
    change = final_amount - GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING);

    if (change < 0) {
        mprintf(L_ERROR, LVL_IMMORT, "liquid_to_container is not adding any liquid to %s (requested: %d)",
                GET_OBJ_NAME(container), amount);
        return;
    }

    GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING) = final_amount;
    GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID) = liquid_type;
    if (poisoned)
        GET_OBJ_VAL(container, VAL_DRINKCON_POISONED) = TRUE;
    weight = LIQUID_MASS(change, GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID));
    weight_change_object(container, weight);
    setup_drinkcon(container, GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID));
}

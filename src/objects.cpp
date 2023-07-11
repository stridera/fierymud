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

#include "objects.hpp"

#include "board.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "genzon.hpp"
#include "handler.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "olc.hpp"
#include "shop.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

#define ZCMD(zone, no) (zone_table[zone].cmd[no])

void init_objtypes(void) {
    int i, j;
    const ObjectTypeDef *type;

    for (i = 0; i < NUM_ITEM_TYPES; ++i) {
        type = &item_types[i];
        if (!type->name || !*type->name) {
            log("SYSERR: No name for object type {:d} in obj_type_def in objects.c", i);
        }
        if (!type->desc || !*type->desc) {
            log("SYSERR: No description for object type {:d} in obj_type_def in objects.c", i);
        }
        for (j = 0; j < NUM_VALUES; ++j)
            if (type->value[j].max < type->value[j].min) {
                log("SYSERR: max less than min for {} value {:d} in obj_type_def in objects.c", type->name, j);
            }
    }
}

static int min_value(ObjData *obj, int val) {
    int min = item_types[(int)GET_OBJ_TYPE(obj)].value[val].min;

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_INSTRUMENT:
        if (val == VAL_WAND_MAX_CHARGES)
            min = std::max(min, GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        if (val == VAL_DRINKCON_CAPACITY)
            min = std::max(min, GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING));
        break;
    }
    return min;
}

static int max_value(ObjData *obj, int val) {
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
    case ITEM_INSTRUMENT:
        if (val == VAL_WAND_CHARGES_LEFT)
            max = std::min(max, GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        if (val == VAL_DRINKCON_REMAINING)
            max = std::min(max, GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY));
        break;
    case ITEM_BOARD:
        if (val == VAL_BOARD_NUMBER)
            max = board_count();
        break;
    }

    return max;
}

bool is_value_within_bounds(ObjData *obj, int val) {
    if (!obj || val < 0 || val >= NUM_VALUES)
        return false;

    return (GET_OBJ_VAL(obj, val) == std::clamp(GET_OBJ_VAL(obj, val), min_value(obj, val), max_value(obj, val)));
}

void limit_obj_values(ObjData *obj) {
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
        GET_OBJ_VAL(obj, i) = std::clamp(GET_OBJ_VAL(obj, i), min_value(obj, i), max_value(obj, i));
}

int parse_obj_type(CharData *ch, char *arg) {
    return parse_obj_name(ch, arg, "obj type", NUM_ITEM_TYPES, (void *)item_types, sizeof(ObjectTypeDef));
}

int parse_liquid(CharData *ch, char *arg) {
    return parse_obj_name(ch, arg, "liquid", NUM_LIQ_TYPES, (void *)liquid_types, sizeof(LiquidDef));
}

bool delete_object(obj_num rnum) {
    int i, vnum;
    int zrnum;
    ObjData *obj, *tmp;
    int shop, j, zone, cmd_no;
    bool save_this_zone;

    if (rnum == NOTHING || rnum > top_of_objt) {
        log(LogSeverity::Stat, LVL_GOD, "ERR: delete_object() rnum {} out of range", rnum);
        return false;
    }

    obj = &obj_proto[rnum];
    vnum = GET_OBJ_VNUM(obj);

    zrnum = find_real_zone_by_room(GET_OBJ_VNUM(obj));
    if (zrnum == -1) {
        log(LogSeverity::Stat, LVL_GOD, "ERR: delete_object() can't identify zone for obj vnum {}", vnum);
        return false;
    }

    for (tmp = object_list; tmp; tmp = tmp->next) {
        if (tmp->item_number != obj->item_number)
            continue;

        /* extract_obj() will just axe contents. */
        if (tmp->contains) {
            ObjData *this_content, *next_content;
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
    RECREATE(obj_index, IndexData, top_of_objt + 1);
    RECREATE(obj_proto, ObjData, top_of_objt + 1);

    /* Renumber shop products. */
    printf("top_shop is %d\n", top_shop);
    for (shop = 0; shop < top_shop; shop++)
        for (j = 0; SHOP_PRODUCT(shop, j) != NOTHING; j++)
            SHOP_PRODUCT(shop, j) -= (SHOP_PRODUCT(shop, j) > rnum);

    /* Renumber zone table. */
    for (zone = 0; zone <= top_of_zone_table; zone++) {
        save_this_zone = false;
        for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++) {
            switch (ZCMD(zone, cmd_no).command) {
            case 'P':
                if (ZCMD(zone, cmd_no).arg3 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                } else
                    ZCMD(zone, cmd_no).arg3 -= (ZCMD(zone, cmd_no).arg3 > rnum);
                break;
                save_this_zone = true;
            case 'O':
            case 'G':
            case 'E':
                if (ZCMD(zone, cmd_no).arg1 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                } else
                    ZCMD(zone, cmd_no).arg1 -= (ZCMD(zone, cmd_no).arg1 > rnum);
                break;
                save_this_zone = true;
            case 'R':
                if (ZCMD(zone, cmd_no).arg2 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                } else
                    ZCMD(zone, cmd_no).arg2 -= (ZCMD(zone, cmd_no).arg2 > rnum);
                break;
                save_this_zone = true;
            }
        }
        if (save_this_zone) {
            olc_add_to_save_list(zone_table[zone].number, OLC_SAVE_ZONE);
        }
    }

    olc_add_to_save_list(zone_table[zrnum].number, OLC_SAVE_OBJ);

    return true;
}

void copy_object(ObjData *to, ObjData *from) {
    free_obj_strings(to);
    *to = *from;
    to->name = from->name ? strdup(from->name) : nullptr;
    to->description = from->description ? strdup(from->description) : nullptr;
    to->short_description = from->short_description ? strdup(from->short_description) : nullptr;
    to->action_description = from->action_description ? strdup(from->action_description) : nullptr;

    if (from->ex_description)
        copy_extra_descriptions(&to->ex_description, from->ex_description);
    else
        to->ex_description = nullptr;
}

#define FREE(var)                                                                                                      \
    do {                                                                                                               \
        free(var);                                                                                                     \
        var = nullptr;                                                                                                 \
    } while (0)

void free_obj_strings_absolutely(ObjData *obj) {
    extern void free_extra_descriptions(ExtraDescriptionData * edesc);

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
        obj->ex_description = nullptr;
    }
}

void free_prototyped_obj_strings(ObjData *obj) {
    ObjData *proto = &obj_proto[GET_OBJ_RNUM(obj)];

    if (obj->name && obj->name != proto->name)
        FREE(obj->name);
    if (obj->description && obj->description != proto->description)
        FREE(obj->description);
    if (obj->short_description && obj->short_description != proto->short_description)
        FREE(obj->short_description);
    if (obj->action_description && obj->action_description != proto->action_description)
        FREE(obj->action_description);
    if (obj->ex_description) {
        ExtraDescriptionData *obj_ed, *proto_ed, *next_ed;
        bool ok_key, ok_desc, ok_item;
        /* O(horrible) */
        for (obj_ed = obj->ex_description; obj_ed; obj_ed = next_ed) {
            next_ed = obj_ed->next;
            /* If this obj_ed is in the proto's ex_desc list, don't free */
            for (ok_item = ok_key = ok_desc = true, proto_ed = proto->ex_description; proto_ed;
                 proto_ed = proto_ed->next) {
                if (proto_ed->keyword == obj_ed->keyword)
                    ok_key = false;
                if (proto_ed->description == obj_ed->description)
                    ok_desc = false;
                if (proto_ed == obj_ed)
                    ok_item = false;
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

void free_obj_strings(ObjData *obj) {
    if (GET_OBJ_RNUM(obj) == NOTHING)
        free_obj_strings_absolutely(obj);
    else
        free_prototyped_obj_strings(obj);
}

#undef FREE

void weight_change_object(ObjData *obj, float weight) {
    ObjData *tmp_obj;
    CharData *tmp_ch;

    if (obj->in_room != NOWHERE) {
        GET_OBJ_EFFECTIVE_WEIGHT(obj) += weight;
    } else if ((tmp_ch = obj->carried_by)) {
        obj_from_char(obj);
        GET_OBJ_EFFECTIVE_WEIGHT(obj) += weight;
        obj_to_char(obj, tmp_ch);
    } else if ((tmp_obj = obj->in_obj)) {
        obj_from_obj(obj);
        GET_OBJ_EFFECTIVE_WEIGHT(obj) += weight;
        obj_to_obj(obj, tmp_obj);
    } else {
        /* Object is not in the world yet. */
        GET_OBJ_EFFECTIVE_WEIGHT(obj) += weight;
    }
}

/* NAME_FROM_DRINKCON
 *
 * Called when a liquid container becomes empty.
 *
 * It will remove the first name from the object's aliases, if it is
 * the same as the name of the liquid.
 */

void name_from_drinkcon(ObjData *obj, int type) {
    int aliaslen;
    char *new_name;

    if (!VALID_LIQ_TYPE(type))
        return;

    aliaslen = strlen(LIQ_ALIAS(type));

    if (strlen(obj->name) > aliaslen + 1 &&
        /* The aliases are long enough to have more than just the drink name */
        (obj->name)[aliaslen] == ' ' &&
        /* An alias terminates just where the drink name should */
        !strncasecmp(LIQ_ALIAS(type), obj->name, aliaslen))
    /* Same string: we are go for removal */
    {
        new_name = strdup((obj->name) + aliaslen + 1);
        REPLACE_OBJ_STR(obj, name, new_name);
    }
}

void name_to_drinkcon(ObjData *obj, int type) {
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

void setup_drinkcon(ObjData *obj, int newliq) {
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
                log(LogSeverity::Warn, LVL_IMMORT,
                    "Error setting up drinkcon {:d}: expected 'l' or 'e' in adesc, but got #{:d}", GET_OBJ_VNUM(obj),
                    line[2]);
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
                    log(LogSeverity::Warn, LVL_IMMORT,
                        "Error setting up drinkcon {:d}: expected 'n', 's', or 'l' in adesc, but got #{:d}",
                        GET_OBJ_VNUM(obj), line[1]);
                    free(newtext);
                }
            }
        }
    ADESC_LINE:
        free(line);
    }
}

void liquid_from_container(ObjData *container, int amount) {
    int loss, remaining;
    float weight;

    if (GET_OBJ_TYPE(container) == ITEM_FOUNTAIN)
        return;

    if (GET_OBJ_TYPE(container) != ITEM_DRINKCON) {
        std::string container_name = GET_OBJ_NAME(container);
        log(LogSeverity::Error, LVL_IMMORT, "liquid_from_container asked to remove {:d} oz from {}", amount,
            container_name);
        return;
    }

    loss = std::min(amount, GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING));
    remaining = GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING) - loss;

    if (loss == 0) {
        log(LogSeverity::Error, LVL_IMMORT,
            "liquid_from_container is not removing any liquid from {} (requested: {:d})", GET_OBJ_NAME(container),
            amount);
        return;
    }

    GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING) = remaining;

    weight = LIQUID_MASS(loss, GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID));
    weight_change_object(container, -weight);

    if (remaining == 0) {
        GET_OBJ_VAL(container, VAL_DRINKCON_POISONED) = false;
        /* Since the container is empty, its strings could be changed. */
        setup_drinkcon(container, 0);
    }
}

void liquid_to_container(ObjData *container, int amount, int liquid_type, bool poisoned) {
    int change, final_amount;
    float weight;

    if (GET_OBJ_TYPE(container) == ITEM_FOUNTAIN)
        return;

    final_amount = std::min(amount + GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING),
                            GET_OBJ_VAL(container, VAL_DRINKCON_CAPACITY));
    change = final_amount - GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING);

    if (change < 0) {
        log(LogSeverity::Error, LVL_IMMORT, "liquid_to_container is not adding any liquid to {} (requested: {:d})",
            GET_OBJ_NAME(container), amount);
        return;
    }

    GET_OBJ_VAL(container, VAL_DRINKCON_REMAINING) = final_amount;
    GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID) = liquid_type;
    if (poisoned)
        GET_OBJ_VAL(container, VAL_DRINKCON_POISONED) = true;
    weight = LIQUID_MASS(change, GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID));
    weight_change_object(container, weight);
    setup_drinkcon(container, GET_OBJ_VAL(container, VAL_DRINKCON_LIQUID));
}

ObjData *carried_key(CharData *ch, int keyvnum) {
    ObjData *o;

    if (keyvnum < 0 || !ch)
        return nullptr;

    for (o = ch->carrying; o; o = o->next_content)
        if (GET_OBJ_VNUM(o) == keyvnum)
            return o;

    if (GET_EQ(ch, WEAR_HOLD))
        if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == keyvnum)
            return GET_EQ(ch, WEAR_HOLD);

    if (GET_EQ(ch, WEAR_HOLD2))
        if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD2)) == keyvnum)
            return GET_EQ(ch, WEAR_HOLD2);

    return nullptr;
}

float calculate_object_weight(ObjData *obj) {
    float weight = GET_OBJ_WEIGHT(obj);

    if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON || GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)
        weight += LIQUID_MASS(GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING), GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID));

    for (ObjData *tmp_obj = obj->contains; tmp_obj; tmp_obj = tmp_obj->next_content)
        weight += calculate_object_weight(tmp_obj);

    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
        float weight_reduction = GET_OBJ_VAL(obj, VAL_CONTAINER_WEIGHT_REDUCTION);
        if (weight_reduction > 0)
            weight -= GET_OBJ_EFFECTIVE_WEIGHT(obj) * (weight_reduction / 100.0);
    }

    return weight;
}
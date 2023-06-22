/***************************************************************************
 *   File: act.get.c                                      Part of FieryMUD *
 *  Usage: the get command                                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "act.hpp"

#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "money.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#define NULL_GCONTEXT                                                                                                  \
    {                                                                                                                  \
        nullptr, nullptr, { 0, 0, 0, 0 }                                                                               \
    }
#define INIT_GCONTEXT(ctx, ch) (ctx).c##h = (ch);

static const size_t MAX_POOL_SIZE = 25;
static size_t pool_size = 0;
static StackNode *pool = nullptr;

static StackNode *request_get_node() {
    StackNode *node;
    if (pool) {
        node = pool;
        pool = pool->next;
        memset(node, 0, sizeof(*node));
        --pool_size;
    } else
        CREATE(node, StackNode, 1);
    return node;
}

static void release_get_node(StackNode *node) {
    if (pool_size >= MAX_POOL_SIZE)
        free(node);
    else {
        node->next = pool;
        pool = node;
        ++pool_size;
    }
}

static bool messages_match(StackNode *node, ObjData *obj, const char *to_char, const char *to_room) {
    ObjData *incumbent = node->obj;

#define BOTH_OBJ_TYPES_MATCH(a, b, type) (GET_OBJ_TYPE(a) == (type) && GET_OBJ_TYPE(b) == (type))

    if (incumbent->item_number == obj->item_number || BOTH_OBJ_TYPES_MATCH(incumbent, obj, ITEM_MONEY))
        if (BOTH_OBJ_TYPES_MATCH(incumbent, obj, ITEM_MONEY) ||
            incumbent->short_description == obj->short_description ||
            !strcasecmp(incumbent->short_description, obj->short_description))
            if (node->to_char == to_char || !node->to_char || !strcasecmp(node->to_char, to_char))
                if (node->to_room == to_room || !node->to_room || !strcasecmp(node->to_room, to_room))
                    return true;
    return false;

#undef BOTH_OBJ_TYPES_MATCH
}

static void queue_message(GetContext *context, ObjData *obj, bool keep_count, const char *to_char,
                          const char *to_room) {
    StackNode *node = context->stack;
    while (node) {
        if (messages_match(node, obj, to_char, to_room)) {
            if (node->count)
                node->count++;
            return;
        }
        node = node->next;
    }
    node = request_get_node();
    node->obj = obj;
    node->count = keep_count ? 1 : 0;
    node->to_char = to_char;
    node->to_room = to_room;
    node->next = context->stack;
    context->stack = node;
}

static void process_get_context(GetContext *context, ObjData *vict_obj) {
    char buf[MAX_INPUT_LENGTH * 3];
    StackNode *node, *next, *reverse = nullptr;

    /* Reverse the order of the stack */
    node = context->stack;
    while (node) {
        next = node->next;
        node->next = reverse;
        reverse = node;
        node = next;
    }

    node = reverse;
    while (node) {
        if (node->obj) {
            if (node->to_char) {
                if (node->count > 1)
                    sprintf(buf, "%s (x%d)", node->to_char, node->count);
                act(node->count <= 1 ? node->to_char : buf, false, context->ch, node->obj, vict_obj, TO_CHAR);
            }
            if (node->to_room) {
                if (node->count > 1)
                    sprintf(buf, "%s (multiple)", node->to_room);
                act(node->count <= 1 ? node->to_room : buf, !HIGHLY_VISIBLE(node->obj) || GET_INVIS_LEV(context->ch),
                    context->ch, node->obj, vict_obj, TO_ROOM);
            }
        }
        next = node->next;
        release_get_node(node);
        node = next;
    }
    context->stack = nullptr;

    if (PRF_FLAGGED(context->ch, PRF_AUTOSPLIT) && IS_GROUPED(context->ch) && CASH_VALUE(context->coins))
        split_coins(context->ch, context->coins, FAIL_SILENTLY);
    context->coins[PLATINUM] = 0;
    context->coins[GOLD] = 0;
    context->coins[SILVER] = 0;
    context->coins[COPPER] = 0;
}

GetContext *begin_get_transaction(CharData *ch) {
    GetContext *context;
    CREATE(context, GetContext, 1);
    context->ch = ch;
    return context;
}

void end_get_transaction(GetContext *context, ObjData *vict_obj) {
    process_get_context(context, vict_obj);
    free(context);
}

/* This function checks to see if the player ch has the consent of  */
/* the owner of the corpse cont. or is the owner of the corpse cont */
/* Returns true if so.             gurlaek 8/7/1999                 */
bool has_corpse_consent(CharData *ch, ObjData *cont) {
    DescriptorData *d;

    if (GET_LEVEL(ch) >= LVL_GOD) {
        /* If you are a god, then you always have consent */
        return true;
    }

    sprintf(buf, "corpse %s", GET_NAME(ch));
    if (!strcasecmp(buf, cont->name)) {
        /* if it's your own corpse then you have consent */
        return true;
    }

    if (REAL_CHAR(ch) != ch) {
        sprintf(buf, "corpse %s", GET_NAME(REAL_CHAR(ch)));
        if (!strcasecmp(buf, cont->name)) {
            /* if you're shapeshifted and it's your own corpse */
            return true;
        }
    } else {
        /* loop through all the connected descriptors */
        for (d = descriptor_list; d; d = d->next) {
            if (d->character) {
                /* compare the name of the corpse to the current d->character  */
                /* if the d->character is consented to the person who's acting */
                /* on the corpse then the act is allowed.                      */
                sprintf(buf, "corpse %s", GET_NAME(d->character));
                if (!strcasecmp(buf, cont->name) && CONSENT(d->character) == ch) {
                    return true;
                }
            }
        }
    }

    /* no consent and not your own corpse */
    char_printf(ch, "Not without consent you don't!\n");

    return false;
}

static bool can_get_obj(GetContext *context, ObjData *obj) {
    CharData *ch = context->ch;
    if (GET_LEVEL(ch) >= LVL_GOD)
        return true;
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
        queue_message(context, obj, true, "$p: you can't carry that many items.", nullptr);
    else if (!ADDED_WEIGHT_OK(ch, obj))
        queue_message(context, obj, true, "$p: you can't carry that much weight.", nullptr);
    else if (!CAN_WEAR(obj, ITEM_WEAR_TAKE))
        queue_message(context, obj, true, "$p: you can't take that!", nullptr);
    else if ((GET_OBJ_LEVEL(obj) > GET_LEVEL(ch) + 15) || (GET_OBJ_LEVEL(obj) > 99 && GET_LEVEL(ch) < 100))
        queue_message(context, obj, true, "You need more experience to lift the awesome might of $p!", nullptr);
    else
        return true;
    return false;
}

/*
 * Public can_get_obj function.  Creates a temporary get context.
 */
bool can_take_obj(CharData *ch, ObjData *obj) {
    GetContext context = NULL_GCONTEXT;
    bool allowed;
    INIT_GCONTEXT(context, ch);
    allowed = can_get_obj(&context, obj);
    process_get_context(&context, nullptr);
    return allowed;
}

static void unhide_object(ObjData *obj) {
    if (GET_OBJ_HIDDENNESS(obj)) {
        GET_OBJ_HIDDENNESS(obj) = 0;
        if (!OBJ_FLAGGED(obj, ITEM_WAS_DISARMED))
            obj->last_to_hold = nullptr;
    }
}

static EVENTFUNC(get_money_event) {
    ObjData *obj = (ObjData *)event_obj;
    extract_obj(obj);
    return EVENT_FINISHED;
}

static void perform_get_check_money(GetContext *context, ObjData *obj) {
    static char buf[MAX_INPUT_LENGTH] = {0};
    bool prior_value = false;
    CharData *ch = context->ch;

    if (GET_OBJ_TYPE(obj) != ITEM_MONEY)
        return;

    /* Check for negative values */
    if (GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM) < 0)
        GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM) = 0;
    if (GET_OBJ_VAL(obj, VAL_MONEY_GOLD) < 0)
        GET_OBJ_VAL(obj, VAL_MONEY_GOLD) = 0;
    if (GET_OBJ_VAL(obj, VAL_MONEY_SILVER) < 0)
        GET_OBJ_VAL(obj, VAL_MONEY_SILVER) = 0;
    if (GET_OBJ_VAL(obj, VAL_MONEY_COPPER) < 0)
        GET_OBJ_VAL(obj, VAL_MONEY_COPPER) = 0;

    obj_from_char(obj);
    if (CASH_VALUE(context->coins))
        prior_value = true;
    context->coins[VAL_MONEY_PLATINUM] += GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM);
    context->coins[VAL_MONEY_GOLD] += GET_OBJ_VAL(obj, VAL_MONEY_GOLD);
    context->coins[VAL_MONEY_SILVER] += GET_OBJ_VAL(obj, VAL_MONEY_SILVER);
    context->coins[VAL_MONEY_COPPER] += GET_OBJ_VAL(obj, VAL_MONEY_COPPER);
    if (CASH_VALUE(context->coins) == 0)
        strcpy(buf,
               "There were 0 coins.\n"
               "Must have been an illusion!");
    else {
        strcpy(buf, prior_value ? "There was a total of " : "There were ");
        statemoney(buf + strlen(buf), context->coins);
        strcat(buf, ".");
    }
    queue_message(context, obj, false, buf, nullptr);
    GET_PLATINUM(ch) += GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM);
    GET_GOLD(ch) += GET_OBJ_VAL(obj, VAL_MONEY_GOLD);
    GET_SILVER(ch) += GET_OBJ_VAL(obj, VAL_MONEY_SILVER);
    GET_COPPER(ch) += GET_OBJ_VAL(obj, VAL_MONEY_COPPER);
    GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM) = 0;
    GET_OBJ_VAL(obj, VAL_MONEY_GOLD) = 0;
    GET_OBJ_VAL(obj, VAL_MONEY_SILVER) = 0;
    GET_OBJ_VAL(obj, VAL_MONEY_COPPER) = 0;
    event_create(EVENT_GET_MONEY, get_money_event, obj, false, &obj->events, 0);
}

/*
 * Public get_check_money function.  Creates a temporary get context.
 */
void get_check_money(CharData *ch, ObjData *obj) {
    GetContext context = NULL_GCONTEXT;
    INIT_GCONTEXT(context, ch);
    perform_get_check_money(&context, obj);
    process_get_context(&context, nullptr);
}

void perform_get_from_container(GetContext *context, ObjData *obj, ObjData *cont) {
    CharData *ch = context->ch;
    if (cont->carried_by == ch || cont->worn_by == ch || can_get_obj(context, obj)) {
        if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
            queue_message(context, obj, true, "$p: you can't hold any more items.", nullptr);
        else if (get_otrigger(obj, ch, cont)) {
            queue_message(context, obj, true, "You get $p from $P.", "$n gets $p from $P.");
            if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) {
                obj_from_obj(obj);
                obj_to_char(obj, ch);
                unhide_object(obj);
                if (IS_PLR_CORPSE(cont))
                    log("CORPSE: {} gets {} from {}.", GET_NAME(ch), strip_ansi(obj->short_description),
                        cont->short_description);
                perform_get_check_money(context, obj);
            }
        }
    }
}

void get_from_container(CharData *ch, ObjData *cont, char *name, int *amount) {
    ObjData *obj;
    int obj_dotmode, found = 0;
    GetContext context = NULL_GCONTEXT;
    obj_iterator iter;

    INIT_GCONTEXT(context, ch);

    obj_dotmode = find_all_dots(&name);

    if (IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
        act("$p is closed.", false, ch, cont, nullptr, TO_CHAR);
    else if (!RIGID(ch) && cont->carried_by != ch && GET_LEVEL(ch) < LVL_IMMORT)
        char_printf(ch, "You can't handle solid objects in your condition.\n");
    else {
        iter = find_objs_in_list(cont->contains, obj_dotmode == FIND_ALL ? find_vis(ch) : find_vis_by_name(ch, name));
        while ((obj = NEXT_FUNC(iter))) {
            ++found;
            perform_get_from_container(&context, obj, cont);
            if (obj_dotmode == FIND_INDIV) {
                if (!amount || *amount == -1 || --*amount <= 0)
                    break;
            }
        }

        process_get_context(&context, cont);

        if (!found) {
            if (obj_dotmode == FIND_ALL)
                act("$p seems to be empty.", false, ch, cont, nullptr, TO_CHAR);
            else {
                sprintf(buf, "You can't seem to find any %s%s in $p.", name, isplural(name) ? "" : "s");
                act(buf, false, ch, cont, nullptr, TO_CHAR);
            }
        }
    }
}

void perform_get_from_room(GetContext *context, ObjData *obj) {
    CharData *ch = context->ch;

    if (!check_get_disarmed_obj(ch, obj->last_to_hold, obj))
        if (can_get_obj(context, obj))
            if (get_otrigger(obj, ch, nullptr)) {
                queue_message(context, obj, true, "You get $p.", "$n gets $p.");
                if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) {
                    obj_from_room(obj);
                    obj_to_char(obj, ch);
                    unhide_object(obj);
                    perform_get_check_money(context, obj);
                }
            }
}

void get_random_object(GetContext *context) {
    ObjData *obj, *chosen = nullptr;
    int count = 0;
    CharData *ch = context->ch;

    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
        if (CAN_SEE_OBJ(ch, obj)) {
            if (chosen == nullptr || random_number(0, count) == 0)
                chosen = obj;
            count++;
        }
    }

    if (chosen)
        perform_get_from_room(context, chosen);
}

void get_from_room(CharData *ch, char *name, int amount) {
    ObjData *obj;
    int dotmode, found = 0;
    bool confused = false;
    obj_iterator iter;
    GetContext context = NULL_GCONTEXT;

    INIT_GCONTEXT(context, ch);

    if (!RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "In your fluid state, you can't pick things up.\n");
        return;
    }

    if (CONFUSED(ch) && random_number(0, 1) == 0)
        confused = true;

    dotmode = find_all_dots(&name);
    iter =
        find_objs_in_list(world[ch->in_room].contents, dotmode == FIND_ALL ? find_vis(ch) : find_vis_by_name(ch, name));
    while ((obj = NEXT_FUNC(iter))) {
        ++found;
        if (confused) {
            char_printf(ch, "&5You fumble haphazardly...&0\n");
            get_random_object(&context);
            /* Note: this loop cannot be allowed to continue, because the
             * iterator may have been invalided by get_random_object().
             * It's ok though, because when you're confused, you're not allowed
             * to use "FIND_ALLDOT" and such - so you only wanted to get one
             * thing anyway. */
            break;
        }
        perform_get_from_room(&context, obj);
        if (dotmode == FIND_INDIV && --amount <= 0)
            break;
    }

    process_get_context(&context, nullptr);

    if (!found) {
        if (dotmode == FIND_ALL)
            char_printf(ch, "There doesn't seem to be anything here.\n");
        else
            char_printf(ch, "You don't see any {}{} here.\n", name, isplural(name) ? "" : "s");
    } else if (!confused && dotmode == FIND_INDIV && amount > 0)
        char_printf(ch, "There {} only {} {}{}.\n", found == 1 ? "was" : "were", found, name,
                    !isplural(name) && found > 1 ? "s" : "");
}

ACMD(do_get) {
    char buf1[MAX_INPUT_LENGTH], *obj_name = buf1;
    char buf2[MAX_INPUT_LENGTH], *cont_name = buf2;
    ObjData *cont;
    obj_iterator iter;
    int amount = -1, orig_amt, found = 0, cont_dotmode;

    argument = one_argument(argument, obj_name);
    if (is_number(obj_name)) {
        amount = atoi(obj_name);
        argument = one_argument(argument, obj_name);
    }
    one_argument(argument, cont_name);

    if (!*obj_name)
        char_printf(ch, "Get what?\n");
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) && GET_LEVEL(ch) < LVL_IMMORT)
        char_printf(ch, "Your arms are already full!\n");
    else if (!amount)
        char_printf(ch, "So you don't want to get anything?\n");
    else if (!strcasecmp(obj_name, "all."))
        char_printf(ch, "Get all of what?\n");
    else if (!*cont_name)
        get_from_room(ch, obj_name, amount);
    else if (CONFUSED(ch))
        char_printf(ch, "You're too confused!\n");
    else if ((cont_dotmode = find_all_dots(&cont_name)) == FIND_ALLDOT && !*cont_name)
        char_printf(ch, "Get from all of what?\n");
    else {
        iter = find_objs(cont_dotmode == FIND_ALL ? find_vis(ch) : find_vis_by_name(ch, cont_name),
                         FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP);
        orig_amt = amount;
        while ((cont = NEXT_FUNC(iter))) {
            if (cont_dotmode != FIND_ALL)
                ++found;
            if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER) {
                if (cont_dotmode != FIND_ALL)
                    act("$p is not a container.", false, ch, cont, 0, TO_CHAR);
            } else if (!IS_PLR_CORPSE(cont) || has_corpse_consent(ch, cont)) {
                if (cont_dotmode == FIND_ALL)
                    ++found;
                get_from_container(ch, cont, obj_name, &amount);
            }
            if (amount == 0 || cont_dotmode == FIND_INDIV)
                break;
        }
        if (found == 0) {
            if (cont_dotmode == FIND_ALL)
                char_printf(ch, "There are no containers here.\n");
            else
                char_printf(ch, "There are no {}{}.\n", cont_name, isplural(cont_name) ? "" : "s");
        } else if (amount > 0) {
            amount = orig_amt - amount;
            if (amount > 0)
                char_printf(ch, "There {} only {} {}{}.\n", amount == 1 ? "was" : "were", amount, obj_name,
                            !isplural(obj_name) && amount > 1 ? "s" : "");
        }
    }
}

ACMD(do_palm) {
    char argbuf1[MAX_INPUT_LENGTH], *arg1 = argbuf1;
    char argbuf2[MAX_INPUT_LENGTH], *arg2 = argbuf2;
    int obj_dotmode, cont_dotmode, cont_mode, roll = 100;
    ObjData *cont, *obj;
    CharData *tch;

    if (CONFUSED(ch)) {
        char_printf(ch, "You are too confused.\n");
        return;
    }

    two_arguments(argument, arg1, arg2);

    obj_dotmode = find_all_dots(&arg1);
    cont_dotmode = find_all_dots(&arg2);

    if (!GET_SKILL(ch, SKILL_CONCEAL))
        do_get(ch, argument, cmd, 0);
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) && GET_LEVEL(ch) < LVL_GOD)
        char_printf(ch, "Your arms are already full!\n");
    else if (!*arg1)
        char_printf(ch, "Palm what?\n");
    else if (obj_dotmode != FIND_INDIV)
        char_printf(ch, "You can only palm one item at a time!\n");
    else if (cont_dotmode != FIND_INDIV)
        char_printf(ch, "You can only palm an item from one container at a time!\n");
    /* No container - palm from room */
    else if (!*arg2) {
        if (!(obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg1))))
            char_printf(ch, "You don't see {} {} here.\n", AN(arg1), arg1);
        else if (!check_get_disarmed_obj(ch, obj->last_to_hold, obj) && can_take_obj(ch, obj) &&
                 get_otrigger(obj, ch, nullptr)) {
            int people = 0;
            roll = conceal_roll(ch, obj);
            obj_from_room(obj);
            obj_to_char(obj, ch);

            if (!SOLIDCHAR(ch) && GET_LEVEL(ch) < LVL_IMMORT)
                sprintf(buf,
                        "%s tries to snatch $p off the ground, but seems to be having "
                        "difficulty.",
                        GET_NAME(ch));
            else
                sprintf(buf, "%s coyly snatches $p off the ground.", GET_NAME(ch));
            LOOP_THRU_PEOPLE(tch, ch) {
                if (tch == ch)
                    continue;
                if (CAN_SEE(ch, tch)) {
                    ++people;
                    if (GET_PERCEPTION(tch) < roll - 50 + random_number(0, 50))
                        continue;
                } else if (GET_PERCEPTION(tch) < roll / 2)
                    continue;
                if (!CAN_SEE(tch, ch))
                    continue;
                if (!CAN_SEE_OBJ(tch, obj) && random_number(0, 1))
                    continue;
                act(buf, true, tch, obj, cont, TO_CHAR);
            }
            if (!SOLIDCHAR(ch) && GET_LEVEL(ch) < LVL_IMMORT)
                act("You casually reach for $p... but you can't get a grip on it!", false, ch, obj, 0, TO_CHAR);
            else {
                if (people)
                    act("You palm $p when you think no one is looking.", false, ch, obj, 0, TO_CHAR);
                else
                    act("You smoothly palm $p.", false, ch, obj, 0, TO_CHAR);
                unhide_object(obj);
                get_check_money(ch, obj);
                WAIT_STATE(ch, PULSE_VIOLENCE);
                improve_skill(ch, SKILL_CONCEAL);
            }
        }
    } else {
        cont_mode = generic_find(arg2, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tch, &cont);
        if (!cont)
            char_printf(ch, "You can't find {} {}.\n", AN(arg2), arg2);
        else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
            act("$p is not a container.", false, ch, cont, nullptr, TO_CHAR);
        else if (IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
            act("$p is closed.", false, ch, cont, nullptr, TO_CHAR);
        else if (!IS_PLR_CORPSE(cont) || has_corpse_consent(ch, cont)) {
            if (!(obj = find_obj_in_list(cont->contains, find_vis_by_name(ch, arg1)))) {
                sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg1), arg1);
                act(buf, false, ch, cont, nullptr, TO_CHAR);
            } else if (cont_mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
                if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
                    act("$p: you can't hold any more items.", false, ch, obj, nullptr, TO_CHAR);
                else if (get_otrigger(obj, ch, cont)) {

                    if (SOLIDCHAR(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
                        roll = conceal_roll(ch, obj);
                        obj_from_obj(obj);
                        obj_to_char(obj, ch);
                        act("You quietly palm $p from $P.", false, ch, obj, cont, TO_CHAR);
                        sprintf(buf, "%s quietly slips $p out of $P.", GET_NAME(ch));
                    } else {
                        act("You quietly reach into $P, but you can't seem to grasp $p.", false, ch, obj, cont,
                            TO_CHAR);
                        sprintf(buf, "%s tries to slip $p out of $P, but is having trouble.", GET_NAME(ch));
                    }

                    LOOP_THRU_PEOPLE(tch, ch) {
                        if (tch == ch)
                            continue;
                        if (!CAN_SEE(tch, ch))
                            continue;
                        if (!CAN_SEE_OBJ(tch, obj) && random_number(0, 1))
                            continue;
                        if (CAN_SEE(ch, tch) ? (GET_PERCEPTION(tch) < roll - 50 + random_number(0, 50))
                                             : (GET_PERCEPTION(tch) < roll / 2))
                            continue;
                        act(buf, true, tch, obj, cont, TO_CHAR);
                    }
                    if (SOLIDCHAR(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
                        unhide_object(obj);
                        if (IS_PLR_CORPSE(cont))
                            log("CORPSE: {} palms {} from {}.", GET_NAME(ch), strip_ansi(obj->short_description),
                                cont->short_description);
                        get_check_money(ch, obj);
                        WAIT_STATE(ch, PULSE_VIOLENCE);
                        improve_skill(ch, SKILL_CONCEAL);
                    }
                }
            }
        }
    }
}
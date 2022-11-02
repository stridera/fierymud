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
#include "math.hpp"
#include "money.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

struct get_context {
    struct char_data *ch;
    struct stack_node {
        struct obj_data *obj;
        int count;
        const char *to_char;
        const char *to_room;
        struct stack_node *next;
    } *stack;
    int coins[NUM_COIN_TYPES];
};

#define NULL_GCONTEXT                                                                                                  \
    {                                                                                                                  \
        NULL, NULL, { 0, 0, 0, 0 }                                                                                     \
    }
#define INIT_GCONTEXT(ctx, ch) (ctx).c##h = (ch);

static const size_t MAX_POOL_SIZE = 25;
static size_t pool_size = 0;
static stack_node *pool = NULL;

static stack_node *request_get_node() {
    stack_node *node;
    if (pool) {
        node = pool;
        pool = pool->next;
        memset(node, 0, sizeof(*node));
        --pool_size;
    } else
        CREATE(node, stack_node, 1);
    return node;
}

static void release_get_node(stack_node *node) {
    if (pool_size >= MAX_POOL_SIZE)
        free(node);
    else {
        node->next = pool;
        pool = node;
        ++pool_size;
    }
}

static bool messages_match(stack_node *node, obj_data *obj, const char *to_char, const char *to_room) {
    obj_data *incumbent = node->obj;

#define BOTH_OBJ_TYPES_MATCH(a, b, type) (GET_OBJ_TYPE(a) == (type) && GET_OBJ_TYPE(b) == (type))

    if (incumbent->item_number == obj->item_number || BOTH_OBJ_TYPES_MATCH(incumbent, obj, ITEM_MONEY))
        if (BOTH_OBJ_TYPES_MATCH(incumbent, obj, ITEM_MONEY) ||
            incumbent->short_description == obj->short_description ||
            !strcmp(incumbent->short_description, obj->short_description))
            if (node->to_char == to_char || !node->to_char || !strcmp(node->to_char, to_char))
                if (node->to_room == to_room || !node->to_room || !strcmp(node->to_room, to_room))
                    return TRUE;
    return FALSE;

#undef BOTH_OBJ_TYPES_MATCH
}

static void queue_message(get_context *context, obj_data *obj, bool keep_count, const char *to_char,
                          const char *to_room) {
    stack_node *node = context->stack;
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

static void process_get_context(get_context *context, const void *vict_obj) {
    char buf[MAX_INPUT_LENGTH * 3];
    struct stack_node *node, *next, *reverse = NULL;

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
                act(node->count <= 1 ? node->to_char : buf, FALSE, context->ch, node->obj, vict_obj, TO_CHAR);
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
    context->stack = NULL;

    if (PRF_FLAGGED(context->ch, PRF_AUTOSPLIT) && IS_GROUPED(context->ch) && CASH_VALUE(context->coins))
        split_coins(context->ch, context->coins, FAIL_SILENTLY);
    context->coins[PLATINUM] = 0;
    context->coins[GOLD] = 0;
    context->coins[SILVER] = 0;
    context->coins[COPPER] = 0;
}

struct get_context *begin_get_transaction(char_data *ch) {
    struct get_context *context;
    CREATE(context, get_context, 1);
    context->ch = ch;
    return context;
}

void end_get_transaction(get_context *context, const void *vict_obj) {
    process_get_context(context, vict_obj);
    free(context);
}

/* This function checks to see if the player ch has the consent of  */
/* the owner of the corpse cont. or is the owner of the corpse cont */
/* Returns TRUE if so.             gurlaek 8/7/1999                 */
bool has_corpse_consent(char_data *ch, obj_data *cont) {
    struct descriptor_data *d;

    if (GET_LEVEL(ch) >= LVL_GOD) {
        /* If you are a god, then you always have consent */
        return TRUE;
    }

    sprintf(buf, "corpse %s", GET_NAME(ch));
    if (!str_cmp(buf, cont->name)) {
        /* if it's your own corpse then you have consent */
        return TRUE;
    }

    if (REAL_CHAR(ch) != ch) {
        sprintf(buf, "corpse %s", GET_NAME(REAL_CHAR(ch)));
        if (!str_cmp(buf, cont->name)) {
            /* if you're shapeshifted and it's your own corpse */
            return TRUE;
        }
    } else {
        /* loop through all the connected descriptors */
        for (d = descriptor_list; d; d = d->next) {
            if (d->character) {
                /* compare the name of the corpse to the current d->character  */
                /* if the d->character is consented to the person who's acting */
                /* on the corpse then the act is allowed.                      */
                sprintf(buf, "corpse %s", GET_NAME(d->character));
                if (!str_cmp(buf, cont->name) && CONSENT(d->character) == ch) {
                    return TRUE;
                }
            }
        }
    }

    /* no consent and not your own corpse */
    send_to_char("Not without consent you don't!\r\n", ch);

    return FALSE;
}

static bool can_get_obj(get_context *context, obj_data *obj) {
    struct char_data *ch = context->ch;
    if (GET_LEVEL(ch) >= LVL_GOD)
        return TRUE;
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
        queue_message(context, obj, TRUE, "$p: you can't carry that many items.", NULL);
    else if (!ADDED_WEIGHT_OK(ch, obj))
        queue_message(context, obj, TRUE, "$p: you can't carry that much weight.", NULL);
    else if (!CAN_WEAR(obj, ITEM_WEAR_TAKE))
        queue_message(context, obj, TRUE, "$p: you can't take that!", NULL);
    else if ((GET_OBJ_LEVEL(obj) > GET_LEVEL(ch) + 15) || (GET_OBJ_LEVEL(obj) > 99 && GET_LEVEL(ch) < 100))
        queue_message(context, obj, TRUE, "You need more experience to lift the awesome might of $p!", NULL);
    else
        return TRUE;
    return FALSE;
}

/*
 * Public can_get_obj function.  Creates a temporary get context.
 */
bool can_take_obj(char_data *ch, obj_data *obj) {
    struct get_context context = NULL_GCONTEXT;
    bool allowed;
    INIT_GCONTEXT(context, ch);
    allowed = can_get_obj(&context, obj);
    process_get_context(&context, NULL);
    return allowed;
}

static void unhide_object(obj_data *obj) {
    if (GET_OBJ_HIDDENNESS(obj)) {
        GET_OBJ_HIDDENNESS(obj) = 0;
        if (!OBJ_FLAGGED(obj, ITEM_WAS_DISARMED))
            obj->last_to_hold = NULL;
    }
}

static EVENTFUNC(get_money_event) {
    struct obj_data *obj = (obj_data *)event_obj;
    extract_obj(obj);
    return EVENT_FINISHED;
}

static void perform_get_check_money(get_context *context, obj_data *obj) {
    static char buf[MAX_INPUT_LENGTH] = {0};
    bool prior_value = FALSE;
    struct char_data *ch = context->ch;

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
        prior_value = TRUE;
    context->coins[VAL_MONEY_PLATINUM] += GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM);
    context->coins[VAL_MONEY_GOLD] += GET_OBJ_VAL(obj, VAL_MONEY_GOLD);
    context->coins[VAL_MONEY_SILVER] += GET_OBJ_VAL(obj, VAL_MONEY_SILVER);
    context->coins[VAL_MONEY_COPPER] += GET_OBJ_VAL(obj, VAL_MONEY_COPPER);
    if (CASH_VALUE(context->coins) == 0)
        strcpy(buf,
               "There were 0 coins.\r\n"
               "Must have been an illusion!");
    else {
        strcpy(buf, prior_value ? "There was a total of " : "There were ");
        statemoney(buf + strlen(buf), context->coins);
        strcat(buf, ".");
    }
    queue_message(context, obj, FALSE, buf, NULL);
    GET_PLATINUM(ch) += GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM);
    GET_GOLD(ch) += GET_OBJ_VAL(obj, VAL_MONEY_GOLD);
    GET_SILVER(ch) += GET_OBJ_VAL(obj, VAL_MONEY_SILVER);
    GET_COPPER(ch) += GET_OBJ_VAL(obj, VAL_MONEY_COPPER);
    GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM) = 0;
    GET_OBJ_VAL(obj, VAL_MONEY_GOLD) = 0;
    GET_OBJ_VAL(obj, VAL_MONEY_SILVER) = 0;
    GET_OBJ_VAL(obj, VAL_MONEY_COPPER) = 0;
    event_create(EVENT_GET_MONEY, get_money_event, obj, FALSE, &obj->events, 0);
}

/*
 * Public get_check_money function.  Creates a temporary get context.
 */
void get_check_money(char_data *ch, obj_data *obj) {
    struct get_context context = NULL_GCONTEXT;
    INIT_GCONTEXT(context, ch);
    perform_get_check_money(&context, obj);
    process_get_context(&context, NULL);
}

void perform_get_from_container(get_context *context, obj_data *obj, obj_data *cont) {
    struct char_data *ch = context->ch;
    if (cont->carried_by == ch || cont->worn_by == ch || can_get_obj(context, obj)) {
        if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
            queue_message(context, obj, TRUE, "$p: you can't hold any more items.", NULL);
        else if (get_otrigger(obj, ch)) {
            queue_message(context, obj, TRUE, "You get $p from $P.", "$n gets $p from $P.");
            if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) {
                obj_from_obj(obj);
                obj_to_char(obj, ch);
                unhide_object(obj);
                if (IS_PLR_CORPSE(cont))
                    log("CORPSE: %s gets %s from %s.", GET_NAME(ch), strip_ansi(obj->short_description),
                        cont->short_description);
                perform_get_check_money(context, obj);
            }
        }
    }
}

void get_from_container(char_data *ch, obj_data *cont, char *name, int *amount) {
    struct obj_data *obj;
    int obj_dotmode, found = 0;
    struct get_context context = NULL_GCONTEXT;
    struct obj_iterator iter;

    INIT_GCONTEXT(context, ch);

    obj_dotmode = find_all_dots(&name);

    if (IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
        act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
    else if (!RIGID(ch) && cont->carried_by != ch && GET_LEVEL(ch) < LVL_IMMORT)
        send_to_char("You can't handle solid objects in your condition.\r\n", ch);
    else {
        iter = find_objs_in_list(cont->contains, obj_dotmode == FIND_ALL ? find_vis(ch) : find_vis_by_name(ch, name));
        while ((obj = next(iter))) {
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
                act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
            else {
                sprintf(buf, "You can't seem to find any %s%s in $p.", name, isplural(name) ? "" : "s");
                act(buf, FALSE, ch, cont, 0, TO_CHAR);
            }
        }
    }
}

void perform_get_from_room(get_context *context, obj_data *obj) {
    struct char_data *ch = context->ch;

    if (!check_get_disarmed_obj(ch, obj->last_to_hold, obj))
        if (can_get_obj(context, obj))
            if (get_otrigger(obj, ch)) {
                queue_message(context, obj, TRUE, "You get $p.", "$n gets $p.");
                if (!MOB_FLAGGED(ch, MOB_ILLUSORY)) {
                    obj_from_room(obj);
                    obj_to_char(obj, ch);
                    unhide_object(obj);
                    perform_get_check_money(context, obj);
                }
            }
}

void get_random_object(get_context *context) {
    struct obj_data *obj, *chosen = NULL;
    int count = 0;
    struct char_data *ch = context->ch;

    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
        if (CAN_SEE_OBJ(ch, obj)) {
            if (chosen == NULL || number(0, count) == 0)
                chosen = obj;
            count++;
        }
    }

    if (chosen)
        perform_get_from_room(context, chosen);
}

void get_from_room(char_data *ch, char *name, int amount) {
    struct obj_data *obj;
    int dotmode, found = 0;
    bool confused = FALSE;
    struct obj_iterator iter;
    struct get_context context = NULL_GCONTEXT;

    INIT_GCONTEXT(context, ch);

    if (!RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("In your fluid state, you can't pick things up.\r\n", ch);
        return;
    }

    if (CONFUSED(ch) && number(0, 1) == 0)
        confused = TRUE;

    dotmode = find_all_dots(&name);
    iter =
        find_objs_in_list(world[ch->in_room].contents, dotmode == FIND_ALL ? find_vis(ch) : find_vis_by_name(ch, name));
    while ((obj = next(iter))) {
        ++found;
        if (confused) {
            send_to_char("&5You fumble haphazardly...&0\r\n", ch);
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

    process_get_context(&context, NULL);

    if (!found) {
        if (dotmode == FIND_ALL)
            send_to_char("There doesn't seem to be anything here.\r\n", ch);
        else
            cprintf(ch, "You don't see any %s%s here.\r\n", name, isplural(name) ? "" : "s");
    } else if (!confused && dotmode == FIND_INDIV && amount > 0)
        cprintf(ch, "There %s only %d %s%s.\r\n", found == 1 ? "was" : "were", found, name,
                !isplural(name) && found > 1 ? "s" : "");
}

ACMD(do_get) {
    char buf1[MAX_INPUT_LENGTH], *obj_name = buf1;
    char buf2[MAX_INPUT_LENGTH], *cont_name = buf2;
    struct obj_data *cont;
    struct obj_iterator iter;
    int amount = -1, orig_amt, found = 0, cont_dotmode;

    argument = one_argument(argument, obj_name);
    if (is_number(obj_name)) {
        amount = atoi(obj_name);
        argument = one_argument(argument, obj_name);
    }
    one_argument(argument, cont_name);

    if (!*obj_name)
        send_to_char("Get what?\r\n", ch);
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) && GET_LEVEL(ch) < LVL_IMMORT)
        send_to_char("Your arms are already full!\r\n", ch);
    else if (!amount)
        send_to_char("So you don't want to get anything?\r\n", ch);
    else if (!strcmp(obj_name, "all."))
        send_to_char("Get all of what?\r\n", ch);
    else if (!*cont_name)
        get_from_room(ch, obj_name, amount);
    else if (CONFUSED(ch))
        send_to_char("You're too confused!\r\n", ch);
    else if ((cont_dotmode = find_all_dots(&cont_name)) == FIND_ALLDOT && !*cont_name)
        send_to_char("Get from all of what?\r\n", ch);
    else {
        iter = find_objs(cont_dotmode == FIND_ALL ? find_vis(ch) : find_vis_by_name(ch, cont_name),
                         FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP);
        orig_amt = amount;
        while ((cont = next(iter))) {
            if (cont_dotmode != FIND_ALL)
                ++found;
            if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER) {
                if (cont_dotmode != FIND_ALL)
                    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
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
                cprintf(ch, "There are no containers here.\r\n");
            else
                cprintf(ch, "There are no %s%s.\r\n", cont_name, isplural(cont_name) ? "" : "s");
        } else if (amount > 0) {
            amount = orig_amt - amount;
            if (amount > 0)
                cprintf(ch, "There %s only %d %s%s.\r\n", amount == 1 ? "was" : "were", amount, obj_name,
                        !isplural(obj_name) && amount > 1 ? "s" : "");
        }
    }
}

ACMD(do_palm) {
    char argbuf1[MAX_INPUT_LENGTH], *arg1 = argbuf1;
    char argbuf2[MAX_INPUT_LENGTH], *arg2 = argbuf2;
    int obj_dotmode, cont_dotmode, cont_mode, roll = 100;
    struct obj_data *cont, *obj;
    struct char_data *tch;

    if (CONFUSED(ch)) {
        send_to_char("You are too confused.\r\n", ch);
        return;
    }

    two_arguments(argument, arg1, arg2);

    obj_dotmode = find_all_dots(&arg1);
    cont_dotmode = find_all_dots(&arg2);

    if (!GET_SKILL(ch, SKILL_CONCEAL))
        do_get(ch, argument, cmd, 0);
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) && GET_LEVEL(ch) < LVL_GOD)
        send_to_char("Your arms are already full!\r\n", ch);
    else if (!*arg1)
        send_to_char("Palm what?\r\n", ch);
    else if (obj_dotmode != FIND_INDIV)
        send_to_char("You can only palm one item at a time!\r\n", ch);
    else if (cont_dotmode != FIND_INDIV)
        send_to_char("You can only palm an item from one container at a time!\r\n", ch);
    /* No container - palm from room */
    else if (!*arg2) {
        if (!(obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, arg1))))
            cprintf(ch, "You don't see %s %s here.\r\n", AN(arg1), arg1);
        else if (!check_get_disarmed_obj(ch, obj->last_to_hold, obj) && can_take_obj(ch, obj) &&
                 get_otrigger(obj, ch)) {
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
                    if (GET_PERCEPTION(tch) < roll - 50 + number(0, 50))
                        continue;
                } else if (GET_PERCEPTION(tch) < roll / 2)
                    continue;
                if (!CAN_SEE(tch, ch))
                    continue;
                if (!CAN_SEE_OBJ(tch, obj) && number(0, 1))
                    continue;
                act(buf, TRUE, tch, obj, cont, TO_CHAR);
            }
            if (!SOLIDCHAR(ch) && GET_LEVEL(ch) < LVL_IMMORT)
                act("You casually reach for $p... but you can't get a grip on it!", FALSE, ch, obj, 0, TO_CHAR);
            else {
                if (people)
                    act("You palm $p when you think no one is looking.", FALSE, ch, obj, 0, TO_CHAR);
                else
                    act("You smoothly palm $p.", FALSE, ch, obj, 0, TO_CHAR);
                unhide_object(obj);
                get_check_money(ch, obj);
                WAIT_STATE(ch, PULSE_VIOLENCE);
                improve_skill(ch, SKILL_CONCEAL);
            }
        }
    } else {
        cont_mode = generic_find(arg2, FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tch, &cont);
        if (!cont)
            cprintf(ch, "You can't find %s %s.\r\n", AN(arg2), arg2);
        else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
        else if (IS_SET(GET_OBJ_VAL(cont, VAL_CONTAINER_BITS), CONT_CLOSED))
            act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
        else if (!IS_PLR_CORPSE(cont) || has_corpse_consent(ch, cont)) {
            if (!(obj = find_obj_in_list(cont->contains, find_vis_by_name(ch, arg1)))) {
                sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg1), arg1);
                act(buf, FALSE, ch, cont, 0, TO_CHAR);
            } else if (cont_mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
                if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
                    act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
                else if (get_otrigger(obj, ch)) {

                    if (SOLIDCHAR(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
                        roll = conceal_roll(ch, obj);
                        obj_from_obj(obj);
                        obj_to_char(obj, ch);
                        act("You quietly palm $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
                        sprintf(buf, "%s quietly slips $p out of $P.", GET_NAME(ch));
                    } else {
                        act("You quietly reach into $P, but you can't seem to grasp $p.", FALSE, ch, obj, cont,
                            TO_CHAR);
                        sprintf(buf, "%s tries to slip $p out of $P, but is having trouble.", GET_NAME(ch));
                    }

                    LOOP_THRU_PEOPLE(tch, ch) {
                        if (tch == ch)
                            continue;
                        if (!CAN_SEE(tch, ch))
                            continue;
                        if (!CAN_SEE_OBJ(tch, obj) && number(0, 1))
                            continue;
                        if (CAN_SEE(ch, tch) ? (GET_PERCEPTION(tch) < roll - 50 + number(0, 50))
                                             : (GET_PERCEPTION(tch) < roll / 2))
                            continue;
                        act(buf, TRUE, tch, obj, cont, TO_CHAR);
                    }
                    if (SOLIDCHAR(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
                        unhide_object(obj);
                        if (IS_PLR_CORPSE(cont))
                            log("CORPSE: %s palms %s from %s.", GET_NAME(ch), strip_ansi(obj->short_description),
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
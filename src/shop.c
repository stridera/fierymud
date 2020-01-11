/***************************************************************************
 * $Id: shop.c,v 1.61 2010/06/20 19:53:47 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: shop.c                                         Part of FieryMUD *
 *  Usage: shopkeepers: loading config files, spec procs.                  *
 *     By: Jeff Fink                                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include <math.h>

#define __SHOP_C__

#include "chars.h"
#include "class.h"
#include "comm.h"
#include "composition.h"
#include "constants.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "limits.h"
#include "math.h"
#include "modify.h"
#include "screen.h"
#include "shop.h"
#include "specprocs.h"
#include "structs.h"
#include "utils.h"

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
void sort_keeper_objs(struct char_data *keeper, int shop_nr);

/* Local variables */
struct shop_data *shop_index;
int top_shop = 0;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_snicker;

int is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr) {
    char buf[200];

    if (!(CAN_SEE(keeper, ch))) {
        do_say(keeper, MSG_NO_SEE_CHAR, cmd_say, 0);
        return (FALSE);
    }
    if (IS_GOD(ch))
        return (TRUE);

    if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) || (IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
        (IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr))) {
        sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
        do_tell(keeper, buf, cmd_tell, 0);
        return (FALSE);
    }
    if (IS_NPC(ch))
        return (TRUE);

    if ((IS_MAGIC_USER(ch) && NOTRADE_MAGIC_USER(shop_nr)) || (IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
        (IS_ROGUE(ch) && NOTRADE_THIEF(shop_nr)) || (IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr))) {
        sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
        do_tell(keeper, buf, cmd_tell, 0);
        return (FALSE);
    }
    return (TRUE);
}

int is_open(struct char_data *keeper, int shop_nr, int msg) {
    char buf[200];

    *buf = 0;
    if (SHOP_OPEN1(shop_nr) > time_info.hours)
        strcpy(buf, MSG_NOT_OPEN_YET);
    else if (SHOP_CLOSE1(shop_nr) < time_info.hours) {
        if (SHOP_OPEN2(shop_nr) > time_info.hours)
            strcpy(buf, MSG_NOT_REOPEN_YET);
        else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
            strcpy(buf, MSG_CLOSED_FOR_DAY);
    }
    if (!(*buf))
        return (TRUE);
    if (msg)
        do_say(keeper, buf, cmd_tell, 0);
    return (FALSE);
}

int is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr) {
    if (is_open(keeper, shop_nr, TRUE)) {
        /* This is kinda hacky, but it prevents shopkeepers from getting laryngitis */
        keeper->char_specials.last_speech_time = 0;
        return (is_ok_char(keeper, ch, shop_nr));
    } else {
        return (FALSE);
    }
}

void push(struct stack_data *stack, int pushval) { S_DATA(stack, S_LEN(stack)++) = pushval; }

int top(struct stack_data *stack) {
    if (S_LEN(stack) > 0)
        return (S_DATA(stack, S_LEN(stack) - 1));
    else
        return (NOTHING);
}

int pop(struct stack_data *stack) {
    if (S_LEN(stack) > 0)
        return (S_DATA(stack, --S_LEN(stack)));
    else {
        log("Illegal expression in shop keyword list");
        return (0);
    }
}

void evaluate_operation(struct stack_data *ops, struct stack_data *vals) {
    int oper;

    if ((oper = pop(ops)) == OPER_NOT)
        push(vals, !pop(vals));
    else if (oper == OPER_AND)
        push(vals, pop(vals) && pop(vals));
    else if (oper == OPER_OR)
        push(vals, pop(vals) || pop(vals));
}

int find_oper_num(char token) {
    int index;

    for (index = 0; index <= MAX_OPER; index++)
        if (strchr(operator_str[index], token))
            return (index);
    return (NOTHING);
}

int evaluate_expression(struct obj_data *obj, char *expr) {
    struct stack_data ops, vals;
    char *ptr, *end, name[200];
    int temp, index;

    if (!expr)
        return TRUE;

    if (!isalpha(*expr))
        return TRUE;

    ops.len = vals.len = 0;
    ptr = expr;
    while (*ptr) {
        if (isspace(*ptr))
            ptr++;
        else {
            if ((temp = find_oper_num(*ptr)) == NOTHING) {
                end = ptr;
                while (*ptr && !isspace(*ptr) && (find_oper_num(*ptr) == NOTHING))
                    ptr++;
                strncpy(name, end, ptr - end);
                name[ptr - end] = 0;
                for (index = 0; *extra_bits[index] != '\n'; index++)
                    if (!str_cmp(name, extra_bits[index])) {
                        push(&vals, OBJ_FLAGGED(obj, index));
                        break;
                    }
                if (*extra_bits[index] == '\n')
                    push(&vals, isname(name, obj->name));
            } else {
                if (temp != OPER_OPEN_PAREN)
                    while (top(&ops) > temp)
                        evaluate_operation(&ops, &vals);

                if (temp == OPER_CLOSE_PAREN) {
                    if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
                        log("Illegal parenthesis in shop keyword expression");
                        return (FALSE);
                    }
                } else
                    push(&ops, temp);
                ptr++;
            }
        }
    }
    while (top(&ops) != NOTHING)
        evaluate_operation(&ops, &vals);
    temp = pop(&vals);
    if (top(&vals) != NOTHING) {
        log("Extra operands left on shop keyword expression stack");
        return (FALSE);
    }
    return (temp);
}

/* trade_with()
 *
 * Determines whether an object is suitable for sale to the given shop.
 */
int trade_with(struct obj_data *item, int shop_nr) {
    int counter;

    /* Free items are not desirable. */
    if (GET_OBJ_COST(item) < 1)
        return OBJECT_NOTOK;

    /* That !SELL flag is there for a reason... */
    if (OBJ_FLAGGED(item, ITEM_NOSELL))
        return OBJECT_NOTOK;

    /* We don't want this. */
    if (GET_OBJ_TYPE(item) == ITEM_KEY)
        return OBJECT_NOTOK;

    /* Go down the list of buytypes in the shop.  If any of them match
     * this object, it's considered validated.  Unless it's a dead staff/wand,
     * of course. */
    for (counter = 0; SHOP_BUYTYPE(shop_nr, counter) != NOTHING; counter++)
        if (SHOP_BUYTYPE(shop_nr, counter) == GET_OBJ_TYPE(item)) {
            if ((GET_OBJ_VAL(item, VAL_WAND_CHARGES_LEFT) == 0) &&
                ((GET_OBJ_TYPE(item) == ITEM_WAND) || (GET_OBJ_TYPE(item) == ITEM_STAFF)))
                return OBJECT_DEAD;
            else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter)))
                return OBJECT_OK;
        }
    return OBJECT_NOTOK;
}

int same_obj(struct obj_data *obj1, struct obj_data *obj2) {
    int index;

    if (!obj1 || !obj2)
        return (obj1 == obj2);

    if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
        return (FALSE);

    if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
        return (FALSE);

    if (!ALL_FLAGGED(GET_OBJ_FLAGS(obj1), GET_OBJ_FLAGS(obj2), NUM_ITEM_FLAGS))
        return (FALSE);

    for (index = 0; index < MAX_OBJ_APPLIES; index++)
        if ((obj1->applies[index].location != obj2->applies[index].location) ||
            (obj1->applies[index].modifier != obj2->applies[index].modifier))
            return (FALSE);

    return (TRUE);
}

int shop_producing(struct obj_data *item, int shop_nr) {
    int counter;

    if (GET_OBJ_RNUM(item) < 0)
        return (FALSE);
    for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
        if (same_obj(item, &obj_proto[SHOP_PRODUCT(shop_nr, counter)]))
            return (TRUE);
    return (FALSE);
}

int transaction_amt(char *arg) {
    int num;
    one_argument(arg, buf);
    if (*buf)
        if ((is_number(buf))) {
            num = atoi(buf);
            if ((strlen(arg)) > 3)
                strcpy(arg, arg + strlen(buf) + 1);
            else
                num = 1;
            return (num);
        }
    return (1);
}

char *times_message(struct obj_data *obj, char *name, int num) {
    static char buf[256];
    char *ptr;

    if (obj)
        strcpy(buf, obj->short_description);
    else {
        if ((ptr = strchr(name, '.')) == NULL)
            ptr = name;
        else
            ptr++;
        strncpy(buf, ptr, 200);
        buf[199] = 0;
    }

    if (num > 1)
        sprintf(END_OF(buf), " (x %d)", num);
    return buf;
}

struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name, struct obj_data *list) {
    struct obj_data *i, *last_match = 0;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = grab_number(&tmp)))
        return (0);

    for (i = list, j = 1; i && (j <= number); i = i->next_content)
        if (isname(tmp, i->name))
            if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
                if (j == number)
                    return (i);
                last_match = i;
                j++;
            }
    return (0);
}

struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name, struct obj_data *list) {
    struct obj_data *loop, *last_obj = 0;
    int index;

    if ((is_number(name + 1)))
        index = atoi(name + 1);
    else
        return (0);

    for (loop = list; loop; loop = loop->next_content)
        if (CAN_SEE_OBJ(ch, loop) && (loop->obj_flags.cost > 0))
            if (!same_obj(last_obj, loop)) {
                if (--index == 0)
                    return (loop);
                last_obj = loop;
            }
    return (0);
}

struct obj_data *get_purchase_obj(struct char_data *ch, char *arg, struct char_data *keeper, int shop_nr, int msg) {
    char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
    struct obj_data *obj;

    one_argument(arg, name);
    do {
        if (is_number(name)) {
            strcpy(buf, name);
            strcpy(name, "#\0");
            strcat(name, buf);
        }
        if (*name == '#')
            obj = get_hash_obj_vis(ch, name, keeper->carrying);
        else
            obj = get_slide_obj_vis(ch, name, keeper->carrying);
        if (!obj) {
            if (msg) {
                sprintf(buf, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
                do_tell(keeper, buf, cmd_tell, 0);
            }
            return (0);
        }
        if (GET_OBJ_COST(obj) <= 0) {
            extract_obj(obj);
            obj = 0;
        }
    } while (!obj);
    return (obj);
}

/* 1/16/01 David Endre - This code is used to count up how many of the
   same objects that the shopkeeper has. This then reduces how much
   a shopkeeper will pay for an object based on how many he already
   has or how much he will sell an object for. */

int do_count_objs(struct char_data *keeper, struct obj_data *last_obj) {
    struct obj_data *obj;
    int cnt = 0;

    if (keeper->carrying)
        for (obj = keeper->carrying; obj; obj = obj->next_content)
            if (obj->obj_flags.cost > 0)
                if (same_obj(last_obj, obj))
                    cnt++;

    return (cnt);
}

/* 1/21/01 - David Endre
   Added var and cha to the buy_price function. Var adds or subtracts the
   price of an object based on how many items a shopkeeper has. While cha
   compares the shopkeepers charisma to the players charisma and adjusts
   the price based on a comparison. Do not mess with these functions
   without talking to me, so I can explain them further. */

int buy_price(struct char_data *ch, struct char_data *keeper, struct obj_data *obj, int shop_nr) {
    int cnt = 0, price = 0;
    float var = 0.0, cha = 0.0;

    cnt = do_count_objs(keeper, obj);
    var = (double)(1.0 - ((double)(cnt - 1) * .007));
    if (var < .875) {
        var = .875;
    }
    cha = (double)(GET_AFFECTED_CHA(ch) / 100.0);
    cha = (double)(((GET_AFFECTED_CHA(keeper) / 100.0) - cha) / 2.0);

    price = (int)(GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr));
    price = (int)(price * var);
    price = (int)(price * (1 + cha));

    if (price < 1)
        price = 1;

    return (price);
}

void apply_getcash(struct char_data *ch, int cash) {
    GET_PLATINUM(ch) += PLATINUM_PART(cash);
    GET_GOLD(ch) += GOLD_PART(cash);
    GET_SILVER(ch) += SILVER_PART(cash);
    GET_COPPER(ch) += COPPER_PART(cash);
}

void adjust_cash(struct char_data *ch) {
    int cash = GET_CASH(ch);
    GET_PLATINUM(ch) = PLATINUM_PART(cash);
    GET_GOLD(ch) = GOLD_PART(cash);
    GET_SILVER(ch) = SILVER_PART(cash);
    GET_COPPER(ch) = COPPER_PART(cash);
}

void apply_cost(int cost, struct char_data *ch) {
    int haveP, haveG, haveS, haveC;

    if (cost <= 0 || cost > GET_CASH(ch)) {
        sprintf(buf, "ERR: %s being charged %d but doesn't have that much money", GET_NAME(ch), cost);
        mudlog(buf, BRF, LVL_GOD, TRUE);
        return;
    }

    /* Find out how much they have */
    haveP = GET_PLATINUM(ch);
    haveG = GET_GOLD(ch);
    haveS = GET_SILVER(ch);
    haveC = GET_COPPER(ch);

    /* Subtract what we need from what we have */
    haveC -= cost;

    /* Exchange the coins on down */
    while (haveC < 0) {
        haveS--;
        haveC += 10;
    }

    while (haveS < 0) {
        haveG--;
        haveS += 10;
    }

    while (haveG < 0) {
        haveP--;
        haveG += 10;
    }

    if (haveP < 0) {
        sprintf(buf, "SYSERR: %s being charged %d and ended up with negative platinum!", GET_NAME(ch), cost);
        mudlog(buf, BRF, LVL_GOD, TRUE);
        haveP = 0;
    }

    /* Give player new amount */
    GET_PLATINUM(ch) = haveP;
    GET_GOLD(ch) = haveG;
    GET_SILVER(ch) = haveS;
    GET_COPPER(ch) = haveC;
}

void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
    char tempstr[200], buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
    struct obj_data *obj, *last_obj = NULL;
    int copperamt = 0, buynum, bought = 0;
    bool amount = 0;
    int counter;

    if (!(is_ok(keeper, ch, shop_nr)))
        return; /* Isn't a shopkeeper */

    if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
        sort_keeper_objs(keeper, shop_nr);

    if ((buynum = transaction_amt(arg)) < 0) {
        sprintf(buf, "%s A negative amount?   Try selling me something.", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    /* Did buyer specify anything to buy? */
    if (!*arg) {
        sprintf(buf, "%s What do you want to buy?", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    /* Does shopkeeper have any of the desired item? */
    if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
        return;

    if (!RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("You can't handle solid objects in your condition.\r\n", ch);
        return;
    }

    /* Can the buyer afford the item? */
    if ((buy_price(ch, keeper, obj, shop_nr) > GET_CASH(ch)) && !IS_GOD(ch)) {
        sprintf(buf, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);

        switch (SHOP_BROKE_TEMPER(shop_nr)) {
        case 0:
            do_action(keeper, GET_NAME(ch), cmd_snicker, 0);
            return;
        case 1:
            do_echo(keeper, "smokes on his joint.", cmd_emote, SCMD_EMOTE);
            return;
        default:
            return;
        }
    }

    if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))) {
        sprintf(buf, "%s: You can't carry any more items.\r\n", fname(obj->name));
        send_to_char(buf, ch);
        return;
    }

    if (!ADDED_WEIGHT_OK(ch, obj)) {
        sprintf(buf, "%s: You can't carry that much weight.\r\n", fname(obj->name));
        send_to_char(buf, ch);
        return;
    }

    /* Loop over each purchased item */
    while ((obj) && ((GET_CASH(ch) >= buy_price(ch, keeper, obj, shop_nr)) || GET_LEVEL(ch) >= LVL_IMMORT) &&
           (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)) && (bought < buynum) && ADDED_WEIGHT_OK(ch, obj)) {

        bought++;

        /* Replenish items produced by the shop (permanent stock) */
        if (shop_producing(obj, shop_nr)) {
            for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
                if (same_obj(obj, &obj_proto[SHOP_PRODUCT(shop_nr, counter)])) {
                    amount = SHOP_AMOUNT(shop_nr, counter);
                }

            if (!(amount))
                obj = read_object(GET_OBJ_RNUM(obj), REAL);
            else {
                obj_from_char(obj);
                SHOP_SORT(shop_nr)--;
            }

            /* This item is not permanent stock */
        } else {
            obj_from_char(obj);
            SHOP_SORT(shop_nr)--;
        }

        obj_to_char(obj, ch);

        /* Give the money to the shopkeeper */
        copperamt += buy_price(ch, keeper, obj, shop_nr);

        /*
         * We should be charging the buyer BEFORE giving them the item,
         * because buy_price changes its return value depending on how
         * many of the item the shopkeeper is holding.
         *
         * However, players have been taking advantage of this bug and
         * I want to catch them. -Lao
         */
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            apply_cost(buy_price(ch, keeper, obj, shop_nr), ch);
        }

        last_obj = obj;
        one_argument(arg, name);
        obj = get_purchase_obj(ch, name, keeper, shop_nr, FALSE);
        if (!same_obj(obj, last_obj))
            break;
    } /* loop each purchased item */

    /* Give feedback to the buyer as to what was purchased */
    if (bought < buynum) {
        if (!obj || !same_obj(last_obj, obj))
            sprintf(buf, "%s I only have %d to sell you.", GET_NAME(ch), bought);
        else if (GET_CASH(ch) < buy_price(ch, keeper, obj, shop_nr))
            sprintf(buf, "%s You can only afford %d.", GET_NAME(ch), bought);
        else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
            sprintf(buf, "%s You can only hold %d.", GET_NAME(ch), bought);
        else if (!ADDED_WEIGHT_OK(ch, obj))
            sprintf(buf, "%s You can only carry %d.", GET_NAME(ch), bought);
        else
            sprintf(buf, "%s Something screwy only gave you %d.", GET_NAME(ch), bought);
        do_tell(keeper, buf, cmd_tell, 0);
    }

    if (GET_LEVEL(ch) < LVL_IMMORT)
        apply_getcash(keeper, copperamt);

    sprintf(tempstr, times_message(ch->carrying, 0, bought));
    sprintf(buf, "$n buys %s.", tempstr);
    act(buf, FALSE, ch, obj, 0, TO_ROOM);

    sprintf(buf, shop_index[shop_nr].message_buy, GET_NAME(ch), 0);
    do_tell(keeper, buf, cmd_tell, 0);
    sprintf(buf, "You now have %s.\r\n", tempstr);
    send_to_char(buf, ch);

    if (SHOP_USES_BANK(shop_nr))
        if (GET_CASH(keeper) > MAX_OUTSIDE_BANK) {
            SHOP_BANK(shop_nr) += (GET_CASH(keeper) - MAX_OUTSIDE_BANK);
            GET_PLATINUM(keeper) = 0;
            GET_GOLD(keeper) = 0;
            GET_SILVER(keeper) = 0;
            GET_COPPER(keeper) = 0;
            GET_COPPER(keeper) = MAX_OUTSIDE_BANK;
            adjust_cash(keeper);
        }
}

struct obj_data *get_selling_obj(struct char_data *ch, char *name, struct char_data *keeper, int shop_nr, int msg) {
    char buf[MAX_STRING_LENGTH];
    struct obj_data *obj;
    int result;

    if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, name)))) {
        if (msg) {
            sprintf(buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch));
            do_tell(keeper, buf, cmd_tell, 0);
        }
        return (0);
    }
    if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
        return (obj);

    switch (result) {
    case OBJECT_NOTOK:
        sprintf(buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
        break;
    case OBJECT_DEAD:
        sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
        break;
    default:
        sprintf(buf, "Illegal return value of %d from trade_with() (shop.c)", result);
        log(buf);
        sprintf(buf, "%s An error has occurred.", GET_NAME(ch));
        break;
    }
    if (msg)
        do_tell(keeper, buf, cmd_tell, 0);
    return (0);
}

/* 1/21/01 - David Endre
   Added var and cha to the sell_price function. Var adds or subtracts the
   price of an object based on how many items a shopkeeper has. While cha
   compares the shopkeepers charisma to the players charisma and adjusts
   the price based on a comparison. Do not mess with these functions
   without talking to me, so I can explain them further. */

int sell_price(struct char_data *ch, struct char_data *keeper, struct obj_data *obj, int shop_nr) {
    int cnt = 0, price = 0;
    double power = 0.0, cha = 0.0;

    cnt = do_count_objs(keeper, obj);

    cha = (double)(GET_AFFECTED_CHA(ch) / 100.0);
    cha = (double)((cha - (GET_AFFECTED_CHA(keeper) / 100.0)) / 2.0);

    price = (int)(GET_OBJ_COST(obj) * SHOP_SELLPROFIT(shop_nr));
    power = pow((1.0 / (cnt + 1.0)), (1.0 / 10.0));
    price = (int)((double)(price)*power);
    price = (int)(price * (1 + cha));

    if (price < 0)
        price = 0;

    return (price);
}

struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper, int shop_nr)
/*
   This function is a slight hack!  To make sure that duplicate items are
   only listed once on the "list", this function groups "identical"
   objects together on the shopkeeper's inventory list.  The hack involves
   knowing how the list is put together, and manipulating the order of
   the objects on the list.  (But since most of DIKU is not encapsulated,
   and information hiding is almost never used, it isn't that big a deal) -JF
 */
{
    struct obj_data *loop;
    int temp;

    if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
        sort_keeper_objs(keeper, shop_nr);

    /* Extract the object if it is identical to one produced */
    if (shop_producing(obj, shop_nr)) {
        temp = GET_OBJ_RNUM(obj);
        extract_obj(obj);
        return (&obj_proto[temp]);
    }
    SHOP_SORT(shop_nr)++;
    loop = keeper->carrying;
    obj_to_char(obj, keeper);
    keeper->carrying = loop;
    while (loop) {
        if (same_obj(obj, loop)) {
            obj->next_content = loop->next_content;
            loop->next_content = obj;
            return (obj);
        }
        loop = loop->next_content;
    }
    keeper->carrying = obj;
    return (obj);
}

void sort_keeper_objs(struct char_data *keeper, int shop_nr) {
    struct obj_data *list = 0, *temp;

    while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper)) {
        temp = keeper->carrying;
        obj_from_char(temp);
        temp->next_content = list;
        list = temp;
    }

    while (list) {
        temp = list;
        list = list->next_content;
        if ((shop_producing(temp, shop_nr)) &&
            !(find_obj_in_list(keeper->carrying, find_by_rnum(GET_OBJ_RNUM(temp))))) {
            obj_to_char(temp, keeper);
            SHOP_SORT(shop_nr)++;
        } else
            (void)slide_obj(temp, keeper, shop_nr);
    }
}

void apply_removecash(struct char_data *ch, int cash) {
    int c_tot = 0;

    c_tot = GET_CASH(ch);
    GET_PLATINUM(ch) = 0;
    GET_GOLD(ch) = 0;
    GET_SILVER(ch) = 0;
    GET_COPPER(ch) = c_tot;
    GET_COPPER(ch) -= cash;
    adjust_cash(ch);
}

void shopping_sell(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
    char tempstr[200], buf[MAX_STRING_LENGTH], name[200], sdesc[200];
    struct obj_data *obj;
    int sellnum, sold = 0, cashamt = 0, cnt = 0;

    if (!(is_ok(keeper, ch, shop_nr)))
        return;

    if ((sellnum = transaction_amt(arg)) < 0) {
        sprintf(buf, "%s A negative amount?  Try buying something.", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }
    if (!(*arg) || !(sellnum)) {
        sprintf(buf, "%s What do you want to sell??", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }
    one_argument(arg, name);
    if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
        return;

    if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        sprintf(buf, "You can't sell $p.  It's CURSED!");
        act(buf, FALSE, ch, obj, obj, TO_CHAR);
        return;
    }

    cnt = do_count_objs(keeper, obj);
    if (cnt >= 25) {
        sprintf(buf, "%s Sorry, I have %d of those in stock already.", GET_NAME(ch), cnt);
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    if (GET_CASH(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, keeper, obj, shop_nr)) {
        sprintf(buf, shop_index[shop_nr].missing_cash1, GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    /* Get a copy of the object's short description, since it may well
     * be freed in the following loop. (And make sure the string is
     * terminated.) */
    strncpy(sdesc, obj->short_description, 200);
    sdesc[199] = 0;

    while ((obj) && (GET_CASH(keeper) + SHOP_BANK(shop_nr) >= sell_price(ch, keeper, obj, shop_nr)) &&
           (sold < sellnum) && (cnt < 25)) {
        sold++;
        cnt++;

        cashamt += sell_price(ch, keeper, obj, shop_nr);
        apply_removecash(keeper, sell_price(ch, keeper, obj, shop_nr));

        obj_from_char(obj);
        /* If obj was just removed from a player, it has started decomposing.
         * We don't want shopkeeper inventory to decompose, so override that: */
        stop_decomposing(obj);
        slide_obj(obj, keeper, shop_nr);
        obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
    }

    if (sold < sellnum) {
        if (!obj)
            sprintf(buf, "%s You only have %d of those.", GET_NAME(ch), sold);
        else if (GET_CASH(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, keeper, obj, shop_nr))
            sprintf(buf, "%s I can only afford to buy %d of those.", GET_NAME(ch), sold);
        else if (cnt >= 25)
            sprintf(buf, "%s I will only buy %d of those.", GET_NAME(ch), sold);
        else
            sprintf(buf, "%s Something really screwy made me buy %d.", GET_NAME(ch), sold);

        do_tell(keeper, buf, cmd_tell, 0);
    }
    apply_getcash(ch, cashamt);
    strcpy(tempstr, times_message(0, sdesc, sold));
    sprintf(buf, "$n sells %s.", tempstr);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    sprintf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), 0);
    do_tell(keeper, buf, cmd_tell, 0);
    if (cashamt == 0) {
        sprintf(buf, "The shopkeeper now has %s.\r\n", tempstr);
        send_to_char(buf, ch);
        sprintf(buf, "You walk away empty handed.\r\nYou feel guilty for pawning "
                     "worthless garbage.\r\n");
    } else {
        sprintf(buf,
                "%s accepts %s and pays you &0&b&6%d&0p,&b&3%d&0g,&0%ds,&0&3%d&0c "
                "coins.\r\n",
                GET_NAME(keeper), tempstr, PLATINUM_PART(cashamt), GOLD_PART(cashamt), SILVER_PART(cashamt),
                COPPER_PART(cashamt));
    }
    send_to_char(buf, ch);

    if (GET_CASH(keeper) < MIN_OUTSIDE_BANK) {
        cashamt = MIN(MAX_OUTSIDE_BANK - GET_CASH(keeper), SHOP_BANK(shop_nr));
        SHOP_BANK(shop_nr) -= cashamt;
        apply_getcash(keeper, cashamt);
    }
}

void shopping_value(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
    char buf[MAX_STRING_LENGTH];
    struct obj_data *obj;
    char name[MAX_INPUT_LENGTH];
    int bp;
    int cnt = 0;
    if (!(is_ok(keeper, ch, shop_nr)))
        return;

    if (!(*arg)) {
        sprintf(buf, "%s What do you want me to evaluate??", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }
    one_argument(arg, name);
    if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
        return;

    if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        sprintf(buf, "%s Sorry, I do not buy cursed items.", GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    cnt = do_count_objs(keeper, obj);
    if (cnt >= 25) {
        sprintf(buf, "%s Sorry that's of no value to me, I already have %d of those.", GET_NAME(ch), cnt);
        do_tell(keeper, buf, cmd_tell, 0);
        return;
    }

    bp = ((int)(sell_price(ch, keeper, obj, shop_nr)));
    if (bp == 0)
        sprintf(buf, "%s You ought to pay me to take that off of your hands!", GET_NAME(ch));
    else
        sprintf(buf, "%s I'll give you %dp, %dg, %ds, %dc for that!", GET_NAME(ch), PLATINUM_PART(bp), GOLD_PART(bp),
                SILVER_PART(bp), COPPER_PART(bp));
    do_tell(keeper, buf, cmd_tell, 0);

    return;
}

char *list_object(struct char_data *keeper, struct obj_data *obj, struct char_data *ch, int cnt, int index,
                  int shop_nr) {
    static char buf[256];
    char buf2[300], buf3[200];
    int bp;

    sprintf(buf, "%2d)  %3d  ", index, GET_OBJ_LEVEL(obj));

    /* Compile object name and information */
    strcpy(buf3, obj->short_description);
    if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && (GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING)))
        sprintf(END_OF(buf3), " of %s", LIQ_NAME(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)));
    if (!shop_producing(obj, shop_nr))
        sprintf(END_OF(buf3), " (x%2d)", cnt);

    /* FUTURE: */
    /* Add glow/hum/etc */

    if ((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF))
        if (GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT) < GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES))
            strcat(buf3, " (partially used)");
    bp = ((int)(buy_price(ch, keeper, obj, shop_nr)));
    sprintf(buf2, "%-48s  &0&b&6%3d&0p,&b&3%d&0g,&0%ds,&0&3%d&0c\r\n", strip_ansi(buf3), PLATINUM_PART(bp),
            GOLD_PART(bp), SILVER_PART(bp), COPPER_PART(bp));
    strcat(buf, buf2);
    return (buf);
}

void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
    char name[200];
    struct obj_data *obj, *last_obj = 0;
    int cnt = 0, index = 0;
    bool any = FALSE;

    if (!IS_GOD(ch))
        if (!(is_ok(keeper, ch, shop_nr)))
            return;

    if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
        sort_keeper_objs(keeper, shop_nr);
    one_argument(arg, name);

    if (keeper->carrying)
        for (obj = keeper->carrying; obj; obj = obj->next_content)
            if ((!(*name) || isname(name, obj->name)) && CAN_SEE_OBJ(ch, obj) && (obj->obj_flags.cost > 0)) {
                if (!any) {
                    any = TRUE;
                    pprintf(ch, " ##  Lvl  Item                                          "
                                "         Cost\r\n");
                    pprintf(ch, "---  ---  ------------------------------------------------  "
                                "-------------\r\n");
                }
                if (!last_obj) {
                    last_obj = obj;
                    cnt = 1;
                } else if (same_obj(last_obj, obj))
                    cnt++;
                else {
                    index++;
                    if (!(*name) || isname(name, last_obj->name))
                        pprintf(ch, list_object(keeper, last_obj, ch, cnt, index, shop_nr));
                    cnt = 1;
                    last_obj = obj;
                }
            }

    index++;
    if (!any) {
        if (*name)
            pprintf(ch, "Presently, none of those are for sale.\r\n");
        else
            pprintf(ch, "Currently, there is nothing for sale.\r\n");
    } else if (!(*name) || isname(name, last_obj->name))
        pprintf(ch, list_object(keeper, last_obj, ch, cnt, index, shop_nr));

    start_paging(ch);
}

int ok_shop_room(int shop_nr, int room) {
    int index;

    for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++)
        if (SHOP_ROOM(shop_nr, index) == room)
            return (TRUE);
    return (FALSE);
}

int my_shop_nr(struct char_data *ch) {
    int shop_nr;

    if (!IS_NPC(ch))
        return -1;

    for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
        if (SHOP_KEEPER(shop_nr) == GET_MOB_RNUM(ch))
            return shop_nr;

    return -1;
}

/* give_shopkeeper_reject()
 *
 * Returns TRUE if it's a shopkeeper who doesn't want it.
 * Returns FALSE otherwise, and sends act() messages.
 */
bool give_shopkeeper_reject(struct char_data *ch, struct char_data *vict, struct obj_data *obj) {
    int shop_nr;

    shop_nr = my_shop_nr(vict);
    if (shop_nr < 0)
        return FALSE;
    switch (trade_with(obj, shop_nr)) {
    case OBJECT_NOTOK:
        act("$N briskly refuses.", FALSE, ch, 0, vict, TO_CHAR);
        act("$N briskly refuses $p from $n.", TRUE, ch, obj, vict, TO_NOTVICT);
        act("You briskly refuse $p from $N.", FALSE, vict, obj, ch, TO_CHAR);
        return TRUE;
        break;
    case OBJECT_DEAD:
        act("$N stares at $p and shakes $S head.", FALSE, ch, obj, vict, TO_CHAR);
        act("$N glares at $n and refuses $p.", TRUE, ch, obj, vict, TO_NOTVICT);
        act("You glare at $N and refuse $p.", FALSE, vict, obj, ch, TO_CHAR);
        return TRUE;
        break;
    }
    return FALSE;
}

SPECIAL(shop_keeper) {
    char argm[MAX_INPUT_LENGTH];
    struct char_data *keeper = (struct char_data *)me;
    int shop_nr;

    shop_nr = my_shop_nr(keeper);
    if (shop_nr < 0)
        return FALSE;

    if (SHOP_FUNC(shop_nr)) /* Check secondary function */
        if ((SHOP_FUNC(shop_nr))(ch, me, cmd, arg))
            return (TRUE);

    if (keeper == ch) {
        if (cmd)
            SHOP_SORT(shop_nr) = 0; /* Safety in case "drop all" */
        return (FALSE);
    }
    if (!ok_shop_room(shop_nr, world[ch->in_room].vnum))
        return (0);

    if (!AWAKE(keeper))
        return (FALSE);

    if (CMD_IS("steal")) {
        sprintf(argm, "$N shouts '%s'", MSG_NO_STEAL_HERE);
        do_action(keeper, GET_NAME(ch), cmd_slap, 0);
        act(argm, FALSE, ch, 0, keeper, TO_CHAR);
        return (TRUE);
    }

    if (CMD_IS("buy")) {
        shopping_buy(argument, ch, keeper, shop_nr);
        return (TRUE);
    } else if (CMD_IS("sell")) {
        shopping_sell(argument, ch, keeper, shop_nr);
        return (TRUE);
    } else if (CMD_IS("value")) {
        shopping_value(argument, ch, keeper, shop_nr);
        return (TRUE);
    } else if (CMD_IS("list")) {
        shopping_list(argument, ch, keeper, shop_nr);
        return (TRUE);
    }
    return (FALSE);
}

int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim) {
    char buf[200];
    int index;

    if (IS_NPC(victim) && (mob_index[GET_MOB_RNUM(victim)].func == shop_keeper))
        for (index = 0; index < top_shop; index++)
            if ((GET_MOB_RNUM(victim) == SHOP_KEEPER(index)) && !SHOP_KILL_CHARS(index)) {
                do_action(victim, GET_NAME(ch), cmd_slap, 0);
                sprintf(buf, "%s %s", GET_NAME(ch), MSG_CANT_KILL_KEEPER);
                do_tell(victim, buf, cmd_tell, 0);
                return (FALSE);
            }
    return (TRUE);
}

int add_to_list(struct shop_buy_data *list, int type, int *len, int *val, int *amount) {
    if (type == LIST_PRODUCE)
        BUY_AMOUNT(list[*len]) = *amount;
    if (*val >= 0) {
        if (*len < MAX_SHOP_OBJ) {
            if (type == LIST_PRODUCE)
                *val = real_object(*val);
            if (*val >= 0) {
                BUY_TYPE(list[*len]) = *val;
                BUY_WORD(list[(*len)++]) = 0;
            } else
                *val = 0;
            return (FALSE);
        } else
            return (TRUE);
    }
    return (FALSE);
}

int end_read_list(struct shop_buy_data *list, int len, int error) {
    if (error) {
        sprintf(buf, "Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
        log(buf);
    }
    BUY_WORD(list[len]) = 0;
    BUY_AMOUNT(list[len]) = 0;
    BUY_TYPE(list[len++]) = NOTHING;
    return (len);
}

/* split into read_int_line and read_float_line by gurlaek 8/27/1999 */
void read_int_line(FILE *shop_f, int *data) {
    if (!get_line(shop_f, buf) || !sscanf(buf, "%d", data)) {
        fprintf(stderr, "Error in shop #%d\n", SHOP_NUM(top_shop));
        exit(1);
    }
}

void read_float_line(FILE *shop_f, float *data) {
    if (!get_line(shop_f, buf) || !sscanf(buf, "%f", data)) {
        fprintf(stderr, "Error in shop #%d\n", SHOP_NUM(top_shop));
        exit(1);
    }
}

/* removed the void *'s and replaced with ints -gurlaek 8/27/1999 */
void read_double_line(FILE *shop_f, int *data, int *data2) {
    if (!get_line(shop_f, buf) || !sscanf(buf, "%d %d", data, data2)) {
        fprintf(stderr, "Error in shop #%d\n", SHOP_NUM(top_shop));
        exit(1);
    }
}

int read_list(FILE *shop_f, struct shop_buy_data *list, int new_format, int max, int type) {
    int count, temp, len = 0, error = 0;
    int amount = 0;
    if (new_format) {
        do {
            amount = 0; /*defult amount set to 0 */
            read_double_line(shop_f, &temp, &amount);
            error += add_to_list(list, type, &len, &temp, &amount);
        } while (temp >= 0);
    } else
        for (count = 0; count < max; count++) {
            read_int_line(shop_f, &temp);
            error += add_to_list(list, type, &len, &temp, &amount);
        }
    return (end_read_list(list, len, error));
}

int read_type_list(FILE *shop_f, struct shop_buy_data *list, int new_format, int max) {
    int index, num, len = 0, error = 0, spare = -10;
    char *ptr;

    if (!new_format)
        return (read_list(shop_f, list, 0, max, LIST_TRADE));
    do {
        fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
        if ((ptr = strchr(buf, ';')) != NULL)
            *ptr = 0;
        else
            *(END_OF(buf) - 1) = 0;
        for (index = 0, num = NOTHING; index < NUM_ITEM_TYPES; ++index)
            if (!strn_cmp(item_types[index].name, buf, strlen(item_types[index].name))) {
                num = index;
                strcpy(buf, buf + strlen(item_types[index].name));
                break;
            }
        ptr = buf;
        if (num == NOTHING) {
            sscanf(buf, "%d", &num);
            while (!isdigit(*ptr))
                ptr++;
            while (isdigit(*ptr))
                ptr++;
        }
        while (isspace(*ptr))
            ptr++;
        while (isspace(*(END_OF(ptr) - 1)))
            *(END_OF(ptr) - 1) = 0;
        error += add_to_list(list, LIST_TRADE, &len, &num, &spare);
        if (*ptr)
            BUY_WORD(list[len - 1]) = strdup(ptr);
    } while (num >= 0);
    return (end_read_list(list, len, error));
}

void boot_the_shops(FILE *shop_f, char *filename, int rec_count) {
    char *buf, buf2[150];
    int temp, count, new_format = 0;
    struct shop_buy_data list[MAX_SHOP_OBJ + 1];
    int done = 0;

    sprintf(buf2, "beginning of shop file %s", filename);

    while (!done) {
        buf = fread_string(shop_f, buf2);
        if (*buf == '#') { /* New shop */
            sscanf(buf, "#%d\n", &temp);
            sprintf(buf2, "shop #%d in shop file %s", temp, filename);
            free(buf); /* Plug memory leak! */
            if (!top_shop)
                CREATE(shop_index, struct shop_data, rec_count);

            SHOP_NUM(top_shop) = temp;
            temp = read_list(shop_f, list, new_format, MAX_PROD, LIST_PRODUCE);
            CREATE(shop_index[top_shop].producing, int, temp);
            CREATE(shop_index[top_shop].amount, int, temp);
            for (count = 0; count < temp; count++) {
                SHOP_PRODUCT(top_shop, count) = BUY_TYPE(list[count]);
                SHOP_AMOUNT(top_shop, count) = BUY_AMOUNT(list[count]);
            }
            read_float_line(shop_f, &SHOP_BUYPROFIT(top_shop));
            read_float_line(shop_f, &SHOP_SELLPROFIT(top_shop));

            temp = read_type_list(shop_f, list, new_format, MAX_TRADE);
            CREATE(shop_index[top_shop].type, struct shop_buy_data, temp);
            for (count = 0; count < temp; count++) {
                SHOP_BUYTYPE(top_shop, count) = BUY_TYPE(list[count]);
                SHOP_BUYAMOUNT(top_shop, count) = BUY_AMOUNT(list[count]);
                SHOP_BUYWORD(top_shop, count) = BUY_WORD(list[count]);
            }

            shop_index[top_shop].no_such_item1 = fread_string(shop_f, buf2);
            shop_index[top_shop].no_such_item2 = fread_string(shop_f, buf2);
            shop_index[top_shop].do_not_buy = fread_string(shop_f, buf2);
            shop_index[top_shop].missing_cash1 = fread_string(shop_f, buf2);
            shop_index[top_shop].missing_cash2 = fread_string(shop_f, buf2);
            shop_index[top_shop].message_buy = fread_string(shop_f, buf2);
            shop_index[top_shop].message_sell = fread_string(shop_f, buf2);
            read_int_line(shop_f, &SHOP_BROKE_TEMPER(top_shop));
            read_int_line(shop_f, &SHOP_BITVECTOR(top_shop));
            read_int_line(shop_f, &SHOP_KEEPER(top_shop));

            SHOP_KEEPER(top_shop) = real_mobile(SHOP_KEEPER(top_shop));
            read_int_line(shop_f, &SHOP_TRADE_WITH(top_shop));

            temp = read_list(shop_f, list, new_format, 1, LIST_ROOM);
            CREATE(shop_index[top_shop].in_room, int, temp);
            for (count = 0; count < temp; count++)
                SHOP_ROOM(top_shop, count) = BUY_TYPE(list[count]);

            read_int_line(shop_f, &SHOP_OPEN1(top_shop));
            read_int_line(shop_f, &SHOP_CLOSE1(top_shop));
            read_int_line(shop_f, &SHOP_OPEN2(top_shop));
            read_int_line(shop_f, &SHOP_CLOSE2(top_shop));

            SHOP_BANK(top_shop) = 0;
            SHOP_SORT(top_shop) = 0;
            SHOP_FUNC(top_shop) = 0;
            top_shop++;
        } else {
            if (*buf == '$') /* EOF */
                done = TRUE;
            else if (strstr(buf, VERSION3_TAG)) /* New format marker */
                new_format = 1;
            free(buf); /* Plug memory leak! */
        }
    }
}

void assign_the_shopkeepers(void) {
    int index;

    cmd_say = find_command("say");
    cmd_tell = find_command("tell");
    cmd_emote = find_command("emote");
    cmd_slap = find_command("slap");
    cmd_snicker = find_command("snicker");
    for (index = 0; index < top_shop; index++) {
        if (SHOP_KEEPER(index) == NOBODY)
            continue;
        if (mob_index[SHOP_KEEPER(index)].func)
            SHOP_FUNC(index) = mob_index[SHOP_KEEPER(index)].func;
        mob_index[SHOP_KEEPER(index)].func = shop_keeper;
    }
}

char *customer_string(int shop_nr, int detailed) {
    int index, cnt = 1;
    static char buf[256];

    *buf = 0;
    for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
        if (!(SHOP_TRADE_WITH(shop_nr) & cnt))
            if (detailed) {
                if (*buf)
                    strcat(buf, ", ");
                strcat(buf, trade_letters[index]);
            } else
                sprintf(END_OF(buf), "%c", *trade_letters[index]);
        else if (!detailed)
            strcat(buf, "_");

    return (buf);
}

void handle_detailed_list(char *buf, char *buf1, struct char_data *ch) {
    if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
        strcat(buf, buf1);
    else {
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        sprintf(buf, "            %s", buf1);
    }
}

void list_detailed_shop(struct char_data *ch, int shop_nr) {
    struct obj_data *obj;
    struct char_data *k;
    int index, temp;

    sprintf(buf, "Vnum:       [%5d], Rnum: [%5d]\r\n", SHOP_NUM(shop_nr), shop_nr + 1);
    send_to_char(buf, ch);

    strcpy(buf, "Rooms:      ");
    for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++) {
        if (index)
            strcat(buf, ", ");
        if ((temp = real_room(SHOP_ROOM(shop_nr, index))) != NOWHERE)
            sprintf(buf1, "%s (#%d)", world[temp].name, world[temp].vnum);
        else
            sprintf(buf1, "<UNKNOWN> (#%d)", SHOP_ROOM(shop_nr, index));
        handle_detailed_list(buf, buf1, ch);
    }
    if (!index)
        send_to_char("Rooms:      None!\r\n", ch);
    else {
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
    }

    strcpy(buf, "Shopkeeper: ");
    if (SHOP_KEEPER(shop_nr) >= 0) {
        sprintf(END_OF(buf), "%s (#%d), Special Function: %s\r\n", GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
                mob_index[SHOP_KEEPER(shop_nr)].virtual, YESNO(SHOP_FUNC(shop_nr)));
        if ((k = find_char_in_world(find_by_rnum(SHOP_KEEPER(shop_nr))))) {
            send_to_char(buf, ch);
            sprintf(buf, "Coins:      [%9d], Bank: [%9d] (Total: %d)\r\n", GET_GOLD(k), SHOP_BANK(shop_nr),
                    GET_GOLD(k) + SHOP_BANK(shop_nr));
        }
    } else
        strcat(buf, "<NONE>\r\n");
    send_to_char(buf, ch);

    strcpy(buf1, customer_string(shop_nr, TRUE));
    sprintf(buf, "Customers:  %s\r\n", (*buf1) ? buf1 : "None");
    send_to_char(buf, ch);

    strcpy(buf, "Produces:   ");
    for (index = 0; SHOP_PRODUCT(shop_nr, index) != NOTHING; index++) {
        obj = &obj_proto[SHOP_PRODUCT(shop_nr, index)];
        if (index)
            strcat(buf, ", ");
        sprintf(buf1, "%s (#%d)", obj->short_description, obj_index[SHOP_PRODUCT(shop_nr, index)].virtual);
        handle_detailed_list(buf, buf1, ch);
    }
    if (!index)
        send_to_char("Produces:   Nothing!\r\n", ch);
    else {
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
    }

    strcpy(buf, "Buys:       ");
    for (index = 0; SHOP_BUYTYPE(shop_nr, index) != NOTHING; index++) {
        if (index)
            strcat(buf, ", ");
        sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop_nr, index)].name, SHOP_BUYTYPE(shop_nr, index));
        if (SHOP_BUYWORD(shop_nr, index))
            sprintf(END_OF(buf1), "[%s]", SHOP_BUYWORD(shop_nr, index));
        else
            strcat(buf1, "[all]");
        handle_detailed_list(buf, buf1, ch);
    }
    if (!index)
        send_to_char("Buys:       Nothing!\r\n", ch);
    else {
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
    }

    sprintf(buf, "Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]%s", SHOP_SELLPROFIT(shop_nr),
            SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr), SHOP_CLOSE1(shop_nr), SHOP_OPEN2(shop_nr),
            SHOP_CLOSE2(shop_nr), "\r\n");

    send_to_char(buf, ch);

    sprintbit((long)SHOP_BITVECTOR(shop_nr), shop_bits, buf1);
    sprintf(buf, "Bits:       %s\r\n", buf1);
    send_to_char(buf, ch);
}

void do_stat_shop(struct char_data *ch, char *arg) {
    int shop_nr, sh_virt;

    if (*arg) {
        if (isdigit(arg[0])) {
            sh_virt = atoi(arg);
            for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
                if (shop_index[shop_nr].virtual == sh_virt)
                    break;
            if (shop_nr >= top_shop) {
                send_to_char("There is no such shop.\r\n", ch);
                return;
            }
        } else {
            send_to_char("That isn't a shop number!\r\n", ch);
            return;
        }
    } else {
        for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
            if (ok_shop_room(shop_nr, world[ch->in_room].vnum))
                break;
        if (shop_nr >= top_shop) {
            send_to_char("There's no shop here.\r\n", ch);
            return;
        }
    }
    list_detailed_shop(ch, shop_nr);
}

void list_shops(struct char_data *ch, int start, int end) {
    int shop_nr, num = 0, room;
    bool any = FALSE;

    strcpy(buf, "\r\n");
    for (shop_nr = 0; shop_nr < top_shop; shop_nr++) {
        if (shop_index[shop_nr].virtual >= start && shop_index[shop_nr].virtual <= end) {
            if (!any || !(num % 19)) {
                any = TRUE;
                strcat(buf, " ##   Virtual   Keeper    Buy     Sell     Customers   Where\r\n");
                strcat(buf, "----------------------------------------------------------"
                            "--------------\r\n");
            }
            num++;
            room = real_room(SHOP_ROOM(shop_nr, 0));
            sprintf(buf2, "%3d  %6d    ", num, SHOP_NUM(shop_nr));
            if (SHOP_KEEPER(shop_nr) < 0)
                strcpy(buf1, "<NONE>");
            else
                sprintf(buf1, "%6d", mob_index[SHOP_KEEPER(shop_nr)].virtual);
            sprintf(END_OF(buf2), "%s     %3.2f    %3.2f     ", buf1, SHOP_SELLPROFIT(shop_nr),
                    SHOP_BUYPROFIT(shop_nr));
            strcat(buf2, customer_string(shop_nr, FALSE));
            sprintf(END_OF(buf), "%s   %6d - %s\r\n", buf2, SHOP_ROOM(shop_nr, 0),
                    room == NOWHERE ? "<NOWHERE>" : world[room].name);
        }
    }

    if (any)
        page_string(ch, buf);
    else
        send_to_char("No shops found.\r\n", ch);
}

int vnum_shop(char *searchname, struct char_data *ch) {
    int nr, found = 0, room;

    for (nr = 0; nr < top_shop; nr++) {
        room = real_room(SHOP_ROOM(nr, 0));
        if (room != NOWHERE && isname(searchname, world[room].name)) {
            sprintf(buf, "%3d. [%5d] (%5d) %s\r\n", ++found, SHOP_NUM(nr), SHOP_ROOM(nr, 0), world[room].name);
            send_to_char(buf, ch);
        }
    }
    return found;
}

void show_shops(struct char_data *ch, char *arg) {
    int shop_nr;

    if (!*arg) {
        list_shops(ch, 0, MAX_VNUM);
        return;
    }

    else {
        if (!str_cmp(arg, ".")) {
            for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
                if (ok_shop_room(shop_nr, world[ch->in_room].vnum))
                    break;

            if (shop_nr == top_shop) {
                send_to_char("This isn't a shop!\r\n", ch);
                return;
            }
        } else if (is_number(arg))
            shop_nr = atoi(arg) - 1;
        else
            shop_nr = -1;

        if ((shop_nr < 0) || (shop_nr >= top_shop)) {
            send_to_char("Illegal shop number.\r\n", ch);
            return;
        }
        list_detailed_shop(ch, shop_nr);
    }
}

void destroy_shops(void) {
    ssize_t cnt, itr;

    if (!shop_index)
        return;

    for (cnt = 0; cnt <= top_shop; cnt++) {
        if (shop_index[cnt].no_such_item1)
            free(shop_index[cnt].no_such_item1);
        if (shop_index[cnt].no_such_item2)
            free(shop_index[cnt].no_such_item2);
        if (shop_index[cnt].missing_cash1)
            free(shop_index[cnt].missing_cash1);
        if (shop_index[cnt].missing_cash2)
            free(shop_index[cnt].missing_cash2);
        if (shop_index[cnt].do_not_buy)
            free(shop_index[cnt].do_not_buy);
        if (shop_index[cnt].message_buy)
            free(shop_index[cnt].message_buy);
        if (shop_index[cnt].message_sell)
            free(shop_index[cnt].message_sell);
        if (shop_index[cnt].in_room)
            free(shop_index[cnt].in_room);
        if (shop_index[cnt].producing)
            free(shop_index[cnt].producing);
        if (shop_index[cnt].amount)
            free(shop_index[cnt].amount);

        if (shop_index[cnt].type) {
            for (itr = 0; BUY_TYPE(shop_index[cnt].type[itr]) != NOTHING; ++itr)
                if (BUY_WORD(shop_index[cnt].type[itr]))
                    free(BUY_WORD(shop_index[cnt].type[itr]));
            free(shop_index[cnt].type);
        }
    }

    free(shop_index);
    shop_index = NULL;
    top_shop = -1;
}

/***************************************************************************
 * $Log: shop.c,v $
 * Revision 1.61  2010/06/20 19:53:47  mud
 * Log to file errors we might want to see.
 *
 * Revision 1.59  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.58  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.57  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.56  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.55  2008/09/21 17:58:35  jps
 * Don't print the shop list header when none of a requested object is for sale.
 *
 * Revision 1.54  2008/09/20 07:51:45  jps
 * Don't charge immortals money in shops.
 *
 * Revision 1.53  2008/09/09 19:02:23  jps
 * Stop items that have been sold to shops from decomposing.
 *
 * Revision 1.52  2008/09/03 17:34:08  myc
 * Moved liquid information into a def struct array.
 *
 * Revision 1.51  2008/08/31 17:09:26  myc
 * Remove debug from shop system.
 *
 * Revision 1.50  2008/08/19 02:11:14  jps
 * Don't apply fluid/rigidity restrictions to immortals.
 *
 * Revision 1.49  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.48  2008/08/15 03:59:08  jps
 * Added pprintf for paging, and changed page_string to take a character.
 *
 * Revision 1.47  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.46  2008/06/19 18:53:12  myc
 * Replaced the item_types array with a typedef struct array in objects.c.
 *
 * Revision 1.45  2008/06/07 19:06:46  myc
 * Moved object-related constants and routines to objects.h.
 *
 * Revision 1.44  2008/06/05 02:07:43  myc
 * Changed object flags to use flagvectors.
 *
 * Revision 1.43  2008/05/18 05:39:59  jps
 * Changed room_data member number to "vnum".
 *
 * Revision 1.42  2008/04/05 22:03:34  jps
 * Ensure that players are never said to have a shop number.
 *
 * Revision 1.41  2008/04/04 03:43:31  jps
 * Add give_shopkeeper_reject(), which allows shopkeepers to refuse
 * to be given objects that they wouldn't sell.
 *
 * Revision 1.40  2008/04/03 02:05:34  myc
 * Depending on screen.h now.
 *
 * Revision 1.39  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.38  2008/03/26 23:32:09  jps
 * Prevent buying things when in a fluid state.
 *
 * Revision 1.37  2008/02/16 20:31:32  myc
 * Adding function to free shops at program termination.
 *
 * Revision 1.36  2008/02/15 03:29:29  jps
 * Was going to fix something, and tidied up some code, but it
 * seemed to have been fixed already and I didn't actually
 * change anything.
 *
 * Revision 1.35  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.34  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.33  2008/01/27 12:12:55  jps
 * Changed IS_THIEF macro to IS_ROGUE.
 *
 * Revision 1.32  2008/01/27 01:42:03  jps
 * Fix grammar when informing player of cursed objects.
 *
 * Revision 1.31  2008/01/23 02:33:26  jps
 * Make shopping bills come out of lower-denomination coins first.
 *
 * Revision 1.30  2008/01/09 02:30:14  jps
 * Use macro to get mob real number.
 *
 * Revision 1.29  2008/01/02 07:11:21  jps
 * Using class.h.
 *
 * Revision 1.28  2007/09/20 09:34:34  jps
 * Improve feedback when selling things.
 *
 * Revision 1.27  2007/09/12 19:23:04  myc
 * Shop keepers give correct response when 'buy' is typed without arguments.
 * Shop keepers won't throw up on you anymore.
 *
 * Revision 1.26  2007/09/03 23:59:43  jps
 * Added macro ADDED_WEIGHT_OK for testing whether a char can have an
 * object added to its inventory.  Avoids an integer overflow problem
 * that could occur if an object's weight was near maxint.
 *
 * Revision 1.25  2007/08/24 22:59:55  jps
 * Correctly state that none are for sale when "list <name>" doesn't
 * match any of the items for sale.
 *
 * Revision 1.24  2007/08/24 22:49:05  jps
 * Added function vnum_shop() for use with the "snum" command.
 *
 * Revision 1.23  2007/08/24 22:10:58  jps
 * Add commands "slist" and "sstat".
 *
 * Revision 1.22  2007/05/24 03:35:11  jps
 * Display an item's full name when selling, rather than just whatever
 * the seller typed.
 *
 * Revision 1.21  2006/11/08 07:21:31  jps
 * Minimum sell price for any item is now 1 copper.
 *
 * Revision 1.20  2006/07/20 07:43:07  cjd
 * Typo fixes.
 *
 * Revision 1.19  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.18  2001/05/03 03:20:05  dce
 * Shopkeepers will no longer buy cursed items.
 *
 * Revision 1.17  2001/03/24 19:26:06  dce
 * Object level shows up in the list command at shops.
 *
 * Revision 1.16  2001/03/14 02:33:55  dce
 * Shopkeepers will only buy 25 of the same object.
 *
 * Revision 1.15  2001/03/13 01:57:29  dce
 * Shopkeepers no longer say, 'I have ...'
 *
 * Revision 1.14  2001/02/18 20:34:05  dce
 * Invis gods can now do a list.
 * Also removed a bogus message.
 *
 * Revision 1.13  2001/02/08 01:31:05  dce
 * Striped the color from short item descriptions in the list
 * command. It was causing mis-aligned columns.
 *
 * Revision 1.12  2001/01/23 00:15:57  dce
 * Lengthed the display for list so that items align more correctly.
 * Changed the value from 40 to 47. You oculd maybe do 50, but it's
 * pushing the width of normal people's screens.
 *
 * Revision 1.11  2001/01/22 04:14:13  dce
 * Added a charisma check and an items count check to shopkeepers
 * Shopkeers will now adjust the price based on these factors.
 *
 * Revision 1.10  2001/01/16 03:13:27  dce
 * Fixed some typos
 *
 * Revision 1.9  2001/01/16 03:10:47  dce
 * Added message for 0 value items.
 *
 * Revision 1.8  2001/01/12 02:35:04  dce
 * Completely re-did the existing shop code and the way it handled
 * money. Previously it used just GET_GOLD and didn't take into
 * account silver, platinum or copper.
 *
 * Revision 1.7  2000/11/24 21:30:22  rsd
 * Altered comment header and added back rlog messages from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.6  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.5  1999/08/29 07:06:04  jimmy
 * Many many small but ver significant bug fixes found using insure.  The
 * code now compiles cleanly and boots cleanly with insure.  The most
 *significant changes were moving all the BREATH's to within normal spell range,
 *and fixing the way socials were allocated.  Too many small fixes to list them
 * all. --gurlaek (now for the runtime debugging :( )
 *
 * Revision 1.4  1999/08/28 00:31:16  mtp
 * typo in apply_getcash fixed now sell should work
 *
 * Revision 1.3  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.2  1999/02/02 02:18:34  mud
 * indented file
 * dos2unix
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

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

#include "shop.hpp"

#include "chars.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "screen.hpp"
#include "specprocs.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

/* Global Vars */
int top_shop;
ShopData *shop_index;

/* Constant list for printing out who we sell to */
const char *trade_letters[] = {"Good",                            /* First, the alignment based ones */
                               "Evil",   "Neutral", "Magic User", /* Then the class based ones */
                               "Cleric", "Thief",   "Warrior",    "\n"};
const char *operator_str[] = {"[({", "])}", "|+", "&*", "^'"};
const char *shop_bits[] = {"WILL_FIGHT", "USES_BANK", "\n"};

const char *msg_not_open_yet = "Come back later!";
const char *msg_not_reopen_yet = "Sorry, we have closed, but come back later.";
const char *msg_closed_for_day = "Sorry, come back tomorrow.";
const char *msg_no_steal_here = "$n is a bloody thief!";
const char *msg_no_see_char = "I don't trade with someone I can't see!";
const char *msg_no_sell_align = "Get out of here before I call the guards!";
const char *msg_no_sell_class = "We don't serve your kind here!";
const char *msg_no_used_wandstaff = "I don't buy used up wands or staves!";
const char *msg_cant_kill_keeper = "Get out of here before I call the guards!";

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
void sort_keeper_objs(CharData *keeper, int shop_nr);

/* Local variables */
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_snicker;

int is_ok_char(CharData *keeper, CharData *ch, int shop_nr) {
    char buf[200];

    if (!(CAN_SEE(keeper, ch))) {
        do_say(keeper, strdup(msg_no_see_char), cmd_say, 0);
        return (false);
    }
    if (IS_GOD(ch))
        return (true);

    if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) || (IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
        (IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr))) {
        sprintf(buf, "%s %s", GET_NAME(ch), msg_no_sell_align);
        do_tell(keeper, buf, cmd_tell, 0);
        return (false);
    }
    if (IS_NPC(ch))
        return (true);

    if ((IS_MAGIC_USER(ch) && NOTRADE_MAGIC_USER(shop_nr)) || (IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
        (IS_ROGUE(ch) && NOTRADE_THIEF(shop_nr)) || (IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr))) {
        sprintf(buf, "%s %s", GET_NAME(ch), msg_no_sell_class);
        do_tell(keeper, buf, cmd_tell, 0);
        return (false);
    }
    return (true);
}

int is_open(CharData *keeper, int shop_nr, int msg) {
    char buf[200];

    *buf = 0;
    if (SHOP_OPEN1(shop_nr) > time_info.hours)
        strcpy(buf, msg_not_open_yet);
    else if (SHOP_CLOSE1(shop_nr) < time_info.hours) {
        if (SHOP_OPEN2(shop_nr) > time_info.hours)
            strcpy(buf, msg_not_reopen_yet);
        else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
            strcpy(buf, msg_closed_for_day);
    }
    if (!(*buf))
        return (true);
    if (msg)
        do_say(keeper, buf, cmd_tell, 0);
    return (false);
}

int is_ok(CharData *keeper, CharData *ch, int shop_nr) {
    if (is_open(keeper, shop_nr, true)) {
        /* This is kinda hacky, but it prevents shopkeepers from getting laryngitis */
        keeper->char_specials.last_speech_time = 0;
        return (is_ok_char(keeper, ch, shop_nr));
    } else {
        return (false);
    }
}

void push(StackData *stack, int pushval) { S_DATA(stack, S_LEN(stack)++) = pushval; }

int top(StackData *stack) {
    if (S_LEN(stack) > 0)
        return (S_DATA(stack, S_LEN(stack) - 1));
    else
        return (NOTHING);
}

int pop(StackData *stack) {
    if (S_LEN(stack) > 0)
        return (S_DATA(stack, --S_LEN(stack)));
    else {
        log("Illegal expression in shop keyword list");
        return (0);
    }
}

void evaluate_operation(StackData *ops, StackData *vals) {
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

int evaluate_expression(ObjData *obj, char *expr) {
    StackData ops, vals;
    char *ptr, *end, name[200];
    int temp, index;

    if (!expr)
        return true;

    if (!isalpha(*expr))
        return true;

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
                    if (!strcasecmp(name, extra_bits[index])) {
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
                        return (false);
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
        return (false);
    }
    return (temp);
}

/* trade_with()
 *
 * Determines whether an object is suitable for sale to the given shop.
 */
int trade_with(ObjData *item, int shop_nr) {
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
                ((GET_OBJ_TYPE(item) == ITEM_WAND) || (GET_OBJ_TYPE(item) == ITEM_STAFF) || (GET_OBJ_TYPE(item) == ITEM_INSTRUMENT)))
                return OBJECT_DEAD;
            else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter)))
                return OBJECT_OK;
        }
    return OBJECT_NOTOK;
}

int same_obj(ObjData *obj1, ObjData *obj2) {
    int index;

    if (!obj1 || !obj2)
        return (obj1 == obj2);

    if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
        return (false);

    if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
        return (false);

    if (!ALL_FLAGGED(GET_OBJ_FLAGS(obj1), GET_OBJ_FLAGS(obj2), NUM_ITEM_FLAGS))
        return (false);

    for (index = 0; index < MAX_OBJ_APPLIES; index++)
        if ((obj1->applies[index].location != obj2->applies[index].location) ||
            (obj1->applies[index].modifier != obj2->applies[index].modifier))
            return (false);

    return (true);
}

int shop_producing(ObjData *item, int shop_nr) {
    int counter;

    if (GET_OBJ_RNUM(item) < 0)
        return (false);
    for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
        if (same_obj(item, &obj_proto[SHOP_PRODUCT(shop_nr, counter)]))
            return (true);
    return (false);
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

char *times_message(ObjData *obj, char *name, int num) {
    static char buf[256];
    char *ptr;

    if (obj)
        strcpy(buf, obj->short_description);
    else {
        if ((ptr = strchr(name, '.')) == nullptr)
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

ObjData *get_slide_obj_vis(CharData *ch, char *name, ObjData *list) {
    ObjData *i, *last_match = 0;
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

ObjData *get_hash_obj_vis(CharData *ch, char *name, ObjData *list) {
    ObjData *loop, *last_obj = 0;
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

ObjData *get_purchase_obj(CharData *ch, char *arg, CharData *keeper, int shop_nr, int msg) {
    char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
    ObjData *obj;

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

int do_count_objs(CharData *keeper, ObjData *last_obj) {
    ObjData *obj;
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

int buy_price(CharData *ch, CharData *keeper, ObjData *obj, int shop_nr) {
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

void apply_getcash(CharData *ch, int cash) {
    GET_PLATINUM(ch) += PLATINUM_PART(cash);
    GET_GOLD(ch) += GOLD_PART(cash);
    GET_SILVER(ch) += SILVER_PART(cash);
    GET_COPPER(ch) += COPPER_PART(cash);
}

void adjust_cash(CharData *ch) {
    int cash = GET_CASH(ch);
    GET_PLATINUM(ch) = PLATINUM_PART(cash);
    GET_GOLD(ch) = GOLD_PART(cash);
    GET_SILVER(ch) = SILVER_PART(cash);
    GET_COPPER(ch) = COPPER_PART(cash);
}

void apply_cost(int cost, CharData *ch) {
    int haveP, haveG, haveS, haveC;

    if (cost <= 0 || cost > GET_CASH(ch)) {
        log(LogSeverity::Warn, LVL_GOD, "ERR: {} being charged {} but doesn't have that much money", GET_NAME(ch),
            cost);
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
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: {} being charged {:d} and ended up with negative platinum!",
            GET_NAME(ch), cost);
        haveP = 0;
    }

    /* Give player new amount */
    GET_PLATINUM(ch) = haveP;
    GET_GOLD(ch) = haveG;
    GET_SILVER(ch) = haveS;
    GET_COPPER(ch) = haveC;
}

void shopping_buy(char *arg, CharData *ch, CharData *keeper, int shop_nr) {
    char tempstr[200], buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
    ObjData *obj, *last_obj = nullptr;
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
    if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, true)))
        return;

    if (!RIGID(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You can't handle solid objects in your condition.\n");
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
            do_echo(keeper, strdup("smokes on his joint."), cmd_emote, SCMD_EMOTE);
            return;
        default:
            return;
        }
    }

    if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))) {
        char_printf(ch, "{}: You can't carry any more items.\n", fname(obj->name));
        return;
    }

    if (!ADDED_WEIGHT_OK(ch, obj)) {
        char_printf(ch, "{}: You can't carry that much weight.\n", fname(obj->name));
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
        obj = get_purchase_obj(ch, name, keeper, shop_nr, false);
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

    strcpy(tempstr, times_message(ch->carrying, 0, bought));
    sprintf(buf, "$n buys %s.", tempstr);
    act(buf, false, ch, obj, 0, TO_ROOM);

    sprintf(buf, shop_index[shop_nr].message_buy, GET_NAME(ch), 0);
    do_tell(keeper, buf, cmd_tell, 0);
    char_printf(ch, "You now have {}.\n", tempstr);

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

ObjData *get_selling_obj(CharData *ch, char *name, CharData *keeper, int shop_nr, int msg) {
    char buf[MAX_STRING_LENGTH];
    ObjData *obj;
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
        sprintf(buf, "%s %s", GET_NAME(ch), msg_no_used_wandstaff);
        break;
    default:
        log("Illegal return value of {:d} from trade_with() (shop.c)", result);
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

int sell_price(CharData *ch, CharData *keeper, ObjData *obj, int shop_nr) {
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

ObjData *slide_obj(ObjData *obj, CharData *keeper, int shop_nr)
/*
   This function is a slight hack!  To make sure that duplicate items are
   only listed once on the "list", this function groups "identical"
   objects together on the shopkeeper's inventory list.  The hack involves
   knowing how the list is put together, and manipulating the order of
   the objects on the list.  (But since most of DIKU is not encapsulated,
   and information hiding is almost never used, it isn't that big a deal) -JF
 */
{
    ObjData *loop;
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

void sort_keeper_objs(CharData *keeper, int shop_nr) {
    ObjData *list = 0, *temp;

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

void apply_removecash(CharData *ch, int cash) {
    int c_tot = 0;

    c_tot = GET_CASH(ch);
    GET_PLATINUM(ch) = 0;
    GET_GOLD(ch) = 0;
    GET_SILVER(ch) = 0;
    GET_COPPER(ch) = c_tot;
    GET_COPPER(ch) -= cash;
    adjust_cash(ch);
}

void shopping_sell(char *arg, CharData *ch, CharData *keeper, int shop_nr) {
    char tempstr[200], buf[MAX_STRING_LENGTH], name[200], sdesc[200];
    ObjData *obj;
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
    if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, true)))
        return;

    if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        sprintf(buf, "You can't sell $p.  It's CURSED!");
        act(buf, false, ch, obj, obj, TO_CHAR);
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
        obj = get_selling_obj(ch, name, keeper, shop_nr, false);
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
    act(buf, false, ch, 0, 0, TO_ROOM);

    sprintf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), 0);
    do_tell(keeper, buf, cmd_tell, 0);
    if (cashamt == 0) {
        char_printf(ch, "The shopkeeper now has {}.\n", tempstr);
        char_printf(ch, "You walk away empty handed.\nYou feel guilty for pawning worthless garbage.\n");
    } else {
        char_printf(ch, "{} accepts {} and pays you &0&b&6{:d}&0p,&b&3{:d}&0g,&0{:d}s,&0&3{:d}&0c coins.\n",
                    GET_NAME(keeper), tempstr, PLATINUM_PART(cashamt), GOLD_PART(cashamt), SILVER_PART(cashamt),
                    COPPER_PART(cashamt));
    }

    if (GET_CASH(keeper) < MIN_OUTSIDE_BANK) {
        cashamt = std::min(MAX_OUTSIDE_BANK - GET_CASH(keeper), SHOP_BANK(shop_nr));
        SHOP_BANK(shop_nr) -= cashamt;
        apply_getcash(keeper, cashamt);
    }
}

void shopping_value(char *arg, CharData *ch, CharData *keeper, int shop_nr) {
    char buf[MAX_STRING_LENGTH];
    ObjData *obj;
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
    if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, true)))
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

std::string list_object(CharData *keeper, ObjData *obj, CharData *ch, int cnt, int index, int shop_nr) {
    std::string buf;
    char buf2[300], buf3[200];
    int bp;

    buf = fmt::format("{:2d})  {:3d}  {}", index, GET_OBJ_LEVEL(obj), obj->short_description);

    /* Compile object name and information */
    if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && (GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING)))
        buf += fmt::format(" of {}", LIQ_NAME(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)));
    if (!shop_producing(obj, shop_nr))
        buf += fmt::format(" (x{:2d})", cnt);
    if ((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF) || (GET_OBJ_TYPE(obj) == ITEM_INSTRUMENT))
        if (GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT) < GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES))
            buf += " (partially used)";

    /* FUTURE: */
    /* Add glow/hum/etc */

    bp = ((int)(buy_price(ch, keeper, obj, shop_nr)));
    return (fmt::format("{:<56}  &0&b&6{:3d}&0p,&b&3{:d}&0g,&0{:d}s,&0&3{:d}&0c\n", strip_ansi(buf.c_str()),
                        PLATINUM_PART(bp), GOLD_PART(bp), SILVER_PART(bp), COPPER_PART(bp)));
}

void shopping_list(char *arg, CharData *ch, CharData *keeper, int shop_nr) {
    char name[200];
    ObjData *obj, *last_obj = 0;
    int cnt = 0, index = 0;
    bool any = false;

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
                    any = true;
                    paging_printf(ch, " ##  Lvl  Item                                              Cost\n");
                    paging_printf(ch, "---  ---  ------------------------------------------------  -------------\n");
                }
                if (!last_obj) {
                    last_obj = obj;
                    cnt = 1;
                } else if (same_obj(last_obj, obj))
                    cnt++;
                else {
                    index++;
                    if (!(*name) || isname(name, last_obj->name))
                        paging_printf(ch, list_object(keeper, last_obj, ch, cnt, index, shop_nr));
                    cnt = 1;
                    last_obj = obj;
                }
            }

    index++;
    if (!any) {
        if (*name)
            paging_printf(ch, "Presently, none of those are for sale.\n");
        else
            paging_printf(ch, "Currently, there is nothing for sale.\n");
    } else if (!(*name) || isname(name, last_obj->name))
        paging_printf(ch, list_object(keeper, last_obj, ch, cnt, index, shop_nr));

    start_paging(ch);
}

int ok_shop_room(int shop_nr, int room) {
    int index;

    for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++)
        if (SHOP_ROOM(shop_nr, index) == room)
            return (true);
    return (false);
}

int my_shop_nr(CharData *ch) {
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
 * Returns true if it's a shopkeeper who doesn't want it.
 * Returns false otherwise, and sends act() messages.
 */
bool give_shopkeeper_reject(CharData *ch, CharData *vict, ObjData *obj) {
    int shop_nr;

    shop_nr = my_shop_nr(vict);
    if (shop_nr < 0)
        return false;
    switch (trade_with(obj, shop_nr)) {
    case OBJECT_NOTOK:
        act("$N briskly refuses.", false, ch, 0, vict, TO_CHAR);
        act("$N briskly refuses $p from $n.", true, ch, obj, vict, TO_NOTVICT);
        act("You briskly refuse $p from $N.", false, vict, obj, ch, TO_CHAR);
        return true;
        break;
    case OBJECT_DEAD:
        act("$N stares at $p and shakes $S head.", false, ch, obj, vict, TO_CHAR);
        act("$N glares at $n and refuses $p.", true, ch, obj, vict, TO_NOTVICT);
        act("You glare at $N and refuse $p.", false, vict, obj, ch, TO_CHAR);
        return true;
        break;
    }
    return false;
}

SPECIAL(shop_keeper) {
    char argm[MAX_INPUT_LENGTH];
    CharData *keeper = (CharData *)me;
    int shop_nr;

    shop_nr = my_shop_nr(keeper);
    if (shop_nr < 0)
        return false;

    if (SHOP_FUNC(shop_nr)) /* Check secondary function */
        if ((SHOP_FUNC(shop_nr))(ch, me, cmd, arg))
            return (true);

    if (keeper == ch) {
        if (cmd)
            SHOP_SORT(shop_nr) = 0; /* Safety in case "drop all" */
        return (false);
    }
    if (!ok_shop_room(shop_nr, world[ch->in_room].vnum))
        return (0);

    if (!AWAKE(keeper))
        return (false);

    if (CMD_IS("steal")) {
        sprintf(argm, "$N shouts '%s'", msg_no_steal_here);
        do_action(keeper, GET_NAME(ch), cmd_slap, 0);
        act(argm, false, ch, 0, keeper, TO_CHAR);
        return (true);
    }

    if (CMD_IS("buy")) {
        shopping_buy(argument, ch, keeper, shop_nr);
        return (true);
    } else if (CMD_IS("sell")) {
        shopping_sell(argument, ch, keeper, shop_nr);
        return (true);
    } else if (CMD_IS("value")) {
        shopping_value(argument, ch, keeper, shop_nr);
        return (true);
    } else if (CMD_IS("list")) {
        shopping_list(argument, ch, keeper, shop_nr);
        return (true);
    }
    return (false);
}

int ok_damage_shopkeeper(CharData *ch, CharData *victim) {
    char buf[200];
    int index;

    if (IS_NPC(victim) && (mob_index[GET_MOB_RNUM(victim)].func == shop_keeper))
        for (index = 0; index < top_shop; index++)
            if ((GET_MOB_RNUM(victim) == SHOP_KEEPER(index)) && !SHOP_KILL_CHARS(index)) {
                do_action(victim, GET_NAME(ch), cmd_slap, 0);
                sprintf(buf, "%s %s", GET_NAME(ch), msg_cant_kill_keeper);
                do_tell(victim, buf, cmd_tell, 0);
                return (false);
            }
    return (true);
}

int add_to_list(ShopBuyData *list, int type, int *len, int *val, int *amount) {
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
            return (false);
        } else
            return (true);
    }
    return (false);
}

int end_read_list(ShopBuyData *list, int len, int error) {
    if (error) {
        log("Raise MAX_SHOP_OBJ constant in shop.h to {:d}", len + error);
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

int read_list(FILE *shop_f, ShopBuyData *list, int new_format, int max, int type) {
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

int read_type_list(FILE *shop_f, ShopBuyData *list, int new_format, int max) {
    int index, num, len = 0, error = 0, spare = -10;
    char *ptr;

    if (!new_format)
        return (read_list(shop_f, list, 0, max, LIST_TRADE));
    do {
        fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
        if ((ptr = strchr(buf, ';')) != nullptr)
            *ptr = 0;
        else
            *(END_OF(buf) - 1) = 0;
        for (index = 0, num = NOTHING; index < NUM_ITEM_TYPES; ++index)
            if (!strncasecmp(item_types[index].name, buf, strlen(item_types[index].name))) {
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
    ShopBuyData list[MAX_SHOP_OBJ + 1];
    int done = 0;

    sprintf(buf2, "beginning of shop file %s", filename);

    while (!done) {
        buf = fread_string(shop_f, buf2);
        if (*buf == '#') { /* New shop */
            sscanf(buf, "#%d\n", &temp);
            sprintf(buf2, "shop #%d in shop file %s", temp, filename);
            free(buf); /* Plug memory leak! */
            if (!top_shop)
                CREATE(shop_index, ShopData, rec_count);

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
            CREATE(shop_index[top_shop].type, ShopBuyData, temp);
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
                done = true;
            else if (strcasestr(buf, VERSION3_TAG)) /* New format marker */
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

void handle_detailed_list(char *buf, char *buf1, CharData *ch) {
    if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
        strcat(buf, buf1);
    else {
        char_printf(ch, "{}\n", buf);
        sprintf(buf, "            %s", buf1);
    }
}

void list_detailed_shop(CharData *ch, int shop_nr) {
    ObjData *obj;
    CharData *k;
    int index, temp;

    char_printf(ch, "Vnum:       [{:5d}], Rnum: [{:5d}]\n", SHOP_NUM(shop_nr), shop_nr + 1);

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
        char_printf(ch, "Rooms:      None!\n");
    else {
        char_printf(ch, "{}\n", buf);
    }

    strcpy(buf, "Shopkeeper: ");
    if (SHOP_KEEPER(shop_nr) >= 0) {
        sprintf(END_OF(buf), "%s (#%d), Special Function: %s\n", GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
                mob_index[SHOP_KEEPER(shop_nr)].vnum, YESNO(SHOP_FUNC(shop_nr)));
        if ((k = find_char_in_world(find_by_rnum(SHOP_KEEPER(shop_nr))))) {
            char_printf(ch, buf);
            sprintf(buf, "Coins:      [%9d], Bank: [%9d] (Total: %d)\n", GET_GOLD(k), SHOP_BANK(shop_nr),
                    GET_GOLD(k) + SHOP_BANK(shop_nr));
        }
    } else
        strcat(buf, "<NONE>\n");
    char_printf(ch, buf);

    strcpy(buf1, customer_string(shop_nr, true));
    char_printf(ch, "Customers:  {}\n", (*buf1) ? buf1 : "None");

    strcpy(buf, "Produces:   ");
    for (index = 0; SHOP_PRODUCT(shop_nr, index) != NOTHING; index++) {
        obj = &obj_proto[SHOP_PRODUCT(shop_nr, index)];
        if (index)
            strcat(buf, ", ");
        sprintf(buf1, "%s (#%d)", obj->short_description, obj_index[SHOP_PRODUCT(shop_nr, index)].vnum);
        handle_detailed_list(buf, buf1, ch);
    }
    if (!index)
        char_printf(ch, "Produces:   Nothing!\n");
    else {
        char_printf(ch, "{}\n", buf);
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
        char_printf(ch, "Buys:       Nothing!\n");
    else {
        char_printf(ch, "{}\n", buf);
    }

    char_printf(ch, "Buy at:     [{:4.2f}], Sell at: [{:4.2f}], Open: [{:d}-{:d}, {:d}-{:d}]{}",
                SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr), SHOP_CLOSE1(shop_nr),
                SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr), "\n");

    sprintbit((long)SHOP_BITVECTOR(shop_nr), shop_bits, buf1);
    char_printf(ch, "Bits:       {}\n", buf1);
}

void do_stat_shop(CharData *ch, char *arg) {
    int shop_nr, sh_virt;

    if (*arg) {
        if (isdigit(arg[0])) {
            sh_virt = atoi(arg);
            for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
                if (shop_index[shop_nr].vnum == sh_virt)
                    break;
            if (shop_nr >= top_shop) {
                char_printf(ch, "There is no such shop.\n");
                return;
            }
        } else {
            char_printf(ch, "That isn't a shop number!\n");
            return;
        }
    } else {
        for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
            if (ok_shop_room(shop_nr, world[ch->in_room].vnum))
                break;
        if (shop_nr >= top_shop) {
            char_printf(ch, "There's no shop here.\n");
            return;
        }
    }
    list_detailed_shop(ch, shop_nr);
}

void list_shops(CharData *ch, int start, int end) {
    int shop_nr, num = 0, room;
    bool any = false;

    strcpy(buf, "\n");
    for (shop_nr = 0; shop_nr < top_shop; shop_nr++) {
        if (shop_index[shop_nr].vnum >= start && shop_index[shop_nr].vnum <= end) {
            if (!any || !(num % 19)) {
                any = true;
                strcat(buf, " ##   Virtual   Keeper    Buy     Sell     Customers   Where\n");
                strcat(buf, "------------------------------------------------------------------------\n");
            }
            num++;
            room = real_room(SHOP_ROOM(shop_nr, 0));
            sprintf(buf2, "%3d  %6d    ", num, SHOP_NUM(shop_nr));
            if (SHOP_KEEPER(shop_nr) < 0)
                strcpy(buf1, "<NONE>");
            else
                sprintf(buf1, "%6d", mob_index[SHOP_KEEPER(shop_nr)].vnum);
            sprintf(END_OF(buf2), "%s     %3.2f    %3.2f     ", buf1, SHOP_SELLPROFIT(shop_nr),
                    SHOP_BUYPROFIT(shop_nr));
            strcat(buf2, customer_string(shop_nr, false));
            sprintf(END_OF(buf), "%s   %6d - %s\n", buf2, SHOP_ROOM(shop_nr, 0),
                    room == NOWHERE ? "<NOWHERE>" : world[room].name);
        }
    }

    if (any)
        page_string(ch, buf);
    else
        char_printf(ch, "No shops found.\n");
}

int vnum_shop(char *searchname, CharData *ch) {
    int nr, found = 0, room;

    for (nr = 0; nr < top_shop; nr++) {
        room = real_room(SHOP_ROOM(nr, 0));
        if (room != NOWHERE && isname(searchname, world[room].name)) {
            char_printf(ch, "{:3d}. [{:5d}] ({:5d}) {}\n", ++found, SHOP_NUM(nr), SHOP_ROOM(nr, 0), world[room].name);
        }
    }
    return found;
}

void show_shops(CharData *ch, char *arg) {
    int shop_nr;

    if (!*arg) {
        list_shops(ch, 0, MAX_VNUM);
        return;
    }

    else {
        if (!strcasecmp(arg, ".")) {
            for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
                if (ok_shop_room(shop_nr, world[ch->in_room].vnum))
                    break;

            if (shop_nr == top_shop) {
                char_printf(ch, "This isn't a shop!\n");
                return;
            }
        } else if (is_number(arg))
            shop_nr = atoi(arg) - 1;
        else
            shop_nr = -1;

        if ((shop_nr < 0) || (shop_nr >= top_shop)) {
            char_printf(ch, "Illegal shop number.\n");
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
    shop_index = nullptr;
    top_shop = -1;
}

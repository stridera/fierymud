/***************************************************************************
 * $Id: money.c,v 1.9 2010/04/25 22:51:45 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: money.c                                        Part of FieryMUD *
 *  Usage: It's all about the cash                                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "money.h"

#include "conf.h"
#include "db.h"
#include "interpreter.h"
#include "math.h"
#include "screen.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* coindefs[]
 *
 * name, shortname, initial, color, abbrev, scale
 */

struct coindef coindefs[NUM_COIN_TYPES] = {{"platinum", "plat", "p", "&6&b", "p", 1000},
                                           {"gold", NULL, "g", "&3&b", "g", 100},
                                           {"silver", NULL, "s", "&7&b", "s", 10},
                                           {"copper", NULL, "c", "&3", "c", 1}};

bool is_coin_name(char *name, int cointype) {
    if (!str_cmp(name, COIN_NAME(cointype)))
        return TRUE;
    if (COIN_SHORTNAME(cointype) && !str_cmp(name, COIN_SHORTNAME(cointype)))
        return TRUE;
    return FALSE;
}

#define APPENDCOIN(coin) sprintf(buf, "%s%s%d" ANRM " %s", buf, COIN_COLOR(coin), coins[coin], COIN_NAME(coin))

void statemoney(char *buf, const int coins[]) {
    int ctypes = 0, ctype2 = 0, amount = 0, i;

    *buf = '\0';

    for (i = 0; i < 4; i++) {
        if (coins[i]) {
            amount += coins[i];
            ctypes++;
            if (ctypes == 2)
                ctype2 = i;
        }
    }

    if (coins[PLATINUM]) {
        APPENDCOIN(PLATINUM);
        if (ctypes == 2)
            strcat(buf, " and ");
        else if (ctypes > 2)
            strcat(buf, ", ");
    }
    if (coins[GOLD]) {
        APPENDCOIN(GOLD);
        if (ctypes == 2 && ctype2 != GOLD)
            strcat(buf, " and ");
        else if (ctypes == 4 || (ctypes == 3 && ctype2 != GOLD))
            strcat(buf, ", ");
        else if (ctypes == 3)
            strcat(buf, ", and ");
    }
    if (coins[SILVER]) {
        APPENDCOIN(SILVER);
        if (ctypes == 2 && ctype2 != SILVER)
            strcat(buf, " and ");
        else if (ctypes == 4 || (ctypes == 3 && ctype2 == SILVER))
            strcat(buf, ", and ");
    }
    if (coins[COPPER])
        APPENDCOIN(COPPER);
    if (amount == 0)
        strcpy(buf, "0 coins");
    else if (amount > 1)
        strcat(buf, " coins");
    else
        strcat(buf, " coin");
}

#undef APPENDCOIN

#define COINBRIEF(coin, amount) sprintf(coinbuf, "%s%s%d%s" ANRM, coinbuf, COIN_COLOR(coin), amount, COIN_INITIAL(coin))

/* Prints a string about some money, in the requested number of spaces,
 * using at most two types of coin. */
void briefmoney(char *buf, int spaces, int amt) {
    bool topset = FALSE;
    bool lowset = FALSE;
    int toptype = 0, topval = 0, topchars = 0, lowtype = 0, lowval = 0, lowchars = 0, padding = 0;
    int i, maxval = 9;
    int coins[4];
    char coinbuf[100];

    /* Limit the number of digits so that there's 1 spot left over for a
     * coin-type designator (p, g, s, or c) */
    for (i = 0; i < spaces - 1; i++) {
        maxval = maxval * 10 + 9;
    }

    coins[PLATINUM] = amt / PLATINUM_SCALE;
    coins[GOLD] = (amt % PLATINUM_SCALE) / GOLD_SCALE;
    coins[SILVER] = (amt % GOLD_SCALE) / SILVER_SCALE;
    coins[COPPER] = (amt % SILVER_SCALE) / COPPER_SCALE;

    for (i = 0; i < 4; i++) {
        if (coins[i]) {
            if (!topset) {
                topset = TRUE;
                toptype = i;
                topval = coins[i];
                topchars = sprintf(buf, "%d", topval); /* count digits */
            } else if (!lowset) {
                lowset = TRUE;
                lowtype = i;
                lowval = coins[i];
                lowchars = sprintf(buf, "%d", lowval); /* count digits */
            }
        }
    }

    /* If the top coin type and low coin type can't fit within the requested
     * space, only the top will be used */
    if (lowset && lowchars + topchars > spaces - 2)
        lowset = FALSE;

    *buf = '\0';

    if (topset) {
        *coinbuf = '\0';
        padding = spaces - 1 - topchars;
        COINBRIEF(toptype, topval > maxval ? maxval : topval);
        if (lowset) {
            padding -= lowchars + 1;
            COINBRIEF(lowtype, lowval);
        }
        sprintf(buf, "%*s%s", padding, "", coinbuf);
    } else {
        sprintf(buf, "%*s0", spaces - 1, "");
    }
}

#undef COINBRIEF

bool parse_money(char **money, int coins[]) {
    char arg[MAX_INPUT_LENGTH];
    int amount, type;
    char *last;
    bool found_coins = FALSE;

    coins[PLATINUM] = 0;
    coins[GOLD] = 0;
    coins[SILVER] = 0;
    coins[COPPER] = 0;

    skip_spaces(money);

    while (**money) {
        *money = any_one_arg(last = *money, arg);
        if (!*arg)
            break;
        else if (!is_number(arg)) {
            *money = last;
            break; /* Not a number! */
        }
        amount = atoi(arg);
        *money = any_one_arg(*money, arg);
        if (!*arg || (type = parse_obj_name(NULL, arg, NULL, NUM_COIN_TYPES, coindefs, sizeof(struct coindef))) < 0) {
            *money = last;
            break;
        }
        coins[type] += amount;
        found_coins = TRUE;
    }

    return found_coins;
}

void money_desc(int amount, char **shortdesc, char **keywords) {
    static char sdbuf[128], kwbuf[128];

    if (amount <= 0) {
        log("SYSERR: Try to create negative or 0 money.");
        strcpy(sdbuf, "an erroneous object");
        strcpy(kwbuf, "erroneous object");
    }
    if (amount == 1) {
        strcpy(sdbuf, "a single coin");
        strcpy(kwbuf, "single coin");
    } else if (amount <= 9) {
        strcpy(sdbuf, "a tiny pile of coins");
        strcpy(kwbuf, "tiny pile coins");
    } else if (amount <= 20) {
        strcpy(sdbuf, "a handful of coins");
        strcpy(kwbuf, "handful coins");
    } else if (amount <= 75) {
        strcpy(sdbuf, "a little pile of coins");
        strcpy(kwbuf, "little pile coins");
    } else if (amount <= 200) {
        strcpy(sdbuf, "a small pile of coins");
        strcpy(kwbuf, "small pile coins");
    } else if (amount <= 1000) {
        strcpy(sdbuf, "a pile of coins");
        strcpy(kwbuf, "pile coins");
    } else if (amount <= 5000) {
        strcpy(sdbuf, "a big pile of coins");
        strcpy(kwbuf, "big pile coins");
    } else if (amount <= 10000) {
        strcpy(sdbuf, "a large heap of coins");
        strcpy(kwbuf, "large heap coins");
    } else if (amount <= 20000) {
        strcpy(sdbuf, "a huge mound of coins");
        strcpy(kwbuf, "huge mound coins");
    } else if (amount <= 75000) {
        strcpy(sdbuf, "an enormous mound of coins");
        strcpy(kwbuf, "enormous mound coins");
    } else if (amount <= 150000) {
        strcpy(sdbuf, "a small mountain of coins");
        strcpy(kwbuf, "small mountain coins");
    } else if (amount <= 250000) {
        strcpy(sdbuf, "a mountain of coins");
        strcpy(kwbuf, "mountain coins");
    } else if (amount <= 500000) {
        strcpy(sdbuf, "a huge mountain of coins");
        strcpy(kwbuf, "huge mountain coins");
    } else if (amount <= 1000000) {
        strcpy(sdbuf, "an enormous mountain of coins");
        strcpy(kwbuf, "enormous mountain coins");
    } else {
        strcpy(sdbuf, "an absolutely colossal mountain of coins");
        strcpy(kwbuf, "colossal mountain coins");
    }

    if (shortdesc)
        *shortdesc = sdbuf;
    if (keywords)
        *keywords = kwbuf;
}

struct obj_data *create_money(const int coins[]) {
    struct obj_data *obj;
    int amount = coins[PLATINUM] + coins[GOLD] + coins[SILVER] + coins[COPPER];
    int which;

    if (amount <= 0) {
        mprintf(L_ERROR, LVL_IMMORT, "SYSERR: create_money: Attempt to create %d money.", amount);
        return NULL;
    }

    for (which = 0; which < NUM_COIN_TYPES; ++which)
        if (coins[which] < 0) {
            mprintf(L_ERROR, LVL_IMMORT,
                    "SYSERR: create_money: Attempt to "
                    "create money with %d %s.",
                    coins[which], COIN_NAME(which));
            return NULL;
        }

    obj = create_obj();
    CREATE(obj->ex_description, struct extra_descr_data, 1);

    GET_OBJ_TYPE(obj) = ITEM_MONEY;
    GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
    GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM) = coins[PLATINUM];
    GET_OBJ_VAL(obj, VAL_MONEY_GOLD) = coins[GOLD];
    GET_OBJ_VAL(obj, VAL_MONEY_SILVER) = coins[SILVER];
    GET_OBJ_VAL(obj, VAL_MONEY_COPPER) = coins[COPPER];
    GET_OBJ_COST(obj) = CASH_VALUE(coins);
    obj->item_number = NOTHING;

    if (amount == 1) {
        if (coins[PLATINUM])
            which = PLATINUM;
        else if (coins[GOLD])
            which = GOLD;
        else if (coins[SILVER])
            which = SILVER;
        else if (coins[COPPER])
            which = COPPER;
        obj->name = strdupf("%s coin", COIN_NAME(which));
        obj->short_description = strdupf("a %s", obj->name);
        obj->description = strdupf("A single %s is lying here.", obj->name);
        obj->ex_description->keyword = strdup(obj->name);
        obj->ex_description->description = strdupf("A shiny %s!", obj->name);
    } else {
        money_desc(amount, &obj->short_description, &obj->name);
        obj->name = strdup(obj->name);
        obj->short_description = strdup(obj->short_description);
        obj->ex_description->keyword = strdup(obj->name);
        obj->description = strdupf("%s is lying here.", obj->short_description);
        cap_by_color(obj->description);
        if (amount < 10)
            obj->ex_description->description = strdupf("There are %d coins.", amount);
        else if (amount < 100)
            obj->ex_description->description = strdupf("There are about %d coins.", (amount / 10) * 10);
        else if (amount < 1000)
            obj->ex_description->description = strdupf("It looks to be about %d coins.", (amount / 100) * 100);
        else if (amount < 100000)
            obj->ex_description->description = strdupf("You guess there are maybe %d coins.",
                                                       ((amount / 1000) + random_number(0, amount / 1000)) * 1000);
        else
            obj->ex_description->description = strdup("There are a LOT of coins.");
    }

    return obj;
}

/***************************************************************************
 * $Log: money.c,v $
 * Revision 1.9  2010/04/25 22:51:45  mud
 * Fix crash bug in parse_money.c due to faulty boolean
 * logic in testing whether there are more args.
 *
 * Revision 1.8  2009/06/09 05:45:49  myc
 * Statemoney now says "0 coins" if there weren't any coins.
 * Declaring several parameters const.
 *
 * Revision 1.7  2009/03/21 08:09:03  myc
 * Fix bug in parse_money.
 *
 * Revision 1.6  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.5  2009/03/19 23:16:23  myc
 * parse_money now takes a char** and moves the pointer up to
 * just past any money phrase it parses.
 *
 * Revision 1.4  2009/03/09 21:43:50  myc
 * Change statemoney from strcat to strcpy semantics.
 *
 * Revision 1.3  2009/03/09 20:36:00  myc
 * Moved money functions from handler to here.
 *
 * Revision 1.2  2009/03/09 05:51:25  jps
 * Moved some money-related functions from utils to money
 *
 * Revision 1.1  2009/03/09 05:41:12  jps
 * Initial revision
 *
 ***************************************************************************/

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

#include "money.hpp"

#include "conf.hpp"
#include "db.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "objects.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <fmt/format.h>

/* coindefs[]
 *
 * name, shortname, initial, color, abbrev, scale
 */

CoinDef coindefs[NUM_COIN_TYPES] = {{"platinum", "plat", "p", "&6&b", "p", 1000},
                                    {"gold", nullptr, "g", "&3&b", "g", 100},
                                    {"silver", nullptr, "s", "&7&b", "s", 10},
                                    {"copper", nullptr, "c", "&3", "c", 1}};

bool is_coin_name(const char *name, int cointype) {
    if (!strcasecmp(name, COIN_NAME(cointype)))
        return true;
    if (COIN_SHORTNAME(cointype) && !strcasecmp(name, COIN_SHORTNAME(cointype)))
        return true;
    return false;
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
    bool topset = false;
    bool lowset = false;
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
                topset = true;
                toptype = i;
                topval = coins[i];
                topchars = sprintf(buf, "%d", topval); /* count digits */
            } else if (!lowset) {
                lowset = true;
                lowtype = i;
                lowval = coins[i];
                lowchars = sprintf(buf, "%d", lowval); /* count digits */
            }
        }
    }

    /* If the top coin type and low coin type can't fit within the requested
     * space, only the top will be used */
    if (lowset && lowchars + topchars > spaces - 2)
        lowset = false;

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
    bool found_coins = false;

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
        if (!*arg || (type = parse_obj_name(nullptr, arg, nullptr, NUM_COIN_TYPES, coindefs, sizeof(CoinDef))) < 0) {
            *money = last;
            break;
        }
        coins[type] += amount;
        found_coins = true;
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

ObjData *create_money(const int coins[]) {
    ObjData *obj;
    int amount = coins[PLATINUM] + coins[GOLD] + coins[SILVER] + coins[COPPER];
    int which;

    if (amount <= 0) {
        log(LogSeverity::Error, LVL_IMMORT, "SYSERR: create_money: Attempt to create {:d} money.", amount);
        return nullptr;
    }

    for (which = 0; which < NUM_COIN_TYPES; ++which)
        if (coins[which] < 0) {
            log(LogSeverity::Error, LVL_IMMORT, "SYSERR: create_money: Attempt to create money with {:d} {}.",
                coins[which], COIN_NAME(which));
            return nullptr;
        }

    obj = create_obj();
    CREATE(obj->ex_description, ExtraDescriptionData, 1);

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
        obj->name = strdup(fmt::format("{} coin", COIN_NAME(which)).c_str());
        obj->short_description = strdup(fmt::format("a {}", obj->name).c_str());
        obj->description = strdup(fmt::format("A single {} is lying here.", obj->name).c_str());
        obj->ex_description->keyword = strdup(obj->name);
        obj->ex_description->description = strdup(fmt::format("A shiny {}!", obj->name).c_str());
    } else {
        money_desc(amount, &obj->short_description, &obj->name);
        obj->name = strdup(obj->name);
        obj->short_description = strdup(obj->short_description);
        obj->ex_description->keyword = strdup(obj->name);
        obj->description = strdup(fmt::format("{} is lying here.", obj->short_description).c_str());
        cap_by_color(obj->description);
        if (amount < 10)
            obj->ex_description->description = strdup(fmt::format("There are {} coins.", amount).c_str());
        else if (amount < 100)
            obj->ex_description->description =
                strdup(fmt::format("There are about {} coins.", (amount / 10) * 10).c_str());
        else if (amount < 1000)
            obj->ex_description->description =
                strdup(fmt::format("It looks to be about {} coins.", (amount / 100) * 100).c_str());
        else if (amount < 100000)
            obj->ex_description->description =
                strdup(fmt::format("You guess there are maybe {} coins.",
                                   ((amount / 1000) + random_number(0, amount / 1000)) * 1000)
                           .c_str());
        else
            obj->ex_description->description = strdup("There are a LOT of coins.");
    }

    return obj;
}

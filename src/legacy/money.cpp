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
#include "string_utils.hpp"
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

int parse_coin_type(std::string_view name) noexcept {
    for (int i = 0; i < NUM_COIN_TYPES; i++) {
        if (is_coin_name(name, i)) {
            return i;
        }
    }
    return -1;
}

bool is_coin_name(std::string_view name, int cointype) {
    if (matches(name, COIN_NAME(cointype)))
        return true;
    if (COIN_SHORTNAME(cointype) && matches(name, COIN_SHORTNAME(cointype)))
        return true;
    return false;
}

std::string statemoney(const Money coins) noexcept {
    if (coins[PLATINUM] == 0 && coins[GOLD] == 0 && coins[SILVER] == 0 && coins[COPPER] == 0) {
        return "Nothing";
    }
    std::vector<std::string> coin_strings;
    bool final_plural = false;
    for (int i = 0; i < NUM_COIN_TYPES; i++) {
        if (coins[i]) {
            coin_strings.push_back(fmt::format("{}{} {}", COIN_COLOR(i), coins[i], COIN_NAME(i)));
            if (coins[i] > 1) {
                final_plural = true;
            }
        }
    }
    return join_strings(coin_strings, ", ", ", and ") + fmt::format(" coin{}", final_plural ? "s" : "");
}

/* Prints a string about some money, in the requested number of spaces, using at most two types of coin. */
std::string briefmoney(int spaces, int amt) noexcept {
    bool topset = false;
    bool lowset = false;
    int toptype = 0, topval = 0, topchars = 0, lowtype = 0, lowval = 0, lowchars = 0, padding = 0;
    int i, maxval = 9;
    int coins[4];
    std::string coinbuf;

    /* Limit the number of digits so that there's 1 spot left over for a coin-type designator (p, g, s, or c) */
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
                topchars = fmt::formatted_size("{}", topval); /* count digits */
            } else if (!lowset) {
                lowset = true;
                lowtype = i;
                lowval = coins[i];
                lowchars = fmt::formatted_size("{}", lowval); /* count digits */
            }
        }
    }

    /* If the top coin type and low coin type can't fit within the requested space, only the top will be used */
    if (lowset && lowchars + topchars > spaces - 2)
        lowset = false;

    if (topset) {
        coinbuf.clear();
        padding = spaces - 1 - topchars;
        coinbuf += fmt::format("{}{}{}", COIN_COLOR(toptype), std::min(topval, maxval), COIN_INITIAL(toptype));
        if (lowset) {
            padding -= lowchars + 1;
            coinbuf += fmt::format("{}{}{}", COIN_COLOR(lowtype), lowval, COIN_INITIAL(lowtype));
        }
        return fmt::format("{:>{}}{}", "", padding, coinbuf);
    } else {
        return fmt::format("{:>{}}0", "", spaces - 1);
    }
}

std::optional<Money> parse_money(std::string_view input) noexcept {
    int amount, type;

    Money coins;
    coins[PLATINUM] = 0;
    coins[GOLD] = 0;
    coins[SILVER] = 0;
    coins[COPPER] = 0;

    input = trim(input);
    while (!input.empty()) {
        // Get the next argument
        auto arg = getline(input, ' ');

        // If the argument is a number, then we know it's the amount of coins and the next argument is the type
        if (is_integer(arg)) {
            amount = svtoi(arg);
            arg = getline(input, ' ');
        } else {
            // If we're here, it's most likely a combination of amount and type
            // We need to split the string into the amount and type
            size_t pos = 0;
            while (pos < arg.size() && isdigit(arg[pos])) {
                ++pos;
            }
            if (pos == 0) {
                return std::nullopt;
            }
            amount = svtoi(arg.substr(0, pos));
            arg.remove_prefix(pos);
        }

        if (amount <= 0) {
            log("SYSERR: parse_money: Attempt to create {} money.", amount);
            return std::nullopt;
        }

        // Find the type of coin
        for (type = 0; type < NUM_COIN_TYPES; ++type) {
            if (is_coin_name(arg, type)) {
                break;
            }
        }
        coins[type] += amount;
    }

    return coins;
}

std::string money_desc(int amount) {
    if (amount == 1) {
        return "single coin";
    } else if (amount <= 9) {
        return "tiny pile coins";
    } else if (amount <= 20) {
        return "handful coins";
    } else if (amount <= 75) {
        return "little pile coins";
    } else if (amount <= 200) {
        return "small pile coins";
    } else if (amount <= 1000) {
        return "pile coins";
    } else if (amount <= 5000) {
        return "big pile coins";
    } else if (amount <= 10000) {
        return "large heap coins";
    } else if (amount <= 20000) {
        return "huge mound coins";
    } else if (amount <= 75000) {
        return "enormous mound coins";
    } else if (amount <= 150000) {
        return "small mountain coins";
    } else if (amount <= 250000) {
        return "mountain coins";
    } else if (amount <= 500000) {
        return "huge mountain coins";
    } else if (amount <= 1000000) {
        return "enormous mountain coins";
    } else {
        return "colossal mountain coins";
    }
}

ObjData *create_money(const Money coins) {
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
        strcpy(obj->name, fmt::format("{} coin", COIN_NAME(which)).c_str());
        strcpy(obj->short_description, fmt::format("a {}", obj->name).c_str());
        strcpy(obj->description, fmt::format("A single {} is lying here.", obj->name).c_str());
        obj->ex_description->keyword = strdup(obj->name);
        strcpy(obj->ex_description->description, fmt::format("A shiny {}!", obj->name).c_str());
    } else {
        auto guess = [](int amount, int scale) -> int {
            return ((amount / scale) + random_number(0, amount / scale)) * scale;
        };
        obj->name = strdup(money_desc(amount).c_str());
        obj->short_description = strdup(fmt::format("a {}", obj->name).c_str());
        obj->ex_description->keyword = obj->name;
        obj->description = strdup(fmt::format("{} is lying here.", capitalize_first(obj->short_description)).c_str());
        if (amount < 10)
            obj->ex_description->description = strdup(fmt::format("There are {} coins.", amount).c_str());
        else if (amount < 100)
            obj->ex_description->description =
                strdup(fmt::format("There are about {} coins.", guess(amount, 10)).c_str());
        else if (amount < 1000)
            obj->ex_description->description =
                strdup(fmt::format("It looks to be about {} coins.", guess(amount, 100)).c_str());
        else if (amount < 100000)
            obj->ex_description->description =
                strdup(fmt::format("You guess there are maybe {} coins.", guess(amount, 1000)).c_str());
        else
            obj->ex_description->description = strdup("There are a LOT of coins.");
    }

    return obj;
}

bool charge_char(CharData *ch, int amount) noexcept {
    if (amount <= 0) {
        log("SYSERR: charge_char: Attempt to charge {:d} money.", amount);
        return false;
    }

    if (GET_CASH(ch) < amount) {
        return false;
    }

    for (int i = 0; i < NUM_COIN_TYPES; ++i) {
        while (amount >= COIN_SCALE(i) && GET_COINS(ch)[i] > 0) {
            amount -= COIN_SCALE(i);
            GET_COINS(ch)[i]--;
        }
    }
    return true;
}
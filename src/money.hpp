/***************************************************************************
 *   File: money.h                                        Part of FieryMUD *
 *  Usage: header file for money, I guess                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

struct CoinDef {
    const char *name;
    const char *shortname;
    const char *initial;
    const char *color;
    const char *abbrev;
    int scale;
};

extern CoinDef coindefs[NUM_COIN_TYPES];

#define VALID_COIN(coin) (coin >= 0 && coin < NUM_COIN_TYPES)
#define COIN_NAME(coin) (VALID_COIN(coin) ? coindefs[coin].name : "coin")
#define COIN_SHORTNAME(coin) (VALID_COIN(coin) ? coindefs[coin].shortname : "c")
bool is_coin_name(const char *name, int cointype);
#define COIN_COLOR(coin) (VALID_COIN(coin) ? coindefs[coin].color : "&9&b")
#define COIN_INITIAL(coin) (VALID_COIN(coin) ? coindefs[coin].initial : "?")

/* Old below */

#define PLATINUM_SCALE 1000
#define GOLD_SCALE 100
#define SILVER_SCALE 10
#define COPPER_SCALE 1

#define PLATINUM_PART(amt) ((amt) / PLATINUM_SCALE)
#define GOLD_PART(amt) (((amt) % PLATINUM_SCALE) / GOLD_SCALE)
#define SILVER_PART(amt) (((amt) % GOLD_SCALE) / SILVER_SCALE)
#define COPPER_PART(amt) (((amt) % SILVER_SCALE) / COPPER_SCALE)
#define CASH_VALUE(coins)                                                                                              \
    ((coins)[PLATINUM] * PLATINUM_SCALE + (coins)[GOLD] * GOLD_SCALE + (coins)[SILVER] * SILVER_SCALE +                \
     (coins)[COPPER] * COPPER_SCALE)

void statemoney(char *buf, const int coins[]);
bool parse_money(char **money, int coins[]);
void briefmoney(char *buf, int spaces, int amt);
void money_desc(int amount, const char **shortdesc, const char **keywords);
ObjData *create_money(const int coins[]);

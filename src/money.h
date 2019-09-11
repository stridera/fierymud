/***************************************************************************
 * $Id: money.h,v 1.7 2009/06/09 05:46:30 myc Exp $
 ***************************************************************************/
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

#ifndef __FIERY_MONEY_H
#define __FIERY_MONEY_H

#define PLATINUM        0
#define PLAT PLATINUM
#define GOLD            1
#define SILVER          2
#define COPPER          3
#define NUM_COIN_TYPES  4

struct coindef {
   char *name;
   char *shortname;
   char *initial;
   char *color;
   char *abbrev;
   int scale;
};

extern struct coindef coindefs[];

#define VALID_COIN(coin) (coin >= 0 && coin < NUM_COIN_TYPES)
#define COIN_NAME(coin) (VALID_COIN(coin) ? coindefs[coin].name : "coin")
#define COIN_SHORTNAME(coin) (VALID_COIN(coin) ? coindefs[coin].shortname : "c")
bool is_coin_name(char *name, int cointype);
#define COIN_COLOR(coin) (VALID_COIN(coin) ? coindefs[coin].color : "&9&b")
#define COIN_INITIAL(coin) (VALID_COIN(coin) ? coindefs[coin].initial : "?")

/* Old below */

#define PLATINUM_SCALE  1000
#define GOLD_SCALE      100
#define SILVER_SCALE    10
#define COPPER_SCALE    1

#define PLATINUM_PART(amt) ((amt) / PLATINUM_SCALE)
#define GOLD_PART(amt)     (((amt) % PLATINUM_SCALE) / GOLD_SCALE)
#define SILVER_PART(amt)   (((amt) % GOLD_SCALE) / SILVER_SCALE)
#define COPPER_PART(amt)   (((amt) % SILVER_SCALE) / COPPER_SCALE)
#define CASH_VALUE(coins)  ((coins)[PLATINUM] * PLATINUM_SCALE + \
                            (coins)[GOLD] * GOLD_SCALE + \
                            (coins)[SILVER] * SILVER_SCALE + \
                            (coins)[COPPER] * COPPER_SCALE)

void statemoney(char *buf, const int coins[]);
bool parse_money(char **money, int coins[]);
void briefmoney(char *buf, int spaces, int amt);
void money_desc(int amount, char **shortdesc, char **keywords);
struct obj_data *create_money(const int coins[]);

#endif

/***************************************************************************
 * $Log: money.h,v $
 * Revision 1.7  2009/06/09 05:46:30  myc
 * Declaring several functions' parameters const.
 *
 * Revision 1.6  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.5  2009/03/19 23:16:23  myc
 * parse_money now takes a char** and moves the pointer up to
 * just past any money phrase it parses.
 *
 * Revision 1.4  2009/03/13 04:40:10  jps
 * Fix up some money macros.
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

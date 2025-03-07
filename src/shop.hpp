/***************************************************************************
 *   File: shop.h                                         Part of FieryMUD *
 *  Usage: shop file definitions,ures, constants                    *
 *     By: Jeff Fink                                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "specprocs.hpp" /* for SPECIAL() */
#include "structs.hpp"
#include "sysdep.hpp"

struct ShopBuyData {
    int type;
    int amount;
    std::string_view keywords;
};
#define BUY_AMOUNT(i) ((i).amount)
#define BUY_TYPE(i) ((i).type)
#define BUY_WORD(i) ((i).keywords)

struct ShopData {
    int vnum;       /* Virtual number of this shop		*/
    int *producing; /* Which item to produce (vnum)	*/
    int *amount;
    float profit_buy;          /* Factor to multiply cost with		*/
    float profit_sell;         /* Factor to multiply cost with		*/
    ShopBuyData *type;         /* Which items to trade			*/
    std::string no_such_item1; /* Message if keeper hasn't got an item	*/
    std::string no_such_item2; /* Message if player hasn't got an item	*/
    std::string missing_cash1; /* Message if keeper hasn't got cash	*/
    std::string missing_cash2; /* Message if player hasn't got cash	*/
    std::string do_not_buy;    /* If keeper dosn't buy such things	*/
    std::string message_buy;   /* Message when player buys item	*/
    std::string message_sell;  /* Message when player sells item	*/
    int temper1;               /* How does keeper react if no money	*/
    int bitvector;             /* Can attack? Use bank? Cast here?	*/
    int keeper;                /* The mobil who owns the shop (vnum)*/
    int with_who;              /* Who does the shop trade with?	*/
    int *in_room;              /* Where is the shop?			*/
    int open1, open2;          /* When does the shop open?		*/
    int close1, close2;        /* When does the shop close?		*/
    int bankAccount;           /* Store all gold over 15000 (disabled)	*/
    int lastsort;              /* How many items are sorted in inven?	*/
    SPECIAL(*func);            /* Secondary spec_proc for shopkeeper	*/
};

#define MAX_TRADE 5         /* List maximums for compatibility	*/
#define MAX_PROD 5          /*	with shops before v3.0		*/
#define VERSION3_TAG "v3.0" /* The file has v3.0 shops in it!	*/
#define MAX_SHOP_OBJ 100    /* "Soft" maximum for list maximums	*/

/* Pretty general macros that could be used elsewhere */
#define IS_GOD(ch) (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD))
#define GET_OBJ_NUM(obj) (obj->item_number)
#define END_OF(buffer) ((buffer) + strlen((buffer)))

/* Possible states for objects trying to be sold */
#define OBJECT_DEAD 0
#define OBJECT_NOTOK 1
#define OBJECT_OK 2

/* Types of lists to read */
#define LIST_PRODUCE 0
#define LIST_TRADE 1
#define LIST_ROOM 2

/* Types of prices */
#define PURCHASE_PRICE 0
#define SERVICE_PRICE 1

/* Whom will we not trade with (bitvector for SHOP_TRADE_WITH()) */
#define TRADE_NOGOOD 1
#define TRADE_NOEVIL 2
#define TRADE_NONEUTRAL 4
#define TRADE_NOMAGIC_USER 8
#define TRADE_NOCLERIC 16
#define TRADE_NOTHIEF 32
#define TRADE_NOWARRIOR 64

struct StackData {
    int data[100];
    int len;
};

#define S_DATA(stack, index) ((stack)->data[(index)])
#define S_LEN(stack) ((stack)->len)

/* Which expression type we are now parsing */
#define OPER_OPEN_PAREN 0
#define OPER_CLOSE_PAREN 1
#define OPER_OR 2
#define OPER_AND 3
#define OPER_NOT 4
#define MAX_OPER 4

extern const std::string_view operator_str[];

#define SHOP_NUM(i) (shop_index[(i)].vnum)
#define SHOP_KEEPER(i) (shop_index[(i)].keeper)
#define SHOP_OPEN1(i) (shop_index[(i)].open1)
#define SHOP_CLOSE1(i) (shop_index[(i)].close1)
#define SHOP_OPEN2(i) (shop_index[(i)].open2)
#define SHOP_CLOSE2(i) (shop_index[(i)].close2)
#define SHOP_ROOM(i, num) (shop_index[(i)].in_room[(num)])
#define SHOP_BUYTYPE(i, num) (BUY_TYPE(shop_index[(i)].type[(num)]))
#define SHOP_BUYAMOUNT(i, num) (BUY_AMOUNT(shop_index[(i)].type[(num)]))
#define SHOP_BUYWORD(i, num) (BUY_WORD(shop_index[(i)].type[(num)]))
#define SHOP_PRODUCT(i, num) (shop_index[(i)].producing[(num)])
#define SHOP_AMOUNT(i, num) (shop_index[(i)].amount[(num)])
#define SHOP_BANK(i) (shop_index[(i)].bankAccount)
#define SHOP_BROKE_TEMPER(i) (shop_index[(i)].temper1)
#define SHOP_BITVECTOR(i) (shop_index[(i)].bitvector)
#define SHOP_TRADE_WITH(i) (shop_index[(i)].with_who)
#define SHOP_SORT(i) (shop_index[(i)].lastsort)
#define SHOP_BUYPROFIT(i) (shop_index[(i)].profit_buy)
#define SHOP_SELLPROFIT(i) (shop_index[(i)].profit_sell)
#define SHOP_FUNC(i) (shop_index[(i)].func)

#define NOTRADE_GOOD(i) (IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOGOOD))
#define NOTRADE_EVIL(i) (IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOEVIL))
#define NOTRADE_NEUTRAL(i) (IS_SET(SHOP_TRADE_WITH((i)), TRADE_NONEUTRAL))
#define NOTRADE_MAGIC_USER(i) (IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOMAGIC_USER))
#define NOTRADE_CLERIC(i) (IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOCLERIC))
#define NOTRADE_THIEF(i) (IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOTHIEF))
#define NOTRADE_WARRIOR(i) (IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOWARRIOR))

#define WILL_START_FIGHT 1
#define WILL_BANK_MONEY 2

#define SHOP_KILL_CHARS(i) (IS_SET(SHOP_BITVECTOR(i), WILL_START_FIGHT))
#define SHOP_USES_BANK(i) (IS_SET(SHOP_BITVECTOR(i), WILL_BANK_MONEY))

extern const std::string_view trade_letters[];
extern const std::string_view shop_bits[];

#define MIN_OUTSIDE_BANK 5000
#define MAX_OUTSIDE_BANK 15000

bool give_shopkeeper_reject(CharData *ch, CharData *vict, ObjData *obj);

extern int top_shop;
extern ShopData *shop_index;

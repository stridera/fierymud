/***************************************************************************
 *   File: sedit.c                                        Part of FieryMUD *
 *  Usage:                                                                 *
 *     By: Harvey Gilpin of TwyliteMud by Rv. (shameless plug)             *
 *         Copyright 1996 Harvey Gilpin.                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "objects.hpp"
#include "olc.hpp"
#include "shop.hpp"
#include "specprocs.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/*-------------------------------------------------------------------*/
/*. Handy  macros .*/

#define S_NUM(i) ((i)->vnum)
#define S_KEEPER(i) ((i)->keeper)
#define S_OPEN1(i) ((i)->open1)
#define S_CLOSE1(i) ((i)->close1)
#define S_OPEN2(i) ((i)->open2)
#define S_CLOSE2(i) ((i)->close2)
#define S_BANK(i) ((i)->bankAccount)
#define S_BROKE_TEMPER(i) ((i)->temper1)
#define S_BITVECTOR(i) ((i)->bitvector)
#define S_NOTRADE(i) ((i)->with_who)
#define S_SORT(i) ((i)->lastsort)
#define S_BUYPROFIT(i) ((i)->profit_buy)
#define S_SELLPROFIT(i) ((i)->profit_sell)
#define S_FUNC(i) ((i)->func)

#define S_ROOMS(i) ((i)->in_room)
#define S_PRODUCTS(i) ((i)->producing)
#define S_AMOUNTS(i) ((i)->amount)
#define S_NAMELISTS(i) ((i)->type)
#define S_ROOM(i, num) ((i)->in_room[(num)])
#define S_PRODUCT(i, num) ((i)->producing[(num)])
#define S_AMOUNT(i, num) ((i)->amount[(num)])
#define S_BUYTYPE(i, num) (BUY_TYPE((i)->type[(num)]))
#define S_BUYWORD(i, num) (BUY_WORD((i)->type[(num)]))

#define S_NOITEM1(i) ((i)->no_such_item1)
#define S_NOITEM2(i) ((i)->no_such_item2)
#define S_NOCASH1(i) ((i)->missing_cash1)
#define S_NOCASH2(i) ((i)->missing_cash2)
#define S_NOBUY(i) ((i)->do_not_buy)
#define S_BUY(i) ((i)->message_buy)
#define S_SELL(i) ((i)->message_sell)

/*-------------------------------------------------------------------*/
/*. Function prototypes .*/

int real_shop(int vshop_num);
void sedit_setup_new(DescriptorData *d);
void sedit_setup_existing(DescriptorData *d, int rmob_num);
void sedit_parse(DescriptorData *d, std::string_view arg);
void sedit_disp_menu(DescriptorData *d);
void sedit_namelist_menu(DescriptorData *d);
void sedit_types_menu(DescriptorData *d);
void sedit_products_menu(DescriptorData *d);
void sedit_rooms_menu(DescriptorData *d);
void sedit_compact_rooms_menu(DescriptorData *d);
void sedit_shop_flags_menu(DescriptorData *d);
void sedit_no_trade_menu(DescriptorData *d);
void sedit_save_internally(DescriptorData *d);
void sedit_save_to_disk(int zone_num);
void copy_shop(ShopData *tshop, ShopData *fshop);
void copy_int_list(int **tlist, int *flist);
void copy_type_list(ShopBuyData **tlist, ShopBuyData *flist);
int int_list_length(int *tlist);
void sedit_add_to_type_list(ShopBuyData **list, ShopBuyData *new_str);
void sedit_remove_from_type_list(ShopBuyData **list, int num);
void free_shop_strings(ShopData *shop);
void free_type_list(ShopBuyData **list);
void free_shop(ShopData *shop);
void sedit_modify_string(std::string_view str, std::string_view new_str);

/*. External .*/
SPECIAL(shop_keeper);

/*-------------------------------------------------------------------*\
  utility functions
\*-------------------------------------------------------------------*/

void sedit_setup_new(DescriptorData *d) {
    ShopData *shop;

    /*. Alloc some shop shaped space . */
    CREATE(shop, ShopData, 1);

    /*. Some default values . */
    S_KEEPER(shop) = -1;
    S_CLOSE1(shop) = 28;
    S_BUYPROFIT(shop) = 1.0;
    S_SELLPROFIT(shop) = 1.0;
    /*. Some default strings . */
    S_NOITEM1(shop) = strdup("%s Sorry, I don't stock that item.");
    S_NOITEM2(shop) = strdup("%s You don't seem to have that.");
    S_NOCASH1(shop) = strdup("%s I can't afford that!");
    S_NOCASH2(shop) = strdup("%s You are too poor!");
    S_NOBUY(shop) = strdup("%s I don't trade in such items.");
    S_BUY(shop) = strdup("%s Thanks for the deal.");
    S_SELL(shop) = strdup("%s Good doing business with you.");
    /*
     * Stir the lists lightly.
     */
    CREATE(S_PRODUCTS(shop), int, 1);
    CREATE(S_AMOUNTS(shop), int, 1);
    S_PRODUCT(shop, 0) = -1;
    S_AMOUNT(shop, 0) = -1;
    CREATE(S_ROOMS(shop), int, 1);

    S_ROOM(shop, 0) = -1;
    CREATE(S_NAMELISTS(shop), ShopBuyData, 1);

    S_BUYTYPE(shop, 0) = -1;

    /*
     * Presto! A shop.
     */

    OLC_SHOP(d) = shop;
    sedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void sedit_setup_existing(DescriptorData *d, int rshop_num) {
    /*
     * Create a scratch shop structure.
     */
    CREATE(OLC_SHOP(d), ShopData, 1);

    copy_shop(OLC_SHOP(d), shop_index + rshop_num);
    sedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void copy_shop(ShopData *tshop, ShopData *fshop) {
    /*
     * Copy basic information over.
     */
    S_NUM(tshop) = S_NUM(fshop);
    S_KEEPER(tshop) = S_KEEPER(fshop);
    S_OPEN1(tshop) = S_OPEN1(fshop);
    S_CLOSE1(tshop) = S_CLOSE1(fshop);
    S_OPEN2(tshop) = S_OPEN2(fshop);
    S_CLOSE2(tshop) = S_CLOSE2(fshop);
    S_BANK(tshop) = S_BANK(fshop);
    S_BROKE_TEMPER(tshop) = S_BROKE_TEMPER(fshop);
    S_BITVECTOR(tshop) = S_BITVECTOR(fshop);
    S_NOTRADE(tshop) = S_NOTRADE(fshop);
    S_SORT(tshop) = S_SORT(fshop);
    S_BUYPROFIT(tshop) = S_BUYPROFIT(fshop);
    S_SELLPROFIT(tshop) = S_SELLPROFIT(fshop);
    S_FUNC(tshop) = S_FUNC(fshop);

    /*
     * Copy lists over.
     */
    copy_int_list(&(S_ROOMS(tshop)), S_ROOMS(fshop));
    copy_int_list(&(S_PRODUCTS(tshop)), S_PRODUCTS(fshop));
    copy_int_list(&(S_AMOUNTS(tshop)), S_AMOUNTS(fshop));
    copy_type_list(&(tshop->type), fshop->type);

    /*. Copy notification strings over . */
    free_shop_strings(tshop);
    S_NOITEM1(tshop) = strdup(S_NOITEM1(fshop));
    S_NOITEM2(tshop) = strdup(S_NOITEM2(fshop));
    S_NOCASH1(tshop) = strdup(S_NOCASH1(fshop));
    S_NOCASH2(tshop) = strdup(S_NOCASH2(fshop));
    S_NOBUY(tshop) = strdup(S_NOBUY(fshop));
    S_BUY(tshop) = strdup(S_BUY(fshop));
    S_SELL(tshop) = strdup(S_SELL(fshop));
}

/*-------------------------------------------------------------------*/

int int_list_length(int *list) {
    int i;
    for (i = 0; (list)[i] != -1; i++)
        ;
    return i;
}

/*-------------------------------------------------------------------*/
/* Make a copy of an integer list. The new copy is of a specified length.
 * If the copy is longer than the original, the extra spots have value 0. */

void _copy_int_list(int **tlist, int *flist, int newlength) {
    int i, flen;

    if (*tlist)
        free(*tlist);

    CREATE(*tlist, int, newlength + 1);

    flen = int_list_length(flist);

    for (i = 0; i < flen && i < newlength; i++)
        (*tlist)[i] = flist[i];

    (*tlist)[newlength] = -1;
}

/*-------------------------------------------------------------------*/

void add_to_int_list(int **list, int newstring) {
    int *nlist = nullptr;
    int num_items = int_list_length(*list) + 1;

    /*. make a new list and slot in the new entry . */
    _copy_int_list(&nlist, *list, num_items);
    nlist[num_items - 1] = newstring;

    /*. Out with the old, in with the new . */
    free(*list);
    *list = nlist;
}

/*-------------------------------------------------------------------*/

void remove_from_int_list(int **list, int num) {
    int *nlist = nullptr, i;
    int num_items = int_list_length(*list) - 1;

    _copy_int_list(&nlist, *list, num_items);
    for (i = num; i < num_items; i++)
        nlist[i] = (*list)[i + 1];

    free(*list);
    *list = nlist;
}

/*-------------------------------------------------------------------*/
/*. Copy a -1 terminated integer array list .*/

void copy_int_list(int **tlist, int *flist) { _copy_int_list(tlist, flist, int_list_length(flist)); }

/*-------------------------------------------------------------------*/
/*. Copy a -1 terminated (in the type field) shop_buy_data
  array list .*/

void copy_type_list(ShopBuyData **tlist, ShopBuyData *flist) {
    int num_items, i;

    if (*tlist)
        free_type_list(tlist);

    /*
     * Count number of entries.
     */
    for (i = 0; BUY_TYPE(flist[i]) != -1; i++)
        ;
    num_items = i + 1;

    /*. Make space for entries . */
    CREATE(*tlist, ShopBuyData, num_items);

    /*. Copy entries over . */
    i = 0;
    do {
        (*tlist)[i].type = flist[i].type;
        if (BUY_WORD(flist[i]))
            BUY_WORD((*tlist)[i]) = strdup(BUY_WORD(flist[i]));
    } while (++i < num_items);
}

/*-------------------------------------------------------------------*/

void sedit_remove_from_type_list(ShopBuyData **list, int num) {
    int i, num_items;
    ShopBuyData *nlist;

    /*
     * Count number of entries.
     */
    for (i = 0; (*list)[i].type != -1; i++)
        ;

    if (num >= i || num < 0)
        return;
    num_items = i;

    CREATE(nlist, ShopBuyData, num_items);

    for (i = 0; i < num_items; i++)
        nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];
    free(BUY_WORD((*list)[num]));
    free(*list);
    *list = nlist;
}

/*-------------------------------------------------------------------*/

void sedit_add_to_type_list(ShopBuyData **list, ShopBuyData *new_data) {
    int i, num_items;
    ShopBuyData *nlist;

    /*
     * Count number of entries.
     */
    for (i = 0; (*list)[i].type != -1; i++)
        ;
    num_items = i;

    /*. make a new list and slot in the new entry . */
    CREATE(nlist, ShopBuyData, num_items + 2);

    for (i = 0; i < num_items; i++)
        nlist[i] = (*list)[i];
    nlist[num_items] = *new_data;
    nlist[num_items + 1].type = -1;

    /*. Out with the old, in with the new . */
    free(*list);
    *list = nlist;
}

/*-------------------------------------------------------------------*/

void sedit_unique_change(ShopData *shop, int change) {

    if (S_AMOUNT(shop, change)) /*switch whatever it is at */
        S_AMOUNT(shop, change) = 0;
    else
        S_AMOUNT(shop, change) = 1;
}

/*-------------------------------------------------------------------*/
/*. Free all the notice character strings in a shop structure .*/

void free_shop_strings(ShopData *shop) {
    if (S_NOITEM1(shop)) {
        free(S_NOITEM1(shop));
        S_NOITEM1(shop) = nullptr;
    }
    if (S_NOITEM2(shop)) {
        free(S_NOITEM2(shop));
        S_NOITEM2(shop) = nullptr;
    }
    if (S_NOCASH1(shop)) {
        free(S_NOCASH1(shop));
        S_NOCASH1(shop) = nullptr;
    }
    if (S_NOCASH2(shop)) {
        free(S_NOCASH2(shop));
        S_NOCASH2(shop) = nullptr;
    }
    if (S_NOBUY(shop)) {
        free(S_NOBUY(shop));
        S_NOBUY(shop) = nullptr;
    }
    if (S_BUY(shop)) {
        free(S_BUY(shop));
        S_BUY(shop) = nullptr;
    }
    if (S_SELL(shop)) {
        free(S_SELL(shop));
        S_SELL(shop) = nullptr;
    }
}

/*-------------------------------------------------------------------*/
/*. Free a type list and all the strings it contains .*/

void free_type_list(ShopBuyData **list) {
    int i;

    for (i = 0; (*list)[i].type != -1; i++)
        if (BUY_WORD((*list)[i]))
            free(BUY_WORD((*list)[i]));
    free(*list);
    *list = nullptr;
}

/*-------------------------------------------------------------------*/
/*. Free up the whole shop structure and it's content .*/

void free_shop(ShopData *shop) {
    free_shop_strings(shop);
    free_type_list(&(S_NAMELISTS(shop)));
    free(S_ROOMS(shop));
    free(S_PRODUCTS(shop));
    free(S_AMOUNTS(shop));
    free(shop);
}

/*-------------------------------------------------------------------*/

int real_shop(int vshop_num) {
    int rshop_num;

    for (rshop_num = 0; rshop_num < top_shop; rshop_num++)
        if (SHOP_NUM(rshop_num) == vshop_num)
            return rshop_num;

    return -1;
}

/*-------------------------------------------------------------------*/
/*. Generic string modifyer for shop keeper messages .*/

void sedit_modify_string(std::string_view str, std::string_view new_str) {
    std::string_view pointer;

    /*
     * Check the '%s' is present, if not, add it.
     */
    if (*new_str != '%') {
        strcpy(buf, "%s ");
        strcat(buf, new_str);
        pointer = buf;
    } else
        pointer = new_str;

    if (*str)
        free(*str);
    *str = strdup(pointer);
}

/*-------------------------------------------------------------------*/

void sedit_save_internally(DescriptorData *d) {
    int rshop, found = 0;
    ShopData *shop;
    ShopData *new_index;

    rshop = real_shop(OLC_NUM(d));
    shop = OLC_SHOP(d);
    S_NUM(shop) = OLC_NUM(d);

    if (rshop > -1) { /* The shop already exists, just update it. */
        copy_shop((shop_index + rshop), shop);
    } else { /* Doesn't exist - have to insert it. */
        CREATE(new_index, ShopData, top_shop + 1);

        for (rshop = 0; rshop < top_shop; rshop++) {
            if (!found) {                           /* Is this the place? */
                if (SHOP_NUM(rshop) > OLC_NUM(d)) { /* Yep, stick it in here. */
                    found = 1;
                    copy_shop(&(new_index[rshop]), shop);
                    /*
                     * Move the entry that used to go here up a place.
                     */
                    new_index[rshop + 1] = shop_index[rshop];
                } else /* This isn't the place, copy over info. */
                    new_index[rshop] = shop_index[rshop];
            } else { /* Shop's already inserted, copy rest over. */
                new_index[rshop + 1] = shop_index[rshop];
            }
        }
        if (!found)
            copy_shop(&(new_index[rshop]), shop);

        /*. Switch index in . */
        free(shop_index);
        shop_index = new_index;
        top_shop++;
    }
    olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_SHOP);
}

/*-------------------------------------------------------------------*/

void sedit_save_to_disk(int zone_num) {
    int i, j, rshop, zone, top;
    FILE *shop_file;
    char fname[64];
    ShopData *shop;

    zone = zone_table[zone_num].number;
    top = zone_table[zone_num].top;

    sprintf(fname, "%s/%d.new", SHP_PREFIX, zone);
    if (!(shop_file = fopen(fname, "w"))) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: Cannot open shop file!");
        return;
    } else if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: Cannot write to shop file!");
        fclose(shop_file);
        return;
    }
    /*
     * Search database for shops in this zone.
     */
    for (i = zone * 100; i <= top; i++) {
        if ((rshop = real_shop(i)) != -1) {
            fprintf(shop_file, "#%d~\n", i);
            shop = shop_index + rshop;

            /*
             * Save the products.
             */
            for (j = 0; S_PRODUCT(shop, j) != -1; j++)
                fprintf(shop_file, "%d %d\n", obj_index[S_PRODUCT(shop, j)].vnum, S_AMOUNT(shop, j));

            /*
             * Save the rates.
             */
            fprintf(shop_file, "-1\n%1.2f\n%1.2f\n", S_BUYPROFIT(shop), S_SELLPROFIT(shop));

            /*. Save buy types and namelists . */
            j = -1;
            do {
                j++;
                fprintf(shop_file, "%d%s\n", S_BUYTYPE(shop, j), S_BUYWORD(shop, j) ? S_BUYWORD(shop, j) : "");
            } while (S_BUYTYPE(shop, j) != -1);

            /*
             * Save messages'n'stuff.
             */
            fprintf(shop_file,
                    "%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n"
                    "%d\n%d\n%d\n%d\n",
                    S_NOITEM1(shop) ? S_NOITEM1(shop) : "%s Ke?!", S_NOITEM2(shop) ? S_NOITEM2(shop) : "%s Ke?!",
                    S_NOBUY(shop) ? S_NOBUY(shop) : "%s Ke?!", S_NOCASH1(shop) ? S_NOCASH1(shop) : "%s Ke?!",
                    S_NOCASH2(shop) ? S_NOCASH2(shop) : "%s Ke?!", S_BUY(shop) ? S_BUY(shop) : "%s Ke?! %d?",
                    S_SELL(shop) ? S_SELL(shop) : "%s Ke?! %d?", S_BROKE_TEMPER(shop), S_BITVECTOR(shop),
                    mob_index[S_KEEPER(shop)].vnum, S_NOTRADE(shop));

            /*
             * Save the rooms.
             */
            j = -1;
            do {
                j++;
                fprintf(shop_file, "%d\n", S_ROOM(shop, j));
            } while (S_ROOM(shop, j) != -1);

            /*
             * Save open/closing times
             */
            fprintf(shop_file, "%d\n%d\n%d\n%d\n", S_OPEN1(shop), S_CLOSE1(shop), S_OPEN2(shop), S_CLOSE2(shop));
        }
    }
    fprintf(shop_file, "$~\n");
    fclose(shop_file);
    sprintf(buf2, "%s/%d.shp", SHP_PREFIX, zone);
    /*
     * We're fubar'd if we crash between the two lines below.
     */
    remove(buf2);
    rename(fname, buf2);

    olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_SHOP);
}

/**************************************************************************
 *			    Menu functions                                *
 **************************************************************************/

void sedit_products_menu(DescriptorData *d) {
    ShopData *shop;
    int i;

    shop = OLC_SHOP(d);
    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    char_printf(d->character, ".[H.[J");
#endif
    char_printf(d->character, "##     VNUM     Product                           Unique\n");
    for (i = 0; S_PRODUCT(shop, i) != -1; i++) {
        sprintf(buf, "%2d - [%s%5d%s] - %s%-30s: %s%s\n", i, cyn, obj_index[S_PRODUCT(shop, i)].vnum, nrm, yel,
                obj_proto[S_PRODUCT(shop, i)].short_description, (S_AMOUNT(shop, i) ? "YES" : "NO"), nrm);
        char_printf(d->character, buf);
    }
    sprintf(buf,
            "\n"
            "%sA%s) Add a new product.\n"
            "%sD%s) Delete a product.\n"
            "%sU%s) Switch product unique setting.\n"
            "%sQ%s) Quit\n"
            "Enter choice:\n",
            grn, nrm, grn, nrm, grn, nrm, grn, nrm);
    char_printf(d->character, buf);

    OLC_MODE(d) = SEDIT_PRODUCTS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_compact_rooms_menu(DescriptorData *d) {
    ShopData *shop;
    int i, count = 0;

    shop = OLC_SHOP(d);
    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    for (i = 0; S_ROOM(shop, i) != -1; i++) {
        char_printf(d->character, "{:2d} - [{}{:5d}{}]  | {}", i, cyn, S_ROOM(shop, i), nrm,
                    !(++count % 5) ? "\n" : "");
    }
    char_printf(d->character,
                "\n"
                "{}A{}) Add a new room.\n"
                "{}D{}) Delete a room.\n"
                "{}L{}) Long display.\n"
                "{}Q{}) Quit\n"
                "Enter choice:\n",
                grn, nrm, grn, nrm, grn, nrm, grn, nrm);

    OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_rooms_menu(DescriptorData *d) {
    ShopData *shop;
    int i;

    shop = OLC_SHOP(d);
    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    char_printf(d->character, ".[H.[J");
#endif
    char_printf(d->character, "##     VNUM     Room\n\n");
    for (i = 0; S_ROOM(shop, i) != -1; i++) {
        char_printf(d->character, "{:2d} - [{}{:5d}{}] - {}{}{}\n", i, cyn, S_ROOM(shop, i), nrm, yel,
                    world[real_room(S_ROOM(shop, i))].name, nrm);
    }
    char_printf(d->character,
                "\n"
                "{}A{}) Add a new room.\n"
                "{}D{}) Delete a room.\n"
                "{}C{}) Compact Display.\n"
                "{}Q{}) Quit\n"
                "Enter choice:\n",
                grn, nrm, grn, nrm, grn, nrm, grn, nrm);

    OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_namelist_menu(DescriptorData *d) {
    ShopData *shop;
    int i;

    shop = OLC_SHOP(d);
    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    char_printf(d->character, ".[H.[J");
#endif
    char_printf(d->character, "##              Type   Namelist\n\n");
    for (i = 0; S_BUYTYPE(shop, i) != -1; i++) {
        char_printf(d->character, "{:2d} - {}{:<15s}{} - {}{}{}\n", i, cyn, item_types[S_BUYTYPE(shop, i)].name, nrm,
                    yel, S_BUYWORD(shop, i) ? S_BUYWORD(shop, i) : "<None>", nrm);
    }
    char_printf(d->character,
                "\n"
                "{}A{}) Add a new entry.\n"
                "{}D{}) Delete an entry.\n"
                "{}Q{}) Quit\n"
                "Enter choice:\n",
                grn, nrm, grn, nrm, grn, nrm);
    OLC_MODE(d) = SEDIT_NAMELIST_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_shop_flags_menu(DescriptorData *d) {
    int i, count = 0;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    for (i = 0; i < NUM_SHOP_FLAGS; i++) {
        char_printf(d->character, "{}{:2d}{}) {:<20s}   {}", grn, i + 1, nrm, shop_bits[i], !(++count % 2) ? "\n" : "");
    }
    sprintbit(S_BITVECTOR(OLC_SHOP(d)), shop_bits, buf1);
    char_printf(d->character, "\nCurrent Shop Flags : {}{}{}\nEnter choice:\n", cyn, buf1, nrm);
    OLC_MODE(d) = SEDIT_SHOP_FLAGS;
}

/*-------------------------------------------------------------------*/

void sedit_no_trade_menu(DescriptorData *d) {
    int i, count = 0;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    for (i = 0; i < NUM_TRADERS; i++) {
        char_printf(d->character, "{}{:2d}{}) {:<20s}   {}", grn, i + 1, nrm, trade_letters[i],
                    !(++count % 2) ? "\n" : "");
    }
    sprintbit(S_NOTRADE(OLC_SHOP(d)), trade_letters, buf1);
    char_printf(d->character,
                "\nCurrently won't trade with: {}{}{}\n"
                "Enter choice:\n",
                cyn, buf1, nrm);
    OLC_MODE(d) = SEDIT_NOTRADE;
}

/*-------------------------------------------------------------------*/

void sedit_types_menu(DescriptorData *d) {
    int i, count = 0;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    for (i = 0; i < NUM_ITEM_TYPES; i++) {
        char_printf(d->character, "{}{:2d}{}) {}{:<20s}{}  {}", grn, i, nrm, cyn, item_types[i].name, nrm,
                    !(++count % 3) ? "\n" : "");
    }
    char_printf(d->character, "{}Enter choice:\n", nrm);
    OLC_MODE(d) = SEDIT_TYPE_MENU;
}

/*-------------------------------------------------------------------*/
/*. Display main menu .*/

void sedit_disp_menu(DescriptorData *d) {
    ShopData *shop;

    shop = OLC_SHOP(d);
    get_char_cols(d->character);

    sprintbit(S_NOTRADE(shop), trade_letters, buf1);
    sprintbit(S_BITVECTOR(shop), shop_bits, buf2);
    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "-- Shop Number : [%s%d%s]\n"
            "%s0%s) Keeper      : [%s%d%s] %s%s\n"
            "%s1%s) Open 1      : %s%4d%s          %s2%s) Close 1     : %s%4d\n"
            "%s3%s) Open 2      : %s%4d%s          %s4%s) Close 2     : %s%4d\n"
            "%s5%s) Sell rate   : %s%1.2f%s          %s6%s) Buy rate    : %s%1.2f\n"
            "%s7%s) Keeper no item : %s%s\n"
            "%s8%s) Player no item : %s%s\n"
            "%s9%s) Keeper no cash : %s%s\n"
            "%sA%s) Player no cash : %s%s\n",
            cyn, OLC_NUM(d), nrm, grn, nrm, cyn, S_KEEPER(shop) == -1 ? -1 : mob_index[S_KEEPER(shop)].vnum, nrm, yel,
            S_KEEPER(shop) == -1 ? "None" : mob_proto[S_KEEPER(shop)].player.short_descr, grn, nrm, cyn, S_OPEN1(shop),
            nrm, grn, nrm, cyn, S_CLOSE1(shop), grn, nrm, cyn, S_OPEN2(shop), nrm, grn, nrm, cyn, S_CLOSE2(shop), grn,
            nrm, cyn, S_BUYPROFIT(shop), nrm, grn, nrm, cyn, S_SELLPROFIT(shop), grn, nrm, yel, S_NOITEM1(shop), grn,
            nrm, yel, S_NOITEM2(shop), grn, nrm, yel, S_NOCASH1(shop), grn, nrm, yel, S_NOCASH2(shop));
    char_printf(d->character, buf);

    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "%sB%s) Keeper no buy  : %s%s\n"
            "%sC%s) Buy sucess     : %s%s\n"
            "%sD%s) Sell sucess    : %s%s\n"
            "%sE%s) No Trade With  : %s%s\n"
            "%sF%s) Shop flags     : %s%s\n"
            "%sR%s) Rooms Menu\n"
            "%sP%s) Products Menu\n"
            "%sT%s) Accept Types Menu\n"
            "%sQ%s) Quit\n"
            "Enter Choice:\n",
            grn, nrm, yel, S_NOBUY(shop), grn, nrm, yel, S_BUY(shop), grn, nrm, yel, S_SELL(shop), grn, nrm, cyn, buf1,
            grn, nrm, cyn, buf2, grn, nrm, grn, nrm, grn, nrm, grn, nrm);

    char_printf(d->character, buf);

    OLC_MODE(d) = SEDIT_MAIN_MENU;
}

/**************************************************************************
 *                   The GARGANTUAN event handler                         *
 **************************************************************************/

void sedit_parse(DescriptorData *d, std::string_view arg) {
    int i, k;

    if (OLC_MODE(d) > SEDIT_NUMERICAL_RESPONSE) {
        if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
            char_printf(d->character, "Field must be numerical, try again:\n");
            return;
        }
    }
    switch (OLC_MODE(d)) {

    case SEDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
        case 'y':
        case 'Y':
            char_printf(d->character, "Saving shop to memory.\n");
            sedit_save_internally(d);
            log(LogSeverity::Debug, std::max(LVL_GOD, GET_INVIS_LEV(d->character)), "OLC: {} edits shop {:d}",
                GET_NAME(d->character), OLC_NUM(d));
            cleanup_olc(d, CLEANUP_STRUCTS);
            return;
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            char_printf(d->character, "Invalid choice!\nDo you wish to save the shop?\n");
            return;
        }
        break;

    case SEDIT_MAIN_MENU:
        i = 0;
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) { /* Anything been changed? */
                char_printf(d->character, "Do you wish to save the changes to the shop? (y/n)\n");
                OLC_MODE(d) = SEDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '0':
            OLC_MODE(d) = SEDIT_KEEPER;
            char_printf(d->character, "Enter vnum of shop keeper:\n");
            return;
        case '1':
            OLC_MODE(d) = SEDIT_OPEN1;
            i++;
            break;
        case '2':
            OLC_MODE(d) = SEDIT_CLOSE1;
            i++;
            break;
        case '3':
            OLC_MODE(d) = SEDIT_OPEN2;
            i++;
            break;
        case '4':
            OLC_MODE(d) = SEDIT_CLOSE2;
            i++;
            break;
        case '5':
            OLC_MODE(d) = SEDIT_BUY_PROFIT;
            i++;
            break;
        case '6':
            OLC_MODE(d) = SEDIT_SELL_PROFIT;
            i++;
            break;
        case '7':
            OLC_MODE(d) = SEDIT_NOITEM1;
            i--;
            break;
        case '8':
            OLC_MODE(d) = SEDIT_NOITEM2;
            i--;
            break;
        case '9':
            OLC_MODE(d) = SEDIT_NOCASH1;
            i--;
            break;
        case 'a':
        case 'A':
            OLC_MODE(d) = SEDIT_NOCASH2;
            i--;
            break;
        case 'b':
        case 'B':
            OLC_MODE(d) = SEDIT_NOBUY;
            i--;
            break;
        case 'c':
        case 'C':
            OLC_MODE(d) = SEDIT_BUY;
            i--;
            break;
        case 'd':
        case 'D':
            OLC_MODE(d) = SEDIT_SELL;
            i--;
            break;
        case 'e':
        case 'E':
            sedit_no_trade_menu(d);
            return;
        case 'f':
        case 'F':
            sedit_shop_flags_menu(d);
            return;
        case 'r':
        case 'R':
            sedit_rooms_menu(d);
            return;
        case 'p':
        case 'P':
            sedit_products_menu(d);
            return;
        case 't':
        case 'T':
            sedit_namelist_menu(d);
            return;
        default:
            sedit_disp_menu(d);
            return;
        }

        if (i != 0) {
            char_printf(d->character,
                        i == 1 ? "\nEnter new value : " : (i == -1 ? "\nEnter new text :\n] " : "Oops...\n"));
            return;
        }
        break;

    case SEDIT_NAMELIST_MENU:
        switch (*arg) {
        case 'a':
        case 'A':
            sedit_types_menu(d);
            return;
        case 'd':
        case 'D':
            char_printf(d->character, "\nDelete which entry?\n");
            OLC_MODE(d) = SEDIT_DELETE_TYPE;
            return;
        case 'q':
        case 'Q':
            break;
        }
        break;
        /*-------------------------------------------------------------------*/
    case SEDIT_PRODUCTS_MENU:
        switch (*arg) {
        case 'a':
        case 'A':
            char_printf(d->character, "\nEnter new product vnum:\n");
            OLC_MODE(d) = SEDIT_NEW_PRODUCT;
            return;
        case 'd':
        case 'D':
            char_printf(d->character, "\nDelete which product?\n");
            OLC_MODE(d) = SEDIT_DELETE_PRODUCT;
            return;
        case 'u':
        case 'U':
            char_printf(d->character, "\nChange which entry?\n");
            OLC_MODE(d) = SEDIT_UNIQUE;
            return;
        case 'q':
        case 'Q':
            break;
        }
        break;
        /*-------------------------------------------------------------------*/
    case SEDIT_ROOMS_MENU:
        switch (*arg) {
        case 'a':
        case 'A':
            char_printf(d->character, "\nEnter new room vnum:\n");
            OLC_MODE(d) = SEDIT_NEW_ROOM;
            return;
        case 'c':
        case 'C':
            sedit_compact_rooms_menu(d);
            return;
        case 'l':
        case 'L':
            sedit_rooms_menu(d);
            return;
        case 'd':
        case 'D':
            char_printf(d->character, "\nDelete which room?\n");
            OLC_MODE(d) = SEDIT_DELETE_ROOM;
            return;
        case 'q':
        case 'Q':
            break;
        }
        break;
        /*-------------------------------------------------------------------*/
        /*. String edits . */
    case SEDIT_NOITEM1:
        sedit_modify_string(&S_NOITEM1(OLC_SHOP(d)), arg);
        break;
    case SEDIT_NOITEM2:
        sedit_modify_string(&S_NOITEM2(OLC_SHOP(d)), arg);
        break;
    case SEDIT_NOCASH1:
        sedit_modify_string(&S_NOCASH1(OLC_SHOP(d)), arg);
        break;
    case SEDIT_NOCASH2:
        sedit_modify_string(&S_NOCASH2(OLC_SHOP(d)), arg);
        break;
    case SEDIT_NOBUY:
        sedit_modify_string(&S_NOBUY(OLC_SHOP(d)), arg);
        break;
    case SEDIT_BUY:
        sedit_modify_string(&S_BUY(OLC_SHOP(d)), arg);
        break;
    case SEDIT_SELL:
        sedit_modify_string(&S_SELL(OLC_SHOP(d)), arg);
        break;
    case SEDIT_NAMELIST: {
        ShopBuyData new_entry;

        BUY_TYPE(new_entry) = OLC_VAL(d);
        BUY_WORD(new_entry) = (arg && *arg) ? strdup(arg) : nullptr;
        sedit_add_to_type_list(&(S_NAMELISTS(OLC_SHOP(d))), &new_entry);
    }
        sedit_namelist_menu(d);
        return;

        /*-------------------------------------------------------------------*/
        /*. Numerical responses . */

    case SEDIT_KEEPER:
        i = atoi(arg);
        if ((i = atoi(arg)) != -1)
            if ((i = real_mobile(i)) < 0) {
                char_printf(d->character, "That mobile does not exist, try again:\n");
                return;
            }

        S_KEEPER(OLC_SHOP(d)) = i;
        if (i == -1)
            break;
        /*. Fiddle with special procs . */
        S_FUNC(OLC_SHOP(d)) = mob_index[i].func;
        mob_index[i].func = shop_keeper;
        break;
    case SEDIT_OPEN1:
        S_OPEN1(OLC_SHOP(d)) = std::clamp(atoi(arg), 0, 28);
        break;
    case SEDIT_OPEN2:
        S_OPEN2(OLC_SHOP(d)) = std::clamp(atoi(arg), 0, 28);
        break;
    case SEDIT_CLOSE1:
        S_CLOSE1(OLC_SHOP(d)) = std::clamp(atoi(arg), 0, 28);
        break;
    case SEDIT_CLOSE2:
        S_CLOSE2(OLC_SHOP(d)) = std::clamp(atoi(arg), 0, 28);
        break;
    case SEDIT_BUY_PROFIT:
        sscanf(arg, "%f", &S_BUYPROFIT(OLC_SHOP(d)));
        break;
    case SEDIT_SELL_PROFIT:
        sscanf(arg, "%f", &S_SELLPROFIT(OLC_SHOP(d)));
        break;
    case SEDIT_TYPE_MENU:
        OLC_VAL(d) = std::clamp(atoi(arg), 0, NUM_ITEM_TYPES - 1);
        char_printf(d->character, "Enter namelist (return for none) :]\n");
        OLC_MODE(d) = SEDIT_NAMELIST;
        return;
    case SEDIT_DELETE_TYPE:
        sedit_remove_from_type_list(&(S_NAMELISTS(OLC_SHOP(d))), atoi(arg));
        sedit_namelist_menu(d);
        return;
    case SEDIT_NEW_PRODUCT:
        if ((i = atoi(arg)) != -1)
            if ((i = real_object(i)) == -1) {
                char_printf(d->character, "That object does not exist, try again:\n");
                return;
            }
        if (i > 0) {
            add_to_int_list(&(S_PRODUCTS(OLC_SHOP(d))), i);
            add_to_int_list(&(S_AMOUNTS(OLC_SHOP(d))), 0);
        }
        sedit_products_menu(d);
        return;
    case SEDIT_UNIQUE:
        for (k = 0; S_PRODUCTS(OLC_SHOP(d))[k] != -1; k++)
            ;
        if ((i = atoi(arg)) != -1) {
            if (!(i < k))
                char_printf(d->character, "&0&1&bThat product entry does not exist, try again : &0\n");
            else
                sedit_unique_change(OLC_SHOP(d), i);
        }
        sedit_products_menu(d);
        return;
    case SEDIT_DELETE_PRODUCT:
        remove_from_int_list(&(S_PRODUCTS(OLC_SHOP(d))), atoi(arg));
        remove_from_int_list(&(S_AMOUNTS(OLC_SHOP(d))), atoi(arg));
        sedit_products_menu(d);
        return;
    case SEDIT_NEW_ROOM:
        if ((i = atoi(arg)) != -1)
            if ((i = real_room(i)) < 0) {
                char_printf(d->character, "That room does not exist, try again:\n");
                return;
            }
        if (i >= 0)
            add_to_int_list(&(S_ROOMS(OLC_SHOP(d))), atoi(arg));
        sedit_rooms_menu(d);
        return;
    case SEDIT_DELETE_ROOM:
        remove_from_int_list(&(S_ROOMS(OLC_SHOP(d))), atoi(arg));
        sedit_rooms_menu(d);
        return;
    case SEDIT_SHOP_FLAGS:
        if ((i = std::clamp(atoi(arg), 0, NUM_SHOP_FLAGS)) > 0) {
            TOGGLE_BIT(S_BITVECTOR(OLC_SHOP(d)), 1 << (i - 1));
            sedit_shop_flags_menu(d);
            return;
        }
        break;
    case SEDIT_NOTRADE:
        if ((i = std::clamp(atoi(arg), 0, NUM_TRADERS)) > 0) {
            TOGGLE_BIT(S_NOTRADE(OLC_SHOP(d)), 1 << (i - 1));
            sedit_no_trade_menu(d);
            return;
        }
        break;

        /*-------------------------------------------------------------------*/
    default:
        /*. We should never get here . */
        cleanup_olc(d, CLEANUP_ALL);
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: sedit_parse(): Reached default case!");
        char_printf(d->character, "Oops...\n");
        break;
    }

    /*-------------------------------------------------------------------*/
    /*. END OF CASE
        If we get here, we have probably changed something, and now want to
        return to main menu.  Use OLC_VAL as a 'has changed' flag .*/

    OLC_VAL(d) = 1;
    sedit_disp_menu(d);
}

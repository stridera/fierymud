/***************************************************************************
 *   File: medit.c                                        Part of FieryMUD *
 *  Usage: OASIS OLC - ?                                                   *
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

#include "casting.hpp"
#include "chars.hpp"
#include "charsize.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_olc.hpp"
#include "fight.hpp"
#include "genzon.hpp"
#include "handler.hpp"
#include "lifeforce.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "races.hpp"
#include "shop.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <fmt/format.h>

/*
 * Set this to 1 for debugging logs in medit_save_internally.
 */
#if 0
#define DEBUG
#endif

/*
 * Set this to 1 as a last resort to save mobiles.
 */
#if 0
#define I_CRASH
#endif

/*-------------------------------------------------------------------*/
/* external variables */

extern PlayerSpecialData dummy_mob;

/*-------------------------------------------------------------------*/
/*. Handy  macros .*/

#define GET_ZONE(ch) ((ch)->mob_specials.zone)
#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)
#define GET_SDESC(mob) ((mob)->player.short_descr)
#define GET_MOBLDESC(mob) ((mob)->player.long_descr)
#define GET_DDESC(mob) ((mob)->player.description)
#define GET_ATTACK(mob) ((mob)->mob_specials.attack_type)
#define S_KEEPER(shop) ((shop)->keeper)
/*-------------------------------------------------------------------*/
/*. Function prototypes .*/
int get_ac(int level, int race, int class_num);
void medit_parse(DescriptorData *d, char *arg);
void medit_disp_menu(DescriptorData *d);
void medit_setup_new(DescriptorData *d);
void medit_setup_existing(DescriptorData *d, int rmob_num);
void medit_save_internally(DescriptorData *d);
void medit_save_to_disk(int zone_num);
void init_mobile(CharData *mob);
void copy_mobile(CharData *tmob, CharData *fmob);
void medit_disp_stances(DescriptorData *d);
void medit_disp_positions(DescriptorData *d);
void medit_disp_mob_flags(DescriptorData *d);
void medit_disp_aff_flags(DescriptorData *d);
void medit_disp_attack_types(DescriptorData *d);
void medit_class_types(DescriptorData *d);
void medit_size(DescriptorData *d);
void medit_race_types(DescriptorData *d);
long get_set_exp(int, int, int, int);
sh_int get_set_hit(int, int, int, int);
sbyte get_set_hd(int, int, int, int);
int get_set_dice(int, int, int, int);

/*-------------------------------------------------------------------*\
  utility functions
\*-------------------------------------------------------------------*/

/* * * * *
 * Free a mobile structure that has been edited.
 * Take care of existing mobiles and their mob_proto!
 * * * * */

void medit_free_mobile(CharData *mob) {
    int i;
    /*
     * Non-prototyped mobile.  Also known as new mobiles.
     */
    if (!mob)
        return;
    else if (GET_MOB_RNUM(mob) == -1) {
        if (mob->player.namelist)
            free(mob->player.namelist);
        if (mob->player.title)
            free(mob->player.title);
        if (mob->player.short_descr)
            free(mob->player.short_descr);
        if (GET_MOBLDESC(mob))
            free(GET_MOBLDESC(mob));
        if (mob->player.description)
            free(mob->player.description);
    } else if ((i = GET_MOB_RNUM(mob)) > -1) { /* Prototyped mobile. */
        if (mob->player.namelist && mob->player.namelist != mob_proto[i].player.namelist)
            free(mob->player.namelist);
        if (mob->player.title && mob->player.title != mob_proto[i].player.title)
            free(mob->player.title);
        if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr)
            free(mob->player.short_descr);
        if (GET_MOBLDESC(mob) && GET_MOBLDESC(mob) != mob_proto[i].player.long_descr)
            free(GET_MOBLDESC(mob));
        if (mob->player.description && mob->player.description != mob_proto[i].player.description)
            free(mob->player.description);
    }
    while (mob->effects)
        effect_remove(mob, mob->effects);

    free(mob);
}

void medit_setup_new(DescriptorData *d) {
    CharData *mob;

    /*. Alloc some mob shaped space . */
    CREATE(mob, CharData, 1);

    init_mobile(mob);

    GET_MOB_RNUM(mob) = -1;
    /*. default strings . */
    GET_NAMELIST(mob) = strdup("mob unfinished");
    GET_SDESC(mob) = strdup("the unfinished mob");
    GET_MOBLDESC(mob) = strdup("An unfinished mob stands here.\n");
    GET_DDESC(mob) = strdup("It looks unfinished.\n");
    GET_CLASS(mob) = CLASS_DEFAULT;

    OLC_MOB(d) = mob;

    /*
     ** to be honest I am not sure if these lines need to be here
     ** but since they didn't seem to be doing any harm I left them
     */
    OLC_ITEM_TYPE(d) = MOB_TRIGGER;
    OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */
    dg_olc_script_copy(d);
    medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(DescriptorData *d, int rmob_num) {
    CharData *mob;

    /*
     * Allocate a scratch mobile structure.
     */
    CREATE(mob, CharData, 1);

    copy_mobile(mob, mob_proto + rmob_num);

    OLC_MOB(d) = mob;
    OLC_ITEM_TYPE(d) = MOB_TRIGGER;
    OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */
    dg_olc_script_copy(d);
    medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/
/*. Copy one mob struct to another .*/

void copy_mobile(CharData *tmob, CharData *fmob) {
    /*. Free up any used strings . */
    if (GET_NAMELIST(tmob))
        free(GET_NAMELIST(tmob));
    if (GET_SDESC(tmob))
        free(GET_SDESC(tmob));
    if (GET_MOBLDESC(tmob))
        free(GET_MOBLDESC(tmob));
    if (GET_DDESC(tmob))
        free(GET_DDESC(tmob));

    /* delete the old script list */
    if (tmob->proto_script && tmob->proto_script != fmob->proto_script)
        free_proto_script(&tmob->proto_script);

    /*
     * Copy mob over.
     */
    *tmob = *fmob;

    /*
     * Reallocate strings.
     */
    GET_NAMELIST(tmob) = strdup((GET_NAMELIST(fmob) && *GET_NAMELIST(fmob)) ? GET_NAMELIST(fmob) : "undefined");
    GET_SDESC(tmob) = strdup((GET_SDESC(fmob) && *GET_SDESC(fmob)) ? GET_SDESC(fmob) : "undefined");
    GET_MOBLDESC(tmob) = strdup((GET_MOBLDESC(fmob) && *GET_MOBLDESC(fmob)) ? GET_MOBLDESC(fmob) : "undefined");
    GET_DDESC(tmob) = strdup((GET_DDESC(fmob) && *GET_DDESC(fmob)) ? GET_DDESC(fmob) : "undefined");
}

/*-------------------------------------------------------------------*/
/*. Ideally, this function should be in db.c, but I'll put it here for
  portability.*/

void init_mobile(CharData *mob) {
    clear_char(mob);

    /*GET_HIT(mob) = 5;
       GET_MANA(mob) = 10;
     */
    (mob)->mob_specials.ex_hpnumdice = 0;
    (mob)->mob_specials.ex_hpsizedice = 0;
    GET_RACE(mob) = DEFAULT_RACE;
    GET_CLASS(mob) = CLASS_UNDEFINED;
    GET_MAX_MANA(mob) = 100;
    GET_MAX_MOVE(mob) = 100;
    GET_NDD(mob) = 0;
    GET_SDD(mob) = 0;
    GET_WEIGHT(mob) = 200;
    GET_HEIGHT(mob) = 198;
    set_base_size(mob, SIZE_MEDIUM);

    mob->natural_abils.str = 11;
    mob->natural_abils.intel = 11;
    mob->natural_abils.wis = 11;
    mob->natural_abils.dex = 11;
    mob->natural_abils.con = 11;
    mob->natural_abils.cha = 11;
    mob->affected_abils = mob->natural_abils;

    SET_FLAG(MOB_FLAGS(mob), MOB_ISNPC);
    mob->player_specials = &dummy_mob;
}

/*-------------------------------------------------------------------*/
/*. Save new/edited mob to memory .*/

#define ZCMD (zone_table[zone].cmd[cmd_no])

void medit_save_internally(DescriptorData *d) {
    int rmob_num, found = 0, new_mob_num = 0, zone, cmd_no, shop;
    CharData *new_proto;
    IndexData *new_index;
    CharData *live_mob;
    DescriptorData *dsc;

    int i;
    /* put the script into proper position */
    OLC_MOB(d)->proto_script = OLC_SCRIPT(d);

    /*
     * Mob exists? Just update it.
     */
    if ((rmob_num = real_mobile(OLC_NUM(d))) != -1) {

        /*
         * Are we purging?
         */
        if (OLC_MODE(d) == MEDIT_PURGE_MOBILE) {
            CharData *placeholder;
            void extract_char(CharData * ch); /* leaves eq behind.. */

            for (live_mob = character_list; live_mob; live_mob = live_mob->next) {
                if (IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num) {
                    placeholder = live_mob;
#if defined(DEBUG)
                    fprintf(stderr, "remove mob %d ", GET_MOB_VNUM(live_mob));
#endif
                    extract_char(live_mob); /*remove all existing mobs */
                    live_mob = placeholder; /*so we can keep removing.. */
#if defined(DEBUG)
                    fprintf(stderr, "(%d left)\n", mob_index[rmob_num].number);
#endif
                }
            }
            CREATE(new_proto, CharData, top_of_mobt);
            CREATE(new_index, IndexData, top_of_mobt);
#if defined(DEBUG)
            fprintf(stderr, "looking to destroy %d (%d)\n", rmob_num, mob_index[rmob_num].vnum);
#endif
            for (i = 0; i <= top_of_mobt; i++) {
                if (!found) { /* Is this the place? */
                    /*    if ((rmob_num > top_of_mobt) || (mob_index[rmob_num].vnum >
                     * OLC_NUM(d))) */
                    if (i == rmob_num) {
                        found = true;
                        /* don't copy..it will be blatted by the free later */
                    } else { /* Nope, copy over as normal. */
                        new_index[i] = mob_index[i];
                        new_proto[i] = mob_proto[i];
                    }
                } else { /* We've already found it, copy the rest over. */
                    new_index[i - 1] = mob_index[i];
                    new_proto[i - 1] = mob_proto[i];
                }
            }
            top_of_mobt--;
#if !defined(I_CRASH)
            free(mob_index);
            free(mob_proto);
#endif
            mob_index = new_index;
            mob_proto = new_proto;
            /*
             * Update live mobile rnums.
             */
            for (live_mob = character_list; live_mob; live_mob = live_mob->next)
                if (GET_MOB_RNUM(live_mob) > rmob_num)
                    GET_MOB_RNUM(live_mob)--;

            /*
             * Update zone table.
             */
            for (zone = 0; zone <= top_of_zone_table; zone++)
                for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
                    if (ZCMD.command == 'M')
                        if (ZCMD.arg1 >= rmob_num)
                            ZCMD.arg1--;

            /*
             * Update shop keepers.
             */
            if (shop_index)
                for (shop = 0; shop <= top_shop; shop++)
                    if (SHOP_KEEPER(shop) >= rmob_num)
                        SHOP_KEEPER(shop)--;

            /*
             * Update keepers in shops being edited and other mobs being edited.
             */
            for (dsc = descriptor_list; dsc; dsc = dsc->next)
                if (dsc->connected == CON_SEDIT) {
                    if (S_KEEPER(OLC_SHOP(dsc)) >= rmob_num)
                        S_KEEPER(OLC_SHOP(dsc))--;
                } else if (dsc->connected == CON_MEDIT) {
                    if (GET_MOB_RNUM(OLC_MOB(dsc)) >= rmob_num)
                        GET_MOB_RNUM(OLC_MOB(dsc))--;
                }
        } else {

            OLC_MOB(d)->proto_script = OLC_SCRIPT(d);
            copy_mobile((mob_proto + rmob_num), OLC_MOB(d));
            /*
             * Update live mobiles.
             */
            for (live_mob = character_list; live_mob; live_mob = live_mob->next)
                if (IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num) {
                    /*
                     * Only really need to update the strings, since these can
                     * cause protection faults.  The rest can wait till a reset/reboot.
                     */
                    GET_NAMELIST(live_mob) = GET_NAMELIST(mob_proto + rmob_num);
                    GET_SDESC(live_mob) = GET_SDESC(mob_proto + rmob_num);
                    GET_MOBLDESC(live_mob) = (GET_MOBLDESC(mob_proto + rmob_num));
                    GET_DDESC(live_mob) = GET_DDESC(mob_proto + rmob_num);
                }
        }
    }
    /*
     * Mob does not exist, we have to add it.
     */
    else {
#if defined(DEBUG)
        fprintf(stderr, "top_of_mobt: %d, new top_of_mobt: %d\n", top_of_mobt, top_of_mobt + 1);
#endif

        CREATE(new_proto, CharData, top_of_mobt + 2);
        CREATE(new_index, IndexData, top_of_mobt + 2);

        for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++) {
            if (!found) { /* Is this the place? */
                /*    if ((rmob_num > top_of_mobt) || (mob_index[rmob_num].vnum >
                 * OLC_NUM(d))) { */
                if (mob_index[rmob_num].vnum > OLC_NUM(d)) {
                    /*
                     * Yep, stick it here.
                     */
                    found = true;
#if defined(DEBUG)
                    fprintf(stderr, "Inserted: rmob_num: %d\n", rmob_num);
#endif
                    new_index[rmob_num].vnum = OLC_NUM(d);
                    new_index[rmob_num].number = 0;
                    new_index[rmob_num].func = nullptr;
                    new_mob_num = rmob_num;
                    GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
                    copy_mobile((new_proto + rmob_num), OLC_MOB(d));
                    /*
                     * Copy the mob that should be here on top.
                     */
                    new_index[rmob_num + 1] = mob_index[rmob_num];
                    new_proto[rmob_num + 1] = mob_proto[rmob_num];
                    GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
                } else { /* Nope, copy over as normal. */
                    new_index[rmob_num] = mob_index[rmob_num];
                    new_proto[rmob_num] = mob_proto[rmob_num];
                }
            } else { /* We've already found it, copy the rest over. */
                new_index[rmob_num + 1] = mob_index[rmob_num];
                new_proto[rmob_num + 1] = mob_proto[rmob_num];
                GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
            }
        }
#if defined(DEBUG)
        fprintf(stderr, "rmob_num: %d, top_of_mobt: %d, array size: 0-%d (%d)\n", rmob_num, top_of_mobt,
                top_of_mobt + 1, top_of_mobt + 2);
#endif
        if (!found) { /* Still not found, must add it to the top of the table. */
#if defined(DEBUG)
            fprintf(stderr, "Append.\n");
#endif
            new_index[rmob_num].vnum = OLC_NUM(d);
            new_index[rmob_num].number = 0;
            new_index[rmob_num].func = nullptr;
            new_mob_num = rmob_num;
            GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
            copy_mobile((new_proto + rmob_num), OLC_MOB(d));
        }

        /*. Replace tables . */
#if defined(DEBUG)
        fprintf(stderr, "Attempted free.\n");
#endif
#if !defined(I_CRASH)
        free(mob_index);
        free(mob_proto);
#endif
        mob_index = new_index;
        mob_proto = new_proto;
        top_of_mobt++;
#if defined(DEBUG)
        fprintf(stderr, "Free ok.\n");
#endif

        /*
         * Update live mobile rnums.
         */
        for (live_mob = character_list; live_mob; live_mob = live_mob->next)
            if (GET_MOB_RNUM(live_mob) > new_mob_num)
                GET_MOB_RNUM(live_mob)++;

        /*
         * Update zone table.
         */
        for (zone = 0; zone <= top_of_zone_table; zone++)
            for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
                if (ZCMD.command == 'M')
                    if (ZCMD.arg1 >= new_mob_num)
                        ZCMD.arg1++;

        /*
         * Update shop keepers.
         */
        if (shop_index)
            for (shop = 0; shop <= top_shop; shop++)
                if (SHOP_KEEPER(shop) >= new_mob_num)
                    SHOP_KEEPER(shop)++;

        /*
         * Update keepers in shops being edited and other mobs being edited.
         */
        for (dsc = descriptor_list; dsc; dsc = dsc->next)
            if (dsc->connected == CON_SEDIT) {
                if (S_KEEPER(OLC_SHOP(dsc)) >= new_mob_num)
                    S_KEEPER(OLC_SHOP(dsc))++;
            } else if (dsc->connected == CON_MEDIT) {
                if (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_mob_num)
                    GET_MOB_RNUM(OLC_MOB(dsc))++;
            }
    }

    /*debug: dump the list */
#if defined(DEBUG)
    fprintf(stderr, "there are now %d mobs in the list:\n", top_of_mobt);
#endif
    /*for (i=0;i<top_of_mobt;i++) */
    /*
       for (live_mob = character_list,i=0; live_mob; live_mob =
       live_mob->next,i++) fprintf(stderr,"(%d -
       %d:%s)",i,GET_MOB_VNUM(live_mob),GET_NAME(live_mob));
     */
    olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_MOB);
}

/*
 * Save ALL mobiles for a zone to their .mob file, mobs are all
 * saved in Extended format, regardless of whether they have any
 * extended fields.  Thanks to Sammy for ideas on this bit of code.
 */
void medit_save_to_disk(int zone_num) {
    int i, rmob_num, zone, top;
    FILE *mob_file;
    char fname[64];
    CharData *mob;

    zone = zone_table[zone_num].number;
    top = zone_table[zone_num].top;

    sprintf(fname, "%s/%d.new", MOB_PREFIX, zone);
    if (!(mob_file = fopen(fname, "w"))) {
        mudlog("SYSERR: OLC: Cannot open mob file!", BRF, LVL_GOD, true);
        return;
    }

    /*
     * Seach the database for mobs in this zone and save them.
     */
    for (i = zone * 100; i <= top; i++) {
        if ((rmob_num = real_mobile(i)) != -1) {
            if (fprintf(mob_file, "#%d\n", i) < 0) {
                mudlog("SYSERR: OLC: Cannot write mob file!\n", BRF, LVL_GOD, true);
                fclose(mob_file);
                return;
            }
            mob = (mob_proto + rmob_num);

            /*
             * Clean up strings.
             */
            strcpy(buf1, (GET_MOBLDESC(mob) && *GET_MOBLDESC(mob)) ? GET_MOBLDESC(mob) : "undefined");
            strip_string(buf1);
            strcpy(buf2, (GET_DDESC(mob) && *GET_DDESC(mob)) ? GET_DDESC(mob) : "undefined");
            strip_string(buf2);

            fprintf(mob_file,
                    "%s~\n"
                    "%s~\n"
                    "%s~\n"
                    "%s~\n"
                    "%ld %ld %d E\n"
                    "%d %d %d %dd%d+%d %dd%d+%d\n"
                    "%d %d %ld %d\n"
                    "%d %d %d %d %d %d %d\n",
                    (GET_NAMELIST(mob) && *GET_NAMELIST(mob)) ? GET_NAMELIST(mob) : "undefined",
                    (GET_SDESC(mob) && *GET_SDESC(mob)) ? GET_SDESC(mob) : "undefined", buf1, buf2, MOB_FLAGS(mob)[0],
                    EFF_FLAGS(mob)[0], GET_ALIGNMENT(mob), GET_LEVEL(mob),
                    20 - mob->mob_specials.ex_hitroll, /* Hitroll -> THAC0 */
                    GET_EX_AC(mob) / 10, (mob)->mob_specials.ex_hpnumdice, (mob)->mob_specials.ex_hpsizedice,
                    GET_MOVE(mob), (mob)->mob_specials.ex_damnodice, (mob)->mob_specials.ex_damsizedice,
                    mob->mob_specials.ex_damroll, GET_EX_GOLD(mob), GET_EX_PLATINUM(mob), GET_EX_EXP(mob), zone,
                    GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob), GET_CLASS(mob), GET_RACE(mob),
                    GET_RACE_ALIGN(mob), GET_SIZE(mob));

            /*
             * Deal with Extra stats in case they are there.
             */
            if (GET_ATTACK(mob) != 0)
                fprintf(mob_file, "BareHandAttack: %d\n", GET_ATTACK(mob));
            if (GET_STR(mob) != 11)
                fprintf(mob_file, "Str: %d\n", GET_STR(mob));
            if (GET_DEX(mob) != 11)
                fprintf(mob_file, "Dex: %d\n", GET_DEX(mob));
            if (GET_INT(mob) != 11)
                fprintf(mob_file, "Int: %d\n", GET_INT(mob));
            if (GET_WIS(mob) != 11)
                fprintf(mob_file, "Wis: %d\n", GET_WIS(mob));
            if (GET_CON(mob) != 11)
                fprintf(mob_file, "Con: %d\n", GET_CON(mob));
            if (GET_CHA(mob) != 11)
                fprintf(mob_file, "Cha: %d\n", GET_CHA(mob));

            fprintf(mob_file, "AFF2: %ld\n", EFF_FLAGS(mob)[1]);
            fprintf(mob_file, "AFF3: %ld\n", EFF_FLAGS(mob)[2]);
            fprintf(mob_file, "MOB2: %ld\n", MOB_FLAGS(mob)[1]);
            fprintf(mob_file, "PERC: %ld\n", GET_PERCEPTION(mob));
            fprintf(mob_file, "HIDE: %ld\n", GET_HIDDENNESS(mob));
            fprintf(mob_file, "Lifeforce: %d\n", GET_LIFEFORCE(mob));
            fprintf(mob_file, "Composition: %d\n", BASE_COMPOSITION(mob));
            fprintf(mob_file, "Stance: %d\n", GET_STANCE(mob));

            /*
             * XXX: Add E-mob handlers here.
             */
            fprintf(mob_file, "E\n");

            script_save_to_disk(mob_file, mob, MOB_TRIGGER);
        }
    }
    fprintf(mob_file, "$\n");
    fclose(mob_file);
    sprintf(buf2, "%s/%d.mob", MOB_PREFIX, zone);
    /*
     * We're fubar'd if we crash between the two lines below.
     */
    remove(buf2);
    rename(fname, buf2);

    olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_MOB);
}

/*************************************************************************
 *                Menu functions                               *
 *************************************************************************/

void medit_disp_stances(DescriptorData *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; *stance_types[i] != '\n'; i++) {
        sprintf(buf, "%s%2d%s) %s\n", grn, i, nrm, stance_types[i]);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter stance number:\n", d->character);
}

/*. Display positions (sitting, standing etc) .*/

void medit_disp_positions(DescriptorData *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; *position_types[i] != '\n'; i++) {
        sprintf(buf, "%s%2d%s) %s\n", grn, i, nrm, position_types[i]);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter position number:\n", d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */

void medit_disp_sex(DescriptorData *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_SEXES; i++) {
        sprintf(buf, "%s%2d%s) %s\n", grn, i, nrm, genders[i]);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter gender number:\n", d->character);
}

/*-------------------------------------------------------------------*/
/*
 * Display the mobile's size! -- Cas
 */

void medit_size(DescriptorData *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_SIZES; i++) {
        sprintf(buf, "%s%2d%s) %c%s\n", grn, i, nrm, (sizes[i].name)[0], sizes[i].name + 1);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter size number:\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display attack types menu .*/

void medit_disp_attack_types(DescriptorData *d) {
    int i;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_ATTACK_TYPES; i++) {
        sprintf(buf, "%s%2d%s) %s\n", grn, i, nrm, attack_hit_text[i].singular);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter attack type:\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/

#define FLAG_INDEX ((NUM_MOB_FLAGS / columns + 1) * j + i)
void medit_disp_mob_flags(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif

    /* Outer loop goes through rows, inner loop goes through columns. */
    for (i = 0; i <= NUM_MOB_FLAGS / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j) {
            if (FLAG_INDEX >= NUM_MOB_FLAGS)
                break;
            sprintf(buf, "%s%s%2d%s) %-20.20s", buf, grn, FLAG_INDEX + 1, nrm, action_bits[FLAG_INDEX]);
        }
        send_to_char(strcat(buf, "\n"), d->character);
    }

    sprintflag(buf1, MOB_FLAGS(OLC_MOB(d)), NUM_MOB_FLAGS, action_bits);
    sprintf(buf, "\nCurrent flags : %s%s%s\nEnter mob flags (0 to quit) : ", cyn, buf1, nrm);
    send_to_char(buf, d->character);
}

#undef FLAG_INDEX

void medit_class_types(DescriptorData *d) {
    int i;
    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    send_to_char("\nListings of Classes\n", d->character);
#endif
    for (i = 0; i < NUM_CLASSES; i++) {
        sprintf(buf, "%s%2d%s) %s\n", grn, i, nrm, classes[i].plainname);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter Class:\n", d->character);
}

void medit_race_types(DescriptorData *d) {
    int i;
    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    /*send_to_char(".[H.[J", d->character);    */
    send_to_char("\nListings of Races\n", d->character);
#endif
    for (i = 0; i < NUM_RACES; i++) {
        sprintf(buf, "%s%2d%s) %s\n", grn, i, nrm, races[i].plainname);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter Races:\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

#define FLAG_INDEX ((NUM_EFF_FLAGS / columns + 1) * j + i)
void medit_disp_aff_flags(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i <= NUM_EFF_FLAGS / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j) {
            if (FLAG_INDEX >= NUM_EFF_FLAGS)
                break;
            sprintf(buf, "%s%s%2d%s) %-20.20s", buf, grn, FLAG_INDEX + 1, nrm, effect_flags[FLAG_INDEX]);
        }
        send_to_char(strcat(buf, "\n"), d->character);
    }

    sprintflag(buf1, EFF_FLAGS(OLC_MOB(d)), NUM_EFF_FLAGS, effect_flags);
    sprintf(buf, "\nCurrent flags   : %s%s%s\nEnter aff flags (0 to quit):\n", cyn, buf1, nrm);
    send_to_char(buf, d->character);
}

#undef FLAG_INDEX

/*-------------------------------------------------------------------*/
/*. Display life forces menu .*/

void medit_disp_lifeforces(DescriptorData *d) {
    int i;

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_LIFEFORCES; i++) {
        sprintf(buf, "%s%2d%s) %s%s%s\n", grn, i, nrm, lifeforces[i].color, capitalize(lifeforces[i].name), nrm);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter life force number:\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display compositions menu .*/

void medit_disp_compositions(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    list_olc_compositions(d->character);
    send_to_char("Enter composition number:\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display main menu .*/

/*
 * Display main menu.
 */
void medit_disp_menu(DescriptorData *d) {
    CharData *mob;
    std::string menu = "";

    mob = OLC_MOB(d);
    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    menu += ".[H.[J";
#endif
    menu += fmt::format("-- Mob: '&5{})&0'  vnum: [&2{:5}&0]\n", GET_SDESC(mob), OLC_NUM(d));
    menu += fmt::format("&2&b0&0) Alias: &6{}&0\n", GET_NAMELIST(mob));
    menu += fmt::format("&2&b1&0) Short: &6{}&0\n", GET_SDESC(mob));
    menu += fmt::format("&2&b2&0) Long : &6{}&0\n", GET_MOBLDESC(mob));
    menu += fmt::format("&2&b3&0) Description:\n&6{}&0\n", GET_DDESC(mob));
    menu +=
        fmt::format("&2&b4&0) Race: &6{:<10}&0\t\t&2&b5&0) Size: &6{:<10}&0\t\t", RACE_PLAINNAME(mob), SIZE_DESC(mob));
    menu += fmt::format("&2&b6&0) Gender: &6{}&0\n", genders[(int)GET_SEX(mob)]);
    menu += fmt::format("&2&b7&0) Level: &6{:<3}&0\t\t\t&2&b8&0) Class: &6{:<10}&0\t\t", GET_LEVEL(mob),
                        classes[(int)GET_CLASS(mob)].plainname);
    menu += fmt::format("&2&b9&0) Alignment: &6{}&0\n", GET_ALIGNMENT(mob));
    send_to_char(menu.c_str(), d->character);

    menu = "";
    menu += fmt::format("&2&bA&0) Hitroll    : (&6{:3}&0)+[&6&b{:3}&0]\t", mob->points.hitroll,
                        mob->mob_specials.ex_hitroll);
    menu += fmt::format("&2&bB&0) Damroll     : (&6{:3}&0)+[&6&b{:3}&0]\n", mob->points.damroll,
                        mob->mob_specials.ex_damroll);
    menu += fmt::format("&2&bC&0&0) # Dam Dice : (&6{:3}&0)+[&6&b{:3}&0]\t", mob->mob_specials.damnodice,
                        mob->mob_specials.ex_damnodice);
    menu += fmt::format("&2&bD&0) Size Dam Die: (&6{:3}&0)+[&6&b{:3}&0]    (ie. XdY + Z)\n",
                        mob->mob_specials.damsizedice, mob->mob_specials.ex_damsizedice);
    menu +=
        fmt::format("&2&bE&0) # HP Dice  : (&6{:3}&0)+[&6&b{:3}&0]\t", mob->points.hit, mob->mob_specials.ex_hpnumdice);
    menu += fmt::format("&2&bF&0) Size HP Dice: (&6{:3}&0)+[&6&b{:3}&0]\t", mob->points.mana,
                        mob->mob_specials.ex_hpsizedice);
    menu += fmt::format("&2&bG&0) Bonus: (&6{:5}&0)+[&6&b{:5}&0]\n", GET_EX_MAIN_HP(mob) + (mob)->points.move,
                        GET_MOVE(mob));
    menu += fmt::format("&2&bH&0) Armor Class: (&6{:3}&0)+[&6&b{:3}&0]\t", GET_AC(mob), GET_EX_AC(mob));
    menu += fmt::format("&2&bI&0) Exp         : [&6&b{:9}&0]\n", GET_EX_EXP(mob));
    menu += fmt::format("&2&bJ&0) Gold       : [&6&b{:8}&0]\t&2&bK&0) Platinum    : [&6&b{:9}&0]\n", GET_EX_GOLD(mob),
                        GET_EX_PLATINUM(mob));
    menu += fmt::format("&2&bL&0) Perception : [&6&b{:4}&0]\t\t&2&bM&0) Hiddenness  : [&6&b{:4}&0]\n",
                        GET_PERCEPTION(mob), GET_HIDDENNESS(mob));
    send_to_char(menu.c_str(), d->character);

    sprintflag(buf1, MOB_FLAGS(mob), NUM_MOB_FLAGS, action_bits);
    sprintflag(buf2, EFF_FLAGS(mob), NUM_EFF_FLAGS, effect_flags);

    menu = "";
    menu += fmt::format("&2&bN&0) Life Force    : {}{}&0\n", LIFEFORCE_COLOR(mob), capitalize(LIFEFORCE_NAME(mob)));
    menu += fmt::format("&2&bO&0) Composition   : {}{}&0\n", COMPOSITION_COLOR(mob), capitalize(COMPOSITION_NAME(mob)));
    menu += fmt::format("&2&bP&0) Stance        : &6{}&0\n", stance_types[(int)GET_STANCE(mob)]);
    menu += fmt::format("&2&bR&0) Load Position : &6{}&0\n", position_types[(int)GET_POS(mob)]);
    menu += fmt::format("&2&bT&0) Default Pos   : &6{}&0\n", position_types[(int)GET_DEFAULT_POS(mob)]);
    menu += fmt::format("&2&bU&0) Attack Type   : &6{}&0\n", attack_hit_text[GET_ATTACK(mob)].singular);
    menu += fmt::format("&2&bV&0) Act Flags     : &6{}&0\n", buf1);
    menu += fmt::format("&2&bW&0) Aff Flags     : &6{}&0\n", buf2);
    menu += fmt::format("&2&bS&0) Script        : &6{}&0\n", mob->proto_script ? "&6&bSet&0" : "&6Not Set&0");
    menu += "&2&bQ&0) Quit\nEnter choice:\n";
    send_to_char(menu.c_str(), d->character);

    OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/**************************************************************************
 *                The GARGANTAUN event handler                      *
 **************************************************************************/

void medit_parse(DescriptorData *d, char *arg) {
    int i;
    if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
        if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1]))))) {
            send_to_char("Field must be numerical, try again:\n", d->character);
            return;
        }
    }

    switch (OLC_MODE(d)) {
        /*-------------------------------------------------------------------*/
    case MEDIT_CONFIRM_SAVESTRING:
        /*. Ensure mob has MOB_ISNPC set or things will go pair shaped . */
        SET_FLAG(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
        switch (*arg) {
        case 'y':
        case 'Y':
            /*. Save the mob in memory and to disk  . */
            send_to_char("Saving mobile to memory.\n", d->character);
            medit_save_internally(d);
            sprintf(buf, "OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
            mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), true);
            /* FALL THROUGH */
            cleanup_olc(d, CLEANUP_STRUCTS);
            return;
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            send_to_char("Invalid choice!\n", d->character);
            send_to_char("Do you wish to save the mobile?\n", d->character);
            return;
        }
        break;

        /*-------------------------------------------------------------------*/
    case MEDIT_MAIN_MENU:
        i = 0;
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) { /* Anything been changed? */
                send_to_char("Do you wish to save the changes to the mobile? (y/n)\n", d->character);
                OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '0':
            OLC_MODE(d) = MEDIT_ALIAS;
            i--;
            break;
        case '1':
            OLC_MODE(d) = MEDIT_S_DESC;
            i--;
            break;
        case '2':
            OLC_MODE(d) = MEDIT_L_DESC;
            i--;
            break;
        case '3':
            OLC_MODE(d) = MEDIT_D_DESC;
            write_to_output("Enter mob description: (/s saves /h for help)\n\n", d);
            string_write(d, &OLC_MOB(d)->player.description, MAX_MOB_DESC);
            OLC_VAL(d) = 1;
            return;
        case '4':
            OLC_MODE(d) = MEDIT_RACE;
            medit_race_types(d);
            return;
        case '5':
            OLC_MODE(d) = MEDIT_SIZE;
            medit_size(d);
            return;
        case '6':
            OLC_MODE(d) = MEDIT_SEX;
            medit_disp_sex(d);
            return;
        case '7':
            OLC_MODE(d) = MEDIT_LEVEL;
            i++;
            break;
        case '8':
            OLC_MODE(d) = MEDIT_CLASS;
            medit_class_types(d);
            return;
        case '9':
            OLC_MODE(d) = MEDIT_ALIGNMENT;
            i++;
            break;
        case 'a':
        case 'A':
            OLC_MODE(d) = MEDIT_HITROLL;
            i++;
            break;
        case 'b':
        case 'B':
            OLC_MODE(d) = MEDIT_DAMROLL;
            i++;
            break;
        case 'c':
        case 'C':
            OLC_MODE(d) = MEDIT_NDD;
            i++;
            break;
        case 'd':
        case 'D':
            OLC_MODE(d) = MEDIT_SDD;
            i++;
            break;
        case 'e':
        case 'E':
            OLC_MODE(d) = MEDIT_NUM_HP_DICE;
            i++;
            break;
        case 'f':
        case 'F':
            OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
            i++;
            break;
        case 'g':
        case 'G':
            OLC_MODE(d) = MEDIT_ADD_HP;
            i++;
            break;
        case 'h':
        case 'H':
            OLC_MODE(d) = MEDIT_AC;
            i++;
            break;
        case 'i':
        case 'I':
            OLC_MODE(d) = MEDIT_EXP;
            i++;
            break;
        case 'j':
        case 'J':
            OLC_MODE(d) = MEDIT_EX_GOLD;
            i++;
            break;
        case 'k':
        case 'K':
            OLC_MODE(d) = MEDIT_EX_PLATINUM;
            i++;
            break;
        case 'l':
        case 'L':
            OLC_MODE(d) = MEDIT_PERCEPTION;
            i++;
            break;
        case 'm':
        case 'M':
            OLC_MODE(d) = MEDIT_HIDDENNESS;
            i++;
            break;
        case 'n':
        case 'N':
            OLC_MODE(d) = MEDIT_LIFEFORCE;
            medit_disp_lifeforces(d);
            return;
            break;
        case 'o':
        case 'O':
            OLC_MODE(d) = MEDIT_COMPOSITION;
            medit_disp_compositions(d);
            return;
            break;
        case 'p':
        case 'P':
            OLC_MODE(d) = MEDIT_STANCE;
            medit_disp_stances(d);
            return;
        case 'r':
        case 'R':
            OLC_MODE(d) = MEDIT_POS;
            medit_disp_positions(d);
            return;
        case 't':
        case 'T':
            OLC_MODE(d) = MEDIT_DEFAULT_POS;
            medit_disp_positions(d);
            return;
        case 'u':
        case 'U':
            OLC_MODE(d) = MEDIT_ATTACK;
            medit_disp_attack_types(d);
            return;
        case 'v':
        case 'V':
            OLC_MODE(d) = MEDIT_NPC_FLAGS;
            medit_disp_mob_flags(d);
            return;
        case 'w':
        case 'W':
            OLC_MODE(d) = MEDIT_AFF_FLAGS;
            medit_disp_aff_flags(d);
            return;
        case 's':
        case 'S':
            OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
            dg_script_menu(d);
            return;
            /*  purging = bad thing!  removed from display back in 2k?, commented out
               20060409 - RLS case '?': if (GET_MOB_RNUM(OLC_MOB(d)) == -1)
               {
               send_to_char("You cannot purge a non-existent (unsaved) mob! Choose
               again:\n",d->character);
               }
               else if (GET_LEVEL(d->character)<LVL_HEAD_B)
               {
               sprintf(buf,"You are too low level to purge! Get a level %d or greater
               to do this.\n",LVL_HEAD_B); send_to_char(buf,d->character);
               }
               else
               {
               OLC_MODE(d) = MEDIT_PURGE_MOBILE;
               * make extra sure*
               send_to_char("Purging will also remove all existing mobiles of this
               sort!\n", d->character); send_to_char("Are you sure you wish to
               PERMANENTLY DELETE the mobile? (y/n) : ", d->character);
               }
               return; */
        default:
            medit_disp_menu(d);
            return;
        }
        if (i != 0) {
            send_to_char(i == 1    ? "\nEnter new value : "
                         : i == -1 ? "\nEnter new text :\n] "
                                   : "\nOops...:\n",
                         d->character);
            return;
        }
        break;

        /*-------------------------------------------------------------------*/
    case OLC_SCRIPT_EDIT:
        if (dg_script_edit_parse(d, arg))
            return;
        break;
        /*-------------------------------------------------------------------*/
    case MEDIT_ALIAS:
        if (GET_NAMELIST(OLC_MOB(d)))
            free(GET_NAMELIST(OLC_MOB(d)));
        GET_NAMELIST(OLC_MOB(d)) = strdup((arg && *arg) ? arg : "undefined");
        break;
        /*-------------------------------------------------------------------*/
    case MEDIT_S_DESC:
        if (GET_SDESC(OLC_MOB(d)))
            free(GET_SDESC(OLC_MOB(d)));
        GET_SDESC(OLC_MOB(d)) = strdup((arg && *arg) ? arg : "undefined");
        break;
        /*-------------------------------------------------------------------*/
    case MEDIT_L_DESC:
        if (GET_MOBLDESC(OLC_MOB(d)))
            free(GET_MOBLDESC(OLC_MOB(d)));
        if (arg && *arg) {
            strcpy(buf, arg);
            strcat(buf, "\n");
            GET_MOBLDESC(OLC_MOB(d)) = strdup(buf);
        } else
            GET_MOBLDESC(OLC_MOB(d)) = strdup("undefined");

        break;
        /*-------------------------------------------------------------------*/
    case MEDIT_D_DESC:
        /*
         * We should never get here.
         */
        cleanup_olc(d, CLEANUP_ALL);
        mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!", BRF, LVL_GOD, true);
        send_to_char("Oops...\n", d->character);
        break;
        /*-------------------------------------------------------------------*/
    case MEDIT_NPC_FLAGS:
        if ((i = atoi(arg)) == 0)
            break;
        else if (i > 0 && i <= NUM_MOB_FLAGS)
            TOGGLE_FLAG(MOB_FLAGS(OLC_MOB(d)), i - 1);
        medit_disp_mob_flags(d);
        return;
        /*-------------------------------------------------------------------*/
    case MEDIT_AFF_FLAGS:
        if ((i = atoi(arg)) == 0)
            break;
        else if (i > 0 && i <= NUM_EFF_FLAGS)
            TOGGLE_FLAG(EFF_FLAGS(OLC_MOB(d)), i - 1);
        medit_disp_aff_flags(d);
        return;
        /*-------------------------------------------------------------------*/
        /*. Numerical responses . */

    case MEDIT_SEX:
        GET_SEX(OLC_MOB(d)) = MAX(0, MIN(NUM_SEXES - 1, atoi(arg)));
        break;

    case MEDIT_SIZE:
        set_base_size(OLC_MOB(d), MAX(0, MIN(NUM_SIZES - 1, atoi(arg))));
        break;

    case MEDIT_HITROLL:
        OLC_MOB(d)->mob_specials.ex_hitroll = MAX(-50, MIN(50, atoi(arg)));
        OLC_MOB(d)->points.hitroll =
            get_set_hd(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
        OLC_MOB(d)->points.hitroll += OLC_MOB(d)->mob_specials.ex_hitroll;

        break;

    case MEDIT_DAMROLL:
        OLC_MOB(d)->mob_specials.ex_damroll = MAX(-50, MIN(50, atoi(arg)));
        OLC_MOB(d)->points.damroll =
            get_set_hd(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
        OLC_MOB(d)->points.damroll += OLC_MOB(d)->mob_specials.ex_damroll;

        break;

    case MEDIT_NDD:
        OLC_MOB(d)->mob_specials.ex_damnodice = MAX(-30, MIN(30, atoi(arg)));
        OLC_MOB(d)->mob_specials.damnodice =
            get_set_dice(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
        OLC_MOB(d)->mob_specials.damnodice += OLC_MOB(d)->mob_specials.ex_damnodice;
        break;

    case MEDIT_SDD:
        OLC_MOB(d)->mob_specials.ex_damsizedice = MAX(-125, MIN(125, atoi(arg)));
        OLC_MOB(d)->mob_specials.damsizedice =
            get_set_dice(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
        OLC_MOB(d)->mob_specials.damsizedice += OLC_MOB(d)->mob_specials.ex_damsizedice;

        break;

    case MEDIT_NUM_HP_DICE:
        OLC_MOB(d)->mob_specials.ex_hpnumdice = MAX(-30, MIN(30, atoi(arg)));
        OLC_MOB(d)->points.hit = 20;
        OLC_MOB(d)->points.hit += OLC_MOB(d)->mob_specials.ex_hpnumdice;
        break;

    case MEDIT_SIZE_HP_DICE:
        OLC_MOB(d)->mob_specials.ex_hpsizedice = MAX(MIN_ALIGNMENT, MIN(MAX_ALIGNMENT, atoi(arg)));
        OLC_MOB(d)->points.mana =
            get_set_hit(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 2);
        OLC_MOB(d)->points.mana += OLC_MOB(d)->mob_specials.ex_hpsizedice;

        break;

    case MEDIT_ADD_HP:
        GET_MOVE(OLC_MOB(d)) = MAX(-30000, MIN(30000, atoi(arg)));
        GET_EX_MAIN_HP(OLC_MOB(d)) =
            (get_set_hit(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1));
        break;

    case MEDIT_AC:
        GET_EX_AC(OLC_MOB(d)) = MAX(-100, MIN(100, atoi(arg)));
        GET_AC(OLC_MOB(d)) = MIN(
            100, MAX(-100, (get_ac(OLC_MOB(d)->player.level, OLC_MOB(d)->player.race, OLC_MOB(d)->player.class_num) +
                            GET_EX_AC(OLC_MOB(d)))));
        break;

    case MEDIT_EXP:
        GET_EX_EXP(OLC_MOB(d)) = atol(arg);
        GET_EXP(OLC_MOB(d)) = get_set_exp(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race,
                                          OLC_MOB(d)->player.level, GET_ZONE(OLC_MOB(d)));
        GET_EXP(OLC_MOB(d)) += GET_EX_EXP(OLC_MOB(d));
        break;

    case MEDIT_EX_GOLD:
        GET_EX_GOLD(OLC_MOB(d)) = atol(arg);
        GET_GOLD(OLC_MOB(d)) += GET_EX_GOLD(OLC_MOB(d));
        break;

    case MEDIT_EX_PLATINUM:
        GET_EX_PLATINUM(OLC_MOB(d)) = atol(arg);
        GET_PLATINUM(OLC_MOB(d)) += GET_EX_PLATINUM(OLC_MOB(d));
        break;

    case MEDIT_PERCEPTION:
        GET_PERCEPTION(OLC_MOB(d)) = MAX(0, MIN(atol(arg), 1000));
        break;

    case MEDIT_HIDDENNESS:
        GET_HIDDENNESS(OLC_MOB(d)) = MAX(0, MIN(atol(arg), 1000));
        break;

    case MEDIT_LIFEFORCE:
        i = parse_lifeforce(d->character, arg);
        if (i != LIFE_UNDEFINED)
            GET_LIFEFORCE(OLC_MOB(d)) = i;
        else {
            medit_disp_lifeforces(d);
            return;
        }
        break;

    case MEDIT_COMPOSITION:
        i = parse_composition(d->character, arg);
        if (i != COMP_UNDEFINED) {
            BASE_COMPOSITION(OLC_MOB(d)) = i;
            GET_COMPOSITION(OLC_MOB(d)) = i;
        } else {
            medit_disp_compositions(d);
            return;
        }
        break;

    case MEDIT_STANCE:
        GET_STANCE(OLC_MOB(d)) = MAX(0, MIN(NUM_STANCES - 1, atoi(arg)));
        break;

    case MEDIT_POS:
        GET_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS - 1, atoi(arg)));
        break;

    case MEDIT_DEFAULT_POS:
        GET_DEFAULT_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS - 1, atoi(arg)));
        break;

    case MEDIT_ATTACK:
        GET_ATTACK(OLC_MOB(d)) = MAX(0, MIN(NUM_ATTACK_TYPES - 1, atoi(arg)));
        break;

    case MEDIT_LEVEL:
        GET_LEVEL(OLC_MOB(d)) = MAX(1, MIN(100, atoi(arg)));
        /* generate autolevel values for mob */
        break;
    case MEDIT_ALIGNMENT:
        GET_ALIGNMENT(OLC_MOB(d)) = MAX(-1000, MIN(1000, atoi(arg)));
        break;
    case MEDIT_CLASS:
        if (atoi(arg) < 0 || atoi(arg) >= NUM_CLASSES) {
            send_to_char("That choice is out of range.  Please try again:\n", d->character);
            return;
        }
        GET_CLASS(OLC_MOB(d)) = atoi(arg);
        break;

    case MEDIT_RACE:
        if (atoi(arg) < 0 || atoi(arg) >= NUM_RACES) {
            send_to_char("That choice is out of range.  Please try again:\n", d->character);
            return;
        }
        GET_RACE(OLC_MOB(d)) = atoi(arg);
        init_proto_race(OLC_MOB(d));
        init_char_race(OLC_MOB(d));
        update_char_race(OLC_MOB(d));
        /* GET_RACE_ALIGN(OLC_MOB(d)) = ALIGN_OF_RACE(atoi(arg)); */
        break;
    case MEDIT_PURGE_MOBILE:
        switch (*arg) {
        case 'y':
        case 'Y':
            /*. Splat the mob in memory .. */
            send_to_char("Purging mobile from memory.\n", d->character);

            /*need to remove all existing mobs of this type too.. */
            /*ok..we use save internally, but we are purging because of the mode */
            medit_save_internally(d);
            sprintf(buf, "OLC: %s PURGES mob %d", GET_NAME(d->character), OLC_NUM(d));
            mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), true);
            /* FALL THROUGH */
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            send_to_char("Invalid choice!\n", d->character);
            send_to_char("Do you wish to purge the mobile?\n", d->character);
            return;
        }
        break;

        /*-------------------------------------------------------------------*/
    default:
        /*. We should never get here . */
        cleanup_olc(d, CLEANUP_ALL);
        mudlog("SYSERR: OLC: medit_parse(): Reached default case!", BRF, LVL_GOD, true);
        send_to_char("Oops...\n", d->character);
        break;
    }
    /*-------------------------------------------------------------------*/
    /*. END OF CASE
       If we get here, we have probably changed something, and now want to
       return to main menu.  Use OLC_VAL as a 'has changed' flag . */

    /* update species effected stuff */
    GET_EX_MAIN_HP(OLC_MOB(d)) =
        (get_set_hit(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1));
    GET_EXP(OLC_MOB(d)) = get_set_exp(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level,
                                      GET_ZONE(OLC_MOB(d)));
    GET_EXP(OLC_MOB(d)) += GET_EX_EXP(OLC_MOB(d));
    OLC_MOB(d)->points.mana =
        get_set_hit(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 2);
    OLC_MOB(d)->points.mana += OLC_MOB(d)->mob_specials.ex_hpsizedice;
    OLC_MOB(d)->points.hit = 20;
    OLC_MOB(d)->points.hit += OLC_MOB(d)->mob_specials.ex_hpnumdice;
    OLC_MOB(d)->points.damroll =
        get_set_hd(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
    OLC_MOB(d)->points.damroll += OLC_MOB(d)->mob_specials.ex_damroll;
    OLC_MOB(d)->points.hitroll =
        get_set_hd(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
    OLC_MOB(d)->points.hitroll += OLC_MOB(d)->mob_specials.ex_hitroll;
    OLC_MOB(d)->mob_specials.damnodice =
        get_set_dice(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
    OLC_MOB(d)->mob_specials.damnodice += OLC_MOB(d)->mob_specials.ex_damnodice;
    OLC_MOB(d)->mob_specials.damsizedice =
        get_set_dice(OLC_MOB(d)->player.class_num, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
    OLC_MOB(d)->mob_specials.damsizedice += OLC_MOB(d)->mob_specials.ex_damsizedice;

    OLC_VAL(d) = 1;
    medit_disp_menu(d);
}

/*. End of medit_parse() .*/

bool delete_mobile(obj_num rnum) {
    int i, vnum, zrnum, zone, cmd_no;
    CharData *mob, *tmp, *next;
    bool save_this_zone, mload_just_deleted;

    if (rnum == NOTHING || rnum > top_of_mobt) {
        sprintf(buf, "ERR: delete_mobile() rnum %d out of range", rnum);
        mudlog(buf, NRM, LVL_GOD, true);
        return false;
    }

    mob = &mob_proto[rnum];
    vnum = GET_MOB_VNUM(mob);

    zrnum = find_real_zone_by_room(GET_MOB_VNUM(mob));
    if (zrnum == -1) {
        sprintf(buf, "ERR: delete_mobile() can't identify zone for mob vnum %d", vnum);
        mudlog(buf, NRM, LVL_GOD, true);
        return false;
    }

    for (tmp = character_list; tmp; tmp = next) {
        next = tmp->next;
        if (GET_MOB_VNUM(tmp) == vnum)
            extract_char(tmp);
    }

    /* Make sure all are removed. */
    assert(mob_index[rnum].number == 0);

    /* Adjust rnums of all other mobiles. */
    for (tmp = character_list; tmp; tmp = tmp->next) {
        if (IS_NPC(tmp))
            GET_MOB_RNUM(tmp) -= (GET_MOB_RNUM(tmp) > rnum);
    }

    for (i = rnum; i < top_of_mobt; i++) {
        mob_index[i] = mob_index[i + 1];
        mob_proto[i] = mob_proto[i + 1];
        GET_MOB_RNUM(&mob_proto[i]) = i;
    }

    top_of_mobt--;
    RECREATE(mob_index, IndexData, top_of_mobt + 1);
    RECREATE(mob_proto, CharData, top_of_mobt + 1);

    /* Renumber zone table. */
    for (zone = 0; zone <= top_of_zone_table; zone++) {
        save_this_zone = false;
        mload_just_deleted = false;
        for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
            switch (ZCMD.command) {
            case 'E':
            case 'F':
            case 'G':
                /* These are commands that refer to a previously-loaded mobile.
                 * If we just deleted a command to load the deleted mobile, then
                 * this command should be deleted too. */
                if (mload_just_deleted) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                    cmd_no--;
                    save_this_zone = true;
                }
                break;
            case 'M':
                if (ZCMD.arg1 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                    cmd_no--;
                    mload_just_deleted = true;
                    save_this_zone = true;
                } else {
                    ZCMD.arg1 -= (ZCMD.arg1 > rnum);
                    mload_just_deleted = false;
                }
                break;
            default:
                mload_just_deleted = false;
            }
        }
        if (save_this_zone) {
            olc_add_to_save_list(zone_table[zone].number, OLC_SAVE_ZONE);
        }
    }

    olc_add_to_save_list(zone_table[zrnum].number, OLC_SAVE_MOB);

    return true;
}

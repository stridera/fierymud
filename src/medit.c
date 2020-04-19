/***************************************************************************
 * $Id: medit.c,v 1.66 2009/03/09 20:36:00 myc Exp $
 ***************************************************************************/
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

#include "casting.h"
#include "chars.h"
#include "charsize.h"
#include "comm.h"
#include "composition.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "dg_olc.h"
#include "fight.h"
#include "genzon.h"
#include "handler.h"
#include "lifeforce.h"
#include "math.h"
#include "modify.h"
#include "olc.h"
#include "races.h"
#include "shop.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

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

extern struct player_special_data dummy_mob;

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
int get_ac(int level, int race, int class);
void medit_parse(struct descriptor_data *d, char *arg);
void medit_disp_menu(struct descriptor_data *d);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(int zone_num);
void init_mobile(struct char_data *mob);
void copy_mobile(struct char_data *tmob, struct char_data *fmob);
void medit_disp_stances(struct descriptor_data *d);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);
void medit_class_types(struct descriptor_data *d);
void medit_size(struct descriptor_data *d);
void medit_race_types(struct descriptor_data *d);
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

void medit_free_mobile(struct char_data *mob) {
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

void medit_setup_new(struct descriptor_data *d) {
    struct char_data *mob;

    /*. Alloc some mob shaped space . */
    CREATE(mob, struct char_data, 1);

    init_mobile(mob);

    GET_MOB_RNUM(mob) = -1;
    /*. default strings . */
    GET_NAMELIST(mob) = strdup("mob unfinished");
    GET_SDESC(mob) = strdup("the unfinished mob");
    GET_MOBLDESC(mob) = strdup("An unfinished mob stands here.\r\n");
    GET_DDESC(mob) = strdup("It looks unfinished.\r\n");
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

void medit_setup_existing(struct descriptor_data *d, int rmob_num) {
    struct char_data *mob;

    /*
     * Allocate a scratch mobile structure.
     */
    CREATE(mob, struct char_data, 1);

    copy_mobile(mob, mob_proto + rmob_num);

    OLC_MOB(d) = mob;
    OLC_ITEM_TYPE(d) = MOB_TRIGGER;
    OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */
    dg_olc_script_copy(d);
    medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/
/*. Copy one mob struct to another .*/

void copy_mobile(struct char_data *tmob, struct char_data *fmob) {
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

void init_mobile(struct char_data *mob) {
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

void medit_save_internally(struct descriptor_data *d) {
    int rmob_num, found = 0, new_mob_num = 0, zone, cmd_no, shop;
    struct char_data *new_proto;
    struct index_data *new_index;
    struct char_data *live_mob;
    struct descriptor_data *dsc;

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
            struct char_data *placeholder;
            void extract_char(struct char_data * ch); /* leaves eq behind.. */

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
            CREATE(new_proto, struct char_data, top_of_mobt);
            CREATE(new_index, struct index_data, top_of_mobt);
#if defined(DEBUG)
            fprintf(stderr, "looking to destroy %d (%d)\n", rmob_num, mob_index[rmob_num].virtual);
#endif
            for (i = 0; i <= top_of_mobt; i++) {
                if (!found) { /* Is this the place? */
                    /*    if ((rmob_num > top_of_mobt) || (mob_index[rmob_num].virtual >
                     * OLC_NUM(d))) */
                    if (i == rmob_num) {
                        found = TRUE;
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

        CREATE(new_proto, struct char_data, top_of_mobt + 2);
        CREATE(new_index, struct index_data, top_of_mobt + 2);

        for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++) {
            if (!found) { /* Is this the place? */
                /*    if ((rmob_num > top_of_mobt) || (mob_index[rmob_num].virtual >
                 * OLC_NUM(d))) { */
                if (mob_index[rmob_num].virtual > OLC_NUM(d)) {
                    /*
                     * Yep, stick it here.
                     */
                    found = TRUE;
#if defined(DEBUG)
                    fprintf(stderr, "Inserted: rmob_num: %d\n", rmob_num);
#endif
                    new_index[rmob_num].virtual = OLC_NUM(d);
                    new_index[rmob_num].number = 0;
                    new_index[rmob_num].func = NULL;
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
            new_index[rmob_num].virtual = OLC_NUM(d);
            new_index[rmob_num].number = 0;
            new_index[rmob_num].func = NULL;
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
    struct char_data *mob;

    zone = zone_table[zone_num].number;
    top = zone_table[zone_num].top;

    sprintf(fname, "%s/%d.new", MOB_PREFIX, zone);
    if (!(mob_file = fopen(fname, "w"))) {
        mudlog("SYSERR: OLC: Cannot open mob file!", BRF, LVL_GOD, TRUE);
        return;
    }

    /*
     * Seach the database for mobs in this zone and save them.
     */
    for (i = zone * 100; i <= top; i++) {
        if ((rmob_num = real_mobile(i)) != -1) {
            if (fprintf(mob_file, "#%d\n", i) < 0) {
                mudlog("SYSERR: OLC: Cannot write mob file!\r\n", BRF, LVL_GOD, TRUE);
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

void medit_disp_stances(struct descriptor_data *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; *stance_types[i] != '\n'; i++) {
        sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, stance_types[i]);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter stance number:\r\n", d->character);
}

/*. Display positions (sitting, standing etc) .*/

void medit_disp_positions(struct descriptor_data *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; *position_types[i] != '\n'; i++) {
        sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter position number:\r\n", d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */

void medit_disp_sex(struct descriptor_data *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_SEXES; i++) {
        sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter gender number:\r\n", d->character);
}

/*-------------------------------------------------------------------*/
/*
 * Display the mobile's size! -- Cas
 */

void medit_size(struct descriptor_data *d) {
    int i;

    get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_SIZES; i++) {
        sprintf(buf, "%s%2d%s) %c%s\r\n", grn, i, nrm, (sizes[i].name)[0], sizes[i].name + 1);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter size number:\r\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display attack types menu .*/

void medit_disp_attack_types(struct descriptor_data *d) {
    int i;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_ATTACK_TYPES; i++) {
        sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter attack type:\r\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/

#define FLAG_INDEX ((NUM_MOB_FLAGS / columns + 1) * j + i)
void medit_disp_mob_flags(struct descriptor_data *d) {
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
        send_to_char(strcat(buf, "\r\n"), d->character);
    }

    sprintflag(buf1, MOB_FLAGS(OLC_MOB(d)), NUM_MOB_FLAGS, action_bits);
    sprintf(buf, "\r\nCurrent flags : %s%s%s\r\nEnter mob flags (0 to quit) : ", cyn, buf1, nrm);
    send_to_char(buf, d->character);
}

#undef FLAG_INDEX

void medit_class_types(struct descriptor_data *d) {
    int i;
    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    send_to_char("\r\nListings of Classes\r\n", d->character);
#endif
    for (i = 0; i < NUM_CLASSES; i++) {
        sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, classes[i].plainname);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter Class:\r\n", d->character);
}

void medit_race_types(struct descriptor_data *d) {
    int i;
    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    /*send_to_char(".[H.[J", d->character);    */
    send_to_char("\r\nListings of Races\r\n", d->character);
#endif
    for (i = 0; i < NUM_RACES; i++) {
        sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, races[i].plainname);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter Races:\r\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

#define FLAG_INDEX ((NUM_EFF_FLAGS / columns + 1) * j + i)
void medit_disp_aff_flags(struct descriptor_data *d) {
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
        send_to_char(strcat(buf, "\r\n"), d->character);
    }

    sprintflag(buf1, EFF_FLAGS(OLC_MOB(d)), NUM_EFF_FLAGS, effect_flags);
    sprintf(buf, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit):\r\n", cyn, buf1, nrm);
    send_to_char(buf, d->character);
}

#undef FLAG_INDEX

/*-------------------------------------------------------------------*/
/*. Display life forces menu .*/

void medit_disp_lifeforces(struct descriptor_data *d) {
    int i;

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    for (i = 0; i < NUM_LIFEFORCES; i++) {
        sprintf(buf, "%s%2d%s) %s%c%s%s\r\n", grn, i, nrm, lifeforces[i].color, UPPER((lifeforces[i].name)[0]),
                lifeforces[i].name + 1, nrm);
        send_to_char(buf, d->character);
    }
    send_to_char("Enter life force number:\r\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display compositions menu .*/

void medit_disp_compositions(struct descriptor_data *d) {
#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif
    list_olc_compositions(d->character);
    send_to_char("Enter composition number:\r\n", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display main menu .*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d) {
    struct char_data *mob;

    mob = OLC_MOB(d);
    get_char_cols(d->character);

    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "-- Mob: '&5%s&0'  vnum: [&2%5d&0]\r\n"
            "&2&b0&0) Alias: &6%s&0\r\n"
            "&2&b1&0) Short: &6%s&0\r\n"
            "&2&b2&0) Long : &6%s&0\r"
            "&2&b3&0) Description:\r\n&6%s&0"
            "&2&b4&0) Race: &6%-10s&0            &2&b5&0) Size: &6%-10s&0        "
            " &2&b6&0) Sex: &6%s&0\r\n"
            "&2&b7&0) Level: &6%-3d&0                  &2&b8&0) Class: &6%-10s&0 "
            "       &2&b9&0) Alignment: &6%d&0\r\n",
            GET_SDESC(mob), OLC_NUM(d), GET_NAMELIST(mob), GET_SDESC(mob), GET_MOBLDESC(mob), GET_DDESC(mob),
            RACE_PLAINNAME(mob), SIZE_DESC(mob), genders[(int)GET_SEX(mob)], GET_LEVEL(mob),
            classes[(int)GET_CLASS(mob)].plainname, GET_ALIGNMENT(mob));
    send_to_char(buf, d->character);

    sprintf(buf,
            "&2&bA&0) Hitroll    : (&6%3d&0) [&6&b%3d&0]    &2&bB&0) Damroll     : "
            "(&6%3d&0) [&6&b%3d&0]\r\n"
            "&2&bC&0&0) # Dam Dice : (&6%3d&0) [&6&b%3d&0]    &2&bD&0) Size Dam Die: "
            "(&6%3d&0) [&6&b%3d&0]  (ie. XdY + Z) \r\n"
            "&2&bE&0) # HP Dice  : (&6%3d&0) [&6&b%3d&0]    &2&bF&0) Size HP Dice: "
            "(&6%3d&0) [&6&b%3d&0] &2&bG&0) Bonus: (&6%5d&0) [&6&b%5d&0]\r\n"
            "&2&bH&0) Armor Class: (&6%3d&0) [&6&b%3d&0]    &2&bI&0) Exp         : "
            "[&6&b%9ld&0]\r\n"
            "&2&bJ&0) Gold       : [&6&b%8d&0]     &2&bK&0) Platinum     : "
            "[&6&b%9d&0]\r\n",
            (mob->points.hitroll), (mob->mob_specials.ex_hitroll), (mob->points.damroll),
            (mob->mob_specials.ex_damroll), (mob->mob_specials.damnodice), (mob->mob_specials.ex_damnodice),
            (mob->mob_specials.damsizedice), (mob->mob_specials.ex_damsizedice), (mob)->points.hit,
            (mob->mob_specials.ex_hpnumdice), (mob->points.mana), (mob->mob_specials.ex_hpsizedice),
            (GET_EX_MAIN_HP(mob) + (mob)->points.move), GET_MOVE(mob), GET_AC(mob), GET_EX_AC(mob),
            (long)GET_EX_EXP(mob), GET_EX_GOLD(mob), GET_EX_PLATINUM(mob));
    send_to_char(buf, d->character);

    sprintflag(buf1, MOB_FLAGS(mob), NUM_MOB_FLAGS, action_bits);
    sprintflag(buf2, EFF_FLAGS(mob), NUM_EFF_FLAGS, effect_flags);

    sprintf(buf,
            "&2&bL&0) Perception : [&6&b%4ld&0]         &2&bM&0) Hiddenness  : "
            "[&6&b%4ld&0]\r\n"
            "&2&bN&0) Life Force    : %s%c%s&0\r\n"
            "&2&bO&0) Composition   : %s%c%s&0\r\n"
            "&2&bP&0) Stance        : &6%s&0\r\n"
            "&2&bR&0) Load Position : &6%s&0\r\n"
            "&2&bT&0) Default Pos   : &6%s&0\r\n"
            "&2&bU&0) Attack Type   : &6%s&0\r\n"
            "&2&bV&0) Act Flags     : &6%s&0\r\n"
            "&2&bW&0) Aff Flags     : &6%s&0\r\n"
            "&2&bS&0) Script        : &6%s&0\r\n"
            "&2&bQ&0) Quit\r\n"
            "Enter choice:\r\n",
            GET_PERCEPTION(mob), GET_HIDDENNESS(mob), LIFEFORCE_COLOR(mob), UPPER(*LIFEFORCE_NAME(mob)),
            LIFEFORCE_NAME(mob) + 1, COMPOSITION_COLOR(mob), UPPER(*COMPOSITION_NAME(mob)), COMPOSITION_NAME(mob) + 1,
            stance_types[(int)GET_STANCE(mob)], position_types[(int)GET_POS(mob)],
            position_types[(int)GET_DEFAULT_POS(mob)], attack_hit_text[GET_ATTACK(mob)].singular, buf1, buf2,
            mob->proto_script ? "&6&bSet&0" : "&6Not Set&0");
    send_to_char(buf, d->character);

    OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/**************************************************************************
 *                The GARGANTAUN event handler                      *
 **************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg) {
    int i;
    if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
        if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1]))))) {
            send_to_char("Field must be numerical, try again:\r\n", d->character);
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
            send_to_char("Saving mobile to memory.\r\n", d->character);
            medit_save_internally(d);
            sprintf(buf, "OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
            mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
            /* FALL THROUGH */
            cleanup_olc(d, CLEANUP_STRUCTS);
            return;
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            send_to_char("Invalid choice!\r\n", d->character);
            send_to_char("Do you wish to save the mobile?\r\n", d->character);
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
                send_to_char("Do you wish to save the changes to the mobile? (y/n)\r\n", d->character);
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
            write_to_output("Enter mob description: (/s saves /h for help)\r\n\r\n", d);
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
               again:\r\n",d->character);
               }
               else if (GET_LEVEL(d->character)<LVL_HEAD_B)
               {
               sprintf(buf,"You are too low level to purge! Get a level %d or greater
               to do this.\r\n",LVL_HEAD_B); send_to_char(buf,d->character);
               }
               else
               {
               OLC_MODE(d) = MEDIT_PURGE_MOBILE;
               * make extra sure*
               send_to_char("Purging will also remove all existing mobiles of this
               sort!\r\n", d->character); send_to_char("Are you sure you wish to
               PERMANENTLY DELETE the mobile? (y/n) : ", d->character);
               }
               return; */
        default:
            medit_disp_menu(d);
            return;
        }
        if (i != 0) {
            send_to_char(i == 1 ? "\r\nEnter new value : "
                                : i == -1 ? "\r\nEnter new text :\r\n] " : "\r\nOops...:\r\n",
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
            strcat(buf, "\r\n");
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
        mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!", BRF, LVL_GOD, TRUE);
        send_to_char("Oops...\r\n", d->character);
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
            get_set_hd(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
        OLC_MOB(d)->points.hitroll += OLC_MOB(d)->mob_specials.ex_hitroll;

        break;

    case MEDIT_DAMROLL:
        OLC_MOB(d)->mob_specials.ex_damroll = MAX(-50, MIN(50, atoi(arg)));
        OLC_MOB(d)->points.damroll =
            get_set_hd(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
        OLC_MOB(d)->points.damroll += OLC_MOB(d)->mob_specials.ex_damroll;

        break;

    case MEDIT_NDD:
        OLC_MOB(d)->mob_specials.ex_damnodice = MAX(-30, MIN(30, atoi(arg)));
        OLC_MOB(d)->mob_specials.damnodice =
            get_set_dice(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
        OLC_MOB(d)->mob_specials.damnodice += OLC_MOB(d)->mob_specials.ex_damnodice;
        break;

    case MEDIT_SDD:
        OLC_MOB(d)->mob_specials.ex_damsizedice = MAX(-125, MIN(125, atoi(arg)));
        OLC_MOB(d)->mob_specials.damsizedice =
            get_set_dice(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
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
            get_set_hit(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 2);
        OLC_MOB(d)->points.mana += OLC_MOB(d)->mob_specials.ex_hpsizedice;

        break;

    case MEDIT_ADD_HP:
        GET_MOVE(OLC_MOB(d)) = MAX(-30000, MIN(30000, atoi(arg)));
        GET_EX_MAIN_HP(OLC_MOB(d)) =
            (get_set_hit(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1));
        break;

    case MEDIT_AC:
        GET_EX_AC(OLC_MOB(d)) = MAX(-100, MIN(100, atoi(arg)));
        GET_AC(OLC_MOB(d)) =
            MIN(100, MAX(-100, (get_ac(OLC_MOB(d)->player.level, OLC_MOB(d)->player.race, OLC_MOB(d)->player.class) +
                                GET_EX_AC(OLC_MOB(d)))));
        break;

    case MEDIT_EXP:
        GET_EX_EXP(OLC_MOB(d)) = atol(arg);
        GET_EXP(OLC_MOB(d)) = get_set_exp(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level,
                                          GET_ZONE(OLC_MOB(d)));
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
            send_to_char("That choice is out of range.  Please try again:\r\n", d->character);
            return;
        }
        GET_CLASS(OLC_MOB(d)) = atoi(arg);
        break;

    case MEDIT_RACE:
        if (atoi(arg) < 0 || atoi(arg) >= NUM_RACES) {
            send_to_char("That choice is out of range.  Please try again:\r\n", d->character);
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
            send_to_char("Purging mobile from memory.\r\n", d->character);

            /*need to remove all existing mobs of this type too.. */
            /*ok..we use save internally, but we are purging because of the mode */
            medit_save_internally(d);
            sprintf(buf, "OLC: %s PURGES mob %d", GET_NAME(d->character), OLC_NUM(d));
            mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
            /* FALL THROUGH */
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            send_to_char("Invalid choice!\r\n", d->character);
            send_to_char("Do you wish to purge the mobile?\r\n", d->character);
            return;
        }
        break;

        /*-------------------------------------------------------------------*/
    default:
        /*. We should never get here . */
        cleanup_olc(d, CLEANUP_ALL);
        mudlog("SYSERR: OLC: medit_parse(): Reached default case!", BRF, LVL_GOD, TRUE);
        send_to_char("Oops...\r\n", d->character);
        break;
    }
    /*-------------------------------------------------------------------*/
    /*. END OF CASE
       If we get here, we have probably changed something, and now want to
       return to main menu.  Use OLC_VAL as a 'has changed' flag . */

    /* update species effected stuff */
    GET_EX_MAIN_HP(OLC_MOB(d)) =
        (get_set_hit(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1));
    GET_EXP(OLC_MOB(d)) =
        get_set_exp(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, GET_ZONE(OLC_MOB(d)));
    GET_EXP(OLC_MOB(d)) += GET_EX_EXP(OLC_MOB(d));
    OLC_MOB(d)->points.mana =
        get_set_hit(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 2);
    OLC_MOB(d)->points.mana += OLC_MOB(d)->mob_specials.ex_hpsizedice;
    OLC_MOB(d)->points.hit = 20;
    OLC_MOB(d)->points.hit += OLC_MOB(d)->mob_specials.ex_hpnumdice;
    OLC_MOB(d)->points.damroll =
        get_set_hd(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
    OLC_MOB(d)->points.damroll += OLC_MOB(d)->mob_specials.ex_damroll;
    OLC_MOB(d)->points.hitroll =
        get_set_hd(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
    OLC_MOB(d)->points.hitroll += OLC_MOB(d)->mob_specials.ex_hitroll;
    OLC_MOB(d)->mob_specials.damnodice =
        get_set_dice(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 0);
    OLC_MOB(d)->mob_specials.damnodice += OLC_MOB(d)->mob_specials.ex_damnodice;
    OLC_MOB(d)->mob_specials.damsizedice =
        get_set_dice(OLC_MOB(d)->player.class, OLC_MOB(d)->player.race, OLC_MOB(d)->player.level, 1);
    OLC_MOB(d)->mob_specials.damsizedice += OLC_MOB(d)->mob_specials.ex_damsizedice;

    OLC_VAL(d) = 1;
    medit_disp_menu(d);
}

/*. End of medit_parse() .*/

bool delete_mobile(obj_num rnum) {
    int i, vnum, zrnum, zone, cmd_no;
    struct char_data *mob, *tmp, *next;
    bool save_this_zone, mload_just_deleted;

    if (rnum == NOTHING || rnum > top_of_mobt) {
        sprintf(buf, "ERR: delete_mobile() rnum %d out of range", rnum);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return FALSE;
    }

    mob = &mob_proto[rnum];
    vnum = GET_MOB_VNUM(mob);

    zrnum = find_real_zone_by_room(GET_MOB_VNUM(mob));
    if (zrnum == -1) {
        sprintf(buf, "ERR: delete_mobile() can't identify zone for mob vnum %d", vnum);
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return FALSE;
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
    RECREATE(mob_index, struct index_data, top_of_mobt + 1);
    RECREATE(mob_proto, struct char_data, top_of_mobt + 1);

    /* Renumber zone table. */
    for (zone = 0; zone <= top_of_zone_table; zone++) {
        save_this_zone = FALSE;
        mload_just_deleted = FALSE;
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
                    save_this_zone = TRUE;
                }
                break;
            case 'M':
                if (ZCMD.arg1 == rnum) {
                    delete_zone_command(&zone_table[zone], cmd_no);
                    cmd_no--;
                    mload_just_deleted = TRUE;
                    save_this_zone = TRUE;
                } else {
                    ZCMD.arg1 -= (ZCMD.arg1 > rnum);
                    mload_just_deleted = FALSE;
                }
                break;
            default:
                mload_just_deleted = FALSE;
            }
        }
        if (save_this_zone) {
            olc_add_to_save_list(zone_table[zone].number, OLC_SAVE_ZONE);
        }
    }

    olc_add_to_save_list(zone_table[zrnum].number, OLC_SAVE_MOB);

    return TRUE;
}

/***************************************************************************
 * $Log: medit.c,v $
 * Revision 1.66  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.65  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.64  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.63  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.62  2008/08/26 03:58:13  jps
 * Replaced real_zone calls with find_real_zone_by_room, since that's what it
 *did. Except the one for wzoneecho, since it needed to find a real zone by zone
 *number.
 *
 * Revision 1.61  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.60  2008/08/17 07:17:58  jps
 * Correctly mark which zone files need saving when deleting a mobile.
 *
 * Revision 1.59  2008/08/17 06:52:31  jps
 * Added delete_mobile.
 *
 * Revision 1.58  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.57  2008/06/11 23:04:48  jps
 * Changed medit menu to be like oedit and redit.
 *
 * Revision 1.56  2008/06/11 22:51:36  jps
 * Tidied the medit menu.
 *
 * Revision 1.55  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.54  2008/04/20 17:49:00  jps
 * Removing unneeded externs.
 *
 * Revision 1.53  2008/04/07 17:24:51  jps
 * Allow mediting of stance.
 *
 * Revision 1.52  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.51  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.50  2008/03/23 00:24:48  jps
 * Fix composition apply editing.
 *
 * Revision 1.49  2008/03/22 21:44:54  jps
 * Add life force and composition to medit.
 *
 * Revision 1.48  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.47  2008/03/17 16:22:42  myc
 * Fixed handling of proto scripts in OLC, including the squashing of
 * a memory leak.
 *
 * Revision 1.46  2008/03/11 02:55:09  jps
 * Use set_base_size when editing mob proto sizes.
 *
 * Revision 1.45  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.44  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.43  2008/03/09 02:40:21  jps
 * Update usage of dummy_mob, which is no longer declared as a pointer.
 *
 * Revision 1.42  2008/02/10 23:30:05  myc
 * Fixing alignment in main medit screen.
 *
 * Revision 1.41  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.40  2008/02/07 01:46:14  myc
 * Removing the size_abbrevs array and renaming SIZE_ABBR to SIZE_DESC,
 * which points to the sizes array.
 *
 * Revision 1.39  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.38  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species
 *into races.
 *
 * Revision 1.37  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.36  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.35  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.34  2007/11/18 16:51:55  myc
 * Fixing LVL_BUILDER references.
 *
 * Revision 1.33  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  They can be set in medit.  They
 * are saved using especs.
 *
 * Revision 1.32  2007/09/15 05:03:46  myc
 * Implementing MOB2 flags.  They are saved in the mob files as an espec.
 * Implemented a new loop method for most of the medit menus so that
 * they get listed column-major instead of by rows.  This applies to the
 * mob flags and aff flags menus, which both also support many flags in
 * one menu; e.g., there is only one aff menu which holds all aff 1,
 * aff2, and aff3 flags.  The distinction between aff 1, 2, and 3 flags
 * has been removed.
 *
 * Revision 1.31  2007/08/14 15:51:52  myc
 * Updating number of aff2 and aff3 flags in medit menus.
 *
 * Revision 1.30  2007/07/24 23:02:52  jps
 * Minor typo fix.
 *
 * Revision 1.29  2007/07/18 00:04:41  jps
 * Save AFF2 and AFF3 flags for mobiles.
 *
 * Revision 1.28  2007/03/27 04:27:05  myc
 * Fixed index bug with sizes where they were all one off.
 *
 * Revision 1.27  2006/04/19 21:04:15  rls
 * Recolorized medit menu and cleaned up formatting
 *
 * Revision 1.26  2006/04/11 09:25:45  rls
 * *** empty log message ***
 *
 * Revision 1.25  2006/04/11 09:07:49  rls
 * Updated mobile display screen... added aff2, aff3
 *
 * Revision 1.24  2004/11/11 22:27:28  rsd
 * Split up a large buffer that the compiler was complaining
 * was over 509 bytes.
 *
 * Revision 1.23  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.22  2002/07/14 03:41:31  rls
 * removed purge functionality from menu
 *
 * Revision 1.21  2001/07/08 16:01:22  mtp
 * added safety check for purge of level LVL_HEAD_B (currently 103)
 *
 * Revision 1.19  2000/11/28 01:46:44  mtp
 * made class explicitly MCLASS_VOID for new mobs
 *
 * Revision 1.18  2000/11/28 01:26:11  mtp
 * removed a lot of mobprog stuff
 *
 * Revision 1.17  2000/11/23 05:11:16  rsd
 * Added the standard comment header, also mopped up some
 * whitespace.  Added back rlog messages from prior to
 * the addition of the $log$ string.
 *
 * Revision 1.16  2000/11/22 01:09:13  mtp
 * added more mob classes (all the ones that are available for players)
 *
 * Revision 1.15  2000/11/09 03:43:19  rsd
 * removed mob progs from the medit display menue and the
 * switch that allows choice P to enter the prog menue
 * system.
 *
 * Revision 1.14  2000/10/14 11:12:40  mtp
 * fixed the olc triggers editting in medit/oedit/redit
 *
 * Revision 1.13  2000/10/13 23:13:42  mtp
 * fixed why scripts weren't showing in medit, but the medit
 * code is _really_ buggy
 *
 * Revision 1.12  1999/12/10 05:13:14  cso
 * added support for choosing mob size (case Z in medit)
 *
 * Revision 1.11  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.10  1999/07/09 22:30:27  jimmy
 * Attempt to control the spiraling of memory.  Added a free() to the
 * prompt code in comm.c to free memory allocated by parse_color().
 * made a global structure dummy_mob and malloc'ed it for mobs
 * to share as their player_specials to cut memory.
 * gurlaek
 *
 * Revision 1.9  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their
 *race/class/level that exactly align with PC's.  PC's no longer have to rent to
 *use skills gained by leveling or when first creating a char.  Languages no
 *longer reset to defaults when a PC levels.  Discovered that languages have
 *been defined right in the middle of the spell area.  This needs to be fixed.
 *A conversion util neeDs to be run on the mob files to compensate for the 13 to
 *-1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.8  1999/06/30 18:25:04  jimmy
 * >> This is a major conversion from the 18 point attribute system to the
 * >> 100 point attribute system.  A few of the major changes are:
 * >> All attributes are now on a scale from 0-100
 * >> Everyone views attribs the same but, the attribs for one race
 * >>   may be differeent for that of another even if they are the
 * >>   same number.
 * >> Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * >> Mobs now have individual random attributes based on race/class.
 * >> The STR_ADD attrib has been completely removed.
 * >> All bonus tables for attribs in constants.c have been replaced by
 * >>   algorithims that closely duplicate the tables except on a 100 scale.
 * >> Some minor changes:
 * >> Race selection at char creation can now be toggled by using
 * >>   <world races off>
 * >> Lots of cleanup done to affected areas of code.
 * >> Setting attributes for mobs in the .mob file no longer functions
 * >>   but is still in the code for later use.
 * >> We now have a spare attribut structure in the pfile because the new
 * >>   system only used three instead of four.
 * >> --gurlaek 6/30/1999
 *
 * Revision 1.7  1999/03/31 16:03:47  jen
 * Added a "()" pair to remove a warning during compilation.
 *
 * Revision 1.6  1999/03/30 19:46:06  jen
 * Changed the medit_mprog_type so that it properly changed the type
 * of mprog... Selina 3-30-99
 *
 * Revision 1.5  1999/02/15 01:12:40  jimmy
 * Yet another atempt to fix the medit crashe bug
 * created by adding the long descr to the pfile.
 * Think this finally got it.
 * fingon
 *
 * Revision 1.4  1999/02/11 23:52:09  jimmy
 * fixed medit crash bug introduced by adding
 * long desc to pfile
 * fingon
 *
 * Revision 1.3  1999/02/10 05:57:14  jimmy
 * Added long description to player file.  Added AFK toggle.
 * removed NOAUCTION toggle.
 * fingon
 *
 * Revision 1.2  1999/01/31 17:07:53  mud
 * Branches 1.2.1
 * Indented entire file
 * IInteresting header comments, and is it just me or
 * Iis about a third of the file commented?
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 * Revision 1.2.1.1
 * Had to backrev to fix medit crash bug.
 * should be all fixed now.
 * fingon
 *
 ***************************************************************************/

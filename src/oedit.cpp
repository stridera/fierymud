/***************************************************************************
 *   File: oedit.c                                        Part of FieryMUD *
 *  Usage:                                                                 *
 *     By: Levork (whoever that is)                                        *
 *         TwyliteMud by Rv. (shameless plug)                              *
 *         Copyright 1996 Harvey Gilpin.                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "bitflags.hpp"
#include "board.hpp"
#include "casting.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_olc.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "shop.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* external variables */
// int *tmp_ptr;

/*------------------------------------------------------------------------*/
/*. Macros .*/

#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*/

void oedit_disp_container_flags_menu(DescriptorData *d);
void oedit_disp_extradesc_menu(DescriptorData *d);
void oedit_disp_weapon_menu(DescriptorData *d);
void oedit_disp_val1_menu(DescriptorData *d);
void oedit_disp_val2_menu(DescriptorData *d);
void oedit_disp_val3_menu(DescriptorData *d);
void oedit_disp_val4_menu(DescriptorData *d);
void oedit_disp_type_menu(DescriptorData *d);
void oedit_disp_extra_menu(DescriptorData *d);
void oedit_disp_wear_menu(DescriptorData *d);
void oedit_disp_menu(DescriptorData *d);

void oedit_parse(DescriptorData *d, char *arg);
void oedit_disp_spells_menu(DescriptorData *d);
void oedit_liquid_type(DescriptorData *d);
void oedit_setup_new(DescriptorData *d);
void oedit_setup_existing(DescriptorData *d, int real_num);
void oedit_save_to_disk(int zone);
void oedit_reverse_exdescs(int zone, CharData *ch);
int oedit_reverse_exdesc(int real_num, CharData *ch);
void oedit_save_internally(DescriptorData *d);
void iedit_save_changes(DescriptorData *d);

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_setup_new(DescriptorData *d) {
    CREATE(OLC_OBJ(d), ObjData, 1);
    clear_object(OLC_OBJ(d));
    OLC_OBJ(d)->name = strdup("unfinished object");
    OLC_OBJ(d)->description = strdup("An unfinished object is lying here.");
    OLC_OBJ(d)->short_description = strdup("an unfinished object");
    GET_OBJ_WEAR(OLC_OBJ(d)) = ITEM_WEAR_TAKE;
    OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
    OLC_VAL(d) = 0;
    oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void oedit_setup_existing(DescriptorData *d, int real_num) {
    ExtraDescriptionData *desc, *temp, *temp2;
    ObjData *obj;

    /*
     * Allocate object in memory.
     */
    CREATE(obj, ObjData, 1);

    clear_object(obj);
    *obj = obj_proto[real_num];

    /*
     * Copy all strings over.
     */
    obj->name = strdup(obj_proto[real_num].name ? obj_proto[real_num].name : "undefined");
    obj->short_description =
        strdup(obj_proto[real_num].short_description ? obj_proto[real_num].short_description : "undefined");
    obj->description = strdup(obj_proto[real_num].description ? obj_proto[real_num].description : "undefined");
    obj->action_description =
        (obj_proto[real_num].action_description ? strdup(obj_proto[real_num].action_description) : nullptr);

    /*
     * Extra descriptions if necessary.
     */
    if (obj_proto[real_num].ex_description) {
        CREATE(temp, ExtraDescriptionData, 1);

        obj->ex_description = temp;
        for (desc = obj_proto[real_num].ex_description; desc; desc = desc->next) {
            temp->keyword = (desc->keyword && *desc->keyword) ? strdup(desc->keyword) : nullptr;
            temp->description = (desc->description && *desc->description) ? strdup(desc->description) : nullptr;
            if (desc->next) {
                CREATE(temp2, ExtraDescriptionData, 1);
                temp->next = temp2;
                temp = temp2;
            } else
                temp->next = nullptr;
        }
    }

    /*. Attatch new obj to players descriptor . */
    OLC_OBJ(d) = obj;
    OLC_VAL(d) = 0;
    OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
    dg_olc_script_copy(d);
    oedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void oedit_save_internally(DescriptorData *d) {
    int i, shop, robj_num, found = false, zone, cmd_no;
    ExtraDescriptionData *desc, *next_one;
    ObjData *obj, *swap, *new_obj_proto;
    IndexData *new_obj_index;
    DescriptorData *dsc;

    /*
     * Write object to internal tables.
     */
    if ((robj_num = real_object(OLC_NUM(d))) > 0) {
        /*
         * Are we purging?
         */
        if (OLC_MODE(d) == OEDIT_PURGE_OBJECT) {
            ObjData *placeholder;
            void extract_obj(ObjData * ch); /* leaves eq behind.. */

            for (obj = object_list; obj; obj = obj->next) {
                if (GET_OBJ_RNUM(obj) == robj_num) {
                    placeholder = obj;
#if defined(DEBUG)
                    fprintf(stderr, "remove object %d ", GET_OBJ_VNUM(obj));
#endif
                    extract_obj(obj);  /*remove all existing objects */
                    obj = placeholder; /*so we can keep removing.. */
#if defined(DEBUG)
                    fprintf(stderr, "(%d left)\n", obj_index[robj_num].number);
#endif
                }
            }
            CREATE(new_obj_proto, ObjData, top_of_objt);
            CREATE(new_obj_index, IndexData, top_of_objt);
#if defined(DEBUG)
            fprintf(stderr, "looking to destroy %d (%d)\n", robj_num, obj_index[robj_num].vnum);
#endif
            for (i = 0; i <= top_of_objt; i++) {
                if (!found) { /* Isdescs the place? */
                    /*    if ((robj_num > top_of_objt) || (mob_index[robj_num].vnum >
                     * OLC_NUM(d))) */
                    if (i == robj_num) {
                        found = true;
                        /* don't copy..it will be blatted by the free later */
                    } else { /* Nope, copy over as normal. */
                        new_obj_index[i] = obj_index[i];
                        new_obj_proto[i] = obj_proto[i];
                    }
                } else { /* We've already found it, copy the rest over. */
                    new_obj_index[i - 1] = obj_index[i];
                    new_obj_proto[i - 1] = obj_proto[i];
                }
            }
            top_of_objt--;
#if !defined(I_CRASH)
            free(obj_index);
            free(obj_proto);
#endif
            obj_index = new_obj_index;
            obj_proto = new_obj_proto;
            /*. Renumber live objects . */
            for (obj = object_list; obj; obj = obj->next)
                if (GET_OBJ_RNUM(obj) != NOTHING && GET_OBJ_RNUM(obj) >= robj_num)
                    GET_OBJ_RNUM(obj)--;

            /*. Renumber zone table . */
            for (zone = 0; zone <= top_of_zone_table; zone++)
                for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
                    switch (ZCMD.command) {
                    case 'P':
                        if (ZCMD.arg3 >= robj_num)
                            ZCMD.arg3--;
                        /*
                         * No break here - drop into next case.
                         */
                    case 'O':
                    case 'G':
                    case 'E':
                        if (ZCMD.arg1 >= robj_num)
                            ZCMD.arg1--;
                        break;
                    case 'R':
                        if (ZCMD.arg2 >= robj_num)
                            ZCMD.arg2--;
                        break;
                    }

            /*. Renumber shop produce . */
            for (shop = 0; shop < top_shop; shop++)
                for (i = 0; SHOP_PRODUCT(shop, i) != -1; i++)
                    if (SHOP_PRODUCT(shop, i) >= robj_num)
                        SHOP_PRODUCT(shop, i)--;

            /*and those being edited     */
            for (dsc = descriptor_list; dsc; dsc = dsc->next)
                if (dsc->connected == CON_SEDIT)
                    for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
                        if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
                            S_PRODUCT(OLC_SHOP(dsc), i)--;

            /*MARK POINT XXXXX */
        } else {
            /*
             * We need to run through each and every object currently in the
             * game to see which ones are pointing todescs prototype.
             * if object is pointing to this prototype, then we need to replace it
             * with the new one.
             */
            CREATE(swap, ObjData, 1);
            for (obj = object_list; obj; obj = obj->next) {
                if (obj->item_number == robj_num) {
                    *swap = *obj;
                    *obj = *OLC_OBJ(d);
                    /*
                     * Copy game-time dependent variables over.
                     */
                    obj->in_room = swap->in_room;
                    obj->item_number = robj_num;
                    obj->carried_by = swap->carried_by;
                    obj->worn_by = swap->worn_by;
                    obj->worn_on = swap->worn_on;
                    obj->in_obj = swap->in_obj;
                    obj->contains = swap->contains;
                    obj->next_content = swap->next_content;
                    obj->next = swap->next;
                    obj->proto_script = OLC_SCRIPT(d);
                }
            }
            free(swap);
            /* now safe to free old proto and write over */
            if (obj_proto[robj_num].name)
                free(obj_proto[robj_num].name);
            if (obj_proto[robj_num].description)
                free(obj_proto[robj_num].description);
            if (obj_proto[robj_num].short_description)
                free(obj_proto[robj_num].short_description);
            if (obj_proto[robj_num].action_description)
                free(obj_proto[robj_num].action_description);
            if (obj_proto[robj_num].ex_description)
                for (desc = obj_proto[robj_num].ex_description; desc; desc = next_one) {
                    next_one = desc->next;
                    if (desc->keyword)
                        free(desc->keyword);
                    if (desc->description)
                        free(desc->description);
                    free(desc);
                }
            /* Must do this before copying OLC_OBJ over */
            if (obj_proto[robj_num].proto_script && obj_proto[robj_num].proto_script != OLC_SCRIPT(d))
                free_proto_script(&obj_proto[robj_num].proto_script);
            obj_proto[robj_num] = *OLC_OBJ(d);
            obj_proto[robj_num].item_number = robj_num;
            obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
        }
    } else {
        /*. It's a new object, we must build new tables to contain it . */

        CREATE(new_obj_index, IndexData, top_of_objt + 2);
        CREATE(new_obj_proto, ObjData, top_of_objt + 2);
        /* start counting through both tables */
        for (i = 0; i <= top_of_objt; i++) {
            /* if we haven't found it */
            if (!found) {
                /*
                 * Check if current vnum is bigger than our vnum.
                 */
                if (obj_index[i].vnum > OLC_NUM(d)) {
                    found = true;
                    robj_num = i;
                    OLC_OBJ(d)->item_number = robj_num;
                    new_obj_index[robj_num].vnum = OLC_NUM(d);
                    new_obj_index[robj_num].number = 0;
                    new_obj_index[robj_num].func = nullptr;
                    new_obj_proto[robj_num] = *(OLC_OBJ(d));
                    new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
                    new_obj_proto[robj_num].in_room = NOWHERE;
                    /*. Copy over the mob that should be here . */
                    new_obj_index[robj_num + 1] = obj_index[robj_num];
                    new_obj_proto[robj_num + 1] = obj_proto[robj_num];
                    new_obj_proto[robj_num + 1].item_number = robj_num + 1;
                } else {
                    /* just copy from old to new, no num change */
                    new_obj_proto[i] = obj_proto[i];
                    new_obj_index[i] = obj_index[i];
                }
            } else {
                /* we HAVE already found it.. therefore copy to object + 1 */
                new_obj_index[i + 1] = obj_index[i];
                new_obj_proto[i + 1] = obj_proto[i];
                new_obj_proto[i + 1].item_number = i + 1;
            }
        }
        if (!found) {
            robj_num = i;
            OLC_OBJ(d)->item_number = robj_num;
            new_obj_index[robj_num].vnum = OLC_NUM(d);
            new_obj_index[robj_num].number = 0;
            new_obj_index[robj_num].func = nullptr;
            new_obj_proto[robj_num] = *(OLC_OBJ(d));
            new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
            new_obj_proto[robj_num].in_room = NOWHERE;
        }

        /* free and replace old tables */
        free(obj_proto);
        free(obj_index);
        obj_proto = new_obj_proto;
        obj_index = new_obj_index;
        top_of_objt++;

        /*. Renumber live objects . */
        for (obj = object_list; obj; obj = obj->next)
            if (GET_OBJ_RNUM(obj) != NOTHING && GET_OBJ_RNUM(obj) >= robj_num)
                GET_OBJ_RNUM(obj)++;

        /*. Renumber zone table . */
        for (zone = 0; zone <= top_of_zone_table; zone++)
            for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
                switch (ZCMD.command) {
                case 'P':
                    if (ZCMD.arg3 >= robj_num)
                        ZCMD.arg3++;
                    /*
                     * No break here - drop into next case.
                     */
                case 'O':
                case 'G':
                case 'E':
                    if (ZCMD.arg1 >= robj_num)
                        ZCMD.arg1++;
                    break;
                case 'R':
                    if (ZCMD.arg2 >= robj_num)
                        ZCMD.arg2++;
                    break;
                }

        /*. Renumber shop produce . */
        for (shop = 0; shop < top_shop; shop++)
            for (i = 0; SHOP_PRODUCT(shop, i) != -1; i++)
                if (SHOP_PRODUCT(shop, i) >= robj_num)
                    SHOP_PRODUCT(shop, i)++;

        /*and those being edited     */
        for (dsc = descriptor_list; dsc; dsc = dsc->next)
            if (dsc->connected == CON_SEDIT)
                for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
                    if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
                        S_PRODUCT(OLC_SHOP(dsc), i)++;
    }
    olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ);
}

/*------------------------------------------------------------------------*/

void oedit_save_to_disk(int zone_num) {
    std::string output;
    int counter, counter2, realcounter;
    FILE *fp;
    ObjData *obj;
    ExtraDescriptionData *ex_desc;

    auto filename = fmt::format("{}/{}.new", OBJ_PREFIX, zone_table[zone_num].number);
    if (!(fp = fopen(filename.c_str(), "w+"))) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: Cannot open objects file!");
        return;
    }
    /*
     * Start running through all objects in this zone.
     */
    for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++) {
        if ((realcounter = real_object(counter)) >= 0) {
            if ((obj = (obj_proto + realcounter))->action_description) {
                strcpy(buf1, obj->action_description);
                strip_string(buf1);
            } else
                *buf1 = '\0';

            fprintf(fp,
                    "#%d\n"
                    "%s~\n"
                    "%s~\n"
                    "%s~\n"
                    "%s~\n"
                    "%d %ld %ld %d\n"
                    "%d %d %d %d %d %d %d\n"
                    "%.2f %d %d %ld %d %d %ld %ld\n",
                    GET_OBJ_VNUM(obj), (obj->name && *obj->name) ? obj->name : "undefined",
                    (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
                    (obj->description && *obj->description) ? obj->description : "undefined", buf1, GET_OBJ_TYPE(obj),
                    GET_OBJ_FLAGS(obj)[0], GET_OBJ_WEAR(obj), GET_OBJ_LEVEL(obj), GET_OBJ_VAL(obj, 0),
                    GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4),
                    GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6), GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj),
                    GET_OBJ_TIMER(obj), GET_OBJ_EFF_FLAGS(obj)[0], 0, 0, GET_OBJ_EFF_FLAGS(obj)[1],
                    GET_OBJ_EFF_FLAGS(obj)[2]);

            script_save_to_disk(fp, obj, OBJ_TRIGGER);

            /*
             * Do we have extra descriptions?
             */
            if (obj->ex_description) { /* Yes, save them too. */
                for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
                    /*
                     * Sanity check to prevent nasty protection faults.
                     */
                    if (!*ex_desc->keyword || !*ex_desc->description) {
                        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!");
                        continue;
                    }
                    strcpy(buf1, ex_desc->description);
                    strip_string(buf1);
                    fprintf(fp,
                            "E\n"
                            "%s~\n"
                            "%s~\n",
                            ex_desc->keyword, buf1);
                }
            }

            /* Do we have affects? */
            for (counter2 = 0; counter2 < MAX_OBJ_APPLIES; counter2++)
                if (obj->applies[counter2].location)
                    fprintf(fp,
                            "A\n"
                            "%d %d\n",
                            obj->applies[counter2].location, obj->applies[counter2].modifier);

            if (obj->obj_flags.hiddenness)
                fprintf(fp, "H\n%ld\n", obj->obj_flags.hiddenness);

            if (GET_OBJ_FLAGS(obj)[1])
                fprintf(fp, "X\n%ld\n", GET_OBJ_FLAGS(obj)[1]);
        }
    }

    /* write final line, close */
    fprintf(fp, "$~\n");
    fclose(fp);
    sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
    /*
     * We're fubar'd if we crash between the two lines below.
     */
    remove(buf2);
    rename(filename.c_str(), buf2);

    olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_OBJ);
}

/*------------------------------------------------------------------------*/

int oedit_reverse_exdesc(int real_num, CharData *ch) {
    ObjData *obj;
    ExtraDescriptionData *ex_desc, *tmp;

    obj = obj_proto + real_num;

    if ((ex_desc = obj->ex_description) && ex_desc->next) {
        for (obj->ex_description = nullptr; ex_desc;) {
            tmp = ex_desc->next;
            ex_desc->next = obj->ex_description;
            obj->ex_description = ex_desc;
            ex_desc = tmp;
        }
        if (ch) {
            char_printf(ch, "Reversed exdescs of object {:d}, {}.\n", GET_OBJ_VNUM(obj), obj->short_description);
        }
        olc_add_to_save_list(zone_table[find_real_zone_by_room(GET_OBJ_VNUM(obj))].number, OLC_SAVE_OBJ);
        return 1;
    }
    return 0;
}

/*------------------------------------------------------------------------*/

void oedit_reverse_exdescs(int zone, CharData *ch) {
    int counter, realcounter, numprocessed = 0, nummodified = 0;

    for (counter = zone_table[zone].number * 100; counter <= zone_table[zone].top; counter++) {
        if ((realcounter = real_object(counter)) >= 0) {
            nummodified += oedit_reverse_exdesc(realcounter, ch);
            numprocessed++;
        }
    }

    /*
       if (nummodified)
       olc_add_to_save_list(zone_table[zone].number, OLC_SAVE_OBJ);
     */

    if (ch) {
        char_printf(ch, "Modified {:d} of {:d} object prototype{} in zone {:d}, {}.\n", nummodified, numprocessed,
                    numprocessed == 1 ? "" : "s", zone_table[zone].number, zone_table[zone].name);
    }
}

/**************************************************************************
 *                         Menu functions                                 *
 **************************************************************************/

/*
 *  For wall flags.
 */
void oedit_disp_wall_block_dirs(DescriptorData *d) {
    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "^[[H^[[J");
#endif
    sprintf(buf,
            "%s0%s) NORTH\n"
            "%s1%s) EAST\n"
            "%s2%s) SOUTH\n"
            "%s3%s) WEST\n"
            "%s4%s) UP\n"
            "%s5%s) DOWN\n"
            "Enter flag:\n",
            grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);
    char_printf(d->character, buf);
}

/*
 * For container flags.
 */
void oedit_disp_container_flags_menu(DescriptorData *d) {
    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    char_printf(d->character,
                "%s1%s) CLOSEABLE\n"
                "%s2%s) PICKPROOF\n"
                "%s3%s) CLOSED\n"
                "%s4%s) LOCKED\n"
                "Container flags: %s%s%s\n"
                "Enter flag, 0 to quit:\n",
                grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn,
                sprintbit(GET_OBJ_VAL(OLC_OBJ(d), VAL_CONTAINER_BITS), container_bits), nrm);
}

/*
 * For extra descriptions.
 */
void oedit_disp_extradesc_menu(DescriptorData *d) {
    ExtraDescriptionData *extra_desc = OLC_DESC(d);

    strcpy(buf1, !extra_desc->next ? "<Not set>\n" : "Set.");

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    sprintf(buf,
            "Extra desc menu\n"
            "%s1%s) Keyword: %s%s\n"
            "%s2%s) Description:\n%s%s\n"
            "%s3%s) Goto next description: %s\n"
            "%s0%s) Quit\n"
            "Enter choice:\n",
            grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>", grn, nrm,
            yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>", grn, nrm,
            buf1, grn, nrm);
    char_printf(d->character, buf);
    OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/*
 * Ask for *which* apply to edit.
 */
void oedit_disp_prompt_apply_menu(DescriptorData *d) {
    int counter;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    for (counter = 0; counter < MAX_OBJ_APPLIES; counter++) {
        sprintf(buf, " %s%d%s) %s\n", grn, counter + 1, nrm,
                format_apply(OLC_OBJ(d)->applies[counter].location, OLC_OBJ(d)->applies[counter].modifier));
        char_printf(d->character, buf);
    }
    char_printf(d->character, "\nEnter apply to modify (0 to quit):\n");
    OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

#define FLAG_INDEX ((NUM_EFF_FLAGS / columns + 1) * j + i)
void oedit_disp_aff_flags(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, ".[H.[J");
#endif

    for (i = 0; i <= NUM_EFF_FLAGS / columns; ++i) {
        std::string output;
        for (j = 0; j < columns; ++j) {
            if (FLAG_INDEX >= NUM_EFF_FLAGS)
                break;
            output += fmt::format("{}{:2d}{}) {:20.20s}", grn, FLAG_INDEX + 1, nrm, effect_flags[FLAG_INDEX]);
        }
        output += "\n";
        char_printf(d->character, output);
    }

    char_printf(d->character,
                "\nSpell flags: {}{}{}\n"
                "Enter spell flag, 0 to quit : ",
                cyn, sprintflag(GET_OBJ_EFF_FLAGS(OLC_OBJ(d)), NUM_EFF_FLAGS, effect_flags), nrm);
}

#undef FLAG_INDEX

void oedit_disp_component(DescriptorData *d) {
    /*  int counter; */

    get_char_cols(d->character);
    char_printf(d->character, "this is not a functional system yet sorry\n");
}

/*
 * Ask for liquid type.
 */
#define TYPE_INDEX ((NUM_LIQ_TYPES / columns + 1) * j + i)
void oedit_liquid_type(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif

    for (i = 0; i <= NUM_LIQ_TYPES / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j)
            if (TYPE_INDEX < NUM_LIQ_TYPES)
                sprintf(buf, "%s%s%2d%s) %s%-20.20s%s ", buf, grn, TYPE_INDEX + 1, nrm, yel, LIQ_NAME(TYPE_INDEX), nrm);
        char_printf(d->character, strcat(buf, "\n"));
    }

    sprintf(buf, "\n%sEnter drink type:\n", nrm);
    char_printf(d->character, buf);
    OLC_MODE(d) = OEDIT_VALUE_3;
}

#undef TYPE_INDEX

/*
 * The actual apply to set.
 */
#define TYPE_INDEX ((NUM_APPLY_TYPES / columns + 1) * j + i)
void oedit_disp_apply_menu(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif

    for (i = 0; i <= NUM_APPLY_TYPES / columns; ++i) {
        std::string output;
        for (j = 0; j < columns; ++j)
            if (TYPE_INDEX < NUM_APPLY_TYPES)
                output += fmt::format("{}{:2d}{}) {:20.20s} ", grn, TYPE_INDEX + 1, nrm, apply_types[TYPE_INDEX]);
        output += "\n";
        char_printf(d->character, output);
    }

    char_printf(d->character, "\nEnter apply type (0 is no apply):\n");
    OLC_MODE(d) = OEDIT_APPLY;
}

#undef TYPE_INDEX

/*
 * Weapon type.
 */
#define TYPE_INDEX ((NUM_ATTACK_TYPES / columns + 1) * j + i)
void oedit_disp_weapon_menu(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif

    for (i = 0; i <= NUM_ATTACK_TYPES / columns; ++i) {
        std::string output;
        for (j = 0; j < columns; ++j)
            if (TYPE_INDEX < NUM_ATTACK_TYPES)
                output += fmt::format("{}{:2d}{}) {:20.20s} ", grn, TYPE_INDEX + 1, nrm,
                                      attack_hit_text[TYPE_INDEX].singular);
        output += "\n";
        char_printf(d->character, output);
    }

    char_printf(d->character, "\nEnter weapon type:\n");
}

#undef TYPE_INDEX

/*
 * Spell type.
 */
void oedit_disp_spells_menu(DescriptorData *d) {
    int counter, columns = 0;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "^[[H^[[J");
#endif
    /* Fixed to use all spells --gurlaek 7/22/1999 */
    for (counter = 0; counter <= MAX_SPELLS; counter++) {
        if (strcasecmp(skills[counter].name, "!UNUSED!")) {
            char_printf(d->character, "{}{:2d}{}) {}{:20.20s} {}", grn, counter, nrm, yel, skills[counter].name,
                        !(++columns % 3) ? "\n" : "");
        }
    }
    char_printf(d->character, "\n{}Enter spell choice (0 for none):\n", nrm);
}

void oedit_disp_portal_messages_menu(DescriptorData *d, const std::string_view messages[]) {
    int i = 0;

    while (messages[i].front() != '\n') {
        char_printf(d->character, "{}{}{}) {}", grn, i, nrm, messages[i]);
        ++i;
    }
}

void oedit_disp_boards(DescriptorData *d) {
    BoardIter *iter = board_iterator();
    const BoardData *board;
    int i = 1;

    while ((board = next_board(iter)))
        desc_printf(d, "{}{}{}) {}\n", grn, i++, nrm, board->title);

    free_board_iterator(iter);

    desc_printf(d, "\n{}Enter board choice:\n", nrm);
}

/*
 * Object value #1
 */
void oedit_disp_val1_menu(DescriptorData *d) {
    OLC_MODE(d) = OEDIT_VALUE_1;
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_LIGHT:
        /* values 0 and 1 are unused.. jump to 2 */
        oedit_disp_val3_menu(d);
        break;
    case ITEM_SCROLL:
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_POTION:
    case ITEM_INSTRUMENT:
        char_printf(d->character, "Spell level:\n");
        break;
    case ITEM_WEAPON:
        /* this seems to be a circleism.. not part of normal diku? */
        char_printf(d->character, "Modifier to Hitroll:\n");
        break;
    case ITEM_ARMOR:
    case ITEM_TREASURE:
        char_printf(d->character, "Apply to AC:\n");
        break;
    case ITEM_CONTAINER:
        char_printf(d->character, "Max weight to contain:\n");
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        char_printf(d->character, "Capacity (ounces):\n");
        break;
    case ITEM_FOOD:
        char_printf(d->character, "Hours to fill stomach:\n");
        break;
    case ITEM_SPELLBOOK:
        char_printf(d->character, "Pages in spellbook:\n");
        break;
    case ITEM_MONEY:
        desc_printf(d, "Number of {} coins:\n", COIN_NAME(PLATINUM));
        break;
    case ITEM_PORTAL:
        char_printf(d->character, "Room to go to:\n");
        break;
    case ITEM_WALL:
        oedit_disp_wall_block_dirs(d);
        break;
    case ITEM_BOARD:
        if (board_count())
            oedit_disp_boards(d);
        else
            desc_printf(d, "No boards defined.  (Hit enter.)\n");
        break;
    default:
        limit_obj_values(OLC_OBJ(d));
        oedit_disp_menu(d);
    }
}

/*
 * Object value #2
 */
void oedit_disp_val2_menu(DescriptorData *d) {
    OLC_MODE(d) = OEDIT_VALUE_2;
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
        oedit_disp_spells_menu(d);
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_INSTRUMENT:
        char_printf(d->character, "Max number of charges:\n");
        break;
    case ITEM_WEAPON:
        char_printf(d->character, "Number of damage dice:\n");
        break;
    case ITEM_FOOD:
        /* values 2 and 3 are unused, jump to 4. how odd */
        oedit_disp_val4_menu(d);
        break;
    case ITEM_CONTAINER:
        /* these are flags, needs a bit of special handling */
        oedit_disp_container_flags_menu(d);
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        char_printf(d->character, "Initial contents (ounces):\n");
        break;
    case ITEM_MONEY:
        desc_printf(d, "Number of {} coins:\n", COIN_NAME(GOLD));
        break;
    case ITEM_PORTAL:
        oedit_disp_portal_messages_menu(d, portal_entry_messages);
        char_printf(d->character, "Portal entry message to original room:\n");
        break;
    case ITEM_WALL:
        char_printf(d->character, "Does wall crumble on dispel magic? (0)No (1)Yes:\n");
        break;
    default:
        limit_obj_values(OLC_OBJ(d));
        oedit_disp_menu(d);
    }
}

/*
 * Object value #3
 */
void oedit_disp_val3_menu(DescriptorData *d) {
    OLC_MODE(d) = OEDIT_VALUE_3;
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_LIGHT:
        char_printf(d->character, "Number of hours (0 = burnt, -1 is infinite):\n");
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
        oedit_disp_spells_menu(d);
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_INSTRUMENT:
        char_printf(d->character, "Number of charges remaining:\n");
        break;
    case ITEM_WEAPON:
        char_printf(d->character, "Size of damage dice:\n");
        break;
    case ITEM_CONTAINER:
        char_printf(d->character, "Vnum of key to open container (-1 for no key):\n");
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        oedit_liquid_type(d);
        break;
    case ITEM_MONEY:
        desc_printf(d, "Number of {} coins:\n", COIN_NAME(SILVER));
        break;
    case ITEM_PORTAL:
        oedit_disp_portal_messages_menu(d, portal_character_messages);
        char_printf(d->character, "Portal entry message to character:\n");
        break;
    case ITEM_WALL:
        char_printf(d->character, "Wall Hit Points:\n");
        break;
    default:
        limit_obj_values(OLC_OBJ(d));
        oedit_disp_menu(d);
    }
}

/*
 * Object value #4
 */
void oedit_disp_val4_menu(DescriptorData *d) {
    OLC_MODE(d) = OEDIT_VALUE_4;
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_INSTRUMENT:
        oedit_disp_spells_menu(d);
        break;
    case ITEM_WEAPON:
        oedit_disp_weapon_menu(d);
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
        char_printf(d->character, "Poisoned (0 = not poison):\n");
        break;
    case ITEM_MONEY:
        desc_printf(d, "Number of {} coins : ", COIN_NAME(COPPER));
        break;
    case ITEM_PORTAL:
        oedit_disp_portal_messages_menu(d, portal_exit_messages);
        char_printf(d->character, "Portal exit message to target room:\n");
        break;
    default:
        limit_obj_values(OLC_OBJ(d));
        oedit_disp_menu(d);
    }
}

/*
 * Object type.
 */
#define FLAG_INDEX (((NUM_ITEM_TYPES - 1) / columns + 1) * j + i)
void oedit_disp_type_menu(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif

    for (i = 0; i <= (NUM_ITEM_TYPES - 1) / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j)
            if (FLAG_INDEX < NUM_ITEM_TYPES - 1)
                sprintf(buf, "%s%s%2d%s) %-20.20s ", buf, grn, FLAG_INDEX + 1, nrm, item_types[FLAG_INDEX + 1].name);
        char_printf(d->character, strcat(buf, "\n"));
    }
    char_printf(d->character, "\nEnter object type:\n");
}

#undef FLAG_INDEX

/*
 * Object extra flags.
 */
#define FLAG_INDEX ((NUM_ITEM_FLAGS / columns + 1) * j + i)
void oedit_disp_extra_menu(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif

    for (i = 0; i <= NUM_ITEM_FLAGS / columns; ++i) {
        std::string output;
        for (j = 0; j < columns; ++j)
            if (FLAG_INDEX < NUM_ITEM_FLAGS)
                output += fmt::format("{}{:2d}{}) {:20.20s} ", grn, FLAG_INDEX + 1, nrm, extra_bits[FLAG_INDEX]);
        output += "\n";
        char_printf(d->character, output);
    }

    char_printf(d->character,
                "\nObject flags: {}{}{}\n"
                "Enter object extra flag (0 to quit):\n",
                cyn, sprintflag(GET_OBJ_FLAGS(OLC_OBJ(d)), NUM_ITEM_FLAGS, extra_bits), nrm);
}

#undef FLAG_INDEX

/*
 * Object wear flags.
 */
#define FLAG_INDEX ((NUM_ITEM_WEAR_FLAGS / columns + 1) * j + i)
void oedit_disp_wear_menu(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif
    for (i = 0; i <= NUM_ITEM_WEAR_FLAGS / columns; ++i) {
        std::string output;
        for (j = 0; j < columns; ++j)
            if (FLAG_INDEX < NUM_ITEM_WEAR_FLAGS)
                output += fmt::format("{}{:2d}{}) {:20.20s} ", grn, FLAG_INDEX + 1, nrm, wear_bits[FLAG_INDEX]);
        output += "\n";
        char_printf(d->character, output);
    }

    char_printf(d->character,
                "\nWear flags: {}{}{}\n"
                "Enter wear flag, 0 to quit:\n",
                cyn, sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits), nrm);
}

#undef FLAG_INDEX

void oedit_disp_obj_values(DescriptorData *d) {
    ObjData *obj = OLC_OBJ(d);
    int i;

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
        if (GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING) == LIGHT_PERMANENT)
            sprintf(buf, "         Hours : %sInfinite%s\n", cyn, nrm);
        else
            sprintf(buf, "         Hours : %s%d%s\n", cyn, GET_OBJ_VAL(obj, VAL_LIGHT_REMAINING), nrm);
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
        sprintf(buf,
                "   Spell Level : %s%d%s\n"
                "        Spells : %s%s, %s, %s%s\n",
                cyn, GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL), nrm, cyn, skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_1)),
                skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_2)), skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL_3)),
                nrm);
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_INSTRUMENT:
        sprintf(buf,
                "         Spell : %s%s%s\n"
                "   Spell Level : %s%d%s\n"
                "       Charges : %s%d remaining of %d max%s\n",
                cyn, skill_name(GET_OBJ_VAL(obj, VAL_WAND_SPELL)), nrm, cyn, GET_OBJ_VAL(obj, VAL_WAND_LEVEL), nrm, cyn,
                GET_OBJ_VAL(obj, VAL_WAND_CHARGES_LEFT), GET_OBJ_VAL(obj, VAL_WAND_MAX_CHARGES), nrm);
        break;
    case ITEM_WEAPON:
        sprintf(buf,
                "   Damage Dice : %s%dd%d%s\n"
                "       Message : %s%s%s\n",
                cyn, GET_OBJ_VAL(obj, VAL_WEAPON_DICE_NUM), GET_OBJ_VAL(obj, VAL_WEAPON_DICE_SIZE), nrm, cyn,
                GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) >= 0 &&
                        GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE) <= TYPE_ALIGN - TYPE_HIT
                    ? attack_hit_text[GET_OBJ_VAL(obj, VAL_WEAPON_DAM_TYPE)].singular
                    : "INVALID",
                nrm);
        break;
    case ITEM_ARMOR:
    case ITEM_TREASURE:
        sprintf(buf, "      AC-Apply : %s%d%s\n", cyn, GET_OBJ_VAL(obj, VAL_ARMOR_AC), nrm);
        break;
    case ITEM_TRAP:
        sprintf(buf,
                "         Spell : %s%s%s\n"
                "     Hitpoints : %s%d%s\n",
                cyn, skill_name(GET_OBJ_VAL(obj, VAL_TRAP_SPELL)), nrm, cyn, GET_OBJ_VAL(obj, VAL_TRAP_HITPOINTS), nrm);
        break;
    case ITEM_CONTAINER:
        sprintf(buf, "      Capacity : %s%d%s\n", cyn, GET_OBJ_VAL(obj, VAL_CONTAINER_CAPACITY), nrm);
        break;
    case ITEM_NOTE:
        sprintf(buf, "        Tongue : %s%d%s\n", cyn, GET_OBJ_VAL(obj, 0), nrm);
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        sprintf(buf,
                "      Capacity : %s%d oz%s\n"
                "      Contains : %s%d oz%s\n"
                "      Poisoned : %s%s%s\n"
                "        Liquid : %s%s%s\n",
                cyn, GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY), nrm, cyn, GET_OBJ_VAL(obj, VAL_DRINKCON_REMAINING), nrm,
                cyn, YESNO(IS_POISONED(obj)), nrm, cyn, LIQ_NAME(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)), nrm);
        break;
    case ITEM_FOOD:
        sprintf(buf,
                "    Makes full : %s%d hours%s\n"
                "      Poisoned : %s%s%s\n",
                cyn, GET_OBJ_VAL(obj, VAL_FOOD_FILLINGNESS), nrm, cyn, YESNO(IS_POISONED(obj)), nrm);
        break;
    case ITEM_MONEY:
        sprintf(buf, "         Coins : %s%dp %dg %ds %dc%s\n", cyn, GET_OBJ_VAL(obj, VAL_MONEY_PLATINUM),
                GET_OBJ_VAL(obj, VAL_MONEY_GOLD), GET_OBJ_VAL(obj, VAL_MONEY_SILVER),
                GET_OBJ_VAL(obj, VAL_MONEY_COPPER), nrm);
        break;
    case ITEM_PORTAL: {
        int room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DESTINATION));

        auto buffer = fmt::format(
            "   Target Room : {}{} ({}){}\n"
            " Entry Message : {}{}{}"
            "  Char Message : {}{}{}"
            "  Exit Message : {}{}{}\n",
            cyn, room == NOWHERE ? "Invalid Room" : world[room].name, GET_OBJ_VAL(obj, VAL_PORTAL_DESTINATION), nrm,
            cyn, sprinttype(GET_OBJ_VAL(obj, VAL_PORTAL_ENTRY_MSG), portal_entry_messages), nrm, cyn,
            sprinttype(GET_OBJ_VAL(obj, VAL_PORTAL_CHAR_MSG), portal_character_messages), nrm, cyn,
            sprinttype(GET_OBJ_VAL(obj, VAL_PORTAL_EXIT_MSG), portal_exit_messages), nrm);
        strcpy(buf, buffer.c_str());
    } break;
    case ITEM_SPELLBOOK:
        sprintf(buf, "         Pages : %s%d%s\n", cyn, GET_OBJ_VAL(obj, VAL_SPELLBOOK_PAGES), nrm);
        break;
    case ITEM_WALL: {
        auto output = fmt::format(
            "     Direction : {}{}{}\n"
            "    Dispelable : {}{}{}\n"
            "     Hitpoints : {}{}{}\n",
            cyn, sprinttype(GET_OBJ_VAL(obj, VAL_WALL_DIRECTION), dirs), nrm, cyn,
            YESNO(GET_OBJ_VAL(obj, VAL_WALL_DISPELABLE)), nrm, cyn, GET_OBJ_VAL(obj, VAL_WALL_HITPOINTS), nrm);
        strcpy(buf, output.c_str());
    } break;
    case ITEM_BOARD:
        sprintf(buf, "   Board Title : %s%s%s\n", cyn, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER))->title, nrm);
        break;
    default:
        return;
    }
    char_printf(d->character, buf);
}

/*
 * Display main menu.
 */
void oedit_disp_menu(DescriptorData *d) {
    ObjData *obj;

    obj = OLC_OBJ(d);
    get_char_cols(d->character);

    /*
     * Build first half of menu.
     */
    char_printf(d->character,
#if defined(CLEAR_SCREEN)
                ".[H.[J"
#endif
                "-- Item: '&5{}&0'  vnum: [&2{:5d}&0]\n"
                "{}1{}) Namelist : {}{}\n"
                "{}2{}) S-Desc   : {}{}\n"
                "{}3{}) L-Desc   :-\n{}{}\n"
                "{}4{}) A-Desc   :-\n{}{}"
                "{}5{}) Type        : {}{}\n"
                "{}6{}) Extra flags : {}{}\n",
                (obj->short_description && *obj->short_description) ? obj->short_description : "undefined", OLC_NUM(d),
                grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined", grn, nrm, yel,
                (obj->short_description && *obj->short_description) ? obj->short_description : "undefined", grn, nrm,
                yel, (obj->description && *obj->description) ? obj->description : "undefined", grn, nrm, yel,
                (obj->action_description && *obj->action_description) ? obj->action_description : "<not set>\n", grn,
                nrm, cyn, OBJ_TYPE_NAME(obj), grn, nrm, cyn,
                sprintflag(GET_OBJ_FLAGS(obj), NUM_ITEM_FLAGS, extra_bits));
    /*
     * Send first half.
     */

    /*. Build second half of menu . */

    char_printf(d->character,
                "{}7{}) Wear flags  : {}{}\n"
                "{}8{}) Weight      : {}{:.2f}\n"
                "{}9{}) Cost        : {}{}\n"
                "{}A{}) Timer       : {}{}\n"
                "{}B{}) Level       : {}{}\n"
                "{}C{}) Hiddenness  : {}{}\n"
                "{}D{}) Values      : {}{} {} {} {} {} {} {}{}\n",
                grn, nrm, cyn, sprintbit(GET_OBJ_WEAR(obj), wear_bits), grn, nrm, cyn, GET_OBJ_WEIGHT(obj), grn, nrm,
                cyn, GET_OBJ_COST(obj), grn, nrm, cyn, GET_OBJ_TIMER(obj), grn, nrm, cyn, GET_OBJ_LEVEL(obj), grn, nrm,
                cyn, GET_OBJ_HIDDENNESS(obj), grn, nrm, cyn, GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
                GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6),
                nrm);

    oedit_disp_obj_values(d);

    char_printf(d->character,
                "{}E{}) Applies menu\n"
                "{}F{}) Extra descriptions menu\n"
                "{}G{}) Spell applies : &6{}&0\n"
                "{}S{}) Script      : {}{}\n"
                "{}Q{}) Quit\n"
                "Enter choice:\n",
                grn, nrm, grn, nrm, grn, nrm, sprintflag(GET_OBJ_EFF_FLAGS(obj), NUM_EFF_FLAGS, effect_flags), grn, nrm,
                cyn, obj->proto_script ? "Set." : "Not Set.", grn, nrm);
    OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 *  main loop (of sorts).. basically interpreter throws all input to here  *
 ***************************************************************************/

void oedit_parse(DescriptorData *d, char *arg) {
    int number = atoi(arg);
    float fnum = atof(arg);

    switch (OLC_MODE(d)) {

    case OEDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
        case 'y':
        case 'Y':
            if (STATE(d) == CON_IEDIT) {
                string_to_output(d, "Saving changes to object.\n");
                iedit_save_changes(d);
                log(LogSeverity::Debug, std::max(LVL_GOD, GET_INVIS_LEV(d->character)), "OLC: {} edits unique obj {}",
                    GET_NAME(d->character), OLC_IOBJ(d)->short_description);
                REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
                STATE(d) = CON_PLAYING;
            } else {
                char_printf(d->character, "Saving object to memory.\n");
                oedit_save_internally(d);
                log(LogSeverity::Debug, std::max(LVL_GOD, GET_INVIS_LEV(d->character)), "OLC: {} edits obj {}",
                    GET_NAME(d->character), OLC_NUM(d));
            }
            cleanup_olc(d, CLEANUP_STRUCTS);
            return;
        case 'n':
        case 'N':
            /*. Cleanup all . */
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            char_printf(d->character, "Invalid choice!\n");
            char_printf(d->character, "Do you wish to save this object internally?\n");
            return;
        }

    case OEDIT_MAIN_MENU:
        /* throw us out to whichever edit mode based on user input */
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) { /* Something has been modified. */
                char_printf(d->character, "Do you wish to save this object internally?\n");
                OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '1':
            char_printf(d->character, "Enter namelist:\n");
            OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
            break;
        case '2':
            char_printf(d->character, "Enter short desc:\n");
            OLC_MODE(d) = OEDIT_SHORTDESC;
            break;
        case '3':
            char_printf(d->character, "Enter long desc :\n");
            OLC_MODE(d) = OEDIT_LONGDESC;
            break;
        case '4':
            OLC_MODE(d) = OEDIT_ACTDESC;
            string_to_output(d, "Enter action description: (/s saves /h for help)\n\n");
            string_write(d, &OLC_OBJ(d)->action_description, MAX_DESC_LENGTH);
            OLC_VAL(d) = 1;
            break;
        case '5':
            oedit_disp_type_menu(d);
            OLC_MODE(d) = OEDIT_TYPE;
            break;
        case '6':
            oedit_disp_extra_menu(d);
            OLC_MODE(d) = OEDIT_EXTRAS;
            break;
        case '7':
            oedit_disp_wear_menu(d);
            OLC_MODE(d) = OEDIT_WEAR;
            break;
        case '8':
            char_printf(d->character, "Enter weight:\n");
            OLC_MODE(d) = OEDIT_WEIGHT;
            break;
        case '9':
            char_printf(d->character, "Enter cost (copper):\n");
            OLC_MODE(d) = OEDIT_COST;
            break;
        case 'a':
        case 'A':
            char_printf(d->character, "Enter timer:\n");
            OLC_MODE(d) = OEDIT_TIMER;
            break;
        case 'b':
        case 'B':
            char_printf(d->character, "Enter level:\n");
            OLC_MODE(d) = OEDIT_LEVEL;
            break;
        case 'c':
        case 'C':
            char_printf(d->character, "Enter hiddenness:\n");
            OLC_MODE(d) = OEDIT_HIDDENNESS;
            break;
        case 'd':
        case 'D':
            /*
             * Clear any old values
             */
            for (number = 0; number < NUM_VALUES; ++number)
                GET_OBJ_VAL(OLC_OBJ(d), number) = 0;
            oedit_disp_val1_menu(d);
            break;
        case 'e':
        case 'E':
            oedit_disp_prompt_apply_menu(d);
            break;
        case 'f':
        case 'F':
            /*
             * If extra descriptions don't exist.
             */
            if (!OLC_OBJ(d)->ex_description) {
                CREATE(OLC_OBJ(d)->ex_description, ExtraDescriptionData, 1);
                OLC_OBJ(d)->ex_description->next = nullptr;
            }
            OLC_DESC(d) = OLC_OBJ(d)->ex_description;
            oedit_disp_extradesc_menu(d);
            break;
        case 'g':
        case 'G':
            oedit_disp_aff_flags(d);
            OLC_MODE(d) = OEDIT_SPELL_APPLY;
            break;
            /*
                case 'h':
                case 'H':
                  OLC_MODE(d) = OEDIT_SPELL_COMPONENT;
                  oedit_disp_component(d);
                  break;
            */
        case 'p':
        case 'P':
            if (GET_OBJ_RNUM(OLC_OBJ(d)) == NOTHING) {
                char_printf(d->character, "You cannot purge a non-existent (unsaved) objext! Choose again:\n");
            } else if (GET_LEVEL(d->character) < LVL_HEAD_B) {
                sprintf(buf, "You are too low level to purge! Get a level %d or greater to do this.\n", LVL_HEAD_B);
                char_printf(d->character, buf);
            } else {
                OLC_MODE(d) = OEDIT_PURGE_OBJECT;
                /* make extra sure */
                char_printf(d->character, "Purging will also remove all existing objects of this sort!\n");
                char_printf(d->character, "Are you sure you wish to PERMANENTLY DELETE the object? (y/n)\n");
            }
            return;
        case 's':
        case 'S':
            OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
            dg_script_menu(d);
            return;

        default:
            oedit_disp_menu(d);
            break;
        }
        return; /* end of OEDIT_MAIN_MENU */

    case OLC_SCRIPT_EDIT:
        if (dg_script_edit_parse(d, arg))
            return;
        break;
    case OEDIT_EDIT_NAMELIST:
        if (OLC_OBJ(d)->name)
            free(OLC_OBJ(d)->name);
        OLC_OBJ(d)->name = strdup((arg && *arg) ? arg : "undefined");
        break;

    case OEDIT_SHORTDESC:
        if (OLC_OBJ(d)->short_description)
            free(OLC_OBJ(d)->short_description);
        OLC_OBJ(d)->short_description = strdup((arg && *arg) ? arg : "undefined");
        break;

    case OEDIT_LONGDESC:
        if (OLC_OBJ(d)->description)
            free(OLC_OBJ(d)->description);
        OLC_OBJ(d)->description = strdup((arg && *arg) ? arg : "undefined");
        break;

    case OEDIT_TYPE:
        if ((number < 1) || (number > NUM_ITEM_TYPES)) {
            char_printf(d->character, "Invalid choice, try again:\n");
            return;
        }
        GET_OBJ_TYPE(OLC_OBJ(d)) = number;
        limit_obj_values(OLC_OBJ(d));
        break;
    case OEDIT_EXTRAS:
        if ((number < 0) || (number > NUM_ITEM_FLAGS)) {
            oedit_disp_extra_menu(d);
            return;
        } else if (number == 0)
            break;
        else {
            TOGGLE_FLAG(GET_OBJ_FLAGS(OLC_OBJ(d)), number - 1);
            /* This flag shouldn't be on object prototypes */
            REMOVE_FLAG(GET_OBJ_FLAGS(OLC_OBJ(d)), ITEM_WAS_DISARMED);
            oedit_disp_extra_menu(d);
            return;
        }

    case OEDIT_WEAR:
        if ((number < 0) || (number > NUM_ITEM_WEAR_FLAGS)) {
            char_printf(d->character, "That's not a valid choice!\n");
            oedit_disp_wear_menu(d);
            return;
        } else if (number == 0) /* Quit. */
            break;
        else {
            TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1));
            oedit_disp_wear_menu(d);
            return;
        }

    case OEDIT_WEIGHT:
        GET_OBJ_WEIGHT(OLC_OBJ(d)) = fnum;
        break;

    case OEDIT_COST:
        GET_OBJ_COST(OLC_OBJ(d)) = number;
        break;

    case OEDIT_TIMER:
        GET_OBJ_TIMER(OLC_OBJ(d)) = number;
        break;

    case OEDIT_LEVEL:
        GET_OBJ_LEVEL(OLC_OBJ(d)) = number;
        break;

    case OEDIT_HIDDENNESS:
        GET_OBJ_HIDDENNESS(OLC_OBJ(d)) = std::clamp(number, 0, 1000);
        break;

    case OEDIT_VALUE_1:
        /* Range-check values at the very end by calling limit_obj_values */
        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
        OLC_VAL(d) = 1;
        /* Proceed to menu 2. */
        oedit_disp_val2_menu(d);
        return;

    case OEDIT_VALUE_2:
        /* Check for out of range values. */
        switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_SCROLL:
        case ITEM_POTION:
            GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
            if (number < 0 || number > MAX_SPELLS || !strcasecmp(skills[number].name, "!UNUSED!")) {
                oedit_disp_val2_menu(d);
                return;
            }
            break;
        case ITEM_CONTAINER:
            if (number < 0 || number > 4)
                oedit_disp_container_flags_menu(d);
            else if (number != 0) {
                TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), VAL_CONTAINER_BITS), 1 << (number - 1));
                oedit_disp_val2_menu(d);
            } else
                oedit_disp_val3_menu(d);
            return;
        default:
            GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
        }
        OLC_VAL(d) = 1;
        oedit_disp_val3_menu(d);
        return;

    case OEDIT_VALUE_3:
        /*
         * Quick'n'easy error checking.
         */
        switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_SCROLL:
        case ITEM_POTION:
            GET_OBJ_VAL(OLC_OBJ(d), 2) = number;
            if (number < 0 || number > MAX_SPELLS || !strcasecmp(skills[number].name, "!UNUSED!")) {
                oedit_disp_val3_menu(d);
                return;
            }
            break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            --number; /* Types are displayed starting with 1 index */
                      /* fall through to default! */
        default:
            GET_OBJ_VAL(OLC_OBJ(d), 2) = number;
        }
        OLC_VAL(d) = 1;
        oedit_disp_val4_menu(d);
        return;

    case OEDIT_VALUE_4:
        switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_INSTRUMENT:
            GET_OBJ_VAL(OLC_OBJ(d), 3) = number;
            if (number < 0 || number > MAX_SPELLS || !strcasecmp(skills[number].name, "!UNUSED!")) {
                oedit_disp_val4_menu(d);
                return;
            }
            break;
        case ITEM_WEAPON:
            --number; /* Types are displayed starting with 1 index */
                      /* fall through to default! */
        default:
            GET_OBJ_VAL(OLC_OBJ(d), 3) = number;
        }
        OLC_VAL(d) = 1;
        limit_obj_values(OLC_OBJ(d));
        break;

    case OEDIT_PROMPT_APPLY:
        if (number == 0)
            break;
        else if (number < 0 || number > MAX_OBJ_APPLIES) {
            oedit_disp_prompt_apply_menu(d);
            return;
        }
        OLC_VAL(d) = number - 1;
        OLC_MODE(d) = OEDIT_APPLY;
        oedit_disp_apply_menu(d);
        return;

    case OEDIT_APPLY:
        if (number == 8) {
            char_printf(d->character, "I don't think so\n");
            return;
        }
        if (number == 0) {
            OLC_OBJ(d)->applies[OLC_VAL(d)].location = 0;
            OLC_OBJ(d)->applies[OLC_VAL(d)].modifier = 0;
            oedit_disp_prompt_apply_menu(d);
        } else if (number < 0 || number >= NUM_APPLY_TYPES)
            oedit_disp_apply_menu(d);
        else {
            OLC_OBJ(d)->applies[OLC_VAL(d)].location = number;
            if (number == APPLY_COMPOSITION) {
                list_olc_compositions(d->character);
                char_printf(d->character, "Composition:\n");
            } else
                char_printf(d->character, "Modifier:\n");
            OLC_MODE(d) = OEDIT_APPLYMOD;
        }
        return;

    case OEDIT_APPLYMOD:
        if (OLC_OBJ(d)->applies[OLC_VAL(d)].location == APPLY_COMPOSITION &&
            (number < 0 || number >= NUM_COMPOSITIONS)) {
            char_printf(d->character, "Invalid composition!\n");
            list_olc_compositions(d->character);
            char_printf(d->character, "Composition:\n");
            return;
        }
        OLC_OBJ(d)->applies[OLC_VAL(d)].modifier = number;
        oedit_disp_prompt_apply_menu(d);
        return;

    case OEDIT_SPELL_APPLY:
        if (number == 0)
            break;
        else if (number > 0 && number <= NUM_EFF_FLAGS)
            TOGGLE_FLAG(GET_OBJ_EFF_FLAGS(OLC_OBJ(d)), number - 1);
        else
            char_printf(d->character, "That's not a valid choice!\n");
        oedit_disp_aff_flags(d);
        return;

    case OEDIT_EXTRADESC_KEY:
        if (OLC_DESC(d)->keyword)
            free(OLC_DESC(d)->keyword);
        OLC_DESC(d)->keyword = strdup((arg && *arg) ? arg : "undefined");
        oedit_disp_extradesc_menu(d);
        return;

    case OEDIT_EXTRADESC_MENU:
        switch (number) {
        case 0:
            if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
                ExtraDescriptionData **tmp_desc;

                if (OLC_DESC(d)->keyword)
                    free(OLC_DESC(d)->keyword);
                if (OLC_DESC(d)->description)
                    free(OLC_DESC(d)->description);

                /*
                 * Clean up pointers
                 */
                for (tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next)) {
                    if (*tmp_desc == OLC_DESC(d)) {
                        *tmp_desc = nullptr;
                        break;
                    }
                }
                free(OLC_DESC(d));
            }
            break;

        case 1:
            OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
            char_printf(d->character, "Enter keywords, separated by spaces:\n");
            return;

        case 2:
            OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
            string_to_output(d, "Enter the extra description: (/s saves /h for help)\n\n");
            string_write(d, &OLC_DESC(d)->description, MAX_DESC_LENGTH);
            OLC_VAL(d) = 1;
            return;

        case 3:
            /*
             * Only go to the next description if this one is finished.
             */
            if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
                ExtraDescriptionData *new_extra;
                if (OLC_DESC(d)->next)
                    OLC_DESC(d) = OLC_DESC(d)->next;
                else { /* Make new extra description and attach at end. */
                    CREATE(new_extra, ExtraDescriptionData, 1);

                    OLC_DESC(d)->next = new_extra;
                    OLC_DESC(d) = OLC_DESC(d)->next;
                }
            }
            /*. No break - drop into default case . */
        default:
            oedit_disp_extradesc_menu(d);
            return;
        }
        break;
    case OEDIT_PURGE_OBJECT:
        switch (*arg) {
        case 'y':
        case 'Y':
            /*. Splat the object in memory .. */
            char_printf(d->character, "Purging object from memory.\n");

            /*need to remove all existing objects of this type too.. */
            /*ok..we use save internally, but we are purging because of the mode */
            oedit_save_internally(d);
            log(LogSeverity::Debug, std::max(LVL_GOD, GET_INVIS_LEV(d->character)), "OLC: {} PURGES object {:d}",
                GET_NAME(d->character), OLC_NUM(d));
        /* FALL THROUGH */
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            char_printf(d->character, "Invalid choice!\n");
            char_printf(d->character, "Do you wish to purge the object?\n");
            return;
        }
        break;
    default:
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: Reached default case in oedit_parse()!");
        char_printf(d->character, "Oops...\n");
        break;
    }

    /*
     * If we get here, we have changed something.
     */
    OLC_VAL(d) = 1;
    oedit_disp_menu(d);
}

void iedit_setup_existing(DescriptorData *d, ObjData *obj) {
    ObjData *temp;

    /* So there's no way for this obj to get extracted (by point_update
     * for example) */
    REMOVE_FROM_LIST(obj, object_list, next);

    /* free any assigned scripts */
    if (SCRIPT(obj))
        extract_script(SCRIPT(obj));
    SCRIPT(obj) = nullptr;

    CREATE(OLC_OBJ(d), ObjData, 1);

    copy_object(OLC_OBJ(d), obj);

    OLC_IOBJ(d) = obj; /* save reference to real object */
    OLC_VAL(d) = 0;
    OLC_NUM(d) = NOTHING;
    OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
    dg_olc_script_copy(d);
    OLC_OBJ(d)->proto_script = nullptr;
    oedit_disp_menu(d);
}

void iedit_save_changes(DescriptorData *d) {
    ObjData *obj = OLC_IOBJ(d);

    GET_ID(OLC_OBJ(d)) = GET_ID(obj);
    if (GET_OBJ_RNUM(obj) != NOTHING)
        obj_index[GET_OBJ_RNUM(obj)].number--;
    copy_object(obj, OLC_OBJ(d));
    obj->proto_script = OLC_SCRIPT(d);
    GET_OBJ_RNUM(obj) = NOTHING;
}

ACMD(do_iedit) {
    ObjData *obj;
    int i;

    argument = one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Which object do you want to edit?\n");
        return;
    }

    if ((obj = find_obj_in_eq(ch, &i, find_vis_by_name(ch, arg))))
        unequip_char(ch, i);
    else if ((obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))))
        obj_from_char(obj);
    else if ((obj = find_obj_in_list(world[IN_ROOM(ch)].contents, find_vis_by_name(ch, arg))))
        obj_from_room(obj);
    else {
        char_printf(ch, "Object not found.\n");
        return;
    }

    /* Setup OLC */
    CREATE(ch->desc->olc, OLCData, 1);

    SET_FLAG(PLR_FLAGS(ch), PLR_WRITING);
    iedit_setup_existing(ch->desc, obj);

    act("$n starts using OLC.", true, ch, 0, 0, TO_ROOM);

    STATE(ch->desc) = CON_IEDIT;
}

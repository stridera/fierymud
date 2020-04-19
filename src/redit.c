/***************************************************************************
 * $Id: redit.c,v 1.34 2009/03/20 20:19:51 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: redit.c                                        Part of FieryMUD *
 *  Usage:                                                                 *
 *     By: Harvey Gilpin of TwyliteMud                                     *
 * Original author: Levork                                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  Copyright 1996 Harvey Gilpin.                                          *
 ***************************************************************************/

#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "dg_olc.h"
#include "directions.h"
#include "exits.h"
#include "math.h"
#include "modify.h"
#include "olc.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/*------------------------------------------------------------------------*/
/*. External data .*/

extern int r_mortal_start_room;
extern int r_immort_start_room;
extern int r_frozen_start_room;
extern int mortal_start_room;
extern int immort_start_room;
extern int frozen_start_room;

/*------------------------------------------------------------------------*/
/* function protos */

void redit_disp_extradesc_menu(struct descriptor_data *d);
void redit_disp_exit_menu(struct descriptor_data *d);
void redit_disp_exit_flag_menu(struct descriptor_data *d);
void redit_disp_flag_menu(struct descriptor_data *d);
void redit_disp_sector_menu(struct descriptor_data *d);
void redit_disp_menu(struct descriptor_data *d);
void redit_parse(struct descriptor_data *d, char *arg);
void redit_setup_new(struct descriptor_data *d);
void redit_setup_existing(struct descriptor_data *d, int real_num);
void redit_save_to_disk(int zone);
void redit_save_internally(struct descriptor_data *d);
void free_room(struct room_data *room);

/*------------------------------------------------------------------------*/

#define W_EXIT(room, num) (world[(room)].exits[(num)])

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void redit_setup_new(struct descriptor_data *d) {
    CREATE(OLC_ROOM(d), struct room_data, 1);

    OLC_ITEM_TYPE(d) = WLD_TRIGGER;
    OLC_ROOM(d)->name = strdup("An unfinished room");
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
    redit_disp_menu(d);
    OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void redit_setup_existing(struct descriptor_data *d, int real_num) {
    struct room_data *room;
    int counter;

    /*
     * Build a copy of the room for editing.
     */
    CREATE(room, struct room_data, 1);

    *room = world[real_num];
    /*
     * Allocate space for all strings.
     */
    room->name = strdup(world[real_num].name ? world[real_num].name : "undefined");
    room->description = strdup(world[real_num].description ? world[real_num].description : "undefined\r\n");
    /*
     * Exits - We allocate only if necessary.
     */
    for (counter = 0; counter < NUM_OF_DIRS; counter++) {
        if (world[real_num].exits[counter]) {
            room->exits[counter] = create_exit(NOWHERE);
            /*
             * Copy the numbers over.
             */
            *room->exits[counter] = *world[real_num].exits[counter];
            /*
             * Allocate the strings.
             */
            room->exits[counter]->general_description =
                (world[real_num].exits[counter]->general_description
                     ? strdup(world[real_num].exits[counter]->general_description)
                     : NULL);
            room->exits[counter]->keyword =
                (world[real_num].exits[counter]->keyword ? strdup(world[real_num].exits[counter]->keyword) : NULL);
        }
    }

    /*
     * Extra descriptions, if necessary.
     */
    if (world[real_num].ex_description) {
        struct extra_descr_data *this, *temp, *temp2;
        CREATE(temp, struct extra_descr_data, 1);

        room->ex_description = temp;
        for (this = world[real_num].ex_description; this; this = this->next) {
            temp->keyword = (this->keyword ? strdup(this->keyword) : NULL);
            temp->description = (this->description ? strdup(this->description) : NULL);
            if (this->next) {
                CREATE(temp2, struct extra_descr_data, 1);
                temp->next = temp2;
                temp = temp2;
            } else
                temp->next = NULL;
        }
    }

    /*. Attatch room copy to players descriptor . */
    OLC_ROOM(d) = room;
    OLC_VAL(d) = 0;
    OLC_ITEM_TYPE(d) = WLD_TRIGGER;
    dg_olc_script_copy(d);
    redit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

#define ZCMD (zone_table[zone].cmd[cmd_no])

void redit_save_internally(struct descriptor_data *d) {
    int i, j, room_num, found = 0, zone, cmd_no;
    struct room_data *new_world;
    struct char_data *temp_ch;
    struct obj_data *temp_obj;
    struct descriptor_data *dsc;

    room_num = real_room(OLC_NUM(d));
    /*
     * Room exists: move contents over then free and replace it.
     */
    if (room_num > 0) {
        OLC_ROOM(d)->contents = world[room_num].contents;
        OLC_ROOM(d)->people = world[room_num].people;
        free_room(world + room_num);
        if (SCRIPT(&world[room_num]))
            extract_script(SCRIPT(&world[room_num]));
        if (world[room_num].proto_script && world[room_num].proto_script != OLC_SCRIPT(d))
            free_proto_script(&world[room_num].proto_script);
        world[room_num] = *OLC_ROOM(d);
        SCRIPT(&world[room_num]) = NULL;
        world[room_num].proto_script = OLC_SCRIPT(d);
        assign_triggers(&world[room_num], WLD_TRIGGER);
    } else { /* Room doesn't exist, hafta add it. */

        /*
         * First, drop any room triggers/scripts.  A necessary evil
         * to prevent crashes.
         */
        for (i = 0; i <= top_of_world; ++i)
            if (SCRIPT(&world[i])) {
                extract_script(SCRIPT(&world[i]));
                SCRIPT(&world[i]) = NULL;
            }

        CREATE(new_world, struct room_data, top_of_world + 2);

        /*
         * Count through world tables.
         */
        for (i = 0; i <= top_of_world; i++) {
            if (!found) {
                /*
                 * Is this the place?
                 */
                if (world[i].vnum > OLC_NUM(d)) {
                    found = TRUE;
                    new_world[i] = *(OLC_ROOM(d));
                    new_world[i].vnum = OLC_NUM(d);
                    new_world[i].func = NULL;
                    new_world[i].proto_script = OLC_SCRIPT(d);
                    room_num = i;

                    /*
                     * Copy from world to new_world + 1.
                     */
                    new_world[i + 1] = world[i];

                    /*
                     * People in this room must have their numbers moved up one.
                     */
                    for (temp_ch = world[i].people; temp_ch; temp_ch = temp_ch->next_in_room)
                        if (temp_ch->in_room != NOWHERE)
                            temp_ch->in_room = i + 1;

                    /* move objects */
                    for (temp_obj = world[i].contents; temp_obj; temp_obj = temp_obj->next_content)
                        if (temp_obj->in_room != NOWHERE)
                            temp_obj->in_room = i + 1;
                } else /* Not yet placed, copy straight over. */
                    new_world[i] = world[i];
            } else { /* Already been found. */
                /*
                 * People in this room must have their in_rooms moved.
                 */
                for (temp_ch = world[i].people; temp_ch; temp_ch = temp_ch->next_in_room)
                    if (temp_ch->in_room != NOWHERE)
                        temp_ch->in_room = i + 1;
                /*
                 * Move objects too.
                 */
                for (temp_obj = world[i].contents; temp_obj; temp_obj = temp_obj->next_content)
                    if (temp_obj->in_room != -1)
                        temp_obj->in_room = i + 1;

                new_world[i + 1] = world[i];
            }
        }
        if (!found) { /* Still not found, insert at top of table. */
            new_world[i] = *(OLC_ROOM(d));
            new_world[i].vnum = OLC_NUM(d);
            new_world[i].func = NULL;
            new_world[i].proto_script = OLC_SCRIPT(d);
            room_num = i;
        }

        /* copy world table over */
        free(world);
        world = new_world;
        top_of_world++;

        /* Now reattach triggers. */
        for (i = 0; i <= top_of_world; ++i)
            assign_triggers(&world[i], WLD_TRIGGER);

        /*. Update zone table . */
        for (zone = 0; zone <= top_of_zone_table; zone++)
            for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
                switch (ZCMD.command) {
                case 'M':
                case 'O':
                    if (ZCMD.arg3 >= room_num)
                        ZCMD.arg3++;
                    break;
                case 'D':
                case 'R':
                    if (ZCMD.arg1 >= room_num)
                        ZCMD.arg1++;
                case 'G':
                case 'P':
                case 'E':
                case '*':
                    break;
                default:
                    sprintf(buf,
                            "SYSERR:redit.c:redit_save_internally(): Unknown command: %d "
                            "in zone %s.",
                            ZCMD.command, zone_table[zone].name);
                    log(buf);
                    mudlog(buf, BRF, LVL_GOD, TRUE);
                }
        /* update load rooms, to fix creeping load room problem */
        if (room_num <= r_mortal_start_room)
            r_mortal_start_room++;
        if (room_num <= r_immort_start_room)
            r_immort_start_room++;
        if (room_num <= r_frozen_start_room)
            r_frozen_start_room++;

        /*. Update world exits . */
        for (i = 0; i < top_of_world + 1; i++)
            for (j = 0; j < NUM_OF_DIRS; j++)
                if (W_EXIT(i, j))
                    if (W_EXIT(i, j)->to_room >= room_num)
                        W_EXIT(i, j)->to_room++;
        /*
         * Update any rooms being edited.
         */
        for (dsc = descriptor_list; dsc; dsc = dsc->next)
            if (dsc->connected == CON_REDIT)
                for (j = 0; j < NUM_OF_DIRS; j++)
                    if (OLC_ROOM(dsc)->exits[j])
                        if (OLC_ROOM(dsc)->exits[j]->to_room >= room_num)
                            OLC_ROOM(dsc)->exits[j]->to_room++;
    }
    olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ROOM);
}

/*------------------------------------------------------------------------*/

void redit_save_to_disk(int zone_num) {
    int counter, counter2, realcounter;
    FILE *fp;
    struct room_data *room;
    struct extra_descr_data *ex_desc;

    sprintf(buf, "%s/%d.new", WLD_PREFIX, zone_table[zone_num].number);
    if (!(fp = fopen(buf, "w+"))) {
        mudlog("SYSERR: OLC: Cannot open room file!", BRF, LVL_GOD, TRUE);
        return;
    }
    for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++) {
        if ((realcounter = real_room(counter)) >= 0) {
            room = (world + realcounter);

            /*. Remove the '\r\n' sequences from description . */
            strcpy(buf1, room->description ? room->description : "Empty");
            strip_string(buf1);

            /*
             * Forget making a buffer, lets just write the thing now.
             */
            fprintf(fp, "#%d\n%s~\n%s~\n%d %ld %d\n", counter, room->name ? room->name : "undefined", buf1,
                    zone_table[room->zone].number, room->room_flags[0], room->sector_type);

            /*
             * Handle exits.
             */
            for (counter2 = 0; counter2 < NUM_OF_DIRS; counter2++) {
                if (room->exits[counter2]) {
                    int temp_door_flag;

                    /*
                     * Again, strip out the garbage.
                     */
                    if (room->exits[counter2]->general_description) {
                        strcpy(buf1, room->exits[counter2]->general_description);
                        strip_string(buf1);
                    } else
                        *buf1 = 0;

                    /*
                     * Figure out door flag.
                     */
                    if (IS_SET(room->exits[counter2]->exit_info, EX_ISDOOR)) {
                        if (IS_SET(room->exits[counter2]->exit_info, EX_PICKPROOF))
                            temp_door_flag = 2;
                        else
                            temp_door_flag = 1;
                    } else if (IS_SET(room->exits[counter2]->exit_info, EX_DESCRIPT)) {
                        temp_door_flag = 3;
                        room->exits[counter2]->to_room = -1;
                        room->exits[counter2]->key = -1;
                    } else
                        temp_door_flag = 0;

                    /*
                     * Check for keywords.
                     */
                    if (room->exits[counter2]->keyword)
                        strcpy(buf2, room->exits[counter2]->keyword);
                    else
                        *buf2 = '\0';

                    /*
                     * Ok, now wrote output to file.
                     */
                    fprintf(fp, "D%d\n%s~\n%s~\n%d %d %d\n", counter2, buf1, buf2, temp_door_flag,
                            room->exits[counter2]->key,
                            room->exits[counter2]->to_room == -1 ? -1 : world[room->exits[counter2]->to_room].vnum);
                }
            }
            /*
             * Home straight, just deal with extra descriptions.
             */
            if (room->ex_description) {
                for (ex_desc = room->ex_description; ex_desc; ex_desc = ex_desc->next) {
                    strcpy(buf1, ex_desc->description);
                    strip_string(buf1);
                    fprintf(fp, "E\n%s~\n%s~\n", ex_desc->keyword, buf1);
                }
            }
            fprintf(fp, "S\n");
            script_save_to_disk(fp, room, WLD_TRIGGER);
        }
    }
    /* write final line and close */
    fprintf(fp, "$~\n");
    fclose(fp);
    sprintf(buf2, "%s/%d.wld", WLD_PREFIX, zone_table[zone_num].number);
    /*
     * We're fubar'd if we crash between the two lines below.
     */
    remove(buf2);
    rename(buf, buf2);

    olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_ROOM);
}

/*------------------------------------------------------------------------*/

void free_room(struct room_data *room) {
    int i;
    struct extra_descr_data *this, *next;

    if (room->name)
        free(room->name);
    if (room->description)
        free(room->description);

    /*
     * Free exits.
     */
    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (room->exits[i]) {
            if (room->exits[i]->general_description)
                free(room->exits[i]->general_description);
            if (room->exits[i]->keyword)
                free(room->exits[i]->keyword);
            free(room->exits[i]);
        }
    }

    /*
     * Free extra descriptions.
     */
    for (this = room->ex_description; this; this = next) {
        next = this->next;
        if (this->keyword)
            free(this->keyword);
        if (this->description)
            free(this->description);
        free(this);
    }
}

/**************************************************************************
 *                       Menu functions                                   *
 **************************************************************************/

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(struct descriptor_data *d) {
    struct extra_descr_data *extra_desc = OLC_DESC(d);

    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "%s1%s) Keyword: %s%s\r\n"
            "%s2%s) Description:\r\n%s%s\r\n"
            "%s3%s) Goto next description: ",
            grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>", grn, nrm, yel,
            extra_desc->description ? extra_desc->description : "<NONE>", grn, nrm);

    strcat(buf, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
    strcat(buf, "Enter choice (0 to quit) : ");
    send_to_char(buf, d->character);
    OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
void redit_disp_exit_menu(struct descriptor_data *d) {
    /*
     * if exit doesn't exist, alloc/create it
     */
    if (!OLC_EXIT(d)) {
        OLC_EXIT(d) = create_exit(NOWHERE);
    }

    *buf2 = '\0';
    /* weird door handling! */
    if (IS_SET(OLC_EXIT(d)->exit_info, EX_DESCRIPT))
        strcat(buf2, "Description only ");
    if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR)) {
        if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
            strcat(buf2, "Pickproof");
        else
            strcat(buf2, "Is a door");
    } else
        strcat(buf2, "No door");

    get_char_cols(d->character);
    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "%s1%s) Exit to     : %s%d\r\n"
            "%s2%s) Description :-\r\n%s%s\r\n"
            "%s3%s) Door name   : %s%s\r\n"
            "%s4%s) Key         : %s%d\r\n"
            "%s5%s) Door flags  : %s%s\r\n"
            "%s6%s) Purge exit.\r\n"
            "Enter choice, 0 to quit : ",
            grn, nrm, cyn, OLC_EXIT(d)->to_room == NOWHERE ? -1 : world[OLC_EXIT(d)->to_room].vnum, grn, nrm, yel,
            OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>", grn, nrm, yel,
            OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>", grn, nrm, cyn, OLC_EXIT(d)->key, grn, nrm, cyn,
            buf2, grn, nrm);

    send_to_char(buf, d->character);
    OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(struct descriptor_data *d) {
    get_char_cols(d->character);
    sprintf(buf,
            "%s0%s) No door\r\n"
            "%s1%s) Closeable door\r\n"
            "%s2%s) Pickproof\r\n"
            "%s3%s) Description\r\n"
            "Enter choice:\r\n",
            grn, nrm, grn, nrm, grn, nrm, grn, nrm);
    send_to_char(buf, d->character);
}

/*
 * For room flags.
 */
#define FLAG_INDEX ((NUM_ROOM_FLAGS / columns + 1) * j + i)
void redit_disp_flag_menu(struct descriptor_data *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif

    for (i = 0; i <= NUM_ROOM_FLAGS / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j)
            if (FLAG_INDEX < NUM_ROOM_FLAGS)
                sprintf(buf, "%s%s%2d%s) %-20.20s ", buf, grn, FLAG_INDEX + 1, nrm, room_bits[FLAG_INDEX]);
        send_to_char(strcat(buf, "\r\n"), d->character);
    }

    sprintflag(buf1, OLC_ROOM(d)->room_flags, NUM_ROOM_FLAGS, room_bits);
    sprintf(buf,
            "\r\nRoom flags: %s%s%s\r\n"
            "Enter room flags, 0 to quit : ",
            cyn, buf1, nrm);
    send_to_char(buf, d->character);
    OLC_MODE(d) = REDIT_FLAGS;
}

#undef FLAG_INDEX

/*
 * For sector type.
 */
#define TYPE_INDEX ((NUM_SECTORS / columns + 1) * j + i)
void redit_disp_sector_menu(struct descriptor_data *d) {
    const int columns = 3;
    int i, j;

#if defined(CLEAR_SCREEN)
    send_to_char("[H[J", d->character);
#endif

    for (i = 0; i < NUM_SECTORS / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j)
            if (TYPE_INDEX < NUM_SECTORS)
                sprintf(buf, "%s%s%2d%s) %-20.20s ", buf, grn, TYPE_INDEX + 1, nrm, sectors[TYPE_INDEX].name);
        send_to_char(strcat(buf, "\r\n"), d->character);
    }

    send_to_char("\r\nEnter sector type, 0 to cancel : ", d->character);
    OLC_MODE(d) = REDIT_SECTOR;
}

#undef TYPE_INDEX

/*
 * The main menu.
 */
void redit_disp_menu(struct descriptor_data *d) {
    struct room_data *room;

    get_char_cols(d->character);
    room = OLC_ROOM(d);

    sprintflag(buf1, room->room_flags, NUM_ROOM_FLAGS, room_bits);
    sprintf(buf2, "%s", sectors[room->sector_type].name);
    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "-- Room: '&5%s&0'  vnum: [&2%5d&0]\r\n"
            "%s1%s) Name        : %s%s\r\n"
            "%s2%s) Description :\r\n%s%s"
            "%s3%s) Room flags  : %s%s\r\n"
            "%s4%s) Sector type : %s%s\r\n",
            room->name, OLC_NUM(d), grn, nrm, yel, room->name, grn, nrm, yel, room->description, grn, nrm, cyn, buf1,
            grn, nrm, cyn, buf2);

    sprintf(buf, "%s%s5%s) Exit north  : %s%s\r\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[NORTH]));

    sprintf(buf, "%s%s6%s) Exit east   : %s%s\r\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[EAST]));
    sprintf(buf, "%s%s7%s) Exit south  : %s%s\r\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[SOUTH]));
    sprintf(buf, "%s%s8%s) Exit west   : %s%s\r\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[WEST]));
    sprintf(buf, "%s%s9%s) Exit up     : %s%s\r\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[UP]));
    sprintf(buf, "%s%sA%s) Exit down   : %s%s\r\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[DOWN]));

    sprintf(buf,
            "%s%sF%s) Extra descriptions menu\r\n"
            "%sS%s) Script      : %s%s\r\n"
            "%sQ%s) Quit\r\n"
            "Enter choice:\r\n",
            buf, grn, nrm, grn, nrm, cyn, room->proto_script ? "Set." : "Not Set.", grn, nrm);
    send_to_char(buf, d->character);

    OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
 *                        The main loop                                   *
 **************************************************************************/

void redit_parse(struct descriptor_data *d, char *arg) {
    int number;

    switch (OLC_MODE(d)) {
    case REDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
        case 'y':
        case 'Y':
            redit_save_internally(d);
            sprintf(buf, "OLC: %s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
            mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
            /*
             * Do NOT free strings! Just the room structure.
             */
            cleanup_olc(d, CLEANUP_STRUCTS);
            send_to_char("Room saved to memory.\r\n", d->character);
            break;
        case 'n':
        case 'N':
            /* free everything up, including strings etc */
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            send_to_char("Invalid choice!\r\nDo you wish to save this room internally?\r\n", d->character);
            break;
        }
        return;

    case REDIT_MAIN_MENU:
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) { /* Something has been modified. */
                send_to_char("Do you wish to save this room internally?\r\n", d->character);
                OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '1':
            send_to_char("Enter room name:]\r\n", d->character);
            OLC_MODE(d) = REDIT_NAME;
            break;
        case '2':
            OLC_MODE(d) = REDIT_DESC;
#if defined(CLEAR_SCREEN)
            write_to_output("\x1B[H\x1B[J", d);
#endif
            write_to_output("Enter room description: (/s saves /h for help)\r\n\r\n", d);
            string_write(d, &OLC_ROOM(d)->description, MAX_ROOM_DESC);
            OLC_VAL(d) = 1;
            break;
        case '3':
            redit_disp_flag_menu(d);
            break;
        case '4':
            redit_disp_sector_menu(d);
            break;
        case '5':
            OLC_VAL(d) = NORTH;
            redit_disp_exit_menu(d);
            break;
        case '6':
            OLC_VAL(d) = EAST;
            redit_disp_exit_menu(d);
            break;
        case '7':
            OLC_VAL(d) = SOUTH;
            redit_disp_exit_menu(d);
            break;
        case '8':
            OLC_VAL(d) = WEST;
            redit_disp_exit_menu(d);
            break;
        case '9':
            OLC_VAL(d) = UP;
            redit_disp_exit_menu(d);
            break;
        case 'a':
        case 'A':
            OLC_VAL(d) = DOWN;
            redit_disp_exit_menu(d);
            break;

        case 'f':
        case 'F':
            /* if extra desc doesn't exist . */
            if (!OLC_ROOM(d)->ex_description) {
                CREATE(OLC_ROOM(d)->ex_description, struct extra_descr_data, 1);
                OLC_ROOM(d)->ex_description->next = NULL;
            }
            OLC_DESC(d) = OLC_ROOM(d)->ex_description;
            redit_disp_extradesc_menu(d);
            break;
        case 's':
        case 'S':
            OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
            dg_script_menu(d);
            return;
        default:
            send_to_char("Invalid choice!\r\n", d->character);
            redit_disp_menu(d);
            break;
        }
        return;

    case OLC_SCRIPT_EDIT:
        if (dg_script_edit_parse(d, arg))
            return;
        break;
    case REDIT_NAME:
        if (OLC_ROOM(d)->name)
            free(OLC_ROOM(d)->name);
        if (strlen(arg) > MAX_ROOM_NAME)
            arg[MAX_ROOM_NAME - 1] = '\0';
        OLC_ROOM(d)->name = strdup((arg && *arg) ? arg : "undefined");
        break;
    case REDIT_DESC:
        /*
         * We will NEVER get here, we hope.
         */
        mudlog("SYSERR: Reached REDIT_DESC case in parse_redit", BRF, LVL_GOD, TRUE);
        break;

    case REDIT_FLAGS:
        number = atoi(arg);
        if ((number < 0) || (number > NUM_ROOM_FLAGS)) {
            send_to_char("That is not a valid choice!\r\n", d->character);
            redit_disp_flag_menu(d);
        } else if (number == 0)
            break;
        else {
            /*
             * Toggle the bit.
             */
            TOGGLE_FLAG(OLC_ROOM(d)->room_flags, number - 1);
            redit_disp_flag_menu(d);
        }
        return;

    case REDIT_SECTOR:
        number = atoi(arg);
        if (number < 0 || number > NUM_SECTORS) {
            send_to_char("Invalid choice!\r\n", d->character);
            redit_disp_sector_menu(d);
            return;
        } else if (number != 0)
            OLC_ROOM(d)->sector_type = number - 1;
        break;

    case REDIT_EXIT_MENU:
        switch (*arg) {
        case '0':
            break;
        case '1':
            OLC_MODE(d) = REDIT_EXIT_NUMBER;
            send_to_char("Exit to room number:\r\n", d->character);
            return;
        case '2':
            OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
            write_to_output("Enter exit description: (/s saves /h for help)\r\n\r\n", d);
            string_write(d, &OLC_EXIT(d)->general_description, MAX_EXIT_DESC);
            return;
        case '3':
            OLC_MODE(d) = REDIT_EXIT_KEYWORD;
            send_to_char("Enter keywords:\r\n", d->character);
            return;
        case '4':
            OLC_MODE(d) = REDIT_EXIT_KEY;
            send_to_char("Enter key number:\r\n", d->character);
            return;
        case '5':
            redit_disp_exit_flag_menu(d);
            OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
            return;
        case '6':
            /* delete exit */
            if (OLC_EXIT(d)->keyword)
                free(OLC_EXIT(d)->keyword);
            if (OLC_EXIT(d)->general_description)
                free(OLC_EXIT(d)->general_description);
            if (OLC_EXIT(d))
                free(OLC_EXIT(d));
            OLC_EXIT(d) = NULL;
            break;
        default:
            send_to_char("Try again:\r\n", d->character);
            return;
        }
        break;

    case REDIT_EXIT_NUMBER:
        if ((number = atoi(arg)) != -1)
            if ((number = real_room(number)) < 0) {
                send_to_char("That room does not exist.  Try again:\r\n", d->character);
                return;
            }
        OLC_EXIT(d)->to_room = number;
        redit_disp_exit_menu(d);
        return;

    case REDIT_EXIT_DESCRIPTION:
        /* we should NEVER get here */
        mudlog("SYSERR: Reached REDIT_EXIT_DESC case in parse_redit", BRF, LVL_GOD, TRUE);
        break;

    case REDIT_EXIT_KEYWORD:
        if (OLC_EXIT(d)->keyword)
            free(OLC_EXIT(d)->keyword);
        OLC_EXIT(d)->keyword = ((arg && *arg) ? strdup(arg) : NULL);
        redit_disp_exit_menu(d);
        return;

    case REDIT_EXIT_KEY:
        OLC_EXIT(d)->key = atoi(arg);
        redit_disp_exit_menu(d);
        return;

    case REDIT_EXIT_DOORFLAGS:
        number = atoi(arg);
        if ((number < 0) || (number > 3)) {
            send_to_char("That's not a valid choice!\r\n", d->character);
            redit_disp_exit_flag_menu(d);
        } else {
            /*
             * Doors are a bit idiotic, don't you think? :) I agree.
             */
            if (number == 1)
                OLC_EXIT(d)->exit_info = EX_ISDOOR;
            else if (number == 2)
                OLC_EXIT(d)->exit_info = EX_ISDOOR | EX_PICKPROOF;
            else if (number == 3)
                OLC_EXIT(d)->exit_info = EX_DESCRIPT;
            else
                OLC_EXIT(d)->exit_info = 0;

            /*   OLC_EXIT(d)->exit_info = (number == 0 ? 0 :
               (number == 1 ? EX_ISDOOR :
               (number == 2 ? EX_ISDOOR | EX_PICKPROOF :
               (number == 3 ? EX_DESCRIPT : 0)))); */
            /*
             * Jump back to the menu system.
             */
            redit_disp_exit_menu(d);
        }
        return;

    case REDIT_EXTRADESC_KEY:
        OLC_DESC(d)->keyword = ((arg && *arg) ? strdup(arg) : NULL);
        redit_disp_extradesc_menu(d);
        return;

    case REDIT_EXTRADESC_MENU:
        switch ((number = atoi(arg))) {
        case 0: {
            /*
             * If something got left out, delete the extra description
             * when backing out to the menu.
             */
            if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
                struct extra_descr_data **tmp_desc;

                if (OLC_DESC(d)->keyword)
                    free(OLC_DESC(d)->keyword);
                if (OLC_DESC(d)->description)
                    free(OLC_DESC(d)->description);

                /*
                 * Clean up pointers.
                 */
                for (tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next))
                    if (*tmp_desc == OLC_DESC(d)) {
                        *tmp_desc = NULL;
                        break;
                    }
                free(OLC_DESC(d));
            }
        } break;
        case 1:
            OLC_MODE(d) = REDIT_EXTRADESC_KEY;
            send_to_char("Enter keywords, separated by spaces:\r\n", d->character);
            return;
        case 2:
            OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
            write_to_output("Enter extra description: (/s saves /h for help)\r\n\r\n", d);
            string_write(d, &OLC_DESC(d)->description, MAX_DESC_LENGTH);
            return;

        case 3:
            if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
                send_to_char("You can't edit the next extra desc without completing this one.\r\n",
                             d->character);
                redit_disp_extradesc_menu(d);
            } else {
                struct extra_descr_data *new_extra;

                if (OLC_DESC(d)->next)
                    OLC_DESC(d) = OLC_DESC(d)->next;
                else {
                    /* make new extra, attach at end */
                    CREATE(new_extra, struct extra_descr_data, 1);
                    OLC_DESC(d)->next = new_extra;
                    OLC_DESC(d) = new_extra;
                }
                redit_disp_extradesc_menu(d);
            }
            return;
        }
        break;

    default:
        /* we should never get here */
        mudlog("SYSERR: Reached default case in parse_redit", BRF, LVL_GOD, TRUE);
        break;
    }
    /*. If we get this far, something has be changed . */
    OLC_VAL(d) = 1;
    redit_disp_menu(d);
}

/***************************************************************************
 * $Log: redit.c,v $
 * Revision 1.34  2009/03/20 20:19:51  myc
 * Removing dependency upon old board system.
 *
 * Revision 1.33  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.32  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.31  2008/08/21 07:10:46  jps
 * Changed the way redit sees exit destinations. It's more informative and less
 *crashy.
 *
 * Revision 1.30  2008/08/19 02:38:15  jps
 * Fix the next bug in room saving...
 *
 * Revision 1.29  2008/08/19 02:34:16  jps
 * Stop crashing when saving description exits.
 *
 * Revision 1.28  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.27  2008/08/10 06:53:55  jps
 * Stop crashing when editing exits in brand new rooms?
 *
 * Revision 1.26  2008/06/11 23:04:34  jps
 * Changed room editing menu a bit
 *
 * Revision 1.25  2008/05/19 06:53:17  jps
 * Got rid of fup and fdown directions.
 *
 * Revision 1.24  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.23  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.22  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.21  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.20  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.19  2008/03/17 16:22:42  myc
 * Fixed handling of proto scripts in OLC, which was causing a crash.
 * Also fixed some memory leaks associated with scripts and OLC.
 * How this got by for so long, I don't know.
 *
 * Revision 1.18  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.17  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.16  2007/11/18 16:51:55  myc
 * Fixing LVL_BUILDER references.
 *
 * Revision 1.15  2007/09/15 05:03:46  myc
 * Implemented a new loop method for some of the menus so that items in
 * the menus get listed column-major instead of by rows.  This applies to
 * the sector and flag menus.
 *
 * Revision 1.14  2007/08/04 01:24:37  myc
 * When saving a new room in redit, all room triggers in the world are now
 * dropped and reloaded (all trigger execution thus stops).  Although
 * inconvenient, this is a necessary evil in order to prevent event-related
 * crashes.
 *
 * Revision 1.13  2007/08/03 22:00:11  myc
 * Fixed some \r\n typoes in send_to_chars.
 *
 * Revision 1.12  2007/07/15 21:16:12  myc
 * No more crash when you edit a script on a new object or room.
 *
 * Revision 1.11  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.10  2001/03/13 00:45:27  dce
 * Keys default to -1 as requested by builders.
 *
 * Revision 1.9  2000/11/24 21:17:12  rsd
 * Altered comment header and added back rlog messgaes from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.8  2000/11/18 20:59:23  jimmy
 * Added sane debug to redit_save_internally
 *
 * Revision 1.7  2000/11/18 06:57:20  rsd
 * changed the comment header and tried to add some
 * sensible debug to redit_save_internal.
 *
 * Revision 1.6  2000/10/14 11:12:40  mtp
 * fixed the olc triggers editting in medit/oedit/redit
 *
 * Revision 1.5  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.4  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.3  1999/04/07 01:20:18  dce
 * Allows extra descriptions on no exits.
 *
 * Revision 1.2  1999/02/01 04:26:53  mud
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

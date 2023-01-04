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

#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_olc.hpp"
#include "directions.hpp"
#include "exits.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/*------------------------------------------------------------------------*/
/* function protos */

void redit_disp_extradesc_menu(DescriptorData *d);
void redit_disp_exit_menu(DescriptorData *d);
void redit_disp_exit_flag_menu(DescriptorData *d);
void redit_disp_flag_menu(DescriptorData *d);
void redit_disp_sector_menu(DescriptorData *d);
void redit_disp_menu(DescriptorData *d);
void redit_parse(DescriptorData *d, char *arg);
void redit_setup_new(DescriptorData *d);
void redit_setup_existing(DescriptorData *d, int real_num);
void redit_save_to_disk(int zone);
void redit_save_internally(DescriptorData *d);
void free_room(RoomData *room);

/*------------------------------------------------------------------------*/

#define W_EXIT(room, num) (world[(room)].exits[(num)])

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void redit_setup_new(DescriptorData *d) {
    CREATE(OLC_ROOM(d), RoomData, 1);

    OLC_ITEM_TYPE(d) = WLD_TRIGGER;
    OLC_ROOM(d)->name = strdup("An unfinished room");
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\n");
    redit_disp_menu(d);
    OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void redit_setup_existing(DescriptorData *d, int real_num) {
    RoomData *room;
    int counter;

    /*
     * Build a copy of the room for editing.
     */
    CREATE(room, RoomData, 1);

    *room = world[real_num];
    /*
     * Allocate space for all strings.
     */
    room->name = strdup(world[real_num].name ? world[real_num].name : "undefined");
    room->description = strdup(world[real_num].description ? world[real_num].description : "undefined\n");
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
                     : nullptr);
            room->exits[counter]->keyword =
                (world[real_num].exits[counter]->keyword ? strdup(world[real_num].exits[counter]->keyword) : nullptr);
        }
    }

    /*
     * Extra descriptions, if necessary.
     */
    if (world[real_num].ex_description) {
        ExtraDescriptionData *cur, *temp, *temp2;
        CREATE(temp, ExtraDescriptionData, 1);

        room->ex_description = temp;
        for (cur = world[real_num].ex_description; cur; cur = cur->next) {
            temp->keyword = (cur->keyword ? strdup(cur->keyword) : nullptr);
            temp->description = (cur->description ? strdup(cur->description) : nullptr);
            if (cur->next) {
                CREATE(temp2, ExtraDescriptionData, 1);
                temp->next = temp2;
                temp = temp2;
            } else
                temp->next = nullptr;
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

void redit_save_internally(DescriptorData *d) {
    int i, j, room_num, found = 0, zone, cmd_no;
    RoomData *new_world;
    CharData *temp_ch;
    ObjData *temp_obj;
    DescriptorData *dsc;

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
        SCRIPT(&world[room_num]) = nullptr;
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
                SCRIPT(&world[i]) = nullptr;
            }

        CREATE(new_world, RoomData, top_of_world + 2);

        /*
         * Count through world tables.
         */
        for (i = 0; i <= top_of_world; i++) {
            if (!found) {
                /*
                 * Is this the place?
                 */
                if (world[i].vnum > OLC_NUM(d)) {
                    found = true;
                    new_world[i] = *(OLC_ROOM(d));
                    new_world[i].vnum = OLC_NUM(d);
                    new_world[i].func = nullptr;
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
            new_world[i].func = nullptr;
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
                    log(LogSeverity::Warn, LVL_GOD,
                        "SYSERR:redit.c:redit_save_internally(): Unknown command: {} in zone {}.", ZCMD.command,
                        zone_table[zone].name);
                }
        /* update load rooms, to fix creeping load room problem */
        if (room_num <= mortal_start_room)
            mortal_start_room++;
        if (room_num <= immort_start_room)
            immort_start_room++;
        if (room_num <= frozen_start_room)
            frozen_start_room++;

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
    RoomData *room;
    ExtraDescriptionData *ex_desc;

    sprintf(buf, "%s/%d.new", WLD_PREFIX, zone_table[zone_num].number);
    if (!(fp = fopen(buf, "w+"))) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: Cannot open room file!");
        return;
    }
    for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++) {
        if ((realcounter = real_room(counter)) >= 0) {
            room = (world + realcounter);

            /*. Remove the '\n' sequences from description . */
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

void free_room(RoomData *room) {
    int i;
    ExtraDescriptionData *cur, *next;

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
    for (cur = room->ex_description; cur; cur = next) {
        next = cur->next;
        if (cur->keyword)
            free(cur->keyword);
        if (cur->description)
            free(cur->description);
        free(cur);
    }
}

/**************************************************************************
 *                       Menu functions                                   *
 **************************************************************************/

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(DescriptorData *d) {
    ExtraDescriptionData *extra_desc = OLC_DESC(d);

    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "%s1%s) Keyword: %s%s\n"
            "%s2%s) Description:\n%s%s\n"
            "%s3%s) Goto next description: ",
            grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>", grn, nrm, yel,
            extra_desc->description ? extra_desc->description : "<NONE>", grn, nrm);

    strcat(buf, !extra_desc->next ? "<NOT SET>\n" : "Set.\n");
    strcat(buf, "Enter choice (0 to quit) : ");
    char_printf(d->character, buf);
    OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
void redit_disp_exit_menu(DescriptorData *d) {
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
            "%s1%s) Exit to     : %s%d\n"
            "%s2%s) Description :-\n%s%s\n"
            "%s3%s) Door name   : %s%s\n"
            "%s4%s) Key         : %s%d\n"
            "%s5%s) Door flags  : %s%s\n"
            "%s6%s) Purge exit.\n"
            "Enter choice, 0 to quit : ",
            grn, nrm, cyn, OLC_EXIT(d)->to_room == NOWHERE ? -1 : world[OLC_EXIT(d)->to_room].vnum, grn, nrm, yel,
            OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>", grn, nrm, yel,
            OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>", grn, nrm, cyn, OLC_EXIT(d)->key, grn, nrm, cyn,
            buf2, grn, nrm);

    char_printf(d->character, buf);
    OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(DescriptorData *d) {
    get_char_cols(d->character);
    sprintf(buf,
            "%s0%s) No door\n"
            "%s1%s) Closeable door\n"
            "%s2%s) Pickproof\n"
            "%s3%s) Description\n"
            "Enter choice:\n",
            grn, nrm, grn, nrm, grn, nrm, grn, nrm);
    char_printf(d->character, buf);
}

/*
 * For room flags.
 */
#define FLAG_INDEX ((NUM_ROOM_FLAGS / columns + 1) * j + i)
void redit_disp_flag_menu(DescriptorData *d) {
    const int columns = 3;
    int i, j;

    get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif

    for (i = 0; i <= NUM_ROOM_FLAGS / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j)
            if (FLAG_INDEX < NUM_ROOM_FLAGS)
                sprintf(buf, "%s%s%2d%s) %-20.20s ", buf, grn, FLAG_INDEX + 1, nrm, room_bits[FLAG_INDEX]);
        char_printf(d->character, strcat(buf, "\n"));
    }

    sprintflag(buf1, OLC_ROOM(d)->room_flags, NUM_ROOM_FLAGS, room_bits);
    sprintf(buf,
            "\nRoom flags: %s%s%s\n"
            "Enter room flags, 0 to quit : ",
            cyn, buf1, nrm);
    char_printf(d->character, buf);
    OLC_MODE(d) = REDIT_FLAGS;
}

#undef FLAG_INDEX

/*
 * For sector type.
 */
#define TYPE_INDEX ((NUM_SECTORS / columns + 1) * j + i)
void redit_disp_sector_menu(DescriptorData *d) {
    const int columns = 3;
    int i, j;

#if defined(CLEAR_SCREEN)
    char_printf(d->character, "[H[J");
#endif

    for (i = 0; i < NUM_SECTORS / columns; ++i) {
        *buf = '\0';
        for (j = 0; j < columns; ++j)
            if (TYPE_INDEX < NUM_SECTORS)
                sprintf(buf, "%s%s%2d%s) %-20.20s ", buf, grn, TYPE_INDEX + 1, nrm, sectors[TYPE_INDEX].name);
        char_printf(d->character, strcat(buf, "\n"));
    }

    char_printf(d->character, "\nEnter sector type, 0 to cancel : ");
    OLC_MODE(d) = REDIT_SECTOR;
}

#undef TYPE_INDEX

/*
 * The main menu.
 */
void redit_disp_menu(DescriptorData *d) {
    RoomData *room;

    get_char_cols(d->character);
    room = OLC_ROOM(d);

    sprintflag(buf1, room->room_flags, NUM_ROOM_FLAGS, room_bits);
    sprintf(buf2, "%s", sectors[room->sector_type].name);
    sprintf(buf,
#if defined(CLEAR_SCREEN)
            ".[H.[J"
#endif
            "-- Room: '&5%s&0'  vnum: [&2%5d&0]\n"
            "%s1%s) Name        : %s%s\n"
            "%s2%s) Description :\n%s%s\n"
            "%s3%s) Room flags  : %s%s\n"
            "%s4%s) Sector type : %s%s\n",
            room->name, OLC_NUM(d), grn, nrm, yel, room->name, grn, nrm, yel, room->description, grn, nrm, cyn, buf1,
            grn, nrm, cyn, buf2);

    sprintf(buf, "%s%s5%s) Exit north  : %s%s\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[NORTH]));

    sprintf(buf, "%s%s6%s) Exit east   : %s%s\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[EAST]));
    sprintf(buf, "%s%s7%s) Exit south  : %s%s\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[SOUTH]));
    sprintf(buf, "%s%s8%s) Exit west   : %s%s\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[WEST]));
    sprintf(buf, "%s%s9%s) Exit up     : %s%s\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[UP]));
    sprintf(buf, "%s%sA%s) Exit down   : %s%s\n", buf, grn, nrm, cyn, exit_dest_desc(room->exits[DOWN]));

    sprintf(buf,
            "%s%sF%s) Extra descriptions menu\n"
            "%sS%s) Script      : %s%s\n"
            "%sQ%s) Quit\n"
            "Enter choice:\n",
            buf, grn, nrm, grn, nrm, cyn, room->proto_script ? "Set." : "Not Set.", grn, nrm);
    char_printf(d->character, buf);

    OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
 *                        The main loop                                   *
 **************************************************************************/

void redit_parse(DescriptorData *d, char *arg) {
    int number;

    switch (OLC_MODE(d)) {
    case REDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
        case 'y':
        case 'Y':
            redit_save_internally(d);
            log(LogSeverity::Debug, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), "OLC: {} edits room {:d}.",
                GET_NAME(d->character), OLC_NUM(d));
            /*
             * Do NOT free strings! Just the room structure.
             */
            cleanup_olc(d, CLEANUP_STRUCTS);
            char_printf(d->character, "Room saved to memory.\n");
            break;
        case 'n':
        case 'N':
            /* free everything up, including strings etc */
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            char_printf(d->character, "Invalid choice!\nDo you wish to save this room internally?\n");
            break;
        }
        return;

    case REDIT_MAIN_MENU:
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) { /* Something has been modified. */
                char_printf(d->character, "Do you wish to save this room internally?\n");
                OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '1':
            char_printf(d->character, "Enter room name:]\n");
            OLC_MODE(d) = REDIT_NAME;
            break;
        case '2':
            OLC_MODE(d) = REDIT_DESC;
#if defined(CLEAR_SCREEN)
            string_to_output(d, "\x1B[H\x1B[J");
#endif
            string_to_output(d, "Enter room description: (/s saves /h for help)\n\n");
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
                CREATE(OLC_ROOM(d)->ex_description, ExtraDescriptionData, 1);
                OLC_ROOM(d)->ex_description->next = nullptr;
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
            char_printf(d->character, "Invalid choice!\n");
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
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: Reached REDIT_DESC case in parse_redit");
        break;

    case REDIT_FLAGS:
        number = atoi(arg);
        if ((number < 0) || (number > NUM_ROOM_FLAGS)) {
            char_printf(d->character, "That is not a valid choice!\n");
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
            char_printf(d->character, "Invalid choice!\n");
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
            char_printf(d->character, "Exit to room number:\n");
            return;
        case '2':
            OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
            string_to_output(d, "Enter exit description: (/s saves /h for help)\n\n");
            string_write(d, &OLC_EXIT(d)->general_description, MAX_EXIT_DESC);
            return;
        case '3':
            OLC_MODE(d) = REDIT_EXIT_KEYWORD;
            char_printf(d->character, "Enter keywords:\n");
            return;
        case '4':
            OLC_MODE(d) = REDIT_EXIT_KEY;
            char_printf(d->character, "Enter key number:\n");
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
            OLC_EXIT(d) = nullptr;
            break;
        default:
            char_printf(d->character, "Try again:\n");
            return;
        }
        break;

    case REDIT_EXIT_NUMBER:
        if ((number = atoi(arg)) != -1)
            if ((number = real_room(number)) < 0) {
                char_printf(d->character, "That room does not exist.  Try again:\n");
                return;
            }
        OLC_EXIT(d)->to_room = number;
        redit_disp_exit_menu(d);
        return;

    case REDIT_EXIT_DESCRIPTION:
        /* we should NEVER get here */
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: Reached REDIT_EXIT_DESC case in parse_redit");
        break;

    case REDIT_EXIT_KEYWORD:
        if (OLC_EXIT(d)->keyword)
            free(OLC_EXIT(d)->keyword);
        OLC_EXIT(d)->keyword = ((arg && *arg) ? strdup(arg) : nullptr);
        redit_disp_exit_menu(d);
        return;

    case REDIT_EXIT_KEY:
        OLC_EXIT(d)->key = atoi(arg);
        redit_disp_exit_menu(d);
        return;

    case REDIT_EXIT_DOORFLAGS:
        number = atoi(arg);
        if ((number < 0) || (number > 3)) {
            char_printf(d->character, "That's not a valid choice!\n");
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
        OLC_DESC(d)->keyword = ((arg && *arg) ? strdup(arg) : nullptr);
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
                ExtraDescriptionData **tmp_desc;

                if (OLC_DESC(d)->keyword)
                    free(OLC_DESC(d)->keyword);
                if (OLC_DESC(d)->description)
                    free(OLC_DESC(d)->description);

                /*
                 * Clean up pointers.
                 */
                for (tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next))
                    if (*tmp_desc == OLC_DESC(d)) {
                        *tmp_desc = nullptr;
                        break;
                    }
                free(OLC_DESC(d));
            }
        } break;
        case 1:
            OLC_MODE(d) = REDIT_EXTRADESC_KEY;
            char_printf(d->character, "Enter keywords, separated by spaces:\n");
            return;
        case 2:
            OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
            string_to_output(d, "Enter extra description: (/s saves /h for help)\n\n");
            string_write(d, &OLC_DESC(d)->description, MAX_DESC_LENGTH);
            return;

        case 3:
            if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
                char_printf(d->character, "You can't edit the next extra desc without completing this one.\n");
                redit_disp_extradesc_menu(d);
            } else {
                ExtraDescriptionData *new_extra;

                if (OLC_DESC(d)->next)
                    OLC_DESC(d) = OLC_DESC(d)->next;
                else {
                    /* make new extra, attach at end */
                    CREATE(new_extra, ExtraDescriptionData, 1);
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
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: Reached default case in parse_redit");
        break;
    }
    /*. If we get this far, something has be changed . */
    OLC_VAL(d) = 1;
    redit_disp_menu(d);
}

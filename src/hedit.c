/***************************************************************************
 * $Id: hedit.c,v 1.15 2008/08/14 09:45:22 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: hedit.c                                        Part of FieryMUD *
 *  Usage: Edits help files, for use with OasisOLC.                        *
 *     By: Steve Wolfe                                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "comm.h"
#include "conf.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "modify.h"
#include "olc.h"
#include "screen.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

int find_help(char *keyword);

void free_help(struct help_index_element *help);

/* function protos */
void hedit_disp_menu(struct descriptor_data *d);
void hedit_parse(struct descriptor_data *d, char *arg);
void hedit_setup_new(struct descriptor_data *d);
void hedit_setup_existing(struct descriptor_data *d, int real_num);
void hedit_save_to_disk(struct descriptor_data *d);
void hedit_save_internally(struct descriptor_data *d);
void index_boot(int mode);

/*
 * Utils and exported functions.
 */

void hedit_setup_new(struct descriptor_data *d) {
    CREATE(OLC_HELP(d), struct help_index_element, 1);
    OLC_HELP(d)->keyword = strdup(OLC_STORAGE(d));
    OLC_HELP(d)->entry = strdup("KEYWORDS\r\n\r\nThis help file is unfinished.\r\n");
    OLC_HELP(d)->min_level = 0;
    OLC_HELP(d)->duplicate = 0;
    OLC_VAL(d) = 0;
    hedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void hedit_setup_existing(struct descriptor_data *d, int real_num) {
    CREATE(OLC_HELP(d), struct help_index_element, 1);
    OLC_HELP(d)->keyword = strdup(help_table[real_num].keyword);
    OLC_HELP(d)->entry = strdup(help_table[real_num].entry);
    OLC_HELP(d)->min_level = help_table[real_num].min_level;
    OLC_HELP(d)->duplicate = help_table[real_num].duplicate;
    OLC_VAL(d) = 0;
    hedit_disp_menu(d);
}

void hedit_save_internally(struct descriptor_data *d) {
    struct help_index_element *new_help_table = NULL;
    int i;
    char *temp = NULL;

    /* add a new help element into the list */
    if (OLC_ZNUM(d) > top_of_helpt) {
        CREATE(new_help_table, struct help_index_element, top_of_helpt + 2);
        for (i = 0; i <= top_of_helpt; i++)
            new_help_table[i] = help_table[i];
        new_help_table[++top_of_helpt] = *OLC_HELP(d);
        free(help_table);
        help_table = new_help_table;
    } else {
        /* This was the original end of hedit_save_internally,
         * however, this would cause crashing in multiple keyword
         * entries.  The fix was posted by Chris Jacobson
         * <fear@ATHENET.NET> on the CircleMUD Mailing List
         *
         *  i = find_help(OLC_HELP(d)->keyword);
         *
         *  free_help(help_table + OLC_ZNUM(d));
         *  help_table[OLC_ZNUM(d)] = *OLC_HELP(d);
         *
         * Begin the fix
         */
        temp = help_table[OLC_ZNUM(d)].entry;
        for (i = 0; i < top_of_helpt; i++) {
            if (help_table[i].entry == temp) {
                /*    if (!help_table[i].duplicate)   free(help_table[i].entry);
                 * The line above had to be commented because freeing based
                 * upon a duplicate is not only unnecessary--it's dangerous.
                 */
                help_table[i].entry = OLC_HELP(d)->entry;
                help_table[i].min_level = OLC_HELP(d)->min_level;
            }
        }
        free(temp);
    }
    olc_add_to_save_list(HEDIT_PERMISSION, OLC_SAVE_HELP);
}

/*------------------------------------------------------------------------*/

void hedit_save_to_disk(struct descriptor_data *d) {
    FILE *fp;
    int i;

    if (!(fp = fopen(HELP_FILE, "w+"))) {
        sprintf(buf, "Can't open help file '%s'", HELP_FILE);
        perror(buf);
        exit(1);
    }
    for (i = 0; i <= top_of_helpt; i++) {
        if (help_table[i].duplicate > 0)
            continue;

        /*. Remove the '\r\n' sequences from entry . */
        strcpy(buf1, help_table[i].entry ? help_table[i].entry : "Empty");
        strip_string(buf1);

        sprintf(buf, "%s#%d\n", buf1, help_table[i].min_level);
        fputs(buf, fp);
    }

    fprintf(fp, "$\n");
    fclose(fp);
    olc_remove_from_save_list(HEDIT_PERMISSION, OLC_SAVE_HELP);
}

/*------------------------------------------------------------------------*/

/* Menu functions */

/* the main menu */
void hedit_disp_menu(struct descriptor_data *d) {
    struct help_index_element *help;

    get_char_cols(d->character);

    help = OLC_HELP(d);

    sprintf(buf,
            "Keywords       : %s%s\r\n"
            "%s1%s) Entry       :\r\n%s%s"
            "%s2%s) Min Level   : %s%d\r\n"
            "%sQ%s) Quit\r\n"
            "Enter choice : ",
            yel, help->keyword, grn, nrm, yel, help->entry, grn, nrm, yel, help->min_level, grn, nrm);
    send_to_char(buf, d->character);

    OLC_MODE(d) = HEDIT_MAIN_MENU;
}

/*
 * The main loop
 */

void hedit_parse(struct descriptor_data *d, char *arg) {
    int i;

    switch (OLC_MODE(d)) {
    case HEDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
        case 'y':
        case 'Y':
            hedit_save_internally(d);
            hedit_save_to_disk(d);
            if (help_table) {
                for (i = 0; i <= top_of_helpt; i++) {
                    if (help_table[i].keyword)
                        free(help_table[i].keyword);
                    if (help_table[i].entry && !help_table[i].duplicate)
                        free(help_table[i].entry);
                }
                free(help_table);
            }
            top_of_helpt = 0;
            index_boot(DB_BOOT_HLP);

            sprintf(buf, "OLC: %s edits help for %s.", GET_NAME(d->character), OLC_HELP(d)->keyword);
            mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
            /* do not free the strings.. just the structure */
            cleanup_olc(d, CLEANUP_STRUCTS);
            send_to_char("Help saved to memory.\r\n", d->character);
            break;
        case 'n':
        case 'N':
            /* free everything up, including strings etc */
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            send_to_char("Invalid choice!\r\nDo you wish to save this help internally? ", d->character);
            break;
        }
        return; /* end of HEDIT_CONFIRM_SAVESTRING */

    case HEDIT_CONFIRM_EDIT:
        switch (*arg) {
        case 'y':
        case 'Y':
            hedit_setup_existing(d, OLC_ZNUM(d));
            break;
        case 'q':
        case 'Q':
            cleanup_olc(d, CLEANUP_ALL);
            break;
        case 'n':
        case 'N':
            OLC_ZNUM(d)++;
            for (; (OLC_ZNUM(d) <= top_of_helpt); OLC_ZNUM(d)++)
                if (is_abbrev(OLC_STORAGE(d), help_table[OLC_ZNUM(d)].keyword))
                    break;
            if (OLC_ZNUM(d) > top_of_helpt) {
                if (find_help(OLC_STORAGE(d)) > NOTHING) {
                    cleanup_olc(d, CLEANUP_ALL);
                    break;
                }
                sprintf(buf, "Do you wish to add help on '%s'? ", OLC_STORAGE(d));
                send_to_char(buf, d->character);
                OLC_MODE(d) = HEDIT_CONFIRM_ADD;
            } else {
                sprintf(buf, "Do you wish to edit help on '%s'? ", help_table[OLC_ZNUM(d)].keyword);
                send_to_char(buf, d->character);
                OLC_MODE(d) = HEDIT_CONFIRM_EDIT;
            }
            break;
        default:
            sprintf(buf, "Invalid choice!\r\nDo you wish to edit help on '%s'? ", help_table[OLC_ZNUM(d)].keyword);
            send_to_char(buf, d->character);
            break;
        }
        return;

    case HEDIT_CONFIRM_ADD:
        switch (*arg) {
        case 'y':
        case 'Y':
            hedit_setup_new(d);
            break;
        case 'n':
        case 'N':
        case 'q':
        case 'Q':
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            sprintf(buf, "Invalid choice!\r\nDo you wish to add help on '%s'? ", OLC_STORAGE(d));
            send_to_char(buf, d->character);
            break;
        }
        return;

    case HEDIT_MAIN_MENU:
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) { /* Something was modified */
                send_to_char("Do you wish to save this help internally? ", d->character);
                OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
            } else
                cleanup_olc(d, CLEANUP_ALL);
            break;
        case '1':
            OLC_MODE(d) = HEDIT_ENTRY;
            write_to_output("Enter the help info: (/s saves /h for help)\r\n\r\n", d);
            string_write(d, &OLC_HELP(d)->entry, MAX_STRING_LENGTH);
            OLC_VAL(d) = 1;
            break;
        case '2':
            send_to_char("Enter the minimum level a player has to be to read this help: ", d->character);
            OLC_MODE(d) = HEDIT_MIN_LEVEL;
            return;
        default:
            hedit_disp_menu(d);
            break;
        }
        return;

    case HEDIT_ENTRY:
        /* should never get here */
        mudlog("SYSERR: Reached HEDIT_ENTRY in hedit_parse", BRF, LVL_ATTENDANT, TRUE);
        break;
    case HEDIT_MIN_LEVEL:
        if (*arg) {
            i = atoi(arg);
            if ((i < 0) && (i > LVL_IMPL)) {
                hedit_disp_menu(d);
                return;
            } else
                OLC_HELP(d)->min_level = i;
        } else {
            hedit_disp_menu(d);
            return;
        }
        break;
    default:
        /* we should never get here */
        break;
    }
    OLC_VAL(d) = 1;
    hedit_disp_menu(d);
}

int find_help(char *keyword) {
    int chk, bot, top, mid, minlen;

    bot = 0;
    top = top_of_helpt;
    minlen = strlen(keyword);

    for (;;) {
        mid = (bot + top) / 2;

        if (bot > top)
            return -1;
        else if (!(chk = strn_cmp(keyword, help_table[mid].keyword, minlen))) {
            /* trace backwards to find first matching entry. Thanks Jeff Fink! */
            while ((mid > 0) && (!(chk = strn_cmp(keyword, help_table[mid - 1].keyword, minlen))))
                mid--;
            return mid;
        } else {
            if (chk > 0)
                bot = mid + 1;
            else
                top = mid - 1;
        }
    }
}

void free_help(struct help_index_element *help) {
    if (help->keyword)
        free(help->keyword);
    if (help->entry)
        free(help->entry);
    memset(help, 0, sizeof(struct help_index_element));
}

/***************************************************************************
 * $Log: hedit.c,v $
 * Revision 1.15  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.14  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.13  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.12  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.11  2008/02/11 21:04:01  myc
 * Fix the formatting in the main menu.
 *
 * Revision 1.10  2008/02/10 20:30:03  myc
 * Clean-up hedit a tiny bit.
 *
 * Revision 1.9  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.8  2007/11/18 16:51:55  myc
 * Fixing LVL_BUILDER references.
 *
 * Revision 1.7  2007/09/07 04:12:25  jps
 * Fix keyword formatting and disable screen-clearing when you start to edit.
 *
 * Revision 1.6  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.5  2000/11/21 18:56:43  rsd
 * Altered comment header and added back rlog messages from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.2  1999/01/31 06:19:33  mud
 * Added comment header
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

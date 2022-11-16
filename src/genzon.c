/***************************************************************************
 *  File: genzon.c                                        Part of FieryMUD *
 *  Usage: zone routines                                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "genzon.h"

#include "conf.h"
#include "db.h"
#include "dg_scripts.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* local functions */
static void remove_cmd_from_list(struct reset_com **list, int pos);

/* Some common code to count the number of commands in the list. */
int count_commands(struct reset_com *list) {
    int count = 0;

    while (list[count].command != 'S')
        count++;

    return count;
}

/* Remove a reset command from a list. Takes a pointer to the list so that it
 * may play with the memory locations. */
static void remove_cmd_from_list(struct reset_com **list, int pos) {
    int count, i, l;
    struct reset_com *newlist;

    /* Count number of commands (not including terminator). */
    count = count_commands(*list);

    /* Value is 'count' because we didn't include the terminator above but since
     * we're deleting one thing anyway we want one less. */
    CREATE(newlist, struct reset_com, count);

    /* Even tighter loop to copy old list and skip unwanted command. */
    for (i = 0, l = 0; i < count; i++) {
        if (i != pos) {
            newlist[l++] = (*list)[i];
        }
    }
    /* Add the terminator, then insert the new list. */
    newlist[count - 1].command = 'S';
    free(*list);
    *list = newlist;
}

/* Error check user input and then remove command. */
void delete_zone_command(struct zone_data *zone, int pos) {
    int subcmd = 0;

    /* Error check to ensure users hasn't given too large an index. */
    while (zone->cmd[subcmd].command != 'S')
        subcmd++;

    if (pos < 0 || pos >= subcmd)
        return;

    /* Ok, let's zap it. */
    remove_cmd_from_list(&zone->cmd, pos);
}
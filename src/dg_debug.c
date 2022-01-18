/***************************************************************************
 *   File: dg_debug.c                                     Part of FieryMUD *
 *  Usage: contains all the trigger debug functions for scripts.           *
 *  		for example set and unset variables and force triggers     *
 *  		to run.                                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *                                                                         *
 *  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "comm.h"
#include "conf.h"
#include "db.h"
#include "dg_scripts.h"
#include "handler.h"
#include "interpreter.h"
#include "olc.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/*
 * do_varset
 *
 * descr:	set a global variable on a room/mob/obj
 * usage:	varset {mob|obj|room} <varname> <vnum> [<varval>]
 *
 * note:	for mob only works in current room
 * 		for obj looks in inv, then room (NOT WORN)
 * see also:	do_varset
 */
ACMD(do_varset) {
    extern void add_var(struct trig_var_data * *var_list, char *name, char *value);
    char victtype[10];
    int vnum;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char("Usage: varset {mob|obj|room} <vnum> <varname> [<varval>]\r\n", ch);
        return;
    }

    argument = one_argument(argument, victtype);
    if (!*victtype) {
        send_to_char("Usage: varset {MOB|OBJ|ROOM} <vnum> <varname> [<varval>]\r\n", ch);
        return;
    }

    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varset {mob|obj|room} <VNUM> <varname> [<varval>]\r\n", ch);
        return;
    }
    if (!(vnum = atoi(buf))) {
        send_to_char("Usage: varset {mob|obj|room} <VNUM> <varname> [<varval>]\r\n", ch);
        return;
    }
    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varset {mob|obj|room} <vnum> <VARNAME> [<varval>]\r\n", ch);
        return;
    }

    /*
     * based on type to set, we need to grab the right array to
     * pass it to add_var
     */
    if (!str_cmp("mob", victtype)) {
        struct char_data *found_char = NULL;

        found_char = world[(ch->in_room)].people;

        while (found_char && (GET_MOB_VNUM(found_char) != vnum))
            found_char = found_char->next_in_room;
        if (!found_char)
            send_to_char("Unable to find that mob in this room\r\n", ch);
        else {
            if (!SCRIPT(found_char))
                CREATE(SCRIPT(found_char), struct script_data, 1);
            add_var(&(SCRIPT(found_char)->global_vars), buf, argument);
        }
    } else if (!str_cmp("obj", victtype)) {
        struct obj_data *found_obj = NULL;
        found_obj = ch->carrying;
        while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
            found_obj = found_obj->next_content;
        if (!found_obj) {
            found_obj = world[(ch->in_room)].contents;
            while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
                found_obj = found_obj->next_content;
        }

        if (!found_obj)
            send_to_char("Unable to find that obj in inventory or room\r\n", ch);
        else {
            if (!SCRIPT(found_obj))
                CREATE(SCRIPT(found_obj), struct script_data, 1);
            add_var(&(SCRIPT(found_obj)->global_vars), buf, argument);
        }
    } else if (!str_cmp("room", victtype)) {
        int rnum = real_room(vnum);
        if (rnum == NOWHERE)
            send_to_char("That room does not exist!\r\n", ch);
        else {
            if (!SCRIPT(&world[rnum]))
                CREATE(SCRIPT(&world[rnum]), struct script_data, 1);
            add_var(&(SCRIPT(&world[rnum])->global_vars), buf, argument);
        }
    } else {
        send_to_char("Usage: varset {MOB|OBJ|ROOM} <vnum> <varname> [<varval>]\r\n", ch);
    }
}

/*
 * do_varunset
 *
 * descr:	remove a _global_ variable on a room/mob/obj
 * usage:	varunset {mob|obj|room} <varname>
 *
 * note:	Note the use of RNUM (not vnum which applies to all mobs!)
 * note:	this just removes the var, to set it to a null value
 * 		use varset without a value
 *
 * see also:	do_varset
 */
ACMD(do_varunset) {
    extern void remove_var(struct trig_var_data * *var_list, char *name, char *value);
    char victtype[10];
    int vnum;
    skip_spaces(&argument);
    if (!*argument) {
        send_to_char("Usage: varunset {mob|obj|room} <rnum> <varname>\r\n", ch);
        return;
    }

    argument = one_argument(argument, victtype);
    if (!*victtype) {
        send_to_char("Usage: varunset {MOB|OBJ|ROOM} <vnum> <varname>\r\n", ch);
        return;
    }

    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varunset {mob|obj|room} <VNUM> <varname>\r\n", ch);
        return;
    }
    if (!(vnum = atoi(buf))) {
        send_to_char("Usage: varunset {mob|obj|room} <VNUM> <varname>\r\n", ch);
        return;
    }
    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varunset {mob|obj|room} <vnum> <VARNAME>\r\n", ch);
        return;
    }

    /*
     * based on type to unset, we need to grab the right array to
     * pass it to remove_var
     */
    if (!str_cmp("mob", victtype)) {
        struct char_data *found_char = NULL;

        found_char = world[(ch->in_room)].people;

        while (found_char && (GET_MOB_VNUM(found_char) != vnum))
            found_char = found_char->next_in_room;
        if (!found_char)
            send_to_char("Unable to find that mob in this room\r\n", ch);
        else
            remove_var(&(SCRIPT(found_char)->global_vars), buf, argument);
    } else if (!str_cmp("obj", victtype)) {
        struct obj_data *found_obj = NULL;
        found_obj = ch->carrying;
        while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
            found_obj = found_obj->next_content;
        if (!found_obj) {
            found_obj = world[(ch->in_room)].contents;
            while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
                found_obj = found_obj->next_content;
        }

        if (!found_obj)
            send_to_char("Unable to find that obj in inventory or room\r\n", ch);
        else
            remove_var(&(SCRIPT(found_obj)->global_vars), buf, argument);
    } else if (!str_cmp("room", victtype)) {
        int rnum = real_room(vnum);
        if (rnum == NOWHERE)
            send_to_char("That room does not exist!\r\n", ch);
        else
            remove_var(&(SCRIPT(&world[rnum])->global_vars), buf, argument);
    } else {
        send_to_char("Usage: varunset {MOB|OBJ|ROOM} <vnum> <varname>\r\n", ch);
    }
}

/*
 * do_runtrigger
 *
 * descr:	force a trigger to run even if conditions are not met
 * usage:	runtrigger {mob|obj|room} <rnum> <trignum>
 *
 * note:	Note the use of RNUM (not vnum which applies to all mobs!)
 * note:	if the trigger uses external variables, the user will be
 * prompted for them. Examples include %actor% in most triggers
 */
ACMD(do_runtrigger) {}

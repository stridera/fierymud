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

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "olc.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "logging.hpp"


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
    char victtype[10];
    int vnum;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char("Usage: varset {mob|obj|room} <vnum> <varname> [<varval>]\n", ch);
        return;
    }

    argument = one_argument(argument, victtype);
    if (!*victtype) {
        send_to_char("Usage: varset {MOB|OBJ|ROOM} <vnum> <varname> [<varval>]\n", ch);
        return;
    }

    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varset {mob|obj|room} <VNUM> <varname> [<varval>]\n", ch);
        return;
    }
    if (!(vnum = atoi(buf))) {
        send_to_char("Usage: varset {mob|obj|room} <VNUM> <varname> [<varval>]\n", ch);
        return;
    }
    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varset {mob|obj|room} <vnum> <VARNAME> [<varval>]\n", ch);
        return;
    }

    /*
     * based on type to set, we need to grab the right array to
     * pass it to add_var
     */
    if (!strcasecmp("mob", victtype)) {
        CharData *found_char = nullptr;

        found_char = world[(ch->in_room)].people;

        while (found_char && (GET_MOB_VNUM(found_char) != vnum))
            found_char = found_char->next_in_room;
        if (!found_char)
            send_to_char("Unable to find that mob in this room\n", ch);
        else {
            if (!SCRIPT(found_char))
                CREATE(SCRIPT(found_char), ScriptData, 1);
            add_var(&(SCRIPT(found_char)->global_vars), buf, argument);
        }
    } else if (!strcasecmp("obj", victtype)) {
        ObjData *found_obj = nullptr;
        found_obj = ch->carrying;
        while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
            found_obj = found_obj->next_content;
        if (!found_obj) {
            found_obj = world[(ch->in_room)].contents;
            while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
                found_obj = found_obj->next_content;
        }

        if (!found_obj)
            send_to_char("Unable to find that obj in inventory or room\n", ch);
        else {
            if (!SCRIPT(found_obj))
                CREATE(SCRIPT(found_obj), ScriptData, 1);
            add_var(&(SCRIPT(found_obj)->global_vars), buf, argument);
        }
    } else if (!strcasecmp("room", victtype)) {
        int rnum = real_room(vnum);
        if (rnum == NOWHERE)
            send_to_char("That room does not exist!\n", ch);
        else {
            if (!SCRIPT(&world[rnum]))
                CREATE(SCRIPT(&world[rnum]), ScriptData, 1);
            add_var(&(SCRIPT(&world[rnum])->global_vars), buf, argument);
        }
    } else {
        send_to_char("Usage: varset {MOB|OBJ|ROOM} <vnum> <varname> [<varval>]\n", ch);
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
    char victtype[10];
    int vnum;
    skip_spaces(&argument);
    if (!*argument) {
        send_to_char("Usage: varunset {mob|obj|room} <rnum> <varname>\n", ch);
        return;
    }

    argument = one_argument(argument, victtype);
    if (!*victtype) {
        send_to_char("Usage: varunset {MOB|OBJ|ROOM} <vnum> <varname>\n", ch);
        return;
    }

    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varunset {mob|obj|room} <VNUM> <varname>\n", ch);
        return;
    }
    if (!(vnum = atoi(buf))) {
        send_to_char("Usage: varunset {mob|obj|room} <VNUM> <varname>\n", ch);
        return;
    }
    argument = one_argument(argument, buf);
    if (!*buf) {
        send_to_char("Usage: varunset {mob|obj|room} <vnum> <VARNAME>\n", ch);
        return;
    }

    /*
     * based on type to unset, we need to grab the right array to
     * pass it to remove_var
     */
    if (!strcasecmp("mob", victtype)) {
        CharData *found_char = nullptr;

        found_char = world[(ch->in_room)].people;

        while (found_char && (GET_MOB_VNUM(found_char) != vnum))
            found_char = found_char->next_in_room;
        if (!found_char)
            send_to_char("Unable to find that mob in this room\n", ch);
        else
            remove_var(&(SCRIPT(found_char)->global_vars), buf);
    } else if (!strcasecmp("obj", victtype)) {
        ObjData *found_obj = nullptr;
        found_obj = ch->carrying;
        while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
            found_obj = found_obj->next_content;
        if (!found_obj) {
            found_obj = world[(ch->in_room)].contents;
            while (found_obj && (GET_OBJ_VNUM(found_obj) != vnum))
                found_obj = found_obj->next_content;
        }

        if (!found_obj)
            send_to_char("Unable to find that obj in inventory or room\n", ch);
        else
            remove_var(&(SCRIPT(found_obj)->global_vars), buf);
    } else if (!strcasecmp("room", victtype)) {
        int rnum = real_room(vnum);
        if (rnum == NOWHERE)
            send_to_char("That room does not exist!\n", ch);
        else
            remove_var(&(SCRIPT(&world[rnum])->global_vars), buf);
    } else {
        send_to_char("Usage: varunset {MOB|OBJ|ROOM} <vnum> <varname>\n", ch);
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

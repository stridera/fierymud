/***************************************************************************
 *   File: dg_mobcmd.c                                    Part of FieryMUD *
 *  Usage: See below                                                       *
 *     By: Apparetntly N'Atas-ha                                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *                                                                         *
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include "casting.h"
#include "chars.h"
#include "comm.h"
#include "conf.h"
#include "damage.h"
#include "db.h"
#include "dg_scripts.h"
#include "directions.h"
#include "events.h"
#include "exits.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "limits.h"
#include "math.h"
#include "movement.h"
#include "pfiles.h"
#include "players.h"
#include "quest.h"
#include "screen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

extern int get_room_location(char *room);
extern int obj_room(struct obj_data *obj);

void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
int script_driver(void *go_address, trig_data *trig, int type, int mode);

/*
 * Local functions.
 */

/* attaches mob's name and vnum to msg and sends it to script_log */
void mob_log(char_data *mob, char *msg) {
    char buf[MAX_INPUT_LENGTH + 100];

    void script_log(struct trig_data * t, char *msg);

    sprintf(buf, "(TRG)(mob %d): %s", GET_MOB_VNUM(mob), msg);
    script_log((struct trig_data *)NULL, buf);
}

int find_mob_target_room(struct char_data *ch, char *rawroomstr) {
    int location;
    struct char_data *target_char;
    struct obj_data *target_obj;
    char roomstr[MAX_INPUT_LENGTH];

    one_argument(rawroomstr, roomstr);

    if (!*roomstr)
        return NOWHERE;

    /*
     * Don't try to get a room if there's a '.', because that usually
     * indicates that the script wants to locate something like '3.guard'.
     */
    if ((location = get_room_location(roomstr)) != NOWHERE)
        ;
    else if ((target_char = find_char_for_mtrig(ch, roomstr)) && (location = IN_ROOM(target_char)) != NOWHERE)
        ;
    else if ((target_obj = find_obj_for_mtrig(ch, roomstr)) && (location = obj_room(target_obj)) != NOWHERE)
        ;
    else
        return NOWHERE;

    if (ROOM_FLAGGED(location, ROOM_GODROOM) || ROOM_FLAGGED(location, ROOM_HOUSE))
        return NOWHERE;

    return location;
}

/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch) (IS_NPC(ch) && (!(ch)->desc || GET_LEVEL((ch)->desc->original) >= LVL_IMPL))

/* mob commands */

ACMD(do_mdamage) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], damtype[MAX_INPUT_LENGTH];
    int dam = 0, dtype = DAM_UNDEFINED, *damdone = NULL;
    trig_data *t;
    char_data *victim;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh!?\r\n", ch);
        return;
    }

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (t->running) {
            damdone = &(t->damdone);
            break;
        }
    }

    argument = one_argument(argument, name);
    if (damdone)
        *damdone = 0;
    else {
        sprintf(buf, "[ WARN: do_mdamage() for %s - can't identify running trigger ]", GET_NAME(ch));
        mudlog(buf, BRF, LVL_GOD, FALSE);
    }

    if (!*name) {
        mob_log(ch, "mdamage called with no arguments");
        return;
    }

    argument = one_argument(argument, amount);

    if (!*amount) {
        mob_log(ch, "mdamage called without second argument (amount)");
        return;
    }

    if (!isdigit(*amount)) {
        sprintf(buf, "mdamage called with invalid second argument (\"%s\") - not a number", amount);
        mob_log(ch, buf);
        return;
    }

    /* hitpoint is a short signed int */
    dam = MAX(-32767, MIN(atoi(amount), 32767));

    if (!(victim = find_char_for_mtrig(ch, name))) {
        sprintf(buf, "mdamage: victim (%s) not found", name);
        mob_log(ch, buf);
        return;
    }

    if (GET_LEVEL(victim) >= LVL_IMMORT)
        return;

    /* Check for and use optional damage-type parameter */
    argument = one_argument(argument, damtype);
    if (*damtype) {
        dtype = parse_damtype(0, damtype);
        if (dtype == DAM_UNDEFINED) {
            sprintf(buf,
                    "mdamage called with invalid third argument (\"%s\") - not a "
                    "damage type",
                    damtype);
            mob_log(ch, buf);
            return;
        }
        dam = dam * susceptibility(victim, dtype) / 100;
        if (!dam)
            return;
    }

    if (damdone)
        *damdone = dam;
    sethurtevent(ch, victim, dam);
}

/* allow a mob to set ANY skill or spell based on targets class
 * and level
 * syntax mskillset <plyrname> <name_skill_or_spell>
 */
ACMD(do_mskillset) {
    struct char_data *victim;
    char arg[MAX_INPUT_LENGTH];
    int skspnum;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }
    argument = one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mskillset called with no arguments");
        return;
    }

    if (!(victim = find_char_for_mtrig(ch, arg))) {
        sprintf(buf, "mskillset: victim (%s) not found", arg);
        mob_log(ch, buf);
        return;
    }
    /*
     * we have a victim, do we have a valid skill?
     */
    skip_spaces(&argument);
    if ((skspnum = find_talent_num(argument, TALENT)) < 0) {
        /* no such spell/skill */
        sprintf(buf, "mskillset called with unknown skill/spell '%s'", argument);
        mob_log(ch, buf);
        return;
    }

    /*
     * because we're nice really, we will give the player the max proficiency for
     * their level at this skill..don't thank me just throw money..
     */
    SET_SKILL(victim, skspnum, return_max_skill(victim, skspnum));
}

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound) {
    int was_in_room;
    int door;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (!*argument) {
        mob_log(ch, "masound called with no argument");
        return;
    }

    skip_spaces(&argument);

    was_in_room = IN_ROOM(ch);
    for (door = 0; door < NUM_OF_DIRS; door++) {
        struct exit *exit;

        if (((exit = world[was_in_room].exits[door]) != NULL) && exit->to_room != NOWHERE &&
            exit->to_room != was_in_room) {
            IN_ROOM(ch) = exit->to_room;
            sub_write(argument, ch, TRUE, TO_ROOM);
        }
    }

    IN_ROOM(ch) = was_in_room;
}

/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill) {
    char arg[MAX_INPUT_LENGTH];
    char_data *victim;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mkill called with no argument");
        return;
    }

    if (!(victim = find_char_for_mtrig(ch, arg))) {
        sprintf(buf, "mkill: victim (%s) not found", arg);
        mob_log(ch, buf);
        return;
    }

    if (victim == ch) {
        mob_log(ch, "mkill: victim is self");
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master == victim) {
        mob_log(ch, "mkill: charmed mob attacking master");
        return;
    }

    if (FIGHTING(ch)) {
        mob_log(ch, "mkill: already fighting");
        return;
    }

    attack(ch, victim);
    return;
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk) {
    char argbuf[MAX_INPUT_LENGTH], *arg = argbuf;
    int pos, dotmode;
    obj_data *obj;
    obj_data *obj_next;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mjunk called with no argument");
        return;
    }

    dotmode = find_all_dots(&arg);

    if (dotmode == FIND_INDIV) {
        if ((obj = find_obj_in_eq(ch, &pos, find_vis_by_name(ch, arg))) != NULL) {
            unequip_char(ch, pos);
            extract_obj(obj);
        } else if ((obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))) != NULL) {
            extract_obj(obj);
        }
    } else {
        for (obj = ch->carrying; obj != NULL; obj = obj_next) {
            obj_next = obj->next_content;
            /* If it is 'all.blah' then find_all_dots converts to 'blah' */
            if (dotmode == FIND_ALL || isname(arg, obj->name))
                extract_obj(obj);
        }
        if (dotmode == FIND_ALL) {
            for (pos = 0; pos < NUM_WEARS; ++pos)
                if (GET_EQ(ch, pos)) {
                    unequip_char(ch, pos);
                    extract_obj(obj);
                }
        } else {
            while ((obj = find_obj_in_eq(ch, &pos, find_vis_by_name(ch, arg)))) {
                unequip_char(ch, pos);
                extract_obj(obj);
            }
        }
    }
}

/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround) {
    char arg[MAX_INPUT_LENGTH];
    char_data *victim;
    char *p;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    p = one_argument(argument, arg);
    skip_spaces(&p);

    if (!*arg) {
        mob_log(ch, "mechoaround called with no argument");
        return;
    }

    if (!(victim = find_char_for_mtrig(ch, arg))) {
        sprintf(buf, "mechoaround: victim (%s) does not exist", arg);
        mob_log(ch, buf);
        return;
    }

    sub_write(p, victim, TRUE, TO_ROOM);
}

/* sends the message to only the victim */
ACMD(do_msend) {
    char arg[MAX_INPUT_LENGTH];
    char_data *victim;
    char *p;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    p = one_argument(argument, arg);
    skip_spaces(&p);

    if (!*arg) {
        mob_log(ch, "msend called with no argument");
        return;
    }

    if (!(victim = find_char_for_mtrig(ch, arg))) {
        sprintf(buf, "msend: victim (%s) does not exist", arg);
        mob_log(ch, buf);
        return;
    }

    sub_write(p, victim, TRUE, TO_CHAR);
}

/* prints the message to the room at large */
ACMD(do_mecho) {
    char *p;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (!*argument) {
        mob_log(ch, "mecho called with no arguments");
        return;
    }
    p = argument;
    skip_spaces(&p);

    sub_write(p, ch, TRUE, TO_ROOM);
}

/*
** run a room trigger..particularly useful when the mob has just
** croaked (so cant use a speech trig)
*/
ACMD(do_m_run_room_trig) {
    int thisrm, trignm, found = 0;
    trig_data *t;
    struct script_data *sc;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (!*argument) {
        mob_log(ch, "m_run_room_trig called with no argument");
        return;
    }

    /* trigger must be in current room */
    thisrm = ch->in_room;
    trignm = atoi(argument);
    if (SCRIPT(&(world[thisrm]))) {
        sc = SCRIPT(&(world[thisrm]));
        for (t = TRIGGERS(sc); t; t = t->next) {
            if (GET_TRIG_VNUM(t) == trignm) {
                found = 1;
                break;
            }
        }

        if (found == 1) {
            room_data *room = &world[thisrm];
            /* found the right trigger, now run it */
            script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        } else {
            char buf[MAX_INPUT_LENGTH];
            sprintf(buf, "m_run_room_trig finds no such trigger %d in room %d\n", trignm, world[thisrm].vnum);
            mob_log(ch, buf);
        }
    }
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
ACMD(do_mload) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0;
    char_data *mob;
    obj_data *object;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (ch->desc && GET_LEVEL(ch->desc->original) < LVL_IMPL)
        return;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
        mob_log(ch, "mload: bad syntax");
        return;
    }

    if (is_abbrev(arg1, "mob")) {
        if ((mob = read_mobile(number, VIRTUAL)) == NULL) {
            mob_log(ch, "mload: bad mob vnum");
            return;
        }
        char_to_room(mob, IN_ROOM(ch));
        load_mtrigger(mob);
    }

    else if (is_abbrev(arg1, "obj")) {
        if ((object = read_object(number, VIRTUAL)) == NULL) {
            mob_log(ch, "mload: bad object vnum");
            return;
        }
        /* Reuse arg1 for third argument: force mload to room */
        any_one_arg(argument, arg1);
        if (!CAN_WEAR(object, ITEM_WEAR_TAKE) || !str_cmp(arg1, "room"))
            obj_to_room(object, IN_ROOM(ch));
        else
            obj_to_char(object, ch);
    }

    else
        mob_log(ch, "mload: bad type");
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge) {
    char arg[MAX_INPUT_LENGTH];
    char_data *victim;
    obj_data *obj;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        /* 'purge' */
        char_data *vnext;
        obj_data *obj_next;

        for (victim = world[IN_ROOM(ch)].people; victim; victim = vnext) {
            vnext = victim->next_in_room;
            if (IS_NPC(victim) && victim != ch)
                fullpurge_char(victim);
        }

        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj_next) {
            obj_next = obj->next_content;
            extract_obj(obj);
        }

        return;
    }

    victim = find_char_for_mtrig(ch, arg);

    if (victim == NULL) {
        if ((obj = find_obj_for_mtrig(ch, arg))) {
            extract_obj(obj);
        } else
            mob_log(ch, "mpurge: bad argument");

        return;
    }

    if (!IS_NPC(victim)) {
        mob_log(ch, "mpurge: purging a PC");
        return;
    }

    fullpurge_char(victim);
}

/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto) {
    char arg[MAX_INPUT_LENGTH];
    int location;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "mgoto called with no argument");
        return;
    }

    if ((location = find_mob_target_room(ch, arg)) == NOWHERE) {
        mob_log(ch, "mgoto: invalid location");
        return;
    }

    char_from_room(ch);
    char_to_room(ch, location);
}

/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat) {
    char arg[MAX_INPUT_LENGTH];
    int location;
    int original;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    argument = one_argument(argument, arg);

    if (!*arg || !*argument) {
        mob_log(ch, "mat: bad argument");
        return;
    }

    if ((location = find_mob_target_room(ch, arg)) == NOWHERE) {
        mob_log(ch, "mat: invalid location");
        return;
    }

    original = IN_ROOM(ch);
    char_from_room(ch);
    char_to_room(ch, location);
    command_interpreter(ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    if (IN_ROOM(ch) == location) {
        char_from_room(ch);
        char_to_room(ch, original);
    }
}

/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int target;
    char_data *vict, *next_ch;

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        mob_log(ch, "mteleport: bad syntax");
        return;
    }

    target = find_mob_target_room(ch, arg2);

    if (target == NOWHERE)
        mob_log(ch, "mteleport target is an invalid room");

    else if (!str_cmp(arg1, "all")) {
        if (target == IN_ROOM(ch)) {
            mob_log(ch, "mteleport all target is itself");
            return;
        }

        for (vict = world[IN_ROOM(ch)].people; vict; vict = next_ch) {
            next_ch = vict->next_in_room;

            if (GET_LEVEL(vict) < LVL_IMMORT) {
                dismount_char(vict);
                char_from_room(vict);
                char_to_room(vict, target);
            }
        }
    } else {
        if (!(vict = find_char_for_mtrig(ch, arg1))) {
            sprintf(buf, "mteleport: victim (%s) does not exist", arg1);
            mob_log(ch, buf);
            return;
        }

        if (GET_LEVEL(vict) < LVL_IMMORT) {
            dismount_char(vict);
            char_from_room(vict);
            char_to_room(vict, target);
        }
    }
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce) {
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
        return;

    argument = one_argument(argument, arg);

    if (!*arg || !*argument) {
        mob_log(ch, "mforce: bad syntax");
        return;
    }

    if (!str_cmp(arg, "all")) {
        struct descriptor_data *i;
        char_data *vch;

        for (i = descriptor_list; i; i = i->next) {
            if ((i->character != ch) && !i->connected && (IN_ROOM(i->character) == IN_ROOM(ch))) {
                vch = i->character;
                if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) && GET_LEVEL(vch) < LVL_IMMORT) {
                    command_interpreter(vch, argument);
                }
            }
        }
    } else {
        char_data *victim;

        if (!(victim = find_char_for_mtrig(ch, arg))) {
            mob_log(ch, "mforce: no such victim");
            return;
        }

        if (victim == ch) {
            mob_log(ch, "mforce: forcing self");
            return;
        }

        if (GET_LEVEL(victim) < LVL_IMMORT)
            command_interpreter(victim, argument);
    }
}

/* increases the target's exp */
ACMD(do_mexp) {
    char_data *victim;
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
        return;

    two_arguments(argument, name, amount);

    if (!*name || !*amount) {
        mob_log(ch, "mexp: too few arguments");
        return;
    }

    if (!(victim = find_char_for_mtrig(ch, name))) {
        sprintf(buf, "mexp: victim (%s) does not exist", name);
        mob_log(ch, buf);
        return;
    }

    gain_exp(victim, atoi(amount), GAIN_IGNORE_LEVEL_BOUNDARY | GAIN_IGNORE_LOCATION);
}

/* increases the target's gold */
ACMD(do_mgold) {
    char_data *victim;
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
        return;

    two_arguments(argument, name, amount);

    if (!*name || !*amount) {
        mob_log(ch, "mgold: too few arguments");
        return;
    }

    if (!(victim = find_char_for_mtrig(ch, name))) {
        sprintf(buf, "mgold: victim (%s) does not exist", name);
        mob_log(ch, buf);
        return;
    }

    if ((GET_GOLD(victim) += atoi(amount)) < 0) {
        mob_log(ch, "mgold subtracting more gold than character has");
        GET_GOLD(victim) = 0;
    }
}

ACMD(do_mob_log) {

    char errbuf[MAX_STRING_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
        return;

    snprintf(errbuf, MAX_STRING_LENGTH, "ERROR mob %d (%s): %s", GET_MOB_VNUM(ch), GET_NAME(ch), argument);

    mob_log(ch, argument);
}

/*
 * do_quest
 *
 * descr:	the controlling routine, this is what will get called by the
 * gods and the mobs using the following structure: quest <cmd> <qname> <player>
 * 		e.g. quest start new_quest zzur
 */

ACMD(do_quest) {
    struct trig_data *t = NULL;

    /*
     * Normal rules for execution by mobs/players, but deities can
     * use this command too.
     */
    if (!MOB_OR_IMPL(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (MOB_FLAGGED(ch, MOB_ANIMATED) || EFF_FLAGGED(ch, EFF_CHARM)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    /* If this is running in a trigger, try to find the trigger its running
       in so that we can pass it to perform_quest */
    if (SCRIPT(ch))
        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
            if (t->running)
                break;

    perform_quest(t, argument, ch, NULL, NULL);
}

/* Save a player. */
ACMD(do_msave) {
    char arg[MAX_INPUT_LENGTH];

    if (!MOB_OR_IMPL(ch)) {
        send_to_char("Huh?!?\r\n", ch);
        return;
    }

    if (EFF_FLAGGED(ch, EFF_CHARM))
        return;

    if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
        return;

    argument = one_argument(argument, arg);

    if (!*arg) {
        mob_log(ch, "msave: bad syntax");
        return;
    }

    if (!str_cmp(arg, "all")) {
        struct descriptor_data *i;
        char_data *vch;

        for (i = descriptor_list; i; i = i->next) {
            if ((i->character != ch) && !i->connected && (IN_ROOM(i->character) == IN_ROOM(ch))) {
                vch = i->character;
                if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) && GET_LEVEL(vch) < LVL_IMMORT) {
                    save_player(vch);
                }
            }
        }
    } else {
        char_data *victim;

        if (!(victim = find_char_for_mtrig(ch, arg))) {
            mob_log(ch, "msave: no such victim");
            return;
        }

        if (victim == ch) {
            mob_log(ch, "msave: forcing self");
            return;
        }

        if (IS_NPC(victim)) {
            sprintf(buf, "msave: cannot save NPC %s", arg);
            mob_log(ch, buf);
            return;
        }

        if (GET_LEVEL(victim) < LVL_IMMORT) {
            save_player(victim);
        }
    }
}

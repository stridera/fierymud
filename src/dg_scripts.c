/***************************************************************************
 * $Id: dg_scripts.c,v 1.130 2009/06/10 20:14:48 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: dg_scripts.c                                   Part of FieryMUD *
 *  Usage: contains general functions for using scripts.                   *
 *  $Author: myc $                                                         *
 *  $Date: 2009/06/10 20:14:48 $                                           *
 *  $Revision: 1.130 $                                                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *                                                                         *
 * This code was received origonally from HubisMUD in 1998 and no lable or *
 * claim of ownership or copyright was made anywhere in the file.          *
 ***************************************************************************/

#define __DG_SCRIPTS_C__

#include "dg_scripts.h"

#include "casting.h"
#include "charsize.h"
#include "clan.h"
#include "class.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "events.h"
#include "exits.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "modify.h"
#include "olc.h"
#include "quest.h"
#include "races.h"
#include "screen.h"
#include "skills.h"
#include "strings.h"
#include "structs.h"
#include "sysdep.h"
#include "trophy.h"
#include "utils.h"

#define PULSES_PER_MUD_HOUR (SECS_PER_MUD_HOUR * PASSES_PER_SEC)

/* external functions */
int find_target_room(char_data *ch, char *rawroomstr);
int obj_room(obj_data *obj);
int is_empty(int zone_nr);
int find_target_room(struct char_data *ch, char *rawroomstr);
trig_data *read_trigger(int nr);
void extract_trigger(struct trig_data *trig);
int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc, trig_data *trig, int type);
extern int find_zone(int num);
int vnumargs(struct char_data *ch, char *argument, int *first, int *second);
int group_size(struct char_data *ch);
int find_talent_num(char *name, int should_restrict);

/* function protos from this file */
int script_driver(void *go_address, trig_data *trig, int type, int mode);
void script_log(struct trig_data *t, char *msg);
struct cmdlist_element *find_done(struct cmdlist_element *cl);
struct cmdlist_element *find_case(struct trig_data *trig, struct cmdlist_element *cl, void *go, struct script_data *sc,
                                  int type, char *cond);
void var_subst(void *go, struct script_data *sc, trig_data *trig, int type, char *line, char *buf);

/* local structures */
struct wait_event_data {
    trig_data *trigger;
    void *go;
    int type;
};

struct trig_data *trigger_list = NULL; /* all attached triggers */

int find_real_zone_by_room(room_num vznum) {
    int bot, top, mid;
    int low, high;

    bot = 0;
    top = top_of_zone_table;

    /* perform binary search on zone-table */
    for (;;) {
        mid = (bot + top) / 2;

        /* Upper/lower bounds of the zone. */
        low = zone_table[mid].number * 100;
        high = zone_table[mid].top;

        if (low <= vznum && vznum <= high)
            return mid;
        if (bot >= top)
            return NOWHERE;
        if (low > vznum)
            top = mid - 1;
        else
            bot = mid + 1;
    }
}

int real_zone(int zvnum) {
    int i;

    for (i = 0; i < top_of_zone_table; i++)
        if (zone_table[i].number == zvnum)
            return i;

    return -1;
}

/************************************************************
 * search by number routines                                *
 ************************************************************/

/* return char with UID n */
struct char_data *find_char(int n) {
    struct char_data *ch;

    for (ch = character_list; ch; ch = ch->next)
        if (GET_ID(ch) == n)
            return (ch);

    return NULL;
}

/* return object with UID n */
obj_data *find_obj(int n) {
    obj_data *i;

    for (i = object_list; i; i = i->next)
        if (n == GET_ID(i))
            return i;

    return NULL;
}

/* return room with UID n */
room_data *find_room(int n) {
    n -= ROOM_ID_BASE;

    if ((n >= 0) && (n <= top_of_world))
        return &world[n];

    return NULL;
}

/************************************************************
 * generic searches based only on name                      *
 ************************************************************/

/* finds room by with name.  returns NULL if not found */
room_data *get_room(char *name) {
    int nr;

    if (*name == UID_CHAR)
        return find_room(atoi(name + 1));
    else if (isdigit(*name) && (nr = real_room(atoi(name))) != NOWHERE)
        return &world[nr];
    else
        return NULL;
}

/* finds room rnum by name.  returns NOWHERE if not found */
int get_room_location(char *name) {
    if (*name == UID_CHAR) {
        int num = atoi(name + 1) - ROOM_ID_BASE;
        if (num >= 0 && num <= top_of_world)
            return num;
    }

    if (isdigit(*name) && !strchr(name, '.'))
        return real_room(atoi(name));

    return NOWHERE;
}

MATCH_CHAR_FUNC(match_dg_vis_char_by_id) { return (GET_ID(ch) == context->number && !GET_INVIS_LEV(ch)); }

static MATCH_CHAR_FUNC(match_dg_vis_char_by_name) {
    if (!GET_INVIS_LEV(ch))
        if (isname(context->string, GET_NAMELIST(ch)))
            if (--context->number <= 0)
                return TRUE;
    return FALSE;
}

struct find_context find_dg_by_name(char *name) {
    struct find_context context = find_by_name(name);
    if (*name == UID_CHAR)
        context.char_func = match_dg_vis_char_by_id;
    else
        context.char_func = match_dg_vis_char_by_name;
    return context;
}

/* checks every PLUSE_SCRIPT for random triggers */
void script_trigger_check(void) {
    char_data *ch;
    obj_data *obj;
    struct room_data *room = NULL;
    int nr;
    struct script_data *sc;

    for (ch = character_list; ch; ch = ch->next) {
        if (SCRIPT(ch)) {
            sc = SCRIPT(ch);

            if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
                (!is_empty(world[IN_ROOM(ch)].zone) || IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
                random_mtrigger(ch);
        }
    }

    for (obj = object_list; obj; obj = obj->next) {
        if (SCRIPT(obj)) {
            sc = SCRIPT(obj);

            if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
                random_otrigger(obj);
        }
    }

    for (nr = 0; nr <= top_of_world; nr++) {
        if (SCRIPT(&world[nr])) {
            room = &world[nr];
            sc = SCRIPT(room);

            if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
                (!is_empty(room->zone) || IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
                random_wtrigger(room);
        }
    }
}

EVENTFUNC(trig_wait_event) {
    struct wait_event_data *wait_event_obj = (struct wait_event_data *)event_obj;
    trig_data *trig;
    void *go;
    int type;

    trig = wait_event_obj->trigger;
    go = wait_event_obj->go;
    type = wait_event_obj->type;

    GET_TRIG_WAIT(trig) = NULL;

    script_driver(&go, trig, type, TRIG_RESTART);
    return EVENT_FINISHED;
}

/* wait for casts...*/
void pause_while_casting(void *go, trig_data *trig, int type, struct cmdlist_element *cl) {
    struct wait_event_data *wait_event_obj;
    long time = 10L;

    CREATE(wait_event_obj, struct wait_event_data, 1);
    wait_event_obj->trigger = trig;
    wait_event_obj->go = go;
    wait_event_obj->type = type;

    GET_TRIG_WAIT(trig) = event_create(EVENT_TRIGGER_WAIT, trig_wait_event, wait_event_obj, TRUE, NULL, time);
    trig->curr_state = cl;
}

void do_stat_trigger(struct char_data *ch, trig_data *trig) {
    struct cmdlist_element *cmd_list;
    char sb[MAX_STRING_LENGTH_BIG];

    if (!trig) {
        log("SYSERR: NULL trigger passed to do_stat_trigger.");
        return;
    }

    get_char_cols(ch);

    sprintf(sb, "Trigger Name: '%s%s%s',  VNum: [%s%5d%s], RNum: [%5d]\r\n", yel, GET_TRIG_NAME(trig), nrm, grn,
            GET_TRIG_VNUM(trig), nrm, GET_TRIG_RNUM(trig));
    send_to_char(sb, ch);

    if (trig->attach_type == OBJ_TRIGGER) {
        send_to_char("Trigger Intended Assignment: Objects\r\n", ch);
        sprintbit(GET_TRIG_TYPE(trig), otrig_types, buf);
    } else if (trig->attach_type == WLD_TRIGGER) {
        send_to_char("Trigger Intended Assignment: Rooms\r\n", ch);
        sprintbit(GET_TRIG_TYPE(trig), wtrig_types, buf);
    } else {
        send_to_char("Trigger Intended Assignment: Mobiles\r\n", ch);
        sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);
    }

    sprintf(sb, "Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n", buf, GET_TRIG_NARG(trig),
            ((GET_TRIG_ARG(trig) && *GET_TRIG_ARG(trig)) ? GET_TRIG_ARG(trig) : "None"));

    strcat(sb, "Commands:\r\n\r\n");

    cmd_list = trig->cmdlist;
    while (cmd_list) {
        if (cmd_list->cmd) {
            strcat(sb, escape_ansi(cmd_list->cmd));
            strcat(sb, "\r\n");
        }
        cmd_list = cmd_list->next;
    }
    page_string(ch, sb);
}

/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name) {
    char_data *ch;
    obj_data *obj;

    if ((ch = find_char_in_world(find_by_name(uid))))
        strcpy(name, GET_NAMELIST(ch));
    else if ((obj = find_obj_in_world(find_by_name(uid))))
        strcpy(name, obj->name);
    else
        sprintf(name, "uid = %s, (not found)", uid + 1);
}

/* general function to display stats on script sc */
void script_stat(char_data *ch, char *buf, struct script_data *sc) {
    struct trig_var_data *tv;
    trig_data *t;
    char name[MAX_INPUT_LENGTH];
    int found = 0;
    extern char *t_listdisplay(int nr, int index);

    get_char_cols(ch);

    buf += sprintf(buf, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");

    for (tv = sc->global_vars; tv; tv = tv->next) {
        if (*(tv->value) == UID_CHAR) {
            find_uid_name(tv->value, name);
            buf += sprintf(buf, "    %15s:  %s\r\n", tv->name, name);
        } else
            buf += sprintf(buf, "    %15s:  %s\r\n", tv->name, tv->value);
    }

    for (t = TRIGGERS(sc); t; t = t->next) {
        buf += sprintf(buf, "%s", t_listdisplay(t->nr, ++found));

#if 1
        if (GET_TRIG_WAIT(t)) {
            buf += sprintf(buf,
                           "  Wait: %ld, Current line: %s\r\n"
                           "  Variables: %s\r\n",
                           event_time(GET_TRIG_WAIT(t)), t->curr_state->cmd, GET_TRIG_VARS(t) ? "" : "None");

            for (tv = GET_TRIG_VARS(t); tv; tv = tv->next) {
                if (*(tv->value) == UID_CHAR) {
                    find_uid_name(tv->value, name);
                    buf += sprintf(buf, "    %15s:  %s\r\n", tv->name, name);
                } else
                    buf += sprintf(buf, "    %15s:  %s\r\n", tv->name, tv->value);
            }
        }
#endif
    }
}

void do_sstat_room(struct char_data *ch, char *buf, struct room_data *rm) {
    strcpy(buf, "Script information:\r\n");
    if (SCRIPT(rm))
        script_stat(ch, buf + strlen(buf), SCRIPT(rm));
    else
        strcat(buf, "  None.\r\n");
}

void do_sstat_object(char_data *ch, char *buf, obj_data *j) {
    strcpy(buf, "Script information:\r\n");
    if (!SCRIPT(j)) {
        strcat(buf, "  None.\r\n");
        return;
    }

    script_stat(ch, buf + strlen(buf), SCRIPT(j));
}

void do_sstat_character(char_data *ch, char *buf, char_data *k) {
    strcpy(buf, "Script information:\r\n");
    if (!SCRIPT(k)) {
        strcat(buf, "  None.\r\n");
        return;
    }

    script_stat(ch, buf + strlen(buf), SCRIPT(k));
}

/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(struct script_data *sc, trig_data *t, int loc) {
    trig_data *i;
    int n;

    for (n = loc, i = TRIGGERS(sc); i && i->next && (n != 0); n--, i = i->next)
        ;

    if (!loc) {
        t->next = TRIGGERS(sc);
        TRIGGERS(sc) = t;
    } else if (!i)
        TRIGGERS(sc) = t;
    else {
        t->next = i->next;
        i->next = t;
    }

    SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);

    t->next_in_world = trigger_list;
    trigger_list = t;
}

ACMD(do_attach) {
    char_data *victim;
    obj_data *object;
    trig_data *trig;
    char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
    char loc_name[MAX_INPUT_LENGTH];
    int loc, room, tn, rn;

    argument = two_arguments(argument, arg, trig_name);
    two_arguments(argument, targ_name, loc_name);

    if (!*arg || !*targ_name || !*trig_name) {
        send_to_char("Usage: attach { mtr | otr | wtr } { trigger } { name } [ location ]\r\n",
                     ch);
        return;
    }

    tn = atoi(trig_name);
    loc = (*loc_name) ? atoi(loc_name) : -1;

    if (is_abbrev(arg, "mtr")) {
        if ((victim = find_char_around_char(ch, find_vis_by_name(ch, targ_name)))) {
            if (IS_NPC(victim)) {

                /* have a valid mob, now get trigger */
                rn = real_trigger(tn);
                if ((rn >= 0) && (trig = read_trigger(rn))) {

                    if (!SCRIPT(victim))
                        CREATE(SCRIPT(victim), struct script_data, 1);
                    add_trigger(SCRIPT(victim), trig, loc);

                    sprintf(buf, "Trigger %d (%s) attached to %s.\r\n", tn, GET_TRIG_NAME(trig), GET_SHORT(victim));
                    send_to_char(buf, ch);
                } else
                    send_to_char("That trigger does not exist.\r\n", ch);
            } else
                send_to_char("Players can't have scripts.\r\n", ch);
        } else
            send_to_char("That mob does not exist.\r\n", ch);
    }

    else if (is_abbrev(arg, "otr")) {
        if ((object = find_obj_around_char(ch, find_vis_by_name(ch, targ_name)))) {

            /* have a valid obj, now get trigger */
            rn = trig_index[tn] ? tn : -1;
            if ((rn >= 0) && (trig = read_trigger(rn))) {

                if (!SCRIPT(object))
                    CREATE(SCRIPT(object), struct script_data, 1);
                add_trigger(SCRIPT(object), trig, loc);

                sprintf(buf, "Trigger %d (%s) attached to %s.\r\n", tn, GET_TRIG_NAME(trig),
                        (object->short_description ? object->short_description : object->name));
                send_to_char(buf, ch);
            } else
                send_to_char("That trigger does not exist.\r\n", ch);
        } else
            send_to_char("That object does not exist.\r\n", ch);
    }

    else if (is_abbrev(arg, "wtr")) {
        if (isdigit(*targ_name) && !strchr(targ_name, '.')) {
            if ((room = find_target_room(ch, targ_name)) != NOWHERE) {

                /* have a valid room, now get trigger */
                rn = trig_index[tn] ? tn : -1;
                if ((rn >= 0) && (trig = read_trigger(rn))) {

                    if (!(world[room].script))
                        CREATE(world[room].script, struct script_data, 1);
                    add_trigger(world[room].script, trig, loc);

                    sprintf(buf, "Trigger %d (%s) attached to room %d.\r\n", tn, GET_TRIG_NAME(trig), world[room].vnum);
                    send_to_char(buf, ch);
                } else
                    send_to_char("That trigger does not exist.\r\n", ch);
            }
        } else
            send_to_char("You need to supply a room number.\r\n", ch);
    }

    else
        send_to_char("Please specify 'mtr', otr', or 'wtr'.\r\n", ch);
}

/* adds a variable with given name and value to trigger */
void add_var(struct trig_var_data **var_list, const char *name, const char *value) {
    struct trig_var_data *vd;

    for (vd = *var_list; vd && str_cmp(vd->name, name); vd = vd->next)
        ;

    if (vd) {
        free(vd->value);
        CREATE(vd->value, char, strlen(value) + 1);
        strcpy(vd->value, value);
    }

    else {
        CREATE(vd, struct trig_var_data, 1);

        CREATE(vd->name, char, strlen(name) + 1);
        strcpy(vd->name, name);

        CREATE(vd->value, char, strlen(value) + 1);
        strcpy(vd->value, value);

        vd->next = *var_list;
        *var_list = vd;
    }
}

/*
 *  removes the trigger specified by name, and the script of o if
 *  it removes the last trigger.  name can either be a number, or
 *  a 'silly' name for the trigger, including things like 2.beggar-death.
 *  returns 0 if did not find the trigger, otherwise 1.  If it matters,
 *  you might need to check to see if all the triggers were removed after
 *  this function returns, in order to remove the script.
 */
int remove_trigger(struct script_data *sc, char *name) {
    trig_data *i, *j;
    int num = 0, string = FALSE, n;
    char *cname;

    if (!sc)
        return 0;

    if ((cname = strstr(name, ".")) || (!isdigit(*name))) {
        string = TRUE;
        if (cname) {
            *cname = '\0';
            num = atoi(name);
            name = ++cname;
        }
    } else
        num = atoi(name);

    for (n = 0, j = NULL, i = TRIGGERS(sc); i; j = i, i = i->next) {
        if (string) {
            if (isname(name, GET_TRIG_NAME(i)))
                if (++n >= num)
                    break;
        }

        else if (++n >= num)
            break;
    }

    if (i) {
        if (j) {
            j->next = i->next;
            extract_trigger(i);
        }

        /* this was the first trigger */
        else {
            TRIGGERS(sc) = i->next;
            extract_trigger(i);
        }

        /* update the script type bitvector */
        SCRIPT_TYPES(sc) = 0;
        for (i = TRIGGERS(sc); i; i = i->next)
            SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(i);

        return 1;
    } else
        return 0;
}

ACMD(do_detach) {
    char_data *victim = NULL;
    obj_data *object = NULL;
    struct room_data *room;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
    char *trigger = 0;

    argument = two_arguments(argument, arg1, arg2);
    one_argument(argument, arg3);

    if (!*arg1 || !*arg2) {
        send_to_char("Usage: detach [ mob | object ] { target } { trigger | 'all' }\r\n", ch);
        return;
    }

    if (!str_cmp(arg1, "room")) {
        room = &world[IN_ROOM(ch)];
        if (!SCRIPT(room))
            send_to_char("This room does not have any triggers.\r\n", ch);
        else if (!str_cmp(arg2, "all")) {
            extract_script(SCRIPT(room));
            SCRIPT(room) = NULL;
            send_to_char("All triggers removed from room.\r\n", ch);
        }

        else if (remove_trigger(SCRIPT(room), arg2)) {
            send_to_char("Trigger removed.\r\n", ch);
            if (!TRIGGERS(SCRIPT(room))) {
                extract_script(SCRIPT(room));
                SCRIPT(room) = NULL;
            }
        } else
            send_to_char("That trigger was not found.\r\n", ch);
    }

    else {
        if (is_abbrev(arg1, "mob")) {
            if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg2))))
                send_to_char("No such mobile around.\r\n", ch);
            else if (!*arg3)
                send_to_char("You must specify a trigger to remove.\r\n", ch);
            else
                trigger = arg3;
        }

        else if (is_abbrev(arg1, "object")) {
            if (!(object = find_obj_around_char(ch, find_vis_by_name(ch, arg2))))
                send_to_char("No such object around.\r\n", ch);
            else if (!*arg3)
                send_to_char("You must specify a trigger to remove.\r\n", ch);
            else
                trigger = arg3;
        } else {
            if ((object = find_obj_in_eq(ch, NULL, find_vis_by_name(ch, arg1))))
                ;
            else if ((object = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
                ;
            else if ((victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg1))))
                ;
            else if ((object = find_obj_in_list(world[IN_ROOM(ch)].contents, find_vis_by_name(ch, arg1))))
                ;
            else if ((victim = find_char_around_char(ch, find_vis_by_name(ch, arg1))))
                ;
            else if ((object = find_obj_in_world(find_vis_by_name(ch, arg1))))
                ;
            else
                send_to_char("Nothing around by that name.\r\n", ch);

            trigger = arg2;
        }

        if (victim) {
            if (!IS_NPC(victim))
                send_to_char("Players don't have triggers.\r\n", ch);

            else if (!SCRIPT(victim))
                send_to_char("That mob doesn't have any triggers.\r\n", ch);
            else if (!str_cmp(arg2, "all")) {
                extract_script(SCRIPT(victim));
                SCRIPT(victim) = NULL;
                sprintf(buf, "All triggers removed from %s.\r\n", GET_SHORT(victim));
                send_to_char(buf, ch);
            }

            else if (remove_trigger(SCRIPT(victim), trigger)) {
                send_to_char("Trigger removed.\r\n", ch);
                if (!TRIGGERS(SCRIPT(victim))) {
                    extract_script(SCRIPT(victim));
                    SCRIPT(victim) = NULL;
                }
            } else
                send_to_char("That trigger was not found.\r\n", ch);
        }

        else if (object) {
            if (!SCRIPT(object))
                send_to_char("That object doesn't have any triggers.\r\n", ch);

            else if (!str_cmp(arg2, "all")) {
                extract_script(SCRIPT(object));
                SCRIPT(object) = NULL;
                sprintf(buf, "All triggers removed from %s.\r\n",
                        object->short_description ? object->short_description : object->name);
                send_to_char(buf, ch);
            }

            else if (remove_trigger(SCRIPT(object), trigger)) {
                send_to_char("Trigger removed.\r\n", ch);
                if (!TRIGGERS(SCRIPT(object))) {
                    extract_script(SCRIPT(object));
                    SCRIPT(object) = NULL;
                }
            } else
                send_to_char("That trigger was not found.\r\n", ch);
        }
    }
}

/* frees memory associated with var */
void free_var_el(struct trig_var_data *var) {
    free(var->name);
    free(var->value);
    free(var);
}

/*
 * remove var name from var_list
 * returns 1 if found, else 0
 */
int remove_var(struct trig_var_data **var_list, char *name) {
    struct trig_var_data *i, *j;

    for (j = NULL, i = *var_list; i && str_cmp(name, i->name); j = i, i = i->next)
        ;

    if (i) {
        if (j) {
            j->next = i->next;
            free_var_el(i);
        } else {
            *var_list = i->next;
            free_var_el(i);
        }

        return 1;
    }

    return 0;
}

/*
 *  Logs any errors caused by scripts to the system log.
 *  Will eventually allow on-line view of script errors.
 */
void script_log(struct trig_data *t, char *msg) {
    char buf[256];

    if (t)
        snprintf(buf, 255, "ERROR trigger %d (%s): %s", GET_TRIG_VNUM(t), GET_TRIG_NAME(t), msg);
    else
        snprintf(buf, 255, "ERROR in trigger: %s", msg);
    mudlog(buf, NRM, LVL_GOD, TRUE);
}

/*
 * Takes a zone RNUM and returns a room RNUM.
 */
int get_random_room_in_zone(int znum) {
    int low, high, to_room;

    /* Find the lower room bound for the zone. */
    for (low = 0; low <= top_of_world && world[low].zone != znum; ++low)
        ;

    /* No rooms for this zone. */
    if (low > top_of_world)
        return NOWHERE;

    /* Find the upper room bound for the zone. */
    for (high = low; high <= top_of_world && world[high].zone == znum; ++high)
        ;
    --high;

    do {
        to_room = number(low, high);
    } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
             ROOM_FLAGGED(to_room, ROOM_GODROOM));

    return to_room;
}

#define UID_VAR(str, i) ((i) ? sprintf((str), "%c%ld", UID_CHAR, GET_ID(i)) : sprintf((str), "0"))
#define ROOM_UID_VAR(str, r) sprintf((str), "%c%ld", UID_CHAR, (long)r + ROOM_ID_BASE)

/* sets str to be the value of var.field */
void find_replacement(void *go, struct script_data *sc, trig_data *trig, int type, char *var, char *field, char *value,
                      char *str) {
    struct trig_var_data *vd;
    char_data *ch, *c = NULL;
    obj_data *obj, *o = NULL;
    struct room_data *room, *r = NULL;
    char *name;
    int num;

    if (!value)
        value = "";

    /*
     * First see if there is a local variable with the specified name.
     * This means that local variables take precedence and can 'mask'
     * globals and static variables.
     */
    for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
        if (!str_cmp(vd->name, var))
            break;

    /*
     * If no local variable was matched, see if there is a global variable
     * with the specified name.
     *
     * Some waitstates could crash the mud if sent here with sc == NULL.
     */
    if (!vd && sc)
        for (vd = sc->global_vars; vd; vd = vd->next)
            if (!str_cmp(vd->name, var))
                break;

    /*
     * Set 'self' variables for use below.  For example, if this is a mob
     * trigger, ch is the mob executing the trigger, and obj and room are
     * null.
     */
    switch (type) {
    case MOB_TRIGGER:
        ch = (char_data *)go;
        obj = NULL;
        room = NULL;
        break;
    case OBJ_TRIGGER:
        ch = NULL;
        obj = (obj_data *)go;
        room = NULL;
        break;
    case WLD_TRIGGER:
        ch = NULL;
        obj = NULL;
        room = (struct room_data *)go;
        break;
    default:
        log("SYSERR: find_replacement encountered invalid trig type (%d) in trig "
            "%d",
            type, GET_TRIG_VNUM(trig));
        *str = '\0';
        return;
    }

    /*
     * If no variable field is given, we can simply drop in the variable's
     * value.
     */
    if (!*field) {
        if (vd)
            strcpy(str, vd->value);
        else {
            if (!str_cmp(var, "self")) {
                switch (type) {
                case MOB_TRIGGER:
                    UID_VAR(str, ch);
                    break;
                case OBJ_TRIGGER:
                    UID_VAR(str, obj);
                    break;
                case WLD_TRIGGER:
                    ROOM_UID_VAR(str, real_room(room->vnum));
                    break;
                }
            }
            /* General scripting variable "damdone", which is the amount of damage
             * that was done by a wdamage, mdamage, or odamage command. */
            else if (!str_cmp(var, "damdone")) {
                sprintf(str, "%d", trig->damdone);
            } else
                *str = '\0';
        }
        return;
    }

    /*
     * If we found a local or global variable above, and a field is
     * being requested, we need to actually locate the character, object,
     * or room.
     */
    if (vd && (name = vd->value) && *name) {
        switch (type) {
        case MOB_TRIGGER:
            if ((o = find_obj_in_eq(ch, NULL, find_by_name(name))))
                ;
            else if ((o = find_obj_in_list(ch->carrying, find_by_name(name))))
                ;
            else if ((c = find_char_in_room(&world[ch->in_room], find_by_name(name))))
                ;
            else if ((o = find_obj_in_list(world[IN_ROOM(ch)].contents, find_by_name(name))))
                ;
            else if ((c = find_char_in_world(find_by_name(name))))
                ;
            else if ((o = find_obj_in_world(find_by_name(name))))
                ;
            else if ((r = get_room(name)))
                ;
            break;
        case OBJ_TRIGGER:
            if ((c = find_char_around_obj(obj, find_dg_by_name(name))))
                ;
            else if ((o = find_obj_around_obj(obj, find_by_name(name))))
                ;
            else if ((r = get_room(name)))
                ;
            break;
        case WLD_TRIGGER:
            if ((c = find_char_around_room(room, find_dg_by_name(name))))
                ;
            else if ((o = find_obj_around_room(room, find_by_name(name))))
                ;
            else if ((r = get_room(name)))
                ;
            break;
        }
    }

    /*
     * If no local or global variable named self was found above,
     * then we must be referring to the runner of the trigger.
     */
    else if (!str_cmp(var, "self")) {
        c = ch;
        o = obj;
        r = room;
    }

    /*
     * These are 'static' variables that do not go into the char/obj/room
     * section below.
     */
    else {

        if (!str_cmp(var, "time")) {
            if (!str_cmp(field, "hour"))
                sprintf(str, "%d", time_info.hours);
            else if (!str_cmp(field, "day"))
                sprintf(str, "%d", time_info.day);
            else if (!str_cmp(field, "month"))
                sprintf(str, "%d", time_info.month);
            else if (!str_cmp(field, "year"))
                sprintf(str, "%d", time_info.year);
            else if (!str_cmp(field, "stamp")) {
                num = time_info.year * SECS_PER_MUD_YEAR + time_info.month * SECS_PER_MUD_MONTH +
                      time_info.day * SECS_PER_MUD_DAY + time_info.hours * SECS_PER_MUD_HOUR;
                /* Only game-hour granularity is available in triggers. */
                num /= SECS_PER_MUD_HOUR;
                sprintf(str, "%d", num);
            } else {
                *str = '\0';
                sprintf(buf2, "Unknown time field '%s'", field);
                script_log(trig, buf2);
            }
        }

        else if (!str_cmp(var, "random")) {

            /* Pick a random character in the room */
            if (!str_cmp(field, "char")) {
                if (type == MOB_TRIGGER)
                    c = get_random_char_around(ch, RAND_DG_MOB);
                else if (type == OBJ_TRIGGER)
                    c = get_random_char_around(world[obj_room(obj)].people, RAND_DG_OBJ);
                else if (type == WLD_TRIGGER)
                    c = get_random_char_around(room->people, RAND_DG_WLD);

                UID_VAR(str, c);
            }

            /* Locate a random room globally */
            else if (!str_cmp(field, "room")) {
                do {
                    num = number(0, top_of_world);
                } while (ROOM_FLAGGED(num, ROOM_PRIVATE) || ROOM_FLAGGED(num, ROOM_DEATH) ||
                         ROOM_FLAGGED(num, ROOM_GODROOM));
                sprintf(str, "%d", world[num].vnum);
            }

            /* Pick a random room in the zone */
            else if (!str_cmp(field, "room_in_zone")) {
                if (type == MOB_TRIGGER && ch->in_room != NOWHERE)
                    num = world[ch->in_room].zone;
                else if (type == OBJ_TRIGGER && (num = obj_room(obj)) != NOWHERE)
                    num = world[num].zone;
                else if (type == WLD_TRIGGER)
                    num = room->zone;
                else
                    num = -1;
                if (num >= 0 && (num = get_random_room_in_zone(num)) >= 0)
                    sprintf(str, "%d", world[num].vnum);
                else
                    strcpy(str, "-1");
            }

            /* Generate a random number */
            else
                sprintf(str, "%d", ((num = atoi(field)) > 0) ? number(1, num) : 0);
        }

        /* Static functions */
        else if (!str_cmp(var, "get")) {
            /* %get.obj_shortdesc[VNUM]% */
            if (!str_cmp(field, "obj_shortdesc")) {
                if (is_positive_integer(value) && (num = real_object(atoi(value))) >= 0)
                    strcpy(str, obj_proto[num].short_description);
                else
                    sprintf(str, "[no description for object %s]", value);

            } else if (!str_cmp(field, "obj_noadesc")) {
                /* %get.obj_noadesc[VNUM]% */
                if (is_positive_integer(value) && (num = real_object(atoi(value))) >= 0)
                    strcpy(str, without_article(obj_proto[num].short_description));
                else
                    sprintf(str, "[no description for object %s]", value);

            } else if (!str_cmp(field, "obj_pldesc")) {
                /* %get.obj_pldesc[VNUM]% */
                if (is_positive_integer(value) && (num = real_object(atoi(value))) >= 0)
                    strcpy(str, pluralize(obj_proto[num].short_description));
                else
                    sprintf(str, "[no description for object %s]", value);

                /* %get.mob_shortdesc[VNUM]% */
            } else if (!str_cmp(field, "mob_shortdesc")) {
                if (is_positive_integer(value) && (num = real_mobile(atoi(value))) >= 0)
                    strcpy(str, mob_proto[num].player.short_descr);
                else
                    sprintf(str, "[no description for mobile %s]", value);

                /* %get.obj_count[VNUM]% is the number of objects with VNUM in game */
            } else if (!str_cmp(field, "obj_count")) {
                if ((num = real_object(atoi(value))) >= 0)
                    sprintf(str, "%d", obj_index[num].number);
                else
                    strcpy(str, "0");
                /* %get.mob_count[VNUM]% is the number of mobiles with VNUM in game */
            } else if (!str_cmp(field, "mob_count")) {
                if ((num = real_mobile(atoi(value))) >= 0)
                    sprintf(str, "%d", mob_index[num].number);
                else
                    strcpy(str, "0");
                /* %get.room[VNUM]% returns a UID variable pointing to that room */
            } else if (!str_cmp(field, "room")) {
                if ((num = real_room(atoi(value))) >= 0)
                    ROOM_UID_VAR(str, num);
                else
                    strcpy(str, "0");
                /* %get.people[VNUM]% is the number of people in room */
            } else if (!str_cmp(field, "people")) {
                if (is_positive_integer(value) && (num = real_room(atoi(value))) >= 0) {
                    ch = world[num].people;
                    for (num = 0; ch; ch = ch->next_in_room)
                        if (!GET_INVIS_LEV(ch))
                            ++num;
                    sprintf(str, "%d", num);
                } else {
                    *str = '\0';
                    sprintf(buf2, "get.people[%s]: room '-1' does not exist", value);
                    script_log(trig, buf2);
                }
            } else if (!str_cmp(field, "opposite_dir")) {
                if ((num = search_block(value, dirs, FALSE)) >= 0)
                    strcpy(str, dirs[rev_dir[num]]);
                else {
                    /*
                     * If they didn't give a valid direction, then reverse
                     * the string, lol.
                     */
                    for (num = strlen(value) - 1; num >= 0; --num)
                        *(str++) = *(value + num);
                    *str = '\0';
                }
            } else if (!str_cmp(field, "uidchar"))
                sprintf(str, "%c", UID_CHAR);
            else {
                *str = '\0';
                sprintf(buf2, "Unknown get field: '%s'", field);
                script_log(trig, buf2);
            }
        }

        /* String functions */
        else if (!str_cmp(var, "string")) {

            if (!str_cmp(field, "reverse")) {
                for (num = strlen(value) - 1; num >= 0; --num)
                    *(str++) = *(value + num);
                *str = '\0';
            }

            else if (!str_cmp(field, "length"))
                sprintf(str, "%d", strlen(value));

            else if (!str_cmp(field, "tolower")) {
                do {
                    *(str++) = LOWER(*(value++));
                } while (*value);
            }

            else if (!str_cmp(field, "toupper")) {
                do {
                    *(str++) = UPPER(*(value++));
                } while (*value);
            }

            else if (!str_cmp(field, "cap") || !str_cmp(field, "capitalize")) {
                strcpy(str, value);
                CAP(str);
            }

            else if (!str_cmp(field, "firstword"))
                any_one_arg(value, str);

            else {
                strcpy(str, value);
                sprintf(buf2, "Unknown string field: '%s'", field);
                script_log(trig, buf2);
            }

        }

        else
            *str = '\0';

        return;
    }

    /*
     * If a local or global variable was located above, or we are
     * accessing the 'self' variable, and we are trying to access
     * a UID variable subfield, then access the field!
     */
    if (c) {
        /* String identifiers */
        if (!str_cmp(field, "name"))
            strcpy(str, GET_SHORT(c) ? GET_SHORT(c) : GET_NAME(c));
        else if (!str_cmp(field, "p") || !str_cmp(field, "hisher"))
            strcpy(str, HSHR(c)); /* Possessive pronoun */
        else if (!str_cmp(field, "o") || !str_cmp(field, "himher"))
            strcpy(str, HMHR(c)); /* Objective pronoun */
        else if (!str_cmp(field, "n") || !str_cmp(field, "heshe"))
            strcpy(str, HSSH(c)); /* Nominative pronoun */
        else if (!str_cmp(field, "alias"))
            strcpy(str, GET_NAME(c));
        else if (!str_cmp(field, "title"))
            strcpy(str, GET_TITLE(c) ? GET_TITLE(c) : "");

        /* Identifying numbers */
        else if (!str_cmp(field, "vnum"))
            sprintf(str, "%d", GET_MOB_VNUM(c));
        else if (!str_cmp(field, "id"))
            sprintf(str, "%ld", GET_ID(c));

        /* Attributes */
        else if (!str_cmp(field, "sex") || !str_cmp(field, "gender"))
            strcpy(str, genders[(int)GET_SEX(c)]);
        else if (!str_cmp(field, "class")) {
            strcpy(str, CLASS_PLAINNAME(c));
            CAP(str);
        } else if (!str_cmp(field, "race"))
            strcpy(str, races[(int)GET_RACE(c)].name);
        else if (!str_cmp(field, "level"))
            sprintf(str, "%d", GET_LEVEL(c));

        else if (!str_cmp(field, "weight"))
            sprintf(str, "%d", GET_WEIGHT(c));
        else if (!str_cmp(field, "height"))
            sprintf(str, "%d", GET_HEIGHT(c));
        else if (!str_cmp(field, "size"))
            sprintf(str, "%s", SIZE_DESC(c));

        else if (!str_cmp(field, "cha"))
            sprintf(str, "%d", GET_VIEWED_CHA(c));
        else if (!str_cmp(field, "str"))
            sprintf(str, "%d", GET_VIEWED_STR(c));
        else if (!str_cmp(field, "int"))
            sprintf(str, "%d", GET_VIEWED_INT(c));
        else if (!str_cmp(field, "wis"))
            sprintf(str, "%d", GET_VIEWED_WIS(c));
        else if (!str_cmp(field, "con"))
            sprintf(str, "%d", GET_VIEWED_CON(c));
        else if (!str_cmp(field, "dex"))
            sprintf(str, "%d", GET_VIEWED_DEX(c));

        else if (!str_cmp(field, "real_cha"))
            sprintf(str, "%d", GET_CHA(c));
        else if (!str_cmp(field, "real_str"))
            sprintf(str, "%d", GET_STR(c));
        else if (!str_cmp(field, "real_int"))
            sprintf(str, "%d", GET_INT(c));
        else if (!str_cmp(field, "real_wis"))
            sprintf(str, "%d", GET_WIS(c));
        else if (!str_cmp(field, "real_con"))
            sprintf(str, "%d", GET_CON(c));
        else if (!str_cmp(field, "real_dex"))
            sprintf(str, "%d", GET_DEX(c));

        else if (!str_cmp(field, "hit"))
            sprintf(str, "%d", GET_HIT(c));
        else if (!str_cmp(field, "maxhit"))
            sprintf(str, "%d", GET_MAX_HIT(c));
        else if (!str_cmp(field, "move"))
            sprintf(str, "%d", GET_MOVE(c));
        else if (!str_cmp(field, "maxmove"))
            sprintf(str, "%d", GET_MAX_MOVE(c));
        else if (!str_cmp(field, "armor"))
            sprintf(str, "%d", GET_AC(c));
        else if (!str_cmp(field, "hitroll"))
            sprintf(str, "%d", GET_HITROLL(c));
        else if (!str_cmp(field, "damroll"))
            sprintf(str, "%d", GET_DAMROLL(c));
        else if (!str_cmp(field, "exp"))
            sprintf(str, "%ld", GET_EXP(c));
        else if (!str_cmp(field, "perception"))
            sprintf(str, "%ld", GET_PERCEPTION(c));
        else if (!str_cmp(field, "hiddenness"))
            sprintf(str, "%ld", GET_HIDDENNESS(c));
        else if (!str_cmp(field, "align") || !str_cmp(field, "alignment"))
            sprintf(str, "%d", GET_ALIGNMENT(c));

        else if (is_coin_name(field, PLATINUM))
            sprintf(str, "%d", GET_PLATINUM(c));
        else if (is_coin_name(field, GOLD))
            sprintf(str, "%d", GET_GOLD(c));
        else if (is_coin_name(field, SILVER))
            sprintf(str, "%d", GET_SILVER(c));
        else if (is_coin_name(field, COPPER))
            sprintf(str, "%d", GET_COPPER(c));

        /* Flags */
        else if (!str_cmp(field, "flags")) {
            *str = '\0';
            if (IS_NPC(c)) /* ACT flags */
                sprintflag(str, MOB_FLAGS(c), NUM_MOB_FLAGS, action_bits);
            else { /* concatenation of PLR and PRF flags */
                if (HAS_FLAGS(PLR_FLAGS(c), NUM_PLR_FLAGS) || !HAS_FLAGS(PRF_FLAGS(c), NUM_PRF_FLAGS))
                    sprintflag(str, PLR_FLAGS(c), NUM_PLR_FLAGS, player_bits);
                if (HAS_FLAGS(PRF_FLAGS(c), NUM_PRF_FLAGS))
                    sprintflag(str + strlen(str), PRF_FLAGS(c), NUM_PRF_FLAGS, preference_bits);
            }
        } else if (!str_cmp(field, "flagged")) {
            if (IS_NPC(c)) {
                if ((num = search_block(value, action_bits, FALSE)) >= 0)
                    strcpy(str, MOB_FLAGGED(c, num) ? "1" : "0");
                else {
                    strcpy(str, "0");
                    sprintf(buf2, "unrecognized NPC flag '%s' to %%%s.flagged[]%%", value, var);
                    script_log(trig, buf2);
                }
            } else {
                if ((num = search_block(value, player_bits, FALSE)) >= 0)
                    strcpy(str, PLR_FLAGGED(c, (1 << num)) ? "1" : "0");
                else if ((num = search_block(value, preference_bits, FALSE)) >= 0)
                    strcpy(str, PRF_FLAGGED(c, (1 << num)) ? "1" : "0");
                else {
                    strcpy(str, "0");
                    sprintf(buf2, "unrecognized player or preference flag '%s' to %%%s.flagged[]%%", value, var);
                    script_log(trig, buf2);
                }
            }
        } else if (!str_cmp(field, "aff_flags") || !str_cmp(field, "eff_flags"))
            sprintflag(str, EFF_FLAGS(c), NUM_EFF_FLAGS, effect_flags);

        else if (!str_cmp(field, "aff_flagged") || !str_cmp(field, "eff_flagged")) {
            if ((num = search_block(value, effect_flags, FALSE)) >= 0)
                strcpy(str, EFF_FLAGGED(c, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized effect flag '%s' to %%%s.eff_flagged[]%%", value, var);
                script_log(trig, buf2);
            }

        } else if (!str_cmp(field, "spells")) {
            struct effect *eff;
            *str = '\0';
            for (eff = c->effects; eff; eff = eff->next)
                if (eff->duration >= 0 && (!eff->next || eff->next->type != eff->type)) {
                    strcat(str, skills[eff->type].name);
                    strcat(str, " ");
                }
        } else if (!str_cmp(field, "has_spell")) {
            if ((num = find_talent_num(value, 0)) >= 0)
                strcpy(str, affected_by_spell(c, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized spell '%s' to %%%s.has_spell[]%%", value, var);
                script_log(trig, buf2);
            }
        }

        /* Character relationships */
        else if (!str_cmp(field, "fighting"))
            UID_VAR(str, FIGHTING(c));
        else if (!str_cmp(field, "hunting"))
            UID_VAR(str, HUNTING(c));
        else if (!str_cmp(field, "riding"))
            UID_VAR(str, RIDING(c));
        else if (!str_cmp(field, "ridden_by"))
            UID_VAR(str, RIDDEN_BY(c));
        else if (!str_cmp(field, "consented"))
            UID_VAR(str, CONSENT(c));
        else if (!str_cmp(field, "master"))
            UID_VAR(str, c->master);
        else if (!str_cmp(field, "next_in_room")) {
            /* Skip any wiz-invis folks */
            while (c->next_in_room && GET_INVIS_LEV(c->next_in_room))
                c = c->next_in_room;
            UID_VAR(str, c->next_in_room);
        } else if (!str_cmp(field, "group_size"))
            sprintf(str, "%d", group_size(c));
        else if (!str_cmp(field, "group_member")) {
            ch = c->group_master ? c->group_master : c;

            num = atoi(value);

            if (!IS_GROUPED(ch) || num <= 0)
                UID_VAR(str, c);
            else if (num == 1)
                UID_VAR(str, ch);
            else {
                struct group_type *g;
                for (g = ch->groupees; g; g = g->next) {
                    if (--num > 1)
                        continue;
                    UID_VAR(str, g->groupee);
                    break;
                }
                if (num > 1)
                    strcpy(str, "0");
            }
        }

        /* Quests */
        else if (!str_cmp(field, "quest_variable")) {
            if (!*value) {
                script_log(trig, "quest_variable called without specifying a quest");
                strcpy(str, "0");
            } else if (IS_NPC(c))
                strcpy(str, "0");
            else {
                char *varptr;
                for (varptr = value; *varptr && *varptr != ':'; ++varptr)
                    ;
                *(varptr++) = '\0';
                if (!*varptr) {
                    script_log(trig, "quest_variable called without specifying a variable");
                    strcpy(str, "0");
                } else
                    strcpy(str, get_quest_variable(c, value, varptr));
            }
        }

        else if (!str_cmp(field, "quest_stage")) {
            if (!*value) {
                script_log(trig, "quest_stage called without specifying a quest");
                strcpy(str, "0");
            } else if (IS_NPC(c))
                strcpy(str, "0");
            else
                sprintf(str, "%d", quest_stage(c, value));
        }

        else if (!str_cmp(field, "has_completed")) {
            if (!*value) {
                script_log(trig, "has_completed called without specifying a quest");
                strcpy(str, "0");
            } else if (IS_NPC(c))
                strcpy(str, "0");
            else
                strcpy(str, has_completed_quest(value, c) ? "1" : "0");
        }

        else if (!str_cmp(field, "has_failed")) {
            if (!*value) {
                script_log(trig, "has_failed called without specifying a quest");
                strcpy(str, "0");
            } else if (IS_NPC(c))
                strcpy(str, "0");
            else
                strcpy(str, has_failed_quest(value, c) ? "1" : "0");
        }

        /* Object relationships */
        else if (!str_cmp(field, "inventory")) {
            if (!str_cmp(value, "count"))
                sprintf(str, "%d", IS_CARRYING_N(c));
            else if (*value) {
                /* An argument was given: find a specific object. */
                num = atoi(value);
                for (obj = c->carrying; obj; obj = obj->next_content)
                    if (GET_OBJ_VNUM(obj) == num) {
                        UID_VAR(str, obj);
                        break;
                    }
                if (!obj) /* No matching object found. */
                    strcpy(str, "0");
            } else
                /* No argument given: return the first inventory item */
                UID_VAR(str, c->carrying);
        } else if (!str_cmp(field, "worn")) {
            int pos;
            if (!str_cmp(value, "count")) {
                for (num = pos = 0; pos < NUM_WEARS; ++pos)
                    if (GET_EQ(c, pos))
                        ++num;
                sprintf(str, "%d", num);
            } else if ((pos = search_block(value, wear_positions, TRUE)) >= 0)
                UID_VAR(str, GET_EQ(c, pos));
            else
                strcpy(str, "0");
        } else if (!str_cmp(field, "wearing")) {
            if (is_positive_integer(value)) {
                int pos;
                num = atoi(value);
                for (pos = 0; pos < NUM_WEARS; ++pos)
                    if (GET_EQ(c, pos) && GET_OBJ_VNUM(GET_EQ(c, pos)) == num) {
                        UID_VAR(str, GET_EQ(c, pos));
                        break;
                    }
                /* Not found */
                if (pos >= NUM_WEARS)
                    strcpy(str, "0");
            } else
                strcpy(str, "0");
        }

        else if (!str_cmp(field, "position")) {
            strcpy(str, position_types[(int)GET_POS(c)]);
        } else if (!str_cmp(field, "stance"))
            strcpy(str, stance_types[(int)GET_STANCE(c)]);

        else if (!str_cmp(field, "room")) {
            if (IN_ROOM(c) >= 0 && IN_ROOM(c) <= top_of_world)
                sprintf(str, "%d", world[IN_ROOM(c)].vnum);
            else
                strcpy(str, "-1");
        }

        else if (!str_cmp(field, "talent") || !str_cmp(field, "skill")) {
            int talent = find_talent_num(value, 0);
            if (talent < 0)
                strcpy(str, "0");
            else
                sprintf(str, "%d", GET_SKILL(c, talent));
        }

        else if (!str_cmp(field, "clan")) {
            if (!IS_NPC(c) && GET_CLAN(c))
                strcpy(str, GET_CLAN(c)->name);
            else
                *str = '\0';
        } else if (!str_cmp(field, "clan_rank"))
            sprintf(str, "%d", IS_NPC(c) ? 0 : GET_CLAN_RANK(c));

        else if (!str_cmp(field, "can_be_seen"))
            strcpy(str, type == MOB_TRIGGER && !CAN_SEE(ch, c) ? "0" : "1");

        else if (!str_cmp(field, "trophy")) {
            if (IS_NPC(c))
                *str = '\0';
            else {
                struct trophy_node *node;
                num = value && *value ? atoi(value) : -1;
                *str = '\0';
                for (node = GET_TROPHY(c); node; node = node->next) {
                    /* Getting list of kills */
                    if (num < 0) {
                        if (node->kill_type == TROPHY_MOBILE)
                            sprintf(str, "%s%d ", str, node->id);
                    }
                    /* Looking for a specific mobile: give count if it exists */
                    else if (node->kill_type == TROPHY_MOBILE && node->id == num) {
                        sprintf(str, "%d", (int)node->amount);
                        break;
                    }
                }
                /* No mobiles found.  0 amount. */
                if (*str == '\0')
                    strcpy(str, "0");
            }
        }

        else {
            *str = '\0';
            sprintf(buf2, "Unknown char field: '%s'", field);
            script_log(trig, buf2);
        }
    }

    else if (o) {
        /* String identifiers */
        if (!str_cmp(field, "name"))
            strcpy(str, o->name);
        else if (!str_cmp(field, "shortdesc"))
            strcpy(str, o->short_description);
        else if (!str_cmp(field, "description"))
            strcpy(str, o->description);

        /* Identifying numbers */
        else if (!str_cmp(field, "vnum"))
            sprintf(str, "%d", GET_OBJ_VNUM(o));
        else if (!str_cmp(field, "type"))
            strcpy(str, OBJ_TYPE_NAME(o));
        else if (!str_cmp(field, "id"))
            sprintf(str, "%ld", GET_ID(o));

        /* Numerical attributes */
        else if (!str_cmp(field, "weight"))
            sprintf(str, "%.2f", o->obj_flags.weight);
        else if (!str_cmp(field, "cost"))
            sprintf(str, "%d", GET_OBJ_COST(o));
        else if (!str_cmp(field, "cost_per_day") || !str_cmp(field, "rent"))
            strcpy(str, "0");
        else if (!str_cmp(field, "level"))
            sprintf(str, "%d", GET_OBJ_LEVEL(o));
        else if (!str_cmp(field, "val0"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 0));
        else if (!str_cmp(field, "val1"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 1));
        else if (!str_cmp(field, "val2"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 2));
        else if (!str_cmp(field, "val3"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 3));
        else if (!str_cmp(field, "timer"))
            sprintf(str, "%d", GET_OBJ_TIMER(o));
        else if (!str_cmp(field, "decomp"))
            sprintf(str, "%d", GET_OBJ_DECOMP(o));
        else if (!str_cmp(field, "hiddenness"))
            sprintf(str, "%ld", GET_OBJ_HIDDENNESS(o));
        else if (!str_cmp(field, "affect") || !str_cmp(field, "effect")) {
            if (!is_positive_integer(value) || (num = atoi(value)) > 5)
                *str = '\0';
            else
                sprintf(str, "%+d %s", o->applies[num].modifier, apply_types[(int)o->applies[num].location]);
        } else if (!str_cmp(field, "affect_value") || !str_cmp(field, "effect_value"))
            sprintf(str, "%d", is_positive_integer(value) && (num = atoi(value) <= 5) ? o->applies[num].modifier : 0);

        /* Flags */
        else if (!str_cmp(field, "flags"))
            sprintflag(str, GET_OBJ_FLAGS(o), NUM_ITEM_FLAGS, extra_bits);
        else if (!str_cmp(field, "flagged")) {
            if ((num = search_block(value, extra_bits, FALSE)) >= 0)
                strcpy(str, OBJ_FLAGGED(o, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized object extra bit '%s' to %%%s.flagged[]%%", value, var);
                script_log(trig, buf2);
            }
        } else if (!str_cmp(field, "spells"))
            sprintflag(str, GET_OBJ_EFF_FLAGS(o), NUM_EFF_FLAGS, effect_flags);

        else if (!str_cmp(field, "has_spell")) {
            if ((num = search_block(value, effect_flags, FALSE)) >= 0)
                strcpy(str, OBJ_EFF_FLAGGED(o, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized effect flag '%s' to %%%s.has_spell[]%%", value, var);
                script_log(trig, buf2);
            }
        }

        /* Location */
        else if (!str_cmp(field, "room")) {
            num = obj_room(o);
            if (num != NOWHERE)
                sprintf(str, "%d", world[num].vnum);
            else
                strcpy(str, "-1");
        } else if (!str_cmp(field, "carried_by"))
            UID_VAR(str, o->carried_by);
        else if (!str_cmp(field, "worn_by"))
            UID_VAR(str, o->worn_by);
        else if (!str_cmp(field, "worn_on")) {
            if (o->worn_by)
                sprinttype(o->worn_on, wear_positions, str);
            else
                *str = '\0';
        } else if (!str_cmp(field, "contents")) {
            if (!str_cmp(value, "count")) {
                for (num = 0, o = o->contains; o; o = o->next_content)
                    ++num;
                sprintf(str, "%d", num);
            } else if (*value) {
                /* An argument was given: find a specific object. */
                num = atoi(value);
                for (obj = o->contains; obj; obj = obj->next_content)
                    if (GET_OBJ_VNUM(obj) == num) {
                        UID_VAR(str, obj);
                        break;
                    }
                if (!obj) /* No matching object found. */
                    strcpy(str, "0");
            } else
                UID_VAR(str, o->contains);
        } else if (!str_cmp(field, "next_in_list"))
            UID_VAR(str, o->next_content);

        else {
            *str = '\0';
            sprintf(buf2, "trigger type: %d. unknown object field: '%s'", type, field);
            script_log(trig, buf2);
        }
    }

    /*
     * Room variables
     */
    else if (r) {
        if (!str_cmp(field, "name"))
            strcpy(str, r->name);

        else if (!str_cmp(field, "vnum"))
            sprintf(str, "%d", r->vnum);
        else if (!strn_cmp(field, "sector", 6))
            sprintf(str, "%s", sectors[r->sector_type].name);
        else if (!str_cmp(field, "is_dark"))
            strcpy(str, r->light > 0 ? "1" : "0");

        else if (!str_cmp(field, "flags"))
            sprintflag(str, r->room_flags, NUM_ROOM_FLAGS, room_bits);
        else if (!str_cmp(field, "flagged")) {
            if ((num = search_block(value, room_bits, FALSE)) >= 0)
                strcpy(str, IS_FLAGGED(r->room_flags, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized room flag '%s' to %%%s.flagged%%", value, var);
                script_log(trig, buf2);
            }
        } else if (!str_cmp(field, "effects") || !str_cmp(field, "affects"))
            sprintflag(str, r->room_effects, NUM_ROOM_EFF_FLAGS, room_effects);
        else if (!str_cmp(field, "has_effect") || !str_cmp(field, "has_affect")) {
            if ((num = search_block(value, room_effects, FALSE)) >= 0)
                strcpy(str, IS_FLAGGED(r->room_effects, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized room effect flag '%s' to %%%s.has_effect%%", value, var);
                script_log(trig, buf2);
            }
        }

        else if (!str_cmp(field, "objects")) {
            if (!str_cmp(value, "count")) {
                for (num = 0, o = r->contents; o; o = o->next_content)
                    ++num;
                sprintf(str, "%d", num);
            } else if (*value) {
                /* An argument was given: find a specific object. */
                num = atoi(value);
                for (obj = r->contents; obj; obj = obj->next_content)
                    if (GET_OBJ_VNUM(obj) == num) {
                        UID_VAR(str, obj);
                        break;
                    }
                if (!obj) /* No matching object found. */
                    strcpy(str, "0");
            } else
                UID_VAR(str, r->contents);
        } else if (!str_cmp(field, "people")) {
            if (!str_cmp(value, "count")) {
                for (num = 0, c = r->people; c; c = c->next_in_room)
                    if (!GET_INVIS_LEV(c))
                        ++num;
                sprintf(str, "%d", num);
            } else if (*value) {
                /* An argument was given: find a specific vnum. */
                num = atoi(value);
                for (ch = r->people; ch; ch = ch->next_in_room)
                    if (GET_MOB_VNUM(ch) == num) {
                        UID_VAR(str, ch);
                        break;
                    }
                if (!ch) /* No matching mobile found. */
                    strcpy(str, "0");
            } else {
                /* Skip any wiz-invis folks */
                c = r->people;
                while (c && GET_INVIS_LEV(c))
                    c = c->next_in_room;
                UID_VAR(str, c);
            }
        }

        /* Exits can have values (which are actually sub-sub-variables)  */
        else if ((num = search_block(field, dirs, TRUE)) >= 0) {
            if (!r->exits[num])
                strcpy(str, "-1");
            else if (!*value) /* %room.DIR% is a vnum */
                sprintf(str, "%d", r->exits[num]->to_room != NOWHERE ? world[r->exits[num]->to_room].vnum : -1);
            else if (!str_cmp(value, "room")) { /* %room.DIR[room]% */
                if (r->exits[num]->to_room != NOWHERE)
                    ROOM_UID_VAR(str, r->exits[num]->to_room);
                else
                    strcpy(str, "0");
            } else if (!str_cmp(value, "key")) /* %room.DIR[key]% */
                sprintf(str, "%d", r->exits[num]->key);
            else if (!str_cmp(value, "bits")) /* %room.DIR[bits]% */
                sprintbit(r->exits[num]->exit_info, exit_bits, str);
            else
                *str = '\0';
        }

        else {
            *str = '\0';
            sprintf(buf2, "trigger type: %d. unknown room field: '%s'", type, field);
            script_log(trig, buf2);
        }
    }

    else {
        *str = '\0';
        /*
         * We didn't find a matching character, object, or room, but we
         * located a variable earlier than that.  If we attempted to
         * access a subfield on a non-UID variable, log an error.
         */
        if (vd && vd->value) {
            sprintf(buf2, "attempt to access field '%s' on %s variable '%s'", field,
                    !*vd->value ? "empty" : (*vd->value == UID_CHAR ? "previously extracted UID" : "non-UID"), var);
            script_log(trig, buf2);
        }
    }
}

/* substitutes any variables into line and returns it as buf */
void var_subst(void *go, struct script_data *sc, trig_data *trig, int type, char *line, char *buf) {
    char tmp[MAX_INPUT_LENGTH], repl_str[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH];
    char *var, *field, *subfield, *p;
    int left, len;

    /* Skip if no %'s */
    if (!strchr(line, '%')) {
        strcpy(buf, line);
        return;
    }

    p = strcpy(tmp, line);
    value[0] = '\0';

    left = MAX_INPUT_LENGTH - 1;

    while (*p && (left > 0)) {

        /* Copy until we find the first % */
        while (*p && (*p != '%') && (left > 0)) {
            *(buf++) = *(p++);
            left--;
        }

        *buf = '\0';

        /* double % */
        if (*p && (*(++p) == '%') && (left > 0)) {
            *(buf++) = *(p++);
            *buf = '\0';
            left--;
            continue;
        }

        else if (*p && (left > 0)) {

            /* search until end of var or beginning of field */
            for (var = p; *p && (*p != '%') && (*p != '.'); ++p)
                ;

            field = p;
            if (*p == '.') {
                *(p++) = '\0';

                /* search until end of field or beginning of subfield/value */
                for (field = p; *p && (*p != '%') && (*p != '['); ++p)
                    ;

                subfield = p;
                if (*p == '[') {
                    *(p++) = '\0';

                    /* search until the end of the value */
                    for (subfield = p; *p && (*p != ']'); ++p)
                        ;

                    if (*p == ']')
                        *(p++) = '\0';
                    else if (*p == '%')
                        *p = '\0'; /* but don't increment p yet */

                    var_subst(go, sc, trig, type, subfield, value);
                }
            }

            *(p++) = '\0';

            find_replacement(go, sc, trig, type, var, field, value, repl_str);

            strncat(buf, repl_str, left);
            len = strlen(repl_str);
            buf += len;
            left -= len;
        }
    }
}

/* returns 1 if string is all digits, else 0 */
int is_num(char *num) {
    if (*num == '\0')
        return FALSE;

    if (*num == '+' || *num == '-')
        ++num;

    for (; *num != '\0'; ++num)
        if (!isdigit(*num))
            return FALSE;

    return TRUE;
}

/* evaluates 'lhs op rhs', and copies to result */
void eval_op(char *op, char *lhs, char *rhs, char *result, void *go, struct script_data *sc, trig_data *trig) {
    char *p;
    int n;

    /* strip off extra spaces at begin and end */
    while (*lhs && isspace(*lhs))
        lhs++;
    while (*rhs && isspace(*rhs))
        rhs++;

    for (p = lhs; *p; p++)
        ;
    for (--p; isspace(*p) && (p > lhs); *p-- = '\0')
        ;
    for (p = rhs; *p; p++)
        ;
    for (--p; isspace(*p) && (p > rhs); *p-- = '\0')
        ;

    /* find the op, and figure out the value */
    if (!strcmp("||", op)) {
        if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
            strcpy(result, "0");
        else
            strcpy(result, "1");
    }

    else if (!strcmp("&&", op)) {
        if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
            strcpy(result, "0");
        else
            strcpy(result, "1");
    }

    else if (!strcmp("==", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) == atoi(rhs));
        else
            sprintf(result, "%d", !str_cmp(lhs, rhs));
    }

    else if (!strcmp("!=", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) != atoi(rhs));
        else
            sprintf(result, "%d", str_cmp(lhs, rhs));
    }

    else if (!strcmp("<=", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
        else
            sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
    }

    else if (!strcmp(">=", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
        else
            sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
    }

    else if (!strcmp("<", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) < atoi(rhs));
        else
            sprintf(result, "%d", str_cmp(lhs, rhs) < 0);
    }

    else if (!strcmp(">", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) > atoi(rhs));
        else
            sprintf(result, "%d", str_cmp(lhs, rhs) > 0);
    }

    else if (!strcmp("/=", op))
        sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');

    else if (!strcmp("*", op))
        sprintf(result, "%d", atoi(lhs) * atoi(rhs));

    else if (!strcmp("/", op))
        sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);

    else if (!strcmp("+", op))
        sprintf(result, "%d", atoi(lhs) + atoi(rhs));

    else if (!strcmp("-", op))
        sprintf(result, "%d", atoi(lhs) - atoi(rhs));

    else if (!strcmp("!", op)) {
        if (is_num(rhs))
            sprintf(result, "%d", !atoi(rhs));
        else
            sprintf(result, "%d", !*rhs);
    }
}

/*
 * p points to the first quote, returns the matching
 * end quote, or the last non-null char in p.
 */
char *matching_quote(char *p) {
    for (p++; *p && (*p != '"'); p++) {
        if (*p == '\\')
            p++;
    }

    if (!*p)
        p--;

    return p;
}

/*
 * p points to the first paren.  returns a pointer to the
 * matching closing paren, or the last non-null char in p.
 */
char *matching_paren(char *p) {
    int i;

    for (p++, i = 1; *p && i; p++) {
        if (*p == '(')
            i++;
        else if (*p == ')')
            i--;
        else if (*p == '"')
            p = matching_quote(p);
    }

    return --p;
}

/* evaluates line, and returns answer in result */
void eval_expr(char *line, char *result, void *go, struct script_data *sc, trig_data *trig, int type) {
    char expr[MAX_INPUT_LENGTH], *p;

    while (*line && isspace(*line))
        line++;

    if (eval_lhs_op_rhs(line, result, go, sc, trig, type))
        ;

    else if (*line == '(') {
        p = strcpy(expr, line);
        p = matching_paren(expr);
        *p = '\0';
        eval_expr(expr + 1, result, go, sc, trig, type);
    }

    else
        var_subst(go, sc, trig, type, line, result);
}

/*
 * evaluates expr if it is in the form lhs op rhs, and copies
 * answer in result.  returns 1 if expr is evaluated, else 0
 */
int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc, trig_data *trig, int type) {
    char *p, *tokens[MAX_INPUT_LENGTH];
    char line[MAX_INPUT_LENGTH], lhr[MAX_INPUT_LENGTH], rhr[MAX_INPUT_LENGTH];
    int i, j;

    /*
     * valid operands, in order of priority
     * each must also be defined in eval_op()
     */
    static char *ops[] = {"||", "&&", "==", "!=", "<=", ">=", "<", ">", "/=", "-", "+", "/", "*", "!", "\n"};

    p = strcpy(line, expr);

    /*
     * initialize tokens, an array of pointers to locations
     * in line where the ops could possibly occur.
     */
    for (j = 0; *p; j++) {
        tokens[j] = p;
        if (*p == '(')
            p = matching_paren(p) + 1;
        else if (*p == '"')
            p = matching_quote(p) + 1;
        else if (isalnum(*p))
            for (p++; *p && (isalnum(*p) || isspace(*p)); p++)
                ;
        else
            p++;
    }
    tokens[j] = NULL;

    for (i = 0; *ops[i] != '\n'; i++)
        for (j = 0; tokens[j]; j++)
            if (!strn_cmp(ops[i], tokens[j], strlen(ops[i]))) {
                *tokens[j] = '\0';
                p = tokens[j] + strlen(ops[i]);

                eval_expr(line, lhr, go, sc, trig, type);
                eval_expr(p, rhr, go, sc, trig, type);
                eval_op(ops[i], lhr, rhr, result, go, sc, trig);

                return 1;
            }

    return 0;
}

/* returns 1 if cond is true, else 0 */
int process_if(char *cond, void *go, struct script_data *sc, trig_data *trig, int type) {
    char result[MAX_INPUT_LENGTH], *p;

    eval_expr(cond, result, go, sc, trig, type);

    p = result;
    skip_spaces(&p);

    if (!*p || *p == '0')
        return 0;
    else
        return 1;
}

/*
 * scans for end of if-block.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
struct cmdlist_element *find_end(trig_data *trig, struct cmdlist_element *cl) {
    struct cmdlist_element *c;
    char *p;

    if (!(cl->next)) {
        script_log(trig, "'if' without 'end'. (error 1)");
        return cl;
    }

    for (c = cl->next; c; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ;

        if (!strn_cmp("if ", p, 3))
            c = find_end(trig, c);
        else if (!strn_cmp("end", p, 3))
            return c;

        if (!c->next) {
            script_log(trig, "'if' without 'end'. (error 2)");
            return c;
        }
    }

    script_log(trig, "'if' without 'end'. (error 3)");
    return c;
}

/*
 * searches for valid elseif, else, or end to continue execution at.
 * returns line of elseif, else, or end if found, or last line of trigger.
 */
struct cmdlist_element *find_else_end(trig_data *trig, struct cmdlist_element *cl, void *go, struct script_data *sc,
                                      int type) {
    struct cmdlist_element *c;
    char *p;

    if (!(cl->next))
        return cl;

    for (c = cl->next; c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ; /* skip spaces */

        if (!strn_cmp("if ", p, 3))
            c = find_end(trig, c);

        else if (!strn_cmp("elseif ", p, 7)) {
            if (process_if(p + 7, go, sc, trig, type)) {
                GET_TRIG_DEPTH(trig)++;
                return c;
            }
        }

        else if (!strn_cmp("else", p, 4)) {
            GET_TRIG_DEPTH(trig)++;
            return c;
        }

        else if (!strn_cmp("end", p, 3))
            return c;

        if (!c->next) {
            script_log(trig, "'if' without 'end'. (error 4)");
            return c;
        }
    }

    /* If we got here, it's the last line, if it's not an end, log it. */
    for (p = c->cmd; *p && isspace(*p); ++p)
        ; /* skip spaces */
    if (strn_cmp("end", p, 3))
        script_log(trig, "'if' without 'end'. (error 5)");

    return c;
}

/* processes any 'wait' commands in a trigger */
void process_wait(void *go, trig_data *trig, int type, char *cmd, struct cmdlist_element *cl) {
    char buf[MAX_INPUT_LENGTH], *arg;
    struct wait_event_data *wait_event_obj;
    long time, hr, min, ntime;
    char c;

    extern long global_pulse;

    arg = any_one_arg(cmd, buf);
    skip_spaces(&arg);

    if (!*arg) {
        sprintf(buf2, "Wait w/o an arg: '%s'", cl->cmd);
        script_log(trig, buf2);
        return;
    }

    else if (!strn_cmp(arg, "until ", 6)) {

        /* valid forms of time are 14:30 and 1430 */
        if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
            min += (hr * 60);
        else
            min = (hr % 100) + ((hr / 100) * 60);

        /* calculate the pulse of the day of "until" time */
        ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

        /* calculate pulse of day of current time */
        time = (global_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
               (time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

        if (time >= ntime) /* adjust for next day */
            time = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - time + ntime;
        else
            time = ntime - time;
    }

    else {
        if (sscanf(arg, "%ld %c", &time, &c) == 2) {
            if (c == 't')
                time *= PULSES_PER_MUD_HOUR;
            else if (c == 's')
                time *= PASSES_PER_SEC;
        }
    }

    CREATE(wait_event_obj, struct wait_event_data, 1);
    wait_event_obj->trigger = trig;
    wait_event_obj->go = go;
    wait_event_obj->type = type;

    GET_TRIG_WAIT(trig) = event_create(EVENT_TRIGGER_WAIT, trig_wait_event, wait_event_obj, TRUE, NULL, time);
    trig->curr_state = cl->next;
}

/* processes a script set command */
void process_set(struct script_data *sc, trig_data *trig, char *cmd) {
    char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;

    value = two_arguments(cmd, arg, name);

    skip_spaces(&value);

    if (!*name) {
        sprintf(buf2, "Set w/o an arg: '%s'", cmd);
        script_log(trig, buf2);
        return;
    }

    add_var(&GET_TRIG_VARS(trig), name, value);
}

/* processes a script eval command */
void process_eval(void *go, struct script_data *sc, trig_data *trig, int type, char *cmd) {
    char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    char result[MAX_INPUT_LENGTH], *expr;

    expr = two_arguments(cmd, arg, name);

    skip_spaces(&expr);

    if (!*name) {
        sprintf(buf2, "Eval w/o an arg: '%s'", cmd);
        script_log(trig, buf2);
        return;
    }

    eval_expr(expr, result, go, sc, trig, type);
    add_var(&GET_TRIG_VARS(trig), name, result);
}

/*
 * processes a script return command.
 * returns the new value for the script to return.
 */
int process_return(trig_data *trig, char *cmd) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(cmd, arg1, arg2);

    if (!*arg2) {
        sprintf(buf2, "Return w/o an arg: '%s'", cmd);
        script_log(trig, buf2);
        return 1;
    }

    return atoi(arg2);
}

/*
 * removes a variable from the global vars of sc,
 * or the local vars of trig if not found in global list.
 */
void process_unset(struct script_data *sc, trig_data *trig, char *cmd) {
    char arg[MAX_INPUT_LENGTH], *var;

    var = any_one_arg(cmd, arg);

    skip_spaces(&var);

    if (!*var) {
        sprintf(buf2, "Unset w/o an arg: '%s'", cmd);
        script_log(trig, buf2);
        return;
    }

    if (!remove_var(&(sc->global_vars), var))
        remove_var(&GET_TRIG_VARS(trig), var);
}

/*
 * makes a local variable into a global variable
 */
void process_global(struct script_data *sc, trig_data *trig, char *cmd) {
    struct trig_var_data *vd;
    char arg[MAX_INPUT_LENGTH], *varlist;

    varlist = any_one_arg(cmd, arg);

    skip_spaces(&varlist);

    if (!*varlist) {
        sprintf(buf2, "Global w/o an arg: '%s'", cmd);
        script_log(trig, buf2);
        return;
    }

    while (*varlist) {
        varlist = any_one_arg(varlist, arg);
        skip_spaces(&varlist);

        for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
            if (!str_cmp(vd->name, arg))
                break;

        if (!vd) {
            sprintf(buf2, "Local var '%s' not found in global call", arg);
            script_log(trig, buf2);
            continue;
        }

        add_var(&(sc->global_vars), vd->name, vd->value);
        remove_var(&GET_TRIG_VARS(trig), vd->name);
    }
}

/*
 * This is the core driver for scripts.
 *
 * Arguments:
 * void *go_address
 *   A pointer to a pointer to the entity running the script.  The
 *   reason for this approach is that we want to be able to see from the
 *   calling function if the entity has been free'd.
 * trig_data *trig
 *   A pointer to the current running trigger.
 * int type
 *   MOB_TRIGGER, OBJ_TRIGGER, or WLD_TRIGGER.
 * int mode
 *   TRIG_NEW     just started from dg_triggers.c
 *   TRIG_RESTART restarted after a 'wait'
 */
int script_driver(void *go_address, trig_data *trig, int type, int mode) {
    static int depth = 0;
    int ret_val = 1;
    struct cmdlist_element *cl;
    char cmd[MAX_INPUT_LENGTH], *p;
    struct script_data *sc = 0;
    struct cmdlist_element *temp;
    unsigned long loops = 0;
    void *go = NULL;

    void obj_command_interpreter(obj_data * obj, struct trig_data * t, char *argument);
    void wld_command_interpreter(struct room_data * room, struct trig_data * t, char *argument);

    if (depth > MAX_SCRIPT_DEPTH) {
        switch (type) {
        case MOB_TRIGGER:
            sprintf(buf, "Triggers recursed beyond maximum allowed depth on mob %d", GET_MOB_VNUM((char_data *)go));
            break;
        case OBJ_TRIGGER:
            sprintf(buf, "Triggers recursed beyond maximum allowed depth on obj %d", GET_OBJ_VNUM((obj_data *)go));
            break;
        case WLD_TRIGGER:
            sprintf(buf, "Triggers recursed beyond maximum allowed depth in room %d", ((struct room_data *)go)->vnum);
            break;
        }
        script_log(trig, buf);
        return ret_val;
    }

    depth++;

    switch (type) {
    case MOB_TRIGGER:
        go = *(char_data **)go_address;
        sc = SCRIPT((char_data *)go);
        break;
    case OBJ_TRIGGER:
        go = *(obj_data **)go_address;
        sc = SCRIPT((obj_data *)go);
        break;
    case WLD_TRIGGER:
        go = *(room_data **)go_address;
        sc = SCRIPT((struct room_data *)go);
        break;
    }

    if (mode == TRIG_NEW) {
        GET_TRIG_DEPTH(trig) = 1;
        GET_TRIG_LOOPS(trig) = 0;
    }

    trig->running = TRUE;
    for (cl = (mode == TRIG_NEW) ? trig->cmdlist : trig->curr_state; cl && GET_TRIG_DEPTH(trig); cl = cl->next) {
        /* no point in continuing if the mob has zapped itself... */
        if (trig->purged)
            break;

        if (type == MOB_TRIGGER && !(trig->trigger_type & MTRIG_DEATH)) { /* only death trigs are immune to all tests */
            if (!AWAKE((char_data *)go)) {
                depth--;
                if (mode == TRIG_NEW)
                    GET_TRIG_DEPTH(trig) = 0; /* reset trigger totally if instant bail */
                return 0;
            }

            if (CASTING((char_data *)go)) {
                pause_while_casting(go, trig, type, cl);
                depth--;
                return ret_val;
            }
        }
        for (p = cl->cmd; *p && isspace(*p); p++)
            ;

        if (*p == '*')
            continue;

        else if (!strn_cmp(p, "if ", 3)) {
            if (process_if(p + 3, go, sc, trig, type))
                GET_TRIG_DEPTH(trig)++;
            else
                cl = find_else_end(trig, cl, go, sc, type);
        }

        else if (!strn_cmp("elseif ", p, 7) || !strn_cmp("else", p, 4)) {
            /* If not in an if-block, ignore the extra 'else[if]' and warn about it.
             */
            if (GET_TRIG_DEPTH(trig) == 1) {
                script_log(trig, "'else' without 'if'.");
                continue;
            }
            cl = find_end(trig, cl);
            GET_TRIG_DEPTH(trig)--;
        } else if (!strn_cmp("while ", p, 6)) {
            temp = find_done(cl);
            if (!temp) {
                script_log(trig, "'while' without 'done'.");
                return ret_val;
            } else if (process_if(p + 6, go, sc, trig, type)) {
                temp->original = cl;
            } else {
                cl = temp;
                loops = 0;
            }
        } else if (!strn_cmp("switch ", p, 7)) {
            cl = find_case(trig, cl, go, sc, type, p + 7);
        } else if (!strn_cmp("end", p, 3)) {
            if (GET_TRIG_DEPTH(trig) == 1) {
                script_log(trig, "'end' without 'if'.");
                continue;
            }
            GET_TRIG_DEPTH(trig)--;
        } else if (!strn_cmp("done", p, 4)) {
            /* if in a while loop, cl->original is non-NULL */
            if (cl->original) {
                char *orig_cmd = cl->original->cmd;
                while (*orig_cmd && isspace(*orig_cmd))
                    orig_cmd++;

                if (cl->original && process_if(orig_cmd + 6, go, sc, trig, type)) {
                    cl = cl->original;
                    temp = find_done(cl);
                    loops++;
                    GET_TRIG_LOOPS(trig)++;
                    if (loops == 30) {
                        process_wait(go, trig, type, "wait 1", cl);
                        depth--;
                        return ret_val;
                    }
                    if (GET_TRIG_LOOPS(trig) >= 100) {
                        script_log(trig, "looped 100 times!!!");
                        break;
                    }
                }
            }
        } else if (!strn_cmp("break", p, 5)) {
            cl = find_done(cl);
        } else if (!strn_cmp("case", p, 4)) {
            /* Do nothing, this allows multiple cases to a single instance */
        }

        else {

            var_subst(go, sc, trig, type, p, cmd);

            if (!strn_cmp(cmd, "eval ", 5))
                process_eval(go, sc, trig, type, cmd);

            else if (!strn_cmp(cmd, "halt", 4))
                break;

            else if (!strn_cmp(cmd, "global ", 7))
                process_global(sc, trig, cmd);

            else if (!strn_cmp(cmd, "return ", 7))
                ret_val = process_return(trig, cmd);

            else if (!strn_cmp(cmd, "set ", 4))
                process_set(sc, trig, cmd);

            else if (!strn_cmp(cmd, "unset ", 6))
                process_unset(sc, trig, cmd);

            else if (!strn_cmp(cmd, "wait ", 5)) {
                process_wait(go, trig, type, cmd, cl);
                depth--;
                return ret_val;
            }

            else
                switch (type) {
                case MOB_TRIGGER:
                    command_interpreter((char_data *)go, cmd);
                    break;
                case OBJ_TRIGGER:
                    obj_command_interpreter((obj_data *)go, trig, cmd);
                    break;
                case WLD_TRIGGER:
                    wld_command_interpreter((struct room_data *)go, trig, cmd);
                    break;
                }
        }
    }
    trig->running = FALSE;

    if (trig->purged) {
        free_trigger(trig);
        go_address = NULL;
    } else {
        free_varlist(GET_TRIG_VARS(trig));
        GET_TRIG_VARS(trig) = NULL;
        GET_TRIG_DEPTH(trig) = 0;
    }

    depth--;
    return ret_val;
}

int real_trigger(int vnum) {
    int rnum;

    for (rnum = 0; rnum < top_of_trigt; rnum++) {
        if (trig_index[rnum]->virtual == vnum)
            break;
    }

    if (rnum == top_of_trigt)
        rnum = -1;
    return (rnum);
}

ACMD(do_tstat) {
    int vnum, rnum;
    char str[MAX_INPUT_LENGTH];

    half_chop(argument, str, argument);
    if (*str) {
        vnum = atoi(str);
        rnum = real_trigger(vnum);
        if (rnum < 0) {
            send_to_char("That vnum does not exist.\r\n", ch);
            return;
        }

        do_stat_trigger(ch, trig_index[rnum]->proto);
    } else
        send_to_char("Usage: tstat <vnum>\r\n", ch);
}

/*
 * scans for a case/default instance
 * returns the line containg the correct case instance, or the last
 * line of the trigger if not found.
 */
struct cmdlist_element *find_case(struct trig_data *trig, struct cmdlist_element *cl, void *go, struct script_data *sc,
                                  int type, char *cond) {
    char cond_expr[MAX_INPUT_LENGTH], *p;
    struct cmdlist_element *c;

    if (!(cl->next))
        return cl;

    eval_expr(cond, cond_expr, go, sc, trig, type);

    for (c = cl->next; c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ;

        if (!strn_cmp("while ", p, 6) || !strn_cmp("switch", p, 6))
            c = find_done(c);
        else if (!strn_cmp("case ", p, 5)) {
            char case_expr[MAX_STRING_LENGTH];
            char result[16]; /* == always returns an integer, so it shouuld be safe */
            eval_expr(p + 5, case_expr, go, sc, trig, type);
            eval_op("==", cond_expr, case_expr, result, go, sc, trig);
            if (*result && *result != '0')
                return c;
        } else if (!strn_cmp("default", p, 7))
            return c;
        else if (!strn_cmp("done", p, 3))
            return c;
    }
    return c;
}

/*
 * scans for end of while/switch-blocks.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
struct cmdlist_element *find_done(struct cmdlist_element *cl) {
    struct cmdlist_element *c;
    char *p;

    if (!cl || !(cl->next))
        return cl;

    for (c = cl->next; c && c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ;

        if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7))
            c = find_done(c);
        else if (!strn_cmp("done", p, 3))
            return c;
    }

    return c;
}

void check_time_triggers(void) {
    char_data *ch;
    obj_data *obj;
    struct room_data *room = NULL;
    int nr;
    struct script_data *sc;

    for (ch = character_list; ch; ch = ch->next) {
        if (SCRIPT(ch)) {
            sc = SCRIPT(ch);

            if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIME) &&
                (!is_empty(world[IN_ROOM(ch)].zone) || IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
                time_mtrigger(ch);
        }
    }

    for (obj = object_list; obj; obj = obj->next) {
        if (SCRIPT(obj)) {
            sc = SCRIPT(obj);

            if (IS_SET(SCRIPT_TYPES(sc), OTRIG_TIME))
                time_otrigger(obj);
        }
    }

    for (nr = 0; nr <= top_of_world; nr++) {
        if (SCRIPT(&world[nr])) {
            room = &world[nr];
            sc = SCRIPT(room);

            if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIME) &&
                (!is_empty(room->zone) || IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
                time_wtrigger(room);
        }
    }
}

/***************************************************************************
 * $Log: dg_scripts.c,v $
 * Revision 1.130  2009/06/10 20:14:48  myc
 * Fix bug in group_members subvariable.
 *
 * Revision 1.129  2009/06/09 05:38:52  myc
 * Slight modification to find_replacement to accomodate the
 * new clan interface.
 *
 * Revision 1.128  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.127  2009/03/17 07:59:42  jps
 * Moved str_str to strings.c
 *
 * Revision 1.126  2009/03/09 21:43:50  myc
 * Use references to coin_names instead of string constants.
 *
 * Revision 1.125  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.124  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.123  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.122  2009/03/06 00:46:31  myc
 * Make MATCH_CHAR_FUNC(match_dg_vis_char_by_id) visible outside the
 * file since find_char_around_room needs to know its address.
 *
 * Revision 1.121  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.120  2009/01/17 00:28:02  myc
 * Fix possible use of uninitialized variable.
 *
 * Revision 1.119  2008/09/22 02:09:17  jps
 * Changed weight into a floating-point value. Precision is preserved to
 * the 1/100 place.
 *
 * Revision 1.118  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.117  2008/09/02 07:16:00  mud
 * Changing object TIMER uses into DECOMP where appropriate
 *
 * Revision 1.116  2008/08/26 03:58:13  jps
 * Replaced real_zone calls with find_real_zone_by_room, since that's what it
 *did. Except the one for wzoneecho, since it needed to find a real zone by zone
 *number.
 *
 * Revision 1.115  2008/08/24 02:34:14  myc
 * Make arguments and return values for str_str const.
 *
 * Revision 1.114  2008/08/15 03:59:08  jps
 * Added pprintf for paging, and changed page_string to take a character.
 *
 * Revision 1.113  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.112  2008/06/20 20:21:44  jps
 * Made fullpurge into an event and moved it to events.c.
 *
 * Revision 1.111  2008/06/19 18:53:12  myc
 * Replaced the item_types array with a typedef struct array.
 *
 * Revision 1.110  2008/06/05 02:07:43  myc
 * Removed cost_per_day field from objects.  Changed object flags
 * to use flagvectors.
 *
 * Revision 1.109  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.108  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.107  2008/04/14 07:16:48  jps
 * Un-hardcode positions.
 *
 * Revision 1.106  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.105  2008/04/05 19:43:15  jps
 * Allow access to general variable %damdone%, which is the amount
 * of damage that a *damage command did to a character.
 *
 * Revision 1.104  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.103  2008/04/02 03:24:44  myc
 * Rewrote group code.
 *
 * Revision 1.102  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.101  2008/03/21 21:41:58  jps
 * Forgot some elses. I was confused by the code formatting.
 *
 * Revision 1.100  2008/03/21 21:36:31  jps
 * Add get.obj_noadesc[vnum] and get.obj_pldesc[vnum].
 *
 * Revision 1.99  2008/03/16 00:21:16  jps
 * Updated references to player trophies.
 *
 * Revision 1.98  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.97  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.96  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.95  2008/03/05 03:03:54  myc
 * Redesigned the trophy structures, so had to update the trophy
 * DG variable to access it correctly.
 *
 * Revision 1.94  2008/02/16 20:26:04  myc
 * Use free_trigger instead of trig_data_free.
 *
 * Revision 1.93  2008/02/16 07:02:15  myc
 * Bug in is_num wasn't parsing negative numbers.
 *
 * Revision 1.92  2008/02/13 21:10:14  myc
 * Fix variable parsing in case statements.
 *
 * Revision 1.91  2008/02/11 08:50:33  jps
 * Make trigger lists given from 'stat' more succinct.
 *
 * Revision 1.90  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not.
 *
 * Revision 1.89  2008/02/09 18:29:11  myc
 * The event code now handles freeing of event objects.
 *
 * Revision 1.88  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.87  2008/02/06 03:45:08  myc
 * Stat room and stat obj now use the pager.
 *
 * Revision 1.86  2008/02/04 00:22:05  myc
 * Making stat char use the pager.
 *
 * Revision 1.85  2008/02/02 19:56:51  myc
 * script_driver now requires an address
 *
 * Revision 1.84  2008/02/02 04:27:55  myc
 * Changing the way script_driver works: you now pass it a pointer
 * to the pointer of what you want to run the script.  That is,
 * script_driver(&ch, ...) instead of script_driver(ch, ...).
 * Adding time triggers (which execute at a given mud time each day).
 *
 * Revision 1.83  2008/02/01 06:44:29  myc
 * Oops, forgot a line in find_case.  Anyway, you'll now be able to use
 * expressions on switch lines.
 *
 * Revision 1.82  2008/02/01 06:32:23  myc
 * Various fixes to scripts: is_num won't think "------" is a number.
 * Find end reports an error if no end is found.  Other stuff...
 *
 * Revision 1.81  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.80  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 *
 * Revision 1.79  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.78  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.77  2008/01/19 02:17:08  myc
 * Memory read bug in actor.wearing variable.
 *
 * Revision 1.76  2008/01/18 20:30:31  myc
 * Changing variable fields that return rooms to return vnums instead
 * references, like they used to be, because that's much easier to work
 * with in triggers.  You can still get a room reference at any time
 * by using the get.room function.
 *
 * Revision 1.75  2008/01/18 08:28:43  myc
 * Added room.people[vnum], room.objects[vnum], and object.contents[vnum]
 * variables.
 *
 * Revision 1.74  2008/01/18 07:11:55  myc
 * Adding a get.opposite_dir[] static variable.
 *
 * Revision 1.73  2008/01/17 19:23:07  myc
 * Added get_room_location which mimics get_room but returns the
 * rnum instead of the actual room.  Modified get_room to only
 * attempt to use a string if it starts with a digit; that way
 * we can chain get_room when looking for a target.
 *
 * Revision 1.72  2008/01/17 07:40:32  myc
 * Taking out the indentation in tstat so you can copy and paste it.
 *
 * Revision 1.71  2008/01/17 06:24:23  myc
 * Oops, wrote out the wrong object for wearing[].
 *
 * Revision 1.70  2008/01/17 06:19:47  myc
 * Adding worn[count] and wearing[vnum] fields to character variables.
 *
 * Revision 1.69  2008/01/17 04:19:07  myc
 * Fixing a bug with nested variables.
 *
 * Revision 1.68  2008/01/17 04:10:07  myc
 * Updating get_obj_by_obj() and get_char_room_mscript() to check
 * UIDs against the calling object and char (since find_replacement
 * now returns a UID for the object/char instead of "self" now).
 *
 * Revision 1.67  2008/01/17 02:44:16  myc
 * Rewrote var_subst.
 *
 * Revision 1.66  2008/01/17 01:51:40  myc
 * Was using the wrong variable (ch not c) in the %room.people[count]%
 * section of find_replacement.
 *
 * Revision 1.65  2008/01/17 01:29:10  myc
 * OMG rewrote find_replacement.  It now exposes like every char_data,
 * obj_data, and room_data field.  This calls for a giant rewrite for
 * most triggers, but it's worth it.  New stuff you can access includes:
 * trophy, effects, contents, room flags, and much more!  Also cleaned
 * up var_subst a bit too.
 *
 * Revision 1.64  2008/01/13 23:06:04  myc
 * Fixed a bug in process_global.
 *
 * Revision 1.63  2008/01/12 23:13:20  myc
 * Created a multi-purpose get_random_char_around() function to pick random
 * chars in a room.
 *
 * Revision 1.62  2008/01/09 02:30:56  jps
 * Use macro to get mob real number.
 *
 * Revision 1.61  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.60  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.59  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.58  2007/10/04 16:20:24  myc
 * Added %object.timer% variable.
 *
 * Revision 1.57  2007/09/20 23:19:00  myc
 * Fixing flags again.
 *
 * Revision 1.56  2007/09/20 23:12:29  myc
 * Wrong flags in %actor.flags%!
 *
 * Revision 1.55  2007/09/20 21:20:43  myc
 * The %actor.flags% variable now returns a sprintbit string of all
 * the flags the actor has, instead of checking bits using a [] thingy.
 *
 * Revision 1.54  2007/09/17 22:29:59  jps
 * Unbreak some triggers I broke by not checking the entire
 * world for object rnums in get_obj_by_room().
 *
 * Revision 1.53  2007/08/30 19:42:46  jps
 * Cause *purge dg script commands to destroy all of a mobile's inventory
 * and equipment when purging mobs.
 *
 * Revision 1.52  2007/08/30 09:10:44  jps
 * Change get_obj_by_room to only find objects in the requested room.
 *
 * Revision 1.51  2007/08/26 22:37:37  jps
 * %people.ROOM% back to the actual number of chars in there.
 * It was causing way too many problems the other way.
 *
 * Revision 1.50  2007/08/24 17:01:36  myc
 * Adding ostat and mstat commands as shorthand for vstat, rstat for stat
 * room, and mnum and onum for vnum.  Also adding rnum and znum with new
 * functionality.
 *
 * Revision 1.49  2007/08/24 10:24:04  jps
 * Moved zstat to act.wizard.c.
 *
 * Revision 1.48  2007/08/14 08:48:11  jps
 * Renamed dg_global_pulse to global_pulse since it will now be used for
 * other things in addition to dg scripting.
 *
 * Revision 1.47  2007/08/08 20:21:10  jps
 * Oops left an unused variable in do_tlist.
 *
 * Revision 1.46  2007/08/08 20:09:42  jps
 * tlist, mlist, olist, and rlist now accept a single parameter which they
 * take to be a zone whose objects should be listed.  If no parameter is
 * given, they will list objects in the current zone - where the character
 * issuing the command is located.
 *
 * Revision 1.45  2007/05/29 00:36:03  jps
 * Use the proper top of zone value when processing the command <tlist #>,
 * which shows a list of triggers in zone #.
 *
 * Revision 1.44  2007/05/29 00:04:19  jps
 * Escape commands in tstat so you can copy and paste the code.
 *
 * Revision 1.43  2007/05/11 19:34:15  myc
 * Modified the quest command functions so they are thin wrappers for
 * perform_quest() in quest.c.  Error handling and messages should be
 * much better now.  Advance and rewind now accept another argument
 * specifying how many stages to advance or rewind.
 *
 * Revision 1.42  2007/04/18 21:58:11  jps
 * The %people.roomvnum% variable now sees exactly the same characters
 * as %random.char% (in other words, not NOHASSLE or wizinvis'd folks).
 *
 * Revision 1.41  2007/04/17 23:53:23  myc
 * The 'global' trigger command now takes a list of local variables and
 * globalizes each one.
 *
 * Revision 1.40  2007/03/27 04:27:05  myc
 * New fields for character dg variables: position and position1.
 *
 * Revision 1.39  2006/12/08 05:09:32  myc
 * Added new fields to actor variables, and sector field to room vars.
 *
 * Revision 1.38  2006/11/13 03:17:06  jps
 * Fix memory leak in trig_wait_event.
 *
 * Revision 1.37  2006/11/07 10:44:13  jps
 * Trigger name, vnum, and rnum are displayed when using tstat.
 *
 * Revision 1.36  2004/11/01 06:02:01  jjl
 * Updating the buffer size for triggers
 *
 * Revision 1.35  2003/07/29 01:42:11  jjl
 * Fixing "while" in dg_scripts.  while only worked properly if it was the first
 *thing on the line.  (IE "while " works, but " while " wouldn't)
 *
 * Revision 1.34  2003/07/13 23:04:45  jjl
 * Fixed a crash bug in the processing of %actor.class% for mobs.
 *
 * Revision 1.33  2003/07/12 05:45:35  jjl
 * Better version of oshdesc, supports variable vnums.
 *
 * Revision 1.32  2003/07/10 03:56:57  jjl
 * Added the "oshdesc" variable for getting object short descriptions.
 * Call it as %oshdesc.vnum% (IE %oshdesc.3011%)
 *
 * Revision 1.31  2003/07/09 04:57:49  jjl
 * Increased an internal buffer that was being overflowed by Zzur's armor
 * quest.
 *
 * Revision 1.30  2002/09/19 01:07:53  jjl
 * Update to add in quest variables!
 *
 * Revision 1.29  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.28  2002/05/08 00:30:06  dce
 * Fixed a stupid string error.
 *
 * Revision 1.27  2002/05/07 03:13:09  dce
 * Added a little error checking to the "exists" trigger
 * code to prevent crashes.
 * /s
 *
 * Revision 1.26  2002/05/06 23:54:04  dce
 * Added three new features: exists, mexists[#], oexists[#]. The show
 * the number currently in the game.
 * /s
 *
 * Revision 1.25  2001/08/03 21:25:00  mtp
 * reset trigger depth which is separate from script recursion depth, in the
 * case that a mob is asleep when a new trigger kicks off and we return
 * instantly
 *
 * Revision 1.24  2001/08/03 19:48:56  mtp
 * missing depth-- which meant that gradully triggers got into recursed error
 *
 * Revision 1.23  2001/07/25 06:59:02  mtp
 * modified logging to hopefully be a bit more helpful by specifying the
 * trigger id wherever possible. This does not apply to logging of mob trigs yet
 * as mobs use the same commands as players :-(
 *
 * Revision 1.22  2001/06/19 23:57:11  mtp
 * improved logging for trigger recursion
 *
 *
 * Revision 1.21  2001/02/12 01:26:52  mtp
 * added 2 fields, groupnum (members in group)
 * group[n] group member n
 *
 * Revision 1.20  2000/12/21 23:29:49  mtp
 * added strip_ansi to actor.class and also removed trailing WS o to allow ==
 * for class names
 *
 * Revision 1.19  2000/11/28 01:16:25  mtp
 * replaced dg_event code with events.c code
 *
 * Revision 1.17  2000/11/21 04:13:38  rsd
 * Altered the comment header and added old rlog messages
 * from prior to the $log$ string. Also the $log$ string
 * had been removed so the last 10 or so revisions weren't
 * commented in the file.  I added the string back in hopes
 * that it will continue to add the comments and included
 * the missing rlog messages.
 *
 * Revision 1.16  2000/11/09 23:59:54  mtp
 * added %actor.n% for he/she/it
 *
 * Revision 1.15  2000/11/07 01:53:34  mtp
 * added some extra fields to do with subclasses and also .o, .p for
 * him/her his/her stuff
 *
 * Revision 1.14  2000/11/03 17:28:33  jimmy
 * Added better checks for real_room to stop players/objs from
 * being placed in room NOWHERE.  This should help pinpoint any
 * weirdness.
 *
 * Revision 1.13  2000/10/30 18:32:35  mtp
 * fix for tlist
 *
 * Revision 1.12  2000/10/29 16:59:12  mtp
 * added puase_while_casting to halt (well pause cos it restarts)
 * trigger execution if mob is casting. This applies whether the cast is
 * within the trigger or external to the trigger
 *
 * Revision 1.11  2000/10/27 00:34:45  mtp
 * new fields in player structure for quest info
 *
 * Revision 1.10  2000/10/01 23:43:29  mtp
 * added extra trigger code
 * 1) can check players flags (flags 1,2,3) eg %plyr.flags[1,1]% returns
 *true/false if player blind 2) can get players in a room eg %self.people[3]%
 *gets the third person in room 3) extension to allow %self.people[%num%]% to
 *get the %num%th person in room
 *
 * note: the people[] call returns a player variable, so all fields are
 *available
 *
 * Revision 1.9  2000/03/04 00:22:16  mtp
 * added extra field for room object - vnum (will be mainly referenced by
 *%self.vnum%)
 *
 * Revision 1.8  2000/02/17 07:32:27  mtp
 * added %time% structure to show mud time and object.weight field
 *
 * Revision 1.7  2000/02/16 08:13:14  mtp
 * fixed while loops (which couldhave been infinite and crashed after 30 loops
 *anyhow) while loops are now killed after 100 loops
 *
 * Revision 1.6  2000/02/13 07:34:13  mtp
 * fixed opurge/mpurge problems by not freeing the running trigger
 * until it completes (set/unset running flag in dg_scripts.c and
 * free trigger if purged flag set at end)
 *
 * Revision 1.5  2000/02/11 00:29:37  mtp
 * another world[vnum] replaced by world[real_room(vnum)] *sigh*
 *
 * Revision 1.4  2000/02/02 22:54:25  mtp
 * added new field to %actor% for %actor.worn[##]% which
 * returns the vnum of the object that is worn there or -1 if none
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/31 01:57:43  mud
 * Added info to the comment header
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:30 mud
 * Initial revision
 *
 ***************************************************************************/

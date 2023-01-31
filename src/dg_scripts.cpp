
#include "dg_scripts.hpp"

#include "casting.hpp"
#include "charsize.hpp"
#include "clan.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "quest.hpp"
#include "races.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "trophy.hpp"
#include "utils.hpp"

#define PULSES_PER_MUD_HOUR (SECS_PER_MUD_HOUR * PASSES_PER_SEC)

/* mob trigger types */
const char *trig_types[] = {"Global",    "Random", "Command", "Speech", "Act",      "Death", "Greet",
                            "Greet-All", "Entry",  "Receive", "Fight",  "HitPrcnt", "Bribe", "SpeechTo*",
                            "Load",      "Cast",   "Leave",   "Door",   "UNUSED",   "Time",  "\n"};

/* obj trigger types */
const char *otrig_types[] = {"Global", "Random", "Command", "Attack", "Defense", "Timer",  "Get",
                             "Drop",   "Give",   "Wear",    "DEATH",  "Remove",  "UNUSED", "UNUSED",
                             "Load",   "Cast",   "Leave",   "UNUSED", "Consume", "Time",   "\n"};

/* wld trigger types */
const char *wtrig_types[] = {"Global", "Random",    "Command", "Speech", "UNUSED", "Reset",  "Preentry",
                             "Drop",   "Postentry", "UNUSED",  "UNUSED", "UNUSED", "UNUSED", "UNUSED",
                             "UNUSED", "Cast",      "Leave",   "Door",   "UNUSED", "Time",   "\n"};

TrigData *trigger_list = nullptr; /* all attached triggers */

/* external functions */
int find_target_room(CharData *ch, char *rawroomstr);
int obj_room(ObjData *obj);
int is_empty(int zone_nr);
int find_target_room(CharData *ch, char *rawroomstr);
TrigData *read_trigger(int nr);
void extract_trigger(TrigData *trig);
int eval_lhs_op_rhs(char *expr, char *result, void *go, ScriptData *sc, TrigData *trig, int type);
int find_zone(int num);
int vnumargs(CharData *ch, char *argument, int *first, int *second);
int group_size(CharData *ch);
int find_talent_num(char *name, int should_restrict);

/* function protos from this file */
int script_driver(void *go_address, TrigData *trig, int type, int mode);
void script_log(TrigData *t, char *msg);
CmdlistElement *find_done(CmdlistElement *cl);
CmdlistElement *find_case(TrigData *trig, CmdlistElement *cl, void *go, ScriptData *sc, int type, char *cond);
void var_subst(void *go, ScriptData *sc, TrigData *trig, int type, char *line, char *buf);

/* local structures */
struct WaitEventData {
    TrigData *trigger;
    void *go;
    int type;
};

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
CharData *find_char(int n) {
    CharData *ch;

    for (ch = character_list; ch; ch = ch->next)
        if (GET_ID(ch) == n)
            return (ch);

    return nullptr;
}

/* return object with UID n */
ObjData *find_obj(int n) {
    ObjData *i;

    for (i = object_list; i; i = i->next)
        if (n == GET_ID(i))
            return i;

    return nullptr;
}

/* return room with UID n */
RoomData *find_room(int n) {
    n -= ROOM_ID_BASE;

    if ((n >= 0) && (n <= top_of_world))
        return &world[n];

    return nullptr;
}

/************************************************************
 * generic searches based only on name                      *
 ************************************************************/

/* finds room by with name.  returns NULL if not found */
RoomData *get_room(char *name) {
    int nr;

    if (*name == UID_CHAR)
        return find_room(atoi(name + 1));
    else if (isdigit(*name) && (nr = real_room(atoi(name))) != NOWHERE)
        return &world[nr];
    else
        return nullptr;
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
                return true;
    return false;
}

FindContext find_dg_by_name(char *name) {
    FindContext context = find_by_name(name);
    if (*name == UID_CHAR)
        context.char_func = match_dg_vis_char_by_id;
    else
        context.char_func = match_dg_vis_char_by_name;
    return context;
}

/* checks every PLUSE_SCRIPT for random triggers */
void script_trigger_check(void) {
    CharData *ch;
    ObjData *obj;
    RoomData *room = nullptr;
    int nr;
    ScriptData *sc;

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
    WaitEventData *wait_event_obj = (WaitEventData *)event_obj;
    TrigData *trig;
    void *go;
    int type;

    trig = wait_event_obj->trigger;
    go = wait_event_obj->go;
    type = wait_event_obj->type;

    GET_TRIG_WAIT(trig) = nullptr;

    script_driver(&go, trig, type, TRIG_RESTART);
    return EVENT_FINISHED;
}

/* wait for casts...*/
void pause_while_casting(void *go, TrigData *trig, int type, CmdlistElement *cl) {
    WaitEventData *wait_event_obj;
    long time = 10L;

    CREATE(wait_event_obj, WaitEventData, 1);
    wait_event_obj->trigger = trig;
    wait_event_obj->go = go;
    wait_event_obj->type = type;

    GET_TRIG_WAIT(trig) = event_create(EVENT_TRIGGER_WAIT, trig_wait_event, wait_event_obj, true, nullptr, time);
    trig->curr_state = cl;
}

void do_stat_trigger(CharData *ch, TrigData *trig) {
    CmdlistElement *cmd_list;
    char sb[MAX_STRING_LENGTH_BIG];

    if (!trig) {
        log("SYSERR: NULL trigger passed to do_stat_trigger.");
        return;
    }

    get_char_cols(ch);

    char_printf(ch, "Trigger Name: '{}{}{}',  VNum: [{}{:5d}{}], RNum: [{:5d}]\n", yel, GET_TRIG_NAME(trig), nrm, grn,
                GET_TRIG_VNUM(trig), nrm, GET_TRIG_RNUM(trig));

    if (trig->attach_type == OBJ_TRIGGER) {
        char_printf(ch, "Trigger Intended Assignment: Objects\n");
        sprintbit(GET_TRIG_TYPE(trig), otrig_types, buf);
    } else if (trig->attach_type == WLD_TRIGGER) {
        char_printf(ch, "Trigger Intended Assignment: Rooms\n");
        sprintbit(GET_TRIG_TYPE(trig), wtrig_types, buf);
    } else {
        char_printf(ch, "Trigger Intended Assignment: Mobiles\n");
        sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);
    }

    sprintf(sb, "Trigger Type: %s, Numeric Arg: %d, Arg list: %s\n", buf, GET_TRIG_NARG(trig),
            ((GET_TRIG_ARG(trig) && *GET_TRIG_ARG(trig)) ? GET_TRIG_ARG(trig) : "None"));

    strcat(sb, "Commands:\n\n");

    cmd_list = trig->cmdlist;
    while (cmd_list) {
        if (cmd_list->cmd) {
            strcat(sb, escape_ansi(cmd_list->cmd).c_str());
            strcat(sb, "\n");
        }
        cmd_list = cmd_list->next;
    }
    page_string(ch, sb);
}

/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name) {
    CharData *ch;
    ObjData *obj;

    if ((ch = find_char_in_world(find_by_name(uid))))
        strcpy(name, GET_NAMELIST(ch));
    else if ((obj = find_obj_in_world(find_by_name(uid))))
        strcpy(name, obj->name);
    else
        sprintf(name, "uid = %s, (not found)", uid + 1);
}

/* general function to display stats on script sc */
void script_stat(CharData *ch, char *buf, ScriptData *sc) {
    TriggerVariableData *tv;
    TrigData *t;
    char name[MAX_INPUT_LENGTH];
    int found = 0;
    extern char *t_listdisplay(int nr, int index);

    get_char_cols(ch);

    buf += sprintf(buf, "Global Variables: %s\n", sc->global_vars ? "" : "None");

    for (tv = sc->global_vars; tv; tv = tv->next) {
        if (*(tv->value) == UID_CHAR) {
            find_uid_name(tv->value, name);
            buf += sprintf(buf, "    %15s:  %s\n", tv->name, name);
        } else
            buf += sprintf(buf, "    %15s:  %s\n", tv->name, tv->value);
    }

    for (t = TRIGGERS(sc); t; t = t->next) {
        buf += sprintf(buf, "%s", t_listdisplay(t->nr, ++found));

#if 1
        if (GET_TRIG_WAIT(t)) {
            buf += sprintf(buf,
                           "  Wait: %ld, Current line: %s\n"
                           "  Variables: %s\n",
                           event_time(GET_TRIG_WAIT(t)), t->curr_state->cmd, GET_TRIG_VARS(t) ? "" : "None");

            for (tv = GET_TRIG_VARS(t); tv; tv = tv->next) {
                if (*(tv->value) == UID_CHAR) {
                    find_uid_name(tv->value, name);
                    buf += sprintf(buf, "    %15s:  %s\n", tv->name, name);
                } else
                    buf += sprintf(buf, "    %15s:  %s\n", tv->name, tv->value);
            }
        }
#endif
    }
}

void do_sstat_room(CharData *ch, char *buf, RoomData *rm) {
    strcpy(buf, "Script information:\n");
    if (SCRIPT(rm))
        script_stat(ch, buf + strlen(buf), SCRIPT(rm));
    else
        strcat(buf, "  None.\n");
}

void do_sstat_object(CharData *ch, char *buf, ObjData *j) {
    strcpy(buf, "Script information:\n");
    if (!SCRIPT(j)) {
        strcat(buf, "  None.\n");
        return;
    }

    script_stat(ch, buf + strlen(buf), SCRIPT(j));
}

void do_sstat_character(CharData *ch, char *buf, CharData *k) {
    strcpy(buf, "Script information:\n");
    if (!SCRIPT(k)) {
        strcat(buf, "  None.\n");
        return;
    }

    script_stat(ch, buf + strlen(buf), SCRIPT(k));
}

/*
 * adds the trigger t to script sc in in location loc.  loc = -1 means
 * add to the end, loc = 0 means add before all other triggers.
 */
void add_trigger(ScriptData *sc, TrigData *t, int loc) {
    TrigData *i;
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
    CharData *victim;
    ObjData *obj;
    TrigData *trig;
    char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
    char loc_name[MAX_INPUT_LENGTH];
    int loc, room, tn, rn;

    argument = two_arguments(argument, arg, trig_name);
    two_arguments(argument, targ_name, loc_name);

    if (!*arg || !*targ_name || !*trig_name) {
        char_printf(ch, "Usage: attach { mtr | otr | wtr } { trigger } { name } [ location ]\n");
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
                        CREATE(SCRIPT(victim), ScriptData, 1);
                    add_trigger(SCRIPT(victim), trig, loc);

                    char_printf(ch, "Trigger {:d} ({}) attached to {}.\n", tn, GET_TRIG_NAME(trig), GET_SHORT(victim));
                } else
                    char_printf(ch, "That trigger does not exist.\n");
            } else
                char_printf(ch, "Players can't have scripts.\n");
        } else
            char_printf(ch, "That mob does not exist.\n");
    }

    else if (is_abbrev(arg, "otr")) {
        if ((obj = find_obj_around_char(ch, find_vis_by_name(ch, targ_name)))) {

            /* have a valid obj, now get trigger */
            rn = trig_index[tn] ? tn : -1;
            if ((rn >= 0) && (trig = read_trigger(rn))) {

                if (!SCRIPT(obj))
                    CREATE(SCRIPT(obj), ScriptData, 1);
                add_trigger(SCRIPT(obj), trig, loc);

                char_printf(ch, "Trigger {:d} ({}) attached to {}.\n", tn, GET_TRIG_NAME(trig),
                            (obj->short_description ? obj->short_description : obj->name));
            } else
                char_printf(ch, "That trigger does not exist.\n");
        } else
            char_printf(ch, "That object does not exist.\n");
    }

    else if (is_abbrev(arg, "wtr")) {
        if (isdigit(*targ_name) && !strchr(targ_name, '.')) {
            if ((room = find_target_room(ch, targ_name)) != NOWHERE) {

                /* have a valid room, now get trigger */
                rn = trig_index[tn] ? tn : -1;
                if ((rn >= 0) && (trig = read_trigger(rn))) {

                    if (!(world[room].script))
                        CREATE(world[room].script, ScriptData, 1);
                    add_trigger(world[room].script, trig, loc);

                    char_printf(ch, "Trigger {:d} ({}) attached to room {:d}.\n", tn, GET_TRIG_NAME(trig),
                                world[room].vnum);
                } else
                    char_printf(ch, "That trigger does not exist.\n");
            }
        } else
            char_printf(ch, "You need to supply a room number.\n");
    }

    else
        char_printf(ch, "Please specify 'mtr', otr', or 'wtr'.\n");
}

/* adds a variable with given name and value to trigger */
void add_var(TriggerVariableData **var_list, const char *name, const char *value) {
    TriggerVariableData *vd;

    for (vd = *var_list; vd && strcasecmp(vd->name, name); vd = vd->next)
        ;

    if (vd) {
        free(vd->value);
        CREATE(vd->value, char, strlen(value) + 1);
        strcpy(vd->value, value);
    }

    else {
        CREATE(vd, TriggerVariableData, 1);

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
int remove_trigger(ScriptData *sc, char *name) {
    TrigData *i, *j;
    int num = 0, string = false, n;
    char *cname;

    if (!sc)
        return 0;

    if ((cname = strcasestr(name, ".")) || (!isdigit(*name))) {
        string = true;
        if (cname) {
            *cname = '\0';
            num = atoi(name);
            name = ++cname;
        }
    } else
        num = atoi(name);

    for (n = 0, j = nullptr, i = TRIGGERS(sc); i; j = i, i = i->next) {
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
    CharData *victim = nullptr;
    ObjData *obj = nullptr;
    RoomData *room;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
    char *trigger = 0;

    argument = two_arguments(argument, arg1, arg2);
    one_argument(argument, arg3);

    if (!*arg1 || !*arg2) {
        char_printf(ch, "Usage: detach [ mob | object ] { target } { trigger | 'all' }\n");
        return;
    }

    if (!strcasecmp(arg1, "room")) {
        room = &world[IN_ROOM(ch)];
        if (!SCRIPT(room))
            char_printf(ch, "This room does not have any triggers.\n");
        else if (!strcasecmp(arg2, "all")) {
            extract_script(SCRIPT(room));
            SCRIPT(room) = nullptr;
            char_printf(ch, "All triggers removed from room.\n");
        }

        else if (remove_trigger(SCRIPT(room), arg2)) {
            char_printf(ch, "Trigger removed.\n");
            if (!TRIGGERS(SCRIPT(room))) {
                extract_script(SCRIPT(room));
                SCRIPT(room) = nullptr;
            }
        } else
            char_printf(ch, "That trigger was not found.\n");
    }

    else {
        if (is_abbrev(arg1, "mob")) {
            if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg2))))
                char_printf(ch, "No such mobile around.\n");
            else if (!*arg3)
                char_printf(ch, "You must specify a trigger to remove.\n");
            else
                trigger = arg3;
        }

        else if (is_abbrev(arg1, "object")) {
            if (!(obj = find_obj_around_char(ch, find_vis_by_name(ch, arg2))))
                char_printf(ch, "No such object around.\n");
            else if (!*arg3)
                char_printf(ch, "You must specify a trigger to remove.\n");
            else
                trigger = arg3;
        } else {
            if ((obj = find_obj_in_eq(ch, nullptr, find_vis_by_name(ch, arg1))))
                ;
            else if ((obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg1))))
                ;
            else if ((victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg1))))
                ;
            else if ((obj = find_obj_in_list(world[IN_ROOM(ch)].contents, find_vis_by_name(ch, arg1))))
                ;
            else if ((victim = find_char_around_char(ch, find_vis_by_name(ch, arg1))))
                ;
            else if ((obj = find_obj_in_world(find_vis_by_name(ch, arg1))))
                ;
            else
                char_printf(ch, "Nothing around by that name.\n");

            trigger = arg2;
        }

        if (victim) {
            if (!IS_NPC(victim))
                char_printf(ch, "Players don't have triggers.\n");

            else if (!SCRIPT(victim))
                char_printf(ch, "That mob doesn't have any triggers.\n");
            else if (!strcasecmp(arg2, "all")) {
                extract_script(SCRIPT(victim));
                SCRIPT(victim) = nullptr;
                char_printf(ch, "All triggers removed from {}.\n", GET_SHORT(victim));
            }

            else if (remove_trigger(SCRIPT(victim), trigger)) {
                char_printf(ch, "Trigger removed.\n");
                if (!TRIGGERS(SCRIPT(victim))) {
                    extract_script(SCRIPT(victim));
                    SCRIPT(victim) = nullptr;
                }
            } else
                char_printf(ch, "That trigger was not found.\n");
        }

        else if (obj) {
            if (!SCRIPT(obj))
                char_printf(ch, "That object doesn't have any triggers.\n");

            else if (!strcasecmp(arg2, "all")) {
                extract_script(SCRIPT(obj));
                SCRIPT(obj) = nullptr;
                char_printf(ch, "All triggers removed from {}.\n",
                            obj->short_description ? obj->short_description : obj->name);
            }

            else if (remove_trigger(SCRIPT(obj), trigger)) {
                char_printf(ch, "Trigger removed.\n");
                if (!TRIGGERS(SCRIPT(obj))) {
                    extract_script(SCRIPT(obj));
                    SCRIPT(obj) = nullptr;
                }
            } else
                char_printf(ch, "That trigger was not found.\n");
        }
    }
}

/* frees memory associated with var */
void free_var_el(TriggerVariableData *var) {
    free(var->name);
    free(var->value);
    free(var);
}

/*
 * remove var name from var_list
 * returns 1 if found, else 0
 */
int remove_var(TriggerVariableData **var_list, const char *name) {
    TriggerVariableData *i, *j;

    for (j = nullptr, i = *var_list; i && strcasecmp(name, i->name); j = i, i = i->next)
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
void script_log(TrigData *t, char *msg) {
    char buf[256];

    if (t)
        log(LogSeverity::Stat, LVL_GOD, "ERROR trigger {:d} ({}): {}", GET_TRIG_VNUM(t), GET_TRIG_NAME(t), msg);
    else
        log(LogSeverity::Stat, LVL_GOD, "ERROR in trigger: {}", msg);
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
        to_room = random_number(low, high);
    } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
             ROOM_FLAGGED(to_room, ROOM_GODROOM));

    return to_room;
}

#define UID_VAR(str, i) ((i) ? sprintf((str), "%c%ld", UID_CHAR, GET_ID(i)) : sprintf((str), "0"))
#define ROOM_UID_VAR(str, r) sprintf((str), "%c%ld", UID_CHAR, (long)r + ROOM_ID_BASE)

/* sets str to be the value of var.field */
void find_replacement(void *go, ScriptData *sc, TrigData *trig, int type, char *var, char *field, char *value,
                      char *str) {
    TriggerVariableData *vd;
    CharData *ch, *c = nullptr;
    ObjData *obj, *o = nullptr;
    RoomData *room, *r = nullptr;
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
        if (!strcasecmp(vd->name, var))
            break;

    /*
     * If no local variable was matched, see if there is a global variable
     * with the specified name.
     *
     * Some waitstates could crash the mud if sent here with sc == NULL.
     */
    if (!vd && sc)
        for (vd = sc->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, var))
                break;

    /*
     * Set 'self' variables for use below.  For example, if this is a mob
     * trigger, ch is the mob executing the trigger, and obj and room are
     * null.
     */
    switch (type) {
    case MOB_TRIGGER:
        ch = (CharData *)go;
        obj = nullptr;
        room = nullptr;
        break;
    case OBJ_TRIGGER:
        ch = nullptr;
        obj = (ObjData *)go;
        room = nullptr;
        break;
    case WLD_TRIGGER:
        ch = nullptr;
        obj = nullptr;
        room = (RoomData *)go;
        break;
    default:
        log("SYSERR: find_replacement encountered invalid trig type ({:d}) in trig {:d}", type, GET_TRIG_VNUM(trig));
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
            if (!strcasecmp(var, "self")) {
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
            else if (!strcasecmp(var, "damdone")) {
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
            if ((o = find_obj_in_eq(ch, nullptr, find_by_name(name))))
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
    else if (!strcasecmp(var, "self")) {
        c = ch;
        o = obj;
        r = room;
    }

    /*
     * These are 'static' variables that do not go into the char/obj/room
     * section below.
     */
    else {

        if (!strcasecmp(var, "time")) {
            if (!strcasecmp(field, "hour"))
                sprintf(str, "%d", time_info.hours);
            else if (!strcasecmp(field, "day"))
                sprintf(str, "%d", time_info.day);
            else if (!strcasecmp(field, "month"))
                sprintf(str, "%d", time_info.month);
            else if (!strcasecmp(field, "year"))
                sprintf(str, "%d", time_info.year);
            else if (!strcasecmp(field, "stamp")) {
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

        else if (!strcasecmp(var, "random")) {

            /* Pick a random character in the room */
            if (!strcasecmp(field, "char")) {
                if (type == MOB_TRIGGER)
                    c = get_random_char_around(ch, RAND_DG_MOB);
                else if (type == OBJ_TRIGGER)
                    c = get_random_char_around(world[obj_room(obj)].people, RAND_DG_OBJ);
                else if (type == WLD_TRIGGER)
                    c = get_random_char_around(room->people, RAND_DG_WLD);

                UID_VAR(str, c);
            }

            /* Locate a random room globally */
            else if (!strcasecmp(field, "room")) {
                do {
                    num = random_number(0, top_of_world);
                } while (ROOM_FLAGGED(num, ROOM_PRIVATE) || ROOM_FLAGGED(num, ROOM_DEATH) ||
                         ROOM_FLAGGED(num, ROOM_GODROOM));
                sprintf(str, "%d", world[num].vnum);
            }

            /* Pick a random room in the zone */
            else if (!strcasecmp(field, "room_in_zone")) {
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
                sprintf(str, "%d", ((num = atoi(field)) > 0) ? random_number(1, num) : 0);
        }

        /* Static functions */
        else if (!strcasecmp(var, "get")) {
            /* %get.obj_shortdesc[VNUM]% */
            if (!strcasecmp(field, "obj_shortdesc")) {
                if (is_positive_integer(value) && (num = real_object(atoi(value))) >= 0)
                    strcpy(str, obj_proto[num].short_description);
                else
                    sprintf(str, "[no description for object %s]", value);

            } else if (!strcasecmp(field, "obj_noadesc")) {
                /* %get.obj_noadesc[VNUM]% */
                if (is_positive_integer(value) && (num = real_object(atoi(value))) >= 0)
                    strcpy(str, without_article(obj_proto[num].short_description));
                else
                    sprintf(str, "[no description for object %s]", value);

            } else if (!strcasecmp(field, "obj_pldesc")) {
                /* %get.obj_pldesc[VNUM]% */
                if (is_positive_integer(value) && (num = real_object(atoi(value))) >= 0)
                    strcpy(str, pluralize(obj_proto[num].short_description));
                else
                    sprintf(str, "[no description for object %s]", value);

                /* %get.mob_shortdesc[VNUM]% */
            } else if (!strcasecmp(field, "mob_shortdesc")) {
                if (is_positive_integer(value) && (num = real_mobile(atoi(value))) >= 0)
                    strcpy(str, mob_proto[num].player.short_descr);
                else
                    sprintf(str, "[no description for mobile %s]", value);

                /* %get.obj_count[VNUM]% is the number of objects with VNUM in game */
            } else if (!strcasecmp(field, "obj_count")) {
                if ((num = real_object(atoi(value))) >= 0)
                    sprintf(str, "%d", obj_index[num].number);
                else
                    strcpy(str, "0");
                /* %get.mob_count[VNUM]% is the number of mobiles with VNUM in game */
            } else if (!strcasecmp(field, "mob_count")) {
                if ((num = real_mobile(atoi(value))) >= 0)
                    sprintf(str, "%d", mob_index[num].number);
                else
                    strcpy(str, "0");
                /* %get.room[VNUM]% returns a UID variable pointing to that room */
            } else if (!strcasecmp(field, "room")) {
                if ((num = real_room(atoi(value))) >= 0)
                    ROOM_UID_VAR(str, num);
                else
                    strcpy(str, "0");
                /* %get.people[VNUM]% is the number of people in room */
            } else if (!strcasecmp(field, "people")) {
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
            } else if (!strcasecmp(field, "opposite_dir")) {
                if ((num = search_block(value, dirs, false)) >= 0)
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
            } else if (!strcasecmp(field, "uidchar"))
                sprintf(str, "%c", UID_CHAR);
            else {
                *str = '\0';
                sprintf(buf2, "Unknown get field: '%s'", field);
                script_log(trig, buf2);
            }
        }

        /* String functions */
        else if (!strcasecmp(var, "string")) {

            if (!strcasecmp(field, "reverse")) {
                for (num = strlen(value) - 1; num >= 0; --num)
                    *(str++) = *(value + num);
                *str = '\0';
            }

            else if (!strcasecmp(field, "length"))
                sprintf(str, "%zu", strlen(value));

            else if (!strcasecmp(field, "tolower")) {
                do {
                    *(str++) = LOWER(*(value++));
                } while (*value);
            }

            else if (!strcasecmp(field, "toupper")) {
                do {
                    *(str++) = UPPER(*(value++));
                } while (*value);
            }

            else if (!strcasecmp(field, "cap") || !strcasecmp(field, "capitalize")) {
                strcpy(str, value);
                cap_by_color(str);
            }

            else if (!strcasecmp(field, "firstword"))
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
        if (!strcasecmp(field, "name"))
            strcpy(str, GET_SHORT(c) ? GET_SHORT(c) : GET_NAME(c));
        else if (!strcasecmp(field, "p") || !strcasecmp(field, "hisher"))
            strcpy(str, HSHR(c)); /* Possessive pronoun */
        else if (!strcasecmp(field, "o") || !strcasecmp(field, "himher"))
            strcpy(str, HMHR(c)); /* Objective pronoun */
        else if (!strcasecmp(field, "n") || !strcasecmp(field, "heshe"))
            strcpy(str, HSSH(c)); /* Nominative pronoun */
        else if (!strcasecmp(field, "alias"))
            strcpy(str, GET_NAME(c));
        else if (!strcasecmp(field, "title"))
            strcpy(str, GET_TITLE(c) ? GET_TITLE(c) : "");

        /* Identifying numbers */
        else if (!strcasecmp(field, "vnum"))
            sprintf(str, "%d", GET_MOB_VNUM(c));
        else if (!strcasecmp(field, "id"))
            sprintf(str, "%ld", GET_ID(c));

        /* Attributes */
        else if (!strcasecmp(field, "sex") || !strcasecmp(field, "gender"))
            strcpy(str, genders[(int)GET_SEX(c)]);
        else if (!strcasecmp(field, "class")) {
            strcpy(str, CLASS_PLAINNAME(c));
            cap_by_color(str);
        } else if (!strcasecmp(field, "race"))
            strcpy(str, races[(int)GET_RACE(c)].name);
        else if (!strcasecmp(field, "level"))
            sprintf(str, "%d", GET_LEVEL(c));

        else if (!strcasecmp(field, "weight"))
            sprintf(str, "%d", GET_WEIGHT(c));
        else if (!strcasecmp(field, "height"))
            sprintf(str, "%d", GET_HEIGHT(c));
        else if (!strcasecmp(field, "size"))
            sprintf(str, "%s", SIZE_DESC(c));

        else if (!strcasecmp(field, "cha"))
            sprintf(str, "%d", GET_VIEWED_CHA(c));
        else if (!strcasecmp(field, "str"))
            sprintf(str, "%d", GET_VIEWED_STR(c));
        else if (!strcasecmp(field, "int"))
            sprintf(str, "%d", GET_VIEWED_INT(c));
        else if (!strcasecmp(field, "wis"))
            sprintf(str, "%d", GET_VIEWED_WIS(c));
        else if (!strcasecmp(field, "con"))
            sprintf(str, "%d", GET_VIEWED_CON(c));
        else if (!strcasecmp(field, "dex"))
            sprintf(str, "%d", GET_VIEWED_DEX(c));

        else if (!strcasecmp(field, "real_cha"))
            sprintf(str, "%d", GET_CHA(c));
        else if (!strcasecmp(field, "real_str"))
            sprintf(str, "%d", GET_STR(c));
        else if (!strcasecmp(field, "real_int"))
            sprintf(str, "%d", GET_INT(c));
        else if (!strcasecmp(field, "real_wis"))
            sprintf(str, "%d", GET_WIS(c));
        else if (!strcasecmp(field, "real_con"))
            sprintf(str, "%d", GET_CON(c));
        else if (!strcasecmp(field, "real_dex"))
            sprintf(str, "%d", GET_DEX(c));

        else if (!strcasecmp(field, "hit"))
            sprintf(str, "%d", GET_HIT(c));
        else if (!strcasecmp(field, "maxhit"))
            sprintf(str, "%d", GET_MAX_HIT(c));
        else if (!strcasecmp(field, "move"))
            sprintf(str, "%d", GET_MOVE(c));
        else if (!strcasecmp(field, "maxmove"))
            sprintf(str, "%d", GET_MAX_MOVE(c));
        else if (!strcasecmp(field, "armor"))
            sprintf(str, "%d", GET_AC(c));
        else if (!strcasecmp(field, "hitroll"))
            sprintf(str, "%d", GET_HITROLL(c));
        else if (!strcasecmp(field, "damroll"))
            sprintf(str, "%d", GET_DAMROLL(c));
        else if (!strcasecmp(field, "exp"))
            sprintf(str, "%ld", GET_EXP(c));
        else if (!strcasecmp(field, "perception"))
            sprintf(str, "%ld", GET_PERCEPTION(c));
        else if (!strcasecmp(field, "hiddenness"))
            sprintf(str, "%ld", GET_HIDDENNESS(c));
        else if (!strcasecmp(field, "align") || !strcasecmp(field, "alignment"))
            sprintf(str, "%d", GET_ALIGNMENT(c));
        else if (!strcasecmp(field, "composition"))
            strcpy(str, compositions[(int)GET_COMPOSITION(c)].name);
        else if (!strcasecmp(field, "lifeforce"))
            strcpy(str, lifeforces[(int)GET_LIFEFORCE(c)].name);

        else if (is_coin_name(field, PLATINUM))
            sprintf(str, "%d", GET_PLATINUM(c));
        else if (is_coin_name(field, GOLD))
            sprintf(str, "%d", GET_GOLD(c));
        else if (is_coin_name(field, SILVER))
            sprintf(str, "%d", GET_SILVER(c));
        else if (is_coin_name(field, COPPER))
            sprintf(str, "%d", GET_COPPER(c));

        /* Flags */
        else if (!strcasecmp(field, "flags")) {
            *str = '\0';
            if (IS_NPC(c)) /* ACT flags */
                sprintflag(str, MOB_FLAGS(c), NUM_MOB_FLAGS, action_bits);
            else { /* concatenation of PLR and PRF flags */
                if (HAS_FLAGS(PLR_FLAGS(c), NUM_PLR_FLAGS) || !HAS_FLAGS(PRF_FLAGS(c), NUM_PRF_FLAGS))
                    sprintflag(str, PLR_FLAGS(c), NUM_PLR_FLAGS, player_bits);
                if (HAS_FLAGS(PRF_FLAGS(c), NUM_PRF_FLAGS))
                    sprintflag(str + strlen(str), PRF_FLAGS(c), NUM_PRF_FLAGS, preference_bits);
            }
        } else if (!strcasecmp(field, "flagged")) {
            if (IS_NPC(c)) {
                if ((num = search_block(value, action_bits, false)) >= 0)
                    strcpy(str, MOB_FLAGGED(c, num) ? "1" : "0");
                else {
                    strcpy(str, "0");
                    sprintf(buf2, "unrecognized NPC flag '%s' to %%%s.flagged[]%%", value, var);
                    script_log(trig, buf2);
                }
            } else {
                if ((num = search_block(value, player_bits, false)) >= 0)
                    strcpy(str, PLR_FLAGGED(c, (1 << num)) ? "1" : "0");
                else if ((num = search_block(value, preference_bits, false)) >= 0)
                    strcpy(str, PRF_FLAGGED(c, (1 << num)) ? "1" : "0");
                else {
                    strcpy(str, "0");
                    sprintf(buf2, "unrecognized player or preference flag '%s' to %%%s.flagged[]%%", value, var);
                    script_log(trig, buf2);
                }
            }
        } else if (!strcasecmp(field, "aff_flags") || !strcasecmp(field, "eff_flags"))
            sprintflag(str, EFF_FLAGS(c), NUM_EFF_FLAGS, effect_flags);

        else if (!strcasecmp(field, "aff_flagged") || !strcasecmp(field, "eff_flagged")) {
            if ((num = search_block(value, effect_flags, false)) >= 0)
                strcpy(str, EFF_FLAGGED(c, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized effect flag '%s' to %%%s.eff_flagged[]%%", value, var);
                script_log(trig, buf2);
            }

        } else if (!strcasecmp(field, "spells")) {
            effect *eff;
            *str = '\0';
            for (eff = c->effects; eff; eff = eff->next)
                if (eff->duration >= 0 && (!eff->next || eff->next->type != eff->type)) {
                    strcat(str, skills[eff->type].name);
                    strcat(str, " ");
                }
        } else if (!strcasecmp(field, "has_spell")) {
            if ((num = find_talent_num(value, 0)) >= 0)
                strcpy(str, affected_by_spell(c, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized spell '%s' to %%%s.has_spell[]%%", value, var);
                script_log(trig, buf2);
            }
        }

        /* Character relationships */
        else if (!strcasecmp(field, "fighting"))
            UID_VAR(str, FIGHTING(c));
        else if (!strcasecmp(field, "hunting"))
            UID_VAR(str, HUNTING(c));
        else if (!strcasecmp(field, "riding"))
            UID_VAR(str, RIDING(c));
        else if (!strcasecmp(field, "ridden_by"))
            UID_VAR(str, RIDDEN_BY(c));
        else if (!strcasecmp(field, "consented"))
            UID_VAR(str, CONSENT(c));
        else if (!strcasecmp(field, "master"))
            UID_VAR(str, c->master);
        else if (!strcasecmp(field, "next_in_room")) {
            /* Skip any wiz-invis folks */
            while (c->next_in_room && GET_INVIS_LEV(c->next_in_room))
                c = c->next_in_room;
            UID_VAR(str, c->next_in_room);
        } else if (!strcasecmp(field, "group_size"))
            sprintf(str, "%d", group_size(c));
        else if (!strcasecmp(field, "group_member")) {
            ch = c->group_master ? c->group_master : c;

            num = atoi(value);

            if (!IS_GROUPED(ch) || num <= 0)
                UID_VAR(str, c);
            else if (num == 1)
                UID_VAR(str, ch);
            else {
                GroupType *g;
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
        else if (!strcasecmp(field, "quest_variable")) {
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

        else if (!strcasecmp(field, "quest_stage")) {
            if (!*value) {
                script_log(trig, "quest_stage called without specifying a quest");
                strcpy(str, "0");
            } else if (IS_NPC(c))
                strcpy(str, "0");
            else
                sprintf(str, "%d", quest_stage(c, value));
        }

        else if (!strcasecmp(field, "has_completed")) {
            if (!*value) {
                script_log(trig, "has_completed called without specifying a quest");
                strcpy(str, "0");
            } else if (IS_NPC(c))
                strcpy(str, "0");
            else
                strcpy(str, has_completed_quest(value, c) ? "1" : "0");
        }

        else if (!strcasecmp(field, "has_failed")) {
            if (!*value) {
                script_log(trig, "has_failed called without specifying a quest");
                strcpy(str, "0");
            } else if (IS_NPC(c))
                strcpy(str, "0");
            else
                strcpy(str, has_failed_quest(value, c) ? "1" : "0");
        }

        /* Object relationships */
        else if (!strcasecmp(field, "inventory")) {
            if (!strcasecmp(value, "count"))
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
        } else if (!strcasecmp(field, "worn")) {
            int pos;
            if (!strcasecmp(value, "count")) {
                for (num = pos = 0; pos < NUM_WEARS; ++pos)
                    if (GET_EQ(c, pos))
                        ++num;
                sprintf(str, "%d", num);
            } else if ((pos = search_block(value, wear_positions, true)) >= 0)
                UID_VAR(str, GET_EQ(c, pos));
            else
                strcpy(str, "0");
        } else if (!strcasecmp(field, "wearing")) {
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

        else if (!strcasecmp(field, "position")) {
            strcpy(str, position_types[(int)GET_POS(c)]);
        } else if (!strcasecmp(field, "stance"))
            strcpy(str, stance_types[(int)GET_STANCE(c)]);

        else if (!strcasecmp(field, "room")) {
            if (IN_ROOM(c) >= 0 && IN_ROOM(c) <= top_of_world)
                sprintf(str, "%d", world[IN_ROOM(c)].vnum);
            else
                strcpy(str, "-1");
        }

        else if (!strcasecmp(field, "talent") || !strcasecmp(field, "skill")) {
            int talent = find_talent_num(value, 0);
            if (talent < 0)
                strcpy(str, "0");
            else
                sprintf(str, "%d", GET_SKILL(c, talent));
        }

        else if (!strcasecmp(field, "clan")) {
            if (!IS_NPC(c) && GET_CLAN(c))
                strcpy(str, GET_CLAN(c)->name);
            else
                *str = '\0';
        } else if (!strcasecmp(field, "clan_rank"))
            sprintf(str, "%d", IS_NPC(c) ? 0 : GET_CLAN_RANK(c));

        else if (!strcasecmp(field, "can_be_seen"))
            strcpy(str, type == MOB_TRIGGER && !CAN_SEE(ch, c) ? "0" : "1");

        else if (!strcasecmp(field, "trophy")) {
            if (IS_NPC(c))
                *str = '\0';
            else {
                TrophyNode *node;
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
        if (!strcasecmp(field, "name"))
            strcpy(str, o->name);
        else if (!strcasecmp(field, "shortdesc"))
            strcpy(str, o->short_description);
        else if (!strcasecmp(field, "description"))
            strcpy(str, o->description);

        /* Identifying numbers */
        else if (!strcasecmp(field, "vnum"))
            sprintf(str, "%d", GET_OBJ_VNUM(o));
        else if (!strcasecmp(field, "type"))
            strcpy(str, OBJ_TYPE_NAME(o));
        else if (!strcasecmp(field, "id"))
            sprintf(str, "%ld", GET_ID(o));

        /* Numerical attributes */
        else if (!strcasecmp(field, "weight"))
            sprintf(str, "%.2f", o->obj_flags.weight);
        else if (!strcasecmp(field, "cost"))
            sprintf(str, "%d", GET_OBJ_COST(o));
        else if (!strcasecmp(field, "cost_per_day") || !strcasecmp(field, "rent"))
            strcpy(str, "0");
        else if (!strcasecmp(field, "level"))
            sprintf(str, "%d", GET_OBJ_LEVEL(o));
        else if (!strcasecmp(field, "val0"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 0));
        else if (!strcasecmp(field, "val1"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 1));
        else if (!strcasecmp(field, "val2"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 2));
        else if (!strcasecmp(field, "val3"))
            sprintf(str, "%d", GET_OBJ_VAL(o, 3));
        else if (!strcasecmp(field, "timer"))
            sprintf(str, "%d", GET_OBJ_TIMER(o));
        else if (!strcasecmp(field, "decomp"))
            sprintf(str, "%d", GET_OBJ_DECOMP(o));
        else if (!strcasecmp(field, "hiddenness"))
            sprintf(str, "%ld", GET_OBJ_HIDDENNESS(o));
        else if (!strcasecmp(field, "affect") || !strcasecmp(field, "effect")) {
            if (!is_positive_integer(value) || (num = atoi(value)) > 5)
                *str = '\0';
            else
                sprintf(str, "%+d %s", o->applies[num].modifier, apply_types[(int)o->applies[num].location]);
        } else if (!strcasecmp(field, "affect_value") || !strcasecmp(field, "effect_value"))
            sprintf(str, "%d", is_positive_integer(value) && (num = atoi(value) <= 5) ? o->applies[num].modifier : 0);

        /* Flags */
        else if (!strcasecmp(field, "flags"))
            sprintflag(str, GET_OBJ_FLAGS(o), NUM_ITEM_FLAGS, extra_bits);
        else if (!strcasecmp(field, "flagged")) {
            if ((num = search_block(value, extra_bits, false)) >= 0)
                strcpy(str, OBJ_FLAGGED(o, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized object extra bit '%s' to %%%s.flagged[]%%", value, var);
                script_log(trig, buf2);
            }
        } else if (!strcasecmp(field, "spells"))
            sprintflag(str, GET_OBJ_EFF_FLAGS(o), NUM_EFF_FLAGS, effect_flags);

        else if (!strcasecmp(field, "has_spell")) {
            if ((num = search_block(value, effect_flags, false)) >= 0)
                strcpy(str, OBJ_EFF_FLAGGED(o, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized effect flag '%s' to %%%s.has_spell[]%%", value, var);
                script_log(trig, buf2);
            }
        }

        /* Location */
        else if (!strcasecmp(field, "room")) {
            num = obj_room(o);
            if (num != NOWHERE)
                sprintf(str, "%d", world[num].vnum);
            else
                strcpy(str, "-1");
        } else if (!strcasecmp(field, "carried_by"))
            UID_VAR(str, o->carried_by);
        else if (!strcasecmp(field, "worn_by"))
            UID_VAR(str, o->worn_by);
        else if (!strcasecmp(field, "worn_on")) {
            if (o->worn_by)
                sprinttype(o->worn_on, wear_positions, str);
            else
                *str = '\0';
        } else if (!strcasecmp(field, "contents")) {
            if (!strcasecmp(value, "count")) {
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
        } else if (!strcasecmp(field, "next_in_list"))
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
        if (!strcasecmp(field, "name"))
            strcpy(str, r->name);

        else if (!strcasecmp(field, "vnum"))
            sprintf(str, "%d", r->vnum);
        else if (!strncasecmp(field, "sector", 6))
            sprintf(str, "%s", sectors[r->sector_type].name);
        else if (!strcasecmp(field, "is_dark"))
            strcpy(str, r->light > 0 ? "1" : "0");

        else if (!strcasecmp(field, "flags"))
            sprintflag(str, r->room_flags, NUM_ROOM_FLAGS, room_bits);
        else if (!strcasecmp(field, "flagged")) {
            if ((num = search_block(value, room_bits, false)) >= 0)
                strcpy(str, IS_FLAGGED(r->room_flags, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized room flag '%s' to %%%s.flagged%%", value, var);
                script_log(trig, buf2);
            }
        } else if (!strcasecmp(field, "effects") || !strcasecmp(field, "affects"))
            sprintflag(str, r->room_effects, NUM_ROOM_EFF_FLAGS, room_effects);
        else if (!strcasecmp(field, "has_effect") || !strcasecmp(field, "has_affect")) {
            if ((num = search_block(value, room_effects, false)) >= 0)
                strcpy(str, IS_FLAGGED(r->room_effects, num) ? "1" : "0");
            else {
                strcpy(str, "0");
                sprintf(buf2, "unrecognized room effect flag '%s' to %%%s.has_effect%%", value, var);
                script_log(trig, buf2);
            }
        }

        else if (!strcasecmp(field, "objects")) {
            if (!strcasecmp(value, "count")) {
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
        } else if (!strcasecmp(field, "people")) {
            if (!strcasecmp(value, "count")) {
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
        else if ((num = search_block(field, dirs, true)) >= 0) {
            if (!r->exits[num])
                strcpy(str, "-1");
            else if (!*value) /* %room.DIR% is a vnum */
                sprintf(str, "%d", r->exits[num]->to_room != NOWHERE ? world[r->exits[num]->to_room].vnum : -1);
            else if (!strcasecmp(value, "room")) { /* %room.DIR[room]% */
                if (r->exits[num]->to_room != NOWHERE)
                    ROOM_UID_VAR(str, r->exits[num]->to_room);
                else
                    strcpy(str, "0");
            } else if (!strcasecmp(value, "key")) /* %room.DIR[key]% */
                sprintf(str, "%d", r->exits[num]->key);
            else if (!strcasecmp(value, "bits")) /* %room.DIR[bits]% */
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
void var_subst(void *go, ScriptData *sc, TrigData *trig, int type, char *line, char *buf) {
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
        return false;

    if (*num == '+' || *num == '-')
        ++num;

    for (; *num != '\0'; ++num)
        if (!isdigit(*num))
            return false;

    return true;
}

/* evaluates 'lhs op rhs', and copies to result */
void eval_op(char *op, char *lhs, char *rhs, char *result, void *go, ScriptData *sc, TrigData *trig) {
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
    if (!strcasecmp("||", op)) {
        if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
            strcpy(result, "0");
        else
            strcpy(result, "1");
    }

    else if (!strcasecmp("&&", op)) {
        if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
            strcpy(result, "0");
        else
            strcpy(result, "1");
    }

    else if (!strcasecmp("==", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) == atoi(rhs));
        else
            sprintf(result, "%d", !strcasecmp(lhs, rhs));
    }

    else if (!strcasecmp("!=", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) != atoi(rhs));
        else
            sprintf(result, "%d", strcasecmp(lhs, rhs));
    }

    else if (!strcasecmp("<=", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
        else
            sprintf(result, "%d", strcasecmp(lhs, rhs) <= 0);
    }

    else if (!strcasecmp(">=", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
        else
            sprintf(result, "%d", strcasecmp(lhs, rhs) <= 0);
    }

    else if (!strcasecmp("<", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) < atoi(rhs));
        else
            sprintf(result, "%d", strcasecmp(lhs, rhs) < 0);
    }

    else if (!strcasecmp(">", op)) {
        if (is_num(lhs) && is_num(rhs))
            sprintf(result, "%d", atoi(lhs) > atoi(rhs));
        else
            sprintf(result, "%d", strcasecmp(lhs, rhs) > 0);
    }

    else if (!strcasecmp("/=", op))
        sprintf(result, "%c", strcasestr(lhs, rhs) ? '1' : '0');

    else if (!strcasecmp("*", op))
        sprintf(result, "%d", atoi(lhs) * atoi(rhs));

    else if (!strcasecmp("/", op))
        sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);

    else if (!strcasecmp("+", op))
        sprintf(result, "%d", atoi(lhs) + atoi(rhs));

    else if (!strcasecmp("-", op))
        sprintf(result, "%d", atoi(lhs) - atoi(rhs));

    else if (!strcasecmp("!", op)) {
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
void eval_expr(char *line, char *result, void *go, ScriptData *sc, TrigData *trig, int type) {
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
int eval_lhs_op_rhs(char *expr, char *result, void *go, ScriptData *sc, TrigData *trig, int type) {
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
    tokens[j] = nullptr;

    for (i = 0; *ops[i] != '\n'; i++)
        for (j = 0; tokens[j]; j++)
            if (!strncasecmp(ops[i], tokens[j], strlen(ops[i]))) {
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
int process_if(char *cond, void *go, ScriptData *sc, TrigData *trig, int type) {
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
CmdlistElement *find_end(TrigData *trig, CmdlistElement *cl) {
    CmdlistElement *c;
    char *p;

    if (!(cl->next)) {
        script_log(trig, "'if' without 'end'. (error 1)");
        return cl;
    }

    for (c = cl->next; c; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ;

        if (!strncasecmp("if ", p, 3))
            c = find_end(trig, c);
        else if (!strncasecmp("end", p, 3))
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
CmdlistElement *find_else_end(TrigData *trig, CmdlistElement *cl, void *go, ScriptData *sc, int type) {
    CmdlistElement *c;
    char *p;

    if (!(cl->next))
        return cl;

    for (c = cl->next; c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ; /* skip spaces */

        if (!strncasecmp("if ", p, 3))
            c = find_end(trig, c);

        else if (!strncasecmp("elseif ", p, 7)) {
            if (process_if(p + 7, go, sc, trig, type)) {
                GET_TRIG_DEPTH(trig)++;
                return c;
            }
        }

        else if (!strncasecmp("else", p, 4)) {
            GET_TRIG_DEPTH(trig)++;
            return c;
        }

        else if (!strncasecmp("end", p, 3))
            return c;

        if (!c->next) {
            script_log(trig, "'if' without 'end'. (error 4)");
            return c;
        }
    }

    /* If we got here, it's the last line, if it's not an end, log it. */
    for (p = c->cmd; *p && isspace(*p); ++p)
        ; /* skip spaces */
    if (strncasecmp("end", p, 3))
        script_log(trig, "'if' without 'end'. (error 5)");

    return c;
}

/* processes any 'wait' commands in a trigger */
void process_wait(void *go, TrigData *trig, int type, char *cmd, CmdlistElement *cl) {
    char buf[MAX_INPUT_LENGTH], *arg;
    WaitEventData *wait_event_obj;
    long time, hr, min, ntime;
    char c;

    arg = any_one_arg(cmd, buf);
    skip_spaces(&arg);

    if (!*arg) {
        sprintf(buf2, "Wait w/o an arg: '%s'", cl->cmd);
        script_log(trig, buf2);
        return;
    }

    else if (!strncasecmp(arg, "until ", 6)) {

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

    CREATE(wait_event_obj, WaitEventData, 1);
    wait_event_obj->trigger = trig;
    wait_event_obj->go = go;
    wait_event_obj->type = type;

    GET_TRIG_WAIT(trig) = event_create(EVENT_TRIGGER_WAIT, trig_wait_event, wait_event_obj, true, nullptr, time);
    trig->curr_state = cl->next;
}

/* processes a script set command */
void process_set(ScriptData *sc, TrigData *trig, char *cmd) {
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
void process_eval(void *go, ScriptData *sc, TrigData *trig, int type, char *cmd) {
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
int process_return(TrigData *trig, char *cmd) {
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
void process_unset(ScriptData *sc, TrigData *trig, char *cmd) {
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
void process_global(ScriptData *sc, TrigData *trig, char *cmd) {
    TriggerVariableData *vd;
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
            if (!strcasecmp(vd->name, arg))
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
int script_driver(void *go_address, TrigData *trig, int type, int mode) {
    static int depth = 0;
    int ret_val = 1;
    CmdlistElement *cl;
    char cmd[MAX_INPUT_LENGTH], *p;
    ScriptData *sc = 0;
    CmdlistElement *temp;
    unsigned long loops = 0;
    void *go = nullptr;

    void obj_command_interpreter(ObjData * obj, TrigData * t, char *argument);
    void wld_command_interpreter(RoomData * room, TrigData * t, char *argument);

    if (depth > MAX_SCRIPT_DEPTH) {
        switch (type) {
        case MOB_TRIGGER:
            sprintf(buf, "Triggers recursed beyond maximum allowed depth on mob %d", GET_MOB_VNUM((CharData *)go));
            break;
        case OBJ_TRIGGER:
            sprintf(buf, "Triggers recursed beyond maximum allowed depth on obj %d", GET_OBJ_VNUM((ObjData *)go));
            break;
        case WLD_TRIGGER:
            sprintf(buf, "Triggers recursed beyond maximum allowed depth in room %d", ((RoomData *)go)->vnum);
            break;
        }
        script_log(trig, buf);
        return ret_val;
    }

    depth++;

    switch (type) {
    case MOB_TRIGGER:
        go = *(CharData **)go_address;
        sc = SCRIPT((CharData *)go);
        break;
    case OBJ_TRIGGER:
        go = *(ObjData **)go_address;
        sc = SCRIPT((ObjData *)go);
        break;
    case WLD_TRIGGER:
        go = *(RoomData **)go_address;
        sc = SCRIPT((RoomData *)go);
        break;
    }

    if (mode == TRIG_NEW) {
        GET_TRIG_DEPTH(trig) = 1;
        GET_TRIG_LOOPS(trig) = 0;
    }

    trig->running = true;
    for (cl = (mode == TRIG_NEW) ? trig->cmdlist : trig->curr_state; cl && GET_TRIG_DEPTH(trig); cl = cl->next) {
        /* no point in continuing if the mob has zapped itself... */
        if (trig->purged)
            break;

        if (type == MOB_TRIGGER && !(trig->trigger_type & MTRIG_DEATH)) { /* only death trigs are immune to all tests */
            if (!AWAKE((CharData *)go)) {
                depth--;
                if (mode == TRIG_NEW)
                    GET_TRIG_DEPTH(trig) = 0; /* reset trigger totally if instant bail */
                return 0;
            }

            if (CASTING((CharData *)go)) {
                pause_while_casting(go, trig, type, cl);
                depth--;
                return ret_val;
            }
        }
        for (p = cl->cmd; *p && isspace(*p); p++)
            ;

        if (*p == '*')
            continue;

        else if (!strncasecmp(p, "if ", 3)) {
            if (process_if(p + 3, go, sc, trig, type))
                GET_TRIG_DEPTH(trig)++;
            else
                cl = find_else_end(trig, cl, go, sc, type);
        }

        else if (!strncasecmp("elseif ", p, 7) || !strncasecmp("else", p, 4)) {
            /* If not in an if-block, ignore the extra 'else[if]' and warn about it.
             */
            if (GET_TRIG_DEPTH(trig) == 1) {
                script_log(trig, "'else' without 'if'.");
                continue;
            }
            cl = find_end(trig, cl);
            GET_TRIG_DEPTH(trig)--;
        } else if (!strncasecmp("while ", p, 6)) {
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
        } else if (!strncasecmp("switch ", p, 7)) {
            cl = find_case(trig, cl, go, sc, type, p + 7);
        } else if (!strncasecmp("end", p, 3)) {
            if (GET_TRIG_DEPTH(trig) == 1) {
                script_log(trig, "'end' without 'if'.");
                continue;
            }
            GET_TRIG_DEPTH(trig)--;
        } else if (!strncasecmp("done", p, 4)) {
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
        } else if (!strncasecmp("break", p, 5)) {
            cl = find_done(cl);
        } else if (!strncasecmp("case", p, 4)) {
            /* Do nothing, this allows multiple cases to a single instance */
        }

        else {

            var_subst(go, sc, trig, type, p, cmd);

            if (!strncasecmp(cmd, "eval ", 5))
                process_eval(go, sc, trig, type, cmd);

            else if (!strncasecmp(cmd, "halt", 4))
                break;

            else if (!strncasecmp(cmd, "global ", 7))
                process_global(sc, trig, cmd);

            else if (!strncasecmp(cmd, "return ", 7))
                ret_val = process_return(trig, cmd);

            else if (!strncasecmp(cmd, "set ", 4))
                process_set(sc, trig, cmd);

            else if (!strncasecmp(cmd, "unset ", 6))
                process_unset(sc, trig, cmd);

            else if (!strncasecmp(cmd, "wait ", 5)) {
                process_wait(go, trig, type, cmd, cl);
                depth--;
                return ret_val;
            }

            else
                switch (type) {
                case MOB_TRIGGER:
                    command_interpreter((CharData *)go, cmd);
                    break;
                case OBJ_TRIGGER:
                    obj_command_interpreter((ObjData *)go, trig, cmd);
                    break;
                case WLD_TRIGGER:
                    wld_command_interpreter((RoomData *)go, trig, cmd);
                    break;
                }
        }
    }
    trig->running = false;

    if (trig->purged) {
        free_trigger(trig);
        go_address = nullptr;
    } else {
        free_varlist(GET_TRIG_VARS(trig));
        GET_TRIG_VARS(trig) = nullptr;
        GET_TRIG_DEPTH(trig) = 0;
    }

    depth--;
    return ret_val;
}

int real_trigger(int vnum) {
    int rnum;

    for (rnum = 0; rnum < top_of_trigt; rnum++) {
        if (trig_index[rnum]->vnum == vnum)
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
            char_printf(ch, "That vnum does not exist.\n");
            return;
        }

        do_stat_trigger(ch, trig_index[rnum]->proto);
    } else
        char_printf(ch, "Usage: tstat <vnum>\n");
}

/*
 * scans for a case/default instance
 * returns the line containg the correct case instance, or the last
 * line of the trigger if not found.
 */
CmdlistElement *find_case(TrigData *trig, CmdlistElement *cl, void *go, ScriptData *sc, int type, char *cond) {
    char cond_expr[MAX_INPUT_LENGTH], *p;
    CmdlistElement *c;

    if (!(cl->next))
        return cl;

    eval_expr(cond, cond_expr, go, sc, trig, type);

    for (c = cl->next; c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ;

        if (!strncasecmp("while ", p, 6) || !strncasecmp("switch", p, 6))
            c = find_done(c);
        else if (!strncasecmp("case ", p, 5)) {
            char case_expr[MAX_STRING_LENGTH];
            char result[16]; /* == always returns an integer, so it shouuld be safe */
            eval_expr(p + 5, case_expr, go, sc, trig, type);
            eval_op("==", cond_expr, case_expr, result, go, sc, trig);
            if (*result && *result != '0')
                return c;
        } else if (!strncasecmp("default", p, 7))
            return c;
        else if (!strncasecmp("done", p, 3))
            return c;
    }
    return c;
}

/*
 * scans for end of while/switch-blocks.
 * returns the line containg 'end', or the last
 * line of the trigger if not found.
 */
CmdlistElement *find_done(CmdlistElement *cl) {
    CmdlistElement *c;
    char *p;

    if (!cl || !(cl->next))
        return cl;

    for (c = cl->next; c && c->next; c = c->next) {
        for (p = c->cmd; *p && isspace(*p); p++)
            ;

        if (!strncasecmp("while ", p, 6) || !strncasecmp("switch ", p, 7))
            c = find_done(c);
        else if (!strncasecmp("done", p, 3))
            return c;
    }

    return c;
}

void check_time_triggers(void) {
    CharData *ch;
    ObjData *obj;
    RoomData *room = nullptr;
    int nr;
    ScriptData *sc;

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

/***************************************************************************
 *   File: dg_wldcmd.c                                    Part of FieryMUD *
 *  Usage: contains the command_interpreter for rooms,room commands.       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *                                                                         *
 * This code was received origonally from HubisMUD in 1998 and no lable or *
 * claim of ownership or copyright was made anywhere in the file.          *
 ***************************************************************************/

#include "chars.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "movement.hpp"
#include "olc.hpp" /* for real_zone */
#include "pfiles.hpp"
#include "players.hpp"
#include "quest.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

void sub_write(char *arg, CharData *ch, byte find_invis, int targets);
long asciiflag_conv(char *flag);
RoomData *get_room(char *name);
int script_driver(void *go_address, TrigData *trig, int type, int mode);
int get_room_location(char *name);
FindContext find_dg_by_name(char *name);

#define WCMD(name) void(name)(RoomData * room, TrigData * t, char *argument, int cmd, int subcmd)

struct WorldCommandInfo {
    char *command;
    void (*command_pointer)(RoomData *room, TrigData *t, char *argument, int cmd, int subcmd);
    int subcmd;
};

/* do_wsend */
#define SCMD_WSEND 0
#define SCMD_WECHOAROUND 1

/* attaches room vnum to msg and sends it to script_log */
void wld_log(RoomData *room, TrigData *t, char *msg) {
    char buf[MAX_INPUT_LENGTH + 100];

    void script_log(TrigData * t, char *msg);

    sprintf(buf, "(TRG)(room %d): %s", room->vnum, msg);
    script_log(t, buf);
}

/* sends str to room */
void act_to_room(char *str, RoomData *room) {
    /* no one is in the room */
    if (!room->people)
        return;

    /*
     * since you can't use act(..., TO_ROOM) for an room, send it
     * TO_ROOM and TO_CHAR for some char in the room.
     * (just dont use $n or you might get strange results)
     */
    act(str, false, room->people, 0, 0, TO_ROOM);
    act(str, false, room->people, 0, 0, TO_CHAR);
}

/* World commands */

/* prints the argument to all the rooms aroud the room */
WCMD(do_wasound) {
    int door;

    skip_spaces(&argument);

    if (!*argument) {
        wld_log(room, t, "wasound called with no argument");
        return;
    }

    for (door = 0; door < NUM_OF_DIRS; door++) {
        Exit *exit;

        if ((exit = room->exits[door]) && (exit->to_room != NOWHERE) && room != &world[exit->to_room])
            act_to_room(argument, &world[exit->to_room]);
    }
}

WCMD(do_wecho) {
    skip_spaces(&argument);

    if (!*argument)
        wld_log(room, t, "wecho called with no args");

    else
        act_to_room(argument, room);
}

WCMD(do_wsend) {
    char buf[MAX_INPUT_LENGTH], *msg;
    CharData *ch;

    msg = any_one_arg(argument, buf);

    if (!*buf) {
        wld_log(room, t, "wsend called with no args");
        return;
    }

    skip_spaces(&msg);

    if (!*msg) {
        wld_log(room, t, "wsend called without a message");
        return;
    }

    if ((ch = find_char_around_room(room, find_dg_by_name(buf)))) {
        if (subcmd == SCMD_WSEND)
            sub_write(msg, ch, true, TO_CHAR);
        else if (subcmd == SCMD_WECHOAROUND)
            sub_write(msg, ch, true, TO_ROOM);
    }

    else
        wld_log(room, t, "no target found for wsend");
}

WCMD(do_wzoneecho) {
    int zone_rnum, zone_vnum;
    char zone_name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

    msg = any_one_arg(argument, zone_name);
    skip_spaces(&msg);

    if (!*zone_name || !*msg)
        wld_log(room, t, "wzoneecho called with too few args");
    else if (!isdigit(*zone_name)) {
        sprintf(buf, "wzoneecho called with invalid zone number \"%s\"", zone_name);
        wld_log(room, t, buf);
    } else {
        zone_vnum = atoi(zone_name);
        if ((zone_rnum = real_zone(zone_vnum)) < 0) {
            sprintf(buf, "wzoneecho called for nonexistent zone %s", zone_name);
            wld_log(room, t, buf);
        } else {
            zone_printf(zone_vnum, NOWHERE, STANCE_RESTING, msg);
        }
    }
}

WCMD(do_wdoor) {
    char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
    char field[MAX_INPUT_LENGTH], *value, *desc;
    RoomData *rm;
    Exit *exit;
    int dir, fd, to_room;

    const char *door_field[] = {"purge", "description", "flags", "key", "name", "room", "\n"};

    argument = two_arguments(argument, target, direction);
    value = one_argument(argument, field);
    skip_spaces(&value);

    if (!*target || !*direction || !*field) {
        wld_log(room, t, "wdoor called with too few args");
        return;
    }

    if ((rm = get_room(target)) == nullptr) {
        wld_log(room, t, "wdoor: invalid target");
        return;
    }

    if ((dir = parse_direction(direction)) == -1) {
        wld_log(room, t, "wdoor: invalid direction");
        return;
    }

    if ((fd = searchblock(field, door_field, false)) == -1) {
        wld_log(room, t, "wdoor: invalid field");
        return;
    }

    exit = rm->exits[dir];

    /* purge exit */
    if (fd == 0) {
        if (exit) {
            if (exit->general_description)
                free(exit->general_description);
            if (exit->keyword)
                free(exit->keyword);
            free(exit);
            rm->exits[dir] = nullptr;
        }
    }

    else {
        if (!exit) {
            exit = create_exit(NOWHERE);
            rm->exits[dir] = exit;
        }

        switch (fd) {
        case 1: /* description */
            if (exit->general_description)
                free(exit->general_description);
            CREATE(desc, char, strlen(value) + 1);
            strcpy(desc, value);
            replace_str(&desc, "\\n", "\n", 1, MAX_INPUT_LENGTH);
            CREATE(exit->general_description, char, strlen(desc) + 3);
            strcpy(exit->general_description, desc);
            strcat(exit->general_description, "\n");
            free(desc);
            break;
        case 2: /* flags       */
            exit->exit_info = (int)asciiflag_conv(value);
            break;
        case 3: /* key         */
            exit->key = atoi(value);
            break;
        case 4: /* name        */
            if (exit->keyword)
                free(exit->keyword);
            CREATE(exit->keyword, char, strlen(value) + 1);
            strcpy(exit->keyword, value);
            break;
        case 5: /* room        */
            if ((to_room = real_room(atoi(value))) != NOWHERE)
                exit->to_room = to_room;
            else
                wld_log(room, t, "wdoor: invalid door target");
            break;
        }
    }
}

WCMD(do_wteleport) {
    CharData *ch, *next_ch;
    int target;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        wld_log(room, t, "wteleport called with too few args");
        return;
    }

    target = get_room_location(arg2);

    if (target == NOWHERE)
        wld_log(room, t, "wteleport target is an invalid room");

    else if (!strcasecmp(arg1, "all")) {
        if (world[target].vnum == room->vnum) {
            wld_log(room, t, "wteleport all target is itself");
            return;
        }

        for (ch = room->people; ch; ch = next_ch) {
            next_ch = ch->next_in_room;

            dismount_char(ch);
            char_from_room(ch);
            char_to_room(ch, target);
        }
    }

    else {
        if ((ch = find_char_around_room(room, find_dg_by_name(arg1)))) {
            dismount_char(ch);
            char_from_room(ch);
            char_to_room(ch, target);
        }

        else
            wld_log(room, t, "wteleport: no target found");
    }
}

WCMD(do_wforce) {
    CharData *ch, *next_ch;
    char arg1[MAX_INPUT_LENGTH], *line;

    line = one_argument(argument, arg1);

    if (!*arg1 || !*line) {
        wld_log(room, t, "wforce called with too few args");
        return;
    }

    if (!strcasecmp(arg1, "all")) {
        for (ch = room->people; ch; ch = next_ch) {
            next_ch = ch->next_in_room;

            if (GET_LEVEL(ch) < LVL_IMMORT) {
                command_interpreter(ch, line);
            }
        }
    }

    else {
        if ((ch = find_char_around_room(room, find_dg_by_name(arg1)))) {
            if (GET_LEVEL(ch) < LVL_IMMORT) {
                command_interpreter(ch, line);
            }
        }

        else
            wld_log(room, t, "wforce: no target found");
    }
}

/* increases the target's exp */
WCMD(do_wexp) {
    CharData *ch;
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

    two_arguments(argument, name, amount);

    if (!*name || !*amount) {
        wld_log(room, t, "wexp: too few arguments");
        return;
    }

    if ((ch = find_char_around_room(room, find_dg_by_name(name))))
        gain_exp(ch, atoi(amount), GAIN_IGNORE_LEVEL_BOUNDARY | GAIN_IGNORE_LOCATION);
    else {
        wld_log(room, t, "wexp: target not found");
        return;
    }
}

/* purge all objects an npcs in room, or specified obj or mob */
WCMD(do_wpurge) {
    char arg[MAX_INPUT_LENGTH];
    CharData *ch, *next_ch;
    ObjData *obj, *next_obj;

    one_argument(argument, arg);

    if (!*arg) {
        for (ch = room->people; ch; ch = next_ch) {
            next_ch = ch->next_in_room;
            if (IS_NPC(ch))
                fullpurge_char(ch);
        }

        for (obj = room->contents; obj; obj = next_obj) {
            next_obj = obj->next_content;
            extract_obj(obj);
        }

        return;
    }

    if (!(ch = find_char_around_room(room, find_dg_by_name(arg)))) {
        if ((obj = find_obj_around_room(room, find_by_name(arg)))) {
            extract_obj(obj);
        } else
            wld_log(room, t, "wpurge: bad argument");

        return;
    }

    if (!IS_NPC(ch)) {
        wld_log(room, t, "wpurge: purging a PC");
        return;
    }

    fullpurge_char(ch);
}

/* loads a mobile or object into the room */
WCMD(do_wload) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH];
    char *line;
    int number = 0, rnum;
    CharData *mob, *ch;
    ObjData *obj, *object2;

    line = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
        wld_log(room, t, "wload: bad syntax");
        return;
    }

    if (is_abbrev(arg1, "mob")) {
        if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
            wld_log(room, t, "wload: bad mob vnum");
            return;
        }
        if ((rnum = real_room(room->vnum)) == NOWHERE) {
            wld_log(room, t, "wload: room is NOWHERE");
            return;
        }
        char_to_room(mob, rnum);
        load_mtrigger(mob);
    }

    else if (is_abbrev(arg1, "obj")) {
        if ((obj = read_object(number, VIRTUAL)) == nullptr) {
            wld_log(room, t, "wload: bad obj vnum");
            return;
        }

        if ((rnum = real_room(room->vnum)) == NOWHERE) {
            wld_log(room, t, "wload: room is NOWHERE");
            return;
        }

        if (*line == '\0' || *line == '\r' || *line == '\n') {
            obj_to_room(obj, rnum);
        } else {
            two_arguments(line, arg3, arg4);
            if (is_abbrev(arg3, "mob")) {
                if ((mob = find_char_around_room(room, find_dg_by_name(arg4)))) {
                    obj_to_char(obj, mob);
                }
            } else if (is_abbrev(arg3, "obj")) {
                if ((object2 = find_obj_around_room(room, find_dg_by_name(arg4)))) {
                    obj_to_obj(obj, object2);
                } else {
                    wld_log(room, t, "wload: no target found");
                }
            } else if (is_abbrev(arg3, "plr")) {
                if ((ch = find_char_around_room(room, find_dg_by_name(arg4)))) {
                    obj_to_char(obj, ch);
                } else {
                    wld_log(room, t, "wload: no target found");
                }
            } else {
                wld_log(room, t, "wload: bad subtype");
            }
        }
    } else {
        wld_log(room, t, "wload: bad type");
    }
}

WCMD(do_wheal) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
    int dam = 0;
    CharData *ch;

    two_arguments(argument, name, amount);

    if (!*name || !*amount || !isdigit(*amount)) {
        wld_log(room, t, "wheal: bad syntax");
        return;
    }

    dam = atoi(amount);
    if (dam > 32767)
        dam = 32767; /* hitpoint is a short signed int */

    if ((ch = find_char_around_room(room, find_dg_by_name(name)))) {
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            char_printf(ch, "Being a god, you don't need healing.\n");
            return;
        }
        hurt_char(ch, nullptr, -dam, true);
    } else
        wld_log(room, t, "wheal: target not found");
}

WCMD(do_wdamage) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], damtype[MAX_INPUT_LENGTH];
    int dam = 0, dtype = DAM_UNDEFINED;
    CharData *ch;

    t->damdone = 0;

    argument = one_argument(argument, name);
    argument = one_argument(argument, amount);

    if (!*name || !*amount || !isdigit(*amount)) {
        wld_log(room, t, "wdamage: bad syntax");
        return;
    }

    dam = atoi(amount);
    if (dam > 32767)
        dam = 32767; /* hitpoint is a short signed int */

    ch = find_char_around_room(room, find_dg_by_name(name));
    if (!ch) {
        wld_log(room, t, "wdamage: target not found");
        return;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return;

    /* Check for and use optional damage-type parameter */
    argument = one_argument(argument, damtype);
    if (*damtype) {
        dtype = parse_damtype(0, damtype);
        if (dtype == DAM_UNDEFINED) {
            sprintf(buf,
                    "wdamage called with invalid third argument (\"%s\") - not a "
                    "damage type",
                    damtype);
            wld_log(room, t, buf);
            return;
        }
        dam = dam * susceptibility(ch, dtype) / 100;
        if (!dam)
            return;
    }

    t->damdone = dam;
    sethurtevent(0, ch, dam);
}

/*
 * Room version of do_quest
 * note, we don't return anything regardless of success of fail (whats a room
 * gonna do?) and we DONT allow the godly commands (rewind, restart) or stage
 * since its a bit pointless.. conversley, we CAN match any player in the mud,
 * even invis/hidden whatever
 */
WCMD(do_wld_quest) { perform_quest(t, argument, nullptr, nullptr, room); }

WCMD(do_wat) {
    char location[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    RoomData *r2;

    void wld_command_interpreter(RoomData * room, TrigData * t, char *argument);

    half_chop(argument, location, arg2);

    if (!*location || !*arg2 || (!isdigit(*location) && *location != UID_CHAR)) {
        wld_log(room, t, "wat: bad syntax");
        return;
    }
    r2 = get_room(location);
    if (r2 == nullptr) {
        wld_log(room, t, "wat: location not found");
        return;
    }

    wld_command_interpreter(r2, t, arg2);
}

WCMD(do_w_run_room_trig) {
    char arg1[MAX_INPUT_LENGTH];
    int runtrig, found = 0;
    ScriptData *sc;
    TrigData *tr;

    if (!*argument) {
        wld_log(room, t, "w_run_room_trig called with no argument");
        return;
    }

    one_argument(argument, arg1);

    if (!*arg1 || !is_number(arg1)) {
        wld_log(room, t, "w_run_room_trig: bad syntax");
        return;
    }

    runtrig = atoi(arg1);

    if ((sc = SCRIPT(room))) {
        for (tr = TRIGGERS(sc); tr; tr = tr->next) {
            if (GET_TRIG_VNUM(tr) == runtrig) {
                found = 1;
                break;
            }
        }
    }

    if (found) {
        script_driver(&room, tr, WLD_TRIGGER, TRIG_NEW);
    } else {
        sprintf(buf, "w_run_room_trig finds no such trigger %d", runtrig);
        wld_log(room, t, buf);
    }
}

WCMD(do_wld_log) { wld_log(room, t, argument); }

WCMD(do_wrent) {
    CharData *ch;

    extern void rem_memming(CharData * ch);

    argument = any_one_arg(argument, arg);

    if (!*arg) {
        wld_log(room, t, "wrent called with no args");
        return;
    }

    if (!(ch = find_char_around_room(room, find_dg_by_name(arg)))) {
        wld_log(room, t, "no target found for wsend");
        return;
    }

    if (IS_NPC(ch)) {
        wld_log(room, t, "wrent target is not player");
        return;
    }

    if (!ch->desc)
        wld_log(room, t, "wrent called on player without descriptor");

    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        act("$N ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
        char_printf(ch, "&8You stop meditating.\n&0");
        REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
    }

    rem_memming(ch);
    sprintf(buf, "%s rented by trigger %d in %s (%d).", GET_NAME(ch), GET_TRIG_VNUM(t), world[ch->in_room].name,
            world[ch->in_room].vnum);
    log(LogSeverity::Stat, LVL_IMMORT, buf);
    remove_player_from_game(ch, QUIT_WRENT);
}

const WorldCommandInfo wld_cmd_info[] = {
    {"RESERVED", 0, 0}, /* this must be first -- for specprocs */

    {"wasound", do_wasound, 0},
    {"wdoor", do_wdoor, 0},
    {"wecho", do_wecho, 0},
    {"wechoaround", do_wsend, SCMD_WECHOAROUND},
    {"wexp", do_wexp, 0},
    {"wforce", do_wforce, 0},
    {"wload", do_wload, 0},
    {"wpurge", do_wpurge, 0},
    {"wsend", do_wsend, SCMD_WSEND},
    {"wteleport", do_wteleport, 0},
    {"wzoneecho", do_wzoneecho, 0},
    {"wdamage", do_wdamage, 0},
    {"wheal", do_wheal, 0},
    {"wat", do_wat, 0},
    {"wrent", do_wrent, 0},
    {"quest", do_wld_quest, 0},
    {"log", do_wld_log, 0},
    {"w_run_room_trig", do_w_run_room_trig, 0},
    {"\n", 0, 0} /* this must be last */
};

/*
 *  This is the command interpreter used by rooms, called by script_driver.
 */
void wld_command_interpreter(RoomData *room, TrigData *t, char *argument) {
    int cmd, length;
    char *line, arg[MAX_INPUT_LENGTH];

    skip_spaces(&argument);

    /* just drop to next line for hitting CR */
    if (!*argument)
        return;

    line = any_one_arg(argument, arg);

    /* find the command */
    for (length = strlen(arg), cmd = 0; *wld_cmd_info[cmd].command != '\n'; cmd++)
        if (!strncasecmp(wld_cmd_info[cmd].command, arg, length))
            break;

    if (*wld_cmd_info[cmd].command == '\n') {
        sprintf(buf2, "Unknown world cmd: '%s'", argument);
        wld_log(room, t, buf2);
    } else
        ((*wld_cmd_info[cmd].command_pointer)(room, t, line, cmd, wld_cmd_info[cmd].subcmd));
}

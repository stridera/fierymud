/***************************************************************************
 *  File: objcmd.c                                       Part of FieryMUD  *
 *  Usage: contains the command_interpreter for objects, object commands.  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "arguments.hpp"
#include "casting.hpp"
#include "chars.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "movement.hpp"
#include "quest.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp" std::string_view
std::string_view std::string_viewstd::string_view std::string_view std::string_view

    void
    sub_write(std::string_view arg, CharData *ch, byte find_invis, int targets);
int get_room_location(std::string_view room);
FindContext find_dg_by_name(std::string_view name);

#define OCMD(name) void(name)(ObjData * obj, TrigData * t, Arguments argument, int cmd, int subcmd)

struct obj_command_info {
    std::string_view command;
    void (*command_pointer)(ObjData *obj, TrigData *t, Arguments argument, int cmd, int subcmd);
    int subcmd;
};

/* do_osend */
#define SCMD_OSEND 0
#define SCMD_OECHOAROUND 1

/* attaches object name and vnum to msg and sends it to script_log */
void obj_log(ObjData *obj, TrigData *t, const std::string_view msg) {
    char buf[MAX_INPUT_LENGTH + 100];

    void script_log(TrigData * t, std::string_view msg);

    sprintf(buf, "(TRG)(object %d): %s", GET_OBJ_VNUM(obj), msg);
    script_log(t, buf);
}

/* returns the real room number that the object or object's carrier is in */
int obj_room(ObjData *obj) {
    if (obj->in_room != NOWHERE)
        return obj->in_room;
    else if (obj->carried_by)
        return IN_ROOM(obj->carried_by);
    else if (obj->worn_by)
        return IN_ROOM(obj->worn_by);
    else if (obj->in_obj)
        return obj_room(obj->in_obj);
    else
        return NOWHERE;
}

/* returns the real room number, or NOWHERE if not found or invalid */
int find_obj_target_room(ObjData *obj, std::string_view roomstr) {
    int location;
    CharData *target_mob;
    ObjData *target_obj;

    if (roomstr.empty())
        return NOWHERE;

    /*
     * See if the string refers to a room UID or vnum first.  If no
     * matching room is found, try a character, and then an object.
     */
    if ((location = get_room_location(roomstr)) != NOWHERE)
        ;
    else if ((target_mob = find_char_around_obj(obj, find_dg_by_name(roomstr))) &&
             (location = IN_ROOM(target_mob)) != NOWHERE)
        ;
    else if ((target_obj = find_obj_around_obj(obj, find_by_name(roomstr))) &&
             (location = obj_room(target_obj)) != NOWHERE)
        ;
    else
        return NOWHERE;

    /* A room has been found.  Check for permission */
    if (ROOM_FLAGGED(location, ROOM_GODROOM) || ROOM_FLAGGED(location, ROOM_HOUSE))
        return NOWHERE;

    return location;
}

/* Object commands */

OCMD(do_oecho) {
    int room;

    if (argument.empty())
        obj_log(obj, t, "oecho called with no args");

    else if ((room = obj_room(obj)) != NOWHERE) {
        if (world[room].people)
            sub_write(argument.get(), world[room].people, true, TO_ROOM | TO_CHAR);
    } else
        obj_log(obj, t, "oecho called by object in NOWHERE");
}

OCMD(do_oforce) {
    CharData *ch, *next_ch;
    int room;

    auto target = argument.shift();
    auto command = argument.get();
    if (target.empty() || command.empty()) {
        obj_log(obj, t, "oforce called with too few args");
        return;
    }

    if (matches(target, "all")) {
        if ((room = obj_room(obj)) == NOWHERE)
            obj_log(obj, t, "oforce called by object in NOWHERE");
        else {
            for (ch = world[room].people; ch; ch = next_ch) {
                next_ch = ch->next_in_room;

                if (GET_LEVEL(ch) < LVL_IMMORT) {
                    command_interpreter(ch, command);
                }
            }
        }
    } else {
        if ((ch = find_char_around_obj(obj, find_dg_by_name(target)))) {
            if (GET_LEVEL(ch) < LVL_IMMORT) {
                command_interpreter(ch, command);
            }
        }

        else
            obj_log(obj, t, "oforce: no target found");
    }
}

OCMD(do_osend) {
    CharData *ch;

    auto target = argument.shift();
    if (target.empty()) {
        obj_log(obj, t, "osend called with no args");
        return;
    }

    auto msg = argument.get();
    if (msg.empty()) {
        obj_log(obj, t, "osend called without a message");
        return;
    }

    if ((ch = find_char_around_obj(obj, find_dg_by_name(target)))) {
        if (subcmd == SCMD_OSEND)
            sub_write(msg, ch, true, TO_CHAR);
        else if (subcmd == SCMD_OECHOAROUND)
            sub_write(msg, ch, true, TO_ROOM);
    }

    else
        obj_log(obj, t, "no target found for osend");
}

/* increases the target's exp */
OCMD(do_oexp) {
    CharData *ch;

    auto name = argument.shift();
    auto amount = argument.try_shift_number();

    if (name.empty() || amount.empty()) {
        obj_log(obj, t, "oexp: too few arguments");
        return;
    }

    if ((ch = find_char_around_obj(obj, find_dg_by_name(name))))
        gain_exp(ch, *amount, GAIN_IGNORE_LEVEL_BOUNDARY | GAIN_IGNORE_LOCATION);
    else {
        obj_log(obj, t, "oexp: target not found");
        return;
    }
}

/* purge all objects and npcs in room, or specified object or mob */
OCMD(do_opurge) {
    CharData *ch, *next_ch;
    ObjData *o, *next_obj;
    int rm;

    auto arg = argument.shift();

    if (arg.empty()) {
        if ((rm = obj_room(obj))) {
            for (ch = world[rm].people; ch; ch = next_ch) {
                next_ch = ch->next_in_room;
                if (IS_NPC(ch))
                    fullpurge_char(ch);
            }
            for (o = world[rm].contents; o; o = next_obj) {
                next_obj = o->next_content;
                if (o != obj)
                    extract_obj(o);
            }
        }
        return;
    }

    if (!(ch = find_char_around_obj(obj, find_dg_by_name(arg)))) {
        if ((o = find_obj_for_keyword(obj, arg)))
            extract_obj(obj);
        else if ((o = find_obj_around_obj(obj, find_by_name(arg))))
            extract_obj(o);
        else
            obj_log(obj, t, "opurge: bad argument");
        return;
    }

    if (!IS_NPC(ch)) {
        obj_log(obj, t, "opurge: attempting to purge PC");
        return;
    }

    fullpurge_char(ch);
}

OCMD(do_oteleport) {
    CharData *ch, *next_ch;
    int target, rm;

    auto arg1 = argument.shift();
    auto arg2 = argument.shift();

    if (arg1.empty() || arg2.empty()) {
        obj_log(obj, t, "oteleport called with too few args");
        return;
    }

    target = find_obj_target_room(obj, arg2);

    if (target == NOWHERE)
        obj_log(obj, t, "oteleport target is an invalid room");

    else if (matches(arg1, "all")) {
        rm = obj_room(obj);
        if (target == rm)
            obj_log(obj, t, "oteleport target is itself");

        for (ch = world[rm].people; ch; ch = next_ch) {
            next_ch = ch->next_in_room;
            dismount_char(ch);
            char_from_room(ch);
            char_to_room(ch, target);
        }
    }

    else {
        if ((ch = find_char_around_obj(obj, find_dg_by_name(arg1)))) {
            dismount_char(ch);
            char_from_room(ch);
            char_to_room(ch, target);
        } else
            obj_log(obj, t, "oteleport: no target found");
    }
}

OCMD(do_dgoload) {
    std::string_view line;
    int number = 0, room_num;
    CharData *mob, *ch;
    ObjData *obj1, *obj2;

    auto arg1 = argument.shift();
    auto arg2 = argument.shift();
    auto arg3 = argument.shift();

    if (arg1.empty() || arg2.empty() || !is_integer(arg2) || ((number = svtoi(arg2)) < 0)) {
        obj_log(obj, t, "oload: bad syntax");
        return;
    }

    if ((room_num = obj_room(obj)) == NOWHERE) {
        obj_log(obj, t, "oload: object in NOWHERE trying to load");
        return;
    }

    if (matches_start(arg1, "mob")) {
        if ((mob = read_mobile(number, VIRTUAL)) == nullptr) {
            obj_log(obj, t, "oload: bad mob vnum");
            return;
        }
        char_to_room(mob, room_num);
        load_mtrigger(mob);
    } else if (matches_start(arg1, "obj")) {
        if ((obj1 = read_object(number, VIRTUAL)) == nullptr) {
            obj_log(obj, t, "oload: bad object vnum");
            return;
        }

        if (*line == '\0') {
            obj_to_room(obj1, room_num);
        } else {
            RoomData *room = &world[room_num];
            auto arg3 = argument.shift();
            auto arg4 = argument.shift();
            if (arg3.empty() || arg4.empty()) {
                obj_log(obj, t, "oload: bad syntax");
                return;
            }

            if (matches_start(arg3, "mob")) {
                if ((mob = find_char_around_room(room, find_dg_by_name(arg4)))) {
                    obj_to_char(obj1, mob);
                }
            } else if (matches_start(arg3, "obj")) {
                if ((obj2 = find_obj_around_room(room, find_dg_by_name(arg4)))) {
                    obj_to_obj(obj1, obj2);
                } else {
                    obj_log(obj, t, "oload: no target found");
                }
            } else if (matches_start(arg3, "plr")) {
                if ((ch = find_char_around_room(room, find_dg_by_name(arg4)))) {
                    obj_to_char(obj1, ch);
                } else {
                    obj_log(obj, t, "oload: no target found");
                }
            } else {
                obj_log(obj, t, "oload: bad subtype");
            }
        }
    } else {
        obj_log(obj, t, "oload: bad type");
    }
}

OCMD(do_oheal) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
    int dam = 0;
    CharData *ch;

    two_arguments(argument, name, amount);

    if (name.empty() || amount.empty() || !isdigit(*amount)) {
        obj_log(obj, t, "oheal: bad syntax");
        return;
    }

    dam = atoi(amount);

    if ((ch = find_char_around_obj(obj, find_dg_by_name(name)))) {
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            char_printf(ch, "Being a god, you don't need healing.\n");
            return;
        }
        hurt_char(ch, nullptr, -dam, true);
    } else
        obj_log(obj, t, "oheal: target not found");
}

OCMD(do_odamage) {
    char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], damtype[MAX_INPUT_LENGTH];
    int dam = 0, dtype = DAM_UNDEFINED;
    CharData *ch;

    t->damdone = 0;
    argument = two_arguments(argument, name, amount);

    if (name.empty() || amount.empty() || !isdigit(*amount)) {
        obj_log(obj, t, "odamage: bad syntax");
        return;
    }

    dam = atoi(amount);

    ch = find_char_around_obj(obj, find_dg_by_name(name));
    if (!ch) {
        obj_log(obj, t, "odamage: target not found");
        return;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return;

    /* Check for and use optional damage-type parameter */
    damtype = argument.shift();
    if (*damtype) {
        dtype = parse_damtype(0, damtype);
        if (dtype == DAM_UNDEFINED) {
            sprintf(buf,
                    "odamage called with invalid third argument (\"%s\") - not a "
                    "damage type",
                    damtype);
            obj_log(obj, t, buf);
            return;
        }
        dam = dam * susceptibility(ch, dtype) / 100;
        if (!dam)
            return;
    }

    t->damdone = dam;
    if (obj->worn_by && (obj->worn_by != ch))
        sethurtevent(obj->worn_by, ch, dam);
    else if (obj->carried_by && (obj->carried_by != ch))
        sethurtevent(obj->worn_by, ch, dam);
    else
        sethurtevent(0, ch, dam);
}

OCMD(do_ocast) {
    char name[MAX_INPUT_LENGTH], spell_level_str[MAX_INPUT_LENGTH], spell_name[MAX_INPUT_LENGTH];
    CharData *target;
    int spellnum, level = 0;

    argument = delimited_arg(argument, spell_name, '\'');
    two_arguments(argument, name, spell_level_str);

    if (name.empty() || spell_name.empty()) {
        obj_log(obj, t, "ocast: bad syntax");
        return;
    }

    target = find_char_around_obj(obj, find_dg_by_name(name));
    if (!target) {
        obj_log(obj, t, "ocast: target not found");
        return;
    }

    level = atoi(spell_level_str);
    if (!level)
        level = 30;
    else if (level > 100) {
        obj_log(obj, t, "ocast: attempt to cast a spell with a skill level above 100.");
        return;
    } else if (level < 0) {
        obj_log(obj, t, "ocast: attempt to cast a spell with a skill level below 0.");
        return;
    }

    spellnum = find_spell_num(spell_name);
    if (!IS_SPELL(spellnum)) {
        sprintf(buf, "ocast: attempt to cast unknown spell: %s", spell_name);
        obj_log(obj, t, buf);
        return;
    }

    if (obj->worn_by)
        call_magic(obj->worn_by, target, nullptr, spellnum, level, SAVING_ROD);
    else if (obj->carried_by)
        call_magic(obj->carried_by, target, nullptr, spellnum, level, SAVING_ROD);
    else {
        obj_log(obj, t, "ocast: target must be carried or worn in order to cast spells");
        return;
    }
}

OCMD(do_ochant) {
    char name[MAX_INPUT_LENGTH], chant_level_str[MAX_INPUT_LENGTH], chant_name[MAX_INPUT_LENGTH];
    CharData *target;
    int chantnum, level = 0;

    argument = delimited_arg(argument, chant_name, '\'');
    two_arguments(argument, name, chant_level_str);

    if (name.empty() || chant_name.empty()) {
        obj_log(obj, t, "ocast: bad syntax");
        return;
    }

    target = find_char_around_obj(obj, find_dg_by_name(name));
    if (!target) {
        obj_log(obj, t, "ocast: target not found");
        return;
    }

    level = atoi(chant_level_str);
    if (!level)
        level = 30;
    else if (level > 1000) {
        obj_log(obj, t, "ocast: attempt to cast a chant at a level above 100.");
        return;
    } else if (level < 0) {
        obj_log(obj, t, "ocast: attempt to cast a chant at a level below 0.");
        return;
    }

    chantnum = find_spell_num(chant_name);
    if (!IS_CHANT(chantnum)) {
        sprintf(buf, "ocast: attempt to chant an unknown chant: %s", chant_name);
        obj_log(obj, t, buf);
        return;
    }

    if (obj->worn_by)
        call_magic(obj->worn_by, target, nullptr, chantnum, level, SAVING_ROD);
    else if (obj->carried_by)
        call_magic(obj->carried_by, target, nullptr, chantnum, level, SAVING_ROD);
    else {
        obj_log(obj, t, "ocast: target must be carried or worn in order to cast spells");
        return;
    }
}

/* allow a mob to set ANY skill or spell based on targets class
 * and level
 * syntax oskillset <plyrname> <name_skill_or_spell>
 */
OCMD(do_oskillset) {
    CharData *victim;
    int skspnum;

    auto arg = argument.shift();

    if (arg.empty()) {
        obj_log(obj, t, "oskillset called with no arguments");
        return;
    }

    if (!(victim = find_char_around_obj(obj, find_dg_by_name(arg)))) {
        sprintf(buf, "oskillset: victim (%s) not found", arg);
        obj_log(obj, t, buf);
        return;
    }
    /*
     * we have a victim, do we have a valid skill?
     */
    skip_spaces(argument);
    if ((skspnum = find_talent_num(argument, TALENT)) < 0) {
        /* no such spell/skill */
        sprintf(buf, "oskillset called with unknown skill/spell '%s'", argument);
        obj_log(obj, t, buf);
        return;
    }

    /*
     * because we're nice really, we will give the player the max proficiency for
     * their level at this skill..don't thank me just throw money..
     */
    SET_SKILL(victim, skspnum, return_max_skill(victim, skspnum));
}

/*
 * Object version of do_quest
 * note, we don't return anything regardless of success of fail (whats an object
 * gonna do?) and we DONT allow the godly commands (rewind, restart) or stage
 * since its a bit pointless.. conversley, we CAN match any player in the mud,
 * even invis/hidden whatever
 */
OCMD(do_obj_quest) { perform_quest(t, argument, nullptr, obj, nullptr); }

OCMD(do_obj_log) { obj_log(obj, t, argument); }

const struct obj_command_info obj_cmd_info[] = {
    {"RESERVED", 0, 0}, /* this must be first -- for specprocs */

    {"oecho", do_oecho, 0},
    {"oechoaround", do_osend, SCMD_OECHOAROUND},
    {"oexp", do_oexp, 0},
    {"oforce", do_oforce, 0},
    {"oload", do_dgoload, 0},
    {"opurge", do_opurge, 0},
    {"osend", do_osend, SCMD_OSEND},
    {"oteleport", do_oteleport, 0},
    {"odamage", do_odamage, 0},
    {"oheal", do_oheal, 0},
    {"quest", do_obj_quest, 0},
    {"ocast", do_ocast, 0},
    {"oskillset", do_oskillset, 0},
    {"log", do_obj_log, 0},
    {"\n", 0, 0} /* this must be last */
};

/*
 *  This is the command interpreter used by objects, called by script_driver.
 */
void obj_command_interpreter(ObjData *obj, TrigData *t, Arguments argument) {
    int cmd, length;
    std::string_view line, arg[MAX_INPUT_LENGTH];

    skip_spaces(argument);

    /* just drop to next line for hitting CR */
    if (argument.empty())
        return;

    line = any_one_arg(argument, arg);

    /* find the command */
    for (length = strlen(arg), cmd = 0; *obj_cmd_info[cmd].command != '\n'; cmd++)
        if (!strncasecmp(obj_cmd_info[cmd].command, arg, length))
            break;

    if (*obj_cmd_info[cmd].command == '\n') {
        sprintf(buf2, "Unknown object cmd: '%s'", argument);
        obj_log(obj, t, buf2);
    } else
        ((*obj_cmd_info[cmd].command_pointer)(obj, t, line, cmd, obj_cmd_info[cmd].subcmd));
}

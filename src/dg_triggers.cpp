/***************************************************************************
 *   File: dg_triggers.c                                  Part of FieryMUD *
 *  Usage: contains all the trigger functions for scripts.                 *
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

#define __DG_TRIGGERS_C__

#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "money.hpp"
#include "olc.hpp"
#include "skills.hpp"
// #include "strings.hpp"
#include "logging.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/*
 *  General functions used by several triggers
 */

bool char_susceptible_to_triggers(const CharData *ch) {
    if (!ch || GET_INVIS_LEV(ch))
        return false;
    else
        return true;
}

int is_substring(const char *sub, const char *string) {
    const char *s;

    if ((s = strcasestr(string, sub))) {
        int len = strlen(string);
        int sublen = strlen(sub);

        /* check front */
        if ((s == string || isspace(*(s - 1)) || ispunct(*(s - 1))) &&
            /* check end */
            ((s + sublen == string + len) || isspace(s[sublen]) || ispunct(s[sublen])))
            return 1;
    }

    return 0;
}

/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
int word_check(const char *str, const char *wordlist) {
    char words[MAX_INPUT_LENGTH], phrase[MAX_INPUT_LENGTH], *s;

    if (*wordlist == '*')
        return 1;

    strcpy(words, wordlist);

    for (s = delimited_arg(words, phrase, '"'); *phrase; s = delimited_arg(s, phrase, '"'))
        if (is_substring(phrase, str))
            return 1;

    return 0;
}

/*
 *  mob triggers
 */

/*
 * MOBILE RANDOM
 *
 * Checked every 13 seconds.  Treats each trigger's numeric argument as
 * a percentage chance to run each time it's checked.  Only one random
 * trigger will be executed.
 */
void random_mtrigger(CharData *ch) {
    TrigData *t;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_RANDOM) || !char_susceptible_to_triggers(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_RANDOM) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

/*
 * MOBILE BRIBE
 *
 * Checked whenever coins are given to a mobile.  The bribe trigger only
 * executes if the total value of the coins is higher than the "raw
 * value" given by the trigger's numeric arg.  It expects a four-integer
 * array for platinum, gold, silver, and copper.  Only one bribe trigger
 * will be executed.
 */
void bribe_mtrigger(CharData *ch, CharData *actor, int coins[]) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int raw_value;
    int type;

    raw_value = 1000 * coins[PLATINUM] + 100 * coins[GOLD] + 10 * coins[SILVER] + coins[COPPER];

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_BRIBE) || !char_susceptible_to_triggers(actor) ||
        !char_susceptible_to_triggers(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_BRIBE) && (raw_value >= GET_TRIG_NARG(t))) {
            sprintf(buf, "%d", raw_value);
            add_var(&GET_TRIG_VARS(t), "value", buf);
            for (type = 0; type < NUM_COIN_TYPES; ++type) {
                sprintf(buf, "%d", coins[type]);
                add_var(&GET_TRIG_VARS(t), COIN_NAME(type), buf);
            }
            ADD_UID_VAR(buf, t, actor, "actor");
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

/*
 * MOBILE GREET
 *
 * Checked whenever the actor enters a room from the given direction.
 * Each mobile can have multiple greet triggers, and all mobiles with
 * great triggers in the room will be checked.
 * Returns true if the entry should be allowed and false otherwise.
 * Quits immediately if either the mobile is extracted or the actor
 * is killed.
 */
int greet_mtrigger(CharData *actor, int dir) {
    TrigData *t;
    CharData *ch, *next_in_room;
    char buf[MAX_INPUT_LENGTH];
    int ret_val = true;

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (ch = world[IN_ROOM(actor)].people; ch; ch = next_in_room) {
        next_in_room = ch->next_in_room;
        if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_GREET | MTRIG_GREET_ALL) || !AWAKE(ch) ||
            FIGHTING(ch) || (ch == actor))
            continue;

        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (((IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET) && CAN_SEE(ch, actor)) ||
                 IS_SET(GET_TRIG_TYPE(t), MTRIG_GREET_ALL)) &&
                !GET_TRIG_DEPTH(t) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
                add_var(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]]);
                ADD_UID_VAR(buf, t, actor, "actor");
                if (!script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW))
                    ret_val = false;
                if (!ch || DECEASED(actor))
                    return false;
            }
        }
    }
    return ret_val;
}

/*
 * MOBILE ENTRY
 *
 * Checked whenever ch attempts to enter a room with vnum destination.
 * Returns true if the entry should be allowed and false otherwise.
 */
int entry_mtrigger(CharData *ch, int destination) {
    TrigData *t;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_ENTRY) || !char_susceptible_to_triggers(ch))
        return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_ENTRY) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            sprintf(buf, "%d", destination);
            add_var(&GET_TRIG_VARS(t), "destination", buf);
            if (!script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW) || !ch)
                return false;
        }
    }
    return true;
}

/*
 * MOBILE COMMAND
 *
 * Checked whenever a command is typed.  Checks all mobiles in the room
 * for command triggers, and the first matching command trigger is used.
 * Command triggers with "*" for arguments match all commands.
 * Returns true if the normal command action should be blocked, and
 * false otherwise.
 */
int command_mtrigger(CharData *actor, char *cmd, char *argument) {
    CharData *ch, *ch_next;
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor))
        return 0;

    for (ch = world[IN_ROOM(actor)].people; ch; ch = ch_next) {
        ch_next = ch->next_in_room;

        if (MOB_PERFORMS_SCRIPTS(ch) && SCRIPT_CHECK(ch, MTRIG_COMMAND)) {
            for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
                if (!TRIGGER_CHECK(t, MTRIG_COMMAND))
                    continue;

                if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
                    log(LogSeverity::Stat, LVL_ATTENDANT, "SYSERR: Command Trigger #{:d} has no text argument!",
                        GET_TRIG_VNUM(t));
                    continue;
                }

                if (*GET_TRIG_ARG(t) == '*' || !strncasecmp(GET_TRIG_ARG(t), cmd, strlen(cmd))) {
                    ADD_UID_VAR(buf, t, actor, "actor");
                    skip_spaces(&argument);
                    add_var(&GET_TRIG_VARS(t), "arg", argument);
                    skip_spaces(&cmd);
                    add_var(&GET_TRIG_VARS(t), "cmd", cmd);

                    return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
                }
            }
        }
    }

    return 0;
}

void speech_mtrigger(CharData *actor, const char *str) {
    CharData *ch, *ch_next;
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor))
        return;

    for (ch = world[IN_ROOM(actor)].people; ch; ch = ch_next) {
        ch_next = ch->next_in_room;

        if (MOB_PERFORMS_SCRIPTS(ch) && (SCRIPT_CHECK(ch, MTRIG_SPEECH) || SCRIPT_CHECK(ch, MTRIG_SPEECHTO)) &&
            AWAKE(ch) && actor != ch)
            for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
                if (!TRIGGER_CHECK(t, MTRIG_SPEECH) && !TRIGGER_CHECK(t, MTRIG_SPEECHTO))
                    continue;

                if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
                    log(LogSeverity::Stat, LVL_GOD, "SYSERR: Speech Trigger #{:d} has no text argument!",
                        GET_TRIG_VNUM(t));
                    continue;
                }

                if (((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
                     (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))) {
                    ADD_UID_VAR(buf, t, actor, "actor");
                    add_var(&GET_TRIG_VARS(t), "speech", str);
                    script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
                    break;
                }
            }
    }
}

/* ch is the vict from ask/whisper/tell, ie the mob */
void speech_to_mtrigger(CharData *actor, CharData *ch, const char *str) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !char_susceptible_to_triggers(ch))
        return;

    if (MOB_PERFORMS_SCRIPTS(ch) && (SCRIPT_CHECK(ch, MTRIG_SPEECHTO) || SCRIPT_CHECK(ch, MTRIG_SPEECH)) && AWAKE(ch) &&
        actor != ch)
        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (!TRIGGER_CHECK(t, MTRIG_SPEECHTO) && !TRIGGER_CHECK(t, MTRIG_SPEECH))
                continue;

            if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
                log(LogSeverity::Stat, LVL_GOD, "SYSERR: Speech-to Trigger #{:d} has no text argument!",
                    GET_TRIG_VNUM(t));
                continue;
            }

            if ((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
                (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str))) {
                ADD_UID_VAR(buf, t, actor, "actor");
                add_var(&GET_TRIG_VARS(t), "speech", str);
                script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
                break;
            }
        }
}

void act_mtrigger(const CharData *ch, const char *str, const CharData *actor, const CharData *victim,
                  const ObjData *obj, const ObjData *target, char *arg, char *arg2) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    extern int circle_shutdown;

    /* Don't process triggers during shutdown */
    if (circle_shutdown)
        return;

    if (!char_susceptible_to_triggers(actor))
        return;

    if (MOB_PERFORMS_SCRIPTS(ch) && SCRIPT_CHECK(ch, MTRIG_ACT))
        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (!TRIGGER_CHECK(t, MTRIG_ACT))
                continue;

            if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
                log(LogSeverity::Stat, LVL_GOD, "SYSERR: Act Trigger #{:d} has no text argument!", GET_TRIG_VNUM(t));
                continue;
            }

            if (((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
                 (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))) {

                if (actor)
                    ADD_UID_VAR(buf, t, actor, "actor");
                if (victim)
                    ADD_UID_VAR(buf, t, victim, "victim");
                if (obj)
                    ADD_UID_VAR(buf, t, obj, "object");
                if (target)
                    ADD_UID_VAR(buf, t, target, "target");
                if (arg) {
                    skip_spaces(&arg);
                    add_var(&GET_TRIG_VARS(t), "arg", arg);
                }
                if (arg2) {
                    skip_spaces(&arg2);
                    add_var(&GET_TRIG_VARS(t), "arg2", arg2);
                }
                add_var(&GET_TRIG_VARS(t), "act", str);
                script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
                break;
            }
        }
}

void fight_mtrigger(CharData *ch) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_FIGHT) || !FIGHTING(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_FIGHT) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            if (FIGHTING(ch))
                ADD_UID_VAR(buf, t, FIGHTING(ch), "actor")
            else
                add_var(&GET_TRIG_VARS(t), "actor", "nobody");
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

void hitprcnt_mtrigger(CharData *ch) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_HITPRCNT) || !FIGHTING(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_HITPRCNT) && GET_MAX_HIT(ch) &&
            (((GET_HIT(ch) * 100) / GET_MAX_HIT(ch)) <= GET_TRIG_NARG(t))) {

            if (FIGHTING(ch))
                ADD_UID_VAR(buf, t, FIGHTING(ch), "actor")
            else
                add_var(&GET_TRIG_VARS(t), "actor", "nobody");
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int receive_mtrigger(CharData *ch, CharData *actor, ObjData *obj) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_RECEIVE) || !char_susceptible_to_triggers(actor) ||
        !char_susceptible_to_triggers(ch))
        return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (IS_SET(GET_TRIG_TYPE(t), MTRIG_RECEIVE)) {
            if (GET_TRIG_DEPTH(t)) {
                /* The receive trigger is currently executing */
                act("$N isn't ready to accept $p.", false, actor, obj, ch, TO_CHAR);
                return 0;
            } else if (random_number(1, 100) <= GET_TRIG_NARG(t)) {
                ADD_UID_VAR(buf, t, actor, "actor");
                ADD_UID_VAR(buf, t, obj, "object");
                return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            }
        }
    }

    return 1;
}

int death_mtrigger(CharData *ch, CharData *actor) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int ret_val = 1;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_DEATH) ||
        (actor && !char_susceptible_to_triggers(actor)) || !char_susceptible_to_triggers(ch))
        return ret_val;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_DEATH)) {

            if (actor)
                ADD_UID_VAR(buf, t, actor, "actor");
            ret_val = script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
        }
    }

    return ret_val;
}

void load_mtrigger(CharData *ch) {
    TrigData *t;

    if (!SCRIPT_CHECK(ch, MTRIG_LOAD) || !char_susceptible_to_triggers(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_LOAD) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int cast_mtrigger(CharData *actor, CharData *ch, int spellnum) {
    TrigData *t;

    if (ch == nullptr)
        return 1;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_CAST) || !char_susceptible_to_triggers(actor) ||
        !char_susceptible_to_triggers(ch))
        return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_CAST) && random_number(1, 100) <= GET_TRIG_NARG(t)) {
            ADD_UID_VAR(buf, t, actor, "actor");
            sprintf(buf, "%d", spellnum);
            add_var(&GET_TRIG_VARS(t), "spellnum", buf);
            add_var(&GET_TRIG_VARS(t), "spell", skill_name(spellnum));
            return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int leave_mtrigger(CharData *actor, int dir) {
    TrigData *t;
    CharData *ch;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (ch = world[IN_ROOM(actor)].people; ch; ch = ch->next_in_room) {
        if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_LEAVE) || !AWAKE(ch) || FIGHTING(ch) || ch == actor)
            continue;

        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (TRIGGER_CHECK(t, MTRIG_LEAVE) && CAN_SEE(ch, actor) && random_number(1, 100) <= GET_TRIG_NARG(t)) {
                if (dir >= 0 && dir < NUM_OF_DIRS)
                    add_var(&GET_TRIG_VARS(t), "direction", dirs[dir]);
                else
                    add_var(&GET_TRIG_VARS(t), "direction", "none");
                ADD_UID_VAR(buf, t, actor, "actor");
                return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            }
        }
    }
    return 1;
}

int door_mtrigger(CharData *actor, int subcmd, int dir) {
    TrigData *t;
    CharData *ch;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (ch = world[IN_ROOM(actor)].people; ch; ch = ch->next_in_room) {
        if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_DOOR) || !AWAKE(ch) || FIGHTING(ch) || ch == actor ||
            !CAN_SEE(ch, actor))
            continue;

        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (TRIGGER_CHECK(t, MTRIG_DOOR) && random_number(1, 100) <= GET_TRIG_NARG(t)) {
                add_var(&GET_TRIG_VARS(t), "cmd", cmd_door[subcmd]);
                add_var(&GET_TRIG_VARS(t), "direction", dirs[dir]);
                ADD_UID_VAR(buf, t, actor, "actor");
                return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            }
        }
    }

    return 1;
}

void time_mtrigger(CharData *ch) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    /* This trigger is called if the hour is the same as specified in Narg. */
    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_TIME) || !char_susceptible_to_triggers(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_TIME) && (time_info.hours == GET_TRIG_NARG(t))) {
            sprintf(buf, "%d", time_info.hours);
            add_var(&GET_TRIG_VARS(t), "time", buf);
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int look_mtrigger(CharData *ch, CharData *actor, const char *str) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int ret_val = 1;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(ch, MTRIG_LOOK) || !char_susceptible_to_triggers(ch))
        return ret_val;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_LOOK) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            if (actor)
                ADD_UID_VAR(buf, t, actor, "actor");
            if (str) {
                add_var(&GET_TRIG_VARS(t), "arg", str);
            }
            /* Don't allow a look to take place, if the mob is purged. */
            ret_val = (script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW) && ch);
        }
    }

    return ret_val;
}


/*
 *  object triggers
 */

void random_otrigger(ObjData *obj) {
    TrigData *t;

    if (!SCRIPT_CHECK(obj, OTRIG_RANDOM))
        return;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_RANDOM) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int timer_otrigger(ObjData *obj) {
    TrigData *t;

    if (!SCRIPT_CHECK(obj, OTRIG_TIMER))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
        if (TRIGGER_CHECK(t, OTRIG_TIMER) && random_number(1, 100) <= GET_TRIG_NARG(t))
            return script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);

    return 1;
}

int get_otrigger(ObjData *obj, CharData *actor) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(obj, OTRIG_GET) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_GET) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            /* Don't allow a get to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

/* checks for command trigger on specific object. assumes obj has cmd trig */
int cmd_otrig(ObjData *obj, CharData *actor, char *cmd, char *argument, int type) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND))
        for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
            if (!TRIGGER_CHECK(t, OTRIG_COMMAND))
                continue;

            if (IS_SET(GET_TRIG_NARG(t), type) && (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))) {
                log(LogSeverity::Stat, LVL_GOD, "SYSERR: O-Command Trigger #{:d} has no text argument!",
                    GET_TRIG_VNUM(t));
                continue;
            }

            if (IS_SET(GET_TRIG_NARG(t), type) &&
                (*GET_TRIG_ARG(t) == '*' || !strncasecmp(GET_TRIG_ARG(t), cmd, strlen(cmd)))) {

                ADD_UID_VAR(buf, t, actor, "actor");
                skip_spaces(&argument);
                add_var(&GET_TRIG_VARS(t), "arg", argument);
                skip_spaces(&cmd);
                add_var(&GET_TRIG_VARS(t), "cmd", cmd);

                return script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
            }
        }

    return 0;
}

int command_otrigger(CharData *actor, char *cmd, char *argument) {
    ObjData *obj;
    int i;

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (i = 0; i < NUM_WEARS; i++)
        if (cmd_otrig(GET_EQ(actor, i), actor, cmd, argument, OCMD_EQUIP))
            return 1;

    for (obj = actor->carrying; obj; obj = obj->next_content)
        if (cmd_otrig(obj, actor, cmd, argument, OCMD_INVEN))
            return 1;

    for (obj = world[IN_ROOM(actor)].contents; obj; obj = obj->next_content)
        if (cmd_otrig(obj, actor, cmd, argument, OCMD_ROOM))
            return 1;

    return 0;
}

void attack_otrigger(CharData *actor, CharData *victim, int dam) {
    char buf[MAX_INPUT_LENGTH], dam_str[MAX_INPUT_LENGTH];
    TrigData *t;
    ObjData *obj;
    int i;

    if (!char_susceptible_to_triggers(actor))
        return;

    sprintf(dam_str, "%d", dam);

    for (i = 0; i < NUM_WEARS; ++i) {
        obj = actor->equipment[i];
        if (obj && SCRIPT_CHECK(obj, OTRIG_ATTACK)) {
            for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
                if (TRIGGER_CHECK(t, OTRIG_ATTACK) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
                    add_var(&GET_TRIG_VARS(t), "damage", dam_str);
                    ADD_UID_VAR(buf, t, actor, "actor");
                    ADD_UID_VAR(buf, t, victim, "victim");
                    script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
                    break;
                }
            }
        }
        obj = victim->equipment[i];
        if (obj && SCRIPT_CHECK(obj, OTRIG_DEFEND)) {
            for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
                if (TRIGGER_CHECK(t, OTRIG_DEFEND) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
                    add_var(&GET_TRIG_VARS(t), "damage", dam_str);
                    ADD_UID_VAR(buf, t, actor, "actor");
                    ADD_UID_VAR(buf, t, victim, "victim");
                    script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
                    break;
                }
            }
        }
    }
}

int wear_otrigger(ObjData *obj, CharData *actor, int where) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(obj, OTRIG_WEAR) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_WEAR)) {
            sprintf(buf, "%d", where);
            add_var(&GET_TRIG_VARS(t), "position", buf);
            ADD_UID_VAR(buf, t, actor, "actor");
            /* Don't allow a wear to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

int death_otrigger(CharData *actor) {
    TrigData *t;
    ObjData *obj;
    char buf[MAX_INPUT_LENGTH];
    int i;

    for (i = 0; i < NUM_WEARS; ++i) {
        obj = actor->equipment[i];
        if (obj && SCRIPT_CHECK(obj, OTRIG_DEATH)) {
            for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
                if (TRIGGER_CHECK(t, OTRIG_DEATH)) {
                    ADD_UID_VAR(buf, t, actor, "actor");
                    return script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
                }
            }
        }
    }
    return 1;
}

int drop_otrigger(ObjData *obj, CharData *actor) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_DROP))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_DROP) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            /* Don't allow a drop to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

int remove_otrigger(ObjData *obj, CharData *actor) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_REMOVE))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_REMOVE) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            /* Don't allow a remove to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

int give_otrigger(ObjData *obj, CharData *actor, CharData *victim) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_GIVE))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_GIVE) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            ADD_UID_VAR(buf, t, victim, "victim");
            /* Don't allow a give to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj && obj->carried_by == actor);
        }
    }

    return 1;
}

void load_otrigger(ObjData *obj) {
    TrigData *t;

    if (!SCRIPT_CHECK(obj, OTRIG_LOAD))
        return;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_LOAD) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
            return;
        }
    }
}

int cast_otrigger(CharData *actor, ObjData *obj, int spellnum) {
    TrigData *t;
    char buf[16];

    if (obj == nullptr || !char_susceptible_to_triggers(actor))
        return 1;

    if (!SCRIPT_CHECK(obj, OTRIG_CAST))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_CAST) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            sprintf(buf, "%d", spellnum);
            add_var(&GET_TRIG_VARS(t), "spellnum", buf);
            add_var(&GET_TRIG_VARS(t), "spell", skill_name(spellnum));
            return script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int leave_otrigger(RoomData *room, CharData *actor, int dir) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int final = 1;
    ObjData *obj, *obj_next;

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (obj = room->contents; obj; obj = obj_next) {
        obj_next = obj->next_content;
        if (!SCRIPT_CHECK(obj, OTRIG_LEAVE))
            continue;

        for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
            if (TRIGGER_CHECK(t, OTRIG_LEAVE) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
                if (dir >= 0 && dir < NUM_OF_DIRS)
                    add_var(&GET_TRIG_VARS(t), "direction", dirs[dir]);
                else
                    add_var(&GET_TRIG_VARS(t), "direction", "none");
                ADD_UID_VAR(buf, t, actor, "actor");
                if (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) == 0)
                    ;
                final = 0;
            }
        }
    }

    return final;
}

int consume_otrigger(ObjData *obj, CharData *actor, int cmd) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_CONSUME))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_CONSUME)) {
            ADD_UID_VAR(buf, t, actor, "actor");
            switch (cmd) {
                /*
                 * This is kind of a hack, since eat, drink, and quaff
                 * are subcommands of different commands.  However,
                 * they are fortunately distinct numbers.
                 */
            case SCMD_EAT: /* 0 */
                add_var(&GET_TRIG_VARS(t), "command", "eat");
                break;
            case SCMD_DRINK: /* 2 */
                add_var(&GET_TRIG_VARS(t), "command", "drink");
                break;
            case SCMD_QUAFF: /* 1 */
                add_var(&GET_TRIG_VARS(t), "command", "quaff");
                break;
            }
            /* Don't allow a consume to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

void time_otrigger(ObjData *obj) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(obj, OTRIG_TIME))
        return;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_TIME) && (time_info.hours == GET_TRIG_NARG(t))) {
            sprintf(buf, "%d", time_info.hours);
            add_var(&GET_TRIG_VARS(t), "time", buf);
            script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int look_otrigger(ObjData *obj, CharData *actor, char *name) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int ret_val = 1;
    FindContext context;
    const char *str;
    int num;
    char *pos;

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_LOOK))
        return ret_val;


    if ((pos = strchr(name, '.'))) {
        num = atoi(name);
        name = ++pos;
        str = name;
    } else
        str = name;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (GET_TRIG_ARG(t) && word_check(str, GET_TRIG_ARG(t)) || 
          (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) && isname(str, obj->name)) {
            if (TRIGGER_CHECK(t, OTRIG_LOOK) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
                if (actor)
                    ADD_UID_VAR(buf, t, actor, "actor");
                if (str)
                    add_var(&GET_TRIG_VARS(t), "arg", str);

                /* Don't allow a look to take place, if the object is purged. */
                ret_val = (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
            }          
        } else
            continue;
    }

    return ret_val;
}

/*
 *  world triggers
 */

void random_wtrigger(RoomData *room) {
    TrigData *t;

    if (!SCRIPT_CHECK(room, WTRIG_RANDOM))
        return;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_RANDOM) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int preentry_wtrigger(RoomData *room, CharData *actor, int dir) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int rev_dir[] = {SOUTH, WEST, NORTH, EAST, DOWN, UP};

    if (!SCRIPT_CHECK(room, WTRIG_PREENTRY) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_PREENTRY) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int postentry_wtrigger(CharData *actor, int dir) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int rev_dir[] = {SOUTH, WEST, NORTH, EAST, DOWN, UP};
    struct RoomData *room = &world[IN_ROOM(actor)];

    if (!SCRIPT_CHECK(room, WTRIG_POSTENTRY) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_POSTENTRY) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

void reset_wtrigger(RoomData *room) {
    TrigData *t;

    if (!SCRIPT_CHECK(room, WTRIG_RESET))
        return;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_RESET) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int command_wtrigger(CharData *actor, char *cmd, char *argument) {
    RoomData *room;
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_COMMAND) || !char_susceptible_to_triggers(actor))
        return 0;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (!TRIGGER_CHECK(t, WTRIG_COMMAND))
            continue;

        if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
            log(LogSeverity::Stat, LVL_GOD, "SYSERR: W-Command Trigger #{:d} has no text argument!", GET_TRIG_VNUM(t));
            continue;
        }

        if (*GET_TRIG_ARG(t) == '*' || !strncasecmp(GET_TRIG_ARG(t), cmd, strlen(cmd))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            skip_spaces(&argument);
            add_var(&GET_TRIG_VARS(t), "arg", argument);
            skip_spaces(&cmd);
            add_var(&GET_TRIG_VARS(t), "cmd", cmd);
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 0;
}

void speech_wtrigger(CharData *actor, const char *str) {
    RoomData *room;
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_SPEECH) || !char_susceptible_to_triggers(actor))
        return;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (!TRIGGER_CHECK(t, WTRIG_SPEECH))
            continue;

        if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
            log(LogSeverity::Stat, LVL_GOD, "SYSERR: W-Speech Trigger #{:d} has no text argument!", GET_TRIG_VNUM(t));
            continue;
        }

        if (((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
             (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            add_var(&GET_TRIG_VARS(t), "speech", str);
            script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int drop_wtrigger(ObjData *obj, CharData *actor) {
    RoomData *room;
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];
    int ret_val;

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_DROP) || !char_susceptible_to_triggers(actor))
        return 1;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
        if (TRIGGER_CHECK(t, WTRIG_DROP) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {

            ADD_UID_VAR(buf, t, actor, "actor");
            ADD_UID_VAR(buf, t, obj, "object");
            ret_val = script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
            if (obj->carried_by != actor)
                return 0;
            else
                return ret_val;
        }

    return 1;
}

int cast_wtrigger(CharData *actor, CharData *vict, ObjData *obj, int spellnum) {
    RoomData *room;
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_CAST) || !char_susceptible_to_triggers(actor))
        return 1;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_CAST) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {

            ADD_UID_VAR(buf, t, actor, "actor");
            if (vict)
                ADD_UID_VAR(buf, t, vict, "victim");
            if (obj)
                ADD_UID_VAR(buf, t, obj, "object");
            sprintf(buf, "%d", spellnum);
            add_var(&GET_TRIG_VARS(t), "spellnum", buf);
            add_var(&GET_TRIG_VARS(t), "spell", skill_name(spellnum));
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int leave_wtrigger(RoomData *room, CharData *actor, int dir) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(room, WTRIG_LEAVE) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_LEAVE) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "direction", dirs[dir]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int door_wtrigger(CharData *actor, int subcmd, int dir) {
    RoomData *room;
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_DOOR) || !char_susceptible_to_triggers(actor))
        return 1;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_DOOR) && (random_number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "cmd", cmd_door[subcmd]);
            add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[dir]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

void time_wtrigger(RoomData *room) {
    TrigData *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(room, WTRIG_TIME))
        return;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_TIME) && (time_info.hours == GET_TRIG_NARG(t))) {
            sprintf(buf, "%d", time_info.hours);
            add_var(&GET_TRIG_VARS(t), "time", buf);
            script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
            break;
        }
    }
}
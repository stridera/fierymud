/***************************************************************************
 * $Id: dg_triggers.c,v 1.34 2010/07/27 22:14:35 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: dg_triggers.c                                  Part of FieryMUD *
 *  Usage: contains all the trigger functions for scripts.                 *
 *  $Author: mud $                                                         *
 *  $Date: 2010/07/27 22:14:35 $                                           *
 *  $Revision: 1.34 $                                                       *
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

#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "dg_scripts.h"
#include "directions.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "money.h"
#include "olc.h"
#include "skills.h"
#include "strings.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* External variables */
extern const char *cmd_door[];

/* external functions from scripts.c */
void add_var(struct trig_var_data **var_list, const char *name, const char *value);
int script_driver(void *go_address, trig_data *trig, int type, int mode);

/* mob trigger types */
const char *trig_types[] = {"Global",    "Random", "Command", "Speech", "Act",      "Death", "Greet",
                            "Greet-All", "Entry",  "Receive", "Fight",  "HitPrcnt", "Bribe", "SpeechTo*",
                            "Load",      "Cast",   "Leave",   "Door",   "UNUSED",   "Time",  "\n"};

/* obj trigger types */
const char *otrig_types[] = {"Global",  "Random", "Command", "Attack", "Defense", "Timer",  "Get",
                             "Drop",    "Give",   "Wear",    "DEATH",  "Remove",  "UNUSED", "UNUSED",
                             "Load",    "Cast",   "Leave",   "UNUSED", "Consume", "Time",   "\n"};

/* wld trigger types */
const char *wtrig_types[] = {"Global", "Random",    "Command", "Speech", "UNUSED", "Reset",  "Preentry",
                             "Drop",   "Postentry", "UNUSED",  "UNUSED", "UNUSED", "UNUSED", "UNUSED",
                             "UNUSED", "Cast",      "Leave",   "Door",   "UNUSED", "Time",   "\n"};

/*
 *  General functions used by several triggers
 */

bool char_susceptible_to_triggers(struct char_data *ch) {
    if (!ch || GET_INVIS_LEV(ch))
        return FALSE;
    else
        return TRUE;
}

int is_substring(char *sub, char *string) {
    const char *s;

    if ((s = str_str(string, sub))) {
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
int word_check(char *str, char *wordlist) {
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
void random_mtrigger(char_data *ch) {
    trig_data *t;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_RANDOM) || !char_susceptible_to_triggers(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_RANDOM) && (number(1, 100) <= GET_TRIG_NARG(t))) {
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
void bribe_mtrigger(char_data *ch, char_data *actor, int coins[]) {
    trig_data *t;
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
 * Returns TRUE if the entry should be allowed and FALSE otherwise.
 * Quits immediately if either the mobile is extracted or the actor
 * is killed.
 */
int greet_mtrigger(char_data *actor, int dir) {
    trig_data *t;
    char_data *ch, *next_in_room;
    char buf[MAX_INPUT_LENGTH];
    int ret_val = TRUE;

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
                !GET_TRIG_DEPTH(t) && (number(1, 100) <= GET_TRIG_NARG(t))) {
                add_var(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]]);
                ADD_UID_VAR(buf, t, actor, "actor");
                if (!script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW))
                    ret_val = FALSE;
                if (!ch || DECEASED(actor))
                    return FALSE;
            }
        }
    }
    return ret_val;
}

/*
 * MOBILE ENTRY
 *
 * Checked whenever ch attempts to enter a room with vnum destination.
 * Returns TRUE if the entry should be allowed and FALSE otherwise.
 */
int entry_mtrigger(char_data *ch, int destination) {
    trig_data *t;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_ENTRY) || !char_susceptible_to_triggers(ch))
        return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_ENTRY) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            sprintf(buf, "%d", destination);
            add_var(&GET_TRIG_VARS(t), "destination", buf);
            if (!script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW) || !ch)
                return FALSE;
        }
    }
    return TRUE;
}

/*
 * MOBILE COMMAND
 *
 * Checked whenever a command is typed.  Checks all mobiles in the room
 * for command triggers, and the first matching command trigger is used.
 * Command triggers with "*" for arguments match all commands.
 * Returns TRUE if the normal command action should be blocked, and
 * FALSE otherwise.
 */
int command_mtrigger(char_data *actor, char *cmd, char *argument) {
    char_data *ch, *ch_next;
    trig_data *t;
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
                    sprintf(buf, "SYSERR: Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
                    mudlog(buf, NRM, LVL_ATTENDANT, TRUE);
                    continue;
                }

                if (*GET_TRIG_ARG(t) == '*' || !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(cmd))) {
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

void speech_mtrigger(char_data *actor, char *str) {
    char_data *ch, *ch_next;
    trig_data *t;
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
                    sprintf(buf, "SYSERR: Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
                    mudlog(buf, NRM, LVL_GOD, TRUE);
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
void speech_to_mtrigger(char_data *actor, char_data *ch, char *str) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !char_susceptible_to_triggers(ch))
        return;

    if (MOB_PERFORMS_SCRIPTS(ch) && (SCRIPT_CHECK(ch, MTRIG_SPEECHTO) || SCRIPT_CHECK(ch, MTRIG_SPEECH)) && AWAKE(ch) &&
        actor != ch)
        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (!TRIGGER_CHECK(t, MTRIG_SPEECHTO) && !TRIGGER_CHECK(t, MTRIG_SPEECH))
                continue;

            if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
                sprintf(buf, "SYSERR: Speech-to Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
                mudlog(buf, NRM, LVL_GOD, TRUE);
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

void act_mtrigger(char_data *ch, const char *str, char_data *actor, char_data *victim, obj_data *object,
        obj_data *target, char *arg, char *arg2) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor))
        return;

    if (MOB_PERFORMS_SCRIPTS(ch) && SCRIPT_CHECK(ch, MTRIG_ACT))
        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (!TRIGGER_CHECK(t, MTRIG_ACT))
                continue;

            if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
                sprintf(buf, "SYSERR: Act Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
                mudlog(buf, NRM, LVL_GOD, TRUE);
                continue;
            }

            if (((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) ||
                 (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))) {

                if (actor)
                    ADD_UID_VAR(buf, t, actor, "actor");
                if (victim)
                    ADD_UID_VAR(buf, t, victim, "victim");
                if (object)
                    ADD_UID_VAR(buf, t, object, "object");
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

void fight_mtrigger(char_data *ch) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_FIGHT) || !FIGHTING(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_FIGHT) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            if (FIGHTING(ch))
                ADD_UID_VAR(buf, t, FIGHTING(ch), "actor")
            else
                add_var(&GET_TRIG_VARS(t), "actor", "nobody");
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

void hitprcnt_mtrigger(char_data *ch) {
    trig_data *t;
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

int receive_mtrigger(char_data *ch, char_data *actor, obj_data *obj) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_RECEIVE) || !char_susceptible_to_triggers(actor) ||
        !char_susceptible_to_triggers(ch))
        return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (IS_SET(GET_TRIG_TYPE(t), MTRIG_RECEIVE)) {
            if (GET_TRIG_DEPTH(t)) {
                /* The receive trigger is currently executing */
                act("$N isn't ready to accept $p.", FALSE, actor, obj, ch, TO_CHAR);
                return 0;
            } else if (number(1, 100) <= GET_TRIG_NARG(t)) {
                ADD_UID_VAR(buf, t, actor, "actor");
                ADD_UID_VAR(buf, t, obj, "object");
                return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            }
        }
    }

    return 1;
}

int death_mtrigger(char_data *ch, char_data *actor) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_DEATH) ||
        (actor && !char_susceptible_to_triggers(actor)) || !char_susceptible_to_triggers(ch))
        return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_DEATH)) {

            if (actor)
                ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

void load_mtrigger(char_data *ch) {
    trig_data *t;

    if (!SCRIPT_CHECK(ch, MTRIG_LOAD) || !char_susceptible_to_triggers(ch))
        return;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_LOAD) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int cast_mtrigger(char_data *actor, char_data *ch, int spellnum) {
    trig_data *t;

    if (ch == NULL)
        return 1;

    if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_CAST) || !char_susceptible_to_triggers(actor) ||
        !char_susceptible_to_triggers(ch))
        return 1;

    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
        if (TRIGGER_CHECK(t, MTRIG_CAST) && number(1, 100) <= GET_TRIG_NARG(t)) {
            ADD_UID_VAR(buf, t, actor, "actor");
            sprintf(buf, "%d", spellnum);
            add_var(&GET_TRIG_VARS(t), "spellnum", buf);
            add_var(&GET_TRIG_VARS(t), "spell", skill_name(spellnum));
            return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int leave_mtrigger(char_data *actor, int dir) {
    trig_data *t;
    char_data *ch;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (ch = world[IN_ROOM(actor)].people; ch; ch = ch->next_in_room) {
        if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_LEAVE) || !AWAKE(ch) || FIGHTING(ch) || ch == actor)
            continue;

        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (TRIGGER_CHECK(t, MTRIG_LEAVE) && CAN_SEE(ch, actor) && number(1, 100) <= GET_TRIG_NARG(t)) {
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

int door_mtrigger(char_data *actor, int subcmd, int dir) {
    trig_data *t;
    char_data *ch;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (ch = world[IN_ROOM(actor)].people; ch; ch = ch->next_in_room) {
        if (!MOB_PERFORMS_SCRIPTS(ch) || !SCRIPT_CHECK(ch, MTRIG_DOOR) || !AWAKE(ch) || FIGHTING(ch) || ch == actor ||
            !CAN_SEE(ch, actor))
            continue;

        for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
            if (TRIGGER_CHECK(t, MTRIG_DOOR) && number(1, 100) <= GET_TRIG_NARG(t)) {
                add_var(&GET_TRIG_VARS(t), "cmd", cmd_door[subcmd]);
                add_var(&GET_TRIG_VARS(t), "direction", dirs[dir]);
                ADD_UID_VAR(buf, t, actor, "actor");
                return script_driver(&ch, t, MOB_TRIGGER, TRIG_NEW);
            }
        }
    }

    return 1;
}

void time_mtrigger(char_data *ch) {
    trig_data *t;
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

/*
 *  object triggers
 */

void random_otrigger(obj_data *obj) {
    trig_data *t;

    if (!SCRIPT_CHECK(obj, OTRIG_RANDOM))
        return;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_RANDOM) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int timer_otrigger(struct obj_data *obj) {
    trig_data *t;

    if (!SCRIPT_CHECK(obj, OTRIG_TIMER))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next)
        if (TRIGGER_CHECK(t, OTRIG_TIMER) && number(1, 100) <= GET_TRIG_NARG(t))
            return script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);

    return 1;
}

int get_otrigger(obj_data *obj, char_data *actor) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(obj, OTRIG_GET) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_GET) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            /* Don't allow a get to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

/* checks for command trigger on specific object. assumes obj has cmd trig */
int cmd_otrig(obj_data *obj, char_data *actor, char *cmd, char *argument, int type) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND))
        for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
            if (!TRIGGER_CHECK(t, OTRIG_COMMAND))
                continue;

            if (IS_SET(GET_TRIG_NARG(t), type) && (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t))) {
                sprintf(buf, "SYSERR: O-Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
                mudlog(buf, NRM, LVL_GOD, TRUE);
                continue;
            }

            if (IS_SET(GET_TRIG_NARG(t), type) &&
                (*GET_TRIG_ARG(t) == '*' || !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(cmd)))) {

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

int command_otrigger(char_data *actor, char *cmd, char *argument) {
    obj_data *obj;
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


void attack_otrigger(char_data *actor, char_data *victim, int dam) {
    char buf[MAX_INPUT_LENGTH], dam_str[MAX_INPUT_LENGTH];
    trig_data *t;
    obj_data *obj;
    int i;

    if (!char_susceptible_to_triggers(actor))
        return 1;

    sprintf(dam_str, "%d", dam);

    for (i = 0; i < NUM_WEARS; ++i)  {
        obj = actor->equipment[i];
        if (obj && SCRIPT_CHECK(obj, OTRIG_ATTACK)) {
            for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
                if (TRIGGER_CHECK(t, OTRIG_ATTACK)) {
                    add_var(&GET_TRIG_VARS(t), "damage", dam_str);
                    ADD_UID_VAR(buf, t, actor, "attacker");
                    ADD_UID_VAR(buf, t, victim, "defender");
                    script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
                    break;
                }
            }
        }
        obj = victim->equipment[i];
        if (obj && SCRIPT_CHECK(obj, OTRIG_DEFEND)) {
            for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
                if (TRIGGER_CHECK(t, OTRIG_DEFEND)) {
                    add_var(&GET_TRIG_VARS(t), "damage", dam_str);
                    ADD_UID_VAR(buf, t, actor, "attacker");
                    ADD_UID_VAR(buf, t, victim, "defender");
                    script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
                    break;
                }
            }
        }
    }
}


int wear_otrigger(obj_data *obj, char_data *actor, int where) {
    trig_data *t;
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

int death_otrigger(struct char_data *actor) {
    trig_data *t;
    obj_data *obj;
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

int drop_otrigger(obj_data *obj, char_data *actor) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_DROP))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_DROP) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            /* Don't allow a drop to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

int remove_otrigger(obj_data *obj, char_data *actor) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_REMOVE))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_REMOVE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            /* Don't allow a remove to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj);
        }
    }

    return 1;
}

int give_otrigger(obj_data *obj, char_data *actor, char_data *victim) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!char_susceptible_to_triggers(actor) || !SCRIPT_CHECK(obj, OTRIG_GIVE))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_GIVE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            ADD_UID_VAR(buf, t, victim, "victim");
            /* Don't allow a give to take place, if the object is purged. */
            return (script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW) && obj && obj->carried_by == actor);
        }
    }

    return 1;
}

void load_otrigger(obj_data *obj) {
    trig_data *t;

    if (!SCRIPT_CHECK(obj, OTRIG_LOAD))
        return;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_LOAD) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
            return;
        }
    }
}

int cast_otrigger(char_data *actor, obj_data *obj, int spellnum) {
    trig_data *t;
    char buf[16];

    if (obj == NULL || !char_susceptible_to_triggers(actor))
        return 1;

    if (!SCRIPT_CHECK(obj, OTRIG_CAST))
        return 1;

    for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
        if (TRIGGER_CHECK(t, OTRIG_CAST) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            ADD_UID_VAR(buf, t, actor, "actor");
            sprintf(buf, "%d", spellnum);
            add_var(&GET_TRIG_VARS(t), "spellnum", buf);
            add_var(&GET_TRIG_VARS(t), "spell", skill_name(spellnum));
            return script_driver(&obj, t, OBJ_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int leave_otrigger(room_data *room, char_data *actor, int dir) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];
    int final = 1;
    obj_data *obj, *obj_next;

    if (!char_susceptible_to_triggers(actor))
        return 1;

    for (obj = room->contents; obj; obj = obj_next) {
        obj_next = obj->next_content;
        if (!SCRIPT_CHECK(obj, OTRIG_LEAVE))
            continue;

        for (t = TRIGGERS(SCRIPT(obj)); t; t = t->next) {
            if (TRIGGER_CHECK(t, OTRIG_LEAVE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
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

int consume_otrigger(obj_data *obj, char_data *actor, int cmd) {
    trig_data *t;
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

void time_otrigger(obj_data *obj) {
    trig_data *t;
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

/*
 *  world triggers
 */

void random_wtrigger(struct room_data *room) {
    trig_data *t;

    if (!SCRIPT_CHECK(room, WTRIG_RANDOM))
        return;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_RANDOM) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int preentry_wtrigger(struct room_data *room, char_data *actor, int dir) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];
    int rev_dir[] = {SOUTH, WEST, NORTH, EAST, DOWN, UP};

    if (!SCRIPT_CHECK(room, WTRIG_PREENTRY) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_PREENTRY) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int postentry_wtrigger(char_data *actor, int dir) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];
    int rev_dir[] = {SOUTH, WEST, NORTH, EAST, DOWN, UP};
    struct room_data *room = &world[IN_ROOM(actor)];

    if (!SCRIPT_CHECK(room, WTRIG_POSTENTRY) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_POSTENTRY) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "direction", dirs[rev_dir[dir]]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

void reset_wtrigger(struct room_data *room) {
    trig_data *t;

    if (!SCRIPT_CHECK(room, WTRIG_RESET))
        return;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_RESET) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
            break;
        }
    }
}

int command_wtrigger(char_data *actor, char *cmd, char *argument) {
    struct room_data *room;
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_COMMAND) || !char_susceptible_to_triggers(actor))
        return 0;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (!TRIGGER_CHECK(t, WTRIG_COMMAND))
            continue;

        if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
            sprintf(buf, "SYSERR: W-Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
            mudlog(buf, NRM, LVL_GOD, TRUE);
            continue;
        }

        if (*GET_TRIG_ARG(t) == '*' || !strn_cmp(GET_TRIG_ARG(t), cmd, strlen(cmd))) {
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

void speech_wtrigger(char_data *actor, char *str) {
    struct room_data *room;
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_SPEECH) || !char_susceptible_to_triggers(actor))
        return;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (!TRIGGER_CHECK(t, WTRIG_SPEECH))
            continue;

        if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
            sprintf(buf, "SYSERR: W-Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
            mudlog(buf, NRM, LVL_GOD, TRUE);
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

int drop_wtrigger(obj_data *obj, char_data *actor) {
    struct room_data *room;
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];
    int ret_val;

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_DROP) || !char_susceptible_to_triggers(actor))
        return 1;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next)
        if (TRIGGER_CHECK(t, WTRIG_DROP) && (number(1, 100) <= GET_TRIG_NARG(t))) {

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

int cast_wtrigger(char_data *actor, char_data *vict, obj_data *obj, int spellnum) {
    room_data *room;
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_CAST) || !char_susceptible_to_triggers(actor))
        return 1;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_CAST) && (number(1, 100) <= GET_TRIG_NARG(t))) {

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

int leave_wtrigger(struct room_data *room, char_data *actor, int dir) {
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(room, WTRIG_LEAVE) || !char_susceptible_to_triggers(actor))
        return 1;

    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_LEAVE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "direction", dirs[dir]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

int door_wtrigger(char_data *actor, int subcmd, int dir) {
    room_data *room;
    trig_data *t;
    char buf[MAX_INPUT_LENGTH];

    if (!SCRIPT_CHECK(&world[IN_ROOM(actor)], WTRIG_DOOR) || !char_susceptible_to_triggers(actor))
        return 1;

    room = &world[IN_ROOM(actor)];
    for (t = TRIGGERS(SCRIPT(room)); t; t = t->next) {
        if (TRIGGER_CHECK(t, WTRIG_DOOR) && (number(1, 100) <= GET_TRIG_NARG(t))) {
            add_var(&GET_TRIG_VARS(t), "cmd", cmd_door[subcmd]);
            add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[dir]);
            ADD_UID_VAR(buf, t, actor, "actor");
            return script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
        }
    }

    return 1;
}

void time_wtrigger(struct room_data *room) {
    trig_data *t;
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

/***************************************************************************
 * $Log: dg_triggers.c,v $
 * Revision 1.34  2010/07/27 22:14:35  mud
 * Fix death triggers on mobs when not killed by player.
 *
 * Revision 1.33  2009/03/20 13:56:22  jps
 * Moved coin info into an array of struct coindef.
 *
 * Revision 1.32  2009/03/17 07:59:42  jps
 * Moved str_str to strings.c
 *
 * Revision 1.31  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.30  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.29  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.28  2009/03/07 09:34:47  jps
 * Changed name of room Entry trigger to Preentry. Added a Postentry room
 *trigger type.
 *
 * Revision 1.27  2008/09/02 03:00:59  jps
 * Changed mob speech and ask triggers to respond to all speech.
 *
 * Revision 1.26  2008/08/24 02:37:01  myc
 * Fix function signature for reference to external function str_str.
 *
 * Revision 1.25  2008/08/15 04:56:53  jps
 * Exempt characters from triggers if they wouldn't be detectable.
 *
 * Revision 1.24  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.23  2008/03/21 19:31:59  jps
 * Prevent giving objects to mobs with running receive triggers.
 *
 * Revision 1.22  2008/03/09 02:08:16  jps
 * Fix logic in command_mtrigger.
 *
 * Revision 1.21  2008/03/08 23:55:53  jps
 * Use MOB_PERFORMS_SCRIPTS macro instead of checking for CHARM
 * affection.
 *
 * Revision 1.20  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.19  2008/02/02 04:27:55  myc
 * Changing the way script_driver works: you now pass it a pointer
 * to the pointer of what you want to run the script.  That is,
 * script_driver(&ch, ...) instead of script_driver(ch, ...).
 * Adding several new trigger types: cast, leave, door, time, load,
 * and consume.
 *
 * Revision 1.18  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.17  2008/01/15 06:49:52  myc
 * When a mob had a trigger marked both ask and speech and the mob
 * was asked a question, that same trigger would be executed twice,
 * once as an ask trigger, and once as a speech trigger.  Fixed this
 * by going through the trigger list only once looking for ask or
 * speech triggers.  However, this isn't optimal--it should give
 * priority to ask triggers.
 *
 * Revision 1.16  2008/01/15 03:18:19  myc
 * Changed bribe_mtrigger to accept an array instead of a pointer.
 *
 * Revision 1.15  2007/11/18 16:51:55  myc
 * Fixing LVL_BUILDER references.
 *
 * Revision 1.14  2007/10/04 16:20:24  myc
 * Added object timer trigger.
 *
 * Revision 1.13  2007/08/31 05:36:19  jps
 * Add variable "destination" to mob Entry trigger.
 *
 * Revision 1.12  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.11  2006/12/08 05:06:58  myc
 * Bribe triggers now give proper amounts and variables.
 *
 * Revision 1.10  2006/11/30 05:06:24  jps
 * Add remove trigger for objects
 *
 * Revision 1.9  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.8  2000/11/22 01:47:05  mtp
 * allow command triggers to run the command on completeion
 *
 * Revision 1.7  2000/11/21 04:37:33  rsd
 * Altered the comment header and added back rlog messages
 * from prior to the addition of the $log$ string.
 *
 * Revision 1.6  2000/11/11 12:03:42  mtp
 * made function ask_mtrigger() void cos it doesnt return anything
 *
 * Revision 1.5  2000/11/11 01:39:05  mtp
 * added ASK trigger for mobs
 *
 * Revision 1.4  2000/02/01 21:02:39  mtp
 * added a %position% variable to object 'wear' trigger
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/31 02:05:15  mud
 * Added fiery stuff to the comment header
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

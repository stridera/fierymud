/***************************************************************************
 *   File: quest.c                                        Part of FieryMUD *
 *  Usage: Implementation of routines for handling quests                  *
 *     By: Matt Proctor 22 Oct 2000                                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 2000 by the Fiery Consortium                    *
 ***************************************************************************/

#define __QUEST_C__

#include "quest.hpp"

#include "casting.hpp"
#include "chars.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "races.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Local functions */
QuestList *quest_find_char(CharData *ch, char *qname);

/* Quest defines */
int max_quests = 0;
QuestInfo *all_quests = nullptr;

void boot_quests() {
    FILE *fl;
    char line[256];
    char *quest_name;
    int quest_num, max_stages, num_records = 0;

    max_quests = 0;

    if ((fl = fopen(ALL_QUEST_FILE, "r")) == nullptr) {
        fprintf(stderr, "Unable to find any quest data file (non-fatal)\n");
        return;
    }

    while (get_line(fl, line))
        ++num_records;

    fclose(fl);

    CREATE(all_quests, QuestInfo, num_records);

    fl = fopen(ALL_QUEST_FILE, "r");
    while (get_line(fl, line)) {
        CREATE(quest_name, char, 30);
        if (sscanf(line, "%s %d %d", quest_name, &quest_num, &max_stages) != 3) {
            fprintf(stderr, "Error in quest file line format (%s)\n", line);
            free(quest_name);
            break;
        } else {
            all_quests[max_quests].quest_name = quest_name;
            all_quests[max_quests].quest_id = quest_num;
            all_quests[max_quests].maxstages = max_stages;
            ++max_quests;
        }
    }
    fclose(fl);
}

/* quest_stat - returns true if any stat info was listed */
bool quest_stat(CharData *ch, CharData *vict, char *qname) {
    QuestList *quest = quest_find_char(vict, qname);
    QuestVariableList *vars;
    int qid_num;

    if (!quest)
        return false;

    if ((qid_num = real_quest(quest->quest_id)) >= 0) {
        char_printf(ch, "Quest %s: ", all_quests[qid_num].quest_name);
        if (quest->stage == QUEST_SUCCESS)
            char_printf(ch, "Completed\n");
        else if (quest->stage == QUEST_FAILURE)
            char_printf(ch, "Failed\n");
        else {
            char_printf(ch, "Stage {:d}\n", quest->stage);
        }
        for (vars = quest->variables; vars; vars = vars->next) {
            char_printf(ch, "\t{} = {}\n", vars->var, vars->val);
        }
    } else {
        char_printf(ch, "Invalid quest: {}\n", qname);
    }

    return true;
}

void quest_mstat(CharData *ch, CharData *vict) {
    int qnum;
    bool any = false;

    for (qnum = 0; qnum < max_quests; qnum++)
        if (quest_stat(ch, vict, all_quests[qnum].quest_name))
            any = true;

    if (!any)
        char_printf(ch, "No quest data found.\n");
}

CharData *quest_id_char(TrigData *t, CharData *ch, char *name) {
    CharData *vict;

    if (!(vict = find_char_for_keyword(ch, name)))
        vict = find_char_in_world(find_by_name(name));
    if (vict && IS_NPC(vict))
        vict = nullptr;
    if (!vict) {
        if (t)
            log(LogSeverity::Warn, LVL_GOD, "QUEST ERROR: quest command couldn't find player {} in trigger {}", name,
                GET_TRIG_VNUM(t));
        else if (ch)
            char_printf(ch, "There's no player by that name here.\n");
        return nullptr;
    }
    return vict;
}

ACMD(do_qstat) {
    CharData *vict;
    char *quest_name;

    argument = any_one_arg(argument, arg);  /* player name */
    argument = any_one_arg(argument, buf1); /* quest */

    if (!*arg) {
        char_printf(ch, "Usage: qstat <player> [<quest>]\n");
        return;
    }

    if (!(vict = quest_id_char(nullptr, ch, arg)))
        return;

    if (*buf1) {
        /* Two arguments: a specific quest */

        /* Try and figure out which quest we want. */
        if (!(quest_name = check_quest_name(buf1))) {
            char_printf(ch, "That is not a valid quest name.\n");
            return;
        }
        if (!quest_stat(ch, vict, quest_name)) {
            char_printf(ch, "No quest data was found.\n");
        }
    } else {
        /* One argument: display all quests for this player */
        quest_mstat(ch, vict);
    }
}

void perform_quest(TrigData *t, char *argument, CharData *ch, ObjData *obj, RoomData *room) {
    char error_string[MAX_INPUT_LENGTH * 2], *quest_name, e2[MAX_INPUT_LENGTH * 2];
    CharData *vict;
    int amount;

    skip_spaces(&argument);
    argument = any_one_arg(argument, arg);  /* command */
    argument = any_one_arg(argument, buf1); /* quest */
    argument = any_one_arg(argument, buf2); /* player name */

    /* Check for the "mstat" command first, since it requires one fewer argument
     * than the rest of them.  This one can only be done by a character (not a
     * trigger). */
    if (*arg && !strcasecmp(arg, "mstat") && ch) {
        if (*buf1) {
            /* OK: mstat command, buf1 = player name */
            if ((vict = quest_id_char(nullptr, ch, buf1)))
                quest_mstat(ch, vict);
        } else {
            char_printf(ch, "Usage: quest mstat <player>\n");
        }
        return;
    }

    /* Make sure we have a quest command. */
    if (!*arg || !*buf1 || !*buf2) {
        if (t) {
            log(LogSeverity::Stat, LVL_GOD, "QUEST ERROR: quest command called with less than 3 args by trigger {:d}",
                GET_TRIG_VNUM(t));
        } else if (ch)
            char_printf(ch, "Usage: quest <command> <quest> <player> [<subclass>] [<var name> <var value>]\n");
        return;
    }

    /* Try and figure out which quest we want. */
    if (!(quest_name = check_quest_name(buf1))) {
        if (t) {
            log(LogSeverity::Stat, LVL_GOD,
                "QUEST ERROR: quest command tried to access invalid quest {} in trigger {:d}", buf1, GET_TRIG_VNUM(t));
        } else if (ch)
            char_printf(ch, "That is not a valid quest name.\n");
        return;
    }

    /* Try and figure out who we want to affect. */
    vict = quest_id_char(t, ch, buf2);
    if (!vict)
        return;

    /* At this point, we assume we have a valid quest_name and victim.  Now
       we just have to see if we have valid command.  But first, let's set
       up the standard error string. Also, we're done with buf1 and buf2
       so they can be reused. */

    /* Mob / deity error string */
    if (ch) {
        if (t)
            sprintf(error_string, "QUEST ERROR: %s (%d) tried to {} [%s on quest %s in trigger %d]", GET_NAME(ch),
                    GET_MOB_VNUM(ch), GET_NAME(vict), quest_name, GET_TRIG_VNUM(t));
        else
            sprintf(error_string, "QUEST_ERROR: %s tried to {} [%s on quest %s]", GET_NAME(ch), GET_NAME(vict),
                    quest_name);
    }
    /* Object trigger error string */
    else if (obj) {
        sprintf(error_string, "QUEST ERROR: %s (%d) tried to {} [%s on quest %s in trigger %d]", obj->short_description,
                GET_OBJ_VNUM(obj), GET_NAME(vict), quest_name, GET_TRIG_VNUM(t));
    }
    /* Room trigger error string */
    else if (room) {
        sprintf(error_string, "QUEST ERROR: %s (%d) tried to {} [%s on quest %s in trigger %d]", room->name, room->vnum,
                GET_NAME(vict), quest_name, GET_TRIG_VNUM(t));
    }
    /* Other error string */
    else {
        sprintf(error_string, "QUEST ERROR: unknown actor tried to {} [%s on quest %s in trigger %d]", GET_NAME(vict),
                quest_name, GET_TRIG_VNUM(t));
    }

    /*
     * This is kind of a hack.  The quest_ subfunctions below use the ch
     * argument to decide whether to log an error or just char_printf.
     * If ch is NULL, then it logs.  So to make sure we get errors logged
     * for mob triggers, we set ch to NULL if there's a trigger.
     */
    if (t)
        ch = nullptr;

    /* advance
     *
     * Advances player <vict> by <amount> stages in the quest <quest_name>.
     * Defaults to 1 stage.
     */
    if (!strncasecmp(arg, "advance", strlen(arg))) {
        argument = any_one_arg(argument, buf1);
        amount = atoi(buf1);
        if (!amount && !*buf1) /* If no amount given, then advance by 1. */
            amount = 1;
        quest_advance(ch, vict, quest_name, error_string, amount);
    }

    /* start
     *
     * Start player <vict> on the quest <quest_name>.
     */
    else if (!strncasecmp(arg, "start", strlen(arg))) {
        argument = any_one_arg(argument, buf1);
        quest_start(ch, vict, quest_name, error_string, buf1);
    }

    /* variable
     *
     * Set quest variable <buf1> to value <buf2> on player <vict> on the
     * quest <quest_name>, or check quest variable <buf1> on player <vict>
     * on the quest <quest_name>.
     */
    else if (!strncasecmp(arg, "variable", strlen(arg))) {
        argument = any_one_arg(argument, buf1);
        argument = any_one_arg(argument, buf2);
        if (ch && !*buf1)
            char_printf(ch, "Which variable?\n");
        else if (ch && !*buf2) {
            char_printf(ch, "Variable {} on {} for quest {}: {}\n", buf1, GET_NAME(vict), quest_name,
                        get_quest_variable(vict, quest_name, buf1));
        } else if (*buf1 && *buf2)
            set_quest_variable(ch, vict, quest_name, error_string, buf1, buf2);
        else if (!*buf1) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "set variable with no variable");
        } else if (!*buf2) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "set variable \"{}\" with no value", buf1);
        }
    }

    /* complete
     *
     * Sets quest <quest_name> to complete on player <vict>.
     */
    else if (!strncasecmp(arg, "complete", strlen(arg)))
        quest_complete(ch, vict, quest_name, error_string);

    /* fail
     *
     * Sets quest <quest_name> to failed on player <vict>.
     */
    else if (!strncasecmp(arg, "fail", strlen(arg)))
        quest_fail(ch, vict, quest_name, error_string);

    /* rewind
     *
     * Rewinds player <vict> by <amount> stages in the quest <quest_name>.
     * Defaults to 1 stage.
     */
    else if (!strncasecmp(arg, "rewind", strlen(arg))) {
        argument = any_one_arg(argument, buf1);
        amount = atoi(buf1);
        if (!amount && !*buf1) /* If no amount given, then rewind by 1. */
            amount = 1;
        quest_rewind(ch, vict, quest_name, error_string, amount);
    }

    /* restart
     *
     * Start player <vict> on the quest <quest_name> if they have already
     * started it.
     */
    else if (!strncasecmp(arg, "restart", strlen(arg)))
        quest_restart(ch, vict, quest_name, error_string);

    /* erase
     *
     * Erase quest <quest_name> on player <vict>.
     */
    else if (!strncasecmp(arg, "erase", strlen(arg)))
        quest_erase(ch, vict, quest_name, error_string);

    /* stage
     *
     * Shows stage for quest <quest_name> on player <vict>.
     */
    else if (ch && !strncasecmp(arg, "stage", strlen(arg))) {
        if (quest_find_char(vict, quest_name)) {
            amount = quest_stage(vict, quest_name);
            if (amount == QUEST_FAILURE)
                char_printf(ch, "{} has failed the {} quest.\n", GET_NAME(vict), quest_name);
            else if (amount == QUEST_SUCCESS)
                char_printf(ch, "{} has completed the {} quest.\n", GET_NAME(vict), quest_name);
            else
                char_printf(ch, "{} is on stage {:d} of the {} quest.\n", GET_NAME(vict), amount, quest_name);
        } else {
            char_printf(ch, "{} has not started the {} quest.\n", GET_NAME(vict), quest_name);
        }
    }

    /* stat
     *
     * Shows variable data for a quest
     */
    else if (ch && !strcasecmp(arg, "stat")) {
        if (!quest_stat(ch, vict, quest_name)) {
            char_printf(ch, "No quest data was found.\n");
        }
    }

    else {
        if (t) {
            sprintf(buf, "use an invalid command %s", arg);
            log(LogSeverity::Stat, LVL_GOD, error_string, buf);
        } else if (ch) {
            char_printf(ch, "Sorry, {} is not a valid quest command.\n", arg);
        }
    }
}

/*
 * quest_find_num
 *
 * descr:	internal routine to return the quest_id for  a specified quest
 * name so that all quests can be referred to by names externally.
 *
 * NOTE:	uses strncasecmp  so abbreviations of quest anmes are tolerated.
 */
unsigned short quest_find_num(char *qname) {
    int count;
    /*
     * Loop through the quests and if we find the one we're looking for,
     * return its quest id.
     */
    for (count = 0; count < max_quests; count++) {
        if (!strncasecmp(all_quests[count].quest_name, qname, strlen(qname)))
            return all_quests[count].quest_id;
    }
    return 0;
}

/*
 * utility function for perform_quest.  Returns the full qualified name of
 * a quest based on an abbreviation.
 */
char *check_quest_name(char *qname) {
    int count;
    for (count = 0; count < max_quests; count++)
        if (!strncasecmp(all_quests[count].quest_name, qname, strlen(qname)))
            return all_quests[count].quest_name;
    return nullptr;
}

/*
 * quest_find_max_stage
 *
 * Internal routine to find the maximum stage for a quest.
 */
short quest_find_max_stage(char *qname) {
    int count = 0;

    for (count = 0; count < max_quests; count++)
        if (!strncasecmp(all_quests[count].quest_name, qname, strlen(qname)))
            return all_quests[count].maxstages;
    return 0;
}

/*
 * quest_find_char
 *
 * descr:	internal routine to find a quest in a quest_list
 */
QuestList *quest_find_char(CharData *ch, char *qname) {
    QuestList *quest;
    int quest_num = quest_find_num(qname);

    for (quest = ch->quests; quest; quest = quest->next)
        if (quest->quest_id == quest_num)
            return quest;
    return nullptr;
}

/*
 * get_quest_variable
 */
char *get_quest_variable(CharData *vict, char *qname, char *variable) {
    QuestList *quest = quest_find_char(vict, qname);
    QuestVariableList *vars;

    if (!quest)
        return "0";

    for (vars = quest->variables; vars; vars = vars->next)
        if (!strcasecmp(variable, vars->var))
            return vars->val;

    return "0";
}

void set_quest_variable(CharData *ch, CharData *vict, char *qname, char *error_string, char *variable, char *value) {
    QuestList *quest = quest_find_char(vict, qname);
    QuestVariableList *vars;

    if (!quest) {
        if (ch) {
            char_printf(ch, "You can't set variables on a quest {} hasn't started yet.\n", GET_NAME(vict));
        } else {
            log(LogSeverity::Stat, LVL_GOD, error_string, "set variable on nonexistent quest");
        }
        return;
    }

    for (vars = quest->variables; vars; vars = vars->next)
        if (!strcasecmp(variable, vars->var)) {
            strncpy(vars->val, value, 20);
            return;
        }

    CREATE(vars, QuestVariableList, 1);
    CREATE(vars->var, char, 21);
    strncpy(vars->var, variable, 20);
    vars->var[20] = '\0';
    CREATE(vars->val, char, 21);
    strncpy(vars->val, value, 20);
    vars->val[20] = '\0';
    vars->next = quest->variables;
    quest->variables = vars;

    if (ch) {
        char_printf(ch, "Set quest {} variable {} to '{}' on {}.\n", qname, variable, value, GET_NAME(vict));
    }
}

/*
 * quest_stage
 *
 * descr:	returns the value (int) of the stage the specified char is at
 * 		for the specified quest, or 0 if the char has not started the
 * 		quest or the quest does not exist.
 *
 * NOTE:	if a zero is returned here it could mean there was an error or
 * 		that the char has failed this quest, so call has_failed_quest
 * 		to be sure
 */
int quest_stage(CharData *ch, char *qname) {
    QuestList *quest;

    if (!ch)
        return (int)quest_find_max_stage(qname);

    if ((quest = quest_find_char(ch, qname)))
        return quest->stage;

    return 0;
}

/*
 * quest_complete, quest_fail, quest_advance, quest_start
 *
 * descr:	stage manipulation routines for quest, failure means set all bits
 * to 0 success means set all bits to 1 and advance is add one to current stage
 *
 * returns:	all return the newly set stage if successful, except quest_fail
 * which returns 1 if successful (cos new stage is 0!)
 *
 * NOTE:	advance will fail and log an error if the quest is already at stage
 * 254 or 0 because starting a quest sets stage to 1 and we can't retry a failed
 * 		quest, and must sue quest_complete to set to 255. If you need
 * more than 254 stages in your quest you are an evil bastard and should seek
 * help.
 */
void quest_complete(CharData *ch, CharData *vict, char *qname, char *error_string) {
    QuestList *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            char_printf(ch, "{} hasn't started that quest yet.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "complete nonexistent quest");
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            char_printf(ch, "{} has already completed that quest!\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "complete already-completed quest");
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            char_printf(ch, "{} has already failed that quest!\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "complete already-failed quest");
        }
        return;
    }

    quest->stage = QUEST_SUCCESS;

    char_printf(vict, "Congratulations, you completed the {} quest!\n", qname);

    if (ch) {
        char_printf(ch, "Set the {} quest to completed&0 on {}.\n", qname, GET_NAME(vict));
    }
}

void quest_fail(CharData *ch, CharData *vict, char *qname, char *error_string) {
    QuestList *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            char_printf(ch, "{} hasn't started that quest yet.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "fail nonexistent quest");
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            char_printf(ch, "{} has already completed that quest!\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "fail already-completed quest");
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            char_printf(ch, "{} has already failed that quest!\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "fail already-failed quest");
        }
        return;
    }

    quest->stage = QUEST_FAILURE;

    if (ch) {
        char_printf(ch, "Set the {} quest to failed&0 on {}.\n", qname, GET_NAME(vict));
    }
}

/*
 * quest advance
 *
 */
void quest_advance(CharData *ch, CharData *vict, char *qname, char *error_string, int amount) {
    short max_stage;
    QuestList *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            char_printf(ch, "{} hasn't started that quest yet.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "advance nonexistent quest");
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            char_printf(ch, "{} has already failed this quest.  Use quest restart.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "advance failed quest");
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            char_printf(ch, "{} has already completed this quest.  Use quest restart.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "advance already completed quest");
        }
        return;
    }

    max_stage = quest_find_max_stage(qname);
    if (quest->stage + amount > max_stage) {
        quest->stage = max_stage;
        if (ch) {
            char_printf(ch, "You can't advance past the quest's max stage.  {}'s stage set to max.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "advance past quest max stage of {:d}", max_stage);
        }
        return;
    }

    /* Great success! */
    quest->stage += amount;

    char_printf(ch, "Advanced the {} quest's stage by {:d} to {:d} on {}.\n", qname, amount, quest->stage,
                GET_NAME(vict));
}

/*
 * quest start
 *
 */
void quest_start(CharData *ch, CharData *vict, char *qname, char *error_string, char *subclass_abbr) {
    QuestList *quest = quest_find_char(vict, qname);
    extern int class_ok_race[NUM_RACES][NUM_CLASSES];
    unsigned short quest_num;
    int subclass = CLASS_UNDEFINED;

    if (quest) {
        if (ch) {
            char_printf(ch, "{} has already started this quest.  Use quest restart.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "start already-started quest");
        }
        return;
    }

    quest_num = quest_find_num(qname);

    if (!*subclass_abbr && IS_SUBCLASS_QUEST(quest_num)) {
        if (ch)
            char_printf(ch, "You must supply the name of the subclass for this quest.\n");
        else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "start subclass quest without specifying which");
        }
        return;
    }

    if (*subclass_abbr && IS_SUBCLASS_QUEST(quest_num)) {

        /* A player can only be on one subclass quest at a time, so check for
         * another one. */
        quest = vict->quests;
        while (quest) {
            if (quest->quest_id & SUBCLASS_BIT) {
                if (ch) {
                    char_printf(ch, "{} is already on a subclass quest.  Erase it before starting another.\n",
                                GET_NAME(vict));
                } else if (error_string) {
                    log(LogSeverity::Stat, LVL_GOD, error_string, "start subclass quest with one already in progress");
                }
                return;
            }
            quest = quest->next;
        }

        subclass = parse_class(0, 0, subclass_abbr);

        if (subclass == CLASS_UNDEFINED) {
            if (ch)
                char_printf(ch, "That's not a valid class.\n");
            else if (error_string) {
                log(LogSeverity::Stat, LVL_GOD, error_string, "start subclass quest with invalid class abbreviation");
            }
            return;
        }

        /* Is the player a base class? */
        if (classes[(int)GET_CLASS(vict)].is_subclass) {
            if (ch) {
                char_printf(ch, "{} has already subclassed.\n", GET_NAME(vict));
            } else if (error_string) {
                log(LogSeverity::Stat, LVL_GOD, error_string, "start subclass quest on already subclassed player");
            }
            return;
        }

        /* Is the proposed class a subclass of the player's current class? */
        if (!classes[subclass].is_subclass || !classes[subclass].subclass_of == GET_CLASS(vict)) {
            if (ch) {
                char_printf(ch, "Invalid subclass for {}'s class.\n", GET_NAME(vict));
            } else if (error_string) {
                log(LogSeverity::Stat, LVL_GOD, error_string,
                    "start subclass quest when player's class does not permit it");
            }
            return;
        }

        /* Does the player's race permit this subclass? */
        if (!class_ok_race[(int)GET_RACE(vict)][subclass]) {
            if (ch) {
                char_printf(ch, "Invalid subclass for {}'s race.\n", GET_NAME(vict));
            } else if (error_string) {
                log(LogSeverity::Stat, LVL_GOD, error_string,
                    "start subclass quest when player's race does not permit it");
            }
            return;
        }

        /* You must be at least level 10 to start a subclass quest,
         * unless a deity hooks you up. */
        if (GET_LEVEL(vict) < 10 && !ch) {
            if (error_string) {
                log(LogSeverity::Stat, LVL_GOD, error_string, "start subclass quest before player is level 10");
            }
            return;
        }

        /* You must be less than the subclass quest max level to start the
         * quest, unless a deity hooks you up. */
        if (!ch && GET_LEVEL(vict) > classes[(int)GET_CLASS(vict)].max_subclass_level) {
            if (error_string) {
                log(LogSeverity::Stat, LVL_GOD, error_string,
                    "start subclass quest after player is past max subclass level");
            }
            return;
        }

        /* You may begin the subclass quest. */
    }

    /* Okay, make the new quest */
    CREATE(quest, QuestList, 1);
    quest->quest_id = quest_num;
    quest->stage = QUEST_START;
    quest->next = vict->quests;
    vict->quests = quest;

    /* For subclass quests, store the name of the subclass in a quest variable. */
    if (subclass != CLASS_UNDEFINED) {
        set_quest_variable(ch, vict, qname, error_string, "subclass_name", classes[subclass].name);
    }

    if (ch) {
        char_printf(ch, "Started the {} quest on {} and set stage to 1.\n", qname, GET_NAME(vict));
    }
}

void quest_rewind(CharData *ch, CharData *vict, char *qname, char *error_string, int amount) {
    QuestList *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            char_printf(ch, "{} hasn't started that quest yet.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "rewind nonexistent quest");
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            char_printf(ch, "{} has already failed this quest.  Use quest restart.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "rewind failed quest");
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            char_printf(ch, "{} has already completed this quest.  Use quest restart.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "rewind already completed quest");
        }
        return;
    }

    if (quest->stage - amount < 1) {
        quest->stage = 1;
        if (ch) {
            char_printf(ch, "You can't rewind past the first stage.  {}'s stage set to 1.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "rewind past quest first stage");
        }
        return;
    }

    /* Great success! */
    quest->stage -= amount;

    if (ch) {
        char_printf(ch, "Rewound the {} quest's stage by {} to {} on {}.\n", qname, amount, quest->stage,
                    GET_NAME(vict));
    }
}

void quest_restart(CharData *ch, CharData *vict, char *qname, char *error_string) {
    QuestList *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            char_printf(ch, "{} hasn't started that quest yet.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "restart nonexistent quest");
        }
        return;
    }

    quest->stage = QUEST_START;

    if (ch) {
        char_printf(ch, "Restarted the {} quest on {} by resetting stage to 1.\n", qname, GET_NAME(vict));
    }
}

/*
 * quest_erase
 *
 * descr:	Removes a named quest from a player (no forced save!)
 *
 * returns:	1 if quest removed from player, 0 if quest not exist or
 * 		unable to remove it
 *
 * NOTE:	This procedure does not force a save of the player. This
 * 		procedure does no checking of stage or even subclassiness - if
 * 		this was a completed subclass and the player has CHANGED
 * 		subclass, then they will not be changed back.
 */
void quest_erase(CharData *ch, CharData *vict, char *qname, char *error_string) {
    QuestList *quest, *prev;
    /*   quest_var_list *vars, *next_vars; */
    unsigned short quest_num;

    if (!vict->quests) {
        if (ch) {
            char_printf(ch, "{} hasn't started that quest yet.\n", GET_NAME(vict));
        } else if (error_string) {
            log(LogSeverity::Stat, LVL_GOD, error_string, "erase nonexistent quest");
        }
        return;
    }

    quest_num = quest_find_num(qname);

    for (quest = vict->quests; quest; prev = quest, quest = quest->next) {
        /* Skip quests with quest_ids that don't match. */
        if (quest->quest_id != quest_num)
            continue;
        /* If we're here, the current quest matches. */
        if (quest == vict->quests)
            vict->quests = vict->quests->next; /* quest is the first quest */
        else
            prev->next = quest->next; /* quest is not the first quest */
        /* Free variables */
        /* For some reason this causes a crash on production.  I dunno.
            for (vars = quest->variables; vars; vars = next_vars) {
              next_vars = vars->next;
              if (vars->var)
                free(vars->var);
              if (vars->val)
                free(vars->val);
              free(vars);
            }
        */
        free(quest);
        if (ch) {
            char_printf(ch, "Erased the {} quest from {}.\n", qname, GET_NAME(vict));
        }
        return;
    }

    if (ch) {
        char_printf(ch, "{} hasn't started that quest yet.\n", GET_NAME(vict));
    } else if (error_string) {
        log(LogSeverity::Stat, LVL_GOD, error_string, "erase nonexistent quest");
    }
}

/*
 * has_failed_quest
 *
 * descr:	Has the player failed the quest specified
 *
 * returns:	1 if the player has failed the quest, 0 if the quest is not
 * failed or if the quest does not exist, or the player has not started it.
 *
 * NOTE:	if has_failed_quest returns 0, and quest_stage also returns 0
 * then the quest does not exist or the player has not started it
 */
int has_failed_quest(char *qname, CharData *ch) {
    QuestList *quest = quest_find_char(ch, qname);

    if (quest && quest->stage == QUEST_FAILURE)
        return 1;

    return 0;
}

/*
 * has_completed_quest
 *
 * descr:	Has the player completed the quest specified
 *
 * returns:	1 if the player has completed the quest successfully, 0 if the
 * quest is not completed or if the quest does not exist, or the player has not
 * started it.
 *
 */
int has_completed_quest(char *qname, CharData *ch) {
    QuestList *quest = quest_find_char(ch, qname);

    if (quest && quest->stage == QUEST_SUCCESS)
        return 1;

    return 0;
}

/*
 * do_add
 *
 * descr:	Allows user to config a new quest (add to misc/quests and array)
 */
ACMD(do_qadd) {
    int new_id = 1, max_id, i, stages;
    FILE *fp;

    argument = any_one_arg(argument, buf1);
    argument = any_one_arg(argument, buf2);

    if (!*buf1 || !*buf2) {
        char_printf(ch, "Usage: qadd <quest_name> <max_stages> [<yes_if_subclass_quest>]\n");
        return;
    }

    if (strlen(buf1) >= MAX_QNAME_LEN) {
        char_printf(ch, "The quest name may not be longer than {:d} characters.\n", MAX_QNAME_LEN - 1);
        return;
    }

    /* Can't create a new quest with the same name as another one! */
    if (quest_find_num(buf1)) {
        char_printf(ch, "There's already a quest named {}!\n", buf1);
        return;
    }

    /* Check and see if this is going to be a subclass quest.  Start the
     * new_id appropriately if so.  We have to check to make sure arg is
     * longer than 0, otherwise all quests are subclass quests.
     */
    argument = any_one_arg(argument, arg);
    if (*arg && !strncasecmp("yes", arg, strlen(arg))) {
        max_id = (MAX_SUBCLASS_QUEST_ID | SUBCLASS_BIT);
        new_id |= SUBCLASS_BIT;
    } else
        max_id = MAX_QUEST_ID;

    /* Generate a new quest ID. */
    for (i = 0; i < max_quests; i++) {
        if (all_quests[i].quest_id > new_id)
            break;
        new_id++;
    }

    if (new_id > max_id) {
        char_printf(ch, "No free quest IDs are left.  Remove some old ones first.\n");
        return;
    }

    if (!(stages = atoi(buf2))) {
        char_printf(ch, "You must specify an integer for the maximum number of stages.\n");
        return;
    }
    if (real_quest(new_id) != NOWHERE) {
        char_printf(ch, "Unable to allocate a quest ID for the new quest.\n");
        return;
    }

    /*
     * rewrite the quest file and add this one in the correct place
     */
    if ((fp = fopen(ALL_QUEST_FILE, "w")) == nullptr) {
        log("Unable to open ALL_QUEST_FILE in qadd\n");
        return;
    }
    if (max_quests)
        for (i = 0; (all_quests[i].quest_id < new_id) && (i < max_quests); i++)
            fprintf(fp, "%s %d %d\n", all_quests[i].quest_name, all_quests[i].quest_id, all_quests[i].maxstages);
    fprintf(fp, "%s %d %d\n", buf1, new_id, stages);
    if (max_quests)
        for (; i < max_quests; i++)
            fprintf(fp, "%s %d %d\n", all_quests[i].quest_name, all_quests[i].quest_id, all_quests[i].maxstages);
    fclose(fp);

    /*
     * Reboot quests.
     */
    boot_quests();
    if (new_id & SUBCLASS_BIT) {
        log(LogSeverity::Stat, MAX(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} created a new subclass quest {}.",
            GET_NAME(ch), buf1);
        char_printf(ch, "New subclass&0 quest {} successfully added.\n", buf1);
    } else {
        log(LogSeverity::Stat, MAX(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} created a new quest {}.", GET_NAME(ch), buf1);
        char_printf(ch, "New quest {} successfully added.\n", buf1);
    }
}

/*
 * do_qdel
 *
 * descr:	opposite of qadd, removes a quest from global list with NO
 * CONFIRM note: doesn't remove quests from players, these will be removed on
 * 		next login, but stats will stop showing themimmediately
 */
ACMD(do_qdel) {
    int quest_num = 0, i;
    FILE *fp;

    argument = any_one_arg(argument, buf1);

    if (!*buf1) {
        char_printf(ch, "Usage: qdel <quest_name>\n");
        return;
    }

    /*
     * Instead of using quest_find_num, we'll search the quest index
     * ourselves, because quest_find_num allows abbreviations, and I
     * think we should be a little more careful than that when deleting.
     */
    for (i = 0; i < max_quests; i++)
        if (!strcasecmp(all_quests[i].quest_name, buf1)) {
            quest_num = all_quests[i].quest_id;
            break;
        }

    /* We didn't find the quest. */
    if (!quest_num) {
        char_printf(ch, "There is no quest named {}.\n", buf1);
        return;
    }

    /* Rewrite the quest file and drop this one. */
    if (!(fp = fopen(ALL_QUEST_FILE, "w"))) {
        log("Unable to open ALL_QUEST_FILE in qdel.\n");
        return;
    }

    if (max_quests)
        for (i = 0; i < max_quests; i++)
            if (all_quests[i].quest_id != quest_num)
                fprintf(fp, "%s %d %d\n", all_quests[i].quest_name, all_quests[i].quest_id, all_quests[i].maxstages);

    fclose(fp);

    /* Reboot quests. */
    boot_quests();
    if (IS_SUBCLASS_QUEST(quest_num)) {
        log(LogSeverity::Stat, MAX(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} deleted subclass quest {}.", GET_NAME(ch),
            buf1);
        char_printf(ch, "Subclass quest {} successfully deleted.\n", buf1);
    } else {
        log(LogSeverity::Stat, MAX(LVL_GOD, GET_INVIS_LEV(ch)), "(GC) {} deleted quest {}.", GET_NAME(ch), buf1);
        char_printf(ch, "Quest {} successfully deleted.\n", buf1);
    }
}

/*
 * do_qlist
 *
 * descr:	just lists all the quests in the array..nothing fancy (yet)
 */
ACMD(do_qlist) {
    int i;
    char intbuf[8];
    char msgbuf[100];

    sprintf(msgbuf, "There are %d quests:\n====================\n", max_quests);
    char_printf(ch, msgbuf);
    sprintf(msgbuf, "%35s %6s %15s\n", "Quest Name", "Stages", "Quest Type");
    char_printf(ch, msgbuf);

    for (i = 0; i < max_quests; i++) {
        sprintf(intbuf, "%d", all_quests[i].maxstages);
        sprintf(msgbuf, "%35s %6s %15s\n", all_quests[i].quest_name, intbuf,
                (IS_SUBCLASS_QUEST(all_quests[i].quest_id)) ? "Subclass Quest" : "Regular Quest");
        char_printf(ch, msgbuf);
    }
}

void free_quest_list(CharData *ch) {
    QuestList *quest, *next_quest;

    quest = ch->quests;
    while (quest) {
        next_quest = quest->next;
        if (quest->variables) {
            QuestVariableList *quest_var;
            QuestVariableList *next_var;

            quest_var = quest->variables;

            while (quest_var) {
                next_var = quest_var->next;
                free(quest_var->var);
                free(quest_var->val);
                free(quest_var);
                quest_var = next_var;
            }
        }

        free(quest);
        quest = next_quest;
    }

    ch->quests = nullptr;
}

void free_quests() {
    int i;

    for (i = 0; i < max_quests; ++i)
        free(all_quests[i].quest_name);

    free(all_quests);
}

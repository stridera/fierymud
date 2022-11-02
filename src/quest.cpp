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
#include "math.hpp"
#include "races.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Local functions */
struct quest_list *quest_find_char(char_data *ch, char *qname);

/* Quest defines */
int max_quests = 0;
struct quest_info *all_quests = NULL;

void boot_quests() {
    FILE *fl;
    char line[256];
    char *quest_name;
    int quest_num, max_stages, num_records = 0;

    max_quests = 0;

    if ((fl = fopen(ALL_QUEST_FILE, "r")) == NULL) {
        fprintf(stderr, "Unable to find any quest data file (non-fatal)\r\n");
        return;
    }

    while (get_line(fl, line))
        ++num_records;

    fclose(fl);

    CREATE(all_quests, quest_info, num_records);

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
bool quest_stat(char_data *ch, char_data *vict, char *qname) {
    struct quest_list *quest = quest_find_char(vict, qname);
    struct quest_var_list *vars;
    int qid_num;

    if (!quest)
        return FALSE;

    if ((qid_num = real_quest(quest->quest_id)) >= 0) {
        sprintf(buf, "Quest %s: ", all_quests[qid_num].quest_name);
        if (quest->stage == QUEST_SUCCESS)
            strcat(buf, "Completed\r\n");
        else if (quest->stage == QUEST_FAILURE)
            strcat(buf, "Failed\r\n");
        else {
            sprintf(buf2, "Stage %d\r\n", quest->stage);
            strcat(buf, buf2);
        }
        send_to_char(buf, ch);
        for (vars = quest->variables; vars; vars = vars->next) {
            sprintf(buf, "\t%s = %s\r\n", vars->var, vars->val);
            send_to_char(buf, ch);
        }
    } else {
        sprintf(buf, "Invalid quest: %s\r\n", qname);
        send_to_char(buf, ch);
    }

    return TRUE;
}

void quest_mstat(char_data *ch, char_data *vict) {
    int qnum;
    bool any = FALSE;

    for (qnum = 0; qnum < max_quests; qnum++)
        if (quest_stat(ch, vict, all_quests[qnum].quest_name))
            any = TRUE;

    if (!any)
        send_to_char("No quest data found.\r\n", ch);
}

struct char_data *quest_id_char(trig_data *t, char_data *ch, char *name) {
    struct char_data *vict;

    if (!(vict = find_char_for_keyword(ch, name)))
        vict = find_char_in_world(find_by_name(name));
    if (vict && IS_NPC(vict))
        vict = NULL;
    if (!vict) {
        if (t)
            mprintf(L_WARN, LVL_GOD,
                    "QUEST ERROR: quest command couldn't "
                    "find player %s in trigger %d",
                    name, GET_TRIG_VNUM(t));
        else if (ch)
            send_to_char("There's no player by that name here.\r\n", ch);
        return NULL;
    }
    return vict;
}

ACMD(do_qstat) {
    struct char_data *vict;
    char *quest_name;

    argument = any_one_arg(argument, arg);  /* player name */
    argument = any_one_arg(argument, buf1); /* quest */

    if (!*arg) {
        send_to_char("Usage: qstat <player> [<quest>]\r\n", ch);
        return;
    }

    if (!(vict = quest_id_char(NULL, ch, arg)))
        return;

    if (*buf1) {
        /* Two arguments: a specific quest */

        /* Try and figure out which quest we want. */
        if (!(quest_name = check_quest_name(buf1))) {
            send_to_char("That is not a valid quest name.\r\n", ch);
            return;
        }
        if (!quest_stat(ch, vict, quest_name)) {
            send_to_char("No quest data was found.\r\n", ch);
        }
    } else {
        /* One argument: display all quests for this player */
        quest_mstat(ch, vict);
    }
}

void perform_quest(trig_data *t, char *argument, char_data *ch, obj_data *obj, room_data *room) {
    char error_string[MAX_INPUT_LENGTH * 2], *quest_name, e2[MAX_INPUT_LENGTH * 2];
    struct char_data *vict;
    int amount;

    skip_spaces(&argument);
    argument = any_one_arg(argument, arg);  /* command */
    argument = any_one_arg(argument, buf1); /* quest */
    argument = any_one_arg(argument, buf2); /* player name */

    /* Check for the "mstat" command first, since it requires one fewer argument
     * than the rest of them.  This one can only be done by a character (not a
     * trigger). */
    if (*arg && !strcmp(arg, "mstat") && ch) {
        if (*buf1) {
            /* OK: mstat command, buf1 = player name */
            if ((vict = quest_id_char(NULL, ch, buf1)))
                quest_mstat(ch, vict);
        } else {
            send_to_char("Usage: quest mstat <player>\r\n", ch);
        }
        return;
    }

    /* Make sure we have a quest command. */
    if (!*arg || !*buf1 || !*buf2) {
        if (t) {
            sprintf(buf,
                    "QUEST ERROR: quest command called with less than 3 args by "
                    "trigger %d",
                    GET_TRIG_VNUM(t));
            mudlog(buf, NRM, LVL_GOD, TRUE);
        } else if (ch)
            send_to_char("Usage: quest <command> <quest> <player> [<subclass>] [<var name> <var value>]\r\n", ch);
        return;
    }

    /* Try and figure out which quest we want. */
    if (!(quest_name = check_quest_name(buf1))) {
        if (t) {
            snprintf(buf, MAX_STRING_LENGTH,
                     "QUEST ERROR: quest command tried to access invalid quest %s in "
                     "trigger %d",
                     buf1, GET_TRIG_VNUM(t));
            mudlog(buf, NRM, LVL_GOD, TRUE);
        } else if (ch)
            send_to_char("That is not a valid quest name.\r\n", ch);
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
            sprintf(error_string,
                    "QUEST ERROR: %s (%d) tried to %%s "
                    "[%s on quest %s in trigger %d]",
                    GET_NAME(ch), GET_MOB_VNUM(ch), GET_NAME(vict), quest_name, GET_TRIG_VNUM(t));
        else
            sprintf(error_string,
                    "QUEST_ERROR: %s tried to %%s "
                    "[%s on quest %s]",
                    GET_NAME(ch), GET_NAME(vict), quest_name);
    }
    /* Object trigger error string */
    else if (obj) {
        sprintf(error_string,
                "QUEST ERROR: %s (%d) tried to %%s "
                "[%s on quest %s in trigger %d]",
                obj->short_description, GET_OBJ_VNUM(obj), GET_NAME(vict), quest_name, GET_TRIG_VNUM(t));
    }
    /* Room trigger error string */
    else if (room) {
        sprintf(error_string,
                "QUEST ERROR: %s (%d) tried to %%s "
                "[%s on quest %s in trigger %d]",
                room->name, room->vnum, GET_NAME(vict), quest_name, GET_TRIG_VNUM(t));
    }
    /* Other error string */
    else {
        sprintf(error_string,
                "QUEST ERROR: unknown actor tried to %%s "
                "[%s on quest %s in trigger %d]",
                GET_NAME(vict), quest_name, GET_TRIG_VNUM(t));
    }

    /*
     * This is kind of a hack.  The quest_ subfunctions below use the ch
     * argument to decide whether to log an error or just send_to_char.
     * If ch is NULL, then it logs.  So to make sure we get errors logged
     * for mob triggers, we set ch to NULL if there's a trigger.
     */
    if (t)
        ch = NULL;

    /* advance
     *
     * Advances player <vict> by <amount> stages in the quest <quest_name>.
     * Defaults to 1 stage.
     */
    if (!strncmp(arg, "advance", strlen(arg))) {
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
    else if (!strncmp(arg, "start", strlen(arg))) {
        argument = any_one_arg(argument, buf1);
        quest_start(ch, vict, quest_name, error_string, buf1);
    }

    /* variable
     *
     * Set quest variable <buf1> to value <buf2> on player <vict> on the
     * quest <quest_name>, or check quest variable <buf1> on player <vict>
     * on the quest <quest_name>.
     */
    else if (!strncmp(arg, "variable", strlen(arg))) {
        argument = any_one_arg(argument, buf1);
        argument = any_one_arg(argument, buf2);
        if (ch && !*buf1)
            send_to_char("Which variable?\r\n", ch);
        else if (ch && !*buf2) {
            sprintf(buf, "Variable %s on %s for quest %s: %s\r\n", buf1, GET_NAME(vict), quest_name,
                    get_quest_variable(vict, quest_name, buf1));
            send_to_char(buf, ch);
        } else if (*buf1 && *buf2)
            set_quest_variable(ch, vict, quest_name, error_string, buf1, buf2);
        else if (!*buf1) {
            sprintf(buf, error_string, "set variable with no variable");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        } else if (!*buf2) {
            sprintf(e2, "set variable \"%s\" with no value", buf1);
            sprintf(buf, error_string, e2);
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
    }

    /* complete
     *
     * Sets quest <quest_name> to complete on player <vict>.
     */
    else if (!strncmp(arg, "complete", strlen(arg)))
        quest_complete(ch, vict, quest_name, error_string);

    /* fail
     *
     * Sets quest <quest_name> to failed on player <vict>.
     */
    else if (!strncmp(arg, "fail", strlen(arg)))
        quest_fail(ch, vict, quest_name, error_string);

    /* rewind
     *
     * Rewinds player <vict> by <amount> stages in the quest <quest_name>.
     * Defaults to 1 stage.
     */
    else if (!strncmp(arg, "rewind", strlen(arg))) {
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
    else if (!strncmp(arg, "restart", strlen(arg)))
        quest_restart(ch, vict, quest_name, error_string);

    /* erase
     *
     * Erase quest <quest_name> on player <vict>.
     */
    else if (!strncmp(arg, "erase", strlen(arg)))
        quest_erase(ch, vict, quest_name, error_string);

    /* stage
     *
     * Shows stage for quest <quest_name> on player <vict>.
     */
    else if (ch && !strncmp(arg, "stage", strlen(arg))) {
        if (quest_find_char(vict, quest_name)) {
            amount = quest_stage(vict, quest_name);
            if (amount == QUEST_FAILURE)
                sprintf(buf, "%s has failed the %s quest.\r\n", GET_NAME(vict), quest_name);
            else if (amount == QUEST_SUCCESS)
                sprintf(buf, "%s has completed the %s quest.\r\n", GET_NAME(vict), quest_name);
            else
                sprintf(buf, "%s is on stage %d of the %s quest.\r\n", GET_NAME(vict), amount, quest_name);
        } else {
            sprintf(buf, "%s has not started the %s quest.\r\n", GET_NAME(vict), quest_name);
        }
        send_to_char(buf, ch);
    }

    /* stat
     *
     * Shows variable data for a quest
     */
    else if (ch && !strcmp(arg, "stat")) {
        if (!quest_stat(ch, vict, quest_name)) {
            send_to_char("No quest data was found.\r\n", ch);
        }
    }

    else {
        if (t) {
            sprintf(buf, "use an invalid command %s", arg);
            sprintf(buf1, error_string, buf);
            mudlog(buf1, NRM, LVL_GOD, TRUE);
        } else if (ch) {
            sprintf(buf, "Sorry, %s is not a valid quest command.\r\n", arg);
            send_to_char(buf, ch);
        }
    }
}

/*
 * quest_find_num
 *
 * descr:	internal routine to return the quest_id for  a specified quest
 * name so that all quests can be referred to by names externally.
 *
 * NOTE:	uses strn_cmp so abbreviations of quest anmes are tolerated.
 */
unsigned short quest_find_num(char *qname) {
    int count;
    /*
     * Loop through the quests and if we find the one we're looking for,
     * return its quest id.
     */
    for (count = 0; count < max_quests; count++) {
        if (!strn_cmp(all_quests[count].quest_name, qname, strlen(qname)))
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
        if (!strn_cmp(all_quests[count].quest_name, qname, strlen(qname)))
            return all_quests[count].quest_name;
    return NULL;
}

/*
 * quest_find_max_stage
 *
 * Internal routine to find the maximum stage for a quest.
 */
short quest_find_max_stage(char *qname) {
    int count = 0;

    for (count = 0; count < max_quests; count++)
        if (!strn_cmp(all_quests[count].quest_name, qname, strlen(qname)))
            return all_quests[count].maxstages;
    return 0;
}

/*
 * quest_find_char
 *
 * descr:	internal routine to find a quest in a quest_list
 */
struct quest_list *quest_find_char(char_data *ch, char *qname) {
    struct quest_list *quest;
    int quest_num = quest_find_num(qname);

    for (quest = ch->quests; quest; quest = quest->next)
        if (quest->quest_id == quest_num)
            return quest;
    return NULL;
}

/*
 * get_quest_variable
 */
char *get_quest_variable(char_data *vict, char *qname, char *variable) {
    struct quest_list *quest = quest_find_char(vict, qname);
    struct quest_var_list *vars;

    if (!quest)
        return "0";

    for (vars = quest->variables; vars; vars = vars->next)
        if (!strcmp(variable, vars->var))
            return vars->val;

    return "0";
}

void set_quest_variable(char_data *ch, char_data *vict, char *qname, char *error_string, char *variable, char *value) {
    struct quest_list *quest = quest_find_char(vict, qname);
    struct quest_var_list *vars;

    if (!quest) {
        if (ch) {
            sprintf(buf, "You can't set variables on a quest %s hasn't started yet.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else {
            sprintf(buf, error_string, "set variable on nonexistent quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    for (vars = quest->variables; vars; vars = vars->next)
        if (!strcmp(variable, vars->var)) {
            strncpy(vars->val, value, 20);
            return;
        }

    CREATE(vars, quest_var_list, 1);
    CREATE(vars->var, char, 21);
    strncpy(vars->var, variable, 20);
    vars->var[20] = '\0';
    CREATE(vars->val, char, 21);
    strncpy(vars->val, value, 20);
    vars->val[20] = '\0';
    vars->next = quest->variables;
    quest->variables = vars;

    if (ch) {
        sprintf(buf, "Set quest %s variable %s to '%s' on %s.\r\n", qname, variable, value, GET_NAME(vict));
        send_to_char(buf, ch);
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
int quest_stage(char_data *ch, char *qname) {
    struct quest_list *quest;

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
void quest_complete(char_data *ch, char_data *vict, char *qname, char *error_string) {
    struct quest_list *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            sprintf(buf, "%s hasn't started that quest yet.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "complete nonexistent quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            sprintf(buf, "%s has already completed that quest!\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "complete already-completed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            sprintf(buf, "%s has already failed that quest!\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "complete already-failed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    quest->stage = QUEST_SUCCESS;

    sprintf(buf, "Congratulations, you completed the %s quest!\r\n", qname);
    send_to_char(buf, vict);

    if (ch) {
        sprintf(buf, "Set the %s quest to &8completed&0 on %s.\r\n", qname, GET_NAME(vict));
        send_to_char(buf, ch);
    }
}

void quest_fail(char_data *ch, char_data *vict, char *qname, char *error_string) {
    struct quest_list *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            sprintf(buf, "%s hasn't started that quest yet.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "fail nonexistent quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            sprintf(buf, "%s has already completed that quest!\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "fail already-completed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            sprintf(buf, "%s has already failed that quest!\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "fail already-failed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    quest->stage = QUEST_FAILURE;

    if (ch) {
        sprintf(buf, "Set the %s quest to &8failed&0 on %s.\r\n", qname, GET_NAME(vict));
        send_to_char(buf, ch);
    }
}

/*
 * quest advance
 *
 */
void quest_advance(char_data *ch, char_data *vict, char *qname, char *error_string, int amount) {
    short max_stage;
    struct quest_list *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            sprintf(buf, "%s hasn't started that quest yet.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "advance nonexistent quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            sprintf(buf, "%s has already failed this quest.  Use quest restart.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "advance failed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            sprintf(buf, "%s has already completed this quest.  Use quest restart.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "advance already completed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    max_stage = quest_find_max_stage(qname);
    if (quest->stage + amount > max_stage) {
        quest->stage = max_stage;
        if (ch) {
            sprintf(buf,
                    "You can't advance past the quest's max stage.  %s's stage set "
                    "to max.\r\n",
                    GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, "advance past quest max stage of %d", max_stage);
            sprintf(buf1, error_string, buf);
            mudlog(buf1, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    /* Great success! */
    quest->stage += amount;

    if (ch) {
        sprintf(buf, "Advanced the %s quest's stage by %d to %d on %s.\r\n", qname, amount, quest->stage,
                GET_NAME(vict));
        send_to_char(buf, ch);
    }
}

/*
 * quest start
 *
 */
void quest_start(char_data *ch, char_data *vict, char *qname, char *error_string, char *subclass_abbr) {
    struct quest_list *quest = quest_find_char(vict, qname);
    extern int class_ok_race[NUM_RACES][NUM_CLASSES];
    unsigned short quest_num;
    int subclass = CLASS_UNDEFINED;

    if (quest) {
        if (ch) {
            sprintf(buf, "%s has already started this quest.  Use quest restart.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "start already-started quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    quest_num = quest_find_num(qname);

    if (!*subclass_abbr && IS_SUBCLASS_QUEST(quest_num)) {
        if (ch)
            send_to_char("You must supply the name of the subclass for this quest.\r\n", ch);
        else if (error_string) {
            sprintf(buf, error_string, "start subclass quest without specifying which");
            mudlog(buf, NRM, LVL_GOD, TRUE);
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
                    sprintf(buf,
                            "%s is already on a subclass quest.  Erase it before "
                            "starting another.\r\n",
                            GET_NAME(vict));
                    send_to_char(buf, ch);
                } else if (error_string) {
                    sprintf(buf, error_string, "start subclass quest with one already in progress");
                    mudlog(buf, NRM, LVL_GOD, TRUE);
                }
                return;
            }
            quest = quest->next;
        }

        subclass = parse_class(0, 0, subclass_abbr);

        if (subclass == CLASS_UNDEFINED) {
            if (ch)
                send_to_char("That's not a valid class.\r\n", ch);
            else if (error_string) {
                sprintf(buf, error_string, "start subclass quest with invalid class abbreviation");
                mudlog(buf, NRM, LVL_GOD, TRUE);
            }
            return;
        }

        /* Is the player a base class? */
        if (classes[(int)GET_CLASS(vict)].is_subclass) {
            if (ch) {
                sprintf(buf, "%s has already subclassed.\r\n", GET_NAME(vict));
                send_to_char(buf, ch);
            } else if (error_string) {
                sprintf(buf, error_string, "start subclass quest on already subclassed player");
                mudlog(buf, NRM, LVL_GOD, TRUE);
            }
            return;
        }

        /* Is the proposed class a subclass of the player's current class? */
        if (!classes[subclass].is_subclass || !classes[subclass].subclass_of == GET_CLASS(vict)) {
            if (ch) {
                sprintf(buf, "Invalid subclass for %s's class.\r\n", GET_NAME(vict));
                send_to_char(buf, ch);
            } else if (error_string) {
                sprintf(buf, error_string, "start subclass quest when player's class does not permit it");
                mudlog(buf, NRM, LVL_GOD, TRUE);
            }
            return;
        }

        /* Does the player's race permit this subclass? */
        if (!class_ok_race[(int)GET_RACE(vict)][subclass]) {
            if (ch) {
                sprintf(buf, "Invalid subclass for %s's race.\r\n", GET_NAME(vict));
                send_to_char(buf, ch);
            } else if (error_string) {
                sprintf(buf, error_string, "start subclass quest when player's race does not permit it");
                mudlog(buf, NRM, LVL_GOD, TRUE);
            }
            return;
        }

        /* You must be at least level 10 to start a subclass quest,
         * unless a deity hooks you up. */
        if (GET_LEVEL(vict) < 10 && !ch) {
            if (error_string) {
                sprintf(buf, error_string, "start subclass quest before player is level 10");
                mudlog(buf, NRM, LVL_GOD, TRUE);
            }
            return;
        }

        /* You must be less than the subclass quest max level to start the
         * quest, unless a deity hooks you up. */
        if (!ch && GET_LEVEL(vict) > classes[(int)GET_CLASS(vict)].max_subclass_level) {
            if (error_string) {
                sprintf(buf, error_string, "start subclass quest after player is past max subclass level");
                mudlog(buf, NRM, LVL_GOD, TRUE);
            }
            return;
        }

        /* You may begin the subclass quest. */
    }

    /* Okay, make the new quest */
    CREATE(quest, quest_list, 1);
    quest->quest_id = quest_num;
    quest->stage = QUEST_START;
    quest->next = vict->quests;
    vict->quests = quest;

    /* For subclass quests, store the name of the subclass in a quest variable. */
    if (subclass != CLASS_UNDEFINED) {
        set_quest_variable(ch, vict, qname, error_string, "subclass_name", classes[subclass].name);
    }

    if (ch) {
        sprintf(buf, "Started the %s quest on %s and set stage to 1.\r\n", qname, GET_NAME(vict));
        send_to_char(buf, ch);
    }
}

void quest_rewind(char_data *ch, char_data *vict, char *qname, char *error_string, int amount) {
    struct quest_list *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            sprintf(buf, "%s hasn't started that quest yet.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "rewind nonexistent quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_FAILURE) {
        if (ch) {
            sprintf(buf, "%s has already failed this quest.  Use quest restart.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "rewind failed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage == QUEST_SUCCESS) {
        if (ch) {
            sprintf(buf, "%s has already completed this quest.  Use quest restart.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "rewind already completed quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    if (quest->stage - amount < 1) {
        quest->stage = 1;
        if (ch) {
            sprintf(buf, "You can't rewind past the first stage.  %s's stage set to 1.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "rewind past quest first stage");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    /* Great success! */
    quest->stage -= amount;

    if (ch) {
        sprintf(buf, "Rewound the %s quest's stage by %d to %d on %s.\r\n", qname, amount, quest->stage,
                GET_NAME(vict));
        send_to_char(buf, ch);
    }
}

void quest_restart(char_data *ch, char_data *vict, char *qname, char *error_string) {
    struct quest_list *quest = quest_find_char(vict, qname);

    if (!quest) {
        if (ch) {
            sprintf(buf, "%s hasn't started that quest yet.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "restart nonexistent quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
        }
        return;
    }

    quest->stage = QUEST_START;

    if (ch) {
        sprintf(buf, "Restarted the %s quest on %s by resetting stage to 1.\r\n", qname, GET_NAME(vict));
        send_to_char(buf, ch);
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
void quest_erase(char_data *ch, char_data *vict, char *qname, char *error_string) {
    struct quest_list *quest, *prev;
    /*  struct quest_var_list *vars, *next_vars; */
    unsigned short quest_num;

    if (!vict->quests) {
        if (ch) {
            sprintf(buf, "%s hasn't started that quest yet.\r\n", GET_NAME(vict));
            send_to_char(buf, ch);
        } else if (error_string) {
            sprintf(buf, error_string, "erase nonexistent quest");
            mudlog(buf, NRM, LVL_GOD, TRUE);
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
            sprintf(buf, "Erased the %s quest from %s.\r\n", qname, GET_NAME(vict));
            send_to_char(buf, ch);
        }
        return;
    }

    if (ch) {
        sprintf(buf, "%s hasn't started that quest yet.\r\n", GET_NAME(vict));
        send_to_char(buf, ch);
    } else if (error_string) {
        sprintf(buf, error_string, "erase nonexistent quest");
        mudlog(buf, NRM, LVL_GOD, TRUE);
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
int has_failed_quest(char *qname, char_data *ch) {
    struct quest_list *quest = quest_find_char(ch, qname);

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
int has_completed_quest(char *qname, char_data *ch) {
    struct quest_list *quest = quest_find_char(ch, qname);

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
        send_to_char("Usage: qadd <quest_name> <max_stages> [<yes_if_subclass_quest>]\r\n", ch);
        return;
    }

    if (strlen(buf1) >= MAX_QNAME_LEN) {
        sprintf(buf, "The quest name may not be longer than %d characters.\r\n", MAX_QNAME_LEN - 1);
        send_to_char(buf, ch);
        return;
    }

    /* Can't create a new quest with the same name as another one! */
    if (quest_find_num(buf1)) {
        sprintf(buf, "There's already a quest named %s!\r\n", buf1);
        send_to_char(buf, ch);
        return;
    }

    /* Check and see if this is going to be a subclass quest.  Start the
     * new_id appropriately if so.  We have to check to make sure arg is
     * longer than 0, otherwise all quests are subclass quests.
     */
    argument = any_one_arg(argument, arg);
    if (*arg && !strn_cmp("yes", arg, strlen(arg))) {
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
        send_to_char("No free quest IDs are left.  Remove some old ones first.\r\n", ch);
        return;
    }

    if (!(stages = atoi(buf2))) {
        send_to_char("You must specify an integer for the maximum number of stages.\r\n", ch);
        return;
    }
    if (real_quest(new_id) != NOWHERE) {
        send_to_char("Unable to allocate a quest ID for the new quest.\r\n", ch);
        return;
    }

    /*
     * rewrite the quest file and add this one in the correct place
     */
    if ((fp = fopen(ALL_QUEST_FILE, "w")) == NULL) {
        log("Unable to open ALL_QUEST_FILE in qadd\r\n");
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
        sprintf(buf, "(GC) %s created a new subclass quest %s.", GET_NAME(ch), buf1);
        sprintf(buf2, "New &8subclass&0 quest %s successfully added.\r\n", buf1);
    } else {
        sprintf(buf, "(GC) %s created a new quest %s.", GET_NAME(ch), buf1);
        sprintf(buf2, "New quest %s successfully added.\r\n", buf1);
    }
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
    send_to_char(buf2, ch);
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
        send_to_char("Usage: qdel <quest_name>\r\n", ch);
        return;
    }

    /*
     * Instead of using quest_find_num, we'll search the quest index
     * ourselves, because quest_find_num allows abbreviations, and I
     * think we should be a little more careful than that when deleting.
     */
    for (i = 0; i < max_quests; i++)
        if (!str_cmp(all_quests[i].quest_name, buf1)) {
            quest_num = all_quests[i].quest_id;
            break;
        }

    /* We didn't find the quest. */
    if (!quest_num) {
        sprintf(buf, "There is no quest named %s.\r\n", buf1);
        send_to_char(buf, ch);
        return;
    }

    /* Rewrite the quest file and drop this one. */
    if (!(fp = fopen(ALL_QUEST_FILE, "w"))) {
        log("Unable to open ALL_QUEST_FILE in qdel.\r\n");
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
        sprintf(buf, "(GC) %s deleted subclass quest %s.", GET_NAME(ch), buf1);
        sprintf(buf2, "Subclass quest %s successfully deleted.\r\n", buf1);
    } else {
        sprintf(buf, "(GC) %s deleted quest %s.", GET_NAME(ch), buf1);
        sprintf(buf2, "Quest %s successfully deleted.\r\n", buf1);
    }
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
    send_to_char(buf2, ch);
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

    sprintf(msgbuf, "There are %d quests:\r\n====================\r\n", max_quests);
    send_to_char(msgbuf, ch);
    sprintf(msgbuf, "%35s %6s %15s\r\n", "Quest Name", "Stages", "Quest Type");
    send_to_char(msgbuf, ch);

    for (i = 0; i < max_quests; i++) {
        sprintf(intbuf, "%d", all_quests[i].maxstages);
        sprintf(msgbuf, "%35s %6s %15s\r\n", all_quests[i].quest_name, intbuf,
                (IS_SUBCLASS_QUEST(all_quests[i].quest_id)) ? "Subclass Quest" : "Regular Quest");
        send_to_char(msgbuf, ch);
    }
}

void free_quest_list(char_data *ch) {
    struct quest_list *quest, *next_quest;

    quest = ch->quests;
    while (quest) {
        next_quest = quest->next;
        if (quest->variables) {
            struct quest_var_list *quest_var;
            struct quest_var_list *next_var;

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

    ch->quests = NULL;
}

void free_quests() {
    int i;

    for (i = 0; i < max_quests; ++i)
        free(all_quests[i].quest_name);

    free(all_quests);
}

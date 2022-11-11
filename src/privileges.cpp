/***************************************************************************
 *   File: privileges.c                                   Part of FieryMUD *
 *  Usage: utilities to facilitate the privileges system                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#define __FIERY_PRIVILEGES_C

#include "privileges.hpp"

#include "clan.hpp"
#include "comm.hpp"
#include "commands.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

struct privflagdef prv_flags[NUM_PRV_FLAGS] = {
    {"clan admin", LVL_ADMIN, clan_admin_check},
    {"title", LVL_GAMEMASTER, nullptr},
    {"anon toggle", LVL_ATTENDANT, nullptr},
    {"auto gain", LVL_ATTENDANT, nullptr},
};

static GrantType *grant_in_list(GrantType *list, int grant) {
    while (list) {
        if (list->grant == grant)
            break;
        list = list->next;
    }

    return list;
}

static bool remove_grant(GrantType **list, int grant) {
    GrantType dummy, *temp, *node;

    if (!list)
        return false;

    dummy.next = *list;
    node = &dummy;

    while (node->next) {
        if (node->next->grant == grant) {
            temp = node->next;
            node->next = node->next->next;
            free(temp->grantor);
            free(temp);
            *list = dummy.next;
            return true;
        }
        node = node->next;
    }

    return false;
}

static void add_grant(GrantType **list, int grant, const char *grantor, int grant_level) {
    GrantType *temp;
    CREATE(temp, GrantType, 1);
    temp->grant = grant;
    temp->grantor = strdup(grantor);
    temp->level = grant_level;
    temp->next = *list;
    *list = temp;
}

/*
 * Precondition:
 */
void cache_grants(CharData *ch) {
    int cmd;

    if (!GET_GRANT_CACHE(ch))
        CREATE(GET_GRANT_CACHE(ch), flagvector, FLAGVECTOR_SIZE(num_of_cmds));
    if (!GET_REVOKE_CACHE(ch))
        CREATE(GET_REVOKE_CACHE(ch), flagvector, FLAGVECTOR_SIZE(num_of_cmds));

    for (cmd = 1; cmd < num_of_cmds; ++cmd) {
        switch (command_grant_usability(ch, cmd)) {
        case CMD_GRANTED:
            SET_FLAG(GET_GRANT_CACHE(ch), cmd);
            break;
        case CMD_REVOKED:
            SET_FLAG(GET_REVOKE_CACHE(ch), cmd);
            break;
        case CMD_NOT_GRANTED:
            /* do nothing */
            break;
        }
    }
}

int command_grant_usability(CharData *ch, int cmd) {
    int usability = CMD_NOT_GRANTED, i;

    /* If we are in a command group with the command, we can use it */
    if (grp_info[cmd].groups)
        for (i = 0; grp_info[cmd].groups[i] >= 0; ++i)
            if (grant_in_list(GET_GRANT_GROUPS(ch), grp_info[cmd].groups[i]))
                usability = CMD_GRANTED;

    /* If the command is explicitly revoked, we can't use it.  Period. */
    if (GET_REVOKES(ch) && grant_in_list(GET_REVOKES(ch), cmd))
        return CMD_REVOKED;

    /* If we're revoked the group, we can't use it unless explicitly
     * granted this command */
    if (grp_info[cmd].groups)
        for (i = 0; grp_info[cmd].groups[i] >= 0; ++i)
            if (grant_in_list(GET_REVOKE_GROUPS(ch), grp_info[cmd].groups[i]))
                usability = CMD_REVOKED;

    /* If we're granted the command, we can use it */
    if (GET_GRANTS(ch) && grant_in_list(GET_GRANTS(ch), cmd))
        usability = CMD_GRANTED;

    return usability;
}

static bool can_grant_group(CharData *ch, int group) {
    if (group < 0 && group >= num_cmd_groups)
        return false;
    else if (grant_in_list(GET_REVOKE_GROUPS(ch), group))
        return false;
    else if (grant_in_list(GET_GRANT_GROUPS(ch), group))
        return true;
    else
        return (cmd_groups[group].minimum_level <= GET_LEVEL(ch));
}

static void cache_grant(flagvector *cache, int cmd, bool is_group, bool set) {
    if (is_group) {
        if (cmd < num_cmd_groups) {
            int group = cmd, grp;
            for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
                if (grp_info[cmd].groups)
                    for (grp = 0; grp_info[cmd].groups[grp] >= 0; ++grp)
                        if (grp_info[cmd].groups[grp] == group) {
                            if (set)
                                SET_FLAG(cache, cmd);
                            else
                                REMOVE_FLAG(cache, cmd);
                            break;
                        }
        }
    }

    else if (cmd < num_of_cmds) {
        if (set)
            SET_FLAG(cache, cmd);
        else
            REMOVE_FLAG(cache, cmd);
    }
}

static void str_cat_command_grant_list(char *buf, GrantType *grant) {
    for (; grant; grant = grant->next)
        str_catf(buf, "  CMD %-20s %s (%d)\r\n", cmd_info[grant->grant].command, grant->grantor, grant->level);
}

static void str_cat_grant_group_list(char *buf, GrantType *grant) {
    for (; grant; grant = grant->next)
        str_catf(buf, "  GRP %-20s %s (%d)\r\n", cmd_groups[grant->grant].alias, grant->grantor, grant->level);
}

static void do_list_grants(CharData *ch, CharData *vict) {
    int found = 0;
    unsigned int priv;

    str_start(buf, sizeof(buf));

    str_catf(buf, "%s's grants:\r\n", GET_NAME(vict));

    if (GET_GRANTS(vict)) {
        found = 1;
        str_cat_command_grant_list(buf, GET_GRANTS(vict));
    }

    if (GET_GRANT_GROUPS(vict)) {
        found = 1;
        str_cat_grant_group_list(buf, GET_GRANT_GROUPS(vict));
    }

    for (priv = 0; priv < NUM_PRV_FLAGS; ++priv)
        if (PRV_FLAGGED(vict, priv)) {
            found = 1;
            str_catf(buf, "  FLG %c%-19s (%d)\r\n", UPPER(*prv_flags[priv].desc), prv_flags[priv].desc + 1,
                     prv_flags[priv].level);
        }

    if (!found)
        str_cat(buf, "  None!\r\n");

    found = 0;
    str_catf(buf, "%s's revocations:\r\n", GET_NAME(vict));

    if (GET_REVOKES(vict)) {
        found = 1;
        str_cat_command_grant_list(buf, GET_REVOKES(vict));
    }

    if (GET_REVOKE_GROUPS(vict)) {
        found = 1;
        str_cat_grant_group_list(buf, GET_REVOKE_GROUPS(vict));
    }

    if (!found)
        str_cat(buf, "  None!\r\n");

    page_string(ch, buf);
}

static void send_grant_usage(CharData *ch) {
    send_to_char(
        "Usage: grant <name> [ command <command> | group <group> ] [ level ]\r\n"
        "       grant <name> flag <flag>\r\n"
        "       revoke <name> [ command <command> | group <group> ] [ level ]\r\n"
        "       revoke <name> flag <flag>\r\n"
        "       ungrant <name> [ command <command> | group <group> ]\r\n"
        "       grant <name> [ clear | list ]\r\n",
        ch);
}

#define GRANT_COMMAND 0
#define GRANT_GROUP 1

static void do_command_grant_revoke(CharData *ch, CharData *vict, char *argument, int subcmd, int type) {
    GrantType **list, **unlist, **temp;
    GrantType *grant;
    flagvector *cache, *uncache;
    CommandGroup *group;
    int level, command = 0;
    const char *past_action, *preposition;
    char name[MAX_INPUT_LENGTH];
    bool is_group;

    argument = any_one_arg(argument, arg);
    skip_spaces(&argument);
    level = *argument && is_number(argument) ? atoi(argument) : GET_LEVEL(ch);

    if ((subcmd != SCMD_GRANT && subcmd != SCMD_REVOKE && subcmd != SCMD_UNGRANT) ||
        (type != GRANT_COMMAND && type != GRANT_GROUP)) {
        send_to_char("Warning: grant command incorrectly invoked.\r\n", ch);
        log("SYSERR: do_grant incorrectly invoked");
        return;
    } else if (type == GRANT_COMMAND) {
        list = &GET_GRANTS(vict);
        unlist = &GET_REVOKES(vict);
        is_group = false;
    } else {
        list = &GET_GRANT_GROUPS(vict);
        unlist = &GET_REVOKE_GROUPS(vict);
        is_group = true;
    }
    if (subcmd == SCMD_REVOKE) {
        temp = list;
        list = unlist;
        unlist = temp;
        past_action = "revoked";
        preposition = "from";
        cache = GET_REVOKE_CACHE(vict);
        uncache = GET_GRANT_CACHE(vict);
    } else {
        past_action = "granted";
        preposition = "to";
        cache = GET_GRANT_CACHE(vict);
        uncache = GET_REVOKE_CACHE(vict);
    }

    if (ch == vict) {
        send_to_char("You cannot grant or revoke your own commands.\r\n", ch);
        return;
    }
    if (!*arg) {
        send_grant_usage(ch);
        return;
    }

    if (type == GRANT_COMMAND && ((command = parse_command(arg)) <= 0 || !strcpy(name, arg))) {
        send_to_char("No such command.\r\n", ch);
        list_similar_commands(ch, arg);
        return;
    }

    else if (type == GRANT_GROUP && ((command = find_command_group(arg)) < 0 || !sprintf(name, "the %s group", arg))) {
        int found = 0;
        str_start(buf, sizeof(buf));
        str_cat(buf, "No such command group.");
        if (cmd_groups) {
            str_cat(buf, "  Possible groups:\r\n");
            for (group = cmd_groups; group < top_of_cmd_groups; ++group)
                str_catf(buf, "%s%-15s", found++ % 4 == 0 ? "\r\n" : "", group->alias);
        }
        str_cat(buf, "\r\n");
        send_to_char(buf, ch);
        return;
    }

    else if (type == GRANT_COMMAND && !can_use_command(ch, command))
        send_to_char("You cannot grant or revoke a command you yourself cannot use.\r\n", ch);
    else if (type == GRANT_GROUP && !can_grant_group(ch, command))
        send_to_char("You cannot grant or revoke a group you yourself cannot use.\r\n", ch);
    else if (subcmd != SCMD_UNGRANT && grant_in_list(*list, command))
        cprintf(ch, "%s has already has %s %s.\r\n", GET_NAME(vict), arg, past_action);
    else if (((grant = grant_in_list(*unlist, command)) ||
              (subcmd == SCMD_UNGRANT && (grant = grant_in_list(*list, command)))) &&
             grant->level > GET_LEVEL(ch))
        cprintf(ch,
                "You cannot change %s's access to %s, because %s (level %d) "
                "granted or revoked it.\r\n",
                GET_NAME(vict), arg, grant->grantor, grant->level);
    else if (subcmd == SCMD_UNGRANT) {
        remove_grant(unlist, command);
        cache_grant(uncache, command, is_group, false);
        remove_grant(list, command);
        cache_grant(cache, command, is_group, false);
        cprintf(ch, "Revoked all grants on %s for %s.\r\n", GET_NAME(vict), arg);
    } else {
        remove_grant(unlist, command);
        cache_grant(uncache, command, is_group, false);
        add_grant(list, command, GET_NAME(ch), level);
        cache_grant(cache, command, is_group, true);
        cprintf(ch, "%c%s %s %s %s at level %d.\r\n", UPPER(*past_action), past_action + 1, arg, preposition,
                GET_NAME(vict), level);
    }
}

static int clear_grant_list(CharData *ch, GrantType **list) {
    GrantType *node, *next;
    int count = 0;

    if (!list)
        return count;

    for (node = *list; node; node = next) {
        next = node->next;
        if (node->level <= GET_LEVEL(ch)) {
            free(node->grantor);
            free(node);
            ++count;
        }
    }

    *list = nullptr;

    return count;
}

static void do_clear_grants(CharData *ch, CharData *vict) {
    int count = 0;
    count += clear_grant_list(ch, &GET_GRANTS(vict));
    count += clear_grant_list(ch, &GET_REVOKES(vict));
    count += clear_grant_list(ch, &GET_GRANT_GROUPS(vict));
    count += clear_grant_list(ch, &GET_REVOKE_GROUPS(vict));
    CLEAR_FLAGS(GET_GRANT_CACHE(vict), num_of_cmds);
    CLEAR_FLAGS(GET_REVOKE_CACHE(vict), num_of_cmds);
    CLEAR_FLAGS(PRV_FLAGS(vict), NUM_PRV_FLAGS);
    cprintf(ch, "%d grant%s cleared.\r\n", count, count == 1 ? "" : "s");
}

static void do_flag_grant_revoke(CharData *ch, CharData *vict, char *argument, int subcmd) {
    int flag;

    any_one_arg(argument, arg);
    flag = search_block(arg, privilege_bits, false);

    if (!*arg)
        send_grant_usage(ch);
    else if (flag < 0) {
        cprintf(ch, "'%s' is not a valid privilege flag.  Try one of:\r\n", arg);
        for (flag = 0; flag < NUM_PRV_FLAGS; ++flag)
            cprintf(ch, "%-15s %-20s %d\r\n", privilege_bits[flag], prv_flags[flag].desc, prv_flags[flag].level);
    } else if (prv_flags[flag].level > GET_LEVEL(ch))
        cprintf(ch, "You don't have the ability to set that flag.\r\n");
    else if (subcmd == SCMD_GRANT) {
        SET_FLAG(PRV_FLAGS(vict), flag);
        if (prv_flags[flag].update_func)
            (prv_flags[flag].update_func)(vict, flag);
        cprintf(ch, "Granted %s to %s at level %d.\r\n", prv_flags[flag].desc, GET_NAME(vict), prv_flags[flag].level);
    } else {
        REMOVE_FLAG(PRV_FLAGS(vict), flag);
        if (prv_flags[flag].update_func)
            (prv_flags[flag].update_func)(vict, flag);
        cprintf(ch, "Ungranted %s from %s at level %d.\r\n", prv_flags[flag].desc, GET_NAME(vict),
                prv_flags[flag].level);
    }
}

ACMD(do_grant) {
    CharData *vict;

    argument = any_one_arg(argument, arg);
    if (!*arg) {
        send_grant_usage(ch);
        return;
    }

    if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        send_to_char(NOPERSON, ch);
        return;
    }

    argument = any_one_arg(argument, arg);
    if (subcmd == SCMD_GRANT && GET_LEVEL(ch) >= GET_LEVEL(vict) && (!*arg || is_abbrev(arg, "list")))
        do_list_grants(ch, vict);
    else if (ch != vict && GET_LEVEL(ch) <= GET_LEVEL(vict))
        act("You cannot grant or revoke $N's commands.", false, ch, 0, vict, TO_CHAR);
    else if (is_abbrev(arg, "command"))
        do_command_grant_revoke(ch, vict, argument, subcmd, GRANT_COMMAND);
    else if (is_abbrev(arg, "group"))
        do_command_grant_revoke(ch, vict, argument, subcmd, GRANT_GROUP);
    else if (is_abbrev(arg, "flag"))
        do_flag_grant_revoke(ch, vict, argument, subcmd);
    else if (subcmd == SCMD_REVOKE)
        send_grant_usage(ch);
    else if (is_abbrev(arg, "clear"))
        do_clear_grants(ch, vict);
    else
        send_grant_usage(ch);
}

void write_player_grants(FILE *fl, CharData *ch) {
    GrantType *grant;

    if (GET_GRANTS(ch)) {
        fprintf(fl, "grants:\n");
        for (grant = GET_GRANTS(ch); grant; grant = grant->next)
            fprintf(fl, "%s %s %d\n", cmd_info[grant->grant].command, grant->grantor, grant->level);
        fprintf(fl, "~\n");
    }

    if (GET_REVOKES(ch)) {
        fprintf(fl, "revokes:\n");
        for (grant = GET_REVOKES(ch); grant; grant = grant->next)
            fprintf(fl, "%s %s %d\n", cmd_info[grant->grant].command, grant->grantor, grant->level);
        fprintf(fl, "~\n");
    }

    if (GET_GRANT_GROUPS(ch)) {
        fprintf(fl, "grantgroups:\n");
        for (grant = GET_GRANT_GROUPS(ch); grant; grant = grant->next)
            fprintf(fl, "%s %s %d\n", cmd_groups[grant->grant].alias, grant->grantor, grant->level);
        fprintf(fl, "~\n");
    }

    if (GET_REVOKE_GROUPS(ch)) {
        fprintf(fl, "revokegroups:\n");
        for (grant = GET_REVOKE_GROUPS(ch); grant; grant = grant->next)
            fprintf(fl, "%s %s %d\n", cmd_groups[grant->grant].alias, grant->grantor, grant->level);
        fprintf(fl, "~\n");
    }
}

static void read_player_grant_list(FILE *fl, GrantType **list, int (*cmd_lookup)(char *name)) {
    char line[MAX_INPUT_LENGTH + 1], *ptr;
    GrantType *grant;
    int line_fail;

    while (true) {
        get_line(fl, line);
        if (*line == '~')
            return;
        else if (strchr(line, ':')) {
            sprintf(buf,
                    "SYSERR: read_player_grants: invalid command/group or grantor "
                    "(err 1): %s",
                    line);
            mudlog(buf, BRF, LVL_IMMORT, true);
            return;
        }
        line_fail = 0;
        CREATE(grant, GrantType, 1);
        ptr = strchr(line, ' ');
        if (ptr) {
            *(ptr++) = '\0';
            if ((grant->grant = (*cmd_lookup)(line)) >= 0) {
                grant->grantor = ptr;
                ptr = strchr(ptr, ' ');
                if (ptr) {
                    *(ptr++) = '\0';
                    grant->grantor = strdup(grant->grantor);
                    grant->level = LIMIT(0, atoi(ptr), LVL_IMPL);
                } else
                    line_fail = 3;
            } else
                line_fail = 2;
        } else
            line_fail = 1;
        if (!line_fail) {
            grant->next = *list;
            *list = grant;
        } else {
            sprintf(buf,
                    "SYSERR: read_player_grants: invalid command/group or grantor "
                    "(err 2.%d): %s/%s",
                    line_fail, line, ptr ? ptr : "(null)");
            mudlog(buf, BRF, LVL_IMMORT, true);
            free(grant);
        }
    }
}

void read_player_grants(FILE *fl, GrantType **list) { read_player_grant_list(fl, list, find_command); }

void read_player_grant_groups(FILE *fl, GrantType **list) { read_player_grant_list(fl, list, find_command_group); }

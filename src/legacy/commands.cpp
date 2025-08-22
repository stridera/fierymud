/***************************************************************************
 *   File: commands.c                                     Part of FieryMUD *
 *  Usage: helpers and utilities for player commands and command groups    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#define __FIERY_COMMANDS_C

#include "commands.hpp"

#include "bitflags.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <fmt/format.h>

/*
 * Private globals
 */
/* The dynamically allocated array of command groups. */

CommandGroup *cmd_groups;
int num_cmd_groups;
CommandGroupInfo *grp_info;

/*
 * Private interface
 */
#define VALID_GROUP_NUM(gg) ((gg) >= cmd_groups && (gg) < top_of_cmd_groups)
#define GROUP_NUM(gg) ((gg) - cmd_groups)
static void gedit_setup_existing(DescriptorData *d, int group);
static void gedit_setup_new(DescriptorData *d);
static void gedit_save_internally(DescriptorData *d);
static void gedit_save_to_disk();

int command_group_number(CommandGroup *group) { return VALID_GROUP_NUM(group) ? GROUP_NUM(group) : -1; }

int find_command_group(const char *name) {
    CommandGroup *group = cmd_groups;

    while (group < top_of_cmd_groups) {
        if (!strcasecmp(group->alias, name))
            return GROUP_NUM(group);
        ++group;
    }

    return (-1);
}

static void free_command_groups() {
    CommandGroup *group;
    int cmd;

    for (group = cmd_groups; group < top_of_cmd_groups; ++group) {
        if (group->alias)
            free(group->alias);
        if (group->name)
            free(group->name);
        if (group->description)
            free(group->description);
    }
    if (cmd_groups)
        free(cmd_groups);
    cmd_groups = nullptr;
    num_cmd_groups = 0;

    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
        if (grp_info[cmd].groups) {
            free(grp_info[cmd].groups);
            grp_info[cmd].groups = nullptr;
        }
}

bool can_use_command(CharData *ch, int cmd) {
    /* NPCs can't have grants */
    if (cmd < 0 || cmd >= num_of_cmds)
        return false;
    else if (IS_NPC(ch))
        return CMD_USEABLE_FOR_LEVEL(ch, cmd);
    else if (IS_FLAGGED(GET_GRANT_CACHE(ch), cmd))
        return true;
    else if (IS_FLAGGED(GET_REVOKE_CACHE(ch), cmd))
        return false;
    else
        return CMD_USEABLE_FOR_LEVEL(ch, cmd);
}

ACMD(do_gedit) {
    int group;
    DescriptorData *d;

    if (IS_NPC(ch) || !ch->desc)
        return;

    /* Edit which command group? */
    argument = any_one_arg(argument, arg);
    if (!*arg) {
        char_printf(ch, "Specify a command group to edit.\n");
        return;
    }

    /* Make sure it exists, or create a new one */
    if (!strcasecmp(arg, "new"))
        group = -1;
    else if (!strcasecmp(arg, "save")) {
        char_printf(ch, "Saving all command groups.\n");
        log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "OLC: {} saves command groups.",
            GET_NAME(ch));
        gedit_save_to_disk();
        return;
    } else if ((group = find_command_group(arg)) < 0) {
        char_printf(ch, "Command group '{}' not found.  Use 'gedit new' to create it.\n", arg);
        return;
    }

    /* Make sure it's not being edited already */
    if (group >= 0)
        for (d = descriptor_list; d; d = d->next)
            if (d->connected == CON_GEDIT && d->olc && OLC_NUM(d) == group) {
                char_printf(ch, "That command group is currently being edited by {}.\n", PERS(ch, d->character));
                return;
            }
    d = ch->desc;

    /* Give descriptor an OLC structure */
    CREATE(d->olc, OLCData, 1);

    if (group >= 0)
        gedit_setup_existing(d, group);
    else
        gedit_setup_new(d);
    STATE(d) = CON_GEDIT;

    act("$n starts using OLC.", true, d->character, 0, 0, TO_ROOM);
    SET_FLAG(PLR_FLAGS(ch), PLR_WRITING);
}

static void gedit_setup_existing(DescriptorData *d, int group) {
    int cmd, grp, count = 0;

    OLC_NUM(d) = group;

    CREATE(OLC_GROUP(d), OLCCommandGroup, 1);

    OLC_GROUP(d)->alias = strdup(cmd_groups[group].alias);
    OLC_GROUP(d)->name = strdup(cmd_groups[group].name);
    OLC_GROUP(d)->description = strdup(cmd_groups[group].description);
    OLC_GROUP(d)->commands = nullptr;

    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
        if (grp_info[cmd].groups)
            for (grp = 0; grp_info[cmd].groups[grp] >= 0; ++grp)
                if (grp_info[cmd].groups[grp] == group)
                    ++count;

    if (count) {
        CREATE(OLC_GROUP(d)->commands, int, count + 1);

        count = 0;

        for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
            if (grp_info[cmd].groups)
                for (grp = 0; grp_info[cmd].groups[grp] >= 0; ++grp)
                    if (grp_info[cmd].groups[grp] == group)
                        OLC_GROUP(d)->commands[count++] = cmd;
    }

    gedit_disp_menu(d);
    OLC_VAL(d) = 0;
}

static void gedit_setup_new(DescriptorData *d) {
    OLC_NUM(d) = -1;

    CREATE(OLC_GROUP(d), OLCCommandGroup, 1);

    OLC_GROUP(d)->alias = strdup("newgroup");
    OLC_GROUP(d)->name = strdup("New Command Group");
    OLC_GROUP(d)->description = strdup("A new command group.\n");
    OLC_GROUP(d)->commands = nullptr;

    gedit_disp_menu(d);
    OLC_VAL(d) = 0;
}

void gedit_parse(DescriptorData *d, char *arg) {
    int num, i;

    switch (OLC_MODE(d)) {
    case GEDIT_MAIN_MENU:
        switch (to_lower(*arg)) {
        case '1':
            string_to_output(d, "Enter new group alias:\n");
            OLC_MODE(d) = GEDIT_ALIAS;
            break;
        case '2':
            string_to_output(d, "Enter new group name:\n");
            OLC_MODE(d) = GEDIT_NAME;
            break;
        case '3':
            string_to_output(d, "Enter new group description (/s saves /h for help):\n\n");
            OLC_MODE(d) = GEDIT_DESCRIPTION;
            string_write(d, &OLC_GROUP(d)->description, MAX_STRING_LENGTH);
            break;
        case '4':
            string_to_output(d, "Enter minimum level for admin access to group:\n");
            OLC_MODE(d) = GEDIT_LEVEL;
            break;
        case 'a':
            string_to_output(d, "Command to add to group:\n");
            OLC_MODE(d) = GEDIT_ADD_COMMAND;
            break;
        case 'r':
            string_to_output(d, "Command to remove from group:\n");
            OLC_MODE(d) = GEDIT_REMOVE_COMMAND;
            break;
        case 'c':
            string_to_output(d, "All commands cleared.\n");
            free(OLC_GROUP(d)->commands);
            OLC_GROUP(d)->commands = nullptr;
            OLC_VAL(d) = 1;
            gedit_disp_menu(d);
            break;
        case 'q':
            for (i = 0; i < num_cmd_groups; ++i)
                if (i != OLC_NUM(d) && !strcasecmp(cmd_groups[i].alias, OLC_GROUP(d)->alias)) {
                    string_to_output(d,
                                     "This command group has the same alias as another existing group.\n"
                                     "It cannot be saved until the alias is changed.\n");
                    return;
                }
            if (OLC_VAL(d)) {
                string_to_output(d, "Do you wish to save the changes to the command group? (y/n)\n");
                OLC_MODE(d) = GEDIT_CONFIRM_SAVE;
            } else {
                string_to_output(d, "No changes made.\n");
                cleanup_olc(d, CLEANUP_ALL);
            }
        }
        return;
    case GEDIT_ALIAS:
        skip_spaces(&arg);
        if (!*arg) {
            string_to_output(d,
                             "Group alias must consist of at least one letter or number.\n"
                             "Try again: ");
            return;
        }
        for (i = 0; arg[i]; ++i)
            if (!isalpha(arg[i]) && !isdigit(arg[i])) {
                string_to_output(d,
                                 "Group alias may only consist of letters and numbers.\n"
                                 "Try again: ");
                return;
            }
        if (OLC_GROUP(d)->alias)
            free(OLC_GROUP(d)->alias);
        else
            log("SYSERR: OLC: GEDIT_ALIAS: no alias to free!");
        OLC_GROUP(d)->alias = strdup(arg);
        gedit_disp_menu(d);
        break;
    case GEDIT_NAME:
        if (OLC_GROUP(d)->name)
            free(OLC_GROUP(d)->name);
        else
            log("SYSERR: OLC: GEDIT_NAME: no name to free!");
        OLC_GROUP(d)->name = strdup(arg);
        gedit_disp_menu(d);
        break;
    case GEDIT_DESCRIPTION:
        if (OLC_GROUP(d)->description)
            free(OLC_GROUP(d)->description);
        else
            log("SYSERR: OLC: GEDIT_DESCRIPTION: no description to free!");
        OLC_GROUP(d)->description = strdup(arg);
        gedit_disp_menu(d);
        break;
    case GEDIT_LEVEL:
        OLC_GROUP(d)->minimum_level = std::clamp(atoi(arg), 0, LVL_IMPL);
        gedit_disp_menu(d);
        break;
    case GEDIT_ADD_COMMAND:
        if (!*arg) {
            gedit_disp_menu(d);
            return;
        }
        if ((num = parse_command(arg)) <= 0) {
            string_to_output(d, "Unrecognized command.  Try again: ");
            return;
        }
        if (OLC_GROUP(d)->commands) {
            for (i = 0; OLC_GROUP(d)->commands[i]; ++i)
                if (OLC_GROUP(d)->commands[i] == num) {
                    string_to_output(d, "This command group already contains that command.\n");
                    gedit_disp_menu(d);
                    return;
                }
            RECREATE(OLC_GROUP(d)->commands, int, i + 2);
            OLC_GROUP(d)->commands[i + 1] = 0;
        } else {
            CREATE(OLC_GROUP(d)->commands, int, 2);
            i = 0;
        }
        OLC_GROUP(d)->commands[i] = num;
        gedit_disp_menu(d);
        break;
    case GEDIT_REMOVE_COMMAND:
        if (!*arg) {
            gedit_disp_menu(d);
            return;
        }
        if ((num = parse_command(arg)) <= 0) {
            string_to_output(d, "Unrecognized command.  Try again: ");
            return;
        }
        if (OLC_GROUP(d)->commands) {
            for (i = 0; OLC_GROUP(d)->commands[i]; ++i)
                if (OLC_GROUP(d)->commands[i] == num)
                    num = -1;
                else if (num < 0)
                    OLC_GROUP(d)->commands[i - 1] = OLC_GROUP(d)->commands[i];
            if (num < 0) {
                OLC_GROUP(d)->commands[i - 1] = 0;
                gedit_disp_menu(d);
                break;
            }
        }
        string_to_output(d, "This command group does not contain that command.\n");
        gedit_disp_menu(d);
        return;
    case GEDIT_CONFIRM_SAVE:
        switch (to_lower(*arg)) {
        case 'y':
            string_to_output(d, "Saving command group in memory.\n");
            gedit_save_internally(d);
            log(LogSeverity::Debug, std::max<int>(LVL_GOD, GET_INVIS_LEV(d->character)),
                "OLC: {} edits command group {}.", GET_NAME(d->character), OLC_GROUP(d)->alias);
            /* Fall through */
        case 'n':
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            string_to_output(d,
                             "Invalid choice!\n"
                             "Do you wish to save the command group?\n");
            break;
        }
        return;
    }
    OLC_VAL(d) = 1;
}

void gedit_disp_menu(DescriptorData *d) {
    std::string resp;
    OLCCommandGroup *group = OLC_GROUP(d);
    int i;

    get_char_cols(d->character);

    resp += fmt::format(
#if defined(CLEAR_SCREEN)
        ".[H.[J"
#endif
        "-- Command Group: id [{}{:5}{}]\n"
        "{}1{}) Alias       : {}{}\n"
        "{}2{}) Name        : {}{}\n"
        "{}3{}) Description :\n{}{}"
        "{}4{}) Minimum Lvl : {}{}\n"
        "{}Group Commands :\n",
        grn, OLC_NUM(d), nrm, grn, nrm, yel, group->alias, grn, nrm, yel, group->name, grn, nrm, yel,
        group->description, grn, nrm, yel, group->minimum_level, nrm);

    if (group->commands)
        for (i = 0; group->commands[i]; ++i)
            resp += fmt::format("  {}\n", cmd_info[group->commands[i]].command);
    else
        resp += "  None.\n";

    resp += fmt::format(
        "{}A{}) Add command to group.\n"
        "{}R{}) Remove command from group.\n"
        "{}C{}) Clear all commands in group.\n"
        "{}Q{}) Quit\n"
        "Enter your choice : ",
        grn, nrm, grn, nrm, grn, nrm, grn, nrm);

    string_to_output(d, resp.c_str());

    OLC_MODE(d) = GEDIT_MAIN_MENU;
}

static bool group_in_list(int *list, CommandGroup *group) {
    int i = 0;
    if (list)
        for (i = 0; list[i] >= 0; ++i)
            if (list[i] == GROUP_NUM(group))
                return true;
    return false;
}

static void add_command_group(int **listptr, CommandGroup *group) {
    int i;

    if (*listptr) {
        for (i = 0; (*listptr)[i] >= 0; ++i)
            ;
        RECREATE(*listptr, int, i + 1);
    } else {
        i = 0;
        CREATE(*listptr, int, 2);
    }

    (*listptr)[i] = GROUP_NUM(group);
    (*listptr)[i + 1] = -1;
}

static void remove_command_group(int **listptr, CommandGroup *group) {
    int i, found = false;

    if (*listptr) {
        for (i = 0; (*listptr)[i] >= 0; ++i)
            if ((*listptr)[i] == GROUP_NUM(group))
                found = true;
            else if (found)
                (*listptr)[i - 1] = (*listptr)[i];
        if (found) {
            (*listptr)[i - 1] = -1;
            if (i - 1 == 0) {
                free(*listptr);
                *listptr = nullptr;
            }
        }
    }
}

static void gedit_save_internally(DescriptorData *d) {
    CommandGroup *group;
    int cmd;

    if (OLC_NUM(d) >= 0)
        group = cmd_groups + OLC_NUM(d);
    else {
        if (cmd_groups)
            RECREATE(cmd_groups, CommandGroup, num_cmd_groups + 1);
        else
            CREATE(cmd_groups, CommandGroup, 1);
        group = &cmd_groups[num_cmd_groups++];
    }

    group->alias = strdup(OLC_GROUP(d)->alias);
    group->name = strdup(OLC_GROUP(d)->name);
    group->description = strdup(OLC_GROUP(d)->description);
    group->minimum_level = OLC_GROUP(d)->minimum_level;

    /* Remove group from all commands */
    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
        remove_command_group(&grp_info[cmd].groups, group);

    /* Add group to commands */
    if (OLC_GROUP(d)->commands)
        for (cmd = 0; OLC_GROUP(d)->commands[cmd]; ++cmd)
            add_command_group(&grp_info[OLC_GROUP(d)->commands[cmd]].groups, group);
}

static void gedit_save_to_disk() {
    CommandGroup *group;
    FILE *file;
    int cmd;

    if (!(file = fopen(GROUP_FILE, "w"))) {
        log(LogSeverity::Warn, LVL_GOD, "SYSERR: OLC: gedit_save_to_disk: Can't write to command group file.");
        return;
    }

    for (group = cmd_groups; group < top_of_cmd_groups; ++group) {
        fprintf(file,
                "alias: %s\n"
                "name: %s\n"
                "desc:\n%s~\n"
                "level: %d\n"
                "commands:\n",
                filter_chars(buf, group->alias, "\n"), filter_chars(buf1, group->name, "\n"),
                filter_chars(buf2, group->description, "\r~"), group->minimum_level);
        for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
            if (group_in_list(grp_info[cmd].groups, group))
                fprintf(file, "%s\n", filter_chars(buf, cmd_info[cmd].command, "\n"));
        fprintf(file, "~\n~~\n");
    }

    fprintf(file, "~~~\n");

    fclose(file);
}

void boot_command_groups() {
    FILE *file;
    char line[MAX_INPUT_LENGTH];
    char tag[128];
    CommandGroup *group;
    int buffer, cmd;

    if (cmd_groups)
        free_command_groups();

    /*
     * This is necessary for the game to work.  The grp_info array
     * parallels the cmd_info array because cmd_info is const.
     */
    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
        ;
    CREATE(grp_info, CommandGroupInfo, cmd + 1);

    if (!(file = fopen(GROUP_FILE, "r"))) {
        log("SYSERR: boot_command_groups: Can't read command group file.");
        return;
    }

    CREATE(cmd_groups, CommandGroup, (buffer = 10));
    group = nullptr;
    num_cmd_groups = 0;

    while (get_line(file, line)) {
        if (!strcasecmp(line, "~~~"))
            break;
        else if (!strcasecmp(line, "~~")) {
            ++num_cmd_groups;
            group = nullptr;
            continue;
        }

        if (!group) {
            if (num_cmd_groups >= buffer) {
                buffer += 10;
                RECREATE(cmd_groups, CommandGroup, buffer);
            }
            group = &cmd_groups[num_cmd_groups];
        }

        tag_argument(line, tag);

        if (!strcasecmp(tag, "alias"))
            group->alias = strdup(line);
        else if (!strcasecmp(tag, "name"))
            group->name = strdup(line);
        else if (!strcasecmp(tag, "desc"))
            group->description = fread_string(file, "boot_command_groups");
        else if (!strcasecmp(tag, "level"))
            group->minimum_level = atoi(line);
        else if (!strcasecmp(tag, "commands")) {
            while (get_line(file, line)) {
                if (*line == '~')
                    break;
                if ((cmd = find_command(line)) >= 0)
                    add_command_group(&grp_info[cmd].groups, group);
            }
        }
    }

    fclose(file);

    if (!num_cmd_groups) {
        free(cmd_groups);
        cmd_groups = nullptr;
    }
}

void do_show_command_groups(CharData *ch, char *argument) {
    std::string resp;
    CommandGroup *group;
    int num, cmd, grp, found = 0;

    skip_spaces(&argument);

    if (*argument && (num = find_command_group(argument)) >= 0) {
        group = &cmd_groups[num];
        resp += fmt::format(
            "Command Group    : @y{}@0 (@g{}@0)\n"
            "Name             : @c{}@0\n"
            "Description      :\n@c{}@0"
            "Min. Admin Lvl.  : @c{}@0\n"
            "Commands         : @c",
            group->alias, num, group->name, group->description, group->minimum_level);
        for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
            if (grp_info[cmd].groups)
                for (grp = 0; grp_info[cmd].groups[grp] >= 0; ++grp)
                    if (grp_info[cmd].groups[grp] == num)
                        resp += fmt::format("{}{}", found++ ? ", " : "", cmd_info[cmd].command);
        resp += found ? "@0\n" : "NONE@0\n";
        char_printf(ch, resp.c_str());
    } else if (cmd_groups) {
        char_printf(ch,
                    "Alias         MinLvl  Name\n"
                    "------------  ------  ---------------------------------\n");
        for (group = cmd_groups; group < top_of_cmd_groups; ++group)
            char_printf(ch, "{:<16s} {:3d}  {}\n", group->alias, group->minimum_level, group->name);
    } else
        char_printf(ch, "No command groups.\n");
}

void do_show_command(CharData *ch, char *argument) {
    std::string resp;
    const CommandInfo *command;
    int cmd, grp;

    constexpr std::string_view command_flags[] = {"MEDITATE", "MAJOR PARA", "MINOR PARA", "HIDE", "BOUND",
                                                  "CAST",     "OLC",        "NOFIGHT",    "\n"};

    skip_spaces(&argument);

    if (!*argument) {
        char_printf(ch, "Usage: show command <command>\n");
        return;
    }

    if ((cmd = parse_command(argument)) <= 0) {
        char_printf(ch, "Command not found.\n");
        return;
    }

    command = &cmd_info[cmd];

    resp += fmt::format(
        "Command           : @y{}@0 (@g{}@0)\n"
        "Minimum Position  : @c{}@0\n"
        "Minimum Stance    : @c{}@0\n"
        "Minimum Level     : @c{}@0\n"
        "Subcommand Code   : @c{}@0\n"
        "Usage Flags       : @c{}@0\n"
        "Groups            : @c",
        command->command, cmd, position_types[(int)command->minimum_position],
        stance_types[(int)command->minimum_stance], command->minimum_level, command->subcmd,
        sprintbit(command->flags, command_flags));

    if (grp_info[cmd].groups) {
        for (grp = 0; grp_info[cmd].groups[grp] >= 0; ++grp)
            resp += fmt::format("{}{}", grp ? ", " : "", cmd_groups[grp_info[cmd].groups[grp]].alias);
        resp += "@0\n";
    } else
        resp += "NONE@0\n";

    char_printf(ch, resp.c_str());
}

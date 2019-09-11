/***************************************************************************
 * $Id: commands.c,v 1.1 2009/07/16 19:14:56 myc Exp $
 ***************************************************************************/
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

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "commands.h"
#include "db.h"
#include "olc.h"
#include "constants.h"
#include "math.h"
#include "modify.h"

/*
 * Private globals
 */
/* The dynamically allocated array of command groups. */
#define top_of_cmd_groups	(cmd_groups + num_cmd_groups)
struct command_group *cmd_groups = NULL;
int num_cmd_groups = 0;

/* Command groups list; MUST have the same # of entries as cmd_info */
struct command_group_info *grp_info = NULL;


/*
 * Private interface
 */
#define VALID_GROUP_NUM(gg)	((gg) >= cmd_groups && (gg) < top_of_cmd_groups)
#define GROUP_NUM(gg)		((gg) - cmd_groups)
static void gedit_setup_existing(struct descriptor_data *d, int group);
static void gedit_setup_new(struct descriptor_data *d);
static void gedit_save_internally(struct descriptor_data *d);
static void gedit_save_to_disk();


int command_group_number(struct command_group *group)
{
  return VALID_GROUP_NUM(group) ? GROUP_NUM(group) : -1;
}

int find_command_group(char *name)
{
  struct command_group *group = cmd_groups;

  while (group < top_of_cmd_groups) {
    if (!str_cmp(group->alias, name))
      return GROUP_NUM(group);
    ++group;
  }

  return (-1);
}

static void free_command_groups() {
  struct command_group *group;
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
  cmd_groups = NULL;
  num_cmd_groups = 0;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
    if (grp_info[cmd].groups) {
      free(grp_info[cmd].groups);
      grp_info[cmd].groups = NULL;
    }
}


bool can_use_command(struct char_data *ch, int cmd)
{
  /* NPCs can't have grants */
  if (cmd < 0 || cmd >= num_of_cmds)
    return FALSE;
  else if (IS_NPC(ch))
    return CMD_USEABLE_FOR_LEVEL(ch, cmd);
  else if (IS_FLAGGED(GET_GRANT_CACHE(ch), cmd))
    return TRUE;
  else if (IS_FLAGGED(GET_REVOKE_CACHE(ch), cmd))
    return FALSE;
  else
    return CMD_USEABLE_FOR_LEVEL(ch, cmd);
}


ACMD(do_gedit)
{
  int group;
  struct descriptor_data *d;

  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Edit which command group? */
  argument = any_one_arg(argument, arg);
  if (!*arg) {
    send_to_char("Specify a command group to edit.\r\n", ch);
    return;
  }

  /* Make sure it exists, or create a new one */
  if (!str_cmp(arg, "new"))
    group = -1;
  else if (!str_cmp(arg, "save")) {
    send_to_char("Saving all command groups.\r\n", ch);
    sprintf(buf, "OLC: %s saves command groups.", GET_NAME(ch));
    mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
    gedit_save_to_disk();
    return;
  }
  else if ((group = find_command_group(arg)) < 0) {
    cprintf(ch, "Command group '%s' not found.  Use 'gedit new' to create it.\r\n", arg);
    return;
  }

  /* Make sure it's not being edited already */
  if (group >= 0)
    for (d = descriptor_list; d; d = d->next)
      if (d->connected == CON_GEDIT && d->olc && OLC_NUM(d) == group) {
        cprintf(ch, "That command group is currently being edited by %s.\r\n",
                PERS(ch, d->character));
        return;
      }
  d = ch->desc;

  /* Give descriptor an OLC structure */
  CREATE(d->olc, struct olc_data, 1);

  if (group >= 0)
    gedit_setup_existing(d, group);
  else
    gedit_setup_new(d);
  STATE(d) = CON_GEDIT;

  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_FLAG(PLR_FLAGS(ch), PLR_WRITING);
}

static void gedit_setup_existing(struct descriptor_data *d, int group)
{
  int cmd, grp, count = 0;

  OLC_NUM(d) = group;

  CREATE(OLC_GROUP(d), struct olc_command_group, 1);

  OLC_GROUP(d)->alias = strdup(cmd_groups[group].alias);
  OLC_GROUP(d)->name = strdup(cmd_groups[group].name);
  OLC_GROUP(d)->description = strdup(cmd_groups[group].description);
  OLC_GROUP(d)->commands = NULL;

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

static void gedit_setup_new(struct descriptor_data *d)
{
  OLC_NUM(d) = -1;

  CREATE(OLC_GROUP(d), struct olc_command_group, 1);

  OLC_GROUP(d)->alias = strdup("newgroup");
  OLC_GROUP(d)->name = strdup("New Command Group");
  OLC_GROUP(d)->description = strdup("A new command group.\r\n");
  OLC_GROUP(d)->commands = NULL;

  gedit_disp_menu(d);
  OLC_VAL(d) = 0;
}

void gedit_parse(struct descriptor_data *d, char *arg)
{
  int num, i;

  switch (OLC_MODE(d)) {
  case GEDIT_MAIN_MENU:
    switch (LOWER(*arg)) {
    case '1':
      write_to_output("Enter new group alias: ", d);
      OLC_MODE(d) = GEDIT_ALIAS;
      break;
    case '2':
      write_to_output("Enter new group name: ", d);
      OLC_MODE(d) = GEDIT_NAME;
      break;
    case '3':
      write_to_output("Enter new group description (/s saves /h for help): \r\n\r\n", d);
      OLC_MODE(d) = GEDIT_DESCRIPTION;
      string_write(d, &OLC_GROUP(d)->description, MAX_STRING_LENGTH);
      break;
    case '4':
      write_to_output("Enter minimum level for admin access to group: ", d);
      OLC_MODE(d) = GEDIT_LEVEL;
      break;
    case 'a':
      write_to_output("Command to add to group: ", d);
      OLC_MODE(d) = GEDIT_ADD_COMMAND;
      break;
    case 'r':
      write_to_output("Command to remove from group: ", d);
      OLC_MODE(d) = GEDIT_REMOVE_COMMAND;
      break;
    case 'c':
      write_to_output("All commands cleared.\r\n", d);
      free(OLC_GROUP(d)->commands);
      OLC_GROUP(d)->commands = NULL;
      OLC_VAL(d) = 1;
      gedit_disp_menu(d);
      break;
    case 'q':
      for (i = 0; i < num_cmd_groups; ++i)
        if (i != OLC_NUM(d) && !strcmp(cmd_groups[i].alias, OLC_GROUP(d)->alias)) {
          write_to_output("This command group has the same alias as another existing group.\r\n"
                          "It cannot be saved until the alias is changed.\r\n", d);
          return;
        }
      if (OLC_VAL(d)) {
        write_to_output("Do you wish to save the changes to the command group? (y/n) : ", d);
        OLC_MODE(d) = GEDIT_CONFIRM_SAVE;
      }
      else {
        write_to_output("No changes made.\r\n", d);
        cleanup_olc(d, CLEANUP_ALL);
      }
    }
    return;
  case GEDIT_ALIAS:
    skip_spaces(&arg);
    if (!*arg) {
      write_to_output("Group alias must consist of at least one letter or number.\r\n"
                      "Try again: ", d);
      return;
    }
    for (i = 0; arg[i]; ++i)
      if (!isalpha(arg[i]) && !isdigit(arg[i])) {
        write_to_output("Group alias may only consist of letters and numbers.\r\n"
                        "Try again: ", d);
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
    OLC_GROUP(d)->minimum_level = LIMIT(0, atoi(arg), LVL_IMPL);
    gedit_disp_menu(d);
    break;
  case GEDIT_ADD_COMMAND:
    if (!*arg) {
      gedit_disp_menu(d);
      return;
    }
    if ((num = parse_command(arg)) <= 0) {
      write_to_output("Unrecognized command.  Try again: ", d);
      return;
    }
    if (OLC_GROUP(d)->commands) {
      for (i = 0; OLC_GROUP(d)->commands[i]; ++i)
        if (OLC_GROUP(d)->commands[i] == num) {
          write_to_output("This command group already contains that command.\r\n", d);
          gedit_disp_menu(d);
          return;
        }
      RECREATE(OLC_GROUP(d)->commands, int, i + 2);
      OLC_GROUP(d)->commands[i + 1] = 0;
    }
    else {
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
      write_to_output("Unrecognized command.  Try again: ", d);
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
    write_to_output("This command group does not contain that command.\r\n", d);
    gedit_disp_menu(d);
    return;
  case GEDIT_CONFIRM_SAVE:
    switch (LOWER(*arg)) {
    case 'y':
      write_to_output("Saving command group in memory.\r\n", d);
      gedit_save_internally(d);
      sprintf(buf, "OLC: %s edits command group %s.",
              GET_NAME(d->character), OLC_GROUP(d)->alias);
      mudlog(buf, CMP, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
      /* Fall through */
    case 'n':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output("Invalid choice!\r\n"
                      "Do you wish to save the command group? : ", d);
      break;
    }
    return;
  }
  OLC_VAL(d) = 1;
}

void gedit_disp_menu(struct descriptor_data *d)
{
  struct olc_command_group *group = OLC_GROUP(d);
  int i;

  get_char_cols(d->character);

  str_start(buf, sizeof(buf));

  str_catf(buf,
#if defined(CLEAR_SCREEN)
           ".[H.[J"
#endif
           "-- Command Group: id [%s%5d%s]\r\n"
           "%s1%s) Alias       : %s%s\r\n"
           "%s2%s) Name        : %s%s\r\n"
           "%s3%s) Description :\r\n%s%s"
           "%s4%s) Minimum Lvl : %s%d\r\n"
           "%sGroup Commands :\r\n",

           grn, OLC_NUM(d), nrm,
           grn, nrm, yel, group->alias,
           grn, nrm, yel, group->name,
           grn, nrm, yel, group->description,
           grn, nrm, yel, group->minimum_level,
           nrm);

  if (group->commands)
    for (i = 0; group->commands[i]; ++i)
      str_catf(buf, "  %s\r\n", cmd_info[group->commands[i]].command);
  else
    str_cat(buf, "  None.\r\n");

  str_catf(buf,
           "%sA%s) Add command to group.\r\n"
           "%sR%s) Remove command from group.\r\n"
           "%sC%s) Clear all commands in group.\r\n"
           "%sQ%s) Quit\r\n"
           "Enter your choice : ",
           grn, nrm, grn, nrm, grn, nrm, grn, nrm
           );

  write_to_output(buf, d);

  OLC_MODE(d) = GEDIT_MAIN_MENU;
}

static bool group_in_list(int *list, struct command_group *group)
{
  int i = 0;
  if (list)
    for (i = 0; list[i] >= 0; ++i)
      if (list[i] == GROUP_NUM(group))
        return TRUE;
  return FALSE;
}

static void add_command_group(int **listptr, struct command_group *group)
{
  int i;

  if (*listptr) {
    for (i = 0; (*listptr)[i] >= 0; ++i);
    RECREATE(*listptr, int, i + 1);
  }
  else {
    i = 0;
    CREATE(*listptr, int, 2);
  }

  (*listptr)[i] = GROUP_NUM(group);
  (*listptr)[i + 1] = -1;
}

static void remove_command_group(int **listptr, struct command_group *group)
{
  int i, found = FALSE;

  if (*listptr) {
    for (i = 0; (*listptr)[i] >= 0; ++i)
      if ((*listptr)[i] == GROUP_NUM(group))
        found = TRUE;
      else if (found)
        (*listptr)[i - 1] = (*listptr)[i];
    if (found) {
      (*listptr)[i - 1] = -1;
      if (i - 1 == 0) {
        free(*listptr);
        *listptr = NULL;
      }
    }
  }
}

static void gedit_save_internally(struct descriptor_data *d)
{
  struct command_group *group;
  int cmd;

  if (OLC_NUM(d) >= 0)
    group = cmd_groups + OLC_NUM(d);
  else {
    if (cmd_groups)
      RECREATE(cmd_groups, struct command_group, num_cmd_groups + 1);
    else
      CREATE(cmd_groups, struct command_group, 1);
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

static void gedit_save_to_disk()
{
  struct command_group *group;
  FILE *file;
  int cmd;

  if (!(file = fopen(GROUP_FILE, "w"))) {
    mudlog("SYSERR: OLC: gedit_save_to_disk: Can't write to command group file.",
           BRF, LVL_GOD, TRUE);
    return;
  }

  for (group = cmd_groups; group < top_of_cmd_groups; ++group) {
    fprintf(file,
            "alias: %s\n"
            "name: %s\n"
            "desc:\n%s~\n"
            "level: %d\n"
            "commands:\n",
            filter_chars(buf, group->alias, "\r\n"),
            filter_chars(buf1, group->name, "\r\n"),
            filter_chars(buf2, group->description, "\r~"),
            group->minimum_level);
    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
      if (group_in_list(grp_info[cmd].groups, group))
        fprintf(file, "%s\n", filter_chars(buf, cmd_info[cmd].command, "\r\n"));
    fprintf(file, "~\n~~\n");
  }

  fprintf(file, "~~~\n");

  fclose(file);
}

void boot_command_groups()
{
  FILE *file;
  char line[MAX_INPUT_LENGTH];
  char tag[128];
  struct command_group *group;
  int buffer, cmd;

  if (cmd_groups)
    free_command_groups();

  /*
   * This is necessary for the game to work.  The grp_info array
   * parallels the cmd_info array because cmd_info is const.
   */
  for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd);
  CREATE(grp_info, struct command_group_info, cmd + 1);

  if (!(file = fopen(GROUP_FILE, "r"))) {
    log("SYSERR: boot_command_groups: Can't read command group file.");
    return;
  }

  CREATE(cmd_groups, struct command_group, (buffer = 10));
  group = NULL;
  num_cmd_groups = 0;

  while (get_line(file, line)) {
    if (!strcmp(line, "~~~"))
      break;
    else if (!strcmp(line, "~~")) {
      ++num_cmd_groups;
      group = NULL;
      continue;
    }

    if (!group) {
      if (num_cmd_groups >= buffer) {
        buffer += 10;
        RECREATE(cmd_groups, struct command_group, buffer);
      }
      group = &cmd_groups[num_cmd_groups];
    }

    tag_argument(line, tag);

    if (!strcmp(tag, "alias"))
      group->alias = strdup(line);
    else if (!strcmp(tag, "name"))
      group->name = strdup(line);
    else if (!strcmp(tag, "desc"))
      group->description = fread_string(file, "boot_command_groups");
    else if (!strcmp(tag, "level"))
      group->minimum_level = atoi(line);
    else if (!strcmp(tag, "commands")) {
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
    cmd_groups = NULL;
  }
}


void do_show_command_groups(struct char_data *ch, char *argument)
{
  struct command_group *group;
  int num, cmd, grp, found = 0;

  skip_spaces(&argument);

  if (*argument && (num = find_command_group(argument)) >= 0) {
    group = &cmd_groups[num];
    str_start(buf, sizeof(buf));
    str_catf(buf, "Command Group    : @y%s@0 (@g%d@0)\r\n"
                  "Name             : @c%s@0\r\n"
                  "Description      :\r\n@c%s@0"
                  "Min. Admin Lvl.  : @c%d@0\r\n"
                  "Commands         : @c",
             group->alias, num,
             group->name,
             group->description,
             group->minimum_level);
    for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
      if (grp_info[cmd].groups)
        for (grp = 0; grp_info[cmd].groups[grp] >= 0; ++grp)
          if (grp_info[cmd].groups[grp] == num)
            str_catf(buf, "%s%s", found++ ? ", " : "", cmd_info[cmd].command);
    str_cat(buf, found ? "@0\r\n" : "NONE@0\r\n");
    send_to_char(buf, ch);
  }
  else if (cmd_groups) {
    send_to_char("Alias         MinLvl  Name\r\n"
                 "------------  ------  ---------------------------------\r\n", ch);
    for (group = cmd_groups; group < top_of_cmd_groups; ++group)
      cprintf(ch, "%-16.16s %3d  %s\r\n",
              group->alias, group->minimum_level, group->name);
  }
  else
    send_to_char("No command groups.\r\n", ch);
}

void do_show_command(struct char_data *ch, char *argument)
{
  struct command_info *command;
  int cmd, grp;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Usage: show command <command>\r\n", ch);
    return;
  }

  if ((cmd = parse_command(argument)) <= 0) {
    send_to_char("Command not found.\r\n", ch);
    return;
  }

  command = &cmd_info[cmd];

  str_start(buf, sizeof(buf));

  sprintbit(command->flags, command_flags, buf1);

  str_catf(buf, "Command           : @y%s@0 (@g%d@0)\r\n"
                "Minimum Position  : @c%s@0\r\n"
                "Minimum Stance    : @c%s@0\r\n"
                "Minimum Level     : @c%d@0\r\n"
                "Subcommand Code   : @c%d@0\r\n"
                "Usage Flags       : @c%s@0\r\n"
                "Groups            : @c",
           command->command, cmd,
           position_types[(int) command->minimum_position],
           stance_types[(int) command->minimum_stance],
           command->minimum_level,
           command->subcmd,
           buf1);

  if (grp_info[cmd].groups) {
    for (grp = 0; grp_info[cmd].groups[grp] >= 0; ++grp)
      str_catf(buf, "%s%s", grp ? ", " : "", cmd_groups[grp_info[cmd].groups[grp]].alias);
    str_cat(buf, "@0\r\n");
  }
  else
    str_cat(buf, "NONE@0\r\n");

  send_to_char(buf, ch);
}


/***************************************************************************
 * $Log: commands.c,v $
 * Revision 1.1  2009/07/16 19:14:56  myc
 * Initial revision
 *
 ***************************************************************************/

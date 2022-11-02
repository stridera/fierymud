/***************************************************************************
 *   File: commands.h                                     Part of FieryMUD *
 *  Usage: Header for utilities for player commands and command groups     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "interpreter.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

struct command_group {
    char *alias;
    char *name;
    char *description;
    int minimum_level;
};

struct olc_command_group {
    char *alias;
    char *name;
    char *description;
    int minimum_level;
    int *commands;
};

struct command_group_info {
    int *groups;
};

#define top_of_cmd_groups (cmd_groups + num_cmd_groups)
extern struct command_group *cmd_groups;
extern int num_cmd_groups;
extern struct command_group_info *grp_info;

#define CMD_USEABLE_FOR_LEVEL(ch, cmd) (cmd_info[(cmd)].minimum_level <= GET_LEVEL(ch))

extern ACMD(do_gedit);
extern int find_command_group(char *name);
extern int command_group_number(command_group *group);
extern bool can_use_command(char_data *ch, int cmd);
extern void boot_command_groups();
extern void gedit_parse(descriptor_data *d, char *arg);
extern void gedit_disp_menu(descriptor_data *d);
extern void do_show_command_groups(char_data *ch, char *argument);
extern void do_show_command(char_data *ch, char *argument);

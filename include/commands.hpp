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

struct CommandGroup {
    char *alias;
    char *name;
    char *description;
    int minimum_level;
};

struct CommandGroupInfo {
    int *groups;
};

#define top_of_cmd_groups (cmd_groups + num_cmd_groups)

extern CommandGroup *cmd_groups;
extern int num_cmd_groups;
extern CommandGroupInfo *grp_info;

#define CMD_USEABLE_FOR_LEVEL(ch, cmd) (cmd_info[(cmd)].minimum_level <= GET_LEVEL(ch))

ACMD(do_gedit);
int find_command_group(const char *name);
int command_group_number(CommandGroup *group);
bool can_use_command(CharData *ch, int cmd);
void boot_command_groups();
void gedit_parse(DescriptorData *d, char *arg);
void gedit_disp_menu(DescriptorData *d);
void do_show_command_groups(CharData *ch, char *argument);
void do_show_command(CharData *ch, char *argument);

/***************************************************************************
 *   File: textfiles.h                                    Part of FieryMUD *
 *  Usage: Text files management                                           *
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

#define TEXT_ANEWS 0
#define TEXT_BACKGROUND 1
#define TEXT_CREDITS 2
#define TEXT_HANDBOOK 3
#define TEXT_HELP 4
#define TEXT_IMOTD 5
#define TEXT_INFO 6
#define TEXT_MOTD 7
#define TEXT_NEWS 8
#define TEXT_POLICIES 9
#define TEXT_WIZLIST 10
#define NUM_TEXT_FILES 11

#define SCMD_ANEWS TEXT_ANEWS
#define SCMD_CREDITS TEXT_CREDITS
#define SCMD_HANDBOOK TEXT_HANDBOOK
#define SCMD_IMOTD TEXT_IMOTD
#define SCMD_INFO TEXT_INFO
#define SCMD_MOTD TEXT_MOTD
#define SCMD_NEWS TEXT_NEWS
#define SCMD_POLICIES TEXT_POLICIES
#define SCMD_WIZLIST TEXT_WIZLIST

void boot_text();
ACMD(do_reload);
ACMD(do_textview);
ACMD(do_tedit);
void free_text_files();
const char *get_text(int text);
time_t get_text_update_time(int text);

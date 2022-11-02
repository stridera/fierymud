/***************************************************************************
 *   File: corpse_save.h                                  Part of FieryMUD *
 *  Usage: Handling of player corpses                                      *
 * Author: Nechtrous                                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

#define MAX_CORPSES 500

extern void register_corpse(obj_data *corpse);
extern void boot_corpses(void);
extern void save_corpse(obj_data *corpse);
extern void update_corpse(obj_data *corpse);
extern void destroy_corpse(obj_data *corpse);
extern int corpse_count(void);
extern void show_corpses(char_data *ch, char *argument);

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

void register_corpse(ObjData *corpse);
void boot_corpses(void);
void save_corpse(ObjData *corpse);
void update_corpse(ObjData *corpse);
void destroy_corpse(ObjData *corpse);
int corpse_count(void);
void show_corpses(CharData *ch, Arguments argument);

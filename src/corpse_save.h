/***************************************************************************
 *   File: corpse_save.h                                  Part of FieryMUD *
 *  Usage: Handling of player corpses                                      *
 * Author: Nechtrous                                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#ifndef __FIERY_CORPSE_SAVE_H
#define __FIERY_CORPSE_SAVE_H

#include "structs.h"
#include "sysdep.h"

#define MAX_CORPSES 500

extern void register_corpse(struct obj_data *corpse);
extern void boot_corpses(void);
extern void save_corpse(struct obj_data *corpse);
extern void update_corpse(struct obj_data *corpse);
extern void destroy_corpse(struct obj_data *corpse);
extern int corpse_count(void);
extern void show_corpses(struct char_data *ch, char *argument);

#endif /* __FIERY_CORPSE_SAVE_H */

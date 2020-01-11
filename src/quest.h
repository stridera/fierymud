/***************************************************************************
 * $Id: quest.h,v 1.11 2008/02/16 20:31:32 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: quest.h                                        Part of FieryMUD *
 *  Usage: Defines a quest(quest_info) and the list of quests a player     *
 *         will have (quest_list)                                          *
 *     By: Matt Proctor 22 Oct 2000                                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 2000 by the Fiery Consortium                    *
 ***************************************************************************/

#ifndef __FIERY_QUEST_H
#define __FIERY_QUEST_H

#include "sysdep.h"
#include "structs.h"

#define MAX_QNAME_LEN 35
/* a short is 16 bits, and the top bit is reserved for subclasses*/
#define MAX_QUEST_ID (2 << 15)
#define SUBCLASS_BIT 0x8000
#define ARRAY_LOCN 14
/* for a subclass quest, the top 4 bits are reserved (3 bits to choose dest
 * class (dependent on current class => 8 choices from current class max*/
#define MAX_SUBCLASS_QUEST_ID ((2 << ARRAY_LOCN) - 1)
struct quest_info {
    unsigned short quest_id; /* 16 bits = 65536 possible quests
                              * although the top bit is reserved for subclasses
                              */
    char *quest_name;
    unsigned char maxstages; /* 8 bits = 254 possible stages, as 0 is reserved
                              * for failed and -1 (all bits set) for passed
                              */
};

struct quest_var_list {
    char *var;
    char *val;
    struct quest_var_list *next;
};

struct quest_list {
    unsigned short quest_id;
    unsigned char stage;
    struct quest_var_list *variables;
    struct quest_list *next;
};

#define QUEST_FAILURE 0x00
#define QUEST_START 0x01
#define QUEST_MAXSTAGE 0xFE
#define QUEST_SUCCESS 0xFF
/* if it is a subclass the top bit oof the quest_id) will be set*/
#define IS_SUBCLASS_QUEST(x) (x & SUBCLASS_BIT)

/*
 * quest function definitions
 */
void perform_quest(struct trig_data *t, char *argument, struct char_data *ch, struct obj_data *obj,
                   struct room_data *room);
void quest_advance(struct char_data *ch, struct char_data *vict, char *qname, char *error_string, int amount);
void quest_start(struct char_data *ch, struct char_data *vict, char *qname, char *error_string, char *subclass);
void quest_complete(struct char_data *ch, struct char_data *vict, char *qname, char *error_string);
void quest_fail(struct char_data *ch, struct char_data *vict, char *qname, char *error_string);
void quest_rewind(struct char_data *ch, struct char_data *vict, char *qname, char *error_string, int amount);
void quest_restart(struct char_data *ch, struct char_data *vict, char *qname, char *error_string);
void quest_erase(struct char_data *ch, struct char_data *vict, char *qname, char *error_string);
int quest_stage(struct char_data *ch, char *qname);
void set_quest_variable(struct char_data *ch, struct char_data *vict, char *qname, char *error_string, char *variable,
                        char *value);
char *get_quest_variable(struct char_data *ch, char *qname, char *variable);
int has_failed_quest(char *qname, struct char_data *ch);
int has_completed_quest(char *qname, struct char_data *ch);
unsigned short quest_find_num(char *qname);
char *check_quest_name(char *qname);
void free_quest_list(struct char_data *ch);
void free_quests(void);

#ifndef __QUEST_C__
extern int max_quests;
extern struct quest_info *all_quests;
#endif

#endif /*__FIERY_QUEST_H */

/***************************************************************************
 * $Log: quest.h,v $
 * Revision 1.11  2008/02/16 20:31:32  myc
 * Adding functions to free quests at program termination.
 *
 * Revision 1.10  2008/02/10 19:43:38  jps
 * Subclass quests now store the target subclass as a quest variable rather
 * than as 3 bits in the quest id.
 *
 * Revision 1.9  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.8  2007/05/11 19:34:15  myc
 * Modified the quest command functions so they are thin wrappers for
 * perform_quest() in quest.c.  Error handling and messages should be
 * much better now.  Advance and rewind now accept another argument
 * specifying how many stages to advance or rewind.
 *
 * Revision 1.7  2002/09/19 01:08:27  jjl
 * Update to add in quest variables!
 *
 * Revision 1.6  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.5  2001/07/08 17:47:40  mtp
 * added quest erase to remove a quest from a player (non documented)
 *
 * Revision 1.4  2000/11/19 01:03:09  rsd
 * added the back revision logs noted from rlog.
 *
 * Revision 1.3  2000/11/19 00:58:55  rsd
 * Added a new comment header to be consistent with the rest
 * of the code.  Also, aded RCS strings to comments. I've
 * noted that RCS appears to use GMT for its timestamps..
 *
 * Revision 1.2  2000/11/07 01:43:46  mtp
 * added function defn to this file and the subclass stuff
 *
 * Revision 1.1  2000/10/27 00:05:14  mtp
 * Initial revision
 *
 ***************************************************************************/

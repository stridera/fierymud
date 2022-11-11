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

#pragma once

#include "rooms.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define MAX_QNAME_LEN 35
/* a short is 16 bits, and the top bit is reserved for subclasses*/
#define MAX_QUEST_ID (2 << 15)
#define SUBCLASS_BIT 0x8000
#define ARRAY_LOCN 14
/* for a subclass quest, the top 4 bits are reserved (3 bits to choose dest
 * class (dependent on current class => 8 choices from current class max*/
#define MAX_SUBCLASS_QUEST_ID ((2 << ARRAY_LOCN) - 1)
struct QuestInfo {
    unsigned short quest_id; /* 16 bits = 65536 possible quests
                              * although the top bit is reserved for subclasses
                              */
    char *quest_name;
    unsigned char maxstages; /* 8 bits = 254 possible stages, as 0 is reserved
                              * for failed and -1 (all bits set) for passed
                              */
};

struct QuestVariableList {
    char *var;
    char *val;
    QuestVariableList *next;
};

struct QuestList {
    unsigned short quest_id;
    unsigned char stage;
    QuestVariableList *variables;
    QuestList *next;
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
void perform_quest(TrigData *t, char *argument, CharData *ch, ObjData *obj, RoomData *room);
void quest_advance(CharData *ch, CharData *vict, char *qname, char *error_string, int amount);
void quest_start(CharData *ch, CharData *vict, char *qname, char *error_string, char *subclass);
void quest_complete(CharData *ch, CharData *vict, char *qname, char *error_string);
void quest_fail(CharData *ch, CharData *vict, char *qname, char *error_string);
void quest_rewind(CharData *ch, CharData *vict, char *qname, char *error_string, int amount);
void quest_restart(CharData *ch, CharData *vict, char *qname, char *error_string);
void quest_erase(CharData *ch, CharData *vict, char *qname, char *error_string);
int quest_stage(CharData *ch, char *qname);
void set_quest_variable(CharData *ch, CharData *vict, char *qname, char *error_string, char *variable, char *value);
char *get_quest_variable(CharData *ch, char *qname, char *variable);
int has_failed_quest(char *qname, CharData *ch);
int has_completed_quest(char *qname, CharData *ch);
unsigned short quest_find_num(char *qname);
char *check_quest_name(char *qname);
void free_quest_list(CharData *ch);
void free_quests(void);

#ifndef __QUEST_C__
int max_quests;
QuestInfo *all_quests;
#endif

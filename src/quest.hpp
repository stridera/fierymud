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
    std::string_view quest_name;
    unsigned char maxstages; /* 8 bits = 254 possible stages, as 0 is reserved
                              * for failed and -1 (all bits set) for passed
                              */
};

struct QuestVariableList {
    std::string_view var;
    std::string_view val;
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
void perform_quest(TrigData *t, std::string_view argument, CharData *ch, ObjData *obj, RoomData *room);
void quest_advance(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string, int amount);
void quest_start(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string,
                 std::string_view subclass);
void quest_complete(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string);
void quest_fail(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string);
void quest_rewind(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string, int amount);
void quest_restart(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string);
void quest_erase(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string);
int quest_stage(CharData *ch, std::string_view qname);
void set_quest_variable(CharData *ch, CharData *vict, std::string_view qname, std::string_view error_string,
                        std::string_view variable, std::string_view value);
std::string_view get_quest_variable(CharData *ch, std::string_view qname, std::string_view variable);
int has_failed_quest(std::string_view qname, CharData *ch);
int has_completed_quest(std::string_view qname, CharData *ch);
unsigned short quest_find_num(std::string_view qname);
std::string_view check_quest_name(std::string_view qname);
void free_quest_list(CharData *ch);
void free_quests(void);

extern int max_quests;
extern QuestInfo *all_quests;

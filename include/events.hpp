/***************************************************************************
 *   File: events.h                                      Part of FieryMUD  *
 *  Usage: structures and prototypes for events                            *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "queue.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define EVENTFUNC(name) long(name)(void *event_obj)
/* Return values for EVENTFUNCs */
#define EVENT_FINISHED (0)
#define EVENT_PREVENT_FREE_OBJ (-1)
#define EVENT_FORCE_FREE_OBJ (-2)

struct GenericEventData {
    CharData *ch;
    CharData *vict;
    ObjData *obj;
};

struct Event {
    int num;
    EVENTFUNC(*func);
    void *event_obj;
    bool free_obj;
    QElement *q_el;
    Event **eventlist, *next;
};

void sethurtevent(CharData *ch, CharData *vict, int dam);
void overweight_check(CharData *ch);

bool char_has_event(CharData *ch, int eventtype);
bool char_has_delayed_command(CharData *ch, const char *command);

/* function protos need by other modules */
void event_init(void);
Event *event_create(int eventnum, EVENTFUNC(*func), void *event_obj, bool free_obj, Event **list, long when);

void cancel_event(Event *eventlist, int eventtype);
void event_cancel(Event *event);
void event_process(void);
long event_time(Event *event);
void event_free_all(void);
const char *eventname(Event *e);
void cancel_event_list(Event **list);
GenericEventData *mkgenericevent(CharData *ch, CharData *vict, ObjData *obj);
void delayed_command(CharData *ch, char *command, int delay, bool repeatable);

#define EVENT_AUTODOUSE 1
#define EVENT_CAMP 2
#define EVENT_HURT 3
#define EVENT_MOB_QUIT 4
#define EVENT_NAME_TIMEOUT 5
#define EVENT_RECALL 6
#define EVENT_ROOM_UNDO 7
#define EVENT_SPELL 8
#define EVENT_TRACK 9
#define EVENT_TRIGGER_WAIT 10
#define EVENT_SINK_AND_LOSE 11
#define EVENT_BATTLE_PARALYSIS 12
#define EVENT_CASTING 13
#define EVENT_REGEN_HP 14
#define EVENT_REGEN_SPELLSLOT 15
#define EVENT_REGEN_MOVE 16
// #define EVENT_REGEN_MANA 17
#define EVENT_SCRIBE 18
#define EVENT_QUICK_AGGRO 19
#define EVENT_DIE 20
#define EVENT_RAGE 21
#define EVENT_EXTRACT 22 /* To extract a char, for whatever reason */
#define EVENT_GRAVITY 23
#define EVENT_COOLDOWN 24
#define EVENT_FULLPURGE 25
#define EVENT_OVERWEIGHT 26
#define EVENT_FALLTOGROUND 27
#define EVENT_COMMAND 28
#define EVENT_EDITOR_START 29
#define EVENT_GET_MONEY 30
/* Update MAX_EVENT to be last event value + 1, please */
#define MAX_EVENT 31

EVENTFUNC(extract_event);
EVENTFUNC(hurt_event);
EVENTFUNC(fullpurge_event);
EVENTFUNC(mobquit_event);
EVENTFUNC(overweight_event);
EVENTFUNC(falltoground_event);
EVENTFUNC(command_event);

/* DATA STRUCTURES FOR VARIOUS EVENTS */

/* These are often used for event_obj. */

struct SinkAndLose {
    ObjData *obj;
    int room;
};

/* name approval time out event */
struct NameTimeoutEvent {
    DescriptorData *d;
};

/* Used for time-based spells */
struct SpellAreaEventObj {
    int spell;
    int room;
    CharData *ch;
};

struct TrackInfo {
    sh_int speed;
    sh_int range;
    sh_int sense;
};
struct RecallEventObj {
    CharData *ch;
    int from_room;
    int room;
};

/* Used to undo wandering woods spell */
struct RoomUndoEventObj {
    int exit;
    int room;
    int connect_room;
};

/* Used for delayed spell damage effects */
struct DelayedCastEventObj {
    CharData *ch;
    CharData *victim;
    int spell;
    int room;
    int rounds;
    int routines;
    int wait;
    int skill;
    int savetype;
    bool sustained; /* Is the caster sustaining the spell (e.g., a chant) (true)
                       or does the spell continue on its own (false)? */
};

struct TrackDelayedEventObj {
    TrackInfo track;
    CharData *victim;
    CharData *ch;
    int track_room;
};

struct GravityEventObj {
    CharData *ch;
    ObjData *obj;
    int distance_fallen;
    int start_room;
};

struct HurtEventData {
    CharData *victim;
    CharData *attacker;
    int damage;
};

struct CommandEventData {
    CharData *ch;
    char *cmd;
};

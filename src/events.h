/***************************************************************************
 * $Id: events.h,v 1.28 2009/03/19 23:16:23 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: events.h                                      Part of FieryMUD  *
 *  Usage: structures and prototypes for events                            *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

/***************************************************************************
 *  Changes:                                                               *
 *      3/6/98 ejg:  Changed return type of EVENTFUNC from void to long.   *
 *                   Moved struct event definition to events.c.            *
 ***************************************************************************/


#ifndef __FIERY_EVENTS_H
#define __FIERY_EVENTS_H

#define EVENTFUNC(name) long (name)(void *event_obj)
/* Return values for EVENTFUNCs */
#define EVENT_FINISHED		(0)
#define EVENT_PREVENT_FREE_OBJ	(-1)
#define EVENT_FORCE_FREE_OBJ	(-2)

struct event {
  int num;
  EVENTFUNC(*func);
  void *event_obj;
  bool free_obj;
  struct q_element *q_el;
  struct event **eventlist, *next;
};


void sethurtevent(struct char_data *ch, struct char_data *vict, int dam);
void overweight_check(struct char_data *ch);

bool char_has_event(struct char_data *ch, int eventtype);
bool char_has_delayed_command(struct char_data *ch, const char *command);

/* function protos need by other modules */
void event_init(void);
struct event *event_create(int eventnum, EVENTFUNC(*func), void *event_obj,
      bool free_obj, struct event **list, long when);

void cancel_event(struct event *eventlist, int eventtype);
void event_cancel(struct event *event);
void event_process(void);
long event_time(struct event *event);
void event_free_all(void);
const char *eventname(struct event *e);
void cancel_event_list(struct event **list);
struct generic_event_data *mkgenericevent(struct char_data *ch, struct char_data *vict,
      struct obj_data *obj);
void delayed_command(struct char_data *ch, char *command, int delay, bool repeatable);

#define EVENT_AUTODOUSE          1
#define EVENT_CAMP               2
#define EVENT_HURT               3
#define EVENT_MOB_QUIT           4
#define EVENT_NAME_TIMEOUT       5
#define EVENT_RECALL             6
#define EVENT_ROOM_UNDO          7
#define EVENT_SPELL              8
#define EVENT_TRACK              9
#define EVENT_TRIGGER_WAIT       10
#define EVENT_SINK_AND_LOSE      11
#define EVENT_BATTLE_PARALYSIS   12
#define EVENT_CASTING            13
#define EVENT_REGEN_HP           14
#define EVENT_REGEN_MANA         15
#define EVENT_REGEN_MOVE         16
#define EVENT_MEM                17
#define EVENT_SCRIBE             18
#define EVENT_QUICK_AGGRO        19
#define EVENT_DIE                20
#define EVENT_RAGE               21
#define EVENT_EXTRACT            22  /* To extract a char, for whatever reason */
#define EVENT_GRAVITY            23
#define EVENT_COOLDOWN           24
#define EVENT_FULLPURGE          25
#define EVENT_OVERWEIGHT         26
#define EVENT_FALLTOGROUND       27
#define EVENT_COMMAND            28
#define EVENT_EDITOR_START       29
#define EVENT_GET_MONEY          30
/* Update MAX_EVENT to be last event value + 1, please */
#define MAX_EVENT                31

EVENTFUNC(extract_event);
EVENTFUNC(hurt_event);
EVENTFUNC(fullpurge_event);
EVENTFUNC(mobquit_event);
EVENTFUNC(overweight_event);
EVENTFUNC(falltoground_event);
EVENTFUNC(command_event);

/* DATA STRUCTURES FOR VARIOUS EVENTS */

/* These are often used for event_obj. */

struct generic_event_data
{
   struct char_data *ch;
   struct char_data *vict;
   struct obj_data *obj;
};

struct sink_and_lose
{
   struct obj_data *obj;
   int room;
};

/* name approval time out event */
struct name_timeout_event
{
  struct descriptor_data *d;
};

/* Used for time-based spells */
struct spell_area_event_obj {
    int spell;
    int room;
    struct char_data *ch;
};

struct track_info {
   sh_int speed;
   sh_int range;
   sh_int sense;
}; 
struct recall_event_obj {
  struct char_data* ch;
  int from_room; 
  int room;
};

/* Used to undo wandering woods spell */
struct room_undo_event_obj {
   int exit;
   int room;
   int connect_room;
};

/* Used for delayed spell damage effects */
struct delayed_cast_event_obj {
   struct char_data *ch;
   struct char_data *victim;
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

struct track_delayed_event_obj {
   struct track_info track;
   struct char_data *victim;
   struct char_data *ch;
   int track_room;
};

struct gravity_event_obj {
  struct char_data *ch;
  struct obj_data *obj;
  int distance_fallen;
  int start_room;
};

struct hurt_event_data {
   struct char_data *victim;
   struct char_data *attacker;
   int damage;
};

struct command_event_data {
   struct char_data *ch;
   char *cmd;
};

#endif

/***************************************************************************
 * $Log: events.h,v $
 * Revision 1.28  2009/03/19 23:16:23  myc
 * Added an event to extract money.
 *
 * Revision 1.27  2009/03/08 02:17:46  jps
 * Delete jail event struct.
 *
 * Revision 1.26  2009/02/11 17:03:39  myc
 * Adding start_editor event.
 *
 * Revision 1.25  2008/09/27 04:50:34  jps
 * Make delayed-cast spells sustained/non-sustained. The non-sustained ones go
 * on by themselves (a little) and don't need the caster to remain alert.
 *
 * Revision 1.24  2008/09/21 21:15:45  jps
 * Add functions to determine whether a character has a particular event or command.
 * delayed_command() takes a parameter that indicates whether it's ok for this
 * command to be in a character's event list multiple times.
 *
 * Revision 1.23  2008/09/14 03:49:47  jps
 * Add a command event
 *
 * Revision 1.22  2008/09/07 01:29:29  jps
 * Add events for falling while overweight, or for falling to the ground
 * in the same room when you lose fly.
 *
 * Revision 1.21  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.20  2008/06/20 20:21:57  jps
 * Added the fullpurge event.
 *
 * Revision 1.19  2008/04/05 20:41:38  jps
 * Adding a hurt event.
 *
 * Revision 1.18  2008/04/04 06:12:52  myc
 * Removed justice code.
 *
 * Revision 1.17  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.16  2008/02/09 21:07:50  myc
 * The event code will now only auto-free an event obj if it was
 * told to at event creation time.  However, you may also force
 * it to free or not to free by returning EVENT_PREVENT_FREE_OBJ
 * or EVENT_FORCE_FREE_OBJ from the event function.  Expanding
 * the spell_delayed_event_obj to cover more types of spells.
 *
 * Revision 1.15  2008/02/09 18:29:11  myc
 * The event code now handles freeing of event objects.  If for
 * some reason you must free the object in the event function (or
 * you just don't want the event object freed), you must return
 * EVENT_NO_FREE_OBJ from the event instead of EVENT_FINISHED.
 *
 * Revision 1.14  2008/01/30 19:20:57  myc
 * Gravity is now an event.
 *
 * Revision 1.13  2008/01/28 02:38:50  jps
 * Add extract event.
 *
 * Revision 1.12  2008/01/27 21:09:12  myc
 * Added rage event.
 *
 * Revision 1.11  2008/01/26 12:29:02  jps
 * Made death into an event.
 *
 * Revision 1.10  2008/01/16 04:12:00  myc
 * Adding quick-aggro event.
 *
 * Revision 1.9  2008/01/09 04:13:05  jps
 * Add event types for memorizing and scribing, and function cancel_event().
 *
 * Revision 1.8  2008/01/09 01:51:01  jps
 * Create three regen events, getting rid of old points event.
 *
 * Revision 1.7  2008/01/04 04:29:31  jps
 * Made spellcasting into an event.
 *
 * Revision 1.6  2008/01/01 07:32:56  jps
 * Made cold-spell freeze-ups into an event so that it can set
 * paralysis which won't be removed immediately after it is set.
 *
 * Revision 1.5  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.4  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.3  2000/11/21 04:53:41  rsd
 * Altered the comment header and added the missing initial
 * rlog message.
 *
 * Revision 1.2  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

/***************************************************************************
 * $Id: events.c,v 1.49 2009/03/19 23:16:23 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: events.c                                      Part of FieryMUD  *
 *  Usage: Contains routines to handle events                              *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "events.h"

#include "casting.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "movement.h"
#include "queue.h"
#include "regen.h"
#include "screen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* external variables */
extern unsigned long pulse;

struct queue *event_q; /* the event queue */
int processing_events = FALSE;

const char *eventnames[MAX_EVENT + 1] = {"!INVALID EVENT!", /* 0 - reserved */
                                         "autodouse",       /* 1 */
                                         "camp",
                                         "hurt",
                                         "mob_quit",
                                         "name_timeout", /* 5 */
                                         "recall",
                                         "room_undo",
                                         "spell",
                                         "track",
                                         "trigger_wait", /* 10 */
                                         "sink_and_lose",
                                         "battle_paralysis",
                                         "casting",
                                         "regen_hp",
                                         "regen_mana", /* 15 */
                                         "regen_move",
                                         "memming",
                                         "scribing",
                                         "quick_aggro",
                                         "die", /* 20 */
                                         "rage",
                                         "extract",
                                         "gravity",
                                         "cooldown",
                                         "fullpurge", /* 25 */
                                         "overweight",
                                         "falltoground",
                                         "command",
                                         "start_editor",
                                         "get_money", /* 30 */
                                         "\n"};

/*************************************************************************/
/*                        EVENT UTILITY FUNCTIONS                        */
/*************************************************************************/

bool char_has_event(struct char_data *ch, int eventtype) {
    struct event *e;

    for (e = ch->events; e; e = e->next)
        if (e->num == eventtype)
            return TRUE;
    return FALSE;
}

bool char_has_delayed_command(struct char_data *ch, const char *command) {
    struct event *e;

    for (e = ch->events; e; e = e->next)
        if (e->num == EVENT_COMMAND && e->event_obj &&
            !strcmp(command, ((struct command_event_data *)(e->event_obj))->cmd))
            return TRUE;
    return FALSE;
}

/*************************************************************************/
/*                            EVENT HANDLERS                             */
/*************************************************************************/

/* BATTLE_PARALYSIS
 *
 * Someone has experienced a paralyzing event in battle.
 *
 * This is handled as an event, because if you set any paralysis flag in
 * the middle of a combat round, other offensive occurrences during that
 * round might remove the flag right away.
 */

EVENTFUNC(battle_paralysis_handler) {
    struct effect eff;
    struct char_data *vict = ((struct generic_event_data *)event_obj)->vict;

    if (FIGHTING(vict))
        stop_fighting(vict);
    stop_attackers(vict);

    memset(&eff, 0, sizeof(eff));
    eff.type = SPELL_MINOR_PARALYSIS;
    eff.modifier = 0;
    eff.location = 0;
    eff.duration = 2;
    SET_FLAG(eff.flags, EFF_MINOR_PARALYSIS);
    effect_to_char(vict, &eff);

    return EVENT_FINISHED;
}

/* CASTING
 *
 * Someone's casting a spell.  They might finish now, or this event might
 * be re-queued to let them cast some more.
 */

EVENTFUNC(casting_handler) {
    struct char_data *ch = (struct char_data *)event_obj;
    struct obj_data *obj;
    int i;
    char castbuf[256];
    int tar_invalid = FALSE, found = FALSE;

    void abort_casting(struct char_data * ch);
    void complete_spell(struct char_data * ch);

    if (!CASTING(ch))
        return EVENT_FINISHED;

    /* Prevent player casting loops */
    if (!IS_SPELL(ch->casting.spell)) {
        STOP_CASTING(ch);
        sprintf(castbuf, "SYSERR: removed casting loop on %s", GET_NAME(ch));
        log(castbuf);
        return EVENT_FINISHED;
    }

    /* Check to make sure target is still valid! */
    if (ch->casting.obj) { /* target is object */
        switch (ch->casting.target_status) {
        case TARGET_IN_ROOM:
            if (ch->casting.obj->in_room != ch->in_room)
                tar_invalid = TRUE;
            break;
        case TARGET_IN_WORLD:
            break;
        case TARGET_IN_INV:
            for (obj = ch->carrying; obj; obj = obj->next_content)
                if (ch->casting.obj == obj)
                    found = TRUE;
            if (!found)
                tar_invalid = TRUE;
            break;
        case TARGET_EQUIP:
            for (i = 0; i < NUM_WEARS; ++i)
                if (ch->casting.obj == ch->equipment[i])
                    found = TRUE;
            if (!found)
                tar_invalid = TRUE;
            break;
        default:
            sprintf(castbuf, "SYSERR: Error in casting_handler() at obj valid check for spell %d.", ch->casting.spell);
            log(castbuf);
        }
    } else if (ch->casting.tch) { /* target is a char */
        switch (ch->casting.target_status) {
        case TARGET_IN_ROOM:
            if (ch->casting.tch->in_room != ch->in_room)
                tar_invalid = TRUE;
            break;
        case TARGET_IN_WORLD:
            break;
        case TARGET_FIGHTING:
            if (!FIGHTING(ch) || ch->casting.tch != FIGHTING(ch))
                tar_invalid = TRUE;
            break;
        case TARGET_SELF:
            break;
        default:
            sprintf(castbuf,
                    "SYSERR: Error in casting_handler() at char valid check for "
                    "spell %d.",
                    ch->casting.spell);
            log(castbuf);
        }
    }

    if (tar_invalid) {
        abort_casting(ch);
        return EVENT_FINISHED;
    }

    /* Finished yet? */
    if (ch->casting.casting_time <= 0) {
        STOP_CASTING(ch);
        complete_spell(ch);
        return EVENT_FINISHED;
    } else {
        sprintf(castbuf, "Casting: %s ", skill_name(ch->casting.spell));

        for (i = 1; i <= ch->casting.casting_time; i += 2)
            strcat(castbuf, "*");
        strcat(castbuf, "\r\n");
        send_to_char(castbuf, ch);
    }

    ch->casting.casting_time -= 2;

    /* The time between casting updates can vary by +/- 2 ticks. */
    return PULSE_VIOLENCE / 2 - 2 + number(0, 4);
}

/* Yep, you're gonna die.
 *
 * In order to be able to interact with the data structure of a doomed
 * character as it dies, we delay the actual death with an event.
 *
 * This is the ONLY place in the entire server that should call perform_die().
 */

EVENTFUNC(die_event) {
    struct char_data *ch = ((struct generic_event_data *)event_obj)->ch;
    struct char_data *killer = ((struct generic_event_data *)event_obj)->vict;

    if (event_target_valid(killer))
        perform_die(ch, killer);
    else
        perform_die(ch, 0);

    return EVENT_FINISHED;
}

/* Hurt
 *
 * You're going to receive damage.  To the hit points.  You might die.
 */

EVENTFUNC(hurt_event) {
    struct char_data *victim = ((struct hurt_event_data *)event_obj)->victim;
    struct char_data *attacker = ((struct hurt_event_data *)event_obj)->attacker;
    int dam = ((struct hurt_event_data *)event_obj)->damage;

    hurt_char(victim, attacker, dam, TRUE);
    return EVENT_FINISHED;
}

/* Extract
 *
 * Extracts a mob from the game.
 */

EVENTFUNC(extract_event) {
    struct char_data *ch = (struct char_data *)event_obj;

    extract_char(ch);

    return EVENT_FINISHED;
}

void sethurtevent(struct char_data *ch, struct char_data *vict, int dam) {
    struct hurt_event_data *he;

    CREATE(he, struct hurt_event_data, 1);
    he->victim = vict;
    he->attacker = ch;
    he->damage = dam;
    event_create(EVENT_HURT, hurt_event, he, TRUE, &(vict->events), 0);
}

/* Fullpurge
 *
 * Extracts a mob from the game and its objects, too.
 */

void fullpurge_char(struct char_data *ch) {
    event_create(EVENT_FULLPURGE, fullpurge_event, ch, FALSE, &(ch->events), 0);
}

EVENTFUNC(fullpurge_event) {
    struct char_data *ch = (struct char_data *)event_obj;
    void purge_objs(struct char_data * ch);

    purge_objs(ch);
    extract_char(ch);

    return EVENT_FINISHED;
}

void overweight_check(struct char_data *ch) {
    if (GET_POS(ch) == POS_FLYING && too_heavy_to_fly(ch) && !EVENT_FLAGGED(ch, EVENT_OVERWEIGHT))
        event_create(EVENT_OVERWEIGHT, overweight_event, ch, FALSE, &(ch->events), 0);
}

EVENTFUNC(overweight_event) {
    struct char_data *ch = (struct char_data *)event_obj;

    /* Prerequisites:
     * - you are flying
     * - you are too heavy to fly
     */
    if (affected_by_spell(ch, SPELL_FLY))
        cprintf(ch, "The spell supporting you falters, unable to bear your weight!\r\n");
    else
        cprintf(ch, "You cannot fly with so much weight!\r\n");
    if (SECT(IN_ROOM(ch)) == SECT_AIR) {
        alter_pos(ch, POS_STANDING, GET_STANCE(ch));
    } else {
        if (IS_SPLASHY(IN_ROOM(ch))) {
            cprintf(ch, "You fall into the water with a splash!\r\n");
            act("$n falls into the water with a splash.", FALSE, ch, 0, 0, TO_ROOM);
        } else {
            cprintf(ch, "You fall down!\r\n");
            act("$n falls to the ground!", FALSE, ch, 0, 0, TO_ROOM);
        }
        alter_pos(ch, POS_SITTING, GET_STANCE(ch));
    }

    return EVENT_FINISHED;
}

EVENTFUNC(falltoground_event) {
    struct char_data *ch = (struct char_data *)event_obj;
    /* Prerequisites:
     *  - Your position is POS_FLYING
     *  - You do not have EFF_FLY
     *  - You are not in an air room
     *
     * You just lost fly, maybe from the spell wearing off or from removing
     * a piece of equipment that conferred fly. You were flying in an ordinary
     * room. You will fall to the ground. */

    if (IS_SPLASHY(IN_ROOM(ch))) {
        cprintf(ch, "You fall into the water with a splash!\r\n");
        act("$n falls into the water with a splash.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        cprintf(ch, "You fall to the ground.\r\n");
        act("$n falls to the ground.", FALSE, ch, 0, 0, TO_ROOM);
    }
    alter_pos(ch, POS_STANDING, GET_STANCE(ch));
    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_FALLTOGROUND);

    return EVENT_FINISHED;
}

void delayed_command(struct char_data *ch, char *command, int delay, bool repeatable) {
    struct command_event_data *ce;

    if (!repeatable && char_has_delayed_command(ch, command))
        return;

    CREATE(ce, struct command_event_data, 1);
    ce->ch = ch;
    ce->cmd = strdup(command);
    event_create(EVENT_COMMAND, command_event, ce, TRUE, &(ch->events), delay);
}

void free_command_event_data(void *e) {
    free(((struct command_event_data *)e)->cmd);
    free(e);
}

EVENTFUNC(command_event) {
    struct command_event_data *ce = (struct command_event_data *)event_obj;

    command_interpreter(ce->ch, ce->cmd);

    return EVENT_FINISHED;
}

/*************************************************************************/
/*                         EVENT INFRASTRUCTURE                          */
/*************************************************************************/

/* initializes the event queue */
void event_init(void) { event_q = queue_init(); }

/* creates an event and returns it */
struct event *event_create(int eventnum, EVENTFUNC(*func), void *event_obj, bool free_obj, struct event **list,
                           long when) {
    struct event *new_event;

    CREATE(new_event, struct event, 1);
    new_event->num = eventnum;
    new_event->func = func;
    new_event->event_obj = event_obj;
    new_event->free_obj = free_obj;
    new_event->q_el = queue_enq(event_q, new_event, when + pulse);

    /* Add this event to the provided event list (if any) */
    if (list) {
        new_event->eventlist = list;
        if (*list)
            new_event->next = *list;
        *list = new_event;
    }

    return new_event;
}

/* Frees an event obj */
void free_event_obj(struct event *event) {
    if (event->event_obj) {
        if (event->num == EVENT_COMMAND)
            free_command_event_data(event->event_obj);
        else
            free(event->event_obj);
        event->event_obj = NULL;
    }
}

/* removes the event from the system */
void event_cancel(struct event *event) {
    struct event *temp;

    if (!event) {
        log("SYSERR:  Attempted to cancel a NULL event");
        return;
    }

    if (event->num == EVENT_GRAVITY)
        printf("DELETING GRAVITY EVENT\n");
    queue_deq(event_q, event->q_el);

    /* Remove it from its event list (if any) */
    if (event->eventlist) {
        REMOVE_FROM_LIST(event, *(event->eventlist), next);
    }

    if (event->free_obj && event->event_obj)
        free_event_obj(event);
    free(event);
}

/* removes an event based on type */
void cancel_event(struct event *eventlist, int eventtype) {
    while (eventlist && eventlist->num != eventtype)
        eventlist = eventlist->next;
    if (eventlist)
        event_cancel(eventlist);
}

/* Process any events whose time has come. */
void event_process(void) {
    struct event *the_event, *temp;
    struct event **list;
    long new_time;

    while ((long)pulse >= queue_key(event_q)) {
        if (!(the_event = (struct event *)queue_head(event_q))) {
            log("SYSERR: Attempt to get a NULL event");
            return;
        }

        /* Remove this event from any eventlist it may be in, now.
         * Then the event will not be in any list during its execution, which
         * occurs next. Therefore, the object/character whose list this event
         * is in may be destroyed during the event without adverse consequences. */
        if (the_event->eventlist) {
            list = the_event->eventlist;
            REMOVE_FROM_LIST(the_event, *(the_event->eventlist), next);
        } else {
            list = NULL;
        }

        /* call event func, reenqueue event if retval > 0 */
        if ((new_time = (the_event->func)(the_event->event_obj)) > 0) {
            the_event->q_el = queue_enq(event_q, the_event, new_time + pulse);
            /* Re-add it to the list. */
            if (list) {
                the_event->eventlist = list;
                if (*list)
                    the_event->next = *list;
                *list = the_event;
            }
        } else {
            if ((the_event->free_obj && new_time != EVENT_PREVENT_FREE_OBJ) || new_time == EVENT_FORCE_FREE_OBJ)
                free_event_obj(the_event);
            free(the_event);
        }
    }
}

/* returns the time remaining before the event */
long event_time(struct event *event) {
    long when;

    when = queue_elmt_key(event->q_el);

    return (when - pulse);
}

/* frees all events in the queue */
void event_free_all(void) {
    struct event *the_event;

    while ((the_event = (struct event *)queue_head(event_q))) {
        if (the_event->free_obj && the_event->event_obj)
            free_event_obj(the_event);
        free(the_event);
    }

    queue_free(event_q);
}

/* Cancel all events in a list. Probably because the character or object
 * that owns the list is being extracted. */
void cancel_event_list(struct event **list) {
    struct event *e, *next_e;

    e = *list;
    while (e) {
        next_e = e->next;
        e->next = NULL;
        e->eventlist = NULL; /* Prevents cancel_event() from messing with the list */
        event_cancel(e);
        e = next_e;
    }
    *list = NULL;
}

/* should be removed eventually so that event lists are attached to
 *  char_data struct and removed form queue when the character is removed
 * check to make sure the target of any timed event is still valid */
bool event_target_valid(struct char_data *ch) {
    struct char_data *current = NULL;

    for (current = character_list; current; current = current->next) {
        /* loop through current valid players and see if the ADDRESS of the valid
         * player is the same as the event target...if it is, then we have a valid
         * target.
         */
        if (ch == current)
            return TRUE;
    }

    /* no valid target found...return; */
    return FALSE;
}

const char *eventname(struct event *e) {
    if (e->num < 1 || e->num > MAX_EVENT)
        return eventnames[0];
    return eventnames[e->num];
}

struct generic_event_data *mkgenericevent(struct char_data *ch, struct char_data *vict, struct obj_data *obj) {
    struct generic_event_data *d;

    CREATE(d, struct generic_event_data, 1);
    d->ch = ch;
    d->vict = vict;
    d->obj = obj;
    return d;
}

/***************************************************************************
 * $Log: events.c,v $
 * Revision 1.49  2009/03/19 23:16:23  myc
 * Added an event to extract money.
 *
 * Revision 1.48  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.47  2009/02/11 17:03:39  myc
 * Adding start_editor event.
 *
 * Revision 1.46  2008/09/26 09:39:21  jps
 * Removing core drops for quick aggro events.
 *
 * Revision 1.45  2008/09/26 09:23:33  jps
 * Add debugging for the first 3 quick aggro events.
 *
 * Revision 1.44  2008/09/21 21:20:53  jps
 * Remove debugging syslog message.
 *
 * Revision 1.43  2008/09/21 21:15:45  jps
 * Add functions to determine whether a character has a particular event or
 *command. delayed_command() takes a parameter that indicates whether it's ok
 *for this command to be in a character's event list multiple times.
 *
 * Revision 1.42  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.41  2008/09/14 03:52:36  jps
 * Added command event. Made a function for freeing event objs.
 *
 * Revision 1.40  2008/09/07 01:29:29  jps
 * Add events for falling while overweight, or for falling to the ground
 * in the same room when you lose fly.
 *
 * Revision 1.39  2008/08/29 04:16:26  myc
 * Moved all gravity functions directly to act.movement.c so that
 * we don't need a reference to act.h in this file.  The events
 * implementation really doesn't belong mixed up with the events
 * system subscribers.
 *
 * Revision 1.38  2008/06/21 09:01:13  myc
 * Improve casting error message.
 *
 * Revision 1.37  2008/06/20 20:21:57  jps
 * Added the fullpurge event.
 *
 * Revision 1.36  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.35  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.34  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.33  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.32  2008/05/14 05:10:19  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.31  2008/05/11 05:49:12  jps
 * alter_hit() now takes care of all morted/death messages, and dying.
 *
 * Revision 1.30  2008/04/14 05:11:40  jps
 * Renamed EFF_FLYING to EFF_FLY, since it only indicates an ability
 * to fly - not that the characer is actually flying.
 *
 * Revision 1.29  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.28  2008/04/05 21:42:32  jps
 * Text cleanup
 *
 * Revision 1.27  2008/04/05 20:43:12  jps
 * Adding hurt event.
 * Now if you add an event with 0 delay, it will actually happen immediately.
 * Sometimes you need an event to happen right away.
 *
 * Revision 1.26  2008/04/04 06:12:52  myc
 * Removed justice code.
 *
 * Revision 1.25  2008/04/02 03:24:44  myc
 * Removed unnecessary function declaration.
 *
 * Revision 1.24  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.23  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.22  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.21  2008/02/09 21:07:50  myc
 * Getting rid of the spell_area event and merging it into the
 * spell (delayed_cast) event.  The casting and extract events
 * now just get passed the char, instead of a whole event obj.
 * The event code will now only auto-free an event obj if it
 * was told to at event creation time.
 *
 * Revision 1.20  2008/02/09 18:29:11  myc
 * The event code now handles freeing of event objects.  If for
 * some reason you must free the object in the event function (or
 * you just don't want the event object freed), you must return
 * EVENT_NO_FREE_OBJ from the event instead of EVENT_FINISHED.
 *
 * Revision 1.19  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.18  2008/01/30 19:20:57  myc
 * Gravity is now an event.
 *
 * Revision 1.17  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.16  2008/01/28 02:38:37  jps
 * Add extract_event.
 *
 * Revision 1.15  2008/01/27 21:09:12  myc
 * Added rage event.
 *
 * Revision 1.14  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.13  2008/01/26 12:29:02  jps
 * Made death into an event.
 *
 * Revision 1.12  2008/01/16 04:12:00  myc
 * Adding quick-aggro event.
 *
 * Revision 1.11  2008/01/09 04:14:46  jps
 * Add events for memming and scribing spells. Add function cancel_event().
 *
 * Revision 1.10  2008/01/09 01:52:14  jps
 * Get rid of points events and create three new regen events.
 *
 * Revision 1.9  2008/01/04 04:31:13  jps
 * Made spellcasting into an event.
 *
 * Revision 1.8  2008/01/01 07:32:56  jps
 * Made cold-spell freeze-ups into an event so that it can set
 * paralysis which won't be removed immediately after it is set.
 *
 * Revision 1.7  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.6  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.5  2000/11/21 04:49:28  rsd
 * Altered the comment header and added back rlog messages
 * that were left out prior to the addition of the $log$
 * string.
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/02/08 23:01:47  jimmy
 * Fixed mortally wounded bug.  Mortally wounded
 * victims now die when they read -11.  Also,
 * no more "attempt to damage corpse"
 * fingon
 *
 * Revision 1.2  1999/01/31 02:16:56  mud
 * Altereed comment header
 * indented file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

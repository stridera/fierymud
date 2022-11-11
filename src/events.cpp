/***************************************************************************
 *   File: events.c                                      Part of FieryMUD  *
 *  Usage: Contains routines to handle events                              *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "events.hpp"

#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "queue.hpp"
#include "regen.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

Queue *event_q; /* the event queue */
int processing_events = false;

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

bool char_has_event(CharData *ch, int eventtype) {
    Event *e;

    for (e = ch->events; e; e = e->next)
        if (e->num == eventtype)
            return true;
    return false;
}

bool char_has_delayed_command(CharData *ch, const char *command) {
    Event *e;

    for (e = ch->events; e; e = e->next)
        if (e->num == EVENT_COMMAND && e->event_obj && !strcmp(command, ((CommandEventData *)(e->event_obj))->cmd))
            return true;
    return false;
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
    effect eff;
    CharData *vict = ((GenericEventData *)event_obj)->vict;

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
    CharData *ch = (CharData *)event_obj;
    ObjData *obj;
    int i;
    char castbuf[256];
    int tar_invalid = false, found = false;

    void abort_casting(CharData * ch);
    void complete_spell(CharData * ch);

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
                tar_invalid = true;
            break;
        case TARGET_IN_WORLD:
            break;
        case TARGET_IN_INV:
            for (obj = ch->carrying; obj; obj = obj->next_content)
                if (ch->casting.obj == obj)
                    found = true;
            if (!found)
                tar_invalid = true;
            break;
        case TARGET_EQUIP:
            for (i = 0; i < NUM_WEARS; ++i)
                if (ch->casting.obj == ch->equipment[i])
                    found = true;
            if (!found)
                tar_invalid = true;
            break;
        default:
            sprintf(castbuf, "SYSERR: Error in casting_handler() at obj valid check for spell %d.", ch->casting.spell);
            log(castbuf);
        }
    } else if (ch->casting.tch) { /* target is a char */
        switch (ch->casting.target_status) {
        case TARGET_IN_ROOM:
            if (ch->casting.tch->in_room != ch->in_room)
                tar_invalid = true;
            break;
        case TARGET_IN_WORLD:
            break;
        case TARGET_FIGHTING:
            if (!FIGHTING(ch) || ch->casting.tch != FIGHTING(ch))
                tar_invalid = true;
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
    CharData *ch = ((GenericEventData *)event_obj)->ch;
    CharData *killer = ((GenericEventData *)event_obj)->vict;

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
    CharData *victim = ((HurtEventData *)event_obj)->victim;
    CharData *attacker = ((HurtEventData *)event_obj)->attacker;
    int dam = ((HurtEventData *)event_obj)->damage;

    hurt_char(victim, attacker, dam, true);
    return EVENT_FINISHED;
}

/* Extract
 *
 * Extracts a mob from the game.
 */

EVENTFUNC(extract_event) {
    CharData *ch = (CharData *)event_obj;

    extract_char(ch);

    return EVENT_FINISHED;
}

void sethurtevent(CharData *ch, CharData *vict, int dam) {
    HurtEventData *he;

    CREATE(he, HurtEventData, 1);
    he->victim = vict;
    he->attacker = ch;
    he->damage = dam;
    event_create(EVENT_HURT, hurt_event, he, true, &(vict->events), 0);
}

/* Fullpurge
 *
 * Extracts a mob from the game and its objects, too.
 */

void fullpurge_char(CharData *ch) { event_create(EVENT_FULLPURGE, fullpurge_event, ch, false, &(ch->events), 0); }

EVENTFUNC(fullpurge_event) {
    CharData *ch = (CharData *)event_obj;
    void purge_objs(CharData * ch);

    purge_objs(ch);
    extract_char(ch);

    return EVENT_FINISHED;
}

void overweight_check(CharData *ch) {
    if (GET_POS(ch) == POS_FLYING && too_heavy_to_fly(ch) && !EVENT_FLAGGED(ch, EVENT_OVERWEIGHT))
        event_create(EVENT_OVERWEIGHT, overweight_event, ch, false, &(ch->events), 0);
}

EVENTFUNC(overweight_event) {
    CharData *ch = (CharData *)event_obj;

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
            act("$n falls into the water with a splash.", false, ch, 0, 0, TO_ROOM);
        } else {
            cprintf(ch, "You fall down!\r\n");
            act("$n falls to the ground!", false, ch, 0, 0, TO_ROOM);
        }
        alter_pos(ch, POS_SITTING, GET_STANCE(ch));
    }

    return EVENT_FINISHED;
}

EVENTFUNC(falltoground_event) {
    CharData *ch = (CharData *)event_obj;
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
        act("$n falls into the water with a splash.", false, ch, 0, 0, TO_ROOM);
    } else {
        cprintf(ch, "You fall to the ground.\r\n");
        act("$n falls to the ground.", false, ch, 0, 0, TO_ROOM);
    }
    alter_pos(ch, POS_STANDING, GET_STANCE(ch));
    REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_FALLTOGROUND);

    return EVENT_FINISHED;
}

void delayed_command(CharData *ch, char *command, int delay, bool repeatable) {
    CommandEventData *ce;

    if (!repeatable && char_has_delayed_command(ch, command))
        return;

    CREATE(ce, CommandEventData, 1);
    ce->ch = ch;
    ce->cmd = strdup(command);
    event_create(EVENT_COMMAND, command_event, ce, true, &(ch->events), delay);
}

void free_command_event_data(void *e) {
    free(((CommandEventData *)e)->cmd);
    free(e);
}

EVENTFUNC(command_event) {
    CommandEventData *ce = (CommandEventData *)event_obj;

    command_interpreter(ce->ch, ce->cmd);

    return EVENT_FINISHED;
}

/*************************************************************************/
/*                         EVENT INFRASTRUCTURE                          */
/*************************************************************************/

/* initializes the event queue */
void event_init(void) { event_q = queue_init(); }

/* creates an event and returns it */
Event *event_create(int eventnum, EVENTFUNC(*func), void *event_obj, bool free_obj, Event **list, long when) {
    Event *new_event;

    CREATE(new_event, Event, 1);
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
void free_event_obj(Event *event) {
    if (event->event_obj) {
        if (event->num == EVENT_COMMAND)
            free_command_event_data(event->event_obj);
        else
            free(event->event_obj);
        event->event_obj = nullptr;
    }
}

/* removes the event from the system */
void event_cancel(Event *event) {
    Event *temp;

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
void cancel_event(Event *eventlist, int eventtype) {
    while (eventlist && eventlist->num != eventtype)
        eventlist = eventlist->next;
    if (eventlist)
        event_cancel(eventlist);
}

/* Process any events whose time has come. */
void event_process(void) {
    Event *the_event, *temp;
    Event **list;
    long new_time;

    while ((long)pulse >= queue_key(event_q)) {
        if (!(the_event = (Event *)queue_head(event_q))) {
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
            list = nullptr;
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
long event_time(Event *event) {
    long when;

    when = queue_elmt_key(event->q_el);

    return (when - pulse);
}

/* frees all events in the queue */
void event_free_all(void) {
    Event *the_event;

    while ((the_event = (Event *)queue_head(event_q))) {
        if (the_event->free_obj && the_event->event_obj)
            free_event_obj(the_event);
        free(the_event);
    }

    queue_free(event_q);
}

/* Cancel all events in a list. Probably because the character or object
 * that owns the list is being extracted. */
void cancel_event_list(Event **list) {
    Event *e, *next_e;

    e = *list;
    while (e) {
        next_e = e->next;
        e->next = nullptr;
        e->eventlist = nullptr; /* Prevents cancel_event() from messing with the list */
        event_cancel(e);
        e = next_e;
    }
    *list = nullptr;
}

/* should be removed eventually so that event lists are attached to
 *  CharData  and removed form queue when the character is removed
 * check to make sure the target of any timed event is still valid */
bool event_target_valid(CharData *ch) {
    CharData *current = nullptr;

    for (current = character_list; current; current = current->next) {
        /* loop through current valid players and see if the ADDRESS of the valid
         * player is the same as the event target...if it is, then we have a valid
         * target.
         */
        if (ch == current)
            return true;
    }

    /* no valid target found...return; */
    return false;
}

const char *eventname(Event *e) {
    if (e->num < 1 || e->num > MAX_EVENT)
        return eventnames[0];
    return eventnames[e->num];
}

GenericEventData *mkgenericevent(CharData *ch, CharData *vict, ObjData *obj) {
    GenericEventData *d;

    CREATE(d, GenericEventData, 1);
    d->ch = ch;
    d->vict = vict;
    d->obj = obj;
    return d;
}

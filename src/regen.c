/***************************************************************************
 * $Id: regen.c,v 1.41 2009/03/08 23:34:14 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: regen.c                                       Part of FieryMUD  *
 *  Usage: Contains routines to handle event based point regeneration      *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "regen.h"

#include "ai.h"
#include "casting.h"
#include "comm.h"
#include "conf.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "limits.h"
#include "math.h"
#include "races.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

void improve_skill(struct char_data *ch, int skill);
void stop_berserking(struct char_data *ch);
void start_berserking(struct char_data *ch);

#define PULSES_PER_MUD_HOUR (SECS_PER_MUD_HOUR * PASSES_PER_SEC)

EVENTFUNC(hp_regen_event) {
    struct char_data *ch = (struct char_data *)event_obj;
    int gain, delay = 0;
    bool is_dying = FALSE;

    void slow_death(struct char_data * victim);

    if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
        if (GET_STANCE(ch) <= STANCE_STUNNED) {
            if (DAMAGE_WILL_KILL(ch, 1)) {
                /* In slow_death(), the character may die. */
                slow_death(ch);
            } else
                hurt_char(ch, NULL, 1, TRUE);
            is_dying = TRUE;
        } else
            hurt_char(ch, NULL, -1, TRUE);

        if (GET_HIT(ch) < GET_MAX_HIT(ch) && !DECEASED(ch)) {
            if (is_dying)
                delay = (PULSES_PER_MUD_HOUR / 4);
            else {
                gain = hit_gain(ch);
                delay = gain < 1 || gain > PULSES_PER_MUD_HOUR ? 1 : PULSES_PER_MUD_HOUR / gain;
            }
        }
    }

    if (!delay)
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_HP);
    return delay;
}

EVENTFUNC(mana_regen_event) {
    struct char_data *ch = (struct char_data *)event_obj;
    int gain, delay = 0;

    if (GET_MANA(ch) < GET_MAX_MANA(ch)) {
        gain = mana_gain(ch);
        /* Mana not actually used so we don't increase it */
        delay = (gain < 1 || gain > PULSES_PER_MUD_HOUR ? 1 : PULSES_PER_MUD_HOUR / gain);
        delay = 0; /* Mana not actually used so cancel the event */
    }

    if (!delay)
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_MANA);
    return delay;
}

EVENTFUNC(move_regen_event) {
    struct char_data *ch = (struct char_data *)event_obj;
    int gain, delay = 0;

    if (GET_MOVE(ch) < GET_MAX_MOVE(ch)) {
        gain = move_gain(ch);
        if (GET_STANCE(ch) >= STANCE_SLEEPING && GET_POS(ch) < POS_STANDING)
            GET_MOVE(ch) = MIN(GET_MOVE(ch) + 1, GET_MAX_MOVE(ch));
        else
            GET_MOVE(ch) = MIN(GET_MOVE(ch) + 1, GET_MAX_MOVE(ch));

        delay = gain < 1 || gain > PULSES_PER_MUD_HOUR ? 1 : PULSES_PER_MUD_HOUR / gain;
    }

    if (!delay)
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_MOVE);
    return delay;
}

EVENTFUNC(rage_event) {
    struct char_data *ch = (struct char_data *)event_obj;

    /* Berserking: diminish rage quickly. */
    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        if (EFF_FLAGGED(ch, EFF_WRATH))
            GET_RAGE(ch) -= number(10, 15);
        else
            GET_RAGE(ch) -= number(20, 30);
        if (GET_RAGE(ch) % 7 == 0 && FIGHTING(ch))
            improve_skill(ch, SKILL_BERSERK);
        if (!FIGHTING(ch))
            event_create(EVENT_QUICK_AGGRO, quick_aggro_event, mkgenericevent(ch, find_aggr_target(ch), 0), TRUE,
                         &(ch->events), 0);
    }

    /* Meditating: increase rage quickly. */
    else if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        GET_RAGE(ch) += number(10, GET_SKILL(ch, SKILL_MEDITATE));
        if (GET_RAGE(ch) % 5 == 0)
            improve_skill(ch, SKILL_MEDITATE);
    }

    /* Otherwise diminish rage slowly. */
    else
        GET_RAGE(ch) -= number(2, 4);

    /* When you reach crazed rage, you are forced to start berserking. */
    if (GET_RAGE(ch) > RAGE_CRAZED && !EFF_FLAGGED(ch, EFF_BERSERK)) {
        if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
            GET_POS(ch) = POS_STANDING;
        }
        start_berserking(ch);
        send_to_char("&1&8Your rage consumes you, taking control of your body...&0\r\n", ch);
        if (IN_ROOM(ch) != NOWHERE)
            act("$n shudders as $s rage causes $m to go berserk!", TRUE, ch, 0, 0, TO_ROOM);
    }

    if (GET_RAGE(ch) > 0)
        return 4 * PASSES_PER_SEC;
    else {
        send_to_char("Your rage recedes and you feel calmer.\r\n", ch);
        stop_berserking(ch);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_RAGE);
        return EVENT_FINISHED;
    }
}

void set_regen_event(struct char_data *ch, int eventtype) {
    long time;
    int gain;

    if (EVENT_FLAGGED(ch, eventtype))
        return;

    if (eventtype == EVENT_REGEN_HP && !EVENT_FLAGGED(ch, EVENT_REGEN_HP) && GET_HIT(ch) < GET_MAX_HIT(ch)) {
        gain = hit_gain(ch);
        time = PULSES_PER_MUD_HOUR / (gain ? gain : 1);
        event_create(EVENT_REGEN_HP, hp_regen_event, ch, FALSE, &(ch->events), time);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_HP);
    }
    if (eventtype == EVENT_REGEN_MANA && !EVENT_FLAGGED(ch, EVENT_REGEN_MANA) && GET_MANA(ch) < GET_MAX_MANA(ch) &&
        FALSE) {
        gain = mana_gain(ch);
        time = PULSES_PER_MUD_HOUR / (gain ? gain : 1);
        event_create(EVENT_REGEN_HP, mana_regen_event, ch, FALSE, &(ch->events), time);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_MANA);
    }
    if (eventtype == EVENT_REGEN_MOVE && !EVENT_FLAGGED(ch, EVENT_REGEN_MOVE) && GET_MOVE(ch) < GET_MAX_MOVE(ch)) {
        gain = move_gain(ch);
        time = PULSES_PER_MUD_HOUR / (gain ? gain : 1);
        event_create(EVENT_REGEN_MOVE, move_regen_event, ch, FALSE, &(ch->events), time);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_MOVE);
    }
    if (eventtype == EVENT_RAGE && !EVENT_FLAGGED(ch, EVENT_RAGE) &&
        (GET_RAGE(ch) > 0 || (GET_SKILL(ch, SKILL_BERSERK) && PLR_FLAGGED(ch, PLR_MEDITATE)))) {
        event_create(EVENT_RAGE, rage_event, ch, FALSE, &(ch->events), 0);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_RAGE);
    }
}

/* subtracts amount of hitpoints from ch's current */

void alter_hit(struct char_data *ch, int amount, bool cap_amount) {
    int old_hit = 0;

    if (ch->in_room <= NOWHERE)
        return;

    if (cap_amount) {
        old_hit = GET_HIT(ch);

        GET_HIT(ch) -= amount;

        if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
            if (old_hit < GET_MAX_HIT(ch))
                GET_HIT(ch) = GET_MAX_HIT(ch);
            else if (amount < 0)
                GET_HIT(ch) += amount;
        }
    } else
        GET_HIT(ch) -= amount;
}

/* Modifies HP, causes dying/falling unconscious/recovery.
 * Starts regeneration event if necessary. */
void hurt_char(struct char_data *ch, struct char_data *attacker, int amount, bool cap_amount) {
    if (IN_ROOM(ch) == NOWHERE)
        abort();
    alter_hit(ch, amount, cap_amount);

    hp_pos_check(ch, attacker, amount);

    if (GET_HIT(ch) > HIT_INCAP && GET_HIT(ch) < GET_MAX_HIT(ch) && !EVENT_FLAGGED(ch, EVENT_REGEN_HP))
        set_regen_event(ch, EVENT_REGEN_HP);
}

/* subtracts amount of mana from ch's current and starts points event */
void alter_mana(struct char_data *ch, int amount) {
    if (ch->in_room <= NOWHERE)
        return;

    GET_MANA(ch) = MIN(GET_MANA(ch) - amount, GET_MAX_MANA(ch));

    if (GET_MANA(ch) < GET_MAX_MANA(ch) && !EVENT_FLAGGED(ch, EVENT_REGEN_MANA))
        set_regen_event(ch, EVENT_REGEN_MANA);
}

/* subtracts amount of moves from ch's current and starts points event */
void alter_move(struct char_data *ch, int amount) {
    if (ch->in_room <= NOWHERE)
        return;

    GET_MOVE(ch) = MIN(GET_MOVE(ch) - amount, GET_MAX_MOVE(ch));

    if (GET_MOVE(ch) < GET_MAX_MOVE(ch) && !EVENT_FLAGGED(ch, EVENT_REGEN_MOVE))
        set_regen_event(ch, EVENT_REGEN_MOVE);
}

/* Ensure that all regeneration is taking place */
void check_regen_rates(struct char_data *ch) {
    if (ch->in_room <= NOWHERE)
        return;
    set_regen_event(ch, EVENT_REGEN_HP);
    set_regen_event(ch, EVENT_REGEN_MANA);
    set_regen_event(ch, EVENT_REGEN_MOVE);
    set_regen_event(ch, EVENT_RAGE);
}

/***************************************************************************
 * $Log: regen.c,v $
 * Revision 1.41  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.40  2008/09/13 18:52:36  jps
 * Using ai.h
 *
 * Revision 1.39  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.38  2008/05/30 16:59:38  myc
 * Stop non-berserkers from gathering rage while meditating.
 *
 * Revision 1.37  2008/05/25 20:59:52  myc
 * Fix so meditate works when rage is 0.
 *
 * Revision 1.36  2008/05/25 18:10:11  myc
 * Fix berzerk auto-attack so it uses quick-aggro to get rid of double
 * hits.
 *
 * Revision 1.35  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.34  2008/05/14 05:11:01  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.33  2008/05/11 05:55:26  jps
 * Changed slow_death call. hp_pos_check is used after alter_hit.
 *
 * Revision 1.32  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.31  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.30  2008/03/26 22:13:38  jps
 * alter_hit and update_pos need work
 *
 * Revision 1.29  2008/03/26 21:59:52  jps
 * Updating the position of characters who got killed in alter_hit().
 *
 * Revision 1.28  2008/03/11 19:50:55  myc
 * Make enranged meditate depend on meditate skill instead of berserk.
 *
 * Revision 1.27  2008/03/02 01:38:18  myc
 * Fixing a bug where if your hitpoints were somewhat larger than your
 * max hitpoints, you wouldn't take damage at all.
 *
 * Revision 1.26  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not.  And instead of creating
 * an event object for every regen event, we'll just pass the char.
 *
 * Revision 1.25  2008/02/09 18:29:11  myc
 * The event code now handles freeing of event objects.
 *
 * Revision 1.24  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.23  2008/01/30 19:20:57  myc
 * Removing the ch->regenerating field and replacing it with an event
 * flags member.
 *
 * Revision 1.22  2008/01/27 21:14:59  myc
 * Hijacking regen event handler for rage events for berserkers.
 * Rage events occur every 4 seconds and cause the berserker
 * to gain or lose rage depending on berserking or meditation.
 *
 * Revision 1.21  2008/01/22 16:16:47  jps
 * Correctly remove the regen_move bit when you lose your movement
 * regeneration event.
 *
 * Revision 1.20  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.19  2008/01/09 01:53:03  jps
 * Replace old points event with three new regen events.
 *
 * Revision 1.18  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.17  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.16  2006/12/04 14:08:51  dce
 * Removed a 'MOO' debug message left by someone.
 *
 * Revision 1.15  2006/11/20 06:44:26  jps
 * Fix creatures dying of bloodloss as you're fighting them.
 *
 * Revision 1.14  2006/11/17 03:47:30  jps
 * Eliminate points_event events being enqueued for outgoing players.
 *
 * Revision 1.13  2006/11/07 14:06:40  jps
 * Insane hit_gain values, such as those generated by Trolls, no
 * longer incorrectly return 0 from points_event crashing the mud.
 *
 * Revision 1.12  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.11  2001/01/15 21:51:24  mtp
 * make sure event type is null to avoid it being called again when the object
 *is null
 *
 * Revision 1.10  2001/01/12 00:51:07  mtp
 * changed comparison on line 52 to <= to trap mobs which are already freed
 *
 * Revision 1.9  2000/11/24 21:17:12  rsd
 * Altered comment header and added back rlog messgaes from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.8  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.7  1999/04/30 17:03:16  mud
 * reverted to version 1.5 to fix bad crashbugs.
 * Gurlaek
 *
 * Revision 1.6  1999/04/29 04:31:12  jimmy
 * Nulled some pointers for sanity.  Maybe this will help.
 * Gurlaek
 *
 * Revision 1.5  1999/03/31 20:18:01  jen
 * branches:  1.5.1;  1.5.2;
 * Changed regen for movement ... increases rate, decreases amt to 1 per event
 *
 * Revision 1.4  1999/03/14 14:28:11  jimmy
 * Movement now has bite!  removed extra "flying" from
 * movement_loss in constants.c to fix the mv bug.  reduced the
 * movement gain by 5 for all ages in limits.c.  Removed the +5
 * and +6 static movement gain so that it now actually updates
 * based on the function in regen.c.  Gosh i'm a bastard.
 * Fingon
 *
 * Revision 1.3  1999/02/08 23:01:47  jimmy
 * Fixed mortally wounded bug.  Mortally wounded
 * victims now die when they read -11.  Also,
 * no more "attempt to damage corpse"
 * fingon
 *
 * Revision 1.2  1999/02/01 23:59:12  mud
 * indented file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 * Revision 1.5.2.1  1999/04/30 17:01:33  mud
 * *** empty log message ***
 *
 * Revision 1.5.1.1  1999/04/30 16:57:56  mud
 * reverted to old rev due to major crashing.
 * gurlaek
 *
 ***************************************************************************/

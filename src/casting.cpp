/***************************************************************************
 *   File: casting.c                                      Part of FieryMUD *
 *  Usage: Functions for spellcasting.                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000, 2001 by the Fiery Consortium. *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University.                                       *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "casting.hpp"

#include "act.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "composition.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "damage.hpp"
#include "db.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "lifeforce.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "objects.hpp"
#include "pfiles.hpp"
#include "races.hpp"
#include "rooms.hpp"
#include "skills.hpp"
#include "spell_parser.hpp"
#include "spells.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/**********************************************************/
/* Event functions for spells that use time-based effects */
/**********************************************************/

EVENTFUNC(room_undo_event) {
    RoomUndoEventObj *room_undo = (RoomUndoEventObj *)event_obj;
    int room, exit, connect_room;

    room = room_undo->room;
    exit = room_undo->exit;
    connect_room = room_undo->connect_room;

    world[room].exits[exit]->to_room = connect_room;
    room_printf(
        room,
        "&2The forest seems to come alive... Trees and shrubs move about, finally resting in different locations.&0\n");
    ROOM_FLAGS(room).reset(ROOM_ALT_EXIT);
    return EVENT_FINISHED;
}

EVENTFUNC(delayed_cast_event) {
    DelayedCastEventObj *cast = (DelayedCastEventObj *)event_obj;
    CharData *ch = cast->ch;
    CharData *victim = cast->victim;
    int spellnum = cast->spell;
    int room = cast->room;
    int routines = cast->routines;
    int rounds = --cast->rounds;
    int wait = rounds > 0 ? cast->wait : EVENT_FINISHED;
    int skill = cast->skill;
    int savetype = cast->savetype;
    bool sustained = cast->sustained;

    /* Event checking */

    if (!ch || event_target_valid(ch) == 0)
        return EVENT_FINISHED;

    if (!IS_NPC(ch) && (!ch->desc || STATE(ch->desc) != CON_PLAYING))
        return EVENT_FINISHED;

    if (sustained) {
        if (ch->in_room != room)
            return EVENT_FINISHED;
        if (victim && ch->in_room != victim->in_room)
            return EVENT_FINISHED;
        if (!valid_cast_stance(ch, spellnum))
            return EVENT_FINISHED;
        if (IS_SET(routines, MAG_GROUP) && !IS_GROUPED(ch))
            return wait;
    }

    if (victim && event_target_valid(victim) == 0)
        return EVENT_FINISHED;

    /* Cast checking */

    if (ROOM_FLAGGED(room, ROOM_PEACEFUL) && SINFO.violent) {
        if (IS_SPELL(spellnum)) {
            if (ch && IN_ROOM(ch) == room) {
                char_printf(ch, "A flash of white light fills the room, dispelling your violent magic!\n");
            }
            if (victim)
                act("White light from no particular source suddenly fills the room, "
                    "then vanishes.",
                    false, victim, 0, 0, TO_ROOM);
        } else { /* song/chant */
            if (ch && IN_ROOM(ch) == room) {
                char_printf(ch, "Your words dissolve into peaceful nothingness...\n");
                act("$n's words fade away into peaceful nothingness...\n", false, ch, 0, 0, TO_ROOM);
            }
        }
        return EVENT_FINISHED;
    }

    /* Check routines */

    if (IS_SPELL(spellnum) && victim && evades_spell(ch, victim, spellnum, skill))
        return wait;

    /* Set REMOTE_AGGR flag so that aggr_lose_spells() won't take
     * off invis, etc. */
    EFF_FLAGS(ch).set(EFF_REMOTE_AGGR);

    if (IS_SET(routines, MAG_DAMAGE))
        mag_damage(skill, ch, victim, spellnum, savetype);

    if (victim && DECEASED(victim)) {
        wait = EVENT_FINISHED;
    } else {
        if (IS_SET(routines, MAG_AFFECT))
            mag_affect(skill, ch, victim, spellnum, savetype, CAST_SPELL);

        if (IS_SET(routines, MAG_UNAFFECT))
            mag_unaffect(skill, ch, victim, spellnum, savetype);

        if (IS_SET(routines, MAG_POINT))
            mag_point(skill, ch, victim, spellnum, savetype);

        if (IS_SET(routines, MAG_AREA))
            mag_area(skill, ch, spellnum, savetype);

        if (IS_SET(SINFO.routines, MAG_GROUP))
            mag_group(skill, ch, spellnum, savetype);

        if (IS_SET(SINFO.routines, MAG_MASS))
            mag_mass(skill, ch, spellnum, savetype);

        if (IS_SET(SINFO.routines, MAG_BULK_OBJS))
            mag_bulk_objs(skill, ch, spellnum, savetype);

        if (IS_SET(SINFO.routines, MAG_ROOM))
            mag_room(skill, ch, spellnum);

        if (IS_SET(SINFO.routines, MAG_MANUAL)) {
            if (spellnum == SPELL_PYRE && wait == EVENT_FINISHED)
                EFF_FLAGS(ch).set(EFF_ON_FIRE);
            switch (spellnum) {
            case SPELL_PYRE:
                spell_pyre_recur(spellnum, skill, ch, victim, nullptr, savetype);
                break;
            case SPELL_SOUL_TAP:
                spell_soul_tap_recur(spellnum, skill, ch, victim, nullptr, savetype);
                break;
            }
            if (victim && DECEASED(victim))
                wait = EVENT_FINISHED;
        }

        /* Violent spells cause fights. */
        if (SINFO.violent && ch && victim && attack_ok(victim, ch, false))
            set_fighting(victim, ch, false);
    }
    EFF_FLAGS(ch).reset(EFF_REMOTE_AGGR);

    return wait;
}

DelayedCastEventObj *construct_delayed_cast(CharData *ch, CharData *victim, int spellnum, int routines, int rounds,
                                            int wait, int skill, int savetype, bool sustained) {
    DelayedCastEventObj *event_obj;
    CREATE(event_obj, DelayedCastEventObj, 1);
    event_obj->ch = ch;
    event_obj->victim = victim;
    event_obj->spell = spellnum;
    event_obj->room = ch->in_room;
    event_obj->routines = routines;
    event_obj->skill = skill;
    event_obj->savetype = savetype;
    event_obj->wait = wait;
    event_obj->rounds = rounds;
    event_obj->sustained = sustained;
    return event_obj;
}

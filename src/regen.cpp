/***************************************************************************
 *   File: regen.c                                       Part of FieryMUD  *
 *  Usage: Contains routines to handle event based point regeneration      *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "regen.hpp"

#include "ai.hpp"
#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "races.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

void improve_skill(CharData *ch, int skill);
void stop_berserking(CharData *ch);
void start_berserking(CharData *ch);

#define PULSES_PER_MUD_HOUR (SECS_PER_MUD_HOUR * PASSES_PER_SEC)

EVENTFUNC(hp_regen_event) {
    CharData *ch = (CharData *)event_obj;
    int gain, delay = 0;
    bool is_dying = false;

    void slow_death(CharData * victim);

    if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
        if (GET_STANCE(ch) <= STANCE_STUNNED) {
            if (DAMAGE_WILL_KILL(ch, 1)) {
                /* In slow_death(), the character may die. */
                slow_death(ch);
            } else
                hurt_char(ch, nullptr, 1, true);
            is_dying = true;
        } else
            hurt_char(ch, nullptr, -1, true);

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

EVENTFUNC(spellslot_restore_event) {
    CharData *ch = (CharData *)event_obj;

    if (PLR_FLAGGED(ch, PLR_MEDITATE)) {

        if (EFF_FLAGGED(ch, EFF_INSANITY)) {
            char_printf(ch, "Your mind is too crazed to meditate!\n");
            act("$n ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
        }

        if (FIGHTING(ch)) {
            char_printf(ch, "Your meditation is rudely interrupted!\n");
            act("$n ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
        }

        if (IS_DRUNK(ch)) {
            char_printf(ch, "You cannot meditate while intoxicated.\n");
            act("$n ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
        }

        if (GET_POS(ch) != POS_SITTING || GET_STANCE(ch) < STANCE_RESTING || GET_STANCE(ch) > STANCE_ALERT) {
            char_printf(ch, "You stop meditating.\n");
            act("$n ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
        }
    }

    if (!ch->spellcasts.empty()) {
        if (EFF_FLAGGED(ch, EFF_INSANITY))
            return 1 RL_SEC;

        spell_slot_restore_tick(ch);

        /* check meditate skill */
        if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
            if (random_number(0, 20) > 17)
                improve_skill(ch, SKILL_MEDITATE);
        }

        return 1 RL_SEC;
    } else {
        char_printf(ch, "&3&bYou have recovered all your spell slots.&0\n&0");
        if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
            if (IS_NPC(ch))
                act("$n ceases $s meditative trance.", true, ch, 0, 0, TO_ROOM);
            else
                char_printf(ch, "You stop meditating.\n&0");
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
        }
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_SPELLSLOT);
        return 0;
    }
}

EVENTFUNC(move_regen_event) {
    CharData *ch = (CharData *)event_obj;
    int gain, delay = 0;

    if (GET_MOVE(ch) < GET_MAX_MOVE(ch)) {
        gain = move_gain(ch);
        if (GET_STANCE(ch) >= STANCE_SLEEPING && GET_POS(ch) < POS_STANDING)
            GET_MOVE(ch) = std::min(GET_MOVE(ch) + 1, GET_MAX_MOVE(ch));
        else
            GET_MOVE(ch) = std::min(GET_MOVE(ch) + 1, GET_MAX_MOVE(ch));

        delay = gain < 1 || gain > PULSES_PER_MUD_HOUR ? 1 : PULSES_PER_MUD_HOUR / gain;
    }

    if (!delay)
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_MOVE);
    return delay;
}

EVENTFUNC(rage_event) {
    CharData *ch = (CharData *)event_obj;

    /* Berserking: diminish rage quickly. */
    if (EFF_FLAGGED(ch, EFF_BERSERK)) {
        if (EFF_FLAGGED(ch, EFF_WRATH))
            GET_RAGE(ch) -= random_number(10, 15);
        else
            GET_RAGE(ch) -= random_number(20, 30);
        if (GET_RAGE(ch) % 7 == 0 && FIGHTING(ch))
            improve_skill(ch, SKILL_BERSERK);
        if (!FIGHTING(ch))
            event_create(EVENT_QUICK_AGGRO, quick_aggro_event, mkgenericevent(ch, find_aggr_target(ch), 0), true,
                         &(ch->events), 0);
    }

    /* Meditating: increase rage quickly. */
    else if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
        GET_RAGE(ch) += random_number(10, GET_SKILL(ch, SKILL_MEDITATE));
        if (GET_RAGE(ch) % 5 == 0)
            improve_skill(ch, SKILL_MEDITATE);
    }

    /* Otherwise diminish rage slowly. */
    else
        GET_RAGE(ch) -= random_number(2, 4);

    /* When you reach crazed rage, you are forced to start berserking. */
    if (GET_RAGE(ch) > RAGE_CRAZED && !EFF_FLAGGED(ch, EFF_BERSERK)) {
        if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
            REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
            GET_POS(ch) = POS_STANDING;
        }
        start_berserking(ch);
        char_printf(ch, "&1Your rage consumes you, taking control of your body...&0\n");
        if (IN_ROOM(ch) != NOWHERE)
            act("$n shudders as $s rage causes $m to go berserk!", true, ch, 0, 0, TO_ROOM);
    }

    if (GET_RAGE(ch) > 0)
        return 4 * PASSES_PER_SEC;
    else {
        char_printf(ch, "Your rage recedes and you feel calmer.\n");
        stop_berserking(ch);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_RAGE);
        return EVENT_FINISHED;
    }
}

void set_regen_event(CharData *ch, int eventtype) {
    long time;
    int gain;

    if (EVENT_FLAGGED(ch, eventtype))
        return;

    if (eventtype == EVENT_REGEN_HP && !EVENT_FLAGGED(ch, EVENT_REGEN_HP) && GET_HIT(ch) < GET_MAX_HIT(ch)) {
        gain = hit_gain(ch);
        time = PULSES_PER_MUD_HOUR / (gain ? gain : 1);
        event_create(EVENT_REGEN_HP, hp_regen_event, ch, false, &(ch->events), time);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_HP);
    }
    if (eventtype == EVENT_REGEN_MOVE && !EVENT_FLAGGED(ch, EVENT_REGEN_MOVE) && GET_MOVE(ch) < GET_MAX_MOVE(ch)) {
        gain = move_gain(ch);
        time = PULSES_PER_MUD_HOUR / (gain ? gain : 1);
        event_create(EVENT_REGEN_MOVE, move_regen_event, ch, false, &(ch->events), time);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_MOVE);
    }
    if (eventtype == EVENT_RAGE && !EVENT_FLAGGED(ch, EVENT_RAGE) &&
        (GET_RAGE(ch) > 0 || (GET_SKILL(ch, SKILL_BERSERK) && PLR_FLAGGED(ch, PLR_MEDITATE)))) {
        event_create(EVENT_RAGE, rage_event, ch, false, &(ch->events), 0);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_RAGE);
    }

    if (eventtype == EVENT_REGEN_SPELLSLOT && !EVENT_FLAGGED(ch, EVENT_REGEN_SPELLSLOT) && !ch->spellcasts.empty()) {
        event_create(EVENT_REGEN_SPELLSLOT, spellslot_restore_event, ch, false, &(ch->events), 1 RL_SEC);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_REGEN_SPELLSLOT);
    }
}

/* subtracts amount of hitpoints from ch's current */
void alter_hit(CharData *ch, int amount, bool cap_amount) {
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
void hurt_char(CharData *ch, CharData *attacker, int amount, bool cap_amount) {
    if (IN_ROOM(ch) == NOWHERE)
        abort();
    alter_hit(ch, amount, cap_amount);

    hp_pos_check(ch, attacker, amount);

    if (GET_HIT(ch) > HIT_INCAP && GET_HIT(ch) < GET_MAX_HIT(ch) && !EVENT_FLAGGED(ch, EVENT_REGEN_HP))
        set_regen_event(ch, EVENT_REGEN_HP);
}

/* subtracts amount of moves from ch's current and starts points event */
void alter_move(CharData *ch, int amount) {
    if (ch->in_room <= NOWHERE)
        return;

    GET_MOVE(ch) = std::min(GET_MOVE(ch) - amount, GET_MAX_MOVE(ch));

    if (GET_MOVE(ch) < GET_MAX_MOVE(ch) && !EVENT_FLAGGED(ch, EVENT_REGEN_MOVE))
        set_regen_event(ch, EVENT_REGEN_MOVE);
}

/* Ensure that all regeneration is taking place */
void check_regen_rates(CharData *ch) {
    if (ch->in_room <= NOWHERE)
        return;
    set_regen_event(ch, EVENT_REGEN_HP);
    set_regen_event(ch, EVENT_REGEN_SPELLSLOT);
    set_regen_event(ch, EVENT_REGEN_MOVE);
    set_regen_event(ch, EVENT_RAGE);
}

/***************************************************************************
 *  File: movement.c                                      Part of FieryMUD *
 *  Usage: functions for characters moving about                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "movement.hpp"

#include "act.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "db.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "regen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <math.h>

/************************************************************/
/****          MISCELLANEOUS MOVEMENT FUNCTIONS          ****/
/************************************************************/

/* simple function to determine if char can walk on water */
bool can_travel_on_water(CharData *ch) {
    ObjData *obj;
    int i;

    /* FIXME: should be in something like items.h or equipment.h */
    /* This function is currently in act.item.c */
    int find_eq_pos(CharData * ch, ObjData * obj, char *arg);

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return true;

    if (EFF_FLAGGED(ch, EFF_WATERWALK))
        return true;

    if (MOB_FLAGGED(ch, MOB_AQUATIC))
        return true;

    /* A boat in the inventory suffices */
    for (obj = ch->carrying; obj; obj = obj->next_content)
        if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, nullptr) < 0))
            return true;

    /* Worn boats are OK too */
    for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
            return true;

    return false;
}

#define CANCEL_GRAVITY                                                                                                 \
    {                                                                                                                  \
        if (ch)                                                                                                        \
            REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_GRAVITY);                                                           \
        else                                                                                                           \
            REMOVE_FLAG(GET_EVENT_FLAGS(obj), EVENT_GRAVITY);                                                          \
        return EVENT_FINISHED;                                                                                         \
    }
EVENTFUNC(gravity_event) {
    GravityEventObj *event = (GravityEventObj *)event_obj;
    CharData *ch = event->ch;
    ObjData *obj = event->obj;
    int in_room = ch ? IN_ROOM(ch) : IN_ROOM(obj);
    int to_room;

    if (in_room == NOWHERE)
        CANCEL_GRAVITY;

    if (world[in_room].sector_type != SECT_AIR || (ch ? !CAN_GO(ch, DOWN) : !CAN_GO(obj, DOWN)))
        CANCEL_GRAVITY;

    to_room = world[in_room].exits[DOWN]->to_room;

    if (ch) {
        if (GET_POS(ch) == POS_FLYING)
            CANCEL_GRAVITY;
        if (RIDING(ch) && GET_POS(RIDING(ch)) == POS_FLYING)
            CANCEL_GRAVITY;
        if (RIDDEN_BY(ch) && GET_POS(RIDDEN_BY(ch)) == POS_FLYING)
            CANCEL_GRAVITY;
        if (GET_LEVEL(ch) >= LVL_IMMORT)
            CANCEL_GRAVITY;

        if (RIDING(ch))
            dismount_char(ch);
        if (RIDDEN_BY(ch))
            dismount_char(RIDDEN_BY(ch));

        if (event->distance_fallen == 0) {
            if (EFF_FLAGGED(ch, EFF_FEATHER_FALL)) {
                act("&1&bYou find yourself in midair and begin descending.&0\n\n", false, ch, 0, 0, TO_CHAR);
                act("&1&b$n finds $mself in midair and begins descending.&0", false, ch, 0, 0, TO_ROOM);
            } else {
                act("&1&bYou find yourself on thin air and fall&0 &2DOWN!&0\n\n", false, ch, 0, 0, TO_CHAR);
                act("&1&b$n finds $mself on thin air and falls&0 &2DOWN!&0", false, ch, 0, 0, TO_ROOM);
                falling_yell(ch);
            }
        }

        char_from_room(ch);
        char_to_room(ch, to_room);

        if (EFF_FLAGGED(ch, EFF_FEATHER_FALL)) {
            char_printf(ch, "\n&2You float slowly downward.&0\n\n");
            act("&2$n floats slowly down from above.&0", false, ch, 0, 0, TO_ROOM);
        } else if (GET_SKILL(ch, SKILL_SAFEFALL)) {
            char_printf(ch, "\n&2You fall gracefully DOWN!&0\n\n");
            act("&2$n gracefully falls from above.&0", false, ch, 0, 0, TO_ROOM);
        } else {
            char_printf(ch, "\n&2DOWN!&0\n\n");
            act("&2$n falls screaming from above.&0", false, ch, 0, 0, TO_ROOM);
        }

        if (ch->desc)
            look_at_room(ch, false);
    } else if (obj) {
        if (event->distance_fallen == 0)
            act("$p &1plummets&0 &2downward!&0", false, 0, obj, 0, TO_ROOM);

        obj_from_room(obj);
        obj_to_room(obj, to_room);

        act("$p &1falls from above.&0", false, 0, obj, 0, TO_ROOM);
    }

    /* to_room is now the room the char/obj is currently in */
    if (to_room == event->start_room) {
        log("Falling room loop detected: {} started falling in room {:d}; is now in {:d}", ch ? "char" : "obj",
            world[event->start_room].vnum, world[in_room].vnum);
        if (ch)
            char_printf(ch, "\nParadoxically, you end up where you began.\n");
        CANCEL_GRAVITY;
    }

    event->distance_fallen++;

    /* If you can still fall, then queue up the event again. */
    if ((ch ? SECT(IN_ROOM(ch)) : SECT(IN_ROOM(obj))) == SECT_AIR)
        if (ch ? CAN_GO(ch, DOWN) : CAN_GO(obj, DOWN)) {
            /* return 1 makes it happen immediately; we want to wait a pulse or so */
            if (ch && EFF_FLAGGED(ch, EFF_FEATHER_FALL))
                return 4;
            else
                return 2;
        }

    /* No exit down means we've hit the bottom. */
    if (obj) {
        if (IS_SPLASHY(to_room))
            act("$p &1lands with a loud&0 &1SPLASH!&0", false, 0, obj, 0, TO_ROOM);
        else
            act("$p &1lands with a dull&0 &1THUD!&0", false, 0, obj, 0, TO_ROOM);
    } else if (ch) {
        if (GET_LEVEL(ch) < LVL_IMMORT)
            gravity_assisted_landing(ch, event->distance_fallen);
    }

    CANCEL_GRAVITY;
}

bool too_heavy_to_fly(CharData *ch) {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return false;
    if (IS_CARRYING_W(ch) < 1 || CAN_CARRY_W(ch) < 1)
        return false;
    return IS_CARRYING_W(ch) > MAXIMUM_FLIGHT_LOAD(ch);
}

void start_char_falling(CharData *ch) {
    GravityEventObj *event_obj;
    if (!EVENT_FLAGGED(ch, EVENT_GRAVITY)) {
        CREATE(event_obj, GravityEventObj, 1);
        event_obj->ch = ch;
        event_obj->start_room = IN_ROOM(ch);
        event_create(EVENT_GRAVITY, gravity_event, event_obj, true, &(ch->events), 0);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_GRAVITY);
    }
}

void start_obj_falling(ObjData *obj) {
    GravityEventObj *event_obj;
    if (!EVENT_FLAGGED(obj, EVENT_GRAVITY)) {
        CREATE(event_obj, GravityEventObj, 1);
        event_obj->obj = obj;
        event_obj->start_room = IN_ROOM(obj);
        event_create(EVENT_GRAVITY, gravity_event, event_obj, true, &(obj->events), 0);
        SET_FLAG(GET_EVENT_FLAGS(obj), EVENT_GRAVITY);
    }
}

void falling_check(CharData *ch) {
    if (ch && GET_LEVEL(ch) < 100 && !EVENT_FLAGGED(ch, EVENT_GRAVITY) && IN_ROOM(ch) != NOWHERE &&
        SECT(IN_ROOM(ch)) == SECT_AIR && GET_POS(ch) != POS_FLYING) {
        start_char_falling(ch);
    }
}

/*
 * Gravity
 *
 * Makes mobiles and objects fall.
 */

/* falling_yell - when you first start falling, your yell of surprise is heard
 * in the surrounding rooms. */
void falling_yell(CharData *ch) {
    int dir, was_in, backdir, i;
    char *dirstr;
    RoomData *oroom;

    if (EFF_FLAGGED(ch, EFF_SILENCE))
        return;

    was_in = ch->in_room;

    for (dir = 0; dir < NUM_OF_DIRS; dir++) {
        if (CAN_GO(ch, dir) && ((oroom = CH_DEST(ch, dir)))) {
            /* other_room will get a scream.
             * If it has an exit back to this room, use that direction as where
             * the scream "came from". Otherwise, use the opposite of the direction
             * going to other_room. */
            backdir = -1;

            /* does the destination room have an exit pointing back? */
            if (oroom->exits[rev_dir[dir]])
                /* does that exit point back to this room? */
                if (EXIT_NDEST(oroom->exits[rev_dir[dir]]) == was_in)
                    backdir = rev_dir[dir];

            /* No exit pointing directly back?  Search for any exit pointing back. */
            if (backdir == -1) {
                for (i = 0; i < NUM_OF_DIRS; i++)
                    if (oroom->exits[i] && EXIT_NDEST(oroom->exits[i]) == was_in) {
                        backdir = i;
                        break;
                    }
            }

            /* Couldn't find any exits back. But since *this* room does have an exit
             * going to that room, we *will* send the sound. */
            if (backdir == -1)
                backdir = rev_dir[dir];

            if (backdir == 5)
                dirstr = "below";
            else if (backdir == 4)
                continue; /* No yell - you'll receive "<person> falls screaming from
                             above" */
            else {
                sprintf(buf2, "the %s", dirs[backdir]);
                dirstr = buf2;
            }

            sprintf(buf, "You hear a %s %s from %s, which quickly fades.", number(0, 10) < 5 ? "surprised" : "sudden",
                    number(0, 10) < 6 ? "shriek" : "yelp", dirstr);

            ch->in_room = EXIT_NDEST(world[was_in].exits[dir]);
            act(buf, false, ch, 0, 0, TO_ROOM);
            ch->in_room = was_in;
        }
    }
}

void gravity_assisted_landing(CharData *ch, int distance_fallen) {
    int damage = 0;

    /* Levitation protects from damage */
    if (EFF_FLAGGED(ch, EFF_FEATHER_FALL)) {
        if (IS_WATER(IN_ROOM(ch))) {
            char_printf(ch, "\nYou come to rest above the surface of the water.\n");
            act("$n comes to rest above the surface of the water.", false, ch, 0, 0, TO_ROOM);
        } else {
            char_printf(ch, "\nYou come to rest just above the ground.\n");
            act("$n's descent ends just above the ground.", false, ch, 0, 0, TO_ROOM);
        }
        return;
    }

    GET_POS(ch) = POS_SITTING;
    GET_STANCE(ch) = STANCE_ALERT;

    /* Are we landing in water? It hurts MUCH less... */
    /* If we have safe fall skill, then we take no damage
       for five rooms, partially for 5-15 and full at 15 David Endre 3/8/99 */
    if (IS_WATER(IN_ROOM(ch))) {
        char_printf(ch, "\nYou land with a tremendous &4SPLASH&2!&0\n");
        act("$n lands with a tremendous &4SPLASH&2!&0", false, ch, 0, 0, TO_ROOM);
    } else {
        if (GET_SKILL(ch, SKILL_SAFEFALL) && distance_fallen <= 5) {
            GET_POS(ch) = POS_STANDING;
            GET_STANCE(ch) = STANCE_ALERT;
            char_printf(ch, "\nYou tuck and roll, performing a beautiful landing!\n");
            act("$n tucks and rolls, performing a beautiful landing!", false, ch, 0, 0, TO_ROOM);
        } else if (GET_SKILL(ch, SKILL_SAFEFALL) && distance_fallen < 15) {
            char_printf(ch, "\nYou gracefully land without taking too much damage.\n");
            act("$n gracefully lands without taking too much damage.", false, ch, 0, 0, TO_ROOM);
        } else {
            char_printf(ch, "\nYou land with a resounding &1S&2P&1L&2A&1T&2!&0\n");
            act("$n lands with a resounding &1S&2P&1L&2A&1T&2!&0", false, ch, 0, 0, TO_ROOM);
        }
    }

    damage = ((distance_fallen * (GET_SIZE(ch) + 1)) / 50.0) * GET_MAX_HIT(ch);

    if (IS_WATER(IN_ROOM(ch)))
        damage /= 4;

    if (GET_SKILL(ch, SKILL_SAFEFALL)) {
        if (distance_fallen <= 5)
            damage = 0;
        else if (distance_fallen < 15)
            damage *= distance_fallen / 15.0;
    }

    hurt_char(ch, nullptr, damage, true);
}

/*************************************/
/****          FOLLOWING          ****/
/*************************************/

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(CharData *ch, int violent) {
    FollowType *j, *k;

    assert(ch->master);

    if (EFF_FLAGGED(ch, EFF_CHARM)) {
        if (DECEASED(ch))
            char_printf(ch->master, "A wave of sorrow nearly overcomes you.\n");
        else if (violent) {
            act("You realize that $N is a jerk!", false, ch, 0, ch->master, TO_CHAR);
            act("$n realizes that $N is a jerk!", false, ch, 0, ch->master, TO_NOTVICT);
            act("$n hates your guts!", false, ch, 0, ch->master, TO_VICT);
        }
        if (affected_by_spell(ch, SPELL_CHARM))
            effect_from_char(ch, SPELL_CHARM);

    } else if (EFF_FLAGGED(ch, EFF_SHADOWING)) {
        act("You stop shadowing $N.", true, ch, 0, ch->master, TO_CHAR);
    } else {
        act("You stop following $N.", false, ch, 0, ch->master, TO_CHAR);
        act("$n stops following $N.", true, ch, 0, ch->master, TO_NOTVICT);
        act("$n stops following you.", true, ch, 0, ch->master, TO_VICT);
    }

    if (ch->master->followers->follower == ch) { /* Head of follower-list? */
        k = ch->master->followers;
        ch->master->followers = k->next;
        free(k);
    } else { /* locate follower who is not head of list */
        for (k = ch->master->followers; k->next->follower != ch; k = k->next)
            ;

        j = k->next;
        k->next = j->next;
        free(j);
    }

    if (EFF_FLAGGED(ch, EFF_CHARM)) {
        REMOVE_FLAG(EFF_FLAGS(ch), EFF_CHARM);

        /* The following is necessary to prevent the sharing of summoned mounts.
         * If the mob were not removed from the game at this point, a paladin
         * could attack his mount, then flee from the fight, and when he left
         * the game the mount would remain. So we remove it now. Fighting your
         * summoned mount is irrelevant anyway (you get no xp). */

        if (IS_NPC(ch))
            event_create(EVENT_MOB_QUIT, mobquit_event, ch, false, &(ch->events), 3 RL_SEC);

        if (MOB_FLAGGED(ch, MOB_SUMMONED_MOUNT) && !IS_NPC(ch->master))
            SET_COOLDOWN(ch->master, CD_SUMMON_MOUNT, 12 MUD_HR);
    }

    REMOVE_FLAG(EFF_FLAGS(ch), EFF_SHADOWING);
    ch->master = nullptr;
}

/* Called when a character that follows/is followed dies */
void die_follower(CharData *ch) {
    FollowType *j, *k;

    if (ch->master)
        stop_follower(ch, 0);

    for (k = ch->followers; k; k = j) {
        j = k->next;
        stop_follower(k->follower, 0);
    }
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(CharData *ch, CharData *leader) {
    FollowType *k;

    assert(!ch->master);

    ch->master = leader;

    CREATE(k, FollowType, 1);

    k->follower = ch;
    k->next = leader->followers;
    leader->followers = k;

    if (!EFF_FLAGGED(ch, EFF_SHADOWING)) {
        act("You now follow $N.", false, ch, 0, leader, TO_CHAR);
        act("$n starts following you.", true, ch, 0, leader, TO_VICT);
        act("$n starts to follow $N.", true, ch, 0, leader, TO_NOTVICT);
    }
}

/************************************/
/****          GROUPING          ****/
/************************************/

void add_groupee(CharData *master, CharData *groupee) {
    GroupType *g;

    if (IS_GROUPED(groupee))
        return;

    act("&2You accept $N &2into your group.&0", false, master, 0, groupee, TO_CHAR);
    act("&2You have been accepted into $n&2's group.&0", false, master, 0, groupee, TO_VICT);
    for (g = master->groupees; g; g = g->next)
        act("&2$N &2has joined your group.&0", true, g->groupee, 0, groupee, TO_CHAR);

    CREATE(g, GroupType, 1);
    g->groupee = groupee;
    g->next = master->groupees;
    master->groupees = g;
    groupee->group_master = master;
}

void disband_group(CharData *master, bool verbose, bool forceful) {
    GroupType *g;

    assert(master->groupees);

    if (verbose)
        char_printf(master, forceful ? "&2The group has been disbanded.&0\n" : "&2You disband the group.&0\n");

    while (master->groupees) {
        g = master->groupees;
        g->groupee->group_master = nullptr;
        master->groupees = g->next;
        if (verbose)
            act(forceful ? "&2The group has been disbanded.&0" : "&2$n &2has disbanded the group.&0", false,
                master, 0, g->groupee, TO_VICT);
        free(g);
    }
}

void ungroup(CharData *ch, bool verbose, bool forceful) {
    /* Character is a group leader... */
    if (ch->groupees) {

        /* Only one other group member? Disband. */
        if (!ch->groupees->next)
            disband_group(ch, verbose, false);

        /* Otherwise promote first group member to new leader. */
        else {
            GroupType *g;
            CharData *new_master = ch->groupees->groupee;

            if (verbose) {
                char_printf(ch, "&2You're no longer leading your group.&0\n");
                char_printf(new_master, "&2You're now leading the group!&0\n");
            }

            /* Move groupees to new master. */
            new_master->groupees = ch->groupees->next;
            new_master->group_master = nullptr;
            for (g = new_master->groupees; g; g = g->next) {
                g->groupee->group_master = new_master;
                if (verbose)
                    act("&2$n &2is now leading your group!&0", true, new_master, 0, g->groupee, TO_VICT);
            }

            free(ch->groupees);
            ch->groupees = nullptr;
        }
    }

    /* Character is a group member */
    else if (ch->group_master) {

        /* Only one other group member? Disband. */
        if (!ch->group_master->groupees->next) {
            if (verbose) {
                act(forceful ? "&2You remove $N from the group.&0" : "&2$N has left the group.&0", false,
                    ch->group_master, 0, ch, TO_CHAR);
                act(forceful ? "&2$n has removed you from the group.&0" : "&2You have left your group!&0", false,
                    ch->group_master, 0, ch, TO_VICT);
            }
            disband_group(ch->group_master, verbose, true);
        }

        /* Otherwise remove this character from group list */
        else {
            CharData *master = ch->group_master;
            GroupType *g, dummy, *found;

            dummy.next = master->groupees;

            for (g = &dummy; g && g->next;) {
                if (g->next->groupee == ch) {
                    found = g->next;
                    g->next = found->next;
                    free(found);
                    continue;
                }
                if (verbose)
                    act(forceful ? "&2$n &2has been kicked out of your group!&0"
                                 : "&2$n &2has left your group!&0",
                        true, ch, 0, g->next->groupee, TO_VICT);
                g = g->next;
            }

            master->groupees = dummy.next;
            ch->group_master = nullptr;

            if (verbose) {
                act(forceful ? "&2You have been kicked out of your group.&0" : "&2You have left your group!&0",
                    false, ch, 0, 0, TO_CHAR);
                act(forceful ? "&2You have kicked $n &2out of your group.&0" : "&2$n &2has left your group!&0",
                    false, ch, 0, master, TO_VICT);
            }
        }
    } else
        assert(false);
}

bool is_grouped(CharData *ch, CharData *tch) {
    CharData *k, *l;

    /* k is ch's group leader */
    k = ch->group_master ? ch->group_master : ch;

    /* l is tch's group leader */
    l = tch->group_master ? tch->group_master : tch;

    return (l == k); /* same group master? */
}

bool battling_my_group(CharData *ch, CharData *tch) {
    CharData *i;

    if (FIGHTING(tch)) {
        /* You are fighting with me */
        if (FIGHTING(tch) == ch)
            return true;
        /* You are fighting someone who is grouped with me */
        if (is_grouped(FIGHTING(tch), ch))
            return true;
    }
    if (FIGHTING(ch)) {
        /* I am fighting with you */
        if (FIGHTING(ch) == tch)
            return true;
        /* I am fighting someone in your group */
        if (is_grouped(FIGHTING(ch), tch))
            return true;
    }
    for (i = world[ch->in_room].people; i; i = i->next_in_room) {
        /* Someone in here is fighting */
        if (FIGHTING(i)) {
            /* This fighting person is grouped with me */
            if (is_grouped(i, ch)) {
                /* They are fighting you */
                if (FIGHTING(i) == tch)
                    return true;
                /* They are fighting someone in your group */
                if (is_grouped(FIGHTING(i), tch))
                    return true;
                /* This person is grouped with you */
            } else if (is_grouped(i, tch)) {
                /* They are fighting me */
                if (FIGHTING(i) == ch)
                    return true;
                /* They are fighting someone in my group */
                if (is_grouped(FIGHTING(i), ch))
                    return true;
            }
        }
    }
    return false;
}

/*********************************************/
/****          RIDING and MOUNTS          ****/
/*********************************************/

void mount_char(CharData *ch, CharData *mount) {
    RIDING(ch) = mount;
    RIDDEN_BY(mount) = ch;
}

void dismount_char(CharData *ch) {
    if (RIDING(ch)) {
        RIDDEN_BY(RIDING(ch)) = nullptr;
        RIDING(ch) = nullptr;
    }

    if (RIDDEN_BY(ch)) {
        RIDING(RIDDEN_BY(ch)) = nullptr;
        RIDDEN_BY(ch) = nullptr;
    }
}

/* ideal_mountlevel
 *
 * Returns the highest level of mob which this character could mount and ride
 * with no difficulty.
 */

int ideal_mountlevel(CharData *ch) {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return 1000; /* arbitrarily large number */

    if (GET_SKILL(ch, SKILL_MOUNT) < 1)
        return -1 - 2 * MOUNT_LEVEL_FUDGE; /* Make it impossible to mount anything
                                              with 0 skill */

    return GET_SKILL(ch, SKILL_MOUNT) * (MAX_MOUNT_LEVEL - 1) / 100 - 5;
}

int ideal_ridelevel(CharData *ch) {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return 1000; /* arbitrarily large number */

    if (GET_SKILL(ch, SKILL_RIDING) < 1)
        return -1 - 2 * MOUNT_LEVEL_FUDGE; /* Make it impossible to ride anything
                                              with 0 skill */

    return GET_SKILL(ch, SKILL_RIDING) * (MAX_MOUNT_LEVEL - 1) / 100 - 5;
}

int ideal_tamelevel(CharData *ch) {
    int tame_skill, tame_bonus;

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return 1000; /* arbitrarily large number */

    if (GET_SKILL(ch, SKILL_TAME) < 1)
        return -1 - 2 * MOUNT_LEVEL_FUDGE; /* Make it impossible to tame anything
                                              with 0 skill */

    tame_skill = GET_SKILL(ch, SKILL_TAME);

    tame_bonus = (int)(GET_CHA(ch) / 3);

    if (GET_CLASS(ch) == CLASS_DRUID) {
        tame_bonus += 30;
    }
    if (GET_CLASS(ch) == CLASS_RANGER) {
        tame_bonus += 15;
    }

    tame_skill += tame_bonus / 10;
    return tame_skill * (MAX_MOUNT_LEVEL - 1) / 100 - 5;
}

int mountlevel(CharData *ch) {
    int l = GET_LEVEL(ch);

    if (EFF_FLAGGED(ch, EFF_TAMED))
        l -= 2;
    if (EFF_FLAGGED(ch, EFF_CHARM))
        l -= 2;

    return l;
}

/* movement_bucked
 *
 * True or false: will this character be bucked while moving?
 */

int movement_bucked(CharData *ch, CharData *mount) {
    int diff = mountlevel(mount) - ideal_ridelevel(ch);

    if (diff < 1)
        return 0;
    if (diff > MOUNT_LEVEL_FUDGE)
        return 1;

    return number(0, 999) < 1 + pow((double)(2 * diff) / MOUNT_LEVEL_FUDGE, 3) * 150 / 8;
}

/* mount_bucked
 *
 * True or false: will this character be bucked while trying to mount?
 */

int mount_bucked(CharData *ch, CharData *mount) {
    int diff = mountlevel(mount) - ideal_mountlevel(ch);

    if (diff < 1)
        return 0;
    if (diff > MOUNT_LEVEL_FUDGE)
        return 1;

    return number(0, 999) < 50 + pow((double)(2 * diff) / MOUNT_LEVEL_FUDGE, 3) * 850 / 8;
}

/* mount_fall
 *
 * True or false: will this character fall while trying to mount?
 */

int mount_fall(CharData *ch, CharData *mount) {
    int diff = mountlevel(mount) - ideal_mountlevel(ch);

    if (diff < 1)
        return 0;
    if (diff > MOUNT_LEVEL_FUDGE)
        return 1;

    return number(0, 999) < 10 + pow((double)(2 * diff) / MOUNT_LEVEL_FUDGE, 3) * 250 / 8;
}

void mount_warning(CharData *ch, CharData *vict) {
    int diff = mountlevel(vict) - ideal_mountlevel(ch);

    if (diff > 15) {
        act("You can't even imagine controlling $N!", false, ch, 0, vict, TO_CHAR);
    } else if (diff > MOUNT_LEVEL_FUDGE + 2) {
        act("You're nowhere near being able to control such a powerful beast.", false, ch, 0, 0, TO_CHAR);
    } else if (diff > MOUNT_LEVEL_FUDGE) {
        act("You don't feel quite up to riding $N.", false, ch, 0, vict, TO_CHAR);
    }
}

/* A mount has changed position.  Will its rider fall? */
void mount_pos_check(CharData *mount) {
    CharData *ch = RIDDEN_BY(mount);

    if (!ch || GET_POS(mount) >= POS_KNEELING)
        return;

    dismount_char(ch);
    if ((EFF_FLAGGED(ch, EFF_FLY) || GET_LEVEL(ch) >= LVL_IMMORT) &&
        !(EFF_FLAGGED(ch, EFF_MINOR_PARALYSIS) || EFF_FLAGGED(ch, EFF_MAJOR_PARALYSIS)) &&
        GET_STANCE(ch) > STANCE_SLEEPING) {
        act("You slide off $N and begin flying.", false, ch, 0, mount, TO_CHAR);
        act("$n sldes off $N and begins flying.", false, ch, 0, mount, TO_ROOM);
        GET_POS(ch) = POS_FLYING;
        GET_STANCE(ch) = STANCE_ALERT;
    } else if (IS_WET(CH_NROOM(ch))) {
        /* What we really need is a generic "got wet" function, which would take
         * care of flames and ruin people's spellbooks (KIDDING!) */
        if (EFF_FLAGGED(ch, EFF_ON_FIRE)) {
            char_printf(ch,
                        "You fall into the water, and your flames are put out with a "
                        "hiss of steam.\n");
            act("$n falls into the water.  $s flames go out with a hissing sound.", false, ch, 0, 0, TO_ROOM);
        } else {
            char_printf(ch, "You fall into the water.\n");
            act("$n falls into the water.", false, ch, 0, 0, TO_ROOM);
        }
        GET_POS(ch) = POS_PRONE;
        GET_STANCE(ch) = STANCE_ALERT;
        WAIT_STATE(ch, PULSE_VIOLENCE);
    } else if (CH_SECT(ch) == SECT_AIR) {
        act("You fall off $N!", false, ch, 0, mount, TO_CHAR);
        act("$n falls off $N!", false, ch, 0, mount, TO_ROOM);
        GET_POS(ch) = POS_PRONE;
        GET_STANCE(ch) = STANCE_ALERT;
        WAIT_STATE(ch, PULSE_VIOLENCE);
    } else {
        char_printf(ch, "You find yourself dumped to the ground.\n");
        act("$n is dumped onto the ground.", false, ch, 0, 0, TO_ROOM);
        GET_POS(ch) = POS_PRONE;
        GET_STANCE(ch) = STANCE_ALERT;
        WAIT_STATE(ch, PULSE_VIOLENCE);
    }
    falling_check(ch);
}

/***************************************************************************
 *   File: graph.c                                        Part of FieryMUD *
 *  Usage: various graph algorithms                                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

/* This is a safety device to limit the number of rooms a BFS will check.
 * It'll come in handy for when there's a bug in BFS, or if the mud
 * happens to have a number of insanely-connected rooms (e.g., 6 unique
 * rooms from every room) */
#define MAX_BFS_ROOMS 500

#include "casting.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "directions.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "rooms.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Externals */
ACMD(do_follow);
void flush_queues(DescriptorData *d);
ACMD(do_gen_door);

struct BFSQueueStruct {
    int room;
    char dir;
    BFSQueueStruct *next;
};

struct TrackNode {
    room_num room;
    int direction;
    TrackNode *next;
};

#define NOT_FOUND_TRACK -5000
static BFSQueueStruct *queue_head_ = 0, *queue_tail_ = 0;
CharData *find_race(char *arg, TrackInfo track, CharData *ch);
bool call_track(bool hunt, TrackInfo track, CharData *ch, CharData *victim, bool follow);
bool cause_single_track(TrackInfo track, CharData *ch, CharData *victim, int track_room);

/* Utility macros */
#define MARK(room) (world[room].bfs_distance = 0)
#define UNMARK(room) (world[room].bfs_distance = -1)
#define BFS_MARKED(room) (world[room].bfs_distance != -1)
#define TOROOM(x, y) (world[(x)].exits[(y)]->to_room)
/*
#define EXIT_CLOSED(x, y) (IS_SET(world[(x)].exits[(y)]->exit_info, EX_CLOSED))
#define EXIT_HIDDEN(x, y) (IS_SET(world[(x)].exits[(y)]->exit_info, EX_HIDDEN))
*/

#define VALID_EDGE(roomvnum, dir, tracker)                                                                             \
    (world[(roomvnum)].exits[(dir)] && (TOROOM(roomvnum, dir) != NOWHERE) &&                                           \
     (!IS_NPC(tracker) || !ROOM_FLAGGED(TOROOM(roomvnum, dir), ROOM_NOTRACK)) &&                                       \
     (!EXIT_IS_HIDDEN(world[roomvnum].exits[dir])) && (!BFS_MARKED(TOROOM(roomvnum, dir))))

void bfs_enqueue(int room, int dir) {
    BFSQueueStruct *curr;

    CREATE(curr, BFSQueueStruct, 1);
    curr->room = room;
    curr->dir = dir;
    curr->next = 0;

    if (queue_tail_) {
        queue_tail_->next = curr;
        queue_tail_ = curr;
    } else
        queue_head_ = queue_tail_ = curr;
}

void bfs_dequeue(void) {
    BFSQueueStruct *curr;

    curr = queue_head_;

    if (!(queue_head_ = queue_head_->next))
        queue_tail_ = 0;
    free(curr);
}

void bfs_clear_queue(void) {
    while (queue_head_)
        bfs_dequeue();
}

/* find_first_step: given a source room and a target room, find the first
    step on the shortest path from the source to the target.

    It's intended for tracking.

    Return values:
      (ret)      A BFS_* constant indicating the state of the search.
      distance   The distance of the path (if any was found).
*/

int find_first_step(int src, int target, CharData *tracker, int *distance) {
    int curr_dir;
    int curr_room;

    if (src < 0 || src > top_of_world || target < 0 || target > top_of_world) {
        log("Illegal value passed to find_first_step (graph.c)");
        return BFS_ERROR;
    }

    if (src == target)
        return BFS_ALREADY_THERE;

    /* clear marks first */
    for (curr_room = 0; curr_room <= top_of_world; curr_room++)
        UNMARK(curr_room);

    world[src].bfs_distance = 0;

    /* first, enqueue the first steps, saving which direction we're going. */
    for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
        if (VALID_EDGE(src, curr_dir, tracker)) {
            world[TOROOM(src, curr_dir)].bfs_distance = 1;
            bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
        }

    /* now, do the classic BFS. */
    while (queue_head_) {
        if (queue_head_->room == target) {
            curr_dir = queue_head_->dir;
            *distance = world[queue_head_->room].bfs_distance;
            bfs_clear_queue();
            return curr_dir;
        } else {
            for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
                if (VALID_EDGE(queue_head_->room, curr_dir, tracker)) {
                    bfs_enqueue(TOROOM(queue_head_->room, curr_dir), queue_head_->dir);
                    world[TOROOM(queue_head_->room, curr_dir)].bfs_distance = world[queue_head_->room].bfs_distance + 1;
                }
            bfs_dequeue();
        }
    }

    return BFS_NO_PATH;
}

/* find_track_victim
 *
 * Takes a tracker, a name of someone to track, and the maximum
 * distance.
 *
 * Finds the nearest detectable mob by that name.
 *
 * Returns an the direction to go (or BFS_[other])
 * Also indicates the victim found.
 */

int find_track_victim(CharData *ch, char *name, int maxdist, CharData **victim) {
    int i, cdist, roomschecked = 0, result = BFS_NO_PATH;
    bool found = false;
    TrackNode *start, *current, *new_node, *end;
    CharData *cd;

    if (!ch || !name || !victim)
        return BFS_ERROR;

    /* Did you say "track me"? */
    if (!strcasecmp(name, "self") || !strcasecmp(name, "me")) {
        *victim = ch;
        return BFS_ALREADY_THERE;
    }

    /* Clear room marks */
    for (i = 0; i <= top_of_world; i++)
        UNMARK(i);

    /* Get our nodes started up... */
    CREATE(start, TrackNode, 1);
    start->room = IN_ROOM(ch);
    start->direction = BFS_ALREADY_THERE;
    start->next = nullptr;
    current = start;
    end = start;

    world[start->room].bfs_distance = 0;
    cdist = 0;
    *victim = nullptr;

    while (current && !found && cdist < maxdist) {
        /* Handle a room: current. */
        cdist = world[current->room].bfs_distance;

        /* Can the tracker see the victim in here? */
        for (cd = world[current->room].people; cd; cd = cd->next_in_room)
            if (isname(name, GET_NAMELIST(cd)) && CAN_SEE(ch, cd)) {
                /* The victim has been found! */
                found = true;
                result = current->direction;
                *victim = cd;
            }

        if (found || roomschecked++ > MAX_BFS_ROOMS)
            break;

        /* Didn't find the victim in this room. */

        if (cdist < maxdist)
            /* Put the rooms around this one on our list of rooms to search. */
            for (i = 0; i < NUM_OF_DIRS; i++)
                if (VALID_EDGE(current->room, i, ch)) {
                    world[TOROOM(current->room, i)].bfs_distance = cdist + 1;
                    CREATE(new_node, TrackNode, 1);
                    new_node->room = TOROOM(current->room, i);
                    new_node->direction = i;
                    new_node->next = nullptr;

                    /* This node goes on the end of our list */
                    end->next = new_node;
                    end = new_node;
                }

        current = current->next;
    }

    /* Discard the list */
    for (current = start; current;) {
        new_node = current->next;
        free(current);
        current = new_node;
    }

    return result;
}

/************************************************************************
 *  Functions and Commands which use the above fns                        *
 ************************************************************************/

EVENTFUNC(track_delayed_event) {
    TrackDelayedEventObj *track_event = (TrackDelayedEventObj *)event_obj;
    CharData *ch;
    CharData *victim;
    TrackInfo track;
    int track_room;

    victim = track_event->victim;
    ch = track_event->ch;
    track_room = track_event->track_room;
    track = track_event->track;

    if (!event_target_valid(ch) || !event_target_valid(victim)) {
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return EVENT_FINISHED;
    }

    if (!cause_single_track(track, ch, victim, track_room)) {
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return EVENT_FINISHED;
    }

    /* re-queue event */
    if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SLOW_TRACK))
        return track.speed + 20;
    return track.speed;
}

ACMD(do_track) {
    CharData *vict;
    TrackInfo track;
    int trackskill;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are too busy to look for a trail.\n");
        return;
    }
    if (!GET_SKILL(ch, SKILL_TRACK) && !IS_NPC(ch)) {
        char_printf(ch, "You have no idea how.\n");
        return;
    }
    if (CONFUSED(ch)) {
        char_printf(ch, "You're far too confused to track anyone!\n");
        return;
    }

    /* Give mobs a break on track skill */
    if (!IS_PC(ch))
        trackskill = std::max<int>(GET_LEVEL(ch), GET_SKILL(ch, SKILL_TRACK));
    else
        trackskill = GET_SKILL(ch, SKILL_TRACK);

    /* Determine the range and speed */
    track.speed = std::clamp<int>(((101 - trackskill) / 2), 3, 20);
    track.sense = std::max<int>(1, (trackskill / 3));
    track.range = std::max<int>(1, (trackskill / 4));
    if ((GET_CLASS(ch) == CLASS_RANGER) || (GET_CLASS(ch) == CLASS_HUNTER)) {
        track.range = track.range + 8;
        track.sense = track.range + 4;
    }
    if (IS_NPC(ch))
        if (!POSSESSED(ch)) {
            track.speed = (int)(track.speed / 2);
            track.range = (int)((130 * track.range) / 100);
        }

    if (IS_NPC(ch))
        if (!POSSESSED(ch))
            if (EVENT_FLAGGED(ch, EVENT_TRACK))
                return;

    /* If you are already tracking, "track" stops you. */
    if (EVENT_FLAGGED(ch, EVENT_TRACK)) {
        char_printf(ch, "You stop tracking.\n");
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return;
    }

    /* Figure out who they're trying to track. */
    one_argument(argument, arg);
    if (!*arg) {
        char_printf(ch, "Whom are you trying to track?\n");
        return;
    }

    find_track_victim(ch, arg, track.range, &vict);

    if (!vict) {
        char_printf(ch, "You can not seem to find tracks for that person.\n");
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    /* Now vict is the person we are trying to track. */

    if (vict == ch) {
        char_printf(ch, "Awesome!  You've found yourself!\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) >= LVL_IMMORT) {
        char_printf(ch, "You feel their touches in everything around you.  How do you expect to track a god?\n");
        return;
    }

    if (vict->in_room == ch->in_room) {
        act("$E's right here!", false, ch, 0, vict, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(vict, EFF_NOTRACK)) {
        char_printf(ch, "You can not seem to find tracks for that person.\n");
        return;
    }

    if (IS_NPC(ch))
        if (POSSESSED(ch))
            if (EVENT_FLAGGED(ch, EVENT_TRACK))
                return;

    if (EVENT_FLAGGED(ch, EVENT_TRACK)) {
        char_printf(ch, "You stop tracking.\n");
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        return;
    }

    if (call_track(false, track, ch, vict, false)) {
        /*if npc dont lag mob */
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
    }
    improve_skill(ch, SKILL_TRACK);
}

ACMD(do_hunt) {
    CharData *vict;
    bool follow = false;
    TrackInfo track;

    if (FIGHTING(ch)) {
        char_printf(ch, "You are too busy to look for a trail.\n");
        return;
    }

    if (IS_NPC(ch) && !POSSESSED(ch))
        return;

    if (!GET_SKILL(ch, SKILL_HUNT)) {
        char_printf(ch, "You have no idea how.\n");
        return;
    }

    if (CONFUSED(ch)) {
        char_printf(ch, "You're far too confused to go hunting!\n");
        return;
    }

    track.speed = std::clamp((101 - GET_SKILL(ch, SKILL_TRACK)) / 2, 3, 20);
    track.sense = std::max(1, (int)((GET_SKILL(ch, SKILL_TRACK)) / 3));
    track.range = std::max(1, (int)((GET_SKILL(ch, SKILL_TRACK)) / 3));

    if (IS_NPC(ch) && !POSSESSED(ch) && EVENT_FLAGGED(ch, EVENT_TRACK))
        return;

    if (EVENT_FLAGGED(ch, EVENT_TRACK)) {
        char_printf(ch, "You stop tracking.\n");
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return;
    }

    argument = one_argument(argument, arg);
    if (!strcasecmp(" follow", argument) || !strcasecmp(" f", argument)) {
        if (GET_CLASS(ch) != CLASS_HUNTER && (GET_LEVEL(ch) < LVL_IMMORT)) {
            char_printf(ch, "You do not have enough skills to follow someone after hunting.\n");
            return;
        }
        follow = true;
    }

    /*check for second argument :follow */
    if (!*arg) {
        char_printf(ch, "Whom are you trying to hunt?\n");
        return;
    }

    find_track_victim(ch, arg, track.range, &vict);

    if (!vict) {
        char_printf(ch, "You can not seem to find tracks for that person.\n");
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    /* Now vict is the person we are trying to track. */

    if (vict == ch) {
        char_printf(ch, "Awesome!   You've found yourself!\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) >= LVL_IMMORT) {
        char_printf(ch, "You're pretty sure you're not the predator of this relationship.\n");
        return;
    }

    if (vict->in_room == ch->in_room) {
        act("$E's right here!", false, ch, 0, vict, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(vict, EFF_NOTRACK)) {
        char_printf(ch, "You can not seem to find tracks for that person.\n");
        return;
    }

    if (IS_NPC(ch) && !POSSESSED(ch) && EVENT_FLAGGED(ch, EVENT_TRACK))
        return;

    if (EVENT_FLAGGED(ch, EVENT_TRACK)) {
        char_printf(ch, "You stop hunting.\n");
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return;
    }

    if (call_track(true, track, ch, vict, follow)) {
        /*if npc dont lag mob */
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
    }
    improve_skill(ch, SKILL_HUNT);
}

void hunt_victim(CharData *ch) {
    ACMD(do_say);

    int dir, dist;
    byte found;
    CharData *tmp;

    if (!ch || !HUNTING(ch))
        return;

    /* make sure the char still exists */
    for (found = 0, tmp = character_list; tmp && !found; tmp = tmp->next)
        if (HUNTING(ch) == tmp)
            found = 1;

    if (!found) {
        do_say(ch, "Damn!   My prey is gone!!", 0, 0);
        HUNTING(ch) = 0;
        return;
    }
    dir = find_first_step(ch->in_room, HUNTING(ch)->in_room, ch, &dist);
    if (dir < 0) {
        sprintf(buf, "Damn!   Lost %s!", HMHR(HUNTING(ch)));
        do_say(ch, buf, 0, 0);
        HUNTING(ch) = 0;
        return;
    } else {
        perform_move(ch, dir, 1, false);
        if (ch->in_room == HUNTING(ch)->in_room)
            hit(ch, HUNTING(ch), TYPE_UNDEFINED);
        return;
    }
}

/* This function initiates tracking.  It checks whether the target is in range
 * and whether a suitable path exists.  If so, it creates a tracking event for
 * the tracker. */

bool call_track(bool hunt, TrackInfo track, CharData *ch, CharData *victim, bool follow) {
    int dir, dist;
    TrackDelayedEventObj *track_event;

    /* Difficult to track over water */
    if (SECT(ch->in_room) == SECT_SHALLOWS || SECT(ch->in_room) == SECT_WATER) {
        /* With a very high track, you can indeed track over water.
         * Others will have great difficulty. */
        int prob = GET_SKILL(ch, SKILL_TRACK) - 5;
        if (prob < 15)
            prob = 15;
        if (random() % 100 > prob) {
            char_printf(ch, "All traces seem to have been lost in the waves...\n");
            if (!IS_NPC(ch) && !POSSESSED(ch))
                WAIT_STATE(ch, PULSE_VIOLENCE);
            return false;
        }
    }

    if (track.speed <= 0)
        track.speed = 5;
    if (track.range <= 0)
        track.range = 5;

    dir = find_first_step(IN_ROOM(ch), IN_ROOM(victim), ch, &dist);

    if (dir == BFS_NO_PATH || dist > track.sense) {
        /* Nope, you got nothing */
        char_printf(ch, "You can not seem to find tracks for that person.\n");
        return false;
    } else if (dist == 0) {
        /* Yeah, the target seems to be right here... */
        char_printf(ch, "Funny, the tracks seem to end right here!\n");
        return false;
    } else if (dist > track.range) {
        /* Out of range, but still sense-able. */
        if ((GET_CLASS(ch) == CLASS_RANGER) || (GET_CLASS(ch) == CLASS_HUNTER)) {
            if ((dist - track.range) <= 5) {
                char_printf(ch, "You get a very very strong sense of {}.\n", HMHR(victim));
            } else if ((dist - track.range) <= 10) {
                act("You sense $M strongly, but you cannot find any tracks.\n", false, ch, 0, victim, TO_CHAR);
            } else {
                char_printf(ch, "Hmmm... You only just sense {}.\n", HMHR(victim));
            }
        } else {
            act("Hmmm... You sense $M.  $E must be close.\n", false, ch, 0, victim, TO_CHAR);
        }
        return false;
    } else {
        /* In range.  Let the tracking commence. */
        char_printf(ch, "You begin to search for tracks...\n");
        CREATE(track_event, TrackDelayedEventObj, 1);
        track_event->ch = ch;
        track_event->victim = victim;
        track_event->track = track;
        if (hunt)
            if (follow)
                track_event->track_room = -2;
            else
                track_event->track_room = -1;
        else
            track_event->track_room = victim->in_room;

        event_create(EVENT_TRACK, track_delayed_event, track_event, true, &(ch->events), track.speed);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return true;
    }
}

/* This function is called by the track event.  It makes the tracker
 * take one step toward the target, if possible.  The trail might be
 * lost, or a door might be in the way. */

bool cause_single_track(TrackInfo track, CharData *ch, CharData *victim, int track_room) {
    int direction, cmd, dist;
    char doorname[40];

    if (track_room <= -1)
        direction = find_first_step(ch->in_room, victim->in_room, ch, &dist);
    else
        direction = find_first_step(ch->in_room, track_room, ch, &dist);

    /* Must check the direction first.  dist will only be meaningful if an
     * actual direction was returned. */

    switch (direction) {
    case BFS_ERROR:
        return false;
    case BFS_ALREADY_THERE:
        char_printf(ch, "The tracks come to an end here!\n");
        if (track_room == -2) { /*follow victim */
            cmd = find_command("follow");
            do_follow(ch, GET_NAMELIST(victim), cmd, 0);
        }
        return false;
    case BFS_NO_PATH:
        return false;
    }

    /* Has the victim has moved out of range? */
    if (dist > track.range) {
        char_printf(ch, "The trail has faded away.\n");
        return false;
    }

    /* Might lose the trail over water */
    if (SECT(ch->in_room) == SECT_SHALLOWS || SECT(ch->in_room) == SECT_WATER) {
        int prob = GET_SKILL(ch, SKILL_TRACK) - 2;
        if (prob < 2)
            prob = 2;
        if (random() % 100 > prob) {
            char_printf(ch, "The trail is lost in the churning water.\n");
            return false;
        }
    }

    /* Might lose the trail for stealthy people */
    if (EFF_FLAGGED(victim, EFF_STEALTH) && GET_CONCEALMENT(victim) > random_number(0, 500)) {
        char_printf(ch, "You can't seem to find any more tracks.\n");
        return false;
    }

    /* Can't track while fighting */
    if (FIGHTING(ch)) {
        char_printf(ch, "You give up the chase for the fight!\n");
        return false;
    }

    /* Eh, you started resting? */
    if (GET_STANCE(ch) < STANCE_ALERT) {
        char_printf(ch, "You are too relaxed to continue tracking now.\n");
        return false;
    }

    if (GET_POS(ch) < POS_STANDING) {
        char_printf(ch, "You stop tracking.\n");
        return false;
    }

    /* I guess you can pause for a spell. */
    if (CASTING(ch))
        return true;

    /* Mobs with MOB_STAY_ZONE shouldn't track outside the zone */
    if (IS_NPC(ch)) {
        if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && CH_ROOM(ch)->zone != CH_ROOM(victim)->zone) {
            char_printf(ch, "You lose your victim's tracks.\n");
            return false;
        }
    }

    /* Umm, I don't think this will work. */
    if (track_room <= -1) { /*if hunt stop rangers hunting in town */
        if ((GET_CLASS(ch) == CLASS_RANGER) && (GET_LEVEL(ch) < LVL_IMMORT)) {
            char_printf(ch, "You lose your victim's tracks.\n");
            return false;
        }
    }

    /*check for a door */
    if (EXIT_IS_CLOSED(CH_EXIT(ch, direction)) && GET_LEVEL(ch) < LVL_GOD) {
        strcpy(doorname, exit_name(CH_EXIT(ch, direction)));
        char_printf(ch, "You try to open the {}.\n", doorname);
        sprintf(doorname, "%s %s", doorname, dirs[direction]);
        cmd = find_command("cmd");
        do_gen_door(ch, doorname, cmd, 0);
        if (EXIT_IS_CLOSED(CH_EXIT(ch, direction))) {
            char_printf(ch, "You stop tracking.\n");
            return false;
        } else {
            return true;
        }
    }

    /* You get to move toward the victim. */
    char_printf(ch, "&0You find signs of a track {} from here!&0\n", dirs[direction]);
    if (!perform_move(ch, direction, 1, false))
        return false;
    act("&0$n searches for tracks.&0", true, ch, 0, 0, TO_ROOM);
    /*check to see if in room */
    if ((ch->in_room == track_room) || (ch->in_room == victim->in_room)) {
        char_printf(ch, "The tracks come to an end here!\n");
        if (track_room == -2) { /*follow victim */
            cmd = find_command("follow");
            do_follow(ch, GET_NAMELIST(victim), cmd, 0);
        }
        return false;
    }

    return true;
}

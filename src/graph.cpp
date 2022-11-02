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
void flush_queues(descriptor_data *d);
ACMD(do_gen_door);

struct bfs_queue_struct {
    int room;
    char dir;
    struct bfs_queue_struct *next;
};

typedef struct _tracknode tracknode;

struct _tracknode {
    room_num room;
    int direction;
    tracknode *next;
};

#define NOT_FOUND_TRACK -5000
static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;
struct char_data *find_race(char *arg, track_info track, char_data *ch);
bool call_track(bool hunt, track_info track, char_data *ch, char_data *victim, bool follow);
bool cause_single_track(track_info track, char_data *ch, char_data *victim, int track_room);

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
    struct bfs_queue_struct *curr;

    CREATE(curr, bfs_queue_struct, 1);
    curr->room = room;
    curr->dir = dir;
    curr->next = 0;

    if (queue_tail) {
        queue_tail->next = curr;
        queue_tail = curr;
    } else
        queue_head = queue_tail = curr;
}

void bfs_dequeue(void) {
    struct bfs_queue_struct *curr;

    curr = queue_head;

    if (!(queue_head = queue_head->next))
        queue_tail = 0;
    free(curr);
}

void bfs_clear_queue(void) {
    while (queue_head)
        bfs_dequeue();
}

/* find_first_step: given a source room and a target room, find the first
    step on the shortest path from the source to the target.

    It's intended for tracking.

    Return values:
      (ret)      A BFS_* constant indicating the state of the search.
      distance   The distance of the path (if any was found).
*/

int find_first_step(int src, int target, char_data *tracker, int *distance) {
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
    while (queue_head) {
        if (queue_head->room == target) {
            curr_dir = queue_head->dir;
            *distance = world[queue_head->room].bfs_distance;
            bfs_clear_queue();
            return curr_dir;
        } else {
            for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
                if (VALID_EDGE(queue_head->room, curr_dir, tracker)) {
                    bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
                    world[TOROOM(queue_head->room, curr_dir)].bfs_distance = world[queue_head->room].bfs_distance + 1;
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

int find_track_victim(char_data *ch, char *name, int maxdist, char_data **victim) {
    int i, cdist, roomschecked = 0, result = BFS_NO_PATH;
    bool found = FALSE;
    tracknode *start, *current, *new, *end;
    struct char_data *cd;

    if (!ch || !name || !victim)
        return BFS_ERROR;

    /* Did you say "track me"? */
    if (!str_cmp(name, "self") || !str_cmp(name, "me")) {
        *victim = ch;
        return BFS_ALREADY_THERE;
    }

    /* Clear room marks */
    for (i = 0; i <= top_of_world; i++)
        UNMARK(i);

    /* Get our nodes started up... */
    CREATE(start, tracknode, 1);
    start->room = IN_ROOM(ch);
    start->direction = BFS_ALREADY_THERE;
    start->next = NULL;
    current = start;
    end = start;

    world[start->room].bfs_distance = 0;
    cdist = 0;
    *victim = NULL;

    while (current && !found && cdist < maxdist) {
        /* Handle a room: current. */
        cdist = world[current->room].bfs_distance;

        /* Can the tracker see the victim in here? */
        for (cd = world[current->room].people; cd; cd = cd->next_in_room)
            if (isname(name, GET_NAMELIST(cd)) && CAN_SEE(ch, cd)) {
                /* The victim has been found! */
                found = TRUE;
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
                    CREATE(new, tracknode, 1);
                    new->room = TOROOM(current->room, i);
                    new->direction = i;
                    new->next = NULL;

                    /* This node goes on the end of our list */
                    end->next = new;
                    end = new;
                }

        current = current->next;
    }

    /* Discard the list */
    for (current = start; current;) {
        new = current->next;
        free(current);
        current = new;
    }

    return result;
}

/************************************************************************
 *  Functions and Commands which use the above fns                        *
 ************************************************************************/

EVENTFUNC(track_delayed_event) {
    struct track_delayed_event_obj *track_event = (track_delayed_event_obj *)event_obj;
    struct char_data *ch;
    struct char_data *victim;
    struct track_info track;
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
    struct char_data *vict;
    struct track_info track;
    int trackskill;

    if (FIGHTING(ch)) {
        send_to_char("You are too busy to look for a trail.\r\n", ch);
        return;
    }
    if (!GET_SKILL(ch, SKILL_TRACK) && !IS_NPC(ch)) {
        send_to_char("You have no idea how.\r\n", ch);
        return;
    }
    if (CONFUSED(ch)) {
        send_to_char("You're far too confused to track anyone!\r\n", ch);
        return;
    }

    /* Give mobs a break on track skill */
    if (!IS_PC(ch))
        trackskill = MAX(GET_LEVEL(ch), GET_SKILL(ch, SKILL_TRACK));
    else
        trackskill = GET_SKILL(ch, SKILL_TRACK);

    /* Determine the range and speed */
    track.speed = MAX(3, MIN(20, (int)((101 - trackskill) / 2)));
    track.sense = MAX(1, (int)(trackskill / 3));
    track.range = MAX(1, (int)(trackskill / 4));
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
        send_to_char("You stop tracking.\r\n", ch);
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return;
    }

    /* Figure out who they're trying to track. */
    one_argument(argument, arg);
    if (!*arg) {
        send_to_char("Whom are you trying to track?\r\n", ch);
        return;
    }

    find_track_victim(ch, arg, track.range, &vict);

    if (!vict) {
        send_to_char("You can not seem to find tracks for that person.\r\n", ch);
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    /* Now vict is the person we are trying to track. */

    if (vict == ch) {
        send_to_char("Awesome!  You've found yourself!\r\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) >= LVL_IMMORT) {
        send_to_char("You feel their touches in everything around you.  How do you expect to track a god?\r\n", ch);
        return;
    }

    if (vict->in_room == ch->in_room) {
        act("$E's right here!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(vict, EFF_NOTRACK)) {
        send_to_char("You can not seem to find tracks for that person.\r\n", ch);
        return;
    }

    if (IS_NPC(ch))
        if (POSSESSED(ch))
            if (EVENT_FLAGGED(ch, EVENT_TRACK))
                return;

    if (EVENT_FLAGGED(ch, EVENT_TRACK)) {
        send_to_char("You stop tracking.\r\n", ch);
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        return;
    }

    if (call_track(FALSE, track, ch, vict, FALSE)) {
        /*if npc dont lag mob */
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
    }
    improve_skill(ch, SKILL_TRACK);
}

ACMD(do_hunt) {
    struct char_data *vict;
    bool follow = FALSE;
    struct track_info track;

    if (FIGHTING(ch)) {
        send_to_char("You are too busy to look for a trail.\r\n", ch);
        return;
    }

    if (IS_NPC(ch) && !POSSESSED(ch))
        return;

    if (!GET_SKILL(ch, SKILL_HUNT)) {
        send_to_char("You have no idea how.\r\n", ch);
        return;
    }

    if (CONFUSED(ch)) {
        send_to_char("You're far too confused to go hunting!\r\n", ch);
        return;
    }

    track.speed = MAX(3, MIN(20, (int)((101 - GET_SKILL(ch, SKILL_TRACK)) / 2)));
    track.sense = MAX(1, (int)((GET_SKILL(ch, SKILL_TRACK)) / 3));
    track.range = MAX(1, (int)((GET_SKILL(ch, SKILL_TRACK)) / 3));

    if (IS_NPC(ch) && !POSSESSED(ch) && EVENT_FLAGGED(ch, EVENT_TRACK))
        return;

    if (EVENT_FLAGGED(ch, EVENT_TRACK)) {
        send_to_char("You stop tracking.\r\n", ch);
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return;
    }

    argument = one_argument(argument, arg);
    if (!str_cmp(" follow", argument) || !str_cmp(" f", argument)) {
        if (GET_CLASS(ch) != CLASS_HUNTER && (GET_LEVEL(ch) < LVL_IMMORT)) {
            send_to_char("You do not have enough skills to follow someone after hunting.\r\n", ch);
            return;
        }
        follow = TRUE;
    }

    /*check for second argument :follow */
    if (!*arg) {
        send_to_char("Whom are you trying to hunt?\r\n", ch);
        return;
    }

    find_track_victim(ch, arg, track.range, &vict);

    if (!vict) {
        send_to_char("You can not seem to find tracks for that person.\r\n", ch);
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
        return;
    }

    /* Now vict is the person we are trying to track. */

    if (vict == ch) {
        send_to_char("Awesome!   You've found yourself!\r\n", ch);
        return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) >= LVL_IMMORT) {
        send_to_char("You're pretty sure you're not the predator of this relationship.\r\n", ch);
        return;
    }

    if (vict->in_room == ch->in_room) {
        act("$E's right here!", FALSE, ch, 0, vict, TO_CHAR);
        return;
    }

    if (EFF_FLAGGED(vict, EFF_NOTRACK)) {
        send_to_char("You can not seem to find tracks for that person.\r\n", ch);
        return;
    }

    if (IS_NPC(ch) && !POSSESSED(ch) && EVENT_FLAGGED(ch, EVENT_TRACK))
        return;

    if (EVENT_FLAGGED(ch, EVENT_TRACK)) {
        send_to_char("You stop hunting.\r\n", ch);
        cancel_event(GET_EVENTS(ch), EVENT_TRACK);
        REMOVE_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return;
    }

    if (call_track(TRUE, track, ch, vict, follow)) {
        /*if npc dont lag mob */
        if (!IS_NPC(ch) && !POSSESSED(ch))
            WAIT_STATE(ch, PULSE_VIOLENCE);
    }
    improve_skill(ch, SKILL_HUNT);
}

void hunt_victim(char_data *ch) {
    ACMD(do_say);

    int dir, dist;
    byte found;
    struct char_data *tmp;

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
        perform_move(ch, dir, 1, FALSE);
        if (ch->in_room == HUNTING(ch)->in_room)
            hit(ch, HUNTING(ch), TYPE_UNDEFINED);
        return;
    }
}

/* This function initiates tracking.  It checks whether the target is in range
 * and whether a suitable path exists.  If so, it creates a tracking event for
 * the tracker. */

bool call_track(bool hunt, track_info track, char_data *ch, char_data *victim, bool follow) {
    int dir, dist;
    struct track_delayed_event_obj *track_event;

    /* Difficult to track over water */
    if (SECT(ch->in_room) == SECT_SHALLOWS || SECT(ch->in_room) == SECT_WATER) {
        /* With a very high track, you can indeed track over water.
         * Others will have great difficulty. */
        int prob = GET_SKILL(ch, SKILL_TRACK) - 5;
        if (prob < 15)
            prob = 15;
        if (random() % 100 > prob) {
            send_to_char("All traces seem to have been lost in the waves...\r\n", ch);
            if (!IS_NPC(ch) && !POSSESSED(ch))
                WAIT_STATE(ch, PULSE_VIOLENCE);
            return FALSE;
        }
    }

    if (track.speed <= 0)
        track.speed = 5;
    if (track.range <= 0)
        track.range = 5;

    dir = find_first_step(IN_ROOM(ch), IN_ROOM(victim), ch, &dist);

    if (dir == BFS_NO_PATH || dist > track.sense) {
        /* Nope, you got nothing */
        send_to_char("You can not seem to find tracks for that person.\r\n", ch);
        return FALSE;
    } else if (dist == 0) {
        /* Yeah, the target seems to be right here... */
        send_to_char("Funny, the tracks seem to end right here!\r\n", ch);
        return FALSE;
    } else if (dist > track.range) {
        /* Out of range, but still sense-able. */
        if ((GET_CLASS(ch) == CLASS_RANGER) || (GET_CLASS(ch) == CLASS_HUNTER)) {
            if ((dist - track.range) <= 5) {
                sprintf(buf, "You get a very very strong sense of %s.\r\n", HMHR(victim));
                send_to_char(buf, ch);
            } else if ((dist - track.range) <= 10) {
                act("You sense $M strongly, but you cannot find any tracks.\r\n", FALSE, ch, 0, victim, TO_CHAR);
            } else {
                sprintf(buf, "Hmmm... You only just sense %s.\r\n", HMHR(victim));
                send_to_char(buf, ch);
            }
        } else {
            act("Hmmm... You sense $M.  $U$E must be close.\r\n", FALSE, ch, 0, victim, TO_CHAR);
        }
        return FALSE;
    } else {
        /* In range.  Let the tracking commence. */
        send_to_char("You begin to search for tracks...\r\n", ch);
        CREATE(track_event, track_delayed_event_obj, 1);
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

        event_create(EVENT_TRACK, track_delayed_event, track_event, TRUE, &(ch->events), track.speed);
        SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
        return TRUE;
    }
}

/* This function is called by the track event.  It makes the tracker
 * take one step toward the target, if possible.  The trail might be
 * lost, or a door might be in the way. */

bool cause_single_track(track_info track, char_data *ch, char_data *victim, int track_room) {
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
        return FALSE;
    case BFS_ALREADY_THERE:
        send_to_char("The tracks come to an end here!\r\n", ch);
        if (track_room == -2) { /*follow victim */
            cmd = find_command("follow");
            do_follow(ch, GET_NAMELIST(victim), cmd, 0);
        }
        return FALSE;
    case BFS_NO_PATH:
        return FALSE;
    }

    /* Has the victim has moved out of range? */
    if (dist > track.range) {
        send_to_char("The trail has faded away.\r\n", ch);
        return FALSE;
    }

    /* Might lose the trail over water */
    if (SECT(ch->in_room) == SECT_SHALLOWS || SECT(ch->in_room) == SECT_WATER) {
        int prob = GET_SKILL(ch, SKILL_TRACK) - 2;
        if (prob < 2)
            prob = 2;
        if (random() % 100 > prob) {
            send_to_char("The trail is lost in the churning water.\r\n", ch);
            return FALSE;
        }
    }

    /* Might lose the trail for stealthy people */
    if (EFF_FLAGGED(victim, EFF_STEALTH) && GET_HIDDENNESS(victim) > number(0, 500)) {
        send_to_char("You can't seem to find any more tracks.\r\n", ch);
        return FALSE;
    }

    /* Can't track while fighting */
    if (FIGHTING(ch)) {
        send_to_char("You give up the chase for the fight!\r\n", ch);
        return FALSE;
    }

    /* Eh, you started resting? */
    if (GET_STANCE(ch) < STANCE_ALERT) {
        send_to_char("You are too relaxed to continue tracking now.\r\n", ch);
        return FALSE;
    }

    if (GET_POS(ch) < POS_STANDING) {
        send_to_char("You stop tracking.\r\n", ch);
        return FALSE;
    }

    /* I guess you can pause for a spell. */
    if (CASTING(ch))
        return TRUE;

    /* Mobs with MOB_STAY_ZONE shouldn't track outside the zone */
    if (IS_NPC(ch)) {
        if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && CH_ROOM(ch)->zone != CH_ROOM(victim)->zone) {
            send_to_char("You lose your victim's tracks.\r\n", ch);
            return FALSE;
        }
    }

    /* Umm, I don't think this will work. */
    if (track_room <= -1) { /*if hunt stop rangers hunting in town */
        if ((GET_CLASS(ch) == CLASS_RANGER) && (GET_LEVEL(ch) < LVL_IMMORT)) {
            send_to_char("You lose your victim's tracks.\r\n", ch);
            return FALSE;
        }
    }

    /*check for a door */
    if (EXIT_IS_CLOSED(CH_EXIT(ch, direction)) && GET_LEVEL(ch) < LVL_GOD) {
        strcpy(doorname, exit_name(CH_EXIT(ch, direction)));
        sprintf(buf, "You try to open the %s.\r\n", doorname);
        send_to_char(buf, ch);
        sprintf(doorname, "%s %s", doorname, dirs[direction]);
        cmd = find_command("cmd");
        do_gen_door(ch, doorname, cmd, 0);
        if (EXIT_IS_CLOSED(CH_EXIT(ch, direction))) {
            cprintf(ch, "You stop tracking.\r\n");
            return FALSE;
        } else {
            return TRUE;
        }
    }

    /* You get to move toward the victim. */
    sprintf(buf, "&0You find signs of a track %s from here!&0\r\n", dirs[direction]);
    send_to_char(buf, ch);
    if (!perform_move(ch, direction, 1, FALSE))
        return FALSE;
    act("&0$n searches for tracks.&0", TRUE, ch, 0, 0, TO_ROOM);
    /*check to see if in room */
    if ((ch->in_room == track_room) || (ch->in_room == victim->in_room)) {
        send_to_char("The tracks come to an end here!\r\n", ch);
        if (track_room == -2) { /*follow victim */
            cmd = find_command("follow");
            do_follow(ch, GET_NAMELIST(victim), cmd, 0);
        }
        return FALSE;
    }

    return TRUE;
}

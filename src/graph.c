/***************************************************************************
 * $Id: graph.c,v 1.53 2010/06/05 04:43:57 mud Exp $
 ***************************************************************************/
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

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "casting.h"
#include "events.h"
#include "races.h"
#include "skills.h"
#include "constants.h"
#include "math.h"
#include "exits.h"
#include "rooms.h"
#include "fight.h"
#include "movement.h"
#include "directions.h"

/* Externals */
ACMD(do_follow);
void flush_queues(struct descriptor_data *d);
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
struct char_data *find_race(char *arg, struct track_info track, struct char_data *ch);
bool call_track (bool hunt, struct track_info track, struct char_data *ch,
      struct char_data *victim, bool follow);
bool cause_single_track(struct track_info track, struct char_data *ch,
      struct char_data *victim, int track_room);

/* Utility macros */
#define MARK(room) (world[room].bfs_distance = 0)
#define UNMARK(room) (world[room].bfs_distance = -1)
#define BFS_MARKED(room) (world[room].bfs_distance != -1)
#define TOROOM(x, y) (world[(x)].exits[(y)]->to_room)
/*
#define EXIT_CLOSED(x, y) (IS_SET(world[(x)].exits[(y)]->exit_info, EX_CLOSED))
#define EXIT_HIDDEN(x, y) (IS_SET(world[(x)].exits[(y)]->exit_info, EX_HIDDEN))
*/

#define VALID_EDGE(roomvnum, dir, tracker) \
   (world[(roomvnum)].exits[(dir)] && \
   (TOROOM(roomvnum, dir) != NOWHERE) && \
   (!IS_NPC(tracker) || !ROOM_FLAGGED(TOROOM(roomvnum, dir), ROOM_NOTRACK)) && \
   (!EXIT_IS_HIDDEN(world[roomvnum].exits[dir])) && \
   (!BFS_MARKED(TOROOM(roomvnum, dir))))

void bfs_enqueue(int room, int dir)
{
   struct bfs_queue_struct *curr;

   CREATE(curr, struct bfs_queue_struct, 1);
   curr->room = room;
   curr->dir = dir;
   curr->next = 0;

   if (queue_tail) {
      queue_tail->next = curr;
      queue_tail = curr;
   } else
      queue_head = queue_tail = curr;
}


void bfs_dequeue(void)
{
   struct bfs_queue_struct *curr;

   curr = queue_head;

   if (!(queue_head = queue_head->next))
      queue_tail = 0;
   free(curr);
}


void bfs_clear_queue(void)
{
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

int find_first_step(int src, int target, struct char_data *tracker, int *distance)
{
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
               world[TOROOM(queue_head->room, curr_dir)].bfs_distance =
                  world[queue_head->room].bfs_distance + 1;
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

int find_track_victim(struct char_data *ch, char *name, int maxdist, struct char_data **victim) {
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

      if (found || roomschecked++ > MAX_BFS_ROOMS) break;

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
      free (current);
      current = new;
   }

   return result;
}

/************************************************************************
 *  Functions and Commands which use the above fns                        *
 ************************************************************************/

EVENTFUNC(track_delayed_event)
{
   struct track_delayed_event_obj *track_event = (struct track_delayed_event_obj *) event_obj;
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

   /* re-queue event*/
   if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SLOW_TRACK))
      return track.speed + 20;
   return track.speed;
}

ACMD(do_track)
{
   struct char_data *vict;
   struct track_info track;
   int trackskill;

   if (FIGHTING(ch)) {
      send_to_char("You are too busy to look for a trail.\r\n",ch);
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

   if (call_track (FALSE, track, ch, vict, FALSE)) {
      /*if npc dont lag mob*/
      if (!IS_NPC(ch) && !POSSESSED(ch))
         WAIT_STATE(ch, PULSE_VIOLENCE);
   }
   improve_skill(ch, SKILL_TRACK);
}


ACMD(do_hunt)
{
   struct char_data *vict;
   bool follow = FALSE;
   struct track_info track;

   if (FIGHTING(ch)) {
      send_to_char("You are too busy to look for a trail.\r\n",ch);
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

   track.speed = MAX(3, MIN(20, (int)((101 - GET_SKILL(ch, SKILL_TRACK))/2)));
   track.sense = MAX(1, (int)((GET_SKILL(ch, SKILL_TRACK))/3));
   track.range = MAX(1, (int)((GET_SKILL(ch, SKILL_TRACK))/3));

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
         send_to_char("You do not have enough skills to follow someone after hunting", ch);
         return;
      }
      follow = TRUE;
   }

   /*check for second argument :follow*/
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

   if (call_track (TRUE, track, ch, vict, follow)) {
      /*if npc dont lag mob*/
      if (!IS_NPC(ch) && !POSSESSED(ch))
         WAIT_STATE(ch, PULSE_VIOLENCE);
   }
   improve_skill(ch, SKILL_HUNT);
}



void hunt_victim(struct char_data * ch)
{
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

bool call_track(bool hunt, struct track_info track, struct char_data *ch,
         struct char_data *victim, bool follow)
{
   int dir, dist;
   struct track_delayed_event_obj *track_event;

   /* Difficult to track over water */
   if (
            SECT(ch->in_room) == SECT_SHALLOWS ||
            SECT(ch->in_room) == SECT_WATER) {
       /* With a very high track, you can indeed track over water.
         * Others will have great difficulty. */
       int prob = GET_SKILL(ch, SKILL_TRACK) - 5;
       if (prob < 15) prob = 15;
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
   } else if (dist > track.range ) {
      /* Out of range, but still sense-able. */
      if ((GET_CLASS(ch) == CLASS_RANGER) || (GET_CLASS(ch) == CLASS_HUNTER)) {
         if ((dist - track.range) <= 5) {
            sprintf(buf, "You get a very very strong sense of %s.\r\n", HMHR(victim));
            send_to_char(buf, ch);
         } else if ((dist - track.range) <= 10) {
            act("You sense $M strongly, but you cannot find any tracks.\r\n",
                  FALSE, ch, 0, victim, TO_CHAR);
         } else {
            sprintf(buf, "Hmmm... You only just sense %s.\r\n", HMHR(victim));
            send_to_char(buf, ch);
         }
      } else {
         act("Hmmm... You sense $M.  $U$E must be close.\r\n",
               FALSE, ch, 0, victim, TO_CHAR);
      }
      return FALSE;
   } else {
      /* In range.  Let the tracking commence. */
      send_to_char("You begin to search for tracks...\r\n", ch);
      CREATE(track_event, struct track_delayed_event_obj, 1);
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

      event_create(EVENT_TRACK, track_delayed_event, track_event, TRUE,
                           &(ch->events), track.speed);
      SET_FLAG(GET_EVENT_FLAGS(ch), EVENT_TRACK);
      return TRUE;
   }
}


/* This function is called by the track event.  It makes the tracker
 * take one step toward the target, if possible.  The trail might be
 * lost, or a door might be in the way. */

bool cause_single_track(struct track_info track, struct char_data *ch,
      struct char_data *victim, int track_room)
{
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
         if (track_room == -2) {/*follow victim*/
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
   if (
            SECT(ch->in_room) == SECT_SHALLOWS ||
            SECT(ch->in_room) == SECT_WATER) {
       int prob = GET_SKILL(ch, SKILL_TRACK) - 2;
       if (prob < 2) prob = 2;
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
      send_to_char("You are too relaxed to continue tracking now.\r\n",ch);
      return FALSE;
   }

   if (GET_POS(ch) < POS_STANDING) {
      send_to_char("You stop tracking.\r\n",ch);
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
   if (track_room <= -1) {/*if hunt stop rangers hunting in town*/
      if ((GET_CLASS(ch) == CLASS_RANGER) && (GET_LEVEL(ch) < LVL_IMMORT)) {
         send_to_char("You lose your victim's tracks.\r\n", ch);
         return FALSE;
      }
   }

   /*check for a door*/
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
   /*check to see if in room*/
   if ((ch->in_room == track_room) || (ch->in_room == victim->in_room)) {
      send_to_char("The tracks come to an end here!\r\n", ch);
      if (track_room == -2) {/*follow victim*/
         cmd = find_command("follow");
         do_follow(ch, GET_NAMELIST(victim), cmd, 0);
      }
      return FALSE;
   }

   return TRUE;
}

/***************************************************************************
 * $Log: graph.c,v $
 * Revision 1.53  2010/06/05 04:43:57  mud
 * Replacing ocean sector type with cave.
 *
 * Revision 1.52  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.51  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.50  2009/01/17 00:28:02  myc
 * Fix possible use of uninitialized variable.
 *
 * Revision 1.49  2008/09/20 17:51:57  jps
 * Stop tracking if you are not successful in opening a door.
 *
 * Revision 1.48  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.47  2008/09/04 06:47:36  jps
 * Changed sector constants to match their strings
 *
 * Revision 1.46  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.45  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.44  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.43  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.42  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.41  2008/04/20 04:09:59  jps
 * Forgot to remove do_dumbmobtrack
 *
 * Revision 1.40  2008/04/20 03:54:41  jps
 * Cut out a lot of deadweight and make mobs use the same tracking
 * mechanisms as players.
 *
 * Revision 1.39  2008/04/19 22:18:27  jps
 * Remove extra and often incorrect message about why you had to stop tracking.
 *
 * Revision 1.38  2008/04/13 19:40:57  jps
 * Prevent hunting and tracking when confused.
 *
 * Revision 1.37  2008/04/13 17:46:52  jps
 * Now only NPCs are stopped by !TRACK flags.
 * Also removed a couple of unused functions.
 *
 * Revision 1.36  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.35  2008/04/05 18:07:09  myc
 * Re-implementing stealth for hide points.
 *
 * Revision 1.34  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.33  2008/03/09 18:10:53  jps
 * perform_move may be misdirected now.
 *
 * Revision 1.32  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.31  2008/02/09 21:07:50  myc
 * Must provide a boolean to event_create saying whether to
 * free the event obj when done or not.
 *
 * Revision 1.30  2008/02/09 18:29:11  myc
 * The event code now handles freeing of event objects.  The track
 * event now uses an event flag instead of storing the track event
 * in a special char_data field.
 *
 * Revision 1.29  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.28  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.27  2008/01/26 12:30:34  jps
 * Use skills.h to import improve_skill().
 *
 * Revision 1.26  2008/01/18 20:30:11  myc
 * Fixing some send_to_char strings that don't end with a newline.
 *
 * Revision 1.25  2008/01/12 23:13:20  myc
 * Removed is_aggr_to_trackee.
 *
 * Revision 1.24  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.23  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.22  2007/11/18 06:03:30  myc
 * Fix crash bug for when summon is used by mobs.
 *
 * Revision 1.21  2007/10/17 18:21:03  myc
 * Summon now uses the find_track_victim algorithm to locate the closest
 * target.  find_track_victim can be passed a room flag mask to skip
 * rooms with those flags.
 *
 * Revision 1.20  2007/08/15 20:48:07  myc
 * Gods tracking no longer open doors.
 *
 * Revision 1.19  2007/08/03 22:00:11  myc
 * Fixed some \r\n typoes in send_to_chars.
 *
 * Revision 1.18  2007/07/11 23:30:16  jps
 * Revert previous change because it introduced a crash bug.
 *
 * Revision 1.16  2007/06/09 18:01:40  jps
 * Make track find the nearest mob of the requested name, rather than
 * the first mob in the world's list who has the requested name.
 *
 * Revision 1.15  2007/04/19 00:53:54  jps
 * Create macros for stopping spellcasting.
 *
 * Revision 1.14  2006/11/11 10:16:47  jps
 * Make tracking over water much more difficult, except for superb trackers.
 *
 * Revision 1.13  2006/11/08 09:16:04  jps
 * Fixed some loose-lose typos.
 *
 * Revision 1.12  2006/11/08 08:52:42  jps
 * Fix typo 'tracks come to a end here' -> 'an end'
 *
 * Revision 1.11  2006/07/20 07:37:43  cjd
 * Typo fixes.
 *
 * Revision 1.10  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.9  2001/07/08 16:21:25  mtp
 * stop tracking if position < fighting (ie resting or less)
 *
 * Revision 1.8  2000/11/21 18:45:49  rsd
 * y
 * Altered the comment header and added missing back rlog
 * messages from prior to the addition of the $log$ string.
 *
 * Revision 1.7  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.6  1999/09/03 23:05:01  mtp
 * added some IS_FIGHTING checks
 *
 * Revision 1.5  1999/04/30 17:09:14  mud
 * reverted to 1.3 due to bad crashbugs. Gurlaek
 *
 * Revision 1.4  1999/04/29 03:47:25  jimmy
 * Nulled some pointers for sanity.  The * room flag ROOM_BFS_MARK
 * appears to work ok.
 * --Gurlaek
 *
 * Revision 1.3  1999/02/20 18:41:36  dce
 * Adds improve_skill calls so that players can imprve their skills.
 *
 * Revision 1.2  1999/01/31 02:49:06  mud
 * Added info to comment header
 * Indented entire file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

/***************************************************************************
 * $Id: dg_wldcmd.c,v 1.59 2009/06/09 19:33:50 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: dg_wldcmd.c                                    Part of FieryMUD *
 *  Usage: contains the command_interpreter for rooms,room commands.       *
 *  $Author: myc $                                                         *
 *  $Date: 2009/06/09 19:33:50 $                                           *
 *  $Revision: 1.59 $                                                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *                                                                         *
 * This code was received origonally from HubisMUD in 1998 and no lable or *
 * claim of ownership or copyright was made anywhere in the file.          *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "screen.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "quest.h"
#include "constants.h"
#include "chars.h"
#include "events.h"
#include "regen.h"
#include "exits.h"
#include "olc.h" /* for real_zone */
#include "pfiles.h"
#include "players.h"
#include "movement.h"
#include "limits.h"
#include "damage.h"
#include "directions.h"

void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
long asciiflag_conv(char *flag);
room_data *get_room(char *name);
int script_driver(void *go_address, trig_data *trig, int type, int mode);
int get_room_location(char *name);
struct find_context find_dg_by_name(char *name);

#define WCMD(name)  \
    void (name)(room_data *room, struct trig_data * t, char *argument, int cmd, int subcmd)


struct wld_command_info {
  char *command;
  void (*command_pointer)
       (room_data *room, struct trig_data * t, char *argument, int cmd, int subcmd);
       int        subcmd;
};


/* do_wsend */
#define SCMD_WSEND        0
#define SCMD_WECHOAROUND  1



/* attaches room vnum to msg and sends it to script_log */
void wld_log(room_data *room, struct trig_data *t,char *msg)
{
  char buf[MAX_INPUT_LENGTH + 100];

  void script_log(struct trig_data *t,char *msg);

  sprintf(buf, "(TRG)(room %d): %s", room->vnum, msg);
  script_log(t,buf);
}


/* sends str to room */
void act_to_room(char *str, room_data *room)
{
  /* no one is in the room */
  if (!room->people)
    return;

  /*
   * since you can't use act(..., TO_ROOM) for an room, send it
   * TO_ROOM and TO_CHAR for some char in the room.
   * (just dont use $n or you might get strange results)
   */
  act(str, FALSE, room->people, 0, 0, TO_ROOM);
  act(str, FALSE, room->people, 0, 0, TO_CHAR);
}



/* World commands */

/* prints the argument to all the rooms aroud the room */
WCMD(do_wasound)
{
  int  door;

  skip_spaces(&argument);

  if (!*argument) {
    wld_log(room, t, "wasound called with no argument");
    return;
  }

  for (door = 0; door < NUM_OF_DIRS; door++) {
    struct exit *exit;

    if ((exit = room->exits[door]) && (exit->to_room != NOWHERE) &&
        room != &world[exit->to_room])
      act_to_room(argument, &world[exit->to_room]);
  }
}


WCMD(do_wecho)
{
  skip_spaces(&argument);

  if (!*argument)
    wld_log(room, t, "wecho called with no args");

  else
    act_to_room(argument, room);
}


WCMD(do_wsend)
{
  char buf[MAX_INPUT_LENGTH], *msg;
  char_data *ch;

  msg = any_one_arg(argument, buf);

  if (!*buf)
    {
      wld_log(room, t, "wsend called with no args");
      return;
    }

  skip_spaces(&msg);

  if (!*msg)
    {
      wld_log(room, t, "wsend called without a message");
      return;
    }

  if ((ch = find_char_around_room(room, find_dg_by_name(buf))))
    {
      if (subcmd == SCMD_WSEND)
        sub_write(msg, ch, TRUE, TO_CHAR);
      else if (subcmd == SCMD_WECHOAROUND)
        sub_write(msg, ch, TRUE, TO_ROOM);
    }

  else
    wld_log(room, t, "no target found for wsend");
}

WCMD(do_wzoneecho)
{
  int zone_rnum, zone_vnum;
  char zone_name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

  msg = any_one_arg(argument, zone_name);
  skip_spaces(&msg);

  if (!*zone_name || !*msg)
    wld_log(room, t, "wzoneecho called with too few args");
  else if (!isdigit(*zone_name)) {
    sprintf(buf, "wzoneecho called with invalid zone number \"%s\"", zone_name);
    wld_log(room, t, buf);
  } else {
    zone_vnum = atoi(zone_name);
    if ((zone_rnum = real_zone(zone_vnum)) < 0) {
      sprintf(buf, "wzoneecho called for nonexistent zone %s", zone_name);
      wld_log(room, t, buf);
    } else {
      sprintf(buf, "%s\r\n", msg);
      send_to_zone(buf, zone_vnum, NOWHERE, STANCE_RESTING);
    }
  }
}


WCMD(do_wdoor)
{
  char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
  char field[MAX_INPUT_LENGTH], *value, *desc;
  room_data *rm;
  struct exit *exit;
  int dir, fd, to_room;

  const char *door_field[] = {
    "purge",
    "description",
    "flags",
    "key",
    "name",
    "room",
    "\n"
  };


  argument = two_arguments(argument, target, direction);
  value = one_argument(argument, field);
  skip_spaces(&value);

  if (!*target || !*direction || !*field) {
    wld_log(room, t, "wdoor called with too few args");
    return;
  }

  if ((rm = get_room(target)) == NULL) {
    wld_log(room, t, "wdoor: invalid target");
    return;
  }

  if ((dir = parse_direction(direction)) == -1) {
    wld_log(room, t, "wdoor: invalid direction");
    return;
  }

  if ((fd = searchblock(field, door_field, FALSE)) == -1) {
    wld_log(room, t, "wdoor: invalid field");
    return;
  }

  exit = rm->exits[dir];

  /* purge exit */
  if (fd == 0) {
    if (exit) {
      if (exit->general_description)
        free(exit->general_description);
      if (exit->keyword)
        free(exit->keyword);
      free(exit);
      rm->exits[dir] = NULL;
    }
  }

  else {
    if (!exit) {
      exit = create_exit(NOWHERE);
      rm->exits[dir] = exit;
    }

    switch (fd) {
    case 1:  /* description */
      if (exit->general_description)
        free(exit->general_description);
      CREATE(desc, char, strlen(value) + 1);
      strcpy(desc, value);
      replace_str(&desc, "\\n", "\r\n", 1, MAX_INPUT_LENGTH);
      CREATE(exit->general_description, char, strlen(desc) + 3);
      strcpy(exit->general_description, desc);
      strcat(exit->general_description, "\r\n");
      free(desc);
      break;
    case 2:  /* flags       */
      exit->exit_info = (int)asciiflag_conv(value);
      break;
    case 3:  /* key         */
      exit->key = atoi(value);
      break;
    case 4:  /* name        */
      if (exit->keyword)
        free(exit->keyword);
      CREATE(exit->keyword, char, strlen(value) + 1);
      strcpy(exit->keyword, value);
      break;
    case 5:  /* room        */
      if ((to_room = real_room(atoi(value))) != NOWHERE)
        exit->to_room = to_room;
      else
        wld_log(room, t, "wdoor: invalid door target");
      break;
    }
  }
}


WCMD(do_wteleport)
{
  char_data *ch, *next_ch;
  int target;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    wld_log(room, t, "wteleport called with too few args");
    return;
  }

  target = get_room_location(arg2);

  if (target == NOWHERE)
    wld_log(room, t, "wteleport target is an invalid room");

  else if (!str_cmp(arg1, "all")) {
    if (world[target].vnum == room->vnum) {
      wld_log(room, t, "wteleport all target is itself");
      return;
    }

    for (ch = room->people; ch; ch = next_ch) {
      next_ch = ch->next_in_room;

      dismount_char(ch);
      char_from_room(ch);
      char_to_room(ch, target);
    }
  }

  else {
    if ((ch = find_char_around_room(room, find_dg_by_name(arg1)))) {
      dismount_char(ch);
      char_from_room(ch);
      char_to_room(ch, target);
    }

    else
      wld_log(room, t, "wteleport: no target found");
  }
}


WCMD(do_wforce)
{
  char_data *ch, *next_ch;
  char arg1[MAX_INPUT_LENGTH], *line;

  line = one_argument(argument, arg1);

  if (!*arg1 || !*line) {
    wld_log(room, t, "wforce called with too few args");
    return;
  }

  if (!str_cmp(arg1, "all"))
    {
      for (ch = room->people; ch; ch = next_ch)
        {
          next_ch = ch->next_in_room;

          if (GET_LEVEL(ch)<LVL_IMMORT)
            {
              command_interpreter(ch, line);
            }
        }
    }

  else
    {
      if ((ch = find_char_around_room(room, find_dg_by_name(arg1))))
        {
          if (GET_LEVEL(ch)<LVL_IMMORT)
            {
              command_interpreter(ch, line);
            }
        }

      else
        wld_log(room, t, "wforce: no target found");
    }
}


/* increases the target's exp */
WCMD(do_wexp)
{
  char_data *ch;
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

  two_arguments(argument, name, amount);

  if (!*name || !*amount) {
    wld_log(room, t, "wexp: too few arguments");
    return;
  }

  if ((ch = find_char_around_room(room, find_dg_by_name(name))))
    gain_exp(ch, atoi(amount), GAIN_IGNORE_LEVEL_BOUNDARY |
                               GAIN_IGNORE_LOCATION);
  else {
    wld_log(room, t, "wexp: target not found");
    return;
  }
}


/* purge all objects an npcs in room, or specified object or mob */
WCMD(do_wpurge)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *ch, *next_ch;
  obj_data *obj, *next_obj;

  one_argument(argument, arg);

  if (!*arg) {
    for (ch = room->people; ch; ch = next_ch ) {
      next_ch = ch->next_in_room;
      if (IS_NPC(ch))
         fullpurge_char(ch);
    }

    for (obj = room->contents; obj; obj = next_obj ) {
      next_obj = obj->next_content;
      extract_obj(obj);
    }

    return;
  }

  if (!(ch = find_char_around_room(room, find_dg_by_name(arg)))) {
    if ((obj = find_obj_around_room(room, find_by_name(arg)))) {
      extract_obj(obj);
    } else
      wld_log(room, t, "wpurge: bad argument");

    return;
  }

  if (!IS_NPC(ch)) {
    wld_log(room, t, "wpurge: purging a PC");
    return;
  }

  fullpurge_char(ch);
}


/* loads a mobile or object into the room */
WCMD(do_wload)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int number = 0, rnum;
  char_data *mob;
  obj_data *object;


  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
    wld_log(room, t, "wload: bad syntax");
    return;
  }

  if (is_abbrev(arg1, "mob")) {
    if ((mob = read_mobile(number, VIRTUAL)) == NULL) {
      wld_log(room, t, "wload: bad mob vnum");
      return;
    }
    if( (rnum = real_room(room->vnum)) == NOWHERE) {
      wld_log(room, t, "wload: room is NOWHERE");
      return;
    }
    char_to_room(mob, rnum);
    load_mtrigger(mob);
  }

  else if (is_abbrev(arg1, "obj")) {
    if ((object = read_object(number, VIRTUAL)) == NULL) {
      wld_log(room, t, "wload: bad object vnum");
      return;
    }

    if( (rnum = real_room(room->vnum)) == NOWHERE) {
      wld_log(room, t, "wload: room is NOWHERE");
      return;
    }
    obj_to_room(object, rnum);
  }

  else
    wld_log(room, t, "wload: bad type");
}

WCMD(do_wheal) {
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  char_data *ch;

  two_arguments(argument, name, amount);

  if (!*name || !*amount || !isdigit(*amount)) {
    wld_log(room, t, "wheal: bad syntax");
    return;
  }

  dam = atoi(amount);
  if (dam > 32767 )
          dam = 32767;        /* hitpoint is a short signed int */

  if ((ch = find_char_around_room(room, find_dg_by_name(name)))) {
    if (GET_LEVEL(ch)>=LVL_IMMORT) {
      send_to_char("Being a god, you don't need healing.\r\n", ch);
      return;
    }
    hurt_char(ch, NULL, -dam, TRUE);
  }
  else
    wld_log(room, t, "wheal: target not found");
}

WCMD(do_wdamage) {
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], damtype[MAX_INPUT_LENGTH];
  int dam = 0, dtype = DAM_UNDEFINED;
  char_data *ch;

  t->damdone = 0;

  argument = one_argument(argument, name);
  argument = one_argument(argument, amount);

  if (!*name || !*amount || !isdigit(*amount)) {
    wld_log(room, t, "wdamage: bad syntax");
    return;
  }

  dam = atoi(amount);
  if (dam > 32767)
    dam = 32767;        /* hitpoint is a short signed int */

  ch = find_char_around_room(room, find_dg_by_name(name));
  if (!ch) {
    wld_log(room, t, "wdamage: target not found");
    return;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT) return;

  /* Check for and use optional damage-type parameter */
  argument = one_argument(argument, damtype);
  if (*damtype) {
    dtype = parse_damtype(0, damtype);
    if (dtype == DAM_UNDEFINED) {
      sprintf(buf,
            "wdamage called with invalid third argument (\"%s\") - not a damage type",
            damtype);
      wld_log(room, t, buf);
      return;
    }
    dam = dam * susceptibility(ch, dtype) / 100;
    if (!dam) return;
  }

  t->damdone = dam;
  sethurtevent(0, ch, dam);
}

/*
 * Room version of do_quest
 * note, we don't return anything regardless of success of fail (whats a room gonna do?)
 * and we DONT allow the godly commands (rewind, restart) or stage since its a bit
 * pointless..
 * conversley, we CAN match any player in the mud, even invis/hidden whatever
 */
WCMD(do_wld_quest)
{
  perform_quest(t, argument, NULL, NULL, room);
}

WCMD(do_wat) {
  char location[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  room_data *r2;

  void wld_command_interpreter(room_data *room, struct trig_data *t, char *argument);

  half_chop(argument, location, arg2);

  if (!*location || !*arg2 || (!isdigit(*location) && *location != UID_CHAR)) {
    wld_log(room, t, "wat: bad syntax");
    return;
  }
  r2 = get_room(location);
  if (r2 == NULL) {
    wld_log(room, t, "wat: location not found");
    return;
  }

  wld_command_interpreter(r2, t, arg2);
}

WCMD(do_w_run_room_trig) {
   char arg1[MAX_INPUT_LENGTH];
   int runtrig, found = 0;
   struct script_data *sc;
   trig_data *tr;

   if (!*argument) {
      wld_log(room, t, "w_run_room_trig called with no argument");
      return;
   }

   one_argument(argument, arg1);

   if (!*arg1 || !is_number(arg1)) {
      wld_log(room, t, "w_run_room_trig: bad syntax");
      return;
   }

   runtrig = atoi(arg1);

   if ((sc = SCRIPT(room))) {
      for (tr = TRIGGERS(sc); tr; tr = tr->next) {
         if (GET_TRIG_VNUM(tr) == runtrig) {
            found = 1;
            break;
         }
      }
   }

   if (found) {
      script_driver(&room, tr, WLD_TRIGGER, TRIG_NEW);
   } else {
      sprintf(buf, "w_run_room_trig finds no such trigger %d", runtrig);
      wld_log(room, t, buf);
   }
}


WCMD(do_wld_log) {
  wld_log(room, t, argument);
}

WCMD(do_wrent)
{
  struct char_data *ch;

  extern void rem_memming(struct char_data *ch);

  argument = any_one_arg(argument, arg);

  if (!*arg) {
    wld_log(room, t, "wrent called with no args");
    return;
  }

  if (!(ch = find_char_around_room(room, find_dg_by_name(arg)))) {
    wld_log(room, t, "no target found for wsend");
    return;
  }

  if (IS_NPC(ch)) {
    wld_log(room, t, "wrent target is not player");
    return;
  }

  if (!ch->desc)
    wld_log(room, t, "wrent called on player without descriptor");

  if (PLR_FLAGGED(ch, PLR_MEDITATE)) {
    act("$N ceases $s meditative trance.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("&8You stop meditating.\r\n&0", ch);
    REMOVE_FLAG(PLR_FLAGS(ch), PLR_MEDITATE);
  }

  rem_memming(ch);
  sprintf(buf, "%s rented by trigger %d in %s (%d).", GET_NAME(ch),
          GET_TRIG_VNUM(t), world[ch->in_room].name,
          world[ch->in_room].vnum);
  mudlog(buf, NRM, LVL_IMMORT, TRUE);
  remove_player_from_game(ch, QUIT_WRENT);
}

const struct wld_command_info wld_cmd_info[] = {
  { "RESERVED", 0, 0 },/* this must be first -- for specprocs */

  { "wasound"    , do_wasound   , 0 },
  { "wdoor"      , do_wdoor     , 0 },
  { "wecho"      , do_wecho     , 0 },
  { "wechoaround", do_wsend     , SCMD_WECHOAROUND },
  { "wexp"       , do_wexp      , 0 },
  { "wforce"     , do_wforce    , 0 },
  { "wload"      , do_wload     , 0 },
  { "wpurge"     , do_wpurge    , 0 },
  { "wsend"      , do_wsend     , SCMD_WSEND },
  { "wteleport"  , do_wteleport , 0 },
  { "wzoneecho"  , do_wzoneecho , 0 },
  { "wdamage"    , do_wdamage   , 0 },
  { "wheal"      , do_wheal     , 0 },
  { "wat"        , do_wat       , 0 },
  { "wrent"      , do_wrent     , 0 },
  { "quest"      , do_wld_quest , 0 },
  { "log"        , do_wld_log   , 0 },
  { "w_run_room_trig", do_w_run_room_trig, 0 },
  { "\n", 0, 0 }        /* this must be last */
};


/*
 *  This is the command interpreter used by rooms, called by script_driver.
 */
void wld_command_interpreter(room_data *room, struct trig_data *t,char *argument)
{
  int cmd, length;
  char *line, arg[MAX_INPUT_LENGTH];

  skip_spaces(&argument);

  /* just drop to next line for hitting CR */
  if (!*argument)
    return;

  line = any_one_arg(argument, arg);


  /* find the command */
  for (length = strlen(arg), cmd = 0;
       *wld_cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(wld_cmd_info[cmd].command, arg, length))
      break;

  if (*wld_cmd_info[cmd].command == '\n') {
    sprintf(buf2, "Unknown world cmd: '%s'", argument);
    wld_log(room, t, buf2);
  } else
    ((*wld_cmd_info[cmd].command_pointer)
     (room, t, line, cmd, wld_cmd_info[cmd].subcmd));
}

/***************************************************************************
 * $Log: dg_wldcmd.c,v $
 * Revision 1.59  2009/06/09 19:33:50  myc
 * Rewrote gain_exp and retired gain_exp_regardless.
 *
 * Revision 1.58  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.57  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.56  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.55  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.54  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.53  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.52  2008/08/26 04:39:21  jps
 * Changed IN_ZONE to IN_ZONE_RNUM or IN_ZONE_VNUM and fixed zone_printf.
 *
 * Revision 1.51  2008/08/26 03:42:01  jps
 * More detailed error reporting for wzoneecho.
 *
 * Revision 1.50  2008/08/17 20:23:56  jps
 * Use macro parse_direction
 *
 * Revision 1.49  2008/07/27 05:50:34  jps
 * Include players.h header file.
 *
 * Revision 1.48  2008/07/27 05:30:45  jps
 * Using remove_player_from_game function for rent trigger.
 *
 * Revision 1.47  2008/07/27 01:33:22  jps
 * Added room vnum to trigger-rented message.
 *
 * Revision 1.46  2008/07/07 05:42:18  myc
 * Added 'wrent' command.
 *
 * Revision 1.45  2008/06/19 18:53:12  myc
 * Now using real_zone() from olc.c.
 *
 * Revision 1.44  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.43  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.42  2008/05/14 05:10:06  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.41  2008/05/11 05:48:55  jps
 * Calling alter_hit() which also takes care of position changes.
 *
 * Revision 1.40  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.39  2008/04/05 20:42:21  jps
 * wdamage sets an event to do damage rather than doing it itself.
 *
 * Revision 1.38  2008/04/05 19:43:46  jps
 * Set damdone to the damage done by wdamage. Don't send any messages
 * when someone receives 0 damage.
 *
 * Revision 1.37  2008/04/05 18:35:57  jps
 * Allow an optional third parameter to wdamage which specifies a
 * damage type, allowing the victim to resist.
 *
 * Revision 1.36  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.35  2008/04/02 03:24:44  myc
 * Removed unnecessary function declaration.
 *
 * Revision 1.34  2008/02/02 19:56:51  myc
 * script_driver now requires an address
 *
 * Revision 1.33  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.32  2008/01/17 19:23:07  myc
 * Modified wdoor, wat, and wteleport to accept room UIDs.
 *
 * Revision 1.31  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.30  2007/12/19 20:49:42  myc
 * send_to_zone is now in a header file.
 *
 * Revision 1.29  2007/10/17 17:18:04  myc
 * Renamed the search_block and search_block2 functions.
 * searchblock is now case sensitive, and search_block is not.
 *
 * Revision 1.28  2007/08/30 19:42:46  jps
 * Cause *purge dg script commands to destroy all of a mobile's inventory
 * and equipment when purging mobs.
 *
 * Revision 1.27  2007/08/30 11:09:12  jps
 * Allow "\n" embedded in wdoor desc commands to insert newlines.
 *
 * Revision 1.26  2007/08/03 22:00:11  myc
 * Fixed some \r\n typoes in send_to_chars.
 *
 * Revision 1.25  2007/07/25 00:38:03  jps
 * Give send_to_zone a room to skip, and make it use virtual zone number.
 *
 * Revision 1.24  2007/07/24 23:34:00  jps
 * Add a parameter min_position to send_to_zone()
 *
 * Revision 1.23  2007/07/24 23:02:52  jps
 * Minor typo fix.
 *
 * Revision 1.22  2007/05/11 19:34:15  myc
 * Modified the quest command functions so they are thin wrappers for
 * perform_quest() in quest.c.  Error handling and messages should be
 * much better now.  Advance and rewind now accept another argument
 * specifying how many stages to advance or rewind.
 *
 * Revision 1.21  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.20  2006/11/30 05:02:40  jps
 * Add w_run_room_trig
 *
 * Revision 1.19  2006/11/14 20:41:49  jps
 * Make trap damage regenerate normally.
 *
 * Revision 1.18  2006/11/12 02:31:01  jps
 * You become unmounted when magically moved to another room.
 *
 * Revision 1.17  2003/07/29 03:36:42  rsd
 * added (TRG) to the logging output of the log command
 * for ease of parsing.
 *
 * Revision 1.16  2003/07/24 22:22:30  jjl
 * Added the "log" command for mob, room, and object triggers.  Spits
 * whatever you want into the log.
 *
 * Revision 1.15  2002/09/19 01:07:53  jjl
 * Update to add in quest variables!
 *
 * Revision 1.14  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.13  2001/07/25 06:59:02  mtp
 * modified logging to hopefully be a bit more helpful by specifying the
 * trigger id wherever possible. This does not apply to logging of mob trigs yet
 * as mobs use the same commands as players :-(
 *
 * Revision 1.12  2001/06/19 23:46:52  mtp
 * improved quest error messages
 *
 * Revision 1.11  2000/11/22 23:15:13  mtp
 * added ability to use quest command in here
 *
 * Revision 1.10  2000/11/21 04:42:14  rsd
 * Altered the comment header and added the early rlog
 * messages that were left out prior to the addition of
 * the $log$ string.
 *
 * Revision 1.9  2000/11/03 17:28:33  jimmy
 * Added better checks for real_room to stop players/objs from
 * being placed in room NOWHERE.  This should help pinpoint any
 * weirdness.
 *
 * Revision 1.8  2000/10/07 00:45:24  mtp
 * amount of hp affected in wheal and wdamage is limited to a signed short int (max hp is restrivcted)
 *
 * Revision 1.7  2000/03/27 22:16:48  mtp
 * added wheal which wasnt there before for some reason
 *
 * Revision 1.6  1999/11/20 00:28:32  rsd
 * Fixed a coders change that was checked in over the paladin
 * exp fix, thus deleting it.  The coder failed to diff in his
 * change so it fried the file.
 * It's fixed now
 *
 * Revision 1.4  1999/10/30 15:33:07  rsd
 * Jimmy coded alignement restrictions for paladins and exp
 * altered gain_exp() to reference victim alignment for check.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/31 02:10:44  mud
 * Alterred comment header
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

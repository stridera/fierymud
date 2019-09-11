/***************************************************************************
 * $Id: dg_objcmd.c,v 1.36 2011/03/16 13:39:58 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: objcmd.c                                       Part of FieryMUD  *
 *  Usage: contains the command_interpreter for objects, object commands.  *
 *  $Author: myc $                                                         *
 *  $Date: 2011/03/16 13:39:58 $                                           *
 *  $Revision: 1.36 $                                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
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
#include "chars.h"
#include "events.h"
#include "regen.h"
#include "movement.h"
#include "limits.h"
#include "damage.h"

void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
int get_room_location(char *room);
struct find_context find_dg_by_name(char *name);


#define OCMD(name)  \
   void (name)(obj_data *obj, struct trig_data *t, char *argument, int cmd, int subcmd)


struct obj_command_info {
  char *command;
  void        (*command_pointer)(obj_data *obj, struct trig_data *t,char *argument, int cmd, int subcmd);
  int        subcmd;
};


/* do_osend */
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1



/* attaches object name and vnum to msg and sends it to script_log */
void obj_log(obj_data *obj, struct trig_data *t, char *msg)
{
  char buf[MAX_INPUT_LENGTH + 100];

  void script_log(struct trig_data *t, char *msg);

  sprintf(buf, "(TRG)(object %d): %s", GET_OBJ_VNUM(obj), msg);
  script_log(t, buf);
}


/* returns the real room number that the object or object's carrier is in */
int obj_room(obj_data *obj)
{
  if (obj->in_room != NOWHERE)
    return obj->in_room;
  else if (obj->carried_by)
    return IN_ROOM(obj->carried_by);
  else if (obj->worn_by)
    return IN_ROOM(obj->worn_by);
  else if (obj->in_obj)
    return obj_room(obj->in_obj);
  else
    return NOWHERE;
}


/* returns the real room number, or NOWHERE if not found or invalid */
int find_obj_target_room(obj_data *obj, char *rawroomstr)
{
  int location;
  char_data *target_mob;
  obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr)
    return NOWHERE;

  /*
   * See if the string refers to a room UID or vnum first.  If no
   * matching room is found, try a character, and then an object.
   */
  if ((location = get_room_location(roomstr)) != NOWHERE);
  else if ((target_mob = find_char_around_obj(obj, find_dg_by_name(roomstr))) &&
           (location = IN_ROOM(target_mob)) != NOWHERE);
  else if ((target_obj = find_obj_around_obj(obj, find_by_name(roomstr))) &&
           (location = obj_room(target_obj)) != NOWHERE);
  else return NOWHERE;

  /* A room has been found.  Check for permission */
  if (ROOM_FLAGGED(location, ROOM_GODROOM) ||
      ROOM_FLAGGED(location, ROOM_HOUSE))
    return NOWHERE;

  if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
      world[location].people && world[location].people->next_in_room)
    return NOWHERE;

  return location;
}



/* Object commands */

OCMD(do_oecho)
{
  int room;

  skip_spaces(&argument);

  if (!*argument)
    obj_log(obj, t, "oecho called with no args");

  else if ((room = obj_room(obj)) != NOWHERE)
    {
      if (world[room].people)
        sub_write(argument, world[room].people, TRUE, TO_ROOM | TO_CHAR);
    }

  else
    obj_log(obj, t, "oecho called by object in NOWHERE");
}


OCMD(do_oforce)
{
  char_data *ch, *next_ch;
  int room;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  /* two_arguments only returns two _words_ but our force string
  ** may be more than one word..
  */
  /*two_arguments(argument, arg1, arg2);*/
  strncpy(arg2, one_argument(argument, arg1), MAX_INPUT_LENGTH);

  if (!*arg1 || !*arg2)
    {
      obj_log(obj, t, "oforce called with too few args");
      return;
    }

  if (!str_cmp(arg1, "all"))
    {
      if ((room = obj_room(obj)) == NOWHERE)
        obj_log(obj, t, "oforce called by object in NOWHERE");
      else
        {
          for (ch = world[room].people; ch; ch = next_ch)
            {
              next_ch = ch->next_in_room;

              if (GET_LEVEL(ch)<LVL_IMMORT)
                {
                  command_interpreter(ch, arg2);
                }
            }
        }
    }

  else
    {
      if ((ch = find_char_around_obj(obj, find_dg_by_name(arg1))))
        {
          if (GET_LEVEL(ch)<LVL_IMMORT)
            {
              command_interpreter(ch, arg2);
            }
        }

      else
        obj_log(obj, t, "oforce: no target found");
    }
}


OCMD(do_osend)
{
  char buf[MAX_INPUT_LENGTH], *msg;
  char_data *ch;

  msg = any_one_arg(argument, buf);

  if (!*buf)
    {
      obj_log(obj, t, "osend called with no args");
      return;
    }

  skip_spaces(&msg);

  if (!*msg)
    {
      obj_log(obj, t, "osend called without a message");
      return;
    }

  if ((ch = find_char_around_obj(obj, find_dg_by_name(buf))))
    {
      if (subcmd == SCMD_OSEND)
        sub_write(msg, ch, TRUE, TO_CHAR);
      else if (subcmd == SCMD_OECHOAROUND)
        sub_write(msg, ch, TRUE, TO_ROOM);
    }

  else
    obj_log(obj, t, "no target found for osend");
}

/* increases the target's exp */
OCMD(do_oexp)
{
  char_data *ch;
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

  two_arguments(argument, name, amount);

  if (!*name || !*amount)
    {
      obj_log(obj, t, "oexp: too few arguments");
      return;
    }

  if ((ch = find_char_around_obj(obj, find_dg_by_name(name))))
    gain_exp(ch, atoi(amount), GAIN_IGNORE_LEVEL_BOUNDARY |
                               GAIN_IGNORE_LOCATION);
  else
    {
      obj_log(obj, t, "oexp: target not found");
      return;
    }
}


/* purge all objects an npcs in room, or specified object or mob */
OCMD(do_opurge)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *ch, *next_ch;
  obj_data *o, *next_obj;
  int rm;

  one_argument(argument, arg);

  if (!*arg) {
    if ((rm = obj_room(obj))) {
      for (ch = world[rm].people; ch; ch = next_ch) {
        next_ch = ch->next_in_room;
        if (IS_NPC(ch))
          fullpurge_char(ch);
      }
      for (o = world[rm].contents; o; o = next_obj) {
         next_obj = o->next_content;
         if (o != obj)
           extract_obj(o);
      }
    }
    return;
  }

  if (!(ch = find_char_around_obj(obj, find_dg_by_name(arg)))) {
    if ((o = find_obj_for_keyword(obj, arg)))
      extract_obj(obj);
    else if ((o = find_obj_around_obj(obj, find_by_name(arg))))
      extract_obj(o);
    else
      obj_log(obj, t, "opurge: bad argument");
      return;
  }

  if (!IS_NPC(ch)) {
    obj_log(obj, t, "opurge: attempting to purge PC");
    return;
  }

  fullpurge_char(ch);
}


OCMD(do_oteleport)
{
  char_data *ch, *next_ch;
  int target, rm;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    obj_log(obj, t, "oteleport called with too few args");
    return;
  }

  target = find_obj_target_room(obj, arg2);

  if (target == NOWHERE)
    obj_log(obj, t, "oteleport target is an invalid room");

  else if (!str_cmp(arg1, "all")) {
    rm = obj_room(obj);
    if (target == rm)
      obj_log(obj, t, "oteleport target is itself");

    for (ch = world[rm].people; ch; ch = next_ch) {
      next_ch = ch->next_in_room;
      dismount_char(ch);
      char_from_room(ch);
      char_to_room(ch, target);
    }
  }

  else {
    if ((ch = find_char_around_obj(obj, find_dg_by_name(arg1)))) {
      dismount_char(ch);
      char_from_room(ch);
      char_to_room(ch, target);
    }
    else
      obj_log(obj, t, "oteleport: no target found");
  }
}


OCMD(do_dgoload)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int number = 0, room;
  char_data *mob;
  obj_data *object;

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0))
    {
      obj_log(obj, t, "oload: bad syntax");
      return;
    }

  if ((room = obj_room(obj)) == NOWHERE)
    {
      obj_log(obj, t, "oload: object in NOWHERE trying to load");
      return;
    }

  if (is_abbrev(arg1, "mob"))
    {
      if ((mob = read_mobile(number, VIRTUAL)) == NULL)
        {
          obj_log(obj, t, "oload: bad mob vnum");
          return;
        }
      char_to_room(mob, room);
      load_mtrigger(mob);
    }

  else if (is_abbrev(arg1, "obj"))
    {
      if ((object = read_object(number, VIRTUAL)) == NULL)
        {
          obj_log(obj, t, "oload: bad object vnum");
          return;
        }

      obj_to_room(object, room);
    }

  else
    obj_log(obj, t, "oload: bad type");

}

OCMD(do_oheal) {
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  char_data *ch;

  two_arguments(argument, name, amount);

  if (!*name || !*amount || !isdigit(*amount)) {
    obj_log(obj, t, "oheal: bad syntax");
    return;
  }

  dam = atoi(amount);

  if ((ch = find_char_around_obj(obj, find_dg_by_name(name)))) {
    if (GET_LEVEL(ch)>=LVL_IMMORT) {
      send_to_char("Being a god, you don't need healing.\r\n", ch);
      return;
    }
    hurt_char(ch, NULL, -dam, TRUE);
  }
  else
    obj_log(obj, t, "oheal: target not found");
}

OCMD(do_odamage) {
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], damtype[MAX_INPUT_LENGTH];
  int dam = 0, dtype = DAM_UNDEFINED;
  char_data *ch;

  t->damdone = 0;
  argument = one_argument(argument, name);
  argument = one_argument(argument, amount);

  if (!*name || !*amount || !isdigit(*amount)) {
    obj_log(obj, t, "odamage: bad syntax");
    return;
  }

  dam = atoi(amount);

  ch = find_char_around_obj(obj, find_dg_by_name(name));
  if (!ch) {
    obj_log(obj, t, "odamage: target not found");
    return;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT) return;

  /* Check for and use optional damage-type parameter */
  argument = one_argument(argument, damtype);
  if (*damtype) {
    dtype = parse_damtype(0, damtype);
    if (dtype == DAM_UNDEFINED) {
      sprintf(buf,
            "odamage called with invalid third argument (\"%s\") - not a damage type",
            damtype);
      obj_log(obj, t, buf);
      return;
    }
    dam = dam * susceptibility(ch, dtype) / 100;
    if (!dam) return;
  }

  t->damdone = dam;
  sethurtevent(0, ch, dam);
}

/*
 * Object version of do_quest
 * note, we don't return anything regardless of success of fail (whats an object gonna do?)
 * and we DONT allow the godly commands (rewind, restart) or stage since its a bit
 * pointless..
 * conversley, we CAN match any player in the mud, even invis/hidden whatever
 */
OCMD(do_obj_quest)
{
  perform_quest(t, argument, NULL, obj, NULL);
}

OCMD(do_obj_log) {
  obj_log(obj, t, argument);
}

const struct obj_command_info obj_cmd_info[] = {
  { "RESERVED", 0, 0 },/* this must be first -- for specprocs */

  { "oecho"      , do_oecho    , 0 },
  { "oechoaround", do_osend    , SCMD_OECHOAROUND },
  { "oexp"       , do_oexp     , 0 },
  { "oforce"     , do_oforce   , 0 },
  { "oload"         , do_dgoload  , 0 },
  { "opurge"     , do_opurge   , 0 },
  { "osend"      , do_osend    , SCMD_OSEND },
  { "oteleport"  , do_oteleport, 0 },
  { "odamage"    , do_odamage  , 0 },
  { "oheal"      , do_oheal    , 0 },
  { "quest"         , do_obj_quest, 0 },
  { "log"        , do_obj_log  , 0 },
  { "\n", 0, 0 }        /* this must be last */
};



/*
 *  This is the command interpreter used by objects, called by script_driver.
 */
void obj_command_interpreter(obj_data *obj, struct trig_data *t,char *argument)
{
  int cmd, length;
  char *line, arg[MAX_INPUT_LENGTH];

  skip_spaces(&argument);

  /* just drop to next line for hitting CR */
  if (!*argument)
    return;

  line = any_one_arg(argument, arg);


  /* find the command */
  for (length = strlen(arg),cmd = 0;
       *obj_cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(obj_cmd_info[cmd].command, arg, length))
      break;

  if (*obj_cmd_info[cmd].command == '\n')
    {
      sprintf(buf2, "Unknown object cmd: '%s'", argument);
      obj_log(obj, t, buf2);
    }
  else
    ((*obj_cmd_info[cmd].command_pointer)
     (obj, t, line, cmd, obj_cmd_info[cmd].subcmd));
}


/***************************************************************************
 * $Log: dg_objcmd.c,v $
 * Revision 1.36  2011/03/16 13:39:58  myc
 * Fix all warnings for "the address of X will always evaluate to 'true'",
 * where X is a variable.
 *
 * Revision 1.35  2009/06/09 19:33:50  myc
 * Rewrote gain_exp and retired gain_exp_regardless.
 *
 * Revision 1.34  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.33  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.32  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.31  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.30  2008/05/14 05:09:30  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.29  2008/05/11 05:48:33  jps
 * Calling alter_hit() which also takes care of position changes.
 *
 * Revision 1.28  2008/04/05 20:41:53  jps
 * odamage sets an event to do damage rather than doing it itself.
 *
 * Revision 1.27  2008/04/05 19:44:22  jps
 * Set damdone to the damage done by odamage. Don't send any messages
 * when someone receives 0 damage.
 *
 * Revision 1.26  2008/04/05 18:46:13  jps
 * Allow a third parameter, damage type, to odamage, which will
 * allow the victim to resist.
 *
 * Revision 1.25  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.24  2008/04/02 03:24:44  myc
 * Removed unnecessary function declaration.
 *
 * Revision 1.23  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.22  2008/01/18 20:30:11  myc
 * Fixing some send_to_char strings that don't end with a newline.
 *
 * Revision 1.21  2008/01/17 19:23:07  myc
 * The find_obj_target_room function now accepts a room UID, and
 * is used by oteleport.
 *
 * Revision 1.20  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.19  2007/08/30 19:42:46  jps
 * Cause *purge dg script commands to destroy all of a mobile's inventory
 * and equipment when purging mobs.
 *
 * Revision 1.18  2007/05/11 19:34:15  myc
 * Modified the quest command functions so they are thin wrappers for
 * perform_quest() in quest.c.  Error handling and messages should be
 * much better now.  Advance and rewind now accept another argument
 * specifying how many stages to advance or rewind.
 *
 * Revision 1.17  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.16  2006/11/14 20:41:49  jps
 * Make trap damage regenerate normally.
 *
 * Revision 1.15  2006/11/12 02:31:01  jps
 * You become unmounted when magically moved to another room.
 *
 * Revision 1.14  2003/07/29 03:36:42  rsd
 * added (TRG) to the logging output of the log command
 * for ease of parsing.
 *
 * Revision 1.13  2003/07/24 22:22:30  jjl
 * Added the "log" command for mob, room, and object triggers.  Spits
 * whatever you want into the log.
 *
 * Revision 1.12  2002/09/19 01:07:53  jjl
 * Update to add in quest variables!
 *
 * Revision 1.11  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.10  2001/07/25 06:59:02  mtp
 * modified logging to hopefully be a bit more helpful by specifying the
 * trigger id wherever possible. This does not apply to logging of mob trigs yet
 * as mobs use the same commands as players :-(
 *
 * Revision 1.9  2001/06/19 23:46:52  mtp
 * improved quest error messages
 *
 * Revision 1.8  2000/11/22 23:15:13  mtp
 * added ability to use quest command in here
 *
 * Revision 1.7  2000/11/21 03:44:47  rsd
 * Altered the comment header slightly and added the back
 * rlog messages from prior to the addition of then $log$
 * string.
 *
 * Revision 1.6  2000/03/27 22:16:48  mtp
 * added oheal which wasnt there before for some reason
 *
 * Revision 1.5  2000/02/01 21:04:42  mtp
 * fixed oforce command so that it takes a line instead of
 * just a second word
 *
 * Revision 1.4  1999/10/30 15:30:42  rsd
 * Jimmy coded alignment restrictions for paladins and exp
 * altered gain_exp() to reference the victim.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/31 00:53:55  mud
 * Added to the comment header
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

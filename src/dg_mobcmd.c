/***************************************************************************
 * $Id: dg_mobcmd.c,v 1.70 2009/06/09 19:33:50 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: dg_mobcmd.c                                    Part of FieryMUD *
 *  Usage: See below                                                       *
 *     By: Apparetntly N'Atas-ha                                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *                                                                         *
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "dg_scripts.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"
#include "skills.h"
#include "casting.h"
#include "quest.h"
#include "screen.h"
#include "math.h"
#include "pfiles.h"
#include "chars.h"
#include "events.h"
#include "exits.h"
#include "fight.h"
#include "players.h"
#include "movement.h"
#include "limits.h"
#include "damage.h"
#include "directions.h"

extern int get_room_location(char *room);
extern int obj_room(struct obj_data *obj);


void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
int script_driver(void *go_address, trig_data *trig, int type, int mode);


/*
 * Local functions.
 */

/* attaches mob's name and vnum to msg and sends it to script_log */
void mob_log(char_data *mob, char *msg)
{
  char buf[MAX_INPUT_LENGTH + 100];

  void script_log(struct trig_data *t, char *msg);

  sprintf(buf, "(TRG)(mob %d): %s", GET_MOB_VNUM(mob), msg);
  script_log((struct trig_data *)NULL,buf);
}


int find_mob_target_room(struct char_data *ch, char *rawroomstr)
{
  int location;
  struct char_data *target_char;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr)
    return NOWHERE;

  /*
   * Don't try to get a room if there's a '.', because that usually
   * indicates that the script wants to locate something like '3.guard'.
   */
  if ((location = get_room_location(roomstr)) != NOWHERE);
  else if ((target_char = find_char_for_mtrig(ch, roomstr)) &&
           (location = IN_ROOM(target_char)) != NOWHERE);
  else if ((target_obj = find_obj_for_mtrig(ch, roomstr)) &&
           (location = obj_room(target_obj)) != NOWHERE);
  else return NOWHERE;

  if (ROOM_FLAGGED(location, ROOM_GODROOM) || ROOM_FLAGGED(location, ROOM_HOUSE))
    return NOWHERE;

  return location;
}


/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch) \
  (IS_NPC(ch) && (!(ch)->desc || GET_LEVEL((ch)->desc->original)>=LVL_IMPL))


/* mob commands */


ACMD(do_mdamage) {
   char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH], damtype[MAX_INPUT_LENGTH];
   int dam = 0, dtype = DAM_UNDEFINED, *damdone = NULL;
   trig_data *t;
   char_data *victim;

   if (!MOB_OR_IMPL(ch)){
     send_to_char("Huh!?\r\n",ch);
     return;
   }

   for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
      if (t->running) {
         damdone = &(t->damdone);
         break;
      }
   }

   argument = one_argument(argument, name);
   if (damdone)
      *damdone = 0;
   else {
      sprintf(buf,
            "[ WARN: do_mdamage() for %s - can't identify running trigger ]",
            GET_NAME(ch));
      mudlog(buf, BRF, LVL_GOD, FALSE);
   }

   if (!*name) {
      mob_log(ch, "mdamage called with no arguments");
      return;
   }

   argument = one_argument(argument, amount);

   if (!*amount) {
      mob_log(ch, "mdamage called without second argument (amount)");
      return;
   }

   if (!isdigit(*amount)) {
      sprintf(buf,
            "mdamage called with invalid second argument (\"%s\") - not a number", amount);
      mob_log(ch, buf);
      return;
   }

   /* hitpoint is a short signed int */
   dam = MAX(-32767, MIN(atoi(amount), 32767));

   if (!(victim = find_char_for_mtrig(ch, name))) {
      sprintf(buf, "mdamage: victim (%s) not found", name);
      mob_log(ch, buf);
      return;
   }

   if (GET_LEVEL(victim) >= LVL_IMMORT) return;

   /* Check for and use optional damage-type parameter */
   argument = one_argument(argument, damtype);
   if (*damtype) {
      dtype = parse_damtype(0, damtype);
      if (dtype == DAM_UNDEFINED) {
         sprintf(buf,
               "mdamage called with invalid third argument (\"%s\") - not a damage type",
               damtype);
         mob_log(ch, buf);
         return;
      }
      dam = dam * susceptibility(victim, dtype) / 100;
      if (!dam) return;
   }

   if (damdone) *damdone = dam;
   sethurtevent(ch, victim, dam);
}


/* allow a mob to set ANY skill or spell based on targets class
 * and level
 * syntax mskillset <plyrname> <name_skill_or_spell>
 */
ACMD(do_mskillset)
{
  struct char_data * victim;
  char arg[MAX_INPUT_LENGTH];
  int skspnum;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n",ch);
    return;
  }
  argument = one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mskillset called with no arguments");
    return;
  }

  if (!(victim = find_char_for_mtrig(ch, arg))) {
    sprintf(buf, "mskillset: victim (%s) not found", arg);
    mob_log(ch, buf);
    return;
  }
  /*
   * we have a victim, do we have a valid skill?
   */
  skip_spaces(&argument);
  if ((skspnum = find_talent_num(argument, TALENT)) < 0) {
    /* no such spell/skill*/
    sprintf(buf,"mskillset called with unknown skill/spell '%s'",argument);
    mob_log(ch,buf);
    return;
  }

  /*
   * because we're nice really, we will give the player the max proficiency for
   * their level at this skill..don't thank me just throw money..
   */
  SET_SKILL(victim, skspnum, return_max_skill(victim,skspnum));
}

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound)
{
  int was_in_room;
  int  door;

  if (!MOB_OR_IMPL(ch))
    {
      send_to_char("Huh?!?\r\n", ch);
      return;
    }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  if (!*argument)
    {
      mob_log(ch, "masound called with no argument");
      return;
    }

  skip_spaces(&argument);

  was_in_room = IN_ROOM(ch);
  for (door = 0; door < NUM_OF_DIRS; door++)
    {
      struct exit *exit;

      if (((exit = world[was_in_room].exits[door]) != NULL) &&
	  exit->to_room != NOWHERE && exit->to_room != was_in_room)
	{
	  IN_ROOM(ch) = exit->to_room;
	  sub_write(argument, ch, TRUE, TO_ROOM);
	}
    }

  IN_ROOM(ch) = was_in_room;
}


/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mkill called with no argument");
    return;
  }

  if (!(victim = find_char_for_mtrig(ch, arg))) {
    sprintf(buf, "mkill: victim (%s) not found",arg);
    mob_log(ch, buf);
    return;
  }

  if (victim == ch) {
    mob_log(ch, "mkill: victim is self");
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM) && ch->master == victim ) {
    mob_log(ch, "mkill: charmed mob attacking master");
    return;
  }

  if (FIGHTING(ch)) {
    mob_log(ch, "mkill: already fighting");
    return;
  }

  attack(ch, victim);
  return;
}


/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk)
{
  char argbuf[MAX_INPUT_LENGTH], *arg = argbuf;
  int pos, dotmode;
  obj_data *obj;
  obj_data *obj_next;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mjunk called with no argument");
    return;
  }

  dotmode = find_all_dots(&arg);

  if (dotmode == FIND_INDIV) {
    if ((obj = find_obj_in_eq(ch, &pos, find_vis_by_name(ch, arg))) != NULL) {
      unequip_char(ch, pos);
      extract_obj(obj);
    }
    else if ((obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, arg))) != NULL) {
      extract_obj(obj);
    }
  } else {
    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
      obj_next = obj->next_content;
      /* If it is 'all.blah' then find_all_dots converts to 'blah' */
      if (dotmode == FIND_ALL || isname(arg, obj->name))
	extract_obj(obj);
    }
    if (dotmode == FIND_ALL) {
      for (pos = 0; pos < NUM_WEARS; ++pos)
        if (GET_EQ(ch, pos)) {
          unequip_char(ch, pos);
          extract_obj(obj);
        }
    }
    else {
      while ((obj = find_obj_in_eq(ch, &pos, find_vis_by_name(ch, arg)))) {
	unequip_char(ch, pos);
	extract_obj(obj);
      }
    }
  }
}


/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char *p;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char( "Huh?!?\r\n", ch );
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  p = one_argument(argument, arg);
  skip_spaces(&p);

  if (!*arg) {
    mob_log(ch, "mechoaround called with no argument");
    return;
  }

  if (!(victim = find_char_for_mtrig(ch, arg))) {
    sprintf(buf, "mechoaround: victim (%s) does not exist",arg);
    mob_log(ch, buf);
    return;
  }

  sub_write(p, victim, TRUE, TO_ROOM);
}


/* sends the message to only the victim */
ACMD(do_msend)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char *p;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char( "Huh?!?\r\n", ch );
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  p = one_argument(argument, arg);
  skip_spaces(&p);

  if (!*arg) {
    mob_log(ch, "msend called with no argument");
    return;
  }

  if (!(victim = find_char_for_mtrig(ch, arg))) {
    sprintf(buf, "msend: victim (%s) does not exist",arg);
    mob_log(ch, buf);
    return;
  }

  sub_write(p, victim, TRUE, TO_CHAR);
}


/* prints the message to the room at large */
ACMD(do_mecho)
{
  char *p;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char( "Huh?!?\r\n", ch );
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  if (!*argument) {
    mob_log(ch, "mecho called with no arguments");
    return;
  }
  p = argument;
  skip_spaces(&p);

  sub_write(p, ch, TRUE, TO_ROOM);
}

/*
** run a room trigger..particularly useful when the mob has just
** croaked (so cant use a speech trig)
*/
ACMD(do_m_run_room_trig)
{
  int thisrm,trignm,found=0;
  trig_data *t;
  struct script_data *sc;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (!*argument)
  {
    mob_log(ch, "m_run_room_trig called with no argument");
    return;
  }

  /* trigger must be in current room */
  thisrm = ch->in_room;
  trignm = atoi(argument);
  if (SCRIPT(&(world[thisrm])))
  {
    sc = SCRIPT(&(world[thisrm]));
    for (t = TRIGGERS(sc); t; t = t->next)
    {
      if (GET_TRIG_VNUM(t) == trignm)
      {
        found = 1;
	break;
      }
    }

    if (found == 1)
    {
      room_data *room = &world[thisrm];
      /* found the right trigger, now run it */
      script_driver(&room, t, WLD_TRIGGER, TRIG_NEW);
    }
    else
    {
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf,"m_run_room_trig finds no such trigger %d in room %d\n",
		      trignm,world[thisrm].vnum);
      mob_log(ch,buf);
    }
  }
}


/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
ACMD(do_mload)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int number = 0;
  char_data *mob;
  obj_data *object;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  if( ch->desc && GET_LEVEL(ch->desc->original) < LVL_IMPL)
    return;

  argument = two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
    mob_log(ch, "mload: bad syntax");
    return;
  }

  if (is_abbrev(arg1, "mob")) {
    if ((mob = read_mobile(number, VIRTUAL)) == NULL) {
      mob_log(ch, "mload: bad mob vnum");
      return;
    }
    char_to_room(mob, IN_ROOM(ch));
    load_mtrigger(mob);
  }

  else if (is_abbrev(arg1, "obj")) {
    if ((object = read_object(number, VIRTUAL)) == NULL) {
      mob_log(ch, "mload: bad object vnum");
      return;
    }
    /* Reuse arg1 for third argument: force mload to room */
    any_one_arg(argument, arg1);
    if (!CAN_WEAR(object, ITEM_WEAR_TAKE) || !str_cmp(arg1, "room"))
      obj_to_room(object, IN_ROOM(ch));
    else
      obj_to_char(object, ch);
  }

  else
    mob_log(ch, "mload: bad type");
}


/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  obj_data  *obj;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    /* 'purge' */
    char_data *vnext;
    obj_data  *obj_next;

    for (victim = world[IN_ROOM(ch)].people; victim; victim = vnext) {
      vnext = victim->next_in_room;
      if (IS_NPC(victim) && victim != ch)
         fullpurge_char(victim);
    }

    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj_next) {
      obj_next = obj->next_content;
      extract_obj(obj);
    }

    return;
  }

  victim = find_char_for_mtrig(ch, arg);

  if (victim == NULL) {
    if ((obj = find_obj_for_mtrig(ch, arg))) {
      extract_obj(obj);
    } else
      mob_log(ch, "mpurge: bad argument");

    return;
  }

  if (!IS_NPC(victim)) {
    mob_log(ch, "mpurge: purging a PC");
    return;
  }

  fullpurge_char(victim);
}


/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto)
{
  char arg[MAX_INPUT_LENGTH];
  int location;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mgoto called with no argument");
    return;
  }

  if ((location = find_mob_target_room(ch, arg)) == NOWHERE) {
    mob_log(ch, "mgoto: invalid location");
    return;
  }

  char_from_room(ch);
  char_to_room(ch, location);
}


/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat)
{
  char arg[MAX_INPUT_LENGTH];
  int location;
  int original;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  argument = one_argument( argument, arg );

  if (!*arg || !*argument) {
    mob_log(ch, "mat: bad argument");
    return;
  }

  if ((location = find_mob_target_room(ch, arg)) == NOWHERE) {
    mob_log(ch, "mat: invalid location");
    return;
  }

  original = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, argument);

  /*
   * See if 'ch' still exists before continuing!
   * Handles 'at XXXX quit' case.
   */
  if (IN_ROOM(ch) == location) {
    char_from_room(ch);
    char_to_room(ch, original);
  }
}


/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport)
{
   char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
   int target;
   char_data *vict, *next_ch;

   if (!MOB_OR_IMPL(ch)) {
      send_to_char("Huh?!?\r\n", ch);
      return;
   }

   if (EFF_FLAGGED(ch, EFF_CHARM))
      return;

   argument = two_arguments(argument, arg1, arg2);

   if (!*arg1 || !*arg2) {
      mob_log(ch, "mteleport: bad syntax");
      return;
   }

   target = find_mob_target_room(ch, arg2);

   if (target == NOWHERE)
      mob_log(ch, "mteleport target is an invalid room");

   else if (!str_cmp(arg1, "all")) {
      if (target == IN_ROOM(ch)) {
         mob_log(ch, "mteleport all target is itself");
         return;
      }

      for (vict = world[IN_ROOM(ch)].people; vict; vict = next_ch) {
         next_ch = vict->next_in_room;

         if (GET_LEVEL(vict)<LVL_IMMORT) {
            dismount_char(vict);
            char_from_room(vict);
            char_to_room(vict, target);
         }
      }
   } else {
      if (!(vict = find_char_for_mtrig(ch, arg1))) {
         sprintf(buf, "mteleport: victim (%s) does not exist",arg1);
         mob_log(ch, buf);
         return;
      }

      if (GET_LEVEL(vict)<LVL_IMMORT) {
         dismount_char(vict);
         char_from_room(vict);
         char_to_room(vict, target);
      }
   }
}


/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce)
{
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;

  argument = one_argument(argument, arg);

  if (!*arg || !*argument) {
    mob_log(ch, "mforce: bad syntax");
    return;
  }

  if (!str_cmp(arg, "all")) {
    struct descriptor_data *i;
    char_data *vch;

    for (i = descriptor_list; i ; i = i->next) {
      if ((i->character != ch) && !i->connected &&
	  (IN_ROOM(i->character) == IN_ROOM(ch))) {
	vch = i->character;
	if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) &&
	    GET_LEVEL(vch)<LVL_IMMORT) {
	  command_interpreter(vch, argument);
	}
      }
    }
  } else {
    char_data *victim;

    if (!(victim = find_char_for_mtrig(ch, arg))) {
      mob_log(ch, "mforce: no such victim");
      return;
    }

    if (victim == ch) {
      mob_log(ch, "mforce: forcing self");
      return;
    }

    if (GET_LEVEL(victim)<LVL_IMMORT)
      command_interpreter(victim, argument);
  }
}


/* increases the target's exp */
ACMD(do_mexp)
{
  char_data *victim;
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;

  two_arguments(argument, name, amount);

  if (!*name || !*amount) {
    mob_log(ch, "mexp: too few arguments");
    return;
  }

  if (!(victim = find_char_for_mtrig(ch, name))) {
    sprintf(buf, "mexp: victim (%s) does not exist",name);
    mob_log(ch, buf);
    return;
  }

  gain_exp(victim, atoi(amount), GAIN_IGNORE_LEVEL_BOUNDARY |
                                 GAIN_IGNORE_LOCATION);
}


/* increases the target's gold */
ACMD(do_mgold)
{
  char_data *victim;
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;

  two_arguments(argument, name, amount);

  if (!*name || !*amount) {
    mob_log(ch, "mgold: too few arguments");
    return;
  }

  if (!(victim = find_char_for_mtrig(ch, name))) {
    sprintf(buf, "mgold: victim (%s) does not exist",name);
    mob_log(ch, buf);
    return;
  }

  if ((GET_GOLD(victim) += atoi(amount)) < 0) {
    mob_log(ch, "mgold subtracting more gold than character has");
    GET_GOLD(victim) = 0;
  }
}

ACMD(do_mob_log) {

  char errbuf[MAX_STRING_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (EFF_FLAGGED(ch, EFF_CHARM)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;

  snprintf(errbuf, MAX_STRING_LENGTH, "ERROR mob %d (%s): %s",
	   GET_MOB_VNUM(ch), GET_NAME(ch), argument);

  mob_log(ch, argument);

}

/*
 * do_quest
 *
 * descr:	the controlling routine, this is what will get called by the gods
 * 		and the mobs using the following structure:
 * 		quest <cmd> <qname> <player>
 * 		e.g. quest start new_quest zzur
 */

ACMD(do_quest)
{
  struct trig_data *t = NULL;

  /*
   * Normal rules for execution by mobs/players, but deities can
   * use this command too.
   */
  if (!MOB_OR_IMPL(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  if (MOB_FLAGGED(ch, MOB_ANIMATED) || EFF_FLAGGED(ch, EFF_CHARM)) {
     send_to_char("Huh?!?\r\n", ch);
     return;
  }

  /* If this is running in a trigger, try to find the trigger its running
     in so that we can pass it to perform_quest */
  if (SCRIPT(ch))
    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      if (t->running)
        break;

  perform_quest(t, argument, ch, NULL, NULL);

}

/* Save a player. */
ACMD(do_msave)
{
   char arg[MAX_INPUT_LENGTH];

   if (!MOB_OR_IMPL(ch)) {
      send_to_char("Huh?!?\r\n", ch);
      return;
   }

   if (EFF_FLAGGED(ch, EFF_CHARM))
      return;

   if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
      return;

   argument = one_argument(argument, arg);

   if (!*arg) {
      mob_log(ch, "msave: bad syntax");
      return;
   }

   if (!str_cmp(arg, "all")) {
      struct descriptor_data *i;
      char_data *vch;

      for (i = descriptor_list; i ; i = i->next) {
         if ((i->character != ch) && !i->connected &&
               (IN_ROOM(i->character) == IN_ROOM(ch))) {
            vch = i->character;
            if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) &&
                  GET_LEVEL(vch) < LVL_IMMORT) {
               save_player(vch);
            }
         }
      }
   } else {
      char_data *victim;

      if (!(victim = find_char_for_mtrig(ch, arg))) {
         mob_log(ch, "msave: no such victim");
         return;
      }

      if (victim == ch) {
         mob_log(ch, "msave: forcing self");
         return;
      }

      if (IS_NPC(victim)) {
         sprintf(buf, "msave: cannot save NPC %s", arg);
         mob_log(ch, buf);
         return;
      }

      if (GET_LEVEL(victim) < LVL_IMMORT) {
         save_player(victim);
      }
   }
}

/***************************************************************************
 * $Log: dg_mobcmd.c,v $
 * Revision 1.70  2009/06/09 19:33:50  myc
 * Rewrote gain_exp and retired gain_exp_regardless.
 *
 * Revision 1.69  2009/03/19 23:16:23  myc
 * find_all_dots takes a char** and shifts the pointer up if
 * it returns FIND_ALLDOT
 *
 * Revision 1.68  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.67  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.66  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.65  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.64  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.63  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.62  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.61  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.60  2008/09/01 00:49:33  mud
 * Removing prototype imported from skills.h.
 *
 * Revision 1.59  2008/07/27 05:27:38  jps
 * Using the new save_player function.
 *
 * Revision 1.58  2008/06/05 02:07:43  myc
 * Rewrote rent saving to use ascii object files.
 *
 * Revision 1.57  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.56  2008/05/18 05:18:06  jps
 * Renaming room_data struct's member "number" to "vnum", cos it's
 * a virtual number.
 *
 * Revision 1.55  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.54  2008/04/05 20:40:21  jps
 * mdamage will set an event to cause damage. This will allow triggers
 * that use it to control what messages are sent.  It will also consolidate
 * the messages about damage which used to be copied to all *damage
 * commands.
 *
 * Revision 1.53  2008/04/05 19:44:08  jps
 * Set damdone to the damage done by mdamage. Don't send any messages
 * when someone receives 0 damage.
 *
 * Revision 1.52  2008/04/05 18:18:21  jps
 * Allow an optional third parameter to mdamage which specifies a
 * damage type, allowing the victim to resist.
 *
 * Revision 1.51  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.50  2008/04/02 03:24:44  myc
 * Removed unnecessary function declaration.
 *
 * Revision 1.49  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.48  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.47  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.46  2008/02/02 19:56:51  myc
 * script_driver now requires an address
 *
 * Revision 1.45  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.44  2008/01/27 21:09:12  myc
 * Replaced hit() with attack().
 *
 * Revision 1.43  2008/01/26 12:58:14  jps
 * Using skills.h.
 *
 * Revision 1.42  2008/01/21 00:51:19  jps
 * Allow mob trigger teleportation commands to use private rooms.
 *
 * Revision 1.41  2008/01/17 19:23:07  myc
 * Moved find_target_room_mscript here from act.wizard.c and renamed
 * it find_mob_target_room.  It now accepts a room UID, and is used
 * by
 * mat, mteleport, and mgoto.
 *
 * Revision 1.40  2008/01/13 23:06:04  myc
 * Cleaned up the mjunk command a little and fixed a bug with
 * "mjunk all.###" for three-letter names.
 *
 * Revision 1.39  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.38  2008/01/04 03:03:48  jps
 * Added msave command, so mobs can save players during triggers.
 *
 * Revision 1.37  2007/12/31 02:00:57  jps
 * Made the general term for spells, skills, chants, and songs 'talent'.
 * Fixed mskillset to handle all talents.
 *
 * Revision 1.36  2007/12/19 20:49:22  myc
 * Fixing bug with mskillset not skipping spaces.
 *
 * Revision 1.35  2007/09/28 20:49:35  myc
 * The mload command now accepts an optional third argument.  If it is
 * "room" the object is loaded to the room instead of the mob's inventory.
 *
 * Revision 1.34  2007/09/01 21:22:29  jps
 * Made _mscript detection routines.
 *
 * Revision 1.33  2007/09/01 20:34:10  jps
 * Add function get_char_in_room, which bypasses CAN_SEE, for
 * scripting m* commands.
 *
 * Revision 1.32  2007/08/30 19:42:46  jps
 * Cause *purge dg script commands to destroy all of a mobile's inventory
 * and equipment when purging mobs.
 *
 * Revision 1.31  2007/05/11 22:23:15  myc
 * Fixed a bug in the log the quest command was using to figure out
 * who was allowed to use it.
 *
 * Revision 1.30  2007/05/11 19:34:15  myc
 * Modified the quest command functions so they are thin wrappers for
 * perform_quest() in quest.c.  Error handling and messages should be
 * much better now.  Advance and rewind now accept another argument
 * specifying how many stages to advance or rewind.
 *
 * Revision 1.29  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.28  2007/01/25 17:03:30  myc
 * You can no longer switch into a mob and use mdamage/mskillset.  Mobs cannot
 * be ordered to do these either.
 *
 * Revision 1.27  2006/11/18 21:10:03  jps
 * Add mdamage for dg scripting.
 *
 * Revision 1.26  2006/11/12 02:31:01  jps
 * You become unmounted when magically moved to another room.
 *
 * Revision 1.25  2006/09/09 01:30:39  dce
 * Added checks to the quest command to prevent charmies and
 * shapechangers from using the command freely.
 *
 * Revision 1.24  2003/07/29 03:36:07  rsd
 * added (TRG) to the logging command for ease of parsing.
 *
 * Revision 1.23  2003/07/24 22:22:30  jjl
 * Added the "log" command for mob, room, and object triggers.  Spits
 * whatever you want into the log.
 *
 * Revision 1.22  2002/10/06 19:34:25  jjl
 * Added some error checking to the variable command so you can't forget a value/variable name.
 *
 * Revision 1.21  2002/09/19 01:07:22  jjl
 * Updated to give mobs the quest variable command
 *
 * Revision 1.20  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.19  2001/07/25 06:59:02  mtp
 * modified logging to hopefully be a bit more helpful by specifying the
 * trigger id wherever possible. This does not apply to logging of mob trigs yet
 * as mobs use the same commands as players :-(
 *
 * Revision 1.18  2001/07/09 23:05:54  mtp
 * removed long syntax line for quest - replace with help topic!
 *
 * Revision 1.17  2001/07/08 17:47:40  mtp
 * added quest erase to remove a quest from a player (non documented)
 *
 * Revision 1.16  2001/06/19 23:46:52  mtp
 * improved quest error messages
 *
 * Revision 1.15  2001/02/12 23:30:02  mtp
 * added inroom check for quest commands on death trigs
 *
 * Revision 1.14  2001/02/12 01:33:05  mtp
 * rmeoved some fprintf commands!
 *
 * Revision 1.13  2001/02/12 01:30:49  mtp
 * if we are in a death trigger, run the quest command on all group
 * members. NO INROOM or AWAKE check!
 *
 * Revision 1.12  2001/02/03 00:56:51  mtp
 * do a race check before starting subclass quest
 * also returing different codes so that calling procs can do something sensible
 * on failure
 *
 * Revision 1.11  2000/11/23 00:57:04  mtp
 * added mskillset to allow a mob to set skill/spell proficiency
 *
 * Revision 1.10  2000/11/21 01:53:39  rsd
 * Altered comment header to preserve proper copyright and
 * credit for work. Also added back rlog messages from prior
 * to the addition of the $log$ string.
 *
 * Revision 1.9  2000/11/07 01:39:59  mtp
 * extended quest command to have rewind and restart and also for subclasses
 * in start
 *
 * Revision 1.8  2000/11/03 05:37:17  jimmy
 * Removed the quest.h file from structs.h arg!! and placed it
 * only in the appropriate files
 * Updated the dependancies in the Makefile and created
 * make supahclean.
 *
 * Revision 1.7  2000/10/27 00:34:45  mtp
 * new do_quest command (can be run by mobs or command for gods)
 *
 * Revision 1.5  2000/02/10 23:09:40  mtp
 * fixed mjunk which inhad interpreted the return code from find_all_dots wrongly
 *
 * Revision 1.4  1999/10/30 15:26:40  rsd
 * Jimmy coded alignment restrictions for paladins and exp
 * altered gain_exp to reference the victim.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/31 00:45:50  mud
 * Added Fieryified comment header
 * indented entire file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

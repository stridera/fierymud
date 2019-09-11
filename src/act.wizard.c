/***************************************************************************
 * $Id: act.wizard.c,v 1.335 2011/03/16 13:39:58 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: act.wizard.c                                   Part of FieryMUD *
 *  Usage: Player-level god commands and other goodies                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "casting.h"
#include "house.h"
#include "screen.h"
#include "olc.h"
#include "dg_scripts.h"
#include "clan.h"
#include "quest.h"
#include "weather.h"
#include "events.h"
#include "class.h"
#include "races.h"
#include "skills.h"
#include "cooldowns.h"
#include "constants.h"
#include "math.h"
#include "players.h"
#include "pfiles.h"
#include "regen.h"
#include "exits.h"
#include "fight.h"
#include "legacy_structs.h"
#include "modify.h"
#include "act.h"
#include "movement.h"
#include "limits.h"
#include "composition.h"
#include "lifeforce.h"
#include "charsize.h"
#include "textfiles.h"

void garble_text(char *string, int percent);
void check_new_surroundings(struct char_data *ch, bool old_room_was_dark, bool tx_obvious);

extern int restrict;
extern int restrict_reason;
extern int max_group_difference;

extern int races_allowed;
extern int evil_races_allowed;
extern int pk_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int summon_allowed;
extern int roomeffect_allowed;
extern int approve_names;
extern int napprove_pause;
extern int gossip_channel_active;
extern int nameserver_is_slow;
extern int level_gain;
extern int damage_amounts;
int Valid_Name(char *newname);
int reserved_word(char *argument);

/* Automatic rebooting */
extern int reboot_auto;
extern long global_pulse;
extern long reboot_pulse;
extern int reboot_warning;
extern int last_reboot_warning;
extern int reboot_warning_minutes;

/* extern functions */
void send_to_xnames(char *name);
int find_zone(int num);
void cure_laryngitis(struct char_data *ch);
void reboot_mud_prep();
void rebootwarning(int minutes);
void update_stats(struct char_data *ch);

/* Internal funct */
void do_wiztitle(char *outbuf, struct char_data *vict, char *argu);

ACMD(do_inctime) {
   skip_spaces(&argument);

   if (str_cmp(argument, "yes") != 0) {
      send_to_char("Are you sure you want to move time forward?\r\n"
            "If so, type 'inctime yes'.\r\n", ch);
      return;
   }

   send_to_char("Advancing game time by one hour.\r\n", ch);
   increment_game_time();
}

ACMD(do_iptables) {
  extern int normalize_ip_address(char *in, char *out); /* utils.c */
  char cBuf[MAX_INPUT_LENGTH];
  char cBuf2[MAX_INPUT_LENGTH];
  char cBuf3[MAX_INPUT_LENGTH];
  char tmpfile[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct descriptor_data *desc;

  /* List of ports we want to open for coders */
  int ports_to_open[2] = {22, 80};
  /* Make sure this number matches the size of ports_to_open */
  int num_ports = 2;

  int i, num_ports_open;
  bool ports_open[2];

  FILE* tmp;

  sprintf(tmpfile,  "/tmp/iptables.list.%d", getpid());

  argument = one_argument(argument, arg);

  /* "list" is the default action */
  if (strcmp(arg, "list") == 0 || !arg[0]) {
    sprintf(cBuf3, "/sbin/iptables --list FieryMUD -v -n --line-numbers > %s",
      tmpfile);
    system(cBuf3);

    tmp = fopen(tmpfile, "rt");

    while (get_line(tmp, cBuf))
      pprintf(ch, "%s\r\n", cBuf);

    start_paging(ch);

    fclose(tmp);

    sprintf(cBuf3, "rm %s", tmpfile);
    system(cBuf3);
  } else if (strcmp(arg, "add") == 0) {
    one_argument(argument, arg);

    if (!*arg) {
      send_to_char("What player or IP address do you want allowed?\r\n", ch);
      return;
    } else {
      /* Must get normalized IP address string into buf now */

      if (normalize_ip_address(arg, buf)) {
         /* The string given is not a good IP address.  See if it's a player's name. */
         if (!strcmp("me", arg)) {
            vict = ch;
         } else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
            send_to_char("Nobody by that name (or invalid IP address).\r\n", ch);
            return;
         }

         /* We have a player's name. */
         desc = NULL;
         if (!IS_NPC(vict)) {
               if (vict->desc) {
                  desc = vict->desc;
               } else if (vict->forward && vict->forward->desc) {
                  desc = vict->forward->desc;
               }
         }
         if (!desc) {
            /* linkless */
            act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
            return;
         }
         if (!desc->host || !(*(desc->host))) {
            act("I can't seem to determine $s IP address.", FALSE, ch, 0, vict, TO_CHAR);
            return;
         }
         /* We have a valid player with a link */
         if (normalize_ip_address(desc->host, buf)) {
            sprintf(buf2, "Error! Could not normalize host '%s'!\r\n", desc->host);
            send_to_char(buf2, ch);
            return;
         }
         /* buf now contains the normlized IP address string for vict */
      }

      /* buf now contains normalized IP address string */

      if (!strcmp("127.0.0.1", buf)) {
         send_to_char("Cancelled - adding 127.0.0.1 is unnecessary.\r\n", ch);
         return;
      }

      /* Determine whether the address is already in the table. */

      sprintf(cBuf3, "/sbin/iptables --list FieryMUD -v -n --line-numbers > %s",
         tmpfile);
      system(cBuf3);

      tmp = fopen(tmpfile, "rt");

      /* Prepare the string to use when looking for the ip address */
      sprintf(buf2, " %s ", buf);
      num_ports_open = 0;
      for (i = 0; i < num_ports; i++) ports_open[i] = FALSE;

      /* Find out which ports (if any) are already opened for the
       * requested address */
      while(get_line(tmp, cBuf)) {
         if (strstr(cBuf, buf2)) {
            /* ip address found */
            for (i = 0; i < num_ports; i++) {
               if (!ports_open[i]) {
                  sprintf(buf1, " tcp dpt:%d ", ports_to_open[i]);
                  if (strstr(cBuf, buf1)) {
                     ports_open[i] = TRUE;
                     num_ports_open++;
                  }
               }
            }
         }
      }

      sprintf(cBuf3, "rm %s", tmpfile);
      system(cBuf3);
      fclose(tmp);

      if (num_ports_open == num_ports) {
         sprintf(buf1, "%s is already in the table.\r\n", buf);
         send_to_char(buf1, ch);
      } else {
         /* Open up the desired ports */
         for (i = 0; i < num_ports; i++) {
            if (!ports_open[i]) {
               sprintf(cBuf,
                   "/sbin/iptables -I FieryMUD --source %s --protocol tcp --dport %d --jump ACCEPT",
                   buf, ports_to_open[i]);
               system(cBuf);
            }
         }
         sprintf(buf2, "Ok, added %s to the table.\r\n", buf);
         send_to_char(buf2, ch);
      }
    }

  } else if (strcmp(arg, "del") == 0) {
    one_argument(argument, arg);
    if (!*arg) {
      send_to_char("Which line should be deleted?\r\n", ch);
    } else {
       int lineNum = 0;
       lineNum = atoi(arg);
       if (lineNum < 1) {
          send_to_char("That is not a valid line number.\r\n", ch);
       } else {
          sprintf(cBuf2, "/sbin/iptables -D FieryMUD %d", lineNum);
          system(cBuf2);

          send_to_char("Ok.\r\n",ch);
       }
    }
  } else {
    send_to_char("Usage:  iptables [add <ip/player>|del <number>|list]\r\n", ch);
  }

}

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes.. but what?\r\n", ch);
  else {
    if (subcmd == SCMD_EMOTE || subcmd == SCMD_EMOTES) {
      if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        send_to_char("Your lips move, but no sound forms.\r\n", ch);
        return;
      }
      if (!speech_ok(ch, 0)) return;
      if (GET_LEVEL(REAL_CHAR(ch)) < LVL_IMMORT)
        argument = strip_ansi(argument);
      sprintf(buf, "$n%s %s&0", subcmd == SCMD_EMOTES ? "'s" : "", argument);
    }
    else
      sprintf(buf, "%s@0", argument);

    act(buf, subcmd != SCMD_ECHO, ch, 0, 0, TO_ROOM);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}


ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Send what to who?\r\n", ch);
    return;
  }
  if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  cprintf(vict, "%s@0\r\n", buf);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("Sent.\r\n", ch);
  else
    cprintf(ch, "You send '%s@0' to %s.\r\n", buf, GET_NAME(vict));
}

void perform_ptell(struct char_data *ch, struct char_data *vict, char *arg)
{
  struct descriptor_data *d;

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    sprintf(buf, "&6You respond to $N, '&b%s&0&6'&0", arg);
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  }
  if (PRF_FLAGGED(vict, PRF_AFK))  {
    send_to_char("You received the previous message while AFK.\r\n", vict);
    send_to_char("That person is AFK right now but received your message.\r\n", ch);
  }

  sprintf(buf, "&6%s responds to %s's petition, '&b%s&0&6'&0\r\n",
          GET_NAME(REAL_CHAR(ch)), GET_NAME(vict), arg);
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_PLAYING && d != ch->desc &&
        GET_LEVEL(d->character) >= LVL_IMMORT &&
        !PLR_FLAGGED(d->character, PLR_WRITING) &&
        !PLR_FLAGGED(d->character, PLR_MAILING) &&
        !EDITING(d))
      send_to_char(buf, d->character);
  }

  if (vict->forward && !vict->desc)
    vict = vict->forward;

  sprintf(buf, "&6$n responds to your petition, '&b%s&0&6'&0", arg);
  act(buf, FALSE, REAL_CHAR(ch), 0, vict, TO_VICT | TO_SLEEP);
}

ACMD(do_ptell)
{
  struct char_data *vict;

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char("Who do you wish to ptell??\r\n", ch);
  else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
    send_to_char(NOPERSON, ch);
  else if (ch == vict)
    send_to_char("You need mental help. Try ptelling someone besides yourself.\r\n", ch);
  else if (GET_LEVEL(vict) >= LVL_IMMORT)
    send_to_char("Just use wiznet!\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc && (!vict->forward || !vict->forward->desc))        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (vict->desc && EDITING(vict->desc))
    act("$E's writing a message right now; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    perform_ptell(ch, vict, buf2);
}


/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
int find_target_room(struct char_data * ch, char *rawroomstr)
{
  int tmp;
  int location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char("You must supply a room number or name.\r\n", ch);
    return NOWHERE;
  }
  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return NOWHERE;
    }
  }
  else if ((target_mob = find_char_around_char(ch, find_vis_by_name(ch, roomstr))))
    location = target_mob->in_room;
  else if ((target_obj = find_obj_around_char(ch, find_vis_by_name(ch, roomstr)))) {
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("That object is not available.\r\n", ch);
      return NOWHERE;
    }
  } else {
    send_to_char("No such creature or object around.\r\n", ch);
    return NOWHERE;
  }

  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (GET_LEVEL(ch) < LVL_GOD) {
    if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
        world[location].people && world[location].people->next_in_room) {
      send_to_char("There's a private conversation going on in that room.\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
        !House_can_enter(ch, world[location].vnum)) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return NOWHERE;
    }
  }
  return location;
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  int location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("You must supply a room number or a name.\r\n", ch);
    return;
  }

  if (!*command) {
    send_to_char("What do you want to do there?\r\n", ch);
    return;
  }

  if ((location = find_target_room(ch, buf)) < 0)
    return;

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  int location;

  skip_spaces(&argument);
  if (strcmp(argument, "home") == 0) {
    if( (location = real_room(GET_HOMEROOM(ch))) < 0 ) {
      send_to_char("Your home room is invalid.\r\n", ch);
      return;
    }
  } else if ((location = find_target_room(ch, argument)) < 0) {
      return;
  }
  if (GET_POOFOUT(ch) && *GET_POOFOUT(ch))
    strcpy(buf, GET_POOFOUT(ch));
  else
    strcpy(buf, "$n disappears in a puff of smoke.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  dismount_char(ch);
  char_from_room(ch);
  char_to_room(ch, location);

  if (GET_POOFIN(ch) && *GET_POOFIN(ch))
    strcpy(buf, GET_POOFIN(ch));
  else
    strcpy(buf, "$n appears with an ear-splitting bang.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, FALSE);
}

ACMD(do_linkload)
{
  struct char_data *victim = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Linkload who?\r\n", ch);
    return;
  }
  if (find_char_around_char(ch, find_plr_by_name(arg))) {
    send_to_char("They are already connected!\r\n", ch);
    return;
  }
  CREATE(victim, struct char_data, 1);
  clear_char(victim);
  if (load_player(arg, victim) > -1) {
    if (GET_LEVEL(victim) <= GET_LEVEL(ch)) {
      victim->player.time.logon = time(0);
      sprintf(buf, "(GC) %s has link-loaded %s.", GET_NAME(ch), GET_NAME(victim));
      mudlog(buf, BRF, GET_LEVEL(ch) + 1, TRUE);
      char_to_room(victim, IN_ROOM(ch));
      load_quests(victim);
      load_objects(victim);
      victim->next = character_list;
      character_list = victim;
      victim->desc = NULL;
      act("You linkload $N.", FALSE, ch, 0, victim, TO_CHAR);
      act("$n linkloads $N.", FALSE, ch, 0, victim, TO_NOTVICT);
    } else {
      send_to_char("Sorry, you aren't high enough to link-load that char.\r\n", ch);
      free_char(victim);
    }
  } else {
    send_to_char("No such player exists.\r\n", ch);
    free(victim);
  }
  return;
}


ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;
  bool wasdark;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to transfer?\r\n", ch);
  else if (str_cmp("all", buf)) {
    if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, buf))))
      send_to_char(NOPERSON, ch);
    else if (victim == ch)
      send_to_char("That doesn't make much sense, does it?\r\n", ch);
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
        send_to_char("Go transfer someone your own size.\r\n", ch);
        return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
      dismount_char(victim);
      char_from_room(victim);
      char_to_room(victim, ch->in_room);
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      check_new_surroundings(victim, wasdark, TRUE);
    }
  } else {                        /* Trans All */
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char("I think not.\r\n", ch);
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i->character && i->character != ch) {
        victim = i->character;
        if (GET_LEVEL(victim) >= GET_LEVEL(ch))
          continue;
        act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
   wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
   dismount_char(victim);
        char_from_room(victim);
        char_to_room(victim, ch->in_room);
        act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
        act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
   check_new_surroundings(victim, wasdark, TRUE);
      }
    send_to_char(OK, ch);
  }
}



ACMD(do_teleport)
{
  struct char_data *victim;
  int target;
  bool wasdark;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char("Whom do you wish to teleport?\r\n", ch);
  else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, buf))))
    send_to_char(NOPERSON, ch);
  else if (victim == ch)
    send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("Maybe you shouldn't do that.\r\n", ch);
  else if (!*buf2)
    send_to_char("Where do you wish to send this person?\r\n", ch);
  else if ((target = find_target_room(ch, buf2)) >= 0) {
    send_to_char(OK, ch);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    wasdark = IS_DARK(victim->in_room) && !CAN_SEE_IN_DARK(victim);
    dismount_char(victim);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    check_new_surroundings(victim, wasdark, TRUE);
  }
}


ACMD(do_shutdown)
{
  extern int circle_shutdown, circle_reboot;

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("If you want to shut something down, say so!\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    sprintf(buf, "(GC) Reboot by %s.", GET_NAME(ch));
    log(buf);
    reboot_mud_prep();
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "now")) {
    sprintf(buf, "(GC) Shutdown NOW by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Rebooting.. come back in a minute or two.\r\n"
                 "           &1&b** ****** ****&0\r\n"
                "         &1&b**&0 &3&b***     *****&0  &1&b**&0\r\n"
                "       &1&b**&0 &3&b**      &1&b*&0     &3&b***&0  &1&b*&0\r\n"
                "       &1&b*&0    &3&b** **   *   *  *&0 &1&b**&0\r\n"
                "      &1&b*&0  &3&b** * *&0          &1&b*&0     &1&b*&0\r\n"
                "      &1&b*&0  &3&b*&0    &1&b**&0            &3&b* *&0 &1&b*&0\r\n"
                "     &1&b*&0 &3&b* &1&b** *&0     &3&b*   ******&0  &1&b*&0\r\n"
                "      &1&b*&0   &3&b* &1&b* **&0  &3&b***&0     &1&b*&0  &3&b*&0 &1&b*&0\r\n");
    send_to_all("        &1&b*&0  &3&b*  *&0 &1&b**********&0  &3&b***&0 &1&b*&0\r\n"
                "         &1&b*****&0   &3&b*     *   * *&0 &1&b*&0\r\n"
                "                &1&b*&0   &3&b*&0 &1&b*&0\r\n"
                "               &1&b*&0  &3&b* *&0  &1&b*&0\r\n"
                "              &1&b*&0  &3&b*  **&0  &1&b*&0\r\n"
                "              &1&b*&0 &3&b**   *&0 &1&b*&0\r\n"
                "                &1&b*&0 &3&b*&0 &1&b*&0\r\n"
                "                &1&b*&0 &3&b*&0  &1&b**&0\r\n"
                "               &1&b**&0     &1&b****&0\r\n"
                "              &1&b***&0  &3&b* *&0    &1&b****&0\r\n");
    circle_shutdown = 1;
    circle_reboot = 2;

  } else if (!str_cmp(arg, "die")) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Shutting down for maintenance.\r\n");
    touch("../.killscript");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Shutting down for maintenance.\r\n");
    touch("../pause");
    circle_shutdown = 1;
  } else
    send_to_char("Unknown shutdown option.\r\n", ch);
}


void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping)
    send_to_char("You aren't snooping anyone.\r\n", ch);
  else {
    send_to_char("You stop snooping.\r\n", ch);
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}

void perform_snoop(struct char_data *ch, struct descriptor_data *d)
{
  struct char_data *tch;

  tch = NULL;

  if (d->snooping == ch->desc) {
    send_to_char("Don't be stupid.\r\n", ch);
    return;
  } else if (d->snoop_by) {
    send_to_char("Busy already.\r\n", ch);
    return;
  } else if (d == ch->desc) {
    send_to_char("Not a good idea.\r\n", ch);
    return;
  } else if (STATE(d) != CON_PLAYING) {
    /* This is to prevent snoopers from seeing passwords. */
    send_to_char("Please wait until they've logged in.\r\n", ch);
    return;
  }

  if (d->original)
    tch = d->original;
  else if (d->character)
    tch = d->character;

  if (tch && GET_LEVEL(tch) >= GET_LEVEL(ch)) {
    send_to_char("You can't.\r\n", ch);
    return;
  }

  /* It's ok to snoop on. */
  send_to_char(OK, ch);

  /* Stop snooping whoever you were snooping on before */
  if (ch->desc->snooping)
    ch->desc->snooping->snoop_by = NULL;

  ch->desc->snooping = d;
  d->snoop_by = ch->desc;
}

ACMD(do_snoop)
{
  struct char_data *victim;
  struct descriptor_data *d;
  int dnum;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (isdigit(*arg)) {
    dnum = atoi(arg);
    for (d = descriptor_list; d; d = d->next) {
      if (d->desc_num == dnum) {
        perform_snoop(ch, d);
        return;
      }
    }
    send_to_char("No such descriptor.\r\n", ch);
  } else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg))))
    send_to_char("No such person around.\r\n", ch);
  else if (!victim->desc)
    send_to_char("There's no link.. nothing to snoop.\r\n", ch);
  else if (victim == ch)
    send_to_char("Not a good idea.\r\n", ch);
  else
    perform_snoop(ch, victim->desc);
}

ACMD(do_switch)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (POSSESSED(ch))
    send_to_char("You're already switched.\r\n", ch);
  else if (!*arg)
    send_to_char("Switch with who?\r\n", ch);
  else if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, arg))))
    send_to_char("No such character.\r\n", ch);
  else if (ch == victim)
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
  else if (victim->desc)
    send_to_char("You can't do that, the body is already in use!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GRGOD) && !IS_NPC(victim))
    send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
  else if ((GET_LEVEL(ch) < GET_LEVEL(victim)))
    send_to_char("You WISHED.\r\n",ch);
  else {
    send_to_char(OK, ch);

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;

    if (!GET_PROMPT(victim) || IS_NPC(victim))
      GET_PROMPT(victim) = strdup(GET_PROMPT(ch));
  }
}

ACMD(do_rename)
{
  int player_i;
  struct char_data *victim=NULL;
  char tmp_name[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  extern struct char_data *is_playing(char *vict_name);

  two_arguments(argument, arg1, arg2);

  if (!ch || IS_NPC(ch))
    return;

  if (!*arg1 || !*arg2) {
    send_to_char("Usage: rename <player name> <new name>\r\n",ch);
    return;
  }

  victim = is_playing(arg1);

  if (!victim) {
    send_to_char("You can only rename a player currently logged in.\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("You don't have permission to change that name.\r\n", ch);
    return;
  }

  if (_parse_name(arg2, tmp_name) || strlen(tmp_name) < 2 ||
      strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
      fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
    send_to_char("The new name is invalid.\r\n", ch);
    return;
  }

  if ((player_i = get_id_by_name(tmp_name)) >= 0) {
    send_to_char("There is already a player with that name.\r\n", ch);
    return;
  }

  sprintf(buf2, "&1&bYou have renamed &7%s&1 to &7%s&0\r\n", GET_NAME(victim), CAP(tmp_name));
  send_to_char(buf2, ch);
  sprintf(buf2, "(GC) %s has renamed %s to %s", GET_NAME(ch), GET_NAME(victim), tmp_name);
  mudlog(buf2, NRM, LVL_HEAD_C, TRUE);

  rename_player(victim, tmp_name);

  sprintf(buf2, "&1&b!!! You have been renamed to &7%s&1.&0\r\n", GET_NAME(victim));
  send_to_char(buf2, victim);
}

ACMD(do_return)
{
  if (POSSESSED(ch)) {
    if (GET_LEVEL(POSSESSOR(ch)) < 101 && subcmd == 0) {
      send_to_char("Huh?!?\r\n", ch);
      return;
    }
    if (GET_COOLDOWN(ch, CD_SHAPECHANGE) < 0 && subcmd == 0) {
      send_to_char("Use 'shapechange me' to return to your body.\r\n", ch);
      return;
    }

    send_to_char("You return to your original body.\r\n", ch);

    /* if someone switched into your original body, disconnect them */
    if (ch->desc->original->desc)
      close_socket(ch->desc->original->desc);

    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
  else
    send_to_char("Huh?!?\r\n", ch);
}



ACMD(do_load)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: load { obj | mob } <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, ch->in_room);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
        0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    sprintf(buf, "(GC) %s loads mob,  %s", GET_NAME(ch), GET_NAME(mob));
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
    load_mtrigger(mob);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    /*obj_to_room(obj, ch->in_room);*/
    obj_to_char(obj, ch);
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", TRUE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    sprintf(buf, "(GC) %s loads OBJ,  %s", GET_NAME(ch), (obj)->short_description);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}


/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf) {                        /* argument supplied. destroy single object
                                 * or char */
    if ((vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict)) && (GET_LEVEL(ch) >= LVL_HEAD_B)) {
        send_to_char("Fuuuuuuuuu!\r\n", ch);
        return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);
      act("$n disintegrates YOU!", TRUE, ch, 0, vict, TO_VICT);
      if (!IS_NPC(vict)) {
        sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
        mudlog(buf, BRF, LVL_GOD, TRUE);
        if (vict->desc) {
          close_socket(vict->desc);
          vict->desc = NULL;
        }
        remove_player_from_game(vict, QUIT_PURGE);
      } else
        extract_char(vict);
    } else if ((obj = find_obj_in_list(world[ch->in_room].contents, find_vis_by_name(ch, buf)))) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("Nothing here by that name.\r\n", ch);
      return;
    }
    send_to_char(OK, ch);
  } else {                        /* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
        FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\r\n", ch->in_room);
    for (vict = world[ch->in_room].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict))
        extract_char(vict);
    }
    for (obj = world[ch->in_room].contents; obj; obj = next_o) {
      next_o = obj->next_content;
      if(GET_OBJ_WEAR(obj) != 0) { /* This prevents items w/o take flags from getting purged
                                      like in Fiery v1 RSD */
        extract_obj(obj);
      }
    }
  }
}

ACMD(do_advance)
{
  struct char_data *victim;
  char *name = arg, *level = buf2;
  int newlevel, oldlevel, i;

  two_arguments(argument, name, level);

  if (!*name) {
    cprintf(ch, "Advance who?\r\n");
    return;
  }
  if (!(victim = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
    send_to_char("That player is not here.\r\n", ch);
    return;
  }

  /*
   * I don't know which wise guy commented out this next check, but if
   * they ever do it again, I'll kill them.
   */
  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("Maybe that's not such a great idea.\r\n", ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("NO!  Not on NPC's.\r\n", ch);
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char("That's not a level!\r\n", ch);
    return;
  }
  if (newlevel > LVL_IMPL) {
    sprintf(buf, "%d is the highest possible level.\r\n", LVL_IMPL);
    send_to_char(buf, ch);
    return;
  }


  if (newlevel > GET_LEVEL(ch)) {
    send_to_char("Yeah, right.\r\n", ch);
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char("They are already at that level.\r\n", ch);
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    send_to_char("&0&9&bYou are momentarily enveloped by darkness!&0\r\n"
                 "&0&9&bYou feel somewhat diminished.&0\r\n", victim);
  } else {
    act("$n makes some strange gestures.\r\n"
        "A strange feeling comes upon you,\r\n"
        "Like a giant hand, light comes down\r\n"
        "from above, grabbing your body, that\r\n"
        "begins to pulse with colored lights\r\n"
        "from inside.\r\n\r\n"
        "Your head seems to be filled with demons\r\n"
        "from another plane as your body dissolves\r\n"
        "to the elements of time and space itself.\r\n"
        "Suddenly a silent explosion of light\r\n"
        "snaps you back to reality.\r\n\r\n"
        "You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  if (oldlevel >= LVL_IMMORT && newlevel < LVL_IMMORT) {
    /* If they are no longer an immortal, remove the immortal only flags. */
    REMOVE_FLAG(PRF_FLAGS(victim), PRF_LOG1);
    REMOVE_FLAG(PLR_FLAGS(victim), PRF_LOG2);
    REMOVE_FLAG(PRF_FLAGS(victim), PRF_NOHASSLE);
    REMOVE_FLAG(PLR_FLAGS(victim), PRF_HOLYLIGHT);
    REMOVE_FLAG(PLR_FLAGS(victim), PRF_SHOWVNUMS);
    REMOVE_FLAG(PLR_FLAGS(victim), PRF_ROOMVIS);
    REMOVE_FLAG(PLR_FLAGS(victim), PRF_ROOMVIS);
  }
  else if (oldlevel < LVL_IMMORT && newlevel >= LVL_IMMORT) {
    SET_FLAG(PRF_FLAGS(victim), PRF_HOLYLIGHT);
    SET_FLAG(PRF_FLAGS(victim), PRF_SHOWVNUMS);
    SET_FLAG(PRF_FLAGS(victim), PRF_AUTOEXIT);
    for (i = 1; i <= MAX_SKILLS; i++)
      SET_SKILL(victim, i, 100);
    /* Make sure they have an empty olc zone list */
    free_olc_zone_list(victim);
  }

  cprintf(ch, "%s", OK);

  if (newlevel < oldlevel)
    mprintf(L_STAT, MAX(LVL_IMMORT, GET_LEVEL(ch)),
            "(GC) %s has demoted %s from level %d to %d.",
            GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    mprintf(L_STAT, MAX(LVL_IMMORT, GET_LEVEL(ch)),
            "(GC) %s has advanced %s to level %d (from %d)",
            GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  gain_exp(victim,
           exp_next_level(newlevel - 1, GET_CLASS(victim)) - GET_EXP(victim) + 1,
           GAIN_IGNORE_ALL);
  save_player_char(victim);

  /*
   * Normally, advance_level calls init_mem_list when losing levels,
   * but since we start over from level 1 in do_advance, we need
   * to call it ourselves.
   */
  if (newlevel < oldlevel)
    init_mem_list(victim);
}


void perform_restore(struct char_data *vict)
{
   extern void dispel_harmful_magic(struct char_data *ch);

   dispel_harmful_magic(vict);
   GET_HIT(vict) = MAX(GET_MAX_HIT(vict), GET_HIT(vict));
   GET_MANA(vict) = MAX(GET_MAX_MANA(vict), GET_MANA(vict));
   GET_MOVE(vict) = MAX(GET_MAX_MOVE(vict), GET_MOVE(vict));
   cure_laryngitis(vict);

   /* Since we didn't call alter_hit, which calls hp_pos_check */
   hp_pos_check(vict, NULL, 0);
}


ACMD(do_restore)
{
   struct char_data *vict;
   int i;

   one_argument(argument, buf);
   if (!*buf)
      send_to_char("Whom do you wish to restore?\r\n", ch);
   else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
      send_to_char(NOPERSON, ch);
   else {
      send_to_char(OK, ch);
      act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR | TO_SLEEP);
      perform_restore(vict);
      if ((GET_LEVEL(ch) >= LVL_GRGOD) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
         for (i = 1; i <= TOP_SKILL; i++)
            SET_SKILL(vict, i, 1000);
         effect_total(vict);
      }
   }
}

void perform_pain(struct char_data *vict)
{
   int change;

   change = GET_HIT(vict) * 0.1;
   hurt_char(vict, NULL, change, TRUE);
   change = GET_MOVE(vict) * 0.1;
   alter_move(vict, change);
}


ACMD(do_pain)
{
   struct char_data *vict;

   one_argument(argument, buf);
   if (!(*buf))
      send_to_char("To whom do you wish to cause pain?\r\n",ch);
   else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, buf))))
      send_to_char(NOPERSON, ch);
   else {
      perform_pain(vict);
      send_to_char(OK, ch);
      act("A wave of pain and pesilence sent by $N harms you!", FALSE, vict, 0, ch, TO_CHAR | TO_SLEEP);
   }
}

void perform_immort_vis(struct char_data *ch)
{
  void appear(struct char_data *ch);

  if (GET_INVIS_LEV(ch) == 0 && !EFF_FLAGGED(ch, EFF_INVISIBLE) &&
      !IS_HIDDEN(ch) && !EFF_FLAGGED(ch, EFF_CAMOUFLAGED)) {
    send_to_char("You are already fully visible.\r\n", ch);
    return;
  }

  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char("You are now fully visible.\r\n", ch);
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  if (IS_NPC(ch))
    return;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
          tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
          tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  sprintf(buf, "Your invisibility level is %d.\r\n", level);
  send_to_char(buf, ch);
}


ACMD(do_invis)
{
  int level;

  if (IS_NPC(ch)) {
    send_to_char("You can't do that!\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_LEVEL(ch))
      send_to_char("You can't go invisible above your own level.\r\n", ch);
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);

  if (!*argument)
    send_to_char("That must be a mistake...\r\n", ch);
  else {
    sprintf(buf, "%s&0\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next)
      if (!pt->connected && pt->character && pt->character != ch)
        send_to_char(buf, pt->character);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      send_to_char(buf, ch);
  }
}

/* completely redone in order to add poofs to the pfile */
/* --Fingon                                             */
ACMD(do_poofset)
{
  skip_spaces(&argument);

  switch (subcmd)
  {
  case SCMD_POOFIN:
    if(argument[0] !='\0')
    {
      GET_POOFIN(ch) = strdup(argument);
      sprintf(buf, "Your poofin is now: %s\r\n", GET_POOFIN(ch));
    }
    else
      sprintf(buf, "Your poofin is: %s\r\n", GET_POOFIN(ch));
    break;
  case SCMD_POOFOUT:
    if(argument[0] !='\0')
    {
      GET_POOFOUT(ch) = strdup(argument);
      sprintf(buf, "Your poofin is now: %s\r\n", GET_POOFOUT(ch));
    }
    else
      sprintf(buf, "Your poofout is: %s\r\n", GET_POOFOUT(ch));
    break;
  default:
    return;
    break;
  }

  send_to_char(buf, ch);
}



ACMD(do_dc)
{
  struct descriptor_data *d = NULL;
  int num_to_dc;

  one_argument(argument, arg);
  if ((num_to_dc = atoi(arg))) {
    for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);
    if (!d) {
      send_to_char("No such connection.\r\n", ch);
      return;
    }
  }
  else if (*arg)
    for (d = descriptor_list; d; d = d->next) {
      if (d->character && GET_NAME(d->character) &&
          !strcasecmp(GET_NAME(d->character), arg))
        break;
      if (d->original && GET_NAME(d->original) &&
          !strcasecmp(GET_NAME(d->original), arg))
        break;
    }

  if (!d) {
    send_to_char("Usage: DC <connection number | character name> (type USERS for a list)\r\n", ch);
    return;
  }

  if ((d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) ||
      (d->original && GET_LEVEL(d->original) >= GET_LEVEL(ch))) {
    send_to_char("Umm.. maybe that's not such a good idea...\r\n", ch);
    return;
  }
  num_to_dc = d->desc_num;
  close_socket(d);
  sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
  send_to_char(buf, ch);
  sprintf(buf, "(GC) Connection closed by %s.", GET_NAME(ch));
  log(buf);
}



ACMD(do_wizlock)
{
  int value;
  char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char("Invalid wizlock value.\r\n", ch);
      return;
    }
    restrict = value;
    when = "now";

    if (restrict)
       restrict_reason = RESTRICT_MANUAL;
    else
       restrict_reason = RESTRICT_NONE;
  } else
    when = "currently";

  switch (restrict) {
  case 0:
    sprintf(buf, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    sprintf(buf, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    sprintf(buf, "Only level %d and above may enter the game %s.\r\n",
            restrict, when);
    break;
  }
  send_to_char(buf, ch);
  if (!*arg && restrict_reason == RESTRICT_AUTOBOOT)
      send_to_char("The restriction was set automatically by the automatic rebooting system.\r\n", ch);
}


ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;
  extern time_t *boot_time;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time[0];

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    sprintf(buf, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = time(0) - boot_time[0];
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
            ((d == 1) ? "" : "s"), h, m);
  }

  send_to_char(buf, ch);
}


ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char("Whom do you wish to force do what?\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg))))
      send_to_char(NOPERSON, ch);
    else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char("No, no, no!\r\n", ch);
    else {
      send_to_char(OK, ch);
      act(buf1, FALSE, ch, NULL, vict, TO_VICT);
      sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced room %d to %s", GET_NAME(ch),
          world[ch->in_room].vnum, to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (GET_LEVEL(vict) >= GET_LEVEL(ch))
        continue;
      act(buf1, FALSE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (i->connected || !(vict = i->character) || GET_LEVEL(vict) >= GET_LEVEL(ch))
        continue;
      act(buf1, FALSE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}



ACMD(do_wiznet)
{
  struct descriptor_data *d;
  char any = FALSE;
  bool explicit_level = FALSE;
  int level = LVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("Usage: wiznet <text> | #<level> <text>\r\n "
                 "       wiznet @<level> | wiz @\r\n", ch);
    return;
  }

  switch (*argument) {
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), 1);
      if (level > GET_LEVEL(ch)) {
        send_to_char("You can't wizline above your own level.\r\n", ch);
        return;
      }
      explicit_level = TRUE;
    }
    break;
  case '@':
    for (d = descriptor_list; d; d = d->next) {
      if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT &&
          !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
          (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL)) {
        if (!any) {
          sprintf(buf1, "Gods online:\r\n");
          any = TRUE;
        }
        sprintf(buf1, "%s  %s", buf1, GET_NAME(d->character));
        if (PLR_FLAGGED(d->character, PLR_WRITING))
          sprintf(buf1, "%s (Writing)\r\n", buf1);
        else if (EDITING(d))
          sprintf(buf1, "%s (Writing)\r\n", buf1);
        else if (PLR_FLAGGED(d->character, PLR_MAILING))
          sprintf(buf1, "%s (Writing mail)\r\n", buf1);
        else
          sprintf(buf1, "%s\r\n", buf1);

      }
    }
    any = FALSE;
    for (d = descriptor_list; d; d = d->next) {
      if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT &&
          PRF_FLAGGED(d->character, PRF_NOWIZ) &&
          CAN_SEE(ch, d->character)) {
        if (!any) {
          sprintf(buf1, "%sGods offline:\r\n", buf1);
          any = TRUE;
        }
        sprintf(buf1, "%s  %s\r\n", buf1, GET_NAME(d->character));
      }
    }
    send_to_char(buf1, ch);
    return;
    break;
  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }

  if (explicit_level) {
    sprintf(buf1, "&6%s: <%d> %s&0\r\n", GET_NAME(ch), level, argument);
    sprintf(buf2, "&6Someone: <%d> %s&0\r\n", level, argument);
  } else {
    sprintf(buf1, "&6%s: %s&0\r\n", GET_NAME(ch), argument);
    sprintf(buf2, "&6Someone: %s&0\r\n", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if (!IS_PLAYING(d))
      continue;
    if (STATE(d) != CON_PLAYING || PLR_FLAGGED(d->character, PLR_WRITING) || PLR_FLAGGED(d->character, PLR_MAILING) || EDITING(d))
      if (!PRF_FLAGGED(d->character, PRF_OLCCOMM))
        continue;
    if (d->original ? GET_LEVEL(d->original) < level : GET_LEVEL(d->character) < level)
      continue;
    if (PRF_FLAGGED(d->character, PRF_NOWIZ))
      continue;
    if (d == ch->desc && PRF_FLAGGED(d->character, PRF_NOREPEAT))
      continue;
    if (CAN_SEE(d->character, ch))
      send_to_char(buf1, d->character);
    else
      send_to_char(buf2, d->character);
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
}

/* added for name approval system by fingh 3/31/99 */
/* Complete name approval system redone by Zantir/David Endre 5/4/99 */
void broadcast_name(char *name)
{
  char temp[80];
  struct descriptor_data *d;

  *temp = '\0';

  sprintf(temp, "&2&b%s is awaiting name approval.&0\r\n", name);

  for (d = descriptor_list; d; d = d->next)
    if (IS_PLAYING(d) && d->character &&
        GET_LEVEL(d->character) >= LVL_IMMORT &&
        !PLR_FLAGGED(d->character, PLR_WRITING) &&
        !PLR_FLAGGED(d->character, PLR_MAILING) &&
        !EDITING(d))
     write_to_output(temp, d);
}

char *reasons[] = {
"Compound words. Examples: LadyJade, BraveBlade, ImmortalSoul\r\n These types of names give a player an unearned arrogance above\r\n other players and in some instances above the gods.",
"Offensive words or names, mispelled or backwards. Eg: Sgurd Kcuf Emod",
"Names from known mythos, movies etc... Examples: Feyd, Conan, Zeus\r\n Come on, you can be more creative than reliance on an existing known\r\n character.",
"Names that do not fit the character you are playing. Examples:\r\n A human named Gruzel\r\n A troll named Bob\r\n A boy named Sue\r\n An orc named Ripenthiel",
"No offensive words or names from other languages or cultures.",
"No Nouns verbs or adverbs. Eg. Jester, Red, Dog, Bloody, Hotly, Freedom\r\n Pervert, PC, Trouble, McIntosh, Desperado.  Sorry guys LAME.",
"No names that resemble common nouns verbs or adverbs. Eg. Jesterx,\r\n Redx, Chamelion, Desperato etc...",
"No modern names. Examples: Rina John Mike Steve",
"Other...Please talk to a god for further information."
};

ACMD(do_name)
{
  char buffer[MAX_STRING_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int choice = 0;
  int z = 0;

  *buffer = '\0';

  switch(subcmd)
  {
  case SCMD_ACCEPT:
    one_argument(argument, arg);
    if (!*arg)
    {
      send_to_char("Accept WHO?\r\n", ch);
      return;
    }
    /* cycle through the current connections */
    for (d = descriptor_list; d; d = d->next)
    {

      /* added to stop crashes while people are at CON_QANSI etc. */
      if(!d->character) continue;

      /* make sure player is either at the CON_NAME_WAIT_APPROVAL state
         or has been auto-accepted and still pending approval
         and the player is not yourself
      */
      if (((STATE(d) == CON_NAME_WAIT_APPROVAL) ||
           (PLR_FLAGGED(d->character, PLR_NAPPROVE))) &&
           ((d != ch->desc) &&
           (strn_cmp(GET_NAME(d->character), arg, strlen(arg)) == 0)))
      {
        /* create the player in the pfile if he isn't there already */
        if (GET_PFILEPOS(d->character) < 0) {
          GET_PFILEPOS(d->character) = create_player_index_entry(GET_NAME(d->character));
          /* initialize the the player */
          init_player(d->character);
          /* log the new player */
          sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
          mudlog(buf, NRM, LVL_IMMORT, TRUE);
        }
        /* accept the players name */
        REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NAPPROVE);
        save_player_char(d->character);

        /* log the acceptance of the name */
        sprintf(buf, "The name: %s has been accepted by %s.", GET_NAME(d->character), GET_NAME(ch));
        mudlog(buf, NRM, LVL_IMMORT, TRUE);

        /* remove the choose new name flag */
        if (PLR_FLAGGED(d->character, PLR_NEWNAME))
           REMOVE_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);

        /* check if the player is waiting approval or already auto approved */
        if(STATE(d) == CON_NAME_WAIT_APPROVAL) {
          /* send the motd and change connection states */
          write_to_output(get_text(TEXT_MOTD), d);
          write_to_output("\r\n\n*** PRESS RETURN: ", d);
          STATE(d) = CON_RMOTD;
        }

        /* tell the player the name has been accepted */
        sprintf(buf, "&2&bThe name: %s has been accepted.&0\r\n", GET_NAME(d->character));
        send_to_char(buf, ch);
        send_to_char(buf, d->character);
        return;
      }
    }
    send_to_char("No such player waiting for approval.\r\n", ch);
    break;
  case SCMD_DECLINE:
    two_arguments(argument, arg, arg2);
    if (!*arg)
    {
      send_to_char("Decline WHO?\r\n", ch);
      return;
    }
    if (!*arg2)
    {
      send_to_char("For what reason? Type 'nlist 0' for a list.\r\n", ch);
      return;
    }

    choice = atoi(arg2);
    choice--;
    if (choice < 0 || choice > 7)
      choice = 8;
    /* cycle through connected players */
    for (d = descriptor_list; d; d = d->next)
    {
      /* added to stop crashes while people are at CON_QANSI etc. */
      if(!d->character) continue;

      if (((STATE(d) == CON_NAME_WAIT_APPROVAL) ||
           (PLR_FLAGGED(d->character, PLR_NAPPROVE))) &&
         (d != ch->desc) &&
         (strn_cmp(GET_NAME(d->character), arg, strlen(arg)) == 0))
      {
         sprintf(buf, "The name: %s has been declined by %s, reason %d.", GET_NAME(d->character), GET_NAME(ch), choice+1);
         mudlog(buf, NRM, LVL_IMMORT, TRUE);
         sprintf(buf, "&2&bThe name: %s has been declined.&0\r\n", GET_NAME(d->character));
         send_to_char(buf, ch);
         send_to_xnames(GET_NAME(d->character));
         write_to_output("That name has been rejected, because it breaks this rule:\r\n", d);
         *buf = '\0';
         sprintf(buf, "%s\r\n", reasons[choice]);
         write_to_output(buf,d);
         SET_FLAG(PLR_FLAGS(d->character), PLR_NEWNAME);
         if(STATE(d) == CON_NAME_WAIT_APPROVAL) {
           write_to_output("Please try another name.\r\n", d);
           write_to_output("Name: ", d);
           STATE(d) = CON_NEW_NAME;
         } else {
           write_to_output("&6&bYou are welcome to play; however, you will be prompted&0\r\n&6&bfor a new name on your next login.&0\r\n", d);
           save_player_char(d->character);
         }
         return;
      }
    }
    send_to_char("No such player waiting for approval.\r\n", ch);
    break;
  case SCMD_LIST:
    one_argument(argument, arg);

    if (*arg)
    {
      if (atoi(arg) == 0) {
         *buf = '\0';
         for(z=0;z<8;z++)
            sprintf(buf, "%s%d. %s\r\n", buf, z+1, reasons[z]);
         send_to_char(buf, ch);
      }
    }

    for (d = descriptor_list; d; d = d->next)
    {
      /* added this NULL check to stop crashes when people where CON_QANSI etc */
      if(d->character) {
        if (((STATE(d) == CON_NAME_WAIT_APPROVAL) ||
             (PLR_FLAGGED(d->character, PLR_NAPPROVE) && !(PLR_FLAGGED(d->character, PLR_NEWNAME)))) &&
            (d != ch->desc)) {
          sprintf(buffer, "%s%s\r\n", buffer, GET_NAME(d->character));
        }
      }
    }

    if ( strlen(buffer) < 2)
    {
      sprintf(buffer, "No one.\r\n");
      send_to_char(buffer, ch);
    }
    else
    {
      send_to_char("The following people are awaiting name approval:\r\n", ch);
      send_to_char(buffer, ch);
    }
    break;
  default:
    break; /* maybe send an error to log here? */
  } /* end switch */
}

ACMD(do_zreset)
{
  void reset_zone(int zone, byte pop);

  int i, j;

  one_argument(argument, arg);

  if (!*arg || *arg == '.') {
    i = world[ch->in_room].zone;
  } else if (*arg == '*') {
      if (GET_LEVEL(ch) <= LVL_HEAD_B)
          return;
          for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i, FALSE);
    send_to_char("Reset world.\r\n", ch);
    sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
    return;
  } else if (!isdigit(*arg)) {
    send_to_char("Usage: zreset [<zone-number>]\r\n", ch);
    return;
  } else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
        break;
  }
  if (GET_LEVEL(ch) < LVL_GRGOD && !has_olc_access(ch, zone_table[i].number)) {
    send_to_char("Testing testing !!are you clear to do this!! no I dont think so\r\n", ch);
    return;
  }
  if (i >= 0 && i <= top_of_zone_table) {
    reset_zone(i, FALSE);
    sprintf(buf, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number,
            zone_table[i].name);
    send_to_char(buf, ch);
    sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), zone_table[i].number, zone_table[i].name);
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
  } else
    send_to_char("Invalid zone number.\r\n", ch);
}


static char *center_wiztitle(char *wiztitle, int len, int noansi_len) {
  char centerbuf[MAX_INPUT_LENGTH];
  int i;
  bool add_zero;

  /* If the wiztitle doesn't end in &0, put it on. */
  add_zero = (len < 2 || wiztitle[len - 1] != '0' ||
              (wiztitle[len - 2] != CREL && wiztitle[len - 2] != CABS));

  sprintf(centerbuf + ((WIZ_TITLE_WIDTH - noansi_len) / 2), "%s%s", wiztitle, add_zero ? "&0" : "");

  /* +2 for the &0 we just tacked on */
  if (add_zero)
    len += 2;

  for (i = 0; i < (WIZ_TITLE_WIDTH - noansi_len) / 2; ++i) {
    centerbuf[i] = ' ';
    centerbuf[WIZ_TITLE_WIDTH - 1 + (len - noansi_len) - i] = ' ';
  }

  /* if odd-lengthed string, need an extra space to cover up \0 */
  if (noansi_len & 1)
    centerbuf[WIZ_TITLE_WIDTH - 1 + (len - noansi_len) - i] = ' ';
  centerbuf[WIZ_TITLE_WIDTH + (len - noansi_len)] = '\0';

  return strdup(centerbuf);
}

void do_wiztitle(char *outbuf, struct char_data *vict, char *argument)
{
  int noansi_len, len;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  /* check size of title w/out ansi chars */
  len = strlen(argument);
  noansi_len = len - count_color_chars(argument);

  if (IS_NPC(vict))
    sprintf(buf, "Mobile's do not have godly titles...nice try.\r\n");
  else if (strstr(argument, "[") || strstr(argument, "]"))
    sprintf(buf, "Godly Titles can't contain the [ or ] characters.\r\n");
  else if (strlen(argument) >= MAX_INPUT_LENGTH)
    sprintf(buf, "Sorry, godly titles can't be longer than %d characters.\r\n",
            MAX_INPUT_LENGTH - 1);
  else if (noansi_len > WIZ_TITLE_WIDTH)
    sprintf(buf, "Sorry, text portion of godly titles can't be longer than 12 characters.\r\n");
  else if (noansi_len == 0) {
    GET_WIZ_TITLE(vict) = NULL;
    sprintf(buf, "Okay, %s's godly title reset to default.\r\n", GET_NAME(vict));
  }
  else {
    if (GET_WIZ_TITLE(vict))
      free(GET_WIZ_TITLE(vict));
    GET_WIZ_TITLE(vict) = center_wiztitle(argument, len, noansi_len);
    sprintf(buf, "Okay, %s's godly title is now [%s]\r\n", GET_NAME(vict), GET_WIZ_TITLE(vict));
  }
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  struct char_data *vict;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Yes, but for whom?!?\r\n", ch);
  else if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, arg))))
    send_to_char("There is no such player.\r\n", ch);
  else if (IS_NPC(vict))
    send_to_char("You can't do that to a mob!\r\n", ch);
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    send_to_char("Hmmm...you'd better not.\r\n", ch);
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char("Rerolled not functinal for now\r\n", ch);
break;
      /*sprintf(buf, "(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      log(buf);
      sprintf(buf, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
              GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
              GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      send_to_char(buf, ch);
      break;*/
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER)) {
        send_to_char("Your victim is not flagged.\r\n", ch);
        return;
      }
      REMOVE_FLAG(PLR_FLAGS(vict), PLR_THIEF);
      REMOVE_FLAG(PLR_FLAGS(vict), PLR_KILLER);
      send_to_char("Pardoned.\r\n", ch);
      send_to_char("You have been pardoned by the Gods!\r\n", vict);
      sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
              GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result),
              GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
        send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
        return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
        send_to_char("Your victim is already pretty cold.\r\n", ch);
        return;
      }
      SET_FLAG(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
      send_to_char("Frozen.\r\n", ch);
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
        send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
        return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
        sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
           GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
        send_to_char(buf, ch);
        return;
      }
      sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      REMOVE_FLAG(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
      send_to_char("Thawed.\r\n", ch);
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->effects) {
        while (vict->effects)
          effect_remove(vict, vict->effects);
        send_to_char("There is a brief flash of light!\r\n"
                     "You feel slightly different.\r\n", vict);
        send_to_char("All spells removed.\r\n", ch);
        check_regen_rates(vict);
        /* if it had been an animated and now isnt then it needs to die */
        if (MOB_FLAGGED(vict, MOB_ANIMATED) &&
            !EFF_FLAGGED(vict, EFF_ANIMATED)) {
          act("$n freezes and falls twitching to the ground.", FALSE, vict, 0,
              0, TO_ROOM);
          die(vict, NULL);
          return;
        }
      } else {
        send_to_char("Your victim does not have any effects!\r\n", ch);
        return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd passed to do_wizutil (act.wizard.c)");
      break;
    }
    save_player_char(vict);
  }
}


#define PC   1
#define NPC  2
#define BOTH 3

#define MISC        0
#define BINARY        1
#define NUMBER        2

#define SET_OR_REMOVE(flagset, flags) do { \
        if (on) SET_FLAG(flagset, flags); \
        else if (off) REMOVE_FLAG(flagset, flags); } while (0)

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

ACMD(do_set)
{
  int i, l;
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH];
  int on = 0, off = 0, value = 0;
  char is_file = 0, is_mob = 0, is_player = 0, save = TRUE;
  int player_i = 0;
  int get_race_align(struct char_data *ch);
  int parse_diety(struct char_data *ch, char arg);

  struct set_struct {
    char *cmd;
    char level;
    char pcnpc;
    char type;
  } fields[] = {
   { "brief",        LVL_ATTENDANT,   PC,   BINARY }, /* 0*/
   { "",             LVL_GAMEMASTER,  PC,   BINARY }, /* 1*/  /* was invstart, now UNUSED */
   { "title",        LVL_GAMEMASTER,  PC,   MISC   },
   { "nosummon",     LVL_GAMEMASTER,  PC,   BINARY },
   { "maxhit",       LVL_GOD,         BOTH, NUMBER },
   { "maxmana",      LVL_GOD,         BOTH, NUMBER }, /* 5*/
   { "maxmove",      LVL_GOD,         BOTH, NUMBER },
   { "hitpoints",    LVL_GOD,         BOTH, NUMBER },
   { "manapoints",   LVL_GOD,         BOTH, NUMBER },
   { "movepoints",   LVL_GOD,         BOTH, NUMBER },
   { "align",        LVL_GOD,         BOTH, NUMBER }, /*10*/
   { "strength",     LVL_GOD,         BOTH, NUMBER },
   { "stradd",       LVL_GOD,         BOTH, NUMBER },
   { "intelligence", LVL_GOD,         BOTH, NUMBER },
   { "wisdom",       LVL_GOD,         BOTH, NUMBER },
   { "dexterity",    LVL_GOD,         BOTH, NUMBER }, /*15*/
   { "constitution", LVL_GOD,         BOTH, NUMBER },
   { "sex",          LVL_GOD,         BOTH, MISC   },
   { "ac",           LVL_GOD,         BOTH, NUMBER },
   { "olc",          LVL_ADMIN,       PC,   NUMBER },
   { "home",         LVL_GOD,         PC,   NUMBER }, /*20*/  /* THIS ONE CAN BE REPLACED - see homeroom below */
   { "experience",   LVL_ADMIN,       BOTH, NUMBER },
   { "hitroll",      LVL_GOD,         BOTH, NUMBER },
   { "damroll",      LVL_GOD,         BOTH, NUMBER },
   { "invis",        LVL_GAMEMASTER,  PC,   NUMBER },
   { "nohassle",     LVL_GAMEMASTER,  PC,   BINARY }, /*25*/
   { "frozen",       LVL_FREEZE,      PC,   BINARY },
   { "practices",    LVL_GAMEMASTER,  PC,   NUMBER },
   { "lessons",      LVL_GAMEMASTER,  PC,   NUMBER },
   { "drunk",        LVL_GOD,         BOTH, MISC   },
   { "hunger",       LVL_GOD,         BOTH, MISC   }, /*30*/
   { "thirst",       LVL_GOD,         BOTH, MISC   },
   { "killer",       LVL_ADMIN,       PC,   BINARY },
   { "thief",        LVL_ADMIN,       PC,   BINARY },
   { "level",        LVL_ADMIN,       BOTH, NUMBER },
   { "room",         LVL_ATTENDANT,   BOTH, NUMBER }, /*35*/
   { "roomflag",     LVL_ADMIN,       PC,   BINARY },
   { "siteok",       LVL_GAMEMASTER,  PC,   BINARY },
   { "deleted",      LVL_IMPL,        PC,   BINARY },
   { "class",        LVL_ATTENDANT,   BOTH, MISC   },
   { "nowizlist",    LVL_ATTENDANT,   PC,   BINARY }, /*40*/
   { "quest",        LVL_GOD,         PC,   BINARY },
   { "homeroom",     LVL_GOD,         PC,   MISC   },
   { "color",        LVL_GOD,         PC,   BINARY },
   { "idnum",        LVL_IMPL,        PC,   NUMBER },
   { "passwd",       LVL_GAMEMASTER,  PC,   MISC   }, /*45*/
   { "nodelete",     LVL_ATTENDANT,   PC,   BINARY },
   { "charisma",     LVL_GOD,         BOTH, NUMBER },
   { "race",         LVL_GOD,         PC,   MISC   },
   { "olc2",         LVL_ADMIN,       PC,   NUMBER },
   { "olc3",         LVL_ADMIN,       PC,   NUMBER }, /*50*/
   { "olc4",         LVL_ADMIN,       PC,   NUMBER },
   { "olc5",         LVL_ADMIN,       PC,   NUMBER },
   { "platinum",     LVL_GOD,         BOTH, NUMBER },
   { "gold",         LVL_GOD,         BOTH, NUMBER },
   { "silver",       LVL_GOD,         BOTH, NUMBER }, /*55*/
   { "copper",       LVL_GOD,         BOTH, NUMBER },
   { "pbank",        LVL_ATTENDANT,   PC,   NUMBER },
   { "gbank",        LVL_ATTENDANT,   PC,   NUMBER },
   { "sbank",        LVL_ATTENDANT,   PC,   NUMBER },
   { "cbank",        LVL_ATTENDANT,   PC,   NUMBER }, /*60*/
   { "UNUSED",       LVL_IMPL,        PC,   NUMBER },
   { "anon",         LVL_ATTENDANT,   PC,   BINARY },
   { "rename",       LVL_GOD,         PC,   BINARY },
   { "napprove",     LVL_GOD,         PC,   BINARY },
   { "holylight",    LVL_ATTENDANT,   PC,   BINARY }, /*65*/
   { "wiztitle",     LVL_GOD,         PC,   MISC   },
   { "chant",        LVL_GAMEMASTER,  PC,   NUMBER },
   { "size",         LVL_GOD,         PC,   MISC   },
   { "hiddenness",   LVL_GOD,         BOTH, NUMBER },
   { "rage",         LVL_GOD,         BOTH, NUMBER }, /*70*/
   { "ptitle",       LVL_GAMEMASTER,  PC,   MISC   },
   { "height",       LVL_GOD,         BOTH, NUMBER },
   { "weight",       LVL_GOD,         BOTH, NUMBER },
   { "lifeforce",    LVL_GOD,         BOTH, MISC   },
   { "composition",  LVL_GOD,         BOTH, MISC   }, /*75*/
   { "\n",           0,                      BOTH, MISC   }
  };

  half_chop(argument, name, buf);
  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob")) {
    is_mob = 1;
    half_chop(buf, name, buf);
  }
  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field) {
    pprintf(ch, "Usage: set <victim> <field> <value>\r\n");
    pprintf(ch, "Set fields currently available to you:\r\n");
    for (l = 0; *(fields[l].cmd) != '\n'; l++) {
      if (fields[l].level <= GET_LEVEL(ch) && *(fields[l].cmd)) {
        pprintf(ch, "%-20.20s %-20.20s %-6.6s\r\n", fields[l].cmd,
          (fields[l].pcnpc==PC?"Player Only":"Player or Mob"),
          (fields[l].type==BINARY?"Binary":(fields[l].type==NUMBER?"Number":
          "Misc")));
      }
    }
    start_paging(ch);
    return;
  }
  if (!is_file) {
    if (is_player) {
      if (!(vict = find_char_around_char(ch, find_vis_plr_by_name(ch, name)))) {
        send_to_char("There is no such player.\r\n", ch);
        return;
      }
    } else {
      if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
        send_to_char("There is no such creature.\r\n", ch);
        return;
      }
    }
  } else if (is_file) {
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    if ((player_i = load_player(name, cbuf)) > -1) {
      if (GET_LEVEL(cbuf) >= GET_LEVEL(ch)) {
        free_char(cbuf);
        send_to_char("Sorry, you can't do that.\r\n", ch);
        return;
      }
      vict = cbuf;
    } else {
      free_char(cbuf);
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
  }
  if (GET_LEVEL(ch) != LVL_HEAD_C) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char("Maybe that's not such a great idea...\r\n", ch);
      return;
    }
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (IS_NPC(vict) && !(fields[l].pcnpc & NPC)) {
    send_to_char("You can't do that to a beast!\r\n", ch);
    return;
  } else if (!IS_NPC(vict) && !(fields[l].pcnpc & PC)) {
    send_to_char("That can only be done to a beast!\r\n", ch);
    return;
  }
  if (fields[l].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char("Value must be on or off.\r\n", ch);
      return;
    }
  } else if (fields[l].type == NUMBER) {
    value = atoi(val_arg);
  }

  strcpy(buf, OK);
  switch (l) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    send_to_char("This option is unused.  You should not have seen this!\r\n", ch);
    return;
  case 2:
    set_title(vict, val_arg);
    sprintf(buf, "%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    on = !on;                        /* so output will be correct */
    break;
  case 4:
    /* Base hit is in a player special. Don't set for npcs */
    if (IS_NPC(vict))
      GET_MAX_HIT(vict) = RANGE(1, 500000);
    else {
      GET_BASE_HIT(vict) = RANGE(1, 500000);
      effect_total(vict);
    }
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 500000);
    effect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 500000);
    effect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    effect_total(vict);
    break;
  case 8:
    vict->points.mana = RANGE(0, vict->points.max_mana);
    effect_total(vict);
    break;
  case 9:
    vict->points.move = RANGE(0, vict->points.max_move);
    effect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    effect_total(vict);
    break;
  case 11:
    GET_NATURAL_STR(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
    effect_total(vict);
    break;
  case 12:
    /*  This is no longer used --Gurlaek 6/23/1999
    vict->view_abils.str_add = RANGE(0, 100);
    vict->view_abils.str_add = value;
    if (value > 0)
      vict->view_abils.str = 100;
    effect_total(vict);
    */
    break;
    /*Edited by Proky,  values for intel*/
  case 13:
    GET_NATURAL_INT(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
    effect_total(vict);
    break;
  case 14:
    GET_NATURAL_WIS(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
    effect_total(vict);
    break;
  case 15:
    GET_NATURAL_DEX(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
    effect_total(vict);
    break;
  case 16:
    GET_NATURAL_CON(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
    effect_total(vict);
    break;
  case 17:
    if (!str_cmp(val_arg, "male"))
      vict->player.sex = SEX_MALE;
    else if (!str_cmp(val_arg, "female"))
      vict->player.sex = SEX_FEMALE;
    else if (!str_cmp(val_arg, "neutral"))
      vict->player.sex = SEX_NEUTRAL;
    else {
      send_to_char("Must be 'male', 'female', or 'neutral'.\r\n", ch);
      return;
    }
    break;
  case 18:
    GET_AC(vict) = RANGE(MIN_AC, MIN_AC);
    effect_total(vict);
    break;
  case 19:
    {
      struct olc_zone_list dummy, *zone, *next;
      dummy.next = GET_OLC_ZONES(vict);
      for (zone = &dummy, i = FALSE; zone->next; zone = next) {
        next = zone->next;
        /* If in list, remove it */
        if (next->zone == value) {
          zone->next = next->next;
          free(next);
          i = TRUE;
          sprintf(buf, "Removed %d from %s allowed OLC zones.\r\n", value, GET_NAME(vict));
          break;
        }
      }
      GET_OLC_ZONES(vict) = dummy.next;
      /* If i is false, we didn't find it, so add it */
      if (!i) {
        CREATE(zone, struct olc_zone_list, 1);
        zone->zone = value;
        zone->next = GET_OLC_ZONES(vict);
        GET_OLC_ZONES(vict) = zone;
        sprintf(buf, "Added %d to %s allowed OLC zones.\r\n", value, GET_NAME(vict));
      }
    }
    break;
  case 20:
    GET_HOMEROOM(vict) = value;
    break;
  case 21:
    vict->points.exp = RANGE(0, 299999999);
    break;
  case 22:
    GET_BASE_HITROLL(vict) = RANGE(MIN_DAMROLL, MAX_DAMROLL);
    effect_total(vict);
    break;
  case 23:
    GET_BASE_DAMROLL(vict) = RANGE(MIN_DAMROLL, MAX_DAMROLL);
    effect_total(vict);
    break;
  case 24:
    if (GET_LEVEL(ch) < LVL_HEAD_C && ch != vict) {
      strcpy(buf, "You aren't godly enough for that!\r\n");
      save = FALSE;
      break;
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25:
    if (GET_LEVEL(ch) < LVL_HEAD_C && ch != vict) {
      strcpy(buf, "You aren't godly enough for that!\r\n");
      save = FALSE;
      break;
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict) {
      send_to_char("Better not -- could be a long winter!\r\n", ch);
      /* OK: ch == vict so no memory was allocated */
      return;
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
  case 28:
    /* GET_PRACTICES(vict) = RANGE(0, 100); */
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off")) {
      GET_COND(vict, (l - 29)) = (char) -1;
      sprintf(buf, "%s's %s now off.\r\n", GET_NAME(vict), fields[l].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (l - 29)) = (char) value;
      sprintf(buf, "%s's %s set to %d.\r\n", GET_NAME(vict), fields[l].cmd,
              value);
    } else {
      strcpy(buf, "Must be 'off' or a value from 0 to 24.\r\n");
      save = FALSE;
      break;
    }
    check_regen_rates(vict);
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    if (value > GET_LEVEL(ch) || value > LVL_OVERLORD) {
      strcpy(buf, "You can't do that.\r\n");
      save = FALSE;
      break;
    }
    if (GET_CLAN(vict) && IS_CLAN_MEMBER(vict))
      GET_CLAN(vict)->power -= GET_LEVEL(vict);
    RANGE(0, LVL_IMPL);
    vict->player.level = (byte) value;
    if (GET_CLAN(vict) && IS_CLAN_MEMBER(vict))
      GET_CLAN(vict)->power += GET_LEVEL(vict);
    break;
  case 35:
    if ((i = real_room(value)) < 0) {
      strcpy(buf, "No room exists with that number.\r\n");
      save = FALSE;
      break;
    }
    if (!is_file) {
      if (IN_ROOM(vict) != NOWHERE)
        char_from_room(vict);
      char_to_room(vict, i);
    }
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = parse_class(ch, vict, val_arg)) == CLASS_UNDEFINED) {
      save = FALSE;
      buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
      break;
    }
    if (i == GET_CLASS(vict)) {
       sprintf(buf, "%s already %s.",
           vict == ch ? "You're" : "$e's",
           with_indefinite_article(CLASS_FULL(vict)));
    } else {
      convert_class(vict, i);
      if (ch != vict) {
        sprintf(buf, "$n has changed your class to %s!", CLASS_FULL(vict));
        act(buf, FALSE, ch, 0, vict, TO_VICT);
      }
      sprintf(buf, "Okay, %s now %s.",
          vict == ch ? "you are" : "$n is",
          with_indefinite_article(CLASS_FULL(vict)));
    }
    act(buf, FALSE, vict, 0, ch, TO_VICT);
    buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (is_number(val_arg)) {
      value = atoi(val_arg);
      if (real_room(value) != NOWHERE) {
        GET_HOMEROOM(vict) = value;
        sprintf(buf, "%s's home room is now #%d.\r\n", GET_NAME(vict),
                GET_HOMEROOM(vict));
      } else {
        strcpy(buf, "That room does not exist!\r\n");
        save = FALSE;
      }
    } else {
      strcpy(buf, "Must be a room's virtual number.\r\n");
      save = FALSE;
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_COLOR_1);
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_COLOR_2);
    break;
  case 44:
    if (GET_IDNUM(ch) != 1 || IS_NPC(vict))
      return;
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (!is_file) {
      send_to_char("Not while they're logged in!\r\n", ch);
      return;
    }
    if (GET_IDNUM(ch) > 2 && !strcasecmp(GET_NAME(ch),"zuur")) {
      strcpy(buf, "Please don't use this command, yet.\r\n");
      save = FALSE;
    }
    else if (GET_LEVEL(vict) > LVL_HEAD_C) {
      strcpy(buf, "You cannot change that.\r\n");
      save = FALSE;
    }
    else {
      strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
      GET_PASSWD(vict)[MAX_PWD_LENGTH] = '\0';
      sprintf(buf, "Password changed to '%s'.\r\n", val_arg);
    }
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    GET_NATURAL_CHA(vict) = RANGE(MIN_ABILITY_VALUE, MAX_ABILITY_VALUE);
    effect_total(vict);
    break;
  case 48:
    if ((i = parse_race(ch, vict, val_arg)) == RACE_UNDEFINED) {
      save = FALSE;
      buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
      break;
    }
    if (i == GET_RACE(vict)) {
       sprintf(buf, "%s already %s.",
           vict == ch ? "You're" : "$e's",
           with_indefinite_article(RACE_FULL(vict)));
    } else {
      convert_race(vict, i);
      if (ch != vict) {
        sprintf(buf, "$n has changed your race to %s!", RACE_FULL(vict));
        act(buf, FALSE, ch, 0, vict, TO_VICT);
      }
      sprintf(buf, "Okay, %s now %s.",
          vict == ch ? "you are" : "$n is",
          with_indefinite_article(RACE_FULL(vict)));
    }
    act(buf, FALSE, vict, 0, ch, TO_VICT);
    buf[0] = '\0'; /* Don't say "Okay", we just gave feedback. */
    break;
  case 49:
    /* GET_OLC2_ZONE(vict) = value; */
    break;
  case 50:
    /* GET_OLC3_ZONE(vict) = value; */
    break;
  case 51:
    /* GET_OLC4_ZONE(vict) = value; */
    break;
  case 52:
    /* GET_OLC5_ZONE(vict) = value; */
    break;
  case 53:
    GET_PLATINUM(vict) = RANGE(0, 100000000);
    break;
  case 54:
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 55:
    GET_SILVER(vict) = RANGE(0, 100000000);
    break;
  case 56:
    GET_COPPER(vict) = RANGE(0, 100000000);
    break;
  case 57:
    GET_BANK_PLATINUM(vict) = RANGE(0, 100000000);
    break;
  case 58:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 59:
    GET_BANK_SILVER(vict) = RANGE(0, 100000000);
    break;
  case 60:
    GET_BANK_COPPER(vict) = RANGE(0, 100000000);
    break;

  case 61:
    break;
  case 62:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ANON);
    break;
  case 63:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NEWNAME);
    break;
  case 64:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NAPPROVE);
    break;
  case 65:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_HOLYLIGHT);
    break;
  case 66:
    do_wiztitle(buf, vict, val_arg);
    break;
  case 67:
    GET_COOLDOWN(vict, CD_CHANT) = RANGE(0, 100);
    break;
  case 68:
    if (!*val_arg) {
      show_sizes(ch);
      save = FALSE;
      buf[0] = '\0';
      break;
    } else if ((value = parse_size(ch, val_arg)) == SIZE_UNDEFINED) {
      save = FALSE;
      buf[0] = '\0';
      break;
    }
    change_natural_size(vict, value);
    sprintf(buf, "%s %s now %s in size.\r\n",
            ch == vict ? "You" : GET_NAME(vict),
            ch == vict ? "are" : "is",
            SIZE_DESC(vict));
    break;
  case 69:
    GET_HIDDENNESS(vict) = RANGE(0, 1000);
    break;
  case 70:
    GET_RAGE(vict) = RANGE(0, 1000);
    check_regen_rates(vict);
    break;
  case 71:
    argument = any_one_arg(val_arg, field);
    skip_spaces(&argument);
    i = atoi(field);
    value = 1;
    if (GET_PERM_TITLES(ch))
      for (; GET_PERM_TITLES(ch)[value - 1]; ++value);
    if (i < 1 || i > value)
      sprintf(buf, "You can only set ptitles from 1 to %d.\r\n", value);
    else {
      if (strlen(argument) > MAX_TITLE_WIDTH)
        argument[MAX_TITLE_WIDTH] = '\0';
      if (i == value)
        add_perm_title(vict, argument);
      else {
        if (GET_PERM_TITLES(vict)[i - 1])
          free(GET_PERM_TITLES(vict)[i - 1]);
        GET_PERM_TITLES(vict)[i - 1] = strdup(argument);
      }
      sprintf(buf, "%s's p-title %d is now: %s\r\n", GET_NAME(vict), i,
              GET_PERM_TITLES(vict)[i - 1]);
    }
    break;
  case 72:
    GET_HEIGHT(vict) = value;
    break;
  case 73:
    GET_WEIGHT(vict) = value;
    break;
  case 74: /* LIFE FORCE */
    if ((i = parse_lifeforce(ch, val_arg)) == LIFE_UNDEFINED) {
      save = FALSE;
      buf[0] = '\0';
      break;
    }
    if (i == GET_LIFEFORCE(vict)) {
       sprintf(buf, "%s already infused with %s.",
           vict == ch ? "You're" : "$e's", LIFEFORCE_NAME(vict));
    } else {
      if (ch != vict) {
        sprintf(buf, "$n has changed your life force to %s!", lifeforces[i].name);
        act(buf, FALSE, ch, 0, vict, TO_VICT);
      }
      sprintf(buf, "Okay, %s's life force is now %s.",
          vict == ch ? "you are" : "$n is",
          lifeforces[i].name);
      convert_lifeforce(vict, i);
    }
    act(buf, FALSE, vict, 0, ch, TO_VICT);
    buf[0] = '\0';
    break;
  case 75: /* COMPOSITION */
    if ((i = parse_composition(ch, val_arg)) == COMP_UNDEFINED) {
      save = FALSE;
      buf[0] = '\0';
      break;
    }
    if (i == BASE_COMPOSITION(vict)) {
       sprintf(buf, "%s already composed of %s.",
           vict == ch ? "You're" : "$e's", compositions[i].name);
    } else {
      if (ch != vict) {
        sprintf(buf, "$n has changed your composition to %s!", compositions[i].name);
        act(buf, FALSE, ch, 0, vict, TO_VICT);
      }
      sprintf(buf, "Okay, %s now composed of %s.",
          vict == ch ? "you are" : "$n is",
          compositions[i].name);
      set_base_composition(vict, i);
    }
    act(buf, FALSE, vict, 0, ch, TO_VICT);
    buf[0] = '\0';
    break;
  default:
    sprintf(buf, "Can't set that!\r\n");
    break;
  }

  if (fields[l].type == BINARY) {
    sprintf(buf, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on),
            GET_NAME(vict));
    CAP(buf);
  } else if (fields[l].type == NUMBER) {
    sprintf(buf, "%s's %s set to %d.\r\n", GET_NAME(vict),
            fields[l].cmd, value);
  }
  if (buf[0])
    send_to_char(CAP(buf), ch);

  if (!is_file && !IS_NPC(vict) && save)
    save_player_char(vict);

  if (is_file) {
    if (save) {
      GET_PFILEPOS(cbuf) = player_i;
      save_player_char(cbuf);
      send_to_char("Saved in file.\r\n", ch);
    }
    free_char(cbuf);
  }
}


ACMD(do_syslog)
{
  int severity;

  one_argument(argument, arg);

  if (!*arg) {
    cprintf(ch, "The minimum severity of syslog messages you see is %s.\r\n",
            sprint_log_severity(GET_LOG_VIEW(ch)));
    return;
  }

  if ((severity = parse_log_severity(arg)) < 0) {
    cprintf(ch, "Usage: syslog [ ");
    for (severity = 0; *log_severities[severity] != '\n'; ++severity)
      cprintf(ch, "%s%s", severity ? " | " : "", log_severities[severity]);
    cprintf(ch, " ]\r\n");
    return;
  }

  GET_LOG_VIEW(ch) = severity;

  cprintf(ch, "The minimum severity of syslog messages you will see is now %s.\r\n",
          sprint_log_severity(severity));
}


 void send_to_imms(char *msg)
 {
   struct descriptor_data *pt;

   for (pt = descriptor_list; pt; pt = pt->next)
     if (!pt->connected && pt->character && GET_LEVEL(pt->character) >= LVL_GOD)
         send_to_char(msg, pt->character);

 }


/* do_game recoded by 321 to allow different toggles to have different
 * minimum levels */
ACMD(do_game)
{
  char field[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  char shortbuf[40], linebuf[160];
  int i, n_visible = 0, value;
  char *msg;

  struct {
    char *name;
    int has_value;  /* Is it on/off (0) or does it have a value (1) */
    int *config;
    int min_level;
    char *turn_on;
    char *turn_off;
    char *enabled;
    char *disabled;
  } commands[] = {
    { "RACES", 0,     &races_allowed,         LVL_ADMIN,
      "[&2&bSYS: %s allows race logins&0]\r\n",
      "[&1&bSYS: %s disallows race logins&0]\r\n",
      "Race login allowed",
      "Race login not allowed" },
    { "EVILRACES",0, &evil_races_allowed,    LVL_ADMIN,
      "[&2&bSYS: %s allows evil race player creation&0]\r\n",
      "[&1&bSYS: %s disallows evil race player creation&0]\r\n",
      "Evil race player creation allowed",
      "Evil race player creation not allowed" },
    { "PK",     0,    &pk_allowed,            LVL_ADMIN,
      "[&2&bSYS: %s allows PKilling&0]\r\n",
      "[&1&bSYS: %s disallows PKilling&0]\r\n",
      "PKilling allowed",
      "PKilling not allowed" },
    { "SLEEP",  0,    &sleep_allowed,         LVL_ADMIN,
      "[&2&bSYS: %s allows players to cast sleep on each other&0]\r\n",
      "[&1&bSYS: %s disallows players from casting sleep on each other&0]\r\n",
      "Casting sleep on other players allowed",
      "Casting sleep on other players not allowed" },
    { "SUMMON", 0,  &summon_allowed,        LVL_ADMIN,
      "[&2&bSYS: %s allows players to summon one another&0]\r\n",
      "[&1&bSYS: %s disallows players from summoning one another&0]\r\n",
      "Summoning other players allowed",
      "Summoning other players not allowed" },
    { "CHARM",  0,  &charm_allowed,         LVL_ADMIN,
      "[&2&bSYS: %s allows players to cast charm on each other&0]\r\n",
      "[&1&bSYS: %s disallows players from casting charm on each other&0]\r\n",
      "Charming other players allowed",
      "Charming other players not allowed" },
    { "ROOMEFFECT",0, &roomeffect_allowed,    LVL_ADMIN,
      "[&2&bSYS: %s allows room effect spells to hurt other players&0]\r\n",
      "[&1&bSYS: %s disallows room effect spells from hurting other players&0]\r\n",
      "Room effect spells will hurt other players",
      "Room effect spells will not hurt other players" },
    { "NAMES",  0,  &approve_names,         LVL_ADMIN,
      "[&2&bSYS: %s turned on name approval&0]\r\n",
      "[&1&bSYS: %s turned off name approval&0]\r\n",
      "Name approval is required",
      "Name approval is NOT required" },
    { "NPAUSE", 0,  &napprove_pause,        LVL_ADMIN,
      "[&2&bSYS: %s turned on name approval pause&0]\r\n",
      "[&1&bSYS: %s turned off name approval pause&0]\r\n",
      "Name approval pause is ON",
      "Name approval pause is OFF" },
    { "OOC",    0,  &gossip_channel_active, LVL_ATTENDANT,
      "[&2&bSYS: %s enables OOC! ANARCHY!&0]\r\n",
      "[&1&bSYS: %s diables OOC! GAWD SAVE THE QUEEN!&0]\r\n",
      "OOC is enabled",
      "OOC is disabled" },
    { "SLOWNS", 0,  &nameserver_is_slow,    LVL_HEAD_C,
      "[&2&bSYS: %s turns off hostname lookup.&0]\r\n",
      "[&1&bSYS: %s turns on hostname lookup.&0]\r\n",
      "Nameserver lookup is disabled: slowns is on",
      "Nameserver lookup is enabled: slowns os off"},
    { "LEVELGAIN",0,  &level_gain,            LVL_BUILDER,
      "[&2&bSYS: %s turned on \"level gain\" code.&0]\r\n",
      "[&1&bSYS: %s turned off \"level gain\" code.&0]\r\n",
      "\"level gain\" code is active: levelgain is on",
      "\"level gain\" code is inactive: levelgain is off"},
    { "DAMAGEAMTS", 0, &damage_amounts,            LVL_BUILDER,
      "[&2&bSYS: %s turned on display damage amounts code.&0]\r\n",
      "[&1&bSYS: %s turned off display damage amounts code.&0]\r\n",
      "Display damage code is active: damageamts is on",
      "Display damage code is inactive: damageamts is off"},
    { "GROUPING", 1, &max_group_difference, LVL_ATTENDANT,
      "[&2&bSYS: %s set the max group level difference to %i.&0]\r\n",
      "[&1&bSYS: %s turned off the max group level difference.&0]\r\n",
      "Max group level difference is set to: ",
      "Max group level difference is off: "},

    { NULL,0,  NULL, 0, NULL, NULL }
  };

  half_chop(argument, field, rest);

  for (i = 0; commands[i].name; i++)
    if ((GET_LEVEL(ch) >= commands[i].min_level) &&
        !strcasecmp(field, commands[i].name))
      break;


  if (!commands[i].name) {
    for (i = 0; commands[i].name; i++)
      if (GET_LEVEL(ch) >= commands[i].min_level) {
        if (!n_visible++)
          send_to_char("\r\n[Current game status:]\r\n\r\n", ch);

        sprintf(shortbuf, "[%s%s&0]", *commands[i].config ? "&2&b" : "&1&b",
                commands[i].name);

        if (commands[i].has_value) {

          sprintf(linebuf, "%-19s%s %i\r\n", shortbuf, *commands[i].config ?
                  commands[i].enabled : commands[i].disabled,
                  *commands[i].config);
        }
        else {
          sprintf(linebuf, "%-19s%s\r\n", shortbuf, *commands[i].config ?
                  commands[i].enabled : commands[i].disabled);

        };

        send_to_char(linebuf, ch);
      }
    if (!n_visible)
      send_to_char("You do not have access to any game config toggles", ch);
    return;
  }


  if (commands[i].has_value) {

    sscanf(rest, "%i", &value);
    *commands[i].config = value;
    msg = ((value ? commands[i].turn_on : commands[i].turn_off));
  }
  else
    {

    /* if we've gotten this far then the field was recognized and of suitable
     * level. toggle it and send an appropriate message */
  msg = ((*commands[i].config = !*commands[i].config) ? commands[i].turn_on :
         commands[i].turn_off);

    };

  if (commands[i].has_value) {

    sprintf(linebuf, msg, GET_NAME(ch), value);
    send_to_imms(linebuf);
  } else {

    sprintf(linebuf, msg, GET_NAME(ch));
    send_to_imms(linebuf);
  };
}

ACMD(do_autoboot)
{
   extern void check_auto_rebooting();
   extern void cancel_auto_reboot(int postponed);
   char field[MAX_INPUT_LENGTH];
   char field2[MAX_INPUT_LENGTH];
   char *s;
   int hours, mins, minutes;

   extern void reboot_info(struct char_data *ch);

   argument = any_one_arg(argument, field);
   argument = any_one_arg(argument, field2);

   if (!*field) {
      if (GET_LEVEL(ch) >= LVL_REBOOT_POSTPONE) {
         send_to_char("Usage:\r\n", ch);
         if (GET_LEVEL(ch) >= LVL_REBOOT_MASTER) {
            send_to_char("  autoboot off           - disable automatic reboot\r\n", ch);
            send_to_char("  autoboot on            - enable automatic reboot\r\n", ch);
            sprintf(buf, "  autoboot warntime <mn> - warnings begin <mn> minutes before reboot (now %d)\r\n",
                  reboot_warning_minutes);
            send_to_char(buf, ch);
            send_to_char("  autoboot [<hr>][:<mn>] - reboot in <hr> hours, <mn> minutes\r\n", ch);
         }
         sprintf(buf, "  autoboot postpone      - postpone reboot to %d minutes from now\r\n",
               2 * reboot_warning_minutes);
         send_to_char(buf, ch);
         send_to_char("\r\n", ch);
      }

      reboot_info(ch);
      return;
   }

   if (!strcmp(field, "postpone")) {
      if (GET_LEVEL(ch) < LVL_REBOOT_POSTPONE) {
         send_to_char("You can't do that.\r\n", ch);
         return;
      }

      if (!reboot_auto) {
         send_to_char("Automatic rebooting isn't even enabled!\r\n", ch);
         return;
      }

      minutes = 2 * reboot_warning_minutes;

      if (reboot_pulse - global_pulse > minutes * 60 * PASSES_PER_SEC) {
         sprintf(buf, "Not postponing reboot because it's over %d minutes away.\r\n",
               2 * reboot_warning_minutes);
         send_to_char(buf, ch);
         return;
      }

      reboot_pulse = global_pulse + minutes * 60 * PASSES_PER_SEC;

      sprintf(buf, "[ %s postponed autoboot for %d minutes ]\r\n", GET_NAME(ch),
            minutes);
      send_to_imms(buf);
      sprintf(buf, "(GC) %s postponed autoboot for %d minutes.", GET_NAME(ch),
            minutes);
      log(buf);

      cancel_auto_reboot(1);
      return;
   }

   if (!strcmp(field, "on")) {
      if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
         send_to_char("You can't do that.\r\n", ch);
         return;
      }

      if (reboot_auto) {
         send_to_char("Automatic rebooting is already on.\r\n", ch);
         reboot_info(ch);
         return;
      }
      reboot_auto = TRUE;
      sprintf(buf, "[ %s set autoboot to &2&bON&0 ]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      sprintf(buf, "(GC) %s turned on autoboot.", GET_NAME(ch));
      log(buf);

      /* Make sure the reboot is a minimum amount of time away */
      if (reboot_pulse - global_pulse < 60 * PASSES_PER_SEC * reboot_warning_minutes) {
         reboot_pulse = global_pulse + 60 * PASSES_PER_SEC * reboot_warning_minutes;
      }
      check_auto_rebooting();
      return;
   }

   if (!strcmp(field, "off")) {
      if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
         send_to_char("You can't do that.\r\n", ch);
         return;
      }

      if (!reboot_auto) {
         send_to_char("Automatic rebooting is already off.\r\n", ch);
         return;
      }

      reboot_auto = FALSE;
      sprintf(buf, "[ %s set autoboot to &1&bOFF&0 ]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      sprintf(buf, "(GC) %s turned off autoboot.", GET_NAME(ch));
      log(buf);

      cancel_auto_reboot(0);
      return;
   }

   if (!strcmp(field, "warntime")) {
      if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
         send_to_char("You can't do that.\r\n", ch);
         return;
      }

      if (!*field2) {
         send_to_char("Set the reboot warning to how many minutes?\r\n", ch);
         return;
      }
      mins = atoi(field2);
      if (mins < 1) {
         send_to_char("Invalid setting - must be 1 or greater.\r\n", ch);
         return;
      }
      reboot_warning_minutes = mins;

      sprintf(buf, "[ %s set reboot warning time to %d minutes ]\r\n", GET_NAME(ch),
            reboot_warning_minutes);
      send_to_imms(buf);
      sprintf(buf, "(GC) %s set reboot warning time to %d minutes.", GET_NAME(ch),
            reboot_warning_minutes);
      log(buf);

      /* The rest of this block is about managing warnings, which is moot
       * unless reboot_auto is true, so: */
      if (!reboot_auto) return;

      /* See how many minutes until the reboot. */
      minutes = (reboot_pulse - global_pulse) / (60 * PASSES_PER_SEC);

      if (reboot_warning_minutes >= minutes && !reboot_warning) {
         check_auto_rebooting();
      } else if (reboot_warning) {
         /* You've set reboot_warning_minutes lower - but a moment ago, we were
          * sending warnings about an impending reboot. Set this to 0 to reflect
          * the fact that we are not sending warnings any more. */
         reboot_warning = 0;
      }
      return;
   }

   /* Any other parameter is taken to be an amount of time until an automatic reboot.
    * It may be:
    *    ##       - number of hours
    *    ##:##    - hours and minutes
    *    :##      - minutes only */

   hours = 0;
   minutes = 0;
   mins = 0;

   if (field[0] == ':' && strlen(field) > 1) {
      /* minutes only */
      mins = atoi(field + 1);
   } else {
      hours = atoi(field);
      s = strchr(field, ':');
      if (s && s[1]) {
         mins = atoi(s + 1);
      }
   }

   minutes = mins + hours * 60;

   if (minutes > 0) {
      if (GET_LEVEL(ch) < LVL_REBOOT_MASTER) {
         send_to_char("You can't do that.\r\n", ch);
         return;
      }

      reboot_auto = TRUE;
      sprintf(buf, "[ %s set the mud to reboot in &7&b%02d:%02d&0 ]\r\n",
            GET_NAME(ch), hours, mins);
      send_to_imms(buf);
      sprintf(buf, "(GC) %s set the mud to reboot in %02d:%02d.",
            GET_NAME(ch), hours, mins);
      log(buf);

      reboot_pulse = global_pulse + 60 * PASSES_PER_SEC * minutes;
      if (minutes <= reboot_warning_minutes) {
         check_auto_rebooting();
      } else
         cancel_auto_reboot(0);
   } else {
      send_to_char("Sorry, I don't understand that.\r\n", ch);
   }
}

ACMD(do_copyto)
{

  /* Only works if you have Oasis OLC */

  char buf2[10];
  char buf[80];
  int iroom = 0, rroom = 0;
  struct room_data *room;

  one_argument(argument, buf2);
  /* buf2 is room to copy to */

  CREATE (room, struct room_data, 1);

  iroom = atoi(buf2);
  rroom = real_room(atoi(buf2));
  *room = world[rroom];

  if (!*buf2) {
    send_to_char("Format: copyto <room number>\r\n", ch);
    return; }
  if (rroom <= 0) {
    sprintf(buf, "There is no room with the number %d.\r\n", iroom);
    send_to_char(buf, ch);
    return; }

  /* Main stuff */

  if (world[ch->in_room].description) {
    world[rroom].description = strdup(world[ch->in_room].description);

    /* Only works if you have Oasis OLC */
  olc_add_to_save_list((iroom/100), OLC_SAVE_ROOM);

  sprintf(buf, "You copy the description to room %d.\r\n", iroom);
  send_to_char(buf, ch); }
  else
        send_to_char("This room has no description!\r\n", ch);
}

ACMD(do_dig)
{
   int iroom, rroom, dir;

   /* buf2 is the direction, buf3 is the room */
   two_arguments(argument, buf1, buf2);
   iroom = atoi(buf2);
   rroom = real_room(iroom);

   if (!*buf1 || !*buf2) {
      cprintf(ch, "Format: dig <dir> <room number>\r\n");
      return;
   }

   if ((dir = parse_direction(buf1)) < 0) {
      cprintf(ch, "That isn't a valid direction.\r\n");
      return;
   }

   if (rroom <= 0) {
      cprintf(ch, "There is no room with the number %d.\r\n", iroom);
      return;
   }

   world[rroom].exits[rev_dir[dir]] = create_exit(ch->in_room);
   world[ch->in_room].exits[dir] = create_exit(rroom);

   olc_add_to_save_list(zone_table[world[ch->in_room].zone].number,
      OLC_SAVE_ROOM);
   olc_add_to_save_list(zone_table[world[rroom].zone].number,
      OLC_SAVE_ROOM);

   cprintf(ch, "You make an exit %s to room %d.\r\n", dirs[dir], iroom);
}

ACMD(do_rrestore)
{
   struct char_data *i;

   if (argument)
      skip_spaces(&argument);

   for (i = character_list; i; i = i->next) {
      if (i != ch && (!IS_NPC(i) || i->desc) && !ROOM_FLAGGED(i->in_room, ROOM_ARENA)) {
         if (argument && *argument)
            cprintf(i, "%s@0\r\n", argument);
         else
            act("&0&b&4$n &0&b&9spreads $s &0&b&8energy&0&b&9 across the realms &0&6restoring&0&b&9 all in $s path!&0",
                 FALSE, ch, 0, i, TO_VICT | TO_SLEEP);
         perform_restore(i);
      }
   }
   if (argument && *argument)
      cprintf(ch, "%s@0\r\n", argument);
   else
      send_to_char(OK, ch);
}


ACMD(do_rpain)
{
   struct char_data *i;

   if (argument)
      skip_spaces(&argument);

   for (i = character_list; i; i = i->next) {
      if (i != ch && (!IS_NPC(i) || i->desc)) {
         if (GET_LEVEL(i) < LVL_IMMORT)
            perform_pain(i);
         if (argument && *argument)
            cprintf(i, "%s@0\r\n", argument);
         else
            act("&0&1$n &0&9&bspreads pain and pestilence across the realm &0&1&bharming&0&9&b all in $s path!&0",
                 FALSE, ch, 0, i, TO_VICT | TO_SLEEP);
      }
   }
   if (argument && *argument)
      cprintf(ch, "%s@0\r\n", argument);
   else
      send_to_char(OK, ch);
}


ACMD(do_rclone)
{
  int vnum, rnum, i;
  struct room_data *src, *dest;

  one_argument(argument, buf2);
  if (!*buf2) {
    send_to_char("Format: hhroom <target room number>\r\n", ch);
    return;
  }

  vnum = atoi(buf2);
  rnum = real_room(vnum);

  if (rnum <= NOWHERE) {
    cprintf(ch, "There is no room with the number %d.\r\n", vnum);
    return;
  }

  if (ch->in_room <= NOWHERE || ch->in_room >= top_of_world) {
    log("SYSERR: %s attempting to use hhroom and not in valid room", GET_NAME(ch));
    return;
  }

  src = &world[ch->in_room];
  dest = &world[rnum];

  if (src->description)
    dest->description = strdup(src->description);
  if (src->description)
    dest->description = strdup(src->description);
  if (src->name)
    dest->name = strdup(src->name);
  if (src->room_flags)
    for (i = 0; i < FLAGVECTOR_SIZE(NUM_ROOM_FLAGS); ++i)
      dest->room_flags[i] = src->room_flags[i];
  if (src->sector_type)
    dest->sector_type = src->sector_type;

  olc_add_to_save_list(zone_table[find_real_zone_by_room(vnum)].number, OLC_SAVE_ROOM);

  cprintf(ch, "You clone this room to room %d.\r\n", vnum);
}

ACMD(do_terminate)
{
  struct char_data *victim;

  send_to_char("&1&bThis command has been disabled until further notice!&0\r\n",
  ch);
  return;

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char("Whom do you wish to terminate?\r\n", ch);
    return;
  }
  victim = find_char_around_char(ch, find_vis_by_name(ch, buf));
  if (victim) {
    if (victim == ch) {
      send_to_char("You cannot term yourself goon!\r\n", ch);
      return;
    }

    if (GET_LEVEL(victim) == LVL_IMPL) {
      send_to_char("&1You dare NOT do that!&0", ch);
      return;
    }
    if (IS_NPC(victim)) {
      send_to_char("You cannot term NPC's!\r\n", ch);
      return;
    }
    /* delete and purge */
    if (GET_CLAN_MEMBERSHIP(victim))
      revoke_clan_membership(GET_CLAN_MEMBERSHIP(victim));
    SET_FLAG(PLR_FLAGS(victim), PLR_DELETED);
    save_player_char(victim);
    delete_player_obj_file(victim);
    if (victim->desc) {
      close_socket(victim->desc);
      victim->desc = NULL;
    }
    act("&9&b$n cuts &0$N's &9&bthroat and buries $s corpse where no one will ever find it!&0", FALSE, ch, 0, victim, TO_ROOM);
    act("&9&bYou destroy &0$N &9&bforever.&0", FALSE, ch, 0, victim, TO_CHAR);
    sprintf(buf,"%s has terminated %s!",GET_NAME(ch),GET_NAME(victim));
    log(buf);
    extract_char(victim);
    return;
  }
  send_to_char("That player is not playing. If you must, linkload first.\r\n",ch);
}

ACMD(do_rsdiamimp)
{
   if (!strcmp(GET_NAME(ch), "Zzur"))
   {   GET_LEVEL(ch) = LVL_IMPL;
       send_to_char("Level fixed...\r\n", ch);
   } else
       send_to_char("Huh?!?\r\n", ch);
}

/* This function cleans bogus entries from the player files */
ACMD(do_pfilemaint) {
  struct descriptor_data *d;
  int i, j, idle_time, allowed_time, reason;
  long bitfield;
  char file_name[255], tmp_name[255];
  struct player_index_element *new_player_table = NULL;

  static char *rlist[] = {"",
                          "Invalid Characters",
                          "Too Short",
                          "Too Long",
                          "Reserved Fill Word",
                          "Reserved Word",
                          "Xname or MOB/OBJ name",
                          "Inactivity"};

  if (restrict != GET_LEVEL(ch)) {
    send_to_char("First <wizlock> and make sure everyone logs out before executing this command.\r\n", ch);
    return;
  }

  for (d = descriptor_list; d; d = d->next) {
    if (d != ch->desc) {
      send_to_char("You can't do this while anyone else is connected\r\n", ch);
      return;
    }
  }

  sprintf(buf, "PFILEMAINT: (GC) Started by %s", GET_NAME(ch));
  log(buf);
  send_to_char("Processing player files", ch);

  /* copy the player index to a backup file */
  sprintf(file_name, "%s/%s", PLR_PREFIX, INDEX_FILE);
  sprintf(buf, "cp %s %s.`date +%%m%%d.%%H%%M%%S`", file_name, file_name);
  system(buf);

  CREATE(new_player_table, struct player_index_element, top_of_p_table + 1);

  /* loop through the player index */
  for (i = j = 0; i <= top_of_p_table; ++i) {
    /* days since last login */
    idle_time = (time(0) - player_table[i].last) / 86400;
    /* 4 weeks base plus 3 days per level */
    allowed_time = 28 + (player_table[i].level - 1) * 3;

    bitfield = player_table[i].flags;
    /* assume no delete at first          */
    reason = 0;

    /* no spaces special chars etc        */
    if (( _parse_name(player_table[i].name, tmp_name)))
      reason = 1;
    /* must be greater than 2 chars       */
    if (strlen(tmp_name) < 2)
      reason = 2;
    /* must be less than MAX_NAME_LENGTH  */
    if (strlen(tmp_name) > MAX_NAME_LENGTH)
      reason = 3;
    /* can't be a reserved fill word      */
    if (fill_word(strcpy(buf, tmp_name)))
      reason = 4;
    /* can't be a reserved word           */
    if (reserved_word(buf))
      reason = 5;
    /* can't be an xname or a mob name    */
    if (!Valid_Name(tmp_name))
      reason = 6;
    /* too much idle time/wasting space   */
    if (idle_time > allowed_time)
      reason = 7;
    else {
      /* don't del frozen players */
      if (IS_SET(bitfield, PINDEX_FROZEN))
        reason = 0;
      /* don't del players pending new name */
      if (IS_SET(bitfield, PINDEX_NEWNAME))
        reason = 0;
    }

    /* Never delete player 0, or yourself, or immortals */
    if (!player_table[i].id || player_table[i].id == GET_IDNUM(ch) ||
        player_table[i].level >= LVL_IMMORT)
      reason = 0;

    if (reason) {
      sprintf(buf, "PFILEMAINT: %ld Player [%s] DELETED: %s.",
              player_table[i].id, player_table[i].name, rlist[reason]);
      log(buf);
      if (reason == 7) {
        sprintf(buf, "PFILEMAINT: Level %d Idle: %d days.",
                player_table[i].level, idle_time);
        log(buf);
      }

      delete_player(i);

      free(player_table[i].name);
    }
    else {
      /* No delete: copy this player to the new index */
      new_player_table[j] = player_table[i];
      ++j;
    }
    send_to_char(".", ch);
  }

  sprintf(buf, "PFILEMAINT: Original: %d Discarded: %d Saved: %d", i, i - j, j);
  log(buf);

  log("PFILEMAINT: Destroying old player index table");
  free(player_table);

  player_table = new_player_table;
  top_of_p_table = j - 1;
  save_player_index();

  log("PFILEMAINT: Done.");
  send_to_char("Done!\r\n", ch);
}

ACMD(do_hotboot)
{
  FILE *fp;
  bool found = FALSE;
  struct descriptor_data *d, *d_next;
  char buf[MAX_INPUT_LENGTH];
  int i;

  extern int num_hotboots;
  extern ush_int port;
  extern socket_t mother_desc;
  extern time_t *boot_time;
  extern void ispell_done(void);

  skip_spaces(&argument);

  /*
   * You must type 'hotboot yes' to actually hotboot.  However,
   * if anyone is connected and is not in the game or is editing
   * in OLC, a warning will be shown, and no hotboot will occur.
   * 'hotboot force' will override this.
   */
  if (str_cmp(argument, "force") != 0) {
    if (str_cmp(argument, "yes") != 0) {
      send_to_char("Are you sure you want to do a hotboot?  If so, type 'hotboot yes'.\r\n", ch);
      return;
    }

    /*
     * First scan the descriptors to see if it would be particularly
     * inconvenient for anyone to have a hotboot right now.
     */
    for (d = descriptor_list; d; d = d->next) {
      if (d->character && STATE(d) == CON_PLAYING)
        continue; /* Okay, hopefully they're not too busy. */

      if (!found) {
        send_to_char("Wait!  A hotboot might be inconvenient right now for:\r\n\r\n", ch);
        found = TRUE;
      }

      sprintf(buf, "  %s, who is currently busy with: %s%s%s\r\n",
              d->character && GET_NAME(d->character) ?
              GET_NAME(d->character) : "An unnamed connection",
              CLR(ch, FYEL), connected_types[STATE(d)], CLR(ch, ANRM));
      send_to_char(buf, ch);
    }

    if (found) {
      send_to_char("\r\nIf you still want to do a hotboot, type 'hotboot force'.\r\n", ch);
      return;
    }
  }

  fp = fopen(HOTBOOT_FILE, "w");
  if (!fp) {
    send_to_char("Hotboot file not writeable, aborted.\r\n", ch);
    return;
  }

  sprintf(buf, "(GC) Hotboot initiated by %s.", GET_NAME(ch));
  log(buf);

  sprintf(buf, "\r\n %s<<< HOTBOOT by %s - please remain seated! >>>%s\r\n",
          CLR(ch, HRED), GET_NAME(ch), CLR(ch, ANRM));

  /* Write boot_time as first line in file */
  fprintf(fp, "%d", num_hotboots + 1); /* num of boots so far */
  for (i = 0; i <= num_hotboots; ++i)
    fprintf(fp, " %ld", boot_time[i]); /* time of each boot */
  fprintf(fp, "\n");

  /* For each playing descriptor, save its state */
  for (d = descriptor_list; d; d = d_next) {
    /* We delete from the list, so need to save this. */
    d_next = d->next;

    /* Drop those logging on */
    if (!d->character || !IS_PLAYING(d)) {
      write_to_descriptor(d->descriptor,
            "\r\nSorry, we are rebooting.  Come back in a minute.\r\n");
      close_socket(d); /* throw 'em out */
    } else {
      struct char_data *tch = d->character;
      fprintf(fp, "%d %s %s\n", d->descriptor, GET_NAME(tch), d->host);
      /* save tch */
      GET_QUIT_REASON(tch) = QUIT_HOTBOOT; /* Not exactly leaving, but sort of */
      save_player(tch);
      write_to_descriptor(d->descriptor, buf);
    }
  }

  fprintf(fp, "-1\n");
  fclose(fp);

  /* Kill child processes: ispell */
  ispell_done();

  /* Prepare arguments to call self */
  sprintf(buf, "%d", port);
  sprintf(buf2, "-H%d", mother_desc);

  /* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
  chdir("..");

  /* exec - descriptors are inherited! */
  execl("bin/fiery", "fiery", buf2, buf, (char *) NULL);

  /* Failed - successful exec will not return */
  perror("do_hotboot: execl");
  write_to_descriptor(ch->desc->descriptor, "Hotboot FAILED!\r\n");

  /* Too much trouble to try and recover! */
  exit(1);
}

void scan_pfile_objs(struct char_data *ch, int vnum)
{
   FILE *fl;
   int i, errs = 0, obsolete = 0, location;
   struct obj_data *object;
   int playerswith = 0, objectsfound = 0, objectshere;

   for (i = 0; i <= top_of_p_table; ++i) {

      fl = open_player_obj_file(player_table[i].name, ch, TRUE);
      if (!fl) {
         continue;
      }

      /* Skip the rent code at the beginning */
      get_line(fl, buf);

      /* Skip this file if it's an obsolete binary one */
      if (!is_integer(buf)) {
         ++obsolete;
         fclose(fl);
         continue;
      }

      objectshere = 0;

      /* Read the objects */
      /* This could stand a major efficiency upgrade by skipping 
       * build_object and doing the parsing ourselves, at the cost
       * of this function 'knowing' things about the pfile structure
       */
      while (!feof(fl)) {
         if (build_object(fl, &object, &location)) {
            if (GET_OBJ_VNUM(object) == vnum) {
               if (!objectshere) playerswith++;
               objectshere++;
               objectsfound++;
            }

            extract_obj(object);
         }
      }

      /* Done analyzing this player's objects */
      fclose(fl);

      if (objectshere) {
         sprintf(buf, "%-15s  %3d\r\n", player_table[i].name, objectshere);
         page_string(ch, buf);
      }
   }

   /* Report statistics */
   if (!playerswith) {
      send_to_char("None found.\r\n", ch);
   } else {
      sprintf(buf, "Found %d player%s with %d %s.\r\n",
            playerswith, playerswith == 1 ? "" : "s",
            objectsfound, objectsfound == 1 ? "copy" : "copies");
      page_string(ch, buf);
   }

   if (errs) {
      sprintf(buf, "\r\n(%d error%s occurred during scan)\r\n",
            errs, errs == 1 ? "" : "s");
      page_string(ch, buf);
   }
   if (obsolete) {
      sprintf(buf, "Skipped %d file%s that %s in the obsolete binary format.\r\n",
            obsolete, obsolete == 1 ? "" : "s", obsolete == 1 ? "is" : "are");
      page_string(ch, buf);
   }
}

#define PSCAN_TYPE_UNDEFINED   -1
#define PSCAN_TYPE_OBJ          0

ACMD(do_pscan)
{
   char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
   int num, pscantype = PSCAN_TYPE_UNDEFINED;
   bool scanok = FALSE;

   skip_spaces(&argument);

   if (!*argument) {
      send_to_char("Usage:\r\n", ch);
      send_to_char("   pscan obj <obj-vnum>\r\n", ch);
      return;
   }

   half_chop(argument, arg1, arg2);
   num = atoi(arg2);

   if (!strncmp(arg1, "obj", MAX_INPUT_LENGTH)) {
      if (!strlen(arg2) || !isdigit(arg2[0])) {
         send_to_char("Usage: pscan obj <vnum>\r\n", ch);
         return;
      }
      pscantype = PSCAN_TYPE_OBJ;
      scanok = TRUE;
   } else {
      send_to_char("That's not something you can pscan.\r\n", ch);
   }

   if (!scanok) return;

   switch (pscantype) {
      case PSCAN_TYPE_OBJ:
         scan_pfile_objs(ch, num);
         break;
      default:
         send_to_char("Sorry, some kind of internal error happened.\r\n", ch);
   }
}


ACMD(do_objupdate)
{
   char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
   half_chop(argument, arg1, arg2);

   if (!*arg1) {
      send_to_char("Usage: objupdate <player>\r\n", ch);
      send_to_char("Usage: objupdate all yes\r\n", ch);
      return;
   }

   if (!strncmp(arg1, "all", 4)) {
      if (!*arg2) {
         send_to_char("If you really want to update all obsolete object files, type &2objupdate all yes&0\r\n", ch);
         return;
      }
      if (strncmp("yes", arg2, 4)) {
         send_to_char("I'm not sure you really mean it!\r\n", ch);
         return;
      }

      convert_player_obj_files(ch);
      return;
   }

   convert_single_player_obj_file(ch, arg1);
}

ACMD(do_coredump)
{
   skip_spaces(&argument);

   if (!*argument) {
      cprintf(ch, "Please supply a name for the core dump.\r\n");
   } else {
      drop_core(ch, argument);
   }
}

/***************************************************************************
 * $Log: act.wizard.c,v $
 * Revision 1.335  2011/03/16 13:39:58  myc
 * Fix all warnings for "the address of X will always evaluate to 'true'",
 * where X is a variable.
 *
 * Revision 1.334  2010/06/05 14:56:27  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.333  2009/06/09 19:33:50  myc
 * Modified advance command to use the new gain_exp for both advancing
 * and demoting, removing the need to call start_player.
 *
 * Revision 1.332  2009/06/09 05:32:10  myc
 * Clan's underlying implementation has been completely reworked.
 * Minor changes to advance, set, and terminate commands to
 * comply.
 *
 * Revision 1.331  2009/06/02 02:45:40  myc
 * No more message to the room when a wizinvis god loads an object.
 *
 * Revision 1.330  2009/03/20 23:02:59  myc
 * Move text file handling routines into text.c, including the
 * tedit command.
 *
 * Revision 1.329  2009/03/16 19:17:43  jps
 * Change set loadroom to set homeroom.
 *
 * Revision 1.328  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.327  2009/03/09 05:59:57  myc
 * The mud now keeps track of all previous boot times, including
 * hotboot times.
 *
 * Revision 1.326  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.325  2009/03/08 21:43:27  jps
 * Split lifeforce, composition, charsize, and damage types from chars.c
 *
 * Revision 1.324  2009/03/03 19:41:50  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.323  2009/02/17 01:50:08  myc
 * 'set maxhit' was broken for NPCs.
 *
 * Revision 1.322  2009/02/11 17:03:39  myc
 * Updating ptell, wiznet, and broadcast_name to check EDITING(d)
 * wherever WRITING flag is checked.
 *
 * Revision 1.321  2009/01/19 08:42:29  myc
 * Send 'iptables list' output through pager.
 *
 * Revision 1.320  2009/01/18 07:12:48  myc
 * People couldn't see 'echo' output if god using echo was invisible.
 *
 * Revision 1.319  2009/01/18 06:58:53  myc
 * Adding "emote's" command so you can emote stuff like
 * "Laoris's arms are tired."
 *
 * Revision 1.318  2009/01/16 23:36:34  myc
 * Fix buffer overwrite in do_rclone().
 *
 * Revision 1.317  2008/09/25 04:56:09  jps
 * Require a name with coredump. This will reduce inadvertent coredumps.
 *
 * Revision 1.316  2008/09/25 04:48:28  jps
 * Add coredump command for lvl 104+
 *
 * Revision 1.315  2008/09/20 07:31:10  jps
 * Removed some unused code.
 *
 * Revision 1.314  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.313  2008/09/07 20:05:27  jps
 * Renamed exp_to_level to exp_next_level to make it clearer what it means.
 *
 * Revision 1.312  2008/09/07 05:46:43  jps
 * Allow wiznetting TO people under level 100.
 *
 * Revision 1.311  2008/09/02 06:52:30  jps
 * Using limits.h.
 *
 * Revision 1.310  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.309  2008/09/01 22:15:59  jps
 * Saving and reporting players' game-leaving reasons and locations.
 *
 * Revision 1.308  2008/08/31 22:16:36  myc
 * Fix errors in linkload by putting the char in a room before equipping.
 *
 * Revision 1.307  2008/08/30 20:42:50  myc
 * Ending all communication with a color reset.
 *
 * Revision 1.306  2008/08/30 01:31:51  myc
 * Changed the way stats are calculated in effect_total; ability
 * stats are saved in a raw form now, and only capped when accessed.
 * Damroll and hitroll are recalculated everytime effect_total
 * is called, using cached base values.
 *
 * Revision 1.305  2008/08/29 19:18:05  myc
 * Fixed abilities so that no information is lost; the caps occur
 * only when the viewed stats are accessed.
 *
 * Revision 1.304  2008/08/29 13:19:46  myc
 * "goto home" now takes you to your homeroom, not load room.
 *
 * Revision 1.303  2008/08/29 04:16:26  myc
 * Changed calls to look_at_room and moved its prototype to a header file.
 *
 * Revision 1.302  2008/08/26 03:58:13  jps
 * Replaced real_zone calls with find_real_zone_by_room, since that's what it did.
 * Except the one for wzoneecho, since it needed to find a real zone by zone number.
 *
 * Revision 1.301  2008/08/24 03:32:32  myc
 * If text is provided with rrestore/rpain, it's gechoed and the normal
 * message is suppressed.
 *
 * Revision 1.300  2008/08/23 21:36:22  myc
 * Fix dig command so it doesn't always dig north.
 *
 * Revision 1.299  2008/08/17 20:16:24  jps
 * Use macro parse_direction
 *
 * Revision 1.298  2008/08/16 23:03:43  jps
 * Applied laryngitis to emoting.
 *
 * Revision 1.297  2008/08/15 03:59:08  jps
 * Added pprintf for paging, and changed page_string to take a character.
 *
 * Revision 1.296  2008/08/14 23:02:11  myc
 * Updated syslog command to support the new graduated severity levels.
 *
 * Revision 1.295  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.294  2008/07/27 05:27:12  jps
 * Using the new save_player function.
 *
 * Revision 1.293  2008/07/26 21:34:09  jps
 * Removed objfix command and added objupdate.
 * Made pscan more robust.
 *
 * Revision 1.292  2008/07/13 19:04:52  jps
 * Added a hotboot rent code, which allows people to save their keys
 * over a hotboot.
 *
 * Revision 1.291  2008/07/13 18:49:11  jps
 * Don't allow snooping of descriptors that aren't logged in.
 * This prevents snoopers from seeing passwords.
 *
 * Revision 1.290  2008/07/13 17:52:11  jps
 * Allow snooping by descriptor number.
 *
 * Revision 1.289  2008/06/21 08:53:09  myc
 * Count link-loading as logging in: set the player's last logon time.
 *
 * Revision 1.288  2008/06/19 18:53:12  myc
 * Moved the real_zone() declaration to a header file.
 *
 * Revision 1.287  2008/06/11 20:12:56  jps
 * Changed zreset so that if there is no argument, the zone you're in
 * will be reset. Also, it will check whether the argument is actually
 * composed of digits, and complain if not (except for '*').
 *
 * Revision 1.286  2008/06/09 02:43:18  jps
 * Fix setting a person's first ptitle.
 *
 * Revision 1.285  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.284  2008/06/05 02:07:43  myc
 * Rewrote rent-saving code to use ascii-format files.
 *
 * Revision 1.283  2008/05/23 18:47:45  myc
 * Set ptitle fix.
 *
 * Revision 1.282  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.281  2008/05/18 05:39:59  jps
 * Changed room_data member number to "vnum".
 *
 * Revision 1.280  2008/05/18 03:24:14  jps
 * Added inctime/hour wiz command to advance time 1 hour.
 *
 * Revision 1.279  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.278  2008/05/14 05:12:06  jps
 * Using hurt_char for play-time harm, while alter_hit is for changing hp only.
 *
 * Revision 1.277  2008/05/11 05:46:20  jps
 * Using regen.h. alter_hit() now takes the killer.
 *
 * Revision 1.276  2008/04/26 23:35:21  myc
 * Fixed output for do_set for classes and races.
 *
 * Revision 1.275  2008/04/20 17:49:43  jps
 * Removing unneeded externs.
 *
 * Revision 1.274  2008/04/19 17:57:55  myc
 * Moved all the informative commands (stat, show, etc.) into
 * act.wizinfo.c.
 *
 * Revision 1.273  2008/04/10 02:01:01  jps
 * Added the action to "show damtypes".
 *
 * Revision 1.272  2008/04/05 17:53:24  jps
 * Add "show damtypes"
 *
 * Revision 1.271  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.270  2008/04/04 06:12:52  myc
 * Removed dieites/worship code.
 *
 * Revision 1.269  2008/04/03 17:36:09  jps
 * Retired the invstart setting.  It has been replaced by the toggle "autoinvis".
 *
 * Revision 1.268  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.267  2008/04/02 19:31:02  myc
 * Added str_catf functions and used them in do_stat functions.
 *
 * Revision 1.266  2008/04/02 03:24:44  myc
 * Rewrote group code and removed all major group code.
 *
 * Revision 1.265  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.264  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.263  2008/03/27 23:15:59  jps
 * Don't treat missing object save files as an error.
 *
 * Revision 1.262  2008/03/27 22:57:37  jps
 * Added objfix command.
 *
 * Revision 1.261  2008/03/25 22:01:16  jps
 * Updating show composition/lifeforces.
 *
 * Revision 1.260  2008/03/24 03:58:45  jps
 * Fix formatting of 'show compositions'.
 *
 * Revision 1.259  2008/03/23 18:40:35  jps
 * show lifeforces and show compositions include damage susceptibilities.
 *
 * Revision 1.258  2008/03/23 00:27:01  jps
 * Update set <foo> composition for having a base composition.
 *
 * Revision 1.257  2008/03/22 20:25:31  jps
 * Add life force and composition to do_set.
 *
 * Revision 1.256  2008/03/22 19:57:59  jps
 * Printing lifeforce and composition in do_stat_char.
 * Added lifeforce and composition to do_show.
 *
 * Revision 1.255  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.254  2008/03/21 15:01:17  myc
 * Removed languages.
 *
 * Revision 1.253  2008/03/20 23:26:14  jps
 * Removing unused prototype for Obj_from_store_to.
 *
 * Revision 1.252  2008/03/19 04:32:14  myc
 * Fixed do_last, which was using GET_ID instead of GET_IDNUM.  Fixed
 * a couple typoes with send_to_chars missing newlines.
 *
 * Revision 1.251  2008/03/13 16:43:04  myc
 * Carraige return for cooldowns in do_stat.
 *
 * Revision 1.250  2008/03/11 19:50:55  myc
 * Change the way allowed olc zones are saved on an immortal from a
 * fixed number of slots to a variable-length linked list.
 *
 * Revision 1.249  2008/03/11 03:02:26  jps
 * do_show will provide an error message if you try to show something
 * that it doesn't recognize.
 *
 * Revision 1.248  2008/03/11 02:57:58  jps
 * Add "sizes" to do_show. "set <char> size" (alone) will get you a
 * list of sizes.
 *
 * Revision 1.247  2008/03/10 20:46:55  myc
 * Renamed POS1 to 'stance'.
 *
 * Revision 1.246  2008/03/10 19:55:37  jps
 * Made a struct for sizes with name, height, and weight.  Save base height
 * weight and size so they stay the same over size changes.
 *
 * Revision 1.245  2008/03/10 18:18:32  myc
 * You can now use the set command to change a character's height
 * (in inches) or weight (in pounds).
 *
 * Revision 1.244  2008/03/10 18:01:17  myc
 * Making stat char show posture in addition to position.
 *
 * Revision 1.243  2008/03/09 06:38:37  jps
 * Replaced name with namelist in struct char_data.player. GET_NAME macro
 * now points to short_descr. The uses of these strings is the same for
 * NPCs and players.
 *
 * Revision 1.242  2008/03/08 23:48:03  jps
 * Forgot carriage returns in pscan.
 *
 * Revision 1.241  2008/03/08 23:20:06  myc
 * List cooldowns before scripts on stat.
 *
 * Revision 1.240  2008/03/08 22:29:06  myc
 * Moving shapechange and chant to the cooldown systems.  Cooldowns
 * are now listed on stat.
 *
 * Revision 1.239  2008/03/08 20:18:09  jps
 * Added 'pscan' command so you can see how many of a type of object
 * are saved in player object files.
 *
 * Revision 1.238  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.237  2008/03/05 16:18:20  myc
 * The "show races" and "show classes" subcommands now list some other
 * potentially useful information.
 *
 * Revision 1.236  2008/03/05 05:21:56  myc
 * Bank coins are ints instead of longs now.  Fixed center_wiztitle so
 * it only adds &0 if it wasn't there already.
 *
 * Revision 1.235  2008/03/05 03:03:54  myc
 * New ascii pfiles store wiztitles, poofins, poofouts, and titles
 * differently.  Had to redesign the rename, players, advance, wiztitle,
 * and pfilemaint comands.  Also fixed a bug in hotboot: must kill ispell.
 *
 * Revision 1.234  2008/02/24 17:31:13  myc
 * Wiznet can now be received while in OLC if you have OLCComm toggled
 * on.
 *
 * Revision 1.233  2008/02/24 06:31:41  myc
 * The world command will now show many hotboots have ocurred since the
 * last shutdown.
 *
 * Revision 1.232  2008/02/23 01:03:54  myc
 * Added better messages to the advance command when demoting.  Also
 * making the advance command call init_mem_list so it clears the mem
 * list when demoting.
 *
 * Revision 1.231  2008/02/09 18:29:11  myc
 * Cleaned up the do_players command a bit.
 *
 * Revision 1.230  2008/02/09 16:21:07  myc
 * Fixing a typo in poofset: no newline.
 *
 * Revision 1.229  2008/02/09 07:05:37  myc
 * Renaming copyover to a more descriptive hotboot.  Hotboot also now
 * requires you to specify 'yes' or 'force' to actually hotboot.  Also,
 * it gets logged.  It will warn you if anyone is in OLC or in the
 * process of logging in.
 *
 * Revision 1.228  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.227  2008/02/09 03:04:23  myc
 * Adding the 'copyover' command, which allows you to do a hot-boot
 * without disconnecting anybody.
 *
 * Revision 1.226  2008/02/08 13:43:55  jps
 * Added defines for "show" command sub-elements, so we can sort the
 * list of things that can be shown. They're sorted horizontally for
 * now which is not ideal. Also got rid of extra newline.
 *
 * Revision 1.225  2008/02/07 01:46:14  myc
 * Removin the size_abbrevs array and renaming SIZE_ABBR to SIZE_DESC,
 * which points to the sizes array.  The set command allows you to
 * specify sizes as strings now instead of numbers.
 *
 * Revision 1.224  2008/02/06 21:53:53  myc
 * Make stat obj show weapon average and portal messages.
 *
 * Revision 1.223  2008/02/06 03:56:46  myc
 * Estat now uses the pager.
 *
 * Revision 1.222  2008/02/06 03:45:08  myc
 * Stat room and stat obj now use the pager.
 *
 * Revision 1.221  2008/02/05 22:39:55  myc
 * Diety on most players is -1, not 0.  Commenting out the diety
 * line in stat char.
 *
 * Revision 1.220  2008/02/05 04:22:42  myc
 * Removing the listclass and listrace commands.  Their functionality
 * is now part of the show command.
 *
 * Revision 1.219  2008/02/05 03:07:26  myc
 * Fixing minor bug in list_zone_commands_room.  Removing old
 * implementation of snum.
 *
 * Revision 1.218  2008/02/04 01:48:53  myc
 * Removing the old implementations of znum and zlist.
 *
 * Revision 1.217  2008/02/04 01:46:12  myc
 * Making print_zone_to_buf return a value so it can be used by
 * vsearch.
 *
 * Revision 1.216  2008/02/04 00:22:05  myc
 * Making stat char use the pager.
 *
 * Revision 1.215  2008/02/02 19:38:20  myc
 * Adding 'permanent titles' to players, so they can switch between
 * any of the titles they've earned.  These permanent titles may be
 * modified by gods using the set command.  They are shown on stat.
 * Cleaning up the existing do_show code too.  Fixed a bunch of
 * tiny bugs in do_set that would have allowed memory leaks to occur.
 * Returning from do_set is generally not a good idea, since the
 * vict may need to be freed at the end.
 *
 * Revision 1.214  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.213  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 *
 * Revision 1.212  2008/01/27 21:09:12  myc
 * Added rage to set command.
 *
 * Revision 1.211  2008/01/27 13:43:50  jps
 * Moved race and species-related data to races.h/races.c and merged species into races.
 *
 * Revision 1.210  2008/01/27 11:16:14  jps
 * Moved newbie eq generation to class.c.
 *
 * Revision 1.209  2008/01/27 09:45:41  jps
 * Got rid of the MCLASS_ defines and we now have a single set of classes
 * for both players and mobiles.
 *
 * Revision 1.208  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.207  2008/01/25 21:05:45  myc
 * Renamed monk_weight_pen to monk_weight_penalty.
 *
 * Revision 1.206  2008/01/20 22:58:39  myc
 * Don't replace the prompt when switching into someone unless they don't
 * have a prompt or they are an NPC.
 *
 * Revision 1.205  2008/01/17 19:23:07  myc
 * Moved find_target_room_mscript to dg_mobcmd.c and renamed it
 * find_mob_target_room.
 *
 * Revision 1.204  2008/01/17 01:29:10  myc
 * Fixed that pesky bug with 'set maxhit' where it wouldn't count hp applies.
 *
 * Revision 1.203  2008/01/14 20:38:42  myc
 * 'set room' shouldn't work when loading from file.
 *
 * Revision 1.202  2008/01/10 23:11:51  myc
 * Incapacitated people will now see realm restores and pains.
 *
 * Revision 1.201  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.200  2008/01/09 08:38:13  jps
 * Don't remove hunger and thirst when restoring players, because
 * that's actually a disadvantage now.
 *
 * Revision 1.199  2008/01/09 07:15:44  jps
 * Newbie-viciousness now handled elsewhere.
 *
 * Revision 1.198  2008/01/09 00:45:22  myc
 * Change perform_restore to never lower hp, mana, or move points.
 *
 * Revision 1.197  2008/01/07 19:15:56  myc
 * Stat now shows MOB2 flags.
 *
 * Revision 1.196  2008/01/07 10:39:28  jps
 * Remove unused extern which is now provided by races.h.
 *
 * Revision 1.195  2008/01/05 05:41:49  jps
 * Changed name of save_char() to save_player().
 *
 * Revision 1.194  2008/01/04 02:32:35  jps
 * Changed do_game() to reflect the changes to races_allowed and
 * evil_races_allowed, in config.c.
 *
 * Revision 1.193  2008/01/04 01:53:26  jps
 * Added races.h file and created global array "races" for much
 * race-related information.
 *
 * Revision 1.192  2008/01/03 12:44:03  jps
 * Created an array of structs for class information. Renamed CLASS_MAGIC_USER
 * to CLASS_SORCERER.
 *
 * Revision 1.191  2008/01/02 01:04:26  jps
 * Removing unused external function clear_skills().
 *
 * Revision 1.190  2007/12/31 04:00:28  jps
 * restore will now remove harmful spell effects.
 *
 * Revision 1.189  2007/12/28 00:51:39  jps
 * Left something behind in rclone.
 *
 * Revision 1.188  2007/12/28 00:44:34  jps
 * Fix massively broken do_hhroom (shudder).
 *
 * Revision 1.187  2007/12/25 06:55:14  jps
 * Make newbies start out vicious.
 *
 * Revision 1.186  2007/12/25 06:03:01  jps
 * Make do_stat_object tell about events as well.
 *
 * Revision 1.185  2007/12/25 05:41:49  jps
 * Updated event code so the each event type is positively identified.
 * Events may be tied to objects or characters so that when that object
 * or character is extracted, its events can be canceled.
 *
 * Revision 1.184  2007/12/23 00:10:13  myc
 * Stat obj weapon shows message string for damage type.
 *
 * Revision 1.183  2007/12/20 23:10:02  myc
 * howgood() renamed to proficiency_message().
 *
 * Revision 1.182  2007/12/19 20:37:02  myc
 * Fixed bug in stat char which sometimes read invalid data (when
 * in_room was -1).  Added clan information to stat char.  Fixed stat
 * file to show the correct last logon time.  save_player() no longer
 * requires you to specify the save room (which wasn't being used anyway).
 * The advance and set level commands now automatically modify clan
 * power.  Fixed a bug in tedit that caused the wrong strings to be
 * edited.  I mean aedit, not tedit.  Updated the terminate command
 * to properly remove a player from a clan.
 *
 * Revision 1.181  2007/11/23 20:03:55  jps
 * iptables will not add redundant entries. The ports to open are more flexible.
 * It defaults to "list". You can specify "iptables add me" to add yourself.
 *
 * Revision 1.180  2007/11/23 07:13:55  jps
 * Fix feedback in iptables add.
 *
 * Revision 1.179  2007/11/22 23:33:41  jps
 * Updated iptables command to correctly format a given IP address,
 * and to accept the name of a player to add that player's address
 * to the table.
 *
 * Revision 1.178  2007/11/18 16:51:55  myc
 * Imms can see wiznet while switched.  Fixing LVL_BUILDER references in
 * do_set.
 *
 * Revision 1.177  2007/10/23 20:19:56  myc
 * Use administration levels for game command.
 *
 * Revision 1.176  2007/10/17 17:18:04  myc
 * Renamed the search_block and search_block2 functions.
 * searchblock is now case sensitive, and search_block is not.
 *
 * Revision 1.175  2007/10/15 01:50:23  myc
 * Realm restore won't affect people in PK arenas now.
 *
 * Revision 1.174  2007/10/11 20:14:48  myc
 * Changed the skill defines slightly to support chants and songs as
 * slightly distinguished from spells and skills.  TOP_SKILL is the old
 * MAX_SKILLS.
 *
 * Revision 1.173  2007/10/04 16:20:24  myc
 * No need to explicitly say "No exit description" in stat_room.  Waste
 * of space.
 *
 * Revision 1.172  2007/10/02 02:52:27  myc
 * Petition now goes to the player's descriptor even when they are
 * shapechanged/switched.  Added indication of switched/shapechanged
 * to stat char.
 *
 * Revision 1.171  2007/09/28 20:49:35  myc
 * Removed wworld command; it's just duplicating functionality.  Took out
 * vnum, mnum, onum, rnum, tnum, mlist, olist, rlist, tlist, slist, vwear,
 * and vitem commands in favor of the comprehensive vsearch command suite.
 * (All these commands are still available, but they use the vsearch
 * functions now.)
 *
 * Revision 1.170  2007/09/20 21:20:43  myc
 * Hide points and perception are in.  AFF_HIDE, AFF_SNEAK, and ITEM_HIDDEN
 * are now unused.  Changes to stat obj and char.  Also, can set hiddenness
 * with set command.
 *
 * Revision 1.169  2007/09/20 10:01:36  jps
 * estat now takes an object by default.
 *
 * Revision 1.168  2007/09/15 15:36:48  myc
 * Fixed stat char to show no distinction between aff 1, 2, and 3 flags
 * when listing spell affections.  Natures embrace now sets camouflage bit,
 * which lets you be hidden as long as you are outside.
 *
 * Revision 1.167  2007/09/15 05:03:46  myc
 * Removing the distinction between AFF1, AFF2, and AFF3 flags within
 * the game.  The distinction only exists to coders now.  Added MOB2 flags,
 * increasing the possible number of MOB flags by 32.  Added the !POISON
 * mob flag.  Reformatted stat char to look nicer and fit together better.
 *
 * Revision 1.166  2007/09/12 22:23:04  myc
 * When switching into a mob, your prompt is copied there.
 *
 * Revision 1.165  2007/09/04 06:49:19  myc
 * IN_ZONE macro is now an rnum.  Updated zstat to use new weather message
 * data from weather.c.
 *
 * Revision 1.164  2007/09/01 22:40:17  jps
 * do_stat_room lists spells that are in effect.
 *
 * Revision 1.163  2007/09/01 21:22:29  jps
 * Made _mscript detection routines.
 *
 * Revision 1.162  2007/08/26 09:08:56  jps
 * Adjust long extra desc output. I was tired!
 *
 * Revision 1.161  2007/08/26 08:49:36  jps
 * Added commands estat, oestat, and restat, for viewing extra
 * descriptions on objects and rooms.
 *
 * Revision 1.160  2007/08/24 22:49:05  jps
 * Added "snum" and "tnum" commands.
 *
 * Revision 1.159  2007/08/24 22:10:58  jps
 * Add commands "slist" and "sstat".
 *
 * Revision 1.158  2007/08/24 19:16:12  myc
 * Fixed 'stat room' to accept a room number, ala 'stat room #'.
 *
 * Revision 1.157  2007/08/24 17:01:36  myc
 * Adding ostat and mstat commands as shorthand for vstat, rstat for stat
 * room, and mnum and onum for vnum.  Also adding rnum and znum with new
 * functionality.
 *
 * Revision 1.156  2007/08/24 10:24:16  jps
 * Added zlist command.
 *
 * Revision 1.155  2007/08/22 18:00:11  jps
 * Keep track of the reason the mud is restricted using restrict_reason.
 * Autoboot usage only shows the things you are allowed to do based
 * on your level.
 *
 * Revision 1.154  2007/08/22 04:21:03  jps
 * Adjust text of reboot-info message.
 *
 * Revision 1.153  2007/08/20 21:24:42  jps
 * Make "vstat obj" verbosely list object extra descs.
 *
 * Revision 1.152  2007/08/17 02:23:36  jps
 * Generalized some autoboot code, fixed an off-by-one error in the
 * reboot warning time, and made use of restrict_manual, which will
 * control whether autoboot code will remove a login restriction.
 *
 * Revision 1.151  2007/08/16 12:51:09  jps
 * Added "postpone" and "warntime" to autoboot.
 *
 * Revision 1.150  2007/08/15 20:48:41  myc
 * The vitem command interprets spellnums for wands and staves correctly now.
 *
 * Revision 1.149  2007/08/14 22:43:07  myc
 * Adding cornering and cornered_by to stat char.
 *
 * Revision 1.148  2007/08/14 20:13:45  jps
 * Added command "autoboot" to manage the mud's automatic rebooting.
 *
 * Revision 1.147  2007/08/14 10:44:40  jps
 * Add perform_restore and perform_pain, which are called from do_restore,
 * do_rrestore, do_pain, or do_rpain as appropriate. Thus, code replication
 * is reduced AND you only have to change ONE function to make do_restore
 * and do_rrestore remove a certain affliction. Such as laryngitis, which
 * by the way restore and rrestore will remove.
 *
 * Revision 1.146  2007/08/08 20:09:42  jps
 * tlist, mlist, olist, and rlist now accept a single parameter which they
 * take to be a zone whose objects should be listed.  If no parameter is
 * given, they will list objects in the current zone - where the character
 * issuing the command is located.
 *
 * Revision 1.145  2007/08/05 20:21:51  myc
 * Made return command say Huh?!? instead of nothing.
 *
 * Revision 1.144  2007/08/05 01:49:45  myc
 * Gods could de-advance other gods who were higher level.
 *
 * Revision 1.143  2007/08/04 03:24:30  myc
 * dc now accepts character names as parameters.
 *
 * Revision 1.142  2007/08/03 22:00:11  myc
 * Fixed several \r\n typos in send_to_chars.
 *
 * Revision 1.141  2007/07/31 23:03:58  jps
 * Add command "zstat" to stat a zone.
 *
 * Revision 1.140  2007/07/19 20:37:17  jps
 * Print any action desc in do_stat_object.
 *
 * Revision 1.139  2007/07/15 18:01:32  jps
 * Created macro IS_POISONED.
 *
 * Revision 1.138  2007/07/15 17:40:22  myc
 * Fixed memory corruption crash bug in vitem and vwear commands.
 *
 * Revision 1.137  2007/07/14 05:26:25  myc
 * Players no longer lose all their hard-earned play time when de-advanced.
 * The last check-in for this mixed up ch and victim.
 *
 * Revision 1.136  2007/06/04 22:24:41  jps
 * Add game-toggle for name approval pause and set name approval to default on.
 *
 * Revision 1.135  2007/05/29 00:36:03  jps
 * Make a utility function find_zone, to find a zone's entry in the zone
 * table, from a vnum.
 *
 * Revision 1.134  2007/05/28 22:34:21  jps
 * Remove unused variable that was causing a warning.
 *
 * Revision 1.133  2007/05/28 20:08:10  jps
 * Include the year in dates in 'show player'.
 *
 * Revision 1.132  2007/05/21 02:18:24  myc
 * Stat room shows hidden door commands too now.
 *
 * Revision 1.131  2007/04/18 00:11:12  myc
 * Stat room now shows zone commands in that room.
 *
 * Revision 1.130  2007/04/17 23:59:16  myc
 * New trigger type: Load.  It goes off any time a mobile is loaded, whether
 * it be god command, zone command, or trigger command.
 *
 * Revision 1.129  2007/04/15 10:50:24  jps
 * Change 'set title' to LVL_GOD.
 *
 * Revision 1.128  2007/04/15 04:12:47  jps
 * Fix bug in stat spellbook.
 *
 * Revision 1.127  2007/04/15 03:55:08  jps
 * do_stat_object now gives information about spells in spellbooks.
 *
 * Revision 1.126  2007/04/04 13:31:02  jps
 * Add year to log timestamps and other dates.
 *
 * Revision 1.125  2007/02/08 01:30:00  myc
 * Players no longer lose all their hard-earned play time when de-advanced.
 *
 * Revision 1.124  2006/11/23 01:38:27  jps
 * vitem will now display the spells in magical casting objects
 *
 * Revision 1.123  2006/11/18 04:26:32  jps
 * Renamed continual light spell to illumination, and it only works on
 * LIGHT items (still rooms too).
 *
 * Revision 1.122  2006/11/16 18:42:45  jps
 * Awareness of new surroundings when magically tranported is related to
 * being asleep, blindness, etc.
 *
 * Revision 1.121  2006/11/13 19:08:49  jps
 * make dig mark the correct zones as needing saving
 *
 * Revision 1.120  2006/11/13 15:54:22  jps
 * Fix widespread misuse of the hide_invisible parameter to act().
 *
 * Revision 1.119  2006/11/12 02:31:01  jps
 * You become unmounted when magically moved to another room.
 *
 * Revision 1.118  2006/11/12 01:47:11  jps
 * restore and rrestore will fix hunger and thirst
 *
 * Revision 1.117  2006/06/24 18:34:22  rsd
 * zzur can change passwords
 *
 * Revision 1.116  2006/05/08 15:55:08  cjd
 * fixed misplaced bracket that was causing pain crash with no arg
 *
 * Revision 1.115  2006/05/03 01:27:30  rsd
 * altered how iptables works to use a user defined chain
 * just for the mud so the muds process doesn't have to
 * see or deal with the main chain.
 *
 * Revision 1.114  2006/04/09 22:37:31  rls
 * Updated poof to return current poof on no argument
 *
 * Revision 1.113  2006/04/09 01:00:00  rls
 * Adjusted set command structure.
 * @
 *
 * Revision 1.112  2005/12/31 21:26:04  dce
 * *** empty log message ***
 *
 * Revision 1.111  2005/12/31 20:39:22  dce
 * *** empty log message ***
 *
 * Revision 1.110  2005/07/27 03:07:08  jwk
 * Changed level for setting wiztitle from 104 to 103
 *
 * Revision 1.109  2005/07/13 21:37:01  cjd
 * changed pain and rpain to from direct edit to use alter's
 * to allow chars to regen. which they weren't before.
 *
 * Revision 1.108  2005/06/26 03:39:37  cjd
 * stupid typo's, fixed rpain bug where
 * chars mv was going to 0
 *
 * Revision 1.107  2005/06/26 03:14:17  cjd
 * created pain and rpain commands as the oppsoite of restore
 *
 * Revision 1.106  2004/11/11 23:12:48  rsd
 * Made the mushroom clouds associated with rebooting
 * into 2 send_to_all()'s to reduce the string size
 * to be less than 509 bytes.
 *
 * Revision 1.105  2004/11/11 20:11:09  cmc
 * commented out reference to title_type titles
 * compiler warning now deceased
 *
 * Revision 1.104  2004/11/01 04:12:01  jjl
 * Changing iptables so it can be run on multiple muds concurrently.
 *
 * Revision 1.103  2004/11/01 03:46:22  jjl
 * Buffed up iptables adding add/list/delete command
 *
 * Revision 1.102  2004/11/01 01:14:26  rsd
 * Ok, added a bit more functionality to the iptables command
 * and added some very basic input checking, it needs more.
 *
 * Revision 1.101  2004/10/31 20:38:11  jjl
 * Adding iptables command
 * for zzur to hack on
 *
 * Revision 1.100  2003/06/25 02:57:15  jjl
 * Fixed a DUMB typo
 *
 * Revision 1.99  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.98  2002/11/30 19:39:38  jjl
 * Added the ability for GAME commands to have integer values, and added a
 * GROUPING game command, that allows you to set a maximum level difference
 * between group masters and potential groupees..
 *
 * Revision 1.97  2002/09/14 00:12:50  dce
 * Put show shops to level 101.
 *
 * Revision 1.96  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.95  2002/07/02 12:51:57  rls
 * forgot the shutdown for maintenence 'shroom... *wink*
 *
 * Revision 1.94  2002/07/02 06:46:24  rls
 * Colorized Mushroom (YAY!)
 *
 * Revision 1.93  2002/06/21 04:32:15  rls
 * Adjusted rrestore to not take away excess hit, mana or move
 * gained from such spells as energy drain, etc.
 *
 * Revision 1.92  2002/06/07 01:14:26  rls
 * Commented out the emote case for Zzur
 *
 * Revision 1.91  2002/05/23 00:19:41  rls
 * Added set size functionality and fixed set charisma for 100pts
 *
 * Revision 1.90  2002/03/30 19:20:14  dce
 * Added damage amounts as a toggable item.
 *
 * Revision 1.89  2002/03/15 02:45:14  dce
 * Increased wiztitle from 11 to 12.
 *
 * Revision 1.88  2002/02/25 09:48:42  rls
 * Updated wiztitle function per request to format for 11 characters
 *
 * Revision 1.87  2001/10/22 01:35:58  dce
 * Changedset passwd to a level 104 command.
 *
 * Revision 1.86  2001/04/08 17:23:46  dce
 * Show file was overwritting the buffer, so I increased
 * the size by using a new buffer.
 *
 * Revision 1.85  2001/04/07 17:02:30  dce
 * Added the vitem command.
 *
 * Revision 1.84  2001/04/02 03:18:41  dce
 * Added the command vwear.
 *
 * Revision 1.83  2001/03/31 22:17:56  dce
 * Tried to increase zonebuf larger than MAX_STIRNG_LENGTH
 * I am curious if this will crash the mud. (show zones).
 *
 * Revision 1.82  2001/03/28 02:08:26  dce
 * Show file works as advertised.
 *
 * Revision 1.81  2001/03/26 02:17:18  dce
 * Show file now goes beyond one page.
 *
 * Revision 1.80  2001/03/24 19:42:27  dce
 * Stating an object will show the level.
 *
 * Revision 1.79  2001/03/10 18:45:33  dce
 * Changed do_return function to pass a subcommand of 1.
 * This way I can make it so players can't use the return command.
 *
 * Revision 1.78  2001/03/07 03:08:09  dce
 * Fixed the output.
 *
 * Revision 1.77  2001/03/04 14:27:39  dce
 * Added the page function to show zones.
 *
 * Revision 1.76  2001/03/03 18:07:10  dce
 * Gods should not be able to use return from a shapechange.
 * Also mortals can no longer use return.
 *
 * Revision 1.75  2001/01/29 05:05:30  rsd
 * Ok, aded a couple of include to call mkdir so Jimmy and
 * I could redo pfile maint to clean up bogus files that
 * weren't being accounted for in the way pfilemaint was
 * being done.
 *
 * Revision 1.74  2000/11/28 00:46:32  mtp
 * remove mobprog stuff
 *
 * Revision 1.73  2000/11/25 08:12:06  rsd
 * Altered some of do_show() to comment why the case:zones
 * can't be put through the paging function.
 *
 * Revision 1.72  2000/11/20 05:02:57  rsd
 * added back rlog messages from before the addition of the
 * $log$ string.
 * grumble
 *
 * Revision 1.71  2000/11/16 02:43:00  rsd
 * added a check in do_name() nlist so if players names have
 * been declined they won't show up in nlist among the players
 * who are awaiting approval.
 *
 * Revision 1.70  2000/11/11 00:04:36  mtp
 * continue processing quest list even if a bad one was found
 *
 * Revision 1.69  2000/11/03 17:28:33  jimmy
 * Added better checks for real_room to stop players/objs from
 * being placed in room NOWHERE.  This should help pinpoint any
 * weirdness.
 *
 * Revision 1.68  2000/11/03 05:37:17  jimmy
 * Removed the quest.h file from structs.h arg!! and placed it
 * only in the appropriate files
 * Updated the dependancies in the Makefile and created
 * make supahclean.
 *
 * Revision 1.67  2000/11/02 23:41:19  mtp
 * added qid_num for debugging purposes
 *
 * Revision 1.66  2000/11/02 23:38:09  mtp
 * real_quest() for non existant virtual quest returns -1!
 *
 * Revision 1.65  2000/11/01 00:44:35  mtp
 * added extra check to avoid crashing mud...dunno why quests is not being nulled though
 *
 * Revision 1.64  2000/10/27 00:34:45  mtp
 * modeified do_stat to show quest info
 *
 * Revision 1.63  2000/10/13 04:27:23  cmc
 * added "level gain" toggle
 * typo correction duties
 *
 * Revision 1.62  2000/09/14 00:54:08  rsd
 * Fixed purge again, put proper logic not to return out
 * of the function in it.
 *
 * Revision 1.61  2000/09/13 22:20:43  rsd
 * altered the level at which set sub commands are avail
 *
 * Revision 1.60  2000/09/13 21:06:28  rsd
 * Fixed it so object without any wear flags, notably TAKE
 * , are not purged unless it's an argument.  So boards and
 * fountains etc won't be purged on a room purge.
 * Like the old Fiery.  Also
 * In do_players added the players level in (parens) so it's
 * easier to tell if anyone has advanced and needs name approval
 * just some admin stuffs.
 *
 * Revision 1.59  2000/08/31 17:45:35  rsd
 * Ok, more passwd changes
 *
 * Revision 1.58  2000/04/26 23:33:56  rsd
 * made name declination a little more obvious to the players
 * Also took some lame text out of the terminate command
 *
 * Revision 1.57  2000/04/22 22:31:41  rsd
 * fixed spelling of deity in player output.
 *
 * Revision 1.56  2000/04/14 19:09:35  rsd
 * passwd changes
 *
 * Revision 1.55  2000/04/14 18:56:15  rsd
 * OK I can definately use it now
 *
 * Revision 1.54  2000/04/14 18:52:26  rsd
 * I moved passwd to my level I swear
 *
 * Revision 1.53  2000/04/14 18:48:34  rsd
 * made passwd my level
 *
 * Revision 1.52  2000/02/25 02:42:00  rsd
 * added room and zone info to do_world, I even made a
 * variable for it woo.
 * /s
 *
 * Revision 1.51  2000/02/24 01:04:18  dce
 * Changed wiztitle from a command to a set.
 *
 * Revision 1.50  2000/02/16 07:58:39  mtp
 * added listrace...it just prints the choice menu that people have when they start
 *
 * Revision 1.49  2000/02/16 07:16:12  rsd
 * Aredryk fixed show zones (all) to fix overflow so the
 * list wouldn't truncate and stomp memory
 *
 * Revision 1.48  2000/01/31 03:58:41  cso
 * modified do_game to allow different game toggles to have different
 * min levels. it also looked ugly and kludgy, so i recoded it from scratch
 *
 * Revision 1.47  2000/01/30 23:11:18  rsd
 * added parts to allow a good races toggle in
 * addition to the existing complete race login toggle.
 *
 * Revision 1.46  1999/11/28 22:49:49  cso
 * do_stat_object: made stat on corpse reflect whether raisable or not
 * do_wizutil: unaffecting animated mob kills it
 *
 * Revision 1.45  1999/10/30 15:21:06  rsd
 * removed superflous void gain_exp(struct... definition
 * redundancy isn't needed.
 *
 * Revision 1.44  1999/10/04 21:39:18  rsd
 * Made holylight a set as well as a toggle, we have a mortal
 * who got it set on in a rez bug.
 *
 * Revision 1.43  1999/09/16 01:15:11  dce
 * Weight restrictions for monks...-hitroll, -damroll + ac
 *
 * Revision 1.42  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.41  1999/08/29 07:06:04  jimmy
 * Many many small but ver significant bug fixes found using insure.  The
 * code now compiles cleanly and boots cleanly with insure.  The most significant
 * changes were moving all the BREATH's to within normal spell range, and
 * fixing the way socials were allocated.  Too many small fixes to list them
 * all. --gurlaek (now for the runtime debugging :( )
 *
 * Revision 1.40  1999/08/18 22:39:24  mtp
 * fixed afk bug in ptell
 *
 * Revision 1.39  1999/08/12 21:01:00  mud
 * Added the build number and compiled date functionality
 * to the world command. Reformatted the game command to add
 * spanky colors with the keyword toggles in addition to the
 * lines that indicate what state each toggle is in.
 *
 * Revision 1.38  1999/08/12 17:54:46  dce
 * Fixed experience so that there are no overflows of integers that are placed into longs.
 * Main problem was max_exp_gain and max_exp_loss. Both were overflowing due to poor
 * Hubis coding.
 *
 * Revision 1.37  1999/08/12 06:15:03  mtp
 * Added AFK message for ptell
 *
 * Revision 1.36  1999/08/12 04:25:39  jimmy
 * This is a Mass ci of the new pfile system.  The pfile has been split into
 * one file for each player in a directory A-Z.  The object files are also
 * located in the A-Z directories.  Fixed a stupid bug in pfilemaint that
 * screwed up the IDNUM of the person who typed it.  Commented out the frag
 * system completely.  It is slated for removal.  Fixed the rename command.
 * Fixed all supporting functions for the new system, I hope!
 * --Gurlaek 8/11/1999
 *
 * Revision 1.35  1999/07/29 20:19:35  jimmy
 * Added an idle time deletion algorithm to pfilemaint.  Players will now
 * be autodeleted after 14+((level - 1) * 3) days of idle time.
 * --gurlaek
 *
 * Revision 1.34  1999/07/26 00:20:59  mud
 * altered the priority of red and yellow color in do_players
 * so that players who are set newname on will take higher
 * precedence over those with napprove on.
 *
 * Revision 1.33  1999/07/24 23:27:13  jimmy
 * Yet another fix to pfilemaint.  Pfilemaint now destroys the old
 * player_table and rebuilds it on the fly.  NO more wierdness.
 *
 * --Gurlaek
 *
 * Revision 1.32  1999/07/23 23:37:38  jimmy
 * Fixed pfilemaint to do more sanity checking before deletions.
 * Only deletions are now logged and each is logged with a reason.
 * removed the send_toxnames from do_set "rename".  This should ahve
 * been in the interpreter all along and that's where i moved it from
 * in the first place.
 * --gurlaek
 *
 * Revision 1.31  1999/07/22 17:43:59  jimmy
 * Wrote a new command to do player file maintenace called pfilemaint.
 * This command will purge the player file using the same criteria
 * for creating new playes.  So, if it's in the xnames, it gets waxed
 * from the player file.  This will also take care of blank records
 * and reserved words.
 * --gurlaek
 *
 * Revision 1.30  1999/07/10 15:41:23  mud
 * added SLOWNS to the game toggles.
 *
 * Revision 1.29  1999/07/10 05:30:49  mud
 * made OOC a game sumcommand to prepare it to be removed
 * from the toggle subcommand list. And it works! wooo!
 *
 * Revision 1.28  1999/07/10 03:13:30  mud
 * Ok, I think I fixed the date command, I'm not very confident
 * that it works perfectly and someone should probably double
 * check my work.
 *
 * Revision 1.27  1999/07/07 22:53:38  mud
 * added the guts of the new do_world command, basically
 * stole the date subcommands and took out the logic splitting
 * them into two commands.
 *
 * Revision 1.26  1999/07/07 22:07:12  mud
 * Changed the do_world command to do game and altered the format
 * and color.
 *
 * Revision 1.25  1999/07/06 19:57:05  jimmy
 * This is a Mass check-in of the new skill/spell/language assignment system.
 * This New system combines the assignment of skill/spell/language for
 * both mobs and PCs.  LOts of code was touched and many errors were fixed.
 * MCLASS_VOID was moved from 13 to -1 to match CLASS_UNDEFINED for PC's.
 * MObs now get random skill/spell/language levels baseed on their race/class/level
 * that exactly align with PC's.  PC's no longer have to rent to use skills gained
 * by leveling or when first creating a char.  Languages no longer reset to defaults
 * when a PC levels.  Discovered that languages have been defined right in the middle
 * of the spell area.  This needs to be fixed.  A conversion util neeDs to be run on
 * the mob files to compensate for the 13 to -1 class change.
 * --gurlaek 7/6/1999
 *
 * Revision 1.24  1999/06/30 18:11:09  jimmy
 * act.offensive.c    config.c      handler.c    spells.c
 * his is a major conversion from the 18 point attribute system to the
 * 100 point attribute system.  A few of the major changes are:
 * All attributes are now on a scale from 0-100
 * Everyone views attribs the same but, the attribs for one race
 *   may be differeent for that of another even if they are the
 *   same number.
 * Mobs attribs now get rolled and scaled using the same algorithim as PC's
 * Mobs now have individual random attributes based on race/class.
 * The STR_ADD attrib has been completely removed.
 * All bonus tables for attribs in constants.c have been replaced by
 *   algorithims that closely duplicate the tables except on a 100 scale.
 * Some minor changes:
 * Race selection at char creation can now be toggled by using
 *   <world races off>
 * Lots of cleanup done to affected areas of code.
 * Setting attributes for mobs in the .mob file no longer functions
 *   but is still in the code for later use.
 * We now have a spare attribut structure in the pfile because the new
 *   system only used three instead of four.
 * --gurlaek 6/30/1999
 *
 * Revision 1.23  1999/06/22 18:23:36  jimmy
 * Added more checks for d->character in ndecline and naccept to stop
 * crashes when people were at CON_QANSI etc etc and ndecline or naccept
 * were typed.
 * gurlaek
 *
 * Revision 1.22  1999/06/18 21:56:51  mud
 * Added code to make set file renames write to xnames file
 * so those players who are denied and aren't online will be
 * written to xnames
 * I'm the man
 *
 * Revision 1.21  1999/06/18 17:25:42  jimmy
 * added check for d->character in the nlist (do_name) command.  This
 * fixed the problem of crashes when people where at CON_QANSI or
 * anywhere before their d->character's where created and someone typed
 * nlist.  --gurlaek.
 *
 * Revision 1.20  1999/06/11 17:58:10  jimmy
 * tweaked do_players to better utilize memory. --gurlaek
 *
 * Revision 1.19  1999/06/10 21:47:42  mud
 * Added color code to do_players I even freed my memory.
 *
 * Revision 1.18  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.17  1999/05/04 18:03:09  dce
 * Offer fixed
 *
 * Revision 1.16  1999/05/04 17:19:33  dce
 * Name accept system...version one...original code by Fingh, fixed up to work
 * by Zantir.
 *
 * Revision 1.15  1999/05/01 03:15:33  dce
 * Set can now toggle anonymous flag
 *
 * Revision 1.14  1999/04/20 22:40:38  jimmy
 * Fixed a silly overflow in do_players.  Once the length of all the
 * player's names exceeded the bufffer length boom!  Now you must type a
 * letter and only player's names beginning with that letter are displayed.
 * --Gurlaek
 *
 * Revision 1.13  1999/03/13 19:42:28  jimmy
 * fixed silly crashbug in show error
 * fingon
 *
 * Revision 1.12  1999/02/26 22:30:30  dce
 * Fixes file command to work in show command
 *
 * Revision 1.11  1999/02/23 16:48:06  dce
 * Creates a new command called file. Allows us to view files
 * through the mud.
 *
 * Revision 1.10  1999/02/19 01:58:44  dce
 * Changes wiztitle to max text length of 10
 *
 * Revision 1.9  1999/02/12 16:25:09  jimmy
 * Fixed poofs to show a default poof if none is set.
 *
 * Revision 1.8  1999/02/10 22:21:42  jimmy
 * Added do_wiztitle that allows gods to edit their
 * godly title ie Overlord.  Also added this title
 * to the playerfile
 * fingon
 *
 * Revision 1.7  1999/02/06 17:49:29  jimmy
 * Gods can now type goto home to get to
 * their homerooms.
 *
 * Revision 1.6  1999/02/06 00:40:36  jimmy
 * Major change to incorporate aliases into the pfile
 * moved alias structure from interpreter.h to structs.h
 * heavily modified alias code in interpreter.c
 * Jimmy Kincaid AKA fingon
 *
 * Revision 1.5  1999/02/05 07:47:42  jimmy
 * Added Poofs to the playerfile as well as 4 extra strings for
 * future use.  fingon
 *
 * Revision 1.4  1999/02/02 02:41:42  mud
 * dos2unix
 * Replaced Occurences of Hubis with Fiery
 *
 * Revision 1.3  1999/02/01 08:12:46  jimmy
 * Fixed show god --Fingon
 *
 * Revision 1.2  1999/01/31 19:26:45  jimmy
 * Fixed "show death" --fingon
 *
 * Revision 1.1
 * Initial Revision
 *
 ***************************************************************************/

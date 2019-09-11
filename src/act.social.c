/***************************************************************************
 * $Id: act.social.c,v 1.12 2009/03/09 03:25:36 jps Exp $
 ***************************************************************************/
/**************************************************************************
*   File: act.social.c                                   Part of FieryMUD *
*  Usage: Functions to handle socials                                     *
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

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "math.h"

/* extern variables */

/* extern functions */
char *fread_action(FILE * fl, int nr);

/* local globals */
static int list_top = -1;

/* gawd this is a hack, someone please fix it! */
#define MAX_SOCIALS 200

struct social_messg {
  int act_nr;
  int hide;
  int min_victim_position;	/* Position of victim */

  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;		/* if NULL, read no further, ignore args */
  char *others_found;
  char *vict_found;

  /* An argument was there, but no victim was found */
  char *not_found;

  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;
} *soc_mess_list[MAX_SOCIALS];



int find_action(int cmd)
{
  int bot, top, mid;

  bot = 0;
  top = list_top;

  if (top < 0)
    return (-1);

  for (;;) {
    mid = (bot + top) >> 1;

    if (soc_mess_list[mid]->act_nr == cmd)
      return (mid);
    if (bot >= top)
      return (-1);

    if (soc_mess_list[mid]->act_nr > cmd)
      top = --mid;
    else
      bot = ++mid;
  }
}



ACMD(do_action)
{
  int act_nr;
  struct social_messg *action;
  struct char_data *vict;

  if ((act_nr = find_action(cmd)) < 0) {
    send_to_char("That action is not supported.\r\n", ch);
    return;
  }
  action = soc_mess_list[act_nr];

  if (action->char_found)
    one_argument(argument, buf);
  else
    *buf = '\0';

  if (!*buf) {
    send_to_char(action->char_no_arg, ch);
    send_to_char("\r\n", ch);
    act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
    return;
  }
  if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, buf)))) {
    send_to_char(action->not_found, ch);
    send_to_char("\r\n", ch);
  } else if (vict == ch) {
    send_to_char(action->char_auto, ch);
    send_to_char("\r\n", ch);
    act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
  } else {
    if (GET_POS(vict) < action->min_victim_position)
      act("$N is not in a proper position for that.",
	  FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else {
      act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
      act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
      act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
    }
  }
}



ACMD(do_insult)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (*arg) {
    if (!(victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
      send_to_char("Can't hear you!\r\n", ch);
    else {
      if (victim != ch) {
	sprintf(buf, "You insult %s.\r\n", GET_NAME(victim));
	send_to_char(buf, ch);

	switch (number(0, 2)) {
	case 0:
	  if (GET_SEX(ch) == SEX_MALE) {
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT);
	  } else {		/* Ch == Woman */
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of having the smallest... (brain?)",
		  FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n tells you that you'd lose a beauty contest against a troll.",
		  FALSE, ch, 0, victim, TO_VICT);
	  }
	  break;
	case 1:
	  act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	default:
	  act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	}			/* end switch */

	act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
      } else {			/* ch == victim */
	send_to_char("You feel insulted.\r\n", ch);
      }
    }
  } else
    send_to_char("I'm sure you don't want to insult *everybody*...\r\n", ch);
}


char *fread_action(FILE * fl, int nr)
{
  char buf[MAX_STRING_LENGTH], *rslt;

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl)) {
    fprintf(stderr, "fread_action - unexpected EOF near action #%d", nr);
    exit(1);
  }
  if (*buf == '#')
    return (NULL);
  else {
    *(buf + strlen(buf) - 1) = '\0';
    CREATE(rslt, char, strlen(buf) + 1);
    strcpy(rslt, buf);
    return (rslt);
  }
}


void boot_social_messages(void)
{
  FILE *fl;
  int nr, i, hide, min_pos, curr_soc = -1;
  char next_soc[100];
  struct social_messg *temp;

  /* open social file */
  if (!(fl = fopen(SOCMESS_FILE, "r"))) {
    sprintf(buf, "Can't open socials file '%s'", SOCMESS_FILE);
    perror(buf);
    exit(1);
  }
  /* count socials & allocate space */
  for (nr = 0; *cmd_info[nr].command != '\n'; nr++)
    if (cmd_info[nr].command_pointer == do_action) {
      list_top++;
    }

  /* now read 'em */
  for (;;) {
    fscanf(fl, " %s ", next_soc);
    if (*next_soc == '$')
      break;
    if ((nr = find_command(next_soc)) < 0) {
      sprintf(buf, "Unknown social '%s' in social file", next_soc);
      log(buf);
    }
    if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) {
      fprintf(stderr, "Format error in social file near social '%s'\n",
	      next_soc);
      exit(1);
    }
    /* read the stuff */
    curr_soc++;
    CREATE(soc_mess_list[curr_soc], struct social_messg, 1);
    soc_mess_list[curr_soc]->act_nr = nr;
    soc_mess_list[curr_soc]->hide = hide;
    soc_mess_list[curr_soc]->min_victim_position = min_pos;

    soc_mess_list[curr_soc]->char_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc]->others_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc]->char_found = fread_action(fl, nr);

    /* if no char_found, the rest is to be ignored */
    if (!soc_mess_list[curr_soc]->char_found)
      continue;

    soc_mess_list[curr_soc]->others_found = fread_action(fl, nr);
    soc_mess_list[curr_soc]->vict_found = fread_action(fl, nr);
    soc_mess_list[curr_soc]->not_found = fread_action(fl, nr);
    soc_mess_list[curr_soc]->char_auto = fread_action(fl, nr);
    soc_mess_list[curr_soc]->others_auto = fread_action(fl, nr);
  }

  /* close file & set top */
  fclose(fl);
  list_top = curr_soc;

  /* now, sort 'em */
  for (curr_soc = 0; curr_soc < list_top; curr_soc++) {
    min_pos = curr_soc;
    for (i = curr_soc + 1; i <= list_top; i++)
      if (soc_mess_list[i]->act_nr < soc_mess_list[min_pos]->act_nr)
	min_pos = i;
    if (curr_soc != min_pos) {
      temp = soc_mess_list[curr_soc];
      soc_mess_list[curr_soc] = soc_mess_list[min_pos];
      soc_mess_list[min_pos] = temp;
    }
  }
}

void free_action(struct social_messg *mess)  {
  if (mess->char_no_arg) free(mess->char_no_arg);
  if (mess->others_no_arg) free(mess->others_no_arg);
  if (mess->char_found) free(mess->char_found);
  if (mess->others_found) free(mess->others_found);
  if (mess->vict_found) free(mess->vict_found);
  if (mess->not_found) free(mess->not_found);
  if (mess->char_auto) free(mess->char_auto);
  if (mess->others_auto) free(mess->others_auto);
  memset(mess, 0, sizeof(struct social_messg));
}

void free_social_messages() {
  struct social_messg *mess;
  int i;

  for (i = 0; i <= list_top; i++)  {
    mess = soc_mess_list[i];
    free_action(mess);
    free(mess);
  }
}


/***************************************************************************
 * $Log: act.social.c,v $
 * Revision 1.12  2009/03/09 03:25:36  jps
 * Remove inclusion of casting.h
 *
 * Revision 1.11  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.10  2009/03/03 19:41:50  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.9  2008/02/16 20:26:04  myc
 * Adding functions to free socials at program termination.
 *
 * Revision 1.8  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.7  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.6  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.5  2000/11/20 04:25:18  rsd
 * added back rlog messages from prior to the addition of
 * the $log$ string.
 *
 * Revision 1.4  2000/04/02 02:37:19  rsd
 * changed the comment header while I was browsing the file for
 * information.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/08/29 07:06:04  jimmy
 * Many many small but ver significant bug fixes found using insure.  The
 * code now compiles cleanly and boots cleanly with insure.  The most significant
 * changes were moving all the BREATH's to within normal spell range, and
 * fixing the way socials were allocated.  Too many small fixes to list them
 * all. --gurlaek (now for the runtime debugging :( )
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

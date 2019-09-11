/***************************************************************************
 * $Id: prefs.c,v 1.1 2010/06/09 22:32:01 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: prefs.c                                        Part of FieryMUD *
 *  Usage: Player preferences management and commands                      *
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
#include "screen.h"
#include "clan.h"

#define TOG_OFF 0
#define TOG_ON  1

ACMD(do_toggle)
{
  int i = 0, column, wimp_lev, page_length;
  bool set;
  long result = 0;
  struct char_data *tch;

  struct set_struct
  {
    char *cmd;
    int level;
    int bitvector;
  }

  /**********************
    These match SCMD defines in interpreter.h, to add or
    remove a toggle you must also update the SCMD's in
    interpreter.h.
  *********************/

  fields[] = {
    { "NoSummon",   LVL_IMMORT, PRF_SUMMONABLE },
    { "NoHassle",   LVL_IMMORT, PRF_NOHASSLE },
    { "Brief",      0,          PRF_BRIEF },
    { "Compact",    0,          PRF_COMPACT },
    { "NoTell",     0,          PRF_NOTELL },
    { "AFK",        0,          PRF_AFK },
    { "NoShout",    0,          PRF_DEAF },
    { "NoGossip",   0,          PRF_NOGOSS },
    { "NoHints",    0,          PRF_NOHINTS },
    { "NoWiznet",   LVL_IMMORT, PRF_NOWIZ },
    { "Quest",      LVL_IMMORT, PRF_QUEST },
    { "RoomFlags",  LVL_IMMORT, PRF_ROOMFLAGS },
    { "NoRepeat",   0,          PRF_NOREPEAT },
    { "Holylight",  LVL_IMMORT, PRF_HOLYLIGHT },
    { "Autoexit",   0,          PRF_AUTOEXIT },
    { "NoPetition", LVL_IMMORT, PRF_NOPETI },
    { "AutoSplit",  0,          PRF_AUTOSPLIT },
    { "Anonymous",  50,         PRF_ANON },
    { "ShowVnums",  LVL_IMMORT, PRF_SHOWVNUMS },
    { "Wimpy",      0,          0 },
    { "NiceArea",   0,          PRF_NICEAREA },
    { "Vicious",    0,          PRF_VICIOUS },
    { "Passive",    0,          PRF_PASSIVE },
    { "PageLength", 0,          0 },
    { "NoFollow",   0,          PRF_NOFOLLOW },
    { "RoomVis",    LVL_IMMORT, PRF_ROOMVIS },
    { "NoClanComm", 0,          PRF_NOCLANCOMM },
    { "OLCComm",    LVL_IMMORT, PRF_OLCCOMM },
    { "LineNums",   0,          PRF_LINENUMS },
    { "AutoLoot",   0,          PRF_AUTOLOOT },
    { "AutoTreas",  0,          PRF_AUTOTREAS },
    { "AutoInvis",  LVL_IMMORT, 0 },
    { "ExpandObjs", 0,          PRF_EXPAND_OBJS },
    { "ExpandMobs", 0,          PRF_EXPAND_MOBS },
    { "Sacrificial",LVL_IMMORT, PRF_SACRIFICIAL },
    { "\n",         0,          0 },
    /* If you add another toggle, add a corresponding SCMD_ define in
     * interpreter.h, even if you don't intend to use it. */

  };
  char *tog_messages[][2] =
  {
        /*00*/ {"You are now safe from summoning by other players.\r\n",
                 "You may now be summoned by other players.\r\n"},
        /*01*/ {"Nohassle disabled, mobs will attack you now.\r\n",
                 "Nohassle enabled, mobs will leave you alone now.\r\n"},
        /*02*/ {"Brief mode off.\r\n",
                 "Brief mode on.\r\n"},
        /*03*/ {"Compact mode off.\r\n",
                 "Compact mode on.\r\n"},
        /*04*/ {"You can now hear tells.\r\n",
                 "You are now deaf to tells.\r\n"},
        /*05*/ {"You are no longer AFK.\r\n",
                 "You are now AFK.\r\n"},
        /*06*/ {"You can now hear shouts.\r\n",
                 "You are now deaf to shouts.\r\n"},
        /*07*/ {"You can now hear gossip.\r\n",
                 "You are now deaf to gossip.\r\n"},
        /*08*/ {"You will receive hints when you enter typos.\r\n",
                 "You will not receive any hints when entering typos.\r\n"},
        /*09*/ {"You can now hear the Wiz-channel.\r\n",
                 "You are now deaf to the Wiz-channel.\r\n"},
        /*10*/ {"You are no longer part of the Quest.\r\n",
                 "Okay, you are part of the Quest!\r\n"},
        /*11*/ {"You will no longer see the room flags.\r\n",
                 "You will now see the room flags.\r\n"},
        /*12*/ {"You will now have your communication repeated.\r\n",
                 "You will no longer have your communication repeated.\r\n"},
        /*13*/ {"HolyLight mode off.\r\n",
                 "HolyLight mode on.\r\n"},
        /*14*/ {"Autoexits disabled.\r\n",
                 "Autoexits enabled.\r\n"},
        /*15*/ {"You will now receive petitions from mortals!\r\n",
                 "You are now deaf to petitions!\r\n"},
        /*16*/ {"You will no longer automatically split coins from corpses.\r\n",
                 "You will now automatically split coins from corpses!\r\n"},
        /*17*/ {"You are no longer anonymous.\r\n",
                 "You are now anonymous.\r\n"},
        /*18*/ {"You will no longer see vnums.\r\n",
                 "You will now see vnums.\r\n"},
        /*19*/ {NULL,
                 NULL},
        /*20*/ {"Your area spells will now hit your race align in towns.\r\n",
                 "Your area spells won't hit your race align in towns.\r\n"},
        /*21*/ {"You feel nice and no longer vicious.\r\n",
                 "You will now kill mortally wounded victims.\r\n"},
        /*22*/ {"You will now auto-engage upon being offensively cast upon.\r\n",
                 "You will no longer auto-engage upon being offensively cast upon.\r\n"},
        /*23*/ {NULL,
                 NULL},
        /*24*/ {"You will now let anyone follow you.\r\n",
                 "You will now avoid attracting new followers.\r\n"},
        /*25*/ {"You will now only be seen by people who might normally see you.\r\n",
                 "You will now be seen by anyone in the same room as you.\r\n"},
        /*26*/ {"You will now hear clan communication.\r\n",
                 "You will no longer hear clan communication.\r\n"},
        /*27*/ {"You will no longer hear communication while in OLC.\r\n",
                 "You will now hear communication while in OLC.\r\n"},
        /*28*/ {"Line numbers will not be displayed when entering the string editor.\r\n",
                 "Line numbers will be displayed when entering the string editor.\r\n"},
        /*29*/ {"You will no longer automatically loot items from corpses.\r\n",
                 "You will now automatically loot items from corpses.\r\n"},
        /*30*/ {"You will no longer automatically loot treasure from corpses.\r\n",
                 "You will now automatically loot treasure from corpses.\r\n"},
        /*31*/ {NULL,
                 NULL},
        /*32*/ {"Objects will now stack in lists.\r\n",
                "Objects will no longer stack in lists.\r\n"},
        /*33*/ {"Mobiles will now stack in lists.\r\n",
                "Mobiles will no longer stack in lists.\r\n"},
        /*34*/ {"When you cast sacrificial spells, they may now auto-target you.\r\n",
                "When you cast sacrificial spells, they will no longer auto-target you.\r\n"},
  };

  argument = one_argument(argument, arg);
  tch = REAL_CHAR(ch);

  /*
   * First, see if the player wants to toggle something.
   */
  if (*arg)
    for (; *fields[i].cmd != '\n'; ++i)
      if (is_abbrev(arg, fields[i].cmd))
        if (GET_LEVEL(tch) >= fields[i].level ||
            (i == SCMD_ANON && PRV_FLAGGED(tch, PRV_ANON_TOGGLE)))
          if (i != SCMD_NOCLANCOMM || GET_CLAN(tch))
            break;

  if (!*arg || *fields[i].cmd == '\n') {
    /* Show a player his/her fields. */

    if (*arg) {
      if (GET_LEVEL(ch) < LVL_GOD || !(tch = find_char_around_char(ch, find_vis_by_name(ch, arg)))) {
        send_to_char("Toggle what!?\r\n", ch);
        return;
      }
      /* Handle switched/shapechanged players. */
      tch = REAL_CHAR(tch);
    }

    if (IS_NPC(tch)) {
      act("$N is an NPC.  They don't have toggles!", FALSE, ch, 0, tch, TO_CHAR);
      return;
    }

    strcpy(buf, "             FieryMUD TOGGLES!  (See HELP TOGGLE)\r\n"
                "===============================================================\r\n");
    for (column = i = 0; *fields[i].cmd != '\n'; ++i) {
      if (i != SCMD_ANON || !PRV_FLAGGED(tch, PRV_ANON_TOGGLE))
        if (fields[i].level > GET_LEVEL(tch))
          continue;
      if (i == SCMD_NOCLANCOMM && !GET_CLAN(tch))
        continue;

      set = FALSE;
      switch (i) {
        case SCMD_WIMPY:
          if ((set = (1 && GET_WIMP_LEV(tch))))
            sprintf(buf2, "%d", GET_WIMP_LEV(tch));
          else
            strcpy(buf2, "NO");
          break;
        case SCMD_PAGELENGTH:
          sprintf(buf2, "%d", GET_PAGE_LENGTH(tch));
          set = GET_PAGE_LENGTH(tch) != 22;
          break;
        case SCMD_AUTOINVIS:
          if (GET_AUTOINVIS(tch) == -1) {
             strcpy(buf2, "NO");
          } else {
             set = TRUE;
             sprintf(buf2, "%d", GET_AUTOINVIS(tch));
          }
          break;
        default:
          set = 1 && PRF_FLAGGED(tch, fields[i].bitvector);
          strcpy(buf2, YESNO(set));
          break;
      }
      sprintf(buf, "%s %s%11s  %5s&0 %s", buf, set ? QHWHT : QWHT,
              fields[i].cmd, buf2, column == 2 ? "\r\n" : "| ");
      if (++column >= 3)
        column = 0;
    }
    if (column)
      strcat(buf, "\r\n");
    strcat(buf, "===============================================================\r\n");
    send_to_char(buf, ch);
    return;
  }

  if (IS_NPC(tch))
    return;

  switch (i) {
    case SCMD_WIMPY:
      one_argument(argument, arg);
      if (!*arg) {
        if (GET_WIMP_LEV(tch)) {
          sprintf(buf, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(tch));
          send_to_char(buf, ch);
        } else
          send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      }
      else if (isdigit(*arg)) {
        if ((wimp_lev = atoi(arg))) {
          if (wimp_lev < 0)
            send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
          else if (wimp_lev > GET_MAX_HIT(tch))
            send_to_char("That doesn't make much sense, now does it?\r\n", ch);
          else {
            sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n", wimp_lev);
            send_to_char(buf, tch);
            GET_WIMP_LEV(tch) = wimp_lev;
          }
        } else {
          send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
          GET_WIMP_LEV(tch) = 0;
        }
      } else
        send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);
      return;
    case SCMD_PAGELENGTH:
      one_argument(argument, arg);
      if (!*arg) {
        if (GET_PAGE_LENGTH(tch) > 0) {
          sprintf(buf, "Your current page length is %d.\r\n", GET_PAGE_LENGTH(tch));
          send_to_char(buf, ch);
        } else {
          send_to_char("Your page length is not valid.  Reset to 22.\r\n", ch);
          GET_PAGE_LENGTH(tch) = 22;
        }
      }
      else if (isdigit(*arg)) {
        if ((page_length = atoi(arg))) {
          if (page_length < 1)
            send_to_char("Invalid page length.\r\n", ch);
          else if (page_length > 50)
            send_to_char("Max page length is 50 right now.\r\n", ch);
          else {
            sprintf(buf, "Your new page length is %d lines.\r\n", page_length);
            send_to_char(buf, ch);
            GET_PAGE_LENGTH(tch) = page_length;
          }
        }
        else {
          send_to_char("Page length restored to default (22 lines).\r\n", ch);
          GET_PAGE_LENGTH(tch) = 22;
        }
      } else
        send_to_char("Specify at how many lines you want your page length to be.  (0 for default length)\r\n", ch);
      return;
    case SCMD_AUTOINVIS:
      any_one_arg(argument, arg);
      if (!*arg) {
         if (GET_AUTOINVIS(tch) == -1)
            send_to_char("Autoinvis is off.\r\n", ch);
         else if (GET_AUTOINVIS(tch) < -1 || GET_AUTOINVIS(tch) > GET_LEVEL(tch)) {
            sprintf(buf, "Your autoinvis is an invalid value: %d\r\n", GET_AUTOINVIS(tch));
            send_to_char(buf, ch);
         } else if (GET_AUTOINVIS(tch) == GET_LEVEL(tch)) {
            sprintf(buf, "Autoinvis is maxxed to &5&b%d&0.\r\n", GET_AUTOINVIS(tch));
            send_to_char(buf, ch);
         } else {
            sprintf(buf, "Autoinvis is set to &6&b%d&0.\r\n", GET_AUTOINVIS(tch));
            send_to_char(buf, ch);
         }
      } else {
         if (isdigit(*arg) || *arg == '-')
            i = atoi(arg);
         else if (!strncasecmp(arg, "off", 4))
            i = -1;
         else if (!strncasecmp(arg, "on", 3))
            i = GET_LEVEL(tch);
         else {
            send_to_char("Invalid input: autoinvis is a number between 0 and your level.\r\n", ch);
            send_to_char("You may also enter -1 or 'off' to disable it, or 'on' which will\r\n", ch);
            send_to_char("set it to the maximum value: your level.\r\n", ch);
            return;
         }

         if (GET_AUTOINVIS(tch) == i) {
            if (i == -1)
               send_to_char("Your autoinvis is already off.\r\n", ch);
            else if (i == GET_LEVEL(tch)) {
               sprintf(buf, "Your autoinvis is already maxxed to %d.\r\n", GET_AUTOINVIS(tch));
               send_to_char(buf, ch);
            } else {
               sprintf(buf, "Your autoinvis is already %d.\r\n", i);
               send_to_char(buf, ch);
            }
         } else if (i < -1 || i > GET_LEVEL(tch)) {
            send_to_char("Invalid input: autoinvis is a number between -1 and your level.\r\n", ch);
         } else {
            GET_AUTOINVIS(tch) = i;
            if (i == -1)
               send_to_char("Autoinvis off.\r\n", ch);
            else {
               sprintf(buf, "Your autoinvis is set to %d.\r\n", i);
               send_to_char(buf, ch);
            }
         }
      }
      return;
    default:
      result = PRF_TOG_CHK(tch, fields[i].bitvector);
      break;
    }

  if (result)
    send_to_char(tog_messages[i][TOG_ON], ch);
  else
    send_to_char(tog_messages[i][TOG_OFF], ch);
}


/***************************************************************************
 * $Log: prefs.c,v $
 * Revision 1.1  2010/06/09 22:32:01  mud
 * Initial revision
 *
 ***************************************************************************/

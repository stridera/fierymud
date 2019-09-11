/***************************************************************************
 * $Id: dg_comm.c,v 1.17 2009/03/03 19:43:44 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: dg_comm.c                                      Part of FieryMUD *
 *  Usage: Who knows?                                                      *
 *     By: Unknown                                                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 2000 by the Fiery Consortium                    *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "screen.h"

/* same as any_one_arg except that it stops at punctuation */
char *any_one_name(char *argument, char *first_arg)
{
  char* arg;
  
  /* Find first non blank */
  while (isspace(*argument))
    ++argument;
  
  /* Find length of first word */
  for (arg = first_arg;
       *argument && !isspace(*argument) &&
        (!ispunct(*argument) || *argument == '#' || *argument == '-');
      arg++, argument++)
    *arg = LOWER(*argument);
  *arg = '\0';
  
  return argument;
}

void sub_write_to_char(char_data *ch, char *tokens[],
                       char_data *ctokens[], obj_data *otokens[], char type[])
{
  char sb[MAX_STRING_LENGTH];
  int i;
  
  strcpy(sb, "");
  
  for (i = 0; tokens[i + 1]; i++)
    {
      strcat(sb, tokens[i]);
      /* changed everything to either c or o tokens respectively --gurlaek */
      switch (type[i])
	{
	case '~':
	  if (!ctokens[i])
	    strcat(sb, "someone");
	  else if (ctokens[i] == ch)
	    strcat(sb, "you");
	  else
	    strcat(sb, PERS(ctokens[i], ch));
	  break;
	  
	case '@':
	  if (!ctokens[i])
	    strcat(sb, "someone's");
	  else if (ctokens[i] == ch)
	    strcat(sb, "your");
	  else
	    {
	      strcat(sb, PERS(ctokens[i], ch));
	      strcat(sb, "'s");
	    }
	  break;
	  
	case '^':
	  if (!ctokens[i] || !CAN_SEE(ch, ctokens[i]))
	    strcat(sb, "its");
	  else if (ctokens[i] == ch)
	    strcat(sb, "your");
	  else
	    strcat(sb, HSHR(ctokens[i]));
	  break;
	  
	case '>':
	  if (!ctokens[i] || !CAN_SEE(ch, ctokens[i]))
	    strcat(sb, "it");
	  else if (ctokens[i] == ch)
	    strcat(sb, "you");
	  else
	    strcat(sb, HSSH(ctokens[i]));
	  break;
	  
	case '*':
	  if (!ctokens[i] || !CAN_SEE(ch, ctokens[i]))
	    strcat(sb, "it");
	  else if (ctokens[i] == ch)
	    strcat(sb, "you");
	  else
	    strcat(sb, HMHR(ctokens[i]));
	  break;
	  
	case '`':
	  if (!otokens[i])
	    strcat(sb, "something");
	  else
	    strcat(sb, OBJS(otokens[i], ch));
	  break;
	}
    }
  
  strcat(sb, tokens[i]);
  strcat(sb, "\r\n");

  /* Want to capitalize it... by passing it through CAP, which will
   * skip past any &D color codes.
   * However, if it starts with &0, that's the signal that the
   * script writer does not want it capitalized. */
  if ((sb[0] == CREL || sb[0] == CABS) && sb[1] == '0')
     send_to_char(sb, ch);
  else
     send_to_char(CAP(sb), ch);
}


void sub_write(char *arg, char_data *ch, byte find_invis, int targets)
{
  char str[MAX_INPUT_LENGTH * 2];
  char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char *tokens[MAX_INPUT_LENGTH], *s, *p;
  char_data *ctokens[MAX_INPUT_LENGTH];
  obj_data  *otokens[MAX_INPUT_LENGTH];

  char_data *to;
  obj_data *obj;
  int i;
  int sleep = 1; /* mainly for windows compiles */
  int olc = 0;
  
  if (!arg)
    return;
  
  tokens[0] = str;
  
  for (i = 0, p = arg, s = str; *p;)
    {
      ctokens[i] = NULL;
      otokens[i] = NULL;
      switch (*p) {
      case '~':
      case '@':
      case '^':
      case '>':
      case '*':
	/* get char_data, move to next token */
	type[i] = *p;
	*s = '\0';
        ++p;
	p = any_one_name(p, name);
	ctokens[i] = find_invis ? find_char_in_world(find_by_name(name)) : find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, name));
	tokens[++i] = ++s;
	break;
	
      case '`':
	/* get obj_data, move to next token */
	type[i] = *p;
	*s = '\0';
        ++p;
	p = any_one_name(++p, name);
        if (find_invis) obj = find_obj_in_world(find_by_name(name));
        else if (!(obj = find_obj_in_list(world[IN_ROOM(ch)].contents, find_vis_by_name(ch, name))));
        else if (!(obj = find_obj_in_eq(ch, NULL, find_vis_by_name(ch, name))));
        else obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, name));
	otokens[i] = obj;
	tokens[++i] = ++s;
	break;
	
      case '\\':
	p++;
	*s++ = *p++;
	break;
	
      default:
	*s++ = *p++;
      }
    }
  
  *s = '\0';
  tokens[++i] = NULL;
  
  if (IS_SET(targets, TO_CHAR) && SENDOK(ch))
    sub_write_to_char(ch, tokens, ctokens, otokens, type);
  
  if (IS_SET(targets, TO_ROOM))
    for (to = world[ch->in_room].people; to; to = to->next_in_room)
      if (to != ch && SENDOK(to))
        sub_write_to_char(to, tokens, ctokens, otokens, type);
}

/***************************************************************************
 * $Log: dg_comm.c,v $
 * Revision 1.17  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.16  2008/08/18 01:35:38  jps
 * Replaced all \\n\\r with \\r\\n, not that it was really necessary...
 *
 * Revision 1.15  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.14  2008/03/05 03:03:54  myc
 * Fix capitalization for DG triggers.
 *
 * Revision 1.13  2008/02/24 17:31:13  myc
 * Added a TO_OLC flag to act() to allow messages to be sent to people
 * while in OLC if they have OLCComm toggled on.
 *
 * Revision 1.12  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.11  2008/01/17 01:29:10  myc
 * Cleaned up sub_write_to_char and sub_write.  Everything seems to
 * still work...
 *
 * Revision 1.10  2007/07/25 00:38:03  jps
 * Give send_to_zone a room to skip, and make it use virtual zone number.
 *
 * Revision 1.9  2007/07/24 23:34:00  jps
 * Add a parameter min_position to send_to_zone()
 *
 * Revision 1.8  2006/11/20 07:02:20  jps
 * Allow color code &0 at the beginning of a line to suppress
 * capitalization of dg script messages.
 *
 * Revision 1.7  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.6  2000/11/21 01:19:10  rsd
 * Altered the comment header slightly, and added back rlog
 * messages from prior to the addition of the $log$ string.
 *
 * Revision 1.5  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.4  1999/08/29 07:06:04  jimmy
 * Many many small but ver significant bug fixes found using insure.  The
 * code now compiles cleanly and boots cleanly with insure.  The most significant
 * changes were moving all the BREATH's to within normal spell range, and
 * fixing the way socials were allocated.  Too many small fixes to list them
 * all. --gurlaek (now for the runtime debugging :( )
 *
 * Revision 1.3  1999/04/24 06:46:52  jimmy
 * changed to work with new pedantic flag --gurlaek
 *
 * Revision 1.2  1999/01/31 00:00:41  mud
 * Added the generic comment header
 * Indented entire file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

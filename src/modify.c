/***************************************************************************
 * $Id: modify.c,v 1.56 2009/07/16 19:15:54 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: modify.c                                       Part of FieryMUD *
 *  Usage: Run-time modification of game variables                         *
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
#include "interpreter.h"
#include "commands.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "casting.h"
#include "mail.h"
#include "olc.h"
#include "clan.h"
#include "skills.h"
#include "math.h"
#include "dg_scripts.h"
#include "screen.h"
#include "modify.h"
#include "strings.h"

void parse_action(int command, char *string, struct descriptor_data *d);

 /* action modes for parse_action */
#define PARSE_FORMAT       0
#define PARSE_REPLACE      1
#define PARSE_HELP         2
#define PARSE_DELETE       3
#define PARSE_INSERT       4
#define PARSE_LIST_NORM    5
#define PARSE_LIST_NUM     6
#define PARSE_EDIT         7

char *string_fields[] =
{
  "name",
  "short",
  "long",
  "description",
  "title",
  "delete-description",
  "\n"
};


/* maximum length for text field x+1 */
int length[] =
{
  15,
  60,
  256,
  240,
  60
};


/* ************************************************************************
*  modification of malloc'ed strings                                      *
************************************************************************ */

/* Basic API function to start writing somewhere. */
void string_write_limit(struct descriptor_data *d, char **writeto, size_t len, int maxlines)
{
  if (!writeto)
    return;

  if (d->character && !IS_NPC(d->character))
    SET_FLAG(PLR_FLAGS(d->character), PLR_WRITING);

  /* A pointer to the actual string you are editing. */
  d->str = writeto;
  /* The abort buffer: a copy of what you originally edited */
  d->backstr = (writeto && *writeto && **writeto) ? strdup(*writeto) : NULL;
  /* The maximum length the final string may consume */
  d->max_str = len;
  /* Zero mail_to to make sure the string editor doesn't try to mail
   * someone.  That means if you use this function with the mailer,
   * you need to set mail_to after calling this. */
  d->mail_to = 0;
  /* If there's a limitation on the number of lines. We do this to ensure
   * that players don't enter many pages of newlines. */
  d->max_buffer_lines = maxlines;

  /* Print out data */
  if (d->backstr) {
    if (d->character && PRF_FLAGGED(d->character, PRF_LINENUMS))
      parse_action(PARSE_LIST_NUM, "", d);
    else
      write_to_output(d->backstr, d);
  }
}

/* Start writing when you don't care about the number of lines. */
void string_write(struct descriptor_data *d, char **writeto, size_t len)
{
   string_write_limit(d, writeto, len, 0);
}

void mail_write(struct descriptor_data *d, char **writeto, size_t len, long recipient)
{
  if (!writeto)
    CREATE(writeto, char *, 1);
  string_write(d, writeto, len);
  d->mail_to = recipient;
}

/*
 * Handle some editor commands.
 */
void parse_action(int command, char *string, struct descriptor_data *d)
{
  int indent = 0, rep_all = 0, flags = 0, total_len, replaced;
  register int j = 0;
  int i, line_low, line_high;
  char *s, *t, temp;

  switch (command) {
  case PARSE_HELP:
    write_to_output("Editor command formats: /<letter>\r\n\r\n"
                    "/a         -  aborts editor\r\n"
                    "/c         -  clears buffer\r\n"
                    "/d#        -  deletes a line #\r\n"
                    "/e# <text> -  changes the line at # with <text>\r\n", d);
    if (STATE(d) == CON_TRIGEDIT)
      write_to_output("/f#        -  formats text with given indentation amount\r\n", d);
    else
      write_to_output("/f         -  formats text\r\n"
                      "/fi        -  indented formatting of text\r\n", d);
    write_to_output("/h         -  list text editor commands\r\n"
                    "/i# <text> -  inserts <text> before line #\r\n"
                    "/l         -  lists buffer\r\n"
                    "/n         -  lists buffer with line numbers\r\n"
                    "/r 'a' 'b' -  replace 1st occurrence of text <a> in buffer with text <b>\r\n"
                    "/ra 'a' 'b'-  replace all occurrences of text <a> within buffer with text <b>\r\n"
                    "              usage: /r[a] 'pattern' 'replacement'\r\n"
                    "/s         -  saves text\r\n", d);
    break;
  case PARSE_FORMAT:
    if (STATE(d) == CON_TRIGEDIT) {
      skip_spaces(&string);
      i = is_number(string) ? atoi(string) : 3;
      replaced = format_script(d, i);
      if (replaced)
        write_to_output("Script formatted.\r\n", d);
      else
        write_to_output("Script not formatted.\r\n", d);
      return;
    }
    while (isalpha(string[j]) && j < 2) {
      if (string[j] == 'i' && !indent) {
        indent = TRUE;
        flags += FORMAT_INDENT;
      }
      ++j;
    }
    format_text(d->str, flags, d, d->max_str);
    sprintf(buf, "Text formatted with%s indent.\r\n", (indent ? "" : "out"));
    write_to_output(buf, d);
    break;
  case PARSE_REPLACE:
    while (isalpha(string[j]) && j < 2) {
      switch (string[j]) {
      case 'a':
        if (!indent)
          rep_all = 1;
        break;
      default:
        break;
      }
      j++;
    }
    if ((s = strtok(string, "'")) == NULL) {
      write_to_output("Invalid format.\r\n", d);
      return;
    } else if ((s = strtok(NULL, "'")) == NULL) {
      write_to_output("Target string must be enclosed in single quotes.\r\n", d);
      return;
    } else if ((t = strtok(NULL, "'")) == NULL) {
      write_to_output("No replacement string.\r\n", d);
      return;
    } else if ((t = strtok(NULL, "'")) == NULL) {
      write_to_output("Replacement string must be enclosed in single quotes.\r\n", d);
      return;
    } else if ((total_len = ((strlen(t) - strlen(s)) + strlen(*d->str))) <= d->max_str) {
      if ((replaced = replace_str(d->str, s, t, rep_all, d->max_str)) > 0) {
        sprintf(buf, "Replaced %d occurrence%sof '%s' with '%s'.\r\n", replaced, ((replaced != 1) ? "s " : " "), s, t);
        write_to_output(buf, d);
      } else if (replaced == 0) {
        sprintf(buf, "String '%s' not found.\r\n", s);
        write_to_output(buf, d);
      } else
        write_to_output("ERROR: Replacement string causes buffer overflow, aborted replace.\r\n", d);
    } else
      write_to_output("Not enough space left in buffer.\r\n", d);
    break;
  case PARSE_DELETE:
    switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
    case 0:
      write_to_output("You must specify a line number or range to delete.\r\n", d);
      return;
    case 1:
      line_high = line_low;
      break;
    case 2:
      if (line_high < line_low) {
        write_to_output("That range is invalid.\r\n", d);
        return;
      }
      break;
    }

    i = 1;
    total_len = 1;
    if ((s = *d->str) == NULL) {
      write_to_output("Buffer is empty.\r\n", d);
      return;
    } else if (line_low > 0) {
      while (s && (i < line_low))
         if ((s = strchr(s, '\n')) != NULL) {
           i++;
          s++;
         }
      if ((i < line_low) || (s == NULL)) {
         write_to_output("Line(s) out of range; not deleting.\r\n", d);
        return;
      }

      t = s;
      while (s && (i < line_high))
         if ((s = strchr(s, '\n')) != NULL) {
          i++;
          total_len++;
          s++;
         }
      if ((s) && ((s = strchr(s, '\n')) != NULL)) {
        s++;
        while (*s != '\0')
          *(t++) = *(s++);
      } else
        total_len--;
      *t = '\0';
      RECREATE(*d->str, char, strlen(*d->str) + 3);

      sprintf(buf, "%d line%sdeleted.\r\n", total_len,
              ((total_len != 1) ? "s " : " "));
      write_to_output(buf, d);
    } else {
      write_to_output("Invalid line numbers to delete must be higher than 0.\r\n", d);
      return;
    }
    break;
  case PARSE_LIST_NORM:
    /*
     * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
     * they are probly ok for what to do here.
     */
    *buf = '\0';
    if (*string != '\0')
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
      } else {
        line_low = 1;
        line_high = 999999;
      }

    if (line_low < 1) {
      write_to_output("Line numbers must be greater than 0.\r\n", d);
      return;
    } else if (line_high < line_low) {
      write_to_output("That range is invalid.\r\n", d);
      return;
    }
    *buf = '\0';
    if ((line_high < 999999) || (line_low > 1))
      sprintf(buf, "Current buffer range [%d - %d]:\r\n", line_low, line_high);
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        s++;
      }
    if ((i < line_low) || (s == NULL)) {
      write_to_output("Line(s) out of range; no buffer listing.\r\n", d);
      return;
    }
    t = s;
    while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        total_len++;
        s++;
      }
    if (s) {
      temp = *s;
      *s = '\0';
      strcat(buf, t);
      *s = temp;
    } else
      strcat(buf, t);
    /*
     * This is kind of annoying...but some people like it.
     */
#if 0
    sprintf(buf, "%s\r\n%d line%sshown.\r\n", buf, total_len,
            ((total_len != 1)?"s ":" "));
#endif
    page_string_desc(d, buf);
    break;
  case PARSE_LIST_NUM:
    /*
     * Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so
     * they are probly ok for what to do here.
     */
    *buf = '\0';
    if (*string != '\0')
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
      } else {
        line_low = 1;
        line_high = 999999;
      }

    if (line_low < 1) {
      write_to_output("Line numbers must be greater than 0.\r\n", d);
      return;
    }
    if (line_high < line_low) {
      write_to_output("That range is invalid.\r\n", d);
      return;
    }
    *buf = '\0';
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        s++;
      }
    if ((i < line_low) || (s == NULL)) {
      write_to_output("Line(s) out of range; no buffer listing.\r\n", d);
      return;
    }
    t = s;
    while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        total_len++;
        s++;
        temp = *s;
        *s = '\0';
        sprintf(buf, "%s%4d: ", buf, (i - 1));
        strcat(buf, t);
        *s = temp;
        t = s;
      }
    if (s && t) {
      temp = *s;
      *s = '\0';
      strcat(buf, t);
      *s = temp;
    } else if (t)
      strcat(buf, t);
    page_string_desc(d, buf);
    break;

  case PARSE_INSERT:
    half_chop(string, buf, buf2);
    if (*buf == '\0') {
      write_to_output("You must specify a line number before which to insert text.\r\n", d);
      return;
    }
    line_low = atoi(buf);
    strcat(buf2, "\r\n");

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL) {
      write_to_output("Buffer is empty, nowhere to insert.\r\n", d);
      return;
    }
    if (line_low > 0) {
      while (s && (i < line_low))
         if ((s = strchr(s, '\n')) != NULL) {
          i++;
          s++;
         }
      if ((i < line_low) || (s == NULL)) {
        write_to_output("Line number out of range; insert aborted.\r\n", d);
        return;
      }
      temp = *s;
      *s = '\0';
      if ((strlen(*d->str) + strlen(buf2) + strlen(s + 1) + 3) > d->max_str) {
        *s = temp;
        write_to_output("Insert text pushes buffer over maximum size, insert aborted.\r\n", d);
        return;
      }
      if (*d->str && (**d->str != '\0'))
        strcat(buf, *d->str);
      *s = temp;
      strcat(buf, buf2);
      if (s && (*s != '\0'))
        strcat(buf, s);
      RECREATE(*d->str, char, strlen(buf) + 3);

      strcpy(*d->str, buf);
      write_to_output("Line inserted.\r\n", d);
    } else {
      write_to_output("Line number must be higher than 0.\r\n", d);
      return;
    }
    break;

  case PARSE_EDIT:
    half_chop(string, buf, buf2);
    if (*buf == '\0') {
      write_to_output("You must specify a line number at which to change text.\r\n", d);
      return;
    }
    line_low = atoi(buf);
    strcat(buf2, "\r\n");

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL) {
      write_to_output("Buffer is empty, nothing to change.\r\n", d);
      return;
    }
    if (line_low > 0) {
      /*
       * Loop through the text counting \\n characters until we get to the line/
       */
      while (s && (i < line_low))
         if ((s = strchr(s, '\n')) != NULL) {
          i++;
          s++;
         }
      /*
       * Make sure that there was a THAT line in the text.
       */
      if ((i < line_low) || (s == NULL)) {
        write_to_output("Line number out of range; change aborted.\r\n", d);
        return;
      }
      /*
       * If s is the same as *d->str that means I'm at the beginning of the
       * message text and I don't need to put that into the changed buffer.
       */
      if (s != *d->str) {
        /*
         * First things first .. we get this part into the buffer.
         */
        temp = *s;
        *s = '\0';
        /*
         * Put the first 'good' half of the text into storage.
         */
        strcat(buf, *d->str);
        *s = temp;
      }
      /*
       * Put the new 'good' line into place.
       */
      strcat(buf, buf2);
      if ((s = strchr(s, '\n')) != NULL) {
        /*
         * This means that we are at the END of the line, we want out of
         * there, but we want s to point to the beginning of the line
         * AFTER the line we want edited
         */
        s++;
        /*
         * Now put the last 'good' half of buffer into storage.
         */
        strcat(buf, s);
      }
      /*
       * Check for buffer overflow.
       */
      if (strlen(buf) > d->max_str) {
        write_to_output("Change causes new length to exceed buffer maximum size, aborted.\r\n", d);
        return;
      }
      /*
       * Change the size of the REAL buffer to fit the new text.
       */
      RECREATE(*d->str, char, strlen(buf) + 3);
      strcpy(*d->str, buf);
      write_to_output("Line changed.\r\n", d);
    } else {
      write_to_output("Line number must be higher than 0.\r\n", d);
      return;
    }
    break;
  default:
    write_to_output("Invalid option.\r\n", d);
    mudlog("SYSERR: invalid command passed to parse_action", BRF, LVL_HEAD_C, TRUE);
    return;
  }
}


/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str)
{
  int terminator = 0, action = 0;
  register int i = 2, j = 0;
  char actions[MAX_INPUT_LENGTH], *s;

  delete_doubledollar(str);

  if ((action = (*str == '/'))) {
    while (str[i] != '\0') {
      actions[j] = str[i];
      i++;
      j++;
    }
    actions[j] = '\0';
    *str = '\0';
    switch (str[1]) {
    case 'a':
      terminator = 2;                /* Working on an abort message, */
      break;
    case 'c':
      if (*(d->str)) {
        free(*d->str);
        *(d->str) = NULL;
        write_to_output("Current buffer cleared.\r\n", d);
      } else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'd':
      if (*d->str)
         parse_action(PARSE_DELETE, actions, d);
      else
         write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'e':
      if (*d->str)
         parse_action(PARSE_EDIT, actions, d);
      else
         write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'f':
      if (*(d->str))
        parse_action(PARSE_FORMAT, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'i':
      if (*(d->str))
        parse_action(PARSE_INSERT, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'h':
      parse_action(PARSE_HELP, actions, d);
      break;
    case 'l':
      if (*d->str)
        parse_action(PARSE_LIST_NORM, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'n':
      if (*d->str)
        parse_action(PARSE_LIST_NUM, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'r':
      if (*d->str)
         parse_action(PARSE_REPLACE, actions, d);
      else
         write_to_output("Current buffer empty.\r\n", d);
      break;
    case 's':
      terminator = 1;
      *str = '\0';
      break;
    default:
      write_to_output("Invalid option.\r\n", d);
      break;
    }
  }

  /* Comparisons below this point are made with d->max_str - 3.
   * That leaves 3 spaces after the last text entered by the user: \r\n\0   */
   if (!(*d->str)) {
      if (strlen(str) > d->max_str - 3) {
         send_to_char("String too long - Truncated.\r\n", d->character);
         *(str + d->max_str) = '\0';
      }
      CREATE(*d->str, char, strlen(str) + 3);
      strcpy(*d->str, str);
      if (strlen(str))
         strcat(*d->str, "\r\n");
   } else if (!action) {
      if (strlen(str) + strlen(*d->str) > d->max_str - 3) {
         send_to_char("String too long, limit reached on message.  Last line ignored.\r\n",
            d->character);
         return;
      } else {
         if (!(*d->str = (char *)realloc(*d->str, strlen(*d->str) +
               strlen(str) + 3))) {
            perror("string_add");
            exit(1);
         }
         strcat(*d->str, str);
         strcat(*d->str, "\r\n");
      }
   }

   if (terminator) {
      /*. OLC Edits .*/
      extern void oedit_disp_menu(struct descriptor_data *d);
      extern void oedit_disp_extradesc_menu(struct descriptor_data *d);
      extern void redit_disp_menu(struct descriptor_data *d);
      extern void redit_disp_extradesc_menu(struct descriptor_data *d);
      extern void redit_disp_exit_menu(struct descriptor_data *d);
      extern void medit_disp_menu(struct descriptor_data *d);
      extern void hedit_disp_menu(struct descriptor_data *d);
      extern void trigedit_disp_menu(struct descriptor_data *d);

      /* Check for too many lines */
      if (terminator == 1 && d->max_buffer_lines > 0 && d->str && *d->str) {
         for (i = 0, s = *d->str; s; )
            if ((s = strchr(s, '\n')) != NULL) {
               i++;
               s++;
            }
         if (i > d->max_buffer_lines) {
            sprintf(buf, "The buffer has %d lines, which is over the limit of %d.\r\n",
                  i, d->max_buffer_lines);
            write_to_output(buf, d);
            write_to_output("Unable to save.  Use /a if you want to abort.\r\n", d);
            return;
         }
      }

    /*
     * Here we check for the abort option and reset the pointers.
     */
      if ((terminator == 2) &&
            ((STATE(d) == CON_REDIT) ||
            (STATE(d) == CON_MEDIT) ||
            (STATE(d) == CON_OEDIT) ||
            (STATE(d) == CON_TRIGEDIT) ||
            (STATE(d) == CON_HEDIT) ||
            (STATE(d) == CON_EXDESC) ||
            (STATE(d) == CON_MEDIT))) {
         free(*d->str);
         if (d->backstr) {
            *d->str = d->backstr;
         } else
            *d->str = NULL;

         d->backstr = NULL;
         d->str = NULL;
      } else if ((d->str) && (*d->str) && (**d->str == '\0')) {
         free(*d->str);
         if (STATE(d) == CON_EXDESC)
            *d->str = strdup("");
         else
            *d->str = strdup("Nothing.\r\n");
      }

      if (STATE(d) == CON_MEDIT)
         medit_disp_menu(d);
      if (STATE(d) == CON_TRIGEDIT)
         trigedit_disp_menu(d);
      if (STATE(d) == CON_GEDIT)
         gedit_disp_menu(d);
      if (STATE(d) == CON_OEDIT) {
         switch(OLC_MODE(d)) {
            case OEDIT_ACTDESC:
               oedit_disp_menu(d);
               break;
            case OEDIT_EXTRADESC_DESCRIPTION:
               oedit_disp_extradesc_menu(d);
               break;
         }
      } else if (STATE(d) == CON_REDIT) {
         switch (OLC_MODE(d)) {
            case REDIT_DESC:
               redit_disp_menu(d);
               break;
            case REDIT_EXIT_DESCRIPTION:
               redit_disp_exit_menu(d);
               break;
            case REDIT_EXTRADESC_DESCRIPTION:
               redit_disp_extradesc_menu(d);
               break;
         }
      } else if (STATE(d) == CON_HEDIT)
         hedit_disp_menu(d);
      else if (!d->connected && (PLR_FLAGGED(d->character, PLR_MAILING))) {
         if ((terminator == 1) && *d->str) {
            store_mail(d->mail_to, GET_IDNUM(d->character), d->mail_vnum, *d->str);
            write_to_output("Message sent!\r\n", d);
         } else {
            write_to_output("Mail aborted.\r\n", d);
            if (d->mail_vnum != NOTHING) {
               struct obj_data *obj = read_object(d->mail_vnum, VIRTUAL);
               if (obj)
                 obj_to_char(obj, d->character);
            }
         }
         d->mail_to = 0;
         d->mail_vnum = NOTHING;
         free(*d->str);
         free(d->str);
         /*  write_to_output("Message sent!\r\n", d);
        if (!IS_NPC(d->character))
        REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);*/
      } else if (STATE(d) == CON_EXDESC) {
         if (terminator != 1)
            write_to_output("Description aborted.\r\n", d);
         STATE(d) = CON_PLAYING;
      } else if (!d->connected && d->character && !IS_NPC(d->character)) {
         if (terminator == 1) {
            if (strlen(*d->str) == 0) {
               free(*d->str);
               *d->str = NULL;
            }
         } else {
            free(*d->str);
            if (d->backstr)
               *d->str = d->backstr;
            else
               *d->str = NULL;
            d->backstr = NULL;
            write_to_output("Message aborted.\r\n", d);
         }
      }
      if (d->character && !IS_NPC(d->character)) {
         REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
         REMOVE_FLAG(PLR_FLAGS(d->character), PLR_MAILING);
      }
      if (d->backstr)
         free(d->backstr);
      d->backstr = NULL;
      d->str = NULL;
   }
}



/* **********************************************************************
 *  Modification of character skills                                     *
 ********************************************************************** */

/* made skillset <vict> only show nonzero skills -321 3/14/00 */
ACMD(do_skillset)
{
  struct char_data *vict;
  char name[100], buf2[100];
  int skill, value, i, j, k;

  argument = one_argument(argument, name);

  if (!*name) {                        /* no arguments. print an informative text */
    send_to_char("Syntax: skillset <name> '<skill>' <value>\r\n", ch);
    strcpy(buf, "Skill being one of the following:\r\n");
    for (j = 0, k = 0; j <= TOP_SKILL; ++j) {
      i = skill_sort_info[j];
      if (*skills[i].name == '!')
        continue;
      sprintf(buf + strlen(buf), "%-18.18s ", skills[i].name);
      if (k % 4 == 3) { /* last column */
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        *buf = '\0';
      }
      ++k;
    }
    if (*buf)
      send_to_char(buf, ch);
    send_to_char("\r\n", ch);
    return;
  }
  if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  skip_spaces(&argument);

  /* If there is no chars in argument */
  if (!*argument){
    if (GET_LEVEL(vict) < GET_LEVEL(ch) || ch==vict) {
      strcpy(buf, "Current values of skills:\r\n");
      for (i = 0; i <= TOP_SKILL; ++i) {
        if (*skills[i].name == '!' || !GET_ISKILL(vict,i))
          continue;
        sprintf(buf, "%s%-20.20s %d\r\n", buf, skills[i].name, GET_ISKILL(vict,i));
      }
      page_string(ch, buf);
    }
    else
      send_to_char("You are unable to divine any information.\r\n", ch);
    return;
  }

  if (*argument == '*') {
    for (i = 1; i <= TOP_SKILL; ++i) {
      SET_SKILL(vict, i, 1000);
      if (str_cmp(skills[i].name, "!UNUSED!")) {
        sprintf(buf2, "You change %s's skill level in %s to 100.\r\n", GET_NAME(vict),
                skills[i].name);
        send_to_char(buf2, ch);
      }
    }
    return;
  }

  argument = delimited_arg(argument, arg, '\'');

  if ((skill = find_talent_num(arg, TALENT)) <= 0) {
    send_to_char("Unrecognized skill.\r\n", ch);
    return;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Learned value expected.\r\n", ch);
    return;
  } else if ((value = atoi(arg))< 0) {
    send_to_char("Minimum value for learned is 0.\r\n", ch);
    return;
  } else if (value > 1000) {
    send_to_char("Max value for learned is 1000.\r\n", ch);
    return;
  } else if (IS_NPC(vict)) {
    send_to_char("You can't set NPC skills.\r\n", ch);
    return;
  }
  sprintf(buf2, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict),
          skills[skill].name, value);
  mudlog(buf2, BRF, -1, TRUE);

  SET_SKILL(vict, skill, value);

  sprintf(buf2, "You change %s's %s to %d.\r\n", GET_NAME(vict),
          skills[skill].name, value);
  send_to_char(buf2, ch);


}

/******************/
/*   PAGINATION   */
/******************/

#define PAGE_WIDTH      120
#define MAX_MORTAL_PAGEBUF  81920
#define MAX_IMMORT_PAGEBUF  512000

char paging_pbuf[MAX_STRING_LENGTH];
char paging_buf[MAX_STRING_LENGTH];

static int get_page_length(struct descriptor_data *d)
{
   int page_length = 22;

   if (d) {
      if (d->original)
         page_length = GET_PAGE_LENGTH(d->original);
      else if (d->character)
         page_length = GET_PAGE_LENGTH(d->character);
      if (page_length <= 0 || page_length > 50)
         page_length = 22;
   }

   return page_length;
}

static void add_paging_element(struct descriptor_data *d, struct paging_line *pl)
{
   int page_length = get_page_length(d);

   if (d->paging_tail) {
      d->paging_tail->next = pl;
      d->paging_tail = pl;
   } else {
      d->paging_lines = pl;
      d->paging_tail = pl;
   }

   /* Recalculate number of pages */
   d->paging_numlines++;
   d->paging_numpages = d->paging_numlines / page_length;
   if (d->paging_numlines % page_length)
      d->paging_numpages++;
   d->paging_bufsize += sizeof(struct paging_line) + strlen(pl->line) + 1;
}

static bool ok_paging_add(struct descriptor_data *d, int size)
{
   int maxbuf = MAX_MORTAL_PAGEBUF;

   if (d->paging_skipped) {
      d->paging_skipped++;
      return FALSE;
   }

   if (d->original) {
      if (GET_LEVEL(d->original) < LVL_IMMORT)
         maxbuf = MAX_MORTAL_PAGEBUF;
      else
         maxbuf = MAX_IMMORT_PAGEBUF;
   } else if (d->character) {
      if (GET_LEVEL(d->character) < LVL_IMMORT)
         maxbuf = MAX_MORTAL_PAGEBUF;
      else
         maxbuf = MAX_IMMORT_PAGEBUF;
   }

   if (d->paging_bufsize + size > maxbuf) {
      d->paging_skipped++;
      return FALSE;
   } else {
      return TRUE;
   }
}

static void add_paging_fragment(struct descriptor_data *d, const char *line, int len)
{
   struct paging_line *pl;

   if (ok_paging_add(d, len + 1)) {
      CREATE(pl, struct paging_line, 1);
      pl->line = (char *)(malloc(len + 1));
      memcpy(pl->line, line, len + 1);
      pl->line[len] = '\0';

      add_paging_element(d, pl);
   }
}

static void paging_addstr(struct descriptor_data *d, const char *str)
{
   int col = 1, spec_code = FALSE;
   const char *line_start, *s;
   char *tmp;

   if (d->paging_fragment) {
      sprintf(paging_buf, "%s%s", d->paging_fragment, str);
      free(d->paging_fragment);
      d->paging_fragment = NULL;
      line_start = paging_buf;
   } else {
      line_start = str;
   }

   s = line_start;

   for (;; s++) {
      /* End of string */
      if (*s == '\0') {
         if (s != line_start) {
            tmp = strdup(line_start);
            d->paging_fragment = tmp;
         }
         return;

      /* Check for the begining of an ANSI color code block. */
      } else if (*s == '\x1B' && !spec_code)
         spec_code = TRUE;

      /* Check for the end of an ANSI color code block. */
      else if (*s == 'm' && spec_code)
         spec_code = FALSE;

      /* Check for inline ansi too! */
      else if ((*s == CREL || *s == CABS) && !spec_code) {
         s++;
         continue;

      /* Check for everything else. */
      } else if (!spec_code) {
         /* Carriage return puts us in column one. */
         if (*s == '\r')
            col = 1;
         /* Newline puts us on the next line. */
         else if (*s == '\n') {
            add_paging_fragment(d, line_start, s - line_start + 1);
            line_start = s + 1;

         /* We need to check here and see if we are over the page width,
          * and if so, compensate by going to the begining of the next line. */
         } else if (col++ > PAGE_WIDTH) {
            col = 1;
            add_paging_fragment(d, line_start, s - line_start);
            line_start = s;
         }
      }
   }
}

void paging_printf(const struct char_data *ch, const char *messg, ...)
{
   va_list args;

   if (ch->desc && messg && *messg) {
      va_start(args, messg);
      vsnprintf(paging_pbuf, sizeof(paging_pbuf), messg, args);
      va_end(args);
      paging_addstr(ch->desc, paging_pbuf);
   }
}

void desc_paging_printf(struct descriptor_data *d, const char *messg, ...)
{
   va_list args;

   if (messg && *messg) {
      va_start(args, messg);
      vsnprintf(paging_pbuf, sizeof(paging_pbuf), messg, args);
      va_end(args);
      paging_addstr(d, paging_pbuf);
   }
}

void free_paged_text(struct descriptor_data *d)
{
   struct paging_line *pl, *plnext;

   for (pl = d->paging_lines; pl; pl = plnext) {
      free(pl->line);
      plnext = pl->next;
      free(pl);
   }
   if (d->paging_fragment) {
      free(d->paging_fragment);
      d->paging_fragment = NULL;
   }
   d->paging_lines = NULL;
   d->paging_tail = NULL;
   d->paging_numlines = 0;
   d->paging_numpages = 0;
   d->paging_curpage = 0;
   d->paging_bufsize = 0;
   d->paging_skipped = 0;
}

struct paging_line *paging_goto_page(struct descriptor_data *d, int page)
{
   int destpage = page;
   int page_length = get_page_length(d);
   int i, j;
   struct paging_line *pl;

   /* Decide exactly which page to go to */
   if (destpage < 0)
      destpage = 0;
   else if (destpage >= PAGING_NUMPAGES(d))
      destpage = PAGING_NUMPAGES(d) - 1;

   PAGING_PAGE(d) = destpage;

   /* Go to it */
   pl = d->paging_lines;
   for (i = 0; i < destpage; i++)
      for (j = 0; j < page_length; j++) {
         if (!pl->next) {
            sprintf(buf, "SYSERR: Pager tried to go to page %d, but there were only %d lines at %d per page!",
                  destpage + 1, j + (i + 1) * page_length, page_length);
            log(buf);
         }
         pl = pl->next;
      }

   return pl;
}

void get_paging_input(struct descriptor_data *d, char *input)
{
   int i, page_length;
   struct paging_line *line;

   one_argument(input, buf);

   /* Q is for quit. :) */
   if (LOWER(*buf) == 'q') {
      free_paged_text(d);
      return;
   }
   /* R is for refresh */
   else if (LOWER(*buf) == 'r')
      line = paging_goto_page(d, PAGING_PAGE(d));

   /* B is for back */
   else if (LOWER(*buf) == 'b')
      line = paging_goto_page(d, PAGING_PAGE(d) - 1);

   /* A digit: goto a page */
   else if (isdigit(*buf))
      line = paging_goto_page(d, atoi(buf) - 1);

   else if (*buf) {
   /* Other input: quit the pager */
      free_paged_text(d);
      return;
   } else
   /* No input: goto the next page */
      line = paging_goto_page(d, PAGING_PAGE(d) + 1);

   page_length = get_page_length(d);
   for (i = 0; i < page_length && line; i++) {
      send_to_char(line->line, d->character);
      line = line->next;
   }

   if (!line)
      free_paged_text(d);
}

void start_paging_desc(struct descriptor_data *d)
{
   int i, page_length = get_page_length(d);
   struct paging_line *line = d->paging_lines;

   PAGING_PAGE(d) = 0;

   /* Now that the show is on the road, any fragmentary text must be put in
    * the list of lines with the rest of the text */
   if (d->paging_fragment) {
      add_paging_fragment(d, paging_buf, sprintf(paging_buf, "%s\r\n", d->paging_fragment));
      free(d->paging_fragment);
      d->paging_fragment = NULL;
   }

   /* Notify player if there was too much text to fit in the pager */
   if (d->paging_skipped) {
      sprintf(buf, "***   OVERFLOW  %d line%s skipped   ***\r\n\r\n",
            d->paging_skipped, d->paging_skipped == 1 ? "" : "s");
      send_to_char(buf, d->character);
   }

   /* Send the initial page of text */
   for (i = 0; i < page_length && line; i++) {
      send_to_char(line->line, d->character);
      line = line->next;
   }

   /* Was there no more than one page? */
   if (!line)
      free_paged_text(d);
}

/* When you have been accumulating some text to someone and are finished,
 * start the paging process. */
void start_paging(struct char_data *ch)
{
   if (ch->desc)
      start_paging_desc(ch->desc);
}

/* LEGACY !!! */
void page_line(struct char_data *ch, char *str)
{
   if (ch->desc)
      paging_addstr(ch->desc, str);
}

void page_string(struct char_data *ch, const char *str)
{
   if (ch->desc)
      page_string_desc(ch->desc, str);
}

void page_string_desc(struct descriptor_data *d, const char *str)
{
   paging_addstr(d, str);
   if (PAGING(d))
      start_paging_desc(d);
}


/***************************************************************************
 * $Log: modify.c,v $
 * Revision 1.56  2009/07/16 19:15:54  myc
 * Moved command stuff from grant.c to commands.c
 *
 * Revision 1.55  2009/06/09 05:45:27  myc
 * Removing the separate connection state for clan description
 * editing.  It's no longer necessary with the new editor.
 *
 * Revision 1.54  2009/03/20 23:02:59  myc
 * Remove text editor connection state.  Make paging input
 * strings declared const.
 *
 * Revision 1.53  2009/03/20 20:19:51  myc
 * Removing dependency upon old board system.
 *
 * Revision 1.52  2009/03/09 04:41:56  jps
 * Put FORMAT_INDENT definition in strings.h
 *
 * Revision 1.51  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.50  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.49  2009/03/03 19:43:44  myc
 * New target finding mechanism in find.c.
 *
 * Revision 1.48  2009/02/11 17:03:39  myc
 * Make some functions static and add desc_paging_printf(),
 * which is just like paging_printf, but it takes a descriptor
 * instead of a character.
 *
 * Revision 1.47  2008/08/18 01:35:22  jps
 * Use sprintf silly wabbit or you lose your text!!!!
 *
 * Revision 1.46  2008/08/16 08:25:40  jps
 * Added a desc command so players can edit their descriptions in game.
 *
 * Revision 1.45  2008/08/15 03:59:08  jps
 * Added pprintf for paging, and changed page_string to take a character.
 *
 * Revision 1.44  2008/08/14 15:40:29  jps
 * Added pager buffer size limits.
 *
 * Revision 1.43  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.42  2008/07/15 17:55:06  myc
 * Gedit needs one string editor, so had to modify string_add.
 *
 * Revision 1.41  2008/07/13 06:34:59  myc
 * Bug in 'skillset *': was overwriting player's player_specials pointer.
 *
 * Revision 1.40  2008/04/05 05:05:42  myc
 * Removed SEND_TO_Q macro, so call write_to_output directly.
 *
 * Revision 1.39  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.38  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.37  2008/03/27 00:52:30  jps
 * Increased line length so pagination will work when showing lists of mobs.
 *
 * Revision 1.36  2008/03/22 03:22:38  myc
 * All invocations of the string editor now go through string_write()
 * instead of messing with the descriptor variables itself.  Also added
 * a toggle, LineNums, to decide whether to do /l or /n when entering
 * the string editor.
 *
 * Revision 1.35  2008/03/21 16:02:05  myc
 * Added info about script formatter to editor help.
 *
 * Revision 1.34  2008/03/21 15:58:34  myc
 * Added a utility format scripts.
 *
 * Revision 1.33  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.32  2008/02/02 19:38:20  myc
 * Adding string_write to set up a descriptor for string editing.
 *
 * Revision 1.31  2008/02/02 05:35:14  myc
 * Making the skillset command have sorted output.
 *
 * Revision 1.30  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 *
 * Revision 1.29  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.28  2008/01/26 12:58:14  jps
 * Using skills.h.
 *
 * Revision 1.27  2007/12/31 02:00:57  jps
 * Made the general term for spells, skills, chants, and songs 'talent'.
 * Fixed mskillset to handle all talents.
 *
 * Revision 1.26  2007/12/19 20:54:56  myc
 * Added support editing clan descriptions to the line editor.
 *
 * Revision 1.25  2007/10/25 20:40:05  myc
 * Fixed bug where if you tried to mail an object but aborted the message,
 * you didn't get the object back.
 *
 * Revision 1.24  2007/10/13 05:05:13  myc
 * do_skillset was using find_skill_num, which restricts the search
 * to skills.  Is now using id_skill_spell.
 *
 * Revision 1.23  2007/10/11 20:14:48  myc
 * Changed skill defines to support chants and songs as skills, but
 * slightly distinguished from spells and skills.  TOP_SKILL is the
 * old MAX_SKILLS.
 *
 * Revision 1.22  2007/10/02 02:52:27  myc
 * Page length is now checked on the original player instead of the mob
 * for switched/shapechanged players.
 *
 * Revision 1.21  2007/09/28 20:49:35  myc
 * skillset uses the global buffer now.  It also uses delimited_arg(),
 * instead of parsing the argument string itself.
 *
 * Revision 1.20  2007/09/15 05:03:46  myc
 * Removed a potentially dangerous (small) buffer from the parse_action
 * function.  It was also unneeded.
 *
 * Revision 1.19  2007/08/08 02:26:36  jps
 * Typo fix
 *
 * Revision 1.18  2007/08/03 22:00:11  myc
 * Fixed some \r\n typoes in send_to_chars.
 *
 * Revision 1.17  2007/07/19 22:19:34  jps
 * Prevent an extra newline from being inserted into the buffer after /c.
 * Compactify the /n display so that each line takes up only one line.
 *
 * Revision 1.16  2007/07/02 05:47:31  jps
 * Inserted empty buffer checks for several editor commands that
 * desperately needed it - you could open a new buffer, type /r 'this' 'that',
 * and instantly crash the mud. Also inserted newline for the first
 * line just as it is for subsequent lines.
 *
 * Revision 1.15  2007/07/02 04:49:05  jps
 * Comment out some debugging code!
 *
 * Revision 1.14  2007/07/02 04:23:52  jps
 * Adjust call to recently-fixed replace_str.
 *
 * Revision 1.13  2007/02/04 18:12:31  myc
 * Page length now saves as a part of player specials.
 *
 * Revision 1.12  2006/11/23 00:36:24  jps
 * Fix newline-appending and length-checking while editing near
 * or past the limits of the given buffer in string_add.
 *
 * Revision 1.11  2004/11/11 23:29:19  rsd
 * Altered the output for the help for posting on a board
 * because the buffer was greater than the 509 bytes
 * allowed for the compiler not to cry.
 *
 * Revision 1.10  2003/06/25 02:21:03  jjl
 * Revised lay hands to not suck.
 *
 * Revision 1.9  2003/06/23 01:47:09  jjl
 * Added a NOFOLLOW flag, and the "note" command, and show notes <player>
 *
 * Revision 1.8  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.7  2000/11/28 01:26:52  mtp
 * removed some mobprog stuff
 *
 * Revision 1.6  2000/11/24 19:03:31  rsd
 * Altered comment header and added back rlog messages from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.5  2000/03/26 21:16:23  cso
 * made skillset <vict> show only nonzero skills
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/08/08 04:20:34  mud
 * Changed one of the page_string functions so that if a bogus
 * character is entered while paging, it falls out of the page
 * and mops up the memory. Commented out the old send to char
 * text below it.
 *
 * Revision 1.2  1999/01/31 17:54:05  mud
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

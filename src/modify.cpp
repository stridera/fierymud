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

#include "modify.hpp"

#include "casting.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "commands.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "mail.hpp"
#include "math.hpp"
#include "messages.hpp"
#include "olc.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <ranges>

using namespace std::string_view_literals;

/* action modes for parse_action */
#define PARSE_FORMAT 0
#define PARSE_REPLACE 1
#define PARSE_HELP 2
#define PARSE_DELETE 3
#define PARSE_INSERT 4
#define PARSE_LIST_NORM 5
#define PARSE_LIST_NUM 6
#define PARSE_EDIT 7

char *string_fields[] = {"name", "short", "long", "description", "title", "delete-description", "\n"};

/* maximum length for text field x+1 */
int length[] = {15, 60, 256, 240, 60};

/* ************************************************************************
 *  modification of malloc'ed strings                                      *
 ************************************************************************ */

/* Basic API function to start writing somewhere. */
void string_write_limit(DescriptorData *d, char **writeto, size_t len, int maxlines) {
    if (!writeto)
        return;

    if (d->character && !IS_NPC(d->character))
        SET_FLAG(PLR_FLAGS(d->character), PLR_WRITING);

    /* A pointer to the actual string you are editing. */
    d->str = writeto;
    /* The abort buffer: a copy of what you originally edited */
    d->backstr = (writeto && *writeto && **writeto) ? strdup(*writeto) : nullptr;
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
            string_to_output(d, d->backstr);
    }
}

/* Start writing when you don't care about the number of lines. */
void string_write(DescriptorData *d, char **writeto, size_t len) { string_write_limit(d, writeto, len, 0); }

void mail_write(DescriptorData *d, char **writeto, size_t len, long recipient) {
    if (!writeto)
        CREATE(writeto, char *, 1);
    string_write(d, writeto, len);
    d->mail_to = recipient;
}

/*
 * Handle some editor commands.
 */
void parse_action(int command, char *string, DescriptorData *d) {
    int indent = 0, rep_all = 0, flags = 0, total_len, replaced;
    int j = 0;
    int i, line_low, line_high;
    char *s, *t, temp;

    switch (command) {
    case PARSE_HELP:
        string_to_output(d,
                         "Editor command formats: /<letter>\n\n"
                         "/a         -  aborts editor\n"
                         "/c         -  clears buffer\n"
                         "/d#        -  deletes a line #\n"
                         "/e# <text> -  changes the line at # with <text>\n");
        if (STATE(d) == CON_TRIGEDIT)
            string_to_output(d, "/f#        -  formats text with given indentation amount\n");
        else
            string_to_output(d,
                             "/f         -  formats text\n"
                             "/fi        -  indented formatting of text\n");
        string_to_output(d,
                         "/h         -  list text editor commands\n"
                         "/i# <text> -  inserts <text> before line #\n"
                         "/l         -  lists buffer\n"
                         "/n         -  lists buffer with line numbers\n"
                         "/r 'a' 'b' -  replace 1st occurrence of text <a> in "
                         "buffer with text <b>\n"
                         "/ra 'a' 'b'-  replace all occurrences of text <a> within "
                         "buffer with text <b>\n"
                         "              usage: /r[a] 'pattern' 'replacement'\n"
                         "/s         -  saves text\n");
        break;
    case PARSE_FORMAT:
        if (STATE(d) == CON_TRIGEDIT) {
            skip_spaces(&string);
            i = is_number(string) ? atoi(string) : 3;
            replaced = format_script(d, i);
            if (replaced)
                string_to_output(d, "Script formatted.\n");
            else
                string_to_output(d, "Script not formatted.\n");
            return;
        }
        while (isalpha(string[j]) && j < 2) {
            if (string[j] == 'i' && !indent) {
                indent = true;
                flags += FORMAT_INDENT;
            }
            ++j;
        }
        format_text(d->str, flags, d, d->max_str);
        sprintf(buf, "Text formatted with%s indent.\n", (indent ? "" : "out"));
        string_to_output(d, buf);
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
        if ((s = strtok(string, "'")) == nullptr) {
            string_to_output(d, "Invalid format.\n");
            return;
        } else if ((s = strtok(nullptr, "'")) == nullptr) {
            string_to_output(d, "Target string must be enclosed in single quotes.\n");
            return;
        } else if ((t = strtok(nullptr, "'")) == nullptr) {
            string_to_output(d, "No replacement string.\n");
            return;
        } else if ((t = strtok(nullptr, "'")) == nullptr) {
            string_to_output(d, "Replacement string must be enclosed in single quotes.\n");
            return;
        } else if ((total_len = ((strlen(t) - strlen(s)) + strlen(*d->str))) <= d->max_str) {
            if ((replaced = replace_str(d->str, s, t, rep_all, d->max_str)) > 0) {
                string_to_output(d, "Replaced {:d} occurrence{}of '{}' with '{}'.\n", replaced,
                                 ((replaced != 1) ? "s " : " "), s, t);
            } else if (replaced == 0) {
                string_to_output(d, "String '{}' not found.\n", s);
            } else
                string_to_output(d, "ERROR: Replacement string causes buffer overflow, aborted replace.\n");
        } else
            string_to_output(d, "Not enough space left in buffer.\n");
        break;
    case PARSE_DELETE:
        switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
        case 0:
            string_to_output(d, "You must specify a line number or range to delete.\n");
            return;
        case 1:
            line_high = line_low;
            break;
        case 2:
            if (line_high < line_low) {
                string_to_output(d, "That range is invalid.\n");
                return;
            }
            break;
        }

        i = 1;
        total_len = 1;
        if ((s = *d->str) == nullptr) {
            string_to_output(d, "Buffer is empty.\n");
            return;
        } else if (line_low > 0) {
            while (s && (i < line_low))
                if ((s = strchr(s, '\n')) != nullptr) {
                    i++;
                    s++;
                }
            if ((i < line_low) || (s == nullptr)) {
                string_to_output(d, "Line(s) out of range; not deleting.\n");
                return;
            }

            t = s;
            while (s && (i < line_high))
                if ((s = strchr(s, '\n')) != nullptr) {
                    i++;
                    total_len++;
                    s++;
                }
            if ((s) && ((s = strchr(s, '\n')) != nullptr)) {
                s++;
                while (*s != '\0')
                    *(t++) = *(s++);
            } else
                total_len--;
            *t = '\0';
            RECREATE(*d->str, char, strlen(*d->str) + 3);

            sprintf(buf, "%d line%sdeleted.\n", total_len, ((total_len != 1) ? "s " : " "));
            string_to_output(d, buf);
        } else {
            string_to_output(d, "Invalid line numbers to delete must be higher than 0.\n");
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
            }
        else {
            line_low = 1;
            line_high = 999999;
        }

        if (line_low < 1) {
            string_to_output(d, "Line numbers must be greater than 0.\n");
            return;
        } else if (line_high < line_low) {
            string_to_output(d, "That range is invalid.\n");
            return;
        }
        *buf = '\0';
        if ((line_high < 999999) || (line_low > 1))
            sprintf(buf, "Current buffer range [%d - %d]:\n", line_low, line_high);
        i = 1;
        total_len = 0;
        s = *d->str;
        while (s && (i < line_low))
            if ((s = strchr(s, '\n')) != nullptr) {
                i++;
                s++;
            }
        if ((i < line_low) || (s == nullptr)) {
            string_to_output(d, "Line(s) out of range; no buffer listing.\n");
            return;
        }
        t = s;
        while (s && (i <= line_high))
            if ((s = strchr(s, '\n')) != nullptr) {
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
	sprintf(buf, "%s\n%d line%sshown.\n", buf, total_len, ((total_len != 1) ? "s " : " "));
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
            }
        else {
            line_low = 1;
            line_high = 999999;
        }

        if (line_low < 1) {
            string_to_output(d, "Line numbers must be greater than 0.\n");
            return;
        }
        if (line_high < line_low) {
            string_to_output(d, "That range is invalid.\n");
            return;
        }
        *buf = '\0';
        i = 1;
        total_len = 0;
        s = *d->str;
        while (s && (i < line_low))
            if ((s = strchr(s, '\n')) != nullptr) {
                i++;
                s++;
            }
        if ((i < line_low) || (s == nullptr)) {
            string_to_output(d, "Line(s) out of range; no buffer listing.\n");
            return;
        }
        t = s;
        while (s && (i <= line_high))
            if ((s = strchr(s, '\n')) != nullptr) {
                i++;
                total_len++;
                s++;
                temp = *s;
                *s = '\0';
                snprintf(buf, sizeof(buf), "%s%4d: ", buf, (i - 1));
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
            string_to_output(d, "You must specify a line number before which to insert text.\n");
            return;
        }
        line_low = atoi(buf);
        strcat(buf2, "\n");

        i = 1;
        *buf = '\0';
        if ((s = *d->str) == nullptr) {
            string_to_output(d, "Buffer is empty, nowhere to insert.\n");
            return;
        }
        if (line_low > 0) {
            while (s && (i < line_low))
                if ((s = strchr(s, '\n')) != nullptr) {
                    i++;
                    s++;
                }
            if ((i < line_low) || (s == nullptr)) {
                string_to_output(d, "Line number out of range; insert aborted.\n");
                return;
            }
            temp = *s;
            *s = '\0';
            if ((strlen(*d->str) + strlen(buf2) + strlen(s + 1) + 3) > d->max_str) {
                *s = temp;
                string_to_output(d, "Insert text pushes buffer over maximum size, insert aborted.\n");
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
            string_to_output(d, "Line inserted.\n");
        } else {
            string_to_output(d, "Line number must be higher than 0.\n");
            return;
        }
        break;

    case PARSE_EDIT:
        half_chop(string, buf, buf2);
        if (*buf == '\0') {
            string_to_output(d, "You must specify a line number at which to change text.\n");
            return;
        }
        line_low = atoi(buf);
        strcat(buf2, "\n");

        i = 1;
        *buf = '\0';
        if ((s = *d->str) == nullptr) {
            string_to_output(d, "Buffer is empty, nothing to change.\n");
            return;
        }
        if (line_low > 0) {
            /*
             * Loop through the text counting \\n characters until we get to the line/
             */
            while (s && (i < line_low))
                if ((s = strchr(s, '\n')) != nullptr) {
                    i++;
                    s++;
                }
            /*
             * Make sure that there was a THAT line in the text.
             */
            if ((i < line_low) || (s == nullptr)) {
                string_to_output(d, "Line number out of range; change aborted.\n");
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
            if ((s = strchr(s, '\n')) != nullptr) {
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
                string_to_output(d, "Change causes new length to exceed buffer maximum size, aborted.\n");
                return;
            }
            /*
             * Change the size of the REAL buffer to fit the new text.
             */
            RECREATE(*d->str, char, strlen(buf) + 3);
            strcpy(*d->str, buf);
            string_to_output(d, "Line changed.\n");
        } else {
            string_to_output(d, "Line number must be higher than 0.\n");
            return;
        }
        break;
    default:
        string_to_output(d, "Invalid option.\n");
        log(LogSeverity::Warn, LVL_HEAD_C, "SYSERR: invalid command passed to parse_action");
        return;
    }
}

/* Add user input to the 'current' string (as defined by d->str) */
void string_add(DescriptorData *d, char *str) {
    int terminator = 0, action = 0;
    int i = 2, j = 0;
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
            terminator = 2; /* Working on an abort message, */
            break;
        case 'c':
            if (*(d->str)) {
                free(*d->str);
                *(d->str) = nullptr;
                string_to_output(d, "Current buffer cleared.\n");
            } else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 'd':
            if (*d->str)
                parse_action(PARSE_DELETE, actions, d);
            else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 'e':
            if (*d->str)
                parse_action(PARSE_EDIT, actions, d);
            else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 'f':
            if (*(d->str))
                parse_action(PARSE_FORMAT, actions, d);
            else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 'i':
            if (*(d->str))
                parse_action(PARSE_INSERT, actions, d);
            else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 'h':
            parse_action(PARSE_HELP, actions, d);
            break;
        case 'l':
            if (*d->str)
                parse_action(PARSE_LIST_NORM, actions, d);
            else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 'n':
            if (*d->str)
                parse_action(PARSE_LIST_NUM, actions, d);
            else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 'r':
            if (*d->str)
                parse_action(PARSE_REPLACE, actions, d);
            else
                string_to_output(d, "Current buffer empty.\n");
            break;
        case 's':
            terminator = 1;
            *str = '\0';
            break;
        default:
            string_to_output(d, "Invalid option.\n");
            break;
        }
    }

    /* Comparisons below this point are made with d->max_str - 3.
     * That leaves 3 spaces after the last text entered by the user: \n\0   */
    if (!(*d->str)) {
        if (strlen(str) > d->max_str - 3) {
            char_printf(d->character, "String too long - Truncated.\n");
            *(str + d->max_str) = '\0';
        }
        CREATE(*d->str, char, strlen(str) + 3);
        strcpy(*d->str, str);
        if (strlen(str))
            strcat(*d->str, "\n");
    } else if (!action) {
        if (strlen(str) + strlen(*d->str) > d->max_str - 3) {
            char_printf(d->character, "String too long, limit reached on message.  Last line ignored.\n");
            return;
        } else {
            if (!(*d->str = (char *)realloc(*d->str, strlen(*d->str) + strlen(str) + 3))) {
                perror("string_add");
                exit(1);
            }
            strcat(*d->str, str);
            strcat(*d->str, "\n");
        }
    }

    if (terminator) {
        /*. OLC Edits . */
        extern void oedit_disp_menu(DescriptorData * d);
        extern void oedit_disp_extradesc_menu(DescriptorData * d);
        extern void redit_disp_menu(DescriptorData * d);
        extern void redit_disp_extradesc_menu(DescriptorData * d);
        extern void redit_disp_exit_menu(DescriptorData * d);
        extern void medit_disp_menu(DescriptorData * d);
        extern void hedit_disp_menu(DescriptorData * d);
        extern void trigedit_disp_menu(DescriptorData * d);

        /* Check for too many lines */
        if (terminator == 1 && d->max_buffer_lines > 0 && d->str && *d->str) {
            for (i = 0, s = *d->str; s;)
                if ((s = strchr(s, '\n')) != nullptr) {
                    i++;
                    s++;
                }
            if (i > d->max_buffer_lines) {
                sprintf(buf, "The buffer has %d lines, which is over the limit of %d.\n", i, d->max_buffer_lines);
                string_to_output(d, buf);
                string_to_output(d, "Unable to save.  Use /a if you want to abort.\n");
                return;
            }
        }

        /*
         * Here we check for the abort option and reset the pointers.
         */
        if ((terminator == 2) && ((STATE(d) == CON_REDIT) || (STATE(d) == CON_MEDIT) || (STATE(d) == CON_OEDIT) ||
                                  (STATE(d) == CON_TRIGEDIT) || (STATE(d) == CON_HEDIT) || (STATE(d) == CON_EXDESC) ||
                                  (STATE(d) == CON_MEDIT))) {
            free(*d->str);
            if (d->backstr) {
                *d->str = d->backstr;
            } else
                *d->str = nullptr;

            d->backstr = nullptr;
            d->str = nullptr;
        } else if ((d->str) && (*d->str) && (**d->str == '\0')) {
            free(*d->str);
            if (STATE(d) == CON_EXDESC)
                *d->str = strdup("");
            else
                *d->str = strdup("Nothing.\n");
        }

        if (STATE(d) == CON_MEDIT)
            medit_disp_menu(d);
        if (STATE(d) == CON_TRIGEDIT)
            trigedit_disp_menu(d);
        if (STATE(d) == CON_GEDIT)
            gedit_disp_menu(d);
        if (STATE(d) == CON_OEDIT) {
            switch (OLC_MODE(d)) {
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
                if (store_mail(d->mail_to, GET_IDNUM(d->character), d->mail_vnum, *d->str))
                    string_to_output(d, "Message sent!\n");
                else
                    string_to_output(d, "Mail system error - message not sent.\n");
            } else {
                string_to_output(d, "Mail aborted.\n");
                if (d->mail_vnum != NOTHING) {
                    ObjData *obj = read_object(d->mail_vnum, VIRTUAL);
                    if (obj)
                        obj_to_char(obj, d->character);
                }
            }
            d->mail_to = 0;
            d->mail_vnum = NOTHING;
            free(*d->str);
            free(d->str);
            /*  string_to_output(d, "Message sent!\n");
               if (!IS_NPC(d->character))
               REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING); */
        } else if (STATE(d) == CON_EXDESC) {
            if (terminator != 1)
                string_to_output(d, "Description aborted.\n");
            STATE(d) = CON_PLAYING;
        } else if (!d->connected && d->character && !IS_NPC(d->character)) {
            if (terminator == 1) {
                if (strlen(*d->str) == 0) {
                    free(*d->str);
                    *d->str = nullptr;
                }
            } else {
                free(*d->str);
                if (d->backstr)
                    *d->str = d->backstr;
                else
                    *d->str = nullptr;
                d->backstr = nullptr;
                string_to_output(d, "Message aborted.\n");
            }
        }
        if (d->character && !IS_NPC(d->character)) {
            REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
            REMOVE_FLAG(PLR_FLAGS(d->character), PLR_MAILING);
        }
        if (d->backstr)
            free(d->backstr);
        d->backstr = nullptr;
        d->str = nullptr;
    }
}

/* **********************************************************************
 *  Modification of character skills                                     *
 ********************************************************************** */

/* made skillset <vict> only show nonzero skills -321 3/14/00 */
ACMD(do_skillset) {
    CharData *vict;
    char name[100], buf2[100];
    int skill, value, i, j, k;

    argument = one_argument(argument, name);

    if (!*name) { /* no arguments. print an informative text */
        char_printf(ch, "Syntax: skillset <name> '<skill>' <value>\n");
        strcpy(buf, "Skill being one of the following:\n");
        for (j = 0, k = 0; j <= TOP_SKILL; ++j) {
            i = skill_sort_info[j];
            if (*skills[i].name == '!')
                continue;
            sprintf(buf + strlen(buf), "%-18.18s ", skills[i].name);
            if (k % 4 == 3) { /* last column */
                char_printf(ch, "{}\n", buf);
                *buf = '\0';
            }
            ++k;
        }
        if (*buf)
            char_printf(ch, buf);
        char_printf(ch, "\n");
        return;
    }
    if (!(vict = find_char_around_char(ch, find_vis_by_name(ch, name)))) {
        char_printf(ch, NOPERSON);
        return;
    }
    skip_spaces(&argument);

    /* If there is no chars in argument */
    if (!*argument) {
        if (GET_LEVEL(vict) < GET_LEVEL(ch) || ch == vict) {
            strcpy(buf, "Current values of skills:\n");
            for (i = 0; i <= TOP_SKILL; ++i) {
                if (*skills[i].name == '!' || !GET_ISKILL(vict, i))
                    continue;
                sprintf(buf, "%s%-20.20s %d\n", buf, skills[i].name, GET_ISKILL(vict, i));
            }
            page_string(ch, buf);
        } else
            char_printf(ch, "You are unable to divine any information.\n");
        return;
    }

    if (*argument == '*') {
        for (i = 1; i <= TOP_SKILL; ++i) {
            SET_SKILL(vict, i, 1000);
            if (strcasecmp(skills[i].name, "!UNUSED!")) {
                char_printf(ch, "You change {}'s skill level in {} to 100.\n", GET_NAME(vict), skills[i].name);
            }
        }
        return;
    }

    argument = delimited_arg(argument, arg, '\'');

    if ((skill = find_talent_num(arg, TALENT)) <= 0) {
        char_printf(ch, "Unrecognized skill.\n");
        return;
    }

    argument = one_argument(argument, arg);

    if (!*arg) {
        char_printf(ch, "Learned value expected.\n");
        return;
    } else if ((value = atoi(arg)) < 0) {
        char_printf(ch, "Minimum value for learned is 0.\n");
        return;
    } else if (value > 1000) {
        char_printf(ch, "Max value for learned is 1000.\n");
        return;
    } else if (IS_NPC(vict)) {
        char_printf(ch, "You can't set NPC skills.\n");
        return;
    }
    log(LogSeverity::Warn, -1, "{} changed {}'s {} to {:d}.", GET_NAME(ch), GET_NAME(vict), skills[skill].name, value);

    SET_SKILL(vict, skill, value);

    char_printf(ch, "You change {}'s {} to {:d}.\n", GET_NAME(vict), skills[skill].name, value);
}

/******************/
/*   PAGINATION   */
/******************/

static int get_page_length(DescriptorData *d) {
    int page_length = 22;

    if (d) {
        if (d->original)
            page_length = GET_PAGE_LENGTH(d->original);
        else if (d->character)
            page_length = GET_PAGE_LENGTH(d->character);
        if (page_length <= 0 || page_length > 50)
            page_length = 0;
    }

    return page_length;
}

static void paging_addstr(DescriptorData *d, std::string_view str) {

    // If pagelength is set to 0, don't page
    if (!get_page_length(d)) {
        string_to_output(d, str);
        return;
    }

    // Requires GCC 12
    // for (auto line : str | std::views::split('\n')) {
    //     std::string line_str{line.begin(), line.end()};
    //     // d->page_outbuf->emplace_back(std::move(line_str));
    // }
    auto lines = str | std::views::split('\n') | std::views::transform([](auto &&s) {
                     auto subrange{s | std::views::common};
                     std::string word{subrange.begin(), subrange.end()};
                     return word;
                 });
    for (auto &&line : lines)
        d->page_outbuf->emplace_back(std::move(line));

    if (!d->page_outbuf->empty() && d->page_outbuf->back().empty())
        d->page_outbuf->pop_back();

    d->paging_numpages = (d->page_outbuf->size() + get_page_length(d) - 1) / get_page_length(d);
}

void paging_printf(CharData *ch, std::string_view messg) { paging_addstr(ch->desc, messg); }

void desc_paging_printf(DescriptorData *d, std::string_view messg) { paging_addstr(d, messg); }

void get_paging_input(DescriptorData *d, char *input) {
    one_argument(input, buf);

    /* Q is for quit. :) */
    if (to_lower(*buf) == 'q') {
        d->page_outbuf->clear();
        return;
    }
    /* R is for refresh */
    else if (to_lower(*buf) == 'r')
        // Do nothing, just refresh the page
        ;
    /* B is for back */
    else if (to_lower(*buf) == 'b')
        d->paging_curpage--;

    /* A digit: goto a page */
    else if (isdigit(*buf)) {
        int page = atoi(buf) - 1;
        if (page > 0 && page < d->paging_numpages) {
            d->paging_curpage = page;
        } else {
            char_printf(d->character, "Invalid page number.\n");
            return;
        }
    } else if (*buf) {
        /* Other input: quit the pager */
        d->page_outbuf->clear();
        return;
    } else
        /* No input: goto the next page */
        d->paging_curpage++;

    if (d->paging_curpage >= d->paging_numpages) {
        d->page_outbuf->clear();
        return;
    }

    print_current_page(d);
}

void start_paging_desc(DescriptorData *d) {
    d->paging_curpage = 0;
    print_current_page(d);

    // If we only have 1 page, don't bother paging
    if (d->paging_numpages <= 1)
        d->page_outbuf->clear();
}

void print_current_page(DescriptorData *d) {
    int page_length = get_page_length(d);
    if (d->paging_curpage < 0 || d->paging_curpage > d->paging_numpages) {
        log(LogSeverity::Error, -1, "Paging error: page {} of {}.", d->paging_curpage, d->paging_numpages);
        d->page_outbuf->clear();
        return;
    }

    auto it = d->page_outbuf->begin();
    std::advance(it, d->paging_curpage * page_length);
    for (int i = 0; i < page_length && it != d->page_outbuf->end(); i++, it++)
        char_printf(d->character, "{}\n", *it);
}

/* When you have been accumulating some text to someone and are finished,
 * start the paging process. */
void start_paging(CharData *ch) {
    if (ch->desc)
        start_paging_desc(ch->desc);
}

void page_string(CharData *ch, std::string_view str) {
    if (ch->desc)
        page_string_desc(ch->desc, str);
}

void page_string_desc(DescriptorData *d, std::string_view str) {
    paging_addstr(d, str);
    if (!d->page_outbuf->empty())
        start_paging_desc(d);
}

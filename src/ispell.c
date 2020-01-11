/***************************************************************************
 * $Id: ispell.c,v 1.12 2010/06/09 18:28:55 mud Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: ispell.c                                        Part of FieryMUD *
 *  Usage: Interface to iSpell                                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (c) 1997 Erwin S. Andreasen <erwin@pip.dknet.dk>             *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "comm.h"
#include "conf.h"
#include "db.h"
#include "interpreter.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define STRINGIFY(x) Str(x)
#define Str(x) #x
#define ISPELL_DICTIONARY "etc/mud.dictionary"

static FILE *ispell_out, *ispell_in;
static int ispell_pid = -1;
static int fiery_to_ispell[2], ispell_to_fiery[2];
bool ispell_name_check(char *);

#define ISPELL_BUF_SIZE 1024

void ispell_init(void) {
    char ignore_buf[1024];
    int i;

#if 0
    if (IS_SET(sysdata.options, OPT_NO_ISPELL)) {
	ispell_pid = -1;
	return;
    }
#endif

    pipe(fiery_to_ispell);
    pipe(ispell_to_fiery);

    ispell_pid = fork();

    if (ispell_pid < 0)
        log("ispell_init fork");

    else if (ispell_pid == 0) { /* child */
        /* Map one end of fiery_to_ispell to STDIN */
        dup2(fiery_to_ispell[0], 0);
        close(fiery_to_ispell[0]);
        close(fiery_to_ispell[1]);

        /* Map one end of ispell_to_fiery to STDOUT */
        dup2(ispell_to_fiery[1], 1);
        close(ispell_to_fiery[0]);
        close(ispell_to_fiery[1]);

        /* Close all the other files */
        for (i = 2; i < 255; i++)
            close(i);

        execlp("ispell", "ispell", "-a", "-p" ISPELL_DICTIONARY, (char *)NULL);
        exit(1);
    }

    else { /* ok ! */
        close(fiery_to_ispell[0]);
        close(ispell_to_fiery[1]);

        ispell_out = fdopen(fiery_to_ispell[1], "w");
        setbuf(ispell_out, NULL);

        ispell_in = fdopen(ispell_to_fiery[0], "r");
        setbuf(ispell_in, NULL);

#if !defined(sun) /* that ispell on sun gives no (c) msg */
        fgets(ignore_buf, 1024, ispell_in);
#endif
    }
}

void ispell_done(void) {
    if (ispell_pid != -1) {
        fprintf(ispell_out, "#\n");
        fclose(ispell_out);
        fclose(ispell_in);
        waitpid(ispell_pid, NULL, 0);
        ispell_pid = -1;
    }
}

const char *get_ispell_line(const char *word) {
    static char buf[ISPELL_BUF_SIZE];
    char throwaway[ISPELL_BUF_SIZE];

    if (ispell_pid == -1)
        return NULL;

    if (word) {
        fprintf(ispell_out, "^%s\n", word);
        fflush(ispell_out);
    }

    fgets(buf, ISPELL_BUF_SIZE, ispell_in);
    if (*buf && *buf != '\n')
        fgets(throwaway, ISPELL_BUF_SIZE, ispell_in);

    return buf;
}

void ispell_check(struct descriptor_data *d, const char *word) {
    const char *pc;

    if (!d)
        return;

    if (!*word || !(pc = get_ispell_line(word))) {
        dprintf(d, "Spellchecker failed.\r\n");
        return;
    }

    switch (*pc) {
    case '*':
    case '+': /* root */
    case '-': /* compound */
        dprintf(d, "'%s' is spelled correctly.\r\n", word);
        break;
    case '&': /* miss */
    case '?': /* guess */
        dprintf(d, "'%s' not found.  Possible words: %s", word, strchr(pc, ':') + 1);
        break;
    case '#': /* none */
        dprintf(d, "Unable to find anything that matches '%s'.\r\n", word);
        break;
    case '\n': /* no response at all */
        dprintf(d, "No response from spellchecker.\r\n");
        break;
    default:
        dprintf(d, "Unknown output from spellchecker: %s\r\n", pc);
    }
}

ACMD(do_ispell) {
    const char *pc;

    if (ispell_pid < 0) {
        send_to_char("Spellchecker is not running.\r\n", ch);
        return;
    }

    skip_spaces(&argument);
    if (!*argument || strchr(argument, ' ')) {
        send_to_char("Invalid input.\r\n", ch);
        return;
    }

    if (*argument == '+') {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            send_to_char("You may not add entries to the dictionary.", ch);
            return;
        }
        for (pc = argument + 1; *pc; ++pc)
            if (!isalpha(*pc)) {
                cprintf(ch, "'%s' contains non-alphabetic character: %c\r\n", argument, *pc);
                return;
            }
        fprintf(ispell_out, "*%s\n", argument + 1);
        fflush(ispell_out);
        return; /* we assume everything is OK.. better be so! */
    }

    ispell_check(ch->desc, argument);
}

/* Return values for the name check are 0 for no name match for
   ispell and a value that will allow the process of character creation
   to continue, and 1 for a match on a name in ispell and a failure
   to allow character creation to continue.

   return 0 = good name continue
   return 1 = bad name bail out

 */
bool ispell_name_check(char *argument) {
    const char *name;

    if (ispell_pid < 0) {
        /* ispell is not running. */
        return 0;
    }

    if (!(name = get_ispell_line(argument))) {
        /* for some reason ispell failed */
        return 0;
    }

    switch (*name) {
    case '*':
    case '+': /* root */
    case '-': /* compound */
        /* Cases for exact Correct match with dictionary */
        return 1;

        /* This case will enable harsh name checking w/o it
           Ducky or Duckman will get through.  Not sure how
           strict this will be, it may be intolerable. Some
           testing will have to be done. - RSD 8/29/2002
         */
    case '?': /* guess */
        return 1;
    case '&':     /* miss */
        return 0; /* name is good  */
    case '#':     /* none */
        return 0;
    default:
        /* log this later */
        return 0;
    }
}

/***************************************************************************
 * $Log: ispell.c,v $
 * Revision 1.12  2010/06/09 18:28:55  mud
 * Catch attempts to spellcheck numbers and punctuation that
 * would cause the mud to hang waiting for a response from the
 * spellchecker.
 *
 * Revision 1.11  2009/02/12 04:44:30  myc
 * Added rudimentary spellchecker to text editor; uses ispell.
 *
 * Revision 1.10  2009/02/12 03:24:45  myc
 * Rewrote ispell to use file descriptors.  Now it works.
 *
 * Revision 1.9  2008/08/14 23:10:35  myc
 * Removed a suspicious %s from a log() call.
 *
 * Revision 1.8  2002/09/14 01:20:28  jjl
 * Fixed the bug gthat  ARGH stupid backspacke key.  Fixed the
 * bug that blocked out names like Tanle.  Well, nbbug is a bad way
 * to put it.  It was to strict.  Now it shouldn't be.
 *
 * Revision 1.7  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.6  2002/08/29 17:36:16  rsd
 * added and prototyped a new function ispell_name_check to
 * be used to check potential player names versus the muds
 * dictionary.  This may require some tweaking as the checks
 * can be harsh or light.
 *
 * Revision 1.5  2000/11/22 19:41:14  rsd
 * Lawd, I retabbed the entire file, it was a disaster.
 * I also added back rlog messages from prior to the
 * addition of the $log$.
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list.  This code compiles fine under both gcc RH5.2 and egcs RH6.0
 *
 * Revision 1.2  1999/01/31 15:55:02  mud
 * Added standard comment header
 * indented file to the right some
 *
 * Revision 1.1  mud
 * Initial revision
 *
 ***************************************************************************/

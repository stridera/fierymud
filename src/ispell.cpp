/***************************************************************************
 *  File: ispell.c                                        Part of FieryMUD *
 *  Usage: Interface to iSpell                                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (c) 1997 Erwin S. Andreasen <erwin@pip.dknet.dk>             *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

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

    if (pipe(fiery_to_ispell) == -1) {
        log("Error creating pipe: fiery_to_ispell");
        return;
    }
    if (pipe(ispell_to_fiery) == -1) {
        log("Error creating pipe: ispell_to_fiery");
        return;
    }

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

        execlp("ispell", "ispell", "-a", "-p" ISPELL_DICTIONARY, (char *)nullptr);
        exit(1);
    }

    else { /* ok ! */
        close(fiery_to_ispell[0]);
        close(ispell_to_fiery[1]);

        ispell_out = fdopen(fiery_to_ispell[1], "w");
        setbuf(ispell_out, nullptr);

        ispell_in = fdopen(ispell_to_fiery[0], "r");
        setbuf(ispell_in, nullptr);
    }
}

void ispell_done(void) {
    if (ispell_pid != -1) {
        fprintf(ispell_out, "#\n");
        fclose(ispell_out);
        fclose(ispell_in);
        waitpid(ispell_pid, nullptr, 0);
        ispell_pid = -1;
    }
}

const char *get_ispell_line(const char *word) {
    static char buf[ISPELL_BUF_SIZE];
    char throwaway[ISPELL_BUF_SIZE];

    if (ispell_pid == -1)
        return nullptr;

    if (word) {
        fprintf(ispell_out, "^%s\n", word);
        fflush(ispell_out);
    }

    if (fgets(buf, ISPELL_BUF_SIZE, ispell_in) == nullptr) {
        log("Error reading from ispell_in");
        return nullptr;
    }
    if (*buf && *buf != '\n') {
        if (fgets(throwaway, ISPELL_BUF_SIZE, ispell_in) == nullptr) {
            log("Error reading from ispell_in");
        }
    }

    return buf;
}

void ispell_check(DescriptorData *d, const char *word) {
    const char *pc;

    if (!d)
        return;

    if (!*word || !(pc = get_ispell_line(word))) {
        desc_printf(d, "Spellchecker failed.\n");
        return;
    }

    switch (*pc) {
    case '*':
    case '+': /* root */
    case '-': /* compound */
        desc_printf(d, "'{}' is spelled correctly.\n", word);
        break;
    case '&': /* miss */
    case '?': /* guess */
        desc_printf(d, "'{}' not found.  Possible words: {}", word, strchr(pc, ':') + 1);
        break;
    case '#': /* none */
        desc_printf(d, "Unable to find anything that matches '{}'.\n", word);
        break;
    case '\n': /* no response at all */
        desc_printf(d, "No response from spellchecker.\n");
        break;
    default:
        desc_printf(d, "Unknown output from spellchecker: {}\n", pc);
    }
}

ACMD(do_ispell) {
    const char *pc;

    if (ispell_pid < 0) {
        char_printf(ch, "Spellchecker is not running.\n");
        return;
    }

    skip_spaces(&argument);
    if (!*argument || strchr(argument, ' ')) {
        char_printf(ch, "Invalid input.\n");
        return;
    }

    if (*argument == '+') {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            char_printf(ch, "You may not add entries to the dictionary.\n");
            return;
        }
        for (pc = argument + 1; *pc; ++pc)
            if (!isalpha(*pc)) {
                char_printf(ch, "'{}' contains non-alphabetic character: {:c}\n", argument, *pc);
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

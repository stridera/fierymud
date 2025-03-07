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

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <iostream>

/* extern variables */

/* extern functions */
std::string_view fread_action(FILE *fl, int nr);

/* local globals */
static int list_top = -1;

/* gawd this is a hack, someone please fix it! */
#define MAX_SOCIALS 200

struct social_messg {
    int act_nr;
    int hide;
    int min_victim_position; /* Position of victim */

    /* No argument was supplied */
    std::string_view char_no_arg;
    std::string_view others_no_arg;

    /* An argument was there, and a victim was found */
    std::string_view char_found; /* if NULL, read no further, ignore args */
    std::string_view others_found;
    std::string_view vict_found;

    /* An argument was there, but no victim was found */
    std::string_view not_found;

    /* The victim turned out to be the character */
    std::string_view char_auto;
    std::string_view others_auto;
} *soc_mess_list[MAX_SOCIALS];

int find_action(int cmd) {
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

ACMD(do_action) {
    int act_nr;
    CharData *vict;

    if ((act_nr = find_action(cmd)) < 0) {
        char_printf(ch, "That action is not supported.\n");
        return;
    }
    social_messg *action = soc_mess_list[act_nr];

    if (action->char_found.empty() || argument.empty()) {
        char_printf(ch, action->char_no_arg);
        char_printf(ch, "\n");
        if (!action->others_no_arg.empty())
            act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
        return;
    }

    auto arg = argument.shift();
    if (!(vict = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg)))) {
        char_printf(ch, action->not_found);
        char_printf(ch, "\n");
    } else if (vict == ch) {
        char_printf(ch, action->char_auto);
        char_printf(ch, "\n");
        if (!action->others_auto.empty())
            act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
    } else {
        if (GET_POS(vict) < action->min_victim_position)
            act("$N is not in a proper position for that.", false, ch, 0, vict, TO_CHAR | TO_SLEEP);
        else {
            act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
            act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
            act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
        }
    }
}

ACMD(do_insult) {
    CharData *victim;

    auto arg = argument.shift();

    if (!arg.empty()) {
        if (!(victim = find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, arg))))
            char_printf(ch, "Can't hear you!\n");
        else {
            if (victim != ch) {
                char_printf(ch, "You insult {}.\n", GET_NAME(victim));

                switch (random_number(0, 2)) {
                case 0:
                    if (GET_SEX(ch) == SEX_MALE) {
                        if (GET_SEX(victim) == SEX_MALE)
                            act("$n accuses you of fighting like a brownie!", false, ch, 0, victim, TO_VICT);
                        else
                            act("$n tells you that you'd lose a beauty contest against a troll.", false, ch, 0, victim,
                                TO_VICT);
                    } else { /* Ch == Woman */
                        if (GET_SEX(victim) == SEX_MALE)
                            act("$n accuses you of having the smallest... (brain?)", false, ch, 0, victim, TO_VICT);
                    }
                    break;
                case 1:
                    act("$n calls your mother a bitch!", false, ch, 0, victim, TO_VICT);
                    break;
                default:
                    act("$n tells you to get lost!", false, ch, 0, victim, TO_VICT);
                    break;
                } /* end switch */

                act("$n insults $N.", true, ch, 0, victim, TO_NOTVICT);
            } else { /* ch == victim */
                char_printf(ch, "You feel insulted.\n");
            }
        }
    } else
        char_printf(ch, "I'm sure you don't want to insult *everybody*...\n");
}

std::string_view fread_action(FILE *fl, int nr) {
    char buf[MAX_STRING_LENGTH], *rslt;

    fgets(buf, MAX_STRING_LENGTH, fl);
    if (feof(fl)) {
        fprintf(stderr, "fread_action - unexpected EOF near action #%d", nr);
        exit(1);
    }
    if (*buf == '#')
        return {};
    else {
        return buf;
    }
}

void boot_social_messages(void) {
    FILE *fl;
    int nr, i, hide, min_pos, curr_soc = -1;
    char next_soc[100];
    social_messg *temp;

    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE.data(), "r"))) {
        std::cerr << "Can't open socials file " << SOCMESS_FILE << std::endl;
        exit(1);
    }
    /* count socials & allocate space */
    for (nr = 0; cmd_info[nr].command != "\n"; nr++)
        if (cmd_info[nr].command_pointer == do_action) {
            list_top++;
        }

    /* now read 'em */
    for (;;) {
        fscanf(fl, " %s ", next_soc);
        if (*next_soc == '$')
            break;
        if ((nr = find_command(next_soc)) < 0) {
            log("Unknown social '{}' in social file", next_soc);
        }
        if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) {
            fprintf(stderr, "Format error in social file near social '%s'\n", next_soc);
            exit(1);
        }
        /* read the stuff */
        curr_soc++;
        CREATE(soc_mess_list[curr_soc], social_messg, 1);
        soc_mess_list[curr_soc]->act_nr = nr;
        soc_mess_list[curr_soc]->hide = hide;
        soc_mess_list[curr_soc]->min_victim_position = min_pos;

        soc_mess_list[curr_soc]->char_no_arg = fread_action(fl, nr);
        soc_mess_list[curr_soc]->others_no_arg = fread_action(fl, nr);
        soc_mess_list[curr_soc]->char_found = fread_action(fl, nr);

        /* if no char_found, the rest is to be ignored */
        if (soc_mess_list[curr_soc]->char_found.empty())
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

void free_social_messages() {
    social_messg *mess;
    int i;

    for (i = 0; i <= list_top; i++) {
        mess = soc_mess_list[i];
        free(mess);
    }
}

/***************************************************************************
 *   File: dg_comm.c                                      Part of FieryMUD *
 *  Usage: Who knows?                                                      *
 *     By: Unknown                                                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 2000 by the Fiery Consortium                    *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "handler.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* same as any_one_arg except that it stops at punctuation */
char *any_one_name(char *argument, char *first_arg) {
    char *arg;

    /* Find first non blank */
    while (isspace(*argument))
        ++argument;

    /* Find length of first word */
    for (arg = first_arg;
         *argument && !isspace(*argument) && (!ispunct(*argument) || *argument == '#' || *argument == '-');
         arg++, argument++)
        *arg = LOWER(*argument);
    *arg = '\0';

    return argument;
}

void sub_write_to_char(char_data *ch, char *tokens[], char_data *ctokens[], obj_data *otokens[], char type[]) {
    char sb[MAX_STRING_LENGTH];
    int i;

    strcpy(sb, "");

    for (i = 0; tokens[i + 1]; i++) {
        strcat(sb, tokens[i]);
        /* changed everything to either c or o tokens respectively --gurlaek */
        switch (type[i]) {
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
            else {
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

void sub_write(char *arg, char_data *ch, byte find_invis, int targets) {
    char str[MAX_INPUT_LENGTH * 2];
    char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    char *tokens[MAX_INPUT_LENGTH], *s, *p;
    char_data *ctokens[MAX_INPUT_LENGTH];
    obj_data *otokens[MAX_INPUT_LENGTH];

    char_data *to;
    obj_data *obj;
    int i;
    int sleep = 1; /* mainly for windows compiles */
    int olc = 0;

    if (!arg)
        return;

    tokens[0] = str;

    for (i = 0, p = arg, s = str; *p;) {
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
            ctokens[i] = find_invis ? find_char_in_world(find_by_name(name))
                                    : find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, name));
            tokens[++i] = ++s;
            break;

        case '`':
            /* get obj_data, move to next token */
            type[i] = *p;
            *s = '\0';
            ++p;
            p = any_one_name(++p, name);
            if (find_invis)
                obj = find_obj_in_world(find_by_name(name));
            else if (!(obj = find_obj_in_list(world[IN_ROOM(ch)].contents, find_vis_by_name(ch, name))))
                ;
            else if (!(obj = find_obj_in_eq(ch, NULL, find_vis_by_name(ch, name))))
                ;
            else
                obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, name));
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

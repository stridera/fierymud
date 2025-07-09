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
#include "logging.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

void sub_write_to_char(CharData *ch, std::string_view tokens[], CharData *ctokens[], ObjData *otokens[], char type[]) {
    char sb[MAX_STRING_LENGTH];
    int i;

    strcpy(sb, "");

    for (i = 0; !tokens[i + 1].empty(); i++) {
        strcat(sb, tokens[i].data());
        /* changed everything to either c or o tokens respectively --gurlaek */
        switch (type[i]) {
        case '~':
            if (!ctokens[i])
                strcat(sb, "someone");
            else if (ctokens[i] == ch)
                strcat(sb, "you");
            else
                strcat(sb, PERS(ctokens[i], ch).c_str());
            break;

        case '@':
            if (!ctokens[i])
                strcat(sb, "someone's");
            else if (ctokens[i] == ch)
                strcat(sb, "your");
            else {
                strcat(sb, PERS(ctokens[i], ch).c_str());
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
                strcat(sb, OBJS(otokens[i], ch).c_str());
            break;
        }
    }

    strcat(sb, tokens[i].data());
    strcat(sb, "\n");

    /* Want to capitalize_first it... by passing it through CAP, which will
     * skip past any &D color codes.
     * However, if it starts with &0, that's the signal that the
     * script writer does not want it capitalize_firstd. */
    if ((sb[0] == CREL || sb[0] == CABS) && sb[1] == '0')
        char_printf(ch, sb);
    else
        char_printf(ch, capitalize_first(sb));
}

void sub_write(std::string_view arg, CharData *ch, byte find_invis, int targets) {
    // TODO: Implement sub_write function - currently stubbed out due to complex string parsing
    return;
    
    // COMMENTED OUT - complex string parsing needs to be rewritten for C++
#if 0
    char str[MAX_INPUT_LENGTH * 2];
    char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    std::string_view tokens[MAX_INPUT_LENGTH];
    std::string_view p;
    char *s;
    CharData *ctokens[MAX_INPUT_LENGTH];
    ObjData *otokens[MAX_INPUT_LENGTH];

    CharData *to;
    ObjData *obj;
    int i;
    int olc = 0;

    if (arg.empty())
        return;

    tokens[0] = str;

    for (i = 0, p = arg, s = str; !p.empty();) {
        ctokens[i] = nullptr;
        otokens[i] = nullptr;
        switch (p.front()) {
        case '~':
        case '@':
        case '^':
        case '>':
        case '*':
            /* get CharData, move to next token */
            type[i] = *p;
            *s = '\0';
            ++p;
            p = any_one_name(p, name);
            ctokens[i] = find_invis ? find_char_in_world(find_by_name(name))
                                    : find_char_in_room(&world[ch->in_room], find_vis_by_name(ch, name));
            tokens[++i] = ++s;
            break;

        case '`':
            /* get ObjData, move to next token */
            type[i] = *p;
            *s = '\0';
            ++p;
            p = any_one_name(++p, name);
            if (find_invis)
                obj = find_obj_in_world(find_by_name(name));
            else if (!(obj = find_obj_in_list(world[IN_ROOM(ch)].contents, find_vis_by_name(ch, name))))
                ;
            else if (!(obj = find_obj_in_eq(ch, nullptr, find_vis_by_name(ch, name))))
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
    tokens[++i] = nullptr;

    if (IS_SET(targets, TO_CHAR) && SENDOK(ch))
        sub_write_to_char(ch, tokens, ctokens, otokens, type);

    if (IS_SET(targets, TO_ROOM))
        for (to = world[ch->in_room].people; to; to = to->next_in_room)
            if (to != ch && SENDOK(to))
                sub_write_to_char(to, tokens, ctokens, otokens, type);
#endif
}

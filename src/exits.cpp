/***************************************************************************
 *   File: exits.c                                       Part of FieryMUD  *
 *  Usage: Functions for managing room exits                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "exits.hpp"

#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "directions.hpp"
#include "handler.hpp"
#include "math.hpp"
#include "rooms.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

struct Exit *create_exit(int dest_room) {
    struct Exit *e;

    CREATE(e, exit, 1);

    e->key = -1;
    e->to_room = dest_room;

    return e;
}

bool exit_has_keyword(exit *exit, char *name) {
    if (exit->keyword && *(exit->keyword))
        return isname(name, exit->keyword);
    return FALSE;
}

struct Exit *opposite_exit(exit *exit, room_num roomvnum, int dir) {
    struct room_data *room;
    struct exit *oe;
    int idir;

    if ((room = EXIT_DEST(exit))) {
        if ((oe = room->exits[rev_dir[dir]]) && EXIT_NDEST(oe) == roomvnum)
            return oe;
        for (idir = 0; idir < NUM_OF_DIRS; idir++)
            if ((oe = room->exits[idir]) && EXIT_NDEST(oe) == roomvnum)
                return oe;
    }

    return NULL;
}

char *exit_name(exit *exit) { return exit->keyword ? fname(exit->keyword) : "door"; }

#define SHOW_EXDESC_LEN 57

char *exit_dest_desc(exit *e) {
    static char buf[MAX_STRING_LENGTH];
    int nlpos, len, showlen;

    if (!e)
        return "";
    if (e->exit_info & EX_DESCRIPT) {
        len = strlen(e->general_description);
        for (nlpos = 0; nlpos < len; nlpos++)
            if (e->general_description[nlpos] == '\r' || e->general_description[nlpos] == '\n') {
                break;
            }
        showlen = MIN(nlpos, SHOW_EXDESC_LEN) + 3;
        snprintf(buf, showlen, "&3%s", e->general_description);
        if (showlen < nlpos + 3 || nlpos <= len - 3)
            sprintf(buf, "%s...&0", buf);
        else
            sprintf(buf, "%s&0", buf);
        return buf;
    }
    if (e->to_room == NOWHERE)
        return "&1&bnowhere&0";
    if (e->to_room < 0 || e->to_room >= top_of_world)
        sprintf(buf, "&1&RNUM OUT OF RANGE: %d&0", e->to_room);
    else
        sprintf(buf, "%d", world[e->to_room].vnum);
    return buf;
}

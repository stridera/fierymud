/***************************************************************************
 *   File: rooms.c                                       Part of FieryMUD  *
 *  Usage: Functions for managing rooms                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "directions.hpp"
#include "exits.hpp"
#include "handler.hpp"
#include "math.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* act.movement.c */
void cantgo_msg(CharData *ch, int dir) {
    char *bumpinto = nullptr;

    if (!CONFUSED(ch)) {
        send_to_char("Alas, you cannot go that way...\r\n", ch);
        return;
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
        bumpinto = "a wall";
    else
        switch (SECT(ch->in_room)) {
        case SECT_CITY:
            switch (number(0, 1)) {
            case 0:
                bumpinto = "a wall";
                break;
            default:
                bumpinto = "a fence";
                break;
            }
            break;
        case SECT_FIELD:
        case SECT_GRASSLANDS:
            bumpinto = "a large bush";
            break;
        case SECT_FOREST:
        case SECT_SWAMP:
            switch (number(0, 3)) {
            case 0:
                bumpinto = "a large bush";
                break;
            case 1:
                bumpinto = "a nearby rock";
                break;
            default:
                bumpinto = "a tree";
                break;
            }
            break;
        case SECT_HILLS:
        case SECT_MOUNTAIN:
            switch (number(0, 5)) {
            case 0:
                bumpinto = "an old stump";
                break;
            case 1:
                bumpinto = "a boulder";
                break;
            case 2:
                bumpinto = "a large rock";
                break;
            case 3:
                bumpinto = "a scraggly tree";
                break;
            default:
                bumpinto = "a steep hillside";
                break;
            }
            break;
        case SECT_SHALLOWS:
            switch (number(0, 4)) {
            case 0:
                bumpinto = "a steep bank";
                break;
            case 1:
                bumpinto = "a submerged log";
                break;
            case 2:
                bumpinto = "some tree trunks";
                break;
            default:
                bumpinto = "an embankment";
                break;
            }
            break;
        case SECT_WATER:
            switch (number(0, 3)) {
            case 0:
                bumpinto = "a crashing wave";
                break;
            case 1:
                bumpinto = "a spinning eddy";
                break;
            default:
                bumpinto = "a powerful current";
                break;
            }
            break;
        case SECT_UNDERWATER:
            switch (number(0, 4)) {
            case 0:
                bumpinto = "a spinning eddy";
                break;
            case 1:
                bumpinto = "a submerged whirlpool";
                break;
            case 2:
                bumpinto = "an underwater cliff";
                break;
            case 3:
                bumpinto = "a wall";
                break;
            default:
                bumpinto = "a powerful current";
                break;
            }
            break;
        case SECT_AIR:
            switch (number(0, 2)) {
            case 0:
                bumpinto = "a gust of wind";
                break;
            default:
                bumpinto = "a blustery gale";
                break;
            }
            break;
        case SECT_BEACH:
            switch (number(0, 2)) {
            case 0:
                bumpinto = "a steep sand dune";
                break;
            case 1:
                bumpinto = "a massive boulder";
                break;
            default:
                bumpinto = "a huge tuft of sandgrass";
                break;
            }
            break;
        case SECT_CAVE:
            switch (number(0, 2)) {
            case 0:
                bumpinto = "a huge stalagmite";
                break;
            case 1:
                bumpinto = "a pile of stones";
                break;
            case 2:
                bumpinto = "a wall of rock";
                break;
            }
            break;
        case SECT_ROAD:
        case SECT_RUINS:
        case SECT_STRUCTURE:
        case SECT_UNDERDARK:
        default:
            bumpinto = "a wall";
            break;
        }

    if (bumpinto) {
        act("Oops!  You bumped into $T!", false, ch, 0, bumpinto, TO_CHAR);
        sprintf(buf, "$n tried to walk away and bumped into %s!", bumpinto);
        act(buf, true, ch, 0, 0, TO_ROOM);
    }
}

#define CANNOT_GO                                                                                                      \
    do {                                                                                                               \
        if (!quiet)                                                                                                    \
            cantgo_msg(ch, dir);                                                                                       \
        return false;                                                                                                  \
    } while (0)

bool check_can_go(CharData *ch, int dir, bool quiet) {
    Exit *exit;

    exit = CH_EXIT(ch, dir);

    /* There's no exit at all this way. */
    if (!exit || EXIT_IS_DESCRIPTION(exit) || exit->to_room == NOWHERE)
        CANNOT_GO;

    /* Exits may be temporarily impassable (hidden, closed door).
     * But gods ignore these conditions. */
    if (GET_LEVEL(ch) < LVL_GOD) {
        if (EXIT_IS_HIDDEN(exit))
            CANNOT_GO;
        if (EXIT_IS_CLOSED(exit)) {
            if (!quiet) {
                if (exit->keyword && *(exit->keyword)) {
                    sprintf(buf, "The %s seem%s to be closed.\r\n", fname(exit->keyword),
                            isplural(exit->keyword) ? "" : "s");
                    send_to_char(buf, ch);
                } else {
                    sprintf(buf, "SYSERR: room %d, exit %d has no keyword", CH_ROOM(ch)->vnum, dir);
                    mudlog(buf, BRF, LVL_GOD, true);
                    send_to_char("It seems to be closed.\r\n", ch);
                }
            }
            return false;
        }
    }

    return true;
}

/* open_door
 *
 * A character is attempting to open a door inside a room.
 *
 * Assumption: The character's ability to detect the door has
 *             already been verified.
 */
void open_door(CharData *ch, room_num roomnum, int dir, bool quiet) {
    Exit *exit, *oexit;

    if (!(exit = world[roomnum].exits[dir]))
        return;

    /* The exit must:
     *
     * 1. Be a door
     * 2. Be closed
     * 3. Be unlocked
     */

    if (!EXIT_IS_DOOR(exit)) {
        if (!quiet && ch)
            send_to_char("You can't open that.\r\n", ch);
        return;
    }

    if (EXIT_IS_OPEN(exit)) {
        if (!quiet && ch)
            send_to_char("It's already open.\r\n", ch);
        return;
    }

    if (EXIT_IS_LOCKED(exit)) {
        if (!quiet && ch)
            send_to_char("It seems to be locked.\r\n", ch);
        return;
    }

    /* Success */

    REMOVE_BIT(exit->exit_info, EX_LOCKED | EX_CLOSED | EX_HIDDEN);

    /* Open the opposite door */

    if ((oexit = opposite_exit(exit, roomnum, dir)) && EXIT_IS_DOOR(oexit)) {
        REMOVE_BIT(oexit->exit_info, EX_LOCKED | EX_CLOSED | EX_HIDDEN);

        /* Feedback to the other room */

        if (!quiet) {
            sprintf(buf, "The %s is opened from the other side.\r\n", exit_name(oexit));
            send_to_room(buf, exit->to_room);
            send_gmcp_room(ch);
        }
    }

    /* Feedback to this room */

    if (ch && !quiet) {
        send_to_char(OK, ch);
        sprintf(buf, "$n opens the %s.", exit_name(exit));
        act(buf, false, ch, 0, 0, TO_ROOM);
        send_gmcp_room(ch);
    }
}

void close_door(CharData *ch, room_num roomnum, int dir, bool quiet) {
    Exit *exit, *oexit;

    if (!(exit = world[roomnum].exits[dir]))
        return;

    /* The exit must:
     *
     * 1. Be a door
     * 2. Be open
     */

    if (!EXIT_IS_DOOR(exit)) {
        if (!quiet && ch)
            send_to_char("You can't close that.\r\n", ch);
        return;
    }

    if (EXIT_IS_CLOSED(exit)) {
        if (!quiet && ch)
            send_to_char("It's already closed.\r\n", ch);
        return;
    }

    /* Success */

    REMOVE_BIT(exit->exit_info, EX_LOCKED | EX_HIDDEN);
    SET_BIT(exit->exit_info, EX_CLOSED);

    /* Close the opposite door */

    if ((oexit = opposite_exit(exit, roomnum, dir)) && EXIT_IS_DOOR(oexit)) {
        REMOVE_BIT(oexit->exit_info, EX_LOCKED | EX_HIDDEN);
        SET_BIT(oexit->exit_info, EX_CLOSED);

        /* Feedback to the other room */

        if (!quiet) {
            sprintf(buf, "The %s is closed from the other side.\r\n", exit_name(oexit));
            send_to_room(buf, exit->to_room);
        }
    }

    /* Feedback to this room */

    if (ch && !quiet) {
        send_to_char(OK, ch);
        sprintf(buf, "$n closes the %s.", exit_name(exit));
        act(buf, false, ch, 0, 0, TO_ROOM);
        send_gmcp_room(ch);
    }
}

void unlock_door(CharData *ch, room_num roomnum, int dir, bool quiet) {
    Exit *exit, *oexit;
    int keyvnum;
    ObjData *key = nullptr;

    if (!(exit = world[roomnum].exits[dir]))
        return;

    /* The exit must:
     *
     * 1. Be a door
     * 2. Be closed
     * 3. Be locked
     */

    if (!EXIT_IS_DOOR(exit)) {
        if (!quiet && ch)
            send_to_char("You can't unlock that.\r\n", ch);
        return;
    }

    if (EXIT_IS_OPEN(exit)) {
        if (!quiet && ch)
            send_to_char("It isn't even closed!\r\n", ch);
        return;
    }

    /* You may need a key, if you're not an immortal. */

    if (ch && GET_LEVEL(ch) < LVL_IMMORT) {
        keyvnum = exit->key;
        if (keyvnum < 0) {
            if (!quiet)
                send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
            return;
        }
        if (!(key = carried_key(ch, keyvnum))) {
            if (!quiet)
                send_to_char("You don't seem to have the proper key.\r\n", ch);
            return;
        }
    }

    /* You wouldn't know that it's unlocked unless you had the key,
     * so this check comes after key verification. */

    if (!EXIT_IS_LOCKED(exit)) {
        if (!quiet && ch) {
            send_to_char("Oh... it wasn't locked, after all.\r\n", ch);
            if (key)
                act("$n inserts $p into the lock on the $F, but finds that it wasn't "
                    "locked.",
                    false, ch, key, exit_name(exit), TO_ROOM);
        }
        return;
    }

    /* Success */

    REMOVE_BIT(exit->exit_info, EX_LOCKED | EX_HIDDEN);

    /* Unlock opposite door, if it uses the same key */

    if ((oexit = opposite_exit(exit, roomnum, dir)) && EXIT_IS_DOOR(oexit) && EXIT_IS_LOCKED(oexit) &&
        oexit->key == exit->key) {
        REMOVE_BIT(oexit->exit_info, EX_LOCKED | EX_HIDDEN);

        /* Feedback to the other room */

        if (!quiet) {
            sprintf(buf, "The %s is unlocked from the other side.\r\n", exit_name(oexit));
            send_to_room(buf, exit->to_room);
        }
    }

    /* Feedback to this room */

    if (ch && !quiet) {
        if (key) {
            sprintf(buf, "*Click*  You unlock the %s with $p.", exit_name(exit));
            act(buf, false, ch, key, 0, TO_CHAR);
            sprintf(buf, "$n unlocks the %s with $p.", exit_name(exit));
            act(buf, false, ch, key, 0, TO_ROOM);
        } else {
            sprintf(buf, "*Click*  You unlock the %s.", exit_name(exit));
            act(buf, false, ch, 0, 0, TO_CHAR);
            sprintf(buf, "$n unlocks the %s.", exit_name(exit));
            act(buf, false, ch, 0, 0, TO_ROOM);
        }

        send_gmcp_room(ch);
    }
}

void lock_door(CharData *ch, room_num roomnum, int dir, bool quiet) {
    Exit *exit, *oexit;
    int keyvnum;
    ObjData *key = nullptr;

    if (!(exit = world[roomnum].exits[dir]))
        return;

    /* The exit must:
     *
     * 1. Be a door
     * 2. Be closed
     * 3. Be unlocked
     */

    if (!EXIT_IS_DOOR(exit)) {
        if (!quiet && ch)
            send_to_char("You can't unlock that.\r\n", ch);
        return;
    }

    if (EXIT_IS_OPEN(exit)) {
        if (!quiet && ch)
            send_to_char("You'll have to close it first.\r\n", ch);
        return;
    }

    /* You may need a key, if you're not an immortal. */

    if (ch && GET_LEVEL(ch) < LVL_IMMORT) {
        keyvnum = exit->key;
        if (keyvnum < 0) {
            if (!quiet)
                send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
            return;
        }
        if (!(key = carried_key(ch, keyvnum))) {
            if (!quiet)
                send_to_char("You don't seem to have the proper key.\r\n", ch);
            return;
        }
    }

    /* You wouldn't know that it's locked unless you had the key,
     * so this check comes after key verification. */

    if (EXIT_IS_LOCKED(exit)) {
        if (!quiet && ch) {
            send_to_char("It seems to be locked already.\r\n", ch);
            if (key)
                act("$n inserts $p into the lock on the $F, but finds that it was "
                    "already locked.",
                    false, ch, key, exit_name(exit), TO_ROOM);
        }
        return;
    }

    /* Success */

    REMOVE_BIT(exit->exit_info, EX_HIDDEN);
    SET_BIT(exit->exit_info, EX_LOCKED);

    /* Lock the opposite door, if it uses the same key. */

    if ((oexit = opposite_exit(exit, roomnum, dir)) && EXIT_IS_DOOR(oexit) && !EXIT_IS_LOCKED(oexit) &&
        oexit->key == exit->key) {
        REMOVE_BIT(oexit->exit_info, EX_HIDDEN);
        SET_BIT(oexit->exit_info, EX_LOCKED);

        /* Feedback to the other room. */

        if (!quiet) {
            sprintf(buf, "The %s is locked from the other side.\r\n", exit_name(oexit));
            send_to_room(buf, exit->to_room);
        }
    }

    /* Feedback to this room. */

    if (ch && !quiet) {
        if (key) {
            sprintf(buf, "*Click*  You lock the %s with $p.", exit_name(exit));
            act(buf, false, ch, key, 0, TO_CHAR);
            sprintf(buf, "$n locks the %s with $p.", exit_name(exit));
            act(buf, false, ch, key, 0, TO_ROOM);
        } else {
            sprintf(buf, "*Click*  You lock the %s.", exit_name(exit));
            act(buf, false, ch, 0, 0, TO_CHAR);
            sprintf(buf, "$n locks the %s.", exit_name(exit));
            act(buf, false, ch, 0, 0, TO_ROOM);
        }

        send_gmcp_room(ch);
    }
}

void pick_door(CharData *ch, room_num roomnum, int dir) {
    Exit *exit, *oexit;

    if (!(exit = world[roomnum].exits[dir]))
        return;

    if (!ch) {
        mudlog("SYSERR: pick_door() called with no actor", BRF, LVL_IMMORT, false);
        return;
    }

    /* The exit must:
     *
     * 1. Be a door
     * 2. Be closed
     * 3. Be locked
     * 4. Be pickable
     */

    if (!EXIT_IS_DOOR(exit)) {
        send_to_char("You can't pick that.\r\n", ch);
        return;
    }

    if (EXIT_IS_OPEN(exit)) {
        send_to_char("It isn't even closed!\r\n", ch);
        return;
    }

    if (!EXIT_IS_LOCKED(exit)) {
        act("$n goes to pick the lock on the $T, but discovers that it isn't "
            "locked.",
            false, ch, 0, exit_name(exit), TO_ROOM);
        send_to_char("Oh... it wasn't locked, after all.\r\n", ch);
        return;
    }

    if (EXIT_IS_PICKPROOF(exit)) {
        act("$n attempts to pick the lock on the $T.", false, ch, 0, exit_name(exit), TO_ROOM);
        send_to_char("It resists your attempts to pick it.\r\n", ch);
        return;
    }

    /* Try your skill. */

    if (number(1, 101) > GET_SKILL(ch, SKILL_PICK_LOCK)) {
        act("$n attempts to pick the lock on the $T.", false, ch, 0, exit_name(exit), TO_ROOM);
        send_to_char("You failed to pick the lock.\r\n", ch);
        improve_skill(ch, SKILL_PICK_LOCK);
        return;
    }

    /* Success. */

    REMOVE_BIT(exit->exit_info, EX_LOCKED | EX_HIDDEN);

    /* Unlock the opposite door, if it uses the same key. */

    if ((oexit = opposite_exit(exit, roomnum, dir)) && EXIT_IS_DOOR(oexit) && !EXIT_IS_LOCKED(oexit) &&
        oexit->key == exit->key) {
        REMOVE_BIT(exit->exit_info, EX_LOCKED | EX_HIDDEN);
        /* There is no notification to the other room.
         * Perhaps if we differentiated between quiet and loud sounds,
         * and someone could hear well... */
    }

    /* Feedback to this room. */

    send_to_char("The lock yields to your skills.\r\n", ch);
    sprintf(buf, "$n skillfully picks the lock on the %s.", exit_name(exit));
    act(buf, false, ch, 0, 0, TO_ROOM);
    send_gmcp_room(ch);

    /* Skill improvement. */

    improve_skill(ch, SKILL_PICK_LOCK);
}

bool can_see_exit(CharData *ch, int roomnum, Exit *exit) {
    return !((EXIT_IS_HIDDEN(exit) && GET_LEVEL(ch) < LVL_IMMORT) ||
             (IS_DARK(exit->to_room) && IS_DARK(roomnum) && !CAN_SEE_IN_DARK(ch)));
}

void send_auto_exits(CharData *ch, int roomnum) {
    int dir;
    RoomData *room, *dest;
    Exit *exit;

    room = &world[roomnum];
    *buf = '\0';

    if (ROOM_EFF_FLAGGED(roomnum, ROOM_EFF_ISOLATION)) {
        if (GET_LEVEL(ch) >= LVL_IMMORT)
            strcpy(buf, " &5&b<&0&5isolation&b>&0");
        else {
            send_to_char("&2Obvious exits: &6None&0.\r\n", ch);
            return;
        }
    }

    for (dir = 0; dir < NUM_OF_DIRS; dir++) {
        if ((exit = room->exits[dir]) && ((dest = EXIT_DEST(exit))) && can_see_exit(ch, roomnum, exit)) {
            sprintf(buf, "%s &0-&6%s", buf, capdirs[dir]);
            if (EXIT_IS_CLOSED(exit))
                sprintf(buf, "%s#", buf);
            if (EXIT_IS_HIDDEN(exit))
                sprintf(buf, "%s(hidden)", buf);
        }
    }

    sprintf(buf2, "&2Obvious exits:%s&0\r\n", *buf ? buf : " &6None&0.");
    send_to_char(buf2, ch);
}

void send_full_exits(CharData *ch, int roomnum) {
    int dir;
    RoomData *room, *dest;
    Exit *exit;

    room = &world[roomnum];
    *buf = '\0';

    if (ROOM_EFF_FLAGGED(roomnum, ROOM_EFF_ISOLATION)) {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            send_to_char("Obvious exits:\r\n None.\r\n", ch);
            return;
        } else {
            strcpy(buf, "&9&b(&0exits obscured by &5isolation&0&9&b)&0\r\n");
        }
    }

    for (dir = 0; dir < NUM_OF_DIRS; dir++) {
        if ((exit = room->exits[dir]) && ((dest = EXIT_DEST(exit))) && can_see_exit(ch, roomnum, exit)) {
            if (GET_LEVEL(ch) >= LVL_IMMORT && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
                strcpy(buf1, " &8&9[&0");
                /* only show keyword if there is one */
                if (exit->keyword) {
                    /* Only show key vnum if there is one */
                    if (exit->key == NOTHING)
                        sprintf(buf1, "%s&2%s&0: ", buf1, exit->keyword);
                    else
                        sprintf(buf1, "%s&2%s&0 (key %d): ", buf1, exit->keyword, exit->key);
                }
                sprintbit(exit->exit_info, exit_bits, buf1 + strlen(buf1));
                strcat(buf1, "&8&9]&0");
                sprintf(buf2, "%-5s - [%5d] %s%s\r\n", dirs[dir], dest->vnum, dest->name, buf1);
            } else {
                sprintf(buf2, "%-5s - ", dirs[dir]);
                if (EXIT_IS_CLOSED(exit))
                    sprintf(buf2, "%s(%s %s closed)", buf2, exit_name(exit), isplural(exit_name(exit)) ? "are" : "is");
                else if (IS_DARK(exit->to_room) && !CAN_SEE_IN_DARK(ch))
                    strcat(buf2, "Too dark to tell");
                else
                    strcat(buf2, dest->name);
                strcat(buf2, "\r\n");
            }
            strcat(buf, CAP(buf2));
        }
    }

    if (*buf) {
        send_to_char("Obvious exits:\r\n", ch);
        send_to_char(buf, ch);
        send_to_char("&0", ch);
    } else
        send_to_char("There are no obvious exits.\r\n", ch);
}

bool room_contains_char(int roomnum, CharData *ch) {
    CharData *i;

    for (i = world[roomnum].people; i; i = i->next_in_room) {
        if (i == ch)
            return true;
    }
    return false;
}

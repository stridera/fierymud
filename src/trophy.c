/***************************************************************************
 *  File: players.c                                       Part of FieryMUD *
 *  Usage: Player loading/saving and utility routines.                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "trophy.h"

#include "comm.h"
#include "conf.h"
#include "db.h"
#include "players.h"
#include "screen.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* init_trophy()
 *
 * Creates a player's trophy list.  It creates the doubly-linked list of
 * trophy nodes, numbering TROPHY_LENGTH.
 */

void init_trophy(struct char_data *ch) {
    struct trophy_node *node;
    int i;

    /* Destroy existing trophy list (if any) */
    free_trophy(ch);

    /* Create trophy list */
    for (i = 0; i < TROPHY_LENGTH; i++) {
        CREATE(node, struct trophy_node, 1);
        if (i == 0) {
            GET_TROPHY(ch) = node;
            node->prev = node;
        } else {
            node->prev = GET_TROPHY(ch)->prev;
            GET_TROPHY(ch)->prev = node;
            node->next = GET_TROPHY(ch);
            GET_TROPHY(ch) = node;
        }
        node->kill_type = TROPHY_NONE;
    }
}

void add_trophy(struct char_data *ch, int kill_type, int id, float amount) {
    /* Move head node backwards onto oldest */
    GET_TROPHY(ch)->prev->next = GET_TROPHY(ch);
    GET_TROPHY(ch) = GET_TROPHY(ch)->prev;
    GET_TROPHY(ch)->prev->next = NULL;

    /* Use for this kill */
    GET_TROPHY(ch)->kill_type = kill_type;
    GET_TROPHY(ch)->id = id;
    GET_TROPHY(ch)->amount = amount;
}

void kill_to_trophy(struct char_data *vict, struct char_data *killer, float amount) {
    int kill_type, id;
    struct trophy_node *node;

    if (IS_NPC(killer)) {
        sprintf(buf, "SYSERR: Non-player '%s' (%d) in kill_to_trophy()", GET_NAME(killer), GET_MOB_VNUM(killer));
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return;
    }

    if (IS_MOB(vict)) {
        id = GET_MOB_VNUM(vict);
        kill_type = TROPHY_MOBILE;
    } else if (!IS_NPC(vict)) {
        id = GET_IDNUM(vict);
        kill_type = TROPHY_PLAYER;
    } else
        return;

    for (node = GET_TROPHY(killer); node; node = node->next)
        if (node->kill_type == kill_type && node->id == id) {
            node->amount += amount;
            return;
        }

    add_trophy(killer, kill_type, id, amount);
}

void load_trophy(FILE *file, struct char_data *ch) {
    int kill_type, id;
    float amount;
    char line[MAX_INPUT_LENGTH + 1];
    struct trophy_node *node;

    node = GET_TROPHY(ch);
    if (!node) {
        sprintf(buf, "SYSERR: Player %s has no trophy list", GET_NAME(ch));
        mudlog(buf, NRM, LVL_GOD, TRUE);
        return;
    }

    do {
        get_line(file, line);
        sscanf(line, "%d %d %f", &kill_type, &id, &amount);
        if (node && kill_type != TROPHY_NONE) {
            node->kill_type = kill_type;
            node->id = id;
            node->amount = amount;
            node = node->next;
        }
    } while (kill_type != TROPHY_NONE);
}

void save_trophy(FILE *file, struct char_data *ch) {
    struct trophy_node *node;

    if (GET_TROPHY(ch) && GET_TROPHY(ch)->kill_type) {
        fprintf(file, "trophy:\n");
        for (node = GET_TROPHY(ch); node; node = node->next)
            if (node->kill_type != TROPHY_NONE)
                fprintf(file, "%d %d %f\n", node->kill_type, node->id, node->amount);
        fprintf(file, "%d 0 0\n", TROPHY_NONE);
    }
}

void free_trophy(struct char_data *ch) {
    struct trophy_node *node, *next_node;

    for (node = GET_TROPHY(ch); node; node = next_node) {
        next_node = node->next;
        free(node);
    }

    GET_TROPHY(ch) = NULL;
}

void show_trophy(struct char_data *ch, struct char_data *vict) {
    struct trophy_node *node;
    int id;
    char *name;

    if (IS_NPC(vict)) {
        send_to_char("Mobs don't have trophies, genius!\r\n", ch);
        return;
    }

    if (!GET_TROPHY(vict) || GET_TROPHY(vict)->kill_type == TROPHY_NONE) {
        if (ch == vict) {
            sprintf(buf, "%sYour trophy list is empty.%s\r\n", CLR(ch, FGRN), CLR(ch, ANRM));
        } else {
            sprintf(buf, "%s%s's trophy list is empty.%s\r\n", CLR(ch, FGRN), GET_NAME(vict), CLR(ch, ANRM));
        }
        send_to_char(buf, ch);
        return;
    }

    if (ch == vict)
        sprintf(buf, "%sYour trophy list is:%s\r\n\r\n", CLR(ch, FGRN), CLR(ch, ANRM));
    else
        sprintf(buf, "%s%s's trophy list is:%s\r\n\r\n", CLR(ch, FGRN), GET_NAME(vict), CLR(ch, ANRM));
    send_to_char(buf, ch);

    sprintf(buf, "%s%sKills       Mobiles%s\r\n", CLR(ch, HRED), CLR(ch, AUND), CLR(ch, ANRM));
    send_to_char(buf, ch);

    for (node = GET_TROPHY(vict); node; node = node->next) {

        if (node->kill_type == TROPHY_MOBILE) {
            if ((id = real_mobile(node->id)) == NOBODY)
                continue;
            name = mob_proto[id].player.short_descr;
        } else if (node->kill_type == TROPHY_PLAYER) {
            if (!(name = get_name_by_id(node->id)))
                continue;
        } else
            continue;
        sprintf(buf, "%s%6.2f     %s%s%s\r\n",
                node->amount < 4.99 ? CLR(ch, FYEL) : node->amount < 7.99 ? CLR(ch, HYEL) : CLR(ch, HRED), node->amount,
                CLR(ch, ANRM), name, CLR(ch, ANRM));
        send_to_char(buf, ch);
    }
}

float get_kills_vnum(struct trophy_node *trophy, int vnum) {
    struct trophy_node *node;

    for (node = trophy; node; node = node->next)
        if (node->kill_type == TROPHY_MOBILE && node->id == vnum)
            return node->amount;

    return 0;
}

float get_kills_id(struct trophy_node *trophy, int id) {
    struct trophy_node *node;

    for (node = trophy; node; node = node->next)
        if (node->kill_type == TROPHY_PLAYER && node->id == id)
            return node->amount;

    return 0;
}

float get_trophy_kills(struct char_data *ch, struct char_data *vict) {
    if (IS_NPC(ch))
        return 0;

    if (IS_NPC(vict))
        return get_kills_vnum(GET_TROPHY(ch), GET_MOB_VNUM(vict));
    else
        return get_kills_id(GET_TROPHY(ch), GET_ID(vict));
}

float exp_trophy_modifier(struct char_data *ch, struct char_data *vict) {
    float amount = get_trophy_kills(ch, vict);

    if (amount < 2.01)
        return 1.0;
    else if (amount < 3.01)
        return 0.95;
    else if (amount < 5.01)
        return 0.85;
    else if (amount < 7.01)
        return 0.65;
    else if (amount < 10.01)
        return 0.45;
    else
        return 0.3;
}

/***************************************************************************
 * $Log: trophy.c,v $
 * Revision 1.7  2008/05/12 00:42:09  jps
 * Return a float for trophy amounts.
 *
 * Revision 1.6  2008/05/11 05:57:16  jps
 * Split up the kill-querying code and add get_tropy_kills, which is
 * mostly for debugging.
 *
 * Revision 1.5  2008/04/05 17:35:42  myc
 * Get rid of the syserrs about unknown tag 0 in load_player.
 *
 * Revision 1.4  2008/04/03 16:09:54  jps
 * Stop duplicating the code in free_trophy().
 *
 * Revision 1.3  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.2  2008/04/02 03:24:44  myc
 * Moved trophy modifier function here.
 *
 * Revision 1.1  2008/03/16 00:22:59  jps
 * Initial revision
 *
 **************************************************************************/

/***************************************************************************
 *  File: players.c                                       Part of FieryMUD *
 *  Usage: Player loading/saving and utility routines.                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "trophy.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "logging.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* init_trophy()
 *
 * Creates a player's trophy list.  It creates the doubly-linked list of
 * trophy nodes, numbering TROPHY_LENGTH.
 */

void init_trophy(CharData *ch) {
    TrophyNode *node;
    int i;

    /* Destroy existing trophy list (if any) */
    free_trophy(ch);

    /* Create trophy list */
    for (i = 0; i < TROPHY_LENGTH; i++) {
        CREATE(node, TrophyNode, 1);
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

void add_trophy(CharData *ch, int kill_type, int id, float amount) {
    /* Move head node backwards onto oldest */
    GET_TROPHY(ch)->prev->next = GET_TROPHY(ch);
    GET_TROPHY(ch) = GET_TROPHY(ch)->prev;
    GET_TROPHY(ch)->prev->next = nullptr;

    /* Use for this kill */
    GET_TROPHY(ch)->kill_type = kill_type;
    GET_TROPHY(ch)->id = id;
    GET_TROPHY(ch)->amount = amount;
}

void kill_to_trophy(CharData *vict, CharData *killer, float amount) {
    int kill_type, id;
    TrophyNode *node;

    if (IS_NPC(killer)) {
        log(LogSeverity::Stat, LVL_GOD, "SYSERR: Non-player '{}' ({:d}) in kill_to_trophy()", GET_NAME(killer),
            GET_MOB_VNUM(killer));
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

void load_trophy(FILE *file, CharData *ch) {
    int kill_type, id;
    float amount;
    char line[MAX_INPUT_LENGTH + 1];
    TrophyNode *node;

    node = GET_TROPHY(ch);
    if (!node) {
        log(LogSeverity::Stat, LVL_GOD, "SYSERR: Player {} has no trophy list", GET_NAME(ch));
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

void save_trophy(FILE *file, CharData *ch) {
    TrophyNode *node;

    if (GET_TROPHY(ch) && GET_TROPHY(ch)->kill_type) {
        fprintf(file, "trophy:\n");
        for (node = GET_TROPHY(ch); node; node = node->next)
            if (node->kill_type != TROPHY_NONE)
                fprintf(file, "%d %d %f\n", node->kill_type, node->id, node->amount);
        fprintf(file, "%d 0 0\n", TROPHY_NONE);
    }
}

void free_trophy(CharData *ch) {
    TrophyNode *node, *next_node;

    for (node = GET_TROPHY(ch); node; node = next_node) {
        next_node = node->next;
        free(node);
    }

    GET_TROPHY(ch) = nullptr;
}

void show_trophy(CharData *ch, CharData *vict) {
    TrophyNode *node;
    int id;
    char *name;

    if (IS_NPC(vict)) {
        char_printf(ch, "Mobs don't have trophies, genius!\n");
        return;
    }

    if (!GET_TROPHY(vict) || GET_TROPHY(vict)->kill_type == TROPHY_NONE) {
        if (ch == vict) {
            sprintf(buf, "%sYour trophy list is empty.%s\n", CLR(ch, FGRN), CLR(ch, ANRM));
        } else {
            sprintf(buf, "%s%s's trophy list is empty.%s\n", CLR(ch, FGRN), GET_NAME(vict), CLR(ch, ANRM));
        }
        char_printf(ch, buf);
        return;
    }

    if (ch == vict)
        sprintf(buf, "%sYour trophy list is:%s\n\n", CLR(ch, FGRN), CLR(ch, ANRM));
    else
        sprintf(buf, "%s%s's trophy list is:%s\n\n", CLR(ch, FGRN), GET_NAME(vict), CLR(ch, ANRM));
    char_printf(ch, buf);

    sprintf(buf, "%s%sKills       Mobiles%s\n", CLR(ch, HRED), CLR(ch, AUND), CLR(ch, ANRM));
    char_printf(ch, buf);

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
        sprintf(buf, "%s%6.2f     %s%s%s\n",
                node->amount < 4.99   ? CLR(ch, FYEL)
                : node->amount < 7.99 ? CLR(ch, HYEL)
                                      : CLR(ch, HRED),
                node->amount, CLR(ch, ANRM), name, CLR(ch, ANRM));
        char_printf(ch, buf);
    }
}

float get_kills_vnum(TrophyNode *trophy, int vnum) {
    TrophyNode *node;

    for (node = trophy; node; node = node->next)
        if (node->kill_type == TROPHY_MOBILE && node->id == vnum)
            return node->amount;

    return 0;
}

float get_kills_id(TrophyNode *trophy, int id) {
    TrophyNode *node;

    for (node = trophy; node; node = node->next)
        if (node->kill_type == TROPHY_PLAYER && node->id == id)
            return node->amount;

    return 0;
}

float get_trophy_kills(CharData *ch, CharData *vict) {
    if (IS_NPC(ch))
        return 0;

    if (IS_NPC(vict))
        return get_kills_vnum(GET_TROPHY(ch), GET_MOB_VNUM(vict));
    else
        return get_kills_id(GET_TROPHY(ch), GET_ID(vict));
}

float exp_trophy_modifier(CharData *ch, CharData *vict) {
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

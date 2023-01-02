/***************************************************************************
 *  File: retain_comms.c                                  Part of FieryMUD *
 *  Usage: Set of routines to retain and show old comms.                   *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "retain_comms.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* init_retained_comms
 *
 * Create the comms lists.  We need to start with an empty node
 */
void init_retained_comms(CharData *ch) {
    RetainedComms *comms;
    CREATE(comms, RetainedComms, 1);
    comms->tells = nullptr;
    comms->gossips = nullptr;

    GET_RETAINED_COMMS(ch) = comms;
}

void add_retained_comms(CharData *ch, int type, char *msg) {
    CharData *tch;
    CommNode *node, *new_node;
    int i = 0;

    tch = REAL_CHAR(ch); /* Just in case you are shapeshifted */

    for (node = GET_RETAINED_COMM_TYPE(tch, type), i = 0; node && node->next; ++i, node = node->next)
        ; /* Find last node */

    CREATE(new_node, CommNode, 1);
    new_node->time = time(0);
    new_node->msg = strdup(filter_chars(buf2, msg, "\n"));
    new_node->next = nullptr;
    if (node == nullptr) {
        SET_RETAINED_COMM_TYPE(tch, type, new_node);
    } else {
        node->next = new_node;
    }

    if (i >= MAX_RETAINED_COMMS - 1) {
        node = GET_RETAINED_COMM_TYPE(tch, type);
        SET_RETAINED_COMM_TYPE(tch, type, node->next);
        free(node);
    }
}

void load_retained_comms(FILE *file, CharData *ch, int type) {
    CommNode *node, *new_node;
    time_t timestamp;
    char *msg;

    node = GET_RETAINED_COMM_TYPE(ch, type);
    get_line(file, buf);
    while (*buf != '$') {
        msg = any_one_arg(buf, buf1);
        sscanf(buf1, "%ld", &timestamp);
        skip_spaces(&msg);
        CREATE(new_node, CommNode, 1);
        new_node->time = timestamp;
        new_node->msg = strdup(msg);
        if (node == nullptr) {
            SET_RETAINED_COMM_TYPE(ch, type, new_node);
            node = new_node;
        } else {
            node->next = new_node;
            node = node->next;
        }
        get_line(file, buf);
    }
}

void save_retained_comms(FILE *file, CharData *ch, int type) {
    CommNode *node;

    if (type == TYPE_RETAINED_TELLS) {
        fprintf(file, "tells:\n");
    } else if (type == TYPE_RETAINED_GOSSIPS) {
        fprintf(file, "gossips:\n");
    }

    node = GET_RETAINED_COMM_TYPE(ch, type);
    for (; node; node = node->next) {
        fprintf(file, "%ld %s\n", node->time, node->msg);
    }
    fprintf(file, "$\n");
}

void free_retained_comms(CharData *ch) {
    free_comms_node_list(GET_RETAINED_TELLS(ch));
    free_comms_node_list(GET_RETAINED_GOSSIPS(ch));
}

void free_comms_node_list(CommNode *root) {
    CommNode *node, *next;

    for (node = root; node; node = next) {
        next = node->next;
        free(node);
    }
    root = nullptr;
}

void show_retained_comms(CharData *ch, CharData *vict, int type) {
    CommNode *node;
    char *comm_name;

    if (IS_MOB(REAL_CHAR(ch))) {
        char_printf(ch, "Nobody talks to mobs.  Such a sad life.\n");
    }

    if (type == TYPE_RETAINED_TELLS) {
        node = GET_RETAINED_TELLS(REAL_CHAR(ch) == REAL_CHAR(vict) ? REAL_CHAR(ch) : REAL_CHAR(vict));
        comm_name = "tell";
    } else if (type == TYPE_RETAINED_GOSSIPS) {
        node = GET_RETAINED_GOSSIPS(REAL_CHAR(ch) == REAL_CHAR(vict) ? REAL_CHAR(ch) : REAL_CHAR(vict));
        comm_name = "gossip";
    } else {
        log("Attempt to print an unknown type.");
        return;
    }

    if (node == nullptr) {
        if (ch == vict) {
            char_printf(ch, "{}Your recent {} list is empty.{}\n", CLR(ch, FGRN), comm_name, CLR(ch, ANRM));
        } else {
            char_printf(ch, "{}{}'s recent {} list is empty.{}\n", CLR(ch, FGRN), GET_NAME(vict), comm_name,
                        CLR(ch, ANRM));
        }
    } else {
        if (ch == vict) {
            char_printf(ch, "{}Your recent {} list:{}\n\n", CLR(ch, FGRN), comm_name, CLR(ch, ANRM));
        } else {
            char_printf(ch, "{}{}'s recent {} list is:{}\n\n", CLR(ch, FGRN), GET_NAME(vict), comm_name, CLR(ch, ANRM));
        }
        for (; node; node = node->next) {
            auto time = std::chrono::system_clock::from_time_t(node->time);
            char_printf(ch, "{%c}: {}\n", time, node->msg);
        }
    }
}

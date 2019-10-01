/***************************************************************************
 *  File: retain_comms.c                                  Part of FieryMUD *
 *  Usage: Set of routines to retain and show old comms.                   *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "players.h"
#include "retain_comms.h"
#include "interpreter.h"

/* init_retained_comms
 *
 * Create the comms lists.  We need to start with an empty node
 */
void init_retained_comms(struct char_data *ch)
{
    struct retained_comms *comms;
    CREATE(comms, struct retained_comms, 1);
    comms->tells = NULL;
    comms->gossips = NULL;

    GET_RETAINED_COMMS(ch) = comms;
}

void add_retained_comms(struct char_data *ch, int type, char *msg)
{
    struct comm_node *node, *new_node;
    int i = 0;

    for (node = GET_RETAINED_COMM_TYPE(ch, type), i = 0; node && node->next; ++i, node = node->next);  // Find last node

    CREATE(new_node, struct comm_node, 1);
    new_node->time = time(0);
    new_node->msg = strdup(filter_chars(buf, msg, "\r\n"));
    new_node->next = NULL;
    if (node == NULL) {
        SET_RETAINED_COMM_TYPE(ch, type, new_node);
    } else {
        node->next = new_node;
    }

    if (i >= MAX_RETAINED_COMMS - 1) {
        node = GET_RETAINED_COMM_TYPE(ch, type);
        SET_RETAINED_COMM_TYPE(ch, type, node->next);
        free(node);
    }
}

void load_retained_comms(FILE *file, struct char_data *ch, int type) {
    struct comm_node *node, *new_node;
    time_t timestamp;
    char *msg;

    node = GET_RETAINED_COMM_TYPE(ch, type);
    get_line(file, buf);
    while (*buf != '$') {
        msg = any_one_arg(buf, buf1);
        sscanf(buf1, "%ld", &timestamp);
        skip_spaces(&msg);
        CREATE(new_node, struct comm_node, 1);
        new_node->time = timestamp;
        new_node->msg = strdup(msg);
        if (node == NULL) {
            SET_RETAINED_COMM_TYPE(ch, type, new_node);
            node = new_node;
        } else {
            node->next = new_node;
            node = node->next;
        }
        get_line(file, buf);
    }
}

void save_retained_comms(FILE *file, struct char_data *ch, int type) {
    struct comm_node *node;

    if (type == TYPE_RETAINED_TELLS) {
        fprintf(file, "tells:\r\n");
    } else if (type == TYPE_RETAINED_GOSSIPS) {
        fprintf(file, "gossips:\r\n");
    }

    node = GET_RETAINED_COMM_TYPE(ch, type);
    for (; node; node = node->next) {
        fprintf(file, "%ld %s\r\n", node->time, node->msg);
    }
    fprintf(file, "$\r\n");
}

void free_retained_comms(struct char_data *ch)
{
    free_comms_node_list(GET_RETAINED_TELLS(ch));
    free_comms_node_list(GET_RETAINED_GOSSIPS(ch));
}

void free_comms_node_list(struct comm_node *root)
{
    struct comm_node *node, *next;

    for (node = root; node; node = next) {
        next = node->next;
        free(node);
    }
    root = NULL;
}

void show_retained_comms(struct char_data *ch, struct char_data *vict, int type)
{
    struct comm_node *node;
    char *comm_name;
    char timebuf[32];

    if (IS_MOB(ch)) {
        send_to_char("Nobody talks to mobs.  Such a sad life.", ch);
    }

    if (type == TYPE_RETAINED_TELLS) {
        node = GET_RETAINED_TELLS(ch == vict ? ch : vict);
        comm_name = "tell";
    } else if (type == TYPE_RETAINED_GOSSIPS) {
        node = GET_RETAINED_GOSSIPS(ch == vict ? ch : vict);
        comm_name = "gossip";
    } else {
        log("Attempt to print an unknown type.");
        return;
    }

    if (node == NULL) {
        if (ch == vict) {
            sprintf(buf, "%sYour recent %s list is empty.%s\r\n", CLR(ch, FGRN), comm_name,
                    CLR(ch, ANRM));
        } else {
            sprintf(buf, "%s%s's recent %s list is empty.%s\r\n", CLR(ch, FGRN),
                    GET_NAME(vict), comm_name, CLR(ch, ANRM));
        }
        send_to_char(buf, ch);
    } else {
        if (ch == vict) {
            sprintf(buf, "%sYour recent %s list:%s\r\n\r\n", CLR(ch, FGRN), comm_name,
                    CLR(ch, ANRM));
        } else {
            sprintf(buf, "%s%s's recent %s list is:%s\r\n\r\n", CLR(ch, FGRN),
                    GET_NAME(vict), comm_name, CLR(ch, ANRM));
        }
        send_to_char(buf, ch);
        for(; node; node = node->next) {
            log("log contains: %s", node->msg);
            strftime(timebuf, 32, TIMEFMT_LOG, localtime(&node->time));
            sprintf(buf, "%s: %s\r\n", timebuf, node->msg);
            send_to_char(buf, ch);
        }
    }
}

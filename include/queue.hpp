/***************************************************************************
 *   File: queue.h                                       Part of FieryMUD  *
 *  Usage: structures and prototypes for queues                            *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

/* number of queues to use (reduces enqueue cost) */
#define NUM_EVENT_QUEUES 100

struct QElement {
    void *data;
    long key;
    QElement *prev, *next;
};

struct Queue {
    QElement *head[NUM_EVENT_QUEUES], *tail[NUM_EVENT_QUEUES];
};

/* function protos need by other modules */
Queue *queue_init(void);
QElement *queue_enq(Queue *q, void *data, long key);
void queue_deq(Queue *q, QElement *qe);
void *queue_head(Queue *q);
long queue_key(Queue *q);
long queue_elmt_key(QElement *qe);
void queue_free(Queue *q);

/***************************************************************************
 *   File: queue.c                                       Part of FieryMUD  *
 *  Usage: generic queue functions for building and using a priority queue *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "queue.h"

#include "conf.hpp"
#include "events.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* number of queues to use (reduces enqueue cost) */
#define NUM_EVENT_QUEUES 100

struct queue {
    struct q_element *head[NUM_EVENT_QUEUES], *tail[NUM_EVENT_QUEUES];
};

struct q_element {
    void *data;
    long key;
    struct q_element *prev, *next;
};

/* external variables */
extern unsigned long pulse;

/* returns a new, initialized queue */
struct queue *queue_init(void) {
    struct queue *q;

    CREATE(q, queue, 1);

    return q;
}

/* add data into the priority queue q with key */
struct q_element *queue_enq(queue *q, void *data, long key) {
    struct q_element *qe, *i;
    int bucket;

    CREATE(qe, q_element, 1);
    qe->data = data;
    qe->key = key;

    bucket = key % NUM_EVENT_QUEUES; /* which queue does this go in */

    if (!q->head[bucket]) { /* queue is empty */
        q->head[bucket] = qe;
        q->tail[bucket] = qe;
    }

    else {
        for (i = q->tail[bucket]; i; i = i->prev) {

            if (i->key < key) { /* found insertion point */
                if (i == q->tail[bucket])
                    q->tail[bucket] = qe;
                else {
                    qe->next = i->next;
                    i->next->prev = qe;
                }

                qe->prev = i;
                i->next = qe;
                break;
            }
        }

        if (i == NULL) { /* insertion point is front of list */
            qe->next = q->head[bucket];
            q->head[bucket] = qe;
            qe->next->prev = qe;
        }
    }

    return qe;
}

/* remove queue element qe from the priority queue q */
void queue_deq(queue *q, q_element *qe) {
    int i;

    assert(qe);

    i = qe->key % NUM_EVENT_QUEUES;

    if (qe->prev == NULL)
        q->head[i] = qe->next;
    else
        qe->prev->next = qe->next;

    if (qe->next == NULL)
        q->tail[i] = qe->prev;
    else
        qe->next->prev = qe->prev;

    free(qe);
}

/*
 * removes and returns the data of the
 * first element of the priority queue q
 */
void *queue_head(queue *q) {
    void *data;
    int i;

    i = pulse % NUM_EVENT_QUEUES;

    if (!q->head[i])
        return NULL;

    data = q->head[i]->data;
    queue_deq(q, q->head[i]);
    return data;
}

/*
 * returns the key of the head element of the priority queue
 * if q is NULL, then return the largest unsigned number
 */
long queue_key(queue *q) {
    int i;

    i = pulse % NUM_EVENT_QUEUES;

    if (q->head[i])
        return q->head[i]->key;
    else
        return LONG_MAX;
}

/* returns the key of queue element qe */
long queue_elmt_key(q_element *qe) { return qe->key; }

/* free q and contents */
void queue_free(queue *q) {
    int i;
    struct q_element *qe, *next_qe;
    struct event *event;

    for (i = 0; i < NUM_EVENT_QUEUES; i++)
        for (qe = q->head[i]; qe; qe = next_qe) {
            next_qe = qe->next;
            /*
             * This is okay for now, but if we ever were to use this queue
             * for something besides events, we'd be in trouble.
             */
            if ((event = (event *)qe->data) != NULL) {
                if (event->free_obj && event->event_obj)
                    free(event->event_obj);
                free(event);
            }
            free(qe);
        }

    free(q);
}

/***************************************************************************
 *   File: queue.c                                       Part of FieryMUD  *
 *  Usage: generic queue functions for building and using a priority queue *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "queue.hpp"

#include "conf.hpp"
#include "events.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "logging.hpp"


/* external variables */
extern unsigned long pulse;

/* returns a new, initialized queue */
Queue *queue_init(void) {
    Queue *q;

    CREATE(q, Queue, 1);

    return q;
}

/* add data into the priority queue q with key */
QElement *queue_enq(Queue *q, void *data, long key) {
    QElement *qe, *i;
    int bucket;

    CREATE(qe, QElement, 1);
    qe->data = data;
    qe->key = key;

    bucket = key % NUM_EVENT_QUEUES; /* which Queue does this go in */

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

        if (i == nullptr) { /* insertion point is front of list */
            qe->next = q->head[bucket];
            q->head[bucket] = qe;
            qe->next->prev = qe;
        }
    }

    return qe;
}

/* remove queue element qe from the priority queue q */
void queue_deq(Queue *q, QElement *qe) {
    int i;

    assert(qe);

    i = qe->key % NUM_EVENT_QUEUES;

    if (qe->prev == nullptr)
        q->head[i] = qe->next;
    else
        qe->prev->next = qe->next;

    if (qe->next == nullptr)
        q->tail[i] = qe->prev;
    else
        qe->next->prev = qe->prev;

    free(qe);
}

/*
 * removes and returns the data of the
 * first element of the priority queue q
 */
void *queue_head(Queue *q) {
    void *data;
    int i;

    i = pulse % NUM_EVENT_QUEUES;

    if (!q->head[i])
        return nullptr;

    data = q->head[i]->data;
    queue_deq(q, q->head[i]);
    return data;
}

/*
 * returns the key of the head element of the priority queue
 * if q is NULL, then return the largest unsigned number
 */
long queue_key(Queue *q) {
    int i;

    i = pulse % NUM_EVENT_QUEUES;

    if (q->head[i])
        return q->head[i]->key;
    else
        return LONG_MAX;
}

/* returns the key of queue element qe */
long queue_elmt_key(QElement *qe) { return qe->key; }

/* free q and contents */
void queue_free(Queue *q) {
    int i;
    QElement *qe, *next_qe;
    Event *event;

    for (i = 0; i < NUM_EVENT_QUEUES; i++)
        for (qe = q->head[i]; qe; qe = next_qe) {
            next_qe = qe->next;
            /*
             * This is okay for now, but if we ever were to use this queue
             * for something besides events, we'd be in trouble.
             */
            if ((event = (Event *)qe->data) != nullptr) {
                if (event->free_obj && event->event_obj)
                    free(event->event_obj);
                free(event);
            }
            free(qe);
        }

    free(q);
}

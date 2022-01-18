/***************************************************************************
 *   File: queue.h                                       Part of FieryMUD  *
 *  Usage: structures and prototypes for queues                            *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#ifndef __FIERY_QUEUE_H
#define __FIERY_QUEUE_H

/* function protos need by other modules */
struct queue *queue_init(void);
struct q_element *queue_enq(struct queue *q, void *data, long key);
void queue_deq(struct queue *q, struct q_element *qe);
void *queue_head(struct queue *q);
long queue_key(struct queue *q);
long queue_elmt_key(struct q_element *qe);
void queue_free(struct queue *q);

#endif /* __FIERY_QUEUE_H */

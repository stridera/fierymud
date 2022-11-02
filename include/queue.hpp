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

/* function protos need by other modules */
struct queue *queue_init(void);
struct q_element *queue_enq(queue *q, void *data, long key);
void queue_deq(queue *q, q_element *qe);
void *queue_head(queue *q);
long queue_key(queue *q);
long queue_elmt_key(q_element *qe);
void queue_free(queue *q);

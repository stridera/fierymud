/***************************************************************************
 * $Id: queue.h,v 1.4 2002/09/13 02:32:10 jjl Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: queue.h                                       Part of FieryMUD  *
 *  Usage: structures and prototypes for queues                            *
 *     By: Eric Green (ejg3@cornell.edu)                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

/***************************************************************************
 *  Changes: 3/6/98 ejg:  Moved defines and structs to queue.c.            *
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

/***************************************************************************
 * $Log: queue.h,v $
 * Revision 1.4  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.3  2000/11/24 21:17:12  rsd
 * Altered comment header and added back rlog messgaes from
 * prior to the addition of the $log$ string.
 *
 * Revision 1.2  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

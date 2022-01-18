/***************************************************************************
 *   File: mail.h                                        Part of FieryMUD  *
 *  Usage: header file for mail system                                     *
 *                                                                         *
 *  written by Jeremy Elson (jelson@cs.jhu.edu)                            *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_MAIL_H
#define __FIERY_MAIL_H

#include "structs.h"
#include "sysdep.h"

/* minimum level a player must be to send mail	*/
#define MIN_MAIL_LEVEL 2

/* # of silver coins required to send mail	*/
#define STAMP_PRICE 200

/* Maximum size of mail in bytes (arbitrary)	*/
#define MAX_MAIL_SIZE 4096

/* size of mail file allocation blocks		*/
#define BLOCK_SIZE 100

/*
 * NOTE:  Make sure that your block size is big enough -- if not,
 * HEADER_BLOCK_DATASIZE will end up negative.  This is a bad thing.
 * Check the define below to make sure it is >0 when choosing values
 * for NAME_SIZE and BLOCK_SIZE.  100 is a nice round number for
 * BLOCK_SIZE and is the default ... why bother trying to change it
 * anyway?
 *
 * The mail system will always allocate disk space in chunks of size
 * BLOCK_SIZE.
 */

int scan_file(void);
int has_mail(long recipient);
void store_mail(long to, long from, int vnum, char *message_pointer);
char *read_delete(long recipient, int *obj_vnum);
void free_mail_index(void);

#define HEADER_BLOCK -1
#define LAST_BLOCK -2
#define DELETED_BLOCK -3

/*
 * note: next_block is part of header_blk in a data block; we can't combine
 * them here because we have to be able to differentiate a data block from a
 * header block when booting mail system.
 */

struct header_data_type {
    long next_block; /* if header block, link to next block	*/
    long from;       /* idnum of the mail's sender		*/
    long to;         /* idnum of mail's recipient		*/
    sh_int vnum;
    time_t mail_time; /* when was the letter mailed?		*/
};

/* size of the data part of a header block */
#define HEADER_BLOCK_DATASIZE (BLOCK_SIZE - sizeof(long) - sizeof(struct header_data_type) - sizeof(char))

/* size of the data part of a data block */
#define DATA_BLOCK_DATASIZE (BLOCK_SIZE - sizeof(long) - sizeof(char))

/* note that an extra space is allowed in all string fields for the
   terminating null character.  */

struct header_block_type_d {
    long block_type;                     /* is this a header or data block?	*/
    struct header_data_type header_data; /* other header data		*/
    char txt[HEADER_BLOCK_DATASIZE + 1]; /* actual text plus 1 for null	*/
};

struct data_block_type_d {
    long block_type;                   /* -1 if header block, -2 if last data block
                                          in mail, otherwise a link to the next */
    char txt[DATA_BLOCK_DATASIZE + 1]; /* actual text plus 1 for null	*/
};

typedef struct header_block_type_d header_block_type;
typedef struct data_block_type_d data_block_type;

struct position_list_type_d {
    long position;
    struct position_list_type_d *next;
};

typedef struct position_list_type_d position_list_type;

struct mail_index_type_d {
    long recipient;                 /* who is this mail for?	*/
    position_list_type *list_start; /* list of mail positions	*/
    struct mail_index_type_d *next; /* link to next one		*/
};

typedef struct mail_index_type_d mail_index_type;

#endif /* __FIERY_MAIL_H */

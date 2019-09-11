/***************************************************************************
 * $Id: house.h,v 1.5 2008/05/17 04:32:25 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: house.h                                        Part of FieryMUD *
 *  Usage: Header file for handling of player houses                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#define MAX_HOUSES	100
#define MAX_GUESTS	10

#define HOUSE_PRIVATE	0

struct house_control_rec {
   int vnum;			/* vnum of this house		*/
   int atrium;		        /* vnum of atrium		*/
   int exit_num;		/* direction of house's exit	*/
   time_t built_on;		/* date this house was built	*/
   int mode;			/* mode of ownership		*/
   long owner;			/* idnum of house's owner	*/
   int num_of_guests;		/* how many guests for house	*/
   long guests[MAX_GUESTS];	/* idnums of house's guests	*/
   time_t last_payment;		/* date of last house payment   */
   long spare0;
   long spare1;
   long spare2;
   long spare3;
   long spare4;
   long spare5;
   long spare6;
   long spare7;
};

#define TOROOM(room, dir) (world[room].exits[dir] ? \
			    world[room].exits[dir]->to_room : NOWHERE)

void	House_listrent(struct char_data *ch, int vnum);
void	House_boot(void);
void	House_save_all(void);
int	House_can_enter(struct char_data *ch, int house);
void	House_crashsave(int vnum);

/***************************************************************************
 * $Log: house.h,v $
 * Revision 1.5  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.4  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.3  2000/11/21 19:11:10  rsd
 * Added a comment header and the initial rlog message.
 *
 * Revision 1.2  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.1  1999/01/29 01:23:31  mud
 * Initial revision
 *
 ***************************************************************************/

/***************************************************************************
 * $Id: dg_handler.c,v 1.9 2008/02/16 20:26:04 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: dg_handler.c                                   Part of FieryMUD *
 *  Usage: who knows?                                                      *
 *     By: Unknown                                                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 2000 by the Fiery Consortium                    *
 ***************************************************************************/

/* There is no claim of ownership or copyright in this file prior to being
 * owned by FieryMUD.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "events.h"

/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig)
{
  struct trig_data *temp;
  
  if (GET_TRIG_WAIT(trig)) {
    event_cancel(GET_TRIG_WAIT(trig));
    GET_TRIG_WAIT(trig) = NULL;
  }
  
  trig_index[trig->nr]->number--; 
  
  /* walk the trigger list and remove this one */
  REMOVE_FROM_LIST(trig, trigger_list, next_in_world);
  
  if (trig->running == TRUE)
    trig->purged = TRUE;
  else
    free_trigger(trig);
}

/* remove all triggers from a mob/obj/room */
void extract_script(struct script_data *sc)
{
  struct trig_data *trig, *next_trig;
  
  for (trig = TRIGGERS(sc); trig; trig = next_trig) {
    next_trig = trig->next;
    extract_trigger(trig);
  }
  TRIGGERS(sc) = NULL;

  free_varlist(sc->global_vars);
  free(sc);
}

/***************************************************************************
 * $Log: dg_handler.c,v $
 * Revision 1.9  2008/02/16 20:26:04  myc
 * Cleaning up function to free triggers.  Adding function to free
 * prototype scripts at program termination.
 *
 * Revision 1.8  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.7  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.6  2000/11/28 01:17:12  mtp
 * replaced dg_event.c code with events.c code
 *
 * Revision 1.5  2000/11/21 01:45:34  rsd
 * Altered comment header to note claim of copyright and
 * added the back rlog messages from prior to the addition
 * of the $log$ string
 *
 * Revision 1.4  2000/02/13 07:34:13  mtp
 * fixed opurge/mpurge [rpblems by not freeing the running trigger
 * until it completes (if running flag set then extract_trigger sets purged
 * flag instead of freeing)
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/31 00:20:14  mud
 * Added comment header
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

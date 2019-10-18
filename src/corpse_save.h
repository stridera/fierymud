/***************************************************************************
 * $Id: corpse_save.h,v 1.10 2008/08/16 08:25:59 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: corpse_save.h                                  Part of FieryMUD *
 *  Usage: Handling of player corpses                                      *
 * Author: Nechtrous                                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/
 
#define MAX_CORPSES	500

extern void register_corpse(struct obj_data *corpse);
extern void boot_corpses(void);
extern void save_corpse(struct obj_data *corpse);
extern void update_corpse(struct obj_data *corpse);
extern void destroy_corpse(struct obj_data *corpse); 
extern void show_corpses(struct char_data *ch, char *argument);
static bool get_corpse_filename(int id, char *filename);

/***************************************************************************
 * $Log: corpse_save.h,v $
 * Revision 1.10  2008/08/16 08:25:59  jps
 * Removing unused record.
 *
 * Revision 1.9  2008/06/05 02:07:43  myc
 * Rewrote corpse saving and loading to use the ascii object files.
 *
 * Revision 1.8  2008/01/14 20:38:42  myc
 * Fix to save corpse level to corpse control file so that resurrect
 * will work properly on corpses booted from file.
 *
 * Revision 1.7  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.6  2000/11/21 00:46:05  rsd
 * Added bacj rlog messages from prior to the addition
 * of the $log$ string.
 *
 * Revision 1.5  2000/09/22 23:26:07  rsd
 * Altered the comment header to reflect that this is fiery
 * code now. Also fixed the spacingof some comments.
 *
 * Revision 1.4  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.3  1999/08/05 14:53:58  dce
 * Fixed
 *
 * Revision 1.2  1999/01/30 21:43:29  mud
 * Added standard comment header 
 * AIndented file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

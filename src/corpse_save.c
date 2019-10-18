/***************************************************************************
 * $Id: corpse_save.c,v 1.23 2009/03/17 07:59:42 jps Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: corpse_save.c                                 Part of FieryMUD  *
 *  Usage: Handling of player corpses                                      *
 * Author: Nechtrous, et al..                                              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/


/* Notes:

This file handles the corpse saving operation for the mud. Some current
points of interest are:
1. There are 2 main file structure that make up the corpse saving routine.
   One is the corpse control record file. This file is currently
   lib/etc/ccontrol. This file contains an index of all ACTIVE player
   corpses. The second is the numerous files dynamically created in
   lib/corpse/ directory. These files contain the object listings of
   the corpses.
2. There can only be a set number of player corpses allowed at a time.
   MAX_CORPSES in corpse_save.h is set to the current number. The higher it
   is set, the more amount of memory that is needed.
3. Currently all corpses will save when either the corpse passes through
   obj_to_room to update the vnum OR when the corpses passes through 
   extract_obj in which case the corpse is deleted from disk. This event
   driven system is workable, however, if this system proves to cause
   any amount of lag under heavy strain a time based system may be used.
   This time based system would save all corpses every set amount of time.
   This would require the use of flagging much like house code. 
4. When a corpse is created, it is inserted into memory and written to disk.
   The files produced are sequential numerical assignments. For example, the
   very 1st corpse created will be 0.corpse, the 2nd will be 1.corpse and so
   on. This system will continue until a time is reached that there are NO
   player corpses in the game, then the numerical system will start over.
   Keep in mind that the corpse control record is dynamic whereas the file
   names are sequential and will not fill gaps created.
5. Currently all corpses loaded off disk from crash will repopulate with
   the player inventory, having taken all of the items out of any 
   containers. To retain the items assignment to particular containers
   would be a good project in the future.
*/

#include <dirent.h>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "corpse_save.h"
#include "pfiles.h"
#include "math.h"
#include "limits.h"
#include "strings.h"

struct corpse_data {
  int id;
  struct obj_data *corpse;
  struct corpse_data *prev, *next;
};

static struct {
  struct corpse_data list;
  int count;
  bool allow_save;
} corpse_control;

#define SENTINEL                 (&corpse_control.list)

static void save_corpse_list(void);

int corpse_count(void)
{
  return corpse_control.count;
}

static void remove_entry(struct corpse_data *entry)
{
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
  entry->prev = entry->next = NULL;
  free(entry);
  --corpse_control.count;
}


void free_corpse_list()
{
  while (SENTINEL->next != SENTINEL)
    remove_entry(SENTINEL->next);
}

void update_corpse(struct obj_data *corpse)
{
}

static int corpse_id(struct obj_data *corpse)
{
  int id = GET_OBJ_VAL(corpse, VAL_CORPSE_ID);
  return (id >= 0 ? id : -1);
}


/* Load all objects for a corpse */
static int load_corpse(struct corpse_data *corpse, struct dirent *ep)
{
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  struct obj_data *obj, *containers[MAX_CONTAINER_DEPTH + 1];
  int location, depth, i, id;
  extern int r_mortal_start_room;


  if (ep->d_type != DT_REG || !endswith(ep->d_name, CORPSE_SUFFIX)) {
    log("INFO: Skipping non-corpse file in corpse directory %s.", ep->d_name);
    return NULL;
  }

  strcpy(fname, CORPSE_DIR);
  strcat(fname, ep->d_name);
  if (!(fl = fopen(fname, "r"))) {
    log("SYSERR: Can't open corpse file %s.", ep->d_name);
    return NULL;
  }
  i = strlen(ep->d_name) - 7; // get the id minus the .corpse
  strncpy(buf1, ep->d_name, i);
  buf1[i] = '\0';
  if (!is_integer(buf1)) {
      log("SYSERR: Unable to grab ID from corpse file: %s.", ep->d_name);
  }
  id = atoi(buf1);

  get_line(fl, buf1);
  if (is_integer(buf1)) {
    depth = atoi(buf1);
    if ((depth = real_room(depth)) < 0) {
      sprintf(buf, "SYSERR: Unable to locate room %s for corpse file %s", buf1, ep->d_name);
      log(buf);
      depth = r_mortal_start_room;
    }
  }
  else {
    sprintf(buf, "SYSERR: First line of corpse file not room vnum for corpse %s", ep->d_name);
    log(buf);
    if (strchr(buf1, ':'))
      rewind(fl);
    depth = r_mortal_start_room;
  }

  if (build_object(fl, &obj, &location)) {
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER || !str_str(obj->name, "corpse")) {
      sprintf(buf, "SYSERR: First object '%s' loaded from corpse %s not corpse", obj->short_description, ep->d_name);
      log(buf);
      extract_obj(obj);
      return NULL;
    }
    containers[0] = obj;
    GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) = CORPSE_PC;
    obj_to_room(obj, depth);
  }
  else {
    sprintf(buf, "SYSERR: Unable to read in corpse data for corpse %s in load_corpse", ep->d_name);
    log(buf);
    return NULL;
  }

  while (!feof(fl)) {
    if (!build_object(fl, &obj, &location))
      break;
    depth = MAX(0, -location);
    for (i = MAX_CONTAINER_DEPTH - 1; i >= depth; --i)
      containers[i] = NULL;
    containers[depth] = obj;
    if (depth > 0)
      obj_to_obj(obj, containers[depth - 1]);
  }

  fclose(fl);

  /* Ensure that the items inside aren't marked for decomposition. */
  stop_decomposing(containers[0]);
  /* And mark the corpse itself as decomposing. */
  SET_FLAG(GET_OBJ_FLAGS(containers[0]), ITEM_DECOMP);

  corpse = containers[0];
  return id;
}


void save_corpse(struct obj_data *corpse)
{
  char buf[MAX_STRING_LENGTH];
  FILE *fp;
  int id;
  struct obj_data *temp;
  
  if (!IS_CORPSE(corpse)) {
    log("SYSERR: Non-corpse object passed to save_corpse");
    return;
  }
  if ((id = corpse_id(corpse)) < 0) {
    log("SYSERR: Invalid corpse id encountered in save_corpse");
    return;
  }
  if (!get_corpse_filename(id, buf)) {
    log("SYSERR: Invalid id passed from save_corpse");
    return;
  }
  if (!(fp = fopen(buf, "w"))) {
    perror("SYSERR: Error saving corpse file");
    return;
  }

  /* Corpse room vnum */
  fprintf(fp, "%d\n", corpse->in_room == NOWHERE ? NOWHERE :
          world[corpse->in_room].vnum);

  /*
   * Warning!  Hack:  write_objects writes out corpse->next_content.
   * But we don't want to write out other objects in the room where
   * the corpse is, so we'll temporarily set next_content to NULL.
   */
  temp = corpse->next_content;
  corpse->next_content = NULL;
  write_objects(corpse, fp, WEAR_INVENTORY);
  corpse->next_content = temp;

  fclose(fp);
}

/* Return a filename given a corpse id */
static bool get_corpse_filename(int id, char *filename)
{
  if (id < 0) {
    log("SYSERR(corpse_save.c): Corpse has id number < 0 no corpse loaded.");
    return FALSE;
  } else {
    sprintf(filename, "corpse/%d.corpse", id);
    return TRUE;
  }
}


/* Delete a corpse save file */
static void delete_corpse_file(int id)
{
  char buf[MAX_INPUT_LENGTH], fname[MAX_INPUT_LENGTH];
  FILE *fl;
  
  if (!get_corpse_filename(id, fname)) {
    log("SYSERR: Invalid id passed from delete_corpse_file");
    return;
  }
  if (!(fl = fopen(fname, "r"))) {
    sprintf(buf, "SYSERR: Error deleting corpse file %s.", fname);
    perror(buf);
    return;
  }
  fclose(fl);
  if (unlink(fname) < 0) {
    sprintf(buf, "SYSERR: Unable to unlink corpse file %s.", fname);
    perror(buf);
  }
}

/* find a corpse in the corpse control record */
static struct corpse_data *find_entry(int id)
{
  struct corpse_data *entry;

  for (entry = SENTINEL->next; entry != SENTINEL; entry = entry->next)
    if (entry->id == id)
      return entry;

  return NULL;
}

/* 8/5/99 David Endre - Fix it so more than one corpse is saved over
   a reboot. The problem was the old code called obj_to_room which
   saves the ccontrol file and since only one corpse is in the world
   it saves that in the ccontrol file and never gets a chance to boot
   another corpse */

/* call from boot_db - will load control recs, load corpses, load objs */
/* does sanity checks on vnums & removes invalid records */
void boot_corpses(void)
{
  DIR *dl;
  struct dirent *ep;
  int id;
  struct corpse_data *entry;
  struct obj_data *corpse;

  memset(&corpse_control, 0x0, sizeof(corpse_control));
  SENTINEL->next = SENTINEL->prev = SENTINEL;

  if (!(dl = opendir(CORPSE_DIR))) {
    log("Unable to load Corpse Directory.");
    return;
  }

  while ((ep) = readdir(dl)) {
    if (!strcmp (ep->d_name, "."))
      continue;
    if (!strcmp (ep->d_name, ".."))
      continue;

    if ((id = load_corpse(corpse, ep)) < 0) {
      continue;
    }

    CREATE(entry, struct corpse_data, 1);
    entry->id = id;
    entry->corpse = corpse;

    SENTINEL->prev->next = entry;
    entry->prev = SENTINEL->prev;
    SENTINEL->prev = entry;
    entry->next = SENTINEL;

    ++corpse_control.count;
  }

  corpse_control.allow_save = TRUE;
}

/* When a player dies, this function is called from make_corpse in fight.c */
void register_corpse(struct obj_data *corpse)
{
  struct corpse_data *entry;

  if (GET_OBJ_VAL(corpse, VAL_CORPSE_ID)) {
    for (entry = SENTINEL->next; entry != SENTINEL; entry = entry->next)
      if (entry->corpse == corpse) {
        log("SYSERR: already registered corpse passed to register_corpse");
        return;
      }
  }

  GET_OBJ_VAL(corpse, VAL_CORPSE_ID) = SENTINEL->prev->id + 1;

  CREATE(entry, struct corpse_data, 1);
  entry->id = GET_OBJ_VAL(corpse, VAL_CORPSE_ID);
  entry->corpse = corpse;

  SENTINEL->prev->next = entry;
  entry->prev = SENTINEL->prev;
  SENTINEL->prev = entry;
  entry->next = SENTINEL;

  ++corpse_control.count;
  
  save_corpse(corpse);
}


/* called from extract_obj when corpse rots or is otherwise removed from play */
void destroy_corpse(struct obj_data *corpse)
{
  int id = corpse_id(corpse);
  struct corpse_data *entry = find_entry(id);

  if (entry) {
    remove_entry(entry);
    delete_corpse_file(id);
  }
  log("Destroying corpse: %d", id);
}

void show_corpses(struct char_data *ch, char *argument)
{
  struct corpse_data *entry;

  if (corpse_control.count) {
    send_to_char("Id  Corpse              Level  Decomp  Location\r\n"
                 "-------------------------------------------------------------------\r\n",
                 ch);
    for (entry = SENTINEL->next; entry != SENTINEL; entry = entry->next) {
      if (!strn_cmp(entry->corpse->short_description, "the corpse of ", 14))
        strcpy(buf1, entry->corpse->short_description + 14);
      else
        strcpy(buf1, entry->corpse->name);
      if (entry->corpse->carried_by)
        sprintf(buf2, "carried by %s", GET_NAME(entry->corpse->carried_by));
      else if (entry->corpse->in_room != NOWHERE)
        sprintf(buf2, "%s @L[&0%d@L]&0", world[entry->corpse->in_room].name,
                world[entry->corpse->in_room].vnum);
      else if (entry->corpse->in_obj)
        sprintf(buf2, "in %s", entry->corpse->in_obj->short_description);
      else if (entry->corpse->worn_by)
        sprintf(buf2, "worn by %s", GET_NAME(entry->corpse->worn_by));
      else
        strcpy(buf2, "an unknown location");
      cprintf(ch, "%-4d%-20.20s%5d  %6d  %25s\r\n", entry->id,
              buf1, GET_OBJ_LEVEL(entry->corpse),
              GET_OBJ_DECOMP(entry->corpse), buf2);
    }
  }
  else
    send_to_char("There are no player corpses in the game.\r\n", ch);
}


/***************************************************************************
 * $Log: corpse_save.c,v $
 * Revision 1.23  2009/03/17 07:59:42  jps
 * Moved str_str to strings.c
 *
 * Revision 1.22  2009/03/04 05:14:16  myc
 * Fixed alignment in show corpse.
 *
 * Revision 1.21  2009/02/16 14:21:04  myc
 * Make sure corpses get the right decomp flags when loaded from file.
 *
 * Revision 1.20  2009/02/05 16:26:57  myc
 * Fix string cut-off on 'show corpse' screen.
 *
 * Revision 1.19  2008/09/02 07:16:00  mud
 * Changing object TIMER uses into DECOMP where appropriate
 *
 * Revision 1.18  2008/08/24 02:37:01  myc
 * Fix function signature for reference to external function str_str.
 *
 * Revision 1.17  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.16  2008/06/05 02:07:43  myc
 * Rewrote corpse saving and loading to use the ascii object files.
 *
 * Revision 1.15  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.14  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.13  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.12  2008/01/14 20:38:42  myc
 * Fix to save corpse level to corpse control file so that resurrect
 * will work properly on corpses booted from file.
 *
 * Revision 1.11  2007/10/04 16:20:24  myc
 * Transient item flag now makes things decay when they are on the ground.
 * Added this flag to corpses.
 *
 * Revision 1.10  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.9  2000/11/21 00:42:30  rsd
 * Added back rlog messages from prior to the addition of
 * the $log$ string.
 *
 * Revision 1.8  2000/09/26 15:03:40  jimmy
 * Fixed a couple of instances of bad index iteration: the ++ was
 * actually inside the [] of another variable, eeeeek!!!
 * Fixed a bug where the wrong index number was referenced that
 * was causing equipment not to load on corpses after a boot.
 *
 * Revision 1.7  2000/09/22 23:24:49  rsd
 * altered the comment header to reflect that it's fiery code
 * now. Also added a syserr message to be logged and mopped
 * up some spacing in the comments.  Wow I feel so useful
 *
 * Revision 1.6  1999/11/28 23:03:02  cso
 * reordered a little of corpse_load to make it look nicer. no functional
 * differences there.
 * changed the values on corpses, so modified corpse_boot to set up the pc
 * corpses correctly.
 *
 * Revision 1.5  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.4  1999/08/05 14:52:02  dce
 * Corpses now save correctly ove a reboot/crash!
 *
 * Revision 1.3  1999/04/22 01:44:49  dce
 * Debuging for corpse saving
 *
 * Revision 1.2  1999/01/30 21:40:33  mud
 * Indented file
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

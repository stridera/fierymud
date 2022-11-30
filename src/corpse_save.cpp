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

#include "corpse_save.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "math.hpp"
#include "objects.hpp"
#include "pfiles.hpp"
// #include "strings.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <string.h>

struct corpse_data {
    int id;
    ObjData *corpse;
    corpse_data *prev, *next;
};

static struct {
    corpse_data list;
    int count;
    bool allow_save;
} corpse_control;

#define SENTINEL (&corpse_control.list)

static void save_corpse_list(void);

int corpse_count(void) { return corpse_control.count; }

static void check_corpse_id(corpse_data *entry) { (void)entry; }

static void remove_entry(corpse_data *entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->prev = entry->next = nullptr;
    free(entry);
    --corpse_control.count;
}

void free_corpse_list() {
    while (SENTINEL->next != SENTINEL)
        remove_entry(SENTINEL->next);
}

void update_corpse(ObjData *corpse) {
    (void)corpse;
    save_corpse_list();
}

static int corpse_id(ObjData *corpse) {
    int id = GET_OBJ_VAL(corpse, VAL_CORPSE_ID);
    return (id >= 0 ? id : -1);
}

/* Return a filename given a corpse id */
static bool get_corpse_filename(int id, char *filename) {
    if (id < 0) {
        log("SYSERR(corpse_save.c): Corpse has id number < 0 no corpse loaded.");
        return false;
    } else {
        sprintf(filename, "corpse/%d.corpse", id);
        return true;
    }
}

/* Load all objects for a corpse */
static ObjData *load_corpse(int id) {
    FILE *fl;
    char fname[MAX_STRING_LENGTH];
    ObjData *obj, *containers[MAX_CONTAINER_DEPTH + 1];
    int location, depth, i;

    if (!get_corpse_filename(id, fname)) {
        log("SYSERR: invalid id passed from load_corpse.");
        return nullptr;
    }
    if (!(fl = fopen(fname, "r"))) {
        log("SYSERR: Corpse in control file not located on disk.");
        return nullptr;
    }

    get_line(fl, buf1);
    if (is_integer(buf1)) {
        depth = atoi(buf1);
        if ((depth = real_room(depth)) < 0) {
            sprintf(buf, "SYSERR: Unable to locate room %s for corpse %d", buf1, id);
            log("%s", buf);
            ;
            depth = r_mortal_start_room;
        }
    } else {
        sprintf(buf, "SYSERR: First line of corpse file not room vnum for corpse %d", id);
        log("%s", buf);
        ;
        if (strchr(buf1, ':'))
            rewind(fl);
        depth = r_mortal_start_room;
    }

    if (build_object(fl, &obj, &location)) {
        if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER || !strstr(obj->name, "corpse")) {
            sprintf(buf, "SYSERR: First object '%s' loaded from corpse %d not corpse", obj->short_description, id);
            log("%s", buf);
            ;
            extract_obj(obj);
            return nullptr;
        }
        containers[0] = obj;
        GET_OBJ_VAL(obj, VAL_CORPSE_ID) = id;
        GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) = CORPSE_PC;
        obj_to_room(obj, depth);
    } else {
        sprintf(buf, "SYSERR: Unable to read in corpse data for corpse %d in load_corpse", id);
        log("%s", buf);
        ;
        return nullptr;
    }

    while (!feof(fl)) {
        if (!build_object(fl, &obj, &location))
            break;
        depth = MAX(0, -location);
        for (i = MAX_CONTAINER_DEPTH - 1; i >= depth; --i)
            containers[i] = nullptr;
        containers[depth] = obj;
        if (depth > 0)
            obj_to_obj(obj, containers[depth - 1]);
    }

    fclose(fl);

    /* Ensure that the items inside aren't marked for decomposition. */
    stop_decomposing(containers[0]);
    /* And mark the corpse itself as decomposing. */
    SET_FLAG(GET_OBJ_FLAGS(containers[0]), ITEM_DECOMP);

    return containers[0];
}

void save_corpse(ObjData *corpse) {
    char buf[MAX_STRING_LENGTH];
    FILE *fp;
    int id;
    ObjData *temp;

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
    fprintf(fp, "%d\n", corpse->in_room == NOWHERE ? NOWHERE : world[corpse->in_room].vnum);

    /*
     * Warning!  Hack:  write_objects writes out corpse->next_content.
     * But we don't want to write out other objects in the room where
     * the corpse is, so we'll temporarily set next_content to NULL.
     */
    temp = corpse->next_content;
    corpse->next_content = nullptr;
    write_objects(corpse, fp, WEAR_INVENTORY);
    corpse->next_content = temp;

    fclose(fp);
}

/* Delete a corpse save file */
static void delete_corpse_file(int id) {
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
static corpse_data *find_entry(int id) {
    corpse_data *entry;

    for (entry = SENTINEL->next; entry != SENTINEL; entry = entry->next)
        if (entry->id == id)
            return entry;

    return nullptr;
}

/* Save the corpse control information */
static void save_corpse_list(void) {
    FILE *fl;
    corpse_data *entry;

    if (!(fl = fopen(CCONTROL_FILE, "w"))) {
        perror("SYSERR: Unable to open corpse control file");
        return;
    }

    for (entry = SENTINEL->next; entry != SENTINEL; entry = entry->next) {
        check_corpse_id(entry);
        fprintf(fl, "%d\n", entry->id);
    }

    fclose(fl);
}

/* 8/5/99 David Endre - Fix it so more than one corpse is saved over
   a reboot. The problem was the old code called obj_to_room which
   saves the ccontrol file and since only one corpse is in the world
   it saves that in the ccontrol file and never gets a chance to boot
   another corpse */

/* call from boot_db - will load control recs, load corpses, load objs */
/* does sanity checks on vnums & removes invalid records */
void boot_corpses(void) {
    FILE *fl;
    int id;
    corpse_data *entry;
    ObjData *corpse;

    memset(&corpse_control, 0x0, sizeof(corpse_control));
    SENTINEL->next = SENTINEL->prev = SENTINEL;

    if (!(fl = fopen(CCONTROL_FILE, "rb"))) {
        log("Corpse control file does not exist.");
        return;
    }

    while (get_line(fl, buf)) {
        id = atoi(buf);

        if (!(corpse = load_corpse(id))) {
            sprintf(buf, "SYSERR: Unable to load corpse %d in corpse control list", id);
            log("%s", buf);
            ;
            continue;
        }

        CREATE(entry, corpse_data, 1);
        entry->id = id;
        entry->corpse = corpse;

        SENTINEL->prev->next = entry;
        entry->prev = SENTINEL->prev;
        SENTINEL->prev = entry;
        entry->next = SENTINEL;

        ++corpse_control.count;
    }
    fclose(fl);

    corpse_control.allow_save = true;

    save_corpse_list();
}

/* When a player dies, this function is called from make_corpse in fight.c */
void register_corpse(ObjData *corpse) {
    corpse_data *entry;

    if (GET_OBJ_VAL(corpse, VAL_CORPSE_ID)) {
        for (entry = SENTINEL->next; entry != SENTINEL; entry = entry->next)
            if (entry->corpse == corpse) {
                log("SYSERR: already registered corpse passed to register_corpse");
                return;
            }
    }

    GET_OBJ_VAL(corpse, VAL_CORPSE_ID) = SENTINEL->prev->id + 1;

    CREATE(entry, corpse_data, 1);
    entry->id = GET_OBJ_VAL(corpse, VAL_CORPSE_ID);
    entry->corpse = corpse;

    SENTINEL->prev->next = entry;
    entry->prev = SENTINEL->prev;
    SENTINEL->prev = entry;
    entry->next = SENTINEL;

    ++corpse_control.count;

    save_corpse(corpse);

    save_corpse_list();
}

/* called from extract_obj when corpse rots or is otherwise removed from play */
void destroy_corpse(ObjData *corpse) {
    int id = corpse_id(corpse);
    corpse_data *entry = find_entry(id);

    if (entry) {
        remove_entry(entry);
        save_corpse_list();
        delete_corpse_file(id);
    }
}

void show_corpses(CharData *ch, char *argument) {
    corpse_data *entry;

    if (corpse_control.count) {
        send_to_char(
            "Id  Corpse              Level  Decomp  Location\n"
            "-------------------------------------------------------------"
            "------\n",
            ch);
        for (entry = SENTINEL->next; entry != SENTINEL; entry = entry->next) {
            if (!strncasecmp(entry->corpse->short_description, "the corpse of ", 14))
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
            char_printf(ch, "%-4d%-20.20s%5d  %6d  %25s\n", entry->id, buf1, GET_OBJ_LEVEL(entry->corpse),
                        GET_OBJ_DECOMP(entry->corpse), buf2);
        }
    } else
        send_to_char("There are no player corpses in the game.\n", ch);
}

/***************************************************************************
 *   File: dg_handler.c                                   Part of FieryMUD *
 *  Usage: Auxillary trigger functions                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 2000 by the Fiery Consortium                    *
 ***************************************************************************/

#include "comm.h"
#include "conf.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "handler.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig) {
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
void extract_script(struct script_data *sc) {
    struct trig_data *trig, *next_trig;

    for (trig = TRIGGERS(sc); trig; trig = next_trig) {
        next_trig = trig->next;
        extract_trigger(trig);
    }
    TRIGGERS(sc) = NULL;

    free_varlist(sc->global_vars);
    free(sc);
}
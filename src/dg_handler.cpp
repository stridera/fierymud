/***************************************************************************
 *   File: dg_handler.c                                   Part of FieryMUD *
 *  Usage: Auxillary trigger functions                                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 2000 by the Fiery Consortium                    *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "handler.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* remove a single trigger from a mob/obj/room */
void extract_trigger(TrigData *trig) {
    TrigData *temp;

    if (GET_TRIG_WAIT(trig)) {
        event_cancel(GET_TRIG_WAIT(trig));
        GET_TRIG_WAIT(trig) = nullptr;
    }

    trig_index[trig->nr]->number--;

    /* walk the trigger list and remove this one */
    REMOVE_FROM_LIST(trig, trigger_list, next_in_world);

    if (trig->running == true)
        trig->purged = true;
    else
        free_trigger(trig);
}

/* remove all triggers from a mob/obj/room */
void extract_script(ScriptData *sc) {
    TrigData *trig, *next_trig;

    for (trig = TRIGGERS(sc); trig; trig = next_trig) {
        next_trig = trig->next;
        extract_trigger(trig);
    }
    TRIGGERS(sc) = nullptr;

    free_varlist(sc->global_vars);
    free(sc);
}

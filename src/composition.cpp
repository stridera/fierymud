/***************************************************************************
 *  File: composition.c                                  Part of FieryMUD  *
 *  Usage: Source file for characters composition                          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "composition.hpp"

#include "act.hpp"
#include "casting.hpp"
#include "class.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "events.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "magic.hpp"
#include "math.hpp"
#include "movement.hpp"
#include "races.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Values for the compdef struct:
 *
 *   Name, Mass noun, Adjective, Color, default_dtype, phase
 *   SUSCEPTIBILITY: slash, pierce, crush, shock, fire, water, cold, acid, poison
 */

int parse_composition(CharData *ch, const char *arg) {
    const char *comp_name = "composition";
    return parse_obj_name(ch, arg, comp_name, NUM_COMPOSITIONS, compositions, sizeof(CompositionDef));
}

void set_base_composition(CharData *ch, int newcomposition) {
    BASE_COMPOSITION(ch) = newcomposition;
    effect_total(ch);
}

/* This function is an intermediate one, for making modifications without
 * checking their consequences.  That will be done later. */
void convert_composition(CharData *ch, int newcomposition) { GET_COMPOSITION(ch) = newcomposition; }

void list_olc_compositions(CharData *ch) {
    int i;

    for (i = 0; i < NUM_COMPOSITIONS; i++) {
        sprintf(buf, "&2% 2d&0) %s%c%s&0\r\n", i, compositions[i].color, UPPER(*(compositions[i].name)),
                compositions[i].name + 1);
        send_to_char(buf, ch);
    }
}

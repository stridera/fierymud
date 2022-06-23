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

#include "composition.h"

#include "act.h"
#include "casting.h"
#include "class.h"
#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "damage.h"
#include "db.h"
#include "dg_scripts.h"
#include "events.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "magic.h"
#include "math.h"
#include "movement.h"
#include "races.h"
#include "screen.h"
#include "skills.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

/* Values for the compdef struct:
 *
 *   Name, Mass noun, Adjective, Color, default_dtype, phase
 *   SUSCEPTIBILITY: slash, pierce, crush, shock, fire, water, cold, acid, poison
 */

struct compdef compositions[NUM_COMPOSITIONS] = {
    {"flesh", "flesh", "fleshy", "&1", DAM_CRUSH, PHASE_SOLID, 100, 100, 100, 100, 100, 100, 100, 100, 100},
    {"earth", "earth", "earthy", "&3", DAM_CRUSH, PHASE_SOLID, 90, 120, 50, 75, 75, 120, 40, 80, 0},
    {"air", "air", "gaseous", "&6", DAM_SHOCK, PHASE_GAS, 20, 20, 20, 0, 120, 75, 0, 0, 0},
    {"fire", "fire", "fiery", "&1&b", DAM_FIRE, PHASE_PLASMA, 30, 30, 30, 75, 0, 120, 100, 0, 0},
    {"water", "water", "watery", "&4&b", DAM_WATER, PHASE_LIQUID, 120, 60, 40, 100, 50, 0, 120, 0, 0},
    {"ice", "ice", "icy", "&4", DAM_CRUSH, PHASE_SOLID, 75, 90, 120, 100, 75, 0, 0, 0, 0},
    {"mist", "mist", "misty", "&6&b", DAM_CRUSH, PHASE_GAS, 30, 30, 30, 80, 50, 100, 120, 0, 0},
    {"ether", "nothing", "ethereal", "&5", DAM_SLASH, PHASE_ETHER, 0, 0, 0, 75, 75, 50, 25, 0, 0},
    {"metal", "metal", "metallic", "&9&b", DAM_CRUSH, PHASE_SOLID, 25, 40, 75, 100, 25, 30, 50, 120, 0},
    {"stone", "stone", "stony", "&8", DAM_CRUSH, PHASE_SOLID, 50, 75, 90, 0, 50, 75, 50, 100, 0},
    {"bone", "bone", "bony", "&7&b", DAM_CRUSH, PHASE_SOLID, 80, 50, 120, 25, 120, 100, 25, 100, 0},
    {"lava", "lava", "fluid", "&1", DAM_FIRE, PHASE_SOLID, 40, 40, 40, 50, 25, 120, 100, 50, 0},
    {"plant", "plant material", "woody", "&2", DAM_SLASH, PHASE_SOLID, 120, 70, 60, 75, 120, 50, 75, 100, 50}};

int parse_composition(struct char_data *ch, char *arg) {
    return parse_obj_name(ch, arg, "composition", NUM_COMPOSITIONS, compositions, sizeof(struct compdef));
}

void set_base_composition(struct char_data *ch, int newcomposition) {
    BASE_COMPOSITION(ch) = newcomposition;
    effect_total(ch);
}

/* This function is an intermediate one, for making modifications without
 * checking their consequences.  That will be done later. */
void convert_composition(struct char_data *ch, int newcomposition) { GET_COMPOSITION(ch) = newcomposition; }

void list_olc_compositions(struct char_data *ch) {
    int i;

    for (i = 0; i < NUM_COMPOSITIONS; i++) {
        sprintf(buf, "&2% 2d&0) %s%c%s&0\r\n", i, compositions[i].color, UPPER(*(compositions[i].name)),
                compositions[i].name + 1);
        send_to_char(buf, ch);
    }
}

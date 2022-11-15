/***************************************************************************
 *  File: lifeforce.c                                    Part of FieryMUD  *
 *  Usage: Source file for character life forces                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "lifeforce.hpp"

#include "conf.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

/* Values:
 *
 *   Name, Color,
 *   SUSCEPTIBILITY: heal, discorporate, dispel magic, mental
 */

LifeDef lifeforces[NUM_LIFEFORCES] = {{"life", "&2&b", 100, 0, 0, 100},    {"undead", "&9&b", 75, 50, 0, 100},
                                      {"magic", "&4&b", 0, 120, 50, 0},    {"celestial", "&6", 100, 50, 0, 75},
                                      {"demonic", "&1&b", 100, 50, 0, 75}, {"elemental", "&3", 50, 100, 0, 50}};

int parse_lifeforce(CharData *ch, char *arg) {
    return parse_obj_name(ch, arg, "life force", NUM_LIFEFORCES, lifeforces, sizeof(LifeDef));
}

void convert_lifeforce(CharData *ch, int newlifeforce) {
    /* Nothing complicated yet! */
    GET_LIFEFORCE(ch) = newlifeforce;
}

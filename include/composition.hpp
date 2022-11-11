/***************************************************************************
 *  File: composition.h                                   Part of FieryMUD *
 *  Usage: header file for character composition                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "damage.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define COMP_UNDEFINED -1
#define COMP_FLESH 0
#define COMP_EARTH 1
#define COMP_AIR 2
#define COMP_FIRE 3
#define COMP_WATER 4
#define COMP_ICE 5
#define COMP_MIST 6
#define COMP_ETHER 7 /* Having no physical incorporation */
#define COMP_METAL 8
#define COMP_STONE 9 /* Like earth, but tougher */
#define COMP_BONE 10 /* Like flesh, but... */
#define COMP_LAVA 11
#define COMP_PLANT 12
#define NUM_COMPOSITIONS 13 /* keep updated */

#define PHASE_ETHER 0
#define PHASE_PLASMA 1
#define PHASE_GAS 2
#define PHASE_LIQUID 3
#define PHASE_SOLID 4

/* This struct exists so that additional attributes may be added to
 * each compdef in the future.  At the same time, having an array
 * of structs will allow parse_composition() to simply call parse_obj_name(). */
struct CompositionDef {
    const char *name;
    const char *massnoun;
    const char *adjective;
    const char *color;
    int default_dtype;
    /* Helps determine whether physical attacks will "pass through" or "bounce
     * off". Also whether you leave a corpse behind. */
    int phase;
    /* Susceptibilities to different kinds of damage */
    int sus_slash;
    int sus_pierce;
    int sus_crush;
    int sus_shock;
    int sus_fire;
    int sus_water;
    int sus_cold;
    int sus_acid;
    int sus_poison;
};

struct CompositionDef compositions[NUM_COMPOSITIONS] = {
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

int parse_composition(CharData *ch, char *arg);
void set_base_composition(CharData *ch, int newcomposition);
void convert_composition(CharData *ch, int newcomposition);
void list_olc_compositions(CharData *ch);

#define BASE_COMPOSITION(ch) ((ch)->player.base_composition)
#define VALID_COMPOSITIONNUM(num) ((num) >= 0 && (num) < NUM_COMPOSITIONS)
#define VALID_COMPOSITION(ch) (VALID_COMPOSITIONNUM(GET_COMPOSITION(ch)))
#define COMPOSITION_NAME(ch) (VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].name : "<INVALID COMPOSITION>")
#define COMPOSITION_MASS(ch)                                                                                           \
    (VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].massnoun : "<INVALID COMPOSITION>")
#define COMPOSITION_COLOR(ch)                                                                                          \
    (MOB_FLAGGED((ch), MOB_ILLUSORY) ? "&5" : VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].color : "")
#define COMPOSITION_DAM(ch) (VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].default_dtype : DAM_CRUSH)
#define COMPOSITION_ADJECTIVE(ch)                                                                                      \
    (MOB_FLAGGED((ch), MOB_ILLUSORY) ? "illusory"                                                                      \
     : VALID_COMPOSITION(ch)         ? compositions[GET_COMPOSITION(ch)].adjective                                     \
                                     : "existential")
#define RIGID(ch) (VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].phase == PHASE_SOLID : true)
#define PHASE(ch) (VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].phase : PHASE_SOLID)

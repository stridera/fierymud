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

#ifndef __FIERY_COMPOSITION_H
#define __FIERY_COMPOSITION_H

#include "sysdep.h"
#include "structs.h"

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
struct compdef {
    char *name;
    char *massnoun;
    char *adjective;
    char *color;
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

extern struct compdef compositions[];

extern int parse_composition(struct char_data *ch, char *arg);
extern void set_base_composition(struct char_data *ch, int newcomposition);
extern void convert_composition(struct char_data *ch, int newcomposition);
extern void list_olc_compositions(struct char_data *ch);

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
    (MOB_FLAGGED((ch), MOB_ILLUSORY)                                                                                   \
         ? "illusory"                                                                                                  \
         : VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].adjective : "existential")
#define RIGID(ch) (VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].phase == PHASE_SOLID : TRUE)
#define PHASE(ch) (VALID_COMPOSITION(ch) ? compositions[GET_COMPOSITION(ch)].phase : PHASE_SOLID)

#endif

/***************************************************************************
 * $Log: composition.h,v $
 * Revision 1.2  2009/03/11 21:12:58  jps
 * Added a phase (none, plasma, gas, liquid, solid) to compositions.
 *
 * Revision 1.1  2009/03/08 21:42:31  jps
 * Initial revision
 *
 ***************************************************************************/

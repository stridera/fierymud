/***************************************************************************
 * $Id: effects.c,v 1.3 2009/07/18 01:17:23 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *   File: effects.c                                      Part of FieryMUD *
 *  Usage: Constants for effects                                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "effects.h"

#include "conf.h"
#include "sysdep.h"

const char *effect_flags[NUM_EFF_FLAGS + 1] = {"BLIND", /* 0 */
                                               "INVIS",
                                               "DET_ALIGN",
                                               "DET_INVIS",
                                               "DET_MAGIC",
                                               "SENSE_LIFE", /* 5 */
                                               "WATWALK",
                                               "SANCT",
                                               "CONFUSION",
                                               "CURSE",
                                               "INFRA", /* 10 */
                                               "POISON",
                                               "PROT_EVIL",
                                               "PROT_GOOD",
                                               "SLEEP",
                                               "!TRACK", /* 15 */
                                               "TAMED",
                                               "BERSERK",
                                               "SNEAK",
                                               "STEALTH",
                                               "FLY", /* 20 */
                                               "CHARM",
                                               "STONE_SKIN",
                                               "FARSEE",
                                               "HASTE",
                                               "BLUR", /* 25 */
                                               "VITALITY",
                                               "GLORY",
                                               "MAJOR_PARALYSIS",
                                               "FAMILIARITY",
                                               "MESMERIZED", /* 30 */
                                               "IMMOBILIZED",
                                               "LIGHT",
                                               "MAJOR_GROUP",
                                               "MINOR_PARALYSIS",
                                               "HURT_THROAT", /* 35 */
                                               "LEVITATE",
                                               "WATERBREATH",
                                               "SOULSHIELD",
                                               "SILENCE",
                                               "PROT_FIRE", /* 40 */
                                               "PROT_COLD",
                                               "PROT_AIR",
                                               "PROT_EARTH",
                                               "FIRESHIELD",
                                               "COLDSHIELD", /* 45 */
                                               "MINOR_GLOBE",
                                               "MAJOR_GLOBE",
                                               "HARNESS",
                                               "ON_FIRE",
                                               "FEAR", /* 50 */
                                               "TONGUES",
                                               "DISEASE",
                                               "INSANITY",
                                               "ULTRAVISION",
                                               "!HEAT", /* 55 */
                                               "!COLD",
                                               "!AIR",
                                               "!EARTH",
                                               "REMOTE_AGGR",
                                               "UNUSED", /* 60 */
                                               "UNUSED",
                                               "UNUSED",
                                               "UNUSED",
                                               "AWARE",
                                               "REDUCE", /* 65 */
                                               "ENLARGE",
                                               "VAMP",
                                               "ENFEEB",
                                               "ANIMATE",
                                               "UNUSED", /* 70 */
                                               "SHADOW",
                                               "CAMOUFLAGE",
                                               "SPIRIT_WOLF",
                                               "SPIRIT_BEAR",
                                               "WRATH", /* 75 */
                                               "MISDIRECTION",
                                               "MISDIRECTING",
                                               "BLESS",
                                               "HEX",
                                               "\n"};

/***************************************************************************
 * $Log: effects.c,v $
 * Revision 1.3  2009/07/18 01:17:23  myc
 * Adding immobilized flag for bone draw.
 *
 * Revision 1.2  2009/06/11 13:36:05  myc
 * When throatcut is successful, apply an injured throat effect
 * which hinders the victim's casting ability.
 *
 * Revision 1.1  2009/03/09 05:09:40  jps
 * Initial revision
 *
 ***************************************************************************/

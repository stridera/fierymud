/***************************************************************************
 *  File: cooldowns.h                                    Part of FieryMUD  *
 *  Usage: Skill cooldowns                                                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

/*
 * The CD_<NAME> and MAX_COOLDOWNS constants are defined in structs.h
 * because the player structure has an array of cooldowns.
 */

#define PULSE_COOLDOWN (1 RL_SEC)

const char *cooldowns[NUM_COOLDOWNS + 1] = {"backstab",
                                            "bash",
                                            "instant kill",
                                            "disarm",
                                            "fumbling primary weapon",
                                            "dropped primary weapon",
                                            "fumbling secondary weapon",
                                            "dropped secondary weapon",
                                            "summon mount",
                                            "lay hands",
                                            "first aid",
                                            "eye gouge",
                                            "throatcut",
                                            "shapechange",
                                            "chant",
                                            "innate invis",
                                            "innate chaz",
                                            "innate darkness",
                                            "innate levitate",
                                            "innate syll",
                                            "innate tren",
                                            "innate tass",
                                            "innate brill",
                                            "innate ascen",
                                            "innate harness",
                                            "breathe",
                                            "innate create",
                                            "innate illumination",
                                            "innate faerie step",
                                            "music 1",
                                            "music 2",
                                            "music 3",
                                            "music 4",
                                            "music 5",
                                            "music 6",
                                            "music 7",
                                            "innate blinding beauty",
                                            "innate statue"
                                            "\n"};

#define CD_CURRENT 0
#define CD_MAX 1

#define GET_COOLDOWN(ch, i) ((ch)->char_specials.cooldowns[(i)][CD_CURRENT])
#define GET_COOLDOWN_MAX(ch, i) ((ch)->char_specials.cooldowns[(i)][CD_MAX])
void SET_COOLDOWN(CharData *ch, int type, int amount);
void clear_cooldowns(CharData *ch);

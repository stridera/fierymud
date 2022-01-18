/***************************************************************************
 *   File: spell_parser.h                                 Part of FieryMUD *
 *  Usage: Spell parsing, casting, events, and dispatch                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#ifndef __FIERY_SPELL_PARSER_H
#define __FIERY_SPELL_PARSER_H

#include "structs.h"
#include "sysdep.h"

bool valid_cast_stance(struct char_data *ch, int spellnum);

#endif

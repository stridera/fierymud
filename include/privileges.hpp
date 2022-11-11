/***************************************************************************
 *   File: privilegse.h                                   Part of FieryMUD *
 *  Usage: Header for utilities to facilitate the privilege system         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "interpreter.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define PRIV_FUNC(name) void(name)(CharData * ch, int flag)

struct GrantType {
    int grant;
    char *grantor;
    int level;
    GrantType *next;
};

struct privflagdef {
    char *desc;
    int level;
    PRIV_FUNC(*update_func);
};

#define SCMD_GRANT 0
#define SCMD_REVOKE 1
#define SCMD_UNGRANT 2

#define CMD_NOT_GRANTED 0
#define CMD_GRANTED 1
#define CMD_REVOKED 2

ACMD(do_grant);
int command_grant_usability(CharData *ch, int cmd);
void cache_grants(CharData *ch);
void write_player_grants(FILE *fl, CharData *ch);
void read_player_grants(FILE *fl, GrantType **list);
void read_player_grant_groups(FILE *fl, GrantType **list);

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

#ifndef __FIERY_PRIVILEGES_H
#define __FIERY_PRIVILEGES_H

#include "interpreter.h"
#include "structs.h"
#include "sysdep.h"

#define PRIV_FUNC(name) void(name)(struct char_data * ch, int flag)

struct grant_type {
    int grant;
    char *grantor;
    int level;
    struct grant_type *next;
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

extern ACMD(do_grant);
extern int command_grant_usability(struct char_data *ch, int cmd);
extern void cache_grants(struct char_data *ch);
extern void write_player_grants(FILE *fl, struct char_data *ch);
extern void read_player_grants(FILE *fl, struct grant_type **list);
extern void read_player_grant_groups(FILE *fl, struct grant_type **list);

#endif

/***************************************************************************
 *  File: players.h                                       Part of FieryMUD *
 *  Usage: Player loading/saving and utility routines.                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "db.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/* Reasons a player was removed from the game or saved.
 * Reason                    Objects-kept   Keys-kept    Loadroom */
#define QUIT_UNDEF 0
#define QUIT_RENT 1                                               /*  yes           no           here   */
#define QUIT_CRYO 2                                               /*  yes           no           here   */
#define QUIT_TIMEOUT 3                                            /*  yes           no           home   */
#define QUIT_HOTBOOT 4                                            /*  yes           yes          home   */
#define QUIT_QUITMORT 5                                           /*  no            no           temple */
#define QUIT_QUITIMM 6                                            /*  yes           no           here   */
#define QUIT_CAMP 7                                               /*  yes           no           here   */
#define QUIT_WRENT 8                                              /*  yes           yes          here   */
#define QUIT_PURGE 9                                              /*  yes           yes          home   */
#define QUIT_AUTOSAVE 10 /*  yes           yes          last   */ /* Not really quit */
#define NUM_QUITTYPES 11
#define VALID_QUITTYPE(s) ((s) >= 0 && (s) < NUM_QUITTYPES)

void reset_weight(CharData *ch);
void build_player_index(void);
int create_player_index_entry(char *name);
void save_player_index(void);
void free_player_index(void);

long get_ptable_by_name(const char *name);
long get_id_by_name(const char *name);
char *get_name_by_id(long id);
int get_pfilename(const char *name, char *filename, int mode);

int load_player(const char *name, CharData *ch);
void save_player_char(CharData *ch);
void delete_player(int pfilepos);
void rename_player(CharData *victim, char *newname);
void init_player(CharData *ch);
void start_player(CharData *ch);

void add_perm_title(CharData *ch, char *line);

template <size_t N> void load_ascii_flags(std::bitset<N> &flags, char *line) {
    int i = 0;

    skip_spaces(&line);

    line = strtok(line, " ");

    while (line && *line) {
        if (flags.size() <= i) {
            if (*line != '0') {
                log("SYSERR: load_ascii_flags: attempting to read in flags for block {:d}, but only {} blocks allowed "
                    "for std::bitset type",
                    i, flags.size());
            }
        } else
            flags[i] = asciiflag_conv(line);
        line = strtok(nullptr, " ");
        ++i;
    }
}

template <size_t N> void write_ascii_flags(FILE *fl, const std::bitset<N> &flags) {
    char flagbuf[flags.size() + 1];

    for (int i = 0; i < flags.size(); ++i) {
        sprintascii(flagbuf, flags);
        fprintf(fl, "%s%s", i ? " " : "", flagbuf);
    }
}
void remove_player_from_game(CharData *ch, int removal_mode);
void send_save_description(CharData *ch, CharData *dest, bool entering);

/* get_pfilename() codes */
#define OBJ_FILE 0
#define PLR_FILE 1
#define QUEST_FILE 2
#define NOTES_FILE 3
#define TEMP_FILE 4
#define PET_FILE 5
#define NUM_PLR_FILES 6

/* player index flags */
#define PINDEX_FROZEN (1 << 0)
#define PINDEX_NEWNAME (1 << 1)
#define PINDEX_NAPPROVE (1 << 2)
#define PINDEX_DELETED (1 << 3)
#define PINDEX_NODELETE (1 << 4)
#define NUM_PINDEX_FLAGS 5

/* Changing the values below may screw up existing ascii player files. */

#define DEFAULT_PAGE_LENGTH 0
#define PFDEF_HUNGER 0
#define PFDEF_THIRST 0
#define PFDEF_DRUNK 0
#define PFDEF_COINS 0
#define PFDEF_BANK 0

#define MAX_TITLE_WIDTH 60
#define WIZ_TITLE_WIDTH 12

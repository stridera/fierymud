/***************************************************************************
 *   File: board.h                                        Part of FieryMUD *
 *  Usage: header file for advanced bulletin boards                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/
#pragma once

#include "rules.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#define MAX_MSG_LEN 10000
#define BOARD_PREFIX "etc/boards"
#define BOARD_SUFFIX ".brd"
#define BOARD_INDEX_FILE "etc/boards/index"

/*
 * Board privileges; each board can be assigned a different privilege
 * type for each privilege.
 */
#define BPRIV_READ 0
#define BPRIV_WRITE_NEW 1
#define BPRIV_REMOVE_OWN 2
#define BPRIV_EDIT_OWN 3
#define BPRIV_REMOVE_ANY 4
#define BPRIV_EDIT_ANY 5
#define BPRIV_WRITE_STICKY 6
#define BPRIV_LOCK 7
#define NUM_BPRIV 8

#define VALID_BOARD_NUM(num) ((num) > 0)
#define VALID_BOARD_INDEX(idx) ((idx) >= 0 && (idx) < num_boards)
#define VALID_PRIV_NUM(num) ((num) >= 0 && (num) < NUM_BPRIV)
#define VALID_ALIAS_CHAR(c) (IS_UPPER(c) || IS_LOWER(c) || isdigit(c) || (c) == '_')

/*
 * These structures are private to board.c
 */

/* list of edits made to a message */
struct BoardMessageEdit {
    char *editor;
    time_t time;
    BoardMessageEdit *next;
};

struct BoardIter {
    int index;
};

/* board message */
struct BoardMessage {
    char *poster; /* player name */
    int level;    /* player's level */
    time_t time;  /* time of posting */
    char *subject;
    char *message;
    bool sticky; /* sticky messages float towards the top of the board */
    BoardMessageEdit *edits;
    CharData *editing; /* message currently being edited by... */
};

struct BoardData {
    int number; /* board id (i.e., vnum) */

    Rule *privileges[NUM_BPRIV];

    char *alias; /* letters, numbers, underscore only; used as filename */
    char *title; /* any characters */

    BoardMessage **messages;
    int message_count;

    bool locked; /* locked by moderator */
    int editing; /* number of people editing messages on this board */
};

/* editing meta data, goes in editor's callback data */
struct board_editing_data {
    BoardData *board;
    BoardMessage *message;
    char *subject;
    bool sticky;
};

BoardData *board(int num);
BoardMessage *board_message(const BoardData *board, int num);

BoardIter *board_iterator();
const BoardData *next_board(BoardIter *iter);
void free_board_iterator(BoardIter *iter);

int board_count();
bool valid_alias(const char *alias);

void board_init();
void board_cleanup();
void save_board_index();
void save_board(BoardData *board);

void look_at_board(CharData *ch, const BoardData *board, const ObjData *face);
void read_message(CharData *ch, BoardData *board, int msg);
void edit_message(CharData *ch, BoardData *board, int msg);
void remove_message(CharData *ch, BoardData *board, int msg, const ObjData *face);
void write_message(CharData *ch, BoardData *board, const char *subject);
bool has_board_privilege(CharData *ch, const BoardData *board, int privnum);

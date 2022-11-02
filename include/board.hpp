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
struct board_message_edit {
    char *editor;
    time_t time;
    board_message_edit *next;
};

struct board_iter {
    int index;
};

/* board message */
struct board_message {
    char *poster; /* player name */
    int level;    /* player's level */
    time_t time;  /* time of posting */
    char *subject;
    char *message;
    bool sticky; /* sticky messages float towards the top of the board */
    board_message_edit *edits;
    char_data *editing; /* message currently being edited by... */
};

struct board_data {
    int number; /* board id (i.e., vnum) */

    rule *privileges[NUM_BPRIV];

    char *alias; /* letters, numbers, underscore only; used as filename */
    char *title; /* any characters */

    board_message **messages;
    int message_count;

    bool locked; /* locked by moderator */
    int editing; /* number of people editing messages on this board */
};

/* editing meta data, goes in editor's callback data */
struct board_editing_data {
    board_data *board;
    board_message *message;
    char *subject;
    bool sticky;
};

// extern board_data *board(int num);
// extern board_message *board_message(const board_data *board, int num);

// extern board_iter *board_iterator();
// extern const board_data *next_board(board_iter *iter);
// extern void free_board_iterator(board_iter *iter);

// extern int board_count();
// extern bool valid_alias(const char *alias);

// extern void board_init();
// extern void board_cleanup();
// extern void save_board_index();
// extern void save_board(board_data *board);

// extern void look_at_board(char_data *ch, const board_data *board, const obj_data *face);
// extern void read_message(char_data *ch, board_data *board, int msg);
// extern void edit_message(char_data *ch, board_data *board, int msg);
// extern void remove_message(char_data *ch, board_data *board, int msg, const obj_data *face);
// extern void write_message(char_data *ch, board_data *board, const char *subject);
// extern bool has_board_privilege(char_data *ch, const board_data *board, int privnum);

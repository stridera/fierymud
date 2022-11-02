/***************************************************************************
 *   File: editor.h                                       Part of FieryMUD *
 *  Usage: Defines for the string editor                                   *
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

enum ed_command_type {
    ED_BEGIN,
    ED_CLEAR,        /* /c */
    ED_FORMAT,       /* /f */
    ED_INSERT_LINE,  /* /i */
    ED_EDIT_LINE,    /* /e */
    ED_REPLACE,      /* /r */
    ED_DELETE_LINE,  /* /d */
    ED_HELP,         /* /h */
    ED_LIST,         /* /l */
    ED_LIST_NUMERIC, /* /n */
    ED_EXIT_SAVE,    /* /s */
    ED_EXIT_ABORT,   /* /a */
    ED_SPELLCHECK,   /* /k */
    ED_OTHER,
    NUM_ED_COMMAND_TYPES
};

enum ed_status_type { ED_IGNORED, ED_PROCESSED, ED_FAILED, NUM_ED_STATUS_TYPES };

enum ed_cleanup_action { ED_NO_ACTION, ED_FREE_DATA, NUM_ED_CLEANUP_ACTIONS };

#define ED_CMD_CHAR '/'
#define ED_DEFAULT_MAX_LINES 100
#define ED_DEFAULT_PAGE_WIDTH 78

#define EDITOR_FUNC(name) enum ed_status_type(name)(editor_context * edit)

struct editor_context {
    struct descriptor_data *descriptor;

    char *string;
    size_t max_length;
    size_t max_lines;
    void *data;

    enum ed_command_type command;
    const char *argument;
};

/* Function prototypes */
EDITOR_FUNC(editor_default_begin);
EDITOR_FUNC(editor_default_clear);
EDITOR_FUNC(editor_default_format);
EDITOR_FUNC(editor_default_insert_line);
EDITOR_FUNC(editor_default_edit_line);
EDITOR_FUNC(editor_default_replace);
EDITOR_FUNC(editor_default_delete_line);
EDITOR_FUNC(editor_default_help);
EDITOR_FUNC(editor_default_list);
EDITOR_FUNC(editor_default_exit);
EDITOR_FUNC(editor_default_spellcheck);
EDITOR_FUNC(editor_default_other);

const struct descriptor_data *editor_edited_by(char **message);
void editor_interpreter(descriptor_data *d, char *line);
void editor_init(descriptor_data *d, char **string, size_t max_length);
void editor_set_callback_data(descriptor_data *d, void *data, enum ed_cleanup_action action);
void editor_set_callback(descriptor_data *d, enum ed_command_type type, EDITOR_FUNC(*callback));
void editor_set_max_lines(descriptor_data *d, size_t max_lines);
void editor_set_begin_string(descriptor_data *d, char *string, ...) __attribute__((format(printf, 2, 3)));
void editor_cleanup(descriptor_data *d);

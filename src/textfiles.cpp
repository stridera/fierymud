/***************************************************************************
 *   File: textfiles.c                                    Part of FieryMUD *
 *  Usage: Text files management                                           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "textfiles.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "editor.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "modify.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

void index_boot(int mode);
int file_to_string_alloc(const char *name, char **buf);
time_t file_last_update(const char *name);
void reload_xnames();

static struct TextFile {
    const char *name;
    const char *path;
    const int level;
    const int max_size;
    char *text; /* auto-initialize to NULL */
    time_t last_update;
} text_files[NUM_TEXT_FILES] = {
    /* Keep this list alphabetized */
    {"anews", "text/anews", LVL_GAMEMASTER, MAX_STRING_LENGTH},
    {"background", "text/background", LVL_ADMIN, MAX_DESC_LENGTH},
    {"credits", "text/credits", LVL_IMPL, MAX_DESC_LENGTH},
    {"handbook", "text/handbook", LVL_GAMEMASTER, MAX_DESC_LENGTH},
    {"help", "text/help/screen", LVL_ADMIN, MAX_DESC_LENGTH},
    {"imotd", "text/imotd", LVL_GAMEMASTER, MAX_DESC_LENGTH},
    {"info", "text/info", LVL_ADMIN, MAX_STRING_LENGTH},
    {"motd", "text/motd", LVL_GAMEMASTER, MAX_DESC_LENGTH},
    {"news", "text/news", LVL_GAMEMASTER, MAX_STRING_LENGTH},
    {"policies", "text/policies", LVL_IMPL, MAX_STRING_LENGTH},
    {"wizlist", "text/wizlist", LVL_ADMIN, MAX_STRING_LENGTH},
};

void boot_text() {
    int i;

    for (i = 0; i < NUM_TEXT_FILES; ++i) {
        file_to_string_alloc(text_files[i].path, &text_files[i].text);
        text_files[i].last_update = file_last_update(text_files[i].path);
    }
}

const char *get_text(int text) { return (text < 0 || text >= NUM_TEXT_FILES) ? "" : text_files[text].text; }

time_t get_text_update_time(int text) {
    return (text < 0 || text >= NUM_TEXT_FILES) ? 0 : text_files[text].last_update;
}

ACMD(do_textview) {
    if (subcmd < 0 || subcmd >= NUM_TEXT_FILES)
        return;

    page_string(ch, text_files[subcmd].text);
}

ACMD(do_reload) {
    const int COLS = 7;

    flagvector files[FLAGVECTOR_SIZE(NUM_TEXT_FILES)];
    unsigned long i;
    bool found, reload_help = false;

    for (i = 0; i < FLAGVECTOR_SIZE(NUM_TEXT_FILES); ++i)
        files[i] = 0;

    any_one_arg(argument, arg);
    if (!str_cmp(arg, "all"))
        SET_FLAGS(files, ALL_FLAGS, NUM_TEXT_FILES);
    else
        for (argument = any_one_arg(argument, arg); *arg; argument = any_one_arg(argument, arg)) {
            found = false;
            for (i = 0; i < NUM_TEXT_FILES; ++i)
                if (!str_cmp(text_files[i].name, arg)) {
                    SET_FLAG(files, i);
                    found = true;
                    break;
                }
            if (!str_cmp(arg, "xhelp"))
                reload_help = true;
            else if (!str_cmp(arg, "xnames")) {
                reload_xnames();
                send_to_char("xnames file reloaded.\r\n", ch);
                return;
            } else if (!found) {
                cprintf(ch, "Unrecognized text file name '%s'.\r\n", arg);
            }
        }

    if (!reload_help && !HAS_FLAGS(files, NUM_TEXT_FILES)) {
        send_to_char("No known text files given.  Text files available:\r\n", ch);
        for (i = 0; i < NUM_TEXT_FILES; ++i)
            cprintf(ch, "%-11.11s%s", text_files[i].name, !((i + COLS) % 7) ? "\r\n" : "");
        cprintf(ch, "xhelp      %s", !(i % COLS) ? "\r\n" : "");
        cprintf(ch, "xnames      %s", !(i % COLS) ? "\r\n" : "");
        if (i % COLS)
            cprintf(ch, "\r\n");
        return;
    }

    for (i = 0; i < NUM_TEXT_FILES; ++i)
        if (IS_FLAGGED(files, i)) {
            file_to_string_alloc(text_files[i].path, &text_files[i].text);
            text_files[i].last_update = file_last_update(text_files[i].path);
            cprintf(ch, "Reloaded %s text from file.\r\n", text_files[i].name);
        }

    if (reload_help) {
        if (help_table) {
            for (i = 0; i <= top_of_helpt; i++) {
                if (help_table[i].keyword)
                    free(help_table[i].keyword);
                if (help_table[i].entry && !help_table[i].duplicate)
                    free(help_table[i].entry);
            }
            free(help_table);
        }
        top_of_helpt = 0;
        index_boot(DB_BOOT_HLP);
    }
}

static EDITOR_FUNC(text_save) {
    DescriptorData *d = edit->descriptor;
    TextFile *text = (TextFile *)edit->data;
    FILE *fl;

    if (edit->command == ED_EXIT_SAVE) {
        if ((fl = fopen(text->path, "w"))) {
            fputs(stripcr(buf1, edit->string), fl);
            mprintf(L_STAT, LVL_GAMEMASTER, "(GC): %s edits file '%s' in tedit", GET_NAME(d->character), text->path);
            fclose(fl);
        } else
            mprintf(L_ERROR, LVL_GAMEMASTER, "SYSERR: Can't write file '%s' for tedit", text->path);
    }

    act("$n stops writing on the large scroll.", true, d->character, 0, 0, TO_ROOM);

    return ED_IGNORED;
}

ACMD(do_tedit) {
    const int COLS = 7;
    int i;

    if (!ch->desc)
        return;

    any_one_arg(argument, arg);

    if (!*arg) {
        send_to_char("Text files available to be edited:\r\n", ch);
        for (i = 0; i < NUM_TEXT_FILES; ++i)
            if (GET_LEVEL(ch) >= text_files[i].level)
                cprintf(ch, "%-11.11s%s", text_files[i].name, !((i + 1) % COLS) ? "\r\n" : "");
        if (i == 0)
            cprintf(ch, "None.\r\n");
        else if (--i % COLS)
            cprintf(ch, "\r\n");
        return;
    }

    for (i = 0; i < NUM_TEXT_FILES; ++i)
        if (GET_LEVEL(ch) >= text_files[i].level)
            if (!str_cmp(arg, text_files[i].name))
                break;

    if (i >= NUM_TEXT_FILES) {
        send_to_char("Invalid text editor option.\r\n", ch);
        return;
    }

    act("$n begins writing on a large scroll.", true, ch, 0, 0, TO_ROOM);

    editor_init(ch->desc, &text_files[i].text, text_files[i].max_size);
    editor_set_begin_string(ch->desc, "Edit file below.");
    editor_set_callback_data(ch->desc, &text_files[i], ED_NO_ACTION);
    editor_set_callback(ch->desc, ED_EXIT_SAVE, text_save);
}

void free_text_files() {
    int i;

    for (i = 0; i < NUM_TEXT_FILES; ++i)
        if (text_files[i].text) {
            free(text_files[i].text);
            text_files[i].text = nullptr;
        }
}

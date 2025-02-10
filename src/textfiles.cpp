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
#include "logging.hpp"
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

    std::bitset<NUM_TEXT_FILES> files;
    unsigned long i;
    bool found, reload_help = false;

    for (i = 0; i < NUM_TEXT_FILES; ++i)
        files[i] = 0;

    any_one_arg(argument, arg);
    if (!strcasecmp(arg, "all"))
        files.set();
    else
        for (argument = any_one_arg(argument, arg); *arg; argument = any_one_arg(argument, arg)) {
            found = false;
            for (i = 0; i < NUM_TEXT_FILES; ++i)
                if (!strcasecmp(text_files[i].name, arg)) {
                    files.set(i);
                    found = true;
                    break;
                }
            if (!strcasecmp(arg, "xhelp"))
                reload_help = true;
            else if (!strcasecmp(arg, "xnames")) {
                reload_xnames();
                char_printf(ch, "xnames file reloaded.\n");
                return;
            } else if (!found) {
                char_printf(ch, "Unrecognized text file name '{}'.\n", arg);
            }
        }

    if (!reload_help && !files.any()) {
        char_printf(ch, "No known text files given.  Text files available:\n");
        for (i = 0; i < NUM_TEXT_FILES; ++i)
            char_printf(ch, "{:<11s}{}", text_files[i].name, !((i + COLS) % 7) ? "\n" : "");
        char_printf(ch, "xhelp      {}", !(i % COLS) ? "\n" : "");
        char_printf(ch, "xnames      {}", !(i % COLS) ? "\n" : "");
        if (i % COLS)
            char_printf(ch, "\n");
        return;
    }

    for (i = 0; i < NUM_TEXT_FILES; ++i)
        if (files.test(i)) {
            file_to_string_alloc(text_files[i].path, &text_files[i].text);
            text_files[i].last_update = file_last_update(text_files[i].path);
            char_printf(ch, "Reloaded {} text from file.\n", text_files[i].name);
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
            log(LogSeverity::Stat, LVL_GAMEMASTER, "(GC): {} edits file '{}' in tedit", GET_NAME(d->character),
                text->path);
            fclose(fl);
        } else
            log(LogSeverity::Error, LVL_GAMEMASTER, "SYSERR: Can't write file '{}' for tedit", text->path);
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
        char_printf(ch, "Text files available to be edited:\n");
        for (i = 0; i < NUM_TEXT_FILES; ++i)
            if (GET_LEVEL(ch) >= text_files[i].level)
                char_printf(ch, "{:<11s}{}", text_files[i].name, !((i + 1) % COLS) ? "\n" : "");
        if (i == 0)
            char_printf(ch, "None.\n");
        else if (--i % COLS)
            char_printf(ch, "\n");
        return;
    }

    for (i = 0; i < NUM_TEXT_FILES; ++i)
        if (GET_LEVEL(ch) >= text_files[i].level)
            if (!strcasecmp(arg, text_files[i].name))
                break;

    if (i >= NUM_TEXT_FILES) {
        char_printf(ch, "Invalid text editor option.\n");
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

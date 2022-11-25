/***************************************************************************
 *   File: editor.c                                       Part of FieryMUD *
 *  Usage: Routines for the string editor                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

/*
 * TODO:
 *   a flag to make default_list escape color characters
 */

#include "editor.hpp"

#include "comm.hpp"
#include "conf.hpp"
#include "events.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

static struct {
    char command;
    EDITOR_FUNC(*default_callback);
} command_types[NUM_ED_COMMAND_TYPES] = {
    {
        '\0',
        editor_default_begin,
    },
    {
        'c',
        editor_default_clear,
    },
    {
        'f',
        editor_default_format,
    },
    {
        'i',
        editor_default_insert_line,
    },
    {
        'e',
        editor_default_edit_line,
    },
    {
        'r',
        editor_default_replace,
    },
    {
        'd',
        editor_default_delete_line,
    },
    {
        'h',
        editor_default_help,
    },
    {
        'l',
        editor_default_list,
    },
    {
        'n',
        editor_default_list,
    },
    {
        's',
        editor_default_exit,
    },
    {
        'a',
        editor_default_exit,
    },
    {
        'k',
        editor_default_spellcheck,
    },
    {
        '\0',
        editor_default_other,
    },
};

static EditorContext *create_context(DescriptorData *d, enum EditorCommandEnum command, const char *argument);
static DescriptorData *consume_context(EditorContext *c);
static void editor_action(DescriptorData *d, enum EditorCommandEnum command, char *argument);
static void editor_append(DescriptorData *d, char *line);
static bool editor_format_text(char **string, int indent, size_t max_length, int first_line, int last_line);
static int editor_replace_string(char **string, char *pattern, char *replacement, bool replace_all, size_t max_length);
static size_t limit_lines(char *string, size_t max_lines);
static EVENTFUNC(editor_start);

void ispell_check(DescriptorData *d, const char *word);

const DescriptorData *editor_edited_by(char **message) {
    DescriptorData *d;

    for (d = descriptor_list; d; d = d->next)
        if (d->editor && d->editor->destination == message)
            return d;

    return nullptr;
}

void editor_init(DescriptorData *d, char **string, size_t max_length) {
    if (!d) {
        log("SYSERR: NULL descriptor passed to editor_init");
        return;
    }

    if (max_length < 1) {
        log("SYSERR: invalid max_length %zu passed to editor_init", max_length);
        return;
    }

    if (EDITING(d)) {
        log("SYSERR: editor_init called on descriptor editing string: forcing "
            "abort");
        editor_cleanup(d);
    }

    CREATE(d->editor, EditorData, 1);

    d->editor->started = false;
    d->editor->string = (string && *string && **string) ? strdup(*string) : nullptr;
    d->editor->destination = string;
    d->editor->begin_string = nullptr;
    d->editor->max_length = max_length;
    d->editor->max_lines = 9999;
    d->editor->data = nullptr;
    d->editor->cleanup_action = ED_NO_ACTION;

    event_create(EVENT_EDITOR_START, editor_start, d, false, nullptr, 0);
}

EVENTFUNC(editor_start) {
    DescriptorData *d = (DescriptorData *)event_obj;

    if (!d)
        log("SYSERR: NULL descriptor passed to editor_start");
    else if (!EDITING(d))
        log("SYSERR: Editor not initialized on descriptor passed to editor_start");
    else if (d->editor->started)
        log("SYSERR: Editor already started on descriptor passed to editor_start");
    else {
        if (!d->editor->action[ED_FORMAT] && d->editor->max_length > MAX_STRING_LENGTH)
            log("WARNING: editor_start: editor max_length is %zu > MAX_STRING_LENGTH;"
                " format command disabled",
                d->editor->max_length);
        d->editor->started = true;
        if (d->editor->string) {
            d->editor->lines = limit_lines(d->editor->string, d->editor->max_lines);
            d->editor->length = strlen(d->editor->string);
        }
        if (d->editor->length > d->editor->max_length) {
            d->editor->string[d->editor->length - 1] = '\0';
            d->editor->length = d->editor->max_length;
        }
        editor_action(d, ED_BEGIN, nullptr);
    }

    return EVENT_FINISHED;
}

void editor_set_callback_data(DescriptorData *d, void *data, enum ed_cleanup_action action) {
    if (!d) {
        log("SYSERR: NULL descriptor passed to editor_set_callback_data");
        return;
    }

    if (!EDITING(d)) {
        log("SYSERR: Editor not initialized on descriptor passed to "
            "editor_set_callback_data");
        return;
    }

    d->editor->data = data;
    d->editor->cleanup_action = action;
}

void editor_set_callback(DescriptorData *d, enum EditorCommandEnum type, EDITOR_FUNC(*callback)) {
    if (!d) {
        log("SYSERR: NULL descriptor passed to editor_set_callback");
        return;
    }

    if (type < 0 || type >= NUM_ED_COMMAND_TYPES) {
        log("SYSERR: Invalid editor command type %d passed to editor_set_callback", type);
        return;
    }

    if (!EDITING(d)) {
        log("SYSERR: Editor not initialized on descriptor passed to "
            "editor_set_callback");
        return;
    }

    d->editor->action[type] = callback;
}

void editor_set_max_lines(DescriptorData *d, size_t max_lines) {
    if (!d) {
        log("SYSERR: NULL descriptor passed to editor_set_max_lines");
        return;
    }

    if (!EDITING(d)) {
        log("SYSERR: Editor not initialized on descriptor passed to editor_set_max_lines");
        return;
    }

    if (max_lines <= 0) {
        log("SYSERR: Invalid maximum number of lines %zu specified in editor_set_max_lines", max_lines);
        return;
    }

    d->editor->max_lines = max_lines;
}

void editor_set_begin_string(DescriptorData *d, const char *string, ...) {
    va_list args;
    char buf[MAX_STRING_LENGTH];

    if (!d) {
        log("SYSERR: NULL descriptor passed to editor_set_begin_string");
        return;
    }

    if (!EDITING(d)) {
        log("SYSERR: Editor not initialized on descriptor passed to "
            "editor_set_begin_string");
        return;
    }

    if (!string) {
        log("SYSERR: NULL string passed to editor_set_begin_string");
        return;
    }

    va_start(args, string);
    vsnprintf(buf, sizeof(buf), string, args);
    va_end(args);

    d->editor->begin_string = strdup(buf);
}

static EditorContext *create_context(DescriptorData *d, enum EditorCommandEnum command, const char *argument) {
    EditorContext *context;

    CREATE(context, EditorContext, 1);
    context->descriptor = d;
    context->string = d->editor->string;
    context->max_length = d->editor->max_length;
    context->data = d->editor->data;
    context->command = command;
    context->argument = argument;

    return context;
}

static DescriptorData *consume_context(EditorContext *c) {
    DescriptorData *d = c->descriptor;

    if ((d->editor->string = c->string)) {
        d->editor->length = strlen(d->editor->string);
        if (d->editor->length >= d->editor->max_length)
            d->editor->string[d->editor->max_length - 1] = '\0';
        d->editor->lines = limit_lines(d->editor->string, d->editor->max_lines);
    } else
        d->editor->lines = d->editor->length = 0;

    d->editor->data = c->data;

    free(c);

    return d;
}

static void editor_action(DescriptorData *d, enum EditorCommandEnum command, char *argument) {
    EditorContext *context;

    if (command < 0 || command >= NUM_ED_COMMAND_TYPES)
        command = ED_OTHER;
    if (!argument)
        argument = "";

    context = create_context(d, command, argument);
    if (d->editor->action[command]) {
        if ((d->editor->action[command])(context) == ED_IGNORED)
            (command_types[command].default_callback)(context);
    } else
        (command_types[command].default_callback)(context);
    consume_context(context);

    if (command == ED_EXIT_SAVE || command == ED_EXIT_ABORT)
        editor_cleanup(d);
}

void editor_interpreter(DescriptorData *d, char *line) {
    char *argument = line;
    size_t i;

    if (!EDITING(d)) {
        log("SYSERR: editor_interpreter invoked on descriptor without allocated "
            "editor member");
        return;
    }

    delete_doubledollar(line);
    smash_tilde(line);
    skip_spaces(&argument);

    if (!d->editor->started)
        editor_action(d, ED_BEGIN, nullptr);

    /* If the line doesn't start with a /, it's not a command: append it */
    if (*argument != ED_CMD_CHAR)
        editor_append(d, line);

    /* Find the correct command to execute */
    else if (*(++argument)) {
        for (i = 0; i < NUM_ED_COMMAND_TYPES; ++i)
            if (*argument == command_types[i].command) {
                editor_action(d, (EditorCommandEnum)i, argument + 1);
                break;
            }
        /* editor action not found above */
        if (i == NUM_ED_COMMAND_TYPES)
            editor_action(d, ED_OTHER, argument);
    }
}

static void editor_append(DescriptorData *d, char *line) {
    size_t line_length = strlen(line) + 2; /* +2 for \n */
    size_t space_left = d->editor->max_length - d->editor->length;
    size_t new_length = d->editor->length + line_length;
    char *orig_string;
    char *new_string;

    if (d->editor->lines >= d->editor->max_lines) {
        desc_printf(d, "Too many lines - Input ignored.  Max lines: %zu lines.\n", d->editor->max_lines);
        return;
    }

    if (line_length >= space_left) {
        /* Shorter than 3?  To short for even a newline.  Ignore line. */
        if (space_left <= 3) {
            desc_printf(d, "String too long - Input ignored.  Max length: %zu characters.\n", d->editor->max_length);
            return;
        }
        desc_printf(d, "String too long - Input truncated.  Max length: %zu characters.\n", d->editor->max_length);
        new_length = d->editor->max_length - 1;
    }

    orig_string = d->editor->string;
    CREATE(new_string, char, new_length + 1);
    if (orig_string)
        strcpy(new_string, orig_string);
    if (line_length >= space_left) {
        line_length = space_left - 1;
        line[line_length + 2] = '\0';
    }
    strcpy(new_string + d->editor->length, line);
    strcpy(new_string + d->editor->length + line_length - 2, "\n");
    if (orig_string)
        free(orig_string);
    d->editor->length += line_length;
    d->editor->string = new_string;
    d->editor->lines++;
}

void editor_cleanup(DescriptorData *d) {
    if (!d) {
        log("SYSERR: NULL descriptor passed to editor_set_other_handler");
        return;
    }

    if (!EDITING(d)) {
        log("SYSERR: Editor not initialized on descriptor passed to "
            "editor_cleanup");
        return;
    }

    if (d->editor->string)
        free(d->editor->string);

    if (d->editor->begin_string)
        free(d->editor->begin_string);

    if (d->editor->cleanup_action == ED_FREE_DATA)
        free(d->editor->data);

    free(d->editor);
    d->editor = nullptr;
}

EDITOR_FUNC(editor_default_begin) {
    desc_printf(edit->descriptor, "%s  (/s saves, /h for help)\n",
                edit->descriptor->editor->begin_string ? edit->descriptor->editor->begin_string
                                                       : "Write your message.");

    editor_action(edit->descriptor, ED_LIST, nullptr);

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_clear) {
    if (edit->string) {
        free(edit->string);
        edit->string = nullptr;
        desc_printf(edit->descriptor, "Current buffer cleared.\n");
    } else
        desc_printf(edit->descriptor, "Current buffer empty.\n");

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_format) {
    int indent = 0;
    int first_line, last_line;

    if (!edit->string) {
        desc_printf(edit->descriptor, "Current buffer empty.\n");
        return ED_PROCESSED;
    }

    /* Does the user want it indented? */
    if (*edit->argument == 'i') {
        ++edit->argument;
        indent = 3;
    }

    /* Does the user want only a portion formatted? */
    switch (sscanf(edit->argument, " %d %d", &first_line, &last_line)) {
    case -1:
    case 0: /* user didn't specify any numbers */
        first_line = 1;
        last_line = 999999;
        break;
    case 1: /* user specified one number */
        last_line = first_line;
        break;
    case 2: /* user specified two numbers */
        if (last_line < first_line) {
            desc_printf(edit->descriptor, "That range is invalid.\n");
            return ED_PROCESSED;
        }
        break;
    }
    first_line = MAX(1, first_line);

    if (editor_format_text(&edit->string, indent, edit->max_length, first_line, last_line))
        desc_printf(edit->descriptor, "Text formatted with%s indent.\n", indent ? "" : "out");
    else
        desc_printf(edit->descriptor, "Text format failed.\n");

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_help) {
    desc_printf(edit->descriptor,
                "Editor command formats: /<letter>\n\n"
                "/a          -  abort editor\n"
                "/c          -  clear buffer\n"
                "/d#         -  delete line #\n"
                "/e# <text>  -  change the line at # with <text>\n"
                "/f          -  format entire text\n"
                "/fi         -  indented formatting of text\n"
                "/h          -  list text editor commands\n"
                "/k <word>   -  spellcheck word\n"
                "/i# <text>  -  insert <text> at line #\n");
    desc_printf(edit->descriptor,
                "/l          -  list entire buffer\n"
                "/n          -  list entire buffer with line numbers\n"
                "/r <a> <b>  -  replace 1st occurrence of text <a> in buffer with "
                "text <b>\n"
                "/ra <a> <b> -  replace all occurrences of text <a> within buffer "
                "with text <b>\n"
                "               usage: /r[a] pattern replacement\n"
                "                      /r[a] 'pattern' 'replacement'\n"
                "                      (enclose in single quotes for multi-word "
                "phrases)\n"
                "/s          -  save text\n");
    desc_printf(edit->descriptor,
                "\n"
                "Note: /d, /f, /fi, /l, and /n also accept ranges of lines.  For "
                "instance:\n"
                "   /d 2 5   -  delete lines 2 through 5\n"
                "   /fi3 6   -  format lines 3 through 6 with indent\n");

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_insert_line) {
    char *str, temp, *start;
    int line, insert_line;
    size_t final_length;
    DescriptorData *d = edit->descriptor;
    const char *argument = edit->argument;

    if (!edit->string) {
        desc_printf(d, "Current buffer empty; nowhere to insert.\n");
        return ED_PROCESSED;
    }

    /* Skip spaces; can't use skip_spaces since argument is const */
    while (*argument && isspace(*argument))
        ++argument;

    if (!isdigit(*argument)) {
        desc_printf(d, "Specify a line number at which to insert.\n");
        return ED_PROCESSED;
    }

    insert_line = atoi(argument);

    if (insert_line <= 0) {
        desc_printf(d, "Line number must be higher than 0.\n");
        return ED_PROCESSED;
    }

    /* Skip over value */
    while (*argument && !isspace(*argument))
        ++argument;
    if (isspace(*argument))
        ++argument; /* skip the first space */
    /* argument now points to what we want to insert */

    final_length = strlen(edit->string) + strlen(argument) + 2;
    if (final_length > edit->max_length) {
        desc_printf(d, "Insert text pushes buffer over maximum size, insert aborted.\n");
        return ED_PROCESSED;
    }

    line = 1;
    str = edit->string;

    while (str && line < insert_line)
        if ((str = strchr(str, '\n'))) {
            ++line;
            ++str;
        }

    if (line < insert_line || !str || !*str) {
        desc_printf(d, "Line number out of range; insert aborted.\n");
        return ED_PROCESSED;
    }

    start = edit->string;
    CREATE(edit->string, char, final_length + 1);

    temp = *str;
    *str = '\0';
    strcat(edit->string, start);
    *str = temp;

    strcat(edit->string, argument);
    strcat(edit->string, "\n");

    strcat(edit->string, str);

    free(start);
    desc_printf(d, "Line inserted.\n");

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_edit_line) {
    char *str, *start, *cut, *cont;
    int line, edit_line;
    size_t final_length;
    DescriptorData *d = edit->descriptor;
    const char *argument = edit->argument;

    if (!edit->string) {
        desc_printf(d, "Specify a line number at which to insert.\n");
        return ED_PROCESSED;
    }

    edit_line = atoi(argument);

    if (edit_line <= 0) {
        desc_printf(d, "Line number must be higher than 0.\n");
        return ED_PROCESSED;
    }

    /* Skip over line number */
    while (*argument && !isspace(*argument))
        ++argument;
    if (isspace(*argument))
        ++argument; /* skip the first space */
    /* argument now points to what we want to insert */

    line = 1;
    str = edit->string;

    while (str && line < edit_line)
        if ((str = strchr(str, '\n'))) {
            ++line;
            ++str;
        }

    start = edit->string;
    cut = str;
    if (str)
        str = strchr(str, '\n');

    if (line < edit_line || !cut || !str) {
        desc_printf(d, "Line number out of range; line replacement aborted.\n");
        return ED_PROCESSED;
    }

    *cut = '\0';
    *str = '\0';
    cont = str + 1;
    final_length = strlen(start) + strlen(argument) + 3 + strlen(cont);

    CREATE(edit->string, char, final_length);
    strcat(edit->string, start);
    strcat(edit->string, argument);
    strcat(edit->string, "\n");
    strcat(edit->string, cont);

    free(start);
    desc_printf(d, "Line replaced.\n");

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_replace) {
    bool replace_all = false;
    char arg[MAX_INPUT_LENGTH], search[MAX_INPUT_LENGTH], replace[MAX_INPUT_LENGTH], *argument;
    int replaced;

    if (!edit->string) {
        desc_printf(edit->descriptor, "Current buffer empty.\n");
        return ED_PROCESSED;
    }

    strcpy(arg, edit->argument);
    argument = arg;

    /* Does the user want it indented? */
    if (*argument == 'a') {
        ++argument;
        replace_all = true;
    }

    argument = delimited_arg_case(argument, search, '\'');
    if (!*search) {
        desc_printf(edit->descriptor, "No target search string.\n");
        return ED_PROCESSED;
    }

    argument = delimited_arg_case(argument, replace, '\'');
    if (!*replace) {
        desc_printf(edit->descriptor, "No replacement string.\n");
        return ED_PROCESSED;
    }

    skip_spaces(&argument);
    if (*argument) {
        desc_printf(edit->descriptor,
                    "Invalid search format.  Enclose search/replacement strings in "
                    "single quotes.\n");
        return ED_PROCESSED;
    }

    replaced = editor_replace_string(&edit->string, search, replace, replace_all, edit->max_length);
    if (replaced > 0)
        desc_printf(edit->descriptor, "Replaced %d occurance%s of '%s' with '%s'.\n", replaced,
                    replaced == 1 ? "" : "s", search, replace);
    else if (replaced == 0)
        desc_printf(edit->descriptor, "String '%s' not found.\n", search);
    else
        desc_printf(edit->descriptor, "Replacement string causes buffer overflow: aborted replace.\n");

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_delete_line) {
    int first_line, last_line, line = 1, total_len = 1;
    DescriptorData *d = edit->descriptor;
    char *start, *str;

    if (!(str = edit->string)) {
        desc_printf(edit->descriptor, "Current buffer empty.\n");
        return ED_PROCESSED;
    }

    switch (sscanf(edit->argument, " %d %d", &first_line, &last_line)) {
    case 2:
        break;
    case 1:
        last_line = first_line;
        break;
    default:
        first_line = 1;
        last_line = 99999;
        break;
    }

    if (first_line < 1) {
        desc_printf(d, "Line numbers must be greater than 0.\n");
        return ED_PROCESSED;
    } else if (last_line < first_line) {
        desc_printf(d, "That line range is invalid.\n");
        return ED_PROCESSED;
    }

    while (str && line < first_line)
        if ((str = strchr(str, '\n'))) {
            ++line;
            ++str;
        }
    if (!str || !*str || line < first_line) {
        desc_printf(d, "Line(s) out of range; not deleting.\n");
        return ED_PROCESSED;
    }
    start = str;
    while (str && line < last_line)
        if ((str = strchr(str, '\n'))) {
            ++line;
            ++total_len;
            ++str;
        }
    if (str && (str = strchr(str, '\n'))) {
        while (*(++str))
            *(start++) = *str;
    } else
        --total_len;
    *start = '\0';
    RECREATE(edit->string, char, strlen(edit->string) + 3);

    desc_printf(d, "%d line%s deleted.\n", total_len, total_len == 1 ? "" : "s");
    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_list) {
    int first_line, last_line, line, lines;
    DescriptorData *d = edit->descriptor;
    bool show_nums = (edit->command == ED_LIST_NUMERIC);
    char *str, *start = nullptr, temp;

    if (!(str = edit->string)) {
        desc_printf(edit->descriptor, "Current buffer empty.\n");
        return ED_PROCESSED;
    }

    switch (sscanf(edit->argument, " %d %d", &first_line, &last_line)) {
    case 2:
        break;
    case 1:
        last_line = first_line;
        break;
    default:
        first_line = 1;
        last_line = 99999;
        break;
    }

    if (first_line < 1) {
        desc_printf(d, "Line numbers must be greater than 0.\n");
        return ED_PROCESSED;
    } else if (last_line < first_line) {
        desc_printf(d, "That line range is invalid.\n");
        return ED_PROCESSED;
    }

    if (first_line > 1 || last_line < 99999)
        pdprintf(d, "Current buffer range [%d - %d]:\n", first_line, last_line);

    line = 1;
    lines = 0;

    while (str && line < first_line)
        if ((str = strchr(str, '\n'))) {
            ++line; /* count line */
            ++str;  /* move past newline */
        }

    if (line < first_line || !str) {
        desc_printf(d, "Line%s out of range; no buffer listing.\n", last_line - first_line ? "(s)" : "");
        return ED_PROCESSED;
    }

    do {
        if (show_nums || !lines)
            start = str;
        str = strchr(str, '\n');
        if (show_nums) {
            if (str) {
                temp = *str;
                *str = '\0';
            }
            pdprintf(d, "%4d: %s\n", line, start);
            if (str)
                *str = temp;
        }
        ++line;
        ++lines;
        if (str)
            ++str;
    } while (str && *str && line <= last_line);

    if (start && *start && !show_nums) {
        if (str) {
            temp = *str;
            *str = '\0';
        }
        pdprintf(d, "%s", start);
        if (str)
            *str = temp;
    }

    start_paging_desc(d);

    return ED_PROCESSED;
}

/*
 * Implementations of ED_EXIT_SAVE *must* set edit->string to NULL
 * to prevent editor_cleanup from freeing it.
 */
EDITOR_FUNC(editor_default_exit) {
    DescriptorData *d = edit->descriptor;

    if (edit->command == ED_EXIT_SAVE) {
        if (!d->editor->destination) {
            desc_printf(d, "ERROR: No location to save changes to.  Edit aborted.\n");
            log("SYSERR: d->editor->destination NULL in save attempt in "
                "editor_default_exit");
        } else {
            desc_printf(d, "Changes saved.\n");
            if (*d->editor->destination)
                /* TODO: do we need a flag to say whether to free this or not? */
                free(*d->editor->destination);
            *d->editor->destination = d->editor->string ? d->editor->string : strdup("Nothing.\n");
            d->editor->string = nullptr;
            edit->string = nullptr;
        }
    } else
        desc_printf(d, "Edit aborted.  Changes not saved.\n");

    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_other) {
    desc_printf(edit->descriptor, "Invalid option.\n");
    return ED_PROCESSED;
}

EDITOR_FUNC(editor_default_spellcheck) {
    const char *ptr = edit->argument;
    char arg[MAX_INPUT_LENGTH + 1];
    int i = 0;

    /* Grab first word from the argument; ignore '+' */
    while (*ptr && (*ptr == ' ' || *ptr == '+'))
        ++ptr;
    while (*ptr && *ptr != ' ' && i < MAX_INPUT_LENGTH)
        arg[i++] = *(ptr++);
    arg[i] = '\0';

    ispell_check(edit->descriptor, arg);
    return ED_PROCESSED;
}

/*
 * Code from stock TBAMUD.
 */
static bool editor_format_text(char **string, int indent, size_t max_length, int first_line, int last_line) {
    int line_chars, color_chars = 0, i;
    bool cap_next = true, cap_next_next = false, pass_line = false;
    char *flow, *start = nullptr, temp;
    char formatted[MAX_STRING_LENGTH + 16] = "";
    char str[MAX_STRING_LENGTH + 16];

    /* Fix memory overrun. */
    if (max_length > MAX_STRING_LENGTH) {
        log("SYSERR: format_text: max_str is greater than buffer size.");
        return 0;
    }

    /* XXX: Want to make sure the string doesn't grow either... */
    if ((flow = *string) == nullptr)
        return 0;

    strcpy(str, flow);

    for (i = 0; i < first_line - 1; i++) {
        start = strtok(str, "\n");
        if (!start)
            return 0;
        strcat(formatted, strcat(start, "\n"));
        flow = strstr(flow, "\n");
        strcpy(str, ++flow);
    }

    if (indent > 0) {
        if (indent > 40) /* arbitrary indentation limit */
            indent = 40;
        line_chars = indent;
        str[indent] = '\0';
        do {
            str[--indent] = ' ';
        } while (indent);
        strcat(formatted, str);
        line_chars = 3;
    } else
        line_chars = 0;

    while (*flow && i < last_line) {
        while (*flow && strchr("\n\r\f\t\v ", *flow)) {
            if (*flow == '\n' && !pass_line)
                if (i++ >= last_line) {
                    pass_line = 1;
                    break;
                }
            flow++;
        }

        if (*flow) {
            start = flow;
            while (*flow && !strchr("\n\r\f\t\v .?!", *flow)) {
                if (*flow == '@') {
                    if (*(flow + 1) == '@')
                        color_chars++;
                    else
                        color_chars += 2;
                    flow++;
                }
                flow++;
            }

            if (cap_next_next) {
                cap_next_next = false;
                cap_next = true;
            }

            /* This is so that if we stopped on a sentence, we move off the sentence
             * delimiter. */
            while (strchr(".!?", *flow)) {
                cap_next_next = true;
                flow++;
            }

            /* Special case: if we're at the end of the last line, and the last
             * character is a delimiter, the flow++ above will have *flow pointing
             * to the \r (or \n) character after the delimiter. Thus *flow will be
             * non-null, and an extra (blank) line might be added erroneously. We
             * fix it by skipping the newline characters in between. - Welcor */
            if (strchr("\n\r", *flow)) {
                *flow = '\0'; /* terminate 'start' string */
                flow++;       /* we know this is safe     */
                if (*flow == '\n' && i++ >= last_line)
                    pass_line = 1;

                while (*flow && strchr("\n\r", *flow) && !pass_line) {
                    flow++; /* skip to next non-delimiter */
                    if (*flow == '\n' && i++ >= last_line)
                        pass_line = 1;
                }
                temp = *flow; /* save this char             */
            } else {
                temp = *flow;
                *flow = '\0';
            }

            if (line_chars + strlen(start) + 1 - color_chars > ED_DEFAULT_PAGE_WIDTH) {
                strcat(formatted, "\n");
                line_chars = 0;
                color_chars = count_color_chars(start);
            }

            if (cap_next) {
                cap_next = false;
                CAP(start);
            } else if (line_chars > 0) {
                strcat(formatted, " ");
                line_chars++;
            }

            line_chars += strlen(start);
            strcat(formatted, start);

            *flow = temp;
        }

        if (cap_next_next && *flow) {
            if (line_chars + 3 - color_chars > ED_DEFAULT_PAGE_WIDTH) {
                strcat(formatted, "\n");
                line_chars = 0;
                color_chars = count_color_chars(start);
            } else if (*flow == '\"' || *flow == '\'') {
                char buf[4];
                sprintf(buf, "%c  ", *flow);
                strcat(formatted, buf);
                flow++;
                line_chars++;
            } else {
                strcat(formatted, "  ");
                line_chars += 2;
            }
        }
    }

    if (*flow)
        strcat(formatted, "\n");
    strcat(formatted, flow);
    if (!*flow)
        strcat(formatted, "\n");

    if (strlen(formatted) + 1 > max_length) {
        /* Make sure the string is nul- and newline-terminated */
        formatted[max_length - 1] = '\0';
        formatted[max_length - 2] = '\n';
        formatted[max_length - 3] = '\r';
    }
    RECREATE(*string, char, MIN(max_length, strlen(formatted) + 1));
    strcpy(*string, formatted);
    return 1;
}

/*
 * Code from stock TBAMUD.
 */
static int editor_replace_string(char **string, char *pattern, char *replacement, bool replace_all, size_t max_length) {
    char *replace_buffer = nullptr;
    char *flow, *jetsam, temp;
    int len, i;

    if (strlen(*string) - strlen(pattern) + strlen(replacement) > max_length)
        return -1;

    CREATE(replace_buffer, char, max_length);
    i = 0;
    jetsam = *string;
    flow = *string;

    if (replace_all) {
        while ((flow = strstr(flow, pattern)) != nullptr) {
            ++i;
            temp = *flow;
            *flow = '\0';
            if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_length) {
                i = -1;
                break;
            }
            strcat(replace_buffer, jetsam);
            strcat(replace_buffer, replacement);
            *flow = temp;
            flow += strlen(pattern);
            jetsam = flow;
        }
        strcat(replace_buffer, jetsam);
    } else if ((flow = strstr(*string, pattern)) != nullptr) {
        ++i;
        flow += strlen(pattern);
        len = (flow - *string) - strlen(pattern);
        strncat(replace_buffer, *string, len);
        strcat(replace_buffer, replacement);
        strcat(replace_buffer, flow);
    }

    if (i > 0) {
        RECREATE(*string, char, strlen(replace_buffer) + 3);
        strcpy(*string, replace_buffer);
    }
    free(replace_buffer);
    return i;
}

static size_t limit_lines(char *string, size_t max_lines) {
    int lines = 0;

    if (string) {
        string = strchr(string, '\n');
        while (string) {
            if (++lines == max_lines) {
                *(++string) = '\0';
                break;
            }
            string = strchr(string + 1, '\n');
        }
    }

    return lines;
}

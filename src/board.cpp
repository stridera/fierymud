/***************************************************************************
 *   File: board.c                                        Part of FieryMUD *
 *  Usage: Advanced board management                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#define __BOARD_C__

#include "board.hpp"

#include "clan.hpp"
#include "comm.hpp"
#include "commands.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "editor.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "objects.hpp"
#include "rules.hpp"
#include "screen.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "vsearch.hpp" /* for ellipsis */

#include <sys/stat.h> /* for mkdir */

/******* BOARD VARIABLES *******/

static EDITOR_FUNC(board_save);
static EDITOR_FUNC(board_list);
static EDITOR_FUNC(board_help);
static EDITOR_FUNC(board_special);

/******* BOARD VARIABLES *******/
static int num_boards = 0;
static BoardData **board_index = nullptr; /* array of pointers to boards */
static BoardData null_board;              /* public undefined board */

static const struct privilege_info {
    char abbr[5];
    const char *alias;
    const char *message;
} privilege_data[NUM_BPRIV] = {
    {"READ", "read", "read"},
    {"WRIT", "write-new", "write new"},
    {"REM", "remove-own", "remove your own"},
    {"EDIT", "edit-own", "edit your own"},
    {"RANY", "remove-any", "remove any"},
    {"EANY", "edit-any", "edit any"},
    {"STKY", "write-sticky", "write sticky"},
    {"LOCK", "lock", "lock"},
};

/******* STRUCTURAL INTERFACE FUNCTIONS *******/

bool valid_alias(const char *alias) {
    if (!alias || !*alias)
        return false;
    for (; *alias; ++alias)
        if (!VALID_ALIAS_CHAR(*alias))
            return false;
    return true;
}

static void free_message(BoardMessage *msg) {
    BoardMessageEdit *edit;
    free(msg->poster);
    free(msg->subject);
    free(msg->message);

    while ((edit = msg->edits)) {
        msg->edits = edit->next;
        free(edit->editor);
        free(edit);
    }

    free(msg);
}

static void free_board(BoardData *board) {
    int i;

    if (board == &null_board) {
        log("SYSERR: attempt to free null_board");
        return;
    }

    free(board->alias);
    free(board->title);

    // for (i = 0; i < NUM_BPRIV; ++i)
    //     free_rule(board->privileges[i]);

    for (i = 0; i < board->message_count; ++i)
        free_message(board->messages[i]);
    free(board->messages);

    free(board);
}

void board_cleanup() {
    int i;

    for (i = 0; i < num_boards; ++i)
        free_board(board_index[i]);

    free(board_index);
    board_index = nullptr;
    num_boards = 0;
}

static BoardData *new_board(const char *alias) {
    BoardData *board;
    size_t priv;

    if (!valid_alias(alias))
        return nullptr;

    ++num_boards;
    RECREATE(board_index, BoardData *, num_boards);
    CREATE(board, BoardData, 1);

    board->number = (num_boards > 1 ? board_index[num_boards - 2]->number : 0) + 1;
    board->alias = strdup(alias);
    board->title = strdup("Untitled Board");

    // for (priv = 0; priv < NUM_BPRIV; ++priv)
    //     board->privileges[priv] = make_level_rule(0, LVL_IMPL);

    board_index[num_boards - 1] = board;

    save_board_index();
    save_board(board);

    return board;
}

static bool delete_board(BoardData *board) {
    int i;
    bool found = false;
    char filename[MAX_INPUT_LENGTH];
    char bkupname[MAX_INPUT_LENGTH];

    if (board->editing)
        return false;

    for (i = 0; i < num_boards; ++i) {
        if (found)
            board_index[i - 1] = board_index[i];
        else if (board_index[i] == board) {
            board_index[i] = nullptr;
            found = true;
        }
    }

    if (!found)
        return false;

    --num_boards;
    board_index[num_boards] = nullptr;

    sprintf(filename, BOARD_PREFIX "/%s" BOARD_SUFFIX, board->alias);
    sprintf(bkupname, BOARD_PREFIX "/%s.bak", board->alias);
    if (rename(filename, bkupname))
        log("SYSERR: Error renaming board file %s to backup name", filename);

    free_board(board);

    save_board_index();

    return true;
}

BoardData *board(int num) {
    int i;

    /* Maybe replace this with a binary search if we get a lot of boards. */
    if (num > 0)
        for (i = 0; i < num_boards; ++i)
            if (board_index[i]->number == num)
                return board_index[i];

    return &null_board;
}

static BoardData *find_board(const char *str) {
    int i;

    if (!str || !*str)
        return nullptr;
    else if (is_integer(str))
        return board(atoi(str));
    else {
        for (i = 0; i < num_boards; ++i)
            if (!strcasecmp(board_index[i]->alias, str))
                return board_index[i];
        return nullptr;
    }
}

BoardMessage *board_message(const BoardData *board, int num) {
    if (board && num > 0 && num <= board->message_count)
        return board->messages[num - 1];
    else
        return nullptr;
}

BoardIter *board_iterator() {
    BoardIter *iter;
    CREATE(iter, BoardIter, 1);
    return iter;
}

const BoardData *next_board(BoardIter *iter) {
    if (!VALID_BOARD_INDEX(iter->index))
        return nullptr;
    return board_index[iter->index++];
}

void free_board_iterator(BoardIter *iter) { free(iter); }

int board_count() { return num_boards; }

bool has_board_privilege(CharData *ch, const BoardData *board, int privnum) {
    // const Rule *priv;

    if (!VALID_PRIV_NUM(privnum)) {
        log("SYSERR: invalid privilege %d passed to has_board_privilege", privnum);
        return false;
    }

    if (can_use_command(ch, find_command("boardadmin")))
        return true;

    return true;

    // priv = board->privileges[privnum];

    // return rule_matches(priv, ch);
}

static bool delete_message(BoardData *board, BoardMessage *msg) {
    int i;
    bool found = false;

    for (i = 0; i < board->message_count; ++i)
        if (found)
            board->messages[i - 1] = board->messages[i];
        else if (board->messages[i] == msg)
            found = true;

    if (found)
        board->message_count--;

    free_message(msg);

    return found;
}

static BoardMessage *add_new_message(BoardData *board, CharData *ch, char *subject, char *message) {
    BoardMessage *msg;

    CREATE(msg, BoardMessage, 1);
    msg->poster = strdup(GET_NAME(ch));
    msg->level = GET_LEVEL(ch);
    msg->time = time(0);
    msg->subject = subject ? subject : strdup("");
    msg->message = message ? message : strdup("Nothing.\n");
    ;

    board->message_count++;
    RECREATE(board->messages, BoardMessage *, board->message_count);
    board->messages[board->message_count - 1] = msg;

    return msg;
}

static void fix_message_order(BoardData *board) {
    BoardMessage **newindex;
    int dest, src;

    CREATE(newindex, BoardMessage *, board->message_count);

    /* Copy over stickies first */
    for (dest = src = board->message_count - 1; src >= 0; --src)
        if (board->messages[src]->sticky)
            newindex[dest--] = board->messages[src];

    /* Then the regular messages */
    for (src = board->message_count - 1; src >= 0; --src)
        if (!board->messages[src]->sticky)
            newindex[dest--] = board->messages[src];

    free(board->messages);
    board->messages = newindex;
}

static void apply_message_edit(BoardMessage *msg, CharData *editor, char *subject, char *message) {
    BoardMessageEdit *edit;

    free(msg->subject);
    msg->subject = subject ? subject : strdup("");
    free(msg->message);
    msg->message = message ? message : strdup("Nothing.\n");

    CREATE(edit, BoardMessageEdit, 1);
    edit->editor = strdup(GET_NAME(editor));
    edit->time = time(0);
    edit->next = msg->edits;
    msg->edits = edit;
}

/******* FILE INTERFACE FUNCTIONS *******/

/* Used by load_board */
static void parse_privilege(BoardData *board, char *line) {
    char arg[MAX_INPUT_LENGTH];
    int num;

    line = any_one_arg(line, arg);
    num = atoi(arg);
    if (num < 0 || num >= NUM_BPRIV) {
        log("SYSERR: invalid privilege %s in parse_privilege", arg);
        return;
    }

    // if (!(board->privileges[num] = parse_rule(line)))
    //     board->privileges[num] = make_level_rule(0, LVL_IMPL);
}

/* Called by board_init */
static BoardData *load_board(const char *name) {
    FILE *fl;
    char filename[MAX_INPUT_LENGTH + 40];
    BoardData *board;
    BoardMessage *msg = nullptr;
    char line[MAX_INPUT_LENGTH + 1];
    char tag[128];
    struct temp_record {
        BoardMessage *msg;
        temp_record *next;
    } *list = nullptr, *temp;
    int i;
    BoardMessageEdit *edit;
    char editname[MAX_INPUT_LENGTH];

    if (!name) {
        log("SYSERR: NULL board name passed to load_board()");
        return nullptr;
    }

    if (!valid_alias(name)) {
        log("SYSERR: invalid character in board name passed to load_board()");
        return nullptr;
    }

    sprintf(filename, BOARD_PREFIX "/%s" BOARD_SUFFIX, name);
    if (!(fl = fopen(filename, "r"))) {
        log("SYSERR: Couldn't open board file %s", filename);
        return nullptr;
    }

    CREATE(board, BoardData, 1);
    board->alias = strdup(name);

    /* Read in board info */
    while (get_line(fl, line)) {
        tag_argument(line, tag);

        if (!strcasecmp(tag, "~~"))
            break;

        switch (toupper(*tag)) {
        case 'A':
            if (!strcasecmp(tag, "alias")) {
                if (strcasecmp(board->alias, line))
                    log("SYSERR: Board alias in board file (%s) doesn't match entry in "
                        "index (%s)",
                        line, board->alias);
            } else
                goto bad_board_tag;
            break;
        case 'N':
            if (!strcasecmp(tag, "number"))
                board->number = atoi(line);
            else
                goto bad_board_tag;
            break;
        case 'P':
            if (!strcasecmp(tag, "privilege"))
                parse_privilege(board, line);
            else
                goto bad_board_tag;
            break;
        case 'T':
            if (!strcasecmp(tag, "title"))
                board->title = strdup(line);
            else
                goto bad_board_tag;
            break;
        default:
        bad_board_tag:
            log("SYSERR: Unknown tag %s in board file %s: %s", tag, name, line);
        }
    }

    /* Now read in messages */
    while (get_line(fl, line)) {
        tag_argument(line, tag);

        if (!strcasecmp(tag, "~~")) {
            msg = nullptr;
            continue;
        }

        /* Create message and put it in a temporary linked list */
        if (!msg) {
            CREATE(msg, BoardMessage, 1);
            CREATE(temp, temp_record, 1);
            temp->next = list;
            list = temp;
            temp->msg = msg;
            board->message_count++;
        }

        switch (toupper(*tag)) {
        case 'E':
            if (!strcasecmp(tag, "edit")) {
                CREATE(edit, BoardMessageEdit, 1);
                if (sscanf(line, "%s %ld", editname, &edit->time) == 2) {
                    edit->editor = strdup(editname);
                    edit->next = msg->edits;
                    msg->edits = edit;
                } else {
                    log("SYSERR: Invalid edit message record for board %s", board->alias);
                    free(edit);
                }
            } else
                goto bad_msg_tag;
            break;
        case 'L':
            if (!strcasecmp(tag, "level"))
                msg->level = atoi(line);
            else
                goto bad_msg_tag;
            break;
        case 'M':
            if (!strcasecmp(tag, "message"))
                msg->message = fread_string(fl, "load_board");
            else
                goto bad_msg_tag;
            break;
        case 'P':
            if (!strcasecmp(tag, "poster"))
                msg->poster = strdup(line);
            else
                goto bad_msg_tag;
            break;
        case 'S':
            if (!strcasecmp(tag, "subject"))
                msg->subject = strdup(line);
            else if (!strcasecmp(tag, "sticky"))
                msg->sticky = atoi(line);
            else
                goto bad_msg_tag;
            break;
        case 'T':
            if (!strcasecmp(tag, "time"))
                msg->time = atol(line);
            else
                goto bad_msg_tag;
            break;
        default:
        bad_msg_tag:
            log("SYSERR: Unknown tag %s in board file %s: %s", tag, name, line);
        }
    }

    /* Transfer linked-list of messages to board's message array */
    CREATE(board->messages, BoardMessage *, board->message_count + 1);
    for (i = board->message_count - 1; i >= 0; --i) {
        temp = list;
        list = list->next;
        board->messages[i] = temp->msg;
        free(temp);
    }

    return board;
}

void board_init() {
    FILE *fl;
    char name[MAX_INPUT_LENGTH];
    struct temp_record {
        BoardData *board;
        temp_record *next;
    } *list = nullptr, *temp;
    BoardData *board;
    int i;

    if (num_boards || board_index) {
        log("Unsafe attempt to initialize boards; board_init() may already have "
            "been invoked");
        return;
    }

    if (!(fl = fopen(BOARD_INDEX_FILE, "r"))) {
        log("No board index.  Creating a new one.");
        mkdir(BOARD_PREFIX, 0775);
        touch(BOARD_INDEX_FILE);
        if (!(fl = fopen(BOARD_INDEX_FILE, "r"))) {
            perror("fatal error opening board index");
            exit(1);
        }
    }

    /* Get list of board aliases and load boards from file */
    while (get_line(fl, name)) {
        if (!(board = load_board(name))) {
            log("Unable to load board '%s' specified in index - skipped", name);
            continue;
        }
        CREATE(temp, temp_record, 1);
        temp->next = list;
        list = temp;
        list->board = board;
        ++num_boards;
    }

    /* Transfer boards from linked list to board index array */
    CREATE(board_index, BoardData *, num_boards + 1);
    for (i = num_boards - 1; i >= 0; --i) {
        temp = list;
        list = list->next;
        board_index[i] = temp->board;
        free(temp);
    }

    fclose(fl);

    /* Initialize the undefined board */
    memset(&null_board, 0x0, sizeof(BoardData));
    null_board.number = 0;
    null_board.alias = "undef";
    null_board.title = "Undefined Board";
}

static bool reload_board(BoardData *board) {
    int i = 0;
    BoardData *new_board;

    if (!(new_board = load_board(board->alias))) {
        log("SYSERR: Unable to reload existing board '%s' from file", board->alias);
        return false;
    }

    for (i = 0; i < num_boards; ++i)
        if (board_index[i] == board) {
            free(board_index[i]);
            board_index[i] = new_board;
            return true;
        }

    if (new_board)
        free_board(new_board);

    /* board not found */
    return false;
}

void save_board_index() {
    FILE *fl;
    int i;

    if (!(fl = fopen(BOARD_INDEX_FILE, "w"))) {
        log("SYSERR: Unable to open board index file for writing.");
        return;
    }

    for (i = 0; i < num_boards; ++i)
        fprintf(fl, "%s\n", board_index[i]->alias);

    fclose(fl);
}

static const char *print_privilege(BoardData *board, int priv) {
    static char buf[MAX_INPUT_LENGTH];
    size_t len;

    sprintf(buf, "%d ", priv);
    len = strlen(buf);
    // sprint_rule(buf + len, sizeof(buf) - len, board->privileges[priv]);

    return buf;
}

void save_board(BoardData *board) {
    FILE *fl;
    char filename[MAX_INPUT_LENGTH + 40];
    char tempfilename[MAX_INPUT_LENGTH + 40];
    BoardMessage *msg;
    BoardMessageEdit *edit;
    int i;

    if (!board) {
        log("SYSERR: NULL board passed to save_board()");
        return;
    }

    sprintf(filename, BOARD_PREFIX "/%s" BOARD_SUFFIX, board->alias);
    sprintf(tempfilename, BOARD_PREFIX "/%s.tmp", board->alias);

    if (!(fl = fopen(tempfilename, "w"))) {
        log("SYSERR: Couldn't open temp board file %s for write", tempfilename);
        return;
    }

    fprintf(fl, "number: %d\n", board->number);
    fprintf(fl, "alias: %s\n", board->alias);
    fprintf(fl, "title: %s\n", board->title);
    for (i = 0; i < NUM_BPRIV; ++i)
        fprintf(fl, "privilege: %s\n", print_privilege(board, i));
    fprintf(fl, "~~\n");

    for (i = 0; i < board->message_count; ++i) {
        msg = board->messages[i];
        fprintf(fl, "level: %d\n", msg->level);
        fprintf(fl, "poster: %s\n", msg->poster);
        fprintf(fl, "time: %ld\n", msg->time);
        if (msg->sticky)
            fprintf(fl, "sticky: 1\n");
        for (edit = msg->edits; edit; edit = edit->next)
            fprintf(fl, "edit: %s %ld\n", edit->editor, edit->time);
        fprintf(fl, "subject: %s\n", filter_chars(buf, msg->subject, "\n"));
        fprintf(fl, "message:\n%s~\n", filter_chars(buf, msg->message, "\r~"));
        fprintf(fl, "~~\n");
    }

    if (fclose(fl))
        log("SYSERR: Error closing board file for %s after write", board->alias);
    else if (rename(tempfilename, filename))
        log("SYSERR: Error renaming temporary board file for %s after write", board->alias);
}

/******* COMMAND INTERFACE *******/

void look_at_board(CharData *ch, const BoardData *board, const ObjData *face) {
    int i;
    char buf[MAX_INPUT_LENGTH];

    if (!has_board_privilege(ch, board, BPRIV_READ)) {
        char_printf(ch, "The words don't seem to make any sense to you.\n");
        return;
    }

    char_printf(ch, "There {} {} message{} on {}.\n", board->message_count == 1 ? "is" : "are", board->message_count,
                board->message_count == 1 ? "" : "s", face ? face->short_description : "the board");

    for (i = board->message_count - 1; i >= 0; --i) {
        strftime(buf, 15, TIMEFMT_DATE, localtime(&board->messages[i]->time));
        paging_printf(ch, "{}{:-2d}" ANRM " : {:<11s} : {:<12s}:: {}" ANRM "\n", board->messages[i]->sticky ? FCYN : "",
                      i + 1, buf, board->messages[i]->poster,
                      board->messages[i]->subject ? board->messages[i]->subject : "<no title>");
    }

    start_paging(ch);
}

ACMD(do_boardadmin) {
    BoardData *board;
    int i, j;
    // Rule *rule;

    argument = any_one_arg(argument, arg);
    skip_spaces(&argument);

    if (!strcasecmp(arg, "delete")) {
        if (!*argument)
            char_printf(ch, "Which board do you want to delete?\n");
        else if (!(board = find_board(argument)))
            char_printf(ch, "No such board: {}\n", argument);
        else if (!delete_board(board))
            char_printf(ch, "Unable to delete board {}.\n", board->alias);
        else {
            auto name = std::string(GET_NAME(ch));
            auto board_name = std::string(argument);
            log(LogSeverity::Stat, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "(GC) {} deleted board {}.", name, board_name);
        }
    }

    else if (!strcasecmp(arg, "create")) {
        if (!*argument)
            char_printf(ch, "What do you want the new board's alias to be?\n");
        else if (!valid_alias(argument))
            char_printf(ch, "Only letters, digits, and underscores are allowed in board aliases.\n");
        else {
            board = new_board(argument);
            char_printf(ch, "New board created.\n");
            log(LogSeverity::Stat, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "(GC) {} created board {}.", GET_NAME(ch),
                argument);
        }
    }

    else if (!strcasecmp(arg, "reload")) {
        if (!*argument)
            char_printf(ch, "Which board do you want to delete?\n");
        else if (!(board = find_board(argument)))
            char_printf(ch, "No such board: {}\n", argument);
        else if (!reload_board(board))
            char_printf(ch, "Unable to reload board {}.\n", board->alias);
        else
            log(LogSeverity::Stat, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "(GC) {} reloaded board {}.", GET_NAME(ch),
                argument);
    }

    else if (!strcasecmp(arg, "title")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            char_printf(ch, "Which board's title do you want to change?\n");
        else if (!(board = find_board(arg)))
            char_printf(ch, "No such board: {}\n", arg);
        else {
            char_printf(ch, "Board title changed from '{}' to '{}'.\n", board->title, argument);
            free(board->title);
            board->title = strdup(argument);
            save_board(board);
        }
    }

    else if (is_abbrev(arg, "list")) {
        char_printf(ch,
                    AUND "Num" ANRM " " AUND "Msg" ANRM " " AUND "Alias    " ANRM " " AUND "Title              " ANRM);
        for (j = 0; j < NUM_BPRIV; ++j)
            char_printf(ch, " " AUND "{:<4s}" ANRM, privilege_data[j].abbr);
        char_printf(ch, "\n");
        for (i = 0; i < num_boards; ++i) {
            board = board_index[i];
            char_printf(ch, fmt::format("{:3d} {:3d} {:9s} {:19s}", board->number, board->message_count, board->alias,
                                        ellipsis(board->title, 19)));
            // for (j = 0; j < NUM_BPRIV; ++j) {
            //     rule_abbr(buf, board->privileges[j]);
            //     char_printf(ch, " " FGRN "%c" ANRM "%3s", UPPER(*rule_name(board->privileges[j])), buf);
            // }
            char_printf(ch, "\n");
        }
        if (!num_boards)
            char_printf(ch, " No boards defined.\n");
    }

    else if (is_abbrev(arg, "info")) {
        if (!*argument)
            char_printf(ch, "Which board do you want info on?\n");
        else if (!(board = find_board(argument)))
            char_printf(ch, "No such board: {}\n", argument);
        else {
            char_printf(ch,
                        "Board          : " FYEL "{}" ANRM " (" FGRN "%d" ANRM
                        ")\n"
                        "Title          : " FCYN "{}" ANRM
                        "\n"
                        "Messages       : " FCYN "%d" ANRM
                        "\n"
                        "Locked         : " FCYN "{}" ANRM
                        "\n"
                        "Privileges     :\n",
                        board->alias, board->number, board->title, board->message_count, YESNO(board->locked));
            // for (i = 0; i < NUM_BPRIV; ++i) {
            //     rule_verbose(buf, sizeof(buf), board->privileges[i]);
            //     char_printf(ch, "  %c%-11s : {}\n", UPPER(*privilege_data[i].alias), privilege_data[i].alias + 1,
            //     buf);
            // }
        }
    }

    else if (is_abbrev(arg, "privilege")) {
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            char_printf(ch, "Which board's privileges do you want to change?\n");
            return;
        }
        if (!(board = find_board(arg))) {
            char_printf(ch, "No such board: {}\n", arg);
            return;
        }
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            char_printf(ch, "Which privilege do you want to change?\n");
            return;
        }
        if (!strcasecmp(arg, "all"))
            i = -1; /* Set all privileges at once */
        else {
            for (i = 0; i < NUM_BPRIV; ++i)
                if (!strcasecmp(privilege_data[i].abbr, arg) || is_abbrev(privilege_data[i].alias, arg))
                    break;
            if (i >= NUM_BPRIV) {
                char_printf(ch, "Invalid privilege.  Allowed privileges:\n");
                for (i = 0; i < NUM_BPRIV; ++i)
                    char_printf(ch, "  {} {}\n", privilege_data[i].abbr, privilege_data[i].alias);
                return;
            }
        }
        // if (!(rule = parse_rule(argument))) {
        //     char_printf(ch, "Invalid rule format.\n");
        //     return;
        // }
        // rule_verbose(buf, sizeof(buf), rule);
        // if (i >= 0) {
        //     char_printf(ch, "Set {}'s {} privilege to {}.\n", board->alias, privilege_data[i].alias, buf);
        //     board->privileges[i] = rule;
        // } else {
        //     char_printf(ch, "Set {}'s privileges to {}.\n", board->alias, buf);
        //     for (i = 0; i < NUM_BPRIV; ++i)
        //         if (i == 0)
        //             board->privileges[i] = rule;
        //         else
        //             board->privileges[i] = parse_rule(argument);
        // }
        save_board(board);
    }

    else if (is_abbrev(arg, "read")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            char_printf(ch, "Which board do you want to view?\n");
        else if (!(board = find_board(arg)))
            char_printf(ch, "No such board: {}\n", arg);
        else if (!*argument)
            look_at_board(ch, board, nullptr);
        else if (!is_number(argument))
            char_printf(ch, "Which message do you want to read?\n");
        else
            read_message(ch, board, atoi(argument));
    }

    else if (is_abbrev(arg, "edit")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            char_printf(ch, "On which board do you want to edit a message?\n");
        else if (!(board = find_board(arg)))
            char_printf(ch, "No such board: {}\n", arg);
        else if (!*argument || !is_number(argument))
            char_printf(ch, "Which message do you want to edit?\n");
        else
            edit_message(ch, board, atoi(argument));
    }

    else if (is_abbrev(arg, "remove")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            char_printf(ch, "On which board do you want to remove a message?\n");
        else if (!(board = find_board(arg)))
            char_printf(ch, "No such board: {}\n", arg);
        else if (!*argument || !is_number(argument))
            char_printf(ch, "Which message do you want to remove?\n");
        else
            remove_message(ch, board, atoi(argument), nullptr);
    }

    else if (is_abbrev(arg, "write")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            char_printf(ch, "Which board do you want to write on?\n");
        else if (!(board = find_board(arg)))
            char_printf(ch, "No such board: {}\n", arg);
        else
            write_message(ch, board, argument);
    }

    else if (is_abbrev(arg, "lock")) {
        if (!*argument)
            char_printf(ch, "Which board do you want to toggle the lock on?\n");
        else if (!(board = find_board(argument)))
            char_printf(ch, "No such board: {}\n", arg);
        else {
            char_printf(ch, "The {} board is now {}locked.\n", board->alias,
                        (board->locked = !board->locked) ? "" : "un");
            save_board(board);
        }
    }

    else
        char_printf(ch,
                    "Usage:\n\n"
                    "<<Moderation commands>>\n"
                    "   boardadmin list\n"
                    "   boardadmin read <board> [<msg#>]\n"
                    "   boardadmin edit <board> <msg#>\n"
                    "   boardadmin remove <board> <msg#>\n"
                    "   boardadmin write <board> [<title>]\n"
                    "   boardadmin lock <board>\n"
                    "<<Administration commands>>\n"
                    "   boardadmin info <board>\n"
                    "   boardadmin title <board> <title>\n"
                    "   boardadmin privilege <board> <privilege> <mode> [<values>]\n"
                    "   boardadmin delete <board>\n"
                    "   boardadmin create <alias>\n"
                    "   boardadmin reload <board>\n");
}

void read_message(CharData *ch, BoardData *board, int msgnum) {
    BoardMessage *msg;
    char timebuf[32];
    char buf[MAX_INPUT_LENGTH];
    BoardMessageEdit *edit;

    if (!has_board_privilege(ch, board, BPRIV_READ)) {
        char_printf(ch, "The words don't seem to make any sense to you.\n");
        return;
    }

    if (!(msg = board_message(board, msgnum))) {
        char_printf(ch, "That message exists only in your imagination.\n");
        return;
    }

    auto time = std::chrono::system_clock::from_time_t(msg->time);
    paging_printf(ch, FCYN "Message {:d} {}: " ANRM "{}" AFCYN "\n", msgnum, msg->sticky ? HCYN "(sticky) " AFCYN : "",
                  msg->subject);
    paging_printf(ch, "{}  posted by {}, {:%c}" ANRM "\n", msg->edits ? "" : AUND, msg->poster, time);

    for (edit = msg->edits; edit; edit = edit->next) {
        time = std::chrono::system_clock::from_time_t(edit->time);
        paging_printf(ch, FCYN "{}  edited by {}, {:%c}" ANRM "\n", edit->next ? "" : AUND, edit->editor, time);
    }
    paging_printf(ch, msg->message);

    start_paging(ch);
}

void edit_message(CharData *ch, BoardData *board, int msgnum) {
    board_editing_data *edit_data;
    BoardMessage *msg;

    if (!(msg = board_message(board, msgnum))) {
        if (has_board_privilege(ch, board, BPRIV_READ))
            char_printf(ch, "That message exists only in your imagination.\n");
        else
            char_printf(ch, "The words don't seem to make any sense to you.\n");
        return;
    }

    if (!strcasecmp(msg->poster, GET_NAME(ch))) {
        if (!has_board_privilege(ch, board, BPRIV_EDIT_OWN) && !has_board_privilege(ch, board, BPRIV_EDIT_ANY)) {
            char_printf(ch, "You can't edit your own posts on this board.\n");
            return;
        }
    } else if (!has_board_privilege(ch, board, BPRIV_EDIT_ANY)) {
        char_printf(ch, "You can't edit others' posts on this board.\n");
        return;
    }

    if (board->locked && !has_board_privilege(ch, board, BPRIV_LOCK)) {
        char_printf(ch, "The board is current locked for posting.\n");
        return;
    }

    if (msg->editing) {
        char_printf(ch, "{} is currently editing that message.\n", capitalize(PERS(msg->editing, ch)));
        return;
    }

    board->editing++;
    msg->editing = ch;

    CREATE(edit_data, board_editing_data, 1);
    edit_data->board = board;
    edit_data->message = msg;
    edit_data->subject = strdup(msg->subject);
    edit_data->sticky = msg->sticky && has_board_privilege(ch, board, BPRIV_WRITE_STICKY);

    editor_init(ch->desc, &msg->message, MAX_MSG_LEN);
    editor_set_begin_string(ch->desc, "Write your message.");
    editor_set_callback_data(ch->desc, edit_data, ED_FREE_DATA);
    editor_set_callback(ch->desc, ED_EXIT_SAVE, board_save);
    editor_set_callback(ch->desc, ED_EXIT_ABORT, board_save);
    editor_set_callback(ch->desc, ED_OTHER, board_special);
    editor_set_callback(ch->desc, ED_LIST, board_list);
    editor_set_callback(ch->desc, ED_LIST_NUMERIC, board_list);
    editor_set_callback(ch->desc, ED_HELP, board_help);
}

void remove_message(CharData *ch, BoardData *board, int msgnum, const ObjData *face) {
    BoardMessage *msg;

    if (!(msg = board_message(board, msgnum))) {
        if (has_board_privilege(ch, board, BPRIV_READ))
            char_printf(ch, "That message exists only in your imagination.\n");
        else
            char_printf(ch, "The words don't seem to make any sense to you.\n");
        return;
    }

    if (!strcasecmp(msg->poster, GET_NAME(ch))) {
        if (!has_board_privilege(ch, board, BPRIV_REMOVE_OWN) && !has_board_privilege(ch, board, BPRIV_REMOVE_ANY)) {
            char_printf(ch, "You can't remove your own posts on this board.\n");
            return;
        }
    } else if (!has_board_privilege(ch, board, BPRIV_REMOVE_ANY)) {
        char_printf(ch, "You can't remove others' posts on this board.\n");
        return;
    }

    if (board->locked && !has_board_privilege(ch, board, BPRIV_LOCK)) {
        char_printf(ch, "The board is current locked for posting.\n");
        return;
    }

    if (msg->editing) {
        char_printf(ch, "{} is currently editing that message.\n", capitalize(PERS(msg->editing, ch)));
        return;
    }

    char_printf(ch, "Removed message {:d} from {}{}.\n", msgnum, face ? face->short_description : board->alias,
                face ? "" : " board");

    delete_message(board, msg);
    save_board(board);
}

void write_message(CharData *ch, BoardData *board, const char *subject) {
    board_editing_data *edit_data;

    if (!board || board == &null_board) {
        char_printf(ch, "Error writing on board.\n");
        return;
    }

    if (!has_board_privilege(ch, board, BPRIV_WRITE_NEW)) {
        char_printf(ch, "You're not quite sure how to write here...\n");
        return;
    }

    if (board->locked && !has_board_privilege(ch, board, BPRIV_LOCK)) {
        char_printf(ch, "The board is current locked for posting.\n");
        return;
    }

    board->editing++;

    CREATE(edit_data, board_editing_data, 1);
    edit_data->board = board;
    edit_data->subject = strdup(subject);

    editor_init(ch->desc, nullptr, MAX_MSG_LEN);
    editor_set_begin_string(ch->desc, "Write your message.");
    editor_set_callback_data(ch->desc, edit_data, ED_FREE_DATA);
    editor_set_callback(ch->desc, ED_EXIT_SAVE, board_save);
    editor_set_callback(ch->desc, ED_EXIT_ABORT, board_save);
    editor_set_callback(ch->desc, ED_OTHER, board_special);
    editor_set_callback(ch->desc, ED_LIST, board_list);
    editor_set_callback(ch->desc, ED_LIST_NUMERIC, board_list);
    editor_set_callback(ch->desc, ED_HELP, board_help);
}

static EDITOR_FUNC(board_save) {
    DescriptorData *d = edit->descriptor;
    board_editing_data *edit_data = (board_editing_data *)edit->data;
    BoardMessage *msg = edit_data->message;

    if (edit->command == ED_EXIT_SAVE) {
        desc_printf(d, "Message posted.\n");

        if (msg)
            apply_message_edit(msg, d->character, edit_data->subject, edit->string);
        else
            msg = add_new_message(edit_data->board, d->character, edit_data->subject, edit->string);

        msg->sticky = edit_data->sticky;

        edit->string = nullptr;

        fix_message_order(edit_data->board);
        save_board(edit_data->board);
    } else {
        desc_printf(d, "Post aborted.  Message not saved.\n");
        free(edit_data->subject);
    }

    if (msg)
        msg->editing = nullptr;
    edit_data->board->editing--;

    return ED_PROCESSED;
}

static EDITOR_FUNC(board_special) {
    DescriptorData *d = edit->descriptor;
    board_editing_data *edit_data = (board_editing_data *)edit->data;

    if (!edit->argument)
        return ED_IGNORED;

    if (*edit->argument == 'u') {
        char subject[MAX_INPUT_LENGTH], *s;

        strcpy(subject, edit->argument + 1);
        s = subject;
        skip_spaces(&s);

        if (edit_data->subject)
            free(edit_data->subject);
        edit_data->subject = strdup(s);

        desc_printf(d, "Message subject set to: {}\n", s);
    } else if (*edit->argument == 'y') {
        if (has_board_privilege(d->character, edit_data->board, BPRIV_WRITE_STICKY))
            desc_printf(d, "Message set as {}sticky.\n", (edit_data->sticky = !edit_data->sticky) ? "" : "non-");
        else
            string_to_output(d, "You don't have the ability to make stickies on this board.\n");
    } else
        return ED_IGNORED;

    return ED_PROCESSED;
}

static EDITOR_FUNC(board_list) {
    DescriptorData *d = edit->descriptor;
    board_editing_data *edit_data = (board_editing_data *)edit->data;
    char buf[32];
    time_t tm;

    tm = edit_data->message ? edit_data->message->time : time(0);
    strftime(buf, 32, TIMEFMT_LOG, localtime(&tm));
    desc_printf(d, "{}" AUND "{} by {} :: {:<30s}\n" ANRM, edit_data->sticky ? AHCYN : AFCYN, buf,
                edit_data->message ? edit_data->message->poster : GET_NAME(d->character), edit_data->subject);

    return ED_IGNORED;
}

static EDITOR_FUNC(board_help) {
    DescriptorData *d = edit->descriptor;
    board_editing_data *edit_data = (board_editing_data *)edit->data;

    string_to_output(d,
                     "Editor command formats: /<letter>\n\n"
                     "/a           -  abort message post\n"
                     "/c           -  clear message\n"
                     "/d#          -  delete line #\n"
                     "/e# <text>   -  change the line at # with <text>\n"
                     "/f           -  format entire text\n"
                     "/fi          -  indented formatting of text\n"
                     "/h           -  list text editor commands\n"
                     "/k <word>    -  spellcheck word\n"
                     "/i# <text>   -  insert <text> at line #\n");
    string_to_output(d,
                     "/l           -  list entire message\n"
                     "/n           -  list entire message with line numbers\n"
                     "/r <a> <b>   -  replace 1st occurrence of text <a> in "
                     "buffer with text <b>\n"
                     "/ra <a> <b>  -  replace all occurrences of text <a> within "
                     "buffer with text <b>\n"
                     "                usage: /r[a] pattern replacement\n"
                     "                       /r[a] 'pattern' 'replacement'\n"
                     "                       (enclose in single quotes for "
                     "multi-word phrases)\n"
                     "/s           -  save text\n"
                     "/u <subject> -  change message subject\n");
    if (has_board_privilege(d->character, edit_data->board, BPRIV_WRITE_STICKY))
        string_to_output(d, "/y           -  toggle message as sticky\n");
    string_to_output(d,
                     "\n"
                     "Note: /d, /f, /fi, /l, and /n also accept ranges of lines. "
                     " For instance:\n"
                     "   /d 2 5   -  delete lines 2 through 5\n"
                     "   /fi3 6   -  format lines 3 through 6 with indent\n");

    return ED_PROCESSED;
}

ACMD(do_edit) {
    ObjData *obj;

    argument = any_one_arg(argument, arg);
    skip_spaces(&argument);

    if (is_number(arg) &&
        universal_find(find_vis_by_type(ch, ITEM_BOARD), FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, nullptr,
                       &obj) &&
        obj)
        edit_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(arg));
    else if (!generic_find(arg, FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, ch, nullptr, &obj) || !obj)
        char_printf(ch, "What do you want to edit?\n");
    else if (GET_OBJ_TYPE(obj) != ITEM_BOARD)
        char_printf(ch, "You can only edit board messages.\n");
    else if (!is_number(argument))
        char_printf(ch, "Which message do you want to edit?\n");
    else
        edit_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(argument));
}

/***************************************************************************
 * $Id: board.c,v 1.8 2009/07/16 19:15:54 myc Exp $
 ***************************************************************************/
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

#include "board.h"

#include "clan.h"
#include "comm.h"
#include "commands.h"
#include "conf.h"
#include "db.h"
#include "editor.h"
#include "handler.h"
#include "interpreter.h"
#include "math.h"
#include "modify.h"
#include "rules.h"
#include "screen.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"
#include "vsearch.h" /* for ellipsis */

#include <sys/stat.h> /* for mkdir */

/******* BOARD VARIABLES *******/

static EDITOR_FUNC(board_save);
static EDITOR_FUNC(board_list);
static EDITOR_FUNC(board_help);
static EDITOR_FUNC(board_special);

/******* BOARD VARIABLES *******/
static int num_boards = 0;
static struct board_data **board_index = NULL; /* array of pointers to boards */
static struct board_data null_board;           /* public undefined board */

static const struct privilege_info {
    char abbr[5];
    char *alias;
    char *message;
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
        return FALSE;
    for (; *alias; ++alias)
        if (!VALID_ALIAS_CHAR(*alias))
            return FALSE;
    return TRUE;
}

static void free_message(struct board_message *msg) {
    struct board_message_edit *edit;
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

static void free_board(struct board_data *board) {
    int i;

    if (board == &null_board) {
        log("SYSERR: attempt to free null_board");
        return;
    }

    free(board->alias);
    free(board->title);

    for (i = 0; i < NUM_BPRIV; ++i)
        free_rule(board->privileges[i]);

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
    board_index = NULL;
    num_boards = 0;
}

static struct board_data *new_board(const char *alias) {
    struct board_data *board;
    size_t priv;

    if (!valid_alias(alias))
        return NULL;

    ++num_boards;
    RECREATE(board_index, struct board_data *, num_boards);
    CREATE(board, struct board_data, 1);

    board->number = (num_boards > 1 ? board_index[num_boards - 2]->number : 0) + 1;
    board->alias = strdup(alias);
    board->title = strdup("Untitled Board");

    for (priv = 0; priv < NUM_BPRIV; ++priv)
        board->privileges[priv] = make_level_rule(0, LVL_IMPL);

    board_index[num_boards - 1] = board;

    save_board_index();
    save_board(board);

    return board;
}

static bool delete_board(struct board_data *board) {
    int i;
    bool found = FALSE;
    char filename[MAX_INPUT_LENGTH];
    char bkupname[MAX_INPUT_LENGTH];

    if (board->editing)
        return FALSE;

    for (i = 0; i < num_boards; ++i) {
        if (found)
            board_index[i - 1] = board_index[i];
        else if (board_index[i] == board) {
            board_index[i] = NULL;
            found = TRUE;
        }
    }

    if (!found)
        return FALSE;

    --num_boards;
    board_index[num_boards] = NULL;

    sprintf(filename, BOARD_PREFIX "/%s" BOARD_SUFFIX, board->alias);
    sprintf(bkupname, BOARD_PREFIX "/%s.bak", board->alias);
    if (rename(filename, bkupname))
        log("SYSERR: Error renaming board file %s to backup name", filename);

    free_board(board);

    save_board_index();

    return TRUE;
}

struct board_data *board(int num) {
    int i;

    /* Maybe replace this with a binary search if we get a lot of boards. */
    if (num > 0)
        for (i = 0; i < num_boards; ++i)
            if (board_index[i]->number == num)
                return board_index[i];

    return &null_board;
}

static struct board_data *find_board(const char *str) {
    int i;

    if (!str || !*str)
        return NULL;
    else if (is_integer(str))
        return board(atoi(str));
    else {
        for (i = 0; i < num_boards; ++i)
            if (!str_cmp(board_index[i]->alias, str))
                return board_index[i];
        return NULL;
    }
}

struct board_message *board_message(const struct board_data *board, int num) {
    if (board && num > 0 && num <= board->message_count)
        return board->messages[num - 1];
    else
        return NULL;
}

struct board_iter *board_iterator() {
    struct board_iter *iter;
    CREATE(iter, struct board_iter, 1);
    return iter;
}

const struct board_data *next_board(struct board_iter *iter) {
    if (!VALID_BOARD_INDEX(iter->index))
        return NULL;
    return board_index[iter->index++];
}

void free_board_iterator(struct board_iter *iter) { free(iter); }

int board_count() { return num_boards; }

bool has_board_privilege(struct char_data *ch, const struct board_data *board, int privnum) {
    const struct rule *priv;

    if (!VALID_PRIV_NUM(privnum)) {
        log("SYSERR: invalid privilege %d passed to has_board_privilege", privnum);
        return FALSE;
    }

    if (can_use_command(ch, find_command("boardadmin")))
        return TRUE;

    priv = board->privileges[privnum];

    return rule_matches(priv, ch);
}

static bool delete_message(struct board_data *board, struct board_message *msg) {
    int i;
    bool found = FALSE;

    for (i = 0; i < board->message_count; ++i)
        if (found)
            board->messages[i - 1] = board->messages[i];
        else if (board->messages[i] == msg)
            found = TRUE;

    if (found)
        board->message_count--;

    free_message(msg);

    return found;
}

static struct board_message *add_new_message(struct board_data *board, struct char_data *ch, char *subject,
                                             char *message) {
    struct board_message *msg;

    CREATE(msg, struct board_message, 1);
    msg->poster = strdup(GET_NAME(ch));
    msg->level = GET_LEVEL(ch);
    msg->time = time(0);
    msg->subject = subject ? subject : strdup("");
    msg->message = message ? message : strdup("Nothing.\r\n");
    ;

    board->message_count++;
    RECREATE(board->messages, struct board_message *, board->message_count);
    board->messages[board->message_count - 1] = msg;

    return msg;
}

static void fix_message_order(struct board_data *board) {
    struct board_message **newindex;
    int dest, src;

    CREATE(newindex, struct board_message *, board->message_count);

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

static void apply_message_edit(struct board_message *msg, struct char_data *editor, char *subject, char *message) {
    struct board_message_edit *edit;

    free(msg->subject);
    msg->subject = subject ? subject : strdup("");
    free(msg->message);
    msg->message = message ? message : strdup("Nothing.\r\n");

    CREATE(edit, struct board_message_edit, 1);
    edit->editor = strdup(GET_NAME(editor));
    edit->time = time(0);
    edit->next = msg->edits;
    msg->edits = edit;
}

/******* FILE INTERFACE FUNCTIONS *******/

/* Used by load_board */
static void parse_privilege(struct board_data *board, char *line) {
    char arg[MAX_INPUT_LENGTH];
    int num;

    line = any_one_arg(line, arg);
    num = atoi(arg);
    if (num < 0 || num >= NUM_BPRIV) {
        log("SYSERR: invalid privilege %s in parse_privilege", arg);
        return;
    }

    if (!(board->privileges[num] = parse_rule(line)))
        board->privileges[num] = make_level_rule(0, LVL_IMPL);
}

/* Called by board_init */
static struct board_data *load_board(const char *name) {
    FILE *fl;
    char filename[MAX_INPUT_LENGTH + 40];
    struct board_data *board;
    struct board_message *msg = NULL;
    char line[MAX_INPUT_LENGTH + 1];
    char tag[128];
    struct temp_record {
        struct board_message *msg;
        struct temp_record *next;
    } *list = NULL, *temp;
    int i;
    struct board_message_edit *edit;
    char editname[MAX_INPUT_LENGTH];

    if (!name) {
        log("SYSERR: NULL board name passed to load_board()");
        return NULL;
    }

    if (!valid_alias(name)) {
        log("SYSERR: invalid character in board name passed to load_board()");
        return NULL;
    }

    sprintf(filename, BOARD_PREFIX "/%s" BOARD_SUFFIX, name);
    if (!(fl = fopen(filename, "r"))) {
        log("SYSERR: Couldn't open board file %s", filename);
        return NULL;
    }

    CREATE(board, struct board_data, 1);
    board->alias = strdup(name);

    /* Read in board info */
    while (get_line(fl, line)) {
        tag_argument(line, tag);

        if (!strcmp(tag, "~~"))
            break;

        switch (toupper(*tag)) {
        case 'A':
            if (!strcmp(tag, "alias")) {
                if (strcmp(board->alias, line))
                    log("SYSERR: Board alias in board file (%s) doesn't match entry in "
                        "index (%s)",
                        line, board->alias);
            } else
                goto bad_board_tag;
            break;
        case 'N':
            if (!strcmp(tag, "number"))
                board->number = atoi(line);
            else
                goto bad_board_tag;
            break;
        case 'P':
            if (!strcmp(tag, "privilege"))
                parse_privilege(board, line);
            else
                goto bad_board_tag;
            break;
        case 'T':
            if (!strcmp(tag, "title"))
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

        if (!strcmp(tag, "~~")) {
            msg = NULL;
            continue;
        }

        /* Create message and put it in a temporary linked list */
        if (!msg) {
            CREATE(msg, struct board_message, 1);
            CREATE(temp, struct temp_record, 1);
            temp->next = list;
            list = temp;
            temp->msg = msg;
            board->message_count++;
        }

        switch (toupper(*tag)) {
        case 'E':
            if (!strcmp(tag, "edit")) {
                CREATE(edit, struct board_message_edit, 1);
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
            if (!strcmp(tag, "level"))
                msg->level = atoi(line);
            else
                goto bad_msg_tag;
            break;
        case 'M':
            if (!strcmp(tag, "message"))
                msg->message = fread_string(fl, "load_board");
            else
                goto bad_msg_tag;
            break;
        case 'P':
            if (!strcmp(tag, "poster"))
                msg->poster = strdup(line);
            else
                goto bad_msg_tag;
            break;
        case 'S':
            if (!strcmp(tag, "subject"))
                msg->subject = strdup(line);
            else if (!strcmp(tag, "sticky"))
                msg->sticky = atoi(line);
            else
                goto bad_msg_tag;
            break;
        case 'T':
            if (!strcmp(tag, "time"))
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
    CREATE(board->messages, struct board_message *, board->message_count + 1);
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
        struct board_data *board;
        struct temp_record *next;
    } *list = NULL, *temp;
    struct board_data *board;
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
        CREATE(temp, struct temp_record, 1);
        temp->next = list;
        list = temp;
        list->board = board;
        ++num_boards;
    }

    /* Transfer boards from linked list to board index array */
    CREATE(board_index, struct board_data *, num_boards + 1);
    for (i = num_boards - 1; i >= 0; --i) {
        temp = list;
        list = list->next;
        board_index[i] = temp->board;
        free(temp);
    }

    fclose(fl);

    /* Initialize the undefined board */
    memset(&null_board, 0x0, sizeof(struct board_data));
    null_board.number = 0;
    null_board.alias = "undef";
    null_board.title = "Undefined Board";
}

static bool reload_board(struct board_data *board) {
    int i = 0;
    struct board_data *new_board;

    if (!(new_board = load_board(board->alias))) {
        log("SYSERR: Unable to reload existing board '%s' from file", board->alias);
        return FALSE;
    }

    for (i = 0; i < num_boards; ++i)
        if (board_index[i] == board) {
            free(board_index[i]);
            board_index[i] = new_board;
            return TRUE;
        }

    if (new_board)
        free_board(new_board);

    /* board not found */
    return FALSE;
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

static const char *print_privilege(struct board_data *board, int priv) {
    static char buf[MAX_INPUT_LENGTH];
    size_t len;

    sprintf(buf, "%d ", priv);
    len = strlen(buf);
    sprint_rule(buf + len, sizeof(buf) - len, board->privileges[priv]);

    return buf;
}

void save_board(struct board_data *board) {
    FILE *fl;
    char filename[MAX_INPUT_LENGTH + 40];
    char tempfilename[MAX_INPUT_LENGTH + 40];
    struct board_message *msg;
    struct board_message_edit *edit;
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
        fprintf(fl, "subject: %s\n", filter_chars(buf, msg->subject, "\r\n"));
        fprintf(fl, "message:\n%s~\n", filter_chars(buf, msg->message, "\r~"));
        fprintf(fl, "~~\n");
    }

    if (fclose(fl))
        log("SYSERR: Error closing board file for %s after write", board->alias);
    else if (rename(tempfilename, filename))
        log("SYSERR: Error renaming temporary board file for %s after write", board->alias);
}

/******* COMMAND INTERFACE *******/

void look_at_board(struct char_data *ch, const struct board_data *board, const struct obj_data *face) {
    int i;
    char buf[MAX_INPUT_LENGTH];

    if (!has_board_privilege(ch, board, BPRIV_READ)) {
        send_to_char("The words don't seem to make any sense to you.\r\n", ch);
        return;
    }

    cprintf(ch, "There %s %d message%s on %s.\r\n", board->message_count == 1 ? "is" : "are", board->message_count,
            board->message_count == 1 ? "" : "s", face ? face->short_description : "the board");

    for (i = board->message_count - 1; i >= 0; --i) {
        strftime(buf, 15, TIMEFMT_DATE, localtime(&board->messages[i]->time));
        pprintf(ch, "%s%-2d" ANRM " : %-11s : %-12s:: %s" ANRM "\r\n", board->messages[i]->sticky ? FCYN : "", i + 1,
                buf, board->messages[i]->poster,
                board->messages[i]->subject ? board->messages[i]->subject : "<no title>");
    }

    start_paging(ch);
}

ACMD(do_boardadmin) {
    struct board_data *board;
    int i, j;
    struct rule *rule;

    argument = any_one_arg(argument, arg);
    skip_spaces(&argument);

    if (!strcmp(arg, "delete")) {
        if (!*argument)
            send_to_char("Which board do you want to delete?\r\n", ch);
        else if (!(board = find_board(argument)))
            cprintf(ch, "No such board: %s\r\n", argument);
        else if (!delete_board(board))
            cprintf(ch, "Unable to delete board %s.\r\n", board->alias);
        else
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "(GC) %s deleted board %s.", GET_NAME(ch), argument);
    }

    else if (!strcmp(arg, "create")) {
        if (!*argument)
            send_to_char("What do you want the new board's alias to be?\r\n", ch);
        else if (!valid_alias(argument))
            send_to_char("Only letters, digits, and underscores are allowed in board aliases.\r\n",
                         ch);
        else {
            board = new_board(argument);
            send_to_char("New board created.\r\n", ch);
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "(GC) %s created board %s.", GET_NAME(ch), argument);
        }
    }

    else if (!strcmp(arg, "reload")) {
        if (!*argument)
            send_to_char("Which board do you want to delete?\r\n", ch);
        else if (!(board = find_board(argument)))
            cprintf(ch, "No such board: %s\r\n", argument);
        else if (!reload_board(board))
            cprintf(ch, "Unable to reload board %s.\r\n", board->alias);
        else
            mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), "(GC) %s reloaded board %s.", GET_NAME(ch), argument);
    }

    else if (!strcmp(arg, "title")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            send_to_char("Which board's title do you want to change?\r\n", ch);
        else if (!(board = find_board(arg)))
            cprintf(ch, "No such board: %s\r\n", arg);
        else {
            cprintf(ch, "Board title changed from '%s' to '%s'.\r\n", board->title, argument);
            free(board->title);
            board->title = strdup(argument);
            save_board(board);
        }
    }

    else if (is_abbrev(arg, "list")) {
        send_to_char(AUND "Num" ANRM " " AUND "Msg" ANRM " " AUND "Alias    " ANRM " " AUND "Title              " ANRM,
                     ch);
        for (j = 0; j < NUM_BPRIV; ++j)
            cprintf(ch, " " AUND "%-4.4s" ANRM, privilege_data[j].abbr);
        send_to_char("\r\n", ch);
        for (i = 0; i < num_boards; ++i) {
            board = board_index[i];
            cprintf(ch, "%-3d %3d %s%-9s" ANRM " " ELLIPSIS_FMT, board->number, board->message_count,
                    board->locked ? FRED : "", board->alias, ELLIPSIS_STR(board->title, 19));
            for (j = 0; j < NUM_BPRIV; ++j) {
                rule_abbr(buf, board->privileges[j]);
                cprintf(ch, " " FGRN "%c" ANRM "%3s", UPPER(*rule_name(board->privileges[j])), buf);
            }
            send_to_char("\r\n", ch);
        }
        if (!num_boards)
            send_to_char(" No boards defined.\r\n", ch);
    }

    else if (is_abbrev(arg, "info")) {
        if (!*argument)
            send_to_char("Which board do you want info on?\r\n", ch);
        else if (!(board = find_board(argument)))
            cprintf(ch, "No such board: %s\r\n", argument);
        else {
            cprintf(ch,
                    "Board          : " FYEL "%s" ANRM " (" FGRN "%d" ANRM ")\r\n"
                    "Title          : " FCYN "%s" ANRM "\r\n"
                    "Messages       : " FCYN "%d" ANRM "\r\n"
                    "Locked         : " FCYN "%s" ANRM "\r\n"
                    "Privileges     :\r\n",
                    board->alias, board->number, board->title, board->message_count, YESNO(board->locked));
            for (i = 0; i < NUM_BPRIV; ++i) {
                rule_verbose(buf, sizeof(buf), board->privileges[i]);
                cprintf(ch, "  %c%-11s : %s\r\n", UPPER(*privilege_data[i].alias), privilege_data[i].alias + 1, buf);
            }
        }
    }

    else if (is_abbrev(arg, "privilege")) {
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            send_to_char("Which board's privileges do you want to change?\r\n", ch);
            return;
        }
        if (!(board = find_board(arg))) {
            cprintf(ch, "No such board: %s\r\n", arg);
            return;
        }
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            send_to_char("Which privilege do you want to change?\r\n", ch);
            return;
        }
        if (!str_cmp(arg, "all"))
            i = -1; /* Set all privileges at once */
        else {
            for (i = 0; i < NUM_BPRIV; ++i)
                if (!str_cmp(privilege_data[i].abbr, arg) || is_abbrev(privilege_data[i].alias, arg))
                    break;
            if (i >= NUM_BPRIV) {
                send_to_char("Invalid privilege.  Allowed privileges:\r\n", ch);
                for (i = 0; i < NUM_BPRIV; ++i)
                    cprintf(ch, "  %s %s\r\n", privilege_data[i].abbr, privilege_data[i].alias);
                return;
            }
        }
        if (!(rule = parse_rule(argument))) {
            send_to_char("Invalid rule format.\r\n", ch);
            return;
        }
        rule_verbose(buf, sizeof(buf), rule);
        if (i >= 0) {
            cprintf(ch, "Set %s's %s privilege to %s.\r\n", board->alias, privilege_data[i].alias, buf);
            board->privileges[i] = rule;
        } else {
            cprintf(ch, "Set %s's privileges to %s.\r\n", board->alias, buf);
            for (i = 0; i < NUM_BPRIV; ++i)
                if (i == 0)
                    board->privileges[i] = rule;
                else
                    board->privileges[i] = parse_rule(argument);
        }
        save_board(board);
    }

    else if (is_abbrev(arg, "read")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            send_to_char("Which board do you want to view?\r\n", ch);
        else if (!(board = find_board(arg)))
            cprintf(ch, "No such board: %s\r\n", arg);
        else if (!*argument)
            look_at_board(ch, board, NULL);
        else if (!is_number(argument))
            send_to_char("Which message do you want to read?\r\n", ch);
        else
            read_message(ch, board, atoi(argument));
    }

    else if (is_abbrev(arg, "edit")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            send_to_char("On which board do you want to edit a message?\r\n", ch);
        else if (!(board = find_board(arg)))
            cprintf(ch, "No such board: %s\r\n", arg);
        else if (!*argument || !is_number(argument))
            send_to_char("Which message do you want to edit?\r\n", ch);
        else
            edit_message(ch, board, atoi(argument));
    }

    else if (is_abbrev(arg, "remove")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            send_to_char("On which board do you want to remove a message?\r\n", ch);
        else if (!(board = find_board(arg)))
            cprintf(ch, "No such board: %s\r\n", arg);
        else if (!*argument || !is_number(argument))
            send_to_char("Which message do you want to remove?\r\n", ch);
        else
            remove_message(ch, board, atoi(argument), NULL);
    }

    else if (is_abbrev(arg, "write")) {
        argument = any_one_arg(argument, arg);
        skip_spaces(&argument);
        if (!*arg)
            send_to_char("Which board do you want to write on?\r\n", ch);
        else if (!(board = find_board(arg)))
            cprintf(ch, "No such board: %s\r\n", arg);
        else
            write_message(ch, board, argument);
    }

    else if (is_abbrev(arg, "lock")) {
        if (!*argument)
            send_to_char("Which board do you want to toggle the lock on?\r\n", ch);
        else if (!(board = find_board(argument)))
            cprintf(ch, "No such board: %s\r\n", arg);
        else {
            cprintf(ch, "The %s board is now %slocked.\r\n", board->alias,
                    (board->locked = !board->locked) ? "" : "un");
            save_board(board);
        }
    }

    else
        send_to_char("Usage:\r\n\r\n"
                     "<<Moderation commands>>\r\n"
                     "   boardadmin list\r\n"
                     "   boardadmin read <board> [<msg#>]\r\n"
                     "   boardadmin edit <board> <msg#>\r\n"
                     "   boardadmin remove <board> <msg#>\r\n"
                     "   boardadmin write <board> [<title>]\r\n"
                     "   boardadmin lock <board>\r\n"
                     "<<Administration commands>>\r\n"
                     "   boardadmin info <board>\r\n"
                     "   boardadmin title <board> <title>\r\n"
                     "   boardadmin privilege <board> <privilege> <mode> [<values>]\r\n"
                     "   boardadmin delete <board>\r\n"
                     "   boardadmin create <alias>\r\n"
                     "   boardadmin reload <board>\r\n",
                     ch);
}

void read_message(struct char_data *ch, struct board_data *board, int msgnum) {
    struct board_message *msg;
    char timebuf[32];
    char buf[MAX_INPUT_LENGTH];
    struct board_message_edit *edit;

    if (!has_board_privilege(ch, board, BPRIV_READ)) {
        send_to_char("The words don't seem to make any sense to you.\r\n", ch);
        return;
    }

    if (!(msg = board_message(board, msgnum))) {
        send_to_char("That message exists only in your imagination.\r\n", ch);
        return;
    }

    strftime(timebuf, 32, TIMEFMT_LOG, localtime(&msg->time));

    sprintf(buf, "  posted by %s, %s", msg->poster, timebuf);

    pprintf(ch, FCYN "Message %d %s: " ANRM "%s" AFCYN "\r\n%s%-70s" ANRM "\r\n", msgnum,
            msg->sticky ? HCYN "(sticky) " AFCYN : "", msg->subject, msg->edits ? "" : AUND, buf);

    for (edit = msg->edits; edit; edit = edit->next) {
        strftime(timebuf, 32, TIMEFMT_LOG, localtime(&edit->time));
        sprintf(buf, "  edited by %s, %s", edit->editor, timebuf);
        pprintf(ch, FCYN "%s%-70s" ANRM "\r\n", edit->next ? "" : AUND, buf);
    }

    pprintf(ch, "%s", msg->message);

    start_paging(ch);
}

void edit_message(struct char_data *ch, struct board_data *board, int msgnum) {
    struct board_editing_data *edit_data;
    struct board_message *msg;

    if (!(msg = board_message(board, msgnum))) {
        if (has_board_privilege(ch, board, BPRIV_READ))
            send_to_char("That message exists only in your imagination.\r\n", ch);
        else
            send_to_char("The words don't seem to make any sense to you.\r\n", ch);
        return;
    }

    if (!strcmp(msg->poster, GET_NAME(ch))) {
        if (!has_board_privilege(ch, board, BPRIV_EDIT_OWN) && !has_board_privilege(ch, board, BPRIV_EDIT_ANY)) {
            send_to_char("You can't edit your own posts on this board.\r\n", ch);
            return;
        }
    } else if (!has_board_privilege(ch, board, BPRIV_EDIT_ANY)) {
        send_to_char("You can't edit others' posts on this board.\r\n", ch);
        return;
    }

    if (board->locked && !has_board_privilege(ch, board, BPRIV_LOCK)) {
        send_to_char("The board is current locked for posting.\r\n", ch);
        return;
    }

    if (msg->editing) {
        cprintf(ch, "%c%s is currently editing that message.\r\n", UPPER(*PERS(msg->editing, ch)),
                PERS(msg->editing, ch) + 1);
        return;
    }

    board->editing++;
    msg->editing = ch;

    CREATE(edit_data, struct board_editing_data, 1);
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

void remove_message(struct char_data *ch, struct board_data *board, int msgnum, const struct obj_data *face) {
    struct board_message *msg;

    if (!(msg = board_message(board, msgnum))) {
        if (has_board_privilege(ch, board, BPRIV_READ))
            send_to_char("That message exists only in your imagination.\r\n", ch);
        else
            send_to_char("The words don't seem to make any sense to you.\r\n", ch);
        return;
    }

    if (!strcmp(msg->poster, GET_NAME(ch))) {
        if (!has_board_privilege(ch, board, BPRIV_REMOVE_OWN) && !has_board_privilege(ch, board, BPRIV_REMOVE_ANY)) {
            send_to_char("You can't remove your own posts on this board.\r\n", ch);
            return;
        }
    } else if (!has_board_privilege(ch, board, BPRIV_REMOVE_ANY)) {
        send_to_char("You can't remove others' posts on this board.\r\n", ch);
        return;
    }

    if (board->locked && !has_board_privilege(ch, board, BPRIV_LOCK)) {
        send_to_char("The board is current locked for posting.\r\n", ch);
        return;
    }

    if (msg->editing) {
        cprintf(ch, "%c%s is currently editing that message.\r\n", UPPER(*PERS(msg->editing, ch)),
                PERS(msg->editing, ch) + 1);
        return;
    }

    cprintf(ch, "Removed message %d from %s%s.\r\n", msgnum, face ? face->short_description : board->alias,
            face ? "" : " board");

    delete_message(board, msg);
    save_board(board);
}

void write_message(struct char_data *ch, struct board_data *board, const char *subject) {
    struct board_editing_data *edit_data;

    if (!board || board == &null_board) {
        send_to_char("Error writing on board.\r\n", ch);
        return;
    }

    if (!has_board_privilege(ch, board, BPRIV_WRITE_NEW)) {
        send_to_char("You're not quite sure how to write here...\r\n", ch);
        return;
    }

    if (board->locked && !has_board_privilege(ch, board, BPRIV_LOCK)) {
        send_to_char("The board is current locked for posting.\r\n", ch);
        return;
    }

    board->editing++;

    CREATE(edit_data, struct board_editing_data, 1);
    edit_data->board = board;
    edit_data->subject = strdup(subject);

    editor_init(ch->desc, NULL, MAX_MSG_LEN);
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
    struct descriptor_data *d = edit->descriptor;
    struct board_editing_data *edit_data = (struct board_editing_data *)edit->data;
    struct board_message *msg = edit_data->message;

    if (edit->command == ED_EXIT_SAVE) {
        dprintf(d, "Message posted.\r\n");

        if (msg)
            apply_message_edit(msg, d->character, edit_data->subject, edit->string);
        else
            msg = add_new_message(edit_data->board, d->character, edit_data->subject, edit->string);

        msg->sticky = edit_data->sticky;

        edit->string = NULL;

        fix_message_order(edit_data->board);
        save_board(edit_data->board);
    } else {
        dprintf(d, "Post aborted.  Message not saved.\r\n");
        free(edit_data->subject);
    }

    if (msg)
        msg->editing = NULL;
    edit_data->board->editing--;

    return ED_PROCESSED;
}

static EDITOR_FUNC(board_special) {
    struct descriptor_data *d = edit->descriptor;
    struct board_editing_data *edit_data = (struct board_editing_data *)edit->data;

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

        dprintf(d, "Message subject set to: %s\r\n", s);
    } else if (*edit->argument == 'y') {
        if (has_board_privilege(d->character, edit_data->board, BPRIV_WRITE_STICKY))
            dprintf(d, "Message set as %ssticky.\r\n", (edit_data->sticky = !edit_data->sticky) ? "" : "non-");
        else
            string_to_output(d, "You don't have the ability to make stickies on this board.\r\n");
    } else
        return ED_IGNORED;

    return ED_PROCESSED;
}

static EDITOR_FUNC(board_list) {
    struct descriptor_data *d = edit->descriptor;
    struct board_editing_data *edit_data = (struct board_editing_data *)edit->data;
    char buf[32];
    time_t tm;

    tm = edit_data->message ? edit_data->message->time : time(0);
    strftime(buf, 32, TIMEFMT_LOG, localtime(&tm));
    dprintf(d, "%s" AUND "%s by %s :: %-30s\r\n" ANRM, edit_data->sticky ? AHCYN : AFCYN, buf,
            edit_data->message ? edit_data->message->poster : GET_NAME(d->character), edit_data->subject);

    return ED_IGNORED;
}

static EDITOR_FUNC(board_help) {
    struct descriptor_data *d = edit->descriptor;
    struct board_editing_data *edit_data = (struct board_editing_data *)edit->data;

    string_to_output(d, "Editor command formats: /<letter>\r\n\r\n"
                        "/a           -  abort message post\r\n"
                        "/c           -  clear message\r\n"
                        "/d#          -  delete line #\r\n"
                        "/e# <text>   -  change the line at # with <text>\r\n"
                        "/f           -  format entire text\r\n"
                        "/fi          -  indented formatting of text\r\n"
                        "/h           -  list text editor commands\r\n"
                        "/k <word>    -  spellcheck word\r\n"
                        "/i# <text>   -  insert <text> at line #\r\n");
    string_to_output(d, "/l           -  list entire message\r\n"
                        "/n           -  list entire message with line numbers\r\n"
                        "/r <a> <b>   -  replace 1st occurrence of text <a> in "
                        "buffer with text <b>\r\n"
                        "/ra <a> <b>  -  replace all occurrences of text <a> within "
                        "buffer with text <b>\r\n"
                        "                usage: /r[a] pattern replacement\r\n"
                        "                       /r[a] 'pattern' 'replacement'\r\n"
                        "                       (enclose in single quotes for "
                        "multi-word phrases)\r\n"
                        "/s           -  save text\r\n"
                        "/u <subject> -  change message subject\r\n");
    if (has_board_privilege(d->character, edit_data->board, BPRIV_WRITE_STICKY))
        string_to_output(d, "/y           -  toggle message as sticky\r\n");
    string_to_output(d, "\r\n"
                        "Note: /d, /f, /fi, /l, and /n also accept ranges of lines. "
                        " For instance:\r\n"
                        "   /d 2 5   -  delete lines 2 through 5\r\n"
                        "   /fi3 6   -  format lines 3 through 6 with indent\r\n");

    return ED_PROCESSED;
}

ACMD(do_edit) {
    struct obj_data *obj;

    argument = any_one_arg(argument, arg);
    skip_spaces(&argument);

    if (is_number(arg) &&
        universal_find(find_vis_by_type(ch, ITEM_BOARD), FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, NULL, &obj) &&
        obj)
        edit_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(arg));
    else if (!generic_find(arg, FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD, ch, NULL, &obj) || !obj)
        send_to_char("What do you want to edit?\r\n", ch);
    else if (GET_OBJ_TYPE(obj) != ITEM_BOARD)
        send_to_char("You can only edit board messages.\r\n", ch);
    else if (!is_number(argument))
        send_to_char("Which message do you want to edit?\r\n", ch);
    else
        edit_message(ch, board(GET_OBJ_VAL(obj, VAL_BOARD_NUMBER)), atoi(argument));
}

/***************************************************************************
 * $Log: board.c,v $
 * Revision 1.8  2009/07/16 19:15:54  myc
 * Moved command stuff from grant.c to commands.c
 *
 * Revision 1.7  2009/06/09 05:33:46  myc
 * Modified the editor to handle freeing of callback data.
 *
 * Revision 1.6  2009/05/01 05:29:40  myc
 * Updated boards to use the new rule system for the privileges.
 *
 * Revision 1.5  2009/03/23 09:40:34  myc
 * Added 'boardadmin reload' command to reload a board from file.
 *
 * Revision 1.4  2009/03/20 23:02:59  myc
 * Align clan requirements properly
 *
 * Revision 1.3  2009/03/20 20:19:51  myc
 * Fix some errors and log messages.
 *
 * Revision 1.2  2009/03/09 02:22:32  myc
 * Fixed bug in saving and loading board privileges to file.  Some
 * minor cosmetic adjustments.  Added edit command.
 *
 * Revision 1.1  2009/02/21 03:30:16  myc
 * Initial revision
 *
 ***************************************************************************/

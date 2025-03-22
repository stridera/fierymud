/***************************************************************************
 *   File: interpreter.h                                  Part of FieryMUD *
 *  Usage: header file: public procs, macro defs, subcommand defines       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#include <span>

#define ACMD(name)                                                                                                     \
    void(name)(CharData * ch, [[maybe_unused]] char *argument, [[maybe_unused]] int cmd, [[maybe_unused]] int subcmd)

#define CMD_NAME (cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcasecmp(cmd_name, cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (cmdnum >= 1 && cmdnum <= 6)

/*
 * searches an array of strings for a target string.  "exact" can be
 * true or false, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.
 *
 * If the passed in haystack is an old c-array, it must be terminated with
 *  a '\n' so it knows to stop searching.
 *
 * If the passed in haystack is a std::array, it will search until the
 * end of the array.
 *
 * search_block follows a similar naming convention to strcasecmp:
 * search_block is case-sensitive, search_block is case-insensitive.
 * Often, which one you use only depends on the case of items in your
 * list, because any_one_arg and one_argument always return lower case
 * arguments.
 */

// Primary implementation for std::array
template <std::size_t N>
int search_block(const std::string_view needle, const std::array<std::string_view, N> &haystack, bool exact) {
    for (std::size_t i = 0; i < haystack.size(); ++i)
        if (exact ? matches(needle, haystack[i]) : matches_start(needle, haystack[i]))
            return static_cast<int>(i);
    return -1;
}

// Implementation for std::span (for C-style arrays with known size)
inline int search_block(const std::string_view needle, std::span<const std::string_view> haystack, bool exact) {
    for (std::size_t i = 0; i < haystack.size(); ++i)
        if (exact ? matches(needle, haystack[i]) : matches_start(needle, haystack[i]))
            return static_cast<int>(i);
    return -1;
}

// Implementation for null-terminated arrays
inline int search_block(const std::string_view needle, const std::string_view haystack[], bool exact) {
    for (std::size_t i = 0; haystack[i].front() != '\n'; ++i)
        if (exact ? matches(needle, haystack[i]) : matches_start(needle, haystack[i]))
            return static_cast<int>(i);
    return -1;
}

// Implementation for C-style null-terminated string arrays
inline int search_block(const std::string_view needle, const char *const haystack[], bool exact) {
    for (std::size_t i = 0; *haystack[i] != '\n'; ++i)
        if (exact ? matches(needle, haystack[i]) : matches_start(needle, haystack[i]))
            return static_cast<int>(i);
    return -1;
}

// Forwarding overloads for different needle types
template <std::size_t N>
inline int search_block(const char *needle, const std::array<std::string_view, N> &haystack, bool exact) {
    return search_block(std::string_view{needle}, haystack, exact);
}

template <std::size_t N>
inline int search_block(char *needle, const std::array<std::string_view, N> &haystack, bool exact) {
    return search_block(std::string_view{needle}, haystack, exact);
}

inline int search_block(const char *needle, const char *const haystack[], bool exact) {
    return search_block(std::string_view{needle}, haystack, exact);
}

inline int search_block(char *needle, const char *const haystack[], bool exact) {
    return search_block(std::string_view{needle}, haystack, exact);
}

inline int search_block(char *needle, std::string_view haystack[], bool exact) {
    return search_block(std::string_view{needle}, haystack, exact);
}

void command_interpreter(CharData *ch, char *argument);
void list_similar_commands(CharData *ch, char *arg);
char lower(char c);
char *one_argument(char *argument, char *first_arg);
char *one_word(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
char *delimited_arg(char *argument, char *quoted_arg, char delimiter);
char *delimited_arg_case(char *argument, char *quoted_arg, char delimiter);
char *delimited_arg_all(char *argument, char *quoted_arg, char delimiter);
int fill_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
void nanny(DescriptorData *d, char *arg);
int is_abbrev(const char *arg1, const char *arg2);
bool is_integer(const char *str);
bool is_positive_integer(const char *str);
bool is_negative_integer(const char *str);
bool is_number(const char *str);
int find_command(const char *command);
int parse_command(char *command);
void skip_slash(char **string);
void skip_spaces(char **string);
char *delete_doubledollar(char *string);

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)

#define CMD_NONE 0
#define CMD_MEDITATE (1 << 0)
#define CMD_MAJOR_PARA (1 << 1)
#define CMD_MINOR_PARA (1 << 2)
#define CMD_HIDE (1 << 3)
#define CMD_BOUND (1 << 4)
#define CMD_CAST (1 << 5)
#define CMD_OLC (1 << 6)
#define CMD_NOFIGHT (1 << 7)
#define CMD_ANY ((1 << 8) - 1 - CMD_NOFIGHT)

struct CommandInfo {
    const char *command;
    byte minimum_position;
    byte minimum_stance;
    void (*command_pointer)(CharData *ch, char *argument, int cmd, int subcmd);
    sh_int minimum_level;
    int subcmd;
    long flags;
};

struct SortStruct {
    int sort_pos;
    byte is_social;
};

/* necessary for CMD_IS macro */
extern const CommandInfo cmd_info[];

extern int num_of_cmds;
extern SortStruct *cmd_sort_info;

/* this is the new xnames structure --Gurlaek 6/9/1999 */
struct XName {
    /* the +3 is for the # \n and NULL chars */
    char name[MAX_NAME_LENGTH + 3];
    XName *next;
};
#define NAME_TIMEOUT 1 * (30 RL_SEC) /* 5 minutes */

void free_alias(AliasData *alias);
void free_aliases(AliasData *alias_list);

#define ALIAS_SIMPLE 0
#define ALIAS_COMPLEX 1
#define ALIAS_NONE -1

#define ALIAS_SEP_CHAR ';'
#define ALIAS_VAR_CHAR '$'
#define ALIAS_GLOB_CHAR '*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_STAY 0
#define SCMD_NORTH 1
#define SCMD_EAST 2
#define SCMD_SOUTH 3
#define SCMD_WEST 4
#define SCMD_UP 5
#define SCMD_DOWN 6

/* name approval */
#define SCMD_ACCEPT 0
#define SCMD_DECLINE 1
#define SCMD_LIST 2

/* Toggles */

/* These numbers are synchronized with a data structure that do_toggle()
 * uses.  So don't change them! */
#define SCMD_NOSUMMON 0
#define SCMD_NOHASSLE 1
#define SCMD_BRIEF 2
#define SCMD_COMPACT 3
#define SCMD_NOTELL 4
#define SCMD_AFK 5
#define SCMD_DEAF 6
#define SCMD_NOGOSSIP 7
#define SCMD_NOGRATZ 8
#define SCMD_NOWIZ 9
#define SCMD_QUEST 10
#define SCMD_ROOMFLAGS 11
#define SCMD_NOREPEAT 12
#define SCMD_HOLYLIGHT 13
#define SCMD_AUTOEXIT 14
#define SCMD_NOPETI 15
#define SCMD_NONAME 16
#define SCMD_ANON 17
#define SCMD_SHOWVNUMS 18
#define SCMD_WIMPY 19
#define SCMD_NICEAREA 20
#define SCMD_VICIOUS 21
#define SCMD_PASSIVE 22
#define SCMD_PAGELENGTH 23
#define SCMD_NO_FOLLOW 24
#define SCMD_ROOMVIS 25
#define SCMD_NOCLANCOMM 26
#define SCMD_OLCCOMM 27
#define SCMD_LINENUMS 28
#define SCMD_AUTOLOOT 29
#define SCMD_AUTOTREAS 30
#define SCMD_AUTOINVIS 31
#define SCMD_EXPANDOBJS 32
#define SCMD_EXPANDMOBS 33
#define SCMD_SACRIFICIAL 34
#define SCMD_PETASSIST 35

/* do_wizutil */
#define SCMD_REROLL 0
#define SCMD_PARDON 1
#define SCMD_NOTITLE 2
#define SCMD_SQUELCH 3
#define SCMD_FREEZE 4
#define SCMD_THAW 5
#define SCMD_UNAFFECT 6
#define SCMD_BLESS 7

/* do_spec_com */
#define SCMD_WHISPER 0
#define SCMD_ASK 1

/* do_gen_com */
#define SCMD_HOLLER 0
#define SCMD_SHOUT 1
#define SCMD_GOSSIP 2
#define SCMD_AUCTION 3
#define SCMD_GRATZ 4

/* do_gen_ps */
#define SCMD_CLEAR 0
#define SCMD_VERSION 1
#define SCMD_WHOAMI 2

/* do_shutdown */
#define SCMD_SHUTDOW 0
#define SCMD_SHUTDOWN 1

/* do_quit */
#define SCMD_QUI 0
#define SCMD_QUIT 1

/* do_date */
#define SCMD_DATE 0
#define SCMD_UPTIME 1

/* do_commands */
#define SCMD_COMMANDS 0
#define SCMD_SOCIALS 1
#define SCMD_WIZHELP 2

/* do_drop */
#define SCMD_DROP 0 /* "drop" command was used */
#define SCMD_JUNK 1 /* "junk" command was used */
#define SCMD_LETDROP                                                                                                   \
    2 /* Item is falling to the ground because someone tried                                                           \
         to give it to an insubstantial person */

/* do_gen_write */
#define SCMD_BUG 0
#define SCMD_TYPO 1
#define SCMD_IDEA 2
#define SCMD_NOTE 3

/* do_qcomm */
#define SCMD_QSAY 0
#define SCMD_QECHO 1

/* do_pour */
#define SCMD_POUR 0
#define SCMD_FILL 1

/* do_poof */
#define SCMD_POOFIN 0
#define SCMD_POOFOUT 1

/* do_hit */
#define SCMD_HIT 0
#define SCMD_MURDER 1

/* do_eat */
#define SCMD_EAT 0
#define SCMD_TASTE 1
#define SCMD_DRINK 2
#define SCMD_SIP 3

/* do_use */
#define SCMD_USE 0
#define SCMD_QUAFF 1
#define SCMD_RECITE 2
#define SCMD_PLAY 3

/* do_echo */
#define SCMD_ECHO 0
#define SCMD_EMOTE 1
#define SCMD_EMOTES 2

/* do_gen_door */
#define SCMD_OPEN 0
#define SCMD_CLOSE 1
#define SCMD_UNLOCK 2
#define SCMD_LOCK 3
#define SCMD_PICK 4

/*. do_olc .*/
#define SCMD_OLC_REDIT 0
#define SCMD_OLC_ZEDIT 1
#define SCMD_OLC_OEDIT 2
#define SCMD_OLC_MEDIT 3
#define SCMD_OLC_SEDIT 4
#define SCMD_OLC_HEDIT 5
#define SCMD_OLC_TRIGEDIT 6
#define SCMD_OLC_SDEDIT 7
#define SCMD_OLC_RCOPY 8
#define SCMD_OLC_OCOPY 9
#define SCMD_OLC_ZCOPY 10
#define SCMD_OLC_MCOPY 11
#define SCMD_OLC_SCOPY 12
#define SCMD_OLC_TRIGCOPY 13
#define SCMD_OLC_SAVEINFO 14

/* do_light */
#define SCMD_LIGHT 0
#define SCMD_EXTINGUISH 1

/* do_follow */
#define SCMD_FOLLOW 0
#define SCMD_SHADOW 1

/* do_stat */
#define SCMD_STAT 0
#define SCMD_RSTAT 1
#define SCMD_SSTAT 2

/* do_vstat */
#define SCMD_VSTAT 0
#define SCMD_MSTAT 1
#define SCMD_OSTAT 2

/* do_estat */
#define SCMD_ESTAT 0
#define SCMD_OESTAT 1
#define SCMD_RESTAT 2

/* do_vsearch */
#define SCMD_VSEARCH 0
#define SCMD_VLIST 1
#define SCMD_VWEAR 2
#define SCMD_VITEM 3
#define SCMD_VNUM 4

/* do_report */
#define SCMD_REPORT 0
#define SCMD_GREPORT 1

/* do_cast */
#define SCMD_CAST 0
#define SCMD_CHANT 1
#define SCMD_PERFORM 2

/* do_hitall */
#define SCMD_HITALL 0
#define SCMD_TANTRUM 1

/* do_bash */
#define SCMD_BASH 0
#define SCMD_BODYSLAM 1
#define SCMD_MAUL 2

/* do_roar */
#define SCMD_ROAR 0
#define SCMD_HOWL 1

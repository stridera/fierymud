/***************************************************************************
 *   File: comm.c                                         Part of FieryMUD *
 *  Usage: Communication, socket handling, main(), central game loop       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "comm.hpp"

#include "act.hpp"
#include "board.hpp"
#include "casting.hpp"
#include "clan.hpp"
#include "commands.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "cooldowns.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "directions.hpp"
#include "editor.hpp"
#include "effects.hpp"
#include "events.hpp"
#include "exits.hpp"
#include "fight.hpp"
#include "handler.hpp"
#include "house.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "mail.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "olc.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "races.hpp"
#include "rules.hpp"
#include "screen.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"
#include "version.hpp"
#include "weather.hpp"

#include <fmt/format.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <utility>

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif
#include <bitflags.hpp>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define GMCP 201
#define MSSP 70
#define MSSP_VAR 1
#define MSSP_VAL 2

char ga_string[] = {(char)IAC, (char)GA, (char)0};
char gmcp_start_data[] = {(char)IAC, (char)SB, (char)GMCP, (char)0};
char gmcp_end_data[] = {(char)IAC, (char)SE, (char)0};

char mssp_start_data[] = {(char)IAC, (char)SB, (char)MSSP};
char mssp_end_data[] = {(char)IAC, (char)SE, (char)0};

ACMD(do_shapechange);

ush_int port;
socket_t mother_desc;

struct timeval null_time;

DescriptorData *descriptor_list = nullptr; /* master desc list */
unsigned long global_pulse = 0;            /* number of pulses since game start */
unsigned long pulse = 0;                   /* number of pulses since game started */

/* local globals */
char comm_buf[MAX_STRING_LENGTH] = {'\0'};
txt_block *bufpool = 0;            /* pool of large output buffers */
int buf_largecount = 0;            /* # of large buffers which exist */
int buf_overflows = 0;             /* # of overflows of output */
int buf_switches = 0;              /* # of switches from small to large buf */
int circle_shutdown = 0;           /* clean shutdown */
int circle_reboot = 0;             /* reboot the game after a shutdown */
int max_players = 0;               /* max descriptors available */
int tics = 0;                      /* for extern checkpointing */
int dg_act_check;                  /* toggle for act_trigger */
bool gossip_channel_active = true; /* Flag for turning on or off gossip for the whole MUD */

/* functions in this file */
int get_from_q(txt_q *queue, std::string_view dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_max_players(void);
int process_output(DescriptorData *t);
int process_input(DescriptorData *t);
void close_socket(DescriptorData *d);
timeval timediff(timeval a, timeval b);
timeval timeadd(timeval a, timeval b);
void flush_queues(DescriptorData *d);
void nonblock(socket_t s);
int perform_alias(DescriptorData *d, std::string_view orig);
void record_usage(void);
void send_gmcp_char_info(DescriptorData *d);
void make_prompt(DescriptorData *d);
void check_idle_passwords(void);
void heartbeat(int pulse);
std::string_view new_txt;
void init_descriptor(DescriptorData *newd, int desc);
int enter_player_game(DescriptorData *d);
void free_bufpools(void);

/* extern fcnts */
void boot_db(void);
void boot_world(void);
void zone_update(void);
void effect_update(void); /* In spells.c */
void point_update(void);  /* In limits.c */
void sick_update(void);   /* In limits.c */
void mobile_activity(void);
void mobile_spec_activity(void);
void string_add(DescriptorData *d, std::string_view str);
void perform_mob_violence(void);
int isbanned(std::string_view hostname);
void redit_save_to_disk(int zone_num);
void oedit_save_to_disk(int zone_num);
void medit_save_to_disk(int zone_num);
void sedit_save_to_disk(int zone_num);
void zedit_save_to_disk(int zone_num);
void ispell_init(void);
void ispell_done(void);
void free_help_table(void);
void free_social_messages(void);
void free_social_messages(void);
void free_invalid_list(void);

/* Init sockets, run game, and cleanup sockets */
void init_game(int port) {
    extern long reboot_pulse;
    extern int reboot_hours_base;
    extern int reboot_hours_deviation;

    srandom(time(0));

    log("Finding player limit.");
    max_players = get_max_players();

    log("Opening mother connection.");
    mother_desc = init_socket(port);

    event_init();

    boot_db();

    log("Signal trapping.");
    signal_setup();

    /* Decide when to reboot */
    reboot_pulse = 3600 * PASSES_PER_SEC * (reboot_hours_base - reboot_hours_deviation) +
                   random_number(0, 3600 * PASSES_PER_SEC * 2 * reboot_hours_deviation);

    log("Entering game loop.");

    ispell_init();

    game_loop(mother_desc);

    auto_save_all();

    ispell_done();

    log("Closing all sockets.");
    while (descriptor_list)
        close_socket(descriptor_list);

    CLOSE_SOCKET(mother_desc);

    if (circle_reboot != 2 && olc_save_list) { /* Don't save zones. */
        OLCSaveInfo *entry, *next_entry;
        for (entry = olc_save_list; entry; entry = next_entry) {
            next_entry = entry->next;
            if (entry->type < 0 || entry->type > 4) {
                log("OLC: Illegal save type {:d}!", entry->type);
            } else if (entry->zone < 0) {
                log("OLC: Illegal save zone {:d}!", entry->zone);
            } else {
                log("OLC: Reboot saving {} for zone {:d}.", save_info_msg[(int)entry->type], entry->zone);
                switch (entry->type) {
                case OLC_SAVE_ROOM:
                    redit_save_to_disk(entry->zone);
                    break;
                case OLC_SAVE_OBJ:
                    oedit_save_to_disk(entry->zone);
                    break;
                case OLC_SAVE_MOB:
                    medit_save_to_disk(entry->zone);
                    break;
                case OLC_SAVE_ZONE:
                    zedit_save_to_disk(entry->zone);
                    break;
                case OLC_SAVE_SHOP:
                    sedit_save_to_disk(entry->zone);
                    break;
                default:
                    log("Unexpected olc_save_list->type");
                    break;
                }
            }
        }
    }

    if (circle_reboot)
        log("Rebooting.");
    else
        log("Normal termination of game.");
}

/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port) {
    int s, opt;
    sockaddr_in sa;

    /*
     * Should the first argument to socket() be AF_INET or PF_INET?  I don't
     * know, take your pick.  PF_INET seems to be more widely adopted, and
     * Comer (_Internetworking with TCP/IP_) even makes a point to say that
     * people erroneously use AF_INET with socket() when they should be using
     * PF_INET.  However, the man pages of some systems indicate that AF_INET
     * is correct; some such as ConvexOS even say that you can use either one.
     * All implementations I've seen define AF_INET and PF_INET to be the same
     * number anyway, so ths point is (hopefully) moot.
     */

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creating socket");
        exit(1);
    }

#if defined(SO_SNDBUF)
    opt = LARGE_BUFSIZE + GARBAGE_SPACE;
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt SNDBUF");
        exit(1);
    }
#endif

#if defined(SO_REUSEADDR)
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt REUSEADDR");
        exit(1);
    }
#endif

#if defined(SO_LINGER)
    {
        linger ld;

        ld.l_onoff = 0;
        ld.l_linger = 0;
        if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0) {
            perror("setsockopt LINGER");
            exit(1);
        }
    }
#endif

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        CLOSE_SOCKET(s);
        exit(1);
    }
    nonblock(s);
    listen(s, 5);
    return s;
}

int get_max_players(void) {
    int max_descs = 0;
    std::string_view method;

    /*
     * First, we'll try using getrlimit/setrlimit.  This will probably work
     * on most systems.
     */
#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
    {
        rlimit limit;

        /* find the limit of file descs */
        method = "rlimit";
        if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
            perror("calling getrlimit");
            exit(1);
        }
        /* set the current to the maximum */
        limit.rlim_cur = limit.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
            perror("calling setrlimit");
            exit(1);
        }
#ifdef RLIM_INFINITY
        if (limit.rlim_max == RLIM_INFINITY)
            max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
        else
            max_descs = std::min<int>(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
#else
        max_descs = std::min(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
    }

#elif defined(OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
    method = "OPEN_MAX";
    max_descs = OPEN_MAX; /* Uh oh.. rlimit didn't work, but we
                           * have OPEN_MAX */
#elif defined(POSIX)
    /*
     * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
     * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming
     * in the UNIX Environment_).
     */
    method = "POSIX sysconf";
    errno = 0;
    if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
        if (errno == 0)
            max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
        else {
            perror("Error calling sysconf");
            exit(1);
        }
    }
#else
    /* if everything has failed, we'll just take a guess */
    max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
#endif

    /* now calculate max _players_ based on max descs */
    max_descs = std::min(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

    if (max_descs <= 0) {
        log("Non-positive max player limit!  (Set at {:d} using {}).", max_descs, method);
        exit(1);
    }
    log("Setting player limit to {:d} using {}.", max_descs, method);
    return max_descs;
}

void reboot_mud_prep() {
    /*do global save */
    auto_save_all();
    House_save_all();
    all_printf(
        "Rebooting.. come back in a minute or two.\n"
        "           &1&b** ****** ****&0\n"
        "         &1&b**&0 &3&b***     *****&0  &1&b**&0\n"
        "       &1&b**&0 &3&b**      &1&b*&0     &3&b***&0  &1&b*&0\n"
        "       &1&b*&0    &3&b** **   *   *  *&0 &1&b**&0\n"
        "      &1&b*&0  &3&b** * *&0          &1&b*&0     &1&b*&0\n"
        "      &1&b*&0  &3&b*&0    &1&b**&0            &3&b* *&0 &1&b*&0\n"
        "     &1&b*&0 &3&b* &1&b** *&0     &3&b*   ******&0  &1&b*&0\n"
        "      &1&b*&0   &3&b* &1&b* **&0  &3&b***&0     &1&b*&0  &3&b*&0 &1&b*&0\n"
        "        &1&b*&0  &3&b*  *&0 &1&b**********&0  &3&b***&0 &1&b*&0\n"
        "         &1&b*****&0   &3&b*     *   * *&0 &1&b*&0\n"
        "                &1&b*&0   &3&b*&0 &1&b*&0\n"
        "               &1&b*&0  &3&b* *&0  &1&b*&0\n"
        "              &1&b*&0  &3&b*  **&0  &1&b*&0\n"
        "              &1&b*&0 &3&b**   *&0 &1&b*&0\n"
        "                &1&b*&0 &3&b*&0 &1&b*&0\n"
        "                &1&b*&0 &3&b*&0  &1&b**&0\n"
        "               &1&b**&0     &1&b****&0\n"
        "              &1&b***&0  &3&b* *&0    &1&b****&0\n");
    touch("../.fastboot");
}

/* This will generally be called upon login, to let people know
 * immediately if a reboot is coming up.  It could be especially useful
 * to a god, if they log in and see "15 seconds to reboot", because
 * it would give them time to abort the reboot if they so desire. */
void personal_reboot_warning(CharData *ch) {
    int minutes_till, seconds_till;

    if ((reboot_pulse - global_pulse) / (PASSES_PER_SEC * 60) < reboot_warning_minutes) {
        minutes_till = (reboot_pulse - global_pulse - 1) / (PASSES_PER_SEC * 60) + 1;
        if (minutes_till == 1) {
            seconds_till = (reboot_pulse - global_pulse) / PASSES_PER_SEC;
            char_printf(ch, "\n\007&4&b***&0 &7&b{:d} second{} to reboot&0 &4&b***&0\n", seconds_till,
                        seconds_till == 1 ? "" : "s");
        } else {
            char_printf(ch,
                        "\n\007&1&b*&3&bATTENTION&1&b*&0  The mud will &7&bREBOOT&0 in &6&b{:d} minute{}&0  "
                        "&1&b*&3&bATTENTION&1&b*&0\n",
                        minutes_till, minutes_till == 1 ? "" : "s");
        }
    }
}

void rebootwarning(int minutes) {
    all_printf(
        "&1&b*&3&bATTENTION&1&b*&0  The mud will &7&bREBOOT&0 in &6&b{:d} minute{}&0  &1&b*&3&bATTENTION&1&b*&0\n",
        minutes, minutes == 1 ? "" : "s");
}

void cancel_auto_reboot(int postponed) {
    if (!reboot_warning)
        return;

    reboot_warning = 0;
    if (postponed)
        all_printf("&6*** Automatic Reboot Postponed ***&0\n");
    else
        all_printf("&6&b*** Automatic Reboot Cancelled ***&0\n");

    if (should_restrict) {
        if (restrict_reason == RESTRICT_NONE || restrict_reason == RESTRICT_AUTOBOOT) {
            should_restrict = 0;
            restrict_reason = RESTRICT_NONE;
            all_printf("*** Mortal logins reenabled ***\n");
            log("Login restriction removed.");
        }
    }
}

void check_auto_rebooting() {
    int minutes_till, seconds_till;
    static int reboot_prepped = 0;

    if (!reboot_auto)
        return;

    if (reboot_pulse <= global_pulse) {
        /* TIME TO REBOOT! */

        /* Note that this block will be visited twice due to incrementing and
         * checking reboot_prepped. This allows the main loop to execute an
         * additional time after we call reboot_mud_prep(). Therefore, the
         * shutdown notification text (the mushroom cloud) will actually be
         * sent to player sockets. */

        if (reboot_prepped == 1) {
            circle_shutdown = circle_reboot = 1;
        } else if (reboot_prepped == 0) {
            log("Automatic reboot.");
            reboot_mud_prep();
        }
        reboot_prepped++;
    } else if ((reboot_pulse - global_pulse) / (PASSES_PER_SEC * 60) < reboot_warning_minutes) {
        minutes_till = (reboot_pulse - global_pulse - 1) / (PASSES_PER_SEC * 60) + 1;
        if (minutes_till < last_reboot_warning || !reboot_warning || !last_reboot_warning) {
            rebootwarning(minutes_till);
            /* The following disables mortal logins when there are two minutes left.
             * Or, if someone suddenly sets the mud to reboot in 1 minute ("autoboot
             * :1") it will disable mortal logins then.
             *
             * Also, if a god reenables mortal logins ("wizlock 0"), this won't
             * override that. This is intentional. */
            if ((minutes_till == 2 || (minutes_till < 2 && !reboot_warning)) && should_restrict < LVL_IMMORT) {
                log("Mortal logins prevented due to imminent automatic reboot.");
                should_restrict = LVL_IMMORT;
                restrict_reason = RESTRICT_AUTOBOOT;
                all_printf("*** No more mortal logins ***\n");
            }
            reboot_warning = 1;
            last_reboot_warning = minutes_till;
        } else if (minutes_till == 1) {
            /* Additional warnings during the last minute */
            seconds_till = (reboot_pulse - global_pulse) / PASSES_PER_SEC;
            if (seconds_till == 30 || seconds_till == 10 || seconds_till == 5) {
                all_printf("&4&b*&0 &7&b{:d} second{} to reboot&0 &4&b*&0\n", seconds_till,
                           seconds_till == 1 ? "" : "s");
            }
        }
    }
}

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat functions
 * such as mobile_activity().
 */
void game_loop(int mother_desc) {
    fd_set input_set, output_set, exc_set, null_set;
    timeval last_time, before_sleep, opt_time, process_time, now, timeout;
    char comm[MAX_INPUT_LENGTH];
    DescriptorData *d, *next_d;
    int missed_pulses, maxdesc, aliased;

    /* initialize various time values */
    null_time.tv_sec = 0;
    null_time.tv_usec = 0;
    opt_time.tv_usec = OPT_USEC;
    opt_time.tv_sec = 0;
    FD_ZERO(&null_set);

    gettimeofday(&last_time, nullptr);

    /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
    while (!circle_shutdown) {

        /* Sleep if we don't have any connections and are not about to reboot */
        if (descriptor_list == nullptr && !reboot_warning) {
            log("No connections.  Going to sleep.");
            FD_ZERO(&input_set);
            FD_SET(mother_desc, &input_set);
            if (select(mother_desc + 1, &input_set, (fd_set *)0, (fd_set *)0, nullptr) < 0) {
                if (errno == EINTR)
                    log("Waking up to process signal.");
                else
                    perror("Select coma");
            } else
                log("New connection.  Waking up.");
            gettimeofday(&last_time, nullptr);
        }
        /* Set up the input, output, and exception sets for select(). */
        FD_ZERO(&input_set);
        FD_ZERO(&output_set);
        FD_ZERO(&exc_set);
        FD_SET(mother_desc, &input_set);

        maxdesc = mother_desc;
        for (d = descriptor_list; d; d = d->next) {
            if (d->descriptor > maxdesc)
                maxdesc = d->descriptor;
            FD_SET(d->descriptor, &input_set);
            FD_SET(d->descriptor, &output_set);
            FD_SET(d->descriptor, &exc_set);
        }

        /*
         * At this point, we have completed all input, output and heartbeat
         * activity from the previous iteration, so we have to put ourselves
         * to sleep until the next 0.1 second tick.  The first step is to
         * calculate how long we took processing the previous iteration.
         */

        gettimeofday(&before_sleep, nullptr); /* current time */
        process_time = timediff(before_sleep, last_time);

        /*
         * If we were asleep for more than one pass, count missed pulses and sleep
         * until we're resynchronized with the next upcoming pulse.
         */
        if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
            missed_pulses = 0;
        } else {
            missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
            missed_pulses += process_time.tv_usec / OPT_USEC;
            process_time.tv_sec = 0;
            process_time.tv_usec = process_time.tv_usec % OPT_USEC;
        }

        /* Calculate the time we should wake up */
        last_time = timeadd(before_sleep, timediff(opt_time, process_time));

        /* Now keep sleeping until that time has come */
        gettimeofday(&now, nullptr);
        timeout = timediff(last_time, now);

        /* go to sleep */
        do {
            if (select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timeout) < 0) {
                if (errno != EINTR) {
                    perror("Select sleep");
                    exit(1);
                }
            }
            gettimeofday(&now, nullptr);
            timeout = timediff(last_time, now);
        } while (timeout.tv_usec || timeout.tv_sec);

        /* poll (without blocking) for new input, output, and exceptions */
        if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
            perror("Select poll");
            return;
        }
        /* If there are new connections waiting, accept them. */
        if (FD_ISSET(mother_desc, &input_set))
            new_descriptor(mother_desc);

        /* kick out the freaky folks in the exception set */
        for (d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if (FD_ISSET(d->descriptor, &exc_set)) {
                FD_CLR(d->descriptor, &input_set);
                FD_CLR(d->descriptor, &output_set);
                close_socket(d);
            }
        }

        /* process descriptors with input pending */
        for (d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if (FD_ISSET(d->descriptor, &input_set))
                if (process_input(d) < 0)
                    close_socket(d);
        }

        /* process commands we just read from process_input */
        for (d = descriptor_list; d; d = next_d) {
            next_d = d->next;

            if ((--(d->wait) <= 0) && get_from_q(&d->input, comm, &aliased)) {
                if (d->character) {
                    /* reset the idle timer & pull char back from void if necessary */
                    if (d->original)
                        d->original->char_specials.timer = 0;
                    else
                        d->character->char_specials.timer = 0;
                    if (!d->connected && GET_WAS_IN(d->character) != NOWHERE) {
                        if (d->character->in_room != NOWHERE)
                            char_from_room(d->character);
                        char_to_room(d->character, GET_WAS_IN(d->character));
                        GET_WAS_IN(d->character) = NOWHERE;
                        act("$n has returned.", true, d->character, 0, 0, TO_ROOM);
                    }
                }
                d->wait = 1;
                d->prompt_mode = 1;

                /* reversed these top 2 if checks so that you can use the page_string */
                /* function in the editor */
                if (!d->page_outbuf->empty()) /* reading something w/ pager     */
                    get_paging_input(d, comm);
                else if (EDITING(d))
                    editor_interpreter(d, comm);
                else if (!d->str.empty()) /* writing boards, mail, etc.     */
                    string_add(d, comm);
                else if (d->connected != CON_PLAYING) /* in menus, etc. */
                    nanny(d, comm);
                else {           /* else: we're playing normally */
                    if (aliased) /* to prevent recursive aliases */
                        d->prompt_mode = 0;
                    else if (perform_alias(d, comm)) /* run it through aliasing system */
                        get_from_q(&d->input, comm, &aliased);
                    desc_printf(d, "\n");
                    command_interpreter(d->character, comm); /* send it to interpreter */
                }
            }
            event_process();
        }

        /* send queued output out to the operating system (ultimately to user) */
        for (d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if (FD_ISSET(d->descriptor, &output_set) && !d->output.empty()) {
                if (process_output(d) < 0)
                    close_socket(d);
                else
                    d->prompt_mode = 1;
            }
        }

        /* kick out folks in the CON_CLOSE state */
        for (d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if (STATE(d) == CON_CLOSE)
                close_socket(d);
        }

        /* give each descriptor an appropriate prompt */
        for (d = descriptor_list; d; d = d->next) {
            if (d->prompt_mode) {
                make_prompt(d);
                d->prompt_mode = 0;
            }
        }

        /*
         * Now, we execute as many pulses as necessary--just one if we haven't
         * missed any pulses, or make up for lost time if we missed a few
         * pulses by sleeping for too long.
         */
        ++missed_pulses;

        if (missed_pulses <= 0) {
            log("SYSERR: MISSED_PULSES IS NONPOSITIVE!!!");
            missed_pulses = 1;
        }

        /* If we missed more than 30 seconds worth of pulses, forget it */
        if (missed_pulses > (30 * PASSES_PER_SEC)) {
            log("Warning: Missed more than 30 seconds worth of pulses");
            missed_pulses = 30 * PASSES_PER_SEC;
        }

        /* Now execute the heartbeat functions */
        while (missed_pulses--) {
            event_process();
            heartbeat(++pulse);
        }

        /* Update tics for deadlock protection (UNIX only) */
        tics++;
    }
}

void heartbeat(int pulse) {
    static int mins_since_autosave = 0;

    global_pulse++;

    if (!(pulse % PULSE_DG_SCRIPT))
        script_trigger_check();

    event_process();

    if (!(pulse % PULSE_ZONE))
        zone_update();

    if (!(pulse % (15 * PASSES_PER_SEC))) /* 15 seconds */
        check_idle_passwords();

    if (!(pulse % PULSE_MOBILE))
        mobile_activity();

    if (!(pulse % PULSE_VIOLENCE)) {
        perform_violence();
        mobile_spec_activity();
    }

    if (!(pulse % (PULSE_VIOLENCE / 2))) {
        /* Every other combat round, NPCs in battle attempt skill and
         * spell-based attacks. */
        perform_mob_violence();
    }

    if (!(pulse % PASSES_PER_SEC)) {
        if (reboot_auto)
            check_auto_rebooting();
    }

    if (!(pulse % (PULSE_VIOLENCE * 6))) {
        /* Check poison and disease every 6 rounds. */
        sick_update();
    }

    if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
        update_weather(pulse);
        increment_game_time(); /* Increment game time by an hour. */
        effect_update();
        point_update();
        check_time_triggers();
    }

    if (!(pulse % PULSE_AUTOSAVE)) { /* 1 minute */
        if (++mins_since_autosave >= 1) {
            mins_since_autosave = 0;
            auto_save_all();
            House_save_all();
        }
    }
    /* Commenting entire 5 minute check section because there would be
       nothing to run once this since function was commented - RSD  */
    /* if (!(pulse % (5 * 60 * PASSES_PER_SEC))) {  */ /* 5 minutes */
    /*  record_usage();
       }
     */
    event_process();
}

/* ******************************************************************
 *  general utility stuff (for local use)                            *
 ****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
timeval timediff(timeval a, timeval b) {
    timeval rslt;

    if (a.tv_sec < b.tv_sec)
        return null_time;
    else if (a.tv_sec == b.tv_sec) {
        if (a.tv_usec < b.tv_usec)
            return null_time;
        else {
            rslt.tv_sec = 0;
            rslt.tv_usec = a.tv_usec - b.tv_usec;
            return rslt;
        }
    } else { /* a->tv_sec > b->tv_sec */
        rslt.tv_sec = a.tv_sec - b.tv_sec;
        if (a.tv_usec < b.tv_usec) {
            rslt.tv_usec = a.tv_usec + 1000000 - b.tv_usec;
            rslt.tv_sec--;
        } else
            rslt.tv_usec = a.tv_usec - b.tv_usec;
        return rslt;
    }
}

/* add 2 timevals */
timeval timeadd(timeval a, timeval b) {
    timeval rslt;

    rslt.tv_sec = a.tv_sec + b.tv_sec;
    rslt.tv_usec = a.tv_usec + b.tv_usec;

    while (rslt.tv_usec >= 1000000) {
        rslt.tv_usec -= 1000000;
        rslt.tv_sec++;
    }

    return rslt;
}

void record_usage(void) {
    int sockets_connected = 0, sockets_playing = 0;
    DescriptorData *d;

    for (d = descriptor_list; d; d = d->next) {
        sockets_connected++;
        if (!d->connected)
            sockets_playing++;
    }

    log("nusage: {:-3d} sockets connected, {:-3d} sockets playing", sockets_connected, sockets_playing);
}

void offer_mssp(DescriptorData *d) {
    char offer_mssp[] = {(char)IAC, (char)WILL, (char)MSSP, (char)0};
    write_to_descriptor(d->descriptor, offer_mssp);
}

void offer_gmcp(DescriptorData *d) {
    char offer_gmcp[] = {(char)IAC, (char)WILL, (char)GMCP, (char)0};
    write_to_descriptor(d->descriptor, offer_gmcp);
}

void request_ttype(DescriptorData *d) {
    char request_ttype[] = {(char)IAC, (char)DO, (char)TELOPT_TTYPE, (char)0};
    write_to_descriptor(d->descriptor, request_ttype);
}

void send_opt(DescriptorData *d, byte a) {
    const byte buf[] = {(char)IAC, (char)SB, a, (char)TELQUAL_SEND, (char)IAC, (char)SE};
    write_to_descriptor(d->descriptor, buf);
}

void send_gmcp(DescriptorData *d, std::string_view package, json j) {
    write_to_descriptor(d->descriptor, fmt::format("{}{} {}{}", gmcp_start_data, package, j.dump(), gmcp_end_data));
}

void offer_gmcp_services(DescriptorData *d) {
    json client = {{"version", std::string(mudlet_client_version)}, {"url", std::string(mudlet_client_url)}};
    send_gmcp(d, "Client.GUI", client);
    send_gmcp(d, "Client.Map", {{"url", std::string(mudlet_map_url)}});
}

void handle_gmcp_request(DescriptorData *d, std::string_view txt) {
    // This is for GMCP requests from the clients.
    if (txt == "External.Discord.Hello") {
        send_gmcp(d, "External.Discord.Info",
                  {{"applicationid", std::string{discord_app_id}}, {"inviteurl", std::string{discord_invite_url}}});
        auto now = std::chrono::system_clock::now();
        send_gmcp(d, "External.Discord.Status",
                  {
                      {"state", "Logging into Fierymud (fierymud.org:4000)"},
                      {"details", "Connecting to the MUD..."},
                      {"game", "Fierymud"},
                      {"smallimage", {"servericon"}},
                      {"smallimagetext", "Fierymud"},
                      //   {"partysize", 0},
                      //   {"partymax", 10},
                      {"starttime", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()},
                  });
    } else if (txt == "Char.Skills.Get") {
        if (d->character) {
            auto known_skills = json::array();
            for (int i = 0; i < MAX_SKILLS; i++) {
                int skill = skill_sort_info[i];
                if (!IS_SKILL(i))
                    continue;
                if (skills[i].name[0] == '!')
                    continue;
                if (GET_SKILL(d->character, i) <= 0)
                    continue;
                known_skills.push_back(skills[i].name);
            }
            send_gmcp(d, "Char.Skills.List", known_skills);
        }
    } else if (txt == "Char.Spells.Get") {
        if (d->character) {
            auto known_spells = json::array();
            for (int i = 0; i < MAX_SKILLS; i++) {
                int skill = skill_sort_info[i];
                if (!IS_SPELL(i))
                    continue;
                if (skills[i].name[0] == '!')
                    continue;
                if (GET_SKILL(d->character, i) <= 0)
                    continue;
                known_spells.push_back(skills[i].name);
            }
            send_gmcp(d, "Char.Spells.List", known_spells);
        }
    }
    // log("GMCP request: {}", txt);
}

static bool supports_ansi(std::string_view detected_term) {
    static const std::vector<std::string_view> ansi_terms{"xterm", "mudlet", "ansi",      "vt100",
                                                          "vt102", "vt220",  "terminator"};
    for (auto &ansi_term : ansi_terms)
        if (matches_start(ansi_term, detected_term))
            return true;
    return false;
}

void handle_telopt_request(DescriptorData *d, std::string_view txt) {
    char bcbuf[15] = "0";

    d->character->player_specials->client = txt;
    if (supports_ansi(txt)) {
        SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
        SET_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
        write_to_descriptor(d->descriptor, "Color is on.\r\n");
    } else {
        REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_1);
        REMOVE_FLAG(PRF_FLAGS(d->character), PRF_COLOR_2);
        write_to_descriptor(d->descriptor, "Color is off.\r\n");
    }
    if ((get_build_number() >= 0)) {
        sprintf(bcbuf, " %d", get_build_number());
    }

    if (environment == ENV_PROD) {
        string_to_output(d, GREETINGS);
    } else if (environment == ENV_TEST) {
        string_to_output(d, TEST_GREETING);
    } else if (environment == ENV_DEV) {
        string_to_output(d, DEV_GREETING);
    }
    string_to_output(d, bcbuf);
    string_to_output(d, WHOAREYOU);
}

/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(DescriptorData *d) {
    char off_string[] = {
        (char)IAC,
        (char)WILL,
        (char)TELOPT_ECHO,
        (char)0,
    };

    desc_printf(d, off_string);
}

void send_gmcp_prompt(DescriptorData *d) {
    CharData *ch = d->character, *vict = FIGHTING(ch), *tank;
    effect *eff;

    if (!d->gmcp_enabled) {
        write_to_descriptor(d->descriptor, ga_string);
        return;
    }

    json effects = json::array();
    for (eff = ch->effects; eff; eff = eff->next) {
        if (eff->duration >= 0 && (!eff->next || eff->next->type != eff->type)) {
            effects.push_back({{"name", skills[eff->type].name}, {"duration", eff->duration + 1}});
        }
    }

    json combat = json::object();
    if (vict && (tank = FIGHTING(vict))) {
        combat["tank"] = {{"name", PERS(tank, ch)}, {"hp", GET_HIT(tank)}, {"max_hp", GET_MAX_HIT(tank)}};
        combat["opponent"] = {{"name", PERS(vict, ch)}, {"hp_percent", 100 * GET_HIT(vict) / GET_MAX_HIT(vict)}};
    }

    /* Need to construct a json string.  Would be nice to get a module to do it for us, but the code base is too old to
     * rely on something like that. */
    auto position = [&]() {
        if (d->original)
            return sprinttype(GET_POS(d->original), position_types);

        else
            return sprinttype(GET_POS(d->character), position_types);
    }();

    json gmcp_data = {
        {"name", strip_ansi(GET_NAME(ch))},
        {"class", CLASS_NAME(ch)},
        {"exp_percent", xp_percentage(REAL_CHAR(ch))},
        {"alignment", GET_ALIGNMENT(ch)},
        {"position", position},
        {"hiddenness", GET_HIDDENNESS(ch)},
        {"level", GET_LEVEL(ch)},
        {"Vitals",
         {{"hp", GET_HIT(ch)}, {"max_hp", GET_MAX_HIT(ch)}, {"mv", GET_MOVE(ch)}, {"max_mv", GET_MAX_MOVE(ch)}}},
        {"Worth",
         {{"Carried",
           {{"platinum", GET_PLATINUM(ch)},
            {"gold", GET_GOLD(ch)},
            {"silver", GET_SILVER(ch)},
            {"copper", GET_COPPER(ch)}}},
          {"Bank",
           {{"platinum", GET_BANK_PLATINUM(ch)},
            {"gold", GET_BANK_GOLD(ch)},
            {"silver", GET_BANK_SILVER(ch)},
            {"copper", GET_BANK_COPPER(ch)}}}}},
        {"Effects", effects},
        {"Combat", combat},
        {"SpellSlots", get_spell_slots_available(ch)}};

    send_gmcp(d, "Char", gmcp_data);
    write_to_descriptor(d->descriptor, ga_string);
    std::string details = fmt::format("Character: {}  Class: {}  Level: {}", GET_NAME(ch),
                                      capitalize_first(CLASS_NAME(ch)), GET_LEVEL(ch));
    auto login = std::chrono::system_clock::from_time_t(ch->player.time.logon);
    send_gmcp(d, "External.Discord.Status",
              {
                  {"state", "Playing Fierymud (fierymud.org:4000)"},
                  {"details", details},
                  {"game", "Fierymud"},
                  {"smallimage", {"servericon"}},
                  {"smallimagetext", "Fierymud"},
                  {"starttime", std::chrono::duration_cast<std::chrono::seconds>(login.time_since_epoch()).count()},
              });
}

void send_gmcp_room(CharData *ch) {
    static char response[MAX_STRING_LENGTH];
    std::string_view cur = response;

    int roomnum = IN_ROOM(ch);
    RoomData *room = &world[roomnum];
    Exit *exit;

    if (IS_MOB(ch) || !ch->desc || !ch->desc->gmcp_enabled) {
        return;
    }

    if (IS_DARK(roomnum) && !CAN_SEE_IN_DARK(ch)) {
        send_gmcp(ch->desc, "Room", {});
        return;
    }

    auto door_name = [](Exit *exit) {
        if (!exit) {
            return std::string_view{"door"};
        }
        if (exit->keyword.empty()) {
            return exit->general_description.empty() ? "door" : exit->general_description;
        }
        return exit->keyword;
    };
    json exits = json::object();
    for (int dir = 0; dir < NUM_OF_DIRS; dir++) {
        if ((exit = world[roomnum].exits[dir]) && EXIT_DEST(exit) && can_see_exit(ch, roomnum, exit)) {
            json exit_json = {{"to_room", EXIT_DEST(exit)->vnum}};
            if (EXIT_IS_DOOR(exit)) {
                exit_json["is_door"] = true;
                exit_json["door_name"] = door_name(exit);
                exit_json["door"] = EXIT_IS_CLOSED(exit) ? EXIT_IS_LOCKED(exit) ? "locked" : "closed" : "open";
            } else {
                exit_json["is_door"] = false;
            }
            exits[capdirs[dir]] = exit_json;
        }
    }

    json gmcp_data = {{"zone", zone_table[room->zone].name},
                      {"id", room->vnum},
                      {"name", room->name},
                      {"type", sectors[room->sector_type].name},
                      {"Exits", exits}};
    send_gmcp(ch->desc, "Room", gmcp_data);
}

void send_mssp(DescriptorData *d) {
    std::string mssp_data;
    DescriptorData *t;
    int sockets_playing = 0;

    if (!d) {
        return;
    }

    for (t = descriptor_list; t; t = t->next)
        if (!t->connected)
            sockets_playing++;

    mssp_data = fmt::format("{:c}{:c}{:c}", IAC, SB, MSSP);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "NAME", MSSP_VAL, "FieryMUD");
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "PLAYERS", MSSP_VAL, sockets_playing);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "UPTIME", MSSP_VAL, boot_time[0]);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "AREAS", MSSP_VAL, top_of_zone_table + 1);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "MOBILES", MSSP_VAL, top_of_mobt + 1);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "OBJECTS", MSSP_VAL, top_of_objt + 1);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "ROOMS", MSSP_VAL, top_of_world + 1);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "CLASSES", MSSP_VAL, NUM_CLASSES);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "LEVELS", MSSP_VAL, 99);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "RACES", MSSP_VAL, NUM_RACES);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "SKILLS", MSSP_VAL, 118);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "DISCORD", MSSP_VAL, discord_invite_url);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "ICON", MSSP_VAL, fierymud_icon);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "WEBSITE", MSSP_VAL, fierymud_url);
    mssp_data += fmt::format("{:c}{}{:c}{}", MSSP_VAR, "GENRE", MSSP_VAL, "Fantasy");

    mssp_data += fmt::format("{:c}{:c}", IAC, SE);
    write_to_descriptor(d->descriptor, mssp_data);
}

/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(DescriptorData *d) {
    char on_string[] = {
        (char)IAC, (char)WONT, (char)TELOPT_ECHO, (char)TELOPT_NAOFFD, (char)TELOPT_NAOCRD, (char)0,
    };

    string_to_output(d, on_string);
}

std::string prompt_str(CharData *ch) {
    enum class Token {
        UNKNOWN, /* Default, not expecting anything in particular */
        CONTROL, /* Last char was a %, next one is a control code */
        PERCENT, /* Last two chars were %p, next one is percent code */
        COIN,    /* Last two chars were %c, next one is coins code */
        COOLDOWN /* Last two chars were %d, next one is cooldown code */
    };

    CharData *vict = FIGHTING(ch), *tank;
    std::string prompt;
    std::string_view raw = GET_PROMPT(ch);
    int color, temp;
    Token expecting = Token::UNKNOWN;
    bool found_x = false;
    effect *eff;

    /* No prompt?  Use the default! */
    if (raw.empty())
        return "&0FieryMUD: Set your prompt (see 'help prompt')>";

    /* Use color? */
    color = (PRF_FLAGGED(ch, PRF_COLOR_1) || PRF_FLAGGED(ch, PRF_COLOR_2) ? 1 : 0);

    /* No need to parse if there aren't % symbols */
    if (raw.find('%') == std::string_view::npos) {
        return fmt::format("{}&0", raw);
    }

    /*
     * Insert wizinvis and AFK flags at the beginning.  If we DON'T find the
     * %x flag below, we'll keep it.  If not, we'll skip it when we return.
     * End all flags with an extra space!  The %x case below expects it.
     */
    if (!IS_NPC(ch)) {
        if (GET_INVIS_LEV(ch) > 0)
            prompt += fmt::format("<wizi {:03d}> ", GET_INVIS_LEV(ch));
        if (PRF_FLAGGED(ch, PRF_ROOMVIS) && GET_INVIS_LEV(ch))
            prompt += "<rmvis> ";
        if (PRF_FLAGGED(ch, PRF_AFK))
            prompt += "<AFK> ";
        /* If any flags above, insert newline. */
        if (!prompt.empty()) {
            prompt += "\n";
        }
    }

    for (char c : raw) {
        if (expecting == Token::UNKNOWN) {
            if (c == '%')
                /* Next character will be a control code. */
                expecting = Token::CONTROL;
            else
                /* Not a control code starter...just copy the character. */
                prompt += c;
        } else if (expecting == Token::CONTROL) {
            /* Reset what we are expecting for the next char. */
            expecting = Token::UNKNOWN;
            switch (c) {
            case 'h':
                prompt += std::to_string(GET_HIT(ch));
                break;
            case 'H':
                prompt += std::to_string(GET_MAX_HIT(ch));
                break;
                /* Mana isn't available...
                        case 'm':
                          prompt += std::to_string(GET_MANA(ch));
                          break;
                        case 'M':
                          prompt += std::to_string(GET_MAX_MANA(ch));
                          break;
                */
            case 'v':
                prompt += std::to_string(GET_MOVE(ch));
                break;
            case 'V':
                prompt += std::to_string(GET_MAX_MOVE(ch));
                break;
            case 'i':
            case 'I':
                prompt += std::to_string(GET_HIDDENNESS(ch));
                break;
            case 'a':
            case 'A':
                prompt += std::to_string(GET_ALIGNMENT(ch));
                break;
            case 'n':
                /* Current character's name. */
                prompt += GET_NAME(ch);
                break;
            case 'k':
                /* Current Character's Class */
                prompt += CLASS_FULL(ch);
                break;
            case 'N':
                /* If switched, show original char's name. */
                prompt += GET_NAME(REAL_CHAR(ch));
                break;
            case 'd':
                /* Cooldown bar: next char will be control code */
                expecting = Token::COOLDOWN;
                break;
            case 'e':
                /* Show current to-next-level exp status */
                prompt += exp_bar(REAL_CHAR(ch), 20, 20, 20, color);
                break;
            case 'E':
                /* Show current to-next-level exp status */
                prompt += exp_message(REAL_CHAR(ch));
                break;
            case 'l':
            case 'L':
                /* List active spells. */
                for (eff = ch->effects; eff; eff = eff->next)
                    if (eff->duration >= 0 && (!eff->next || eff->next->type != eff->type)) {
                        if (color && EFF_FLAGGED(ch, EFF_DETECT_MAGIC)) {
                            if (eff->duration <= 1)
                                prompt += CLR(ch, FRED);
                            else if (eff->duration <= 3)
                                prompt += CLR(ch, HRED);
                        }
                        prompt += skills[eff->type].name;
                        if (eff->next)
                            prompt += ", ";
                        if (color && EFF_FLAGGED(ch, EFF_DETECT_MAGIC))
                            prompt += CLR(ch, ANRM);
                    }
                break;
            case 'p':
            case 'P':
                /* Percent of x remaining: next char will be a control code. */
                expecting = Token::PERCENT;
                break;
            case 'c':
            case 'C':
                /* Amount of gold held/in bank: next char will be a control code. */
                expecting = Token::COIN;
                break;
            case 'w':
                /* All money held. */
                prompt += fmt::format("&0{}&6p&0 {}&3g&0 {}s {}&3c&0", GET_PLATINUM(ch), GET_GOLD(ch), GET_SILVER(ch),
                                      GET_COPPER(ch));
                break;
            case 'W':
                /* All money in bank. */
                prompt += fmt::format("&0{}&6p&0 {}&3g&0 {}s {}&3c&0", GET_BANK_PLATINUM(ch), GET_BANK_GOLD(ch),
                                      GET_BANK_SILVER(ch), GET_BANK_COPPER(ch));
                break;
            case 'o':
                /* Show the victim's name and status. */
                if (vict)
                    prompt += fmt::format("{} &0({})", PERS(vict, ch),
                                          status_string(GET_HIT(vict), GET_MAX_HIT(vict), STATUS_ALIAS));
                break;
            case 'O':
                /* Just victim's name. */
                if (vict)
                    prompt += PERS(vict, ch);
                break;
            case 't':
                /* Show the tank's name and status. */
                if (vict && (tank = FIGHTING(vict)))
                    prompt += fmt::format("{} &0({}&0)", PERS(tank, ch),
                                          status_string(GET_HIT(tank), GET_MAX_HIT(tank), STATUS_ALIAS));
                break;
            case 'T':
                /* Just the tank's name. */
                if (vict && (tank = FIGHTING(vict)))
                    prompt += PERS(tank, ch);
                break;
            case 'g':
                if (ch->group_master)
                    prompt += fmt::format(
                        "{} &0({}&0)", PERS(ch->group_master, ch),
                        status_string(GET_HIT(ch->group_master), GET_MAX_HIT(ch->group_master), STATUS_ALIAS));
                break;
            case 'G':
                if (ch->group_master)
                    prompt += PERS(ch->group_master, ch);
                break;
            case 'r':
                prompt += std::to_string(GET_RAGE(ch));
                break;
            case 'x':
            case 'X':
                /* Flags like wizinvis and AFK. */
                found_x = true;
                break;
            case 'z':
            case 'Z':
                /* Show the zone */
                prompt += zone_table[world[IN_ROOM(ch)].zone].name;
                break;
            case '#':
                prompt += std::to_string(GET_LEVEL(ch));
                break;
            case '_':
                prompt += "\n";
                break;
            case '-':
                prompt += " ";
                break;
            case '%':
                prompt += "%";
                break;
            }
        } else if (expecting == Token::PERCENT) {
            expecting = Token::UNKNOWN;
            switch (c) {
            case 'h':
            case 'H':
                temp = (100 * GET_HIT(ch)) / std::max(1, GET_MAX_HIT(ch));
                break;
                /*
                        case 'm':
                        case 'M':
                          temp = (100 * GET_MANA(ch)) / std::max(1, GET_MAX_MANA(ch));
                          break;
                */
            case 'v':
            case 'V':
                temp = (100 * GET_MOVE(ch)) / std::max(1, GET_MAX_MOVE(ch));
                break;
            default:
                continue; /* don't print anything */
            }
            prompt += fmt::format("{}%", temp);
        } else if (expecting == Token::COIN) {
            expecting = Token::UNKNOWN;
            switch (c) {
            case 'p':
                temp = GET_PLATINUM(ch);
                break;
            case 'g':
                temp = GET_GOLD(ch);
                break;
            case 's':
                temp = GET_SILVER(ch);
                break;
            case 'c':
                temp = GET_COPPER(ch);
                break;
            case 'P':
                temp = GET_BANK_PLATINUM(ch);
                break;
            case 'G':
                temp = GET_BANK_GOLD(ch);
                break;
            case 'S':
                temp = GET_BANK_SILVER(ch);
                break;
            case 'C':
                temp = GET_BANK_COPPER(ch);
                break;
            default:
                continue; /* don't print anything */
            }
            prompt += std::to_string(temp);
        } else if (expecting == Token::COOLDOWN) {
            expecting = Token::UNKNOWN;
            /* Show time left for particular cooldown */
            switch (c) {
            case 'a':
                temp = CD_INNATE_ASCEN;
                break;
            case 'b':
                temp = CD_BREATHE;
                break;
            case 'c':
                temp = CD_INNATE_BRILL;
                break;
            case 'd':
                temp = CD_DEFENSE_CHANT;
                break;
            case 'D':
                temp = CD_OFFENSE_CHANT;
                break;
            case 'e':
                temp = CD_INNATE_CHAZ;
                break;
            case 'f':
                temp = CD_INNATE_CREATE;
                break;
            case 'g':
                temp = CD_INNATE_DARKNESS;
                break;
            case 'h':
                temp = CD_DISARM;
                break;
            case 'i':
                temp = CD_FIRST_AID;
                break;
            case 'j':
                temp = CD_INSTANT_KILL;
                break;
            case 'k':
                temp = CD_INNATE_INVISIBLE;
                break;
            case 'l':
                temp = CD_LAY_HANDS;
                break;
            case 'm':
                temp = CD_INNATE_FEATHER_FALL;
                break;
            case 'n':
                temp = CD_SHAPECHANGE;
                break;
            case 'o':
                temp = CD_SUMMON_MOUNT;
                break;
            case 'p':
                temp = CD_INNATE_SYLL;
                break;
            case 'q':
                temp = CD_INNATE_TASS;
                break;
            case 'r':
                temp = CD_THROATCUT;
                break;
            case 's':
                temp = CD_INNATE_TREN;
                break;
            case 't':
                temp = CD_INNATE_BLINDING_BEAUTY;
                break;
            case 'u':
                temp = CD_INNATE_ILLUMINATION;
                break;
            case 'v':
                temp = CD_INNATE_FAERIE_STEP;
                break;
            case 'w':
                temp = CD_INNATE_STATUE;
                break;
            case 'x':
                temp = CD_INNATE_BARKSKIN;
                break;
            case 'y':
                temp = CD_INNATE_HARNESS;
                break;
            case '1':
                temp = CD_MUSIC_1;
                break;
            case '2':
                temp = CD_MUSIC_2;
                break;
            case '3':
                temp = CD_MUSIC_3;
                break;
            case '4':
                temp = CD_MUSIC_4;
                break;
            case '5':
                temp = CD_MUSIC_5;
                break;
            case '6':
                temp = CD_MUSIC_6;
                break;
            case '7':
                temp = CD_MUSIC_7;
                break;
            default:
                continue; /* don't print anything */
            }
            prompt += cooldown_bar(ch, temp, 20, 20, color);
        }
    }

    prompt += "&0";
    return prompt;
}

void make_prompt(DescriptorData *d) {
    /* Do not use dprintf or any function that ultimately calls
     * string_to_output within make_prompt!  You must use
     * write_to_descriptor, because string_to_output resets the
     * prompt flag.
     */

    if (!d->page_outbuf->empty()) {

        auto prompt = fmt::format("\r[ Return to continue, (q)uit, (r)efresh,{} or page number ({}/{}) ]\n",
                                  PAGING_PAGE(d) == 0 ? "" : " (b)ack,", PAGING_PAGE(d) + 1, PAGING_NUMPAGES(d));
        write_to_descriptor(d->descriptor, prompt);
    } else if (EDITING(d) || !d->str.empty())
        write_to_descriptor(d->descriptor, "] ");
    else if (!d->connected) {
        std::string_view prompt = prompt_str(d->character);

        write_to_descriptor(d->descriptor,
                            process_colors(prompt, COLOR_LEV(d->character) >= C_NRM ? CLR_PARSE : CLR_STRIP));
        send_gmcp_prompt(d);
    }
}

/*
 * This is one of the biggest hacks ever.
 *
 * When input is processed by the game loop (in process_input),
 * before the command is queued up for normal interpreting,
 * casting_command is called, and if the character is casting
 * and the command is ok while casting, then it's handled
 * immediately here.
 */
bool casting_command(DescriptorData *d, Arguments argument) {
    int cmd;

    /*
     * Only use this hack for descriptors with characters who are
     * actually playing.  Additionally, they must be casting.
     */
    if (!d || STATE(d) != CON_PLAYING || !d->character || !CASTING(d->character))
        return false;

    /*
     * A hack-within-a-hack to enable usage of paging while casting.
     */
    if (!d->page_outbuf->empty()) {
        get_paging_input(d, argument.get());
        return true;
    } else if (EDITING(d)) {
        editor_interpreter(d, argument.get());
        return true;
    } else if (!d->str.empty()) {
        string_add(d, argument.get());
        return true;
    }

    auto command = argument.shift();

    for (cmd = 0; cmd_info[cmd].command[0] != '\n'; ++cmd)
        if (matches(cmd_info[cmd].command, command))
            if (can_use_command(d->character, cmd))
                break;

    if (cmd_info[cmd].command[0] == '\n')
        return false;

    /*
     * Non-casting commands will be queued up normally.
     */
    if (!IS_SET(cmd_info[cmd].flags, CMD_CAST))
        return false;

    /* Handle the casting-ok command immediately. */
    desc_printf(d, "\n");
    command_interpreter(d->character, argument.get());
    return true;
}

void write_to_q(std::string_view txt, txt_q *queue, int aliased, DescriptorData *d) {
    txt_block *new_msg;

    CREATE(new_msg, txt_block, 1);
    new_msg->text = txt;
    new_msg->aliased = aliased;

    /* queue empty? */
    if (!queue->head) {
        new_msg->next = nullptr;
        queue->head = queue->tail = new_msg;
    } else {
        queue->tail->next = new_msg;
        queue->tail = new_msg;
        new_msg->next = nullptr;
    }
}

int get_from_q(txt_q *queue, std::string_view dest, int *aliased) {
    txt_block *tmp;

    /* queue empty? */
    if (!queue->head)
        return 0;

    tmp = queue->head;
    dest = queue->head->text;
    *aliased = queue->head->aliased;
    queue->head = queue->head->next;

    free(tmp);
    return 1;
}

/* Empty the queues before closing connection */
void flush_queues(DescriptorData *d) {
    txt_block *tmp;
    while (d->input.head) {
        tmp = d->input.head;
        d->input.head = d->input.head->next;
        free(tmp);
    }
}

/* Add a new string to a player's output queue */
void string_to_output(DescriptorData *t, std::string_view txt) {
    if (!t || txt.empty())
        return;

    t->output += process_colors(txt, t->character && COLOR_LEV(t->character) >= C_NRM ? CLR_PARSE : CLR_STRIP);
}

void free_bufpools(void) {
    extern struct txt_block *bufpool;
    txt_block *tmp;

    while (bufpool) {
        tmp = bufpool->next;
        free(bufpool);
        bufpool = tmp;
    }
}

/* ******************************************************************
 *  socket handling                                                  *
 ****************************************************************** */

void init_descriptor(DescriptorData *newd, int desc) {
    static int last_desc = 0;

    newd->descriptor = desc;
    newd->idle_tics = 0;
    newd->login_time = time(0);
    newd->wait = 1;
    newd->gmcp_enabled = false;
    newd->page_outbuf = new std::list<std::string>();

    if (newd->character == nullptr) {
        CREATE(newd->character, CharData, 1);
        clear_char(newd->character);
        CREATE(newd->character->player_specials, PlayerSpecialData, 1);
        newd->character->desc = newd;
    }

    STATE(newd) = CON_GET_NAME;

    if (++last_desc == 1000)
        last_desc = 1;
    newd->desc_num = last_desc;
}

int new_descriptor(int s) {
    socket_t desc;
    int sockets_connected = 0;
    unsigned long addr;
    unsigned int i;
    DescriptorData *newd;
    sockaddr_in peer;
    hostent *from;

    /* accept the new connection */
    i = sizeof(peer);
    if ((desc = accept(s, (sockaddr *)&peer, &i)) == INVALID_SOCKET) {
        perror("accept");
        return -1;
    }
    /* keep it from blocking */
    nonblock(desc);

    /* make sure we have room for it */
    for (newd = descriptor_list; newd; newd = newd->next)
        sockets_connected++;

    if (sockets_connected >= max_players) {
        write_to_descriptor(desc, "Sorry, FieryMUD is full right now... please try again later!\n");
        CLOSE_SOCKET(desc);
        return 0;
    }
    /* create a new descriptor */
    CREATE(newd, DescriptorData, 1);
    memset((char *)newd, 0, sizeof(DescriptorData));

    /* find the sitename */
    if (nameserver_is_slow || !(from = gethostbyaddr((char *)&peer.sin_addr, sizeof(peer.sin_addr), AF_INET))) {

        /* resolution failed */
        if (!nameserver_is_slow)
            perror("gethostbyaddr");

        /* find the numeric site address */
        addr = ntohl(peer.sin_addr.s_addr);
        sprintf(newd->host, "%03u.%03u.%03u.%03u", (int)((addr & 0xFF000000) >> 24), (int)((addr & 0x00FF0000) >> 16),
                (int)((addr & 0x0000FF00) >> 8), (int)((addr & 0x000000FF)));
    } else {
        strncpy(newd->host, from->h_name, HOST_LENGTH);
        *(newd->host + HOST_LENGTH) = '\0';
    }

    /* determine if the site is banned */
    if (isbanned(newd->host) == BAN_ALL) {
        write_to_descriptor(desc, BANNEDINTHEUSA);
        write_to_descriptor(desc, "\n Connection logged from: {}\n\n", newd->host);
        CLOSE_SOCKET(desc);
        log(LogSeverity::Stat, LVL_GOD, "BANNED: Connection attempt denied from [{}]", newd->host);
        free(newd);
        return 0;
    }

    init_descriptor(newd, desc);

    /* prepend to list */
    newd->next = descriptor_list;
    descriptor_list = newd;

    request_ttype(newd);
    offer_mssp(newd);
    offer_gmcp(newd);

    return 0;
}

int process_output(DescriptorData *t) {
    static int result;

    /* add the extra CRLF if the person isn't in compact mode */
    if (!t->connected && t->character && !PRF_FLAGGED(t->character, PRF_COMPACT))
        t->output += "\n";

    /*
     * now, send the output.  If this is an 'interruption', use the prepended
     * CRLF, otherwise send the straight output sans CRLF.
     */
    if (!t->prompt_mode) /* && !t->connected) */
        result = write_to_descriptor(t->descriptor, "\r\n" + t->output);
    else
        result = write_to_descriptor(t->descriptor, t->output);

    /* handle snooping: prepend "% " and send to snooper */
    if (t->snoop_by)
        desc_printf(t->snoop_by, "&2((&0 {} &2))&0", trim(t->output));

    t->output.clear();

    return result;
}

int write_to_descriptor(socket_t desc, std::string_view txt) {
    int total, bytes_written;

    total = txt.length();
    bytes_written = write(desc, txt.data(), total);

    if (bytes_written < 0) {
        perror("Write to socket");
        return -1;
    } else
        return bytes_written;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this function is called.  We must maintain that
 * before returning.
 */
int process_input(DescriptorData *t) {
    if (!t)
        return -1;

    std::string input_buffer(t->inbuf);
    char tmp[MAX_INPUT_LENGTH + 8] = {0};
    std::string telnet_opts;

    size_t buf_length = input_buffer.length();
    size_t space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;
    size_t command_space_left = MAX_INPUT_LENGTH - 1;

    if (space_left <= 0) {
        log("process_input: closing connection due to input overflow [{}]", t->host);
        return -1;
    }

    ssize_t bytes_read = read(t->descriptor, &input_buffer[buf_length], space_left);
    if (bytes_read < 0) {
        if (errno != EAGAIN) {
            log("process_input: connection lost [{}]", t->host);
            return -1;
        }
    } else if (bytes_read == 0) {
        log("EOF on socket read (connection closed by peer) [{}]", t->host);
        return -1;
    }

    input_buffer.resize(buf_length + bytes_read);

    bool in_telnet_negotiation = false;
    char telcmd = 0;
    int data_mode = 0;
    std::string processed_input;
    std::string telnet_command;

    for (size_t i = 0; i < input_buffer.length(); ++i) {
        char c = input_buffer[i];

        if (c == (char)IAC) {
            in_telnet_negotiation = true;
        } else if (in_telnet_negotiation && c == (char)SE) {
            if (data_mode == 1) {
                handle_gmcp_request(t, telnet_opts);
            } else if (data_mode == 2) {
                handle_telopt_request(t, telnet_opts);
            }
            data_mode = 0;
            in_telnet_negotiation = false;
        } else if (in_telnet_negotiation) {
            telcmd = c;
            in_telnet_negotiation = false;
        } else if (data_mode) {
            telnet_command += c;
        } else {
            if (c == '\n' || c == '\r' || processed_input.length() >= MAX_INPUT_LENGTH - 1) {
                if (!processed_input.empty()) {
                    if (t->snoop_by) {
                        desc_printf(t->snoop_by, "&6>>&b {} &0\n", processed_input);
                    }
                    write_to_q(processed_input.c_str(), &t->input, 0, t);
                }
                processed_input.clear();
            } else if (c == '\b' && !processed_input.empty()) {
                processed_input.pop_back();
            } else if (isprint(c)) {
                processed_input += c;
            }
        }
    }

    strncpy(t->inbuf, processed_input.c_str(), MAX_INPUT_LENGTH);
    return 0;
}

void close_socket(DescriptorData *d) {
    DescriptorData *temp;

    CLOSE_SOCKET(d->descriptor);
    flush_queues(d);

    /* Forget snooping */
    if (d->snooping)
        d->snooping->snoop_by = nullptr;

    if (d->snoop_by) {
        desc_printf(d->snoop_by, "Your victim is no longer among us.\n");
        d->snoop_by->snooping = nullptr;
    }

    /* Shapechange a player back to normal form */
    if (d->character) {
        if (POSSESSED(d->character))
            do_shapechange(d->character, {"me"}, 0, 1);
    }

    /*. Kill any OLC stuff . */
    switch (d->connected) {
    case CON_OEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
    case CON_TRIGEDIT:
        cleanup_olc(d, CLEANUP_ALL);
        break;
    case CON_EXDESC:
        /* I apologize for this, but seriously, look at the editing code here */
        if (d->character)
            REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
        d->connected = CON_PLAYING;
        break;
    default:
        break;
    }

    if (EDITING(d))
        editor_cleanup(d);

    if (d->character) {
        if (IS_PLAYING(d)) {
            save_player_char(d->character);
            act("$n has lost $s link.", true, d->character, 0, 0, TO_ROOM);
            log(LogSeverity::Stat, std::max<int>(LVL_IMMORT, GET_INVIS_LEV(d->character)), "Closing link to: {}.",
                GET_NAME(d->character));
            d->character->desc = nullptr;
        } else {
            if (!GET_NAME(d->character).empty())
                log(LogSeverity::Stat, std::max<int>(LVL_IMMORT, GET_INVIS_LEV(d->character)), "Losing player: {}.",
                    GET_NAME(d->character));
            /*else
               log(LogSeverity::Stat, LVL_IMMORT, "Losing player: <null>.");
               d->character->desc = NULL;
               free_char(d->character);
             */
        }
    } else
        log(LogSeverity::Info, LVL_IMMORT, "Losing descriptor without char.");

    /* JE 2/22/95 -- part of my unending quest to make switch stable */
    if (d->original && d->original->desc)
        d->original->desc = nullptr;

    REMOVE_FROM_LIST(d, descriptor_list, next);

    if (d->page_outbuf)
        free(d->page_outbuf);

    free(d);
}

void check_idle_passwords(void) {
    DescriptorData *d, *next_d;

    for (d = descriptor_list; d; d = next_d) {
        next_d = d->next;
        if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME && STATE(d) != CON_MENU &&
            STATE(d) != CON_ISPELL_BOOT) {
            continue;
        }
        if (!d->idle_tics) {
            d->idle_tics++;
            continue;
        } else {
            if (d->character && STATE(d) == CON_MENU)
                save_player_char(d->character);
            echo_on(d);
            desc_printf(d, "\nTimed out... goodbye.\n");
            STATE(d) = CON_CLOSE;
        }
    }
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s) {
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(s, F_SETFL, flags) < 0) {
        perror("Fatal error executing nonblock (comm.c)");
        exit(1);
    }
}

/* ******************************************************************
 *  signal-handling functions (formerly signals.c)                   *
 ****************************************************************** */

RETSIGTYPE checkpointing(int signo) {
    if (!tics) {
        log("SYSERR: CHECKPOINT shutdown: tics not updated");
        abort();
    } else
        tics = 0;
}

RETSIGTYPE dump_core(int signo) {
    log(LogSeverity::Info, LVL_IMMORT, "Received SIGUSR1 - dumping core.");
    drop_core(nullptr, "usrsig");
}

RETSIGTYPE unrestrict_game(int signo) {
    extern struct BanListElement *ban_list;

    log(LogSeverity::Warn, LVL_IMMORT, "Received SIGUSR2 - completely unrestricting game (emergent)");
    ban_list = nullptr;
    should_restrict = 0;
    restrict_reason = RESTRICT_NONE;
}

RETSIGTYPE hupsig(int signo) {
    log("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
    circle_shutdown = true; /* added by Gurlaek 2/14/2000 */
}

void signal_setup(void) {
    itimerval itime;
    timeval interval;

    /* user signal 1: reread wizlists.  Used by autowiz system. */
    /* I'm removing the autowiz stuff. I'm leaving this here because
     * it seems to serve some vital function. It may be able to be
     * reassigned to something else now that autowiz is gone RSD 3/19/00
     */
    signal(SIGUSR1, dump_core);

    /*
     * user signal 2: unrestrict game.  Used for emergencies if you lock
     * yourself out of the MUD somehow.  (Duh...)
     */
    signal(SIGUSR2, unrestrict_game);
    signal(SIGUSR2, unrestrict_game);

    /*
     * set up the deadlock-protection so that the MUD aborts itself if it gets
     * caught in an infinite loop for more than 3 minutes.  Doesn't work with
     * OS/2.
     */
    interval.tv_sec = 180;
    interval.tv_usec = 0;
    itime.it_interval = interval;
    itime.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &itime, nullptr);
    signal(SIGVTALRM, checkpointing);

    /* just to be on the safe side: */
    signal(SIGHUP, hupsig);
    signal(SIGINT, hupsig);
    signal(SIGTERM, hupsig);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
}

// Updated variadic functions
void all_printf(std::string_view str) {
    DescriptorData *i;

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected)
            string_to_output(i, str);
}

void all_except_printf(CharData *ch, std::string_view str) {
    DescriptorData *i;
    CharData *avoid = REAL_CHAR(ch);

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && i->character != avoid)
            string_to_output(i, str);
}

void char_printf(const CharData *ch, std::string_view str) {
    if (ch && ch->desc)
        string_to_output(ch->desc, str);
}

void room_printf(int rvnum, std::string_view str) {
    CharData *i;
    RoomData *room;
    Exit *exit;
    int dir;

    if (rvnum >= 0 && rvnum <= top_of_world)
        room = &world[rvnum];
    else {
        log(LogSeverity::Error, LVL_GOD, "SYSERR: bad room rnum {} passed to room_printf", rvnum);
        return;
    }

    if (!room)
        return;

    for (i = room->people; i; i = i->next_in_room)
        if (i->desc)
            string_to_output(i->desc, str);

    /* Reflect to OBSERVATORY rooms if this room is an ARENA. */
    if (ROOM_FLAGGED(rvnum, ROOM_ARENA)) {
        for (dir = 0; dir < NUM_OF_DIRS; ++dir) {
            if ((exit = room->exits[dir]) && EXIT_NDEST(exit) != NOWHERE && EXIT_NDEST(exit) != rvnum &&
                ROOM_FLAGGED(EXIT_NDEST(exit), ROOM_OBSERVATORY)) {
                for (i = EXIT_DEST(exit)->people; i; i = i->next_in_room)
                    if (i->desc) {
                        string_to_output(i->desc, str);
                        break;
                    }
            }
        }
    }
}

void zone_printf(int zone_vnum, int skip_room, int min_stance, std::string_view str) {
    DescriptorData *i;
    int zone_rnum = real_zone(zone_vnum);

    if (zone_rnum == NOWHERE) {
        log(LogSeverity::Error, LVL_GOD, "SYSERR: bad zone vnum {} passed to zone_printf", zone_vnum);
        return;
    }

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && i->character && i->character->in_room != NOWHERE && i->character->in_room != skip_room &&
            i->character->char_specials.stance >= min_stance && world[i->character->in_room].zone == zone_rnum)
            string_to_output(i, str);
}

void callback_printf(CBP_FUNC(callback), int data, std::string_view str) {
    DescriptorData *i;

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && i->character && i->character->desc && callback(i->character, data))
            string_to_output(i, str);
}

void outdoor_printf(int zone_num, std::string_view str) {
    DescriptorData *i;

    for (i = descriptor_list; i; i = i->next)
        if (!i->connected && i->character && AWAKE(i->character) && CH_OUTSIDE(i->character) &&
            IN_ZONE_RNUM(i->character) == zone_num && STATE(i) == CON_PLAYING)
            string_to_output(i, str);
}

/* SPEECH_OK
 *
 * Call this function when a character is about to do some spammable type
 * of communication, such as gossip or tell.  It will keep track of how
 * blabby they've been lately, and may suppress their communication.
 *
 * INPUTS
 *
 *     ch - Character attempting communication
 *  quiet - Boolean value. true prevents this function from sending any
 *          feedback to the character.
 *
 * RETURN VALUES
 *
 *   true - the communication is OK
 *  false - the communication should not occur. Unless quiet=true, a message
 *          has already been sent to the character, so the caller should
 *          probably do nothing.
 */

/* How long we remember communications; therefore, if you spam a lot and get
 * squelched, this is how long you must wait to completely recover */
#define SPAM_PERIOD (double)(PASSES_PER_SEC * 30)

/* How many communications during SPAM_PERIOD is enough to get you squelched? */
/* (Actually it's fewer than this, because once you get halfway, each additional
 * communication moves you twice as fast.) */
#define SPAM_THRESHOLD 15

int speech_ok(CharData *ch, int quiet) {
    CharSpecialData *sd;
    TrigData *t;

    if (GET_LEVEL(ch) >= LVL_IMMORT)
        return true;

    if (IS_NPC(ch) && POSSESSED(ch)) {
        /* A shapechanged player */
        sd = &ch->desc->original->char_specials;
    } else {
        sd = &ch->char_specials;
    }

    /* If it's been at least SPAM_PERIOD since the last communication, there is
     * no need for calculation - any speech_rate you may have is obsolete and
     * can be forgotten. */
    if (sd->last_speech_time == 0 || global_pulse - sd->last_speech_time > SPAM_PERIOD ||
        sd->last_speech_time > global_pulse /* rollover, in case the mud runs insanely long? */
    ) {
        sd->speech_rate = 1;
        sd->last_speech_time = global_pulse;
        return true;
    }

    /* If the char is executing a trigger, it's all good */
    if (SCRIPT(ch))
        for (t = SCRIPT(ch)->trig_list; t; t = t->next)
            if (t->running)
                return true;

    /* The following calculation considers the amount of time since the last
     * communication and uses it to reduce the speech rate. Then it adds 1 for
     * this communication. */
    sd->speech_rate = 1 + sd->speech_rate * (SPAM_PERIOD - global_pulse + sd->last_speech_time) / SPAM_PERIOD;
    sd->last_speech_time = global_pulse;

    if (sd->speech_rate > SPAM_THRESHOLD) {
        /* Try to blab with laryngitis, and the recovery period starts over again.
         */
        sd->speech_rate = SPAM_THRESHOLD * 1.5;
        if (!quiet)
            char_printf(ch, "&5&bYour mouth refuses to move!&0\n");
        return false;

    } else if (sd->speech_rate > SPAM_THRESHOLD / 2) {
        /* You're in the sore-throat zone! */

        /* Add a random number to discourage brinkmanship. If you keep it up
         * even with a minor sore throat, you don't know what it will take
         * to bring on full-blown laryngitis (which occurs without warning). */
        sd->speech_rate += random_number(1, 4);
        if (sd->speech_rate > SPAM_THRESHOLD) {
            sd->speech_rate = SPAM_THRESHOLD * 1.5;
            if (!quiet)
                char_printf(ch, "&6&bYou feel laryngitis coming on!&0\n");
        } else {
            if (!quiet)
                char_printf(ch, "&5Your throat feels a little sore.&0\n");
        }
    }

    return true;
}

void speech_report(CharData *ch, CharData *tch) {
    CharSpecialData *sd;

    if (GET_LEVEL(tch) >= LVL_IMMORT)
        return;

    if (IS_NPC(tch) && POSSESSED(tch)) {
        /* A shapechanged player */
        sd = &tch->desc->original->char_specials;
    } else {
        sd = &tch->char_specials;
    }

    if (sd->last_speech_time == 0 || global_pulse - sd->last_speech_time > SPAM_PERIOD ||
        sd->last_speech_time > global_pulse /* rollover, in case the mud runs insanely long? */
    ) {
        return;
    }

    sd->speech_rate = sd->speech_rate * (SPAM_PERIOD - global_pulse + sd->last_speech_time) / SPAM_PERIOD;
    sd->last_speech_time = global_pulse;

    if (sd->speech_rate > SPAM_THRESHOLD)
        char_printf(ch, "&5&b{} {} an acute case of laryngitis.&0\n", ch == tch ? "You" : GET_NAME(tch),
                    ch == tch ? "have" : "has");
    else if (sd->speech_rate > SPAM_THRESHOLD / 2)
        char_printf(ch, "&5{}{} throat feels a little sore.&0\n", ch == tch ? "Your" : GET_NAME(tch),
                    ch == tch ? "" : "'s");
}

void cure_laryngitis(CharData *ch) {
    CharSpecialData *sd;

    if (IS_NPC(ch) && POSSESSED(ch)) {
        /* A shapechanged player */
        sd = &ch->desc->original->char_specials;
    } else {
        sd = &ch->char_specials;
    }
    sd->speech_rate = 0;
}

/* higher-level communication: the act() function */
const std::string_view ACTNULL = "<NULL>";
std::string format_act(std::string_view format, const CharData *ch, ActArg obj, ActArg vict_obj, const CharData *to) {
    std::string rtn;
    bool code = false;

    for (auto c : format) {
        if (!std::exchange(code, false)) {
            if (c == '$') {
                code = true;
            } else {
                rtn += c;
            }
            continue;
        }
        switch (c) {
        case 'n': // Show name (or 'someone' if you can't see them) of ch
            rtn += PERS(ch, to);
            break;
        case 'N': // Show name (or 'someone' if you can't see them) of vict_obj
            if (auto victim = std::get_if<CharData *>(&vict_obj)) {
                rtn += PERS(*victim, to);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'm':
            rtn += HMHR(ch);
            break;
        case 'M':
            if (auto victim = std::get_if<CharData *>(&vict_obj)) {
                rtn += HMHR(*victim);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 's':
            rtn += HSHR(ch);
            break;
        case 'S':
            if (auto victim = std::get_if<CharData *>(&vict_obj)) {
                rtn += HSHR(*victim);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'D':
            if (auto victim = std::get_if<CharData *>(&vict_obj)) {
                rtn += without_article(HSSH(*victim));
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'e':
            rtn += HSSH(ch);
            break;
        case 'E':
            if (auto victim = std::get_if<CharData *>(&vict_obj)) {
                rtn += HSSH(*victim);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'o':
            if (auto target_object = std::get_if<ObjData *>(&obj)) {
                rtn += OBJN(*target_object, to);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'O':
            if (auto victim_object = std::get_if<ObjData *>(&vict_obj)) {
                rtn += OBJN(*victim_object, to);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'p':
            if (auto target_object = std::get_if<ObjData *>(&obj)) {
                rtn += OBJS(*target_object, to);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'P':
            if (auto victim_object = std::get_if<ObjData *>(&vict_obj)) {
                rtn += OBJS(*victim_object, to);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'a':
            if (auto target_object = std::get_if<ObjData *>(&obj)) {
                rtn += SANA(*target_object);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 'A':
            if (auto victim_object = std::get_if<ObjData *>(&vict_obj)) {
                rtn += SANA(*victim_object);
            } else {
                rtn += ACTNULL;
            }
            break;
        case 't':
            if (auto obj_as_string_view = std::get_if<std::string_view>(&obj)) {
                rtn += *obj_as_string_view;
            } else {
                log("SYSERR: format_act: $t used with no obj");
                rtn += ACTNULL;
            }
            break;
        case 'T':
            if (auto vict_obj_as_string_view = std::get_if<std::string_view>(&vict_obj)) {
                rtn += *vict_obj_as_string_view;
            } else {
                log("SYSERR: format_act: $T used with no vict_obj");
                rtn += ACTNULL;
            }
            break;
        case 'f':
            if (auto obj_as_string_view_2 = std::get_if<std::string_view>(&obj)) {
                rtn += fname(*obj_as_string_view_2);
            } else {
                log("SYSERR: format_act: $f used with no obj");
                rtn += ACTNULL;
            }
            break;
        case 'F':
            if (auto vict_obj_as_string_view2 = std::get_if<std::string_view>(&vict_obj)) {
                rtn += fname(*vict_obj_as_string_view2);
            } else {
                log("SYSERR: format_act: $F used with no vict_obj");
                rtn += ACTNULL;
            }
            break;
        case 'i':
            if (auto obj_as_int = std::get_if<int>(&obj)) {
                rtn += std::to_string(*obj_as_int);
            } else {
                log("SYSERR: format_act: $i used with no obj");
                rtn += ACTNULL;
            }
            break;
        case 'I':
            if (auto vict_obj_as_int = std::get_if<int>(&vict_obj))
                rtn += std::to_string(*vict_obj_as_int);
            else {
                log("SYSERR: format_act: $I used with no vict_obj");
                rtn += ACTNULL;
            }
            break;
        case '$':
            rtn += "$";
            break;
        default:
            log("SYSERR: Illegal $-code to act(): {} ({})", c, format);
            break;
        }
    }

    // act_mtrigger(to, rtn, ch, victim, victim_object, target_object, target_string, target_string2);
    rtn += "\n";
    return capitalize_first(rtn);
}

/* The "act" action interpreter */
void act(std::string_view str, int hide_invisible, const CharData *ch, ActArg obj, ActArg vict_obj, int type) {
    char lbuf[MAX_STRING_LENGTH];
    CharData *to = nullptr, *victim = nullptr;
    ObjData *target_object = nullptr, *victim_object = nullptr;
    int sleep, olc, in_room, i, to_victroom;

    if (str.empty() || !ch)
        return;

    if (!(dg_act_check = !IS_SET(type, DG_NO_TRIG)))
        REMOVE_BIT(type, DG_NO_TRIG);

    /*
     * Warning: the following TO_SLEEP code is a hack.
     *
     * I wanted to be able to tell act to deliver a message regardless of sleep
     * without adding an additional argument.  TO_SLEEP is 128 (a single bit
     * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
     * command.  It's not legal to combine TO_x's with each other otherwise.
     *
     * The TO_OLC flag is also a hack.
     */

    if ((olc = IS_SET(type, TO_OLC)))
        REMOVE_BIT(type, TO_OLC);

    if ((sleep = IS_SET(type, TO_SLEEP)))
        REMOVE_BIT(type, TO_SLEEP);

    if ((to_victroom = IS_SET(type, TO_VICTROOM)))
        REMOVE_BIT(type, TO_VICTROOM);

    if (type == TO_CHAR) {
        if (ch && ((MOB_PERFORMS_SCRIPTS(ch) && SCRIPT_CHECK(ch, MTRIG_ACT)) || SENDOK(ch))) {
            auto formatted = format_act(str, ch, obj, vict_obj, ch);
            char_printf(ch, formatted);
        }

        return;
    }

    if (type == TO_VICT) {
        if (!std::holds_alternative<CharData *>(vict_obj)) {
            log("SYSERR: act: TO_VICT: no victim sent.  str: {}", str);
            return;
        }
        to = std::get<CharData *>(vict_obj);
        if (to && ((MOB_PERFORMS_SCRIPTS(to) && SCRIPT_CHECK(to, MTRIG_ACT)) || SENDOK(to)) &&
            !(hide_invisible && ch && !CAN_SEE(to, ch))) {
            auto formatted = format_act(str, ch, obj, vict_obj, to);
            char_printf(to, formatted);
        }

        return;
    }

    /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

    if (ch) {
        if ((ch->in_room != NOWHERE)) {
            in_room = ch->in_room;
        } else {
            log("SYSERR:comm.c:act(): ch->in_room = -1, {}", GET_NAME(ch));
            return;
        }
    } else if (std::holds_alternative<ObjData *>(obj)) {
        target_object = std::get<ObjData *>(obj);
        if ((target_object->in_room != NOWHERE)) {
            if ((target_object->in_room) != 0) {
                in_room = target_object->in_room;
            } else {
                log("SYSERR:comm.c:act(): obj->in_room = 0, {}, {}", target_object->name, GET_OBJ_VNUM(target_object));
                return;
            }
        } else { /*if here then NO ch and obj->in_room = NOWHERE */
            log("SYSERR:comm.c:act(): obj->in_room = -1, {}, {}", target_object->name, GET_OBJ_VNUM(target_object));
            return;
        }
    } else {
        /* no obj or ch */
        log("SYSERR:comm.c:act(): NULL char and object pointer");
        return;
    }

    if (std::holds_alternative<CharData *>(vict_obj)) {
        victim = std::get<CharData *>(vict_obj);
    }

    if (to_victroom) {
        if (victim) {
            in_room = victim->in_room;
            if (in_room == NOWHERE) {
                log("SYSERR: act(): TO_VICTROOM option used but vict is in NOWHERE");
                return;
            }
        } else {
            log("SYSERR: act(): TO_VICTROOM option used but vict is NULL");
            return;
        }
    }
    for (to = world[in_room].people; to; to = to->next_in_room)
        if (((MOB_PERFORMS_SCRIPTS(to) && SCRIPT_CHECK(to, MTRIG_ACT)) || SENDOK(to)) &&
            !(hide_invisible && ch && !CAN_SEE(to, ch)) && (to != ch) && (type == TO_ROOM || (to != victim))) {
            auto formatted = format_act(str, ch, obj, vict_obj, to);
            char_printf(to, formatted);
        }
    /*
     * Reflect TO_ROOM and TO_NOTVICT calls that occur in ARENA rooms
     * into OBSERVATORY rooms, allowing players standing in observatories
     * to watch arena battles safely.
     */
    if (ROOM_FLAGGED(in_room, ROOM_ARENA))
        for (i = 0; i < NUM_OF_DIRS; ++i)
            if (world[in_room].exits[i] && world[in_room].exits[i]->to_room != NOWHERE &&
                world[in_room].exits[i]->to_room != in_room &&
                ROOM_FLAGGED(world[in_room].exits[i]->to_room, ROOM_OBSERVATORY))
                for (to = world[world[in_room].exits[i]->to_room].people; to; to = to->next_in_room)
                    if (SENDOK(to) && !(hide_invisible && ch && !CAN_SEE(to, ch)) && (to != ch) &&
                        (type == TO_ROOM || (to != victim))) {
                        char_printf(to, "&4<&0{}&0&4>&0 ", world[in_room].name);
                        auto formatted = format_act(str, ch, obj, vict_obj, to);
                        char_printf(to, formatted);
                    }
}

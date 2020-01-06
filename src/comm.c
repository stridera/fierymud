/***************************************************************************
 * $Id: comm.c,v 1.180 2010/06/05 14:56:27 mud Exp $
 ***************************************************************************/
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

#define __COMM_C__
/* if yer really hard core....
#define __STRICT_ANSI__
*/
#include "conf.h"
#include "sysdep.h"


#ifdef CIRCLE_WINDOWS                /* Includes for Win32 */
#include <direct.h>
#include <mmsystem.h>
#else                                /* Includes for UNIX */
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "screen.h"
#include "olc.h"
#include "events.h"
#include "dg_scripts.h"
#include "weather.h"
#include "casting.h"
#include "skills.h"
#include "constants.h"
#include "math.h"
#include "board.h"
#include "clan.h"
#include "mail.h"
#include "players.h"
#include "pfiles.h"
#include "races.h"
#include "exits.h"
#include "cooldowns.h"
#include "fight.h"
#include "commands.h"
#include "modify.h"
#include "act.h"
#include "editor.h"
#include "directions.h"
#include "effects.h"
#include "rules.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define YES        1
#define NO        0

#define GMCP 201

char ga_string[] = { (char) IAC, (char) GA, (char) 0 };
char gmcp_start_data[] = { (char) IAC, (char) SB, (char) GMCP, (char) 0 };
char gmcp_end_data[] = { (char) IAC, (char) SE, (char) 0};

/* externs */
extern int should_restrict;
extern int restrict_reason;
extern int mini_mud;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int MAX_PLAYERS;
extern int MAX_DESCRIPTORS_AVAILABLE;
extern const char mudlet_client_version[];
extern const char mudlet_client_url[];
extern const char mudlet_map_url[];

extern const char *save_info_msg[];        /* In olc.c */
ACMD(do_shapechange);

extern int num_hotboots;
extern int reboot_auto;
extern long reboot_pulse;
extern int reboot_warning_minutes;
extern int last_reboot_warning;
extern int reboot_warning;

/* local globals */
static char comm_buf[MAX_STRING_LENGTH];
struct descriptor_data *descriptor_list = NULL;                /* master desc list */
struct txt_block *bufpool = 0;        /* pool of large output buffers */
int buf_largecount = 0;                /* # of large buffers which exist */
int buf_overflows = 0;                /* # of overflows of output */
int buf_switches = 0;                /* # of switches from small to large buf */
int circle_shutdown = 0;        /* clean shutdown */
int circle_reboot = 0;                /* reboot the game after a shutdown */
int no_specials = 0;                /* Suppress ass. of special routines */
int max_players = 0;                /* max descriptors available */
int tics = 0;                        /* for extern checkpointing */
int scheck = 0;                        /* for syntax checking mode */
int dg_act_check;               /* toggle for act_trigger */
unsigned long global_pulse = 0; /* number of pulses since game start */
extern int nameserver_is_slow;        /* see config.c */
struct timeval null_time;        /* zero-valued time structure */
unsigned long pulse = 0;        /* number of pulses since game started */
int gossip_channel_active = 1;        /* Flag for turning on or off gossip for the whole MUD */

ush_int port;
socket_t mother_desc;

/* functions in this file */
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_max_players(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void close_socket(struct descriptor_data *d);
struct timeval timediff(struct timeval a, struct timeval b);
struct timeval timeadd(struct timeval a, struct timeval b);
void flush_queues(struct descriptor_data *d);
void nonblock(socket_t s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
void send_gmcp_char_info(struct descriptor_data *d);
void make_prompt(struct descriptor_data *d);
void check_idle_passwords(void);
void heartbeat(int pulse);
char *new_txt;
void init_descriptor(struct descriptor_data *newd, int desc);
int enter_player_game(struct descriptor_data *d);
void free_bufpools(void);


/* extern fcnts */
void boot_db(void);
void boot_world(void);
void zone_update(void);
void effect_update(void);        /* In spells.c */
void point_update(void);        /* In limits.c */
void mobile_activity(void);
void mobile_spec_activity(void);
void string_add(struct descriptor_data *d, char *str);
void perform_mob_violence(void);
int isbanned(char *hostname);
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


/**********************************************************************
 *  main game loop and related stuff                                    *
 ********************************************************************* */

/* Windows doesn't have gettimeofday, so we'll simulate it. */
#ifdef CIRCLE_WINDOWS

void gettimeofday(struct timeval *t, struct timezone *dummy)
{
  DWORD millisec = GetTickCount();

  t->tv_sec = (int) (millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}

#endif


int main(int argc, char **argv)
{
  int pos = 1;
  char *dir;

#ifdef MEMORY_DEBUG
  zmalloc_init();
#endif

  port = DFLT_PORT;
  dir = DFLT_DIR;

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'd':
      if (*(argv[pos] + 2))
        dir = argv[pos] + 2;
      else if (++pos < argc)
        dir = argv[pos];
      else {
        log("Directory arg expected after option -d.");
        exit(1);
      }
      break;
    case 'H': /* -H<socket number> recover from hotbot, this is the control socket */
      num_hotboots = 1;
      mother_desc = atoi(argv[pos]+2);
      break;
    case 'm':
      mini_mud = 1;
      log("Running in minimized mode.");
      break;
    case 'c':
      scheck = 1;
      log("Syntax check mode enabled.");
      break;
    case 'q':
      log("Quick boot mode.");
      break;
    case 'r':
      should_restrict = 1;
      restrict_reason = RESTRICT_ARGUMENT;
      log("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      log("Suppressing assignment of special routines.");
      break;
    default:
      log("SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      fprintf(stderr, "Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      fprintf(stderr, "Illegal port number.\n");
      exit(1);
    }
  }
#ifdef CIRCLE_WINDOWS
  if (_chdir(dir) < 0) {
#else
  if (chdir(dir) < 0) {
#endif
    perror("Fatal error changing to data directory");
    exit(1);
  }
  log("Using %s as data directory.", dir);

  log("Initializing runtime game constants.");
  init_flagvectors();
  init_rules();
  init_colors();
  init_races();
  init_classes();
  init_objtypes();
  init_exp_table();

  if (scheck) {
    boot_world();
  } else {
    log("Running game on port %d.", port);
    init_game(port);
  }

  log("Clearing game world.");
  destroy_db();

#ifdef MEMORY_DEBUG
  if (!scheck) {
    log("Clearing other memory.");
    free_bufpools();                /* comm.c */
    free_player_index();        /* db.c */
    free_messages();                /* fight.c */
    free_text_files();                /* db.c */
    board_cleanup();                  /* board.c */
    free(cmd_sort_info);
    free_social_messages();        /* act.social.c */
    free_help_table();                /* db.c */
    free_invalid_list();        /* ban.c */
    free_save_list();                /* olc.c */
    free_clans();                /* clan.c */
    free(ships);
    free_mail_index();                /* mail.c */
  }

  zmalloc_check();
#endif

  return 0;
}

void hotboot_recover()
{
  struct descriptor_data *d;
  FILE *fp;
  char host[1024];
  int desc, player_i;
  bool fOld;
  char name[MAX_INPUT_LENGTH];
  int count;
  char *p;

  extern time_t *boot_time;

  log("Hotboot recovery initiated.");

  fp = fopen(HOTBOOT_FILE, "r");
  /* There are some descriptors open which will hang forever then? */
  if (!fp) {
    perror("hotboot_recover:fopen");
    log("Hotboot file not found.  Exiting.\r\n");
    exit(1);
  }

  /* In case something crashes - doesn't prevent reading */
  unlink(HOTBOOT_FILE);

  /* read boot_time - first line in file */
  if (boot_time)
    free(boot_time);
  fgets(p = buf, MAX_STRING_LENGTH, fp);
  p = any_one_arg(p, name);
  num_hotboots = atoi(name); /* actually the total number of boots */
  CREATE(boot_time, time_t, num_hotboots + 1);
  for (count = 0; count < num_hotboots; ++count) {
    p = any_one_arg(p, name);
    boot_time[count] = atol(name);
  }
  boot_time[num_hotboots] = time(0);

  /* More than 1000 iterations means something is pretty wrong. */
  for (count = 0; count <= 1000; ++count) {
    fOld = TRUE;
    fscanf(fp, "%d %s %s\n", &desc, name, host);
    if (desc == -1)
      break;

    /* Write something, and check if it goes error-free */
    if (write_to_descriptor(desc, "\r\nRestoring from hotboot...\r\n") < 0) {
      close(desc); /* nope */
      continue;
    }

    /* Create a new descriptor */
    CREATE(d, struct descriptor_data, 1);
    memset((char *) d, 0, sizeof(struct descriptor_data));
    init_descriptor(d, desc); /* set up various stuff */

    strcpy(d->host, host);
    d->next = descriptor_list;
    descriptor_list = d;

    d->connected = CON_CLOSE;

    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;

    if ((player_i = load_player(name, d->character)) >= 0) {
      if (!PLR_FLAGGED(d->character, PLR_DELETED)) {
        REMOVE_FLAG(PLR_FLAGS(d->character), PLR_WRITING);
        REMOVE_FLAG(PLR_FLAGS(d->character), PLR_MAILING);
      }
      else
        fOld = FALSE;
    }
    else
      fOld = FALSE;

    if (!fOld) {
      write_to_descriptor(desc, "\r\nSomehow, your character was lost in the hotboot.  Sorry.\r\n");
      close_socket(d);
    }
    else {
      sprintf(buf, "\r\n%sHotboot recovery complete.%s\r\n",
              CLR(d->character, HGRN), CLR(d->character, ANRM));
      write_to_descriptor(desc, buf);
      enter_player_game(d);
      d->connected = CON_PLAYING;
      look_at_room(d->character, FALSE);
    }
  }

  fclose(fp);
}

/* Init sockets, run game, and cleanup sockets */
void init_game(int port)
{
  extern long reboot_pulse;
  extern int reboot_hours_base;
  extern int reboot_hours_deviation;

  srandom(time(0));

  log("Finding player limit.");
  max_players = get_max_players();

  if (num_hotboots == 0) {
    log("Opening mother connection.");
    mother_desc = init_socket(port);
  }

  event_init();

  boot_db();

#ifndef CIRCLE_WINDOWS
  log("Signal trapping.");
  signal_setup();
#endif

  /* Decide when to reboot */
  reboot_pulse = 3600 * PASSES_PER_SEC *
          (reboot_hours_base - reboot_hours_deviation) +
          number(0, 3600 * PASSES_PER_SEC * 2 * reboot_hours_deviation);

  if (num_hotboots > 0)
    hotboot_recover();

  log("Entering game loop.");

#ifndef CIRCLE_WINDOWS
  ispell_init();
#endif

  game_loop(mother_desc);

  auto_save_all();

#ifndef CIRCLE_WINDOWS
  ispell_done();
#endif

  log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  CLOSE_SOCKET(mother_desc);

  if (circle_reboot != 2 && olc_save_list) { /* Don't save zones. */
    struct olc_save_info *entry, *next_entry;
    for (entry = olc_save_list; entry; entry = next_entry) {
      next_entry = entry->next;
      if (entry->type < 0 || entry->type > 4) {
        log("OLC: Illegal save type %d!", entry->type);
      } else if (entry->zone < 0) {
        log("OLC: Illegal save zone %d!", entry->zone);
      } else {
        log("OLC: Reboot saving %s for zone %d.",
            save_info_msg[(int)entry->type], entry->zone);
        switch (entry->type) {
          case OLC_SAVE_ROOM: redit_save_to_disk(entry->zone); break;
          case OLC_SAVE_OBJ:  oedit_save_to_disk(entry->zone); break;
          case OLC_SAVE_MOB:  medit_save_to_disk(entry->zone); break;
          case OLC_SAVE_ZONE: zedit_save_to_disk(entry->zone); break;
          case OLC_SAVE_SHOP: sedit_save_to_disk(entry->zone); break;
          default:      log("Unexpected olc_save_list->type"); break;
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
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

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

#ifdef CIRCLE_WINDOWS
  {
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      log("WinSock not available!\n");
      exit(1);
    }
    if ((wsaData.iMaxSockets - 4) < max_players) {
      max_players = wsaData.iMaxSockets - 4;
    }
    log("Max players set to %d", max_players);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      fprintf(stderr, "Error opening network connection: Winsock err #%d\n", WSAGetLastError());
      exit(1);
    }
  }
#else
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error creating socket");
    exit(1);
  }
#endif                                /* CIRCLE_WINDOWS */

#if defined(SO_SNDBUF)
  opt = LARGE_BUFSIZE + GARBAGE_SPACE;
  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt SNDBUF");
    exit(1);
  }
#endif

#if defined(SO_REUSEADDR)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEADDR");
    exit(1);
  }
#endif

#if defined(SO_LINGER)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0) {
      perror("setsockopt LINGER");
      exit(1);
    }
  }
#endif

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("bind");
    CLOSE_SOCKET(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return s;
}


int get_max_players(void)
{
  int max_descs = 0;
  char *method;

#if defined(CIRCLE_OS2) || defined(CIRCLE_WINDOWS)
  return MAX_PLAYERS;
#else

  /*
   * First, we'll try using getrlimit/setrlimit.  This will probably work
   * on most systems.
   */
#if defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
  {
    struct rlimit limit;

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
      max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
#else
    max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
  }

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  method = "OPEN_MAX";
  max_descs = OPEN_MAX;         /* Uh oh.. rlimit didn't work, but we
                                 * have OPEN_MAX */
#elif defined (POSIX)
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
  max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    log("Non-positive max player limit!  (Set at %d using %s).",
            max_descs, method);
    exit(1);
  }
  log("Setting player limit to %d using %s.", max_descs, method);
  return max_descs;
#endif                                /* WINDOWS or OS2 */
}

void reboot_mud_prep()
{
  /*do global save*/
  auto_save_all();
  House_save_all();
  all_printf("Rebooting.. come back in a minute or two.\r\n"
             "           &1&b** ****** ****&0\r\n"
             "         &1&b**&0 &3&b***     *****&0  &1&b**&0\r\n"
             "       &1&b**&0 &3&b**      &1&b*&0     &3&b***&0  &1&b*&0\r\n"
             "       &1&b*&0    &3&b** **   *   *  *&0 &1&b**&0\r\n"
             "      &1&b*&0  &3&b** * *&0          &1&b*&0     &1&b*&0\r\n"
             "      &1&b*&0  &3&b*&0    &1&b**&0            &3&b* *&0 &1&b*&0\r\n"
             "     &1&b*&0 &3&b* &1&b** *&0     &3&b*   ******&0  &1&b*&0\r\n"
             "      &1&b*&0   &3&b* &1&b* **&0  &3&b***&0     &1&b*&0  &3&b*&0 &1&b*&0\r\n");
  all_printf("        &1&b*&0  &3&b*  *&0 &1&b**********&0  &3&b***&0 &1&b*&0\r\n"
             "         &1&b*****&0   &3&b*     *   * *&0 &1&b*&0\r\n"
             "                &1&b*&0   &3&b*&0 &1&b*&0\r\n"
             "               &1&b*&0  &3&b* *&0  &1&b*&0\r\n"
             "              &1&b*&0  &3&b*  **&0  &1&b*&0\r\n"
             "              &1&b*&0 &3&b**   *&0 &1&b*&0\r\n"
             "                &1&b*&0 &3&b*&0 &1&b*&0\r\n"
             "                &1&b*&0 &3&b*&0  &1&b**&0\r\n"
             "               &1&b**&0     &1&b****&0\r\n"
             "              &1&b***&0  &3&b* *&0    &1&b****&0\r\n");
  touch("../.fastboot");
}

/* This will generally be called upon login, to let people know
 * immediately if a reboot is coming up.  It could be especially useful
 * to a god, if they log in and see "15 seconds to reboot", because
 * it would give them time to abort the reboot if they so desire. */
void personal_reboot_warning(struct char_data *ch)
{
   int minutes_till, seconds_till;

   if ((reboot_pulse - global_pulse) / (PASSES_PER_SEC * 60) < reboot_warning_minutes) {
      minutes_till = (reboot_pulse - global_pulse - 1) / (PASSES_PER_SEC * 60) + 1;
      if (minutes_till == 1) {
         seconds_till = (reboot_pulse - global_pulse) / PASSES_PER_SEC;
         cprintf(ch, "\r\n\007&4&b***&0 &7&b%d second%s to reboot&0 &4&b***&0\r\n",
               seconds_till, seconds_till == 1 ? "" : "s");
      } else {
         cprintf(ch, "\r\n\007&1&b*&3&bATTENTION&1&b*&0  The mud will &7&bREBOOT&0 in &6&b%d minute%s&0  &1&b*&3&bATTENTION&1&b*&0\r\n",
               minutes_till, minutes_till == 1 ? "" : "s");
      }
   }
}

void rebootwarning(int minutes)
{
   all_printf("&1&b*&3&bATTENTION&1&b*&0  The mud will &7&bREBOOT&0 in &6&b%d minute%s&0  &1&b*&3&bATTENTION&1&b*&0\r\n",
         minutes, minutes == 1 ? "" : "s");
}

void cancel_auto_reboot(int postponed)
{
   if (!reboot_warning) return;

   reboot_warning = 0;
   if (postponed)
      all_printf("&6*** Automatic Reboot Postponed ***&0\r\n");
   else
      all_printf("&6&b*** Automatic Reboot Cancelled ***&0\r\n");

   if (should_restrict) {
      if (restrict_reason == RESTRICT_NONE || restrict_reason == RESTRICT_AUTOBOOT) {
         should_restrict = 0;
         restrict_reason = RESTRICT_NONE;
         all_printf("*** Mortal logins reenabled ***\r\n");
         log("Login restriction removed.");
      }
   }
}

void check_auto_rebooting()
{
   int minutes_till, seconds_till;
   static int reboot_prepped = 0;

   if (!reboot_auto) return;

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
          * Or, if someone suddenly sets the mud to reboot in 1 minute ("autoboot :1")
          * it will disable mortal logins then.
          *
          * Also, if a god reenables mortal logins ("wizlock 0"), this won't
          * override that. This is intentional. */
         if ((minutes_till == 2 || (minutes_till < 2 && !reboot_warning)) && should_restrict < LVL_IMMORT) {
            log("Mortal logins prevented due to imminent automatic reboot.");
            should_restrict = LVL_IMMORT;
            restrict_reason = RESTRICT_AUTOBOOT;
            all_printf("*** No more mortal logins ***\r\n");
         }
         reboot_warning = 1;
         last_reboot_warning = minutes_till;
      } else if (minutes_till == 1) {
         /* Additional warnings during the last minute */
         seconds_till = (reboot_pulse - global_pulse) / PASSES_PER_SEC;
         if (seconds_till == 30 || seconds_till == 10 || seconds_till == 5) {
            all_printf("&4&b*&0 &7&b%d second%s to reboot&0 &4&b*&0\r\n",
                  seconds_till, seconds_till == 1 ? "" : "s");
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
void game_loop(int mother_desc)
{
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, before_sleep, opt_time, process_time, now, timeout;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int missed_pulses, maxdesc, aliased;

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  FD_ZERO(&null_set);

  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    /* Sleep if we don't have any connections and are not about to reboot */
    if (descriptor_list == NULL && !reboot_warning) {
      log("No connections.  Going to sleep.");
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
        if (errno == EINTR)
          log("Waking up to process signal.");
        else
          perror("Select coma");
      } else
        log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);

    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
#ifndef CIRCLE_WINDOWS
      if (d->descriptor > maxdesc)
        maxdesc = d->descriptor;
#endif
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

    gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
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
    gettimeofday(&now, (struct timezone *) 0);
    timeout = timediff(last_time, now);

    /* go to sleep */
    do {
#ifdef CIRCLE_WINDOWS
      Sleep(timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#else
      if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0) {
        if (errno != EINTR) {
          perror("Select sleep");
          exit(1);
        }
      }
#endif /* CIRCLE_WINDOWS */
      gettimeofday(&now, (struct timezone *) 0);
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
            act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
          }
        }
        d->wait = 1;
        d->prompt_mode = 1;

        /* reversed these top 2 if checks so that you can use the page_string */
        /* function in the editor */
        if (PAGING(d))        /* reading something w/ pager     */
          get_paging_input(d, comm);
        else if (EDITING(d))
          editor_interpreter(d, comm);
        else if (d->str)                /* writing boards, mail, etc.     */
          string_add(d, comm);
        else if (d->connected != CON_PLAYING)        /* in menus, etc. */
          nanny(d, comm);
        else {                        /* else: we're playing normally */
          if (aliased)                /* to prevent recursive aliases */
            d->prompt_mode = 0;
          else if (perform_alias(d, comm))                /* run it through aliasing system */
            get_from_q(&d->input, comm, &aliased);
          dprintf(d, "\r\n");
          command_interpreter(d->character, comm);        /* send it to interpreter */
        }
      }
      event_process();
    }

    /* send queued output out to the operating system (ultimately to user) */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &output_set) && *(d->output)) {
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


void heartbeat(int pulse)
{
  static int mins_since_autosave = 0;

  global_pulse++;

  if (!(pulse % PULSE_DG_SCRIPT))
    script_trigger_check();

  event_process();

  if (!(pulse % PULSE_ZONE))
    zone_update();

  if (!(pulse % (15 * PASSES_PER_SEC)))                /* 15 seconds */
    check_idle_passwords();

  if (!(pulse % PULSE_MOBILE))
    mobile_activity();

  if (!(pulse % PULSE_VIOLENCE)) {
    perform_violence();
    mobile_spec_activity();
  }

  if (!(pulse % (PULSE_VIOLENCE/2))) {
     /* Every other combat round, NPCs in battle attempt skill and
      * spell-based attacks. */
    perform_mob_violence();
  }

  if (!(pulse % PASSES_PER_SEC)) {
    if (reboot_auto)
      check_auto_rebooting();
  }

  if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
    update_weather(pulse);
    increment_game_time(); /* Increment game time by an hour. */
    effect_update();
    point_update();
    check_time_triggers();
  }

  if (!(pulse % PULSE_AUTOSAVE)) {        /* 1 minute */
    if (++mins_since_autosave >= 1) {
      mins_since_autosave = 0;
      auto_save_all();
      House_save_all();
    }
  }
  /* Commenting entire 5 minute check section because there would be
     nothing to run once this since function was commented - RSD  */
  /* if (!(pulse % (5 * 60 * PASSES_PER_SEC))) {  */     /* 5 minutes */
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
struct timeval timediff(struct timeval a, struct timeval b)
{
  struct timeval rslt;

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
  } else {                        /* a->tv_sec > b->tv_sec */
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
struct timeval timeadd(struct timeval a, struct timeval b)
{
  struct timeval rslt;

  rslt.tv_sec = a.tv_sec + b.tv_sec;
  rslt.tv_usec = a.tv_usec + b.tv_usec;

  while (rslt.tv_usec >= 1000000) {
    rslt.tv_usec -= 1000000;
    rslt.tv_sec++;
  }

  return rslt;
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (!d->connected)
      sockets_playing++;
  }

  log("nusage: %-3d sockets connected, %-3d sockets playing",
          sockets_connected, sockets_playing);

#ifdef RUSAGE
  {
    struct rusage ru;

    getrusage(0, &ru);
    log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
            ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
  }
#endif
}

void offer_gmcp(struct descriptor_data *d)
{
    char offer_gmcp[] = {
        (char) IAC,
        (char) WILL,
        (char) GMCP,
        (char) 0
    };

    write_to_descriptor(d->descriptor, offer_gmcp);
}

void offer_gmcp_services(struct descriptor_data *d)
{
    static char response[MAX_STRING_LENGTH];
    sprintf(response,
            "%sClient.GUI {" \
            "\"version\":\"%s\"," \
            "\"url\":\"%s\"" \
            "}%s" \
            "%sClient.Map {" \
            "\"url\":\"%s\"" \
            "}%s",
            gmcp_start_data,
            mudlet_client_version,
            mudlet_client_url,
            gmcp_end_data,
            gmcp_start_data,
            mudlet_map_url,
            gmcp_end_data
    );

    write_to_descriptor(d->descriptor, response);
}

void handle_gmcp_request(struct descriptor_data *d, char *txt)
{
    // This is for GMCP requests from the clients.
    // Do nothing now, but we might want to actually handle this in the future.
}

/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] = {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  dprintf(d, off_string);
}

void send_gmcp_prompt(struct descriptor_data *d)
{
    struct char_data *ch = d->character, *vict = FIGHTING(ch), *tank;
    struct effect *eff;

    static char gmcp_prompt[MAX_STRING_LENGTH];
    char *cur = gmcp_prompt;

    if (!d->gmcp_enabled) {
        return;
    }

    // Need to construct a json string.  Would be nice to get a module to do it for us, but the code base is too old to
    // rely on something like that.
    cur += sprintf(cur,
            "%sChar {" \
            "\"name\":\"%s\","
            "\"class\":\"%s\","
            "\"exp_percent\":%ld," \
            "\"alignment\":%d," \
            "\"hiddenness\":%ld," \
            "\"level\":%d," \
            "\"Vitals\": {" \
                "\"hp\":%d, \"max_hp\": %d," \
                "\"mv\":%d, \"max_mv\": %d " \
            "}}%s",
            gmcp_start_data,
            GET_NAME(ch),
            CLASS_NAME(ch),
            xp_percentage(ch),
            GET_ALIGNMENT(ch),
            GET_HIDDENNESS(ch),
            GET_LEVEL(ch),
            GET_HIT(ch), GET_MAX_HIT(ch),
            GET_MOVE(ch), GET_MAX_MOVE(ch),
            gmcp_end_data
    );

    cur += sprintf(cur,
        "%sChar.Worth {" \
            "\"Carried\": { " \
                "\"platinum\":%d,"
                "\"gold\":%d," \
                "\"silver\":%d," \
                "\"copper\":%d" \
            "}," \
            "\"Bank\": { " \
                "\"platinum\":%d,"
                "\"gold\":%d," \
                "\"silver\":%d," \
                "\"copper\":%d" \
            "}" \
        "}%s",
        gmcp_start_data,
        GET_PLATINUM(ch), GET_GOLD(ch), GET_SILVER(ch), GET_COPPER(ch),
        GET_BANK_PLATINUM(ch), GET_BANK_GOLD(ch), GET_BANK_SILVER(ch), GET_BANK_COPPER(ch),
        gmcp_end_data
    );

    cur += sprintf(cur,
       "%sChar.Effects [",
       gmcp_start_data
    );
    for (eff = ch->effects; eff; eff = eff->next) {
      if (eff->duration >= 0 && (!eff->next || eff->next->type != eff->type)) {
        cur += sprintf(cur, "\"%s\"%s", skills[eff->type].name, eff->next ? ", " : "");
      }
    }
    cur += sprintf(cur, "]%s", gmcp_end_data);

    if (vict && (tank = FIGHTING(vict))) {
      cur += sprintf(cur,
        "%sChar.Combat {" \
          "\"tank\": {" \
            "\"name\": \"%s\"," \
            "\"hp\": \"%d\"," \
            "\"max_hp\": \"%d\"" \
          "}," \
          "\"opponent\": {" \
            "\"name\": \"%s\"," \
            "\"hp_percent\": %d" \
          "}" \
        "}%s",
        gmcp_start_data,
        PERS(tank, ch), GET_HIT(tank), GET_MAX_HIT(tank),
        PERS(vict, ch), 100 * GET_HIT(vict) / GET_MAX_HIT(vict),
        gmcp_end_data
      );
    } else {
        cur += sprintf(cur, "%sChar.Combat {}%s", gmcp_start_data, gmcp_end_data);
    }

    cur += sprintf(cur, "%s", ga_string);

    write_to_descriptor(d->descriptor, gmcp_prompt);
}

void send_gmcp_room(struct char_data *ch)
{
    static char response[MAX_STRING_LENGTH];
    char *cur = response;

    if (IS_MOB(ch) || !ch->desc->gmcp_enabled) {
        return;
    }

    int roomnum = IN_ROOM(ch);
    struct room_data *room = &world[roomnum];
    struct exit *exit;

    if (IS_DARK(roomnum) && !CAN_SEE_IN_DARK(ch)) {
        sprintf(response, "%sRoom.Info {}%s", gmcp_start_data, gmcp_end_data);
    } else {
        cur += sprintf(cur,
            "%sRoom {" \
            "\"zone\":\"%s\"," \
            "\"id\":%d," \
            "\"name\":\"%s\"," \
            "\"type\":\"%s\"," \
            "\"Exits\": {",
            gmcp_start_data,
            zone_table[room->zone].name,
            room->vnum,
            room->name,
            sectors[room->sector_type].name
        );

        bool first = true;
        int dir;

        for (dir = 0; dir < NUM_OF_DIRS; dir++) {
            if ((exit = world[roomnum].exits[dir]) && EXIT_DEST(exit) && can_see_exit(ch, roomnum, exit)) {
                cur += sprintf(cur, "%s\"%s\": {", first ? "" : ",", capdirs[dir]);

                cur += sprintf(cur, "\"to_room\": %d,", EXIT_DEST(exit)->vnum);

                if (EXIT_IS_HIDDEN(exit)) {
                    cur += sprintf(cur, "\"is_hidden\": true,");
                }

                if(EXIT_IS_DOOR(exit)) {
                    cur += sprintf(cur, "\"is_door\": true,");
                    cur += sprintf(cur, "\"door_name\": \"%s\",", exit->keyword);
                    cur += sprintf(cur, "\"door\": \"%s\"",
                            EXIT_IS_CLOSED(exit) ? EXIT_IS_LOCKED(exit) ? "locked" : "closed" : "open");
                } else {
                    cur += sprintf(cur, "\"is_door\": false");
                }

                cur += sprintf(cur, "}");
                first = false;
            }
        }

        cur += sprintf(cur, "}}%s", gmcp_end_data);
    }
    write_to_descriptor(ch->desc->descriptor, response);
}

/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] = {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) TELOPT_NAOFFD,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  dprintf(d, on_string);
}

char *prompt_str(struct char_data *ch) {
  enum token {
    TOK_UNKNOWN, /* Default, not expecting anything in particular */
    TOK_CONTROL, /* Last char was a %, next one is a control code */
    TOK_PERCENT, /* Last two chars were %p, next one is percent code */
    TOK_COIN,    /* Last two chars were %c, next one is coins code */
    TOK_COOLDOWN /* Last two chars were %d, next one is cooldown code */
  };

  struct char_data *vict = FIGHTING(ch), *tank;
  static char prompt[MAX_STRING_LENGTH];
  char block[256], *cur, *raw = GET_PROMPT(ch);
  int color, temp, pre_length = 0;
  enum token expecting = TOK_UNKNOWN;
  bool found_x = FALSE;
  struct effect *eff;

  /* No prompt?  Use the default! */
  if (!raw || !*raw)
    raw = "&0FieryMUD: Set your prompt (see 'help prompt')>";

  /* Use color? */
  color = (PRF_FLAGGED(ch, PRF_COLOR_1) || PRF_FLAGGED(ch, PRF_COLOR_2) ? 1 : 0);

  /* No need to parse if there aren't % symbols */
  if (!strchr(raw, '%')) {
    sprintf(prompt, "%s&0", raw);
    return prompt;
  }

  cur = prompt;

  /*
   * Insert wizinvis and AFK flags at the beginning.  If we DON'T find the
   * %x flag below, we'll keep it.  If not, we'll skip it when we return.
   * End all flags with an extra space!  The %x case below expects it.
   */
  if (!IS_NPC(ch)) {
    if (GET_INVIS_LEV(ch) > 0)
      pre_length += sprintf(cur + pre_length, "<wizi %03d> ",
              GET_INVIS_LEV(ch));
    if (PRF_FLAGGED(ch, PRF_ROOMVIS) && GET_INVIS_LEV(ch))
      pre_length += sprintf(cur + pre_length, "<rmvis> ");
    if (PRF_FLAGGED(ch, PRF_AFK))
      pre_length += sprintf(cur + pre_length, "<AFK> ");
    /* If any flags above, insert newline. */
    if (pre_length) {
      pre_length += sprintf(cur + pre_length, "\r\n");
      /* Advance cursor. */
      cur += pre_length;
    }
  }

  for (block[0] = '\0'; *raw; ++raw) {
    if (expecting == TOK_UNKNOWN) {
      if (*raw == '%')
        /* Next character will be a control code. */
        expecting = TOK_CONTROL;
      else
        /* Not a control code starter...just copy the character. */
        *(cur++) = *raw;
    }
    else if (expecting == TOK_CONTROL) {
      /* Reset what we are expecting for the next char. */
      expecting = TOK_UNKNOWN;
      switch (*raw) {
        case 'h':
          cur += sprintf(cur, "%d", GET_HIT(ch));
          break;
        case 'H':
          cur += sprintf(cur, "%d", GET_MAX_HIT(ch));
          break;
/* Mana isn't available...
        case 'm':
          cur += sprintf(cur, "%d", GET_MANA(ch));
          break;
        case 'M':
          cur += sprintf(cur, "%d", GET_MAX_MANA(ch));
          break;
*/
        case 'v':
          cur += sprintf(cur, "%d", GET_MOVE(ch));
          break;
        case 'V':
          cur += sprintf(cur, "%d", GET_MAX_MOVE(ch));
          break;
        case 'i':
        case 'I':
          cur += sprintf(cur, "%ld", GET_HIDDENNESS(ch));
          break;
        case 'a':
        case 'A':
          cur += sprintf(cur, "%d", GET_ALIGNMENT(ch));
          break;
        case 'n':
          /* Current character's name. */
          cur += sprintf(cur, "%s", GET_NAME(ch));
          break;
        case 'k':
            /* Current Character's Class */
            cur += sprintf(cur, "%s", CLASS_FULL(ch));
            break;
        case 'N':
          /* If switched, show original char's name. */
          cur += sprintf(cur, "%s", GET_NAME(REAL_CHAR(ch)));
          break;
        case 'd':
          /* Cooldown bar: next char will be control code */
          expecting = TOK_COOLDOWN;
          break;
        case 'e':
          /* Show current to-next-level exp status */
          cur += sprintf(cur, "%s", exp_bar(REAL_CHAR(ch), 20, 20, 20, color));
          break;
        case 'E':
          /* Show current to-next-level exp status */
          cur += sprintf(cur, "%s", exp_message(REAL_CHAR(ch)));
          break;
        case 'l':
        case 'L':
          /* List active spells. */
          for (eff = ch->effects; eff; eff = eff->next)
            if (eff->duration >= 0 && (!eff->next || eff->next->type != eff->type)) {
              if (color && EFF_FLAGGED(ch, EFF_DETECT_MAGIC)) {
                if (eff->duration <= 1)
                  cur += sprintf(cur, "%s", CLR(ch, FRED));
                else if (eff->duration <= 3)
                  cur += sprintf(cur, "%s", CLR(ch, HRED));
              }
              cur += sprintf(cur, "%s%s",
                             skills[eff->type].name,
                             eff->next ? ", " : "");
              if (color && EFF_FLAGGED(ch, EFF_DETECT_MAGIC))
                cur += sprintf(cur, "%s", CLR(ch, ANRM));
            }
          break;
        case 'p':
        case 'P':
          /* Percent of x remaining: next char will be a control code. */
          expecting = TOK_PERCENT;
          break;
        case 'c':
        case 'C':
          /* Amount of gold held/in bank: next char will be a control code. */
          expecting = TOK_COIN;
          break;
        case 'w':
          /* All money held. */
          cur += sprintf(cur, "&0%d&6&8p&0 %d&3&8g&0 %ds %d&3c&0",
                  GET_PLATINUM(ch), GET_GOLD(ch),
                  GET_SILVER(ch), GET_COPPER(ch));
          break;
        case 'W':
          /* All money in bank. */
          cur += sprintf(cur, "&0%d&6&8p&0 %d&3&8g&0 %ds %d&3c&0",
                  GET_BANK_PLATINUM(ch), GET_BANK_GOLD(ch),
                  GET_BANK_SILVER(ch), GET_BANK_COPPER(ch));
          break;
        case 'o':
          /* Show the victim's name and status. */
          if (vict)
            cur += snprintf(cur, 255, "%s &0(%s)", PERS(vict, ch),
                    status_string(GET_HIT(vict), GET_MAX_HIT(vict), STATUS_ALIAS));
          break;
        case 'O':
          /* Just victim's name. */
          if (vict)
            cur += snprintf(cur, 255, "%s", PERS(vict, ch));
          break;
        case 't':
          /* Show the tank's name and status. */
          if (vict && (tank = FIGHTING(vict)))
            cur += sprintf(cur, "%s &0(%s&0)", PERS(tank, ch),
                    status_string(GET_HIT(tank), GET_MAX_HIT(tank), STATUS_ALIAS));
          break;
        case 'T':
          /* Just the tank's name. */
          if (vict && (tank = FIGHTING(vict)))
            cur += sprintf(cur, "%s", PERS(tank, ch));
          break;
        case 'g':
          if (ch->group_master)
            cur += sprintf(cur, "%s &0(%s&0)", PERS(ch->group_master, ch),
                    status_string(GET_HIT(ch->group_master),
                                  GET_MAX_HIT(ch->group_master),
                                  STATUS_ALIAS));
          break;
        case 'G':
          if (ch->group_master)
            cur += sprintf(cur, "%s", PERS(ch->group_master, ch));
          break;
        case 'r':
          cur += sprintf(cur, "%d", GET_RAGE(ch));
          break;
        case 'x':
        case 'X':
          /* Flags like wizinvis and AFK. */
          found_x = TRUE;
          /* If any flags were inserted above, copy them here instead. */
          if (pre_length >= 3) {
            /* We don't want to keep the extra space or newline. */
            snprintf(cur, pre_length - 2, "%s", prompt);
            /* snprintf returns the number of chars it WOULD have printed
             * if it didn't truncate, so we add to the pointer manually. */
            cur += pre_length - 3;
          }
          break;
        case 'z':
        case 'Z':
          /* Show the zone */
          cur += sprintf(cur, "%s", zone_table[world[IN_ROOM(ch)].zone].name);
          break;
        case '#':
          cur += sprintf(cur, "%d", GET_LEVEL(ch));
          break;
        case '_':
          cur += sprintf(cur, "\r\n");
          break;
        case '-':
          cur += sprintf(cur, " ");
          break;
        case '%':
          cur += sprintf(cur, "%%");
          break;
      }
    }
    else if (expecting == TOK_PERCENT) {
      expecting = TOK_UNKNOWN;
      switch (*raw) {
        case 'h':
        case 'H':
          temp = (100 * GET_HIT(ch)) / MAX(1, GET_MAX_HIT(ch));
          break;
/*
        case 'm':
        case 'M':
          temp = (100 * GET_MANA(ch)) / MAX(1, GET_MAX_MANA(ch));
          break;
*/
        case 'v':
        case 'V':
          temp = (100 * GET_MOVE(ch)) / MAX(1, GET_MAX_MOVE(ch));
          break;
        default:
          /* If we set expecting to 0 ahead of time up here, we'll skip
             the percent sprintf below. */
          continue;
      }
      cur += sprintf(cur, "%d%%", temp);
    }
    else if (expecting == TOK_COIN) {
      expecting = TOK_UNKNOWN;
      switch (*raw) {
        case 'p': temp = GET_PLATINUM(ch); break;
        case 'g': temp = GET_GOLD(ch); break;
        case 's': temp = GET_SILVER(ch); break;
        case 'c': temp = GET_COPPER(ch); break;
        case 'P': temp = GET_BANK_PLATINUM(ch); break;
        case 'G': temp = GET_BANK_GOLD(ch); break;
        case 'S': temp = GET_BANK_SILVER(ch); break;
        case 'C': temp = GET_BANK_COPPER(ch); break;
        default:  continue; /* don't print anything */
      }
      cur += sprintf(cur, "%d", temp);
    }
    else if (expecting == TOK_COOLDOWN) {
      expecting = TOK_UNKNOWN;
      /* Show time left for particular cooldown */
      switch (*raw) {
        case 'i': temp = CD_INSTANT_KILL; break;
        case 'd': temp = CD_DISARM;       break;
        case 'm': temp = CD_SUMMON_MOUNT; break;
        case 'l': temp = CD_LAY_HANDS;    break;
        case 'f': temp = CD_FIRST_AID;    break;
        case 't': temp = CD_THROATCUT;    break;
        case 's': temp = CD_SHAPECHANGE;  break;
        case 'c': temp = CD_CHANT;        break;
        default:  continue; /* don't print anything */
      }
      cur += sprintf(cur, "%s", cooldown_bar(ch, temp, 20, 20, color));
    }
  }

  sprintf(cur, "&0");
  cur = prompt;
  if (found_x)
    cur += pre_length;
  return cur;
}

void make_prompt(struct descriptor_data *d)
{
  /* Do not use dprintf or any function that ultimately calls
   * string_to_output within make_prompt!  You must use
   * write_to_descriptor, because string_to_output resets the
   * prompt flag.
   */

  if (PAGING(d)) {
    char prompt[MAX_INPUT_LENGTH];
    sprintf(prompt, "\r[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]",
            PAGING_PAGE(d) + 1, PAGING_NUMPAGES(d));
    write_to_descriptor(d->descriptor, prompt);
  }
  else if (EDITING(d) || d->str)
    write_to_descriptor(d->descriptor, "] ");
  else if (!d->connected) {
    char *prompt = prompt_str(d->character);
    process_colors(prompt, MAX_STRING_LENGTH, prompt,
                   COLOR_LEV(d->character) >= C_NRM ? CLR_PARSE : CLR_STRIP);

    write_to_descriptor(d->descriptor, prompt);
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
bool casting_command(struct descriptor_data *d, char *txt) {
  int cmd;

  /*
   * Only use this hack for descriptors with characters who are
   * actually playing.  Additionally, they must be casting.
   */
  if (!d || STATE(d) != CON_PLAYING || !d->character || !CASTING(d->character))
    return FALSE;

  /*
   * A hack-within-a-hack to enable usage of paging while casting.
   */
  if (PAGING(d)) {
    get_paging_input(d, txt);
    return TRUE;
  }
  else if (EDITING(d)) {
    editor_interpreter(d, txt);
    return TRUE;
  }
  else if (d->str) {
    string_add(d, txt);
    return TRUE;
  }

  any_one_arg(txt, arg);

  for (cmd = 0; *cmd_info[cmd].command != '\n'; ++cmd)
    if (!strncmp(cmd_info[cmd].command, arg, strlen(arg)))
      if (can_use_command(d->character, cmd))
        break;

  if (*cmd_info[cmd].command == '\n')
    return FALSE;

  /*
   * Non-casting commands will be queued up normally.
   */
  if (!IS_SET(cmd_info[cmd].flags, CMD_CAST))
    return FALSE;

  /* Handle the casting-ok command immediately. */
  dprintf(d, "\r\n");
  command_interpreter(d->character, txt);
  return TRUE;
}


void write_to_q(char *txt, struct txt_q *queue, int aliased, struct descriptor_data *d)
{
  struct txt_block *new;

  CREATE(new, struct txt_block, 1);
  CREATE(new->text, char, strlen(txt) + 1);
  strcpy(new->text, txt);
  new->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    new->next = NULL;
    queue->head = queue->tail = new;
  } else {
    queue->tail->next = new;
    queue->tail = new;
    new->next = NULL;
  }
}


int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return 0;

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return 1;
}


/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }

  while (get_from_q(&d->input, buf2, &dummy));
}


/* aka dprintf */
void desc_printf(struct descriptor_data *t, const char *txt, ...)
{
  va_list args;

  static unsigned int vcount = 0;
  static unsigned int scount = 0;

  if (txt && *txt) {
    if (strchr(txt, '%')) {
      va_start(args, txt);
      vsnprintf(comm_buf, sizeof(comm_buf), txt, args);
      va_end(args);
      txt = comm_buf;
      ++vcount;
    }
    else
      ++scount;
    string_to_output(t, txt);
  }

  /* TODO: check this debug data and determine if the split
   * for static strings is necessary
   */
  if ((vcount + scount) % 100 == 0)
    fprintf(stderr, "DEBUG :: log :: vprintf calls - %u, fputs calls - %u\n",
            vcount, scount);
}

/* Add a new string to a player's output queue */
void string_to_output(struct descriptor_data *t, const char *txt)
{
  int size;
  static char new_txt[2 * MAX_STRING_LENGTH];

  if (!t || !txt || !*txt)
    return;

  size = process_colors(new_txt, sizeof(new_txt), txt, t->character &&
                        COLOR_LEV(t->character) >= C_NRM ? CLR_PARSE : CLR_STRIP);

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace > size) {
    strcpy(t->output + t->bufptr, new_txt);
    t->bufspace -= size;
    t->bufptr += size;
    return;
  }

  /*
   * If we're already using the large buffer, or if even the large buffer
   * is too small to handle this new text, chuck the text and switch to the
   * overflow state.
   */
  if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
    t->bufptr = -1;
    buf_overflows++;
    return;
  }

  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else {                      /* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output);     /* copy to big buffer */
  t->output = t->large_outbuf->text;    /* make big buffer primary */
  strcat(t->output, new_txt);   /* now add new text */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;
}

void free_bufpools(void)
{
  extern struct txt_block *bufpool;
  struct txt_block *tmp;

  while (bufpool) {
    tmp = bufpool->next;
    if (bufpool->text)
      free(bufpool->text);
    free(bufpool);
    bufpool = tmp;
  }
}


  /* ******************************************************************
   *  socket handling                                                  *
   ****************************************************************** */

void init_descriptor(struct descriptor_data *newd, int desc)
{
  static int last_desc = 0;

  newd->descriptor = desc;
  newd->idle_tics = 0;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->login_time = time(0);
  *newd->output = '\0';
  newd->bufptr = 0;
  newd->wait = 1;
  newd->gmcp_enabled = false;
  STATE(newd) = CON_QANSI;

  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;
}


int new_descriptor(int s) {
  socket_t desc;
  int sockets_connected = 0;
  unsigned long addr;
  unsigned int i;
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;
  extern char *BANNEDINTHEUSA;
  extern char *BANNEDINTHEUSA2;
  extern char *BANNEDINTHEUSA3;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET) {
    perror("accept");
    return -1;
  }
  /* keep it from blocking */
  nonblock(desc);

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= max_players) {
    write_to_descriptor(desc, "Sorry, FieryMUD is full right now... please try again later!\r\n");
    CLOSE_SOCKET(desc);
    return 0;
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the sitename */
  if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
      sizeof(peer.sin_addr), AF_INET))) {

    /* resolution failed */
    if (!nameserver_is_slow)
      perror("gethostbyaddr");

    /* find the numeric site address */
    addr = ntohl(peer.sin_addr.s_addr);
    sprintf(newd->host, "%03u.%03u.%03u.%03u", (int) ((addr & 0xFF000000) >> 24),
            (int) ((addr & 0x00FF0000) >> 16), (int) ((addr & 0x0000FF00) >> 8),
            (int) ((addr & 0x000000FF)));
  } else {
    strncpy(newd->host, from->h_name, HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    write_to_descriptor(desc, BANNEDINTHEUSA);
    write_to_descriptor(desc, BANNEDINTHEUSA2);
    write_to_descriptor(desc, BANNEDINTHEUSA3);
    sprintf(buf, "\r\n Connection logged from: %s\r\n\r\n", newd->host);
    write_to_descriptor(desc, buf);
    CLOSE_SOCKET(desc);
    mprintf(L_STAT, LVL_GOD, "BANNED: Connection attempt denied from [%s]", newd->host);
    free(newd);
    return 0;
  }
#if 0
  /* Log new connections - probably unnecessary, but you may want it */
  mprintf(L_STAT, LVL_GOD, "New connection from [%s]", newd->host);
#endif

  init_descriptor(newd, desc);

  /* prepend to list */
  newd->next = descriptor_list;
  descriptor_list = newd;

  offer_gmcp(newd);

  dprintf(newd, "Do you want ANSI terminal support? (Y/n) ");

  return 0;
}


int process_output(struct descriptor_data *t)
{
  static char i[LARGE_BUFSIZE + GARBAGE_SPACE];
  static int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");

  /* now, append the 'real' output */
  strcpy(i + 2, t->output);

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    strcat(i, "**OVERFLOW**");

  /* add the extra CRLF if the person isn't in compact mode */
  if (!t->connected && t->character && !PRF_FLAGGED(t->character, PRF_COMPACT))
    strcat(i + 2, "\r\n");

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (!t->prompt_mode)                /* && !t->connected) */
    result = write_to_descriptor(t->descriptor, i);
  else
    result = write_to_descriptor(t->descriptor, i + 2);

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by)
    dprintf(t->snoop_by, "&2((&0 %s &2))&0", t->output);

  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  return result;
}


int write_to_descriptor(socket_t desc, char *txt)
{
  int total, bytes_written;

  total = strlen(txt);

  do {
#ifdef CIRCLE_WINDOWS
    if ((bytes_written = send(desc, txt, total, 0)) < 0) {
      if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
    if ((bytes_written = write(desc, txt, total)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
        errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno == EAGAIN)
#endif /* CIRCLE_WINDOWS */
        log("process_output: socket write would block, about to close");
      else
        perror("Write to socket");
      return -1;
    } else {
      txt += bytes_written;
      total -= bytes_written;
    }
  } while (total > 0);

  return 0;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data *t)
{
  int bytes_read, space_left, command_space_left, failed_subst, telopt = 0;
  char *ptr, *read_point, *write_point = NULL;
  char inbuf[MAX_RAW_INPUT_LENGTH - 1], tmp[MAX_INPUT_LENGTH + 8], telcmd = 0;
  bool gmcp = false;

  read_point = inbuf;
  space_left = MAX_RAW_INPUT_LENGTH - 1;
  command_space_left = MAX_INPUT_LENGTH - 1;

  write_point = tmp;

  do {
    if (space_left <= 0) {
      log("process_input: about to close connection: input overflow");
      return -1;
    }
#ifdef CIRCLE_WINDOWS
    if ((bytes_read = recv(t->descriptor, read_point, space_left, 0)) < 0) {
      if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
    if ((bytes_read = read(t->descriptor, read_point, space_left)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
        errno = EAGAIN;
#endif /* EWOULDBLOCK */
      if (errno != EAGAIN) {
#endif /* CIRCLE_WINDOWS */
        log("process_input: about to lose connection");
        return -1; /* some error condition was encountered on
                    * read */
      } else {
        return 0;  /* the read would have blocked: just means no data there but everything's okay */
      }
    } else if (bytes_read == 0) {
      log("EOF on socket read (connection broken by peer)");
      return -1;
    }
    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';        /* terminate the string */

    /* search for a newline or control codes in the data we just read */
    for (ptr = read_point; *ptr; ++ptr) {
        if (*ptr == (char) IAC) { // Telnet Control Option Starting
            telopt = 1;
        } else if (telopt == 1 && gmcp && *ptr == (char) SE) { // End of GMCP message
            telopt = 0;
            gmcp = false;;
            *write_point = '\0';
            handle_gmcp_request(t, tmp);
            write_point = tmp;
            command_space_left = MAX_INPUT_LENGTH - 1;
        } else if (telopt == 1) {
            telopt = 2;
            telcmd = *ptr;
        } else if (telopt == 2 && *ptr == (char) GMCP) { // Ready to handle GMCP data
            telopt = 0;
            if (telcmd == (char) DO) {
                t->gmcp_enabled = true;
                offer_gmcp_services(t);
            } else if (telcmd == (char) DONT) {
                t->gmcp_enabled = false;
            } else if (telcmd == (char) SB) {
                gmcp = true;
            } else {
                // ERROR
                log("Invalid GMCP code %d", (int) telcmd);
            }
        } else if (telopt == 2) {
            // log("Invalid 2nd level IAC code %d", (int) *ptr);
        } else if (telopt > 0) { // If we are here, there is an error
            log("Invalid telnet IAC code %d", (int) *ptr);
        } else {
            if (IS_NEWLINE(*ptr) || command_space_left <= 0) { // End of command, process it*write_point = '\0';
                *write_point = '\0';
                if (t->snoop_by)
                    dprintf(t->snoop_by, "&6>>&b %s &0\r\n", tmp);

                failed_subst = 0;

                if (*tmp == '!')
                    strcpy(tmp, t->last_input);
                else if (*tmp == '^') {
                    if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
                        strcpy(t->last_input, tmp);
                } else
                    strcpy(t->last_input, tmp);
                /*
                 * If the user is casting, and the command is ok
                 * when casting, then it gets processed immediately
                 * by the command interpreter within casting_command().
                 * Otherwise, the command is queued up and handled by
                 * the game loop normally.  Oh, and this is a hack.
                 */
                if (!failed_subst && !casting_command(t, tmp))
                    write_to_q(tmp, &t->input, 0, t);

                if (command_space_left <= 0) {
                    char buffer[MAX_INPUT_LENGTH + 64];
                    snprintf(buffer, sizeof(buffer), "Line too long.  Truncated to:\r\n%s\r\n", tmp);
                    if (write_to_descriptor(t->descriptor, buffer) < 0)
                        return -1;
                    while (*ptr && !IS_NEWLINE(*ptr)) // Find next newline
                        ++ptr;
                }
                while (*(ptr+1) && IS_NEWLINE(*(ptr+1))) // Find start of next command.
                    ++ptr;

                write_point = tmp;
                command_space_left = MAX_INPUT_LENGTH - 1;
            } else if (*ptr == '\b') {        /* handle backspacing */
                if (write_point > tmp) {
                    if (*(--write_point) == '$') {
                        write_point--;
                        command_space_left += 2;
                    } else
                        command_space_left++;
                }
            } else if (isascii(*ptr) && isprint(*ptr)) {
                if ((*(write_point++) = *ptr) == '$') {   /* copy one character */
                    *(write_point++) = '$';        /* if it's a $, double it */
                    command_space_left -= 2;
                } else
                    command_space_left--;
            }
        }
    }

    read_point += bytes_read;
    space_left -= bytes_read;

  } while (true);
}

/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char new[MAX_INPUT_LENGTH + 5];
  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    dprintf(t, "Invalid substitution.\r\n");
    return 1;
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    dprintf(t, "Invalid substitution.\r\n");
    return 1;
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(new, orig, (strpos - orig));
  new[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(new, second, (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(new, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* terminate the string in case of an overflow from strncat */
  new[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, new);

  return 0;
}



void close_socket(struct descriptor_data *d)
{
  struct descriptor_data *temp;
  long target_idnum = -1;

  CLOSE_SOCKET(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    dprintf(d->snoop_by, "Your victim is no longer among us.\r\n");
    d->snoop_by->snooping = NULL;
  }

  /* Shapechange a player back to normal form */
  if (d->character) {
    if (POSSESSED(d->character))
      do_shapechange(d->character, "me", 0, 1);
  }

  /*. Kill any OLC stuff .*/
  switch(d->connected) {
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
      free(*d->str);
      if (d->backstr) {
        *d->str = d->backstr;
      } else
        *d->str = NULL;
      d->backstr = NULL;
      d->str = NULL;
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
    target_idnum = GET_IDNUM(d->character);
    /*
     * Plug memory leak, from Eric Green.
     */
    if (PLR_FLAGGED(d->character, PLR_MAILING) && d->str) {
      if (*(d->str))
        free(*(d->str));
      free(d->str);
    }
    if (IS_PLAYING(d)) {
      save_player_char(d->character);
      act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
      mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
             "Closing link to: %s.", GET_NAME(d->character));
      d->character->desc = NULL;
    }
    else {
      if (GET_NAME(d->character))
        mprintf(L_STAT, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
               "Losing player: %s.", GET_NAME(d->character));
      /*else
      mprintf(L_STAT, LVL_IMMORT, "Losing player: <null>.");
      d->character->desc = NULL;
      free_char(d->character);
     */
    }
  }
  else
    mprintf(L_INFO, LVL_IMMORT, "Losing descriptor without char.");

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  REMOVE_FROM_LIST(d, descriptor_list, next);

  free_paged_text(d);
  if (d->storage)
    free(d->storage);

  free(d);
}


void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME &&
        STATE(d) != CON_QANSI && STATE(d) != CON_MENU &&
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
      dprintf(d, "\r\nTimed out... goodbye.\r\n");
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
#ifdef CIRCLE_WINDOWS

void nonblock(socket_t s)
{
  long val = 1;
  ioctlsocket(s, FIONBIO, &val);
}

#else

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
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

RETSIGTYPE checkpointing()
{
  if (!tics) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated");
    abort();
  } else
    tics = 0;
}

RETSIGTYPE dump_core()
{
  mprintf(L_STAT, LVL_IMMORT, "Received SIGUSR1 - dumping core.");
  drop_core(NULL, "usrsig");
}

RETSIGTYPE unrestrict_game()
{
  extern struct ban_list_element *ban_list;

  mprintf(L_WARN, LVL_IMMORT,
         "Received SIGUSR2 - completely unrestricting game (emergent)");
  ban_list = NULL;
  should_restrict = 0;
  restrict_reason = RESTRICT_NONE;
}

RETSIGTYPE hupsig()
{
  log("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  circle_shutdown = TRUE;  /* added by Gurlaek 2/14/2000 */
}

#define my_signal(signo, func) signal(signo, func)

void signal_setup(void)
{
#ifndef CIRCLE_OS2
  struct itimerval itime;
#endif
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  /* I'm removing the autowiz stuff. I'm leaving this here because
   * it seems to serve some vital function. It may be able to be
   * reassigned to something else now that autowiz is gone RSD 3/19/00
   */
  my_signal(SIGUSR1, dump_core);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.  Doesn't work with
   * OS/2.
   */
#ifndef CIRCLE_OS2
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);
#endif

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);

#ifdef CIRCLE_OS2
#if defined(SIGABRT)
  my_signal(SIGABRT, hupsig);
#endif
#if defined(SIGFPE)
  my_signal(SIGFPE, hupsig);
#endif
#if defined(SIGILL)
  my_signal(SIGILL, hupsig);
#endif
#if defined(SIGSEGV)
  my_signal(SIGSEGV, hupsig);
#endif
#endif                                /* CIRCLE_OS2 */

}

#endif                                /* CIRCLE_WINDOWS */


/* ****************************************************************
 *       Public routines for system-to-player-communication        *
 **************************************************************** */

/* aka cprintf */
void char_printf(const struct char_data *ch, const char *messg, ...)
{
  va_list args;

  static unsigned int vcount = 0;
  static unsigned int scount = 0;

  if (ch->desc && messg && *messg) {
    if (strchr(messg, '%')) {
      va_start(args, messg);
      vsnprintf(comm_buf, sizeof(comm_buf), messg, args);
      va_end(args);
      messg = comm_buf;
      ++vcount;
    }
    else
      ++scount;
    string_to_output(ch->desc, messg);
  }

  /* TODO: check this debug data and determine if the split
   * for static strings is necessary
   */
//  if ((vcount + scount) % 100 == 0)
//    fprintf(stderr, "DEBUG :: cprintf :: vsnprintf calls - %u, no calls - %u\n", vcount, scount);
}

void zone_printf(int zone_vnum, int skip_room, int min_stance, const char *messg, ...)
{
  struct descriptor_data *i;
  va_list args;
  bool found = FALSE;
  int zone_num = real_zone(zone_vnum);

  if (!messg || !*messg)
     return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character &&
        GET_STANCE(i->character) >= min_stance &&
        IN_ROOM(i->character) != NOWHERE &&
        IN_ROOM(i->character) != skip_room &&
        IN_ZONE_RNUM(i->character) == zone_num) {
      if (!found) {
        va_start(args, messg);
        vsnprintf(comm_buf, sizeof(comm_buf), messg, args);
        va_end(args);
        found = TRUE;
      }
      string_to_output(i, comm_buf);
    }
}

void callback_printf(CBP_FUNC(callback), void *data, const char *messg, ...)
{
  struct descriptor_data *i;
  bool found;
  va_list args;

  if (messg && *messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i->character && callback(i->character, data)) {
        if (!found) {
          va_start(args, messg);
          vsnprintf(comm_buf, sizeof(comm_buf), messg, args);
          va_end(args);
          found = TRUE;
        }
        string_to_output(i, comm_buf);
      }


}

void all_except_printf(struct char_data *ch, const char *messg, ...)
{
  struct descriptor_data *i;
  va_list args;
  bool found = FALSE;
  struct char_data *avoid = REAL_CHAR(ch);

  if (messg && *messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected &&
          !(avoid && avoid->desc && avoid->desc == i)) {
        if (!found) {
          va_start(args, messg);
          vsnprintf(comm_buf, sizeof(comm_buf), messg, args);
          va_end(args);
          found = TRUE;
        }
        string_to_output(i, comm_buf);
      }
}
void all_printf(const char *messg, ...)
{
  struct descriptor_data *i;
  va_list args;
  bool found = FALSE;

  if (messg && *messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected) {
        if (!found) {
          va_start(args, messg);
          vsnprintf(comm_buf, sizeof(comm_buf), messg, args);
          va_end(args);
          found = TRUE;
        }
        string_to_output(i, comm_buf);
      }
}


void outdoor_printf(int zone_num, char *messg, ...)
{
  struct descriptor_data *i;
  va_list args;
  bool found = FALSE;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && AWAKE(i->character) &&
        CH_OUTSIDE(i->character) && IN_ZONE_RNUM(i->character) == zone_num &&
        STATE(i) == CON_PLAYING) {
      if (!found) {
        va_start(args, messg);
        vsnprintf(comm_buf, sizeof(comm_buf), messg, args);
        va_end(args);
        found = TRUE;
      }
      string_to_output(i, comm_buf);
    }
}

void room_printf(int roomnum, const char *messg, ...)
{
  struct char_data *i;
  int dir, rmnmlen;
  struct room_data *room;
  struct exit *exit;
  bool found = FALSE;
  va_list args;

  if (!messg || !*messg)
    return;

  if (roomnum >= 0 && roomnum <= top_of_world)
    room = &world[roomnum];
  else {
    mprintf(L_ERROR, LVL_GOD, "SYSERR: bad room rnum %d passed to send_to_room", roomnum);
    return;
  }

  for (i = room->people; i; i = i->next_in_room)
    if (i->desc) {
      if (!found) {
        snprintf(comm_buf, sizeof(comm_buf), "@B<@0%s@B>@0 ", room->name);
        rmnmlen = strlen(comm_buf);
        va_start(args, messg);
        vsnprintf(comm_buf + rmnmlen, sizeof(comm_buf) - rmnmlen, messg, args);
        va_end(args);
        found = TRUE;
      }
      /* Skip over the room name */
      string_to_output(i->desc, comm_buf + rmnmlen);
    }

  /* Reflect to OBSERVATORY rooms if this room is an ARENA. */
  if (ROOM_FLAGGED(roomnum, ROOM_ARENA)) {
    for (dir = 0; dir < NUM_OF_DIRS; ++dir)
      if ((exit = room->exits[dir]) &&
            EXIT_NDEST(exit) != NOWHERE &&
            EXIT_NDEST(exit) != roomnum &&
            ROOM_FLAGGED(EXIT_NDEST(exit), ROOM_OBSERVATORY))
        for (i = EXIT_DEST(exit)->people; i; i = i->next_in_room)
          if (i->desc) {
            if (!found) {
              snprintf(comm_buf, sizeof(comm_buf), "@B<@0%s@B>@0 ", room->name);
              rmnmlen = strlen(comm_buf);
              va_start(args, messg);
              vsnprintf(comm_buf + rmnmlen, sizeof(comm_buf) - rmnmlen, messg, args);
              va_end(args);
              found = TRUE;
            }
            string_to_output(i->desc, comm_buf);
          }
  }
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
 *  quiet - Boolean value. TRUE prevents this function from sending any
 *          feedback to the character.
 *
 * RETURN VALUES
 *
 *   TRUE - the communication is OK
 *  FALSE - the communication should not occur. Unless quiet=TRUE, a message
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

int speech_ok(struct char_data *ch, int quiet)
{
   struct char_special_data *sd;
   struct trig_data *t;

   if (GET_LEVEL(ch) >= LVL_IMMORT) return TRUE;

   if (IS_NPC(ch) && POSSESSED(ch)) {
      /* A shapechanged player */
      sd = &ch->desc->original->char_specials;
   } else {
      sd = &ch->char_specials;
   }

   /* If it's been at least SPAM_PERIOD since the last communication, there is
    * no need for calculation - any speech_rate you may have is obsolete and
    * can be forgotten. */
   if (sd->last_speech_time == 0 ||
         global_pulse - sd->last_speech_time > SPAM_PERIOD ||
         sd->last_speech_time > global_pulse /* rollover, in case the mud runs insanely long? */
         ) {
      sd->speech_rate = 1;
      sd->last_speech_time = global_pulse;
      return TRUE;
   }

   /* If the char is executing a trigger, it's all good */
   if (SCRIPT(ch))
      for (t = SCRIPT(ch)->trig_list; t; t = t->next)
         if (t->running)
            return TRUE;

   /* The following calculation considers the amount of time since the last
    * communication and uses it to reduce the speech rate. Then it adds 1 for
    * this communication. */
   sd->speech_rate = 1 + sd->speech_rate * (SPAM_PERIOD - global_pulse +
         sd->last_speech_time) / SPAM_PERIOD;
   sd->last_speech_time = global_pulse;

   if (sd->speech_rate > SPAM_THRESHOLD) {
      /* Try to blab with laryngitis, and the recovery period starts over again. */
      sd->speech_rate = SPAM_THRESHOLD * 1.5;
      if (!quiet)
         cprintf(ch, "&5&bYour mouth refuses to move!&0\r\n");
      return FALSE;

   } else if (sd->speech_rate > SPAM_THRESHOLD / 2) {
      /* You're in the sore-throat zone!*/

      /* Add a random number to discourage brinkmanship. If you keep it up
       * even with a minor sore throat, you don't know what it will take
       * to bring on full-blown laryngitis (which occurs without warning). */
      sd->speech_rate += number(1, 4);
      if (sd->speech_rate > SPAM_THRESHOLD) {
         sd->speech_rate = SPAM_THRESHOLD * 1.5;
         if (!quiet)
            cprintf(ch, "&6&bYou feel laryngitis coming on!&0\r\n");
      } else {
         if (!quiet)
            cprintf(ch, "&5Your throat feels a little sore.&0\r\n");
      }
   }

   return TRUE;
}

void speech_report(struct char_data *ch, struct char_data *tch)
{
   struct char_special_data *sd;

   if (GET_LEVEL(tch) >= LVL_IMMORT) return;

   if (IS_NPC(tch) && POSSESSED(tch)) {
      /* A shapechanged player */
      sd = &tch->desc->original->char_specials;
   } else {
      sd = &tch->char_specials;
   }

   if (sd->last_speech_time == 0 ||
         global_pulse - sd->last_speech_time > SPAM_PERIOD ||
         sd->last_speech_time > global_pulse /* rollover, in case the mud runs insanely long? */
         ) {
      return;
   }

   sd->speech_rate = sd->speech_rate * (SPAM_PERIOD - global_pulse +
         sd->last_speech_time) / SPAM_PERIOD;
   sd->last_speech_time = global_pulse;

   if (sd->speech_rate > SPAM_THRESHOLD)
      cprintf(ch, "&5&b%s %s an acute case of laryngitis.&0\r\n",
              ch == tch ? "You" : GET_NAME(tch),
              ch == tch ? "have" : "has");
   else if (sd->speech_rate > SPAM_THRESHOLD / 2)
      cprintf(ch, "&5%s%s throat feels a little sore.&0\r\n",
              ch == tch ? "Your" : GET_NAME(tch),
              ch == tch ? "" : "'s");
}

void cure_laryngitis(struct char_data *ch)
{
   struct char_special_data *sd;

   if (IS_NPC(ch) && POSSESSED(ch)) {
      /* A shapechanged player */
      sd = &ch->desc->original->char_specials;
   } else {
      sd = &ch->char_specials;
   }
   sd->speech_rate = 0;
}


char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);

/* higher-level communication: the act() function */
void format_act(char *rtn, const char *orig, struct char_data *ch, struct obj_data *obj,
      const void *vict_obj, const struct char_data *to) {
   const char *i = NULL;
   char *bufptr, *j, ibuf[20];
   bool uppercasenext = FALSE;

   bufptr = rtn;

   if (!to->desc)
      return;

   for (;;) {
      if (*orig == '$') {
         switch (*(++orig)) {
            case 'n':
               i = PERS(ch, to);
               break;
            case 'N':
               CHECK_NULL(vict_obj, PERS((const struct char_data *) vict_obj, to));
               break;
            case 'm':
               i = HMHR(ch);
               break;
            case 'M':
               CHECK_NULL(vict_obj, HMHR((const struct char_data *) vict_obj));
               break;
            case 's':
               i = HSHR(ch);
               break;
            case 'S':
               CHECK_NULL(vict_obj, HSHR((const struct char_data *) vict_obj));
               break;
            case 'D':
               CHECK_NULL(vict_obj,
                     without_article(GET_NAME((const struct char_data *) vict_obj)));
               break;
            case 'e':
               i = HSSH(ch);
               break;
            case 'E':
               CHECK_NULL(vict_obj, HSSH((const struct char_data *) vict_obj));
               break;
            case 'o':
               CHECK_NULL(obj, OBJN(obj, to));
               break;
            case 'O':
               CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj, to));
               break;
            case 'p':
               CHECK_NULL(obj, OBJS(obj, to));
               break;
            case 'P':
               CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to));
               break;
            case 'a':
               CHECK_NULL(obj, SANA(obj));
               break;
            case 'A':
               CHECK_NULL(vict_obj, SANA((const struct obj_data *) vict_obj));
               break;
            case 't':
               CHECK_NULL(obj, (const char *) obj);
               break;
            case 'T':
               CHECK_NULL(vict_obj, (const char*) vict_obj);
               break;
            case 'f':
               CHECK_NULL(vict_obj, fname((char *) obj));
               break;
            case 'F':
               CHECK_NULL(vict_obj, fname((char *) vict_obj));
               break;
            case 'i':
               i = ibuf;
               sprintf(ibuf, "%d", (int) obj);
               break;
            case 'I':
               i = ibuf;
               sprintf(ibuf, "%d", (int) vict_obj);
               break;
            /* uppercase previous word */
            case 'u':
               for (j=bufptr; j > rtn && !isspace((int) *(j-1)); j--);
               if (j != bufptr)
               *j = UPPER(*j);
               i = "";
               break;
            /* uppercase next word */
            case 'U':
               uppercasenext = TRUE;
               i = "";
               break;
               case '$':
               i = "$";
               break;
            default:
               log("SYSERR: Illegal $-code to act():\n"
                   "SYSERR: %s", orig);
               i = "";
               break;
         }
         while ((*bufptr = *(i++))) {
            if (uppercasenext && !isspace((int) *bufptr)) {
               *bufptr = UPPER(*bufptr);
               uppercasenext = FALSE;
            }
            bufptr++;
         }
         orig++;
      } else if (!(*(bufptr++) = *(orig++))) {
         break;
      } else if (uppercasenext && !isspace((int) *(bufptr-1))) {
         *(bufptr-1) = UPPER(*(bufptr-1));
         uppercasenext = FALSE;
      }
   }

   *(--bufptr) = '\r';
   *(++bufptr) = '\n';
   *(++bufptr) = '\0';

   CAP(rtn);
}

/* The "act" action interpreter */
void act(const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, int type)
{
   char lbuf[MAX_STRING_LENGTH];
   struct char_data *to = NULL;
   int sleep, olc, in_room, i, to_victroom;

   if (!str) return;

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
      if (ch && SENDOK(ch))
         format_act(lbuf, str, ch, obj, vict_obj, ch);
         cprintf(ch, "%s", lbuf);

       return;
   }

   if (type == TO_VICT) {
      if ((to = (struct char_data *) vict_obj) && SENDOK(to) &&
            !(hide_invisible && ch && !CAN_SEE(to, ch))) {
         format_act(lbuf, str, ch, obj, vict_obj, to);
         cprintf(to, "%s", lbuf);
      }

      return;
   }
   /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

   if (ch) {
      if ((ch->in_room != NOWHERE)) {
         in_room = ch->in_room;
      } else {
         log("SYSERR:comm.c:act(): ch->in_room = -1, %s", GET_NAME(ch));
         return;
      }
   } else if ((obj != NULL)) {
      if ((obj->in_room != NOWHERE)) {
         if ((obj->in_room) != 0) {
            in_room = obj->in_room;
         } else {
            log("SYSERR:comm.c:act(): obj->in_room = 0, %s, %d", obj->name, GET_OBJ_VNUM(obj));
            return;
         }
      } else {/*if here then NO ch and obj->in_room = NOWHERE*/
         log("SYSERR:comm.c:act(): obj->in_room = -1, %s, %d", obj->name, GET_OBJ_VNUM(obj));
         return;
      }
   } else {
      /* no obj or ch */
      log("SYSERR:comm.c:act(): NULL char and object pointer");
      return;
   }

   if (to_victroom) {
         if (vict_obj) {
            in_room = ((struct char_data *) vict_obj)->in_room;
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
      if (SENDOK(to) && !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
            (to != ch) && (type == TO_ROOM || (to != vict_obj))) {
         format_act(lbuf, str, ch, obj, vict_obj, to);
         cprintf(to, "%s", lbuf);
      }


    /*
     * Reflect TO_ROOM and TO_NOTVICT calls that occur in ARENA rooms
     * into OBSERVATORY rooms, allowing players standing in observatories
     * to watch arena battles safely.
     */
   if (ROOM_FLAGGED(in_room, ROOM_ARENA))
      for (i = 0; i < NUM_OF_DIRS; ++i)
         if (world[in_room].exits[i] &&
             world[in_room].exits[i]->to_room != NOWHERE &&
             world[in_room].exits[i]->to_room != in_room &&
             ROOM_FLAGGED(world[in_room].exits[i]->to_room, ROOM_OBSERVATORY))
            for (to = world[world[in_room].exits[i]->to_room].people; to; to = to->next_in_room)
               if (SENDOK(to) && !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
                   (to != ch) && (type == TO_ROOM || (to != vict_obj))) {
                  cprintf(to, "&4&8<&0%s&0&4&8>&0 ", world[in_room].name);
                  format_act(lbuf, str, ch, obj, vict_obj, to);
                  cprintf(to, "%s", lbuf);
               }
}

/* deprecated functions */
void send_to_char(const char *messg, struct char_data *ch)
{
  if (ch->desc && messg && *messg)
    string_to_output(ch->desc, messg);
}

void write_to_output(const char *messg, struct descriptor_data *d)
{
  if (messg && *messg)
    string_to_output(d, messg);
}

void send_to_all(const char *messg) {
  all_printf("%s", messg);
}

void send_to_room(const char *messg, int room) {
  room_printf(room, "%s", messg);
}

void send_to_zone(const char *messg, int zone_vnum, int skip_room, int min_stance) {
  zone_printf(zone_vnum, skip_room, min_stance, "%s", messg);
}

/***************************************************************************
 * $Log: comm.c,v $
 * Revision 1.180  2010/06/05 14:56:27  mud
 * Moving cooldowns to their own file.
 *
 * Revision 1.179  2009/07/16 19:15:54  myc
 * Moved command stuff from grant.c to commands.c
 *
 * Revision 1.178  2009/06/11 13:36:05  myc
 * Make the exp bar in the prompt work while shapechanged.
 *
 * Revision 1.177  2009/06/10 02:27:14  myc
 * Added an optimization to char_printf and desc_printf which checks
 * to see if the message to be printed has any %'s before attempting
 * to vsnprintf.  This can save some time, but to see whether it's
 * really worth it, we're going to count the number of times each
 * branch is used and log it.
 *
 * Revision 1.176  2009/05/01 05:29:40  myc
 * Make the mud initialize the rules vtable index at boot.
 *
 * Revision 1.175  2009/03/21 19:11:37  myc
 * Add cooldown bar to prompt.
 *
 * Revision 1.174  2009/03/20 23:02:59  myc
 * Get rid of autowiz signal handler.
 *
 * Revision 1.173  2009/03/20 20:19:51  myc
 * Remove dependency on boards.[ch].
 *
 * Revision 1.172  2009/03/09 20:36:00  myc
 * Renamed all *PLAT macros to *PLATINUM.
 *
 * Revision 1.171  2009/03/09 05:59:57  myc
 * The mud now keeps track of all previous boot times, including
 * hotboot times.
 *
 * Revision 1.170  2009/03/09 05:41:31  jps
 * Moved money stuff into money.h, money.c
 *
 * Revision 1.169  2009/03/09 04:33:20  jps
 * Moved direction information from structs.h, constants.h, and constants.c
 * into directions.h and directions.c.
 *
 * Revision 1.168  2009/03/08 23:34:14  jps
 * Renamed spells.[ch] to casting.
 *
 * Revision 1.167  2009/03/07 22:30:29  jps
 * Add act flag TO_VICTROOM which makes the message go to the victim's room.
 *
 * Revision 1.166  2009/02/26 17:40:41  rsd
 * Removed the once every 5 minutes tracking of how many
 * people are connected to the mud and recorded in the syslog
 * in nearly 10 years we've never used the data and it's just
 * spam.
 *
 * Revision 1.165  2009/02/21 03:30:16  myc
 * Added hooks for new board system.  Removed L_FILE flag--mprintf
 * now logs to file by default; assert L_NOFILE to prevent that.
 *
 * Revision 1.164  2009/02/16 19:37:21  myc
 * Mobs won't get laryngitis during scripts anymore.
 *
 * Revision 1.163  2009/02/11 17:03:39  myc
 * Adding hooks for new text editor to game_loop, make_prompt, and
 * close_socket.
 *
 * Revision 1.162  2009/02/09 20:09:56  myc
 * Use status_string function to describe health status in prompts.
 *
 * Revision 1.161  2009/02/05 17:15:04  myc
 * Fixing minor bug in send_to_room/room_printf that was sporadically
 * sending the room name along with the message even when not in an arena.
 *
 * Revision 1.160  2009/01/19 08:42:29  myc
 * Added $i and $I to act().
 *
 * Revision 1.159  2009/01/17 00:28:02  myc
 * Fix use of uninitialized variable.
 *
 * Revision 1.158  2008/09/21 21:50:56  jps
 * Stop trying to keep track of who's attacking who when there's a shapechange,
 * since do_shapechange handles that internally now.
 *
 * Revision 1.157  2008/09/21 20:40:40  jps
 * Keep a list of attackers with each character, so that at the proper times -
 * such as char_from_room - they can be stopped from battling.
 *
 * Revision 1.156  2008/09/21 05:26:55  myc
 * Added <rmvis> flag to prompt when imms are invis and roomvis.
 *
 * Revision 1.155  2008/09/20 17:37:29  jps
 * Added 'D' format char for act() which provides vict without an article
 * (as a direct object).
 *
 * Revision 1.154  2008/09/20 06:05:06  jps
 * Add macros POSSESSED and POSSESSOR.
 *
 * Revision 1.153  2008/09/09 08:23:37  jps
 * Placed sector info into a struct and moved its macros into rooms.h.
 *
 * Revision 1.152  2008/09/07 20:07:39  jps
 * Added all_except_printf, which is like all_printf but it avoids one character.
 *
 * Revision 1.151  2008/09/01 23:47:49  jps
 * Using movement.h/c for movement functions.
 *
 * Revision 1.150  2008/08/31 07:16:41  jps
 * Properly abort editing in descs. Ug.
 *
 * Revision 1.149  2008/08/30 04:13:45  myc
 * Replaced the exp_to_level monstrosity with a lookup table that gets
 * populated at boot time.
 *
 * Revision 1.148  2008/08/29 04:16:26  myc
 * Added reference to act.h header file.
 * ..
 *
 * Revision 1.147  2008/08/26 04:39:21  jps
 * Changed IN_ZONE to IN_ZONE_RNUM or IN_ZONE_VNUM and fixed zone_printf.
 *
 * Revision 1.146  2008/08/25 02:31:30  jps
 * The logic of the act() targets bits didn't fit well with the way
 * the targets were being used, so I undid the change.
 *
 * Revision 1.145  2008/08/25 00:20:33  myc
 * Changed the way mobs memorize spells.
 *
 * Revision 1.144  2008/08/23 14:03:30  jps
 * Changed the way act() messages are targeted. Now there are three basic
 * target flags, which may be or'd together in any combination, plus three
 * modifier flags for sleep, old, and trigger. The old TO_NOTVICT and
 * TO_ROOM targets are defined in terms of the new flags.
 *
 * Revision 1.143  2008/08/14 23:02:11  myc
 * Added vararg capability to all the standard output functions (like
 * send_to_char and write_to_output).  The old functions are still
 * available.  The new ones follow a *printf naming convention.
 * However, removed the send_to_outdoor functionality, and replaced
 * it with callback_printf.
 *
 * Revision 1.142  2008/08/14 09:45:22  jps
 * Replaced the pager.
 *
 * Revision 1.141  2008/08/13 05:53:18  jps
 * Allow NPCs to be affected by laryingitis too.  Ehlissa has corrupted the necromancers.
 *
 * Revision 1.140  2008/07/27 05:30:09  jps
 * Renamed "crash" stuff to "autosave". Renamed save_player to
 * save_player_char.
 *
 * Revision 1.139  2008/07/15 17:49:24  myc
 * Whether you can use a command depends not only on level now, but
 * also on command grants.
 *
 * Revision 1.138  2008/07/13 18:49:29  jps
 * Change the formatting of snoop messages.
 *
 * Revision 1.137  2008/06/19 18:53:12  myc
 * Replaced the item_types string list with a struct array,
 * and added an init_objtypes() function to check values at
 * boot time.
 *
 * Revision 1.136  2008/06/07 19:06:46  myc
 * Moved all object-related constants and structures to objects.h
 *
 * Revision 1.135  2008/06/05 02:07:43  myc
 * Rewrote rent-saving code to use ascii-format files.
 *
 * Revision 1.134  2008/05/26 18:24:48  jps
 * Removed code that deletes player object files.
 *
 * Revision 1.133  2008/05/18 20:16:11  jps
 * Created fight.h and set dependents.
 *
 * Revision 1.132  2008/05/17 22:03:01  jps
 * Moving room-related code into rooms.h and rooms.c.
 *
 * Revision 1.131  2008/05/17 04:32:25  jps
 * Moved exits into exits.h/exits.c and changed the name to "exit".
 *
 * Revision 1.130  2008/04/26 23:35:43  myc
 * Info about permanent effects and race skills are stored in the
 * class/race structs now, but need to be initialized at runtime
 * by the init_races and init_classes functions.
 *
 * Revision 1.129  2008/04/19 20:17:46  jps
 * Add comment at call to perform_mob_violence.
 *
 * Revision 1.128  2008/04/07 03:02:54  jps
 * Changed the POS/STANCE system so that POS reflects the position
 * of your body, while STANCE describes your condition or activity.
 *
 * Revision 1.127  2008/04/05 21:42:50  jps
 * Change event calling so that events scheduled for immediate
 * execution will occur before the next pulse.  If they are scheduled
 * during the execution of a player's command, they will be executed
 * before the player's next prompt is printed.
 *
 * Revision 1.126  2008/04/05 16:49:21  myc
 * Fixing conditional color for people who turn color off.
 *
 * Revision 1.125  2008/04/05 05:05:00  myc
 * Reformatted most functions.
 *
 * Revision 1.124  2008/04/04 06:12:52  myc
 * Removed dieites/worship and ships code.
 *
 * Revision 1.123  2008/04/04 05:13:46  myc
 * Removing maputil code.
 *
 * Revision 1.122  2008/04/03 02:02:05  myc
 * Upgraded ansi color handling code.
 *
 * Revision 1.121  2008/04/02 03:24:44  myc
 * Rewrote group code and removed all major group code.
 *
 * Revision 1.120  2008/03/30 17:30:38  jps
 * Renamed objsave.c to pfiles.c and introduced pfiles.h. Files using functions
 * from pfiles.c now include pfiles.h and depend on it in the makefile.
 *
 * Revision 1.119  2008/03/28 17:54:53  myc
 * Now using flagvectors for effect, mob, player, preference, room, and
 * room effect flags.  AFF, AFF2, and AFF3 flags are now just EFF flags.
 *
 * Revision 1.118  2008/03/17 15:31:27  myc
 * Shutdown reboot will exit with a normal status code now, and go
 * through normal shutdown routines (like memory checks).
 *
 * Revision 1.117  2008/03/08 22:29:06  myc
 * Moving shapechange and chant to the cooldown systems.
 *
 * Revision 1.116  2008/03/07 21:21:57  myc
 * Replaced action delays and skill delays with a single list of
 * 'cooldowns', which are decremented by a recurring event and
 * also save to the player file.
 *
 * Revision 1.115  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.114  2008/03/06 04:34:38  myc
 * Added a PULSE_AUTOSAVE define that regulates how often autosaves occur.
 *
 * Revision 1.113  2008/03/05 05:21:56  myc
 * Removed some debug logs lines from hotboot.  Bank coins are ints instead
 * of longs now.
 *
 * Revision 1.112  2008/03/05 03:03:54  myc
 * Ascii pfiles change the way players are loaded.
 *
 * Revision 1.111  2008/02/24 17:31:13  myc
 * Added a TO_OLC flag to act() to allow messages to be sent to people
 * while in OLC if they have OLCComm toggled on.
 *
 * Revision 1.110  2008/02/24 06:31:41  myc
 * The world command will now show many hotboots have ocurred since the
 * last shutdown.
 *
 * Revision 1.109  2008/02/16 20:26:04  myc
 * Adding zmalloc, a lightweight memory debugger that wraps all malloc
 * and free calls, keeping track of your memory expenditures.  To be
 * effective, however, we have to free the database and a lot of other
 * stuff at program termination, so adding a lot of functions to do all
 * of that.  Also adding some more prompt codes.
 *
 * Revision 1.108  2008/02/09 18:29:11  myc
 * No need for the name_timeout eventfunc here.
 *
 * Revision 1.107  2008/02/09 07:05:37  myc
 * Copyover is now renamed to hotboot.  Fixed the color codes in the
 * hotboot messages.
 *
 * Revision 1.106  2008/02/09 04:27:47  myc
 * Now relying on math header file.
 *
 * Revision 1.105  2008/02/09 03:04:23  myc
 * Adding the 'copyover' command, which allows you to do a hot-boot
 * without disconnecting anybody.
 *
 * Revision 1.104  2008/02/06 21:53:53  myc
 * Make the format arg to act() const.
 *
 * Revision 1.103  2008/02/02 04:27:55  myc
 * Adding time triggers (they execute at a given mud time each day).
 *
 * Revision 1.102  2008/01/30 19:20:57  myc
 * Removing the loopy gravity code from here.  It's now an event.
 *
 * Revision 1.101  2008/01/29 21:02:31  myc
 * Removing a lot of extern declarations from code files and moving
 * them to header files, mostly db.h and constants.h.
 *
 * Revision 1.100  2008/01/29 16:51:12  myc
 * Moving skill names to the skilldef struct.
 *
 * Revision 1.99  2008/01/27 21:09:12  myc
 * Took out the spell circle codes from prompt and adding rage.
 *
 * Revision 1.98  2008/01/26 14:26:31  jps
 * Moved a lot of skill-related code into skills.h and skills.c.
 *
 * Revision 1.97  2008/01/25 21:05:45  myc
 * Added attack() as a macro alias for hit() with fewer arguments.
 *
 * Revision 1.96  2008/01/15 06:51:47  myc
 * Made a change to exp_bar; it now accepts a sub-gradient size.
 *
 * Revision 1.95  2008/01/10 05:39:43  myc
 * alter_hit now takes a boolean specifying whether to cap any increase in
 * hitpoints by the victim's max hp.
 *
 * Revision 1.94  2008/01/09 04:14:19  jps
 * Remove spell mem and scribe funcs now that that's handled by events.
 *
 * Revision 1.93  2008/01/05 05:37:04  jps
 * Changed name of function save_char() to save_player(). Because it
 * only operates on players.
 *
 * Revision 1.92  2008/01/04 04:30:47  jps
 * Made spellcasting into an event.
 *
 * Revision 1.91  2007/12/25 05:21:49  jps
 * Remove comments about trouble with obj_to_room - now alleviated due to
 * improved event code.
 *
 * Revision 1.90  2007/12/20 23:10:39  myc
 * Adding some new prompt control codes:
 *   i/I  hiddenness
 *   a/A  alignment
 *   n    current char's name
 *   N    if switched, original char's name, otherwise current
 *   e    exp TNL in bar form
 *   E    exp progress message
 *   l/L  list of active spells
 * Changed speech_report to just cprintf instead of buffering it.
 *
 * Revision 1.89  2007/12/19 20:45:06  myc
 * Added const modifiers to the char arguments to write_to_output,
 * cprintf, send_to_zone, send_to_room, and parse_color, which
 * allows you to output a const string without casting it.  save_player()
 * no longer requires a save room (which wasn't being used anyway).
 *
 * Revision 1.88  2007/11/28 09:52:19  jps
 * Clean up do_gravity_check() a bit, and hopefully avoid an intermittent
 * crash bug that would strike when a falling object ended up sinking
 * in water.
 *
 * Revision 1.87  2007/11/18 16:51:55  myc
 * Adding OBSERVATORY behavior to send_to_room.
 *
 * Revision 1.86  2007/10/23 20:20:22  myc
 * Modify the casting command handler to allow for use of the pager
 * during casting (for informative commands).
 *
 * Revision 1.85  2007/10/02 02:52:27  myc
 * Fixed idle timer to work for shapechanged druids.
 *
 * Revision 1.84  2007/09/15 15:36:48  myc
 * Removed defunct ITEM_ bitvector flags.  They are duplicating AFF_ flags.
 *
 * Revision 1.83  2007/09/04 06:49:19  myc
 * IN_ZONE macro is now an rnum.  Changed weather function calls.
 *
 * Revision 1.82  2007/08/27 21:18:00  myc
 * You can now queue up commands while casting as well as abort midcast.
 * Casting commands such as look and abort are caught and interpreted
 * before the input is normally queued up by the game loop.
 *
 * Revision 1.81  2007/08/22 18:01:46  jps
 * Reduce the beeping for autoboot warnings. Add warnings at 30, 10,
 * and 5 seconds.
 *
 * Revision 1.80  2007/08/19 18:34:39  jps
 * Don't send falling_yell to the room below.
 *
 * Revision 1.79  2007/08/17 02:23:36  jps
 * Don't let the mud sleep while it's in the automatic reboot warning phase.
 * Otherwise you would have to have someone logged in for it to reboot.
 * Generalized some autobooting code.
 *
 * Revision 1.78  2007/08/14 20:13:57  jps
 * The mud will automatically reboot after a randomized period of time,
 * though powerful deities can delay or prevent this.
 *
 * Revision 1.77  2007/08/14 10:44:05  jps
 * Add functions to keep track of how much players have been communicating
 * and to squelch them with "laryngitis" if they spam too much.
 *
 * Revision 1.76  2007/08/03 22:00:11  myc
 * act() now supports observatories: a room marked observatory that is
 * adjacent to an arena room will see all to_notvict and to_room act
 * calls to the arena room.
 * Also changed perform_act so it returns immediately if the 'to' target
 * doesn't have a descriptor.  This should save some CPU cycles.
 *
 * Revision 1.75  2007/07/25 00:38:03  jps
 * Give send_to_zone a room to skip, and make it use virtual zone number.
 *
 * Revision 1.74  2007/07/24 23:34:00  jps
 * Add a parameter min_position to send_to_zone()
 *
 * Revision 1.73  2007/05/28 06:59:42  jps
 * Removed extra space from end of prompt. Added %- code so that
 * people whose clients give them trouble with trailing spaces
 * can still add one.
 *
 * Revision 1.72  2007/05/24 07:28:34  jps
 * Transmit a surprised yell to surrounding rooms when someone starts to fall.
 *
 * Revision 1.71  2007/04/18 01:02:17  myc
 * Added some new codes to the prompt parser and added some NPC checks so
 * mobs can't use player specific ones.
 *
 * Revision 1.69  2006/11/20 19:52:04  jps
 * Levitate prevents falling damage
 *
 * Revision 1.68  2006/11/14 22:47:10  jps
 * Make sure imms don't fall in flight rooms.
 *
 * Revision 1.67  2006/11/14 21:30:44  jps
 * Stop invis'd gods from being seen by lower level imms as they
 * disconnect from the main menu.
 *
 * Revision 1.66  2006/11/13 15:54:22  jps
 * Fix implementation of hide_informative parameter to act(), when
 * the target is TO_VICT.
 *
 * Revision 1.65  2006/11/13 02:51:02  jps
 * Fix illegal pointer access.
 *
 * Revision 1.64  2006/11/10 21:00:26  jps
 * Update act() to accept $U, $u and some other format codes.
 *
 * Revision 1.63  2006/04/01 21:46:23  rls
 * Modified gravity to allow arial combat.  Simplistic hack really...
 * using aff_flying as the check rather than pos_flying (logically)
 *
 * Revision 1.62  2004/11/11 23:02:23  rsd
 * Added additional char stars for the Banning message to
 * the descriptor output because the original message was
 * to large for the compiler to not complain about
 *
 * Revision 1.61  2003/04/16 02:00:22  jjl
 * Added skill timers for Zzur.  They don't save to file, so they were a
 * quickie.
 *
 * Revision 1.60  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.59  2002/08/29 17:29:03  rsd
 * added a check for CON_ISPELL_BOOT for a password checking
 * state to be kicked offline if idle to long.
 *
 * Revision 1.58  2002/04/17 22:41:06  dce
 * Added attacker = null; to prevent shapechanged players
 * from crashing the mud.
 *
 * Revision 1.57  2001/12/09 14:08:32  dce
 * Had to add the d->character check for the shapechange thing
 * in close_socket.
 *
 * Revision 1.56  2001/12/07 02:09:56  dce
 * Linkdead players will now lose exp when they die.
 * Linkdead shapechanged players will now shapechange
 * to their original form before linking out.
 *
 * Revision 1.55  2001/05/17 02:55:38  dce
 * Fixed crash bug with menu timeout.
 *
 * Revision 1.54  2001/05/17 02:45:49  dce
 * Fixed a crash bug around saving a character at check_idle_passwords
 *
 * Revision 1.53  2001/05/13 16:15:58  dce
 * Fixed a bug where somethings wouldn't save when a player
 * died and exitied menu option 0 rather than menu option 1.
 *
 * Revision 1.52  2001/05/03 01:26:16  dce
 * Players now timeout at the menu screen.
 *
 * Revision 1.51  2001/03/14 19:04:59  rsd
 * made the default for ooc ON again.
 *
 * Revision 1.50  2001/03/09 03:05:21  dce
 * Fixed it so that immortals do not get booted out of shapechanged
 * form no matter what their alignment is.
 *
 * Revision 1.49  2001/03/06 03:10:18  dce
 * Fixed a bug where players awaiting a name approval could
 * cut their link and then crash the mud.
 *
 * Revision 1.48  2001/03/04 17:33:19  dce
 * Fixed the falling problem where players would not
 * gain hp after falling. Also fixed the problem where
 * players could cast while sitting after falling.
 *
 * Revision 1.47  2001/03/03 18:08:20  dce
 * Minor fix for shapechange
 *
 * Revision 1.46  2001/02/24 16:47:31  dce
 * Changes made for shapechange. Shapechange uses the chant
 * variable to limit its use.
 *
 * Revision 1.45  2001/01/23 01:51:46  rsd
 * turned off occ by detault
 *
 * Revision 1.44  2000/11/28 01:36:34  mtp
 * remveove last vestiges of mobprgos (damn these things get around)
 *
 * Revision 1.43  2000/11/28 01:19:01  mtp
 * remove process_events() (replaced dg_event code with events.c code)
 * removed mobprog references
 *
 * Revision 1.42  2000/11/20 19:43:45  rsd
 * added back rlog messages from prior to the $log$ string.
 *
 * Revision 1.41  2000/11/15 22:55:01  rsd
 * changed a perror() to a log()
 *
 * Revision 1.40  2000/04/14 00:47:00  rsd
 * Added some spanky code to send banned players a message about
 * being banned, also added something to the log message to make
 * it bloody obvious someone banned tried to connect...
 *
 * Revision 1.39  2000/04/02 02:38:23  rsd
 * Added the comment header while I was browsing for information.
 * I also fixed bracket problems in the social section and re-tabbed
 * that part of the function.
 *
 * Revision 1.38  2000/03/20 04:28:47  rsd
 * added comments regarding my_signal(SIGUSR1, reread_wizlists)
 *
 * Revision 1.37  2000/02/14 05:13:23  jimmy
 * Fixed hupsig signal to not exit the instant it received a SIGHUP.
 * The mud now shuts down cleanly.
 *
 * Revision 1.36  1999/11/28 22:55:42  cso
 * *** empty log message ***
 *
 * Revision 1.35  1999/10/06 17:53:01  rsd
 * Added afk to the beginning of the prompt and cmc added
 * wizinvis and afk into the prompt properly.
 *
 * Revision 1.34  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.33  1999/08/12 18:21:47  dce
 * Potential lock out bug.
 * Mud times out connections waiting at ANSI prompt.
 *
 * Revision 1.32  1999/08/12 04:25:39  jimmy
 * This is a Mass ci of the new pfile system.  The pfile has been split into
 * one file for each player in a directory A-Z.  The object files are also
 * located in the A-Z directories.  Fixed a stupid bug in pfilemaint that
 * screwed up the IDNUM of the person who typed it.  Commented out the frag
 * system completely.  It is slated for removal.  Fixed the rename command.
 * Fixed all supporting functions for the new system, I hope!
 * --Gurlaek 8/11/1999
 *
 * Revision 1.31  1999/07/26 03:49:00  jimmy
 * added debug to help with pfile problems
 *
 * Revision 1.30  1999/07/09 22:30:27  jimmy
 * Attempt to control the spiraling of memory.  Added a free() to the
 * prompt code in comm.c to free memory allocated by parse_color().
 * made a global structure dummy_mob and malloc'ed it for mobs
 * to share as their player_specials to cut memory.
 * gurlaek
 *
 * Revision 1.29  1999/07/09 13:57:00  mud
 * Changed missed reference to HUBIS to Fiery
 *
 * Revision 1.28  1999/07/09 02:59:46  jimmy
 * doh, forgot to recompile after commenting. Stupid parse error.
 *
 * Revision 1.27  1999/07/09 02:38:50  jimmy
 * rewrote parse_color() to fix silly overflows due to an estimation of the
 * length of the converted ansi string.  The function now does not malloc
 * until the string has been converted and is in it's final form.
 * --gurlaek 7/8/1999
 *
 * Revision 1.26  1999/07/02 21:58:04  jimmy
 * Highely modified worship.c/h to change from many small functions that
 * were passes as (void *) to 4 large functions that use switch().  This
 * was done to remove the warnings produced by the -pedantic compiler flag.
 * divine_intervention was also commented out for now in comm.c because we
 * don't currently use it anyway. --Gurlaek 7/2/1999
 *
 * Revision 1.25  1999/06/11 16:56:55  jimmy
 * date: 1999/06/11 16:56:55;  author: jimmy;  state: Exp;  lines: +1 -1
 * Ok, fixed do_quit to check for fighting and also not crash when mortally
 * wounded.  This was done in die() by checking for killer=NULL.
 * since no one killed you if you quit while morted the die code
 * didn't know how to deal with a NULL killer.
 * --Gurlaek 6/11/1999
 *
 * Revision 1.24  1999/06/10 16:56:28  mud
 * This is a mass check in after a code freeze due to an upgrade to RedHat 6.0.
 * This fixes all of the warnings associated with the new compiler and
 * libraries.  Many many curly braces had to be added to "if" statements to
 * clarify their behavior to the compiler.  The name approval code was also
 * debugged, and tested to be stable.  The xnames list was converted from an
 * array to a linked list to allow for on the fly adding of names to the
 * xnames list. This code compiles fine under both gcc RH5.2 and egcs RH6.0.
 * --Gurlaek 6/10/1999
 *
 * Revision 1.23  1999/04/23 23:27:10  jimmy
 * Fixed warnings/errors associated with the addition of the pendantic compiler flag
 * yeeeeehaaawwww.  --gurlaek
 *
 * Revision 1.22  1999/04/23 21:35:11  jimmy
 * fixed typecast of int i to work with the accept() function
 * in  new_descriptor().
 * -- Gurlaek
 *
 * Revision 1.21  1999/04/16 20:50:34  dce
 * Attempt to fix parse_color bug
 *
 * Revision 1.20  1999/04/10 06:04:52  dce
 * Testing
 *
 * Revision 1.19  1999/04/10 05:53:40  jen
 * sigh
 *
 * Revision 1.18  1999/04/10 05:13:48  jen
 * Revisions w/ debug info
 *
 * Revision 1.17  1999/04/10 05:05:58  jen
 * Revision for testing #1
 *
 * Revision 1.16  1999/04/09 23:19:49  jen
 * An attempt to fix a core dump when perform_act handles
 * i = "someone"; I think more drastic measures may be needed
 * but this is a first shot.
 *
 * Revision 1.15  1999/04/05 19:37:55  jen
 * Redone cap code... using toupper fn instead of CAP macro
 *
 * Revision 1.14  1999/04/05 19:01:05  jen
 * Commented out the car needscap :p
 *
 * Revision 1.13  1999/04/05 18:55:46  jen
 * Backed out the capitalization code until
 * I can figure out why it's crashing sometimes.
 *
 * Revision 1.12  1999/04/02 17:07:06  jen
 * Fixes the problem with mobs telling you stuff (or conceivably any
 * instance where an 'act' string is parsed with a $n for a mob at
 * the beginning - i.e. '$n tells you') ... it capitalizes the first
 * char of the mob name in that instance only.
 *
 * Selina
 *
 * Revision 1.11  1999/03/26 19:47:10  jen
 * Added a mortal gossip channel with 103+ godly control
 *
 * Revision 1.10  1999/03/21 21:18:12  dce
 * Fixed a Hubis crash bug.
 *
 * Revision 1.9  1999/03/12 19:44:42  jimmy
 * Updated debug messages in comm.c and handler.c to include
 * more useful info, as well as adding an abort where exit
 * used to be in order to get a core.
 * fingon
 *
 * Revision 1.8  1999/03/08 20:22:35  dce
 * Adds the skill safefall for monks.
 *
 * Revision 1.7  1999/03/06 23:51:54  dce
 * Add's chant songs, and can only chant once every four hours
 *
 * Revision 1.6  1999/03/01 05:31:34  jimmy
 * Rewrote spellbooks.  Moved the spells from fingh's PSE to a standard linked
 * list.  Added Spellbook pages.  Rewrote Scribe to be a time based event based
 * on the spell mem code.  Very basic at this point.  All spells are 5 pages long,
 * and take 20 seconds to scribe each page.  This will be more dynamic when the
 * SCRIBE skill is introduced.  --Fingon.
 *
 * Revision 1.5  1999/02/24 22:59:35  jimmy
 * Fixed gravity code to handle IN_FLIGHT rooms with no down exits.
 * Also added fix for air rooms over water as well as semantics
 * for splashing and sinking.
 * fingon
 *
 * Revision 1.4  1999/02/07 06:46:21  jimmy
 * fixed silly div by zero in the prompt code
 * fingon
 *
 * Revision 1.3  1999/01/30 19:42:27  mud
 * Indented entire file
 *
 * Revision 1.2  1999/01/29 05:42:00  jimmy
 * ixed AFF_FLYING to GET_POS(ch) == POS_FLYING for gravity
 *
 * Revision 1.1  1999/01/29 01:23:30  mud
 * Initial revision
 *
 ***************************************************************************/

/***************************************************************************
 *  File: conf.h                                          Part of FieryMUD *
 *  Usage: Uh, like defines and stuff based on system dependencies         *
 *  By: Generated automatically by configure.                              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "defines.hpp"

#include <time.h>

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if you have the crypt function.  */
#define HAVE_CRYPT 1

/* Define if you have the random function.  */
#define HAVE_RANDOM 1

/* Define if you have the <arpa/telnet.h> header file.  */
#define HAVE_ARPA_TELNET_H 1

/* Define if you have the <assert.h> header file.  */
#define HAVE_ASSERT_H 1

/* Define if you have the <crypt.h> header file.  */
#define HAVE_CRYPT_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <net/errno.h> header file.  */
/* #undef HAVE_NET_ERRNO_H */

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/fcntl.h> header file.  */
#define HAVE_SYS_FCNTL_H 1

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the crypt library (-lcrypt).  */
#define HAVE_LIBCRYPT 1

/* Define if you have the malloc library (-lmalloc).  */
#define HAVE_LIBMALLOC 1

/* Define if you have the nsl library (-lnsl).  */
#define HAVE_LIBNSL 1

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

// Extern declarations for the global variables in conf.cpp
extern int pk_allowed;
extern int summon_allowed;
extern int charm_allowed;
extern int sleep_allowed;
extern int roomeffect_allowed;
extern int races_allowed;
extern int evil_races_allowed;
extern int races_allowed;
extern int evil_races_allowed;
extern int level_gain;
extern int damage_amounts;
extern int pt_allowed;
extern int level_can_shout;
extern int max_group_difference;
extern int holler_move_cost;
extern int max_npc_corpse_time;
extern int max_pc_corpse_time;
extern int short_pc_corpse_time;
extern int approve_names;
extern int napprove_pause;
extern int dts_are_dumps;
extern int reboot_hours_base;
extern int reboot_hours_deviation;
extern int reboot_warning_minutes;
extern int reboot_auto;
extern int reboot_auto;
extern long reboot_pulse;
extern int reboot_warning;
extern int last_reboot_warning;
extern int mortal_start_room;
extern int immort_start_room;
extern int frozen_start_room;
extern int r_mortal_start_room, r_immort_start_room, r_frozen_start_room;
extern int donation_room_1;
extern int donation_room_2;
extern int donation_room_3;
extern time_t *boot_time;
extern int num_hotboots;
extern int should_restrict;
extern int restrict_reason;
extern int environment;
extern const char *environments[];
extern int DFLT_PORT;
extern const char *DFLT_DIR;
extern const char *DFLT_ENV;
extern int MAX_PLAYERS;
extern int max_filesize;
extern int max_bad_pws;
extern int nameserver_is_slow;
extern const char *MENU;
extern const char *GREETINGS;
extern const char *GREETINGS2;
extern const char *GREETINGS3;
extern const char *GREETINGS4;
extern const char *TEST_GREETING;
extern const char *TEST_GREETING2;
extern const char *TEST_GREETING3;
extern const char *DEV_GREETING;
extern const char *DEV_GREETING2;
extern const char *DEV_GREETING3;
extern const char *WHOAREYOU;
extern const char *WELC_MESSG;
extern const char *START_MESSG;
extern const char *NAMES_EXPLANATION;
extern const char *BANNEDINTHEUSA;
extern const char *BANNEDINTHEUSA2;
extern const char *BANNEDINTHEUSA3;
extern const char *NEWSUPDATED1;
extern const char *NEWSUPDATED2;

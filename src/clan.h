/***************************************************************************
 * $Id: clan.h,v 1.13 2009/07/16 22:27:56 myc Exp $
 ***************************************************************************/
/***************************************************************************
 *  File: clan.h                                          Part of FieryMUD *
 *  Usage: header file for clans                                           *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#ifndef __FIERY_CLAN_H
#define __FIERY_CLAN_H

#include "sysdep.h"
#include "structs.h"
#include "privileges.h"

/* Defaults */
#define ALLOW_CLAN_LINKLOAD TRUE
#define ALLOW_CLAN_ALTS TRUE
#define ALLOW_CLAN_QUIT TRUE
#define BACKUP_CLAN_ON_DELETE TRUE

#define GET_CLAN_MEMBERSHIP(ch) ((ch)->player_specials->clan)
#define GET_CLAN(ch) (GET_CLAN_MEMBERSHIP(ch) ? GET_CLAN_MEMBERSHIP(ch)->clan : NULL)
#define GET_CLAN_RANK(ch) (GET_CLAN_MEMBERSHIP(ch) ? GET_CLAN_MEMBERSHIP(ch)->rank : RANK_NONE)
#define GET_CLAN_TITLE(ch) (IS_CLAN_MEMBER(ch) ? GET_CLAN(ch)->ranks[GET_CLAN_RANK(ch) - 1].title : NULL)
#define MEMBER_CAN(member, priv)                                                                                       \
    (IS_MEMBER_RANK(member->rank) && IS_FLAGGED(member->clan->ranks[member->rank - 1].privileges, (priv)))
#define HAS_CLAN_PRIV(ch, priv) (GET_CLAN_MEMBERSHIP(ch) ? MEMBER_CAN(GET_CLAN_MEMBERSHIP(ch), (priv)) : FALSE)
#define CAN_DO_PRIV(ch, priv) (IS_CLAN_ADMIN(ch) || IS_CLAN_SUPERADMIN(ch) || HAS_CLAN_PRIV((ch), (priv)))
#define GET_CLAN_SNOOP(ch) ((ch)->player_specials->clan_snoop)

#define CPRIV_DESC 0
#define CPRIV_MOTD 1
#define CPRIV_GRANT 2
#define CPRIV_RANKS 3
#define CPRIV_TITLE 4
#define CPRIV_ENROLL 5
#define CPRIV_EXPEL 6
#define CPRIV_PROMOTE 7
#define CPRIV_DEMOTE 8
#define CPRIV_APP_FEE 9
#define CPRIV_APP_LEV 10
#define CPRIV_DUES 11
#define CPRIV_WITHDRAW 12
#define CPRIV_ALTS 13
#define CPRIV_CHAT 14
#define NUM_CLAN_PRIVS 15 /* Number of clan privileges */

#define MIN_CLAN_RANKS 2
#define MAX_CLAN_RANKS 100
#define RANK_ADMIN 0
#define RANK_LEADER 1
#define ALT_RANK_OFFSET MAX_CLAN_RANKS
#define MAX_ALT_RANK (ALT_RANK_OFFSET + RANK_LEADER)
#define MIN_ALT_RANK (ALT_RANK_OFFSET + MAX_CLAN_RANKS)
#define RANK_APPLICANT (ALT_RANK_OFFSET + MAX_CLAN_RANKS + 1)
#define RANK_REJECT (RANK_APPLICANT + 1)
#define RANK_NONE (RANK_REJECT + 1)

#define OUTRANKS(a, b) ((a) < (b))

#define IS_REJECT_RANK(rank) ((rank) == RANK_REJECT)
#define IS_APPLICANT_RANK(rank) ((rank) == RANK_APPLICANT)
#define IS_ALT_RANK(rank) ((rank) > ALT_RANK_OFFSET && (rank) <= ALT_RANK_OFFSET + MAX_CLAN_RANKS)
#define IS_MEMBER_RANK(rank) ((rank) >= RANK_LEADER && (rank) <= MAX_CLAN_RANKS)
#define IS_LEADER_RANK(rank) ((rank) == RANK_LEADER)
#define IS_ADMIN_RANK(rank) ((rank) == RANK_ADMIN)

#define IS_CLAN_REJECT(ch) IS_REJECT_RANK(GET_CLAN_RANK(ch))
#define IS_CLAN_APPLICANT(ch) IS_APPLICANT_RANK(GET_CLAN_RANK(ch))
#define IS_CLAN_MEMBER(ch) IS_MEMBER_RANK(GET_CLAN_RANK(ch))
#define IS_CLAN_ALT(ch) IS_ALT_RANK(GET_CLAN_RANK(ch))
#define IS_CLAN_LEADER(ch) IS_LEADER_RANK(GET_CLAN_RANK(ch))
#define IS_CLAN_ADMIN(ch) IS_ADMIN_RANK(GET_CLAN_RANK(ch))
#define IS_CLAN_SUPERADMIN(ch) PRV_FLAGGED(ch, PRV_CLAN_ADMIN)

#define DEFAULT_APP_LVL 25
#define MAX_CLAN_DESC_LENGTH 5000
#define REJECTION_WAIT_DAYS 5
#define CLAN_SNOOP_OFF 0
#define MAX_CLAN_ABBR_LEN 10
#define MAX_CLAN_NAME_LEN 30
#define MAX_CLAN_TITLE_LEN 30

void init_clans(void);
void save_clans(void);
struct clan *find_clan(const char *id);
struct clan *find_clan_by_abbr(const char *abbr);
struct clan *find_clan_by_number(unsigned int number);
struct clan_membership *find_clan_membership(const char *name);
struct clan_membership *find_clan_membership_in_clan(const char *name, const struct clan *clan);
bool revoke_clan_membership(struct clan_membership *);
void add_clan_membership(struct clan *, struct clan_membership *);
void save_clan(const struct clan *);
void update_clan(struct clan *);
void free_clans(void);
void clan_notification(const struct clan *, const struct char_data *skip, const char *str, ...)
    __attribute__((format(printf, 3, 4)));
void clan_set_title(struct char_data *ch);
struct clan *alloc_clan(void);
void dealloc_clan(struct clan *);
typedef struct clan **clan_iter;
unsigned int days_until_reapply(const struct clan_membership *member);
PRIV_FUNC(clan_admin_check);

clan_iter clans_start(void);
clan_iter clans_end(void);
unsigned int clan_count(void);

struct clan_membership {
    char *name;
    unsigned int rank;
    time_t since;
    union {
        struct clan_membership *alts;
        struct clan_membership *member;
    } relation;
    struct clan_membership *next;
    struct clan *clan;
    struct char_data *player;
};

struct clan_rank {
    char *title;
    flagvector privileges[FLAGVECTOR_SIZE(NUM_CLAN_PRIVS)];
};

struct clan {
    /* Identifying characteristics */
    unsigned int number;
    char *name;
    char *abbreviation;
    char *description;
    char *motd;

    /* Status variables */
    unsigned int dues;
    unsigned int app_fee;
    unsigned int app_level;
    unsigned int power;
    int treasure[NUM_COIN_TYPES];

    /* Lists */
    size_t rank_count;       /* array length */
    struct clan_rank *ranks; /* dynamically-allocated array */

    struct clan_membership *people; /* linked list */
    size_t people_count;            /* cached total number of people */

    struct clan_membership *members; /* start of members in people list */
    size_t member_count;             /* cached number of members */

    struct clan_membership *admins; /* start of admins in people list */
    size_t admin_count;             /* cached number of admins */

    struct clan_membership *applicants; /* start of applicants in people list */
    size_t applicant_count;             /* cached number of applicants */

    struct clan_membership *rejects; /* start of rejects in people list */
    size_t reject_count;             /* cached number of rejects */
};

struct clan_snoop {
    struct clan *clan;
    struct clan_snoop *next;
};

#endif

/***************************************************************************
 * $Log: clan.h,v $
 * Revision 1.13  2009/07/16 22:27:56  myc
 * When someone is made a clan admin, remove them from any clans.
 *
 * Revision 1.12  2009/06/11 01:09:58  myc
 * Allow ctell for alts.
 *
 * Revision 1.11  2009/06/09 05:34:56  myc
 * Completely rewrote all clan-related code.  The structures and
 * functions have changed.
 *
 * Revision 1.10  2008/03/06 05:11:51  myc
 * Combined the 'saved' and 'unsaved' portions of the char_specials and
 * player_specials structures by moving all fields of each saved structure
 * to its parent structure.  Also combined the skills array from the
 * player and mob structures since they are identical.
 *
 * Revision 1.9  2008/02/16 20:26:04  myc
 * Adding functions to free clans at program termination.  Replaced
 * a few RECREATE calls with CREATE calls since zmalloc was complaining
 * about them.
 *
 * Revision 1.8  2007/12/19 20:42:01  myc
 * Completely rewrote clan code.  Clans now use ASCII files.  Several new
 * features are also available, including alts, abbreviations (vs names),
 * descriptions, and smarter handling of just about everything.
 *
 * Revision 1.7  2006/05/07 19:42:43  cjd
 * adjusted level so that L103's have clan command access.
 *
 * Revision 1.6  2004/11/11 18:47:11  cmc
 * fixed warning message "clan.h:45: warning: array type has incomplete element
 *type reordered statements. warning no longer generated.
 *
 * Revision 1.5  2002/09/13 02:32:10  jjl
 * Updated header comments
 *
 * Revision 1.4  2000/11/20 18:46:57  rsd
 * Really added the proper comment header.  Added back rlog
 * messages from prior to the addition of the $log$ string.
 *
 * Revision 1.3  1999/09/05 07:00:39  jimmy
 * Added RCS Log and Id strings to each source file
 *
 * Revision 1.2  1999/01/30 18:55:49  mud
 * Added standard comment header
 *
 * Revision 1.1
 * Initial revision
 *
 ***************************************************************************/

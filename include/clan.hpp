/***************************************************************************
 *  File: clan.h                                          Part of FieryMUD *
 *  Usage: header file for clans                                           *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#pragma once

#include "privileges.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

/* Defaults */
#define ALLOW_CLAN_LINKLOAD TRUE
#define ALLOW_CLAN_ALTS TRUE
#define ALLOW_CLAN_QUIT TRUE
#define BACKUP_CLAN_ON_DELETE TRUE

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

struct clan_rank {
    char *title;
    flagvector privileges[FLAGVECTOR_SIZE(NUM_CLAN_PRIVS)];
};

struct Clan {
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
    size_t rank_count; /* array length */
    clan_rank *ranks;  /* dynamically-allocated array */

    clan_membership *people; /* linked list */
    size_t people_count;     /* cached total number of people */

    clan_membership *members; /* start of members in people list */
    size_t member_count;      /* cached number of members */

    clan_membership *admins; /* start of admins in people list */
    size_t admin_count;      /* cached number of admins */

    clan_membership *applicants; /* start of applicants in people list */
    size_t applicant_count;      /* cached number of applicants */

    clan_membership *rejects; /* start of rejects in people list */
    size_t reject_count;      /* cached number of rejects */
};

struct clan_membership {
    char *name;
    unsigned int rank;
    time_t since;
    union {
        clan_membership *alts;
        clan_membership *member;
    } relation;
    clan_membership *next;
    Clan *clan;
    char_data *player;
};

struct clan_snoop {
    Clan *clan;
    clan_snoop *next;
};

#define GET_CLAN_MEMBERSHIP(ch) ((ch)->player_specials->clan)
#define GET_CLAN(ch) (GET_CLAN_MEMBERSHIP(ch) ? GET_CLAN_MEMBERSHIP(ch)->clan : NULL)
#define GET_CLAN_RANK(ch) (GET_CLAN_MEMBERSHIP(ch) ? GET_CLAN_MEMBERSHIP(ch)->rank : RANK_NONE)
#define GET_CLAN_TITLE(ch) (IS_CLAN_MEMBER(ch) ? GET_CLAN(ch)->ranks[GET_CLAN_RANK(ch) - 1].title : NULL)
#define MEMBER_CAN(member, priv)                                                                                       \
    (IS_MEMBER_RANK(member->rank) && IS_FLAGGED(member->clan->ranks[member->rank - 1].privileges, (priv)))
#define HAS_CLAN_PRIV(ch, priv) (GET_CLAN_MEMBERSHIP(ch) ? MEMBER_CAN(GET_CLAN_MEMBERSHIP(ch), (priv)) : FALSE)
#define CAN_DO_PRIV(ch, priv) (IS_CLAN_ADMIN(ch) || IS_CLAN_SUPERADMIN(ch) || HAS_CLAN_PRIV((ch), (priv)))
#define GET_CLAN_SNOOP(ch) ((ch)->player_specials->clan_snoop)

void init_clans(void);
void save_clans(void);
Clan *find_clan(const char *id);
Clan *find_clan_by_abbr(const char *abbr);
Clan *find_clan_by_number(unsigned int number);
clan_membership *find_clan_membership(const char *name);
clan_membership *find_clan_membership_in_clan(const char *name, const Clan *clan);
bool revoke_clan_membership(clan_membership *);
void add_clan_membership(Clan *, clan_membership *);
void save_clan(const Clan *);
void update_clan(Clan *);
void free_clans(void);
void clan_notification(const Clan *, const char_data *skip, const char *str, ...) __attribute__((format(printf, 3, 4)));
void clan_set_title(char_data *ch);
Clan *alloc_clan(void);
void dealloc_clan(Clan *);
typedef Clan **clan_iter;
unsigned int days_until_reapply(const clan_membership *member);
PRIV_FUNC(clan_admin_check);

clan_iter clans_start(void);
clan_iter clans_end(void);
unsigned int clan_count(void);

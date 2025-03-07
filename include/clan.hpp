/***************************************************************************
 *  File: clan.h                                          Part of FieryMUD *
 *  Usage: header file for clans                                           *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#pragma once

#include "money.hpp"
#include "privileges.hpp"
#include "structs.hpp"
#include "sysdep.hpp"

#include <fmt/format.h>
#include <memory>
#include <string>
#include <vector>

/* Defaults */
constexpr bool ALLOW_CLAN_LINKLOAD = true;
constexpr bool ALLOW_CLAN_ALTS = true;
constexpr bool ALLOW_CLAN_QUIT = true;
constexpr bool BACKUP_CLAN_ON_DELETE = true;

enum ClanPrivilege {
    CPRIV_DESC,
    CPRIV_MOTD,
    CPRIV_GRANT,
    CPRIV_RANKS,
    CPRIV_TITLE,
    CPRIV_ENROLL,
    CPRIV_EXPEL,
    CPRIV_PROMOTE,
    CPRIV_DEMOTE,
    CPRIV_APP_FEE,
    CPRIV_APP_LEV,
    CPRIV_DUES,
    CPRIV_WITHDRAW,
    CPRIV_ALTS,
    CPRIV_CHAT,
};

constexpr int MIN_CLAN_RANKS = 2;
constexpr int MAX_CLAN_RANKS = 100;
constexpr int RANK_ADMIN = 0;
constexpr int RANK_LEADER = 1;
constexpr int ALT_RANK_OFFSET = MAX_CLAN_RANKS;
constexpr int MAX_ALT_RANK = ALT_RANK_OFFSET + RANK_LEADER;
constexpr int MIN_ALT_RANK = ALT_RANK_OFFSET + MAX_CLAN_RANKS;
constexpr int RANK_APPLICANT = ALT_RANK_OFFSET + MAX_CLAN_RANKS + 1;
constexpr int RANK_REJECT = RANK_APPLICANT + 1;
constexpr int RANK_NONE = RANK_REJECT + 1;

constexpr int DEFAULT_APP_LVL = 25;
constexpr int MAX_CLAN_DESC_LENGTH = 5000;
constexpr int REJECTION_WAIT_DAYS = 5;
constexpr int CLAN_SNOOP_OFF = 0;
constexpr int MAX_CLAN_ABBR_LEN = 10;
constexpr int MAX_CLAN_NAME_LEN = 30;
constexpr int MAX_CLAN_TITLE_LEN = 30;

struct ClanMembership;
struct ClanRank {
    std::string title;
    flagvector privileges[FLAGVECTOR_SIZE(NUM_CLAN_PRIVS)];
};

struct Clan {
    /* Identifying characteristics */
    unsigned int number;
    std::string name;
    std::string abbreviation;
    std::string description;
    std::string motd;

    /* Status variables */
    unsigned int dues;
    unsigned int app_fee;
    unsigned int app_level;
    Money treasure;

    /* Lists */
    size_t rank_count;           /* array length */
    std::vector<ClanRank> ranks; /* dynamically-allocated array */

    std::vector<ClanMembership> members;
};

struct ClanMembership {
    CharData *player;
    std::string name;
    std::weak_ptr<Clan> clan;
    unsigned int rank;
    time_t since;
    union {
        ClanMembership *alts;
        ClanMembership *member;
    } relation;
};

/***************************************************************************
 * Clan Constants
 ***************************************************************************/

static std::vector<std::shared_ptr<Clan>> clans;

constexpr struct {
    const std::string_view abbr;
    bool default_on;
    const std::string_view desc;
} clan_privileges[NUM_CLAN_PRIVS] = {
    {"desc", false, "Change Description"},
    {"motd", false, "Change Message of the Day"},
    {"grant", false, "Grant Privilege"},
    {"ranks", false, "Change Ranks"},
    {"title", false, "Change Titles"},
    {"enroll", false, "Enroll"},
    {"expel", false, "Expel"},
    {"promote", false, "Promote"},
    {"demote", false, "Demote"},
    {"appfee", false, "Change Application Fee"},
    {"applev", false, "Change Application Level"},
    {"dues", false, "Change Dues"},
    {"withdraw", false, "Withdraw"},
    {"alts", true, "Alts"},
    {"chat", true, "Chat"},
};

#define GET_CLAN_MEMBERSHIP(ch) ((ch)->player_specials->clan_memberships)
#define GET_CLAN(ch) (GET_CLAN_MEMBERSHIP(ch) ? GET_CLAN_MEMBERSHIP(ch)->clan : nullptr)
#define GET_CLAN_RANK(ch) (GET_CLAN_MEMBERSHIP(ch) ? GET_CLAN_MEMBERSHIP(ch)->rank : RANK_NONE)
#define GET_CLAN_TITLE(ch) (IS_CLAN_MEMBER(ch) ? GET_CLAN(ch)->ranks[GET_CLAN_RANK(ch) - 1].title : "")
#define MEMBER_CAN(member, priv)                                                                                       \
    (IS_MEMBER_RANK(member->rank) && IS_FLAGGED(member->clan->ranks[member->rank - 1].privileges, (priv)))
#define HAS_CLAN_PRIV(ch, priv) (GET_CLAN_MEMBERSHIP(ch) ? MEMBER_CAN(GET_CLAN_MEMBERSHIP(ch), (priv)) : false)
#define CAN_DO_PRIV(ch, priv) (IS_CLAN_ADMIN(ch) || IS_CLAN_SUPERADMIN(ch) || HAS_CLAN_PRIV((ch), (priv)))

void init_clans(void);
void save_clans(void);
Clan *find_clan(const std::string_view id);
Clan *find_clan_by_abbr(const std::string_view abbr);
Clan *find_clan_by_number(unsigned int number);
ClanMembership *find_clan_membership(const std::string_view name);
ClanMembership *find_clan_membership_in_clan(const std::string_view name, const std::shared_ptr<Clan> clan);
bool revoke_clan_membership(ClanMembership *);
void add_clan_membership(Clan *, ClanMembership *);
void save_clan(const Clan *);
void update_clan(Clan *);
template <typename... Args>
void clan_notification(std::shared_ptr<Clan> clan, CharData *skip, fmt::string_view str, Args &&...args) {
    clan_notification(clan, skip, fmt::vformat(str, fmt::make_format_args(args...)));
}
void clan_set_title(CharData *ch);
Clan *alloc_clan(void);
void dealloc_clan(Clan *);
unsigned int days_until_reapply(const ClanMembership *member);
PRIV_FUNC(clan_admin_check);

unsigned int clan_count(void);

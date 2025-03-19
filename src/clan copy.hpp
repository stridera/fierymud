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

#include <bitset>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <vector>

enum ClanPrivilege {
    CPRIV_NONE,
    CPRIV_DESC = 1 << 0,
    CPRIV_MOTD = 1 << 1,
    CPRIV_GRANT = 1 << 2,
    CPRIV_RANKS = 1 << 3,
    CPRIV_TITLE = 1 << 4,
    CPRIV_ENROLL = 1 << 5,
    CPRIV_EXPEL = 1 << 6,
    CPRIV_PROMOTE = 1 << 7,
    CPRIV_DEMOTE = 1 << 8,
    CPRIV_APP_FEES = 1 << 9,
    CPRIV_APP_LEV = 1 << 10,
    CPRIV_DUES = 1 << 11,
    CPRIV_WITHDRAW = 1 << 12,
    CPRIV_ALTS = 1 << 13,
    CPRIV_CHAT = 1 << 14,

    CPRIV_SIZE = 15
};
constexpr size_t NUM_CLAN_PRIVS = static_cast<size_t>(CPRIV_SIZE);
typedef std::bitset<NUM_CLAN_PRIVS> ClanPrivilegeSet;
struct ClanMembership;

struct ClanRank {
    std::string title;
    flagvector privileges[FLAGVECTOR_SIZE(NUM_CLAN_PRIVS)];
};

struct Clan {
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
    std::vector<ClanRank> ranks;
    std::vector<ClanMembership> members;
};

class ClanMembership {
    CharData *player;
    std::string name;
    std::weak_ptr<Clan> clan; // Avoid circular reference
    unsigned int rank;
    time_t since;
    union {
        ClanMembership *alts;
        ClanMembership *member;
    } relation;

  public:
    ClanMembership(CharData *ch, std::shared_ptr<Clan> clan, unsigned int rank)
        : player(ch), clan(clan), rank(rank), since(time(0)) {
        name = ch->player.short_descr;
    }
    inline std::string rank_title() const { return clan.lock()->ranks[rank].title; }
    inline std::string clan_name() const { return clan.lock()->name; }
    inline std::string clan_abbr() const { return clan.lock()->abbreviation; }
    inline std::string clan_desc() const { return clan.lock()->description; }
};

// Target Group.  This is used to determine who can use a command.
enum TargetGroup { TARGET_ADMIN = 0, TARGET_MEMBER = 1 << 0, TARGET_ANYONE = ~0ULL };
struct ClanCmd {
    std::string_view name;
    void (*func)(CharData *, Arguments);
    TargetGroup target; // Minimum rank required to use the command
    ClanPrivilegeSet privileges;
};
static std::vector<std::shared_ptr<Clan>> clans;


/***************************************************************************
 * Clan Defaults
 ***************************************************************************/

constexpr bool ALLOW_CLAN_ALTS = true;
constexpr bool BACKUP_CLAN_ON_DELETE = true;

constexpr int DEFAULT_APP_LVL = 25;
constexpr int MAX_CLAN_DESC_LENGTH = 5000;
constexpr int REJECTION_WAIT_DAYS = 5;
constexpr int CLAN_SNOOP_OFF = 0;
constexpr int MAX_CLAN_ABBR_LEN = 10;
constexpr int MAX_CLAN_NAME_LEN = 30;
constexpr int MAX_CLAN_TITLE_LEN = 30;

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

unsigned int clan_count(void);

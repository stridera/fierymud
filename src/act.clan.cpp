#include "clan.hpp"

#define CLANCMD(name) static void(name)(CharData * ch, Arguments argument)
// Macro for free functions (outside of class)
#define REGISTER_FREE_FUNCTION(func_name, priority, ...)                                                               \
    void func_name(__VA_ARGS__);                                                                                       \
    inline static const FunctionEntry<&func_name> func_name##_entry{#func_name, priority};                             \
    void func_name(__VA_ARGS__)

CLANCMD(clan_list) {
    if (clan_count() == 0) {
        char_printf(ch, "No clans have formed yet.\n");
        return;
    }

    /* List clans, # of members, power, and app fee */
    paging_printf(ch, AUND " Num   Clan           Members  App Fee/Lvl\n" ANRM);
    for (const auto &clan : clans) {
        paging_printf(ch, fmt::format("[{:3}] {:<{}} {:3}   {:5}p/{:3}\n", clan->number, clan->abbreviation,
                                      18 + count_color_chars(clan->abbreviation) * 2, clan->members.size(),
                                      clan->app_fee, clan->app_level));
    }

    start_paging(ch);
}

CLANCMD(clan_bank) {
    bool deposit;
    std::string_view verb, preposition;

    auto arg = argument.shift();
    deposit = matches_start(arg, "deposit");
    verb = deposit ? "deposit" : "withdraw";
    preposition = deposit ? "into" : "from";

    auto money_opt = parse_money(argument.get());
    if (!money_opt || CASH_VALUE(*money_opt) <= 0) {
        char_printf(ch, "How much do you want to {}?\n", verb);
        return;
    }

    auto coins = *money_opt;

    if (deposit) {
        /* Gods have bottomless pockets */
        if (GET_LEVEL(ch) < LVL_GOD) {
            if (GET_PLATINUM(ch) < coins[PLATINUM] || GET_GOLD(ch) < coins[GOLD] || GET_SILVER(ch) < coins[SILVER] ||
                GET_COPPER(ch) < coins[COPPER]) {
                char_printf(ch, "You do not have that kind of money!\n");
                return;
            }

            GET_PLATINUM(ch) -= coins[PLATINUM];
            GET_GOLD(ch) -= coins[GOLD];
            GET_SILVER(ch) -= coins[SILVER];
            GET_COPPER(ch) -= coins[COPPER];
            save_player_char(ch);
        }
    } else {
        if (clan->treasure[PLATINUM] < coins[PLATINUM] || clan->treasure[GOLD] < coins[GOLD] ||
            clan->treasure[SILVER] < coins[SILVER] || clan->treasure[COPPER] < coins[COPPER]) {
            char_printf(ch, "The clan is not wealthy enough for your needs!\n");
            return;
        }
        GET_PLATINUM(ch) += coins[PLATINUM];
        GET_GOLD(ch) += coins[GOLD];
        GET_SILVER(ch) += coins[SILVER];
        GET_COPPER(ch) += coins[COPPER];
        save_player_char(ch);
    }

    char_printf(ch, "You {} {} {}'s account: {}\n", verb, preposition, clan->abbreviation, statemoney(coins));

    if (deposit) {
        clan->treasure[PLATINUM] += coins[PLATINUM];
        clan->treasure[GOLD] += coins[GOLD];
        clan->treasure[SILVER] += coins[SILVER];
        clan->treasure[COPPER] += coins[COPPER];
    } else {
        clan->treasure[PLATINUM] -= coins[PLATINUM];
        clan->treasure[GOLD] -= coins[GOLD];
        clan->treasure[SILVER] -= coins[SILVER];
        clan->treasure[COPPER] -= coins[COPPER];
    }
    save_clan(clan);
}

static bool is_snooping(CharData *ch, const Clan *clan) {
    ClanSnoop *snoop;

    for (snoop = GET_CLAN_SNOOP(ch); snoop; snoop = snoop->next)
        if (snoop->clan == clan)
            return true;

    return false;
}

CLANCMD(clan_tell) {
    DescriptorData *d;
    CharData *tch;
    CharData *me = REAL_CHAR(ch);

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        char_printf(ch, "Your lips move, but no sound forms.\n");
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    if (argument.empty()) {
        char_printf(ch, "What do you want to tell the clan?\n");
        return;
    }

    auto speech = drunken_speech(argument.get(), GET_COND(ch, DRUNK));

    char_printf(ch, AFMAG "You tell {}" AFMAG ", '" AHMAG "{}" AFMAG "'\n" ANRM,
                member ? "your clan" : clan->abbreviation, speech);

    for (d = descriptor_list; d; d = d->next) {
        if (!IS_PLAYING(d) || !d->character)
            continue;
        tch = REAL_CHAR(d->character);
        if (!tch || tch == me)
            continue;
        if (STATE(d) != CON_PLAYING || PLR_FLAGGED(tch, PLR_WRITING) || PLR_FLAGGED(tch, PLR_MAILING) || EDITING(d))
            if (!PRF_FLAGGED(tch, PRF_OLCCOMM))
                continue;
        if (PRF_FLAGGED(tch, PRF_NOCLANCOMM))
            continue;
        if ((IS_CLAN_SUPERADMIN(tch) && is_snooping(tch, clan)) ||
            (GET_CLAN(tch) == clan && !OUTRANKS(MIN_ALT_RANK, GET_CLAN_RANK(tch))))
            char_printf(FORWARD(tch), AFMAG "{} tells {}" AFMAG ", '" AHMAG "{}" AFMAG "'\n" ANRM,
                        GET_INVIS_LEV(me) > GET_LEVEL(tch) ? "Someone" : GET_NAME(me),
                        member && !IS_CLAN_SUPERADMIN(tch) ? "your clan" : clan->abbreviation, speech);
    }
}

CLANCMD(clan_set) {
    auto cmd = argument.get();

    if (matches_start(cmd, "abbr")) {
        std::string old_abbr{clan->abbreviation};
        auto abbreviation = argument.get();
        if (ansi_strlen(abbreviation) > MAX_CLAN_ABBR_LEN) {
            char_printf(ch, "Clan abbreviationss may be at most {} characters in length.\n", MAX_CLAN_ABBR_LEN);
            return;
        }
        clan->abbreviation = {}; /* so find_clan doesn't find this clan */
        if (find_clan(abbreviation)) {
            char_printf(ch, "A clan with that name or abbreviation already exists!\n");
            clan->abbreviation = old_abbr; /* revert */
            return;
        }
        clan->abbreviation = abbreviation;
        char_printf(ch, "{} is now abbreviated {}.\n", clan->name, clan->abbreviation);
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} changes {}'s to {}", GET_NAME(ch), clan->name, clan->abbreviation);
    } else if (matches_start(cmd, "addrank")) {
        if (clan->rank_count >= MAX_CLAN_RANKS) {
            char_printf(ch, "{} already has the maximum number of ranks.\n", clan->name);
            return;
        }

        ++clan->rank_count;
        clan->ranks = std::make_unique<ClanRank[]>(clan->rank_count);
        clan->ranks[clan->rank_count - 1].title = "Member";
        for (int i = 0; i < magic_enum::enum_count<ClanPrivilege>(); ++i)
            if (clan_privileges[i].default_on)
                SET_FLAG(clan->ranks[clan->rank_count - 1].privileges, i);
            else
                REMOVE_FLAG(clan->ranks[clan->rank_count - 1].privileges, i);

        char_printf(ch, "You add a new rank ({}) to {}.\n", clan->rank_count, clan->name);
        clan_notification(clan, ch, "{} adds a new rank to your clan.", GET_NAME(ch));
    } else if (matches_start(cmd, "appfee")) {
        auto fee = argument.try_shift_number();
        if (!fee) {
            char_printf(ch, "How much platinum should the clan's application fee be?\n");
            return;
        }
        clan->app_fee = *fee;
        char_printf(ch, "{}'s application fee is now {} platinum.\n", clan->name, clan->app_fee);
    } else if (matches_start(cmd, "applev")) {
        auto level_opt = argument.try_shift_number();
        if (!level_opt) {
            char_printf(ch, "What should the clan's minimum application level be?\n");
            return;
        }
        auto level = *level_opt;
        if (level < 1 || level > LVL_IMPL) {
            char_printf(ch, "The minimum application level must be between 1 and {}.\n", LVL_IMPL);
            return;
        }
        clan->app_level = level;
        char_printf(ch, "{}'s minimum application level is now {}.\n", clan->name, clan->app_level);
    } else if (matches_start(cmd, "delrank")) {
        if (clan->rank_count <= MIN_CLAN_RANKS) {
            char_printf(ch, "{} already has the minimum number of ranks.\n", clan->name);
            return;
        }

        --clan->rank_count;

        char_printf(ch, "You remove a rank ({}) from {}.\n", clan->rank_count + 1, clan->name);
        clan_notification(clan, ch, "{} removes a rank from your clan.", GET_NAME(ch));

        for (member = clan->members; member; member = member->next)
            if (member->rank == clan->rank_count + 1) {
                --member->rank;
                if (member->player)
                    char_printf(FORWARD(member->player),
                                AFMAG "You have been automatically promoted to rank {}.\n" ANRM, member->rank);
            }
    } else if (matches_start(cmd, "dues")) {
        auto dues_opt = argument.try_shift_number();
        if (!dues_opt) {
            char_printf(ch, "How much platinum should the clan's dues be?\n");
            return;
        }
        clan->dues = *dues_opt;
        char_printf(ch, "{}'s monthly dues are now {} platinum.\n", clan->name, clan->dues);
    } else if (matches_start(cmd, "name")) {
        std::string_view old_name = clan->name;
        auto new_name = argument.get();
        if (ansi_strlen(new_name) > MAX_CLAN_NAME_LEN) {
            char_printf(ch, "Clan names may be at most {} characters in length.\n", MAX_CLAN_NAME_LEN);
            return;
        }
        clan->name = {}; /* so find_clan doesn't find this clan */
        if (find_clan(new_name)) {
            char_printf(ch, "A clan with that name already exists!\n");
            clan->name = old_name; /* revert */
            return;
        }
        clan->name = new_name;
        char_printf(ch, "{} is now named {}.\n", old_name, clan->name);
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} renames {} to {}", GET_NAME(ch), old_name, clan->name);
    } else if (matches_start(cmd, "title")) {
        auto rank_opt = argument.try_shift_number();
        if (!rank_opt) {
            char_printf(ch, "For which rank do you want to set a title?\n");
            return;
        }
        auto rank = *rank_opt;
        if (rank < 1 || rank > clan->rank_count) {
            char_printf(ch, "'{}' is an invalid rank.  Valid ranks are 1-{}.\n", rank, clan->rank_count);
            return;
        }
        if (!IS_CLAN_SUPERADMIN(ch) && !IS_CLAN_ADMIN(ch) && OUTRANKS(rank, GET_CLAN_RANK(ch))) {
            char_printf(ch, "You cannot set the title for a rank above your own.\n");
            return;
        }
        auto title = argument.get();
        if (ansi_strlen(title) > MAX_CLAN_TITLE_LEN) {
            char_printf(ch, "Clan titles may be at most {} characters long.\n", MAX_CLAN_TITLE_LEN);
            return;
        }
        clan->ranks[rank - 1].title = title;
        char_printf(ch, "Rank {}'s title is now: {}\n", rank, title);
        clan_notification(clan, ch, "{} has changed rank %u's title to {}.", GET_NAME(ch), rank, title);
    } else {
        log(LogSeverity::Error, LVL_GOD, "SYSERR: clan_set: unknown subcommand '{}'", cmd);
        return;
    }

    save_clan(clan);
}

CLANCMD(clan_alt) {
    CharData *tch;
    ClanMembership *alt;

    auto target = argument.shift();

    if (target.empty()) {
        char_printf(ch, "Whom do you want to add or remove as an alt?\n");
        return;
    }

    /*
     * First, let's see if we're trying to remove an alt: they don't have
     * to be online for that.
     */
    for (alt = member->relation.alts; alt; alt = alt->next) {
        if (matches(alt->name, target)) {
            char_printf(ch, "You remove {} as one of {}{} clan alts.\n", alt->name,
                        ch == member->player ? "your" : member->name, ch == member->player ? "" : "'s");
            if (ch != member->player && member->player)
                char_printf(FORWARD(member->player), AFMAG "{} removes {} as one of your clan alts.\n" ANRM,
                            GET_NAME(ch), alt->name);
            if (alt->player)
                char_printf(FORWARD(alt->player), AFMAG "You are no longer one of {}'s clan alts.\n" ANRM,
                            member->name);
            revoke_clan_membership(alt);
            return;
        }
    }

    if (!(tch = find_char_by_desc(find_vis_by_name(ch, target))))
        char_printf(ch, "There's no one online by the name of {}.\n", target);
    else if (ch == tch)
        char_printf(ch, "You want to be your own alt?\n");
    else if (GET_CLAN_MEMBERSHIP(tch))
        char_printf(ch, "{} is already in a clan!\n", GET_NAME(tch));
    else if (!IS_CLAN_SUPERADMIN(ch) && !IS_CLAN_ADMIN(ch) && strcasecmp(ch->desc->host, tch->desc->host))
        char_printf(ch, "{} was not found logged in as your alt.\n", GET_NAME(tch));
    else if (IS_CLAN_SUPERADMIN(tch))
        char_printf(ch, "{} is already a clan super-admin!\n", GET_NAME(tch));
    else {
        char_printf(ch, "You make {} one of {}{} clan alts.\n", GET_NAME(tch),
                    ch == member->player ? "your" : member->name, ch == member->player ? "" : "'s");
        char_printf(tch, AFMAG "{} makes you one of {}{} clan alts.\n" ANRM, GET_NAME(ch),
                    ch == member->player ? HSHR(member->player) : member->name, ch == member->player ? "" : "'s");
        if (ch != member->player && member->player)
            char_printf(FORWARD(member->player), AFMAG "{} makes {} one of your clan alts.\n" ANRM, GET_NAME(ch),
                        GET_NAME(tch));
        CREATE(alt, ClanMembership, 1);
        alt->name = GET_NAME(tch);
        alt->rank = ALT_RANK_OFFSET + member->rank;
        alt->since = member->since;
        alt->relation.member = member;
        member->relation.alts = alt;
        alt->player = tch;
        GET_CLAN_MEMBERSHIP(tch) = alt;
        save_player_char(tch);
        save_clan(clan);
    }
}

CLANCMD(clan_apply) {
    if (GET_LEVEL(ch) < LVL_GOD && GET_PLATINUM(ch) < clan->app_fee) {
        char_printf(ch, "You don't have enough money to cover the {} platinum application fee.\n", clan->app_fee);
        return;
    }

    if (GET_LEVEL(ch) < clan->app_level) {
        char_printf(ch, "{} does not accept players beneath level {}.\n", clan->name, clan->app_level);
        return;
    }

    char_printf(ch, "You apply to {}.\n", clan->name);

    if (GET_LEVEL(ch) < LVL_GOD) {
        GET_PLATINUM(ch) -= clan->app_fee;
        clan->treasure[PLATINUM] += clan->app_fee;
    }

    clan_notification(clan, nullptr, "{} has applied to your clan.", GET_NAME(ch));

    CREATE(member, ClanMembership, 1);
    member->name = GET_NAME(ch);
    member->rank = RANK_APPLICANT;
    member->since = time(0);
    member->player = ch;
    GET_CLAN_MEMBERSHIP(ch) = member;
    add_clan_membership(clan, member);
    save_player_char(ch);
    save_clan(clan);
}

CLANCMD(clan_create) {
    unsigned int i = 0;

    auto abbreviation = argument.get();

    if (abbreviation.empty())
        char_printf(ch, "What is the abbreviation for the new clan?\n");
    else if (ansi_strlen(abbreviation) > 10)
        char_printf(ch, "Clan abbreviations can be at most 10 visible characters long.\n");
    else if (find_clan_by_abbr(strip_ansi(abbreviation)))
        char_printf(ch, "A clan with a similar abbreviation already exists.\n");
    else {
        clan = alloc_clan();
        clan->name = abbreviation;
        clan->abbreviation = abbreviation;
        clan->description.clear();
        clan->motd.clear();

        clan->dues = 0;
        clan->app_fee = 0;
        clan->app_level = 0;
        for (i = 0; i < NUM_COIN_TYPES; ++i)
            clan->treasure[i] = 0;

        clan->rank_count = 2;
        clan->ranks = std::make_unique<ClanRank[]>(clan->rank_count);
        clan->ranks[0].title = "Leader";
        clan->ranks[1].title = "Member";
        for (i = 0; i < NUM_CLAN_PRIVS; ++i) {
            SET_FLAG(clan->ranks[0].privileges, i);
            if (clan_privileges[i].default_on)
                SET_FLAG(clan->ranks[1].privileges, i);
        }

        char_printf(ch, "New clan {} created.\n", abbreviation);
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} creates new clan: {}", GET_NAME(ch), abbreviation);

        save_clan(clan);
    }
}

CLANCMD(update_clan_rank) {
    unsigned int rank;
    std::string_view action;

    auto arg = argument.shift();

    if (matches_start(arg, "demote")) {
        if (!IS_CLAN_SUPERADMIN(ch) && ch != member->player && !OUTRANKS(GET_CLAN_RANK(ch), member->rank))
            char_printf(ch, "You cannot demote someone at or above your rank.\n");
        else if (member->rank == clan->rank_count)
            char_printf(ch, "{} is already the minimum rank.\n", member->name);
        else if (!IS_MEMBER_RANK(member->rank))
            char_printf(ch, "{} isn't a clan member.\n", member->name);
        else {
            rank = member->rank + 1;
            action = "demote";
        }
    } else if (matches_start(arg, "promote")) {
        if (!IS_CLAN_SUPERADMIN(ch) && !OUTRANKS(GET_CLAN_RANK(ch), member->rank))
            char_printf(ch, "You cannot promote someone at or above your rank.\n");
        else if (member->rank == RANK_LEADER)
            char_printf(ch, "{} is already the maximum rank.\n", member->name);
        else if (!IS_MEMBER_RANK(member->rank))
            char_printf(ch, "{} isn't a clan member.\n", member->name);
        else {
            rank = member->rank - 1;
            action = "promote";
        }
    } else {
        log("SYSERR: update_clan_rank: invalid subcommand '{}'", arg);
        return;
    }

    /* action only gets set if all checks above were successful. */
    if (action.empty())
        return;

    if (ch == member->player)
        char_printf(ch, "You {} yourself to rank {}: {}\n", action, rank, clan->ranks[rank - 1].title);
    else {
        if (member->player)
            char_printf(FORWARD(member->player), AFMAG "{} has {}d you to rank {}: " ANRM "{}\n", GET_NAME(ch), action,
                        rank, clan->ranks[rank - 1].title);
        char_printf(ch, "You {} {} to rank {}: {}\n", action, member->name, rank, clan->ranks[rank - 1].title);
    }
    member->rank = RANK_NONE; /* Temporary so they don't get the notification */
    clan_notification(clan, ch, "{} has {}d {} to rank %u: " ANRM "{}", GET_NAME(ch), action, member->name, rank,
                      clan->ranks[rank - 1].title);
    member->rank = rank;
    /* Shift alts' ranks too */
    for (member = member->relation.alts; member; member = member->next)
        member->rank = rank + ALT_RANK_OFFSET;
    save_clan(clan);
}

CLANCMD(clan_destroy) {
    clan_notification(clan, ch, "Your clan has been disbanded!");
    char_printf(ch, AFMAG "You have deleted the clan {}.\n" ANRM, clan->name);
    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} has destroyed the clan {}.", GET_NAME(ch), clan->name);
    dealloc_clan(clan);
}

CLANCMD(clan_enroll) {
    int num;

    member->since = time(0);
    member->rank = clan->member_count ? clan->rank_count : RANK_LEADER;

    update_clan(clan);
    save_clan(clan);

    char_printf(ch, "You {} {} {} {}.\n", member->rank == RANK_LEADER ? "appoint" : "enroll", member->name,
                member->rank == RANK_LEADER ? "the leader of" : "in", clan->name);
    if (member->player)
        char_printf(FORWARD(member->player), AFMAG "You've been {} {}" AFMAG "!\n" ANRM,
                    member->rank == RANK_LEADER ? "appointed the leader of" : "enrolled in", clan->name);

    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} enrolls {} in {}.", GET_NAME(ch), member->name, clan->name);
}

CLANCMD(clan_expel) {
    std::string_view name = member->name;

    if (!IS_CLAN_SUPERADMIN(ch) && !OUTRANKS(GET_CLAN_RANK(ch), member->rank)) {
        char_printf(ch, "{} outranks you!\n", name);
        return;
    }

    clan = member->clan;

    char_printf(ch, "You expel {} from {}.\n", name, clan->name);

    if (member->player)
        char_printf(FORWARD(member->player), AFMAG "{} has expelled you from {}" AFMAG ".\n" ANRM, GET_NAME(ch),
                    clan->name);

    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} expels {} from {}.", GET_NAME(ch), name, clan->name);
    revoke_clan_membership(member);

    clan_notification(clan, ch, "{} has expelled {} from your clan.", GET_NAME(ch), name);
}

CLANCMD(clan_priv) {
    enum { GRANT, REVOKE } action;
    int rank, priv;

    auto subcommand = argument.shift();

    if (matches_start(subcommand, "grant"))
        action = GRANT;
    else if (matches_start(subcommand, "revoke"))
        action = REVOKE;
    else {
        log("SYSERR: clan_priv: invalid subcommand '{}'", subcommand);
        return;
    }

    auto rank_opt = argument.try_shift_number();

    if (!rank_opt || *rank_opt < RANK_LEADER || *rank_opt > clan->rank_count) {
        char_printf(ch, "'{}' is an invalid rank.  Valid ranks are 1-{}.\n", *rank_opt, clan->rank_count);
        return;
    }
    rank = *rank_opt;
    auto priv_arg = argument.shift();
    for (priv = 0; priv < NUM_CLAN_PRIVS; ++priv)
        if (matches_start(priv_arg, clan_privileges[priv].abbr))
            break;
    if (priv >= NUM_CLAN_PRIVS) {
        char_printf(ch, "'{}' is an invalid privilege.  Valid privileges are listed on clan info.\n", priv_arg);
        return;
    }

    if (!IS_CLAN_SUPERADMIN(ch) && !IS_CLAN_ADMIN(ch) && !HAS_CLAN_PRIV(ch, priv)) {
        char_printf(ch, "You cannot grant or revoke a privilege you do not have!\n");
        return;
    }

    if (IS_CLAN_MEMBER(ch) && !OUTRANKS(GET_CLAN_RANK(ch), rank)) {
        char_printf(ch, "You may only grant or revoke privileges on ranks below yours.\n");
        return;
    }

    if (action == GRANT) {
        if (IS_FLAGGED(clan->ranks[rank - 1].privileges, priv))
            char_printf(ch, "Rank {} already has the {} privilege.\n", rank, clan_privileges[priv].desc);
        else {
            SET_FLAG(clan->ranks[rank - 1].privileges, priv);
            char_printf(ch, "Granted rank {} access to the {} privilege.\n", rank, clan_privileges[priv].desc);
        }
    } else if (action == REVOKE) {
        if (IS_FLAGGED(clan->ranks[rank - 1].privileges, priv)) {
            REMOVE_FLAG(clan->ranks[rank - 1].privileges, priv);
            char_printf(ch, "Revoked rank {} access to the {} privilege.\n", rank, clan_privileges[priv].desc);
        } else
            char_printf(ch, "Rank {} doesn't have the {} privilege.\n", rank, clan_privileges[priv].desc);
    }

    save_clan(clan);
}

static void show_clan_info(CharData *ch, const Clan *clan) {
    const ClanMembership *member = GET_CLAN_MEMBERSHIP(ch);
    size_t i, j;
    bool show_all =
        ((member && member->clan == clan && OUTRANKS(member->rank, RANK_APPLICANT)) || IS_CLAN_SUPERADMIN(ch));

    std::string title = fmt::format("[ Clan {}: {} ]", clan->number, clan->name);
    paging_printf(ch, "{:-^70}\n", title);

    paging_printf(ch,
                  "Nickname: " AFYEL "{}" ANRM
                  "  "
                  "Ranks: " AFYEL "{}" ANRM
                  "  "
                  "Members: " AFYEL "{}" ANRM
                  "  "
                  "Power: " AFYEL "{}" ANRM
                  "\n"
                  "Applicants: " AFYEL "{}" ANRM
                  "  "
                  "App Fee: " AFCYN "{}" ANRM
                  "  "
                  "App Level: " AFYEL "{}" ANRM
                  "  "
                  "Dues: " AFCYN "{}" ANRM "\n",
                  clan->abbreviation, clan->rank_count, clan->member_count, clan->power, clan->applicant_count,
                  clan->app_fee, clan->app_level, clan->dues);

    if (show_all) {
        paging_printf(ch, "Treasure: {}\n", statemoney(clan->treasure));

        paging_printf(ch, "\nRanks:\n");
        for (i = 0; i < clan->rank_count; ++i)
            paging_printf(ch, "{:3}  {}\n", i + 1, clan->ranks[i].title);

        paging_printf(ch, "\nPrivileges:\n");
        for (j = 1; j <= clan->rank_count; ++j)
            paging_printf(ch, "{:3}", j);
        for (i = 0; i < NUM_CLAN_PRIVS; ++i) {
            paging_printf(ch, "\n{:<9}", clan_privileges[i].abbr);
            for (j = 0; j < clan->rank_count; ++j)
                paging_printf(ch, "  {}{}" ANRM, IS_FLAGGED(clan->ranks[j].privileges, i) ? AFGRN : AFRED,
                              IS_FLAGGED(clan->ranks[j].privileges, i) ? 'Y' : 'N');
        }
        paging_printf(ch, "\n");
    }

    if (!clan->description.empty())
        paging_printf(ch, "\nDescription:\n{}", clan->description);

    if (show_all)
        if (!clan->motd.empty())
            paging_printf(ch, "\nMessage of the Day:\n{}", clan->motd);

    start_paging(ch);
}

CLANCMD(clan_info) {
    auto clan_name = argument.get();

    if (clan_name.empty()) {
        if (clan)
            show_clan_info(ch, clan);
        else
            char_printf(ch, "Which clan's info do you want to view?\n");
    } else if ((clan = find_clan(clan_name)))
        show_clan_info(ch, clan);
    else
        char_printf(ch, "'{}' does not refer to a valid clan.\n", clan_name);
}

static void show_clan_member_status(CharData *ch, CharData *tch) {
    if (IS_CLAN_SUPERADMIN(tch))
        char_printf(ch, "{} {} a clan super-administrator.\n", ch == tch ? "You" : GET_NAME(tch),
                    ch == tch ? "are" : "is");
    else if (IS_CLAN_REJECT(tch)) {
        unsigned int days = days_until_reapply(GET_CLAN_MEMBERSHIP(tch));
        char_printf(ch, "{} {} rejected from {} and may re-apply in {:d} day{}.\n", ch == tch ? "You" : GET_NAME(tch),
                    ch == tch ? "were" : "was", GET_CLAN(tch)->name, days, days == 1 ? "" : "s");
    } else if (IS_CLAN_ADMIN(tch))
        char_printf(ch, "{} {} an administrator for {}.\n", ch == tch ? "You" : GET_NAME(tch), ch == tch ? "are" : "is",
                    GET_CLAN(tch)->name);
    else if (IS_CLAN_MEMBER(tch) || IS_CLAN_ALT(tch) || IS_CLAN_APPLICANT(tch)) {
        auto since = std::format("{:%a, %d %b %Y}", std::localtime(&GET_CLAN_MEMBERSHIP(tch)->since));
        paging_printf(ch,
                      "Clan membership status for {}:\n"
                      "  Clan: {}\n",
                      GET_NAME(tch), GET_CLAN(tch)->name);
        if (IS_CLAN_MEMBER(tch))
            paging_printf(ch, "  Rank: {:d} - {}\n", GET_CLAN_RANK(tch), GET_CLAN_TITLE(tch),
                          IS_CLAN_LEADER(tch) ? " (Leader)" : "");
        else if (IS_CLAN_ALT(tch))
            paging_printf(ch, "  Alt rank: {:d} ({})\n", GET_CLAN_RANK(tch) - ALT_RANK_OFFSET,
                          GET_CLAN_MEMBERSHIP(tch)->relation.member->name);
        else if (IS_CLAN_APPLICANT(tch))
            paging_printf(ch, "  Rank: Applicant\n");
        paging_printf(ch, "  Member since: {}\n", since);
        if (IS_CLAN_MEMBER(tch)) {
            if (HAS_FLAGS(GET_CLAN(tch)->ranks[GET_CLAN_RANK(tch) - 1].privileges, NUM_CLAN_PRIVS)) {
                ScreenBuf *sb = new_screen_buf();
                int i, seen = 0;
                const size_t len = strlen("  Privileges: ");
                sb_set_first_indentation(sb, len);
                sb_set_other_indentation(sb, len);
                for (i = 0; i < NUM_CLAN_PRIVS; ++i)
                    if (HAS_CLAN_PRIV(tch, i))
                        sb_append(sb, "{}{}", seen++ ? ", " : "", clan_privileges[i].abbr);
                /* skip over the first 14 spaces (dummy indentation) */
                paging_printf(ch, "  Privileges: {}\n", sb_get_buffer(sb));
                free_screen_buf(sb);
            }
            if (GET_CLAN_MEMBERSHIP(tch)->relation.alts) {
                ClanMembership *alt;
                ScreenBuf *sb = new_screen_buf();
                int seen = 0;
                const size_t len = strlen("  Alts: ");
                sb_set_first_indentation(sb, len);
                sb_set_other_indentation(sb, len);
                for (alt = GET_CLAN_MEMBERSHIP(tch)->relation.alts; alt; alt = alt->next)
                    sb_append(sb, "{}{}", seen++ ? ", " : "", alt->name);
                /* skip over the first 8 spaces (dummy indentation) */
                paging_printf(ch, "  Alts: {}\n", sb_get_buffer(sb));
                free_screen_buf(sb);
            }
        }
        start_paging(ch);
    } else
        char_printf(ch, "{} {} not associated with any clan.\n", ch == tch ? "You" : GET_NAME(tch),
                    ch == tch ? "are" : "is");
}

CLANCMD(clan_status) {
    CharData *tch;

    auto arg = argument.get();

    if (IS_CLAN_SUPERADMIN(ch)) {
        if ((tch = find_char_around_char(ch, find_vis_by_name(ch, arg))))
            show_clan_member_status(ch, tch);
        else
            char_printf(ch, "Couldn't find a player by the name of '{}'.\n", arg);
    } else
        show_clan_member_status(ch, ch);
}

struct ClanEdit {
    Clan *clan;
    char string[20];
};

static EDITOR_FUNC(clan_edit_done) {
    DescriptorData *d = edit->descriptor;
    ClanEdit *data = (ClanEdit *)edit->data;

    if (edit->command == ED_EXIT_SAVE) {
        editor_default_exit(edit);
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} edits {}'s {}", GET_NAME(d->character), data->clan->name,
            data->string);
        save_clan(data->clan);
    }

    act("$n stops writing on the large scroll.", true, d->character, 0, 0, TO_ROOM);

    return ED_PROCESSED;
}

CLANCMD(clan_edit) {
    std::string_view message, editing;
    ClanEdit *data;

    if (!ch->desc)
        return;

    auto arg = argument.get();

    if (matches_start(arg, "motd")) {
        message = clan->motd;
        editing = "message of the day";
    } else if (matches_start(arg, "desc")) {
        message = clan->description;
        editing = "description";
    } else {
        log("SYSECR: E unknown string specified");
        return;
    }

    CREATE(data, ClanEdit, 1);
    data->clan = clan;
    strncpy(data->string, arg.data(), 20);

    if (editor_edited_by(message)) {
        char_printf(ch, "{}'s {} is already being edited.", clan->name, editing);
        return;
    }

    act("$n begins writing on a large scroll.", true, ch, 0, 0, TO_ROOM);

    editor_init(ch->desc, message, MAX_DESC_LENGTH);
    editor_set_begin_string(ch->desc, "Edit {}'s {} below.", clan->name, editing);
    editor_set_callback_data(ch->desc, data, ED_FREE_DATA);
    editor_set_callback(ch->desc, ED_EXIT_SAVE, clan_edit_done);
    editor_set_callback(ch->desc, ED_EXIT_ABORT, clan_edit_done);
}

CLANCMD(clan_quit) {
    if (IS_CLAN_APPLICANT(ch))
        char_printf(ch, "You are no longer applying to {}.\n", clan->name);
    else if (IS_CLAN_ALT(ch))
        char_printf(ch, "You are no longer a clan alt in {}.\n", clan->name);
    else if (IS_CLAN_ADMIN(ch))
        char_printf(ch, "You are no longer an administrator for {}.\n", clan->name);
    else if (IS_CLAN_MEMBER(ch))
        char_printf(ch, "You are no longer a member of {}.\n", clan->name);
    else
        char_printf(ch, "You are no longer in {}.\n", clan->name);
    if (!IS_CLAN_ALT(ch)) {
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} quits {}.", GET_NAME(ch), clan->name);
        clan_notification(clan, ch, "{} has quit your clan.", GET_NAME(ch));
    }
    revoke_clan_membership(GET_CLAN_MEMBERSHIP(ch));
}

CLANCMD(clan_reject) {
    member->since = time(0);
    member->rank = RANK_REJECT;
    --clan->applicant_count;
    ++clan->reject_count;

    save_clan(clan);

    if (member->player)
        char_printf(FORWARD(member->player),
                    AFMAG "You have been rejected from {} and may reapply in {} " AFMAG "days.\n" ANRM,
                    member->clan->name, days_until_reapply(member));

    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} rejects {}'s application to {}.", GET_NAME(ch), member->name,
        member->clan->name);
    char_printf(ch, "You reject {}'s application to {}.\n", member->name, clan->name);
    clan_notification(clan, ch, "{} rejects {}'s application to your clan.", GET_NAME(ch), member->name);
}

CLANCMD(clan_snoop) {
    ClanSnoop *snoop, *temp;

    auto arg = argument.get();

    if (arg.empty()) {
        if (GET_CLAN_SNOOP(ch)) {
            char_printf(ch, "You are currently snooping:\n");
            for (snoop = GET_CLAN_SNOOP(ch); snoop; snoop = snoop->next)
                char_printf(ch, "  {}\n", snoop->clan->name);
        } else
            char_printf(ch, "You are not currently snooping any clan channels.\n");
    } else if (matches(arg, "off")) {
        if (GET_CLAN_SNOOP(ch)) {
            while (GET_CLAN_SNOOP(ch)) {
                snoop = GET_CLAN_SNOOP(ch)->next;
                free(GET_CLAN_SNOOP(ch));
                GET_CLAN_SNOOP(ch) = snoop;
            }
            char_printf(ch, "You are no longer snooping any clan channels.\n");
        } else
            char_printf(ch, "You are not currently snooping any clan channels.\n");
    } else if (matches(arg, "all")) {
        for (const auto &clan : clans)
            if (!is_snooping(ch, &clan)) {
                CREATE(snoop, ClanSnoop, 1);
                snoop->clan = clan;
                snoop->next = GET_CLAN_SNOOP(ch);
                GET_CLAN_SNOOP(ch) = snoop;
            }
        char_printf(ch, "You are now snooping all clan channels.\n");
    } else if ((clan = find_clan(arg))) {
        if (is_snooping(ch, clan)) {
            snoop = GET_CLAN_SNOOP(ch);
            if (snoop->clan == clan) {
                GET_CLAN_SNOOP(ch) = snoop->next;
                free(snoop);
            } else {
                for (; snoop && snoop->next; snoop = snoop->next) {
                    if (snoop->next->clan == clan) {
                        temp = snoop->next;
                        snoop->next = snoop->next->next;
                        free(temp);
                    }
                }
            }
            char_printf(ch, "You are no longer snooping {}.\n", clan->name);
        } else {
            CREATE(snoop, ClanSnoop, 1);
            snoop->clan = clan;
            snoop->next = GET_CLAN_SNOOP(ch);
            GET_CLAN_SNOOP(ch) = snoop;
            char_printf(ch, "You are now snooping {}.\n", clan->name);
        }
    } else
        char_printf(ch, "'{}' does not refer to a valid clan.\n", arg);
}

static void send_clan_who_line(CharData *ch, const ClanMembership *member) {
    long num;
    std::string_view level, title, last_logon, name_color;

    num = get_ptable_by_name(member->name);
    if (num >= 0) {
        level = fmt::format("{}", player_table[num].level);
        last_logon = fmt::format("{:%a, %d %b %Y %H:%M}", localtime(&player_table[num].last));
    } else {
        level = "??";
        last_logon = "";
    }

    if (IS_MEMBER_RANK(member->rank))
        title = member->clan->ranks[member->rank - 1].title;
    else if (IS_APPLICANT_RANK(member->rank))
        title = "(applicant)";
    else if (IS_ALT_RANK(member->rank)) {
        title = fmt::format("({}'s alt)", member->relation.member->name);
    } else
        title = "";

    if (IS_ALT_RANK(member->rank))
        name_color = AFYEL;
    else if (member->player)
        name_color = AFGRN;
    else
        name_color = "";

    char_printf(ch, fmt::format("{:>3s} {}{:15} {:25s} {}\n", level, name_color, member->name, ellipsis(title, 25),
                                last_logon));
}

static void send_clan_who_header(CharData *ch) {
    char_printf(ch,
                "Lvl " ANRM "Name            " ANRM "Rank                    " ANRM "Last Login            " ANRM "\n");
}

CLANCMD(clan_who) {
    DescriptorData *d;
    CharData *tch;
    bool found = false;

    char_printf(ch, AHYEL "Members in " ANRM "{}" AHYEL ":" ANRM "\n", clan->name);

    for (d = descriptor_list; d; d = d->next) {
        if (!IS_PLAYING(d))
            continue;
        tch = d->character;
        if (!CAN_SEE(ch, tch) || clan != GET_CLAN(tch))
            continue;
        if (IS_CLAN_MEMBER(tch) || IS_CLAN_ALT(tch)) {
            if (!found) {
                send_clan_who_header(ch);
                found = true;
            }
            send_clan_who_line(ch, GET_CLAN_MEMBERSHIP(tch));
        }
    }

    for (member = clan->members; member && IS_MEMBER_RANK(member->rank); member = member->next)
        if (!member->player || !member->player->desc) {
            if (!found) {
                send_clan_who_header(ch);
                found = true;
            }
            send_clan_who_line(ch, member);
        }

    if (!clan->member_count)
        char_printf(ch, "  None!\n");

    if (clan->applicant_count) {
        found = false;
        char_printf(ch, AHYEL "\nApplicants to " ANRM "{}" AHYEL ":" ANRM "\n", clan->name);
        for (member = clan->applicants; member && IS_APPLICANT_RANK(member->rank); member = member->next) {
            if (!found) {
                send_clan_who_header(ch);
                found = true;
            }
            send_clan_who_line(ch, member);
        }
    }
}

/* Clan command information structure */
static const struct clan_subcommand {
    const std::string_view name;
    unsigned int group;
    unsigned int type;
    unsigned int data;
    unsigned int args;
    const std::string_view more_args;
    CLANCMD(*handler);
} commands[] = {
    /* KEEP THIS LIST ALPHABETIZED */
    {"abbr", ADMIN, ADMIN, 0, IGNORE | REPEAT, "<name>", clan_set},
    {"abbr", ADMIN, SUPER, 0, CLAN | REPEAT, "<name>", clan_set},
    {"addrank", MODIFY, PRIV | ADMIN, CPRIV_RANKS, IGNORE | REPEAT, "", clan_set},
    {"addrank", MODIFY, SUPER, CPRIV_RANKS, CLAN | REPEAT, "", clan_set},
    {"alt", PRIV, PRIV, CPRIV_ALTS, IGNORE, "<player>", clan_alt},
    {"alt", PRIV, ADMIN | SUPER, 0, MEMBER, "<player>", clan_alt},
    {"appfee", MODIFY, PRIV | ADMIN, CPRIV_APP_FEE, IGNORE | REPEAT, "<platinum>", clan_set},
    {"appfee", MODIFY, SUPER, CPRIV_APP_FEE, CLAN | REPEAT, "<platinum>", clan_set},
    {"applev", MODIFY, PRIV | ADMIN, CPRIV_APP_LEV, IGNORE | REPEAT, "<level>", clan_set},
    {"applev", MODIFY, SUPER, CPRIV_APP_LEV, CLAN | REPEAT, "<level>", clan_set},
    {"apply", GENERAL, NONE, 0, CLAN, "", clan_apply},
    {"create", ADMIN, SUPER, 0, IGNORE, "<abbr>", clan_create},
    {"delrank", MODIFY, PRIV | ADMIN, CPRIV_RANKS, IGNORE | REPEAT, "", clan_set},
    {"delrank", MODIFY, SUPER, CPRIV_RANKS, CLAN | REPEAT, "", clan_set},
    {"demote", MGMT, PRIV | ADMIN | SUPER, CPRIV_DEMOTE, MEMBER | REPEAT, "", update_clan_rank},
    {"deposit", GENERAL, RANK, MIN_ALT_RANK, REPEAT, "<money>", clan_bank},
    {"deposit", GENERAL, SUPER, 0, CLAN | REPEAT, "<money>", clan_bank},
    {"desc", MODIFY, PRIV | ADMIN, CPRIV_DESC, IGNORE | REPEAT, "", clan_edit},
    {"desc", MODIFY, SUPER, CPRIV_DESC, CLAN | REPEAT, "", clan_edit},
    {"destroy", ADMIN, SUPER, 0, CLAN, "", clan_destroy},
    {"dues", MODIFY, PRIV | ADMIN, CPRIV_DUES, IGNORE | REPEAT, "<platinum>", clan_set},
    {"dues", MODIFY, SUPER, CPRIV_DUES, CLAN | REPEAT, "<platinum>", clan_set},
    {"enroll", MGMT, PRIV | ADMIN | SUPER, CPRIV_ENROLL, APPLICANT, "", clan_enroll},
    {"expel", MGMT, PRIV | ADMIN | SUPER, CPRIV_EXPEL, MEMBER, "", clan_expel},
    {"grant", MGMT, PRIV | ADMIN, CPRIV_GRANT, IGNORE | REPEAT, "<rank> <priv>", clan_priv},
    {"grant", MGMT, SUPER, CPRIV_GRANT, CLAN | REPEAT, "<rank> <priv>", clan_priv},
    {"info", GENERAL, RANK, RANK_NONE, IGNORE, "[<clan>]", clan_info},
    {"list", GENERAL, RANK, RANK_NONE, IGNORE, "", clan_list},
    {"motd", MODIFY, PRIV | ADMIN, CPRIV_MOTD, IGNORE | REPEAT, "", clan_edit},
    {"motd", MODIFY, SUPER, CPRIV_MOTD, CLAN | REPEAT, "", clan_edit},
    {"name", ADMIN, ADMIN, 0, IGNORE | REPEAT, "<name>", clan_set},
    {"name", ADMIN, SUPER, 0, CLAN | REPEAT, "<name>", clan_set},
    {"promote", MGMT, PRIV | ADMIN | SUPER, CPRIV_PROMOTE, MEMBER | REPEAT, "", update_clan_rank},
    {"quit", GENERAL, RANK, RANK_APPLICANT, IGNORE, "", clan_quit},
    {"reject", MGMT, PRIV | ADMIN | SUPER, CPRIV_ENROLL, APPLICANT, "", clan_reject},
    {"revoke", MGMT, PRIV | ADMIN, CPRIV_GRANT, IGNORE | REPEAT, "<rank> <privilege>", clan_priv},
    {"revoke", MGMT, SUPER, CPRIV_GRANT, CLAN | REPEAT, "<rank> <privilege>", clan_priv},
    {"snoop", GENERAL, SUPER, 0, IGNORE, "{off | all | <clan>}", clan_snoop},
    {"status", GENERAL, RANK, RANK_REJECT, IGNORE, "", clan_status},
    {"status", GENERAL, SUPER, 0, IGNORE, "[<player>]", clan_status},
    {"tell", GENERAL, PRIV | ADMIN, CPRIV_CHAT, IGNORE, "<message>", clan_tell},
    {"tell", GENERAL, SUPER, CPRIV_CHAT, CLAN, "<message>", clan_tell},
    {"title", MODIFY, PRIV | ADMIN, CPRIV_TITLE, IGNORE | REPEAT, "<rank> <title>", clan_set},
    {"title", MODIFY, SUPER, CPRIV_TITLE, CLAN | REPEAT, "<rank> <title>", clan_set},
    {"who", GENERAL, RANK, MIN_ALT_RANK, IGNORE, "", clan_who},
    {"who", GENERAL, SUPER, 0, CLAN, "", clan_who},
    {"withdraw", PRIV, PRIV | ADMIN, CPRIV_WITHDRAW, REPEAT, "<money>", clan_bank},
    {"withdraw", PRIV, SUPER, 0, CLAN | REPEAT, "<money>", clan_bank},
    {{}, 0, 0, 0, 0, {}, 0},
};

static bool can_use_clan_command(CharData *ch, const clan_subcommand *command) {
    if IS_SET (command->type, NONE)
        if (GET_CLAN_RANK(ch) == RANK_NONE && !IS_CLAN_SUPERADMIN(ch))
            return true;
    if (IS_SET(command->type, RANK))
        if (!OUTRANKS(command->data, GET_CLAN_RANK(ch)))
            return true;
    if (IS_SET(command->type, PRIV))
        if (HAS_CLAN_PRIV(ch, command->data))
            return true;
    if (IS_SET(command->type, ADMIN))
        if (IS_CLAN_ADMIN(ch))
            return true;
    if (IS_SET(command->type, SUPER))
        if (IS_CLAN_SUPERADMIN(ch))
            return true;
    return false;
}

static const clan_subcommand *determine_command(CharData *ch, const std::string_view cmd) {
    const clan_subcommand *command = nullptr;

    for (command = commands; !command->name.empty(); ++command) {
        if (matches_start(cmd, command->name))
            if (can_use_clan_command(ch, command))
                return command;
    }

    return nullptr;
}

ACMD(do_clan) {
    const clan_subcommand *command = nullptr;
    Clan *clan;
    ClanMembership *member;
    unsigned int group;

    if (IS_NPC(ch) || !ch->desc) {
        char_printf(ch, HUH);
        return;
    }

    clan = GET_CLAN(ch);
    member = GET_CLAN_MEMBERSHIP(ch);

    /* Determine which command to invoke */
    auto arg = argument.shift();
    if (arg.length() >= 3)
        if ((command = determine_command(ch, arg))) {
            if (IS_SET(command->args, CLAN)) {
                arg = argument.shift();
                if (arg.empty()) {
                    char_printf(ch, "Which clan?\n");
                    return;
                } else if (!(clan = find_clan(arg))) {
                    char_printf(ch, "'{}' does not refer to a valid clan.\n", arg);
                    return;
                }
            }
            if (IS_SET(command->args, MEMBER | APPLICANT)) {
                arg = capitalize_first(argument.shift());
                if (arg.empty()) {
                    char_printf(ch, "Whom do you want to {}?\n", arg);
                    return;
                } else if (!(member = find_clan_membership(arg))) {
                    char_printf(ch, "{} is not a member or applicant of any clan.\n", arg);
                    return;
                } else if (IS_SET(command->args, APPLICANT) && member->rank != RANK_APPLICANT) {
                    char_printf(ch, "{} is not an applicant of any clan.\n", arg);
                    return;
                } else if (!IS_CLAN_SUPERADMIN(ch) && GET_CLAN(ch) != member->clan) {
                    char_printf(ch, "{} is not a member of your clan.\n", arg);
                    return;
                }
                if (!IS_SET(command->args, CLAN))
                    clan = member->clan;
            }
            if (IS_SET(command->args, REPEAT))
                argument = Arguments(std::string(arg) + " " + std::string(argument.get()));
            command->handler(ch, member, clan, argument);
            return;
        }

    for (group = 0; group < NUM_GROUPS; ++group) {
        auto group_title = fmt::format("[ {} Commands ]", clan_cmdgroup[group]);
        char_printf(ch, fmt::format("{:-^70}\n", group_title));
        for (command = commands; !command->name.empty(); ++command) {
            if (group != command->group)
                continue;
            if (can_use_clan_command(ch, command)) {
                char_printf(ch, "\n   clan {:<8s}", command->name);
                if (IS_SET(command->args, CLAN))
                    char_printf(ch, " <clan>");
                if (IS_SET(command->args, APPLICANT))
                    char_printf(ch, " <applicant>");
                else if (IS_SET(command->args, MEMBER))
                    char_printf(ch, " <member>");
                if (!command->more_args.empty())
                    char_printf(ch, " {}", command->more_args);
            }
        }
    }
    char_printf(ch, "\n");
}

ACMD(do_ctell) {
    CharData *me = REAL_CHAR(ch);
    Clan *clan = GET_CLAN(me);

    if (IS_CLAN_SUPERADMIN(me)) {
        /* Only snooping one clan: auto send to that one */
        auto arg = argument.get();
        if (arg.empty())
            char_printf(ch, "Which clan do you want to talk to?\n");
        else if (!(clan = find_clan(arg)))
            char_printf(ch,
                        "'{}' does not refer to a valid clan.\n"
                        "You can only omit the clan if you are snooping just one clan.\n",
                        arg);
        else if (!is_snooping(me, clan))
            char_printf(ch, "You must be snooping {} first.\n", clan->name);
        else
            clan_tell(ch, nullptr, clan, argument);
    } else if (!clan || IS_CLAN_REJECT(me))
        char_printf(ch, "You're not part of a clan.\n");
    else if (IS_CLAN_APPLICANT(me))
        char_printf(ch, "You're not part of a clan.\n");
    else if (CAN_DO_PRIV(me, CPRIV_CHAT) ||
             (IS_CLAN_ALT(me) && MEMBER_CAN(GET_CLAN_MEMBERSHIP(me)->relation.member, CPRIV_CHAT)))
        clan_tell(ch, GET_CLAN_MEMBERSHIP(me), clan, argument);
    else
        char_printf(ch, "You don't have access to clan chat.\n");
}

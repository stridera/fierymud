/***************************************************************************
 *   File: clan.c                                         Part of FieryMUD *
 *  Usage: Front-end for the clan system                                   *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "clan.hpp"

#include "act.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "editor.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "math.hpp"
#include "modify.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "strings.h"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#define CLANCMD(name) void(name)(char_data * ch, clan_membership * member, clan * clan, char *argument)

extern const struct {
    char *abbr;
    bool default_on;
    char *desc;
} clan_privileges[NUM_CLAN_PRIVS];

CLANCMD(clan_list) {
    clan_iter iter;

    if (clan_count() == 0) {
        cprintf(ch, "No clans have formed yet.\r\n");
        return;
    }

    /* List clans, # of members, power, and app fee */
    pprintf(ch, AUND " Num   Clan           Members/Power  App Fee/Lvl\r\n" ANRM);
    for (iter = clans_start(); iter != clans_end(); ++iter) {
        pprintf(ch,
                "[%3d]  " ELLIPSIS_FMT
                " "
                "%3u/%-5u   " AHCYN "%5up" ANRM "/%-3u\n",
                (*iter)->number, ELLIPSIS_STR((*iter)->abbreviation, 18), (*iter)->member_count, (*iter)->power,
                (*iter)->app_fee, (*iter)->app_level);
    }

    start_paging(ch);
}

CLANCMD(clan_bank) {
    int coins[NUM_COIN_TYPES];
    bool deposit;
    const char *verb, *preposition;

    argument = any_one_arg(argument, arg);
    deposit = is_abbrev(arg, "deposit");
    verb = deposit ? "deposit" : "withdraw";
    preposition = deposit ? "into" : "from";

    if (!parse_money(&argument, coins)) {
        cprintf(ch, "How much do you want to %s?\r\n", verb);
        return;
    }

    if (deposit) {
        /* Gods have bottomless pockets */
        if (GET_LEVEL(ch) < LVL_GOD) {
            if (GET_PLATINUM(ch) < coins[PLATINUM] || GET_GOLD(ch) < coins[GOLD] || GET_SILVER(ch) < coins[SILVER] ||
                GET_COPPER(ch) < coins[COPPER]) {
                cprintf(ch, "You do not have that kind of money!\r\n");
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
            cprintf(ch, "The clan is not wealthy enough for your needs!\r\n");
            return;
        }
        GET_PLATINUM(ch) += coins[PLATINUM];
        GET_GOLD(ch) += coins[GOLD];
        GET_SILVER(ch) += coins[SILVER];
        GET_COPPER(ch) += coins[COPPER];
        save_player_char(ch);
    }

    statemoney(buf, coins);
    cprintf(ch, "You %s %s %s's account: %s\r\n", verb, preposition, clan->abbreviation, buf);

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

static bool is_snooping(const struct char_data *ch, const struct Clan *clan) {
    struct clan_snoop *snoop;

    for (snoop = GET_CLAN_SNOOP(ch); snoop; snoop = snoop->next)
        if (snoop->clan == clan)
            return TRUE;

    return FALSE;
}

CLANCMD(clan_tell) {
    struct descriptor_data *d;
    struct char_data *tch;
    struct char_data *me = REAL_CHAR(ch);

    skip_spaces(&argument);

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        cprintf(ch, "Your lips move, but no sound forms.\r\n");
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    if (!*argument) {
        cprintf(ch, "What do you want to tell the clan?\r\n");
        return;
    }

    argument = drunken_speech(argument, GET_COND(ch, DRUNK));

    cprintf(ch, AFMAG "You tell %s" AFMAG ", '" AHMAG "%s" AFMAG "'\r\n" ANRM,
            member ? "your clan" : clan->abbreviation, argument);

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
            cprintf(FORWARD(tch), AFMAG "%s tells %s" AFMAG ", '" AHMAG "%s" AFMAG "'\r\n" ANRM,
                    GET_INVIS_LEV(me) > GET_LEVEL(tch) ? "Someone" : GET_NAME(me),
                    member && !IS_CLAN_SUPERADMIN(tch) ? "your clan" : clan->abbreviation, argument);
    }
}

CLANCMD(clan_set) {
    argument = any_one_arg(argument, arg);

    if (is_abbrev(arg, "abbr")) {
        char *old_abbr = clan->abbreviation;
        skip_spaces(&argument);
        if (strlen(strip_ansi(argument)) > MAX_CLAN_ABBR_LEN) {
            cprintf(ch, "Clan abbreviationss may be at most %d characters in length.\r\n", MAX_CLAN_ABBR_LEN);
            return;
        }
        clan->abbreviation = ""; /* so find_clan doesn't find this clan */
        if (find_clan(argument)) {
            cprintf(ch, "A clan with that name or abbreviation already exists!\r\n");
            clan->abbreviation = old_abbr; /* revert */
            return;
        }
        clan->abbreviation = strdupf("%s&0", argument);
        cprintf(ch, "%s is now abbreviated %s.\r\n", clan->name, clan->abbreviation);
        mprintf(L_STAT, LVL_GOD, "(CLAN) %s changes %s's to %s", GET_NAME(ch), clan->name, clan->abbreviation);
        free(old_abbr);
    }

    else if (is_abbrev(arg, "addrank")) {
        int i;

        if (clan->rank_count >= MAX_CLAN_RANKS) {
            cprintf(ch, "%s already has the maximum number of ranks.\r\n", clan->name);
            return;
        }

        ++clan->rank_count;
        /* Warning! RECREATE does not initialize to zero... */
        RECREATE(clan->ranks, clan_rank, clan->rank_count);
        clan->ranks[clan->rank_count - 1].title = strdup("Member");
        for (i = 0; i < NUM_CLAN_PRIVS; ++i)
            if (clan_privileges[i].default_on)
                SET_FLAG(clan->ranks[clan->rank_count - 1].privileges, i);
            else
                REMOVE_FLAG(clan->ranks[clan->rank_count - 1].privileges, i);

        cprintf(ch, "You add a new rank (%u) to %s.\r\n", clan->rank_count, clan->name);
        clan_notification(clan, ch, "%s adds a new rank to your clan.", GET_NAME(ch));
    }

    else if (is_abbrev(arg, "appfee")) {
        any_one_arg(argument, arg);
        if (!is_number(arg)) {
            cprintf(ch, "How much platinum should the clan's application fee be?\r\n");
            return;
        }
        clan->app_fee = atoi(arg);
        cprintf(ch, "%s's application fee is now %u platinum.\r\n", clan->name, clan->app_fee);
    }

    else if (is_abbrev(arg, "applev")) {
        unsigned int level;
        any_one_arg(argument, arg);
        if (!is_number(arg)) {
            cprintf(ch, "What should the clan's minimum application level be?\r\n");
            return;
        }
        level = atoi(arg);
        if (level < 1 || level > LVL_IMPL) {
            cprintf(ch, "The minimum application level must be between 1 and %d.\r\n", LVL_IMPL);
            return;
        }
        clan->app_level = level;
        cprintf(ch, "%s's minimum application level is now %u.\r\n", clan->name, clan->app_level);
    }

    else if (is_abbrev(arg, "delrank")) {
        if (clan->rank_count <= MIN_CLAN_RANKS) {
            cprintf(ch, "%s already has the minimum number of ranks.\r\n", clan->name);
            return;
        }

        --clan->rank_count;
        free(clan->ranks[clan->rank_count].title);

        cprintf(ch, "You remove a rank (%u) from %s.\r\n", clan->rank_count + 1, clan->name);
        clan_notification(clan, ch, "%s removes a rank from your clan.", GET_NAME(ch));

        for (member = clan->members; member; member = member->next)
            if (member->rank == clan->rank_count + 1) {
                --member->rank;
                if (member->player)
                    cprintf(FORWARD(member->player), AFMAG "You have been automatically promoted to rank %u.\r\n" ANRM,
                            member->rank);
            }
    }

    else if (is_abbrev(arg, "dues")) {
        any_one_arg(argument, arg);
        if (!is_number(arg)) {
            cprintf(ch, "How much platinum should the clan's dues be?\r\n");
            return;
        }
        clan->dues = atoi(arg);
        cprintf(ch, "%s's monthly dues are now %u platinum.\r\n", clan->name, clan->dues);
    }

    else if (is_abbrev(arg, "name")) {
        char *old_name = clan->name;
        skip_spaces(&argument);
        if (strlen(strip_ansi(argument)) > MAX_CLAN_NAME_LEN) {
            cprintf(ch, "Clan names may be at most %d characters in length.\r\n", MAX_CLAN_NAME_LEN);
            return;
        }
        clan->name = ""; /* so find_clan doesn't find this clan */
        if (find_clan(argument)) {
            cprintf(ch, "A clan with that name already exists!\r\n");
            clan->name = old_name; /* revert */
            return;
        }
        clan->name = strdupf("%s&0", argument);
        cprintf(ch, "%s is now named %s.\r\n", old_name, clan->name);
        mprintf(L_STAT, LVL_GOD, "(CLAN) %s renames %s to %s", GET_NAME(ch), old_name, clan->name);
        free(old_name);
    }

    else if (is_abbrev(arg, "title")) {
        unsigned int rank;
        argument = any_one_arg(argument, arg);
        if (!*arg) {
            cprintf(ch, "For which rank do you want to set a title?\r\n");
            return;
        }
        rank = atoi(arg);
        if (!is_number(arg) || rank < 1 || rank > clan->rank_count) {
            cprintf(ch, "'%s' is an invalid rank.  Valid ranks are 1-%u.\r\n", arg, clan->rank_count);
            return;
        }
        skip_spaces(&argument);
        if (!IS_CLAN_SUPERADMIN(ch) && !IS_CLAN_ADMIN(ch) && OUTRANKS(rank, GET_CLAN_RANK(ch))) {
            cprintf(ch, "You cannot set the title for a rank above your own.\r\n");
            return;
        }
        if (strlen(strip_ansi(argument)) > MAX_CLAN_TITLE_LEN) {
            cprintf(ch, "Clan titles may be at most %u characters long.\r\n", MAX_CLAN_TITLE_LEN);
            return;
        }
        free(clan->ranks[rank - 1].title);
        clan->ranks[rank - 1].title = strdupf("%s&0", argument);
        cprintf(ch, "Rank %u's title is now: %s\r\n", rank, argument);
        clan_notification(clan, ch, "%s has changed rank %u's title to %s.", GET_NAME(ch), rank, argument);
    }

    else {
        mprintf(L_ERROR, LVL_GOD, "SYSERR: clan_set: unknown subcommand '%s'", arg);
        return;
    }

    save_clan(clan);
}

CLANCMD(clan_alt) {
    struct char_data *tch;
    struct clan_membership *alt;

    argument = any_one_arg(argument, arg);

    if (!*arg) {
        cprintf(ch, "Whom do you want to add or remove as an alt?\r\n");
        return;
    }

    /*
     * First, let's see if we're trying to remove an alt: they don't have
     * to be online for that.
     */
    for (alt = member->relation.alts; alt; alt = alt->next) {
        if (!str_cmp(alt->name, arg)) {
            cprintf(ch, "You remove %s as one of %s%s clan alts.\r\n", alt->name,
                    ch == member->player ? "your" : member->name, ch == member->player ? "" : "'s");
            if (ch != member->player && member->player)
                cprintf(FORWARD(member->player), AFMAG "%s removes %s as one of your clan alts.\r\n" ANRM, GET_NAME(ch),
                        alt->name);
            if (alt->player)
                cprintf(FORWARD(alt->player), AFMAG "You are no longer one of %s's clan alts.\r\n" ANRM, member->name);
            revoke_clan_membership(alt);
            return;
        }
    }

    if (!(tch = find_char_by_desc(find_vis_by_name(ch, arg))))
        cprintf(ch, "There's no one online by the name of %s.\r\n", arg);
    else if (ch == tch)
        cprintf(ch, "You want to be your own alt?\r\n");
    else if (GET_CLAN_MEMBERSHIP(tch))
        cprintf(ch, "%s is already in a clan!\r\n", GET_NAME(tch));
    else if (!IS_CLAN_SUPERADMIN(ch) && !IS_CLAN_ADMIN(ch) && strcmp(ch->desc->host, tch->desc->host))
        cprintf(ch, "%s was not found logged in as your alt.\r\n", GET_NAME(tch));
    else if (IS_CLAN_SUPERADMIN(tch))
        cprintf(ch, "%s is already a clan super-admin!\r\n", GET_NAME(tch));
    else {
        cprintf(ch, "You make %s one of %s%s clan alts.\r\n", GET_NAME(tch),
                ch == member->player ? "your" : member->name, ch == member->player ? "" : "'s");
        cprintf(tch, AFMAG "%s makes you one of %s%s clan alts.\r\n" ANRM, GET_NAME(ch),
                ch == member->player ? HSHR(member->player) : member->name, ch == member->player ? "" : "'s");
        if (ch != member->player && member->player)
            cprintf(FORWARD(member->player), AFMAG "%s makes %s one of your clan alts.\r\n" ANRM, GET_NAME(ch),
                    GET_NAME(tch));
        CREATE(alt, clan_membership, 1);
        alt->name = strdup(GET_NAME(tch));
        alt->rank = ALT_RANK_OFFSET + member->rank;
        alt->since = member->since;
        alt->relation.member = member;
        alt->next = member->relation.alts;
        member->relation.alts = alt;
        alt->clan = clan;
        alt->player = tch;
        GET_CLAN_MEMBERSHIP(tch) = alt;
        save_player_char(tch);
        save_clan(clan);
    }
}

CLANCMD(clan_apply) {
    if (GET_LEVEL(ch) < LVL_GOD && GET_PLATINUM(ch) < clan->app_fee) {
        cprintf(ch,
                "You don't have enough money to cover the %u platinum "
                "application fee.\r\n",
                clan->app_fee);
        return;
    }

    if (GET_LEVEL(ch) < clan->app_level) {
        cprintf(ch, "%s does not accept players beneath level %u.\r\n", clan->name, clan->app_level);
        return;
    }

    cprintf(ch, "You apply to %s.\r\n", clan->name);

    if (GET_LEVEL(ch) < LVL_GOD) {
        GET_PLATINUM(ch) -= clan->app_fee;
        clan->treasure[PLATINUM] += clan->app_fee;
    }

    clan_notification(clan, NULL, "%s has applied to your clan.", GET_NAME(ch));

    CREATE(member, clan_membership, 1);
    member->name = strdup(GET_NAME(ch));
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

    fetch_word(argument, buf, sizeof(buf));

    if (!*buf)
        cprintf(ch, "What is the abbreviation for the new clan?\r\n");
    else if (strlen(strip_ansi(buf)) > 10)
        cprintf(ch, "Clan abbreviations can be at most 10 visible characters long.\r\n");
    else if (find_clan_by_abbr(strip_ansi(buf)))
        cprintf(ch, "A clan with a similar abbreviation already exists.\r\n");
    else {
        strcat(buf, "&0");
        clan = alloc_clan();
        clan->name = strdup(buf);
        clan->abbreviation = strdup(buf);
        clan->description = NULL;
        clan->motd = NULL;

        clan->dues = 0;
        clan->app_fee = 0;
        clan->app_level = 0;
        clan->power = 0;
        for (i = 0; i < NUM_COIN_TYPES; ++i)
            clan->treasure[i] = 0;

        clan->rank_count = 2;
        CREATE(clan->ranks, clan_rank, 2);
        clan->ranks[0].title = strdup("Leader");
        clan->ranks[1].title = strdup("Member");
        for (i = 0; i < NUM_CLAN_PRIVS; ++i) {
            SET_FLAG(clan->ranks[0].privileges, i);
            if (clan_privileges[i].default_on)
                SET_FLAG(clan->ranks[1].privileges, i);
        }

        clan->people_count = 0;
        clan->people = NULL;
        clan->member_count = 0;
        clan->members = NULL;
        clan->admin_count = 0;
        clan->admins = NULL;
        clan->applicant_count = 0;
        clan->applicants = NULL;

        cprintf(ch, "New clan created.\r\n");
        mprintf(L_STAT, LVL_GOD, "(CLAN) %s creates new clan: %s", GET_NAME(ch), buf);

        save_clan(clan);
    }
}

CLANCMD(clan_rank) {
    unsigned int rank;
    const char *action = NULL;

    argument = any_one_arg(argument, arg);

    if (is_abbrev(arg, "demote")) {
        if (!IS_CLAN_SUPERADMIN(ch) && ch != member->player && !OUTRANKS(GET_CLAN_RANK(ch), member->rank))
            cprintf(ch, "You cannot demote someone at or above your rank.\r\n");
        else if (member->rank == clan->rank_count)
            cprintf(ch, "%s is already the minimum rank.\r\n", member->name);
        else if (!IS_MEMBER_RANK(member->rank))
            cprintf(ch, "%s isn't a clan member.\r\n", member->name);
        else {
            rank = member->rank + 1;
            action = "demote";
        }
    } else if (is_abbrev(arg, "promote")) {
        if (!IS_CLAN_SUPERADMIN(ch) && !OUTRANKS(GET_CLAN_RANK(ch), member->rank))
            cprintf(ch, "You cannot promote someone at or above your rank.\r\n");
        else if (member->rank == RANK_LEADER)
            cprintf(ch, "%s is already the maximum rank.\r\n", member->name);
        else if (!IS_MEMBER_RANK(member->rank))
            cprintf(ch, "%s isn't a clan member.\r\n", member->name);
        else {
            rank = member->rank - 1;
            action = "promote";
        }
    } else {
        log("SYSERR: clan_rank: invalid subcommand '%s'", arg);
        return;
    }

    /* action only gets set if all checks above were successful. */
    if (!action)
        return;

    if (ch == member->player)
        cprintf(ch, "You %s yourself to rank %u: %s\r\n", action, rank, clan->ranks[rank - 1].title);
    else {
        if (member->player)
            cprintf(FORWARD(member->player), AFMAG "%s has %sd you to rank %u: " ANRM "%s\r\n", GET_NAME(ch), action,
                    rank, clan->ranks[rank - 1].title);
        cprintf(ch, "You %s %s to rank %u: %s\r\n", action, member->name, rank, clan->ranks[rank - 1].title);
    }
    member->rank = RANK_NONE; /* Temporary so they don't get the notification */
    clan_notification(clan, ch, "%s has %sd %s to rank %u: " ANRM "%s", GET_NAME(ch), action, member->name, rank,
                      clan->ranks[rank - 1].title);
    member->rank = rank;
    /* Shift alts' ranks too */
    for (member = member->relation.alts; member; member = member->next)
        member->rank = rank + ALT_RANK_OFFSET;
    save_clan(clan);
}

CLANCMD(clan_destroy) {
    clan_notification(clan, ch, "Your clan has been disbanded!");
    cprintf(ch, AFMAG "You have deleted the clan %s.\r\n" ANRM, clan->name);
    mprintf(L_STAT, LVL_GOD, "(CLAN) %s has destroyed the clan %s.", GET_NAME(ch), clan->name);
    dealloc_clan(clan);
}

CLANCMD(clan_enroll) {
    int num;

    member->since = time(0);
    member->rank = clan->member_count ? clan->rank_count : RANK_LEADER;
    --clan->applicant_count;
    ++clan->member_count;
    if (member->player)
        clan->power += GET_LEVEL(member->player);
    else if ((num = get_ptable_by_name(member->name)))
        clan->power += player_table[num].level;

    update_clan(clan);
    save_clan(clan);

    cprintf(ch, "You %s %s %s %s.\r\n", member->rank == RANK_LEADER ? "appoint" : "enroll", member->name,
            member->rank == RANK_LEADER ? "the leader of" : "in", clan->name);
    if (member->player)
        cprintf(FORWARD(member->player), AFMAG "You've been %s %s" AFMAG "!\r\n" ANRM,
                member->rank == RANK_LEADER ? "appointed the leader of" : "enrolled in", clan->name);

    mprintf(L_STAT, LVL_GOD, "(CLAN) %s enrolls %s in %s.", GET_NAME(ch), member->name, clan->name);
}

CLANCMD(clan_expel) {
    char *name = strdup(member->name);

    if (!IS_CLAN_SUPERADMIN(ch) && !OUTRANKS(GET_CLAN_RANK(ch), member->rank)) {
        cprintf(ch, "%s outranks you!\r\n", name);
        free(name);
        return;
    }

    clan = member->clan;

    cprintf(ch, "You expel %s from %s.\r\n", name, clan->name);

    if (member->player)
        cprintf(FORWARD(member->player), AFMAG "%s has expelled you from %s" AFMAG ".\r\n" ANRM, GET_NAME(ch),
                clan->name);

    mprintf(L_STAT, LVL_GOD, "(CLAN) %s expels %s from %s.", GET_NAME(ch), name, clan->name);
    revoke_clan_membership(member);

    clan_notification(clan, ch, "%s has expelled %s from your clan.", GET_NAME(ch), name);
    free(name);
}

CLANCMD(clan_priv) {
    enum { GRANT, REVOKE } action;
    int rank, priv;

    argument = any_one_arg(argument, arg);

    if (is_abbrev(arg, "grant"))
        action = GRANT;
    else if (is_abbrev(arg, "revoke"))
        action = REVOKE;
    else {
        log("SYSERR: clan_priv: invalid subcommand '%s'", arg);
        return;
    }

    argument = any_one_arg(argument, arg);
    rank = atoi(arg);

    if (rank < RANK_LEADER || rank > clan->rank_count) {
        cprintf(ch, "'%s' is an invalid rank.  Valid ranks are 1-%d.\r\n", arg, clan->rank_count);
        return;
    }

    argument = any_one_arg(argument, arg);
    for (priv = 0; priv < NUM_CLAN_PRIVS; ++priv)
        if (is_abbrev(arg, clan_privileges[priv].abbr))
            break;
    if (priv >= NUM_CLAN_PRIVS) {
        cprintf(ch,
                "'%s' is an invalid privilege.  Valid privileges are "
                "listed on clan info.\r\n",
                arg);
        return;
    }

    if (!IS_CLAN_SUPERADMIN(ch) && !IS_CLAN_ADMIN(ch) && !HAS_CLAN_PRIV(ch, priv)) {
        cprintf(ch, "You cannot grant or revoke a privilege you do not have!\r\n");
        return;
    }

    if (IS_CLAN_MEMBER(ch) && !OUTRANKS(GET_CLAN_RANK(ch), rank)) {
        cprintf(ch, "You may only grant or revoke privileges on ranks below yours.\r\n");
        return;
    }

    if (action == GRANT) {
        if (IS_FLAGGED(clan->ranks[rank - 1].privileges, priv))
            cprintf(ch, "Rank %d already has the %s privilege.\r\n", rank, clan_privileges[priv].desc);
        else {
            SET_FLAG(clan->ranks[rank - 1].privileges, priv);
            cprintf(ch, "Granted rank %d access to the %s privilege.\r\n", rank, clan_privileges[priv].desc);
        }
    } else if (action == REVOKE) {
        if (IS_FLAGGED(clan->ranks[rank - 1].privileges, priv)) {
            REMOVE_FLAG(clan->ranks[rank - 1].privileges, priv);
            cprintf(ch, "Revoked rank %d access to the %s privilege.\r\n", rank, clan_privileges[priv].desc);
        } else
            cprintf(ch, "Rank %d doesn't have the %s privilege.\r\n", rank, clan_privileges[priv].desc);
    }

    save_clan(clan);
}

static void show_clan_info(char_data *ch, const struct Clan *clan) {
    const struct clan_membership *member = GET_CLAN_MEMBERSHIP(ch);
    size_t i, j;
    bool show_all =
        ((member && member->clan == clan && OUTRANKS(member->rank, RANK_APPLICANT)) || IS_CLAN_SUPERADMIN(ch));

    strcpy(buf,
           "----------------------------------------------------------------"
           "------\r\n");
    sprintf(buf1, "[ Clan %u: %s ]", clan->number, clan->name);
    memcpy(buf + 29 - strlen(clan->name) / 2, buf1, strlen(buf1));
    pprintf(ch, "%s", buf);

    pprintf(ch,
            "Nickname: " AFYEL "%s" ANRM
            "  "
            "Ranks: " AFYEL "%u" ANRM
            "  "
            "Members: " AFYEL "%u" ANRM
            "  "
            "Power: " AFYEL "%u" ANRM
            "\r\n"
            "Applicants: " AFYEL "%u" ANRM
            "  "
            "App Fee: " AFCYN "%up" ANRM
            "  "
            "App Level: " AFYEL "%u" ANRM
            "  "
            "Dues: " AFCYN "%up" ANRM "\r\n",
            clan->abbreviation, clan->rank_count, clan->member_count, clan->power, clan->applicant_count, clan->app_fee,
            clan->app_level, clan->dues);

    if (show_all) {
        statemoney(buf, clan->treasure);
        pprintf(ch, "Treasure: %s\r\n", buf);

        pprintf(ch, "\r\nRanks:\r\n");
        for (i = 0; i < clan->rank_count; ++i)
            pprintf(ch, "%3u  %s\r\n", i + 1, clan->ranks[i].title);

        pprintf(ch, "\r\nPrivileges:\r\n         ");
        for (j = 1; j <= clan->rank_count; ++j)
            pprintf(ch, "%3u", j);
        for (i = 0; i < NUM_CLAN_PRIVS; ++i) {
            pprintf(ch, "\r\n%-9s", clan_privileges[i].abbr);
            for (j = 0; j < clan->rank_count; ++j)
                pprintf(ch, "  %s%c" ANRM, IS_FLAGGED(clan->ranks[j].privileges, i) ? AFGRN : AFRED,
                        IS_FLAGGED(clan->ranks[j].privileges, i) ? 'Y' : 'N');
        }
        pprintf(ch, "\r\n");
    }

    if (clan->description)
        pprintf(ch, "\r\nDescription:\r\n%s", clan->description);

    if (show_all)
        if (clan->motd)
            pprintf(ch, "\r\nMessage of the Day:\r\n%s", clan->motd);

    start_paging(ch);
}

CLANCMD(clan_info) {
    argument = any_one_arg(argument, arg);

    if (!*arg) {
        if (clan)
            show_clan_info(ch, clan);
        else
            cprintf(ch, "Which clan's info do you want to view?\r\n");
    } else if ((clan = find_clan(arg)))
        show_clan_info(ch, clan);
    else
        cprintf(ch, "'%s' does not refer to a valid clan.\r\n", arg);
}

static void show_clan_member_status(char_data *ch, const struct char_data *tch) {
    if (IS_CLAN_SUPERADMIN(tch))
        cprintf(ch, "%s %s a clan super-administrator.\r\n", ch == tch ? "You" : GET_NAME(tch),
                ch == tch ? "are" : "is");
    else if (IS_CLAN_REJECT(tch)) {
        unsigned int days = days_until_reapply(GET_CLAN_MEMBERSHIP(tch));
        cprintf(ch, "%s %s rejected from %s and may re-apply in %d day%s.\r\n", ch == tch ? "You" : GET_NAME(tch),
                ch == tch ? "were" : "was", GET_CLAN(tch)->name, days, days == 1 ? "" : "s");
    } else if (IS_CLAN_ADMIN(tch))
        cprintf(ch, "%s %s an administrator for %s.\r\n", ch == tch ? "You" : GET_NAME(tch), ch == tch ? "are" : "is",
                GET_CLAN(tch)->name);
    else if (IS_CLAN_MEMBER(tch) || IS_CLAN_ALT(tch) || IS_CLAN_APPLICANT(tch)) {
        strftime(buf, sizeof(buf), "%a, %d %b %Y", localtime(&GET_CLAN_MEMBERSHIP(tch)->since));
        pprintf(ch,
                "Clan membership status for %s:\r\n"
                "  Clan: %s\r\n",
                GET_NAME(tch), GET_CLAN(tch)->name);
        if (IS_CLAN_MEMBER(tch))
            pprintf(ch, "  Rank: %d - %s%s\r\n", GET_CLAN_RANK(tch), GET_CLAN_TITLE(tch),
                    IS_CLAN_LEADER(tch) ? " (Leader)" : "");
        else if (IS_CLAN_ALT(tch))
            pprintf(ch, "  Alt rank: %d (%s)\r\n", GET_CLAN_RANK(tch) - ALT_RANK_OFFSET,
                    GET_CLAN_MEMBERSHIP(tch)->relation.member->name);
        else if (IS_CLAN_APPLICANT(tch))
            pprintf(ch, "  Rank: Applicant\r\n");
        pprintf(ch, "  Member since: %s\r\n", buf);
        if (IS_CLAN_MEMBER(tch)) {
            if (HAS_FLAGS(GET_CLAN(tch)->ranks[GET_CLAN_RANK(tch) - 1].privileges, NUM_CLAN_PRIVS)) {
                screen_buf sb = new_screen_buf();
                int i, seen = 0;
                const size_t len = strlen("  Privileges: ");
                sb_set_first_indentation(sb, len);
                sb_set_other_indentation(sb, len);
                for (i = 0; i < NUM_CLAN_PRIVS; ++i)
                    if (HAS_CLAN_PRIV(tch, i))
                        sb_append(sb, "%s%s", seen++ ? ", " : "", clan_privileges[i].abbr);
                /* skip over the first 14 spaces (dummy indentation) */
                pprintf(ch, "  Privileges: %s\r\n", sb_get_buffer(sb) + len);
                free_screen_buf(sb);
            }
            if (GET_CLAN_MEMBERSHIP(tch)->relation.alts) {
                struct clan_membership *alt;
                screen_buf sb = new_screen_buf();
                int seen = 0;
                const size_t len = strlen("  Alts: ");
                sb_set_first_indentation(sb, len);
                sb_set_other_indentation(sb, len);
                for (alt = GET_CLAN_MEMBERSHIP(tch)->relation.alts; alt; alt = alt->next)
                    sb_append(sb, "%s%s", seen++ ? ", " : "", alt->name);
                /* skip over the first 8 spaces (dummy indentation) */
                pprintf(ch, "  Alts: %s\r\n", sb_get_buffer(sb) + len);
                free_screen_buf(sb);
            }
        }
        start_paging(ch);
    } else
        cprintf(ch, "%s %s not associated with any clan.\r\n", ch == tch ? "You" : GET_NAME(tch),
                ch == tch ? "are" : "is");
}

CLANCMD(clan_status) {
    struct char_data *tch;

    argument = any_one_arg(argument, arg);

    if (IS_CLAN_SUPERADMIN(ch)) {
        if ((tch = find_char_around_char(ch, find_vis_by_name(ch, arg))))
            show_clan_member_status(ch, tch);
        else
            cprintf(ch, "Couldn't find a player by the name of '%s'.\r\n", arg);
    } else
        show_clan_member_status(ch, ch);
}

struct clan_edit {
    struct Clan *clan;
    char string[20];
};

static EDITOR_FUNC(clan_edit_done) {
    struct descriptor_data *d = edit->descriptor;
    struct clan_edit *data = edit->data;

    if (edit->command == ED_EXIT_SAVE) {
        editor_default_exit(edit);
        mprintf(L_STAT, LVL_GOD, "(CLAN) %s edits %s's %s", GET_NAME(d->character), data->clan->name, data->string);
        save_clan(data->clan);
    }

    act("$n stops writing on the large scroll.", TRUE, d->character, 0, 0, TO_ROOM);

    return ED_PROCESSED;
}

CLANCMD(clan_edit) {
    char **message;
    struct clan_edit *data;

    if (!ch->desc)
        return;

    any_one_arg(argument, arg);

    if (is_abbrev(arg, "motd")) {
        message = &clan->motd;
        argument = "message of the day";
    } else if (is_abbrev(arg, "desc")) {
        message = &clan->description;
        argument = "description";
    } else {
        log("SYSERR: clan_edit: unknown string specified");
        return;
    }

    CREATE(data, clan_edit, 1);
    data->clan = clan;
    strcpy(data->string, argument);

    if (editor_edited_by(message)) {
        cprintf(ch, "%s's %s is already being edited.", clan->name, argument);
        return;
    }

    act("$n begins writing on a large scroll.", TRUE, ch, 0, 0, TO_ROOM);

    editor_init(ch->desc, message, MAX_DESC_LENGTH);
    editor_set_begin_string(ch->desc, "Edit %s's %s below.", clan->name, argument);
    editor_set_callback_data(ch->desc, data, ED_FREE_DATA);
    editor_set_callback(ch->desc, ED_EXIT_SAVE, clan_edit_done);
    editor_set_callback(ch->desc, ED_EXIT_ABORT, clan_edit_done);
}

CLANCMD(clan_quit) {
    if (IS_CLAN_APPLICANT(ch))
        cprintf(ch, "You are no longer applying to %s.\r\n", clan->name);
    else if (IS_CLAN_ALT(ch))
        cprintf(ch, "You are no longer a clan alt in %s.\r\n", clan->name);
    else if (IS_CLAN_ADMIN(ch))
        cprintf(ch, "You are no longer an administrator for %s.\r\n", clan->name);
    else if (IS_CLAN_MEMBER(ch))
        cprintf(ch, "You are no longer a member of %s.\r\n", clan->name);
    else
        cprintf(ch, "You are no longer in %s.\r\n", clan->name);
    if (!IS_CLAN_ALT(ch)) {
        mprintf(L_STAT, LVL_GOD, "(CLAN) %s quits %s.", GET_NAME(ch), clan->name);
        clan_notification(clan, ch, "%s has quit your clan.", GET_NAME(ch));
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
        cprintf(FORWARD(member->player),
                AFMAG "You have been rejected from %s and may reapply in %u " AFMAG "days.\r\n" ANRM,
                member->clan->name, days_until_reapply(member));

    mprintf(L_STAT, LVL_GOD, "(CLAN) %s rejects %s's application to %s.", GET_NAME(ch), member->name,
            member->clan->name);
    cprintf(ch, "You reject %s's application to %s.\r\n", member->name, clan->name);
    clan_notification(clan, ch, "%s rejects %s's application to your clan.", GET_NAME(ch), member->name);
}

CLANCMD(clan_snoop) {
    struct clan_snoop *snoop, *temp;
    clan_iter iter;

    fetch_word(argument, arg, sizeof(arg));

    if (!*arg) {
        if (GET_CLAN_SNOOP(ch)) {
            cprintf(ch, "You are currently snooping:\r\n");
            for (snoop = GET_CLAN_SNOOP(ch); snoop; snoop = snoop->next)
                cprintf(ch, "  %s\r\n", snoop->clan->name);
        } else
            cprintf(ch, "You are not currently snooping any clan channels.\r\n");
    }

    else if (!str_cmp(arg, "off")) {
        if (GET_CLAN_SNOOP(ch)) {
            while (GET_CLAN_SNOOP(ch)) {
                snoop = GET_CLAN_SNOOP(ch)->next;
                free(GET_CLAN_SNOOP(ch));
                GET_CLAN_SNOOP(ch) = snoop;
            }
            cprintf(ch, "You are no longer snooping any clan channels.\r\n");
        } else
            cprintf(ch, "You are not currently snooping any clan channels.\r\n");
    }

    else if (!str_cmp(arg, "all")) {
        for (iter = clans_start(); iter != clans_end(); ++iter)
            if (!is_snooping(ch, *iter)) {
                CREATE(snoop, clan_snoop, 1);
                snoop->clan = *iter;
                snoop->next = GET_CLAN_SNOOP(ch);
                GET_CLAN_SNOOP(ch) = snoop;
            }
        cprintf(ch, "You are now snooping all clan channels.\r\n");
    }

    else if ((clan = find_clan(arg))) {
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
            cprintf(ch, "You are no longer snooping %s.\r\n", clan->name);
        } else {
            CREATE(snoop, clan_snoop, 1);
            snoop->clan = clan;
            snoop->next = GET_CLAN_SNOOP(ch);
            GET_CLAN_SNOOP(ch) = snoop;
            cprintf(ch, "You are now snooping %s.\r\n", clan->name);
        }
    }

    else
        cprintf(ch, "'%s' does not refer to a valid clan.\r\n", arg);
}

static void send_clan_who_line(const struct char_data *ch, const struct clan_membership *member) {
    long num;
    char level_buf[4];
    char alt_buf[50];
    char logon_buf[30];
    const char *level, *title, *last_logon, *name_color;

    num = get_ptable_by_name(member->name);
    if (num >= 0) {
        snprintf(level_buf, sizeof(level_buf), "%d", player_table[num].level);
        level = level_buf;
        strftime(logon_buf, sizeof(logon_buf), "%a, %d %b %Y %H:%M", localtime(&player_table[num].last));
        last_logon = logon_buf;
    } else {
        level = "??";
        last_logon = "";
    }

    if (IS_MEMBER_RANK(member->rank))
        title = member->clan->ranks[member->rank - 1].title;
    else if (IS_APPLICANT_RANK(member->rank))
        title = "(applicant)";
    else if (IS_ALT_RANK(member->rank)) {
        snprintf(alt_buf, sizeof(alt_buf), "(%s's alt)", member->relation.member->name);
        title = alt_buf;
    } else
        title = "";

    if (IS_ALT_RANK(member->rank))
        name_color = AFYEL;
    else if (member->player)
        name_color = AFGRN;
    else
        name_color = "";

    cprintf(ch, "[%3s] %s%-10s" ANRM " " ELLIPSIS_FMT " %s\r\n", level, name_color, member->name,
            ELLIPSIS_STR(title, 25), last_logon);
}

static void send_clan_who_header(const struct char_data *ch) {
    cprintf(ch, " " AUND "Lvl" ANRM "  " AUND "Name     " ANRM "  " AUND "Rank                    " ANRM "  " AUND
                "Last Login            " ANRM "\r\n");
}

CLANCMD(clan_who) {
    struct descriptor_data *d;
    struct char_data *tch;
    bool found = FALSE;

    cprintf(ch, AHYEL "Members in " ANRM "%s" AHYEL ":" ANRM "\r\n", clan->name);

    for (d = descriptor_list; d; d = d->next) {
        if (!IS_PLAYING(d))
            continue;
        tch = d->character;
        if (!CAN_SEE(ch, tch) || clan != GET_CLAN(tch))
            continue;
        if (IS_CLAN_MEMBER(tch) || IS_CLAN_ALT(tch)) {
            if (!found) {
                send_clan_who_header(ch);
                found = TRUE;
            }
            send_clan_who_line(ch, GET_CLAN_MEMBERSHIP(tch));
        }
    }

    for (member = clan->members; member && IS_MEMBER_RANK(member->rank); member = member->next)
        if (!member->player || !member->player->desc) {
            if (!found) {
                send_clan_who_header(ch);
                found = TRUE;
            }
            send_clan_who_line(ch, member);
        }

    if (!clan->member_count)
        cprintf(ch, "  None!\r\n");

    if (clan->applicant_count) {
        found = FALSE;
        cprintf(ch, AHYEL "Applicants to " ANRM "%s" AHYEL ":" ANRM "\r\n", clan->name);
        for (member = clan->applicants; member && IS_APPLICANT_RANK(member->rank); member = member->next) {
            if (!found) {
                send_clan_who_header(ch);
                found = TRUE;
            }
            send_clan_who_line(ch, member);
        }
    }
}

/* Clan command permission modes */
#define NONE (1 << 0)
#define PRIV (1 << 1)  /* RANK and PRIV are mutually exclusive   */
#define ADMIN (1 << 2) /* because the clan_subcommand data field */
#define RANK (1 << 3)  /* holds either the minimum rank for RANK */
#define SUPER (1 << 4) /* or the privilege for PRIV, not both    */

/* Clan command argument modes */
#define IGNORE 0
#define CLAN (1 << 0)   /* CLAN and MEMBER are not mutually       */
#define MEMBER (1 << 1) /* exclusive but if they are encountered  */
#define REPEAT (1 << 2) /* together, the clan is parsed first     */
#define APPLICANT (1 << 3)

/* Clan command groupings - for aesthetics only */
#define GENERAL 0
#define MGMT 1         /* Hack alert: the PRIV/ADMIN constants   */
#define PRIV (1 << 1)  /* are already used above for a bitfield  */
#define MODIFY 3       /* but for convience we're reusing them   */
#define ADMIN (1 << 2) /* here as their actual values of 2 and 4 */
#define NUM_GROUPS 5

static const char *clan_cmdgroup[NUM_GROUPS] = {"General", "Management", "Privileged", "Modifier", "Administrative"};

/* Clan command information structure */
static const struct clan_subcommand {
    char *name;
    unsigned int group;
    unsigned int type;
    unsigned int data;
    unsigned int args;
    char *more_args;
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
    {"demote", MGMT, PRIV | ADMIN | SUPER, CPRIV_DEMOTE, MEMBER | REPEAT, "", clan_rank},
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
    {"promote", MGMT, PRIV | ADMIN | SUPER, CPRIV_PROMOTE, MEMBER | REPEAT, "", clan_rank},
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
    {0, 0, 0, 0, 0, 0, 0},
};

static bool can_use_clan_command(const struct char_data *ch, const struct clan_subcommand *command) {
    if (IS_SET(command->type, NONE))
        if (GET_CLAN_RANK(ch) == RANK_NONE && !IS_CLAN_SUPERADMIN(ch))
            return TRUE;
    if (IS_SET(command->type, RANK))
        if (!OUTRANKS(command->data, GET_CLAN_RANK(ch)))
            return TRUE;
    if (IS_SET(command->type, PRIV))
        if (HAS_CLAN_PRIV(ch, command->data))
            return TRUE;
    if (IS_SET(command->type, ADMIN))
        if (IS_CLAN_ADMIN(ch))
            return TRUE;
    if (IS_SET(command->type, SUPER))
        if (IS_CLAN_SUPERADMIN(ch))
            return TRUE;
    return FALSE;
}

static const struct clan_subcommand *determine_command(const struct char_data *ch, const char *cmd) {
    const struct clan_subcommand *command = NULL;

    for (command = commands; command->name; ++command) {
        if (*command->name < *cmd)
            continue;
        else if (*command->name > *cmd)
            break;
        else if (is_abbrev(cmd, command->name))
            if (can_use_clan_command(ch, command))
                return command;
    }

    return NULL;
}

ACMD(do_clan) {
    const struct clan_subcommand *command = NULL;
    struct Clan *clan;
    struct clan_membership *member;
    char argbuf[MAX_INPUT_LENGTH];
    unsigned int group;

    if (IS_NPC(ch) || !ch->desc) {
        cprintf(ch, "%s", HUH);
        return;
    }

    clan = GET_CLAN(ch);
    member = GET_CLAN_MEMBERSHIP(ch);

    /* Determine which command to invoke */
    argument = any_one_arg(argument, argbuf);
    if (strlen(argbuf) >= 3)
        if ((command = determine_command(ch, argbuf))) {
            if (IS_SET(command->args, CLAN)) {
                argument = any_one_arg(argument, arg);
                if (!*arg) {
                    cprintf(ch, "Which clan?\r\n");
                    return;
                } else if (!(clan = find_clan(arg))) {
                    cprintf(ch, "'%s' does not refer to a valid clan.\r\n", arg);
                    return;
                }
            }
            if (IS_SET(command->args, MEMBER | APPLICANT)) {
                argument = any_one_arg(argument, arg);
                CAP(arg);
                if (!*arg) {
                    cprintf(ch, "Whom do you want to %s?\r\n", argbuf);
                    return;
                } else if (!(member = find_clan_membership(arg))) {
                    cprintf(ch, "%s is not a member or applicant of any clan.\r\n", arg);
                    return;
                } else if (IS_SET(command->args, APPLICANT) && member->rank != RANK_APPLICANT) {
                    cprintf(ch, "%s is not an applicant of any clan.\r\n", arg);
                    return;
                } else if (!IS_CLAN_SUPERADMIN(ch) && GET_CLAN(ch) != member->clan) {
                    cprintf(ch, "%s is not a member of your clan.\r\n", arg);
                    return;
                }
                if (!IS_SET(command->args, CLAN))
                    clan = member->clan;
            }
            if (IS_SET(command->args, REPEAT)) {
                strcat(argbuf, argument);
                argument = argbuf;
            } else
                skip_spaces(&argument);

            command->handler(ch, member, clan, argument);
            return;
        }

    for (group = 0; group < NUM_GROUPS; ++group) {
        strcpy(buf, "--------------------------------------------------------------------");
        sprintf(buf1, "[ %s commands ]", clan_cmdgroup[group]);
        strncpy(buf + 2, buf1, strlen(buf1));
        for (command = commands; command->name; ++command) {
            if (group != command->group)
                continue;
            if (can_use_clan_command(ch, command)) {
                if (*buf) {
                    cprintf(ch, "\r\n%s", buf);
                    *buf = '\0';
                }
                cprintf(ch, "\r\n   clan %-8s", command->name);
                if (IS_SET(command->args, CLAN))
                    cprintf(ch, " <clan>");
                if (IS_SET(command->args, APPLICANT))
                    cprintf(ch, " <applicant>");
                else if (IS_SET(command->args, MEMBER))
                    cprintf(ch, " <member>");
                if (command->more_args)
                    cprintf(ch, " %s", command->more_args);
            }
        }
    }
    cprintf(ch, "\r\n");
}

ACMD(do_ctell) {
    struct char_data *me = REAL_CHAR(ch);
    struct Clan *clan = GET_CLAN(me);

    if (IS_CLAN_SUPERADMIN(me)) {
        /* Only snooping one clan: auto send to that one */
        if (GET_CLAN_SNOOP(me) && !GET_CLAN_SNOOP(me)->next)
            clan_tell(ch, NULL, GET_CLAN_SNOOP(me)->clan, argument);
        else {
            argument = any_one_arg(argument, arg);
            if (!*arg)
                cprintf(ch, "Which clan do you want to talk to?\r\n");
            else if (!(clan = find_clan(arg)))
                cprintf(ch,
                        "'%s' does not refer to a valid clan.\r\nYou can "
                        "only omit the clan if you are snooping just one "
                        "clan.\r\n",
                        arg);
            else if (!is_snooping(me, clan))
                cprintf(ch, "You must be snooping %s first.\r\n", clan->name);
            else
                clan_tell(ch, NULL, clan, argument);
        }
    } else if (!clan || IS_CLAN_REJECT(me))
        cprintf(ch, "You're not part of a clan.\r\n");
    else if (IS_CLAN_APPLICANT(me))
        cprintf(ch, "You're not part of a clan.\r\n");
    else if (CAN_DO_PRIV(me, CPRIV_CHAT) ||
             (IS_CLAN_ALT(me) && MEMBER_CAN(GET_CLAN_MEMBERSHIP(me)->relation.member, CPRIV_CHAT)))
        clan_tell(ch, GET_CLAN_MEMBERSHIP(me), clan, argument);
    else
        cprintf(ch, "You don't have access to clan chat.\r\n");
}

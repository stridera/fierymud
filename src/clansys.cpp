/***************************************************************************
 *   File: clansys.c                                      Part of FieryMUD *
 *  Usage: Infrastructure for the clan system                              *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "clan.hpp"
#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "limits.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <sys/stat.h>

/***************************************************************************
 * Clan Variables
 ***************************************************************************/
static unsigned int num_of_clans = 0;
// static std::vector<std::unique_ptr<Clan>> clans;

/***************************************************************************
 * Clan Infrastructure
 ***************************************************************************/

static void sort_clan_people(Clan *clan) {
    if (clan->people_count == 0)
        return;

    std::vector<ClanMembership *> array(clan->people_count);
    ClanMembership *member = clan->people;
    for (size_t i = 0; i < clan->people_count; ++i) {
        array[i] = member;
        member = member->next;
    }

    std::sort(array.begin(), array.end(),
              [](ClanMembership *a, ClanMembership *b) { return OUTRANKS(b->rank, a->rank); });

    clan->people = array[0];
    for (size_t i = 1; i < clan->people_count; ++i)
        array[i - 1]->next = array[i];
    array.back()->next = nullptr;
}

static void load_clan_member(Clan *clan, const std::string line) {
    ClanMembership *member, *alt;
    int num;
    std::string_view name;

    name = capitalize_first(getline(line, ' '));
    if ((num = get_ptable_by_name(name)) < 0)
        return;

    CREATE(member, ClanMembership, 1);
    member->name = name;
    member->clan = clan;
    member->relation.alts = nullptr;
    member->player = nullptr;

    name = getline(line, ' ');
    member->rank = std::max(1, svtoi(name));

    name = getline(line, ' ');
    member->since = svtoi(name);

    for (;;) {
        name = getline(line, ' ');
        if (name.empty())
            break;
        name = capitalize_first(name);
        if (get_ptable_by_name(name) < 0)
            continue;
        CREATE(alt, ClanMembership, 1);
        alt->name = name;
        alt->rank = ALT_RANK_OFFSET + member->rank;
        alt->relation.member = member;
        alt->since = member->since;
        alt->next = member->relation.alts;
        member->relation.alts = alt;
    }
}

bool load_clan(const std::string_view clan_num, Clan *clan) {
    std::ifstream file(fmt::format("{}/{}{}", CLAN_PREFIX, clan_num, CLAN_SUFFIX));
    if (!file.is_open()) {
        log("Couldn't open clan file '{}'", fmt::format("{}/{}{}", CLAN_PREFIX, clan_num, CLAN_SUFFIX));
        return false;
    }

    clan->rank_count = 0;
    clan->power = 0;

    std::string line;
    while (getline(file, line)) {
        auto tag = line.substr(0, line.find(' '));
        auto value = line.substr(line.find(' ') + 1);
        int num = std::stoi(value);

        if (tag == "appfee") {
            clan->app_fee = num;
        } else if (tag == "applevel") {
            clan->app_level = num;
        } else if (tag == "abbr") {
            clan->abbreviation = value;
        } else if (tag == "copper") {
            clan->treasure[COPPER] = num;
        } else if (tag == "dues") {
            clan->dues = num;
        } else if (tag == "description") {
            clan->description = value;
        } else if (tag == "gold") {
            clan->treasure[GOLD] = num;
        } else if (tag == "member") {
            load_clan_member(clan, value);
        } else if (tag == "motd") {
            clan->motd = value;
        } else if (tag == "name") {
            clan->name = value;
        } else if (tag == "number") {
            clan->number = num;
        } else if (tag == "platinum") {
            clan->treasure[PLATINUM] = num;
        } else if (tag == "privilege") {
            if (num <= 0 || num > MAX_CLAN_RANKS) {
                log("SYSERR: load_clan: attempt to set clan privilege for invalid rank {}", num);
            } else {
                auto privileges = value;
                while (!privileges.empty()) {
                    auto privilege = privileges.substr(0, privileges.find(' '));
                    privileges = privileges.substr(privileges.find(' ') + 1);
                    for (int i = 0; i < NUM_CLAN_PRIVS; ++i) {
                        if (matches(clan_privileges[i].abbr, privilege)) {
                            clan->rank_count = std::max<size_t>(clan->rank_count, num);
                            SET_FLAG(clan->ranks[num - 1].privileges, i);
                            break;
                        }
                    }
                }
            }
        } else if (tag == "silver") {
            clan->treasure[SILVER] = num;
        } else if (tag == "title") {
            if (num <= 0 || num > MAX_CLAN_RANKS) {
                log("SYSERR: load_clan: attempt to set clan title for invalid rank {}", num);
            } else {
                clan->rank_count = std::max<size_t>(clan->rank_count, num);
                clan->ranks[num - 1].title = value;
            }
        } else {
            log("SYSERR: Unknown tag {} in clan file {}: {}", tag, clan_num, line);
        }
    }

    clan->ranks = std::make_unique<ClanRank[]>(clan->rank_count);

    update_clan(clan);

    return true;
}

void init_clans(void) {
    FILE *fl;
    char clan_id[17];
    unsigned int pos;

    /*
     * Legacy-checking code.  If we successfully open the file, that means
     * it is not a directory, and is probably the binary format from the
     * old clan system.  In this case, back it up with a .old extension.
     */
    if ((fl = fopen(CLAN_PREFIX.data(), "r"))) {
        struct stat statbuf;
        fstat(fileno(fl), &statbuf);
        fclose(fl);
        if (!S_ISDIR(statbuf.st_mode)) {
            rename(CLAN_PREFIX.data(), CLAN_PREFIX_OLD.data());
            /*
             * This is a POSIX-only.  Create the new clan directory.
             */
            log("Backed-up old clans file and creating new clan directory.");
            mkdir(CLAN_PREFIX.data(), 0775);
        }
    }

    if (!(fl = fopen(CLAN_INDEX_FILE.data(), "r"))) {
        log("No clan index.  Creating a new one.");
        touch(CLAN_INDEX_FILE.data());
        if (!(fl = fopen(CLAN_INDEX_FILE.data(), "r"))) {
            perror("fatal error opening clan index");
            exit(1);
        }
    }

    while (fgets(clan_id, sizeof(clan_id), fl)) {
        clan_id[strlen(clan_id) - 1] = '\0'; /* remove the \n */
        Clan tmp{};
        if (load_clan(clan_id, &tmp)) {
            clans.emplace_back(std::make_unique<Clan>(tmp));
        }
    }

    log("{:3d} clan{} loaded.", clans.size(), clans.size() == 1 ? "" : "s");

    fclose(fl);
}

static void save_clan_index(void) {
    FILE *fl;
    unsigned int i;

    if (!(fl = fopen(CLAN_INDEX_FILE.data(), "w"))) {
        log("No clan index.  Creating a new one.");
        touch(CLAN_INDEX_FILE);
        if (!(fl = fopen(CLAN_INDEX_FILE.data(), "w"))) {
            perror("fatal error opening clan index");
            exit(1);
        }
    }

    for (i = 0; i < num_of_clans; ++i)
        fprintf(fl, "%u\n", clans[i]->number);

    fclose(fl);
}

static void perform_save_clan(const Clan *clan) {
    std::string temp_filename = fmt::format("{}/{}.tmp", CLAN_PREFIX, clan->number);
    std::string filename = fmt::format("{}/{}{}", CLAN_PREFIX, clan->number, CLAN_SUFFIX);

    std::ofstream file(temp_filename);
    if (!file.is_open()) {
        log(LogSeverity::Error, LVL_GOD, "SYSERR: Couldn't open temp clan file {} for write", temp_filename);
        return;
    }

    file << fmt::format("number: {}\n", clan->number);
    file << fmt::format("name: {}\n", clan->name);
    file << fmt::format("abbr: {}\n", clan->abbreviation);
    file << fmt::format("description:\n{}~\n", filter_chars(clan->description, "\r~"));
    file << fmt::format("motd:\n{}~\n", filter_chars(clan->motd, "\r~"));
    file << fmt::format("dues: {}\n", clan->dues);
    file << fmt::format("appfee: {}\n", clan->app_fee);
    file << fmt::format("applevel: {}\n", clan->app_level);

    file << fmt::format("copper: {}\n", clan->treasure[COPPER]);
    file << fmt::format("silver: {}\n", clan->treasure[SILVER]);
    file << fmt::format("gold: {}\n", clan->treasure[GOLD]);
    file << fmt::format("platinum: {}\n", clan->treasure[PLATINUM]);

    for (unsigned int i = 0; i < clan->rank_count; ++i) {
        file << fmt::format("title: {} {}\n", i + 1, clan->ranks[i].title);
        file << fmt::format("privilege: {}", i + 1);
        for (unsigned int j = 0; j < NUM_CLAN_PRIVS; ++j)
            if (IS_FLAGGED(clan->ranks[i].privileges, j))
                file << fmt::format(" {}", clan_privileges[j].abbr);
        file << "\n";
    }

    for (ClanMembership *member = clan->people; member; member = member->next) {
        file << fmt::format("member: {} {} {}", member->name, member->rank, member->since);
        for (ClanMembership *alt = member->relation.alts; alt; alt = alt->next)
            file << fmt::format(" {}", alt->name);
        file << "\n";
    }

    file.close();

    if (std::rename(temp_filename.c_str(), filename.c_str()) != 0) {
        log("SYSERR: Error renaming temp clan file {} to {}: {}", temp_filename, filename, strerror(errno));
    }
}

void save_clan(const Clan *clan) {
    perform_save_clan(clan);
    save_clan_index();
}

void save_clans(void) {
    for (const auto &clan : clans)
        perform_save_clan(clan.get());
    save_clan_index();
}

Clan *find_clan_by_number(unsigned int number) {
    auto it = std::find_if(clans.begin(), clans.end(), [number](const auto &clan) { return clan->number == number; });
    return it != clans.end() ? it->get() : nullptr;
}

Clan *find_clan_by_abbr(const std::string_view abbr) {
    auto it =
        std::find_if(clans.begin(), clans.end(), [abbr](const auto &clan) { return isname(abbr, clan->abbreviation); });
    return it != clans.end() ? it->get() : nullptr;
}

Clan *find_clan(const std::string_view id) {
    Clan *clan;
    if (is_integer(id))
        if ((clan = find_clan_by_number(svtoi(id))))
            return clan;
    if ((clan = find_clan_by_abbr(id)))
        return clan;
    for (const auto &clan : clans)
        if (isname(id, clan->name))
            return *clan;
    return nullptr;
}

ClanMembership *find_clan_membership_in_clan(const std::string_view name, std::unique_ptr<Clan> &clan) {
    ClanMembership *member = clan->people;

    while (member) {
        if (matches(name, member->name))
            return member;

        if (IS_ALT_RANK(member->rank)) {
            /* We're currently on an alt.  If there is another alt for
             * this member, go to that one.  Otherwise, go to the next
             * member in the clan.
             */
            if (member->next)
                member = member->next;
            else
                member = member->relation.member->next;
        } else {
            /* We're currently on a member.  If this member has alts,
             * go to the first alt in the list.  Otherwise go to the
             * next member in the clan.
             */
            if (member->relation.alts)
                member = member->relation.alts;
            else
                member = member->next;
        }
    }

    return nullptr;
}

ClanMembership *find_clan_membership(const std::string_view name) {
    ClanMembership *member;

    for (const auto &iter : clans)
        if ((member = find_clan_membership_in_clan(name, &iter)))
            return member;

    return nullptr;
}

bool revoke_clan_membership(ClanMembership *member) {
    ClanMembership *p;
    Clan *clan;
    int i;

    if (!member)
        return false;

    clan = member->clan;

    /* Remove from clan lists */
    if (IS_ALT_RANK(member->rank)) {
        p = member->relation.member;
        if (p->relation.alts == member)
            p->relation.alts = member->next;
        else
            for (p = p->relation.alts; p->next; p = p->next)
                if (p->next == member) {
                    p->next = member->next;
                    break;
                }
    } else if (clan) {
        if (clan->people == member)
            clan->people = member->next;
        else
            for (p = clan->people; p->next; p = p->next)
                if (p->next == member) {
                    p->next = member->next;
                    break;
                }
    }

    free_clan_membership(member);

    if (clan)
        save_clan(clan);

    return true;
}

void add_clan_membership(Clan *clan, ClanMembership *member) {
    ClanMembership *m;

    if (!clan->people || OUTRANKS(member->rank, clan->people->rank)) {
        member->next = clan->people;
        clan->people = member;
    } else {
        member->next = nullptr;
        for (m = clan->people; m->next; m = m->next)
            if (OUTRANKS(member->rank, m->next->rank)) {
                member->next = m->next;
                break;
            }
        m->next = member;
    }

    member->clan = clan;
}

void clan_notification(Clan *clan, CharData *skip, const std::string_view messg, ...) {
    DescriptorData *d;
    size_t found = false;
    va_list args;
    char comm_buf[MAX_STRING_LENGTH];

    if (clan && !messg.empty())
        for (d = descriptor_list; d; d = d->next)
            if (IS_PLAYING(d) && d->character && (!skip || (d->character != skip && d->original != skip)) &&
                GET_CLAN(REAL_CHAR(d->character)) == clan && !PRF_FLAGGED(d->character, PRF_NOCLANCOMM) &&
                !OUTRANKS(RANK_APPLICANT, GET_CLAN_RANK(REAL_CHAR(d->character)))) {
                if (!found) {
                    strcpy(comm_buf, AFMAG "[[ ");
                    found = strlen(comm_buf);
                    va_start(args, messg);
                    vsnprintf(comm_buf + found, sizeof(comm_buf) - found, std::string(messg).c_str(), args);
                    va_end(args);
                    strcat(comm_buf, AFMAG " ]]\n" ANRM);
                    found = true;
                }
                string_to_output(d, comm_buf);
            }
}

unsigned int clan_count(void) { return clans.size(); }

void clan_set_title(CharData *ch) {
    if (IS_CLAN_MEMBER(ch)) {
        set_title(ch, fmt::format("{} {}", GET_CLAN_TITLE(ch), GET_CLAN(ch)->abbreviation));
    } else
        set_title(ch, {});
}

Clan *alloc_clan() {
    auto new_clan = std::make_unique<Clan>();
    new_clan->number = clans.empty() ? 1 : clans.back()->number + 1;
    clans.push_back(std::move(new_clan));
    save_clan_index();
    return clans.back().get();
}

void dealloc_clan(Clan *clan) {
    auto it = std::remove_if(clans.begin(), clans.end(), [clan](const auto &c) { return c.get() == clan; });
    clans.erase(it, clans.end());
    free_clan(clan);
    save_clan_index();
}

unsigned int days_until_reapply(const ClanMembership *member) {
    if (IS_REJECT_RANK(member->rank)) {
        double diff = difftime(time(0), member->since);
        if (diff > 0)
            return diff;
    }

    return 0;
}

PRIV_FUNC(clan_admin_check) {
    if (PRV_FLAGGED(ch, PRV_CLAN_ADMIN) && GET_CLAN_MEMBERSHIP(ch))
        revoke_clan_membership(GET_CLAN_MEMBERSHIP(ch));
}

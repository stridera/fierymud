/***************************************************************************
 *   File: ban.c                                          Part of FieryMUD *
 *  Usage: banning/unbanning/checking sites and player names               *
 *                                                                         *
 *  FieryMUD enhancements by David Endre and Jimmy Kincaid                 *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "db.hpp"
#include "handler.hpp"
#include "interpreter.hpp"
#include "logging.hpp"
#include "math.hpp"
#include "structs.hpp"
#include "sysdep.hpp"
#include "utils.hpp"

#include <fstream>

std::vector<BanListElement> ban_list;
std::vector<std::string> xname_list;

const std::string_view ban_types[] = {"no", "new", "select", "all", "ERROR"};

void load_banned(void) {
    FILE *fl;
    int date, type;
    char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
    char name[MAX_NAME_LENGTH + 1];

    if (!(fl = fopen(BAN_FILE.data(), "r"))) {
        perror("Unable to open banfile");
        return;
    }
    while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4) {
        for (int i = BAN_NOT; i <= BAN_ALL; i++)
            if (matches(ban_type, ban_types[i]))
                type = i;
        ban_list.emplace_back(std::string(site_name), type, date, name);
    }

    fclose(fl);
}

bool isbanned(std::string_view hostname) {
    if (hostname.empty())
        return (0);

    for (const auto &banned_node : ban_list)
        if (matches(hostname, banned_node.site)) /* if hostname is a substring */
            return std::max(0, banned_node.type);
    return BAN_NOT;
}

void write_ban_list(void) {
    std::ofstream ban_file(BAN_FILE.data());

    if (!ban_file.is_open()) {
        perror("write_ban_list");
        return;
    }
    for (const auto &ban_node : ban_list)
        ban_file << ban_types[ban_node.type] << ' ' << ban_node.site << ' ' << ban_node.date << ' ' << ban_node.name
                 << '\n';

    ban_file.close();
}

ACMD(do_ban) {
    if (argument.empty()) {
        if (ban_list.empty()) {
            char_printf(ch, "No sites are banned.\n");
            return;
        }
        std::string_view format{"{:<25}  {:<8}  {:<11}  {:16}\n"};
        char_printf(ch, format, "Banned Site Name", "Ban Type", "Banned On", "Banned By");
        char_printf(ch, format, "---------------------------------", "---------------------------------",
                    "---------------------------------", "---------------------------------");

        for (auto ban_node : ban_list) {
            std::string_view site;
            if (ban_node.date) {
                site = fmt::format(TIMEFMT_DATE, fmt::localtime(ban_node.date));
            } else
                site = "Unknown";
            char_printf(ch, format, ban_node.site, ban_types[ban_node.type], site, ban_node.name);
        }

        return;
    }
    auto flag = argument.shift();
    auto site = argument.shift();
    if (site.empty() || flag.empty()) {
        char_printf(ch, "Usage: ban {all | select | new} site_name\n");
        return;
    }
    if (!(matches(flag, "select") || matches(flag, "all") || matches(flag, "new"))) {
        char_printf(ch, "Flag must be ALL, SELECT, or NEW.\n");
        return;
    }
    for (auto ban_node : ban_list) {
        if (matches(ban_node.site, site)) {
            char_printf(ch, "That site has already been banned -- unban it to change the ban type.\n");
            return;
        }
    }

    int type = BAN_NEW;
    for (int i = BAN_NOT; i <= BAN_ALL; i++)
        if (matches(flag, ban_types[i]))
            type = i;
    ban_list.push_back(BanListElement(std::string(site), type, time(0), GET_NAME(ch)));

    log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "{} has banned {} for {} players.", GET_NAME(ch),
        site, ban_types[type]);
    char_printf(ch, "Site banned.\n");
    write_ban_list();
}

ACMD(do_unban) {
    auto site = argument.shift();
    if (site.empty()) {
        char_printf(ch, "A site to unban might help.\n");
        return;
    }
    // Find and remove the ban_list in the std::vector
    auto ban_node = std::find_if(ban_list.begin(), ban_list.end(),
                                 [&site](const BanListElement &ban) { return matches(ban.site, site); });
    if (ban_node == ban_list.end()) {
        char_printf(ch, "That site is not currently banned.\n");
        return;
    }
    ban_list.erase(ban_node);
    char_printf(ch, "Site unbanned.\n");
    log(LogSeverity::Stat, std::max<int>(LVL_GOD, GET_INVIS_LEV(ch)), "{} removed the {}-player ban on {}.",
        GET_NAME(ch), ban_types[ban_node->type], ban_node->site);

    write_ban_list();
}

/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

bool is_valid_name(std::string_view newname) {
    DescriptorData *dt;
    /*
     * Make sure someone isn't trying to create this same name.  We want to
     * do a 'strcasecmp' so people can't do 'Bob' and 'BoB'.  The creating login
     * will not have a character name yet and other people sitting at the
     * prompt won't have characters yet.
     */
    for (dt = descriptor_list; dt; dt = dt->next) {
        if (dt->character && !GET_NAME(dt->character).empty() && matches(GET_NAME(dt->character), newname)) {
            return (STATE(dt) == CON_PLAYING);
        }
    }
    /* return valid if list doesn't exist */
    if (xname_list.empty()) {
        log("no xname_list found");
        return true;
    }

    /* Does the desired name contain a string in the invalid list? */

    /* Tested, fixed  & commented by David Endre 1/13/99
       We check the first character for a '#', if it contains one then
       we allow the name to contain that word, but not be only that
       word. Ex: #cum in the xname file...cucumber would be allowed
       along with cumter, however cum would not be allowed. If the name
       doesn't contain '#' then we go on as usual. Original idea and
       code by Therlos on 1/13/99
     */
    for (auto invalid_name : xname_list) {
        if (invalid_name[0] == '#') {
            if (matches(invalid_name.substr(1), newname))
                return false;
        } else {
            if (matches(invalid_name, newname))
                return false;
        }
    }

    /* is there a mobile with that name? */
    for (int i = 0; i < top_of_mobt; i++) {
        if (isname(newname, mob_proto[i].player.namelist))
            return false;
    }

    return true;
}

/* rewritten by gurlaek 6/9/1999 */
void Read_Xname_List(void) {
    std::ifstream fp(XNAME_FILE.data());
    std::string string;

    if (!fp.is_open()) {
        perror("Unable to open invalid name file");
        return;
    }
    while (std::getline(fp, string)) {
        /* make sure we don't load a blank xname --gurlaek 6/12/1999 */
        if (string.empty() || string[0] == '\n' || string[0] == '\r')
            continue;

        xname_list.push_back(string);
    }

    fp.close();
}

void reload_xnames() { Read_Xname_List(); }

/* send rejected names to the xnames file */
void send_to_xnames(std::string_view name) {
    std::ofstream xnames(XNAME_FILE.data(), std::ios::app);

    if (!xnames.is_open()) {
        log(LogSeverity::Warn, LVL_IMMORT, "SYSERR: Cannot open xnames file.\n");
        return;
    }

    /* print it to the xnames file with # prepended */
    auto tempname = to_lower(fmt::format("#{}\n", name));
    xname_list.push_back(tempname);
    xnames << tempname;
    xnames.close();
}

ACMD(do_xnames) {
    auto flag = argument.shift();
    auto name = argument.shift();

    if (!flag.empty() && matches(flag, "reload")) {
        reload_xnames();
        char_printf(ch, "Done.\n");
        return;
    }

    if (flag.empty() || name.empty()) {
        char_printf(ch, "Usage: xnames {add NAME | reload}\n");
        return;
    }

    if (matches(flag, "add")) {
        if (!is_valid_name(name)) {
            char_printf(ch, "Name is already banned.\n");
            return;
        }
        send_to_xnames(name);
        char_printf(ch, "Done.\n");
    } else {
        char_printf(ch, "Usage: xnames {add NAME | reload}\n");
    }
}

#include "act.hpp"

#include "arguments.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "find.hpp"
#include "handler.hpp"
#include "logging.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "money.hpp"
#include "objects.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "utils.hpp"
#include "function_registration.hpp"

#include <expected>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <vector>

#define CLANCMD(name) static void(name)(CharData * ch, Arguments argument)

// Fuzzy string matching utilities
namespace {
    // Calculate Levenshtein distance between two strings
    size_t levenshtein_distance(std::string_view s1, std::string_view s2) {
        const size_t len1 = s1.length();
        const size_t len2 = s2.length();

        // Create a matrix to store distances
        std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));

        // Initialize first row and column
        for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
        for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;

        // Fill the matrix
        for (size_t i = 1; i <= len1; ++i) {
            for (size_t j = 1; j <= len2; ++j) {
                if (std::tolower(s1[i-1]) == std::tolower(s2[j-1])) {
                    dp[i][j] = dp[i-1][j-1]; // No operation needed
                } else {
                    dp[i][j] = 1 + std::min({
                        dp[i-1][j],     // Deletion
                        dp[i][j-1],     // Insertion
                        dp[i-1][j-1]    // Substitution
                    });
                }
            }
        }

        return dp[len1][len2];
    }

    // Calculate fuzzy match score (0.0 = no match, 1.0 = perfect match)
    double fuzzy_match_score(std::string_view target, std::string_view query) {
        if (query.empty()) return 0.0;
        if (target.empty()) return 0.0;

        // Exact match gets perfect score
        if (std::equal(target.begin(), target.end(), query.begin(), query.end(),
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            return 1.0;
        }

        // Prefix match gets high score
        if (target.length() >= query.length()) {
            bool is_prefix = std::equal(query.begin(), query.end(), target.begin(),
                                       [](char a, char b) { return std::tolower(a) == std::tolower(b); });
            if (is_prefix) {
                return 0.9;
            }
        }

        // Substring match gets good score
        std::string target_lower(target);
        std::string query_lower(query);
        std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);
        std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

        if (target_lower.find(query_lower) != std::string::npos) {
            return 0.8;
        }

        // Use Levenshtein distance for fuzzy matching
        size_t distance = levenshtein_distance(target, query);
        size_t max_len = std::max(target.length(), query.length());

        if (distance > max_len) return 0.0;

        // Score based on how many edits are needed relative to string length
        double similarity = 1.0 - (static_cast<double>(distance) / max_len);

        // Only consider it a fuzzy match if similarity is above threshold
        return similarity > 0.5 ? similarity : 0.0;
    }

    // Find best fuzzy matches for clan names/abbreviations
    std::vector<std::pair<ClanPtr, double>> find_fuzzy_clan_matches(std::string_view query, size_t max_results = 3) {
        std::vector<std::pair<ClanPtr, double>> matches;

        for (const auto &clan : clan_repository.all()) {
            // Check both name and abbreviation (with and without ANSI codes)
            double name_score = std::max(
                fuzzy_match_score(clan->name(), query),
                fuzzy_match_score(strip_ansi(clan->name()), query)
            );

            double abbr_score = std::max(
                fuzzy_match_score(clan->abbreviation(), query),
                fuzzy_match_score(strip_ansi(clan->abbreviation()), query)
            );

            double best_score = std::max(name_score, abbr_score);

            if (best_score > 0.0) {
                matches.emplace_back(clan, best_score);
            }
        }

        // Sort by score (highest first)
        std::sort(matches.begin(), matches.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });

        // Limit results
        if (matches.size() > max_results) {
            matches.resize(max_results);
        }

        return matches;
    }

    // Helper function to find a clan with full fuzzy search support
    std::pair<std::optional<ClanPtr>, bool> find_clan_with_fuzzy_search(std::string_view clan_name) {
        if (clan_name.empty()) {
            return {std::nullopt, false};
        }

        // Try exact matches first
        auto found_clan = clan_repository.find_by_abbreviation(clan_name);
        if (!found_clan) {
            found_clan = clan_repository.find_by_name(clan_name);
        }

        // If not found, try matching against stripped ANSI versions
        if (!found_clan) {
            for (const auto &c : clan_repository.all()) {
                if (strip_ansi(c->abbreviation()) == clan_name || strip_ansi(c->name()) == clan_name) {
                    found_clan = c;
                    break;
                }
            }
        }

        // If still not found, try fuzzy matching
        bool used_fuzzy = false;
        if (!found_clan) {
            auto fuzzy_matches = find_fuzzy_clan_matches(clan_name, 1);
            if (!fuzzy_matches.empty() && fuzzy_matches[0].second >= 0.7) {
                found_clan = fuzzy_matches[0].first;
                used_fuzzy = true;
            }
        }

        return {found_clan, used_fuzzy};
    }

    // Helper function to show clan suggestions when lookup fails
    void show_clan_suggestions(CharData *ch, std::string_view searched_name) {
        auto fuzzy_matches = find_fuzzy_clan_matches(searched_name, 3);
        if (!fuzzy_matches.empty()) {
            char_printf(ch, "Did you mean:\n");
            for (const auto& [suggested_clan, score] : fuzzy_matches) {
                char_printf(ch, "  {} ({})\n",
                           strip_ansi(suggested_clan->name()),
                           strip_ansi(suggested_clan->abbreviation()));
            }
        }
    }
}
enum ClanPermissions : PermissionFlags {
    Everybody = 0,
    ClanMember = 1,
    ClanAdmin = 2,
    God = 4,
    NonClanMember = 8,  // For commands that should only be visible to non-clan members
};

// Determine clan permission flags for a character
static PermissionFlags get_clan_permissions(CharData *ch) {
    PermissionFlags flags = ClanPermissions::Everybody;

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        flags |= ClanPermissions::God;
        flags |= ClanPermissions::ClanAdmin;
        flags |= ClanPermissions::ClanMember;
        flags |= ClanPermissions::NonClanMember;  // Gods can see everything
    } else {
        auto membership = ch->player_specials->clan_membership;
        if (membership.has_value()) {
            flags |= ClanPermissions::ClanMember;
            // Check if they have admin privileges
            if (membership.value()->has_permission(ClanPrivilege::Admin) ||
                membership.value()->has_permission(ClanPrivilege::Grant)) {
                flags |= ClanPermissions::ClanAdmin;
            }
        } else {
            flags |= ClanPermissions::NonClanMember;  // Not in a clan
        }
    }

    return flags;
}

// Get the clan from the character. Gods always need to specify the clan.
static std::shared_ptr<ClanMembership> find_memberships(CharData *ch, Arguments &argument) {
    auto membership = ch->player_specials->clan_membership;
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (!membership.has_value()) {
            char_printf(ch, "You are not a member of a clan.\n");
            return nullptr;
        }

        // For single membership, just return it
        return membership.value();
    }

    if (argument.empty()) {
        if (membership.has_value()) {
            return membership.value();
        }
        char_printf(ch, "Please specify a clan.\n");
        return nullptr;
    }

    // Gods can specify the clan. We'll create a temporary membership for them.
    std::string searched_clan_name;
    bool used_fuzzy_match = false;
    auto clan = [&argument, &searched_clan_name, &used_fuzzy_match]() -> std::optional<ClanPtr> {
        auto clan_id_opt = argument.try_shift_number();
        if (clan_id_opt) {
            searched_clan_name = std::to_string(*clan_id_opt);
            return clan_repository.find_by_id(*clan_id_opt);
        }
        auto clan_name = argument.shift();
        searched_clan_name = std::string(clan_name);

        auto [found_clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);
        used_fuzzy_match = used_fuzzy;

        return found_clan;
    }();

    if (!clan) {
        char_printf(ch, "No such clan '{}'.\n", searched_clan_name);

        // Provide fuzzy search suggestions
        show_clan_suggestions(ch, searched_clan_name);

        // Debug: List available clans for gods
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            char_printf(ch, "\nAll available clans:\n");
            for (const auto &c : clan_repository.all()) {
                char_printf(ch, "  ID: {} Name: \"{}\" (stripped: \"{}\") Abbr: \"{}\" (stripped: \"{}\")\n",
                           c->id(), c->name(), strip_ansi(c->name()), c->abbreviation(), strip_ansi(c->abbreviation()));
            }
        }
        return nullptr;
    }

    // Notify user if we used fuzzy matching to find the clan
    if (used_fuzzy_match) {
        char_printf(ch, "Assuming you meant '{}' ({}).\n",
                   strip_ansi((*clan)->name()), strip_ansi((*clan)->abbreviation()));
    }

    auto me = std::shared_ptr<CharData>(ch, [](CharData *) { /* Non-owning */ });
    if (auto membership = (*clan)->get_membership(me)) {
        return *membership;
    }

    // Create a temporary membership for gods with all permissions
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        PermissionSet perms;
        // Set all privileges for gods
        perms.set(); // Set all bits
        
        // Create a non-owning shared_ptr for the character
        auto char_shared = std::shared_ptr<CharData>(ch, [](CharData *) { /* Non-owning deleter */ });
        
        // Create god membership directly - the "God" rank title allows bypassing character checks
        auto membership = std::make_shared<ClanMembership>(*clan, char_shared, ClanRank("God", perms));
        
        return membership;
    }

    char_printf(ch, "You are not a member of that clan.\n");
    return nullptr;
}

CLANCMD(clan_list) {
    if (clan_repository.count() == 0) {
        char_printf(ch, "No clans have formed yet.\n");
        return;
    }

    /* List clans, # of members, power, and app fee */
    paging_printf(ch, AUND " Num   Clan           Members  App Fee/Lvl\n" ANRM);
    for (const auto &clan : clan_repository.all()) {
        paging_printf(ch, fmt::format("[{:3}] {:<{}} {:3}   {:5}p/{:3}\n", clan->id(), clan->abbreviation(),
                                      18 + count_color_chars(clan->abbreviation()) * 2, clan->member_count(),
                                      clan->app_fee(), clan->min_application_level()));
    }

    start_paging(ch);
}
REGISTER_FUNCTION(clan_list, "clan_list", 0, ClanPermissions::Everybody,
                "list                         - List all clans and their application fees");

CLANCMD(clan_deposit) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    // Check if clan bank is accessible from this room (gods can access from anywhere)
    auto clan = membership->get_clan();
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (clan && clan->bank_room() != NOWHERE && ch->in_room != clan->bank_room()) {
            char_printf(ch, "You can only access the clan bank from the designated bank room.\n");
            return;
        }

        // If bank_room is NOWHERE, bank access is disabled
        if (clan && clan->bank_room() == NOWHERE) {
            char_printf(ch, "Clan bank access is currently disabled.\n");
            return;
        }
    }

    auto arg = argument.shift();

    auto money_opt = parse_money(argument.get());
    if (!money_opt || money_opt->value() <= 0) {
        char_printf(ch, "How much do you want to deposit?\n");
        return;
    }

    Money coins = *money_opt;
    /* Gods have bottomless pockets */
    if (GET_LEVEL(ch) < LVL_GOD) {
        Money owned = ch->points.money;
        if (!owned.charge(coins)) {
            char_printf(ch, "You do not have that kind of money!\n");
            return;
        }
        for (int i = 0; i < NUM_COIN_TYPES; i++) {
            ch->points.money[i] -= coins[i];
        }
    }

    if (auto result = membership->add_clan_treasure(coins); !result) {
        char_printf(ch, "{}.\n", result.error());
        return;
    }

    save_player_char(ch);

    char_printf(ch, "You {}'s account: {}\n", membership->get_clan_abbreviation(), statemoney(coins));
    clan_repository.save();
}
REGISTER_FUNCTION(clan_deposit, "clan_deposit", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "deposit <clan> <amount>      - Deposit money into the clan treasury");

CLANCMD(clan_withdraw) {

    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    // Check if clan bank is accessible from this room (gods can access from anywhere)
    auto clan = membership->get_clan();
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (clan && clan->bank_room() != NOWHERE && ch->in_room != clan->bank_room()) {
            char_printf(ch, "You can only access the clan bank from the designated bank room.\n");
            return;
        }

        // If bank_room is NOWHERE, bank access is disabled
        if (clan && clan->bank_room() == NOWHERE) {
            char_printf(ch, "Clan bank access is currently disabled.\n");
            return;
        }
    }

    auto arg = argument.shift();

    auto money_opt = parse_money(argument.get());
    if (!money_opt || money_opt->value() <= 0) {
        char_printf(ch, "How much do you want to withdraw?\n");
        return;
    }

    Money coins = *money_opt;

    auto treasure = membership->get_clan_treasure();
    if (treasure.value() < coins.value()) {
        char_printf(ch, "The clan is not wealthy enough for your needs!\n");
        return;
    }

    if (auto result = membership->subtract_clan_treasure(coins); !result) {
        char_printf(ch, "{}.\n", result.error());
        return;
    }

    for (int i = 0; i < NUM_COIN_TYPES; i++) {
        ch->points.money[i] -= coins[i];
    }
    save_player_char(ch);

    char_printf(ch, "You withdraw from  {}'s account: {}\n", membership->get_clan_abbreviation(), statemoney(coins));
    clan_repository.save();
}
REGISTER_FUNCTION(clan_withdraw, "clan_withdraw", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "withdraw <clan> <amount>     - Withdraw money from the clan treasury");

CLANCMD(clan_store) {
    ObjData *obj;

    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    // Check if clan chest is accessible from this room (gods can access from anywhere)
    auto clan = membership->get_clan();
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (clan && clan->chest_room() != NOWHERE && ch->in_room != clan->chest_room()) {
            char_printf(ch, "You can only access the clan chest from the designated chest room.\n");
            return;
        }

        // If chest_room is NOWHERE, chest access is disabled
        if (clan && clan->chest_room() == NOWHERE) {
            char_printf(ch, "Clan chest access is currently disabled.\n");
            return;
        }
    }

    auto arg_pair = argument.try_shift_number_and_arg();

    if (!arg_pair || arg_pair->second.empty()) {
        char_printf(ch, "What do you want to store?\n");
        return;
    }

    if (arg_pair->first && arg_pair->first != 1) {
        char_printf(ch, "You can only store one item at a time.\n");
        return;
    }

    auto obj_name = arg_pair->second;
    if (!(obj =
              find_obj_in_list(ch->carrying, find_vis_by_name(ch, const_cast<char *>(std::string(obj_name).data()))))) {
        char_printf(ch, "You aren't carrying {} {}.\n", an(obj_name), obj_name);
    } else if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        act("You can't store $p in $P because it's CURSED!", false, ch, obj, nullptr, TO_CHAR);
    } else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {
        char_printf(ch, "You can't store a container with items in it.\n");
    } else if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
        char_printf(ch, "You can't store money in the clan vault.\n");
    } else if (GET_OBJ_VNUM(obj) == -1) {
        char_printf(ch, "That item is much too unique to store.\n");
    } else {
        obj_from_char(obj);
        auto vnum = GET_OBJ_VNUM(obj);
        auto name = obj->short_description;
        free_obj(obj);

        char_printf(ch, "You store {} in the clan vault.\n", name);
        if (auto result = membership->add_clan_storage_item(vnum, 1); !result) {
            char_printf(ch, "{}.\n", result.error());
            return;
        }

        membership->notify_clan("{} stores {} in the clan vault.", GET_NAME(ch), name);
    }
}
REGISTER_FUNCTION(clan_store, "clan_store", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "store <clan> <item>          - Store an item in the clan vault");

CLANCMD(clan_retrieve) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    // Check if clan chest is accessible from this room (gods can access from anywhere)
    auto clan = membership->get_clan();
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (clan && clan->chest_room() != NOWHERE && ch->in_room != clan->chest_room()) {
            char_printf(ch, "You can only access the clan chest from the designated chest room.\n");
            return;
        }

        // If chest_room is NOWHERE, chest access is disabled
        if (clan && clan->chest_room() == NOWHERE) {
            char_printf(ch, "Clan chest access is currently disabled.\n");
            return;
        }
    }

    auto arg_pair = argument.try_shift_number_and_arg();

    if (!arg_pair || arg_pair->second.empty()) {
        char_printf(ch, "What do you want to retrieve?\n");
        return;
    }

    if (arg_pair->first && arg_pair->first != 1) {
        char_printf(ch, "You can only retrieve one item at a time.\n");
        return;
    }

    auto obj_name = arg_pair->second;
    // We need to find the vnum in the storage that matches this name
    ObjectId obj_vnum = 0;
    bool found = false;

    // Find the object in storage matching the name
    // Build a temporary cache for faster lookup
    std::unordered_map<std::string, ObjectId> name_to_vnum_cache;
    for (const auto &[vnum, count] : membership->get_clan_storage()) {
        if (count <= 0)
            continue;

        auto obj = read_object(vnum, VIRTUAL);
        if (!obj)
            continue;

        // Cache all possible names for this object
        std::string names = obj->name;
        std::string name_token;
        std::istringstream name_stream(names);
        while (name_stream >> name_token) {
            name_to_vnum_cache[name_token] = vnum;
        }
        free_obj(obj);
    }

    // Quick lookup in cache
    auto cache_it = name_to_vnum_cache.find(std::string(obj_name));
    if (cache_it != name_to_vnum_cache.end()) {
        obj_vnum = cache_it->second;
        found = true;
    }

    if (!found) {
        char_printf(ch, "There is no {} {} in the clan vault.\n", an(obj_name), obj_name);
        return;
    }

    auto obj = read_object(obj_vnum, VIRTUAL);
    if (!obj) {
        char_printf(ch, "Error retrieving object.\n");
        return;
    }

    obj_to_char(obj, ch);

    char_printf(ch, "You retrieve {} from the clan vault.\n", obj->short_description);
    if (auto result = membership->remove_clan_storage_item(obj_vnum, 1); !result) {
        char_printf(ch, "{}.\n", result.error());
        return;
    }

    membership->notify_clan("{} retrieves {} from the clan vault.", GET_NAME(ch), obj->short_description);
}
REGISTER_FUNCTION(clan_retrieve, "clan_retrieve", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "retrieve <clan> <item>       - Retrieve an item from the clan vault");

CLANCMD(clan_tell) {
    CharData *me = REAL_CHAR(ch);

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        char_printf(ch, "Your lips move, but no sound forms.\n");
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    if (argument.empty()) {
        char_printf(ch, "What do you want to tell the clan?\n");
        return;
    }

    auto speech = drunken_speech(std::string(argument.get()), GET_COND(ch, DRUNK));

    char_printf(ch, AFMAG "You tell {}" AFMAG ", '" AHMAG "{}" AFMAG "'\n" ANRM,
                ch->player.level < LVL_IMMORT ? "your clan" : membership->get_clan_abbreviation(), speech);

    for (auto member : membership->get_clan_members()) {
        // Find the online character by name
        auto target = find_char_in_world(find_by_name(const_cast<char*>(member.name.c_str())));
        if (!target || !target->desc || !IS_PLAYING(target->desc))
            continue;

        auto tch = REAL_CHAR(target);
        if (!tch || tch == me)
            continue;

        if (STATE(target->desc) != CON_PLAYING || PLR_FLAGGED(tch, PLR_WRITING) || PLR_FLAGGED(tch, PLR_MAILING) ||
            EDITING(target->desc)) {
            if (!PRF_FLAGGED(tch, PRF_OLCCOMM))
                continue;
        }

        if (PRF_FLAGGED(tch, PRF_NOCLANCOMM))
            continue;

        char_printf(tch, AFMAG "{} tells {}" AFMAG ", '" AHMAG "{}" AFMAG "'\n" ANRM,
                    GET_INVIS_LEV(me) > GET_LEVEL(tch) ? "Someone" : GET_NAME(me),
                    tch->player.level < LVL_IMMORT ? "your clan" : membership->get_clan_abbreviation(), speech);
    }
    
    // Send to snooping gods
    auto clan_id_opt = membership->get_clan_id();
    if (clan_id_opt) {
        auto clan_id = clan_id_opt.value();
        auto it = clan_snoop_table.find(clan_id);
        if (it != clan_snoop_table.end()) {
            for (auto snoop_ch : it->second) {
                if (!snoop_ch || !snoop_ch->desc || snoop_ch == me)
                    continue;
                    
                if (STATE(snoop_ch->desc) != CON_PLAYING)
                    continue;
                    
                // Send snoop message with special formatting
                char_printf(snoop_ch, AFYEL "[SNOOP {}] {} tells clan, '" AHMAG "{}" AFYEL "'\n" ANRM,
                           membership->get_clan_abbreviation(),
                           GET_INVIS_LEV(me) > GET_LEVEL(snoop_ch) ? "Someone" : GET_NAME(me),
                           speech);
            }
        }
    }
}
REGISTER_FUNCTION(clan_tell, "clan_tell", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "tell <clan> <message>        - Send a message to all clan members");

CLANCMD(clan_set) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto cmd = argument.shift();

    if (cmd.empty()) {
        char_printf(ch, "Available clan set options:\n");
        char_printf(ch, "  abbr <text>           - Set clan abbreviation\n");
        char_printf(ch, "  addrank <title>       - Add a new rank\n");
        char_printf(ch, "  appfee <amount>       - Set application fee (platinum)\n");
        char_printf(ch, "  applev <level>        - Set minimum application level\n");
        char_printf(ch, "  delrank <number>      - Delete a rank\n");
        char_printf(ch, "  dues <amount>         - Set monthly dues (platinum)\n");
        char_printf(ch, "  name <text>           - Set clan name\n");
        char_printf(ch, "  permissions <rank> <list> - Set permissions for a rank\n");
        char_printf(ch, "  title <rank> <text>   - Set title for a rank\n");
        if (GET_LEVEL(ch) >= LVL_GOD) {
            char_printf(ch, "  bankroom <vnum>       - Set bank room (-1 to disable, gods only)\n");
            char_printf(ch, "  chestroom <vnum>      - Set chest room (-1 to disable, gods only)\n");
        }
        return;
    }

    if (matches_start(cmd, "abbr")) {
        auto new_abbreviation = argument.get();
        if (new_abbreviation.empty()) {
            char_printf(ch, "What do you want to set the clan abbreviation to?\n");
            return;
        }

        if (ansi_strlen(new_abbreviation) > Clan::MAX_CLAN_ABBR_LEN) {
            char_printf(ch, "Clan abbreviations may be at most {} characters in length.\n", Clan::MAX_CLAN_ABBR_LEN);
            return;
        }

        auto result = membership->update_clan_abbreviation(std::string(new_abbreviation));
        if (result) {
            char_printf(ch, "Clan abbreviation updated to {}.\n", new_abbreviation);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "addrank")) {
        if (argument.empty()) {
            char_printf(ch, "What do you want to name the new rank?\n");
            return;
        }

        auto title = argument.get();
        // Create a default rank with empty privileges
        PermissionSet privileges;
        ClanRank rank(std::string(title), privileges);

        auto result = membership->update_clan_rank(rank);
        if (result) {
            char_printf(ch, "New rank '{}' added.\n", title);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "appfee")) {
        auto fee_opt = argument.try_shift_number();
        if (!fee_opt) {
            char_printf(ch, "How much platinum should the clan's application fee be?\n");
            return;
        }

        if (*fee_opt < 0) {
            char_printf(ch, "Application fee cannot be negative.\n");
            return;
        }

        auto result = membership->update_clan_app_fee(*fee_opt);
        if (result) {
            char_printf(ch, "{}'s application fee is now {} platinum.\n", membership->get_clan_name(), *fee_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "applev")) {
        auto level_opt = argument.try_shift_number();
        if (!level_opt) {
            char_printf(ch, "What should the clan's minimum application level be?\n");
            return;
        }

        if (*level_opt < 1 || *level_opt > LVL_IMPL) {
            char_printf(ch, "The minimum application level must be between 1 and {}.\n", LVL_IMPL);
            return;
        }

        auto result = membership->update_clan_min_application_level(*level_opt);
        if (result) {
            char_printf(ch, "{}'s minimum application level is now {}.\n", membership->get_clan_name(), *level_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "delrank")) {
        auto rank_opt = argument.try_shift_number();
        if (!rank_opt) {
            char_printf(ch, "Which rank do you want to delete?\n");
            return;
        }

        if (*rank_opt < 1) {
            char_printf(ch, "Rank number must be positive.\n");
            return;
        }

        auto ranks = membership->get_clan_ranks();
        if (*rank_opt > ranks.size()) {
            char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", ranks.size());
            return;
        }

        auto result = membership->delete_clan_rank(*rank_opt - 1);  // Convert to 0-based index
        if (result) {
            char_printf(ch, "Rank {} deleted.\n", *rank_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "dues")) {
        auto dues_opt = argument.try_shift_number();
        if (!dues_opt) {
            char_printf(ch, "How much platinum should the clan's dues be?\n");
            return;
        }

        if (*dues_opt < 0) {
            char_printf(ch, "Dues cannot be negative.\n");
            return;
        }

        auto result = membership->update_clan_dues(*dues_opt);
        if (result) {
            char_printf(ch, "{}'s monthly dues are now {} platinum.\n", membership->get_clan_name(), *dues_opt);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "name")) {
        auto new_name = argument.get();
        if (new_name.empty()) {
            char_printf(ch, "What do you want to name the clan?\n");
            return;
        }

        if (ansi_strlen(new_name) > Clan::MAX_CLAN_NAME_LEN) {
            char_printf(ch, "Clan names may be at most {} characters in length.\n", Clan::MAX_CLAN_NAME_LEN);
            return;
        }

        auto result = membership->update_clan_name(std::string{new_name});
        if (result) {
            char_printf(ch, "Clan name updated to {}.\n", new_name);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "permissions")) {
        auto rank_opt = argument.try_shift_number();
        if (!rank_opt) {
            char_printf(ch, "For which rank do you want to set permissions?\n");
            char_printf(ch, "Usage: clan set permissions <rank> <permission1> [permission2] ...\n");
            char_printf(ch, "Available permissions: description motd grant ranks title enroll expel\n");
            char_printf(ch, "                       promote demote app_fees app_level dues deposit\n");
            char_printf(ch, "                       withdraw store retrieve alts chat all none\n");
            return;
        }

        if (*rank_opt < 1) {
            char_printf(ch, "Rank number must be positive.\n");
            return;
        }

        auto ranks = membership->get_clan_ranks();
        if (*rank_opt > ranks.size()) {
            char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", ranks.size());
            return;
        }

        if (argument.empty()) {
            char_printf(ch, "What permissions do you want to set for rank {}?\n", *rank_opt);
            char_printf(ch, "Available permissions: description motd grant ranks title enroll expel\n");
            char_printf(ch, "                       promote demote app_fees app_level dues deposit\n");
            char_printf(ch, "                       withdraw store retrieve alts chat all none\n");
            return;
        }

        // Get the current rank
        auto current_rank = ranks[*rank_opt - 1];
        PermissionSet new_permissions;
        
        // Parse permission list
        std::string perm_str;
        while (!(perm_str = std::string(argument.shift())).empty()) {
            if (perm_str == "all") {
                new_permissions.set(); // Set all bits
            } else if (perm_str == "none") {
                new_permissions.reset(); // Clear all bits
            } else if (perm_str == "description") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Description));
            } else if (perm_str == "motd") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Motd));
            } else if (perm_str == "grant") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Grant));
            } else if (perm_str == "ranks") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Ranks));
            } else if (perm_str == "title") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Title));
            } else if (perm_str == "enroll") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Enroll));
            } else if (perm_str == "expel") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Expel));
            } else if (perm_str == "promote") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Promote));
            } else if (perm_str == "demote") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Demote));
            } else if (perm_str == "app_fees") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::App_Fees));
            } else if (perm_str == "app_level") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::App_Level));
            } else if (perm_str == "dues") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Dues));
            } else if (perm_str == "deposit") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Deposit));
            } else if (perm_str == "withdraw") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Withdraw));
            } else if (perm_str == "store") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Store));
            } else if (perm_str == "retrieve") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Retrieve));
            } else if (perm_str == "alts") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Alts));
            } else if (perm_str == "chat") {
                new_permissions.set(static_cast<size_t>(ClanPrivilege::Chat));
            } else {
                char_printf(ch, "Unknown permission '{}'. Use 'clan set permissions {}' to see available permissions.\n", 
                           perm_str, *rank_opt);
                return;
            }
        }

        // Update the rank permissions directly
        auto result = membership->update_clan_rank_permissions(*rank_opt - 1, new_permissions);
        if (result) {
            char_printf(ch, "Permissions updated for rank {} ({}).\n", *rank_opt, current_rank.title());
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "title")) {
        auto rank_opt = argument.try_shift_number();
        if (!rank_opt) {
            char_printf(ch, "For which rank do you want to set a title?\n");
            return;
        }

        if (*rank_opt < 1) {
            char_printf(ch, "Rank number must be positive.\n");
            return;
        }

        auto title = argument.get();
        if (title.empty()) {
            char_printf(ch, "What title do you want to set?\n");
            return;
        }

        if (ansi_strlen(title) > Clan::MAX_CLAN_TITLE_LEN) {
            char_printf(ch, "Clan titles may be at most {} characters long.\n", Clan::MAX_CLAN_TITLE_LEN);
            return;
        }

        auto ranks = membership->get_clan_ranks();
        if (*rank_opt > ranks.size()) {
            char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", ranks.size());
            return;
        }

        auto result = membership->update_clan_rank_title(*rank_opt - 1, std::string(title));  // Convert to 0-based index
        if (result) {
            char_printf(ch, "Rank {} title set to '{}.'\n", *rank_opt, title);
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "bankroom")) {
        if (GET_LEVEL(ch) < LVL_GOD) {
            char_printf(ch, "Only gods can set clan bank rooms.\n");
            return;
        }

        auto room_vnum_opt = argument.try_shift_number();
        if (!room_vnum_opt) {
            char_printf(ch, "What room vnum should be the clan's bank room? (Use -1 to disable bank access)\n");
            return;
        }

        room_num room_vnum = *room_vnum_opt;

        // Validate room exists if positive vnum
        if (room_vnum > 0 && real_room(room_vnum) == NOWHERE) {
            char_printf(ch, "Room {} does not exist.\n", room_vnum);
            return;
        }

        auto result = membership->update_clan_bank_room(room_vnum);
        if (result) {
            if (room_vnum == NOWHERE) {
                char_printf(ch, "Clan bank access disabled.\n");
            } else {
                char_printf(ch, "Clan bank room set to {}.\n", room_vnum);
            }
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else if (matches_start(cmd, "chestroom")) {
        if (GET_LEVEL(ch) < LVL_GOD) {
            char_printf(ch, "Only gods can set clan chest rooms.\n");
            return;
        }

        auto room_vnum_opt = argument.try_shift_number();
        if (!room_vnum_opt) {
            char_printf(ch, "What room vnum should be the clan's chest room? (Use -1 to disable chest access)\n");
            return;
        }

        room_num room_vnum = *room_vnum_opt;

        // Validate room exists if positive vnum
        if (room_vnum > 0 && real_room(room_vnum) == NOWHERE) {
            char_printf(ch, "Room {} does not exist.\n", room_vnum);
            return;
        }

        auto result = membership->update_clan_chest_room(room_vnum);
        if (result) {
            if (room_vnum == NOWHERE) {
                char_printf(ch, "Clan chest access disabled.\n");
            } else {
                char_printf(ch, "Clan chest room set to {}.\n", room_vnum);
            }
        } else {
            char_printf(ch, "{}.\n", result.error());
            return;
        }
    } else {
        char_printf(ch, "Unknown clan set option '{}'. Use 'clan set' to see available options.\n", cmd);
        return;
    }

    clan_repository.save();
}
REGISTER_FUNCTION(clan_set, "clan_set", 0, ClanPermissions::ClanAdmin | ClanPermissions::God,
                "set <clan> <option>          - Set clan properties (admin only)");


CLANCMD(clan_apply) {
    if (argument.empty()) {
        char_printf(ch, "Apply to which clan?\n");
        return;
    }

    auto clan_name = argument.shift();
    auto [clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);

    if (!clan) {
        char_printf(ch, "No such clan exists.\n");
        show_clan_suggestions(ch, clan_name);
        return;
    }

    if (used_fuzzy) {
        char_printf(ch, "Assuming you meant '{}' ({}).\n",
                   strip_ansi((*clan)->name()), strip_ansi((*clan)->abbreviation()));
    }

    // Check if character meets application requirements
    if (GET_LEVEL(ch) < (*clan)->min_application_level()) {
        char_printf(ch, "You must be at least level {} to apply to {}.\n",
                   (*clan)->min_application_level(), (*clan)->name());
        return;
    }

    // Check if character can afford application fee
    if (GET_LEVEL(ch) < LVL_IMMORT && (*clan)->app_fee() > 0) {
        Money app_fee;
        app_fee[PLATINUM] = (*clan)->app_fee();
        Money owned = ch->points.money;
        if (!owned.charge(app_fee)) {
            char_printf(ch, "You cannot afford the {} platinum application fee.\n", (*clan)->app_fee());
            return;
        }

        // Charge the fee
        for (int i = 0; i < NUM_COIN_TYPES; i++) {
            ch->points.money[i] -= app_fee[i];
        }
    }

    // Check if already a member
    auto me = std::shared_ptr<CharData>(ch, [](CharData *) {});
    if ((*clan)->get_membership(me)) {
        char_printf(ch, "You are already a member of {}.\n", (*clan)->name());
        return;
    }

    // For now, applications are disabled - clan members must manually invite
    char_printf(ch, "Clan applications are currently disabled. Contact a clan member to be invited.\n");
}
REGISTER_FUNCTION(clan_apply, "clan_apply", 0, ClanPermissions::NonClanMember | ClanPermissions::God,
                "apply <clan>                 - Apply to join a clan");


CLANCMD(clan_create) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You don't have permission to create clans.\n");
        return;
    }

    auto abbreviation = argument.shift();
    if (abbreviation.empty()) {
        char_printf(ch, "What is the abbreviation for the new clan?\n");
        return;
    }

    if (ansi_strlen(abbreviation) > Clan::MAX_CLAN_ABBR_LEN) {
        char_printf(ch, "Clan abbreviations can be at most {} visible characters long.\n", Clan::MAX_CLAN_ABBR_LEN);
        return;
    }

    if (clan_repository.find_by_abbreviation(abbreviation)) {
        char_printf(ch, "A clan with a similar abbreviation already exists.\n");
        return;
    }

    // Generate a new ID - this is simplistic, should be improved
    ClanId new_id = clan_repository.count() + 1;

    auto clan = clan_repository.create(new_id, std::string{abbreviation}, std::string{abbreviation});
    if (!clan) {
        char_printf(ch, "Error creating clan.\n");
        return;
    }

    // Add leader rank
    // TODO: Add proper rank initialization

    char_printf(ch, "New clan {} created.\n", abbreviation);
    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} creates new clan: {}", GET_NAME(ch), abbreviation);

    clan_repository.save();
}
REGISTER_FUNCTION(clan_create, "clan_create", 0, ClanPermissions::God,
                "create <abbreviation>        - Create a new clan (gods only)");


CLANCMD(clan_destroy) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto clan_name = membership->get_clan_name();
    auto clan_id_opt = membership->get_clan_id();
    if (!clan_id_opt) {
        char_printf(ch, "Error: Invalid clan ID.\n");
        return;
    }
    auto clan_id = *clan_id_opt;

    // TODO: This should notify all members that the clan is disbanded
    membership->notify_clan("Your clan has been disbanded!");

    char_printf(ch, AFMAG "You have deleted the clan {}&0.\n" ANRM, clan_name);
    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} has destroyed the clan {}.", GET_NAME(ch), clan_name);

    clan_repository.remove(clan_id);
    clan_repository.save();
}
REGISTER_FUNCTION(clan_destroy, "clan_destroy", 0, ClanPermissions::ClanAdmin | ClanPermissions::God,
                "destroy <clan>               - Destroy a clan (gods only)");

CLANCMD(clan_info) {
    auto clan_name = argument.get();
    ClanPtr clan;

    if (clan_name.empty()) {
        auto membership = find_memberships(ch, argument);
        if (!membership) {
            char_printf(ch, "Which clan's info do you want to view?\n");
            return;
        }
        clan = membership->clan();
    } else {
        auto [found_clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);

        if (!found_clan) {
            char_printf(ch, "'{}' does not refer to a valid clan.\n", clan_name);
            show_clan_suggestions(ch, clan_name);
            return;
        }

        if (used_fuzzy) {
            char_printf(ch, "Assuming you meant '{}' ({}).\n",
                       strip_ansi((*found_clan)->name()), strip_ansi((*found_clan)->abbreviation()));
        }

        clan = *found_clan;
    }

    // Show clan information
    if (!clan) {
        char_printf(ch, "Error getting clan information.\n");
        return;
    }

    std::string title = fmt::format("[ Clan {}: {} ]", clan->id(), clan->name());
    paging_printf(ch, "{:-^70}\n", title);

    paging_printf(ch,
                  "Nickname: " AFYEL "{}&0" ANRM
                  "  "
                  "Ranks: " AFYEL "{}&0" ANRM
                  "  "
                  "Members: " AFYEL "{}&0" ANRM
                  "\n"
                  "App Fee: " AFCYN "{}&0" ANRM
                  "  "
                  "App Level: " AFYEL "{}&0" ANRM
                  "  "
                  "Dues: " AFCYN "{}&0" ANRM "\n",
                  clan->abbreviation(), clan->ranks().size(), clan->member_count(), clan->app_fee(),
                  clan->min_application_level(), clan->dues());

    // Show additional information for members
    auto me = std::shared_ptr<CharData>(ch, [](CharData *) {});
    bool show_all =
        GET_LEVEL(ch) >= LVL_IMMORT || (clan->get_membership(me) && clan->has_permission(me, ClanPrivilege::None));

    if (show_all) {
        paging_printf(ch, "Treasure: {}\n", statemoney(clan->treasure()));

        paging_printf(ch, "\nRanks:\n");
        for (size_t i = 0; i < clan->ranks().size(); ++i) {
            paging_printf(ch, "{:3}  {}\n", i + 1, clan->ranks()[i].title());
        }

        paging_printf(ch, "\nPrivileges:\n");
        // Format privilege table
        // TODO: Implement privilege display
    }

    if (!clan->description().empty()) {
        paging_printf(ch, "\nDescription:\n{}", clan->description());
    }

    if (show_all && !clan->motd().empty()) {
        paging_printf(ch, "\nMessage of the Day:\n{}", clan->motd());
    }

    start_paging(ch);
}
REGISTER_FUNCTION(clan_info, "clan_info", 0, ClanPermissions::Everybody,
                "info [clan]                  - Display information about a clan");

// TODO: Implement the rest of the clan commands

ACMD(do_clan) {
    if (IS_NPC(ch) || !ch->desc) {
        char_printf(ch, HUH);
        return;
    }

    Arguments args(argument);
    auto command = args.shift();

    if (command.empty()) {
        // Display help using the function registry
        auto permissions = get_clan_permissions(ch);
        char_printf(ch, "Available clan commands:\n");

        auto clan_help = FunctionRegistry::print_available_with_prefix("clan_", permissions,
            [](std::string_view name) -> std::string {
                return name.starts_with("clan_") ? std::string(name.substr(5)) : std::string(name);
            });
        char_printf(ch, "{}", clan_help);

        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            char_printf(ch, "\nNote: As a god, you must specify the clan for most commands.\n");
        }
        return;
    }

    // Try to call the command using the function registry with permission checking
    std::string full_command = "clan_" + std::string(command);
    auto permissions = get_clan_permissions(ch);

    if (!FunctionRegistry::call_by_abbrev_with_permissions(full_command, ch, args, permissions)) {
        // Check if the function exists but we lack permissions
        if (FunctionRegistry::can_call_function(full_command, ClanPermissions::Everybody)) {
            char_printf(ch, "You don't have permission to use that clan command.\n");
        } else {
            char_printf(ch, "Unknown clan command: '{}'\n", command);
        }
        char_printf(ch, "Type 'clan' with no arguments to see available commands.\n");
    }
}

ACMD(do_ctell) {
    Arguments args(argument);
    if (args.empty()) {
        char_printf(ch, "What do you want to tell your clan?\n");
        return;
    }

    clan_tell(ch, args);
}

CLANCMD(clan_snoop) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "Only gods may snoop clan communications.\n");
        return;
    }

    if (argument.empty()) {
        // Show current snooped clans
        auto snooped_clans = get_snooped_clans(ch);
        if (snooped_clans.empty()) {
            char_printf(ch, "You are not currently snooping any clan communications.\n");
        } else {
            char_printf(ch, "You are currently snooping the following clans:\n");
            for (auto clan_id : snooped_clans) {
                auto clan = clan_repository.find_by_id(clan_id);
                if (clan) {
                    char_printf(ch, "  {} [{}] (ID: {})\n", 
                               clan.value()->name(), 
                               clan.value()->abbreviation(), 
                               clan_id);
                }
            }
        }
        return;
    }

    auto subcommand = argument.shift();
    
    if (subcommand == "start" || subcommand == "on") {
        if (argument.empty()) {
            char_printf(ch, "Start snooping which clan? Usage: clan snoop start <clan>\n");
            return;
        }
        
        auto clan_name = std::string(argument.get());
        
        // Find clan by name or abbreviation
        auto clan = clan_repository.find_by_name(clan_name);
        if (!clan) {
            clan = clan_repository.find_by_abbreviation(clan_name);
        }
        
        if (!clan) {
            char_printf(ch, "No clan found with name or abbreviation '{}'.\n", clan_name);
            return;
        }
        
        auto clan_id = clan.value()->id();
        
        if (is_snooping_clan(ch, clan_id)) {
            char_printf(ch, "You are already snooping {} communications.\n", clan.value()->name());
            return;
        }
        
        add_clan_snoop(ch, clan_id);
        char_printf(ch, "You are now snooping {} [{}] communications.\n", 
                   clan.value()->name(), clan.value()->abbreviation());
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} starts snooping clan {} communications", 
            GET_NAME(ch), clan.value()->name());
            
    } else if (subcommand == "stop" || subcommand == "off") {
        if (argument.empty()) {
            char_printf(ch, "Stop snooping which clan? Usage: clan snoop stop <clan>\n");
            return;
        }
        
        auto clan_name = std::string(argument.get());
        
        // Find clan by name or abbreviation
        auto clan = clan_repository.find_by_name(clan_name);
        if (!clan) {
            clan = clan_repository.find_by_abbreviation(clan_name);
        }
        
        if (!clan) {
            char_printf(ch, "No clan found with name or abbreviation '{}'.\n", clan_name);
            return;
        }
        
        auto clan_id = clan.value()->id();
        
        if (!is_snooping_clan(ch, clan_id)) {
            char_printf(ch, "You are not currently snooping {} communications.\n", clan.value()->name());
            return;
        }
        
        remove_clan_snoop(ch, clan_id);
        char_printf(ch, "You stop snooping {} [{}] communications.\n", 
                   clan.value()->name(), clan.value()->abbreviation());
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} stops snooping clan {} communications", 
            GET_NAME(ch), clan.value()->name());
            
    } else if (subcommand == "clear" || subcommand == "all") {
        auto snooped_clans = get_snooped_clans(ch);
        if (snooped_clans.empty()) {
            char_printf(ch, "You are not currently snooping any clan communications.\n");
            return;
        }
        
        remove_all_clan_snoops(ch);
        char_printf(ch, "You stop snooping all clan communications.\n");
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} stops snooping all clan communications", GET_NAME(ch));
        
    } else {
        char_printf(ch, "Usage:\n");
        char_printf(ch, "  clan snoop                  - Show current snooped clans\n");
        char_printf(ch, "  clan snoop start <clan>     - Start snooping clan communications\n");
        char_printf(ch, "  clan snoop stop <clan>      - Stop snooping clan communications\n");
        char_printf(ch, "  clan snoop clear            - Stop snooping all clans\n");
    }
}

REGISTER_FUNCTION(clan_snoop, "clan_snoop", 0, ClanPermissions::God,
                "snoop <start|stop|clear> [clan] - Monitor clan communications (gods only)");

CLANCMD(clan_enroll) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You don't have permission to enroll clan members.\n");
        return;
    }

    if (argument.empty()) {
        char_printf(ch, "Usage: clan enroll <clan> <player> [rank]\n");
        return;
    }

    auto clan_name = argument.shift();
    auto player_name = argument.shift();
    auto rank_arg = argument.shift();

    if (clan_name.empty() || player_name.empty()) {
        char_printf(ch, "Usage: clan enroll <clan> <player> [rank]\n");
        return;
    }

    // Find the target player (must be online)
    auto target = find_char_in_world(find_by_name(const_cast<char*>(std::string(player_name).c_str())));
    if (!target || !target->desc || !IS_PLAYING(target->desc)) {
        char_printf(ch, "Player '{}' is not currently online.\n", player_name);
        return;
    }

    // Get the correct case of the player's name
    std::string correct_player_name = GET_NAME(target);

    // Find the clan
    auto [clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);
    if (!clan) {
        char_printf(ch, "No such clan exists.\n");
        show_clan_suggestions(ch, clan_name);
        return;
    }

    if (used_fuzzy) {
        char_printf(ch, "Assuming you meant '{}' ({}).\n",
                   strip_ansi((*clan)->name()), strip_ansi((*clan)->abbreviation()));
    }

    // Default to rank 0 (lowest rank) if not specified
    int rank_index = 0;
    if (!rank_arg.empty()) {
        // Create a temporary Arguments object to use try_shift_number
        Arguments rank_args(rank_arg);
        auto rank_opt = rank_args.try_shift_number();
        if (!rank_opt || *rank_opt < 1) {
            char_printf(ch, "Invalid rank number. Use a positive integer.\n");
            return;
        }
        rank_index = *rank_opt - 1; // Convert to 0-based index
    }

    // Validate rank index
    if (rank_index < 0 || rank_index >= static_cast<int>((*clan)->ranks().size())) {
        char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", (*clan)->ranks().size());
        return;
    }

    auto rank_title = (*clan)->ranks()[rank_index].title();
    
    // Check if player is already a member and update their rank
    if ((*clan)->get_member_by_name(correct_player_name).has_value()) {
        if ((*clan)->update_member_rank(correct_player_name, rank_index)) {
            char_printf(ch, "{} is already a member of {}. Updated their rank to '{}'.\n", 
                       correct_player_name, (*clan)->name(), rank_title);
            log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} updates {}'s rank in {} to {}", 
                GET_NAME(ch), correct_player_name, (*clan)->name(), rank_title);
            
            // Save clan ID to player file for efficient loading
            target->player_specials->clan_id = (*clan)->id();
            
            // Load runtime membership for the online player using efficient method
            auto target_shared = std::make_shared<CharData>(*target);
            target->player_specials->clan_membership = clan_repository.load_clan_membership_by_id(target_shared);
            
            char_printf(target, AFGRN "Your rank in {}&0 has been updated to '{}&0'!\n" ANRM, 
                       (*clan)->name(), rank_title);
            
            // Save changes
            clan_repository.save();
        } else {
            char_printf(ch, "Failed to update {}'s rank in {}.\n", correct_player_name, (*clan)->name());
        }
        return;
    }

    // Add the member to persistent storage
    if ((*clan)->add_member_by_name(correct_player_name, rank_index)) {
        char_printf(ch, "{} has been enrolled in {} with rank '{}'.\n", 
                   correct_player_name, (*clan)->name(), rank_title);
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} enrolls {} in {} as {}", 
            GET_NAME(ch), correct_player_name, (*clan)->name(), rank_title);
        
        // Save clan ID to player file for efficient loading
        target->player_specials->clan_id = (*clan)->id();
        
        // Load runtime membership for the online player using efficient method
        auto target_shared = std::make_shared<CharData>(*target);
        target->player_specials->clan_membership = clan_repository.load_clan_membership_by_id(target_shared);
        
        char_printf(target, AFGRN "You have been enrolled in {}&0 with rank '{}&0'!\n" ANRM, 
                   (*clan)->name(), rank_title);
        
        // Save changes
        clan_repository.save();
    } else {
        char_printf(ch, "Failed to enroll {} in {}.\n", correct_player_name, (*clan)->name());
    }
}
REGISTER_FUNCTION(clan_enroll, "clan_enroll", 0, ClanPermissions::God,
                "enroll <clan> <player> [rank] - Enroll a player in a clan (gods only)");

CLANCMD(clan_expel) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "You don't have permission to expel clan members.\n");
        return;
    }

    if (argument.empty()) {
        char_printf(ch, "Usage: clan expel <clan> <player>\n");
        return;
    }

    auto clan_name = argument.shift();
    auto player_name = argument.shift();

    if (clan_name.empty() || player_name.empty()) {
        char_printf(ch, "Usage: clan expel <clan> <player>\n");
        return;
    }

    // Try to find the target player if they're online
    auto target = find_char_in_world(find_by_name(const_cast<char*>(std::string(player_name).c_str())));
    std::string correct_player_name;
    
    if (target && target->desc && IS_PLAYING(target->desc)) {
        // Player is online, get their exact name
        correct_player_name = GET_NAME(target);
    } else {
        // Player is offline, use the provided name (case-sensitive)
        correct_player_name = std::string(player_name);
        target = nullptr;  // Clear target since they're offline
        char_printf(ch, "Note: Player '{}' is offline. Using exact name as provided.\n", correct_player_name);
    }

    // Find the clan
    auto [clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);
    if (!clan) {
        char_printf(ch, "No such clan exists.\n");
        show_clan_suggestions(ch, clan_name);
        return;
    }

    if (used_fuzzy) {
        char_printf(ch, "Assuming you meant '{}' ({}).\n",
                   strip_ansi((*clan)->name()), strip_ansi((*clan)->abbreviation()));
    }

    // Check if player is a member
    if (!(*clan)->get_member_by_name(correct_player_name).has_value()) {
        char_printf(ch, "{} is not a member of {}.\n", correct_player_name, (*clan)->name());
        return;
    }

    // Remove the member
    if ((*clan)->remove_member_by_name(correct_player_name)) {
        char_printf(ch, "{} has been expelled from {}.\n", correct_player_name, (*clan)->name());
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} expels {} from {}", 
            GET_NAME(ch), correct_player_name, (*clan)->name());
        
        // If player is online, update their runtime data
        if (target) {
            // Clear clan ID from player file
            target->player_specials->clan_id = CLAN_ID_NONE;
            
            // Update the player's runtime membership
            target->player_specials->clan_membership.reset();
            char_printf(target, AFYEL "You have been expelled from {}&0!\n" ANRM, (*clan)->name());
        }
        
        // Save changes
        clan_repository.save();
    } else {
        char_printf(ch, "Failed to expel {} from {}.\n", correct_player_name, (*clan)->name());
    }
}
REGISTER_FUNCTION(clan_expel, "clan_expel", 0, ClanPermissions::God,
                "expel <clan> <player>        - Expel a player from a clan (online or offline, gods only)");

CLANCMD(clan_members) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto clan = membership->get_clan();
    if (!clan) {
        char_printf(ch, "Error getting clan information.\n");
        return;
    }

    const auto& members = clan->members();
    const auto& ranks = clan->ranks();

    if (members.empty()) {
        char_printf(ch, "{} has no members.\n", clan->name());
        return;
    }

    std::string title = fmt::format("[ Members of {} ({}) ]", clan->name(), clan->abbreviation());
    paging_printf(ch, "{:-^70}\n", title);
    
    paging_printf(ch, "{:<20} {:<15} {:<20} {:<10}\n", "Name", "Rank", "Title", "Joined");
    paging_printf(ch, "{:-^70}\n", "");

    // Sort members by rank (highest rank first)
    auto sorted_members = members;
    std::sort(sorted_members.begin(), sorted_members.end(), 
              [](const auto& a, const auto& b) {
                  return a.rank_index > b.rank_index; // Higher rank index = higher rank
              });

    for (const auto& member : sorted_members) {
        std::string rank_title = "Unknown";
        if (member.rank_index >= 0 && member.rank_index < static_cast<int>(ranks.size())) {
            rank_title = std::string(ranks[member.rank_index].title());
        }

        // Format join time
        char join_date[32];
        struct tm* time_info = localtime(&member.join_time);
        strftime(join_date, sizeof(join_date), "%m/%d/%Y", time_info);

        paging_printf(ch, "{:<20} {:<15} {:<20} {:<10}\n", 
                     member.name, 
                     member.rank_index + 1, // Display as 1-based
                     rank_title,
                     join_date);
    }

    paging_printf(ch, "{:-^70}\n", "");
    paging_printf(ch, "Total members: {}\n", members.size());
}
REGISTER_FUNCTION(clan_members, "clan_members", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "members [clan]               - Show clan member list");

CLANCMD(clan_chest) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    // Check if clan chest is accessible from this room (gods can access from anywhere)
    auto clan = membership->get_clan();
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (clan && clan->chest_room() != NOWHERE && ch->in_room != clan->chest_room()) {
            char_printf(ch, "You can only access the clan chest from the designated chest room.\n");
            return;
        }

        // If chest_room is NOWHERE, chest access is disabled
        if (clan && clan->chest_room() == NOWHERE) {
            char_printf(ch, "Clan chest access is currently disabled.\n");
            return;
        }
    }

    if (!clan) {
        char_printf(ch, "Error getting clan information.\n");
        return;
    }

    const auto& storage = clan->storage();

    if (storage.empty()) {
        char_printf(ch, "The {} clan chest is empty.\n", clan->name());
        return;
    }

    std::string title = fmt::format("[ {} ({}) Clan Chest ]", clan->name(), clan->abbreviation());
    paging_printf(ch, "{:-^70}\n", title);
    
    bool show_vnums = GET_LEVEL(ch) >= LVL_IMMORT;
    
    if (show_vnums) {
        paging_printf(ch, "{:<6} {:<8} {:<45} {:<8}\n", "VNUM", "Qty", "Item", "Type");
    } else {
        paging_printf(ch, "{:<8} {:<53} {:<8}\n", "Qty", "Item", "Type");
    }
    paging_printf(ch, "{:-^70}\n", "");

    int total_items = 0;
    
    // Sort storage by vnum for consistent display
    std::vector<std::pair<ObjectId, int>> sorted_storage(storage.begin(), storage.end());
    std::sort(sorted_storage.begin(), sorted_storage.end());

    for (const auto& [vnum, quantity] : sorted_storage) {
        // Try to load the object to get its information
        auto obj = read_object(vnum, VIRTUAL);
        if (obj) {
            std::string item_name = obj->short_description ? obj->short_description : "Unknown Item";
            std::string item_type = "Unknown";
            
            // Get item type name
            if (GET_OBJ_TYPE(obj) >= 0 && GET_OBJ_TYPE(obj) < NUM_ITEM_TYPES) {
                item_type = item_types[GET_OBJ_TYPE(obj)].name;
            }
            
            if (show_vnums) {
                paging_printf(ch, "{:<6} {:<8} {:<45} {:<8}\n", 
                             vnum, quantity, item_name, item_type);
            } else {
                paging_printf(ch, "{:<8} {:<53} {:<8}\n", 
                             quantity, item_name, item_type);
            }
            
            free_obj(obj);
        } else {
            // Object couldn't be loaded (might be deleted from game)
            if (show_vnums) {
                paging_printf(ch, "{:<6} {:<8} {:<45} {:<8}\n", 
                             vnum, quantity, "[MISSING OBJECT]", "N/A");
            } else {
                paging_printf(ch, "{:<8} {:<53} {:<8}\n", 
                             quantity, "[MISSING OBJECT]", "N/A");
            }
        }
        
        total_items += quantity;
    }

    paging_printf(ch, "{:-^70}\n", "");
    paging_printf(ch, "Total unique items: {}  Total item count: {}\n", 
                 storage.size(), total_items);
}
REGISTER_FUNCTION(clan_chest, "clan_chest", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "chest [clan]                 - Show clan chest contents");

CLANCMD(clan_ranks) {
    auto membership = find_memberships(ch, argument);
    if (!membership) {
        return;
    }

    auto clan = membership->get_clan();
    if (!clan) {
        char_printf(ch, "Error getting clan information.\n");
        return;
    }

    const auto& ranks = clan->ranks();
    
    if (ranks.empty()) {
        char_printf(ch, "{} has no ranks defined.\n", clan->name());
        return;
    }

    char_printf(ch, "Ranks for {}:\n", clan->name());
    char_printf(ch, "================================================================================\n");
    
    for (size_t i = 0; i < ranks.size(); ++i) {
        const auto& rank = ranks[i];
        char_printf(ch, "Rank {}: {}\n", i + 1, rank.title());
        
        // Show permissions
        char_printf(ch, "  Permissions: ");
        std::vector<std::string> perms;
        
        // Check each permission
        if (rank.has_permission(ClanPrivilege::Description)) perms.push_back("Description");
        if (rank.has_permission(ClanPrivilege::Motd)) perms.push_back("MOTD");
        if (rank.has_permission(ClanPrivilege::Grant)) perms.push_back("Grant");
        if (rank.has_permission(ClanPrivilege::Ranks)) perms.push_back("Ranks");
        if (rank.has_permission(ClanPrivilege::Title)) perms.push_back("Title");
        if (rank.has_permission(ClanPrivilege::Enroll)) perms.push_back("Enroll");
        if (rank.has_permission(ClanPrivilege::Expel)) perms.push_back("Expel");
        if (rank.has_permission(ClanPrivilege::Promote)) perms.push_back("Promote");
        if (rank.has_permission(ClanPrivilege::Demote)) perms.push_back("Demote");
        if (rank.has_permission(ClanPrivilege::App_Fees)) perms.push_back("App_Fees");
        if (rank.has_permission(ClanPrivilege::App_Level)) perms.push_back("App_Level");
        if (rank.has_permission(ClanPrivilege::Dues)) perms.push_back("Dues");
        if (rank.has_permission(ClanPrivilege::Deposit)) perms.push_back("Deposit");
        if (rank.has_permission(ClanPrivilege::Withdraw)) perms.push_back("Withdraw");
        if (rank.has_permission(ClanPrivilege::Store)) perms.push_back("Store");
        if (rank.has_permission(ClanPrivilege::Retrieve)) perms.push_back("Retrieve");
        if (rank.has_permission(ClanPrivilege::Alts)) perms.push_back("Alts");
        if (rank.has_permission(ClanPrivilege::Chat)) perms.push_back("Chat");
        
        if (perms.empty()) {
            char_printf(ch, "None\n");
        } else {
            // Print permissions in rows of 4
            for (size_t j = 0; j < perms.size(); ++j) {
                if (j > 0 && j % 4 == 0) {
                    char_printf(ch, "\n                ");
                }
                char_printf(ch, "{:12} ", perms[j]);
            }
            char_printf(ch, "\n");
        }
        char_printf(ch, "\n");
    }
}
REGISTER_FUNCTION(clan_ranks, "clan_ranks", 0, ClanPermissions::ClanMember | ClanPermissions::God,
                "ranks [clan]                 - Show clan ranks and their permissions");

ACMD(do_csnoop) {
    Arguments args(argument);
    clan_snoop(ch, args);
}

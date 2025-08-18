#include "act.hpp"

#include "arguments.hpp"
#include "clan.hpp"
#include "comm.hpp"
#include "db.hpp"
#include "dg_scripts.hpp"
#include "find.hpp"
#include "function_registration.hpp"
#include "handler.hpp"
#include "logging.hpp"
#include "messages.hpp"
#include "modify.hpp"
#include "money.hpp"
#include "objects.hpp"
#include "pfiles.hpp"
#include "players.hpp"
#include "screen.hpp"
#include "utils.hpp"

#include <algorithm>
#include <expected>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Template implementations will be defined later

// Legacy permission checking - updated for new enum
// Legacy function replaced by decorator system
bool can_use_clan_command(const CharData *ch, ClanPermission required_permission) {
    return has_clan_permission_or_god(ch, required_permission);
}

// Forward declaration - defined after permissions namespace
uint32_t get_clan_permissions(const CharData *ch);

// =============================================================================
// MODERN C++23 HELPER FUNCTIONS - ELIMINATE STATIC CASTS
// =============================================================================

// Type-safe enum to underlying type conversion using magic_enum
template <typename Enum> constexpr auto to_underlying(Enum e) noexcept { return magic_enum::enum_integer(e); }

// =============================================================================
// PERMISSION SYSTEM ALIASES - 3-STAGE DESIGN
// =============================================================================

// Readable aliases for the 3-stage permission system
namespace permissions {
// Define distinct permission flags for the help system
constexpr uint32_t PUBLIC_FLAG = 1;    // Everyone can run - flag value 1
constexpr uint32_t GOD_FLAG = 2;       // Only gods can run - flag value 2
constexpr uint32_t CLAN_BASE_FLAG = 4; // Base flag for clan permissions - starts at 4

constexpr auto PUBLIC = std::optional<uint32_t>(PUBLIC_FLAG); // Everyone can run
constexpr auto GOD_ONLY = std::optional<uint32_t>(GOD_FLAG);  // Only gods can run

// Helper function for specific clan permissions
constexpr std::optional<uint32_t> clan_permission(ClanPermission perm) {
    // Shift clan permissions to bits 8+ to avoid conflict with system flags (bits 0-7)
    constexpr uint32_t CLAN_PERM_SHIFT = 8;
    return std::optional<uint32_t>(CLAN_BASE_FLAG | (to_underlying(perm) << CLAN_PERM_SHIFT));
}

// Basic clan membership (any clan member can access)
constexpr auto CLAN_MEMBER = std::optional<uint32_t>(CLAN_BASE_FLAG);
} // namespace permissions

// For function registry compatibility - provide appropriate permission flags for help display
uint32_t get_clan_permissions(const CharData *ch) {
    uint32_t user_permissions = 0;

    // Everyone gets public permissions
    user_permissions |= permissions::PUBLIC_FLAG;

    // Gods get god permissions (which includes public) and all clan permissions
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        user_permissions |= permissions::GOD_FLAG;
        user_permissions |= permissions::CLAN_BASE_FLAG;

        // Give gods all possible clan permissions
        constexpr uint32_t CLAN_PERM_SHIFT = 8;
        for (int perm = 0; perm < static_cast<int>(ClanPermission::MAX_PERMISSIONS); ++perm) {
            user_permissions |= (1u << (CLAN_PERM_SHIFT + perm));
        }
    }

    // Clan members get clan-specific permissions based on their rank
    auto clan = get_clan(ch);
    if (clan) {
        auto member = get_clan_member(ch);
        if (member) {
            const auto &ranks = clan.value()->ranks();
            if (member->rank_index >= 0 && member->rank_index < static_cast<int>(ranks.size())) {
                const auto &rank = ranks[member->rank_index];

                // Add clan base flag for all clan members
                user_permissions |= permissions::CLAN_BASE_FLAG;

                // Add specific clan permissions based on rank
                // Shift clan permissions to bits 8+ to avoid conflict with system flags (bits 0-7)
                constexpr uint32_t CLAN_PERM_SHIFT = 8;
                for (int i = 0; i < static_cast<int>(ClanPermission::MAX_PERMISSIONS); ++i) {
                    ClanPermission perm = static_cast<ClanPermission>(i);
                    if (rank.has_permission(perm)) {
                        uint32_t perm_flag = static_cast<uint32_t>(perm) << CLAN_PERM_SHIFT;
                        user_permissions |= perm_flag;
                    }
                }
            }
        }
    }

    return user_permissions;
}

// Type-safe enum to size_t conversion for bitset operations
template <typename Enum> constexpr size_t to_bitset_index(Enum e) noexcept {
    return static_cast<size_t>(to_underlying(e));
}

// Set permission in bitset using type-safe conversion
inline void set_permission(PermissionSet &permissions, ClanPermission perm) { permissions.set(to_bitset_index(perm)); }

// Legacy macro - now replaced by CLAN_COMMAND decorator system
// #define CLANCMD(name) static void(name)(CharData * ch, Arguments argument)

// =============================================================================
// CLAN SYSTEM IMPLEMENTATION
// =============================================================================
//
// This file implements the clan system using a modern C++ decorator pattern
// for permission checking. All commands use the CLAN_COMMAND macro with
// appropriate permission decorators defined in clan.hpp.
//
// Key improvements:
// - Template-based permission decorators eliminate boilerplate
// - Centralized permission logic in clan.hpp
// - Clean separation between public and member commands
// - Fuzzy search for clan names/abbreviations
// - Consistent error handling and messaging
//
// =============================================================================

// SECTION: Permission and validation helpers

// Fuzzy string matching utilities
namespace {
// Calculate Levenshtein distance between two strings
size_t levenshtein_distance(std::string_view s1, std::string_view s2) {
    const size_t len1 = s1.length();
    const size_t len2 = s2.length();

    // Create a matrix to store distances
    std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));

    // Initialize first row and column
    for (size_t i = 0; i <= len1; ++i)
        dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j)
        dp[0][j] = j;

    // Fill the matrix
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            if (std::tolower(s1[i - 1]) == std::tolower(s2[j - 1])) {
                dp[i][j] = dp[i - 1][j - 1]; // No operation needed
            } else {
                dp[i][j] = 1 + std::min({
                                   dp[i - 1][j],    // Deletion
                                   dp[i][j - 1],    // Insertion
                                   dp[i - 1][j - 1] // Substitution
                               });
            }
        }
    }

    return dp[len1][len2];
}

// Calculate fuzzy match score (0.0 = no match, 1.0 = perfect match)
double fuzzy_match_score(std::string_view target, std::string_view query) {
    if (query.empty())
        return 0.0;
    if (target.empty())
        return 0.0;

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
    std::ranges::transform(target_lower, target_lower.begin(), ::tolower);
    std::ranges::transform(query_lower, query_lower.begin(), ::tolower);

    if (target_lower.find(query_lower) != std::string::npos) {
        return 0.8;
    }

    // Use Levenshtein distance for fuzzy matching
    size_t distance = levenshtein_distance(target, query);
    size_t max_len = std::max(target.length(), query.length());

    if (distance > max_len)
        return 0.0;

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
        double name_score =
            std::max(fuzzy_match_score(clan->name(), query), fuzzy_match_score(strip_ansi(clan->name()), query));

        double abbr_score = std::max(fuzzy_match_score(clan->abbreviation(), query),
                                     fuzzy_match_score(strip_ansi(clan->abbreviation()), query));

        double best_score = std::max(name_score, abbr_score);

        if (best_score > 0.0) {
            matches.emplace_back(clan, best_score);
        }
    }

    // Sort by score (highest first)
    std::ranges::sort(matches, [](const auto &a, const auto &b) { return a.second > b.second; });

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
        for (const auto &[suggested_clan, score] : fuzzy_matches) {
            char_printf(ch, "  {} ({})\n", strip_ansi(suggested_clan->name()),
                        strip_ansi(suggested_clan->abbreviation()));
        }
    }
}
} // namespace

// SECTION: Clan lookup and command helpers
// Helper function to display clan command help
static void display_clan_help(CharData *ch) {
    char_printf(ch, "Available clan commands:\n");

    // Get user's current permission flags
    auto permissions = get_clan_permissions(ch);

    // Check if user is already in a clan
    bool is_clan_member = get_clan(ch).has_value();

    // Use function registry to print available commands with "clan_" prefix
    auto help_text =
        FunctionRegistry::print_available_with_prefix("clan_", permissions, [](std::string_view name) -> std::string {
            // Strip "clan_" prefix for cleaner display
            if (name.starts_with("clan_")) {
                return std::string(name.substr(5));
            }
            return std::string(name);
        });

    // Apply context-sensitive filtering
    std::string filtered_help;
    std::istringstream help_stream(help_text);
    std::string line;

    while (std::getline(help_stream, line)) {
        // Hide 'apply' command from clan members
        if (is_clan_member && line.find("apply") != std::string::npos &&
            line.find("Apply to join a clan") != std::string::npos) {
            continue; // Skip this line
        }

        filtered_help += line + "\n";
    }

    if (filtered_help.empty()) {
        char_printf(ch, "   No clan commands are available to you.\n");
    } else {
        char_printf(ch, "{}", filtered_help);
    }

    // Add god-specific note if applicable
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        char_printf(ch, "\nNote: As a god, you must specify the clan for most commands.\n");
    }
}

// Helper function to validate room access for clan operations
static bool validate_clan_room_access(CharData *ch, room_num clan_room, const std::string &room_type) {
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        return true; // Gods can access from anywhere
    }

    if (clan_room == NOWHERE) {
        char_printf(ch, "Clan {} access is currently disabled.\n", room_type);
        return false;
    }

    // Convert clan_room (VNUM) to RNUM for comparison with ch->in_room (RNUM)
    room_num clan_room_rnum = real_room(clan_room);
    if (clan_room_rnum == NOWHERE) {
        char_printf(ch, "Clan {} room does not exist.\n", room_type);
        return false;
    }

    if (ch->in_room != clan_room_rnum) {
        char_printf(ch, "You must be in your clan's {} room to perform this action.\n", room_type);
        return false;
    }

    return true;
}

// Helper function to handle god permission checks with consistent messaging
static bool check_god_only_operation(CharData *ch, const std::string &operation) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        char_printf(ch, "Only gods may {}.\n", operation);
        return false;
    }
    return true;
}

static std::optional<ClanPtr> find_clan_for_command(CharData *ch, Arguments &argument) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
        // For non-gods, always use their own clan (ignore any arguments)
        auto clan = get_clan(ch);
        if (!clan) {
            char_printf(ch, "You are not a member of a clan.\n");
            return std::nullopt;
        }
        return clan;
    }

    if (argument.empty()) {
        auto clan = get_clan(ch);
        if (clan) {
            return clan;
        }
        char_printf(ch, "Please specify a clan.\n");
        return std::nullopt;
    }

    // Gods can specify the clan.
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
                char_printf(ch, "  ID: {} Name: \"{}\" (stripped: \"{}\") Abbr: \"{}\" (stripped: \"{}\")\n", c->id(),
                            c->name(), strip_ansi(c->name()), c->abbreviation(), strip_ansi(c->abbreviation()));
            }
        }
        return std::nullopt;
    }

    // Notify user if we used fuzzy matching to find the clan
    if (used_fuzzy_match) {
        char_printf(ch, "Assuming you meant '{}' ({}).\n", strip_ansi((*clan)->name()),
                    strip_ansi((*clan)->abbreviation()));
    }

    // For gods, allow access to any clan even if they're not a member
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        return *clan;
    }

    // Check if the character is actually a member of this clan
    if (get_clan_id(ch) == (*clan)->id()) {
        return *clan;
    }

    char_printf(ch, "You are not a member of that clan.\n");
    return std::nullopt;
}

// SECTION: Public clan commands (no permissions required)

CLAN_COMMAND(clan_list, clan_permissions::RequiresNoPermissions) {
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
REGISTER_FUNCTION_WITH_CATEGORY(clan_list, "clan_list", CommandCategory::CLAN, permissions::PUBLIC,
                                "list                         - List all clans and their application fees");

// SECTION: Financial clan commands (require specific permissions)

CLAN_COMMAND(clan_deposit, clan_permissions::RequiresDepositFunds) {
    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    // Check bank room access
    if (!validate_clan_room_access(ch, clan.value()->bank_room(), "bank")) {
        return;
    }

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

    // Add money to clan treasury
    if (auto result = clan.value()->admin_add_treasure(coins); !result) {
        char_printf(ch, "Failed to add money to clan treasury: {}\n", result.error());
        return;
    }

    save_player_char(ch);

    char_printf(ch, "You deposit {} into {}'s account.\n", statemoney(coins), clan.value()->abbreviation());
    clan_repository.save();
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_deposit, "clan_deposit", CommandCategory::CLAN,
                                permissions::clan_permission(ClanPermission::DEPOSIT_FUNDS),
                                "deposit <clan> <amount>      - Deposit money into the clan treasury");

CLAN_COMMAND(clan_withdraw, clan_permissions::RequiresWithdrawFunds) {

    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    // Check bank room access
    if (!validate_clan_room_access(ch, clan.value()->bank_room(), "bank")) {
        return;
    }

    auto money_opt = parse_money(argument.get());
    if (!money_opt || money_opt->value() <= 0) {
        char_printf(ch, "How much do you want to withdraw?\n");
        return;
    }

    Money coins = *money_opt;

    auto treasure = clan.value()->treasure();
    if (treasure.value() < coins.value()) {
        char_printf(ch, "The clan doesn't have that much money.\n");
        return;
    }

    // Subtract money from clan treasury
    if (auto result = clan.value()->admin_subtract_treasure(coins); !result) {
        char_printf(ch, "Failed to withdraw money from clan treasury: {}\n", result.error());
        return;
    }

    for (int i = 0; i < NUM_COIN_TYPES; i++) {
        ch->points.money[i] += coins[i];
    }
    save_player_char(ch);

    char_printf(ch, "You withdraw from {}'s account: {}\n", clan.value()->abbreviation(), statemoney(coins));
    clan_repository.save();
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_withdraw, "clan_withdraw", CommandCategory::CLAN,
                                permissions::clan_permission(ClanPermission::WITHDRAW_FUNDS),
                                "withdraw <clan> <amount>     - Withdraw money from the clan treasury");

// SECTION: Storage clan commands (require storage permissions)

CLAN_COMMAND(clan_store, clan_permissions::RequiresStoreItems) {
    ObjData *obj;

    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    // Check chest room access
    if (!validate_clan_room_access(ch, clan.value()->chest_room(), "chest")) {
        return;
    }

    // Try to parse an optional number followed by object name
    auto quantity_opt = argument.try_shift_number();
    if (quantity_opt && *quantity_opt != 1) {
        char_printf(ch, "You can only store one item at a time.\n");
        return;
    }

    auto obj_name = argument.shift();
    if (obj_name.empty()) {
        char_printf(ch, "Store what in the clan vault?\n");
        return;
    }
    if (!(obj = find_obj_in_list(ch->carrying, find_vis_by_name(ch, std::string(obj_name).data())))) {
        char_printf(ch, "You aren't carrying {} {}.\n", an(obj_name), obj_name);
    } else if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        act("You can't store $p in $P because it's CURSED!", false, ch, obj, nullptr, TO_CHAR);
    } else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {
        char_printf(ch, "You must empty the container before storing it.\n");
    } else if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
        char_printf(ch, "You cannot store money items in the clan vault.\n");
    } else if (GET_OBJ_VNUM(obj) == -1) {
        char_printf(ch, "You cannot store objects without a valid item number.\n");
    } else {
        obj_from_char(obj);
        auto vnum = GET_OBJ_VNUM(obj);
        std::string name = obj->short_description ? obj->short_description : "unknown item";
        extract_obj(obj);

        if (auto result = clan.value()->admin_add_storage_item(vnum, 1); !result) {
            char_printf(ch, "Failed to store item in clan vault: {}\n", result.error());
            return;
        }

        save_player_char(ch);
        char_printf(ch, "You store {} in the clan vault.\n", name);
        clan.value()->notify(ch, fmt::format("{} stores {} in the clan vault.", GET_NAME(ch), name));
        clan_repository.save();
    }
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_store, "clan_store", CommandCategory::CLAN,
                                permissions::clan_permission(ClanPermission::STORE_ITEMS),
                                "store <clan> <item>          - Store an item in the clan vault");

CLAN_COMMAND(clan_retrieve, clan_permissions::RequiresRetrieveItems) {
    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    // Check chest room access
    if (!validate_clan_room_access(ch, clan.value()->chest_room(), "chest")) {
        return;
    }

    // Try to parse an optional number followed by object name
    auto quantity_opt = argument.try_shift_number();
    if (quantity_opt && *quantity_opt != 1) {
        char_printf(ch, "You can only retrieve one item at a time.\n");
        return;
    }

    auto obj_name = argument.shift();
    if (obj_name.empty()) {
        char_printf(ch, "Retrieve what from the clan vault?\n");
        return;
    }
    // We need to find the vnum in the storage that matches this name
    ObjectId obj_vnum = 0;
    bool found = false;

    // Find the object in storage matching the name
    // Build a temporary cache for faster lookup
    std::unordered_map<std::string, ObjectId> name_to_vnum_cache;
    for (const auto &[vnum, count] : clan.value()->storage()) {
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
        extract_obj(obj);
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
        char_printf(ch, "Unable to read object from vault.\n");
        return;
    }

    obj_to_char(obj, ch);

    save_player_char(ch);
    char_printf(ch, "You retrieve {} from the clan vault.\n", obj->short_description);
    clan.value()->admin_remove_storage_item(obj_vnum, 1);

    clan.value()->notify(ch, fmt::format("{} retrieves {} from the clan vault.", GET_NAME(ch), obj->short_description));
    clan_repository.save();
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_retrieve, "clan_retrieve", CommandCategory::CLAN,
                                permissions::clan_permission(ClanPermission::RETRIEVE_ITEMS),
                                "retrieve <clan> <item>       - Retrieve an item from the clan vault");

// SECTION: Communication commands (require chat permissions)

CLAN_COMMAND(clan_tell, clan_permissions::RequiresClanMembership) {
    CharData *me = REAL_CHAR(ch);

    if (EFF_FLAGGED(ch, EFF_SILENCE)) {
        char_printf(ch, "You cannot speak while silenced.\n");
        return;
    }

    if (!speech_ok(ch, 0))
        return;

    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    if (argument.empty()) {
        char_printf(ch, "Tell your clan what?\n");
        return;
    }

    auto speech = drunken_speech(std::string(argument.get()), GET_COND(ch, DRUNK));

    char_printf(ch, AFMAG "You tell {}" AFMAG ", '" AHMAG "{}" AFMAG "'\n" ANRM,
                ch->player.level < LVL_IMMORT ? "your clan" : clan.value()->abbreviation(), speech);

    for (const auto &member : clan.value()->members()) {
        // Find the online character by name
        auto target = clan_security::find_clan_member_safe(member.name);
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
                    tch->player.level < LVL_IMMORT ? "your clan" : clan.value()->abbreviation(), speech);
    }

    // Send to snooping gods
    auto clan_id = clan.value()->id();
    auto it = clan_snoop_table.find(clan_id);
    if (it != clan_snoop_table.end()) {
        for (auto snoop_ch : it->second) {
            if (!snoop_ch || !snoop_ch->desc || snoop_ch == me)
                continue;

            if (STATE(snoop_ch->desc) != CON_PLAYING)
                continue;

            // Send snoop message with special formatting
            char_printf(snoop_ch, AFYEL "[SNOOP {}] {} tells clan, '" AHMAG "{}" AFYEL "'\n" ANRM,
                        clan.value()->abbreviation(),
                        GET_INVIS_LEV(me) > GET_LEVEL(snoop_ch) ? "Someone" : GET_NAME(me), speech);
        }
    }
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_tell, "clan_tell", CommandCategory::CLAN,
                                permissions::clan_permission(ClanPermission::CLAN_CHAT),
                                "tell <clan> <message>        - Send a message to all clan members");

// SECTION: Administrative commands (require management permissions)

CLAN_COMMAND(clan_set, clan_permissions::RequiresRankManagement) {
    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
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

        // Gods have direct admin access
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            clan.value()->admin_set_abbreviation(std::string(new_abbreviation));
            char_printf(ch, "Clan abbreviation updated to {}.\n", new_abbreviation);
        } else {
            if (!has_clan_permission(ch, ClanPermission::LEADER_OVERRIDE)) {
                char_printf(ch, "You don't have permission to change the clan abbreviation.\n");
                return;
            }
            clan.value()->admin_set_abbreviation(std::string(new_abbreviation));
            char_printf(ch, "Clan abbreviation updated to {}.\n", new_abbreviation);
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

        // Gods have direct admin access
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            if (auto result = clan.value()->admin_add_rank(rank); !result) {
                char_printf(ch, "Failed to add rank: {}\n", result.error());
                return;
            }
            char_printf(ch, "New rank '{}' added.\n", title);
        } else {
            if (!has_clan_permission(ch, ClanPermission::MANAGE_RANKS)) {
                char_printf(ch, "You don't have permission to add ranks.\n");
                return;
            }
            if (auto result = clan.value()->admin_add_rank(rank); !result) {
                char_printf(ch, "Failed to add rank: {}\n", result.error());
                return;
            }
            char_printf(ch, "New rank '{}' added.\n", title);
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

        // Gods have direct admin access
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            clan.value()->admin_set_app_fee(*fee_opt);
            char_printf(ch, "{}'s application fee is now {} platinum.\n", clan.value()->name(), *fee_opt);
        } else {
            if (!has_clan_permission(ch, ClanPermission::SET_APP_FEES)) {
                char_printf(ch, "You don't have permission to change application fees.\n");
                return;
            }
            clan.value()->admin_set_app_fee(*fee_opt);
            char_printf(ch, "{}'s application fee is now {} platinum.\n", clan.value()->name(), *fee_opt);
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

        // Gods have direct admin access
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            clan.value()->admin_set_min_application_level(*level_opt);
            char_printf(ch, "{}'s minimum application level is now {}.\n", clan.value()->name(), *level_opt);
        } else {
            if (!has_clan_permission(ch, ClanPermission::SET_APP_LEVEL)) {
                char_printf(ch, "You don't have permission to change application level.\n");
                return;
            }
            clan.value()->admin_set_min_application_level(*level_opt);
            char_printf(ch, "{}'s minimum application level is now {}.\n", clan.value()->name(), *level_opt);
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

        const auto &ranks = clan.value()->ranks();
        if (*rank_opt > ranks.size()) {
            char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", ranks.size());
            return;
        }

        // NOTE: This would require adding admin_remove_rank to Clan class
        // For now, just indicate it's not implemented
        char_printf(ch, "Rank deletion is not yet implemented.\n");
        return;
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

        // Gods have direct admin access
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            clan.value()->admin_set_dues(*dues_opt);
            char_printf(ch, "{}'s monthly dues are now {} platinum.\n", clan.value()->name(), *dues_opt);
        } else {
            if (!has_clan_permission(ch, ClanPermission::SET_DUES)) {
                char_printf(ch, "You don't have permission to change dues.\n");
                return;
            }
            clan.value()->admin_set_dues(*dues_opt);
            char_printf(ch, "{}'s monthly dues are now {} platinum.\n", clan.value()->name(), *dues_opt);
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

        // Gods have direct admin access
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            clan.value()->admin_set_name(std::string(new_name));
            char_printf(ch, "Clan name updated to {}.\n", new_name);
        } else {
            if (!has_clan_permission(ch, ClanPermission::LEADER_OVERRIDE)) {
                char_printf(ch, "You don't have permission to change the clan name.\n");
                return;
            }
            clan.value()->admin_set_name(std::string(new_name));
            char_printf(ch, "Clan name updated to {}.\n", new_name);
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

        const auto &ranks = clan.value()->ranks();
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
                set_permission(new_permissions, ClanPermission::SET_DESCRIPTION);
            } else if (perm_str == "motd") {
                set_permission(new_permissions, ClanPermission::SET_MOTD);
            } else if (perm_str == "grant") {
                set_permission(new_permissions, ClanPermission::LEADER_OVERRIDE);
            } else if (perm_str == "ranks") {
                set_permission(new_permissions, ClanPermission::MANAGE_RANKS);
            } else if (perm_str == "title") {
                set_permission(new_permissions, ClanPermission::NONE);
            } else if (perm_str == "enroll") {
                set_permission(new_permissions, ClanPermission::INVITE_MEMBERS);
            } else if (perm_str == "expel") {
                set_permission(new_permissions, ClanPermission::KICK_MEMBERS);
            } else if (perm_str == "promote") {
                set_permission(new_permissions, ClanPermission::PROMOTE_MEMBERS);
            } else if (perm_str == "demote") {
                set_permission(new_permissions, ClanPermission::DEMOTE_MEMBERS);
            } else if (perm_str == "app_fees") {
                set_permission(new_permissions, ClanPermission::SET_APP_FEES);
            } else if (perm_str == "app_level") {
                set_permission(new_permissions, ClanPermission::SET_APP_LEVEL);
            } else if (perm_str == "dues") {
                set_permission(new_permissions, ClanPermission::SET_DUES);
            } else if (perm_str == "deposit") {
                set_permission(new_permissions, ClanPermission::DEPOSIT_FUNDS);
            } else if (perm_str == "withdraw") {
                set_permission(new_permissions, ClanPermission::WITHDRAW_FUNDS);
            } else if (perm_str == "store") {
                set_permission(new_permissions, ClanPermission::STORE_ITEMS);
            } else if (perm_str == "retrieve") {
                set_permission(new_permissions, ClanPermission::RETRIEVE_ITEMS);
            } else if (perm_str == "alts") {
                set_permission(new_permissions, ClanPermission::MANAGE_ALTS);
            } else if (perm_str == "chat") {
                set_permission(new_permissions, ClanPermission::CLAN_CHAT);
            } else {
                char_printf(ch,
                            "Unknown permission '{}'. Use 'clan set permissions {}' to see available permissions.\n",
                            perm_str, *rank_opt);
                return;
            }
        }

        // Update the rank permissions directly
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            if (clan.value()->admin_update_rank_permissions(*rank_opt - 1, new_permissions)) {
                char_printf(ch, "Permissions updated for rank {} ({}).\n", *rank_opt, current_rank.title());
            } else {
                char_printf(ch, "Failed to update permissions for rank {}.\n", *rank_opt);
                return;
            }
        } else {
            if (!has_clan_permission(ch, ClanPermission::MANAGE_RANKS)) {
                char_printf(ch, "You don't have permission to change rank permissions.\n");
                return;
            }
            if (clan.value()->admin_update_rank_permissions(*rank_opt - 1, new_permissions)) {
                char_printf(ch, "Permissions updated for rank {} ({}).\n", *rank_opt, current_rank.title());
            } else {
                char_printf(ch, "Failed to update permissions for rank {}.\n", *rank_opt);
                return;
            }
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

        const auto &ranks = clan.value()->ranks();
        if (*rank_opt > ranks.size()) {
            char_printf(ch, "Invalid rank number. Clan has {} ranks.\n", ranks.size());
            return;
        }

        // NOTE: This would require adding admin_update_rank_title to Clan class
        // For now, just indicate it's not implemented
        char_printf(ch, "Rank title updates are not yet implemented.\n");
        return;
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

        clan.value()->admin_set_bank_room(room_vnum);
        if (room_vnum == NOWHERE) {
            char_printf(ch, "Clan bank access disabled.\n");
        } else {
            char_printf(ch, "Clan bank room set to {}.\n", room_vnum);
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

        clan.value()->admin_set_chest_room(room_vnum);
        if (room_vnum == NOWHERE) {
            char_printf(ch, "Clan chest access disabled.\n");
        } else {
            char_printf(ch, "Clan chest room set to {}.\n", room_vnum);
        }
    } else {
        char_printf(ch, "Unknown clan set option '{}'. Use 'clan set' to see available options.\n", cmd);
        return;
    }

    clan_repository.save();
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_set, "clan_set", CommandCategory::CLAN,
                                permissions::clan_permission(ClanPermission::MANAGE_RANKS),
                                "set <clan> <option>          - Set clan properties (admin only)");

CLAN_COMMAND(clan_apply, clan_permissions::RequiresNoPermissions) {
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
        char_printf(ch, "Assuming you meant '{}' ({}).\n", strip_ansi((*clan)->name()),
                    strip_ansi((*clan)->abbreviation()));
    }

    // Check if character meets application requirements
    if (GET_LEVEL(ch) < (*clan)->min_application_level()) {
        char_printf(ch, "You must be at least level {} to apply to {}.\n", (*clan)->min_application_level(),
                    (*clan)->name());
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
    if (get_clan_id(ch) == (*clan)->id()) {
        char_printf(ch, "You are already a member of {}.\n", (*clan)->name());
        return;
    }

    // For now, applications are disabled - clan members must manually invite
    char_printf(ch, "Clan applications are currently disabled. Contact a clan member to be invited.\n");
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_apply, "clan_apply", CommandCategory::CLAN, permissions::PUBLIC,
                                "apply <clan>                 - Apply to join a clan");

// SECTION: God-only commands (require CLAN_ADMIN permission)

CLAN_COMMAND(clan_create, clan_permissions::RequiresPermissions<ClanPermission::CLAN_ADMIN>) {
    if (!check_god_only_operation(ch, "create clans")) {
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
    ClanID new_id = clan_repository.count() + 1;

    auto clan = clan_repository.create(new_id, std::string{abbreviation}, std::string{abbreviation});
    if (!clan) {
        char_printf(ch, "Error creating clan.\n");
        return;
    }

    // Add default ranks
    // Leader rank with full permissions
    PermissionSet leader_permissions;
    leader_permissions.set(); // Set all bits for full admin access
    if (auto result = clan->admin_add_rank(ClanRank("Leader", leader_permissions)); !result) {
        char_printf(ch, "Failed to create leader rank: {}\n", result.error());
        clan_repository.remove(clan->id());
        return;
    }

    // Member rank with basic permissions
    PermissionSet member_permissions;
    set_permission(member_permissions, ClanPermission::MANAGE_ALTS);
    set_permission(member_permissions, ClanPermission::CLAN_CHAT);
    if (auto result = clan->admin_add_rank(ClanRank("Member", member_permissions)); !result) {
        char_printf(ch, "Failed to create member rank: {}\n", result.error());
        clan_repository.remove(clan->id());
        return;
    }

    // Add the creator as the leader (rank 0)
    std::string creator_name = GET_NAME(ch);
    if (clan->add_member_by_name(creator_name, 0)) { // 0-based index, so rank 0 is the leader
        ch->player_specials->clan_id = clan->id();
        save_player(ch);
    } else {
        char_printf(ch, "Warning: Failed to add you as clan leader.\n");
    }

    char_printf(ch, "New clan {} created.\n", abbreviation);
    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} creates new clan: {}", GET_NAME(ch), abbreviation);

    clan_repository.save();
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_create, "clan_create", CommandCategory::CLAN, permissions::GOD_ONLY,
                                "create <abbreviation>        - Create a new clan (gods only)");

CLAN_COMMAND(clan_destroy, clan_permissions::RequiresPermissions<ClanPermission::CLAN_ADMIN>) {
    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    auto clan_name = clan.value()->name();
    auto clan_id = clan.value()->id();

    // Notify all members that the clan is disbanded
    clan.value()->notify(ch, "Your clan has been disbanded!");

    char_printf(ch, AFMAG "You have deleted the clan {}&0.\n" ANRM, clan_name);
    log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} has destroyed the clan {}.", GET_NAME(ch), clan_name);

    clan_repository.remove(clan_id);
    clan_repository.save();
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_destroy, "clan_destroy", CommandCategory::CLAN, permissions::GOD_ONLY,
                                "destroy <clan>               - Destroy a clan (gods only)");

CLAN_COMMAND(clan_info, clan_permissions::RequiresNoPermissions) {
    auto clan_name = argument.get();
    ClanPtr clan;

    if (clan_name.empty()) {
        auto clan_opt = find_clan_for_command(ch, argument);
        if (!clan_opt) {
            char_printf(ch, "Which clan's info do you want to view?\n");
            return;
        }
        clan = clan_opt.value();
    } else {
        auto [found_clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);

        if (!found_clan) {
            char_printf(ch, "'{}' does not refer to a valid clan.\n", clan_name);
            show_clan_suggestions(ch, clan_name);
            return;
        }

        if (used_fuzzy) {
            char_printf(ch, "Assuming you meant '{}' ({}).\n", strip_ansi((*found_clan)->name()),
                        strip_ansi((*found_clan)->abbreviation()));
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

    // Show clan description
    if (!clan->description().empty()) {
        paging_printf(ch, "\nDescription:\n{}", clan->description());
    } else {
        paging_printf(ch, "\nNo description set for this clan.\n");
    }

    // Show additional information for members
    bool show_all = GET_LEVEL(ch) >= LVL_IMMORT || (get_clan_id(ch) == clan->id());

    if (show_all) {
        paging_printf(ch, "Treasure: {}\n", statemoney(clan->treasure()));

        paging_printf(ch, "Number of items in Storage: {}\n", clan->storage().size());

        paging_printf(ch, "\nRanks:\n");
        for (size_t i = 0; i < clan->ranks().size(); ++i) {
            paging_printf(ch, "{:3}  {}\n", i + 1, clan->ranks()[i].title());
        }

        if (show_all && !clan->motd().empty()) {
            paging_printf(ch, "\nMessage of the Day:\n{}\n\n", clan->motd());
        }
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        paging_printf(ch, "Bank Room: {}\n",
                      clan->bank_room() == NOWHERE ? "Disabled" : std::to_string(clan->bank_room()));
        paging_printf(ch, "Chest Room: {}\n",
                      clan->chest_room() == NOWHERE ? "Disabled" : std::to_string(clan->chest_room()));
        paging_printf(ch, "Hall Room: {}\n",
                      clan->hall_room() == NOWHERE ? "Disabled" : std::to_string(clan->hall_room()));
    }

    // Show user's rank and permissions if they're a member of this clan
    if (get_clan_id(ch) == clan->id()) {
        auto member = get_clan_member(ch);
        auto rank = get_clan_rank(ch);

        if (member && rank) {
            paging_printf(ch, "\n{:-^70}\n", "[ Your Clan Status ]");
            paging_printf(ch, "Your Rank: {} ({})\n", member->rank_index + 1, rank->title());

            // Show permissions
            paging_printf(ch, "Your Permissions:\n");
            bool has_any_permission = false;

            // List all permissions the user has
            for (int i = 0; i < static_cast<int>(ClanPermission::MAX_PERMISSIONS); ++i) {
                ClanPermission perm = static_cast<ClanPermission>(i);
                if (rank->has_permission(perm)) {
                    if (!has_any_permission) {
                        has_any_permission = true;
                    }

                    // Convert permission enum to readable name - only show used/settable permissions
                    std::string perm_name;
                    switch (perm) {
                    case ClanPermission::CLAN_CHAT:
                        perm_name = "Clan Chat";
                        break;
                    case ClanPermission::INVITE_MEMBERS:
                        perm_name = "Invite Members";
                        break;
                    case ClanPermission::KICK_MEMBERS:
                        perm_name = "Kick Members";
                        break;
                    case ClanPermission::PROMOTE_MEMBERS:
                        perm_name = "Promote Members";
                        break;
                    case ClanPermission::DEMOTE_MEMBERS:
                        perm_name = "Demote Members";
                        break;
                    case ClanPermission::MANAGE_ALTS:
                        perm_name = "Manage Alts";
                        break;
                    case ClanPermission::MANAGE_RANKS:
                        perm_name = "Manage Ranks";
                        break;
                    case ClanPermission::SET_MOTD:
                        perm_name = "Set MOTD";
                        break;
                    case ClanPermission::SET_DESCRIPTION:
                        perm_name = "Set Description";
                        break;
                    case ClanPermission::SET_DUES:
                        perm_name = "Set Dues";
                        break;
                    case ClanPermission::SET_APP_FEES:
                        perm_name = "Set App Fees";
                        break;
                    case ClanPermission::SET_APP_LEVEL:
                        perm_name = "Set App Level";
                        break;
                    case ClanPermission::DEPOSIT_FUNDS:
                        perm_name = "Deposit Funds";
                        break;
                    case ClanPermission::WITHDRAW_FUNDS:
                        perm_name = "Withdraw Funds";
                        break;
                    case ClanPermission::STORE_ITEMS:
                        perm_name = "Store Items";
                        break;
                    case ClanPermission::RETRIEVE_ITEMS:
                        perm_name = "Retrieve Items";
                        break;
                    case ClanPermission::LEADER_OVERRIDE:
                        perm_name = "Leader Override";
                        break;
                    case ClanPermission::CLAN_ADMIN:
                        perm_name = "Clan Admin";
                        break;
                    default:
                        continue; // Skip unused permissions
                    }
                    paging_printf(ch, "  - {}\n", perm_name);
                }
            }

            if (!has_any_permission) {
                paging_printf(ch, "  - None\n");
            }
        }
    }

    start_paging(ch);
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_info, "clan_info", CommandCategory::CLAN, permissions::PUBLIC,
                                "info [clan]                  - Display information about a clan");

// SECTION: Main command handlers and entry points

ACMD(do_clan) {
    if (IS_NPC(ch) || !ch->desc) {
        char_printf(ch, HUH);
        return;
    }

    Arguments args(argument);
    auto command = args.shift();

    if (command.empty()) {
        display_clan_help(ch);
        return;
    }

    // Try to call the command using the function registry with permission checking
    std::string full_command = "clan_" + std::string(command);
    auto permissions = get_clan_permissions(ch);

    if (!FunctionRegistry::call_by_abbrev_with_permissions(full_command, ch, args, permissions)) {
        // Check if the function exists but we lack permissions
        if (FunctionRegistry::can_call_function(full_command, 0, ch)) { // Check if function exists
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

CLAN_COMMAND(clan_snoop, clan_permissions::RequiresPermissions<ClanPermission::CLAN_ADMIN>) {
    if (!check_god_only_operation(ch, "snoop clan communications")) {
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
                    char_printf(ch, "  {} [{}] (ID: {})\n", clan.value()->name(), clan.value()->abbreviation(),
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

        auto clan_name = argument.get();

        // Find clan using fuzzy search
        auto [clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);

        if (!clan) {
            char_printf(ch, "No clan found with name or abbreviation '{}'.\n", clan_name);
            show_clan_suggestions(ch, clan_name);
            return;
        }

        if (used_fuzzy) {
            char_printf(ch, "Assuming you meant '{}' ({}).\n", strip_ansi((*clan)->name()),
                        strip_ansi((*clan)->abbreviation()));
        }

        auto clan_id = (*clan)->id();

        if (is_snooping_clan(ch, clan_id)) {
            char_printf(ch, "You are already snooping {} communications.\n", (*clan)->name());
            return;
        }

        add_clan_snoop(ch, clan_id);
        char_printf(ch, "You are now snooping {} [{}] communications.\n", (*clan)->name(), (*clan)->abbreviation());
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} starts snooping clan {} communications", GET_NAME(ch),
            (*clan)->name());

    } else if (subcommand == "stop" || subcommand == "off") {
        if (argument.empty()) {
            char_printf(ch, "Stop snooping which clan? Usage: clan snoop stop <clan>\n");
            return;
        }

        auto clan_name = argument.get();

        // Find clan using fuzzy search
        auto [clan, used_fuzzy] = find_clan_with_fuzzy_search(clan_name);

        if (!clan) {
            char_printf(ch, "No clan found with name or abbreviation '{}'.\n", clan_name);
            show_clan_suggestions(ch, clan_name);
            return;
        }

        if (used_fuzzy) {
            char_printf(ch, "Assuming you meant '{}' ({}).\n", strip_ansi((*clan)->name()),
                        strip_ansi((*clan)->abbreviation()));
        }

        auto clan_id = (*clan)->id();

        if (!is_snooping_clan(ch, clan_id)) {
            char_printf(ch, "You are not currently snooping {} communications.\n", (*clan)->name());
            return;
        }

        remove_clan_snoop(ch, clan_id);
        char_printf(ch, "You stop snooping {} [{}] communications.\n", (*clan)->name(), (*clan)->abbreviation());
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} stops snooping clan {} communications", GET_NAME(ch),
            (*clan)->name());

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

REGISTER_FUNCTION_WITH_CATEGORY(clan_snoop, "clan_snoop", CommandCategory::CLAN, permissions::GOD_ONLY,
                                "snoop <start|stop|clear> [clan] - Monitor clan communications (gods only)");

CLAN_COMMAND(clan_enroll, clan_permissions::RequiresPermissions<ClanPermission::INVITE_MEMBERS>) {
    if (!check_god_only_operation(ch, "enroll clan members")) {
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
    auto target = clan_security::find_player_safe(player_name);
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
        char_printf(ch, "Assuming you meant '{}' ({}).\n", strip_ansi((*clan)->name()),
                    strip_ansi((*clan)->abbreviation()));
    }

    // Default to highest rank index (lowest privilege level) if not specified
    int rank_index = static_cast<int>((*clan)->ranks().size()) - 1;
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
            char_printf(ch, "{} is already a member of {}. Updated their rank to '{}'.\n", correct_player_name,
                        (*clan)->name(), rank_title);
            log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} updates {}'s rank in {} to {}", GET_NAME(ch),
                correct_player_name, (*clan)->name(), rank_title);

            // Save clan ID to player file for efficient loading
            target->player_specials->clan_id = (*clan)->id();

            // Player clan_id has already been set above, no additional runtime data needed

            char_printf(target, AFGRN "Your rank in {}&0 has been updated to '{}&0'!\n" ANRM, (*clan)->name(),
                        rank_title);

            // Save changes
            clan_repository.save();
            save_player(target);
        } else {
            char_printf(ch, "Failed to update {}'s rank in {}.\n", correct_player_name, (*clan)->name());
        }
        return;
    }

    // Add the member to persistent storage
    if ((*clan)->add_member_by_name(correct_player_name, rank_index)) {
        char_printf(ch, "{} has been enrolled in {} with rank '{}'.\n", correct_player_name, (*clan)->name(),
                    rank_title);
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} enrolls {} in {} as {}", GET_NAME(ch), correct_player_name,
            (*clan)->name(), rank_title);

        // Save clan ID to player file for efficient loading
        target->player_specials->clan_id = (*clan)->id();

        // Player clan_id has already been set above, no additional runtime data needed

        char_printf(target, AFGRN "You have been enrolled in {}&0 with rank '{}&0'!\n" ANRM, (*clan)->name(),
                    rank_title);

        // Save changes
        clan_repository.save();
        save_player(target);
    } else {
        char_printf(ch, "Failed to enroll {} in {}.\n", correct_player_name, (*clan)->name());
    }
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_enroll, "clan_enroll", CommandCategory::CLAN, permissions::GOD_ONLY,
                                "enroll <clan> <player> [rank] - Enroll a player in a clan (gods only)");

CLAN_COMMAND(clan_expel, clan_permissions::RequiresPermissions<ClanPermission::KICK_MEMBERS>) {
    if (!check_god_only_operation(ch, "expel clan members")) {
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
    auto target = clan_security::find_player_safe(player_name);
    std::string correct_player_name;

    if (target && target->desc && IS_PLAYING(target->desc)) {
        // Player is online, get their exact name
        correct_player_name = GET_NAME(target);
    } else {
        // Player is offline, use the provided name (case-sensitive)
        correct_player_name = std::string(player_name);
        target = nullptr; // Clear target since they're offline
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
        char_printf(ch, "Assuming you meant '{}' ({}).\n", strip_ansi((*clan)->name()),
                    strip_ansi((*clan)->abbreviation()));
    }

    // Check if player is a member
    if (!(*clan)->get_member_by_name(correct_player_name).has_value()) {
        char_printf(ch, "{} is not a member of {}.\n", correct_player_name, (*clan)->name());
        return;
    }

    // Remove the member
    if ((*clan)->remove_member_by_name(correct_player_name)) {
        char_printf(ch, "{} has been expelled from {}.\n", correct_player_name, (*clan)->name());
        log(LogSeverity::Stat, LVL_GOD, "(CLAN) {} expels {} from {}", GET_NAME(ch), correct_player_name,
            (*clan)->name());

        // If player is online, update their runtime data
        if (target) {
            // Clear clan ID from player file
            target->player_specials->clan_id = CLAN_ID_NONE;

            // Player clan_id has already been cleared above, no additional runtime data needed
            char_printf(target, AFYEL "You have been expelled from {}&0!\n" ANRM, (*clan)->name());
        }

        // Save changes
        clan_repository.save();
    } else {
        char_printf(ch, "Failed to expel {} from {}.\n", correct_player_name, (*clan)->name());
    }
}
REGISTER_FUNCTION_WITH_CATEGORY(
    clan_expel, "clan_expel", CommandCategory::CLAN, permissions::GOD_ONLY,
    "expel <clan> <player>        - Expel a player from a clan (online or offline, gods only)");

// SECTION: Member information commands (require basic member permissions)

CLAN_COMMAND(clan_members, clan_permissions::RequiresClanMembership) {
    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    const auto &members = clan.value()->members();
    const auto &ranks = clan.value()->ranks();

    if (members.empty()) {
        char_printf(ch, "{} has no members.\n", clan.value()->name());
        return;
    }

    std::string title = fmt::format("[ Members of {} ({}) ]", clan.value()->name(), clan.value()->abbreviation());
    paging_printf(ch, "{:-^70}\n", title);

    paging_printf(ch, "{:<20} {:<15} {:<20} {:<10}\n", "Name", "Rank", "Title", "Joined");
    paging_printf(ch, "{:-^70}\n", "");

    // Sort members by rank (highest rank first)
    auto sorted_members = members;
    std::ranges::sort(sorted_members, [](const auto &a, const auto &b) {
        return a.rank_index > b.rank_index; // Higher rank index = higher rank
    });

    for (const auto &member : sorted_members) {
        std::string rank_title = "Unknown";
        if (member.rank_index >= 0 && member.rank_index < static_cast<int>(ranks.size())) {
            rank_title = std::string(ranks[member.rank_index].title());
        }

        // Format join time
        char join_date[32];
        struct tm *time_info = localtime(&member.join_time);
        strftime(join_date, sizeof(join_date), "%m/%d/%Y", time_info);

        paging_printf(ch, "{:<20} {:<15} {:<20} {:<10}\n", member.name,
                      member.rank_index + 1, // Display as 1-based
                      rank_title, join_date);
    }

    paging_printf(ch, "{:-^70}\n", "");
    paging_printf(ch, "Total members: {}\n", members.size());

    start_paging(ch);
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_members, "clan_members", CommandCategory::CLAN, permissions::CLAN_MEMBER,
                                "members [clan]               - Show clan member list");

CLAN_COMMAND(clan_chest, clan_permissions::RequiresStorageAccess) {
    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    // Check chest room access
    if (!validate_clan_room_access(ch, clan.value()->chest_room(), "chest")) {
        return;
    }

    const auto &storage = clan.value()->storage();

    if (storage.empty()) {
        char_printf(ch, "The {} clan chest is empty.\n", clan.value()->name());
        return;
    }

    start_paging(ch);
    std::string title = fmt::format("[ {} ({}) Clan Chest ]", clan.value()->name(), clan.value()->abbreviation());
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
    std::ranges::sort(sorted_storage);

    for (const auto &[vnum, quantity] : sorted_storage) {
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
                paging_printf(ch, "{:<6} {:<8} {:<45} {:<8}\n", vnum, quantity, item_name, item_type);
            } else {
                paging_printf(ch, "{:<8} {:<53} {:<8}\n", quantity, item_name, item_type);
            }

            extract_obj(obj);
        } else {
            // Object couldn't be loaded (might be deleted from game)
            if (show_vnums) {
                paging_printf(ch, "{:<6} {:<8} {:<45} {:<8}\n", vnum, quantity, "[MISSING OBJECT]", "N/A");
            } else {
                paging_printf(ch, "{:<8} {:<53} {:<8}\n", quantity, "[MISSING OBJECT]", "N/A");
            }
        }

        total_items += quantity;
    }

    paging_printf(ch, "{:-^70}\n", "");
    paging_printf(ch, "Total unique items: {}  Total item count: {}\n", storage.size(), total_items);
    start_paging(ch);
}
REGISTER_FUNCTION_WITH_CATEGORY(clan_chest, "clan_chest", CommandCategory::CLAN,
                                permissions::clan_permission(ClanPermission::VIEW_STORAGE),
                                "chest [clan]                 - Show clan chest contents");

CLAN_COMMAND(clan_ranks, clan_permissions::RequiresClanMembership) {
    auto clan = find_clan_for_command(ch, argument);
    if (!clan) {
        return;
    }

    const auto &ranks = clan.value()->ranks();

    if (ranks.empty()) {
        char_printf(ch, "{} has no ranks defined.\n", clan.value()->name());
        return;
    }

    char_printf(ch, "Ranks for {}:\n", clan.value()->name());
    char_printf(ch, "================================================================================\n");

    for (size_t i = 0; i < ranks.size(); ++i) {
        const auto &rank = ranks[i];
        char_printf(ch, "Rank {}: {}\n", i + 1, rank.title());

        // Show permissions
        char_printf(ch, "  Permissions: ");
        std::vector<std::string> perms;

        // Check each permission
        if (rank.has_permission(ClanPermission::SET_DESCRIPTION))
            perms.push_back("Description");
        if (rank.has_permission(ClanPermission::SET_MOTD))
            perms.push_back("MOTD");
        if (rank.has_permission(ClanPermission::LEADER_OVERRIDE))
            perms.push_back("Grant");
        if (rank.has_permission(ClanPermission::MANAGE_RANKS))
            perms.push_back("Ranks");
        if (rank.has_permission(ClanPermission::NONE))
            perms.push_back("Title");
        if (rank.has_permission(ClanPermission::INVITE_MEMBERS))
            perms.push_back("Enroll");
        if (rank.has_permission(ClanPermission::KICK_MEMBERS))
            perms.push_back("Expel");
        if (rank.has_permission(ClanPermission::PROMOTE_MEMBERS))
            perms.push_back("Promote");
        if (rank.has_permission(ClanPermission::DEMOTE_MEMBERS))
            perms.push_back("Demote");
        if (rank.has_permission(ClanPermission::SET_APP_FEES))
            perms.push_back("App_Fees");
        if (rank.has_permission(ClanPermission::SET_APP_LEVEL))
            perms.push_back("App_Level");
        if (rank.has_permission(ClanPermission::SET_DUES))
            perms.push_back("Dues");
        if (rank.has_permission(ClanPermission::DEPOSIT_FUNDS))
            perms.push_back("Deposit");
        if (rank.has_permission(ClanPermission::WITHDRAW_FUNDS))
            perms.push_back("Withdraw");
        if (rank.has_permission(ClanPermission::STORE_ITEMS))
            perms.push_back("Store");
        if (rank.has_permission(ClanPermission::RETRIEVE_ITEMS))
            perms.push_back("Retrieve");
        if (rank.has_permission(ClanPermission::MANAGE_ALTS))
            perms.push_back("Alts");
        if (rank.has_permission(ClanPermission::CLAN_CHAT))
            perms.push_back("Chat");

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
REGISTER_FUNCTION_WITH_CATEGORY(clan_ranks, "clan_ranks", CommandCategory::CLAN, permissions::CLAN_MEMBER,
                                "ranks [clan]                 - Show clan ranks and their permissions");

ACMD(do_csnoop) {
    Arguments args(argument);
    clan_snoop(ch, args);
}

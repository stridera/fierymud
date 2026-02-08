#pragma once

// Forward declarations
class CharData;

// Standard library includes for modern C++23 features
#include <bitset>
#include <ctime>
#include <expected>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Game-specific includes from legacy
#include "../../legacy/src/chars.hpp"
#include "../../legacy/src/comm.hpp"
#include "../../legacy/src/conf.hpp"
#include "../../legacy/src/db.hpp"
#include "../../legacy/src/find.hpp"
#include "../../legacy/src/money.hpp"
#include "../../legacy/src/screen.hpp"
#include "../../legacy/src/utils.hpp"

/**
 * Modern C++23 clan permission system
 */
enum class ClanPermission : std::uint32_t {
    // Basic member permissions (0-10)
    NONE = 0,
    CLAN_CHAT = 1,  // Send messages to clan channel
    CLAN_WHO = 2,   // View clan member list
    VIEW_MOTD = 3,  // View clan message of the day
    VIEW_RANKS = 4, // View clan rank structure

    // Invitation and membership management (11-20)
    INVITE_MEMBERS = 11,  // Invite new members
    KICK_MEMBERS = 12,    // Remove members from clan
    PROMOTE_MEMBERS = 13, // Promote members to higher ranks
    DEMOTE_MEMBERS = 14,  // Demote members to lower ranks
    MANAGE_ALTS = 15,     // Add/remove alt characters

    // Clan administration (21-30)
    MANAGE_RANKS = 21,    // Create/modify/delete ranks
    SET_MOTD = 22,        // Set clan message of the day
    SET_DESCRIPTION = 23, // Set clan description
    SET_DUES = 24,        // Set clan dues amounts
    SET_APP_FEES = 25,    // Set application fees
    SET_APP_LEVEL = 26,   // Set minimum application level

    // Financial management (31-40)
    VIEW_FINANCES = 31,  // View clan treasury and financial info
    DEPOSIT_FUNDS = 32,  // Add money to clan treasury
    WITHDRAW_FUNDS = 33, // Remove money from clan treasury
    MANAGE_BANK = 34,    // Set bank room and access rules

    // Storage and equipment (41-50)
    VIEW_STORAGE = 41,   // View clan storage contents
    STORE_ITEMS = 42,    // Add items to clan storage
    RETRIEVE_ITEMS = 43, // Remove items from clan storage
    MANAGE_STORAGE = 44, // Set storage room and access rules

    // Special privileges (51-60)
    LEADER_OVERRIDE = 51, // Can use any clan command regardless of other permissions
    CLAN_ADMIN = 52,      // Administrative powers (gods only)

    // Marker for bitset size
    MAX_PERMISSIONS,
};

constexpr size_t NUM_PERMISSIONS = static_cast<size_t>(ClanPermission::MAX_PERMISSIONS);
constexpr ClanID CLAN_ID_NONE = 0;

// Constants for limits
constexpr size_t MAX_CLAN_NAME_LENGTH = 50;
constexpr size_t MAX_CLAN_ABBR_LENGTH = 10;
constexpr size_t MAX_CLAN_DESCRIPTION_LENGTH = 1000;
constexpr size_t MAX_CLAN_MEMBERS = 100;
constexpr size_t MAX_CLAN_RANKS = 10;
constexpr size_t MAX_CLAN_STORAGE_ITEMS = 1000;

// Type aliases for better readability
using ClanID = unsigned int;
using CharacterPtr = std::shared_ptr<CharData>;
using PermissionSet = std::bitset<NUM_PERMISSIONS>;

// Forward declarations
class Clan;
class ClanRank;
class ClanMember;
class ClanRepository;
using ClanPtr = std::shared_ptr<Clan>;

/**
 * Modern error handling structures
 */
struct PermissionError {
    std::string reason;
    ClanPermission permission = ClanPermission::NONE;
    bool has_permission_context = false;

    PermissionError(std::string_view msg) : reason(msg) {}
    PermissionError(std::string_view msg, ClanPermission perm)
        : reason(msg), permission(perm), has_permission_context(true) {}
    PermissionError(std::string_view msg, ClanPermission perm, bool context)
        : reason(msg), permission(perm), has_permission_context(context) {}
};

struct ValidationError {
    std::string reason;
    std::string field;
    std::string value;

    ValidationError(std::string_view msg, std::string_view fld, std::string_view val)
        : reason(msg), field(fld), value(val) {}
};

using PermissionResult = std::expected<void, PermissionError>;
using ValidationResult = std::expected<void, ValidationError>;

/**
 * Clan rank with modern permission system
 */
class ClanRank {
  public:
    ClanRank() = default;
    ClanRank(std::string title, PermissionSet permissions);

    // Accessors
    const std::string &title() const { return title_; }
    const PermissionSet &privileges() const { return permissions_; }

    // Permission checking
    bool has_permission(ClanPermission permission) const;

    // Setters (for JSON deserialization)
    void set_title(std::string title) { title_ = std::move(title); }
    void set_privileges(PermissionSet permissions) { permissions_ = std::move(permissions); }

    // Comparison for rank cache
    bool operator==(const ClanRank &other) const = default;

    // Hash function for unordered containers
    struct Hash {
        std::size_t operator()(const ClanRank &rank) const;
    };

  private:
    std::string title_;
    PermissionSet permissions_;
};

/**
 * Clan member information
 */
struct ClanMember {
    std::string name;
    int rank_index = 0;
    time_t join_time = 0;
    std::vector<std::string> alts;

    ClanMember() = default;
    ClanMember(std::string name, int rank_idx, time_t join_tm, std::vector<std::string> alts = {})
        : name(std::move(name)), rank_index(rank_idx), join_time(join_tm), alts(std::move(alts)) {}
};

/**
 * Modern clan class using C++23 features
 */
class Clan {
  public:
    // Constructors
    Clan(ClanID id, std::string name, std::string abbreviation);
    ~Clan() = default;

    // Core accessors
    ClanID id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &abbreviation() const { return abbreviation_; }
    const std::string &description() const { return description_; }
    const std::string &motd() const { return motd_; }

    // Financial accessors
    unsigned int dues() const { return dues_; }
    unsigned int app_fee() const { return app_fee_; }
    unsigned int min_application_level() const { return min_application_level_; }
    const Money &treasure() const { return treasure_; }

    // Room accessors
    room_num bank_room() const { return bank_room_; }
    room_num chest_room() const { return chest_room_; }
    room_num hall_room() const { return hall_room_; }

    // Container accessors
    const std::vector<ClanRank> &ranks() const { return ranks_; }
    const std::vector<ClanMember> &members() const { return members_; }
    const std::unordered_map<unsigned long, int> &storage() const { return storage_; }

    // Counts
    size_t member_count() const { return members_.size(); }
    size_t rank_count() const { return ranks_.size(); }

    // Member management
    bool add_member(CharacterPtr character, int rank_index);
    void remove_member(const CharacterPtr &character);
    bool is_member(const CharacterPtr &character) const;
    std::optional<ClanMember> get_member_by_name(std::string_view name) const;
    std::vector<ClanMember> get_members_by_rank_index(int rank_index) const;

    // Member management by name (for persistence)
    bool add_member_by_name(const std::string &name, int rank_index, time_t join_time = 0,
                            std::vector<std::string> alts = {});
    bool remove_member_by_name(const std::string &name);
    bool update_member_rank(const std::string &name, int new_rank_index);

    // Online member management
    std::vector<CharacterPtr> get_members_by_rank(const ClanRank &rank) const;

    // Permission system
    bool has_permission(const CharacterPtr &character, ClanPermission permission) const;
    bool has_permission(const CharData *character, ClanPermission permission) const;
    bool grant_permission(const CharacterPtr &character, ClanPermission permission);

    // Communication
    void notify(const CharacterPtr &skip, std::string_view message);
    void notify(const CharData *skip, std::string_view message);

    // Admin functions (for gods and initialization)
    void admin_set_dues(unsigned int dues) { dues_ = dues; }
    void admin_set_app_fee(unsigned int fee) { app_fee_ = fee; }
    void admin_set_min_application_level(unsigned int level) { min_application_level_ = level; }
    std::expected<void, std::string> admin_add_treasure(const Money &money);
    void admin_set_motd(std::string motd) { motd_ = std::move(motd); }
    std::expected<void, std::string> admin_add_rank(ClanRank rank);

    // Cache invalidation
    void invalidate_rank_cache() { rank_cache_valid_ = false; }

    // JSON serialization support (friend functions declared in cpp)
    friend void to_json(nlohmann::json &j, const Clan &clan);
    friend void from_json(const nlohmann::json &j, Clan &clan);

  private:
    // Basic properties
    ClanID id_;
    std::string name_;
    std::string abbreviation_;
    std::string description_;
    std::string motd_;

    // Financial
    unsigned int dues_ = 0;
    unsigned int app_fee_ = 0;
    unsigned int min_application_level_ = 1;
    Money treasure_;

    // Rooms
    room_num bank_room_ = NOWHERE;
    room_num chest_room_ = NOWHERE;
    room_num hall_room_ = NOWHERE;

    // Members and structure
    std::vector<ClanRank> ranks_;
    std::vector<ClanMember> members_;
    std::unordered_map<unsigned long, int> storage_;

    // Caching for online members
    mutable std::unordered_map<ClanRank, std::vector<CharacterPtr>, ClanRank::Hash> rank_cache_;
    mutable bool rank_cache_valid_ = false;

    void build_rank_cache() const;
};

/**
 * Clan repository for managing all clans
 */
class ClanRepository {
  public:
    // Core accessors
    std::optional<ClanPtr> find_by_id(ClanID id) const;
    std::optional<ClanPtr> find_by_name(std::string_view name) const;
    std::optional<ClanPtr> find_by_abbreviation(std::string_view abbr) const;

    // All clans
    std::vector<ClanPtr> all() const;

    // Persistence
    std::expected<void, std::string> save() const;
    std::expected<void, std::string> load();
    std::expected<void, std::string> save_to_file(const std::filesystem::path &filepath) const;
    std::expected<void, std::string> load_from_file(const std::filesystem::path &filepath);

    // Initialization
    void init_clans();
    bool load_legacy(std::string_view clan_num);

    // Management
    void add_clan(ClanPtr clan) {
        clans_[clan->id()] = std::move(clan);
        caches_valid_ = false;
    }
    void remove_clan(ClanID id) {
        clans_.erase(id);
        caches_valid_ = false;
    }

  private:
    std::unordered_map<ClanID, ClanPtr> clans_;

    // Caches for fast lookups
    mutable std::unordered_map<std::string, ClanID> name_to_id_;
    mutable std::unordered_map<std::string, ClanID> abbr_to_id_;
    mutable bool caches_valid_ = false;

    void build_caches() const;
};

// Global repository instance
extern ClanRepository clan_repository;

// Clan snoop system for administration
// Uses character names instead of raw pointers to avoid dangling pointer issues
extern std::unordered_map<ClanID, std::unordered_set<std::string>> clan_snoop_table;

// Clan snoop management functions (use character name for safe lookup)
void add_clan_snoop(std::string_view char_name, ClanID clan_id);
void remove_clan_snoop(std::string_view char_name, ClanID clan_id);
void remove_all_clan_snoops(std::string_view char_name);
bool is_snooping_clan(std::string_view char_name, ClanID clan_id);
std::vector<ClanID> get_snooped_clans(std::string_view char_name);

// Legacy interface wrappers (extract name from CharData)
void add_clan_snoop(CharData *ch, ClanID clan_id);
void remove_clan_snoop(CharData *ch, ClanID clan_id);
void remove_all_clan_snoops(CharData *ch);
bool is_snooping_clan(CharData *ch, ClanID clan_id);
std::vector<ClanID> get_snooped_clans(CharData *ch);

// Permission checking functions
bool has_clan_permission_or_god(const CharData *ch, ClanPermission permission);

/**
 * Modern C++23 permission checking namespace
 */
namespace clan_permissions {
PermissionResult check_permission(const CharData *ch, ClanPermission permission);
PermissionResult check_clan_member(const CharData *ch);
bool check_god_override(const CharData *ch);
std::string get_permission_error_message(ClanPermission permission);

// Command wrapper functions
bool execute_with_clan_permission(CharData *ch, ClanPermission permission, std::function<void()> command_func);
bool execute_with_clan_membership(CharData *ch, std::function<void()> command_func);
} // namespace clan_permissions

/**
 * Legacy conversion utilities
 */
namespace legacy_conversion {
std::optional<ClanPermission> convert_legacy_privilege(int legacy_value);
PermissionSet convert_legacy_permissions(const std::bitset<64> &legacy_bits);
} // namespace legacy_conversion

/**
 * Security and validation utilities
 */
namespace clan_security {
ValidationResult validate_clan_name(std::string_view name);
ValidationResult validate_clan_abbreviation(std::string_view abbr);
ValidationResult validate_clan_description(std::string_view desc);
ValidationResult validate_player_name(std::string_view name);

CharData *find_player_safe(std::string_view name);
CharData *find_clan_member_safe(std::string_view name);

bool check_member_limit(const Clan &clan);
bool check_rank_limit(const Clan &clan);
bool check_storage_limit(const Clan &clan);

std::expected<Money, std::string> safe_add_money(const Money &base, const Money &addition);
std::expected<Money, std::string> safe_subtract_money(const Money &base, const Money &subtraction);
std::expected<int, std::string> safe_add_quantity(int base, int addition);

bool contains_unsafe_characters(std::string_view input);
} // namespace clan_security

// JSON serialization functions
void to_json(nlohmann::json &j, const ClanRank &rank);
void from_json(const nlohmann::json &j, ClanRank &rank);
void to_json(nlohmann::json &j, const ClanMember &member);
void from_json(const nlohmann::json &j, ClanMember &member);
void to_json(nlohmann::json &j, const Clan &clan);
void from_json(const nlohmann::json &j, Clan &clan);

// Convenience functions that may be referenced in clan.cpp
inline std::optional<ClanPtr> get_clan(const CharData *ch) {
    if (!ch || !ch->player_specials)
        return std::nullopt;
    return clan_repository.find_by_id(ch->player_specials->clan_id);
}

inline std::optional<ClanMember> get_clan_member(const CharData *ch) {
    if (!ch || !ch->player.short_descr)
        return std::nullopt;
    auto clan = get_clan(ch);
    if (!clan)
        return std::nullopt;
    return (*clan)->get_member_by_name(ch->player.short_descr);
}

inline std::optional<ClanRank> get_clan_rank(const CharData *ch) {
    auto member = get_clan_member(ch);
    if (!member)
        return std::nullopt;
    auto clan = get_clan(ch);
    if (!clan)
        return std::nullopt;

    const auto &ranks = (*clan)->ranks();
    if (member->rank_index < 0 || member->rank_index >= static_cast<int>(ranks.size())) {
        return std::nullopt;
    }

    return ranks[member->rank_index];
}

inline bool has_clan_permission(const CharData *ch, ClanPermission permission) {
    auto clan = get_clan(ch);
    if (!clan)
        return false;

    return (*clan)->has_permission(ch, permission);
}

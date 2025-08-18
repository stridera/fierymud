/***************************************************************************
 *  File: clan.h                                          Part of FieryMUD *
 *  Usage: header file for clans                                           *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#pragma once

#include "comm.hpp"
#include "money.hpp"
#include "pfiles.hpp"
#include "structs.hpp"

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

// Forward declarations
class CharData;



// Error Messages with string for user.
struct AccessError {
    static constexpr std::string_view ClanNotFound = "Clan not found.";
    static constexpr std::string_view PermissionDenied = "You do not have permission to do that.";
    static constexpr std::string_view InvalidOperation = "Invalid operation";
};

enum class ClanPermission : std::uint32_t {
    // Basic member permissions (0-10)
    NONE = 0,
    CLAN_CHAT = 1,           // Send messages to clan channel
    CLAN_WHO = 2,            // View clan member list
    VIEW_MOTD = 3,           // View clan message of the day
    VIEW_RANKS = 4,          // View clan rank structure
    
    // Invitation and membership management (11-20)
    INVITE_MEMBERS = 11,     // Invite new members
    KICK_MEMBERS = 12,       // Remove members from clan
    PROMOTE_MEMBERS = 13,    // Promote members to higher ranks
    DEMOTE_MEMBERS = 14,     // Demote members to lower ranks
    MANAGE_ALTS = 15,        // Add/remove alt characters
    
    // Clan administration (21-30)
    MANAGE_RANKS = 21,       // Create/modify/delete ranks
    SET_MOTD = 22,           // Set clan message of the day
    SET_DESCRIPTION = 23,    // Set clan description
    SET_DUES = 24,           // Set clan dues amounts
    SET_APP_FEES = 25,       // Set application fees
    SET_APP_LEVEL = 26,      // Set minimum application level
    
    // Financial management (31-40)
    VIEW_FINANCES = 31,      // View clan treasury and financial info
    DEPOSIT_FUNDS = 32,      // Add money to clan treasury
    WITHDRAW_FUNDS = 33,     // Remove money from clan treasury
    MANAGE_BANK = 34,        // Set bank room and access rules
    
    // Storage and equipment (41-50)
    VIEW_STORAGE = 41,       // View clan storage contents
    STORE_ITEMS = 42,        // Add items to clan storage
    RETRIEVE_ITEMS = 43,     // Remove items from clan storage
    MANAGE_STORAGE = 44,     // Set storage room and access rules
    
    // Special privileges (51-60)
    LEADER_OVERRIDE = 51,    // Can use any clan command regardless of other permissions
    CLAN_ADMIN = 52,         // Administrative powers (gods only)
    
    // Marker for bitset size
    MAX_PERMISSIONS,
};

constexpr size_t NUM_PERMISSIONS = static_cast<size_t>(ClanPermission::MAX_PERMISSIONS);


// Forward declarations
class Clan;

// Type aliases for better readability
using ClanID = unsigned int;
using CharacterPtr = std::shared_ptr<CharData>;
using ClanPtr = std::shared_ptr<Clan>;
using WeakCharacterPtr = std::weak_ptr<CharData>;
using WeakClanPtr = std::weak_ptr<Clan>;
using PermissionSet = std::bitset<NUM_PERMISSIONS>;
using ObjectId = unsigned int; // VNUM of the object
using Storage = std::unordered_map<ObjectId, int>;

// Structure to store member information persistently
struct ClanMember {
    std::string name;
    int rank_index;
    time_t join_time;
    std::vector<std::string> alts;

    ClanMember() = default;
    ClanMember(std::string n, int r, time_t t, std::vector<std::string> a = {})
        : name(std::move(n)), rank_index(r), join_time(t), alts(std::move(a)) {}
};

class ClanRank {
  private:
    std::string title_;
    PermissionSet privileges_;

  public:
    ClanRank(std::string title, PermissionSet privileges)
        : title_(std::move(title)), privileges_(std::move(privileges)) {}

    // Default constructor for deserialization
    ClanRank() = default;

    [[nodiscard]] std::string_view title() const { return title_; }
    [[nodiscard]] PermissionSet privileges() const { return privileges_; }
    [[nodiscard]] bool has_permission(ClanPermission permission) const {
        // CLAN_ADMIN grants all privileges
        if (permission == ClanPermission::CLAN_ADMIN) {
            return privileges_.test(static_cast<size_t>(ClanPermission::CLAN_ADMIN));
        }
        // LEADER_OVERRIDE allows any clan command
        if (privileges_.test(static_cast<size_t>(ClanPermission::LEADER_OVERRIDE))) {
            return true;
        }
        // Normal permission check
        if (static_cast<size_t>(permission) >= NUM_PERMISSIONS) {
            return false;
        }
        return privileges_.test(static_cast<size_t>(permission));
    }

    // Add setter methods
    void set_title(std::string title) { title_ = std::move(title); }
    void set_privileges(PermissionSet privileges) { privileges_ = std::move(privileges); }

    void set_permission(ClanPermission permission, bool value) {
        if (static_cast<size_t>(permission) < NUM_PERMISSIONS) {
            privileges_.set(static_cast<size_t>(permission), value);
        }
    }
    void add_permission(ClanPermission permission) {
        if (static_cast<size_t>(permission) < NUM_PERMISSIONS) {
            privileges_.set(static_cast<size_t>(permission));
        }
    }
    void remove_permission(ClanPermission permission) {
        if (static_cast<size_t>(permission) < NUM_PERMISSIONS) {
            privileges_.reset(static_cast<size_t>(permission));
        }
    }

    bool operator==(const ClanRank &other) const { return title_ == other.title_ && privileges_ == other.privileges_; }
    bool operator<(const ClanRank &other) const {
        if (title_ != other.title_)
            return title_ < other.title_;
        return privileges_.to_string() < other.privileges_.to_string();
    }
};

// Clan class with restricted direct access
class Clan : public std::enable_shared_from_this<Clan> {
  private:
    ClanID id_;
    std::string name_;
    std::string abbreviation_;
    std::string description_;
    std::string motd_;

    unsigned int dues_;
    unsigned int app_fee_;
    unsigned int min_application_level_;
    Money treasure_;
    Storage storage_;

    // Room vnums for clan bank, chest, and hall access (NOWHERE = any room)
    room_num bank_room_;
    room_num chest_room_;
    room_num hall_room_;

    std::vector<ClanRank> ranks_;

    // Store members by name and rank
    std::vector<ClanMember> members_;

    // Cache for performance (mutable to allow modification in const methods)
    mutable std::map<ClanRank, std::vector<CharacterPtr>> rank_cache_;
    mutable bool rank_cache_valid_ = false;

    // Make mutation methods private
    void set_name(std::string new_name) { name_ = std::move(new_name); }
    void set_abbreviation(std::string new_abbreviation) { abbreviation_ = std::move(new_abbreviation); }
    void set_description(std::string new_description) { description_ = std::move(new_description); }
    void set_motd(std::string new_motd) { motd_ = std::move(new_motd); }
    void set_dues(unsigned int new_dues) { dues_ = new_dues; }
    void set_app_fee(unsigned int new_app_fee) { app_fee_ = new_app_fee; }
    void set_min_application_level(unsigned int new_level) { min_application_level_ = new_level; }
    void set_treasure(Money new_treasure) { treasure_ = std::move(new_treasure); }
    void set_bank_room(room_num room) { bank_room_ = room; }
    void set_chest_room(room_num room) { chest_room_ = room; }
    void set_hall_room(room_num room) { hall_room_ = room; }
    [[nodiscard]] std::expected<void, std::string> add_storage_item(ObjectId id, int amount) {
        if (storage_.size() >= 1000) {  // Simple limit check
            return std::unexpected("Storage limit reached.");
        }
        
        auto it = storage_.find(id);
        if (it != storage_.end()) {
            // Simple overflow check
            if (amount > 0 && it->second > std::numeric_limits<int>::max() - amount) {
                return std::unexpected("Quantity addition would cause overflow.");
            }
            it->second += amount;
            return {};
        } else {
            if (amount > 0) {
                storage_[id] = amount;
                return {};
            }
            return std::unexpected("Cannot add negative quantity.");
        }
    }
    void remove_storage_item(ObjectId id, int amount) {
        auto it = storage_.find(id);
        if (it != storage_.end()) {
            it->second -= amount;
            if (it->second <= 0) {
                storage_.erase(it);
            }
        }
    }

    // Method to add money to the clan's treasure with overflow protection
    [[nodiscard]] std::expected<void, std::string> add_treasure(const Money &coins) {
        // Simple overflow check for each denomination
        Money result = treasure_;
        for (int denom = PLATINUM; denom <= COPPER; ++denom) {
            if (treasure_[denom] > 0 && coins[denom] > 0) {
                if (treasure_[denom] > std::numeric_limits<int>::max() - coins[denom]) {
                    return std::unexpected("Money addition would cause overflow.");
                }
            }
            result[denom] = treasure_[denom] + coins[denom];
        }
        treasure_ = result;
        return {};
    }
    
    [[nodiscard]] std::expected<void, std::string> subtract_treasure(const Money &coins) {
        // Simple underflow check
        for (int denom = PLATINUM; denom <= COPPER; ++denom) {
            if (treasure_[denom] < coins[denom]) {
                return std::unexpected("Insufficient coins for withdrawal.");
            }
        }
        Money result = treasure_;
        for (int denom = PLATINUM; denom <= COPPER; ++denom) {
            result[denom] = treasure_[denom] - coins[denom];
        }
        treasure_ = result;
        return {};
    }

    // Add a character with a rank - now returns success status
    bool add_member(CharacterPtr character, int rank_index);

    // Remove a character
    void remove_member(const CharacterPtr &character);

    // Cache management
    void invalidate_rank_cache() const { rank_cache_valid_ = false; }
    void build_rank_cache() const;

    // Allow Repository to access private methods for loading
    friend class ClanRepository;

    // Allow test access to private methods
    friend class ClanTestFixture;

  public:
    Clan(ClanID id, std::string name, std::string abbreviation)
        : id_(std::move(id)), name_(std::move(name)), abbreviation_(std::move(abbreviation)), dues_(0), app_fee_(0),
          min_application_level_(0), bank_room_(NOWHERE), chest_room_(NOWHERE), hall_room_(NOWHERE) {}

    // Constants
    static constexpr int MAX_CLAN_ABBR_LEN = 10;
    static constexpr int MAX_CLAN_NAME_LEN = 30;
    static constexpr int MAX_CLAN_TITLE_LEN = 30;

    // Public access is read-only
    [[nodiscard]] ClanID id() const { return id_; }
    [[nodiscard]] std::string_view name() const { return name_; }
    [[nodiscard]] std::string_view abbreviation() const { return abbreviation_; }
    [[nodiscard]] std::string_view description() const { return description_; }
    [[nodiscard]] std::string_view motd() const { return motd_; }
    [[nodiscard]] unsigned int dues() const { return dues_; }
    [[nodiscard]] unsigned int app_fee() const { return app_fee_; }
    [[nodiscard]] unsigned int min_application_level() const { return min_application_level_; }
    [[nodiscard]] Money treasure() const { return treasure_; }
    [[nodiscard]] const Storage &storage() const { return storage_; }
    [[nodiscard]] const std::vector<ClanRank> &ranks() const { return ranks_; }
    [[nodiscard]] room_num bank_room() const { return bank_room_; }
    [[nodiscard]] room_num chest_room() const { return chest_room_; }
    [[nodiscard]] room_num hall_room() const { return hall_room_; }
    [[nodiscard]] const std::vector<ClanMember> &members() const { return members_; }
    [[nodiscard]] std::size_t member_count() const { return members_.size(); }
    [[nodiscard]] std::size_t rank_count() const { return ranks_.size(); }

    // Check if character is a member
    [[nodiscard]] bool is_member(const CharacterPtr &character) const;

    // Member management by name
    [[nodiscard]] std::optional<ClanMember> get_member_by_name(const std::string_view name) const;
    [[nodiscard]] std::vector<ClanMember> get_members_by_rank_index(int rank_index) const;
    [[nodiscard]] bool add_member_by_name(const std::string &name, int rank_index, time_t join_time = 0,
                                          std::vector<std::string> alts = {});
    bool remove_member_by_name(const std::string &name); // Remove nodiscard - cleanup operation
    [[nodiscard]] bool update_member_rank(const std::string &name, int new_rank_index);

    // Get all members with a specific rank
    [[nodiscard]] std::vector<CharacterPtr> get_members_by_rank(const ClanRank &rank) const;

    // Check if a character has a specific permission
    [[nodiscard]] bool has_permission(const CharacterPtr &character, ClanPermission permission) const;

    // Grant specific permission to a member
    bool grant_permission(const CharacterPtr &character, ClanPermission permission);

    // Administrative methods for gods (bypass membership requirements)
    void admin_set_name(std::string new_name) { set_name(std::move(new_name)); }
    void admin_set_abbreviation(std::string new_abbreviation) { set_abbreviation(std::move(new_abbreviation)); }
    void admin_set_description(std::string new_description) { set_description(std::move(new_description)); }
    void admin_set_motd(std::string new_motd) { set_motd(std::move(new_motd)); }
    void admin_set_dues(unsigned int new_dues) { set_dues(new_dues); }
    void admin_set_app_fee(unsigned int new_app_fee) { set_app_fee(new_app_fee); }
    void admin_set_min_application_level(unsigned int new_level) { set_min_application_level(new_level); }
    void admin_set_bank_room(room_num room) { set_bank_room(room); }
    void admin_set_chest_room(room_num room) { set_chest_room(room); }
    void admin_set_hall_room(room_num room) { set_hall_room(room); }
    [[nodiscard]] std::expected<void, std::string> admin_add_rank(ClanRank rank) {
        if (ranks_.size() >= 20) {  // Simple limit check
            return std::unexpected("Maximum rank limit reached.");
        }
        ranks_.push_back(std::move(rank));
        invalidate_rank_cache();
        return {};
    }
    bool admin_update_rank_permissions(size_t rank_index, PermissionSet permissions) {
        if (rank_index >= ranks_.size())
            return false;
        ranks_[rank_index].set_privileges(std::move(permissions));
        return true;
    }
    [[nodiscard]] std::expected<void, std::string> admin_add_treasure(const Money &coins) { 
        return add_treasure(coins); 
    }
    [[nodiscard]] std::expected<void, std::string> admin_subtract_treasure(const Money &coins) { 
        return subtract_treasure(coins); 
    }
    [[nodiscard]] std::expected<void, std::string> admin_add_storage_item(ObjectId id, int amount) { 
        return add_storage_item(id, amount); 
    }
    void admin_remove_storage_item(ObjectId id, int amount) { 
        remove_storage_item(id, amount); 
    }

    // Notify all members of the clan
    void notify(const CharacterPtr &skip, const std::string_view str);
    void notify(const CharData *skip, const std::string_view str);
    template <typename... Args> void notify(const CharacterPtr &skip, std::string_view str, Args &&...args) {
        notify(skip, fmt::vformat(str, fmt::make_format_args(args...)));
    }
    template <typename... Args> void notify(const CharData *skip, std::string_view str, Args &&...args) {
        notify(skip, fmt::vformat(str, fmt::make_format_args(args...)));
    }

    // Make JSON functions friends of the Clan class
    friend void to_json(nlohmann::json &j, const Clan &clan);
    friend void from_json(const nlohmann::json &j, Clan &clan);
};

// Forward declare JSON serialization functions
void to_json(nlohmann::json &j, const ClanRank &rank);
void from_json(const nlohmann::json &j, ClanRank &rank);
void to_json(nlohmann::json &j, const ClanMember &member);
void from_json(const nlohmann::json &j, ClanMember &member);
void to_json(nlohmann::json &j, const Clan &clan);
void from_json(const nlohmann::json &j, Clan &clan);

// Repository class for Clan
class ClanRepository {
  private:
    std::unordered_map<ClanID, ClanPtr> clans_;
    // Performance caches for faster lookups
    mutable std::unordered_map<std::string, ClanID> name_to_id_;
    mutable std::unordered_map<std::string, ClanID> abbr_to_id_;
    mutable bool caches_valid_ = false;

  public:
    ClanRepository() = default;

    ClanPtr create(ClanID id, std::string name, std::string abbreviation) {
        auto clan = std::make_shared<Clan>(std::move(id), std::move(name), std::move(abbreviation));
        clans_[clan->id()] = clan;
        caches_valid_ = false; // Invalidate caches
        return clan;
    }
    void remove(ClanID id) {
        clans_.erase(id);
        caches_valid_ = false; // Invalidate caches
    }

    [[nodiscard]] std::optional<ClanPtr> find_by_id(ClanID id) const {
        auto it = clans_.find(id);
        return it != clans_.end() ? std::optional{it->second} : std::nullopt;
    }
    [[nodiscard]] std::optional<ClanPtr> find_by_name(const std::string_view name) const;
    [[nodiscard]] std::optional<ClanPtr> find_by_abbreviation(const std::string_view abbr) const;

    [[nodiscard]] auto all() const { return clans_ | std::views::values; }
    [[nodiscard]] std::size_t count() const { return clans_.size(); }

    constexpr std::string_view default_file_path() const { return "etc/clans/clans.json"; }
    std::expected<void, std::string> save() const { return save_to_file(default_file_path()); }
    std::expected<void, std::string> save_to_file(const std::filesystem::path &filepath) const;
    std::expected<void, std::string> load() { return load_from_file(default_file_path()); }
    std::expected<void, std::string> load_from_file(const std::filesystem::path &filepath);

    // Load clan membership for character on login
    void load_clan_membership(const CharacterPtr &ch) {
        if (!ch || !ch->player_specials) {
            return;
        }

        // Migration case: if clan_id is CLAN_ID_NONE, try legacy lookup and update clan_id
        if (ch->player_specials->clan_id == CLAN_ID_NONE) {
            auto clan_id = find_clan_by_member_name(ch);
            if (clan_id != CLAN_ID_NONE) {
                ch->player_specials->clan_id = clan_id;
                // Save immediately to ensure clan membership is persistent
                save_player(ch.get());
            }
            return;
        }

        // Verify player is still in their clan (handles expulsion/clan deletion)
        if (!verify_clan_membership(ch)) {
            ch->player_specials->clan_id = CLAN_ID_NONE;
        }
    }

    // Legacy migration: find clan by searching all clans for member name
    [[nodiscard]] ClanID find_clan_by_member_name(const CharacterPtr &ch) {
        if (!ch || !ch->player.short_descr) {
            return CLAN_ID_NONE;
        }

        std::string player_name = ch->player.short_descr;

        // Search all clans for this player
        for (const auto &clan : all()) {
            auto member_opt = clan->get_member_by_name(player_name);
            if (member_opt.has_value()) {
                return clan->id();
            }
        }

        return CLAN_ID_NONE;
    }

    // Verify character is still a valid member of their clan
    [[nodiscard]] bool verify_clan_membership(const CharacterPtr &ch) {
        if (!ch || !ch->player.short_descr || ch->player_specials->clan_id == CLAN_ID_NONE) {
            return false;
        }

        auto clan_opt = find_by_id(ch->player_specials->clan_id);
        if (!clan_opt) {
            return false; // Clan no longer exists
        }

        auto member_opt = clan_opt.value()->get_member_by_name(ch->player.short_descr);
        return member_opt.has_value();
    }

    // Legacy loading function for migration from .clan files to JSON format
    bool load_legacy(const std::string_view clan_num);

    // Initialize clans from legacy files or JSON
    void init_clans();

  private:
    void build_caches() const {
        if (caches_valid_)
            return;

        name_to_id_.clear();
        abbr_to_id_.clear();

        for (const auto &[id, clan] : clans_) {
            name_to_id_[std::string(clan->name())] = id;
            abbr_to_id_[std::string(clan->abbreviation())] = id;
        }

        caches_valid_ = true;
    }
};

// ClanRepository instance
extern ClanRepository clan_repository;

// Clan snooping system - maps clan ID to set of character pointers
extern std::unordered_map<ClanID, std::unordered_set<CharData *>> clan_snoop_table;

// Clan snoop management functions
void add_clan_snoop(CharData *ch, ClanID clan_id);
void remove_clan_snoop(CharData *ch, ClanID clan_id);
void remove_all_clan_snoops(CharData *ch);
bool is_snooping_clan(CharData *ch, ClanID clan_id);
std::vector<ClanID> get_snooped_clans(CharData *ch);

// Direct clan access functions (replacing ClanMembership system)
[[nodiscard]] inline ClanID get_clan_id(const CharData *ch) { 
    return (!ch || !ch->player_specials) ? CLAN_ID_NONE : ch->player_specials->clan_id; 
}

[[nodiscard]] inline std::optional<ClanPtr> get_clan(const CharData *ch) {
    if (!ch || !ch->player_specials || ch->player_specials->clan_id == CLAN_ID_NONE) {
        return std::nullopt;
    }
    return clan_repository.find_by_id(ch->player_specials->clan_id);
}

[[nodiscard]] inline std::optional<ClanMember> get_clan_member(const CharData *ch) {
    auto clan = get_clan(ch);
    if (!clan || !ch->player.short_descr) {
        return std::nullopt;
    }
    return clan.value()->get_member_by_name(ch->player.short_descr);
}

[[nodiscard]] inline std::optional<ClanRank> get_clan_rank(const CharData *ch) {
    auto clan = get_clan(ch);
    auto member = get_clan_member(ch);
    if (!clan || !member) {
        return std::nullopt;
    }

    const auto &ranks = clan.value()->ranks();
    if (member->rank_index < 0 || member->rank_index >= static_cast<int>(ranks.size())) {
        return std::nullopt;
    }

    return ranks[member->rank_index];
}

[[nodiscard]] inline bool has_clan_permission(const CharData *ch, ClanPermission permission) {
    if (!ch) {
        return false;
    }
    auto rank = get_clan_rank(ch);
    if (!rank) {
        return false;
    }
    return rank->has_permission(permission);
}

// Function for use with function registry - checks clan permission or god status
[[nodiscard]] bool has_clan_permission_or_god(const CharData *ch, ClanPermission permission);

// Get clan permissions for function registry (PermissionFlags defined in function_registration.hpp)
[[nodiscard]] uint32_t get_clan_permissions(const CharData *ch);

inline void set_clan_id(CharData *ch, ClanID clan_id) { ch->player_specials->clan_id = clan_id; }

inline void clear_clan_membership(CharData *ch) { ch->player_specials->clan_id = CLAN_ID_NONE; }

// Legacy compatibility function
[[nodiscard]] inline std::optional<ClanPtr> get_clan_membership(const CharData *ch) { return get_clan(ch); }

// Modern C++23 permission checking system with std::expected
namespace clan_permissions {
    
    // Permission check result with detailed error information
    struct PermissionError {
        std::string reason;
        ClanPermission required_permission;
        bool is_clan_member;
        
        PermissionError(std::string r, ClanPermission perm = ClanPermission::NONE, bool member = false)
            : reason(std::move(r)), required_permission(perm), is_clan_member(member) {}
    };
    
    using PermissionResult = std::expected<void, PermissionError>;
    
    // Check if character has specific clan permission
    [[nodiscard]] PermissionResult check_permission(const CharData *ch, ClanPermission permission);
    
    // Check if character is a clan member (any rank)
    [[nodiscard]] PermissionResult check_clan_member(const CharData *ch);
    
    // Simple permission checking helpers
    [[nodiscard]] bool check_god_override(const CharData *ch);
    
    // Enhanced permission checking with detailed error messages
    [[nodiscard]] std::string get_permission_error_message(ClanPermission permission);
    
    // Enhanced permission decorator system for clean, readable command implementation
    
    // Public command decorator (no permissions or clan membership required)
    struct RequiresNoPermissions {
        template<typename Func>
        static bool execute(CharData *ch, Func&& command_func) {
            command_func();
            return true;
        }
    };

template<ClanPermission... Permissions>
struct RequiresPermissions {
    static constexpr std::array<ClanPermission, sizeof...(Permissions)> perms = {Permissions...};
    
    template<typename Func>
    static bool execute(CharData *ch, Func&& command_func) {
        // Gods bypass all permission checks
        if (check_god_override(ch)) {
            command_func();
            return true;
        }
        
        // Check all required permissions
        for (auto perm : perms) {
            auto result = check_permission(ch, perm);
            if (!result) {
                char_printf(ch, "{}", result.error().reason);
                return false;
            }
        }
        
        // Execute command
        command_func();
        return true;
    }
};

// Convenience aliases for common permission combinations
struct RequiresClanMembership {
    template<typename Func>
    static bool execute(CharData *ch, Func&& command_func) {
        // Gods bypass all permission checks
        if (check_god_override(ch)) {
            command_func();
            return true;
        }
        
        // Check clan membership
        auto result = check_clan_member(ch);
        if (!result) {
            char_printf(ch, "{}", result.error().reason);
            return false;
        }
        
        // Execute command
        command_func();
        return true;
    }
};
using RequiresClanChat = RequiresPermissions<ClanPermission::CLAN_CHAT>;
using RequiresFinancialAccess = RequiresPermissions<ClanPermission::VIEW_FINANCES>;
using RequiresDepositFunds = RequiresPermissions<ClanPermission::DEPOSIT_FUNDS>;
using RequiresWithdrawFunds = RequiresPermissions<ClanPermission::WITHDRAW_FUNDS>;
using RequiresStorageAccess = RequiresPermissions<ClanPermission::VIEW_STORAGE>;
using RequiresStoreItems = RequiresPermissions<ClanPermission::STORE_ITEMS>;
using RequiresRetrieveItems = RequiresPermissions<ClanPermission::RETRIEVE_ITEMS>;
using RequiresMemberManagement = RequiresPermissions<ClanPermission::INVITE_MEMBERS, ClanPermission::KICK_MEMBERS>;
using RequiresRankManagement = RequiresPermissions<ClanPermission::MANAGE_RANKS>;
using RequiresLeaderOverride = RequiresPermissions<ClanPermission::LEADER_OVERRIDE>;

// Macro for clean clan command declarations with permission requirements
#define CLAN_COMMAND(name, permission_class) \
    static void name##_impl(CharData *ch, Arguments argument); \
    static void(name)(CharData *ch, Arguments argument) { \
        permission_class::execute(ch, [ch, argument]() { name##_impl(ch, argument); }); \
    } \
    static void name##_impl(CharData *ch, Arguments argument)

// Command wrapper functions (simpler than templates)
bool execute_with_clan_permission(CharData *ch, ClanPermission permission, 
                                  std::function<void()> command_func);
bool execute_with_clan_membership(CharData *ch, std::function<void()> command_func);

} // namespace clan_permissions

// Legacy conversion functions for migration from old ClanPrivilege system
namespace legacy_conversion {
    // Convert old ClanPrivilege values to new ClanPermission values
    [[nodiscard]] std::optional<ClanPermission> convert_legacy_privilege(int legacy_value);
    
    // Convert legacy bitset to new permission set
    [[nodiscard]] PermissionSet convert_legacy_permissions(const std::bitset<64>& legacy_bits);
    
} // namespace legacy_conversion

// Complete security and validation utilities namespace
namespace clan_security {
    
    // Input validation constants  
    static constexpr size_t MAX_CLAN_NAME_LENGTH = 30;
    static constexpr size_t MAX_CLAN_ABBR_LENGTH = 10;
    static constexpr size_t MAX_CLAN_DESCRIPTION_LENGTH = 1000;
    static constexpr size_t MAX_CLAN_MOTD_LENGTH = 2000;
    static constexpr size_t MAX_CLAN_MEMBERS = 100;
    static constexpr size_t MAX_CLAN_RANKS = 20;
    static constexpr size_t MAX_CLAN_STORAGE_ITEMS = 1000;
    
    // Input validation result
    struct ValidationError {
        std::string reason;
        std::string field;
        std::string value;
        
        ValidationError(std::string r, std::string f = "", std::string v = "")
            : reason(std::move(r)), field(std::move(f)), value(std::move(v)) {}
    };
    
    using ValidationResult = std::expected<void, ValidationError>;
    
    // Resource limit validation
    [[nodiscard]] bool check_member_limit(const Clan& clan);
    [[nodiscard]] bool check_rank_limit(const Clan& clan);
    [[nodiscard]] bool check_storage_limit(const Clan& clan);
    
    // Financial operation safety
    [[nodiscard]] std::expected<Money, std::string> safe_add_money(const Money& base, const Money& addition);
    [[nodiscard]] std::expected<Money, std::string> safe_subtract_money(const Money& base, const Money& subtraction);
    [[nodiscard]] std::expected<int, std::string> safe_add_quantity(int base, int addition);
    
    // Safe character lookup without const_cast  
    [[nodiscard]] CharData* find_player_safe(std::string_view name);
    [[nodiscard]] CharData* find_clan_member_safe(std::string_view name);
    
    // Basic string safety check
    [[nodiscard]] bool contains_unsafe_characters(std::string_view input);
    
} // namespace clan_security

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


// Error Messages with string for user.
struct AccessError {
    static constexpr std::string_view ClanNotFound = "Clan not found.";
    static constexpr std::string_view PermissionDenied = "You do not have permission to do that.";
    static constexpr std::string_view InvalidOperation = "Invalid operation";
};

enum class ClanPrivilege : std::uint8_t {
    None,
    Description,
    Motd,
    Grant,
    Ranks,
    Title,
    Enroll,
    Expel,
    Promote,
    Demote,
    App_Fees,
    App_Level,
    Dues,
    Deposit,
    Withdraw,
    Store,
    Retrieve,
    Alts,
    Chat,
    // Add new privileges here
    MAX_PERMISSIONS,
    Admin = static_cast<std::uint8_t>(0xFF) // Admins have all privileges.
};

constexpr size_t NUM_PERMISSIONS = static_cast<size_t>(ClanPrivilege::MAX_PERMISSIONS);

// Forward declarations
class ClanMembership;
class Clan;

// Type aliases for better readability
using ClanId = unsigned int;

// Special clan ID values
constexpr ClanId CLAN_ID_NONE = 0;                          // No clan
using CharacterPtr = std::shared_ptr<CharData>;
using ClanPtr = std::shared_ptr<Clan>;
using ClanMembershipPtr = std::shared_ptr<ClanMembership>;
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
    [[nodiscard]] bool has_permission(ClanPrivilege privilege) const {
        // Admin is a special value (0xFF) that grants all privileges
        if (privilege == ClanPrivilege::Admin) {
            // Check if this rank has all privileges set
            return privileges_.all();
        }
        // Normal privilege check
        if (static_cast<size_t>(privilege) >= NUM_PERMISSIONS) {
            return false;
        }
        return privileges_.test(static_cast<size_t>(privilege));
    }

    // Add setter methods
    void set_title(std::string title) { title_ = std::move(title); }
    void set_privileges(PermissionSet privileges) { privileges_ = std::move(privileges); }

    void set_privilege(ClanPrivilege privilege, bool value) { 
        if (privilege == ClanPrivilege::Admin) {
            // Admin privilege means all privileges
            if (value) {
                privileges_.set(); // Set all bits
            } else {
                privileges_.reset(); // Clear all bits
            }
        } else if (static_cast<size_t>(privilege) < NUM_PERMISSIONS) {
            privileges_.set(static_cast<size_t>(privilege), value); 
        }
    }
    void add_privilege(ClanPrivilege privilege) { 
        if (privilege == ClanPrivilege::Admin) {
            privileges_.set(); // Set all bits
        } else if (static_cast<size_t>(privilege) < NUM_PERMISSIONS) {
            privileges_.set(static_cast<size_t>(privilege)); 
        }
    }
    void remove_privilege(ClanPrivilege privilege) { 
        if (privilege == ClanPrivilege::Admin) {
            privileges_.reset(); // Clear all bits
        } else if (static_cast<size_t>(privilege) < NUM_PERMISSIONS) {
            privileges_.reset(static_cast<size_t>(privilege)); 
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
    ClanId id_;
    std::string name_;
    std::string abbreviation_;
    std::string description_;
    std::string motd_;

    unsigned int dues_;
    unsigned int app_fee_;
    unsigned int min_application_level_;
    Money treasure_;
    Storage storage_;

    // Room vnums for clan bank and chest access (NOWHERE = any room)
    room_num bank_room_;
    room_num chest_room_;

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
    void add_storage_item(ObjectId id, int amount) {
        auto it = storage_.find(id);
        if (it != storage_.end()) {
            it->second += amount;
        } else {
            storage_[id] = amount;
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

    // Method to add money to the clan's treasure
    void add_treasure(const Money &coins) { treasure_ += coins; }
    void subtract_treasure(const Money &coins) { treasure_ -= coins; }

    // Add a character with a rank
    ClanMembershipPtr add_member(CharacterPtr character, ClanRank rank);

    // Remove a character
    void remove_member(const CharacterPtr &character);

    // Cache management
    void invalidate_rank_cache() const { rank_cache_valid_ = false; }
    void build_rank_cache() const;

    // Allow Membership to access private methods
    friend class ClanMembership;

    // Allow Repository to access private methods for loading
    friend class ClanRepository;

    // Allow test access to private methods
    friend class ClanTestFixture;

  public:
    Clan(ClanId id, std::string name, std::string abbreviation)
        : id_(std::move(id)), name_(std::move(name)), abbreviation_(std::move(abbreviation)), dues_(0), app_fee_(0),
          min_application_level_(0), bank_room_(NOWHERE), chest_room_(NOWHERE) {}

    // Constants
    static constexpr int MAX_CLAN_ABBR_LEN = 10;
    static constexpr int MAX_CLAN_NAME_LEN = 30;
    static constexpr int MAX_CLAN_TITLE_LEN = 30;

    // Public access is read-only
    [[nodiscard]] ClanId id() const { return id_; }
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
    [[nodiscard]] const std::vector<ClanMember> &members() const { return members_; }
    [[nodiscard]] std::size_t member_count() const { return members_.size(); }
    [[nodiscard]] std::size_t rank_count() const { return ranks_.size(); }

    // Get a member's membership
    [[nodiscard]] std::optional<ClanMembershipPtr> get_membership(const CharacterPtr &character) const;

    // Member management by name
    [[nodiscard]] std::optional<ClanMember> get_member_by_name(const std::string_view name) const;
    [[nodiscard]] std::vector<ClanMember> get_members_by_rank_index(int rank_index) const;
    [[nodiscard]] bool add_member_by_name(const std::string& name, int rank_index, time_t join_time = 0, std::vector<std::string> alts = {});
    bool remove_member_by_name(const std::string& name);  // Remove nodiscard - cleanup operation
    [[nodiscard]] bool update_member_rank(const std::string& name, int new_rank_index);

    // Get all members with a specific rank
    [[nodiscard]] std::vector<CharacterPtr> get_members_by_rank(const ClanRank &rank) const;

    // Check if a character has a specific permission
    [[nodiscard]] bool has_permission(const CharacterPtr &character, ClanPrivilege privilege) const;

    // Grant specific permission to a member
    bool grant_permission(const CharacterPtr &character, ClanPrivilege privilege);

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
    void admin_add_rank(ClanRank rank) { ranks_.push_back(std::move(rank)); invalidate_rank_cache(); }
    bool admin_update_rank_permissions(size_t rank_index, PermissionSet permissions) {
        if (rank_index >= ranks_.size()) return false;
        ranks_[rank_index].set_privileges(std::move(permissions));
        return true;
    }

    // Notify all members of the clan
    void notify(const CharacterPtr &skip, const std::string_view str);
    template <typename... Args> void notify(const CharacterPtr &skip, std::string_view str, Args &&...args) {
        notify(skip, fmt::vformat(str, fmt::make_format_args(args...)));
    }

    // Make JSON functions friends of the Clan class
    friend void to_json(nlohmann::json &j, const Clan &clan);
    friend void from_json(const nlohmann::json &j, Clan &clan);
};

// ClanMembership class to represent the relationship and act as a mediator
class ClanMembership : public std::enable_shared_from_this<ClanMembership> {
  private:
    WeakClanPtr Clan_;
    WeakCharacterPtr character_;
    PermissionSet permissions_;
    std::string main_character_name_;

  public:
    ClanRank rank_; // Made public for direct access by friend classes

  public:
    ClanMembership(ClanPtr Clan, CharacterPtr character, ClanRank rank)
        : Clan_(Clan), character_(character), rank_(rank) {}

    [[nodiscard]] ClanRank rank() const { return rank_; }
    [[nodiscard]] ClanPtr clan() const { return Clan_.lock(); }
    [[nodiscard]] CharacterPtr character() const { return character_.lock(); }
    [[nodiscard]] const PermissionSet &permissions() const { return permissions_; }
    [[nodiscard]] std::string_view main_character_name() const {
        if (!main_character_name_.empty()) {
            return main_character_name_;
        }
        if (auto ch = character_.lock()) {
            return ch->player.short_descr;
        }
        return "Unknown";
    }
    [[nodiscard]] bool is_main_character() const { return !main_character_name_.empty(); }
    [[nodiscard]] bool is_member() const { return rank_ != ClanRank{}; }
    [[nodiscard]] bool is_applicant() const { return rank_ == ClanRank{}; }

    [[nodiscard]] std::vector<std::string> get_alts(std::string_view main_character_name) const {
        std::vector<std::string> alts;
        if (auto clan_ptr = clan()) {
            for (const auto &member : clan_ptr->members()) {
                if (member.name == main_character_name) {
                    return member.alts;
                }
            }
        }
        return alts;
    }

    void remove_member() {
        auto character = character_.lock();
        if (auto clan = Clan_.lock()) {
            clan->remove_member(character);
        }

        // Remove from the character's clan membership
        if (character) {
            character->player_specials->clan_membership.reset();
        }
    }

    // Permissions
    void add_permission(ClanPrivilege perm) { 
        if (perm == ClanPrivilege::Admin) {
            permissions_.set(); // Set all bits
        } else if (static_cast<size_t>(perm) < NUM_PERMISSIONS) {
            permissions_.set(static_cast<size_t>(perm)); 
        }
    }
    void remove_permission(ClanPrivilege perm) { 
        if (perm == ClanPrivilege::Admin) {
            permissions_.reset(); // Clear all bits
        } else if (static_cast<size_t>(perm) < NUM_PERMISSIONS) {
            permissions_.reset(static_cast<size_t>(perm)); 
        }
    }
    [[nodiscard]] bool has_permission(ClanPrivilege perm) const { 
        // Admin is a special value (0xFF) that grants all privileges
        if (perm == ClanPrivilege::Admin) {
            // Check if this membership has all privileges set OR if the rank has all privileges
            return permissions_.all() || rank_.has_permission(perm);
        }
        // Normal privilege check
        if (static_cast<size_t>(perm) >= NUM_PERMISSIONS) {
            return false;
        }
        // Check both rank permissions and individual member permissions
        return rank_.has_permission(perm) || permissions_.test(static_cast<size_t>(perm)); 
    }
    
    // Helper function to check if this is a god membership that should bypass character checks
    [[nodiscard]] bool is_god_membership() const;

    // Clan Helper Functions
    [[nodiscard]] ClanPtr get_clan() const { return Clan_.lock(); }
    [[nodiscard]] CharacterPtr get_character() const { return character_.lock(); }

    [[nodiscard]] std::optional<ClanId> get_clan_id() const {
        if (auto clan = get_clan()) {
            return clan->id();
        }
        return std::nullopt;
    }
    [[nodiscard]] std::string_view get_clan_name() const {
        if (auto clan = get_clan()) {
            return clan->name();
        }
        return "";
    }
    [[nodiscard]] std::string_view get_clan_abbreviation() const {
        if (auto clan = get_clan()) {
            return clan->abbreviation();
        }
        return "";
    }
    [[nodiscard]] std::string_view get_clan_description() const {
        if (auto clan = get_clan()) {
            return clan->description();
        }
        return "";
    }
    [[nodiscard]] std::string_view get_clan_motd() const {
        if (auto clan = get_clan()) {
            return clan->motd();
        }
        return "";
    }
    [[nodiscard]] unsigned int get_clan_dues() const {
        if (auto clan = get_clan()) {
            return clan->dues();
        }
        return 0;
    }
    [[nodiscard]] unsigned int get_clan_app_fee() const {
        if (auto clan = get_clan()) {
            return clan->app_fee();
        }
        return 0;
    }
    [[nodiscard]] unsigned int get_clan_min_application_level() const {
        if (auto clan = get_clan()) {
            return clan->min_application_level();
        }
        return 0;
    }
    [[nodiscard]] Money get_clan_treasure() const {
        if (auto clan = get_clan()) {
            return clan->treasure();
        }
        return Money{};
    }
    [[nodiscard]] Storage get_clan_storage() const {
        if (auto clan = get_clan()) {
            return clan->storage();
        }
        return Storage{};
    }
    [[nodiscard]] std::vector<ClanRank> get_clan_ranks() const {
        if (auto clan = get_clan()) {
            return clan->ranks();
        }
        return std::vector<ClanRank>{};
    }
    [[nodiscard]] std::vector<ClanMember> get_clan_members() const {
        if (auto clan = get_clan()) {
            return clan->members();
        }
        return std::vector<ClanMember>{};
    }
    [[nodiscard]] std::size_t get_clan_member_count() const {
        if (auto clan = get_clan()) {
            return clan->member_count();
        }
        return 0;
    }
    [[nodiscard]] std::size_t get_clan_rank_count() const {
        if (auto clan = get_clan()) {
            return clan->rank_count();
        }
        return 0;
    }
    [[nodiscard]] std::optional<ClanMembershipPtr> get_clan_membership(const CharacterPtr &character) const {
        if (auto clan = get_clan()) {
            return clan->get_membership(character);
        }
        return std::nullopt;
    }
    [[nodiscard]] std::vector<CharacterPtr> get_clan_members_by_rank(const ClanRank &rank) const {
        if (auto clan = get_clan()) {
            return clan->get_members_by_rank(rank);
        }
        return std::vector<CharacterPtr>{};
    }
    [[nodiscard]] bool has_clan_permission(const CharacterPtr &character, ClanPrivilege privilege) const {
        if (auto clan = get_clan()) {
            return clan->has_permission(character, privilege);
        }
        return false;
    }
    void notify_clan(const std::string_view str) {
        if (auto clan = get_clan()) {
            clan->notify(character_.lock(), str);
        }
    }
    template <typename... Args> void notify_clan(std::string_view str, Args &&...args) {
        if (auto clan = get_clan()) {
            clan->notify(character_.lock(), fmt::vformat(str, fmt::make_format_args(args...)));
        }
    }

    // Updates
    std::expected<void, std::string_view> update_clan_name(std::string new_name);
    std::expected<void, std::string_view> update_clan_abbreviation(std::string new_abbreviation);
    std::expected<void, std::string_view> update_clan_description(std::string new_description);
    std::expected<void, std::string_view> update_clan_motd(std::string new_motd);
    std::expected<void, std::string_view> update_clan_dues(unsigned int new_dues);
    std::expected<void, std::string_view> update_clan_app_fee(unsigned int new_app_fee);
    std::expected<void, std::string_view> update_clan_min_application_level(unsigned int new_level);
    std::expected<void, std::string_view> add_clan_treasure(Money money);
    std::expected<void, std::string_view> subtract_clan_treasure(Money money);
    std::expected<void, std::string_view> add_clan_storage_item(ObjectId id, int amount);
    std::expected<void, std::string_view> remove_clan_storage_item(ObjectId id, int amount);
    std::expected<void, std::string_view> update_clan_rank(ClanRank new_rank);
    std::expected<void, std::string_view> delete_clan_rank(size_t rank_index);
    std::expected<void, std::string_view> update_clan_rank_title(size_t rank_index, std::string new_title);
    std::expected<void, std::string_view> update_clan_rank_permissions(size_t rank_index, PermissionSet permissions);
    std::expected<void, std::string_view> remove_clan_member(const std::string& member_name);
    std::expected<void, std::string_view> update_clan_member_rank(const std::string& member_name, int new_rank_index);
    std::expected<void, std::string_view> add_clan_member(const std::string& member_name, int rank_index);
    std::expected<void, std::string_view> update_clan_bank_room(room_num room);
    std::expected<void, std::string_view> update_clan_chest_room(room_num room);
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
    std::unordered_map<ClanId, ClanPtr> clans_;
    // Performance caches for faster lookups
    mutable std::unordered_map<std::string, ClanId> name_to_id_;
    mutable std::unordered_map<std::string, ClanId> abbr_to_id_;
    mutable bool caches_valid_ = false;

  public:
    ClanRepository() = default;

    ClanPtr create(ClanId id, std::string name, std::string abbreviation) {
        auto clan = std::make_shared<Clan>(std::move(id), std::move(name), std::move(abbreviation));
        clans_[clan->id()] = clan;
        caches_valid_ = false; // Invalidate caches
        return clan;
    }
    void remove(ClanId id) {
        clans_.erase(id);
        caches_valid_ = false; // Invalidate caches
    }

    [[nodiscard]] std::optional<ClanPtr> find_by_id(ClanId id) const {
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

    [[nodiscard]] std::optional<ClanMembershipPtr> get_clan_membership(const CharacterPtr &ch) {
        return ch->player_specials->clan_membership;
    }
    
    // Load clan membership from persistent storage (efficient version)
    [[nodiscard]] std::optional<ClanMembershipPtr> load_clan_membership_by_id(const CharacterPtr &ch) {
        if (!ch || !ch->player_specials) {
            return std::nullopt;
        }
        
        // Migration case: if clan_id is CLAN_ID_NONE, try legacy lookup and update clan_id
        // This handles existing players who were in clans before the clan_id optimization
        if (ch->player_specials->clan_id == CLAN_ID_NONE) {
            auto legacy_membership = load_clan_membership(ch);
            if (legacy_membership.has_value()) {
                // Found clan membership via legacy search, update clan_id for future efficiency
                auto clan = legacy_membership.value()->clan();
                if (clan) {
                    ch->player_specials->clan_id = clan->id();
                    // Note: Player file will be saved automatically during normal save cycles
                }
                return legacy_membership;
            }
            // Player is not in any clan, leave clan_id as 0
            return std::nullopt;
        }
        
        ClanId clan_id = ch->player_specials->clan_id;
        std::string player_name = ch->player.short_descr ? ch->player.short_descr : "";
        
        if (player_name.empty()) {
            return std::nullopt;
        }
        
        // Direct clan lookup by ID (O(1) instead of O(n))
        auto clan_opt = find_by_id(clan_id);
        if (!clan_opt) {
            // Clan no longer exists, clear the player's clan_id
            ch->player_specials->clan_id = CLAN_ID_NONE;
            return std::nullopt;
        }
        
        auto clan = clan_opt.value();
        
        // Verify player is still a member (handles expulsion)
        auto member_opt = clan->get_member_by_name(player_name);
        if (!member_opt.has_value()) {
            // Player was expelled, clear their clan_id
            ch->player_specials->clan_id = CLAN_ID_NONE;
            return std::nullopt;
        }
        
        const auto& member = member_opt.value();
        
        // Validate rank index
        if (member.rank_index < 0 || member.rank_index >= static_cast<int>(clan->ranks().size())) {
            return std::nullopt;
        }
        
        auto clan_rank = clan->ranks()[member.rank_index];
        return std::make_shared<ClanMembership>(clan, ch, clan_rank);
    }
    
    // Load clan membership from persistent storage (legacy version - searches all clans)
    [[nodiscard]] std::optional<ClanMembershipPtr> load_clan_membership(const CharacterPtr &ch) {
        if (!ch || !ch->player.short_descr) {
            return std::nullopt;
        }
        
        std::string player_name = ch->player.short_descr;
        
        // Search all clans for this player
        for (const auto& clan : all()) {
            auto member_opt = clan->get_member_by_name(player_name);
            if (member_opt.has_value()) {
                const auto& member = member_opt.value();
                
                // Validate rank index
                if (member.rank_index >= 0 && member.rank_index < static_cast<int>(clan->ranks().size())) {
                    auto clan_rank = clan->ranks()[member.rank_index];
                    return std::make_shared<ClanMembership>(clan, ch, clan_rank);
                }
            }
        }
        
        return std::nullopt;
    }

    // TODO: Legacy loading function.  Remove after first load (after json creation)
    bool load_legacy(const std::string_view clan_num);

    // Initialize clans from legacy files or JSON
    void init_clans();

  private:
    void build_caches() const {
        if (caches_valid_) return;

        name_to_id_.clear();
        abbr_to_id_.clear();

        for (const auto& [id, clan] : clans_) {
            name_to_id_[std::string(clan->name())] = id;
            abbr_to_id_[std::string(clan->abbreviation())] = id;
        }

        caches_valid_ = true;
    }
};

// ClanRepository instance
extern ClanRepository clan_repository;

// Clan snooping system - maps clan ID to set of character pointers
extern std::unordered_map<ClanId, std::unordered_set<CharData*>> clan_snoop_table;

// Clan snoop management functions
void add_clan_snoop(CharData *ch, ClanId clan_id);
void remove_clan_snoop(CharData *ch, ClanId clan_id);
void remove_all_clan_snoops(CharData *ch);
bool is_snooping_clan(CharData *ch, ClanId clan_id);
std::vector<ClanId> get_snooped_clans(CharData *ch);

// Todo:  Move to character class when it is refactored
inline std::optional<ClanMembershipPtr> get_clan_membership(const CharData *ch) {
    return ch->player_specials->clan_membership;
}

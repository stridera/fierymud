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
#include <expected>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::literals::string_view_literals;

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
    App_Lev,
    Dues,
    Deposit,
    Withdraw,
    Store,
    Retrieve,
    Alts,
    Chat,
    // Add new privileges here

    God,                                              // God mode allows full access and Snooping.
    MAX_PERMISSIONS = static_cast<std::uint8_t>(0xFF) // Max value for a uint8_t
};

constexpr size_t NUM_PERMISSIONS = static_cast<size_t>(ClanPrivilege::MAX_PERMISSIONS);

// Forward declarations
class ClanMembership;
class Clan;

// Type aliases for better readability
using ClanId = unsigned int;
using CharacterPtr = std::shared_ptr<CharData>;
using ClanPtr = std::shared_ptr<Clan>;
using ClanMembershipPtr = std::shared_ptr<ClanMembership>;
using WeakCharacterPtr = std::weak_ptr<CharData>;
using WeakClanPtr = std::weak_ptr<Clan>;
using PermissionSet = std::bitset<NUM_PERMISSIONS>;
using ObjectId = unsigned int; // VNUM of the object
using Storage = std::unordered_map<ObjectId, int>;

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
        return privileges_.test(static_cast<size_t>(privilege));
    }

    // Add setter methods
    void set_title(std::string title) { title_ = std::move(title); }
    void set_privileges(PermissionSet privileges) { privileges_ = std::move(privileges); }

    void set_privilege(ClanPrivilege privilege, bool value) { privileges_.set(static_cast<size_t>(privilege), value); }
    void add_privilege(ClanPrivilege privilege) { privileges_.set(static_cast<size_t>(privilege)); }
    void remove_privilege(ClanPrivilege privilege) { privileges_.reset(static_cast<size_t>(privilege)); }

    bool operator==(const ClanRank &other) const { return title_ == other.title_ && privileges_ == other.privileges_; }
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

    std::vector<ClanRank> ranks_;

    // Store memberships directly
    std::vector<ClanMembershipPtr> memberships_;

    // Make mutation methods private
    void set_name(std::string new_name) { name_ = std::move(new_name); }
    void set_abbreviation(std::string new_abbreviation) { abbreviation_ = std::move(new_abbreviation); }
    void set_description(std::string new_description) { description_ = std::move(new_description); }
    void set_motd(std::string new_motd) { motd_ = std::move(new_motd); }
    void set_dues(unsigned int new_dues) { dues_ = new_dues; }
    void set_app_fee(unsigned int new_app_fee) { app_fee_ = new_app_fee; }
    void set_min_application_level(unsigned int new_level) { min_application_level_ = new_level; }
    void set_treasure(Money new_treasure) { treasure_ = std::move(new_treasure); }
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

    // Allow Membership to access private methods
    friend class ClanMembership;

  public:
    Clan(ClanId id, std::string name, std::string abbreviation)
        : id_(std::move(id)), name_(std::move(name)), abbreviation_(std::move(abbreviation)) {}

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
    [[nodiscard]] const std::vector<ClanMembershipPtr> &memberships() const { return memberships_; }
    [[nodiscard]] std::size_t member_count() const { return memberships_.size(); }
    [[nodiscard]] std::size_t rank_count() const { return ranks_.size(); }

    // Get a member's membership
    [[nodiscard]] std::optional<ClanMembershipPtr> get_membership(const CharacterPtr &character) const;

    // Get all members with a specific rank
    [[nodiscard]] std::vector<CharacterPtr> get_members_by_rank(const ClanRank &rank) const;

    // Check if a character has a specific permission
    [[nodiscard]] bool has_permission(const CharacterPtr &character, ClanPrivilege privilege) const;

    // Grant specific permission to a member
    bool grant_permission(const CharacterPtr &character, ClanPrivilege privilege);

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
    ClanRank rank_;
    PermissionSet permissions_;
    std::string main_character_name_;

  public:
    ClanMembership(ClanPtr Clan, CharacterPtr character, ClanRank rank)
        : Clan_(Clan), character_(character), rank_(rank) {}

    [[nodiscard]] ClanRank rank() const { return rank_; }
    [[nodiscard]] ClanPtr clan() const { return Clan_.lock(); }
    [[nodiscard]] CharacterPtr character() const { return character_.lock(); }
    [[nodiscard]] const PermissionSet &permissions() const { return permissions_; }
    [[nodiscard]] std::string_view main_character_name() const {
        return main_character_name_.empty() ? character_.lock()->player.short_descr : main_character_name_;
    }
    [[nodiscard]] bool is_main_character() const { return !main_character_name_.empty(); }
    [[nodiscard]] bool is_member() const { return rank_ != ClanRank{}; }
    [[nodiscard]] bool is_applicant() const { return rank_ == ClanRank{}; }

    [[nodiscard]] std::vector<std::string> get_alts(std::string_view main_character_name) const {
        std::vector<std::string> alts;
        for (const auto &membership : clan()->memberships()) {
            if (membership->main_character_name() == main_character_name) {
                alts.push_back(membership->character()->player.short_descr);
            }
        }
        return alts;
    }

    void remove_member() {
        auto character = character_.lock();
        if (auto clan = Clan_.lock()) {
            clan->remove_member(character);
        }

        // Remove from the character's clan memberships
        if (character) {
            auto &memberships = character->player_specials->clan_memberships;
            memberships.erase(std::remove_if(memberships.begin(), memberships.end(),
                                             [this](const ClanMembershipPtr &m) { return m == shared_from_this(); }),
                              memberships.end());
        }
    }

    // Permissions
    void add_permission(ClanPrivilege perm) { permissions_.set(static_cast<size_t>(perm)); }
    void remove_permission(ClanPrivilege perm) { permissions_.reset(static_cast<size_t>(perm)); }
    [[nodiscard]] bool has_permission(ClanPrivilege perm) const { return permissions_.test(static_cast<size_t>(perm)); }

    // Clan Helper Functions
    [[nodiscard]] ClanPtr get_clan() const { return Clan_.lock(); }
    [[nodiscard]] CharacterPtr get_character() const { return character_.lock(); }

    [[nodiscard]] ClanId get_clan_id() const { return get_clan() ? get_clan()->id() : -1; }
    [[nodiscard]] std::string_view get_clan_name() const { return get_clan() ? get_clan()->name() : ""; }
    [[nodiscard]] std::string_view get_clan_abbreviation() const {
        return get_clan() ? get_clan()->abbreviation() : "";
    }
    [[nodiscard]] std::string_view get_clan_description() const { return get_clan() ? get_clan()->description() : ""; }
    [[nodiscard]] std::string_view get_clan_motd() const { return get_clan() ? get_clan()->motd() : ""; }
    [[nodiscard]] unsigned int get_clan_dues() const { return get_clan() ? get_clan()->dues() : 0; }
    [[nodiscard]] unsigned int get_clan_app_fee() const { return get_clan() ? get_clan()->app_fee() : 0; }
    [[nodiscard]] unsigned int get_clan_min_application_level() const {
        return get_clan() ? get_clan()->min_application_level() : 0;
    }
    [[nodiscard]] Money get_clan_treasure() const { return get_clan() ? get_clan()->treasure() : Money{}; }
    [[nodiscard]] Storage get_clan_storage() const { return get_clan() ? get_clan()->storage() : Storage{}; }
    [[nodiscard]] std::vector<ClanRank> get_clan_ranks() const {
        return get_clan() ? get_clan()->ranks() : std::vector<ClanRank>{};
    }
    [[nodiscard]] std::vector<ClanMembershipPtr> get_clan_members() const {
        return get_clan() ? get_clan()->memberships() : std::vector<ClanMembershipPtr>{};
    }
    [[nodiscard]] std::size_t get_clan_member_count() const { return get_clan() ? get_clan()->member_count() : 0; }
    [[nodiscard]] std::size_t get_clan_rank_count() const { return get_clan() ? get_clan()->rank_count() : 0; }
    [[nodiscard]] std::optional<ClanMembershipPtr> get_clan_membership(const CharacterPtr &character) const {
        return get_clan() ? get_clan()->get_membership(character) : std::nullopt;
    }
    [[nodiscard]] std::vector<CharacterPtr> get_clan_members_by_rank(const ClanRank &rank) const {
        return get_clan() ? get_clan()->get_members_by_rank(rank) : std::vector<CharacterPtr>{};
    }
    [[nodiscard]] bool has_clan_permission(const CharacterPtr &character, ClanPrivilege privilege) const {
        return get_clan() ? get_clan()->has_permission(character, privilege) : false;
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
    std::expected<void, std::string_view> update_clan_member(ClanMembershipPtr new_member);
    std::expected<void, std::string_view> update_clan_member_rank(ClanMembershipPtr new_member, ClanRank new_rank);
    std::expected<void, std::string_view> update_clan_member_alts(ClanMembershipPtr new_member,
                                                                  std::vector<std::string> alts);
};

// Forward declare JSON serialization functions
void to_json(nlohmann::json &j, const ClanRank &rank);
void from_json(const nlohmann::json &j, ClanRank &rank);
void to_json(nlohmann::json &j, const Clan &clan);
void from_json(const nlohmann::json &j, Clan &clan);

// Repository class for Clan
class ClanRepository {
  private:
    std::unordered_map<ClanId, ClanPtr> clans_;

  public:
    ClanRepository() = default;

    ClanPtr create(ClanId id, std::string name, std::string abbreviation) {
        auto clan = std::make_shared<Clan>(std::move(id), std::move(name), std::move(abbreviation));
        clans_[clan->id()] = clan;
        return clan;
    }

    [[nodiscard]] std::optional<ClanPtr> find_by_id(ClanId id) const {
        auto it = clans_.find(id);
        return it != clans_.end() ? std::optional{it->second} : std::nullopt;
    }

    [[nodiscard]] std::optional<ClanPtr> find_by_name(const std::string_view name) const;
    [[nodiscard]] std::optional<ClanPtr> find_by_abbreviation(const std::string_view abbr) const;

    void remove(ClanId id) { clans_.erase(id); }

    [[nodiscard]] auto all() const { return clans_ | std::views::values; }
    [[nodiscard]] std::size_t count() const { return clans_.size(); }

    // Move complex file operations to cpp
    constexpr std::string_view default_file_path() const { return "etc/clans/clans.json"sv; }

    std::expected<void, std::string> save() const { return save_to_file(default_file_path()); }
    std::expected<void, std::string> save_to_file(const std::filesystem::path &filepath) const;
    std::expected<void, std::string> load() { return load_from_file(default_file_path()); }
    std::expected<void, std::string> load_from_file(const std::filesystem::path &filepath);

    [[nodiscard]] std::vector<ClanMembershipPtr> get_clan_memberships(const CharacterPtr &ch) {
        std::vector<ClanMembershipPtr> memberships;

        for (const auto &pair : clans_) {
            auto membership_opt = pair.second->get_membership(ch);
            if (membership_opt) {
                memberships.push_back(*membership_opt);
            }
        }
        return memberships;
    }

    // TODO: Legacy loading function.  Remove after first load (after json creation)
    bool load_legacy(const std::string_view clan_num);
};

// ClanRepository instance
static ClanRepository clan_repository;

// Todo:  Move to character class when it is refactored
inline std::vector<ClanMembershipPtr> get_clan_memberships(const CharData *ch) {
    return ch->player_specials->clan_memberships;
}

/***************************************************************************
 *   File: clan.c                                         Part of FieryMUD *
 *  Usage: Front-end for the clan system                                   *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on HubisMUD Copyright (C) 1997, 1998.                *
 *  HubisMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 ***************************************************************************/

#include "clan.hpp"

#include "comm.hpp"
#include "logging.hpp"
#include "money.hpp"
#include "screen.hpp"
#include "utils.hpp"

// Clan class implementation

ClanMembershipPtr Clan::add_member(CharacterPtr character, ClanRank rank) {
    auto membership = std::make_shared<ClanMembership>(shared_from_this(), character, rank);
    // Note: This is for runtime character objects, separate from persistent member storage
    character->player_specials->clan_membership = membership;
    invalidate_rank_cache();
    return membership;
}

void Clan::remove_member(const CharacterPtr &character) {
    // Remove from the character's clan membership
    if (character->player_specials->clan_membership && 
        character->player_specials->clan_membership.value()->character() == character) {
        character->player_specials->clan_membership.reset();
    }
    
    // Also remove from persistent member storage by name
    if (character->player.short_descr) {
        remove_member_by_name(character->player.short_descr);
    }
    
    invalidate_rank_cache();
}

std::optional<ClanMembershipPtr> Clan::get_membership(const CharacterPtr &character) const {
    // Check the character's clan membership for this clan
    if (character->player_specials->clan_membership && 
        character->player_specials->clan_membership.value()->clan().get() == this) {
        return character->player_specials->clan_membership.value();
    }
    
    return std::nullopt;
}

std::optional<ClanMember> Clan::get_member_by_name(const std::string_view name) const {
    auto it = std::ranges::find_if(members_, [&name](const ClanMember &m) { return m.name == name; });
    return it != members_.end() ? std::optional{*it} : std::nullopt;
}

std::vector<ClanMember> Clan::get_members_by_rank_index(int rank_index) const {
    std::vector<ClanMember> result;
    for (const auto &member : members_) {
        if (member.rank_index == rank_index) {
            result.push_back(member);
        }
    }
    return result;
}

bool Clan::add_member_by_name(const std::string& name, int rank_index, time_t join_time, std::vector<std::string> alts) {
    // Check if member already exists
    if (get_member_by_name(name).has_value()) {
        return false;
    }
    
    // Validate rank index
    if (rank_index < 0 || rank_index >= static_cast<int>(ranks_.size())) {
        return false;
    }
    
    if (join_time == 0) {
        join_time = time(nullptr);
    }
    
    members_.emplace_back(name, rank_index, join_time, std::move(alts));
    invalidate_rank_cache();
    return true;
}

bool Clan::remove_member_by_name(const std::string& name) {
    auto it = std::ranges::find_if(members_, [&name](const ClanMember &m) { return m.name == name; });
    if (it != members_.end()) {
        members_.erase(it);
        invalidate_rank_cache();
        return true;
    }
    return false;
}

bool Clan::update_member_rank(const std::string& name, int new_rank_index) {
    // Validate rank index
    if (new_rank_index < 0 || new_rank_index >= static_cast<int>(ranks_.size())) {
        return false;
    }
    
    auto it = std::ranges::find_if(members_, [&name](ClanMember &m) { return m.name == name; });
    if (it != members_.end()) {
        it->rank_index = new_rank_index;
        invalidate_rank_cache();
        return true;
    }
    return false;
}

void Clan::build_rank_cache() const {
    if (rank_cache_valid_)
        return;

    rank_cache_.clear();
    // Note: This cache is for CharacterPtr objects, which are only available when players are online
    // The persistent member storage is separate from this cache
    // TODO: Implement a way to populate this cache from online characters
    rank_cache_valid_ = true;
}

std::vector<CharacterPtr> Clan::get_members_by_rank(const ClanRank &rank) const {
    build_rank_cache();

    auto it = rank_cache_.find(rank);
    if (it != rank_cache_.end()) {
        return it->second;
    }
    return {};
}

bool Clan::has_permission(const CharacterPtr &character, ClanPrivilege privilege) const {
    auto membership = get_membership(character);
    return membership.has_value() && (*membership)->has_permission(privilege);
}

bool Clan::grant_permission(const CharacterPtr &character, ClanPrivilege privilege) {
    auto membership = get_membership(character);
    if (membership) {
        (*membership)->add_permission(privilege);
        return true;
    }
    return false;
}

void Clan::notify(const CharacterPtr &skip, const std::string_view messg) {
    if (!messg.empty()) {
        // TODO: This needs to be updated to work with the new member system
        // For now, we'll need to iterate through all online characters and check if they're members
        // This is a placeholder implementation
    }
}

// ClanMembership implementation

std::expected<void, std::string_view> ClanMembership::update_clan_name(std::string new_name) {
    auto clan_ptr = Clan_.lock();
    if (!clan_ptr) {
        return std::unexpected(AccessError::ClanNotFound);
    }
    
    if (!is_god_membership()) {
        auto character_ptr = character_.lock();
        if (!character_ptr) {
            return std::unexpected(AccessError::ClanNotFound);
        }

        if (character_ptr->player.level < LVL_IMMORT) {
            return std::unexpected(AccessError::PermissionDenied);
        }
    }

    clan_ptr->name_ = std::move(new_name);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_abbreviation(std::string new_abbreviation) {
    static const std::string ABBR_TOO_LONG =
        fmt::format("Clan abbreviations may be at most {} characters in length.", Clan::MAX_CLAN_ABBR_LEN);
    static constexpr std::string_view ABBR_EXISTS = "A clan already exists with that abbreviation.";

    auto clan_ptr = Clan_.lock();
    if (!clan_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    
    if (!is_god_membership()) {
        auto character_ptr = character_.lock();
        if (!character_ptr)
            return std::unexpected(AccessError::ClanNotFound);
        if (character_ptr->player.level < LVL_IMMORT)
            return std::unexpected(AccessError::PermissionDenied);
    }
    
    if (ansi_strlen(new_abbreviation) > Clan::MAX_CLAN_ABBR_LEN) {
        return std::unexpected(ABBR_TOO_LONG);
    }
    if (clan_repository.find_by_abbreviation(new_abbreviation))
        return std::unexpected(ABBR_EXISTS);

    clan_ptr->abbreviation_ = std::move(new_abbreviation);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_description(std::string new_description) {
    const auto clan_ptr = Clan_.lock();
    const auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Description))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->description_ = std::move(new_description);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_motd(std::string new_motd) {
    const auto clan_ptr = Clan_.lock();
    const auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Motd))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->motd_ = std::move(new_motd);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_dues(unsigned int new_dues) {
    const auto clan_ptr = Clan_.lock();
    const auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Dues))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->dues_ = new_dues;
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_app_fee(unsigned int new_app_fee) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::App_Fees))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->app_fee_ = new_app_fee;
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_min_application_level(unsigned int new_level) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::App_Level))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->min_application_level_ = new_level;
    return {};
}

std::expected<void, std::string_view> ClanMembership::add_clan_treasure(Money money) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Deposit))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->add_treasure(money);
    return {};
}
std::expected<void, std::string_view> ClanMembership::subtract_clan_treasure(Money money) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Withdraw))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->subtract_treasure(money);
    return {};
}
std::expected<void, std::string_view> ClanMembership::add_clan_storage_item(ObjectId id, int amount) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Store))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->add_storage_item(id, amount);
    return {};
}
std::expected<void, std::string_view> ClanMembership::remove_clan_storage_item(ObjectId id, int amount) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Retrieve))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->remove_storage_item(id, amount);
    return {};
}
std::expected<void, std::string_view> ClanMembership::update_clan_rank(ClanRank new_rank) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Ranks))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->ranks_.push_back(std::move(new_rank));
    return {};
}
std::expected<void, std::string_view> ClanMembership::remove_clan_member(const std::string& member_name) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Expel))
        return std::unexpected(AccessError::PermissionDenied);

    if (!clan_ptr->remove_member_by_name(member_name)) {
        return std::unexpected("Member not found in clan.");
    }

    return {};
}
std::expected<void, std::string_view> ClanMembership::delete_clan_rank(size_t rank_index) {
    static constexpr std::string_view RANK_NOT_FOUND = "Rank not found.";
    static constexpr std::string_view RANK_IN_USE = "Cannot delete rank - members are using it.";

    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();
    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Ranks))
        return std::unexpected(AccessError::PermissionDenied);

    if (rank_index >= clan_ptr->ranks_.size())
        return std::unexpected(RANK_NOT_FOUND);

    // Check if any members are using this rank
    for (const auto &member : clan_ptr->members_) {
        if (member.rank_index == static_cast<int>(rank_index)) {
            return std::unexpected(RANK_IN_USE);
        }
    }

    clan_ptr->ranks_.erase(clan_ptr->ranks_.begin() + rank_index);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_rank_title(size_t rank_index, std::string new_title) {
    static constexpr std::string_view RANK_NOT_FOUND = "Rank not found.";
    static const std::string TITLE_TOO_LONG =
        fmt::format("Rank titles may be at most {} characters long.", Clan::MAX_CLAN_TITLE_LEN);

    auto clan_ptr = Clan_.lock();
    if (!clan_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    
    if (!is_god_membership()) {
        auto character_ptr = character_.lock();
        if (!character_ptr)
            return std::unexpected(AccessError::ClanNotFound);
        if (!has_permission(ClanPrivilege::Ranks))
            return std::unexpected(AccessError::PermissionDenied);
    }

    if (rank_index >= clan_ptr->ranks_.size())
        return std::unexpected(RANK_NOT_FOUND);

    if (new_title.empty()) {
        return std::unexpected("Rank title cannot be empty.");
    }

    if (ansi_strlen(new_title) > Clan::MAX_CLAN_TITLE_LEN) {
        return std::unexpected(TITLE_TOO_LONG);
    }

    clan_ptr->ranks_[rank_index].set_title(std::move(new_title));
    return {};
}

// Helper function to check if this is a god membership that should bypass character checks
inline bool ClanMembership::is_god_membership() const {
    return rank_.title() == "God";
}

std::expected<void, std::string_view> ClanMembership::update_clan_rank_permissions(size_t rank_index, PermissionSet permissions) {
    static constexpr std::string_view RANK_NOT_FOUND = "Rank not found.";

    auto clan_ptr = Clan_.lock();
    if (!clan_ptr) {
        return std::unexpected(AccessError::ClanNotFound);
    }
    
    // For gods, skip character validation and permission checks
    if (!is_god_membership()) {
        auto character_ptr = character_.lock();
        if (!character_ptr) {
            return std::unexpected(AccessError::ClanNotFound);
        }
        
        if (!has_permission(ClanPrivilege::Ranks))
            return std::unexpected(AccessError::PermissionDenied);
    }

    if (rank_index >= clan_ptr->ranks_.size())
        return std::unexpected(RANK_NOT_FOUND);

    clan_ptr->ranks_[rank_index].set_privileges(std::move(permissions));
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_member_rank(const std::string& member_name, int new_rank_index) {
    static constexpr std::string_view MEMBER_NOT_FOUND = "Member not found in clan.";
    static constexpr std::string_view RANK_UNCHANGED = "Member already has that rank.";

    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();
    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Ranks))
        return std::unexpected(AccessError::PermissionDenied);
    
    auto member_opt = clan_ptr->get_member_by_name(member_name);
    if (!member_opt)
        return std::unexpected(MEMBER_NOT_FOUND);
    
    if (member_opt->rank_index == new_rank_index)
        return std::unexpected(RANK_UNCHANGED);
    
    if (!clan_ptr->update_member_rank(member_name, new_rank_index))
        return std::unexpected(MEMBER_NOT_FOUND);
    
    clan_ptr->invalidate_rank_cache();
    return {};
}

std::expected<void, std::string_view> ClanMembership::add_clan_member(const std::string& member_name, int rank_index) {
    static constexpr std::string_view ALREADY_MEMBER = "Character is already a member of this clan.";

    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();
    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Enroll))
        return std::unexpected(AccessError::PermissionDenied);

    // Check if character is already a member
    if (clan_ptr->get_member_by_name(member_name)) {
        return std::unexpected(ALREADY_MEMBER);
    }

    // Add member to persistent storage
    if (!clan_ptr->add_member_by_name(member_name, rank_index)) {
        return std::unexpected("Failed to add member to clan.");
    }

    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_bank_room(room_num room) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();
    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Admin))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->set_bank_room(room);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_chest_room(room_num room) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();
    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Admin))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->set_chest_room(room);
    return {};
}

// JSON serialization implementations

void to_json(nlohmann::json &j, const ClanRank &rank) {
    j = nlohmann::json{{"title", rank.title()}, {"privileges", rank.privileges().to_string()}};
}

void from_json(const nlohmann::json &j, ClanRank &rank) {
    std::string title = j["title"].get<std::string>();
    std::string privileges_str = j["privileges"].get<std::string>();
    PermissionSet permissions(privileges_str);

    // Use proper setters instead of placement new
    rank.set_title(std::move(title));
    rank.set_privileges(std::move(permissions));
}

void to_json(nlohmann::json &j, const ClanMember &member) {
    j = nlohmann::json{{"name", member.name},
                       {"rank_index", member.rank_index},
                       {"join_time", member.join_time},
                       {"alts", member.alts}};
}

void from_json(const nlohmann::json &j, ClanMember &member) {
    member.name = j["name"].get<std::string>();
    member.rank_index = j["rank_index"].get<int>();
    member.join_time = j["join_time"].get<time_t>();
    if (j.contains("alts") && j["alts"].is_array()) {
        member.alts = j["alts"].get<std::vector<std::string>>();
    }
}

void to_json(nlohmann::json &j, const Clan &clan) {
    j = nlohmann::json{{"id", clan.id()},
                       {"name", clan.name()},
                       {"abbreviation", clan.abbreviation()},
                       {"description", clan.description()},
                       {"motd", clan.motd()},
                       {"dues", clan.dues()},
                       {"app_fee", clan.app_fee()},
                       {"min_application_level", clan.min_application_level()},
                       {"treasure", clan.treasure().to_json()},
                       {"storage", clan.storage()},
                       {"bank_room", clan.bank_room()},
                       {"chest_room", clan.chest_room()},
                       {"ranks", clan.ranks()},
                       {"members", clan.members()}};

    // Add storage as a separate object
    j["storage"] = nlohmann::json::object();
    for (const auto &[obj_id, amount] : clan.storage()) {
        j["storage"][std::to_string(obj_id)] = amount;
    }
}

void from_json(const nlohmann::json &j, Clan &clan) {
    // Validate required fields
    if (!j.contains("id") || !j.contains("name") || !j.contains("abbreviation")) {
        throw std::runtime_error("Missing required clan fields in JSON");
    }

    // Set basic properties using private methods
    clan.id_ = j["id"].get<ClanId>();
    clan.name_ = j["name"].get<std::string>();
    clan.abbreviation_ = j["abbreviation"].get<std::string>();

    if (j.contains("description")) {
        clan.set_description(j["description"].get<std::string>());
    }

    if (j.contains("motd")) {
        clan.set_motd(j["motd"].get<std::string>());
    }

    if (j.contains("dues")) {
        clan.set_dues(j["dues"].get<unsigned int>());
    }

    if (j.contains("app_fee")) {
        clan.set_app_fee(j["app_fee"].get<unsigned int>());
    }

    if (j.contains("min_application_level")) {
        clan.set_min_application_level(j["min_application_level"].get<unsigned int>());
    }

    if (j.contains("treasure")) {
        Money treasure(j["treasure"]);
        clan.set_treasure(std::move(treasure));
    }

    if (j.contains("bank_room")) {
        clan.set_bank_room(j["bank_room"].get<room_num>());
    }

    if (j.contains("chest_room")) {
        clan.set_chest_room(j["chest_room"].get<room_num>());
    }

    // Load ranks
    if (j.contains("ranks") && j["ranks"].is_array()) {
        clan.ranks_.clear();
        for (const auto &j_rank : j["ranks"]) {
            clan.ranks_.emplace_back(std::move(j_rank));
        }
    }

    // Load storage items with validation
    if (j.contains("storage") && j["storage"].is_object()) {
        clan.storage_.clear();
        for (const auto &[obj_id_str, amount] : j["storage"].items()) {
            try {
                ObjectId obj_id = std::stoul(obj_id_str);
                int item_amount = amount.get<int>();
                if (item_amount > 0) {
                    clan.add_storage_item(obj_id, item_amount);
                }
            } catch (const std::exception &e) {
                // Skip invalid storage entries
                continue;
            }
        }
    }
    
    // Load members
    if (j.contains("members") && j["members"].is_array()) {
        clan.members_.clear();
        for (const auto &j_member : j["members"]) {
            ClanMember member;
            from_json(j_member, member);
            clan.members_.push_back(std::move(member));
        }
    }
}

// Global ClanRepository instance
ClanRepository clan_repository;

// ClanRepository implementation

std::optional<ClanPtr> ClanRepository::find_by_name(const std::string_view name) const {
    build_caches();
    
    const auto it = name_to_id_.find(std::string(name));
    if (it != name_to_id_.end()) {
        return find_by_id(it->second);
    }
    return std::nullopt;
}

std::optional<ClanPtr> ClanRepository::find_by_abbreviation(const std::string_view abbr) const {
    build_caches();
    
    const auto it = abbr_to_id_.find(std::string(abbr));
    if (it != abbr_to_id_.end()) {
        return find_by_id(it->second);
    }
    return std::nullopt;
}

std::expected<void, std::string> ClanRepository::save_to_file(const std::filesystem::path &filepath) const {
    try {
        nlohmann::json j_clans = nlohmann::json::array();

        for (const auto &clan_ptr : all()) {
            // Use the to_json conversion directly
            nlohmann::json j_clan = *clan_ptr;
            j_clans.push_back(j_clan);
        }

        std::ofstream file(filepath);
        if (!file) {
            return std::unexpected(fmt::format("Failed to open file: {}", filepath.string()));
        }

        file << std::setw(4) << j_clans << std::endl;
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(fmt::format("Error saving clans: {}", e.what()));
    }
}

std::expected<void, std::string> ClanRepository::load_from_file(const std::filesystem::path &filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            return std::unexpected(fmt::format("File not found: {}", filepath.string()));
        }

        std::ifstream file(filepath);
        if (!file) {
            return std::unexpected(fmt::format("Failed to open file: {}", filepath.string()));
        }

        nlohmann::json j_clans;
        file >> j_clans;

        // Clear existing clans
        clans_.clear();
        caches_valid_ = false;

        for (const auto &j_clan : j_clans) {
            // Extract basic info to create the clan
            ClanId id = j_clan["id"].get<ClanId>();
            std::string name = j_clan["name"].get<std::string>();
            std::string abbreviation = j_clan["abbreviation"].get<std::string>();

            // Create the clan with basic info
            auto clan = create(id, name, abbreviation);

            // Use from_json to populate the rest of the clan data
            from_json(j_clan, *clan);
        }

        return {};
    } catch (const std::exception &e) {
        return std::unexpected(fmt::format("Error loading clans: {}", e.what()));
    }
}

// TODO: Move to config
constexpr std::string_view CLAN_PREFIX = "etc/clans"; /* clan directory		*/
constexpr std::string_view CLAN_SUFFIX = ".clan";     /* clan file suffix		*/
bool ClanRepository::load_legacy(const std::string_view clan_num) {
    std::ifstream file(fmt::format("{}/{}{}", CLAN_PREFIX, clan_num, CLAN_SUFFIX));
    if (!file.is_open()) {
        log("Couldn't open clan file '{}/{}{}'", CLAN_PREFIX, clan_num, CLAN_SUFFIX);
        return false;
    }

    ClanId clan_id = svtoi(clan_num);
    std::string clan_name = "Unknown";
    std::string clan_abbr = "Unknown";
    std::string description = "";
    std::string motd = "";
    unsigned int dues = 0;
    unsigned int app_fee = 0;
    unsigned int min_app_level = 0;
    Money treasure;

    // Map to store rank titles and privileges temporarily
    std::map<int, std::string> rank_titles;
    std::map<int, std::string> rank_privileges;

    // Vector to store member data
    std::vector<std::tuple<std::string, int, time_t, std::string>> member_data;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
            continue;

        auto tag = line.substr(0, colon_pos);
        auto value =
            (colon_pos + 2 < line.length()) ? line.substr(colon_pos + 2) : std::string{}; // Skip ": " if it exists

        if (tag == "number") {
            clan_id = svtoi(value);
        } else if (tag == "name") {
            clan_name = value;
        } else if (tag == "abbr") {
            clan_abbr = value;
        } else if (tag == "description") {
            // Multi-line description ending with ~
            description = "";
            while (std::getline(file, line) && line != "~") {
                if (!description.empty())
                    description += "\n";
                description += line;
            }
        } else if (tag == "motd") {
            // Multi-line MOTD ending with ~
            motd = "";
            while (std::getline(file, line) && line != "~") {
                if (!motd.empty())
                    motd += "\n";
                motd += line;
            }
        } else if (tag == "dues") {
            dues = svtoi(value);
        } else if (tag == "appfee") {
            app_fee = svtoi(value);
        } else if (tag == "applevel") {
            min_app_level = svtoi(value);
        } else if (tag == "copper") {
            treasure[COPPER] = svtoi(value);
        } else if (tag == "silver") {
            treasure[SILVER] = svtoi(value);
        } else if (tag == "gold") {
            treasure[GOLD] = svtoi(value);
        } else if (tag == "platinum") {
            treasure[PLATINUM] = svtoi(value);
        } else if (tag == "title") {
            // Parse "rank_num title_text"
            auto space_pos = value.find(' ');
            if (space_pos != std::string::npos) {
                int rank_num = svtoi(value.substr(0, space_pos));
                std::string title = value.substr(space_pos + 1);
                rank_titles[rank_num] = title;
            }
        } else if (tag == "privilege") {
            // Parse "rank_num privilege_list"
            auto space_pos = value.find(' ');
            if (space_pos != std::string::npos) {
                int rank_num = svtoi(value.substr(0, space_pos));
                std::string privileges = value.substr(space_pos + 1);
                rank_privileges[rank_num] = privileges;
            }
        } else if (tag == "member") {
            // Parse "name rank_num timestamp alt_list"
            std::istringstream iss(value);
            std::string name;
            int rank_num;
            time_t timestamp;

            if (iss >> name >> rank_num >> timestamp) {
                std::string alts;
                std::string alt;
                while (iss >> alt) {
                    if (!alts.empty())
                        alts += " ";
                    alts += alt;
                }
                member_data.emplace_back(name, rank_num, timestamp, alts);
            }
        }
    }

    // Create the clan
    auto clan = std::make_shared<Clan>(clan_id, clan_name, clan_abbr);
    // Set private fields directly since we're in ClanRepository (friend class)
    clan->description_ = description;
    clan->motd_ = motd;
    clan->dues_ = dues;
    clan->app_fee_ = app_fee;
    clan->min_application_level_ = min_app_level;
    clan->treasure_ = treasure;

    // Create ranks from the parsed data
    for (const auto &[rank_num, title] : rank_titles) {
        PermissionSet perms;

        // Parse privileges if they exist for this rank
        if (rank_privileges.find(rank_num) != rank_privileges.end()) {
            std::istringstream priv_stream(rank_privileges[rank_num]);
            std::string priv;
            while (priv_stream >> priv) {
                // Map privilege strings to enum values
                if (priv == "desc")
                    perms.set(static_cast<size_t>(ClanPrivilege::Description));
                else if (priv == "motd")
                    perms.set(static_cast<size_t>(ClanPrivilege::Motd));
                else if (priv == "grant")
                    perms.set(static_cast<size_t>(ClanPrivilege::Grant));
                else if (priv == "ranks")
                    perms.set(static_cast<size_t>(ClanPrivilege::Ranks));
                else if (priv == "title")
                    perms.set(static_cast<size_t>(ClanPrivilege::Title));
                else if (priv == "enroll")
                    perms.set(static_cast<size_t>(ClanPrivilege::Enroll));
                else if (priv == "expel")
                    perms.set(static_cast<size_t>(ClanPrivilege::Expel));
                else if (priv == "promote")
                    perms.set(static_cast<size_t>(ClanPrivilege::Promote));
                else if (priv == "demote")
                    perms.set(static_cast<size_t>(ClanPrivilege::Demote));
                else if (priv == "appfee")
                    perms.set(static_cast<size_t>(ClanPrivilege::App_Fees));
                else if (priv == "applev")
                    perms.set(static_cast<size_t>(ClanPrivilege::App_Level));
                else if (priv == "dues")
                    perms.set(static_cast<size_t>(ClanPrivilege::Dues));
                else if (priv == "deposit")
                    perms.set(static_cast<size_t>(ClanPrivilege::Deposit));
                else if (priv == "withdraw")
                    perms.set(static_cast<size_t>(ClanPrivilege::Withdraw));
                else if (priv == "store")
                    perms.set(static_cast<size_t>(ClanPrivilege::Store));
                else if (priv == "retrieve")
                    perms.set(static_cast<size_t>(ClanPrivilege::Retrieve));
                else if (priv == "alts")
                    perms.set(static_cast<size_t>(ClanPrivilege::Alts));
                else if (priv == "chat")
                    perms.set(static_cast<size_t>(ClanPrivilege::Chat));
            }
        }

        clan->ranks_.emplace_back(title, perms);
    }

    // Add members from the parsed data
    for (const auto& [name, rank_num, timestamp, alts] : member_data) {
        // Convert rank number to rank index (ranks are 1-based in legacy files)
        int rank_index = rank_num - 1;
        
        // Parse alts into vector
        std::vector<std::string> alt_list;
        if (!alts.empty()) {
            std::istringstream alt_stream(alts);
            std::string alt;
            while (alt_stream >> alt) {
                alt_list.push_back(alt);
            }
        }
        
        // Add main member
        if (!clan->add_member_by_name(name, rank_index, timestamp, alt_list)) {
            log("WARNING: Failed to add member '{}' to clan {} during legacy loading", name, clan_id);
        }
        
        // Add alts as members at the lowest rank if they exist
        if (!alt_list.empty() && !clan->ranks_.empty()) {
            int lowest_rank = clan->ranks_.size() - 1; // Lowest rank is last in the list
            for (const auto& alt : alt_list) {
                if (!clan->add_member_by_name(alt, lowest_rank, timestamp)) {
                    log("WARNING: Failed to add alt '{}' to clan {} during legacy loading", alt, clan_id);
                }
            }
        }
    }

    // Add clan to repository
    clans_[clan_id] = clan;

    log("Loaded legacy clan {}: {} ({})", clan_id, clan_name, clan_abbr);
    return true;
}

void ClanRepository::init_clans() {
    // First try to load from JSON
    if (load().has_value()) {
        log("Loaded clans from JSON file.");
        return;
    }

    // If JSON loading fails, try to load from legacy files
    log("JSON loading failed, attempting legacy clan loading...");

    // Try to load existing clan files (numbered 1-999)
    bool loaded_any = false;
    for (int i = 1; i <= 999; ++i) {
        std::string clan_num = std::to_string(i);

        // Check if the file exists
        std::ifstream test_file(fmt::format("{}/{}{}", CLAN_PREFIX, clan_num, CLAN_SUFFIX));
        if (test_file.good()) {
            test_file.close();
            if (load_legacy(clan_num)) {
                loaded_any = true;
            }
        }
    }

    if (loaded_any) {
        log("Loaded {} clans from legacy files.", clans_.size());

        // Save to JSON for future use
        log("Converting legacy clans to JSON format...");
        if (save().has_value()) {
            log("Successfully saved clans to JSON file.");
        } else {
            log("WARNING: Failed to save clans to JSON file.");
        }
    } else {
        log("No clan files found. Starting with empty clan system.");
    }
}

// Clan snooping system implementation

// Global clan snoop table
std::unordered_map<ClanId, std::unordered_set<CharData*>> clan_snoop_table;

void add_clan_snoop(CharData *ch, ClanId clan_id) {
    if (!ch) return;
    clan_snoop_table[clan_id].insert(ch);
}

void remove_clan_snoop(CharData *ch, ClanId clan_id) {
    if (!ch) return;
    auto it = clan_snoop_table.find(clan_id);
    if (it != clan_snoop_table.end()) {
        it->second.erase(ch);
        if (it->second.empty()) {
            clan_snoop_table.erase(it);
        }
    }
}

void remove_all_clan_snoops(CharData *ch) {
    if (!ch) return;
    for (auto it = clan_snoop_table.begin(); it != clan_snoop_table.end();) {
        it->second.erase(ch);
        if (it->second.empty()) {
            it = clan_snoop_table.erase(it);
        } else {
            ++it;
        }
    }
}

bool is_snooping_clan(CharData *ch, ClanId clan_id) {
    if (!ch) return false;
    auto it = clan_snoop_table.find(clan_id);
    if (it != clan_snoop_table.end()) {
        return it->second.count(ch) > 0;
    }
    return false;
}

std::vector<ClanId> get_snooped_clans(CharData *ch) {
    std::vector<ClanId> result;
    if (!ch) return result;
    
    for (const auto& [clan_id, snoopers] : clan_snoop_table) {
        if (snoopers.count(ch) > 0) {
            result.push_back(clan_id);
        }
    }
    return result;
}

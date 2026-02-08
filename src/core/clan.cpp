#include "clan.hpp"

#include "../../legacy/src/chars.hpp"
#include "../../legacy/src/comm.hpp"
#include "../../legacy/src/conf.hpp"
#include "../../legacy/src/db.hpp"
#include "../../legacy/src/find.hpp"
#include "../../legacy/src/screen.hpp"
#include "../../legacy/src/utils.hpp"

#include <expected>
#include <format>

// Global clan repository instance
ClanRepository clan_repository;

// Clan snoop system - maps clan ID to set of character names
// Uses names instead of raw pointers to avoid dangling pointer issues
std::unordered_map<ClanID, std::unordered_set<std::string>> clan_snoop_table;

// Clan class implementation

bool Clan::add_member(CharacterPtr character, int rank_index) {
    if (!character || !character->player.short_descr) {
        return false;
    }

    if (rank_index < 0 || rank_index >= static_cast<int>(ranks_.size())) {
        return false;
    }

    // Add to persistent storage
    bool success = add_member_by_name(character->player.short_descr, rank_index);
    if (success) {
        // Update character's clan_id
        character->player_specials->clan_id = id_;
        invalidate_rank_cache();
    }

    return success;
}

void Clan::remove_member(const CharacterPtr &character) {
    if (!character)
        return;

    // Clear character's clan_id
    character->player_specials->clan_id = CLAN_ID_NONE;

    // Also remove from persistent member storage by name
    if (character->player.short_descr) {
        remove_member_by_name(character->player.short_descr);
    }

    invalidate_rank_cache();
}

bool Clan::is_member(const CharacterPtr &character) const {
    if (!character || !character->player.short_descr) {
        return false;
    }

    // Check if character's clan_id matches this clan
    if (character->player_specials->clan_id != id_) {
        return false;
    }

    // Verify the character is in our member list
    return get_member_by_name(character->player.short_descr).has_value();
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

bool Clan::add_member_by_name(const std::string &name, int rank_index, time_t join_time,
                              std::vector<std::string> alts) {
    // Check if member already exists
    if (get_member_by_name(name).has_value()) {
        return false;
    }

    if (join_time == 0) {
        join_time = time(nullptr);
    }

    members_.emplace_back(name, rank_index, join_time, std::move(alts));
    invalidate_rank_cache();
    return true;
}

bool Clan::remove_member_by_name(const std::string &name) {
    auto it = std::ranges::find_if(members_, [&name](const ClanMember &m) { return m.name == name; });
    if (it != members_.end()) {
        members_.erase(it);
        invalidate_rank_cache();
        return true;
    }
    return false;
}

bool Clan::update_member_rank(const std::string &name, int new_rank_index) {
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

    // Build cache of online characters by rank
    for (CharData *ch = character_list; ch; ch = ch->next) {
        // Skip NPCs and characters without descriptors (not online)
        if (IS_NPC(ch) || !ch->desc) {
            continue;
        }

        // Check if this character is a member of this clan
        if (ch->player_specials->clan_id != id_) {
            continue;
        }

        // Get their clan membership info
        auto member_opt = get_member_by_name(ch->player.short_descr);
        if (!member_opt.has_value()) {
            continue;
        }

        // Get their rank
        int rank_index = member_opt->rank_index;
        if (rank_index < 0 || rank_index >= static_cast<int>(ranks_.size())) {
            continue;
        }

        const auto &rank = ranks_[rank_index];

        // Create a shared_ptr wrapper for the character
        // Note: This is a temporary wrapper for the cache - the actual character
        // lifetime is managed by the game engine, not by these shared_ptrs
        CharacterPtr char_ptr(ch, [](CharData *) { /* no-op deleter */ });

        // Add to the cache
        rank_cache_[rank].push_back(char_ptr);
    }

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

bool Clan::has_permission(const CharacterPtr &character, ClanPermission permission) const {
    return has_permission(character ? character.get() : nullptr, permission);
}

bool Clan::has_permission(const CharData *character, ClanPermission permission) const {
    if (!character || !is_member(character)) {
        return false;
    }

    auto member = get_member_by_name(character->player.short_descr);
    if (!member.has_value()) {
        return false;
    }

    if (member->rank_index < 0 || member->rank_index >= static_cast<int>(ranks_.size())) {
        return false;
    }

    return ranks_[member->rank_index].has_permission(permission);
}

bool Clan::grant_permission(const CharacterPtr &character, ClanPermission permission) {
    // Individual permissions are no longer supported - only rank-based permissions
    // To grant permissions, promote the character to a rank that has the permission
    return false;
}

void Clan::notify(const CharacterPtr &skip, const std::string_view messg) {
    notify(skip ? skip.get() : nullptr, messg);
}

void Clan::notify(const CharData *skip, const std::string_view messg) {
    if (messg.empty()) {
        return;
    }

    // Iterate through all online characters and notify clan members
    for (CharData *ch = character_list; ch; ch = ch->next) {
        // Skip NPCs and characters without descriptors (not online)
        if (IS_NPC(ch) || !ch->desc) {
            continue;
        }

        // Skip the character we don't want to notify
        if (skip && ch == skip) {
            continue;
        }

        // Check if this character is a member of this clan
        if (ch->player_specials->clan_id != id_) {
            continue;
        }

        // Verify they're actually in our member list
        if (!get_member_by_name(ch->player.short_descr).has_value()) {
            continue;
        }

        // Send the message
        char_printf(ch, "{}", messg);
    }
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
                       {"hall_room", clan.hall_room()},
                       {"ranks", clan.ranks()},
                       {"members", clan.members()}};

    // Add storage as a separate object
    j["storage"] = nlohmann::json::object();
    for (const auto &[obj_id, amount] : clan.storage()) {
        j["storage"][std::to_string(obj_id)] = amount;
    }
}

void from_json(const nlohmann::json &j, Clan &clan) {
    clan.id_ = j["id"].get<ClanID>();
    clan.name_ = j["name"].get<std::string>();
    clan.abbreviation_ = j["abbreviation"].get<std::string>();
    clan.description_ = j["description"].get<std::string>();
    clan.motd_ = j["motd"].get<std::string>();
    clan.dues_ = j["dues"].get<unsigned int>();
    clan.app_fee_ = j["app_fee"].get<unsigned int>();
    clan.min_application_level_ = j["min_application_level"].get<unsigned int>();
    clan.treasure_ = Money(j["treasure"]);
    clan.bank_room_ = j["bank_room"].get<room_num>();
    clan.chest_room_ = j["chest_room"].get<room_num>();
    clan.hall_room_ = j["hall_room"].get<room_num>();
    clan.ranks_ = j["ranks"].get<std::vector<ClanRank>>();
    clan.members_ = j["members"].get<std::vector<ClanMember>>();

    // Load storage
    clan.storage_.clear();
    if (j.contains("storage") && j["storage"].is_object()) {
        for (const auto &[key, value] : j["storage"].items()) {
            clan.storage_[std::stoul(key)] = value.get<int>();
        }
    }

    clan.invalidate_rank_cache();
}

// ClanRepository implementation

std::optional<ClanPtr> ClanRepository::find_by_name(const std::string_view name) const {
    build_caches();
    auto it = name_to_id_.find(std::string(name));
    return it != name_to_id_.end() ? find_by_id(it->second) : std::nullopt;
}

std::optional<ClanPtr> ClanRepository::find_by_abbreviation(const std::string_view abbr) const {
    build_caches();
    auto it = abbr_to_id_.find(std::string(abbr));
    return it != abbr_to_id_.end() ? find_by_id(it->second) : std::nullopt;
}

std::expected<void, std::string> ClanRepository::save_to_file(const std::filesystem::path &filepath) const {
    try {
        nlohmann::json j = nlohmann::json::array();
        for (const auto &clan : all()) {
            j.push_back(*clan);
        }

        std::ofstream file(filepath);
        if (!file) {
            return std::unexpected("Could not open file for writing");
        }

        file << j.dump(4);

        log(LogSeverity::Info, LVL_IMMORT, "Saved clan data to file: {}", filepath.string());
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(e.what());
    }
}

std::expected<void, std::string> ClanRepository::load_from_file(const std::filesystem::path &filepath) {
    try {
        std::ifstream file(filepath);
        if (!file) {
            return std::unexpected("Could not open file for reading");
        }

        nlohmann::json j;
        file >> j;

        clans_.clear();
        for (const auto &clan_json : j) {
            auto clan = std::make_shared<Clan>(0, "", "");
            from_json(clan_json, *clan);
            clans_[clan->id()] = clan;
        }

        caches_valid_ = false;
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(e.what());
    }
}

void ClanRepository::init_clans() {
    auto result = load();
    if (!result) {
        log(LogSeverity::Info, LVL_IMMORT, "JSON clan data not found, trying legacy format: {}", result.error());

        // Try to load legacy clan files
        std::filesystem::path clans_dir = "etc/clans";
        if (std::filesystem::exists(clans_dir)) {
            // Look for .clan files and load them
            int loaded_count = 0;
            for (const auto &entry : std::filesystem::directory_iterator(clans_dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".clan") {
                    std::string filename = entry.path().stem().string();
                    if (load_legacy(filename)) {
                        loaded_count++;
                        log(LogSeverity::Info, LVL_IMMORT, "Loaded legacy clan: {}", filename);
                    } else {
                        log(LogSeverity::Warn, LVL_IMMORT, "Failed to load legacy clan: {}", filename);
                    }
                }
            }

            if (loaded_count > 0) {
                log(LogSeverity::Info, LVL_IMMORT, "Loaded {} legacy clan(s). Consider converting to JSON format.",
                    loaded_count);

                // Optionally save the loaded clans in JSON format for future use
                auto save_result = save();
                if (save_result) {
                    log(LogSeverity::Info, LVL_IMMORT, "Converted legacy clans to JSON format");
                } else {
                    log(LogSeverity::Warn, LVL_IMMORT, "Failed to save converted clans: {}", save_result.error());
                }
            } else {
                log(LogSeverity::Warn, LVL_IMMORT, "No valid clan files found");
            }
        } else {
            log(LogSeverity::Warn, LVL_IMMORT, "Clans directory not found: {}", clans_dir.string());
        }
    }
}

bool ClanRepository::load_legacy(const std::string_view clan_num) {
    std::filesystem::path clan_file = std::filesystem::path("etc/clans") / (std::string(clan_num) + ".clan");

    std::ifstream file(clan_file);
    if (!file) {
        return false;
    }

    // Create a temporary clan object to populate
    ClanID clan_id = 0;
    std::string clan_name;
    std::string clan_abbr;
    std::string clan_motd;
    unsigned int dues = 0;
    unsigned int app_fee = 0;
    unsigned int min_app_level = 0;
    Money treasure;
    std::vector<ClanRank> ranks;
    std::vector<ClanMember> members;

    // Maps to build ranks and permissions
    std::map<int, std::string> rank_titles;
    std::map<int, std::vector<std::string>> rank_privileges;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
            continue;

        if (key == "number:") {
            iss >> clan_id;
        } else if (key == "name:") {
            std::string rest;
            std::getline(iss, rest);
            clan_name = rest.substr(1); // Remove leading space
        } else if (key == "abbr:") {
            std::string rest;
            std::getline(iss, rest);
            clan_abbr = rest.substr(1); // Remove leading space
        } else if (key == "motd:") {
            // Handle multi-line motd that ends with ~
            std::string motd_line;
            while (std::getline(file, motd_line)) {
                if (motd_line == "~") {
                    break;
                }
                if (!clan_motd.empty()) {
                    clan_motd += "\n";
                }
                clan_motd += motd_line;
            }
        } else if (key == "dues:") {
            iss >> dues;
        } else if (key == "appfee:") {
            iss >> app_fee;
        } else if (key == "applevel:") {
            iss >> min_app_level;
        } else if (key == "copper:") {
            int copper;
            iss >> copper;
            treasure[COPPER] = copper;
        } else if (key == "silver:") {
            int silver;
            iss >> silver;
            treasure[SILVER] = silver;
        } else if (key == "gold:") {
            int gold;
            iss >> gold;
            treasure[GOLD] = gold;
        } else if (key == "platinum:") {
            int platinum;
            iss >> platinum;
            treasure[PLATINUM] = platinum;
        } else if (key == "title:") {
            int rank_num;
            iss >> rank_num;
            std::string rest;
            std::getline(iss, rest);
            rank_titles[rank_num] = rest.substr(1); // Remove leading space
        } else if (key == "privilege:") {
            int rank_num;
            iss >> rank_num;
            std::string privilege;
            std::vector<std::string> privileges;
            while (iss >> privilege) {
                privileges.push_back(privilege);
            }
            rank_privileges[rank_num] = privileges;
        } else if (key == "member:") {
            std::string name;
            int rank_index;
            time_t join_time;
            iss >> name >> rank_index >> join_time;

            // Read optional alts
            std::vector<std::string> alts;
            std::string alt;
            while (iss >> alt) {
                alts.push_back(alt);
            }

            // Convert rank_index from 1-based to 0-based
            rank_index = rank_index > 0 ? rank_index - 1 : 0;

            members.emplace_back(name, rank_index, join_time, std::move(alts));
        }
    }

    // Build ranks from the collected data
    for (const auto &[rank_num, title] : rank_titles) {
        PermissionSet permissions;

        auto priv_it = rank_privileges.find(rank_num);
        if (priv_it != rank_privileges.end()) {
            for (const auto &priv : priv_it->second) {
                if (priv == "desc" || priv == "setdesc") {
                    permissions.set(static_cast<size_t>(ClanPermission::SET_DESCRIPTION));
                } else if (priv == "motd") {
                    permissions.set(static_cast<size_t>(ClanPermission::SET_MOTD));
                } else if (priv == "grant") {
                    permissions.set(static_cast<size_t>(ClanPermission::LEADER_OVERRIDE));
                } else if (priv == "ranks") {
                    permissions.set(static_cast<size_t>(ClanPermission::MANAGE_RANKS));
                } else if (priv == "title" || priv == "settitle") {
                    // Title setting is deprecated, ignore
                } else if (priv == "enroll") {
                    permissions.set(static_cast<size_t>(ClanPermission::INVITE_MEMBERS));
                } else if (priv == "expel") {
                    permissions.set(static_cast<size_t>(ClanPermission::KICK_MEMBERS));
                } else if (priv == "promote") {
                    permissions.set(static_cast<size_t>(ClanPermission::PROMOTE_MEMBERS));
                } else if (priv == "demote") {
                    permissions.set(static_cast<size_t>(ClanPermission::DEMOTE_MEMBERS));
                } else if (priv == "appfee" || priv == "setappfee") {
                    permissions.set(static_cast<size_t>(ClanPermission::SET_APP_FEES));
                } else if (priv == "applev" || priv == "setapplev") {
                    permissions.set(static_cast<size_t>(ClanPermission::SET_APP_LEVEL));
                } else if (priv == "dues" || priv == "setdues") {
                    permissions.set(static_cast<size_t>(ClanPermission::SET_DUES));
                } else if (priv == "deposit") {
                    permissions.set(static_cast<size_t>(ClanPermission::DEPOSIT_FUNDS));
                } else if (priv == "withdraw") {
                    permissions.set(static_cast<size_t>(ClanPermission::WITHDRAW_FUNDS));
                } else if (priv == "store") {
                    permissions.set(static_cast<size_t>(ClanPermission::STORE_ITEMS));
                } else if (priv == "retrieve") {
                    permissions.set(static_cast<size_t>(ClanPermission::RETRIEVE_ITEMS));
                } else if (priv == "alts" || priv == "setalt") {
                    permissions.set(static_cast<size_t>(ClanPermission::MANAGE_ALTS));
                } else if (priv == "chat") {
                    permissions.set(static_cast<size_t>(ClanPermission::CLAN_CHAT));
                }
            }
        }

        // Special case: if a rank has 'grant' privilege, give it LEADER_OVERRIDE
        // This handles legacy admin ranks that may be missing some newer permissions
        if (permissions.test(static_cast<size_t>(ClanPermission::LEADER_OVERRIDE))) {
            // Leader override allows access to all clan commands
        }

        ranks.emplace_back(title, permissions);
    }

    // Create the clan and add it to the repository
    auto clan = std::make_shared<Clan>(clan_id, clan_name, clan_abbr);

    // Set clan properties using admin methods
    clan->admin_set_dues(dues);
    clan->admin_set_app_fee(app_fee);
    clan->admin_set_min_application_level(min_app_level);
    if (auto result = clan->admin_add_treasure(treasure); !result) {
        log(LogSeverity::Warn, LVL_IMMORT, "Failed to add treasure to clan {}: {}", clan_name, result.error());
    }
    if (!clan_motd.empty()) {
        clan->admin_set_motd(clan_motd);
    }

    // Add ranks
    for (auto &rank : ranks) {
        if (auto result = clan->admin_add_rank(std::move(rank)); !result) {
            log(LogSeverity::Warn, LVL_IMMORT, "Failed to add rank to clan {}: {}", clan_name, result.error());
        }
    }

    // Add members
    for (const auto &member : members) {
        if (!clan->add_member_by_name(member.name, member.rank_index, member.join_time, member.alts)) {
            log(LogSeverity::Warn, LVL_IMMORT, "Failed to add member {} to clan {}", member.name, clan_name);
        }
    }

    // Add to repository
    clans_[clan_id] = clan;
    caches_valid_ = false;

    return true;
}

// Clan snoop management functions (string-based, safe against dangling pointers)
void add_clan_snoop(std::string_view char_name, ClanID clan_id) {
    if (char_name.empty())
        return;
    clan_snoop_table[clan_id].insert(std::string(char_name));
}

void remove_clan_snoop(std::string_view char_name, ClanID clan_id) {
    if (char_name.empty())
        return;
    auto it = clan_snoop_table.find(clan_id);
    if (it != clan_snoop_table.end()) {
        it->second.erase(std::string(char_name));
        if (it->second.empty()) {
            clan_snoop_table.erase(it);
        }
    }
}

void remove_all_clan_snoops(std::string_view char_name) {
    if (char_name.empty())
        return;
    std::string name_str(char_name);
    for (auto it = clan_snoop_table.begin(); it != clan_snoop_table.end();) {
        it->second.erase(name_str);
        if (it->second.empty()) {
            it = clan_snoop_table.erase(it);
        } else {
            ++it;
        }
    }
}

bool is_snooping_clan(std::string_view char_name, ClanID clan_id) {
    if (char_name.empty())
        return false;
    auto it = clan_snoop_table.find(clan_id);
    if (it != clan_snoop_table.end()) {
        return it->second.count(std::string(char_name)) > 0;
    }
    return false;
}

std::vector<ClanID> get_snooped_clans(std::string_view char_name) {
    std::vector<ClanID> result;
    if (char_name.empty())
        return result;

    std::string name_str(char_name);
    for (const auto &[clan_id, snoops] : clan_snoop_table) {
        if (snoops.count(name_str) > 0) {
            result.push_back(clan_id);
        }
    }
    return result;
}

// Legacy interface wrappers (extract name from CharData)
void add_clan_snoop(CharData *ch, ClanID clan_id) {
    if (!ch || !ch->player.short_descr)
        return;
    add_clan_snoop(std::string_view(ch->player.short_descr), clan_id);
}

void remove_clan_snoop(CharData *ch, ClanID clan_id) {
    if (!ch || !ch->player.short_descr)
        return;
    remove_clan_snoop(std::string_view(ch->player.short_descr), clan_id);
}

void remove_all_clan_snoops(CharData *ch) {
    if (!ch || !ch->player.short_descr)
        return;
    remove_all_clan_snoops(std::string_view(ch->player.short_descr));
}

bool is_snooping_clan(CharData *ch, ClanID clan_id) {
    if (!ch || !ch->player.short_descr)
        return false;
    return is_snooping_clan(std::string_view(ch->player.short_descr), clan_id);
}

std::vector<ClanID> get_snooped_clans(CharData *ch) {
    if (!ch || !ch->player.short_descr)
        return {};
    return get_snooped_clans(std::string_view(ch->player.short_descr));
}

// Function for use with function registry - checks clan permission or god status
bool has_clan_permission_or_god(const CharData *ch, ClanPermission permission) {
    // Null character check
    if (!ch) {
        return false;
    }

    // Gods have all permissions
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        return true;
    }
    return has_clan_permission(ch, permission);
}

// Modern C++23 permission checking implementation
namespace clan_permissions {

PermissionResult check_permission(const CharData *ch, ClanPermission permission) {
    using enum ClanPermission;

    // Null character check
    if (!ch) {
        return std::unexpected(PermissionError{"Invalid character.", permission});
    }

    // Check if character has a clan
    auto clan = get_clan(ch);
    if (!clan) {
        return std::unexpected(PermissionError{"You are not a member of any clan.", permission});
    }

    // Get character's rank
    auto rank = get_clan_rank(ch);
    if (!rank) {
        return std::unexpected(PermissionError{"Unable to determine your clan rank.", permission});
    }

    // Check permission
    if (!rank->has_permission(permission)) {
        auto permission_name = std::string(magic_enum::enum_name(permission));
        return std::unexpected(PermissionError{fmt::format("You need the '{}' permission to do that.", permission_name),
                                               permission, true});
    }

    return {}; // Success
}

PermissionResult check_clan_member(const CharData *ch) {
    auto clan = get_clan(ch);
    if (!clan) {
        return std::unexpected(PermissionError{"You are not a member of any clan."});
    }

    auto member = get_clan_member(ch);
    if (!member) {
        return std::unexpected(PermissionError{"Unable to verify your clan membership."});
    }

    return {}; // Success
}

// Simple permission checking helpers
bool check_god_override(const CharData *ch) { return GET_LEVEL(ch) >= LVL_IMMORT; }

// Enhanced permission checking with detailed error messages
std::string get_permission_error_message(ClanPermission permission) {
    auto permission_name = std::string(magic_enum::enum_name(permission));
    return fmt::format("You need the '{}' permission to do that.", permission_name);
}

// Command wrapper functions (simpler than templates)
bool execute_with_clan_permission(CharData *ch, ClanPermission permission, std::function<void()> command_func) {
    // Gods bypass all permission checks
    if (check_god_override(ch)) {
        command_func();
        return true;
    }

    // Check permission
    auto result = check_permission(ch, permission);
    if (!result) {
        char_printf(ch, "{}", result.error().reason);
        return false;
    }

    // Execute command
    command_func();
    return true;
}

bool execute_with_clan_membership(CharData *ch, std::function<void()> command_func) {
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

} // namespace clan_permissions

// Legacy permission conversion for existing clan files
namespace legacy_conversion {

// Convert old ClanPrivilege values to new ClanPermission values
std::optional<ClanPermission> convert_legacy_privilege(int legacy_value) {
    switch (legacy_value) {
    case 1:
        return ClanPermission::SET_DESCRIPTION; // Description
    case 2:
        return ClanPermission::SET_MOTD; // Motd
    case 3:
        return ClanPermission::LEADER_OVERRIDE; // Grant (legacy admin)
    case 4:
        return ClanPermission::MANAGE_RANKS; // Ranks
    case 5:
        return ClanPermission::NONE; // Title (deprecated)
    case 6:
        return ClanPermission::INVITE_MEMBERS; // Enroll
    case 7:
        return ClanPermission::KICK_MEMBERS; // Expel
    case 8:
        return ClanPermission::PROMOTE_MEMBERS; // Promote
    case 9:
        return ClanPermission::DEMOTE_MEMBERS; // Demote
    case 10:
        return ClanPermission::SET_APP_FEES; // App_Fees
    case 11:
        return ClanPermission::SET_APP_LEVEL; // App_Level
    case 12:
        return ClanPermission::SET_DUES; // Dues
    case 13:
        return ClanPermission::DEPOSIT_FUNDS; // Deposit
    case 14:
        return ClanPermission::WITHDRAW_FUNDS; // Withdraw
    case 15:
        return ClanPermission::STORE_ITEMS; // Store
    case 16:
        return ClanPermission::RETRIEVE_ITEMS; // Retrieve
    case 17:
        return ClanPermission::MANAGE_ALTS; // Alts
    case 18:
        return ClanPermission::CLAN_CHAT; // Chat
    default:
        return std::nullopt;
    }
}

// Convert legacy bitset to new permission set
PermissionSet convert_legacy_permissions(const std::bitset<64> &legacy_bits) {
    PermissionSet new_permissions;

    // Convert each bit position
    for (size_t i = 0; i < 64 && i < NUM_PERMISSIONS; ++i) {
        if (legacy_bits.test(i)) {
            auto new_perm = convert_legacy_privilege(static_cast<int>(i));
            if (new_perm) {
                new_permissions.set(static_cast<size_t>(*new_perm));
            }
        }
    }

    return new_permissions;
}

} // namespace legacy_conversion

// Security and validation utilities implementation
namespace clan_security {

ValidationResult validate_clan_name(std::string_view name) {
    if (name.empty()) {
        return std::unexpected(ValidationError{"Clan name cannot be empty.", "name", std::string(name)});
    }
    if (name.length() > MAX_CLAN_NAME_LENGTH) {
        return std::unexpected(ValidationError{
            fmt::format("Clan name too long (max {} characters).", MAX_CLAN_NAME_LENGTH), "name", std::string(name)});
    }
    if (contains_unsafe_characters(name)) {
        return std::unexpected(ValidationError{"Clan name contains invalid characters.", "name", std::string(name)});
    }
    return {};
}

ValidationResult validate_clan_abbreviation(std::string_view abbr) {
    if (abbr.empty()) {
        return std::unexpected(
            ValidationError{"Clan abbreviation cannot be empty.", "abbreviation", std::string(abbr)});
    }
    if (abbr.length() > MAX_CLAN_ABBR_LENGTH) {
        return std::unexpected(
            ValidationError{fmt::format("Clan abbreviation too long (max {} characters).", MAX_CLAN_ABBR_LENGTH),
                            "abbreviation", std::string(abbr)});
    }
    if (contains_unsafe_characters(abbr)) {
        return std::unexpected(
            ValidationError{"Clan abbreviation contains invalid characters.", "abbreviation", std::string(abbr)});
    }
    return {};
}

ValidationResult validate_clan_description(std::string_view desc) {
    if (desc.length() > MAX_CLAN_DESCRIPTION_LENGTH) {
        return std::unexpected(
            ValidationError{fmt::format("Clan description too long (max {} characters).", MAX_CLAN_DESCRIPTION_LENGTH),
                            "description", std::string(desc)});
    }
    return {}; // Descriptions can be empty and have more flexible character restrictions
}

ValidationResult validate_player_name(std::string_view name) {
    if (name.empty()) {
        return std::unexpected(ValidationError{"Player name cannot be empty.", "player_name", std::string(name)});
    }
    if (name.length() > 20) { // Standard player name limit
        return std::unexpected(ValidationError{"Player name too long.", "player_name", std::string(name)});
    }
    // Basic character validation for player names
    for (char c : name) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            return std::unexpected(ValidationError{"Player name contains invalid characters (alphanumeric, _, - only).",
                                                   "player_name", std::string(name)});
        }
    }
    return {};
}

CharData *find_player_safe(std::string_view name) {
    // Validate input first
    auto validation = validate_player_name(name);
    if (!validation) {
        return nullptr;
    }

    // Create a safe copy for the legacy API
    std::string name_copy(name);
    return find_char_in_world(find_by_name(name_copy.data()));
}

CharData *find_clan_member_safe(std::string_view name) {
    // Same implementation as find_player_safe for now
    // Could be extended with clan-specific validation
    return find_player_safe(name);
}

bool check_member_limit(const Clan &clan) { return clan.member_count() < MAX_CLAN_MEMBERS; }

bool check_rank_limit(const Clan &clan) { return clan.rank_count() < MAX_CLAN_RANKS; }

bool check_storage_limit(const Clan &clan) { return clan.storage().size() < MAX_CLAN_STORAGE_ITEMS; }

std::expected<Money, std::string> safe_add_money(const Money &base, const Money &addition) {
    Money result = base;

    // Check for overflow in each denomination
    for (int denom = COPPER; denom <= PLATINUM; ++denom) {
        if (base[denom] > 0 && addition[denom] > 0) {
            // Check if addition would cause overflow
            if (base[denom] > std::numeric_limits<int>::max() - addition[denom]) {
                return std::unexpected("Money addition would cause overflow.");
            }
        }
        result[denom] = base[denom] + addition[denom];
    }

    return result;
}

std::expected<Money, std::string> safe_subtract_money(const Money &base, const Money &subtraction) {
    Money result = base;

    // Check for underflow in each denomination
    for (int denom = COPPER; denom <= PLATINUM; ++denom) {
        if (base[denom] < subtraction[denom]) {
            return std::unexpected("Insufficient coins for withdrawal.");
        }
        result[denom] = base[denom] - subtraction[denom];
    }

    return result;
}

std::expected<int, std::string> safe_add_quantity(int base, int addition) {
    if (base > 0 && addition > 0) {
        if (base > std::numeric_limits<int>::max() - addition) {
            return std::unexpected("Quantity addition would cause overflow.");
        }
    }
    return base + addition;
}

bool contains_unsafe_characters(std::string_view input) {
    // Basic safety check - reject null bytes and some dangerous chars
    for (char c : input) {
        if (c == '\0' || c == '<' || c == '>' || c == '&' || c == '"' || c == '\'') {
            return true;
        }
    }
    return false;
}

} // namespace clan_security

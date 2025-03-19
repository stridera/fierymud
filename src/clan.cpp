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
#include "money.hpp"
#include "screen.hpp"
#include "utils.hpp"

// Clan class implementation

ClanMembershipPtr Clan::add_member(CharacterPtr character, ClanRank rank) {
    auto membership = std::make_shared<ClanMembership>(shared_from_this(), character, rank);
    memberships_.push_back(membership);
    character->player_specials->clan_memberships.push_back(std::move(membership));
    return membership;
}

void Clan::remove_member(const CharacterPtr &character) {
    // Remove from the clan's membership list
    auto it = std::ranges::find_if(memberships_,
                                   [&character](const ClanMembershipPtr &m) { return m->character() == character; });

    if (it != memberships_.end()) {
        memberships_.erase(it);
    }

    // Remove from the character's clan memberships
    auto membership_it =
        std::ranges::find_if(character->player_specials->clan_memberships,
                             [&character](const ClanMembershipPtr &m) { return m->character() == character; });
    if (membership_it != character->player_specials->clan_memberships.end()) {
        character->player_specials->clan_memberships.erase(membership_it);
    }
}

std::optional<ClanMembershipPtr> Clan::get_membership(const CharacterPtr &character) const {
    auto it = std::ranges::find_if(memberships_,
                                   [&character](const ClanMembershipPtr &m) { return m->character() == character; });

    return it != memberships_.end() ? std::optional{*it} : std::nullopt;
}

std::vector<CharacterPtr> Clan::get_members_by_rank(const ClanRank &rank) const {
    std::vector<CharacterPtr> result;
    for (const auto &membership : memberships_) {
        if (membership->rank() == rank) {
            if (auto character = membership->character()) {
                result.push_back(character);
            }
        }
    }
    return result;
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
        for (auto &membership : memberships_) {
            if (auto character = membership->character()) {
                if (character != skip && !PRF_FLAGGED(character.get(), PRF_NOCLANCOMM)) {
                    char_printf(character.get(),
                                AFMAG
                                "[[ "
                                "{}" AFMAG " ]]\n" ANRM,
                                messg);
                }
            }
        }
    }
}

// ClanMembership implementation

std::expected<void, std::string_view> ClanMembership::update_clan_name(std::string new_name) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr) {
        return std::unexpected(AccessError::ClanNotFound);
    }

    if (character_ptr->player.level < LVL_IMMORT) {
        return std::unexpected(AccessError::PermissionDenied);
    }

    clan_ptr->name_ = std::move(new_name);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_abbreviation(std::string new_abbreviation) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (character_ptr->player.level < LVL_IMMORT)
        return std::unexpected(AccessError::PermissionDenied);
    if (ansi_strlen(new_abbreviation) > Clan::MAX_CLAN_ABBR_LEN) {
        std::string_view error_msg =
            fmt::format("Clan abbreviations may be at most {} characters in length.", Clan::MAX_CLAN_ABBR_LEN);
        return std::unexpected(error_msg);
    }
    if (clan_ptr->find_clan(new_abbreviation))
        return std::unexpected("A clan already exists with that abbreviation.");

    clan_ptr->abbreviation_ = std::move(new_abbreviation);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_description(std::string new_description) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Description))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->description_ = std::move(new_description);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_motd(std::string new_motd) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Motd))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->motd_ = std::move(new_motd);
    return {};
}

std::expected<void, std::string_view> ClanMembership::update_clan_dues(unsigned int new_dues) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

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
    if (!has_permission(ClanPrivilege::App_Lev))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->min_application_level_ = new_level;
    return {};
}

std::expected<void, std::string_view> ClanMembership::add_clan_treasure(Money money) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->add_treasure(money);
    return {};
}
std::expected<void, std::string_view> ClanMembership::subtract_clan_treasure(Money money) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Treasure))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->subtract_treasure(money);
    return {};
}
std::expected<void, std::string_view> ClanMembership::add_clan_storage_item(ObjectId id, int amount) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Storage))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->add_storage_item(id, amount);
    return {};
}
std::expected<void, std::string_view> ClanMembership::remove_clan_storage_item(ObjectId id, int amount) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Storage))
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
std::expected<void, std::string_view> ClanMembership::update_clan_member(ClanMembershipPtr new_member) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();

    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Enroll))
        return std::unexpected(AccessError::PermissionDenied);

    clan_ptr->memberships_.push_back(std::move(new_member));
    return {};
}
std::expected<void, std::string_view> ClanMembership::update_clan_member_rank(ClanMembershipPtr new_member,
                                                                              ClanRank new_rank) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();
    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Ranks))
        return std::unexpected(AccessError::PermissionDenied);
    auto it = std::ranges::find_if(clan_ptr->memberships_,
                                   [&new_member](const ClanMembershipPtr &m) { return m == new_member; });
    if (it == clan_ptr->memberships_.end())
        return std::unexpected(AccessError::InvalidOperation);
    if (new_member->rank() == new_rank)
        return std::unexpected(AccessError::InvalidOperation);
}

std::expected<void, std::string_view> ClanMembership::update_clan_member_alts(ClanMembershipPtr new_member,
                                                                              std::vector<std::string> alts) {
    auto clan_ptr = Clan_.lock();
    auto character_ptr = character_.lock();
    if (!clan_ptr || !character_ptr)
        return std::unexpected(AccessError::ClanNotFound);
    if (!has_permission(ClanPrivilege::Alts))
        return std::unexpected(AccessError::PermissionDenied);
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
                       {"ranks", clan.ranks()}};

    // Add storage as a separate object
    j["storage"] = nlohmann::json::object();
    for (const auto &[obj_id, amount] : clan.storage()) {
        j["storage"][std::to_string(obj_id)] = amount;
    }
}

void from_json(const nlohmann::json &j, Clan &clan) {
    // We'll implement this differently since Clan has private setters
    // This function will be used by ClanRepository::load_from_file
    ClanId id = j["id"].get<ClanId>();
    std::string name = j["name"].get<std::string>();
    std::string abbreviation = j["abbreviation"].get<std::string>();

    // Create new Clan object at the provided address
    new (&clan) Clan(id, std::move(name), std::move(abbreviation));

    // Set basic properties using private methods
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

    // Load ranks
    if (j.contains("ranks") && j["ranks"].is_array()) {
        clan.ranks_.clear();
        for (const auto &j_rank : j["ranks"]) {
            clan.ranks_.emplace_back(std::move(j_rank));
        }
    }

    // Load storage items
    if (j.contains("storage") && j["storage"].is_object()) {
        clan.storage_.clear();
        for (const auto &[obj_id_str, amount] : j["storage"].items()) {
            ObjectId obj_id = std::stoul(obj_id_str);
            clan.add_storage_item(obj_id, amount.get<int>());
        }
    }
}

// ClanRepository implementation

std::optional<ClanPtr> ClanRepository::find_by_name(const std::string_view name) const {
    auto it =
        std::find_if(Clans_.begin(), Clans_.end(), [&name](const auto &pair) { return pair.second->name() == name; });
    return it != Clans_.end() ? std::optional{it->second} : std::nullopt;
}

std::optional<ClanPtr> ClanRepository::find_by_abbreviation(const std::string_view abbr) const {
    auto it = std::find_if(Clans_.begin(), Clans_.end(),
                           [&abbr](const auto &pair) { return pair.second->abbreviation() == abbr; });
    return it != Clans_.end() ? std::optional{it->second} : std::nullopt;
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
        Clans_.clear();

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

// TODO: Move to config
constexpr std::string_view CLAN_PREFIX = "etc/clans"; /* clan directory		*/
constexpr std::string_view CLAN_SUFFIX = ".clan";     /* clan file suffix		*/
bool ClanRepository::load_legacy(const std::string_view clan_num) {
    std::ifstream file(fmt::format("{}/{}{}", CLAN_PREFIX, clan_num, CLAN_SUFFIX));
    if (!file.is_open()) {
        log("Couldn't open clan file '{}'", fmt::format("{}/{}{}", CLAN_PREFIX, clan_num, CLAN_SUFFIX));
        return false;
    }

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

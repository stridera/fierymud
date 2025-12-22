#include "social_queries.hpp"
#include "connection_pool.hpp"
#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>

namespace {

/**
 * Convert string to lowercase for case-insensitive comparison.
 */
std::string to_lower(std::string_view str) {
    std::string result{str};
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * Get optional string from pqxx field.
 */
std::optional<std::string> get_optional_string(const pqxx::field& field) {
    if (field.is_null()) {
        return std::nullopt;
    }
    return field.as<std::string>();
}

/**
 * Parse a single Social from a database row.
 */
Social parse_social_row(const pqxx::row& row) {
    Social social;

    social.id = row["id"].as<int>();
    social.name = row["name"].as<std::string>();
    social.hide = row["hide"].as<bool>();
    social.min_victim_position = SocialQueries::parse_position(
        row["min_victim_position"].as<std::string>());

    social.char_no_arg = get_optional_string(row["char_no_arg"]);
    social.others_no_arg = get_optional_string(row["others_no_arg"]);
    social.char_found = get_optional_string(row["char_found"]);
    social.others_found = get_optional_string(row["others_found"]);
    social.vict_found = get_optional_string(row["vict_found"]);
    social.not_found = get_optional_string(row["not_found"]);
    social.char_auto = get_optional_string(row["char_auto"]);
    social.others_auto = get_optional_string(row["others_auto"]);

    return social;
}

} // anonymous namespace

namespace SocialQueries {

SocialPosition parse_position(std::string_view str) {
    static const std::unordered_map<std::string, SocialPosition> position_map = {
        {"DEAD", SocialPosition::Dead},
        {"MORTALLY_WOUNDED", SocialPosition::MortallyWounded},
        {"INCAPACITATED", SocialPosition::Incapacitated},
        {"STUNNED", SocialPosition::Stunned},
        {"SLEEPING", SocialPosition::Sleeping},
        {"RESTING", SocialPosition::Resting},
        {"SITTING", SocialPosition::Sitting},
        {"FIGHTING", SocialPosition::Fighting},
        {"STANDING", SocialPosition::Standing},
        {"FLYING", SocialPosition::Flying}
    };

    std::string upper{str};
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    auto it = position_map.find(upper);
    if (it != position_map.end()) {
        return it->second;
    }

    // Default to standing if unknown
    spdlog::warn("Unknown position value: '{}', defaulting to STANDING", str);
    return SocialPosition::Standing;
}

std::string_view position_to_string(SocialPosition pos) {
    switch (pos) {
        case SocialPosition::Dead: return "DEAD";
        case SocialPosition::MortallyWounded: return "MORTALLY_WOUNDED";
        case SocialPosition::Incapacitated: return "INCAPACITATED";
        case SocialPosition::Stunned: return "STUNNED";
        case SocialPosition::Sleeping: return "SLEEPING";
        case SocialPosition::Resting: return "RESTING";
        case SocialPosition::Sitting: return "SITTING";
        case SocialPosition::Fighting: return "FIGHTING";
        case SocialPosition::Standing: return "STANDING";
        case SocialPosition::Flying: return "FLYING";
    }
    return "STANDING";
}

Result<std::vector<Social>> load_all_socials(pqxx::work& txn) {
    try {
        constexpr auto query = R"(
            SELECT id, name, hide, min_victim_position,
                   char_no_arg, others_no_arg,
                   char_found, others_found, vict_found, not_found,
                   char_auto, others_auto
            FROM "Social"
            ORDER BY name
        )";

        auto result = txn.exec(query);

        std::vector<Social> socials;
        socials.reserve(result.size());

        for (const auto& row : result) {
            socials.push_back(parse_social_row(row));
        }

        spdlog::info("Loaded {} socials from database", socials.size());
        return socials;

    } catch (const pqxx::sql_error& e) {
        spdlog::error("SQL error loading socials: {} (query: {})", e.what(), e.query());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load socials: {}", e.what())});
    } catch (const std::exception& e) {
        spdlog::error("Error loading socials: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load socials: {}", e.what())});
    }
}

Result<std::vector<Social>> load_all_socials(pqxx::nontransaction& txn) {
    try {
        constexpr auto query = R"(
            SELECT id, name, hide, min_victim_position,
                   char_no_arg, others_no_arg,
                   char_found, others_found, vict_found, not_found,
                   char_auto, others_auto
            FROM "Social"
            ORDER BY name
        )";

        auto result = txn.exec(query);

        std::vector<Social> socials;
        socials.reserve(result.size());

        for (const auto& row : result) {
            socials.push_back(parse_social_row(row));
        }

        spdlog::info("Loaded {} socials from database (read-only)", socials.size());
        return socials;

    } catch (const pqxx::sql_error& e) {
        spdlog::error("SQL error loading socials: {} (query: {})", e.what(), e.query());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load socials: {}", e.what())});
    } catch (const std::exception& e) {
        spdlog::error("Error loading socials: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load socials: {}", e.what())});
    }
}

Result<std::optional<Social>> load_social_by_name(pqxx::work& txn, std::string_view name) {
    try {
        constexpr auto query = R"(
            SELECT id, name, hide, "minVictimPosition",
                   "charNoArg", "othersNoArg",
                   "charFound", "othersFound", "victFound", "notFound",
                   "charAuto", "othersAuto"
            FROM "Social"
            WHERE LOWER(name) = LOWER($1)
        )";

        auto result = txn.exec_params(query, std::string{name});

        if (result.empty()) {
            return std::optional<Social>{std::nullopt};
        }

        return std::optional<Social>{parse_social_row(result[0])};

    } catch (const pqxx::sql_error& e) {
        spdlog::error("SQL error loading social '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load social: {}", e.what())});
    } catch (const std::exception& e) {
        spdlog::error("Error loading social '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load social: {}", e.what())});
    }
}

Result<std::optional<Social>> load_social_by_id(pqxx::work& txn, int id) {
    try {
        constexpr auto query = R"(
            SELECT id, name, hide, "minVictimPosition",
                   "charNoArg", "othersNoArg",
                   "charFound", "othersFound", "victFound", "notFound",
                   "charAuto", "othersAuto"
            FROM "Social"
            WHERE id = $1
        )";

        auto result = txn.exec_params(query, id);

        if (result.empty()) {
            return std::optional<Social>{std::nullopt};
        }

        return std::optional<Social>{parse_social_row(result[0])};

    } catch (const pqxx::sql_error& e) {
        spdlog::error("SQL error loading social ID {}: {}", id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load social: {}", e.what())});
    } catch (const std::exception& e) {
        spdlog::error("Error loading social ID {}: {}", id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load social: {}", e.what())});
    }
}

int get_social_count(pqxx::work& txn) {
    try {
        auto result = txn.exec("SELECT COUNT(*) FROM \"Social\"");
        return result[0][0].as<int>();
    } catch (const std::exception& e) {
        spdlog::error("Error getting social count: {}", e.what());
        return 0;
    }
}

Result<std::vector<Social>> search_socials(pqxx::work& txn, std::string_view pattern) {
    try {
        constexpr auto query = R"(
            SELECT id, name, hide, "minVictimPosition",
                   "charNoArg", "othersNoArg",
                   "charFound", "othersFound", "victFound", "notFound",
                   "charAuto", "othersAuto"
            FROM "Social"
            WHERE name ILIKE $1
            ORDER BY name
        )";

        auto result = txn.exec_params(query, std::string{pattern});

        std::vector<Social> socials;
        socials.reserve(result.size());

        for (const auto& row : result) {
            socials.push_back(parse_social_row(row));
        }

        return socials;

    } catch (const pqxx::sql_error& e) {
        spdlog::error("SQL error searching socials: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to search socials: {}", e.what())});
    } catch (const std::exception& e) {
        spdlog::error("Error searching socials: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to search socials: {}", e.what())});
    }
}

} // namespace SocialQueries

// SocialCache implementation

SocialCache& SocialCache::instance() {
    static SocialCache instance;
    return instance;
}

Result<void> SocialCache::load_from_database() {
    try {
        auto result = ConnectionPool::instance().execute_read_only(
            [](pqxx::nontransaction& txn) {
                return SocialQueries::load_all_socials(txn);
            });

        if (!result) {
            return std::unexpected(result.error());
        }

        socials_.clear();
        for (auto& social : *result) {
            std::string key = to_lower(social.name);
            socials_[key] = std::move(social);
        }

        loaded_ = true;
        spdlog::info("SocialCache loaded {} socials", socials_.size());
        return {};

    } catch (const std::exception& e) {
        spdlog::error("Failed to load social cache: {}", e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
                         fmt::format("Failed to load social cache: {}", e.what())});
    }
}

const Social* SocialCache::get(std::string_view name) const {
    std::string key = to_lower(name);
    auto it = socials_.find(key);
    if (it != socials_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string_view> SocialCache::find_by_prefix(std::string_view prefix) const {
    std::vector<std::string_view> matches;
    std::string lower_prefix = to_lower(prefix);

    for (const auto& [name, social] : socials_) {
        if (name.starts_with(lower_prefix)) {
            matches.push_back(name);
        }
    }

    // Sort by length (shorter = better match) then alphabetically
    std::sort(matches.begin(), matches.end(),
              [](std::string_view a, std::string_view b) {
                  if (a.length() != b.length()) {
                      return a.length() < b.length();
                  }
                  return a < b;
              });

    return matches;
}

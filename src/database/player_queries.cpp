#include "database/player_queries.hpp"
#include "core/logging.hpp"
#include <fmt/format.h>

namespace PlayerQueries {

// Level at which characters become immortals
constexpr int kImmortalLevel = 100;

Result<std::unique_ptr<Player>> load_player_by_name(pqxx::work& txn, std::string_view name) {
    auto logger = Log::database();
    logger->debug("Loading player '{}' from database", name);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, name, level, alignment,
                   strength, intelligence, wisdom, dexterity, constitution, charisma, luck,
                   hit_points, hit_points_max, movement, movement_max,
                   wealth, bank_wealth,
                   password_hash, race, gender, player_class,
                   height, weight, base_height, base_weight, base_size, current_size,
                   hit_roll, damage_roll, armor_class,
                   description, title
            FROM "Characters"
            WHERE LOWER(name) = LOWER($1)
            LIMIT 1
        )", std::string(name));

        if (result.empty()) {
            logger->debug("Player '{}' not found in database", name);
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Player '{}' not found", name)});
        }

        const auto& row = result[0];

        // Extract basic character data
        std::string player_id = row["id"].as<std::string>();
        std::string player_name = row["name"].as<std::string>();
        int level = row["level"].as<int>(1);

        // Create player using factory method
        // Note: Player creation might need EntityId - for now using a placeholder
        // Players don't use zone-based IDs like rooms/mobs
        EntityId player_entity_id(0, 0);  // Players might need special ID handling

        auto player_result = Player::create(player_entity_id, player_name);
        if (!player_result) {
            return std::unexpected(player_result.error());
        }

        auto player = std::move(*player_result);

        // Set the database ID for persistence operations (mail, saving, etc.)
        player->set_database_id(player_id);

        // Stats - set level FIRST (before set_class, which needs level for spell slots)
        auto& stats = player->stats();
        stats.level = level;

        // Set god level for immortals (level 100+)
        // god_level = level - 99, so level 100 = god_level 1, level 105 = god_level 6
        if (level >= kImmortalLevel) {
            player->set_god_level(level - (kImmortalLevel - 1));
        }

        // Set properties from database
        // Race and class (after level is set, since set_class initializes spell slots)
        if (!row["race"].is_null()) {
            player->set_race(row["race"].as<std::string>());
        }
        if (!row["player_class"].is_null()) {
            player->set_class(row["player_class"].as<std::string>());
        }
        if (!row["gender"].is_null()) {
            player->set_gender(row["gender"].as<std::string>());
        }

        // Size - use current_size if available, otherwise base_size
        if (!row["current_size"].is_null()) {
            player->set_size(row["current_size"].as<std::string>());
        } else if (!row["base_size"].is_null()) {
            player->set_size(row["base_size"].as<std::string>());
        }

        // Remaining stats
        stats.alignment = row["alignment"].as<int>(0);
        stats.strength = row["strength"].as<int>(10);
        stats.intelligence = row["intelligence"].as<int>(10);
        stats.wisdom = row["wisdom"].as<int>(10);
        stats.dexterity = row["dexterity"].as<int>(10);
        stats.constitution = row["constitution"].as<int>(10);
        stats.charisma = row["charisma"].as<int>(10);
        // Note: luck is in database but not in Stats struct - skipped
        stats.hit_points = row["hit_points"].as<int>(100);
        stats.max_hit_points = row["hit_points_max"].as<int>(100);
        stats.movement = row["movement"].as<int>(100);
        stats.max_movement = row["movement_max"].as<int>(100);
        // Convert legacy DB stats to new combat system
        stats.accuracy = row["hit_roll"].as<int>(0);
        stats.attack_power = row["damage_roll"].as<int>(0);
        stats.armor_rating = std::max(0, 100 - row["armor_class"].as<int>(100));

        // Currency - wealth is already stored in copper
        stats.gold = row["wealth"].as<long>(0);

        // Description and title
        if (!row["description"].is_null()) {
            player->set_description(row["description"].as<std::string>());
        }
        if (!row["title"].is_null()) {
            player->set_title(row["title"].as<std::string>());
        }

        logger->debug("Loaded player '{}' (id: {}, level: {}, class: {}, race: {}) from database",
                    player_name, player_id, level,
                    row["player_class"].is_null() ? "Unknown" : row["player_class"].as<std::string>(),
                    row["race"].is_null() ? "Unknown" : row["race"].as<std::string>());

        return player;

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading player '{}': {}", name, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load player: {}", e.what())});
    }
}

Result<std::unique_ptr<Player>> load_player_by_id(pqxx::work& txn, std::string_view player_id) {
    auto logger = Log::database();
    logger->debug("Loading player by ID '{}' from database", player_id);

    try {
        auto result = txn.exec_params(R"(
            SELECT id, name, level, alignment,
                   strength, intelligence, wisdom, dexterity, constitution, charisma, luck,
                   hit_points, hit_points_max, movement, movement_max,
                   wealth, bank_wealth,
                   password_hash, race, gender, player_class
            FROM "Characters"
            WHERE id = $1
            LIMIT 1
        )", std::string(player_id));

        if (result.empty()) {
            return std::unexpected(Error{ErrorCode::NotFound,
                fmt::format("Player ID '{}' not found", player_id)});
        }

        const auto& row = result[0];
        std::string player_name = row["name"].as<std::string>();

        // Reuse load_by_name logic
        return load_player_by_name(txn, player_name);

    } catch (const pqxx::sql_error& e) {
        logger->error("SQL error loading player by ID '{}': {}", player_id, e.what());
        return std::unexpected(Error{ErrorCode::InternalError,
            fmt::format("Failed to load player: {}", e.what())});
    }
}

bool player_exists(pqxx::work& txn, std::string_view name) {
    try {
        auto result = txn.exec_params(R"(
            SELECT 1 FROM "Characters"
            WHERE LOWER(name) = LOWER($1)
            LIMIT 1
        )", std::string(name));

        return !result.empty();

    } catch (const pqxx::sql_error& e) {
        Log::database()->error("SQL error checking player existence '{}': {}", name, e.what());
        return false;
    }
}

int get_player_count(pqxx::work& txn) {
    try {
        auto result = txn.exec(R"(
            SELECT COUNT(*) as count FROM "Characters"
        )");

        if (!result.empty()) {
            return result[0]["count"].as<int>(0);
        }

        return 0;

    } catch (const pqxx::sql_error& e) {
        Log::database()->error("SQL error getting player count: {}", e.what());
        return 0;
    }
}

} // namespace PlayerQueries

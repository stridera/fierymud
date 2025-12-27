#include "persistence_manager.hpp"

#include "../core/logging.hpp"
#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "../core/object.hpp"
#include "../text/string_utils.hpp"
#include "mud_server.hpp"
#include "../database/connection_pool.hpp"
#include "../database/player_queries.hpp"
#include "../database/world_queries.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>

PersistenceManager& PersistenceManager::instance() {
    static PersistenceManager instance;
    return instance;
}

Result<void> PersistenceManager::initialize(const ServerConfig &config) {
    config_ = &config;
    Log::info("PersistenceManager initialized (placeholder)");
    return Success();
}

Result<void> PersistenceManager::save_all_players() {
    Log::debug("PersistenceManager save_all_players (placeholder)");
    return Success();
}

Result<void> PersistenceManager::backup_data() {
    Log::info("PersistenceManager backup_data (placeholder)");
    return Success();
}

Result<void> PersistenceManager::save_player(const Player& player) {
    if (!config_) {
        return std::unexpected(Errors::InvalidState("PersistenceManager not initialized"));
    }

    try {
        // Create player data directory if it doesn't exist
        std::string player_dir = config_->player_directory;
        if (!std::filesystem::exists(player_dir)) {
            std::filesystem::create_directories(player_dir);
        }

        // Convert player name to lowercase for consistent file naming
        std::string player_name = to_lowercase(player.name());

        // Save to JSON file (backup/debugging purposes)
        std::string filename = player_dir + "/" + player_name + ".json";

        nlohmann::json player_data = player.to_json();
        player_data["saved_at"] = std::chrono::system_clock::now().time_since_epoch().count();

        std::ofstream file(filename);
        if (!file.is_open()) {
            return std::unexpected(Errors::FileSystem("Cannot open player file for writing: " + filename));
        }

        file << player_data.dump(2); // Pretty-print with 2-space indentation
        file.close();

        Log::debug("Saved player {} to JSON file {}", player.name(), filename);

        // Save items to database
        std::string char_id{player.database_id()};
        if (!char_id.empty()) {
            // Build CharacterItemData from inventory and equipment
            std::vector<WorldQueries::CharacterItemData> items_data;

            // Add inventory items
            for (const auto& item : player.inventory().get_all_items()) {
                if (!item) continue;

                WorldQueries::CharacterItemData item_data;
                item_data.character_id = char_id;
                item_data.object_id = item->id();
                item_data.equipped_location = "";  // Not equipped
                item_data.condition = item->condition();
                item_data.charges = item->charges();
                // Container nesting handled through database relationships

                items_data.push_back(std::move(item_data));
            }

            // Add equipped items
            for (const auto& [slot, item] : player.equipment().get_all_equipped_with_slots()) {
                if (!item) continue;

                WorldQueries::CharacterItemData item_data;
                item_data.character_id = char_id;
                item_data.object_id = item->id();
                item_data.equipped_location = std::string(magic_enum::enum_name(slot));
                item_data.condition = item->condition();
                item_data.charges = item->charges();

                items_data.push_back(std::move(item_data));
            }

            // Save to database
            auto save_result = ConnectionPool::instance().execute(
                [&char_id, &items_data](pqxx::work& txn) {
                    return WorldQueries::save_character_items(txn, char_id, items_data);
                });

            if (!save_result) {
                Log::error("Failed to save items for player {}: {}",
                          player.name(), save_result.error().message);
                // Don't fail the entire save - JSON backup was successful
            } else {
                Log::info("Saved {} items to database for player {}",
                         items_data.size(), player.name());
            }

            // Save account wealth if player has a linked user account
            if (player.has_user_account()) {
                std::string user_id{player.user_id()};
                long account_wealth = player.account_bank().value();

                auto account_save_result = ConnectionPool::instance().execute(
                    [&user_id, account_wealth](pqxx::work& txn) {
                        return WorldQueries::save_account_wealth(txn, user_id, account_wealth);
                    });

                if (!account_save_result) {
                    Log::error("Failed to save account wealth for player {}: {}",
                              player.name(), account_save_result.error().message);
                } else {
                    Log::debug("Saved account wealth {} for player {}",
                              account_wealth, player.name());
                }
            }
        } else {
            Log::warn("Cannot save items to database: player {} has no database_id", player.name());
        }

        Log::info("Saved player {}", player.name());
        return Success();

    } catch (const std::exception& e) {
        return std::unexpected(Errors::FileSystem("Failed to save player: " + std::string(e.what())));
    }
}

Result<std::shared_ptr<Player>> PersistenceManager::load_player(std::string_view name) {
    if (!config_) {
        return std::unexpected(Errors::InvalidState("PersistenceManager not initialized"));
    }

    // Load player from database using connection pool
    auto player_result = ConnectionPool::instance().execute([name](pqxx::work& txn) {
        return PlayerQueries::load_player_by_name(txn, name);
    });

    if (!player_result) {
        Log::debug("Failed to load player '{}' from database: {}", name, player_result.error().message);
        return std::unexpected(player_result.error());
    }

    Log::info("Loaded player '{}' from database", name);

    // Convert unique_ptr to shared_ptr
    return std::shared_ptr<Player>(player_result.value().release());
}
#include "persistence_manager.hpp"

#include "../core/logging.hpp"
#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "../text/string_utils.hpp"
#include "mud_server.hpp"
#include "../database/connection_pool.hpp"
#include "../database/player_queries.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>

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

        // Save to JSON file
        std::string filename = player_dir + "/" + player_name + ".json";
        
        nlohmann::json player_data = player.to_json();
        player_data["saved_at"] = std::chrono::system_clock::now().time_since_epoch().count();
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return std::unexpected(Errors::FileSystem("Cannot open player file for writing: " + filename));
        }
        
        file << player_data.dump(2); // Pretty-print with 2-space indentation
        file.close();
        
        Log::info("Saved player {} to {}", player.name(), filename);
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
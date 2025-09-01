/***************************************************************************
 *   File: src/server/persistence_manager.cpp             Part of FieryMUD *
 *  Usage: Modern data persistence implementation                          *
 ***************************************************************************/

#include "persistence_manager.hpp"

#include "../core/logging.hpp"
#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "mud_server.hpp"

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
        std::string player_name{player.name()};
        std::transform(player_name.begin(), player_name.end(), player_name.begin(), ::tolower);
        
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
    
    try {
        // Convert player name to lowercase for consistent file naming
        std::string player_name{name};
        std::transform(player_name.begin(), player_name.end(), player_name.begin(), ::tolower);
        
        std::string player_dir = config_->player_directory;
        std::string filename = player_dir + "/" + player_name + ".json";
        
        if (!std::filesystem::exists(filename)) {
            return std::unexpected(Errors::NotFound("Player file not found: " + std::string{name}));
        }
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            return std::unexpected(Errors::FileSystem("Cannot open player file for reading: " + filename));
        }
        
        nlohmann::json player_data;
        file >> player_data;
        file.close();
        
        // Load player from JSON
        auto player_result = Player::from_json(player_data);
        if (!player_result) {
            return std::unexpected(player_result.error());
        }
        
        Log::info("Loaded player {} from {}", name, filename);
        // Convert unique_ptr to shared_ptr
        return std::shared_ptr<Player>(player_result.value().release());
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(Errors::ParseError("Failed to parse player JSON", e.what()));
    } catch (const std::exception& e) {
        return std::unexpected(Errors::FileSystem("Failed to load player: " + std::string(e.what())));
    }
}
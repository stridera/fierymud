/***************************************************************************
 *   File: src/modern/core/config.cpp                 Part of FieryMUD *
 *  Usage: Modern server configuration implementation                      *
 ***************************************************************************/

#include "config.hpp"
#include "logging.hpp"
#include <fstream>
#include <filesystem>
#include <fmt/format.h>

// Static member definitions
Config Config::instance_;
bool Config::initialized_ = false;

Result<Config> Config::load_from_file(const std::string& config_file) {
    try {
        if (!std::filesystem::exists(config_file)) {
            Log::warn("Config file '{}' not found, using defaults", config_file);
            return load_defaults();
        }
        
        std::ifstream file(config_file);
        if (!file.is_open()) {
            return std::unexpected(Error{ErrorCode::FileAccessError,
                fmt::format("Failed to open config file: {}", config_file)});
        }
        
        nlohmann::json data;
        file >> data;
        
        return from_json(data);
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::FileAccessError,
            fmt::format("Failed to load config from {}: {}", config_file, e.what())});
    }
}

Config Config::load_defaults() {
    Config config;
    // All members are already initialized with default values
    Log::info("Using default configuration");
    return config;
}

Config& Config::instance() {
    if (!initialized_) {
        Log::warn("Config not initialized, using defaults");
        instance_ = load_defaults();
        initialized_ = true;
    }
    return instance_;
}

Result<void> Config::initialize(const std::string& config_file) {
    auto result = load_from_file(config_file);
    if (!result) {
        return std::unexpected(result.error());
    }
    
    instance_ = std::move(result.value());
    initialized_ = true;
    
    Log::info("Configuration initialized from {}", config_file);
    return {};
}

Result<void> Config::initialize_with_overrides(const std::string& config_file,
                                               const std::optional<uint16_t>& port_override,
                                               const std::optional<std::string>& data_path_override) {
    auto result = load_from_file(config_file);
    if (!result) {
        return std::unexpected(result.error());
    }
    
    instance_ = std::move(result.value());
    
    // Apply command line overrides
    if (port_override) {
        instance_.port_ = *port_override;
        Log::info("Port overridden to: {}", *port_override);
    }
    
    if (data_path_override) {
        instance_.world_directory_ = *data_path_override + "/world";
        instance_.player_directory_ = *data_path_override + "/players";
        Log::info("Data path overridden to: {}", *data_path_override);
    }
    
    initialized_ = true;
    
    Log::info("Configuration initialized from {} with overrides", config_file);
    return {};
}

Result<Config> Config::from_json(const nlohmann::json& data) {
    Config config;
    
    try {
        // Load game settings
        if (data.contains("game")) {
            const auto& game = data["game"];
            if (game.contains("default_starting_room")) {
                config.default_starting_room_ = EntityId{game["default_starting_room"].get<uint64_t>()};
            }
            if (game.contains("world_directory")) {
                config.world_directory_ = game["world_directory"];
            }
            if (game.contains("player_directory")) {
                config.player_directory_ = game["player_directory"];
            }
            if (game.contains("mud_name")) {
                config.mud_name_ = game["mud_name"];
            }
        }
        
        // Load network settings  
        if (data.contains("network")) {
            const auto& network = data["network"];
            if (network.contains("port")) {
                config.port_ = network["port"];
            }
            if (network.contains("connection_timeout_seconds")) {
                config.connection_timeout_seconds_ = network["connection_timeout_seconds"];
            }
        }
        
        // Load player settings
        if (data.contains("player")) {
            const auto& player = data["player"];
            if (player.contains("starting_health")) {
                config.starting_health_ = player["starting_health"];
            }
            if (player.contains("starting_movement")) {
                config.starting_movement_ = player["starting_movement"];
            }
            if (player.contains("max_level")) {
                config.max_level_ = player["max_level"];
            }
        }
        
        // Load spell settings
        if (data.contains("spells")) {
            const auto& spells = data["spells"];
            if (spells.contains("max_circles")) {
                config.max_spell_circles_ = spells["max_circles"];
            }
            if (spells.contains("base_refresh_seconds_per_circle")) {
                config.base_refresh_seconds_per_circle_ = spells["base_refresh_seconds_per_circle"];
            }
        }
        
        // Load persistence settings
        if (data.contains("persistence")) {
            const auto& persistence = data["persistence"];
            if (persistence.contains("auto_save_interval_seconds")) {
                config.auto_save_interval_seconds_ = persistence["auto_save_interval_seconds"];
            }
        }
        
        return config;
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::ParseError,
            fmt::format("Failed to parse Config: {}", e.what())});
    }
}
#pragma once

#include "ids.hpp"
#include "result.hpp"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// Forward declaration
struct ServerConfig;

/**
 * Modern server configuration system.
 * Loads configuration from existing FieryMUD JSON config files.
 */

/**
 * Configuration class that loads from existing config JSON files.
 */
class Config {
  public:
    /** Load configuration from file */
    static Result<Config> load_from_file(const std::string &config_file = "config/test_mud.json");

    /** Load configuration with defaults */
    static Config load_defaults();

    /** Get singleton instance */
    static Config &instance();

    /** Initialize singleton with config file */
    static Result<void> initialize(const std::string &config_file = "config/test_mud.json");

    /** Initialize singleton with config file and command line overrides */
    static Result<void> initialize_with_overrides(const std::string &config_file,
                                                  const std::optional<uint16_t> &port_override = std::nullopt,
                                                  const std::optional<std::string> &data_path_override = std::nullopt);

    /** Initialize singleton from ServerConfig */
    static Result<void> initialize_from_server_config(const ServerConfig &server_config);

    // Accessors for configuration values
    EntityId default_starting_room() const { return default_starting_room_; }
    uint16_t port() const { return port_; }
    int starting_health() const { return starting_health_; }
    int starting_movement() const { return starting_movement_; }
    int max_level() const { return max_level_; }
    int max_spell_circles() const { return max_spell_circles_; }
    int base_refresh_seconds_per_circle() const { return base_refresh_seconds_per_circle_; }
    int connection_timeout_seconds() const { return connection_timeout_seconds_; }
    int auto_save_interval_seconds() const { return auto_save_interval_seconds_; }
    const std::string &world_directory() const { return world_directory_; }
    const std::string &player_directory() const { return player_directory_; }
    const std::string &mud_name() const { return mud_name_; }

  private:
    Config() = default;

    /** Load from JSON */
    static Result<Config> from_json(const nlohmann::json &data);

    // Configuration values loaded from JSON
    EntityId default_starting_room_{1001};
    uint16_t port_ = 4000;
    int starting_health_ = 100;
    int starting_movement_ = 100;
    int max_level_ = 50;
    int max_spell_circles_ = 9;
    int base_refresh_seconds_per_circle_ = 60;
    int connection_timeout_seconds_ = 300;
    int auto_save_interval_seconds_ = 300;
    std::string world_directory_ = "lib_test/world";
    std::string player_directory_ = "lib_test/players";
    std::string mud_name_ = "Modern FieryMUD";

    static Config instance_;
    static bool initialized_;
};
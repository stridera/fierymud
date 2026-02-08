/**
 * @file config_loader.hpp
 * @brief Database-driven configuration loader for FieryMUD
 *
 * Loads all game configuration from the PostgreSQL GameConfig table,
 * making the MUD fully data-driven with only DATABASE_URL in .env.
 */

#pragma once

#include "../core/result.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace fierymud::config {

/**
 * Configuration value types matching the database schema.
 */
enum class ConfigValueType { STRING, INT, FLOAT, BOOL, JSON };

/**
 * A single configuration entry from the database.
 */
struct ConfigEntry {
    std::string category;
    std::string key;
    std::string value;
    ConfigValueType value_type;
    std::optional<std::string> description;
    std::optional<std::string> min_value;
    std::optional<std::string> max_value;
    bool is_secret;
    bool restart_required;
};

/**
 * ConfigLoader - Loads and manages game configuration from PostgreSQL.
 *
 * All configuration values are stored in the GameConfig table with categories:
 * - server: Network settings (port, max_connections, etc.)
 * - security: Login attempts, timeouts, TLS settings
 * - timing: Game tick rates
 * - combat: Combat system parameters
 * - progression: Experience formulas, level caps
 * - character: Starting stats for new players
 * - display: MUD name, starting room, etc.
 *
 * Usage:
 *   auto& loader = ConfigLoader::instance();
 *   if (loader.load_from_database()) {
 *       int port = loader.get_int("server", "port").value_or(4000);
 *       std::string name = loader.get_string("display", "mud_name").value_or("FieryMUD");
 *   }
 */
class ConfigLoader {
  public:
    /**
     * Get the singleton instance.
     */
    static ConfigLoader &instance();

    // Delete copy/move constructors
    ConfigLoader(const ConfigLoader &) = delete;
    ConfigLoader &operator=(const ConfigLoader &) = delete;
    ConfigLoader(ConfigLoader &&) = delete;
    ConfigLoader &operator=(ConfigLoader &&) = delete;

    /**
     * Load all configuration from the database.
     * @return Result indicating success or failure with error message
     */
    Result<void> load_from_database();

    /**
     * Reload a specific category from the database.
     * Useful for hot-reloading certain settings without restart.
     * @param category The category to reload (e.g., "combat", "display")
     */
    Result<void> reload_category(std::string_view category);

    /**
     * Check if configuration has been loaded.
     */
    [[nodiscard]] bool is_loaded() const { return loaded_; }

    /**
     * Get the number of loaded configuration entries.
     */
    [[nodiscard]] size_t entry_count() const { return entries_.size(); }

    // ========================================
    // Typed Getters
    // ========================================

    /**
     * Get a string configuration value.
     */
    [[nodiscard]] std::optional<std::string> get_string(std::string_view category, std::string_view key) const;

    /**
     * Get an integer configuration value.
     */
    [[nodiscard]] std::optional<int> get_int(std::string_view category, std::string_view key) const;

    /**
     * Get a floating-point configuration value.
     */
    [[nodiscard]] std::optional<double> get_float(std::string_view category, std::string_view key) const;

    /**
     * Get a boolean configuration value.
     */
    [[nodiscard]] std::optional<bool> get_bool(std::string_view category, std::string_view key) const;

    /**
     * Get a duration in seconds.
     */
    [[nodiscard]] std::optional<std::chrono::seconds> get_seconds(std::string_view category,
                                                                  std::string_view key) const;

    /**
     * Get a duration in minutes.
     */
    [[nodiscard]] std::optional<std::chrono::minutes> get_minutes(std::string_view category,
                                                                  std::string_view key) const;

    // ========================================
    // Typed Getters with Defaults
    // ========================================

    [[nodiscard]] std::string get_string_or(std::string_view category, std::string_view key,
                                            std::string_view default_value) const;

    [[nodiscard]] int get_int_or(std::string_view category, std::string_view key, int default_value) const;

    [[nodiscard]] double get_float_or(std::string_view category, std::string_view key, double default_value) const;

    [[nodiscard]] bool get_bool_or(std::string_view category, std::string_view key, bool default_value) const;

    // ========================================
    // Category Access
    // ========================================

    /**
     * Get all entries in a category.
     */
    [[nodiscard]] std::vector<ConfigEntry> get_category(std::string_view category) const;

    /**
     * Get all available categories.
     */
    [[nodiscard]] std::vector<std::string> get_categories() const;

    /**
     * Check if a configuration key exists.
     */
    [[nodiscard]] bool has(std::string_view category, std::string_view key) const;

  private:
    ConfigLoader() = default;
    ~ConfigLoader() = default;

    /**
     * Generate a lookup key from category and key.
     */
    static std::string make_key(std::string_view category, std::string_view key);

    /**
     * Get raw entry by category and key.
     */
    [[nodiscard]] const ConfigEntry *get_entry(std::string_view category, std::string_view key) const;

    // Configuration storage
    std::unordered_map<std::string, ConfigEntry> entries_;
    bool loaded_ = false;
};

} // namespace fierymud::config

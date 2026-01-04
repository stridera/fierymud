#pragma once

#include "../core/result.hpp"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/**
 * @brief Character class data loaded from database
 */
struct ClassData {
    int id{0};
    std::string name;        // With color codes
    std::string plain_name;  // Clean name for matching
    std::string description;
    std::string hit_dice{"1d8"};
    std::string primary_stat;

    // Combat modifiers (data-driven from database)
    int bonus_hitroll{0};
    int bonus_damroll{0};
    int base_ac{100};         // Base armor class (100 = standard)
    int hp_per_level{10};     // HP gained per level
    int thac0_base{20};       // THAC0 at level 1
    double thac0_per_level{1.0}; // THAC0 improvement per level

    // Normalized name for case-insensitive lookups
    std::string lookup_key() const;
};

/**
 * @brief Character size enumeration
 */
enum class CharacterSize { Tiny, Small, Medium, Large, Huge };

/**
 * @brief Character race data loaded from database
 */
struct RaceData {
    std::string race_key;    // Database key (e.g., "HALF_ELF")
    std::string name;        // With color codes
    std::string plain_name;  // Clean name (e.g., "Half-Elf")
    bool playable{false};
    bool humanoid{false};

    // Combat modifiers
    int bonus_hitroll{0};
    int bonus_damroll{0};

    // Size
    CharacterSize size{CharacterSize::Medium};

    // Stat caps
    int max_strength{76};
    int max_dexterity{76};
    int max_intelligence{76};
    int max_wisdom{76};
    int max_constitution{76};
    int max_charisma{76};

    // Scaling factors (percentage, 100 = normal)
    int exp_factor{100};
    int hp_factor{100};
    int damage_factor{100};
    int ac_factor{100};

    // Start room (optional - race-specific starting location, e.g., Drow start in Underdark)
    std::optional<int> start_room_zone_id;
    std::optional<int> start_room_id;

    bool has_start_room() const {
        return start_room_zone_id.has_value() && start_room_id.has_value();
    }

    // Normalized name for case-insensitive lookups
    std::string lookup_key() const;
};

/**
 * @brief Liquid type data loaded from database
 *
 * Defines properties for drink containers - appearance, effects when consumed
 */
struct LiquidData {
    int id{0};
    std::string name;        // Display name: "dark ale"
    std::string alias;       // Command keyword: "dark-ale"
    std::string color_desc;  // Appearance when unidentified: "dark"

    // Effects when consumed (per sip)
    int drunk_effect{0};     // Intoxication increase
    int hunger_effect{0};    // Hunger satisfaction
    int thirst_effect{0};    // Thirst quenching
    // Note: is_alcoholic derived from drunk_effect > 0
    // Note: Poison and other effects are in ConsumableEffects table

    bool is_alcoholic() const { return drunk_effect > 0; }

    // Normalized alias for case-insensitive lookups
    std::string lookup_key() const;
};

/**
 * @brief Singleton cache for game data loaded from database
 *
 * Loads class, race, and liquid data at startup and provides fast lookups.
 * Thread-safe for reads after initialization.
 */
class GameDataCache {
  public:
    static GameDataCache& instance();

    // Initialization - call once at startup
    Result<void> load_all();
    Result<void> load_classes();
    Result<void> load_races();
    Result<void> load_liquids();

    // Reload data (for hot-reloading after database changes)
    Result<void> reload();

    // Class lookups
    const ClassData* find_class_by_id(int id) const;
    const ClassData* find_class_by_name(std::string_view name) const;
    const std::vector<ClassData>& all_classes() const { return classes_; }

    // Race lookups
    const RaceData* find_race_by_key(std::string_view key) const;
    const RaceData* find_race_by_name(std::string_view name) const;
    const std::vector<RaceData>& all_races() const { return races_; }
    std::vector<const RaceData*> playable_races() const;

    // Liquid lookups
    const LiquidData* find_liquid_by_name(std::string_view name) const;
    const LiquidData* find_liquid_by_alias(std::string_view alias) const;
    const std::vector<LiquidData>& all_liquids() const { return liquids_; }

    // Status
    bool is_loaded() const { return loaded_; }
    std::chrono::system_clock::time_point last_load_time() const { return last_load_time_; }

  private:
    GameDataCache() = default;
    ~GameDataCache() = default;
    GameDataCache(const GameDataCache&) = delete;
    GameDataCache& operator=(const GameDataCache&) = delete;

    // Normalize string for case-insensitive lookup
    static std::string normalize_key(std::string_view input);

    // Data storage
    std::vector<ClassData> classes_;
    std::vector<RaceData> races_;
    std::vector<LiquidData> liquids_;

    // Lookup maps (key -> index in vector)
    std::unordered_map<int, size_t> class_by_id_;
    std::unordered_map<std::string, size_t> class_by_name_;
    std::unordered_map<std::string, size_t> race_by_key_;
    std::unordered_map<std::string, size_t> race_by_name_;
    std::unordered_map<std::string, size_t> liquid_by_name_;
    std::unordered_map<std::string, size_t> liquid_by_alias_;

    // State
    bool loaded_{false};
    std::chrono::system_clock::time_point last_load_time_;
    mutable std::mutex mutex_;
};

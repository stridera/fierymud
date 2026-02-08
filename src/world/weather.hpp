#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/ids.hpp"
#include "core/result.hpp"

// Forward declarations
class Zone;
class Room;
class Actor;

/**
 * Modern weather system for FieryMUD.
 *
 * Provides realistic weather patterns with:
 * - Dynamic weather transitions based on patterns and randomness
 * - Zone-specific weather variations and overrides
 * - Weather effects on gameplay (visibility, movement, combat)
 * - Seasonal changes and time-based patterns
 * - Administrative weather control
 */

/** Weather conditions affecting the world */
enum class WeatherType {
    Clear = 0,     // Clear skies, good visibility
    Partly_Cloudy, // Some clouds, normal visibility
    Cloudy,        // Overcast, slightly reduced visibility
    Light_Rain,    // Light precipitation, reduced visibility
    Heavy_Rain,    // Heavy precipitation, poor visibility
    Thunderstorm,  // Storm with lightning, very poor visibility
    Light_Snow,    // Light snowfall, reduced visibility
    Heavy_Snow,    // Blizzard conditions, very poor visibility
    Fog,           // Dense fog, extremely poor visibility
    Windy,         // High winds, affects flying/ranged attacks
    Hot,           // Extreme heat, affects stamina
    Cold,          // Extreme cold, affects stamina
    Magical_Storm  // Supernatural weather, special effects
};

/** Weather intensity levels */
enum class WeatherIntensity {
    Calm = 0, // No effects
    Light,    // Minor effects
    Moderate, // Noticeable effects
    Severe,   // Significant effects
    Extreme   // Major effects
};

/** Weather patterns for realistic transitions */
enum class WeatherPattern {
    Stable = 0, // Weather tends to stay the same
    Variable,   // Normal weather changes
    Stormy,     // Tends toward storms and precipitation
    Calm,       // Tends toward clear and calm weather
    Seasonal,   // Follows seasonal patterns
    Chaotic,    // Rapid, unpredictable changes
    Magical     // Supernatural weather patterns
};

/** Seasonal weather modifiers */
enum class Season {
    Spring = 0, // Mild, variable weather
    Summer,     // Hot, occasional storms
    Autumn,     // Cooling, increasing precipitation
    Winter      // Cold, snow and ice
};

/** Natural disaster types */
enum class DisasterType {
    None = 0,   // No active disaster
    Tornado,    // Violent windstorm
    Blizzard,   // Severe snowstorm
    Earthquake, // Ground shaking
    Flood,      // Water overflow
    Hailstorm,  // Ice precipitation
    Sandstorm,  // Desert windstorm
    Heatwave,   // Extreme heat
    Hurricane,  // Tropical cyclone
    Tsunami,    // Tidal wave
    Waterspout  // Water tornado
};

/** Weather effects on gameplay */
struct WeatherEffects {
    float visibility_modifier = 1.0f; // 0.0 = no visibility, 1.0 = full visibility
    float movement_modifier = 1.0f;   // 0.5 = half speed, 1.0 = normal speed
    float combat_modifier = 1.0f;     // Combat accuracy modifier
    float stamina_drain = 1.0f;       // Stamina consumption multiplier
    bool blocks_flying = false;       // Prevents flying
    bool blocks_ranged = false;       // Reduces ranged attack accuracy
    bool provides_water = false;      // Can refill water containers
    bool fire_resistance = false;     // Fire spells less effective
    bool lightning_chance = false;    // Random lightning strikes possible

    /** Get description of weather effects */
    std::string describe_effects() const;

    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<WeatherEffects> from_json(const nlohmann::json &json);
};

/** Current weather state */
struct WeatherState {
    WeatherType type = WeatherType::Clear;
    WeatherIntensity intensity = WeatherIntensity::Calm;
    Season season = Season::Spring;
    DisasterType disaster = DisasterType::None;

    std::chrono::steady_clock::time_point last_change;
    std::chrono::minutes duration{0};            // How long this weather has lasted
    std::chrono::minutes predicted_duration{60}; // How long weather will likely last
    std::chrono::minutes disaster_duration{0};   // How long disaster has lasted

    /** Get weather effects based on current state */
    WeatherEffects get_effects() const;

    /** Get weather description for players */
    std::string get_description() const;

    /** Get short weather summary */
    std::string get_summary() const;

    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<WeatherState> from_json(const nlohmann::json &json);
};

/** Weather configuration for zones */
struct WeatherConfig {
    WeatherPattern pattern = WeatherPattern::Variable;
    bool override_global = false;                              // Use zone-specific weather
    std::unordered_map<WeatherType, float> type_probabilities; // Custom weather chances
    float change_frequency = 1.0f;                             // Weather change rate multiplier
    WeatherIntensity max_intensity = WeatherIntensity::Extreme;

    /** Get probability for weather type */
    float get_probability(WeatherType type) const;

    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<WeatherConfig> from_json(const nlohmann::json &json);
};

/** Weather forecast entry */
struct WeatherForecast {
    std::chrono::steady_clock::time_point time;
    WeatherType predicted_type;
    WeatherIntensity predicted_intensity;
    float confidence; // 0.0-1.0 prediction confidence

    std::string describe() const;
};

/** Weather transition information */
struct WeatherTransition {
    WeatherType from_type;
    WeatherType to_type;
    float probability;                     // Chance of this transition
    std::chrono::minutes typical_duration; // How long transition takes
    std::string transition_message;        // Description for players
};

/** Weather system manager */
class WeatherSystem {
  public:
    /** Initialize weather system */
    static Result<void> initialize();

    /** Get global weather system instance */
    static WeatherSystem &instance();

    /** Shutdown weather system */
    void shutdown();

    // Global Weather Management
    const WeatherState &global_weather() const { return global_weather_; }
    void set_global_weather(WeatherType type, WeatherIntensity intensity = WeatherIntensity::Moderate);
    void advance_global_weather(std::chrono::minutes elapsed);

    // Zone-Specific Weather
    void set_zone_weather_config(EntityId zone_id, const WeatherConfig &config);
    WeatherConfig get_zone_weather_config(EntityId zone_id) const;
    WeatherState get_zone_weather(EntityId zone_id) const;
    void set_zone_weather(EntityId zone_id, WeatherType type, WeatherIntensity intensity = WeatherIntensity::Moderate);

    // Weather Effects
    WeatherEffects get_weather_effects(EntityId zone_id) const;
    WeatherEffects get_room_weather_effects(EntityId room_id) const;

    // Weather Forecasting
    std::vector<WeatherForecast> get_forecast(EntityId zone_id,
                                              std::chrono::hours duration = std::chrono::hours(24)) const;
    float get_weather_change_probability(WeatherType from, WeatherType to, Season season) const;

    // Seasonal Management
    Season current_season() const { return current_season_; }
    void set_season(Season season);
    void advance_season(std::chrono::days elapsed);

    // Weather Updates
    void update_weather(std::chrono::minutes elapsed);
    void process_weather_effects();

    // Disaster Management
    void trigger_disaster(EntityId zone_id, DisasterType type,
                          std::chrono::minutes duration = std::chrono::minutes(60));
    void end_disaster(EntityId zone_id);
    DisasterType get_active_disaster(EntityId zone_id) const;
    bool has_active_disaster(EntityId zone_id) const;
    std::string get_disaster_description(DisasterType type) const;

    // Administrative Functions
    void force_weather_change(EntityId zone_id = INVALID_ENTITY_ID);
    void reset_weather_to_default(EntityId zone_id = INVALID_ENTITY_ID);
    std::string get_weather_report(EntityId zone_id = INVALID_ENTITY_ID) const;

    // Configuration
    void load_weather_patterns(const std::string &filename);
    void save_weather_state(const std::string &filename) const;
    Result<void> load_weather_state(const std::string &filename);

    // Weather Event System
    using WeatherChangeCallback =
        std::function<void(EntityId zone_id, const WeatherState &old_state, const WeatherState &new_state)>;
    void set_weather_change_callback(WeatherChangeCallback callback) { weather_change_callback_ = std::move(callback); }

  public:
    ~WeatherSystem() = default;

  private:
    WeatherSystem() = default;
    WeatherSystem(const WeatherSystem &) = delete;
    WeatherSystem &operator=(const WeatherSystem &) = delete;

    // Core weather state
    WeatherState global_weather_;
    Season current_season_ = Season::Spring;
    std::unordered_map<EntityId, WeatherState> zone_weather_;
    std::unordered_map<EntityId, WeatherConfig> zone_configs_;

    // Weather transition system
    std::unordered_map<WeatherType, std::vector<WeatherTransition>> transitions_;
    std::unordered_map<Season, std::unordered_map<WeatherType, float>> seasonal_modifiers_;

    // Random number generation
    mutable std::mt19937 rng_{std::random_device{}()};

    // Event callbacks
    WeatherChangeCallback weather_change_callback_;

    // Helper methods
    WeatherType generate_next_weather(const WeatherState &current, const WeatherConfig &config) const;
    std::chrono::minutes generate_weather_duration(WeatherType type, Season season) const;
    void notify_weather_change(EntityId zone_id, const WeatherState &old_state, const WeatherState &new_state);
    void initialize_default_transitions();
    void initialize_seasonal_modifiers();
    float get_seasonal_modifier(WeatherType type, Season season) const;

    // Disaster helper methods
    void update_disasters(std::chrono::minutes elapsed);
    float calculate_disaster_probability(const WeatherState &state) const;
    DisasterType select_disaster_type(const WeatherState &state, Season season) const;
    bool is_disaster_compatible_with_weather(DisasterType disaster, const WeatherState &state) const;

    static std::unique_ptr<WeatherSystem> instance_;
    static bool initialized_;
};

/** Weather utility functions */
namespace WeatherUtils {
/** Get weather type name */
std::string_view get_weather_name(WeatherType type);

/** Get weather intensity name */
std::string_view get_intensity_name(WeatherIntensity intensity);

/** Get season name */
std::string_view get_season_name(Season season);

/** Parse weather type from string */
std::optional<WeatherType> parse_weather_type(std::string_view name);

/** Parse weather intensity from string */
std::optional<WeatherIntensity> parse_weather_intensity(std::string_view name);

/** Parse season from string */
std::optional<Season> parse_season(std::string_view name);

/** Get weather color code for display */
std::string get_weather_color(WeatherType type);

/** Get weather icon/symbol */
std::string get_weather_symbol(WeatherType type);

/** Check if weather type is precipitation */
bool is_precipitation(WeatherType type);

/** Check if weather type is extreme */
bool is_extreme_weather(WeatherType type);

/** Get default weather effects for type and intensity */
WeatherEffects get_default_effects(WeatherType type, WeatherIntensity intensity);

/** Get disaster type name */
std::string_view get_disaster_name(DisasterType type);

/** Parse disaster type from string */
std::optional<DisasterType> parse_disaster_type(std::string_view name);

/** Get disaster color code for display */
std::string get_disaster_color(DisasterType type);

/** Get disaster icon/symbol */
std::string get_disaster_symbol(DisasterType type);
} // namespace WeatherUtils

/** Global weather system access */
inline WeatherSystem &Weather() { return WeatherSystem::instance(); }

/** Formatting support for weather enums */
template <> struct fmt::formatter<WeatherType> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const WeatherType &type, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", WeatherUtils::get_weather_name(type));
    }
};

template <> struct fmt::formatter<WeatherIntensity> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const WeatherIntensity &intensity, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", WeatherUtils::get_intensity_name(intensity));
    }
};

template <> struct fmt::formatter<Season> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const Season &season, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", WeatherUtils::get_season_name(season));
    }
};

template <> struct fmt::formatter<WeatherPattern> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const WeatherPattern &pattern, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", magic_enum::enum_name(pattern));
    }
};

template <> struct fmt::formatter<DisasterType> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const DisasterType &type, FormatContext &ctx) const {
        return fmt::format_to(ctx.out(), "{}", WeatherUtils::get_disaster_name(type));
    }
};

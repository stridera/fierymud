#include "weather.hpp"

#include "../core/logging.hpp"
#include "../core/result.hpp"
#include "room.hpp"
#include "zone.hpp"

#include <algorithm>
#include <fstream>

// Static members
std::unique_ptr<WeatherSystem> WeatherSystem::instance_ = nullptr;
bool WeatherSystem::initialized_ = false;

// WeatherEffects Implementation

std::string WeatherEffects::describe_effects() const {
    std::vector<std::string> effects;

    if (visibility_modifier < 0.9f) {
        if (visibility_modifier < 0.3f) {
            effects.push_back("visibility is severely reduced");
        } else if (visibility_modifier < 0.6f) {
            effects.push_back("visibility is reduced");
        } else {
            effects.push_back("visibility is slightly reduced");
        }
    }

    if (movement_modifier < 0.9f) {
        effects.push_back("movement is slowed");
    } else if (movement_modifier > 1.1f) {
        effects.push_back("movement is aided");
    }

    if (stamina_drain > 1.1f) {
        effects.push_back("activities are more tiring");
    }

    if (blocks_flying) {
        effects.push_back("flying is dangerous");
    }

    if (blocks_ranged) {
        effects.push_back("ranged attacks are difficult");
    }

    if (provides_water) {
        effects.push_back("water containers can be refilled");
    }

    if (lightning_chance) {
        effects.push_back("lightning strikes are possible");
    }

    if (effects.empty()) {
        return "no significant effects";
    }

    if (effects.size() == 1) {
        return effects[0];
    }

    std::string result = effects[0];
    for (size_t i = 1; i < effects.size() - 1; ++i) {
        result += ", " + effects[i];
    }
    result += " and " + effects.back();

    return result;
}

nlohmann::json WeatherEffects::to_json() const {
    nlohmann::json json;
    json["visibility_modifier"] = visibility_modifier;
    json["movement_modifier"] = movement_modifier;
    json["combat_modifier"] = combat_modifier;
    json["stamina_drain"] = stamina_drain;
    json["blocks_flying"] = blocks_flying;
    json["blocks_ranged"] = blocks_ranged;
    json["provides_water"] = provides_water;
    json["fire_resistance"] = fire_resistance;
    json["lightning_chance"] = lightning_chance;
    return json;
}

Result<WeatherEffects> WeatherEffects::from_json(const nlohmann::json &json) {
    try {
        WeatherEffects effects;

        if (json.contains("visibility_modifier")) {
            effects.visibility_modifier = json["visibility_modifier"].get<float>();
        }
        if (json.contains("movement_modifier")) {
            effects.movement_modifier = json["movement_modifier"].get<float>();
        }
        if (json.contains("combat_modifier")) {
            effects.combat_modifier = json["combat_modifier"].get<float>();
        }
        if (json.contains("stamina_drain")) {
            effects.stamina_drain = json["stamina_drain"].get<float>();
        }
        if (json.contains("blocks_flying")) {
            effects.blocks_flying = json["blocks_flying"].get<bool>();
        }
        if (json.contains("blocks_ranged")) {
            effects.blocks_ranged = json["blocks_ranged"].get<bool>();
        }
        if (json.contains("provides_water")) {
            effects.provides_water = json["provides_water"].get<bool>();
        }
        if (json.contains("fire_resistance")) {
            effects.fire_resistance = json["fire_resistance"].get<bool>();
        }
        if (json.contains("lightning_chance")) {
            effects.lightning_chance = json["lightning_chance"].get<bool>();
        }

        return effects;

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError("WeatherEffects JSON parsing", e.what()));
    }
}

// WeatherState Implementation

WeatherEffects WeatherState::get_effects() const { return WeatherUtils::get_default_effects(type, intensity); }

std::string WeatherState::get_description() const {
    std::string desc;

    switch (type) {
    case WeatherType::Clear:
        desc = "The sky is clear and bright";
        break;
    case WeatherType::Partly_Cloudy:
        desc = "Scattered clouds drift across the sky";
        break;
    case WeatherType::Cloudy:
        desc = "The sky is overcast with thick clouds";
        break;
    case WeatherType::Light_Rain:
        desc = "Light rain falls from the cloudy sky";
        break;
    case WeatherType::Heavy_Rain:
        desc = "Heavy rain pounds down from dark clouds";
        break;
    case WeatherType::Thunderstorm:
        desc = "A violent thunderstorm rages overhead";
        break;
    case WeatherType::Light_Snow:
        desc = "Light snow drifts down from the gray sky";
        break;
    case WeatherType::Heavy_Snow:
        desc = "A blizzard whips snow through the air";
        break;
    case WeatherType::Fog:
        desc = "Dense fog obscures everything beyond a few feet";
        break;
    case WeatherType::Windy:
        desc = "Strong winds blow across the land";
        break;
    case WeatherType::Hot:
        desc = "The air shimmers with intense heat";
        break;
    case WeatherType::Cold:
        desc = "Bitter cold penetrates to the bone";
        break;
    case WeatherType::Magical_Storm:
        desc = "Unnatural energies crackle through the air";
        break;
    }

    // Add intensity modifiers
    switch (intensity) {
    case WeatherIntensity::Light:
        if (type == WeatherType::Clear || type == WeatherType::Partly_Cloudy) {
            desc += " with a gentle breeze";
        }
        break;
    case WeatherIntensity::Severe:
        desc += " with intense fury";
        break;
    case WeatherIntensity::Extreme:
        desc += " with overwhelming force";
        break;
    default:
        break;
    }

    return desc + ".";
}

std::string WeatherState::get_summary() const {
    std::string summary = std::string(WeatherUtils::get_weather_name(type));

    if (intensity != WeatherIntensity::Moderate) {
        summary = fmt::format("{} {}", WeatherUtils::get_intensity_name(intensity), summary);
    }

    return summary;
}

nlohmann::json WeatherState::to_json() const {
    nlohmann::json json;
    json["type"] = std::string(magic_enum::enum_name(type));
    json["intensity"] = std::string(magic_enum::enum_name(intensity));
    json["season"] = std::string(magic_enum::enum_name(season));
    json["duration_minutes"] = duration.count();
    json["predicted_duration_minutes"] = predicted_duration.count();
    return json;
}

Result<WeatherState> WeatherState::from_json(const nlohmann::json &json) {
    try {
        WeatherState state;

        if (json.contains("type")) {
            auto type_name = json["type"].get<std::string>();
            if (auto type = WeatherUtils::parse_weather_type(type_name)) {
                state.type = type.value();
            }
        }

        if (json.contains("intensity")) {
            auto intensity_name = json["intensity"].get<std::string>();
            if (auto intensity = WeatherUtils::parse_weather_intensity(intensity_name)) {
                state.intensity = intensity.value();
            }
        }

        if (json.contains("season")) {
            auto season_name = json["season"].get<std::string>();
            if (auto season = WeatherUtils::parse_season(season_name)) {
                state.season = season.value();
            }
        }

        if (json.contains("duration_minutes")) {
            state.duration = std::chrono::minutes(json["duration_minutes"].get<int>());
        }

        if (json.contains("predicted_duration_minutes")) {
            state.predicted_duration = std::chrono::minutes(json["predicted_duration_minutes"].get<int>());
        }

        state.last_change = std::chrono::steady_clock::now();

        return state;

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError("WeatherState JSON parsing", e.what()));
    }
}

// WeatherConfig Implementation

float WeatherConfig::get_probability(WeatherType type) const {
    auto it = type_probabilities.find(type);
    if (it != type_probabilities.end()) {
        return it->second;
    }

    // Default probabilities based on pattern
    switch (pattern) {
    case WeatherPattern::Calm:
        return (type == WeatherType::Clear || type == WeatherType::Partly_Cloudy) ? 0.6f : 0.1f;
    case WeatherPattern::Stormy:
        return WeatherUtils::is_precipitation(type) ? 0.4f : 0.1f;
    case WeatherPattern::Variable:
        return 0.2f; // Equal chance for most weather
    case WeatherPattern::Chaotic:
        return 0.15f; // Slightly more random
    default:
        return 0.1f;
    }
}

nlohmann::json WeatherConfig::to_json() const {
    nlohmann::json json;
    json["pattern"] = std::string(magic_enum::enum_name(pattern));
    json["override_global"] = override_global;
    json["change_frequency"] = change_frequency;
    json["max_intensity"] = std::string(magic_enum::enum_name(max_intensity));

    if (!type_probabilities.empty()) {
        nlohmann::json probs;
        for (const auto &[type, prob] : type_probabilities) {
            probs[std::string(magic_enum::enum_name(type))] = prob;
        }
        json["type_probabilities"] = probs;
    }

    return json;
}

Result<WeatherConfig> WeatherConfig::from_json(const nlohmann::json &json) {
    try {
        WeatherConfig config;

        if (json.contains("pattern")) {
            auto pattern_name = json["pattern"].get<std::string>();
            if (auto pattern = magic_enum::enum_cast<WeatherPattern>(pattern_name)) {
                config.pattern = pattern.value();
            }
        }

        if (json.contains("override_global")) {
            config.override_global = json["override_global"].get<bool>();
        }

        if (json.contains("change_frequency")) {
            config.change_frequency = json["change_frequency"].get<float>();
        }

        if (json.contains("max_intensity")) {
            auto intensity_name = json["max_intensity"].get<std::string>();
            if (auto intensity = magic_enum::enum_cast<WeatherIntensity>(intensity_name)) {
                config.max_intensity = intensity.value();
            }
        }

        if (json.contains("type_probabilities")) {
            for (const auto &[type_name, prob] : json["type_probabilities"].items()) {
                if (auto type = magic_enum::enum_cast<WeatherType>(type_name)) {
                    config.type_probabilities[type.value()] = prob.get<float>();
                }
            }
        }

        return config;

    } catch (const nlohmann::json::exception &e) {
        return std::unexpected(Errors::ParseError("WeatherConfig JSON parsing", e.what()));
    }
}

// WeatherForecast Implementation

std::string WeatherForecast::describe() const {
    auto confidence_desc = [this]() -> std::string {
        if (confidence >= 0.9f)
            return "very likely";
        if (confidence >= 0.7f)
            return "likely";
        if (confidence >= 0.5f)
            return "possible";
        return "uncertain";
    };

    return fmt::format("{} {} ({})", WeatherUtils::get_intensity_name(predicted_intensity),
                       WeatherUtils::get_weather_name(predicted_type), confidence_desc());
}

// WeatherSystem Implementation

Result<void> WeatherSystem::initialize() {
    if (initialized_) {
        return {};
    }

    instance_ = std::unique_ptr<WeatherSystem>(new WeatherSystem());
    instance_->initialize_default_transitions();
    instance_->initialize_seasonal_modifiers();

    // Set initial global weather
    instance_->global_weather_.last_change = std::chrono::steady_clock::now();
    instance_->global_weather_.predicted_duration = std::chrono::minutes(120);

    auto logger = Log::game();
    logger->info("Weather system initialized");

    initialized_ = true;
    return {};
}

WeatherSystem &WeatherSystem::instance() {
    if (!initialized_ || !instance_) {
        // Emergency initialization if not done properly
        auto result = initialize();
        if (!result) {
            throw std::runtime_error("Failed to initialize weather system");
        }
    }
    return *instance_;
}

void WeatherSystem::shutdown() {
    auto logger = Log::game();
    logger->info("Weather system shutting down");

    zone_weather_.clear();
    zone_configs_.clear();
    transitions_.clear();
    seasonal_modifiers_.clear();

    initialized_ = false;
    instance_.reset(); // Explicitly reset the unique_ptr
}

void WeatherSystem::set_global_weather(WeatherType type, WeatherIntensity intensity) {
    auto old_state = global_weather_;

    global_weather_.type = type;
    global_weather_.intensity = intensity;
    global_weather_.last_change = std::chrono::steady_clock::now();
    global_weather_.duration = std::chrono::minutes(0);
    global_weather_.predicted_duration = generate_weather_duration(type, current_season_);

    notify_weather_change(INVALID_ENTITY_ID, old_state, global_weather_);

    auto logger = Log::game();
    logger->info("Global weather changed to: {}", global_weather_.get_summary());
}

void WeatherSystem::advance_global_weather(std::chrono::minutes elapsed) {
    global_weather_.duration += elapsed;

    // Check if weather should change
    if (global_weather_.duration >= global_weather_.predicted_duration) {
        WeatherConfig default_config; // Use default global config
        auto next_type = generate_next_weather(global_weather_, default_config);

        // Generate next intensity
        std::uniform_int_distribution<int> intensity_dist(0, static_cast<int>(WeatherIntensity::Extreme));
        auto next_intensity = static_cast<WeatherIntensity>(intensity_dist(rng_));

        set_global_weather(next_type, next_intensity);
    }
}

void WeatherSystem::set_zone_weather_config(EntityId zone_id, const WeatherConfig &config) {
    zone_configs_[zone_id] = config;

    auto logger = Log::game();
    logger->debug("Weather config set for zone {}: pattern={}", zone_id, config.pattern);
}

WeatherConfig WeatherSystem::get_zone_weather_config(EntityId zone_id) const {
    auto it = zone_configs_.find(zone_id);
    if (it != zone_configs_.end()) {
        return it->second;
    }
    return WeatherConfig{}; // Default config
}

WeatherState WeatherSystem::get_zone_weather(EntityId zone_id) const {
    auto it = zone_weather_.find(zone_id);
    if (it != zone_weather_.end()) {
        const auto &config = get_zone_weather_config(zone_id);
        if (config.override_global) {
            return it->second;
        }
    }
    return global_weather_;
}

void WeatherSystem::set_zone_weather(EntityId zone_id, WeatherType type, WeatherIntensity intensity) {
    auto old_state = get_zone_weather(zone_id);

    WeatherState new_state;
    new_state.type = type;
    new_state.intensity = intensity;
    new_state.season = current_season_;
    new_state.last_change = std::chrono::steady_clock::now();
    new_state.duration = std::chrono::minutes(0);
    new_state.predicted_duration = generate_weather_duration(type, current_season_);

    zone_weather_[zone_id] = new_state;

    notify_weather_change(zone_id, old_state, new_state);

    auto logger = Log::game();
    logger->info("Zone {} weather changed to: {}", zone_id, new_state.get_summary());
}

WeatherEffects WeatherSystem::get_weather_effects(EntityId zone_id) const {
    auto weather = get_zone_weather(zone_id);
    return weather.get_effects();
}

WeatherEffects WeatherSystem::get_room_weather_effects(EntityId room_id) const {
    // For now, room weather is same as zone weather
    // Could be extended to support room-specific effects (indoors, etc.)
    return get_weather_effects(room_id); // Assuming room_id maps to zone somehow
}

std::vector<WeatherForecast> WeatherSystem::get_forecast(EntityId zone_id, std::chrono::hours duration) const {
    std::vector<WeatherForecast> forecast;

    auto current_weather = get_zone_weather(zone_id);
    auto config = get_zone_weather_config(zone_id);

    auto time_point = std::chrono::steady_clock::now();
    auto end_time = time_point + duration;

    WeatherType current_type = current_weather.type;
    float confidence = 0.9f;

    while (time_point < end_time) {
        WeatherForecast entry;
        entry.time = time_point;
        entry.predicted_type = current_type;
        entry.predicted_intensity = WeatherIntensity::Moderate; // Simplified for now
        entry.confidence = std::max(0.1f, confidence);

        forecast.push_back(entry);

        // Advance time and reduce confidence
        time_point += std::chrono::hours(4);
        confidence *= 0.8f;

        // Potentially change weather type
        if (confidence < 0.5f) {
            WeatherState temp_state;
            temp_state.type = current_type;
            temp_state.intensity = WeatherIntensity::Moderate;
            temp_state.season = current_season_;
            current_type = generate_next_weather(temp_state, config);
        }
    }

    return forecast;
}

float WeatherSystem::get_weather_change_probability(WeatherType from, WeatherType to, Season season) const {
    auto it = transitions_.find(from);
    if (it != transitions_.end()) {
        for (const auto &transition : it->second) {
            if (transition.to_type == to) {
                float base_prob = transition.probability;
                float seasonal_mod = get_seasonal_modifier(to, season);
                return base_prob * seasonal_mod;
            }
        }
    }
    return 0.1f; // Default low probability
}

void WeatherSystem::set_season(Season season) {
    if (current_season_ != season) {
        auto logger = Log::game();
        logger->info("Season changed from {} to {}", current_season_, season);

        current_season_ = season;
        global_weather_.season = season;

        // Update all zone weather seasons
        for (auto &[zone_id, weather] : zone_weather_) {
            weather.season = season;
        }
    }
}

void WeatherSystem::advance_season(std::chrono::days elapsed) {
    // Simple seasonal progression - could be more sophisticated
    static std::chrono::days season_length{90}; // 3 months per season
    static std::chrono::days current_season_days{0};

    current_season_days += elapsed;

    if (current_season_days >= season_length) {
        current_season_days = std::chrono::days{0};

        Season next_season = Season::Spring; // Default, will be overwritten
        switch (current_season_) {
        case Season::Spring:
            next_season = Season::Summer;
            break;
        case Season::Summer:
            next_season = Season::Autumn;
            break;
        case Season::Autumn:
            next_season = Season::Winter;
            break;
        case Season::Winter:
            next_season = Season::Spring;
            break;
        }

        set_season(next_season);
    }
}

void WeatherSystem::update_weather(std::chrono::minutes elapsed) {
    // Update global weather
    advance_global_weather(elapsed);

    // Update zone-specific weather
    for (auto &[zone_id, weather] : zone_weather_) {
        weather.duration += elapsed;

        const auto &config = get_zone_weather_config(zone_id);
        if (config.override_global && weather.duration >= weather.predicted_duration) {
            auto next_type = generate_next_weather(weather, config);

            std::uniform_int_distribution<int> intensity_dist(0, static_cast<int>(config.max_intensity));
            auto next_intensity = static_cast<WeatherIntensity>(intensity_dist(rng_));

            set_zone_weather(zone_id, next_type, next_intensity);
        }
    }
}

void WeatherSystem::process_weather_effects() {
    // This would process ongoing weather effects like lightning strikes,
    // stamina drain, etc. For now, effects are applied when queried.
}

void WeatherSystem::force_weather_change(EntityId zone_id) {
    if (zone_id == INVALID_ENTITY_ID) {
        // Force global weather change
        WeatherConfig default_config;
        auto next_type = generate_next_weather(global_weather_, default_config);

        std::uniform_int_distribution<int> intensity_dist(0, static_cast<int>(WeatherIntensity::Extreme));
        auto next_intensity = static_cast<WeatherIntensity>(intensity_dist(rng_));

        set_global_weather(next_type, next_intensity);
    } else {
        // Force zone weather change
        auto config = get_zone_weather_config(zone_id);
        auto current_weather = get_zone_weather(zone_id);
        auto next_type = generate_next_weather(current_weather, config);

        std::uniform_int_distribution<int> intensity_dist(0, static_cast<int>(config.max_intensity));
        auto next_intensity = static_cast<WeatherIntensity>(intensity_dist(rng_));

        set_zone_weather(zone_id, next_type, next_intensity);
    }
}

void WeatherSystem::reset_weather_to_default(EntityId zone_id) {
    if (zone_id == INVALID_ENTITY_ID) {
        set_global_weather(WeatherType::Clear, WeatherIntensity::Calm);
    } else {
        set_zone_weather(zone_id, WeatherType::Clear, WeatherIntensity::Calm);
    }
}

std::string WeatherSystem::get_weather_report(EntityId zone_id) const {
    auto weather = get_zone_weather(zone_id);
    auto effects = weather.get_effects();

    std::string report = fmt::format("Weather Report\n");
    report += fmt::format("Current: {}\n", weather.get_description());
    report += fmt::format("Summary: {}\n", weather.get_summary());
    report += fmt::format("Season: {}\n", current_season_);
    report += fmt::format("Duration: {} minutes\n", weather.duration.count());
    report += fmt::format("Effects: {}\n", effects.describe_effects());

    // Add forecast
    auto forecast = get_forecast(zone_id, std::chrono::hours(12));
    if (!forecast.empty()) {
        report += "\nForecast:\n";
        for (size_t i = 0; i < std::min(size_t(3), forecast.size()); ++i) {
            report += fmt::format("  +{}h: {}\n", i * 4, forecast[i].describe());
        }
    }

    return report;
}

WeatherType WeatherSystem::generate_next_weather(const WeatherState &current, const WeatherConfig &config) const {
    // Get possible transitions from current weather
    auto it = transitions_.find(current.type);
    if (it == transitions_.end()) {
        // No transitions defined, stay the same or pick random
        std::uniform_int_distribution<int> type_dist(0, static_cast<int>(WeatherType::Magical_Storm));
        return static_cast<WeatherType>(type_dist(rng_));
    }

    // Calculate weighted probabilities
    std::vector<std::pair<WeatherType, float>> weighted_options;
    float total_weight = 0.0f;

    for (const auto &transition : it->second) {
        float weight = transition.probability;
        weight *= get_seasonal_modifier(transition.to_type, current.season);
        weight *= config.get_probability(transition.to_type);
        weight *= config.change_frequency;

        weighted_options.emplace_back(transition.to_type, weight);
        total_weight += weight;
    }

    if (total_weight <= 0.0f || weighted_options.empty()) {
        return current.type; // No valid transitions
    }

    // Select based on weighted random
    std::uniform_real_distribution<float> dist(0.0f, total_weight);
    float selected = dist(rng_);

    float cumulative = 0.0f;
    for (const auto &[type, weight] : weighted_options) {
        cumulative += weight;
        if (selected <= cumulative) {
            return type;
        }
    }

    return current.type; // Fallback
}

std::chrono::minutes WeatherSystem::generate_weather_duration(WeatherType type, Season season) const {
    // Base durations for different weather types
    std::chrono::minutes base_duration{60}; // Default 1 hour

    switch (type) {
    case WeatherType::Clear:
    case WeatherType::Partly_Cloudy:
        base_duration = std::chrono::minutes(180); // 3 hours
        break;
    case WeatherType::Cloudy:
        base_duration = std::chrono::minutes(120); // 2 hours
        break;
    case WeatherType::Light_Rain:
    case WeatherType::Light_Snow:
        base_duration = std::chrono::minutes(90); // 1.5 hours
        break;
    case WeatherType::Heavy_Rain:
    case WeatherType::Heavy_Snow:
        base_duration = std::chrono::minutes(60); // 1 hour
        break;
    case WeatherType::Thunderstorm:
        base_duration = std::chrono::minutes(45); // 45 minutes
        break;
    case WeatherType::Fog:
        base_duration = std::chrono::minutes(240); // 4 hours
        break;
    case WeatherType::Windy:
        base_duration = std::chrono::minutes(150); // 2.5 hours
        break;
    case WeatherType::Hot:
    case WeatherType::Cold:
        base_duration = std::chrono::minutes(300); // 5 hours
        break;
    case WeatherType::Magical_Storm:
        base_duration = std::chrono::minutes(30); // 30 minutes
        break;
    }

    // Apply seasonal modifiers
    float seasonal_modifier = 1.0f;
    switch (season) {
    case Season::Winter:
        if (type == WeatherType::Heavy_Snow || type == WeatherType::Cold) {
            seasonal_modifier = 2.0f; // Winter weather lasts longer
        }
        break;
    case Season::Summer:
        if (type == WeatherType::Hot || type == WeatherType::Thunderstorm) {
            seasonal_modifier = 1.5f;
        }
        break;
    default:
        break;
    }

    // Add some randomness
    std::uniform_real_distribution<float> variance(0.5f, 1.5f);
    float final_modifier = seasonal_modifier * variance(rng_);

    return std::chrono::duration_cast<std::chrono::minutes>(base_duration * final_modifier);
}

void WeatherSystem::notify_weather_change(EntityId zone_id, const WeatherState &old_state,
                                          const WeatherState &new_state) {
    if (weather_change_callback_) {
        weather_change_callback_(zone_id, old_state, new_state);
    }
}

void WeatherSystem::initialize_default_transitions() {
    // Define realistic weather transition probabilities
    transitions_[WeatherType::Clear] = {
        {WeatherType::Clear, WeatherType::Partly_Cloudy, 0.3f, std::chrono::minutes(30), "Clouds begin to gather."},
        {WeatherType::Clear, WeatherType::Windy, 0.2f, std::chrono::minutes(15), "The wind picks up."},
        {WeatherType::Clear, WeatherType::Hot, 0.1f, std::chrono::minutes(60), "The temperature begins to rise."}};

    transitions_[WeatherType::Partly_Cloudy] = {
        {WeatherType::Partly_Cloudy, WeatherType::Clear, 0.3f, std::chrono::minutes(45), "The clouds part."},
        {WeatherType::Partly_Cloudy, WeatherType::Cloudy, 0.4f, std::chrono::minutes(30), "More clouds roll in."},
        {WeatherType::Partly_Cloudy, WeatherType::Light_Rain, 0.2f, std::chrono::minutes(20),
         "Light rain begins to fall."}};

    transitions_[WeatherType::Cloudy] = {
        {WeatherType::Cloudy, WeatherType::Partly_Cloudy, 0.3f, std::chrono::minutes(60),
         "The clouds begin to break up."},
        {WeatherType::Cloudy, WeatherType::Light_Rain, 0.4f, std::chrono::minutes(20), "Rain starts to fall."},
        {WeatherType::Cloudy, WeatherType::Fog, 0.1f, std::chrono::minutes(30), "Fog begins to form."}};

    transitions_[WeatherType::Light_Rain] = {
        {WeatherType::Light_Rain, WeatherType::Cloudy, 0.3f, std::chrono::minutes(15), "The rain stops."},
        {WeatherType::Light_Rain, WeatherType::Heavy_Rain, 0.3f, std::chrono::minutes(10), "The rain intensifies."},
        {WeatherType::Light_Rain, WeatherType::Thunderstorm, 0.1f, std::chrono::minutes(15),
         "Thunder rumbles in the distance."}};

    transitions_[WeatherType::Heavy_Rain] = {{WeatherType::Heavy_Rain, WeatherType::Light_Rain, 0.4f,
                                              std::chrono::minutes(20), "The rain begins to lighten."},
                                             {WeatherType::Heavy_Rain, WeatherType::Thunderstorm, 0.3f,
                                              std::chrono::minutes(10), "Lightning flashes overhead."},
                                             {WeatherType::Heavy_Rain, WeatherType::Fog, 0.1f, std::chrono::minutes(45),
                                              "Fog forms as the rain continues."}};

    transitions_[WeatherType::Thunderstorm] = {
        {WeatherType::Thunderstorm, WeatherType::Heavy_Rain, 0.5f, std::chrono::minutes(15), "The thunder fades away."},
        {WeatherType::Thunderstorm, WeatherType::Light_Rain, 0.3f, std::chrono::minutes(30), "The storm passes."},
        {WeatherType::Thunderstorm, WeatherType::Clear, 0.1f, std::chrono::minutes(45), "The storm clears suddenly."}};

    // Add more transitions for other weather types...
    transitions_[WeatherType::Fog] = {
        {WeatherType::Fog, WeatherType::Cloudy, 0.4f, std::chrono::minutes(60), "The fog begins to lift."},
        {WeatherType::Fog, WeatherType::Clear, 0.2f, std::chrono::minutes(90), "The fog dissipates."}};

    transitions_[WeatherType::Windy] = {
        {WeatherType::Windy, WeatherType::Clear, 0.4f, std::chrono::minutes(30), "The wind dies down."},
        {WeatherType::Windy, WeatherType::Partly_Cloudy, 0.3f, std::chrono::minutes(20),
         "Clouds are blown in by the wind."}};
}

void WeatherSystem::initialize_seasonal_modifiers() {
    // Spring - variable weather
    seasonal_modifiers_[Season::Spring][WeatherType::Light_Rain] = 1.5f;
    seasonal_modifiers_[Season::Spring][WeatherType::Partly_Cloudy] = 1.3f;
    seasonal_modifiers_[Season::Spring][WeatherType::Windy] = 1.2f;

    // Summer - hot and stormy
    seasonal_modifiers_[Season::Summer][WeatherType::Hot] = 2.0f;
    seasonal_modifiers_[Season::Summer][WeatherType::Thunderstorm] = 1.5f;
    seasonal_modifiers_[Season::Summer][WeatherType::Clear] = 1.4f;

    // Autumn - cooling and wet
    seasonal_modifiers_[Season::Autumn][WeatherType::Heavy_Rain] = 1.4f;
    seasonal_modifiers_[Season::Autumn][WeatherType::Fog] = 1.3f;
    seasonal_modifiers_[Season::Autumn][WeatherType::Windy] = 1.2f;

    // Winter - cold and snowy
    seasonal_modifiers_[Season::Winter][WeatherType::Cold] = 2.0f;
    seasonal_modifiers_[Season::Winter][WeatherType::Light_Snow] = 1.8f;
    seasonal_modifiers_[Season::Winter][WeatherType::Heavy_Snow] = 1.5f;
}

float WeatherSystem::get_seasonal_modifier(WeatherType type, Season season) const {
    auto season_it = seasonal_modifiers_.find(season);
    if (season_it != seasonal_modifiers_.end()) {
        auto type_it = season_it->second.find(type);
        if (type_it != season_it->second.end()) {
            return type_it->second;
        }
    }
    return 1.0f; // Default no modifier
}

// WeatherUtils Implementation

namespace WeatherUtils {
std::string_view get_weather_name(WeatherType type) { return magic_enum::enum_name(type); }

std::string_view get_intensity_name(WeatherIntensity intensity) { return magic_enum::enum_name(intensity); }

std::string_view get_season_name(Season season) { return magic_enum::enum_name(season); }

std::optional<WeatherType> parse_weather_type(std::string_view name) {
    return magic_enum::enum_cast<WeatherType>(name);
}

std::optional<WeatherIntensity> parse_weather_intensity(std::string_view name) {
    return magic_enum::enum_cast<WeatherIntensity>(name);
}

std::optional<Season> parse_season(std::string_view name) { return magic_enum::enum_cast<Season>(name); }

std::string get_weather_color(WeatherType type) {
    switch (type) {
    case WeatherType::Clear:
        return "\033[93m"; // Yellow
    case WeatherType::Partly_Cloudy:
        return "\033[37m"; // White
    case WeatherType::Cloudy:
        return "\033[90m"; // Dark gray
    case WeatherType::Light_Rain:
    case WeatherType::Heavy_Rain:
        return "\033[94m"; // Blue
    case WeatherType::Thunderstorm:
        return "\033[95m"; // Magenta
    case WeatherType::Light_Snow:
    case WeatherType::Heavy_Snow:
        return "\033[97m"; // Bright white
    case WeatherType::Fog:
        return "\033[37m"; // White
    case WeatherType::Windy:
        return "\033[96m"; // Cyan
    case WeatherType::Hot:
        return "\033[91m"; // Red
    case WeatherType::Cold:
        return "\033[96m"; // Cyan
    case WeatherType::Magical_Storm:
        return "\033[92m"; // Green
    }
    return "\033[0m"; // Reset
}

std::string get_weather_symbol(WeatherType type) {
    switch (type) {
    case WeatherType::Clear:
        return "☀";
    case WeatherType::Partly_Cloudy:
        return "⛅";
    case WeatherType::Cloudy:
        return "☁";
    case WeatherType::Light_Rain:
        return "🌦";
    case WeatherType::Heavy_Rain:
        return "🌧";
    case WeatherType::Thunderstorm:
        return "⛈";
    case WeatherType::Light_Snow:
        return "🌨";
    case WeatherType::Heavy_Snow:
        return "❄";
    case WeatherType::Fog:
        return "🌫";
    case WeatherType::Windy:
        return "💨";
    case WeatherType::Hot:
        return "🔥";
    case WeatherType::Cold:
        return "🧊";
    case WeatherType::Magical_Storm:
        return "✨";
    }
    return "?";
}

bool is_precipitation(WeatherType type) {
    return type == WeatherType::Light_Rain || type == WeatherType::Heavy_Rain || type == WeatherType::Thunderstorm ||
           type == WeatherType::Light_Snow || type == WeatherType::Heavy_Snow;
}

bool is_extreme_weather(WeatherType type) {
    return type == WeatherType::Thunderstorm || type == WeatherType::Heavy_Snow || type == WeatherType::Hot ||
           type == WeatherType::Cold || type == WeatherType::Magical_Storm;
}

WeatherEffects get_default_effects(WeatherType type, WeatherIntensity intensity) {
    WeatherEffects effects;

    float intensity_multiplier = 1.0f;
    switch (intensity) {
    case WeatherIntensity::Calm:
        intensity_multiplier = 0.5f;
        break;
    case WeatherIntensity::Light:
        intensity_multiplier = 0.7f;
        break;
    case WeatherIntensity::Moderate:
        intensity_multiplier = 1.0f;
        break;
    case WeatherIntensity::Severe:
        intensity_multiplier = 1.3f;
        break;
    case WeatherIntensity::Extreme:
        intensity_multiplier = 1.6f;
        break;
    }

    switch (type) {
    case WeatherType::Clear:
        // No negative effects, slight bonus to visibility
        effects.visibility_modifier = 1.1f;
        break;

    case WeatherType::Partly_Cloudy:
        // Minor effects
        effects.visibility_modifier = 0.95f;
        break;

    case WeatherType::Cloudy:
        effects.visibility_modifier = 0.85f * intensity_multiplier;
        break;

    case WeatherType::Light_Rain:
        effects.visibility_modifier = 0.75f * intensity_multiplier;
        effects.movement_modifier = 0.95f;
        effects.blocks_ranged = true;
        effects.provides_water = true;
        break;

    case WeatherType::Heavy_Rain:
        effects.visibility_modifier = 0.5f * intensity_multiplier;
        effects.movement_modifier = 0.85f;
        effects.combat_modifier = 0.9f;
        effects.blocks_ranged = true;
        effects.provides_water = true;
        effects.fire_resistance = true;
        break;

    case WeatherType::Thunderstorm:
        effects.visibility_modifier = 0.3f * intensity_multiplier;
        effects.movement_modifier = 0.75f;
        effects.combat_modifier = 0.8f;
        effects.blocks_flying = true;
        effects.blocks_ranged = true;
        effects.provides_water = true;
        effects.fire_resistance = true;
        effects.lightning_chance = true;
        break;

    case WeatherType::Light_Snow:
        effects.visibility_modifier = 0.7f * intensity_multiplier;
        effects.movement_modifier = 0.9f;
        effects.stamina_drain = 1.1f;
        break;

    case WeatherType::Heavy_Snow:
        effects.visibility_modifier = 0.4f * intensity_multiplier;
        effects.movement_modifier = 0.7f;
        effects.combat_modifier = 0.85f;
        effects.stamina_drain = 1.3f;
        effects.blocks_flying = true;
        effects.fire_resistance = true;
        break;

    case WeatherType::Fog:
        effects.visibility_modifier = 0.2f * intensity_multiplier;
        effects.movement_modifier = 0.8f;
        effects.combat_modifier = 0.7f;
        effects.blocks_ranged = true;
        break;

    case WeatherType::Windy:
        effects.movement_modifier = 1.1f; // Tailwind can help
        effects.blocks_flying = intensity >= WeatherIntensity::Severe;
        effects.blocks_ranged = true;
        break;

    case WeatherType::Hot:
        effects.movement_modifier = 0.9f;
        effects.stamina_drain = 1.2f * intensity_multiplier;
        break;

    case WeatherType::Cold:
        effects.movement_modifier = 0.85f;
        effects.stamina_drain = 1.3f * intensity_multiplier;
        break;

    case WeatherType::Magical_Storm:
        effects.visibility_modifier = 0.4f;
        effects.movement_modifier = 0.8f;
        effects.combat_modifier = 1.2f; // Magic enhanced
        effects.stamina_drain = 1.1f;
        effects.blocks_flying = true;
        effects.lightning_chance = true;
        break;
    }

    return effects;
}

std::string_view get_disaster_name(DisasterType type) { return magic_enum::enum_name(type); }

std::optional<DisasterType> parse_disaster_type(std::string_view name) {
    return magic_enum::enum_cast<DisasterType>(name);
}

std::string get_disaster_color(DisasterType type) {
    switch (type) {
    case DisasterType::None:
        return "\033[0m"; // Reset
    case DisasterType::Tornado:
        return "\033[95m"; // Magenta
    case DisasterType::Blizzard:
        return "\033[97m"; // Bright white
    case DisasterType::Earthquake:
        return "\033[33m"; // Yellow/brown
    case DisasterType::Flood:
        return "\033[94m"; // Blue
    case DisasterType::Hailstorm:
        return "\033[96m"; // Cyan
    case DisasterType::Sandstorm:
        return "\033[93m"; // Yellow
    case DisasterType::Heatwave:
        return "\033[91m"; // Red
    case DisasterType::Hurricane:
        return "\033[35m"; // Purple
    case DisasterType::Tsunami:
        return "\033[34m"; // Dark blue
    case DisasterType::Waterspout:
        return "\033[36m"; // Teal
    }
    return "\033[0m"; // Reset
}

std::string get_disaster_symbol(DisasterType type) {
    switch (type) {
    case DisasterType::None:
        return "";
    case DisasterType::Tornado:
        return "🌪";
    case DisasterType::Blizzard:
        return "🌨";
    case DisasterType::Earthquake:
        return "🌋";
    case DisasterType::Flood:
        return "🌊";
    case DisasterType::Hailstorm:
        return "🧊";
    case DisasterType::Sandstorm:
        return "🏜";
    case DisasterType::Heatwave:
        return "🔥";
    case DisasterType::Hurricane:
        return "🌀";
    case DisasterType::Tsunami:
        return "🌊";
    case DisasterType::Waterspout:
        return "🌊";
    }
    return "?";
}
} // namespace WeatherUtils

// Disaster Management Implementation

void WeatherSystem::trigger_disaster(EntityId zone_id, DisasterType type, std::chrono::minutes duration) {
    if (zone_id == INVALID_ENTITY_ID) {
        // Trigger disaster globally
        global_weather_.disaster = type;
        global_weather_.disaster_duration = duration;
    } else {
        // Trigger disaster for specific zone
        auto &weather = zone_weather_[zone_id];
        weather.disaster = type;
        weather.disaster_duration = duration;
    }
}

void WeatherSystem::end_disaster(EntityId zone_id) {
    if (zone_id == INVALID_ENTITY_ID) {
        global_weather_.disaster = DisasterType::None;
        global_weather_.disaster_duration = std::chrono::minutes{0};
    } else {
        auto it = zone_weather_.find(zone_id);
        if (it != zone_weather_.end()) {
            it->second.disaster = DisasterType::None;
            it->second.disaster_duration = std::chrono::minutes{0};
        }
    }
}

DisasterType WeatherSystem::get_active_disaster(EntityId zone_id) const {
    if (zone_id == INVALID_ENTITY_ID) {
        return global_weather_.disaster;
    }

    auto it = zone_weather_.find(zone_id);
    if (it != zone_weather_.end()) {
        return it->second.disaster;
    }

    return global_weather_.disaster;
}

bool WeatherSystem::has_active_disaster(EntityId zone_id) const {
    return get_active_disaster(zone_id) != DisasterType::None;
}

std::string WeatherSystem::get_disaster_description(DisasterType type) const {
    switch (type) {
    case DisasterType::None:
        return "No disaster in effect.";
    case DisasterType::Tornado:
        return "A violent tornado tears through the area, uprooting trees and destroying structures!";
    case DisasterType::Blizzard:
        return "A severe blizzard rages, with blinding snow and freezing winds making travel nearly impossible!";
    case DisasterType::Earthquake:
        return "The ground shakes violently as an earthquake rumbles through the region!";
    case DisasterType::Flood:
        return "Rising waters flood the area, submerging the land and washing away everything in its path!";
    case DisasterType::Hailstorm:
        return "Massive hailstones rain down from the sky, battering everything below!";
    case DisasterType::Sandstorm:
        return "A fierce sandstorm engulfs the area, with stinging sand reducing visibility to nothing!";
    case DisasterType::Heatwave:
        return "Extreme heat scorches the land, making the air shimmer and sapping strength from all living things!";
    case DisasterType::Hurricane:
        return "A massive hurricane batters the region with torrential rain and catastrophic winds!";
    case DisasterType::Tsunami:
        return "A colossal tsunami wave crashes onto the shore, devastating everything in its path!";
    case DisasterType::Waterspout:
        return "A towering waterspout forms over the water, spinning violently and threatening nearby vessels!";
    }
    return "An unknown disaster strikes!";
}

void WeatherSystem::update_disasters(std::chrono::minutes elapsed) {
    // Update global disaster duration
    if (global_weather_.disaster != DisasterType::None) {
        global_weather_.disaster_duration -= elapsed;
        if (global_weather_.disaster_duration <= std::chrono::minutes{0}) {
            global_weather_.disaster = DisasterType::None;
            global_weather_.disaster_duration = std::chrono::minutes{0};
        }
    }

    // Update zone disaster durations
    for (auto &[zone_id, weather] : zone_weather_) {
        if (weather.disaster != DisasterType::None) {
            weather.disaster_duration -= elapsed;
            if (weather.disaster_duration <= std::chrono::minutes{0}) {
                weather.disaster = DisasterType::None;
                weather.disaster_duration = std::chrono::minutes{0};
            }
        }
    }
}

float WeatherSystem::calculate_disaster_probability(const WeatherState &state) const {
    float probability = 0.0f;

    // Base probability from weather intensity
    switch (state.intensity) {
    case WeatherIntensity::Calm:
        probability = 0.001f;
        break;
    case WeatherIntensity::Light:
        probability = 0.005f;
        break;
    case WeatherIntensity::Moderate:
        probability = 0.01f;
        break;
    case WeatherIntensity::Severe:
        probability = 0.05f;
        break;
    case WeatherIntensity::Extreme:
        probability = 0.15f;
        break;
    }

    // Increase probability for extreme weather types
    if (WeatherUtils::is_extreme_weather(state.type)) {
        probability *= 2.0f;
    }

    // Reduce probability if disaster already active
    if (state.disaster != DisasterType::None) {
        probability = 0.0f;
    }

    return probability;
}

DisasterType WeatherSystem::select_disaster_type(const WeatherState &state, Season season) const {
    std::vector<DisasterType> compatible_disasters;

    // Select disasters based on weather type
    switch (state.type) {
    case WeatherType::Heavy_Rain:
    case WeatherType::Thunderstorm:
        compatible_disasters.push_back(DisasterType::Flood);
        compatible_disasters.push_back(DisasterType::Tornado);
        if (season == Season::Summer || season == Season::Autumn) {
            compatible_disasters.push_back(DisasterType::Hurricane);
        }
        break;

    case WeatherType::Heavy_Snow:
        compatible_disasters.push_back(DisasterType::Blizzard);
        compatible_disasters.push_back(DisasterType::Hailstorm);
        break;

    case WeatherType::Windy:
        if (state.intensity >= WeatherIntensity::Severe) {
            compatible_disasters.push_back(DisasterType::Tornado);
            compatible_disasters.push_back(DisasterType::Hurricane);
        }
        break;

    case WeatherType::Hot:
        if (state.intensity >= WeatherIntensity::Severe) {
            compatible_disasters.push_back(DisasterType::Heatwave);
            compatible_disasters.push_back(DisasterType::Sandstorm);
        }
        break;

    case WeatherType::Cold:
        if (state.intensity >= WeatherIntensity::Severe) {
            compatible_disasters.push_back(DisasterType::Blizzard);
        }
        break;

    default:
        // Earthquake can happen any time (not weather-dependent)
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) < 0.01f) {
            compatible_disasters.push_back(DisasterType::Earthquake);
        }
        break;
    }

    // Select random disaster from compatible types
    if (compatible_disasters.empty()) {
        return DisasterType::None;
    }

    std::uniform_int_distribution<size_t> dist(0, compatible_disasters.size() - 1);
    return compatible_disasters[dist(rng_)];
}

bool WeatherSystem::is_disaster_compatible_with_weather(DisasterType disaster, const WeatherState &state) const {
    switch (disaster) {
    case DisasterType::None:
        return true;

    case DisasterType::Tornado:
    case DisasterType::Hurricane:
        return state.type == WeatherType::Windy || state.type == WeatherType::Thunderstorm ||
               state.type == WeatherType::Heavy_Rain;

    case DisasterType::Blizzard:
        return state.type == WeatherType::Heavy_Snow || state.type == WeatherType::Cold;

    case DisasterType::Flood:
        return state.type == WeatherType::Heavy_Rain || state.type == WeatherType::Thunderstorm;

    case DisasterType::Hailstorm:
        return state.type == WeatherType::Thunderstorm || state.type == WeatherType::Heavy_Snow;

    case DisasterType::Sandstorm:
        return state.type == WeatherType::Windy || state.type == WeatherType::Hot;

    case DisasterType::Heatwave:
        return state.type == WeatherType::Hot;

    case DisasterType::Earthquake:
        // Earthquakes are not weather-dependent
        return true;

    case DisasterType::Tsunami:
    case DisasterType::Waterspout:
        // Coastal/oceanic disasters
        return state.type == WeatherType::Windy || state.type == WeatherType::Thunderstorm ||
               state.type == WeatherType::Heavy_Rain;
    }

    return false;
}

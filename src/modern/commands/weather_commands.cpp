/***************************************************************************
 *   File: src/commands/weather_commands.cpp              Part of FieryMUD *
 *  Usage: Weather-related commands for players and administrators         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "builtin_commands.hpp"
#include "command_context.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../world/weather.hpp"
#include "../world/room.hpp"
#include "../world/zone.hpp"
#include "../world/world_manager.hpp"

#include <fmt/format.h>

using namespace std::string_literals;

namespace Commands {

/** Weather command - Show current weather conditions */
class WeatherCommand : public Command {
public:
    WeatherCommand() : Command("weather", "Shows current weather conditions and forecast") {}
    
    CommandResult execute(CommandContext& ctx) override {
        auto actor = ctx.actor();
        auto room = ctx.current_room();
        
        if (!actor || !room) {
            return CommandResult::error("Unable to determine location.");
        }
        
        // Get zone weather
        EntityId zone_id = 1; // TODO: Get actual zone ID from room
        auto weather_state = Weather().get_zone_weather(zone_id);
        auto effects = Weather().get_weather_effects(zone_id);
        
        std::string response;
        
        // Current weather description
        response += fmt::format("{}Weather:{}\n", 
                               WeatherUtils::get_weather_color(weather_state.type),
                               "\033[0m");
        response += weather_state.get_description() + "\n\n";
        
        // Weather summary with symbol
        response += fmt::format("Current Conditions: {} {}\n",
                               WeatherUtils::get_weather_symbol(weather_state.type),
                               weather_state.get_summary());
        
        // Season information
        response += fmt::format("Season: {}\n", Weather().current_season());
        
        // Duration
        response += fmt::format("Duration: {} minutes\n", weather_state.duration.count());
        
        // Effects on gameplay
        if (effects.visibility_modifier != 1.0f || 
            effects.movement_modifier != 1.0f ||
            effects.blocks_flying || effects.blocks_ranged) {
            response += fmt::format("\nEffects: {}\n", effects.describe_effects());
        }
        
        // Simple forecast
        auto forecast = Weather().get_forecast(zone_id, std::chrono::hours(8));
        if (forecast.size() > 1) {
            response += "\nForecast:\n";
            for (size_t i = 1; i < std::min(size_t(3), forecast.size()); ++i) {
                response += fmt::format("  +{}h: {}\n", i * 4, forecast[i].describe());
            }
        }
        
        return CommandResult::success(response);
    }
    
    bool is_available_to(const Actor& actor) const override {
        return true; // Available to all players
    }
};

/** Forecast command - Extended weather forecast */
class ForecastCommand : public Command {
public:
    ForecastCommand() : Command("forecast", "Shows extended weather forecast") {}
    
    CommandResult execute(CommandContext& ctx) override {
        auto actor = ctx.actor();
        
        if (!actor) {
            return CommandResult::error("Unable to determine actor.");
        }
        
        // Parse arguments for forecast duration
        auto args = ctx.arguments();
        std::chrono::hours duration{24}; // Default 24 hours
        
        if (!args.empty()) {
            try {
                int hours = std::stoi(args[0]);
                if (hours > 0 && hours <= 168) { // Max 1 week
                    duration = std::chrono::hours(hours);
                } else {
                    return CommandResult::error("Forecast duration must be between 1 and 168 hours.");
                }
            } catch (const std::exception&) {
                return CommandResult::error("Invalid forecast duration. Use a number of hours.");
            }
        }
        
        EntityId zone_id = 1; // TODO: Get actual zone ID from room
        auto forecast = Weather().get_forecast(zone_id, duration);
        
        std::string response = fmt::format("Weather Forecast - Next {} hours:\n\n", duration.count());
        
        for (size_t i = 0; i < forecast.size(); ++i) {
            response += fmt::format("+{:2}h: {} {}\n", 
                                   i * 4,
                                   WeatherUtils::get_weather_symbol(forecast[i].predicted_type),
                                   forecast[i].describe());
        }
        
        return CommandResult::success(response);
    }
    
    bool is_available_to(const Actor& actor) const override {
        return true; // Available to all players
    }
};

/** Weatherset command - Administrative weather control */
class WeatherSetCommand : public Command {
public:
    WeatherSetCommand() : Command("weatherset", "Sets weather conditions (admin only)") {}
    
    CommandResult execute(CommandContext& ctx) override {
        auto actor = ctx.actor();
        
        if (!actor) {
            return CommandResult::error("Unable to determine actor.");
        }
        
        auto args = ctx.arguments();
        if (args.empty()) {
            return show_usage();
        }
        
        std::string subcommand = args[0];
        
        if (subcommand == "type" || subcommand == "weather") {
            return set_weather_type(ctx);
        } else if (subcommand == "intensity") {
            return set_weather_intensity(ctx);
        } else if (subcommand == "season") {
            return set_season(ctx);
        } else if (subcommand == "force") {
            return force_weather_change(ctx);
        } else if (subcommand == "reset") {
            return reset_weather(ctx);
        } else if (subcommand == "report") {
            return show_weather_report(ctx);
        } else {
            return show_usage();
        }
    }
    
    bool is_available_to(const Actor& actor) const override {
        return actor.is_god(); // Admin only
    }
    
private:
    CommandResult show_usage() {
        std::string usage = R"(Weather Set Commands:
  weatherset type <weather_type> [zone_id]    - Set weather type
  weatherset intensity <intensity> [zone_id]  - Set weather intensity  
  weatherset season <season>                  - Set global season
  weatherset force [zone_id]                  - Force weather change
  weatherset reset [zone_id]                  - Reset to clear weather
  weatherset report [zone_id]                 - Show detailed weather report

Weather Types: Clear, Partly_Cloudy, Cloudy, Light_Rain, Heavy_Rain, 
               Thunderstorm, Light_Snow, Heavy_Snow, Fog, Windy, Hot, Cold, Magical_Storm

Intensities: Calm, Light, Moderate, Severe, Extreme

Seasons: Spring, Summer, Autumn, Winter)";
        
        return CommandResult::success(usage);
    }
    
    CommandResult set_weather_type(CommandContext& ctx) {
        auto args = ctx.arguments();
        if (args.size() < 2) {
            return CommandResult::error("Usage: weatherset type <weather_type> [zone_id]");
        }
        
        auto weather_type = WeatherUtils::parse_weather_type(args[1]);
        if (!weather_type) {
            return CommandResult::error("Invalid weather type. Use 'weatherset' for valid types.");
        }
        
        EntityId zone_id = INVALID_ENTITY_ID;
        if (args.size() >= 3) {
            try {
                zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[2]))};
            } catch (const std::exception&) {
                return CommandResult::error("Invalid zone ID.");
            }
        }
        
        if (zone_id == INVALID_ENTITY_ID) {
            Weather().set_global_weather(weather_type.value());
            return CommandResult::success(fmt::format("Global weather set to: {}", weather_type.value()));
        } else {
            Weather().set_zone_weather(zone_id, weather_type.value());
            return CommandResult::success(fmt::format("Zone {} weather set to: {}", zone_id, weather_type.value()));
        }
    }
    
    CommandResult set_weather_intensity(CommandContext& ctx) {
        auto args = ctx.arguments();
        if (args.size() < 2) {
            return CommandResult::error("Usage: weatherset intensity <intensity> [zone_id]");
        }
        
        auto intensity = WeatherUtils::parse_weather_intensity(args[1]);
        if (!intensity) {
            return CommandResult::error("Invalid weather intensity. Use 'weatherset' for valid intensities.");
        }
        
        EntityId zone_id = INVALID_ENTITY_ID;
        if (args.size() >= 3) {
            try {
                zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[2]))};
            } catch (const std::exception&) {
                return CommandResult::error("Invalid zone ID.");
            }
        }
        
        // Get current weather type
        WeatherType current_type;
        if (zone_id == INVALID_ENTITY_ID) {
            current_type = Weather().global_weather().type;
            Weather().set_global_weather(current_type, intensity.value());
            return CommandResult::success(fmt::format("Global weather intensity set to: {}", intensity.value()));
        } else {
            current_type = Weather().get_zone_weather(zone_id).type;
            Weather().set_zone_weather(zone_id, current_type, intensity.value());
            return CommandResult::success(fmt::format("Zone {} weather intensity set to: {}", zone_id, intensity.value()));
        }
    }
    
    CommandResult set_season(CommandContext& ctx) {
        auto args = ctx.arguments();
        if (args.size() < 2) {
            return CommandResult::error("Usage: weatherset season <season>");
        }
        
        auto season = WeatherUtils::parse_season(args[1]);
        if (!season) {
            return CommandResult::error("Invalid season. Use: Spring, Summer, Autumn, Winter");
        }
        
        Weather().set_season(season.value());
        return CommandResult::success(fmt::format("Season set to: {}", season.value()));
    }
    
    CommandResult force_weather_change(CommandContext& ctx) {
        auto args = ctx.arguments();
        
        EntityId zone_id = INVALID_ENTITY_ID;
        if (args.size() >= 2) {
            try {
                zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[1]))};
            } catch (const std::exception&) {
                return CommandResult::error("Invalid zone ID.");
            }
        }
        
        Weather().force_weather_change(zone_id);
        
        if (zone_id == INVALID_ENTITY_ID) {
            auto new_weather = Weather().global_weather();
            return CommandResult::success(fmt::format("Global weather changed to: {}", new_weather.get_summary()));
        } else {
            auto new_weather = Weather().get_zone_weather(zone_id);
            return CommandResult::success(fmt::format("Zone {} weather changed to: {}", zone_id, new_weather.get_summary()));
        }
    }
    
    CommandResult reset_weather(CommandContext& ctx) {
        auto args = ctx.arguments();
        
        EntityId zone_id = INVALID_ENTITY_ID;
        if (args.size() >= 2) {
            try {
                zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[1]))};
            } catch (const std::exception&) {
                return CommandResult::error("Invalid zone ID.");
            }
        }
        
        Weather().reset_weather_to_default(zone_id);
        
        if (zone_id == INVALID_ENTITY_ID) {
            return CommandResult::success("Global weather reset to clear skies.");
        } else {
            return CommandResult::success(fmt::format("Zone {} weather reset to clear skies.", zone_id));
        }
    }
    
    CommandResult show_weather_report(CommandContext& ctx) {
        auto args = ctx.arguments();
        
        EntityId zone_id = INVALID_ENTITY_ID;
        if (args.size() >= 2) {
            try {
                zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[1]))};
            } catch (const std::exception&) {
                return CommandResult::error("Invalid zone ID.");
            }
        }
        
        std::string report = Weather().get_weather_report(zone_id);
        return CommandResult::success(report);
    }
};

/** Weatherconfig command - Configure zone weather patterns */
class WeatherConfigCommand : public Command {
public:
    WeatherConfigCommand() : Command("weatherconfig", "Configure zone weather patterns (admin only)") {}
    
    CommandResult execute(CommandContext& ctx) override {
        auto actor = ctx.actor();
        
        if (!actor) {
            return CommandResult::error("Unable to determine actor.");
        }
        
        auto args = ctx.arguments();
        if (args.empty()) {
            return show_usage();
        }
        
        std::string subcommand = args[0];
        
        if (subcommand == "pattern") {
            return set_weather_pattern(ctx);
        } else if (subcommand == "frequency") {
            return set_change_frequency(ctx);
        } else if (subcommand == "override") {
            return set_override_global(ctx);
        } else if (subcommand == "show") {
            return show_config(ctx);
        } else {
            return show_usage();
        }
    }
    
    bool is_available_to(const Actor& actor) const override {
        return actor.is_god(); // Admin only
    }
    
private:
    CommandResult show_usage() {
        std::string usage = R"(Weather Config Commands:
  weatherconfig pattern <pattern> <zone_id>     - Set weather pattern for zone
  weatherconfig frequency <multiplier> <zone_id> - Set weather change frequency (0.5-3.0)
  weatherconfig override <on|off> <zone_id>     - Enable/disable global weather override
  weatherconfig show <zone_id>                  - Show current zone weather configuration

Weather Patterns: Stable, Variable, Stormy, Calm, Seasonal, Chaotic, Magical)";
        
        return CommandResult::success(usage);
    }
    
    CommandResult set_weather_pattern(CommandContext& ctx) {
        auto args = ctx.arguments();
        if (args.size() < 3) {
            return CommandResult::error("Usage: weatherconfig pattern <pattern> <zone_id>");
        }
        
        auto pattern = magic_enum::enum_cast<WeatherPattern>(args[1]);
        if (!pattern) {
            return CommandResult::error("Invalid weather pattern. Use 'weatherconfig' for valid patterns.");
        }
        
        EntityId zone_id;
        try {
            zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[2]))};
        } catch (const std::exception&) {
            return CommandResult::error("Invalid zone ID.");
        }
        
        auto config = Weather().get_zone_weather_config(zone_id);
        config.pattern = pattern.value();
        Weather().set_zone_weather_config(zone_id, config);
        
        return CommandResult::success(fmt::format("Zone {} weather pattern set to: {}", zone_id, pattern.value()));
    }
    
    CommandResult set_change_frequency(CommandContext& ctx) {
        auto args = ctx.arguments();
        if (args.size() < 3) {
            return CommandResult::error("Usage: weatherconfig frequency <multiplier> <zone_id>");
        }
        
        float frequency;
        try {
            frequency = std::stof(args[1]);
            if (frequency < 0.1f || frequency > 5.0f) {
                return CommandResult::error("Frequency multiplier must be between 0.1 and 5.0");
            }
        } catch (const std::exception&) {
            return CommandResult::error("Invalid frequency multiplier.");
        }
        
        EntityId zone_id;
        try {
            zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[2]))};
        } catch (const std::exception&) {
            return CommandResult::error("Invalid zone ID.");
        }
        
        auto config = Weather().get_zone_weather_config(zone_id);
        config.change_frequency = frequency;
        Weather().set_zone_weather_config(zone_id, config);
        
        return CommandResult::success(fmt::format("Zone {} weather change frequency set to: {:.1f}", zone_id, frequency));
    }
    
    CommandResult set_override_global(CommandContext& ctx) {
        auto args = ctx.arguments();
        if (args.size() < 3) {
            return CommandResult::error("Usage: weatherconfig override <on|off> <zone_id>");
        }
        
        bool override_global;
        if (args[1] == "on" || args[1] == "true" || args[1] == "1") {
            override_global = true;
        } else if (args[1] == "off" || args[1] == "false" || args[1] == "0") {
            override_global = false;
        } else {
            return CommandResult::error("Use 'on' or 'off' for override setting.");
        }
        
        EntityId zone_id;
        try {
            zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[2]))};
        } catch (const std::exception&) {
            return CommandResult::error("Invalid zone ID.");
        }
        
        auto config = Weather().get_zone_weather_config(zone_id);
        config.override_global = override_global;
        Weather().set_zone_weather_config(zone_id, config);
        
        return CommandResult::success(fmt::format("Zone {} global weather override: {}", 
                                                 zone_id, override_global ? "enabled" : "disabled"));
    }
    
    CommandResult show_config(CommandContext& ctx) {
        auto args = ctx.arguments();
        if (args.size() < 2) {
            return CommandResult::error("Usage: weatherconfig show <zone_id>");
        }
        
        EntityId zone_id;
        try {
            zone_id = EntityId{static_cast<std::uint64_t>(std::stoull(args[1]))};
        } catch (const std::exception&) {
            return CommandResult::error("Invalid zone ID.");
        }
        
        auto config = Weather().get_zone_weather_config(zone_id);
        
        std::string response = fmt::format("Weather Configuration for Zone {}:\n", zone_id);
        response += fmt::format("Pattern: {}\n", config.pattern);
        response += fmt::format("Change Frequency: {:.1f}\n", config.change_frequency);
        response += fmt::format("Override Global: {}\n", config.override_global ? "Yes" : "No");
        response += fmt::format("Max Intensity: {}\n", config.max_intensity);
        
        if (!config.type_probabilities.empty()) {
            response += "\nCustom Weather Probabilities:\n";
            for (const auto& [type, prob] : config.type_probabilities) {
                response += fmt::format("  {}: {:.2f}\n", type, prob);
            }
        }
        
        return CommandResult::success(response);
    }
};

// Register weather commands
void register_weather_commands(CommandRegistry& registry) {
    registry.register_command(std::make_unique<WeatherCommand>());
    registry.register_command(std::make_unique<ForecastCommand>());
    registry.register_command(std::make_unique<WeatherSetCommand>());
    registry.register_command(std::make_unique<WeatherConfigCommand>());
}

} // namespace Commands
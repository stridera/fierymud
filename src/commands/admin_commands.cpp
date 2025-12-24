#include "admin_commands.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../world/weather.hpp"
#include "../world/world_manager.hpp"

namespace AdminCommands {

// =============================================================================
// Administrative Commands
// =============================================================================

Result<CommandResult> cmd_shutdown(const CommandContext &ctx) {
    // Check shutdown permission
    if (ctx.actor_permissions.find(std::string(Permissions::SHUTDOWN)) == ctx.actor_permissions.end()) {
        ctx.send_error("You don't have permission to shutdown the MUD.");
        return CommandResult::InsufficientPrivs;
    }

    ctx.send_to_all("SYSTEM: MUD is shutting down in 30 seconds!");
    Log::warn("Shutdown initiated by {}", ctx.actor->name());

    // Trigger graceful server shutdown
    // Note: In a real implementation, this should send a shutdown signal to the main server
    // For now, we'll use a simple approach through the command system
    Log::error("Shutdown command executed - server should begin graceful shutdown");
    
    // In the future, this could:
    // 1. Save all player data
    // 2. Notify connected players
    // 3. Close network connections gracefully
    // 4. Stop game loops and cleanup resources
    // 5. Signal main thread to exit

    return CommandResult::Success;
}

Result<CommandResult> cmd_goto(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<room_id>"); !result) {
        ctx.send_usage("goto <room_id>");
        return CommandResult::InvalidSyntax;
    }

    auto room_id_str = ctx.arg(0);
    EntityId room_id;
    try {
        room_id = EntityId{std::stoull(std::string{room_id_str})};
    } catch (const std::exception &e) {
        ctx.send_error(fmt::format("Invalid room ID format: {}.", room_id_str));
        return CommandResult::InvalidSyntax;
    }

    if (!room_id.is_valid()) {
        ctx.send_error("Invalid room ID.");
        return CommandResult::InvalidSyntax;
    }

    auto result = WorldManager::instance().move_actor_to_room(ctx.actor, room_id);
    if (!result.success) {
        ctx.send_error(result.failure_reason);
        return CommandResult::ResourceError;
    }

    ctx.send_success("Teleported.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_teleport(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<player> <room_id>"); !result) {
        ctx.send_usage("teleport <player> <room_id>");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_global(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not online.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Parse room ID from command argument
    auto room_id_opt = CommandParserUtils::parse_entity_id(ctx.arg(1));
    if (!room_id_opt || !room_id_opt->is_valid()) {
        ctx.send_error("Invalid room ID.");
        return CommandResult::InvalidSyntax;
    }
    auto room_id = *room_id_opt;

    // Move actor to the specified room
    auto movement_result = WorldManager::instance().move_actor_to_room(target, room_id);
    if (!movement_result.success) {
        ctx.send_error(fmt::format("Failed to teleport player: {}", movement_result.failure_reason));
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("Teleported {} to room {}.", target->display_name(), room_id));

    ctx.send_to_actor(target, "You have been teleported!");

    return CommandResult::Success;
}

Result<CommandResult> cmd_summon(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<player>"); !result) {
        ctx.send_usage("summon <player>");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_global(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not online.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Move target actor to current room
    auto movement_result = WorldManager::instance().move_actor_to_room(target, ctx.room->id());
    if (!movement_result.success) {
        ctx.send_error(fmt::format("Failed to summon player: {}", movement_result.failure_reason));
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You summon {}.", target->display_name()));
    ctx.send_to_actor(target, "You have been summoned!");

    return CommandResult::Success;
}

Result<CommandResult> cmd_weather_control(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("setweather <action> [options]");
        ctx.send("Available actions:");
        ctx.send("  setweather list                    - Show available weather types");
        ctx.send("  setweather set <type> [intensity]  - Set weather type with optional intensity");
        ctx.send("  setweather clear                   - Clear weather to default");
        ctx.send("  setweather force                   - Force immediate weather change");
        ctx.send("  setweather report [zone]           - Get detailed weather report");
        return CommandResult::InvalidSyntax;
    }

    std::string action{ctx.arg(0)};

    if (action == "list") {
        ctx.send("<b:cyan>--- Available Weather Types ---</>");
        ctx.send("Clear, Partly_Cloudy, Cloudy, Light_Rain, Heavy_Rain,");
        ctx.send("Thunderstorm, Light_Snow, Heavy_Snow, Fog, Windy, Hot, Cold, Magical_Storm");
        ctx.send("\n<b:yellow>Available Intensities:</>");
        ctx.send("Calm, Light, Moderate, Severe, Extreme");
        return CommandResult::Success;
    }

    if (action == "set") {
        if (ctx.arg_count() < 2) {
            ctx.send_usage("setweather set <type> [intensity]");
            return CommandResult::InvalidSyntax;
        }

        std::string weather_type_str{ctx.arg(1)};
        auto weather_type = WeatherUtils::parse_weather_type(weather_type_str);
        if (!weather_type) {
            ctx.send_error(fmt::format("Unknown weather type: {}. Use 'setweather list' to see available types.",
                                       weather_type_str));
            return CommandResult::InvalidSyntax;
        }

        WeatherIntensity intensity = WeatherIntensity::Moderate;
        if (ctx.arg_count() >= 3) {
            auto parsed_intensity = WeatherUtils::parse_weather_intensity(ctx.arg(2));
            if (!parsed_intensity) {
                ctx.send_error(fmt::format(
                    "Unknown weather intensity: {}. Use 'setweather list' to see available intensities.", ctx.arg(2)));
                return CommandResult::InvalidSyntax;
            }
            intensity = *parsed_intensity;
        }

        // Get current room's zone
        auto current_room = ctx.actor->current_room();
        if (!current_room) {
            ctx.send_error("You are not in a valid location.");
            return CommandResult::ResourceError;
        }

        EntityId zone_id = current_room->zone_id();

        // Set the weather
        Weather().set_zone_weather(zone_id, *weather_type, intensity);

        ctx.send_success(fmt::format("Weather set to {} ({}) in this zone.",
                                     WeatherUtils::get_weather_name(*weather_type),
                                     WeatherUtils::get_intensity_name(intensity)));

        return CommandResult::Success;
    }

    if (action == "clear") {
        auto current_room = ctx.actor->current_room();
        if (!current_room) {
            ctx.send_error("You are not in a valid location.");
            return CommandResult::ResourceError;
        }

        EntityId zone_id = current_room->zone_id();
        Weather().reset_weather_to_default(zone_id);
        ctx.send_success("Weather reset to default for this zone.");
        return CommandResult::Success;
    }

    if (action == "force") {
        auto current_room = ctx.actor->current_room();
        if (!current_room) {
            ctx.send_error("You are not in a valid location.");
            return CommandResult::ResourceError;
        }

        EntityId zone_id = current_room->zone_id();
        Weather().force_weather_change(zone_id);
        ctx.send_success("Forced immediate weather change in this zone.");
        return CommandResult::Success;
    }

    if (action == "report") {
        EntityId zone_id = INVALID_ENTITY_ID;

        if (ctx.arg_count() >= 2) {
            // Parse zone ID from argument
            auto zone_id_opt = CommandParserUtils::parse_entity_id(ctx.arg(1));
            if (zone_id_opt && zone_id_opt->is_valid()) {
                zone_id = *zone_id_opt;
            } else {
                ctx.send_error("Invalid zone ID.");
                return CommandResult::InvalidSyntax;
            }
        }

        if (zone_id == INVALID_ENTITY_ID) {
            auto current_room = ctx.actor->current_room();
            if (!current_room) {
                ctx.send_error("You are not in a valid location.");
                return CommandResult::ResourceError;
            }
            zone_id = current_room->zone_id();
        }

        std::string report = Weather().get_weather_report(zone_id);
        ctx.send(report);
        return CommandResult::Success;
    }

    ctx.send_error(fmt::format("Unknown action: {}. Use 'setweather' without arguments for help.", action));
    return CommandResult::InvalidSyntax;
}

// =============================================================================
// Zone Development Commands
// =============================================================================

Result<CommandResult> cmd_reload_zone(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("reloadzone <zone_id>");
        ctx.send("Reload a zone from its JSON file on disk.");
        ctx.send("This is useful for development when you edit zone files externally.");
        return CommandResult::InvalidSyntax;
    }

    auto zone_id_str = ctx.arg(0);
    EntityId zone_id;
    try {
        zone_id = EntityId{std::stoull(std::string{zone_id_str})};
    } catch (const std::exception &e) {
        ctx.send_error(fmt::format("Invalid zone ID format: {}.", zone_id_str));
        return CommandResult::InvalidSyntax;
    }

    if (!zone_id.is_valid()) {
        ctx.send_error("Invalid zone ID.");
        return CommandResult::InvalidSyntax;
    }

    auto result = WorldManager::instance().reload_zone(zone_id);
    if (!result) {
        ctx.send_error(fmt::format("Failed to reload zone {}: {}", zone_id, result.error().message));
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("Zone {} reloaded successfully from disk.", zone_id));
    return CommandResult::Success;
}

Result<CommandResult> cmd_save_zone(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("savezone <zone_id>");
        ctx.send("Save a zone's current state to its JSON file on disk.");
        ctx.send("This preserves any runtime modifications made through OLC or commands.");
        return CommandResult::InvalidSyntax;
    }

    auto zone_id_str = ctx.arg(0);
    EntityId zone_id;
    try {
        zone_id = EntityId{std::stoull(std::string{zone_id_str})};
    } catch (const std::exception &e) {
        ctx.send_error(fmt::format("Invalid zone ID format: {}.", zone_id_str));
        return CommandResult::InvalidSyntax;
    }

    if (!zone_id.is_valid()) {
        ctx.send_error("Invalid zone ID.");
        return CommandResult::InvalidSyntax;
    }

    auto zone = WorldManager::instance().get_zone(zone_id);
    if (!zone) {
        ctx.send_error(fmt::format("Zone {} does not exist.", zone_id));
        return CommandResult::ResourceError;
    }

    // Determine zone file path (same pattern as reload_zone uses)
    std::string filename = fmt::format("lib/world/{}.json", zone_id.value());

    auto result = zone->save_to_file(filename);
    if (!result) {
        ctx.send_error(fmt::format("Failed to save zone {} to {}: {}", zone_id, filename, result.error().message));
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("Zone {} saved successfully to {}.", zone_id, filename));
    return CommandResult::Success;
}

Result<CommandResult> cmd_reload_all_zones(const CommandContext &ctx) {
    ctx.send("Reloading all zones from disk...");

    auto result = WorldManager::instance().reload_all_zones();
    if (!result) {
        ctx.send_error(fmt::format("Failed to reload all zones: {}", result.error().message));
        return CommandResult::ResourceError;
    }

    ctx.send_success("All zones reloaded successfully from disk.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_toggle_file_watching(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("filewatch <on|off>");
        ctx.send("Toggle automatic file watching for zone hot-reload.");
        ctx.send("When enabled, zones are automatically reloaded when their files change on disk.");
        return CommandResult::InvalidSyntax;
    }

    std::string action{ctx.arg(0)};
    bool enable;

    if (action == "on" || action == "enable" || action == "true" || action == "1") {
        enable = true;
    } else if (action == "off" || action == "disable" || action == "false" || action == "0") {
        enable = false;
    } else {
        ctx.send_error(fmt::format("Invalid action: {}. Use 'on' or 'off'.", action));
        return CommandResult::InvalidSyntax;
    }

    WorldManager::instance().enable_file_watching(enable);
    ctx.send_success(fmt::format("File watching {}.", enable ? "enabled" : "disabled"));
    return CommandResult::Success;
}

Result<CommandResult> cmd_dump_world(const CommandContext &ctx) {
    std::string filename = "world_dump.json";
    if (ctx.arg_count() > 0) {
        filename = std::string{ctx.arg(0)};
    }

    ctx.send(fmt::format("Dumping world state to {}...", filename));
    WorldManager::instance().dump_world_state(filename);
    ctx.send_success(fmt::format("World state dumped to {}.", filename));
    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    // Administrative commands
    Commands()
        .command("shutdown", cmd_shutdown)
        .category("Admin")
        .privilege(PrivilegeLevel::Coder)
        .build();

    Commands()
        .command("goto", cmd_goto)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("teleport", cmd_teleport)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("summon", cmd_summon)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("setweather", cmd_weather_control)
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .build();

    // Zone development commands
    Commands()
        .command("reloadzone", cmd_reload_zone)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    Commands()
        .command("savezone", cmd_save_zone)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    Commands()
        .command("reloadallzones", cmd_reload_all_zones)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    Commands()
        .command("filewatch", cmd_toggle_file_watching)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    Commands()
        .command("dumpworld", cmd_dump_world)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    return Success();
}

} // namespace AdminCommands
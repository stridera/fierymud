#include "admin_commands.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../scripting/script_engine.hpp"
#include "../scripting/trigger_manager.hpp"
#include "../world/weather.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>
#include <sstream>

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
        ctx.send_usage("goto <room_id> (format: zone:local_id or single number)");
        return CommandResult::InvalidSyntax;
    }

    auto room_id_str = std::string{ctx.arg(0)};
    EntityId room_id;

    // Check for zone:local_id format (e.g., "30:89")
    auto colon_pos = room_id_str.find(':');
    if (colon_pos != std::string::npos) {
        try {
            int zone_id = std::stoi(room_id_str.substr(0, colon_pos));
            int local_id = std::stoi(room_id_str.substr(colon_pos + 1));
            room_id = EntityId(zone_id, local_id);
        } catch (const std::exception &e) {
            ctx.send_error(fmt::format("Invalid room ID format: {}. Use zone:local_id (e.g., 30:89).", room_id_str));
            return CommandResult::InvalidSyntax;
        }
    } else {
        // Fall back to legacy single number format
        try {
            room_id = EntityId{std::stoull(room_id_str)};
        } catch (const std::exception &e) {
            ctx.send_error(fmt::format("Invalid room ID format: {}.", room_id_str));
            return CommandResult::InvalidSyntax;
        }
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

    ctx.send_success(fmt::format("Teleported to room {}:{}.", room_id.zone_id(), room_id.local_id()));

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

Result<CommandResult> cmd_load(const CommandContext &ctx) {
    if (ctx.arg_count() < 2) {
        ctx.send_usage("load <obj|mob> <zone:id>");
        ctx.send_info("Examples: load obj 30:31, load mob 30:0");
        return CommandResult::InvalidSyntax;
    }

    std::string type_str{ctx.arg(0)};
    std::string id_str{ctx.arg(1)};

    // Parse zone:id format
    auto colon_pos = id_str.find(':');
    if (colon_pos == std::string::npos) {
        ctx.send_error("Invalid ID format. Use zone:id (e.g., 30:31)");
        return CommandResult::InvalidSyntax;
    }

    int zone_id = 0;
    int local_id = 0;
    try {
        zone_id = std::stoi(id_str.substr(0, colon_pos));
        local_id = std::stoi(id_str.substr(colon_pos + 1));
    } catch (const std::exception&) {
        ctx.send_error("Invalid ID format. Zone and ID must be numbers.");
        return CommandResult::InvalidSyntax;
    }

    EntityId prototype_id(zone_id, local_id);

    if (type_str == "obj" || type_str == "object") {
        // Load an object
        auto new_object = WorldManager::instance().create_object_instance(prototype_id);
        if (!new_object) {
            ctx.send_error(fmt::format("Object prototype {}:{} not found.", zone_id, local_id));
            return CommandResult::InvalidTarget;
        }

        // Add to player's inventory
        auto add_result = ctx.actor->inventory().add_item(new_object);
        if (!add_result) {
            ctx.send_error("You can't carry any more items!");
            return CommandResult::InvalidState;
        }

        ctx.send_success(fmt::format("You create {} [{}:{}].",
                                     new_object->short_description(), zone_id, local_id));
        Log::info("LOAD: {} loaded object {}:{} ({})",
                 ctx.actor->name(), zone_id, local_id, new_object->short_description());

    } else if (type_str == "mob" || type_str == "mobile") {
        // Load a mobile - spawn it in the current room
        auto spawned_mobile = WorldManager::instance().spawn_mobile_to_room(
            prototype_id, ctx.room->id());
        if (!spawned_mobile) {
            ctx.send_error(fmt::format("Failed to spawn mobile {}:{}.", zone_id, local_id));
            return CommandResult::InvalidTarget;
        }

        ctx.send_success(fmt::format("You create {} [{}:{}].",
                                     spawned_mobile->short_description(), zone_id, local_id));
        Log::info("LOAD: {} loaded mobile {}:{} ({}) in room {}",
                 ctx.actor->name(), zone_id, local_id, spawned_mobile->short_description(),
                 ctx.room->id());

    } else {
        ctx.send_error("Invalid type. Use 'obj' or 'mob'.");
        return CommandResult::InvalidSyntax;
    }

    return CommandResult::Success;
}

// =============================================================================
// Trigger Management Commands
// =============================================================================

Result<CommandResult> cmd_tstat(const CommandContext &ctx) {
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();

    if (!trigger_mgr.is_initialized()) {
        ctx.send_error("Trigger system is not initialized.");
        return CommandResult::ResourceError;
    }

    if (ctx.arg_count() == 0) {
        // Show global trigger statistics
        ctx.send("<b:cyan>--- Trigger System Status ---</>");
        ctx.send(fmt::format("Total triggers cached: {}", trigger_mgr.trigger_count()));
        ctx.send(fmt::format("  MOB triggers: {}", trigger_mgr.trigger_count(FieryMUD::ScriptType::MOB)));
        ctx.send(fmt::format("  OBJECT triggers: {}", trigger_mgr.trigger_count(FieryMUD::ScriptType::OBJECT)));
        ctx.send(fmt::format("  WORLD triggers: {}", trigger_mgr.trigger_count(FieryMUD::ScriptType::WORLD)));

        // Show execution statistics
        const auto& stats = trigger_mgr.stats();
        ctx.send("\n<b:yellow>Execution Statistics:</>");
        ctx.send(fmt::format("  Total executions: {}", stats.total_executions));
        ctx.send(fmt::format("  Successful: {}", stats.successful_executions));
        ctx.send(fmt::format("  Halted: {}", stats.halted_executions));
        ctx.send(fmt::format("  Yielded (wait): {}", stats.yielded_executions));
        ctx.send(fmt::format("  Failed: {}", stats.failed_executions));

        // Show bytecode cache statistics
        auto& engine = FieryMUD::ScriptEngine::instance();
        if (engine.is_initialized()) {
            ctx.send(fmt::format("\nBytecode cache entries: {}", engine.cache_size()));
            if (engine.failed_cache_size() > 0) {
                ctx.send(fmt::format("<b:yellow>Failed script cache: {}</> (scripts with compile errors)",
                                     engine.failed_cache_size()));
            }
        }

        if (!trigger_mgr.last_error().empty()) {
            ctx.send(fmt::format("\n<b:red>Last error:</> {}", trigger_mgr.last_error()));
        }

        return CommandResult::Success;
    }

    // Parse entity ID from argument
    std::string id_str{ctx.arg(0)};
    auto colon_pos = id_str.find(':');
    if (colon_pos == std::string::npos) {
        ctx.send_error("Invalid entity ID format. Use zone:id (e.g., 30:0)");
        return CommandResult::InvalidSyntax;
    }

    int zone_id = 0;
    int local_id = 0;
    try {
        zone_id = std::stoi(id_str.substr(0, colon_pos));
        local_id = std::stoi(id_str.substr(colon_pos + 1));
    } catch (const std::exception&) {
        ctx.send_error("Invalid ID format. Zone and ID must be numbers.");
        return CommandResult::InvalidSyntax;
    }

    EntityId entity_id(zone_id, local_id);

    // Show triggers for this entity
    ctx.send(fmt::format("<b:cyan>--- Triggers for Entity {}:{} ---</>", zone_id, local_id));

    // Helper to display a trigger with its script
    auto display_trigger = [&ctx](const FieryMUD::TriggerDataPtr& trigger) {
        ctx.send(fmt::format("\n<b:green>Trigger:</> {} (ID: {})", trigger->name, trigger->id));
        ctx.send(fmt::format("  <b:white>Type:</> {}",
            trigger->attach_type == FieryMUD::ScriptType::MOB ? "MOB" :
            trigger->attach_type == FieryMUD::ScriptType::OBJECT ? "OBJECT" : "WORLD"));
        ctx.send(fmt::format("  <b:white>Flags:</> {}", trigger->flags_string()));
        if (trigger->num_args > 0) {
            std::string args_str;
            for (size_t i = 0; i < trigger->arg_list.size(); ++i) {
                if (i > 0) args_str += ", ";
                args_str += trigger->arg_list[i];
            }
            ctx.send(fmt::format("  <b:white>Args:</> {} - {}", trigger->num_args, args_str));
        }
        ctx.send("  <b:white>Script:</>");
        // Split script by lines and indent each line
        std::istringstream script_stream(trigger->commands);
        std::string line;
        while (std::getline(script_stream, line)) {
            ctx.send(fmt::format("    {}", line));
        }
    };

    auto mob_triggers = trigger_mgr.get_mob_triggers(entity_id);
    if (!mob_triggers.empty()) {
        ctx.send("<b:yellow>MOB triggers:</>");
        for (const auto& trigger : mob_triggers) {
            display_trigger(trigger);
        }
    }

    auto obj_triggers = trigger_mgr.get_object_triggers(entity_id);
    if (!obj_triggers.empty()) {
        ctx.send("<b:yellow>OBJECT triggers:</>");
        for (const auto& trigger : obj_triggers) {
            display_trigger(trigger);
        }
    }

    if (local_id == 0) {
        auto world_triggers = trigger_mgr.get_world_triggers(zone_id);
        if (!world_triggers.empty()) {
            ctx.send("<b:yellow>WORLD triggers:</>");
            for (const auto& trigger : world_triggers) {
                display_trigger(trigger);
            }
        }
    }

    if (mob_triggers.empty() && obj_triggers.empty()) {
        ctx.send("No triggers found for this entity.");
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_treload(const CommandContext &ctx) {
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();

    if (!trigger_mgr.is_initialized()) {
        ctx.send_error("Trigger system is not initialized.");
        return CommandResult::ResourceError;
    }

    if (ctx.arg_count() == 0) {
        ctx.send_usage("treload <zone_id|all>");
        ctx.send("Reload triggers for a zone or all zones from the database.");
        return CommandResult::InvalidSyntax;
    }

    std::string target{ctx.arg(0)};

    if (target == "all") {
        // Reload all triggers
        trigger_mgr.clear_all_triggers();
        ctx.send("Cleared all cached triggers.");

        // Load triggers for all loaded zones
        auto zones = WorldManager::instance().get_all_zones();
        std::size_t total = 0;
        for (const auto& zone : zones) {
            auto result = trigger_mgr.load_zone_triggers(zone->id().zone_id());
            if (result) {
                total += *result;
            }
        }

        ctx.send_success(fmt::format("Reloaded {} triggers from {} zones.", total, zones.size()));
        return CommandResult::Success;
    }

    // Parse zone ID
    int zone_id = 0;
    try {
        zone_id = std::stoi(target);
    } catch (const std::exception&) {
        ctx.send_error("Invalid zone ID. Use a number or 'all'.");
        return CommandResult::InvalidSyntax;
    }

    auto result = trigger_mgr.reload_zone_triggers(static_cast<std::uint32_t>(zone_id));
    if (!result) {
        ctx.send_error(fmt::format("Failed to reload triggers: {}", result.error()));
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("Reloaded {} triggers for zone {}.", *result, zone_id));
    return CommandResult::Success;
}

Result<CommandResult> cmd_tlist(const CommandContext &ctx) {
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();

    if (!trigger_mgr.is_initialized()) {
        ctx.send_error("Trigger system is not initialized.");
        return CommandResult::ResourceError;
    }

    if (ctx.arg_count() == 0) {
        ctx.send_usage("tlist <zone_id>");
        ctx.send("List all triggers for a zone.");
        return CommandResult::InvalidSyntax;
    }

    int zone_id = 0;
    try {
        zone_id = std::stoi(std::string{ctx.arg(0)});
    } catch (const std::exception&) {
        ctx.send_error("Invalid zone ID.");
        return CommandResult::InvalidSyntax;
    }

    // Get world triggers for the zone
    auto world_triggers = trigger_mgr.get_world_triggers(zone_id);

    ctx.send(fmt::format("<b:cyan>--- Triggers in Zone {} ---</>", zone_id));

    if (world_triggers.empty()) {
        ctx.send("No triggers loaded for this zone.");
        ctx.send("Use 'treload <zone_id>' to load triggers from database.");
    } else {
        for (const auto& trigger : world_triggers) {
            ctx.send(fmt::format("  [{}] {} - {}",
                static_cast<int>(trigger->attach_type),
                trigger->name,
                trigger->flags_string()));
        }
    }

    return CommandResult::Success;
}

// =============================================================================
// Stat Commands
// =============================================================================

/**
 * Helper to parse entity ID from string in format "zone:id" or just "id"
 * Returns std::nullopt if parsing fails
 */
std::optional<EntityId> parse_stat_entity_id(std::string_view id_str) {
    auto colon_pos = id_str.find(':');
    if (colon_pos != std::string::npos) {
        try {
            int zone_id = std::stoi(std::string(id_str.substr(0, colon_pos)));
            int local_id = std::stoi(std::string(id_str.substr(colon_pos + 1)));
            return EntityId(zone_id, local_id);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    // Legacy single number format
    try {
        return EntityId{std::stoull(std::string(id_str))};
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

/**
 * Check if string looks like an ID (contains digits and optionally a colon)
 */
bool looks_like_id(std::string_view str) {
    if (str.empty()) return false;
    // Check for zone:id format
    auto colon_pos = str.find(':');
    if (colon_pos != std::string::npos) {
        // Check that both parts are numeric
        auto zone_part = str.substr(0, colon_pos);
        auto id_part = str.substr(colon_pos + 1);
        if (zone_part.empty() || id_part.empty()) return false;
        return std::all_of(zone_part.begin(), zone_part.end(), ::isdigit) &&
               std::all_of(id_part.begin(), id_part.end(), ::isdigit);
    }
    // Check for single number
    return std::all_of(str.begin(), str.end(), ::isdigit);
}

Result<CommandResult> cmd_rstat(const CommandContext &ctx) {
    std::shared_ptr<Room> room = nullptr;

    if (ctx.arg_count() == 0) {
        // Default to current room
        room = ctx.room;
        if (!room) {
            ctx.send_error("You are not in a room.");
            return CommandResult::InvalidState;
        }
    } else {
        // Parse room ID from argument
        auto room_id_opt = parse_stat_entity_id(ctx.arg(0));
        if (!room_id_opt || !room_id_opt->is_valid()) {
            ctx.send_error("Invalid room ID format. Use zone:id (e.g., 30:89).");
            return CommandResult::InvalidSyntax;
        }
        room = WorldManager::instance().get_room(*room_id_opt);
        if (!room) {
            ctx.send_error(fmt::format("Room {}:{} not found.",
                room_id_opt->zone_id(), room_id_opt->local_id()));
            return CommandResult::InvalidTarget;
        }
    }

    // Display room statistics
    ctx.send(fmt::format("<b:cyan>--- Room Statistics: {} ---</>", room->name()));
    ctx.send(fmt::format("ID: <b:yellow>{}:{}</>", room->id().zone_id(), room->id().local_id()));
    ctx.send(fmt::format("Zone: <b:white>{}</>", room->zone_id()));
    ctx.send(fmt::format("Sector: <b:green>{}</>", room->sector_type()));
    ctx.send(fmt::format("Light Level: {}", room->light_level()));

    // Flags
    const auto& flags = room->flags();
    if (!flags.empty()) {
        std::string flag_str;
        for (const auto& flag : flags) {
            if (!flag_str.empty()) flag_str += ", ";
            flag_str += std::string(RoomUtils::get_flag_name(flag));
        }
        ctx.send(fmt::format("Flags: <b:yellow>{}</>", flag_str));
    } else {
        ctx.send("Flags: <dim>none</>");
    }

    // Exits
    auto exits = room->get_available_exits();
    if (!exits.empty()) {
        ctx.send("\n<b:white>Exits:</>");
        for (const auto& dir : exits) {
            const auto* exit = room->get_exit(dir);
            if (exit && exit->to_room.is_valid()) {
                std::string door_info;
                if (exit->has_door) {
                    door_info = fmt::format(" [Door: {}{}{}, key: {}]",
                        exit->is_closed ? "closed" : "open",
                        exit->is_locked ? ", locked" : "",
                        exit->is_hidden ? ", hidden" : "",
                        exit->key_id.is_valid() ? fmt::format("{}:{}", exit->key_id.zone_id(), exit->key_id.local_id()) : "none");
                }
                ctx.send(fmt::format("  {} → {}:{}{}",
                    RoomUtils::get_direction_name(dir),
                    exit->to_room.zone_id(), exit->to_room.local_id(),
                    door_info));
            }
        }
    } else {
        ctx.send("\n<dim>No exits</>");
    }

    // Contents
    const auto& contents = room->contents();
    if (!contents.actors.empty()) {
        ctx.send(fmt::format("\n<b:white>Actors ({}):</>", contents.actors.size()));
        int count = 0;
        for (const auto& actor : contents.actors) {
            if (!actor) continue;
            ctx.send(fmt::format("  [{}:{}] {}",
                actor->id().zone_id(), actor->id().local_id(),
                actor->short_description()));
            if (++count >= 20) {
                ctx.send(fmt::format("  ... and {} more", contents.actors.size() - count));
                break;
            }
        }
    }

    if (!contents.objects.empty()) {
        ctx.send(fmt::format("\n<b:white>Objects ({}):</>", contents.objects.size()));
        int count = 0;
        for (const auto& obj : contents.objects) {
            if (!obj) continue;
            ctx.send(fmt::format("  [{}:{}] {}",
                obj->id().zone_id(), obj->id().local_id(),
                obj->short_description()));
            if (++count >= 20) {
                ctx.send(fmt::format("  ... and {} more", contents.objects.size() - count));
                break;
            }
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_zstat(const CommandContext &ctx) {
    std::shared_ptr<Zone> zone = nullptr;

    if (ctx.arg_count() == 0) {
        // Default to current zone
        if (!ctx.room) {
            ctx.send_error("You are not in a room.");
            return CommandResult::InvalidState;
        }
        zone = WorldManager::instance().get_zone(ctx.room->zone_id());
        if (!zone) {
            ctx.send_error("Could not find your current zone.");
            return CommandResult::ResourceError;
        }
    } else {
        // Parse zone ID from argument
        std::string zone_id_str{ctx.arg(0)};
        try {
            int zone_num = std::stoi(zone_id_str);
            zone = WorldManager::instance().get_zone(EntityId(zone_num, 0));
            if (!zone) {
                ctx.send_error(fmt::format("Zone {} not found.", zone_num));
                return CommandResult::InvalidTarget;
            }
        } catch (const std::exception&) {
            ctx.send_error("Invalid zone ID. Use a zone number (e.g., 30).");
            return CommandResult::InvalidSyntax;
        }
    }

    // Display zone statistics
    ctx.send(fmt::format("<b:cyan>--- Zone Statistics: {} ---</>", zone->name()));
    ctx.send(fmt::format("ID: <b:yellow>{}</>", zone->id().zone_id()));
    ctx.send(fmt::format("Builders: <b:white>{}</>",
        zone->builders().empty() ? "none" : zone->builders()));
    ctx.send(fmt::format("Level Range: {}-{}", zone->min_level(), zone->max_level()));
    ctx.send(fmt::format("Reset Mode: <b:green>{}</>", zone->reset_mode()));
    ctx.send(fmt::format("Reset Timer: {} minutes", zone->reset_minutes()));

    // Flags
    const auto& flags = zone->flags();
    if (!flags.empty()) {
        std::string flag_str;
        for (const auto& flag : flags) {
            if (!flag_str.empty()) flag_str += ", ";
            flag_str += std::string(ZoneUtils::get_flag_name(flag));
        }
        ctx.send(fmt::format("Flags: <b:yellow>{}</>", flag_str));
    } else {
        ctx.send("Flags: <dim>none</>");
    }

    // Room range
    ctx.send(fmt::format("Room Range: {}:{} to {}:{}",
        zone->first_room().zone_id(), zone->first_room().local_id(),
        zone->last_room().zone_id(), zone->last_room().local_id()));
    ctx.send(fmt::format("Total Rooms: {}", zone->rooms().size()));

    // Statistics
    const auto& stats = zone->stats();
    ctx.send(fmt::format("\n<b:white>Statistics:</>"));
    ctx.send(fmt::format("  Reset Count: {}", stats.reset_count));
    ctx.send(fmt::format("  Time Since Last Reset: {}s", stats.time_since_reset().count()));
    ctx.send(fmt::format("  Players in Zone: {}", stats.player_count));
    ctx.send(fmt::format("  Mobiles in Zone: {}", stats.mobile_count));
    ctx.send(fmt::format("  Objects in Zone: {}", stats.object_count));

    // Zone commands summary
    const auto& commands = zone->commands();
    if (!commands.empty()) {
        int mob_loads = 0, obj_loads = 0, door_cmds = 0, other_cmds = 0;
        for (const auto& cmd : commands) {
            switch (cmd.command_type) {
                case ZoneCommandType::Load_Mobile: mob_loads++; break;
                case ZoneCommandType::Load_Object:
                case ZoneCommandType::Give_Object:
                case ZoneCommandType::Equip_Object: obj_loads++; break;
                case ZoneCommandType::Open_Door:
                case ZoneCommandType::Close_Door:
                case ZoneCommandType::Lock_Door:
                case ZoneCommandType::Unlock_Door: door_cmds++; break;
                default: other_cmds++; break;
            }
        }
        ctx.send(fmt::format("\n<b:white>Zone Commands ({} total):</>", commands.size()));
        ctx.send(fmt::format("  Mob Loads: {}", mob_loads));
        ctx.send(fmt::format("  Object Commands: {}", obj_loads));
        ctx.send(fmt::format("  Door Commands: {}", door_cmds));
        if (other_cmds > 0) ctx.send(fmt::format("  Other: {}", other_cmds));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_mstat(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("mstat <zone:id | name>");
        ctx.send("Display statistics for a mobile by ID or name.");
        ctx.send("  mstat 30:0       - Show mobile with ID 30:0");
        ctx.send("  mstat guard      - Find and show guard mobile");
        return CommandResult::InvalidSyntax;
    }

    std::string search_term = ctx.command.join_args();
    std::shared_ptr<Mobile> target = nullptr;

    // Check if it looks like an ID (zone:id format or just a number)
    if (looks_like_id(ctx.arg(0))) {
        auto entity_id = parse_stat_entity_id(ctx.arg(0));
        if (entity_id && entity_id->is_valid()) {
            // Search spawned mobiles for this ID
            WorldManager::instance().for_each_mobile([&](const std::shared_ptr<Mobile>& mobile) {
                if (!target && mobile && mobile->id() == *entity_id) {
                    target = mobile;
                }
            });
            // Also check if it's a prototype ID and mobile is in current room
            if (!target && ctx.room) {
                for (const auto& actor : ctx.room->contents().actors) {
                    auto mob = std::dynamic_pointer_cast<Mobile>(actor);
                    if (mob && (mob->id() == *entity_id || mob->prototype_id() == *entity_id)) {
                        target = mob;
                        break;
                    }
                }
            }
        }
    }

    // If not found by ID, search by name
    if (!target) {
        std::string search_lower = search_term;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        // First search current room
        if (ctx.room) {
            for (const auto& actor : ctx.room->contents().actors) {
                auto mob = std::dynamic_pointer_cast<Mobile>(actor);
                if (!mob) continue;

                std::string mob_name = std::string(mob->short_description());
                std::string mob_name_lower = mob_name;
                std::transform(mob_name_lower.begin(), mob_name_lower.end(), mob_name_lower.begin(), ::tolower);

                if (mob_name_lower.find(search_lower) != std::string::npos) {
                    target = mob;
                    break;
                }

                // Check keywords
                for (const auto& kw : mob->keywords()) {
                    std::string kw_lower = kw;
                    std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                    if (kw_lower.find(search_lower) != std::string::npos) {
                        target = mob;
                        break;
                    }
                }
                if (target) break;
            }
        }

        // If not in room, search globally
        if (!target) {
            WorldManager::instance().for_each_mobile([&](const std::shared_ptr<Mobile>& mobile) {
                if (target) return; // Already found

                std::string mob_name = std::string(mobile->short_description());
                std::string mob_name_lower = mob_name;
                std::transform(mob_name_lower.begin(), mob_name_lower.end(), mob_name_lower.begin(), ::tolower);

                if (mob_name_lower.find(search_lower) != std::string::npos) {
                    target = mobile;
                    return;
                }

                for (const auto& kw : mobile->keywords()) {
                    std::string kw_lower = kw;
                    std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                    if (kw_lower.find(search_lower) != std::string::npos) {
                        target = mobile;
                        return;
                    }
                }
            });
        }
    }

    if (!target) {
        ctx.send_error(fmt::format("Mobile '{}' not found.", search_term));
        return CommandResult::InvalidTarget;
    }

    // Display mobile statistics
    ctx.send(fmt::format("<b:cyan>--- Mobile Statistics: {} ---</>", target->short_description()));
    ctx.send(fmt::format("ID: <b:yellow>{}:{}</>", target->id().zone_id(), target->id().local_id()));
    if (target->prototype_id().is_valid()) {
        ctx.send(fmt::format("Prototype: {}:{}",
            target->prototype_id().zone_id(), target->prototype_id().local_id()));
    }
    // Format keywords as comma-separated string
    std::string mob_keywords_str;
    for (const auto& kw : target->keywords()) {
        if (!mob_keywords_str.empty()) mob_keywords_str += ", ";
        mob_keywords_str += kw;
    }
    ctx.send(fmt::format("Keywords: <b:white>{}</>", mob_keywords_str));

    // Location
    auto room = target->current_room();
    if (room) {
        ctx.send(fmt::format("Location: {}:{} ({})",
            room->id().zone_id(), room->id().local_id(), room->name()));
    } else {
        ctx.send("Location: <dim>nowhere</>");
    }

    // Stats
    const auto& stats = target->stats();
    ctx.send(fmt::format("\n<b:white>Combat Stats:</>"));
    ctx.send(fmt::format("  Level: {} | HP: {}/{} | Move: {}/{}",
        stats.level, stats.hit_points, stats.max_hit_points,
        stats.movement, stats.max_movement));
    ctx.send(fmt::format("  Accuracy: {} | Attack Power: {} | Evasion: {} | Armor: {}",
        stats.accuracy, stats.attack_power, stats.evasion, stats.armor_rating));

    ctx.send(fmt::format("\n<b:white>Attributes:</>"));
    ctx.send(fmt::format("  STR: {} | DEX: {} | CON: {} | INT: {} | WIS: {} | CHA: {}",
        stats.strength, stats.dexterity, stats.constitution,
        stats.intelligence, stats.wisdom, stats.charisma));

    // Properties
    ctx.send(fmt::format("\n<b:white>Properties:</>"));
    ctx.send(fmt::format("  Race: {} | Gender: {} | Size: {}",
        target->race(), target->gender(), target->size()));
    ctx.send(fmt::format("  Life Force: {} | Composition: {} | Damage Type: {}",
        target->life_force(), target->composition(), target->damage_type()));
    ctx.send(fmt::format("  Bare Hand Damage: {}d{}+{}",
        target->bare_hand_damage_dice_num(),
        target->bare_hand_damage_dice_size(),
        target->bare_hand_damage_dice_bonus()));
    ctx.send(fmt::format("  Position: {} | Stance: {}",
        target->position(), magic_enum::enum_name(target->stance())));

    // Money
    const auto& money = target->money();
    if (money.value() > 0) {
        ctx.send(fmt::format("  Money: {}", money.to_string(false)));
    }

    // Flags - iterate through all set flags using magic_enum
    ctx.send(fmt::format("\n<b:white>Behavior Flags:</>"));
    std::string behavior_flags;
    for (auto flag : magic_enum::enum_values<MobFlag>()) {
        if (flag == MobFlag::IsNpc) continue; // Skip the always-set IsNpc flag
        if (target->has_flag(flag)) {
            if (!behavior_flags.empty()) behavior_flags += " ";
            behavior_flags += std::string(magic_enum::enum_name(flag));
        }
    }
    if (target->is_shopkeeper()) {
        if (!behavior_flags.empty()) behavior_flags += " ";
        behavior_flags += "SHOPKEEPER";
    }
    ctx.send(fmt::format("  {}", behavior_flags.empty() ? "none" : behavior_flags));

    // Effect flags
    const auto& effects = target->effect_flags();
    if (!effects.empty()) {
        std::string effect_str;
        for (const auto& effect : effects) {
            if (!effect_str.empty()) effect_str += ", ";
            effect_str += std::string(magic_enum::enum_name(effect));
        }
        ctx.send(fmt::format("  Effects: <b:green>{}</>", effect_str));
    }

    // Equipment summary
    auto equipped = target->equipment().get_all_equipped();
    if (!equipped.empty()) {
        ctx.send(fmt::format("\n<b:white>Equipment ({} items):</>", equipped.size()));
        for (const auto& item : equipped) {
            if (!item) continue;
            ctx.send(fmt::format("  [{}:{}] {}",
                item->id().zone_id(), item->id().local_id(), item->short_description()));
        }
    }

    // Inventory summary
    if (target->inventory().item_count() > 0) {
        ctx.send(fmt::format("\n<b:white>Inventory ({} items):</>", target->inventory().item_count()));
        int count = 0;
        for (const auto& item : target->inventory().get_all_items()) {
            if (!item) continue;
            ctx.send(fmt::format("  [{}:{}] {}",
                item->id().zone_id(), item->id().local_id(), item->short_description()));
            if (++count >= 10) {
                ctx.send(fmt::format("  ... and {} more", target->inventory().item_count() - count));
                break;
            }
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_ostat(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("ostat <zone:id | name>");
        ctx.send("Display statistics for an object by ID or name.");
        ctx.send("  ostat 30:31      - Show object with ID 30:31");
        ctx.send("  ostat sword      - Find and show sword object");
        return CommandResult::InvalidSyntax;
    }

    std::string search_term = ctx.command.join_args();
    std::shared_ptr<Object> target = nullptr;
    std::string location_info;

    // Helper to search container contents
    std::function<std::shared_ptr<Object>(const Object&, const std::string&, std::string&)> search_container;
    search_container = [&search_container](const Object& container, const std::string& search_lower, std::string& loc) -> std::shared_ptr<Object> {
        if (!container.is_container()) return nullptr;
        auto* cont = dynamic_cast<const Container*>(&container);
        if (!cont) return nullptr;

        for (const auto& obj : cont->get_contents()) {
            if (!obj) continue;

            std::string obj_name = std::string(obj->short_description());
            std::string obj_name_lower = obj_name;
            std::transform(obj_name_lower.begin(), obj_name_lower.end(), obj_name_lower.begin(), ::tolower);

            if (obj_name_lower.find(search_lower) != std::string::npos) {
                loc = fmt::format("in {}", container.short_description());
                return obj;
            }

            for (const auto& kw : obj->keywords()) {
                std::string kw_lower = kw;
                std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                if (kw_lower.find(search_lower) != std::string::npos) {
                    loc = fmt::format("in {}", container.short_description());
                    return obj;
                }
            }

            // Recursively search nested containers
            if (obj->is_container()) {
                auto found = search_container(*obj, search_lower, loc);
                if (found) {
                    loc = fmt::format("in {} > {}", container.short_description(), loc);
                    return found;
                }
            }
        }
        return nullptr;
    };

    // Check if it looks like an ID
    if (looks_like_id(ctx.arg(0))) {
        auto entity_id = parse_stat_entity_id(ctx.arg(0));
        if (entity_id && entity_id->is_valid()) {
            // Search all rooms for this object
            for (const auto& zone : WorldManager::instance().get_all_zones()) {
                for (const auto& room : WorldManager::instance().get_rooms_in_zone(zone->id())) {
                    if (!room) continue;

                    for (const auto& obj : room->contents().objects) {
                        if (obj && obj->id() == *entity_id) {
                            target = obj;
                            location_info = fmt::format("in room {}:{} ({})",
                                room->id().zone_id(), room->id().local_id(), room->name());
                            break;
                        }
                    }
                    if (target) break;

                    // Check actors' inventories
                    for (const auto& actor : room->contents().actors) {
                        if (!actor) continue;
                        for (const auto& item : actor->inventory().get_all_items()) {
                            if (item && item->id() == *entity_id) {
                                target = item;
                                location_info = fmt::format("carried by {}", actor->short_description());
                                break;
                            }
                        }
                        if (target) break;
                    }
                    if (target) break;
                }
                if (target) break;
            }
        }
    }

    // If not found by ID, search by name
    if (!target) {
        std::string search_lower = search_term;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        // Lambda to check object match
        auto matches = [&search_lower](const Object& obj) -> bool {
            std::string obj_name = std::string(obj.short_description());
            std::string obj_name_lower = obj_name;
            std::transform(obj_name_lower.begin(), obj_name_lower.end(), obj_name_lower.begin(), ::tolower);

            if (obj_name_lower.find(search_lower) != std::string::npos) return true;

            for (const auto& kw : obj.keywords()) {
                std::string kw_lower = kw;
                std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                if (kw_lower.find(search_lower) != std::string::npos) return true;
            }
            return false;
        };

        // First search current room
        if (ctx.room) {
            for (const auto& obj : ctx.room->contents().objects) {
                if (!obj) continue;
                if (matches(*obj)) {
                    target = obj;
                    location_info = "in current room";
                    break;
                }
                // Check container contents
                if (obj->is_container()) {
                    auto found = search_container(*obj, search_lower, location_info);
                    if (found) {
                        target = found;
                        location_info = "in current room, " + location_info;
                        break;
                    }
                }
            }

            // Check player inventory
            if (!target) {
                for (const auto& item : ctx.actor->inventory().get_all_items()) {
                    if (!item) continue;
                    if (matches(*item)) {
                        target = item;
                        location_info = "in your inventory";
                        break;
                    }
                }
            }
        }

        // Global search if not found
        if (!target) {
            for (const auto& zone : WorldManager::instance().get_all_zones()) {
                for (const auto& room : WorldManager::instance().get_rooms_in_zone(zone->id())) {
                    if (!room) continue;

                    for (const auto& obj : room->contents().objects) {
                        if (!obj) continue;
                        if (matches(*obj)) {
                            target = obj;
                            location_info = fmt::format("in room {}:{} ({})",
                                room->id().zone_id(), room->id().local_id(), room->name());
                            break;
                        }
                    }
                    if (target) break;
                }
                if (target) break;
            }
        }
    }

    if (!target) {
        ctx.send_error(fmt::format("Object '{}' not found.", search_term));
        return CommandResult::InvalidTarget;
    }

    // Display object statistics
    ctx.send(fmt::format("<b:cyan>--- Object Statistics: {} ---</>", target->short_description()));
    ctx.send(fmt::format("ID: <b:yellow>{}:{}</>", target->id().zone_id(), target->id().local_id()));
    // Format keywords as comma-separated string
    std::string obj_keywords_str;
    for (const auto& kw : target->keywords()) {
        if (!obj_keywords_str.empty()) obj_keywords_str += ", ";
        obj_keywords_str += kw;
    }
    ctx.send(fmt::format("Keywords: <b:white>{}</>", obj_keywords_str));
    ctx.send(fmt::format("Location: {}", location_info));

    // Basic properties
    ctx.send(fmt::format("\n<b:white>Properties:</>"));
    ctx.send(fmt::format("  Type: <b:green>{}</>", target->type()));
    ctx.send(fmt::format("  Weight: {} lbs | Value: {} copper | Level: {}",
        target->weight(), target->value(), target->level()));
    ctx.send(fmt::format("  Condition: {}%", target->condition()));

    if (target->is_wearable()) {
        ctx.send(fmt::format("  Wear Slot: <b:yellow>{}</>", target->equip_slot()));
    }

    // Type-specific info
    if (target->is_weapon()) {
        const auto& dmg = target->damage_profile();
        ctx.send(fmt::format("\n<b:white>Weapon Stats:</>"));
        ctx.send(fmt::format("  Damage: {}d{}+{} (avg: {:.1f})",
            dmg.dice_count, dmg.dice_sides, dmg.damage_bonus, dmg.average_damage()));
    }

    if (target->is_armor()) {
        ctx.send(fmt::format("\n<b:white>Armor Stats:</>"));
        ctx.send(fmt::format("  Armor Class: {}", target->armor_class()));
    }

    if (target->is_container()) {
        const auto& cont = target->container_info();
        ctx.send(fmt::format("\n<b:white>Container Stats:</>"));
        ctx.send(fmt::format("  Capacity: {} items / {} lbs", cont.capacity, cont.weight_capacity));
        ctx.send(fmt::format("  Closeable: {} | Closed: {} | Lockable: {} | Locked: {}",
            cont.closeable ? "yes" : "no",
            cont.closed ? "yes" : "no",
            cont.lockable ? "yes" : "no",
            cont.locked ? "yes" : "no"));
        if (cont.key_id.is_valid()) {
            ctx.send(fmt::format("  Key: {}:{}", cont.key_id.zone_id(), cont.key_id.local_id()));
        }

        auto* container = dynamic_cast<Container*>(target.get());
        if (container && !container->is_empty()) {
            ctx.send(fmt::format("  Contents ({} items):", container->contents_count()));
            int count = 0;
            for (const auto& item : container->get_contents()) {
                if (!item) continue;
                ctx.send(fmt::format("    [{}:{}] {}",
                    item->id().zone_id(), item->id().local_id(), item->short_description()));
                if (++count >= 10) {
                    ctx.send(fmt::format("    ... and {} more", container->contents_count() - count));
                    break;
                }
            }
        }
    }

    if (target->is_light_source()) {
        const auto& light = target->light_info();
        ctx.send(fmt::format("\n<b:white>Light Stats:</>"));
        ctx.send(fmt::format("  Duration: {} hours | Brightness: {} | Lit: {} | Permanent: {}",
            light.duration, light.brightness,
            light.lit ? "yes" : "no",
            light.permanent ? "yes" : "no"));
    }

    if (target->is_magic_item()) {
        ctx.send(fmt::format("\n<b:white>Magic Item Stats:</>"));
        ctx.send(fmt::format("  Spell Level: {}", target->spell_level()));
        const auto& spells = target->spell_ids();
        for (int i = 0; i < 3; ++i) {
            if (spells[i] > 0) {
                ctx.send(fmt::format("  Spell {}: {}", i + 1, spells[i]));
            }
        }
        if (target->is_wand() || target->is_staff()) {
            ctx.send(fmt::format("  Charges: {}/{}", target->charges(), target->max_charges()));
        }
    }

    // Flags
    const auto& flags = target->flags();
    if (!flags.empty()) {
        std::string flag_str;
        for (const auto& flag : flags) {
            if (!flag_str.empty()) flag_str += ", ";
            flag_str += std::string(magic_enum::enum_name(flag));
        }
        ctx.send(fmt::format("\n<b:white>Flags:</> <b:yellow>{}</>", flag_str));
    }

    // Effect flags
    const auto& effects = target->effect_flags();
    if (!effects.empty()) {
        std::string effect_str;
        for (const auto& effect : effects) {
            if (!effect_str.empty()) effect_str += ", ";
            effect_str += std::string(magic_enum::enum_name(effect));
        }
        ctx.send(fmt::format("<b:white>Effects:</> <b:green>{}</>", effect_str));
    }

    // Extra descriptions
    const auto& extras = target->get_all_extra_descriptions();
    if (!extras.empty()) {
        ctx.send(fmt::format("\n<b:white>Extra Descriptions ({}):</>", extras.size()));
        for (const auto& extra : extras) {
            std::string kw_str;
            for (const auto& kw : extra.keywords) {
                if (!kw_str.empty()) kw_str += ", ";
                kw_str += kw;
            }
            ctx.send(fmt::format("  [{}]", kw_str));
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_stat(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("stat <target>");
        ctx.send("Display statistics for a mobile or object.");
        ctx.send("Automatically determines whether target is a mobile or object.");
        ctx.send("\nRelated commands:");
        ctx.send("  rstat [room_id]  - Room statistics (default: current room)");
        ctx.send("  zstat [zone_id]  - Zone statistics (default: current zone)");
        ctx.send("  mstat <target>   - Mobile statistics");
        ctx.send("  ostat <target>   - Object statistics");
        ctx.send("  tstat [zone:id]  - Trigger statistics");
        return CommandResult::InvalidSyntax;
    }

    std::string search_term = ctx.command.join_args();
    std::string search_lower = search_term;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    // First check if it's an ID and try to find the entity
    if (looks_like_id(ctx.arg(0))) {
        auto entity_id = parse_stat_entity_id(ctx.arg(0));
        if (entity_id && entity_id->is_valid()) {
            // Check if it's a mobile
            std::shared_ptr<Mobile> mob = nullptr;
            WorldManager::instance().for_each_mobile([&](const std::shared_ptr<Mobile>& mobile) {
                if (!mob && mobile && mobile->id() == *entity_id) {
                    mob = mobile;
                }
            });
            if (mob) {
                return cmd_mstat(ctx);
            }

            // If not a mobile, assume it's an object
            return cmd_ostat(ctx);
        }
    }

    // Search by name - check current room first for both mobs and objects
    if (ctx.room) {
        // Check actors first
        for (const auto& actor : ctx.room->contents().actors) {
            auto mob = std::dynamic_pointer_cast<Mobile>(actor);
            if (!mob) continue;

            std::string mob_name = std::string(mob->short_description());
            std::string mob_name_lower = mob_name;
            std::transform(mob_name_lower.begin(), mob_name_lower.end(), mob_name_lower.begin(), ::tolower);

            if (mob_name_lower.find(search_lower) != std::string::npos) {
                return cmd_mstat(ctx);
            }

            for (const auto& kw : mob->keywords()) {
                std::string kw_lower = kw;
                std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                if (kw_lower.find(search_lower) != std::string::npos) {
                    return cmd_mstat(ctx);
                }
            }
        }

        // Check objects
        for (const auto& obj : ctx.room->contents().objects) {
            if (!obj) continue;

            std::string obj_name = std::string(obj->short_description());
            std::string obj_name_lower = obj_name;
            std::transform(obj_name_lower.begin(), obj_name_lower.end(), obj_name_lower.begin(), ::tolower);

            if (obj_name_lower.find(search_lower) != std::string::npos) {
                return cmd_ostat(ctx);
            }

            for (const auto& kw : obj->keywords()) {
                std::string kw_lower = kw;
                std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                if (kw_lower.find(search_lower) != std::string::npos) {
                    return cmd_ostat(ctx);
                }
            }
        }

        // Check player inventory
        for (const auto& item : ctx.actor->inventory().get_all_items()) {
            if (!item) continue;

            std::string obj_name = std::string(item->short_description());
            std::string obj_name_lower = obj_name;
            std::transform(obj_name_lower.begin(), obj_name_lower.end(), obj_name_lower.begin(), ::tolower);

            if (obj_name_lower.find(search_lower) != std::string::npos) {
                return cmd_ostat(ctx);
            }
        }
    }

    // Global search - try mobiles first, then objects
    bool found_mob = false;
    WorldManager::instance().for_each_mobile([&](const std::shared_ptr<Mobile>& mobile) {
        if (found_mob) return;

        std::string mob_name = std::string(mobile->short_description());
        std::string mob_name_lower = mob_name;
        std::transform(mob_name_lower.begin(), mob_name_lower.end(), mob_name_lower.begin(), ::tolower);

        if (mob_name_lower.find(search_lower) != std::string::npos) {
            found_mob = true;
            return;
        }

        for (const auto& kw : mobile->keywords()) {
            std::string kw_lower = kw;
            std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
            if (kw_lower.find(search_lower) != std::string::npos) {
                found_mob = true;
                return;
            }
        }
    });

    if (found_mob) {
        return cmd_mstat(ctx);
    }

    // Default to object search
    return cmd_ostat(ctx);
}

// =============================================================================
// Search Commands
// =============================================================================

Result<CommandResult> cmd_msearch(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("msearch <name>");
        ctx.send("Search for mobiles by name. Shows name and room location.");
        return CommandResult::InvalidSyntax;
    }

    std::string search_term = ctx.command.join_args();
    // Convert to lowercase for case-insensitive search
    std::string search_lower = search_term;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    ctx.send(fmt::format("<b:cyan>--- Mobiles matching '{}' ---</>", search_term));

    int count = 0;
    const int MAX_RESULTS = 100;

    WorldManager::instance().for_each_mobile([&](const std::shared_ptr<Mobile>& mobile) {
        if (count >= MAX_RESULTS) return;

        // Check name match (case-insensitive)
        std::string mob_name = std::string(mobile->short_description());
        std::string mob_name_lower = mob_name;
        std::transform(mob_name_lower.begin(), mob_name_lower.end(), mob_name_lower.begin(), ::tolower);

        // Also check keywords (iterate over the span)
        bool keyword_match = false;
        for (const auto& kw : mobile->keywords()) {
            std::string kw_lower = kw;
            std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
            if (kw_lower.find(search_lower) != std::string::npos) {
                keyword_match = true;
                break;
            }
        }

        if (mob_name_lower.find(search_lower) != std::string::npos || keyword_match) {

            auto room = mobile->current_room();
            std::string room_str = room ?
                fmt::format("{}:{} ({})", room->id().zone_id(), room->id().local_id(), room->name()) :
                "nowhere";

            ctx.send(fmt::format("  [{}:{}] {} - {}",
                mobile->id().zone_id(), mobile->id().local_id(),
                mob_name, room_str));
            count++;
        }
    });

    if (count == 0) {
        ctx.send("No mobiles found matching that name.");
    } else if (count >= MAX_RESULTS) {
        ctx.send(fmt::format("(Showing first {} results, search may have more)", MAX_RESULTS));
    } else {
        ctx.send(fmt::format("Found {} mobile(s).", count));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_osearch(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("osearch <name>");
        ctx.send("Search for objects by name. Shows name, holder (if any), and room.");
        return CommandResult::InvalidSyntax;
    }

    std::string search_term = ctx.command.join_args();
    // Convert to lowercase for case-insensitive search
    std::string search_lower = search_term;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    ctx.send(fmt::format("<b:cyan>--- Objects matching '{}' ---</>", search_term));

    int count = 0;
    const int MAX_RESULTS = 100;

    // Helper to check if object name matches
    auto matches_search = [&search_lower](const Object& obj) -> bool {
        std::string obj_name = std::string(obj.short_description());
        std::string obj_name_lower = obj_name;
        std::transform(obj_name_lower.begin(), obj_name_lower.end(), obj_name_lower.begin(), ::tolower);

        // Check keywords (iterate over the span)
        for (const auto& kw : obj.keywords()) {
            std::string kw_lower = kw;
            std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
            if (kw_lower.find(search_lower) != std::string::npos) {
                return true;
            }
        }

        return obj_name_lower.find(search_lower) != std::string::npos;
    };

    // Helper to display an object
    auto display_object = [&ctx, &count, MAX_RESULTS](
        const Object& obj, std::string_view holder, std::string_view location) {
        if (count >= MAX_RESULTS) return;

        std::string holder_str = holder.empty() ? "" : fmt::format(" [held by {}]", holder);
        ctx.send(fmt::format("  [{}:{}] {}{} - {}",
            obj.id().zone_id(), obj.id().local_id(),
            obj.short_description(), holder_str, location));
        count++;
    };

    // Search objects in rooms
    for (const auto& zone : WorldManager::instance().get_all_zones()) {
        for (const auto& room : WorldManager::instance().get_rooms_in_zone(zone->id())) {
            if (!room) continue;

            std::string room_str = fmt::format("{}:{} ({})",
                room->id().zone_id(), room->id().local_id(), room->name());

            // Objects directly in room
            for (const auto& obj : room->contents().objects) {
                if (!obj || !matches_search(*obj)) continue;
                display_object(*obj, "", room_str);

                // Check container contents
                if (obj->is_container()) {
                    auto* container = dynamic_cast<Container*>(obj.get());
                    if (container) {
                        for (const auto& inner : container->get_contents()) {
                            if (inner && matches_search(*inner)) {
                                display_object(*inner, obj->short_description(), room_str);
                            }
                        }
                    }
                }
            }

            // Objects on actors in room
            for (const auto& actor : room->contents().actors) {
                if (!actor) continue;

                std::string actor_name = std::string(actor->short_description());

                for (const auto& obj : actor->inventory().get_all_items()) {
                    if (!obj || !matches_search(*obj)) continue;
                    display_object(*obj, actor_name, room_str);

                    // Check container contents in inventory
                    if (obj->is_container()) {
                        auto* container = dynamic_cast<Container*>(obj.get());
                        if (container) {
                            for (const auto& inner : container->get_contents()) {
                                if (inner && matches_search(*inner)) {
                                    std::string holder = fmt::format("{}'s {}", actor_name, obj->short_description());
                                    display_object(*inner, holder, room_str);
                                }
                            }
                        }
                    }
                }
            }

            if (count >= MAX_RESULTS) break;
        }
        if (count >= MAX_RESULTS) break;
    }

    if (count == 0) {
        ctx.send("No objects found matching that name.");
    } else if (count >= MAX_RESULTS) {
        ctx.send(fmt::format("(Showing first {} results, search may have more)", MAX_RESULTS));
    } else {
        ctx.send(fmt::format("Found {} object(s).", count));
    }

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

    Commands()
        .command("load", cmd_load)
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

    // Trigger management commands
    Commands()
        .command("tstat", cmd_tstat)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    Commands()
        .command("treload", cmd_treload)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    Commands()
        .command("tlist", cmd_tlist)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .build();

    // Entity stat commands
    Commands()
        .command("stat", cmd_stat)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("rstat", cmd_rstat)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("zstat", cmd_zstat)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("mstat", cmd_mstat)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("ostat", cmd_ostat)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    // Search commands for debugging
    Commands()
        .command("msearch", cmd_msearch)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("osearch", cmd_osearch)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    return Success();
}

} // namespace AdminCommands
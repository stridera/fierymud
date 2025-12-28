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
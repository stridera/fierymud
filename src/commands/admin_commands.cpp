#include "admin_commands.hpp"
#include "information_commands.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../core/money.hpp"
#include "../core/shopkeeper.hpp"
#include "../scripting/script_engine.hpp"
#include "../scripting/trigger_manager.hpp"
#include "../world/weather.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>
#include <sstream>

namespace AdminCommands {

// Forward declarations for helper functions used by multiple commands
std::optional<EntityId> parse_stat_entity_id(std::string_view id_str,
                                              std::optional<int> current_zone = std::nullopt);
bool looks_like_id(std::string_view str);

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
    if (ctx.arg_count() == 0) {
        ctx.show_help();
        return CommandResult::Success;
    }

    auto target_str = std::string{ctx.arg(0)};
    EntityId room_id;
    bool found_room = false;

    // Get current zone for shorthand ID parsing
    std::optional<int> current_zone;
    if (ctx.room) {
        current_zone = static_cast<int>(ctx.room->id().zone_id());
    }

    // Check for "home" keyword - teleport to player's start room
    std::string target_lower = target_str;
    std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);
    if (target_lower == "home") {
        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (player) {
            room_id = player->start_room();
            if (room_id.is_valid()) {
                found_room = true;
                ctx.send_info(fmt::format("Teleporting to your home room {}:{}.",
                    room_id.zone_id(), room_id.local_id()));
            } else {
                ctx.send_error("You don't have a home room set.");
                return CommandResult::InvalidState;
            }
        } else {
            ctx.send_error("Only players have home rooms.");
            return CommandResult::InvalidTarget;
        }
    }

    // First, check if it looks like a room ID (zone:local_id or single number)
    if (!found_room && looks_like_id(target_str)) {
        auto room_id_opt = parse_stat_entity_id(target_str, current_zone);
        if (room_id_opt && room_id_opt->is_valid()) {
            // Verify the room exists
            auto room = WorldManager::instance().get_room(*room_id_opt);
            if (room) {
                room_id = *room_id_opt;
                found_room = true;
            }
        }
    }

    // If not a valid room ID, try to find a player by name
    if (!found_room) {
        auto target = ctx.find_actor_global(target_str);
        if (target) {
            auto target_room = target->current_room();
            if (target_room) {
                room_id = target_room->id();
                found_room = true;
                ctx.send_info(fmt::format("Found player {} in room {}:{}.",
                    target->display_name(), room_id.zone_id(), room_id.local_id()));
            } else {
                ctx.send_error(fmt::format("Player {} is not in a valid room.", target->display_name()));
                return CommandResult::InvalidState;
            }
        }
    }

    // If still not found, search for a mobile by name globally
    if (!found_room) {
        std::string search_lower = target_str;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        std::shared_ptr<Mobile> found_mob = nullptr;

        // Search all mobiles
        WorldManager::instance().for_each_mobile([&](const std::shared_ptr<Mobile>& mobile) {
            if (found_mob) return; // Already found

            // Check short description
            std::string mob_name = std::string(mobile->short_description());
            std::string mob_name_lower = mob_name;
            std::transform(mob_name_lower.begin(), mob_name_lower.end(), mob_name_lower.begin(), ::tolower);

            if (mob_name_lower.find(search_lower) != std::string::npos) {
                found_mob = mobile;
                return;
            }

            // Check keywords
            for (const auto& kw : mobile->keywords()) {
                std::string kw_lower = kw;
                std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                if (kw_lower.find(search_lower) != std::string::npos) {
                    found_mob = mobile;
                    return;
                }
            }
        });

        if (found_mob) {
            auto mob_room = found_mob->current_room();
            if (mob_room) {
                room_id = mob_room->id();
                found_room = true;
                ctx.send_info(fmt::format("Found mobile {} in room {}:{}.",
                    found_mob->short_description(), room_id.zone_id(), room_id.local_id()));
            } else {
                ctx.send_error(fmt::format("Mobile {} is not in a valid room.", found_mob->short_description()));
                return CommandResult::InvalidState;
            }
        }
    }

    // If we still haven't found a valid target, error out
    if (!found_room || !room_id.is_valid()) {
        ctx.send_error(fmt::format("No room, player, or mobile found matching '{}'.", target_str));
        return CommandResult::InvalidTarget;
    }

    // Perform the teleport
    auto result = WorldManager::instance().move_actor_to_room(ctx.actor, room_id);
    if (!result.success) {
        ctx.send_error(result.failure_reason);
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("Teleported to room {}:{}.", room_id.zone_id(), room_id.local_id()));

    // Show the room we arrived in (create context with no args and updated room)
    CommandContext look_ctx = ctx;
    look_ctx.room = ctx.actor->current_room();
    look_ctx.command.arguments.clear();
    look_ctx.command.full_argument_string.clear();
    InformationCommands::cmd_look(look_ctx);

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

    // Show the room to the teleported player
    ctx.send_to_actor(target, "You have been teleported!");
    CommandContext target_ctx = ctx;
    target_ctx.actor = target;
    target_ctx.room = target->current_room();
    InformationCommands::cmd_look(target_ctx);

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

    // Show the room to the summoned player
    ctx.send_to_actor(target, "You have been summoned!");
    CommandContext target_ctx = ctx;
    target_ctx.actor = target;
    target_ctx.room = target->current_room();
    InformationCommands::cmd_look(target_ctx);

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
        ctx.send_usage("load <obj|mob> <zone:id | id>");
        ctx.send_info("Examples: load obj 30:31, load mob 0 (uses current zone)");
        return CommandResult::InvalidSyntax;
    }

    std::string type_str{ctx.arg(0)};

    // Get current zone for shorthand ID parsing
    std::optional<int> current_zone;
    if (ctx.room) {
        current_zone = static_cast<int>(ctx.room->id().zone_id());
    }

    // Parse entity ID (accepts "id" or "zone:id")
    auto prototype_id_opt = parse_stat_entity_id(ctx.arg(1), current_zone);
    if (!prototype_id_opt || !prototype_id_opt->is_valid()) {
        ctx.send_error("Invalid ID format. Use zone:id (e.g., 30:31) or just id.");
        return CommandResult::InvalidSyntax;
    }

    EntityId prototype_id = *prototype_id_opt;
    int zone_id = static_cast<int>(prototype_id.zone_id());
    int local_id = static_cast<int>(prototype_id.local_id());

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

    // Get current zone for shorthand ID parsing
    std::optional<int> current_zone;
    if (ctx.room) {
        current_zone = static_cast<int>(ctx.room->id().zone_id());
    }

    // Parse entity ID from argument (accepts "id" or "zone:id")
    auto entity_id_opt = parse_stat_entity_id(ctx.arg(0), current_zone);
    if (!entity_id_opt || !entity_id_opt->is_valid()) {
        ctx.send_error("Invalid entity ID format. Use zone:id (e.g., 30:0) or just id.");
        return CommandResult::InvalidSyntax;
    }

    EntityId entity_id = *entity_id_opt;

    // Show triggers for this entity
    ctx.send(fmt::format("<b:cyan>--- Triggers for Entity {}:{} ---</>",
        entity_id.zone_id(), entity_id.local_id()));

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

    if (entity_id.local_id() == 0) {
        auto world_triggers = trigger_mgr.get_world_triggers(static_cast<int>(entity_id.zone_id()));
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
 * If only a single number is provided and current_zone is specified, uses that zone.
 * Returns std::nullopt if parsing fails.
 */
std::optional<EntityId> parse_stat_entity_id(std::string_view id_str,
                                              std::optional<int> current_zone) {
    auto colon_pos = id_str.find(':');
    if (colon_pos != std::string::npos) {
        // Full zone:id format
        try {
            int zone_id = std::stoi(std::string(id_str.substr(0, colon_pos)));
            int local_id = std::stoi(std::string(id_str.substr(colon_pos + 1)));
            return EntityId(zone_id, local_id);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    // Single number format - use current zone if provided
    try {
        int local_id = std::stoi(std::string(id_str));
        if (current_zone.has_value()) {
            return EntityId(*current_zone, local_id);
        }
        // Fallback: treat as legacy format (zone 0 or full ID)
        return EntityId{static_cast<std::uint32_t>(local_id), 0};
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
        // Get current zone for shorthand ID parsing
        std::optional<int> current_zone;
        if (ctx.room) {
            current_zone = static_cast<int>(ctx.room->id().zone_id());
        }

        // Parse room ID from argument (accepts "id" or "zone:id")
        auto room_id_opt = parse_stat_entity_id(ctx.arg(0), current_zone);
        if (!room_id_opt || !room_id_opt->is_valid()) {
            ctx.send_error("Invalid room ID format. Use zone:id (e.g., 30:89) or just id.");
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
        ctx.send_usage("mstat <zone:id | id | name>");
        ctx.send("Display statistics for a mobile by ID or name.");
        ctx.send("  mstat 30:0       - Show mobile with ID 30:0");
        ctx.send("  mstat 0          - Show mobile 0 in current zone");
        ctx.send("  mstat guard      - Find and show guard mobile");
        return CommandResult::InvalidSyntax;
    }

    std::string search_term = ctx.command.join_args();
    std::shared_ptr<Mobile> target = nullptr;

    // Get current zone for shorthand ID parsing
    std::optional<int> current_zone;
    if (ctx.room) {
        current_zone = static_cast<int>(ctx.room->id().zone_id());
    }

    // Check if it looks like an ID (zone:id format or just a number)
    if (looks_like_id(ctx.arg(0))) {
        auto entity_id = parse_stat_entity_id(ctx.arg(0), current_zone);
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
    ctx.send(fmt::format("  Level: {} | HP: {}/{} | Stamina: {}/{}",
        stats.level, stats.hit_points, stats.max_hit_points,
        stats.stamina, stats.max_stamina));
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
    // Display bare hand damage - handle special cases where dice are invalid
    const int dam_num = target->bare_hand_damage_dice_num();
    const int dam_size = target->bare_hand_damage_dice_size();
    const int dam_bonus = target->bare_hand_damage_dice_bonus();
    if (dam_num <= 0 || dam_size <= 0) {
        // Invalid dice values - mob likely uses special attacks/spells
        if (dam_bonus > 0) {
            ctx.send(fmt::format("  Bare Hand Damage: +{} (special attack)",
                dam_bonus));
        } else {
            ctx.send("  Bare Hand Damage: None (uses abilities/spells)");
        }
    } else {
        ctx.send(fmt::format("  Bare Hand Damage: {}d{}+{}",
            dam_num, dam_size, dam_bonus));
    }
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
    // Note: Shopkeeper flag is already included in the loop above via MobFlag::Shopkeeper
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

    // Trigger/Script information
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();
    if (trigger_mgr.is_initialized()) {
        // Get triggers by prototype ID (all instances of this mob type share triggers)
        auto trigger_set = trigger_mgr.get_mob_triggers(target->prototype_id());
        if (!trigger_set.empty()) {
            ctx.send(fmt::format("\n<b:white>Script Information ({} triggers):</>", trigger_set.size()));
            int index = 1;
            for (const auto& trigger : trigger_set) {
                if (!trigger) continue;
                // Format: index) [zone:id] name - flags (for use with tstat zone:id)
                ctx.send(fmt::format("  {}) <b:yellow>[{}:{}]</> {} - <b:green>{}</>",
                    index++,
                    trigger->zone_id.value_or(0),
                    trigger->id,
                    trigger->name,
                    trigger->flags_string()));
            }
        } else {
            ctx.send("\n<b:white>Script Information:</> None.");
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_ostat(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_usage("ostat <zone:id | id | name>");
        ctx.send("Display statistics for an object by ID or name.");
        ctx.send("  ostat 30:31      - Show object with ID 30:31");
        ctx.send("  ostat 31         - Show object 31 in current zone");
        ctx.send("  ostat sword      - Find and show sword object");
        return CommandResult::InvalidSyntax;
    }

    std::string search_term = ctx.command.join_args();
    std::shared_ptr<Object> target = nullptr;
    std::string location_info;

    // Get current zone for shorthand ID parsing
    std::optional<int> current_zone;
    if (ctx.room) {
        current_zone = static_cast<int>(ctx.room->id().zone_id());
    }

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
        auto entity_id = parse_stat_entity_id(ctx.arg(0), current_zone);
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
        if (cont.weight_reduction > 0) {
            ctx.send(fmt::format("  Weight Reduction: {}% (bag of holding)", cont.weight_reduction));
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

    // Trigger/Script information for objects
    auto& obj_trigger_mgr = FieryMUD::TriggerManager::instance();
    if (obj_trigger_mgr.is_initialized()) {
        auto trigger_set = obj_trigger_mgr.get_object_triggers(target->id());
        if (!trigger_set.empty()) {
            ctx.send(fmt::format("\n<b:white>Script Information ({} triggers):</>", trigger_set.size()));
            int index = 1;
            for (const auto& trigger : trigger_set) {
                if (!trigger) continue;
                // Format: index) [zone:id] name - flags (for use with tstat zone:id)
                ctx.send(fmt::format("  {}) <b:yellow>[{}:{}]</> {} - <b:green>{}</>",
                    index++,
                    trigger->zone_id.value_or(0),
                    trigger->id,
                    trigger->name,
                    trigger->flags_string()));
            }
        } else {
            ctx.send("\n<b:white>Script Information:</> None.");
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_sstat(const CommandContext &ctx) {
    // Determine which room to check for shopkeepers
    std::shared_ptr<Room> target_room;

    if (ctx.arg_count() == 0) {
        // No arguments - use current room
        if (!ctx.room) {
            ctx.send_error("You are not in a room.");
            return CommandResult::InvalidState;
        }
        target_room = ctx.room;
    } else {
        // Parse room ID argument
        std::optional<int> current_zone;
        if (ctx.room) {
            current_zone = static_cast<int>(ctx.room->id().zone_id());
        }

        auto room_id = parse_stat_entity_id(ctx.arg(0), current_zone);
        if (!room_id || !room_id->is_valid()) {
            ctx.send_error(fmt::format("Invalid room ID: {}", ctx.arg(0)));
            return CommandResult::InvalidSyntax;
        }

        target_room = WorldManager::instance().get_room(*room_id);
        if (!target_room) {
            ctx.send_error(fmt::format("Room {} does not exist.", *room_id));
            return CommandResult::ResourceError;
        }
    }

    // Look for shopkeepers in the target room
    std::shared_ptr<Mobile> shopkeeper_mobile;
    for (const auto& actor : target_room->contents().actors) {
        if (auto mobile = std::dynamic_pointer_cast<Mobile>(actor)) {
            if (mobile->is_shopkeeper()) {
                shopkeeper_mobile = mobile;
                break;
            }
        }
    }

    if (!shopkeeper_mobile) {
        ctx.send_error(fmt::format("There are no shopkeepers in room {}.", target_room->id()));
        return CommandResult::ResourceError;
    }

    // Get the shop from ShopManager
    auto& shop_manager = ShopManager::instance();
    EntityId lookup_id = shopkeeper_mobile->prototype_id();
    const auto* shop = shop_manager.get_shopkeeper(lookup_id);

    if (!shop) {
        ctx.send_error(fmt::format("Mob {} is marked as shopkeeper but has no shop data.",
            shopkeeper_mobile->short_description()));
        return CommandResult::ResourceError;
    }

    // Display shop statistics
    ctx.send(fmt::format("<b:cyan>============ Shop Statistics ============</>"));
    ctx.send(fmt::format("<b:white>Shop Name:</> {}", shop->get_name()));
    ctx.send(fmt::format("<b:white>Shop ID:</> {}", shop->get_shop_id()));
    ctx.send(fmt::format("<b:white>Shopkeeper:</> {} [{}]",
        shopkeeper_mobile->short_description(), shopkeeper_mobile->prototype_id()));

    ctx.send(fmt::format("\n<b:white>Pricing:</>"));
    ctx.send(fmt::format("  Buy Rate:  {:.1f}x (customers pay this multiplier)",
        shop->get_buy_rate()));
    ctx.send(fmt::format("  Sell Rate: {:.1f}x (customers receive this multiplier)",
        shop->get_sell_rate()));

    // Show items
    auto items = shop->get_available_items();
    ctx.send(fmt::format("\n<b:white>Items for Sale:</> {} item(s)", items.size()));
    if (!items.empty()) {
        ctx.send("  ##   ID          Stock  Level  Cost        Name");
        ctx.send("  ---  ----------  -----  -----  ----------  -----------------------");
        int idx = 1;
        for (const auto& item : items) {
            auto price = fiery::Money::from_copper(item.cost);
            std::string stock_str = (item.stock == -1) ? "  -  " : fmt::format("{:>5}", item.stock);
            ctx.send(fmt::format("  {:>2})  [{:>4}:{:<4}]  {}  {:>5}  {:>10}  {}",
                idx++,
                item.prototype_id.zone_id(), item.prototype_id.local_id(),
                stock_str, item.level,
                price.to_brief(),
                item.name));
        }
    }

    // Show mobs
    if (shop->sells_mobs()) {
        auto mobs = shop->get_available_mobs();
        ctx.send(fmt::format("\n<b:white>Pets/Mounts for Sale:</> {} mob(s)", mobs.size()));
        if (!mobs.empty()) {
            ctx.send("  ##   ID          Stock  Level  Cost        Name");
            ctx.send("  ---  ----------  -----  -----  ----------  -----------------------");
            int idx = 1;
            for (const auto& mob : mobs) {
                auto price = fiery::Money::from_copper(mob.cost);
                std::string stock_str = (mob.stock == -1) ? "  -  " : fmt::format("{:>5}", mob.stock);
                ctx.send(fmt::format("  {:>2})  [{:>4}:{:<4}]  {}  {:>5}  {:>10}  {}",
                    idx++,
                    mob.prototype_id.zone_id(), mob.prototype_id.local_id(),
                    stock_str, mob.level,
                    price.to_brief(),
                    mob.name));
            }
        }
    }

    ctx.send(fmt::format("<b:cyan>=========================================</>"));
    return CommandResult::Success;
}

Result<CommandResult> cmd_slist(const CommandContext &ctx) {
    // Parse optional zone filter
    int filter_zone = -1;

    for (size_t i = 0; i < ctx.arg_count(); ++i) {
        std::string arg{ctx.arg(i)};

        if ((arg == "--zone" || arg == "-z") && i + 1 < ctx.arg_count()) {
            try {
                filter_zone = std::stoi(std::string{ctx.arg(++i)});
            } catch (const std::exception&) {
                ctx.send_error("Invalid zone ID.");
                return CommandResult::InvalidSyntax;
            }
        } else if (!arg.starts_with("-")) {
            // Treat bare number as zone filter
            try {
                filter_zone = std::stoi(arg);
            } catch (const std::exception&) {
                ctx.send_error(fmt::format("Invalid zone ID: {}", arg));
                return CommandResult::InvalidSyntax;
            }
        }
    }

    // Get all shops from ShopManager
    auto& shop_manager = ShopManager::instance();

    // Collect all registered shops
    std::vector<std::tuple<EntityId, std::string, int, int, bool>> shop_list;

    shop_manager.for_each_shop([&](const EntityId& keeper_id, const Shopkeeper& shop) {
        // Check zone filter
        if (filter_zone >= 0 && static_cast<int>(keeper_id.zone_id()) != filter_zone) {
            return;
        }

        int item_count = static_cast<int>(shop.get_available_items().size());
        int mob_count = shop.sells_mobs() ? static_cast<int>(shop.get_available_mobs().size()) : 0;

        // Get shopkeeper name from the shop
        shop_list.emplace_back(keeper_id, shop.get_name(),
                               item_count, mob_count, shop.sells_mobs());
    });

    // Sort by zone, then by local ID
    std::sort(shop_list.begin(), shop_list.end(),
              [](const auto& a, const auto& b) {
                  if (std::get<0>(a).zone_id() != std::get<0>(b).zone_id()) {
                      return std::get<0>(a).zone_id() < std::get<0>(b).zone_id();
                  }
                  return std::get<0>(a).local_id() < std::get<0>(b).local_id();
              });

    if (shop_list.empty()) {
        if (filter_zone >= 0) {
            ctx.send(fmt::format("No shops found in zone {}.", filter_zone));
        } else {
            ctx.send("No shops found.");
        }
        return CommandResult::Success;
    }

    // Display header
    if (filter_zone >= 0) {
        ctx.send(fmt::format("<b:cyan>===== Shops in Zone {} =====</>", filter_zone));
    } else {
        ctx.send("<b:cyan>===== All Shops =====</>");
    }
    ctx.send("");
    ctx.send("  ID          Items  Mobs  Shopkeeper");
    ctx.send("  ----------  -----  ----  ----------------------------------");

    for (const auto& [keeper_id, name, items, mobs, sells_mobs] : shop_list) {
        std::string mob_str = sells_mobs ? fmt::format("{:>4}", mobs) : "   -";
        ctx.send(fmt::format("  [{:>4}:{:<4}]  {:>5}  {}  {}",
            keeper_id.zone_id(), keeper_id.local_id(),
            items, mob_str, name));
    }

    ctx.send("");
    ctx.send(fmt::format("Total: {} shop(s)", shop_list.size()));

    return CommandResult::Success;
}

Result<CommandResult> cmd_stat(const CommandContext &ctx) {
    // No arguments - default to room stats
    if (ctx.arg_count() == 0) {
        return cmd_rstat(ctx);
    }

    std::string search_term = ctx.command.join_args();
    std::string search_lower = search_term;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    // Get current zone for shorthand ID parsing
    std::optional<int> current_zone;
    if (ctx.room) {
        current_zone = static_cast<int>(ctx.room->id().zone_id());
    }

    // First check if it's an ID and try to find the entity
    if (looks_like_id(ctx.arg(0))) {
        auto entity_id = parse_stat_entity_id(ctx.arg(0), current_zone);
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
// List Command
// =============================================================================

Result<CommandResult> cmd_list(const CommandContext &ctx) {
    // No arguments - list entities in current room
    if (ctx.arg_count() == 0) {
        if (!ctx.room) {
            ctx.send_error("You are not in a room.");
            return CommandResult::InvalidState;
        }

        ctx.send(fmt::format("<b:cyan>--- {} [{}:{}] ---</>",
            ctx.room->name(), ctx.room->id().zone_id(), ctx.room->id().local_id()));

        const auto& contents = ctx.room->contents();

        // List actors (mobs and players)
        if (!contents.actors.empty()) {
            ctx.send(fmt::format("\n<b:yellow>Actors ({}):</>", contents.actors.size()));
            for (const auto& actor : contents.actors) {
                if (!actor) continue;
                auto mob = std::dynamic_pointer_cast<Mobile>(actor);
                std::string type_str = mob ? "MOB" : "PLAYER";
                ctx.send(fmt::format("  [{}:{}] {} <dim>({})</>",
                    actor->id().zone_id(), actor->id().local_id(),
                    actor->short_description(), type_str));
            }
        } else {
            ctx.send("\n<dim>No actors in room.</>");
        }

        // List objects
        if (!contents.objects.empty()) {
            ctx.send(fmt::format("\n<b:green>Objects ({}):</>", contents.objects.size()));
            for (const auto& obj : contents.objects) {
                if (!obj) continue;
                ctx.send(fmt::format("  [{}:{}] {} <dim>({})</>",
                    obj->id().zone_id(), obj->id().local_id(),
                    obj->short_description(), obj->type()));
            }
        } else {
            ctx.send("\n<dim>No objects in room.</>");
        }

        // Show exits
        auto exits = ctx.room->get_available_exits();
        if (!exits.empty()) {
            ctx.send("\n<b:white>Exits:</>");
            for (const auto& dir : exits) {
                const auto* exit = ctx.room->get_exit(dir);
                if (exit && exit->to_room.is_valid()) {
                    std::string door_str = exit->has_door ?
                        fmt::format(" [{}{}]",
                            exit->is_closed ? "closed" : "open",
                            exit->is_locked ? ", locked" : "") : "";
                    ctx.send(fmt::format("  {} → {}:{}{}",
                        RoomUtils::get_direction_name(dir),
                        exit->to_room.zone_id(), exit->to_room.local_id(),
                        door_str));
                }
            }
        }

        return CommandResult::Success;
    }

    // With target argument - delegate to stat command for details
    return cmd_stat(ctx);
}

// =============================================================================
// Search Commands
// =============================================================================

Result<CommandResult> cmd_msearch(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.show_help();
        return CommandResult::Success;
    }

    // Parse arguments
    int filter_zone = -1;
    int filter_level_min = -1;
    int filter_level_max = -1;
    std::string search_name;

    for (size_t i = 0; i < ctx.arg_count(); ++i) {
        std::string arg{ctx.arg(i)};

        if (arg == "--zone" && i + 1 < ctx.arg_count()) {
            try {
                filter_zone = std::stoi(std::string{ctx.arg(++i)});
            } catch (const std::exception&) {
                ctx.send_error("Invalid zone ID.");
                return CommandResult::InvalidSyntax;
            }
        } else if (arg == "--level" && i + 1 < ctx.arg_count()) {
            std::string level_str{ctx.arg(++i)};
            auto dash_pos = level_str.find('-');
            try {
                if (dash_pos != std::string::npos) {
                    filter_level_min = std::stoi(level_str.substr(0, dash_pos));
                    filter_level_max = std::stoi(level_str.substr(dash_pos + 1));
                } else {
                    filter_level_min = filter_level_max = std::stoi(level_str);
                }
            } catch (const std::exception&) {
                ctx.send_error("Invalid level format. Use --level N or --level N-M.");
                return CommandResult::InvalidSyntax;
            }
        } else if (!arg.starts_with("--")) {
            // Not an option, treat as search name
            if (!search_name.empty()) search_name += " ";
            search_name += arg;
        }
    }

    std::string search_lower = search_name;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    // Build filter description
    std::string filter_desc;
    if (!search_name.empty()) filter_desc += fmt::format("name '{}'", search_name);
    if (filter_zone >= 0) {
        if (!filter_desc.empty()) filter_desc += ", ";
        filter_desc += fmt::format("zone {}", filter_zone);
    }
    if (filter_level_min >= 0) {
        if (!filter_desc.empty()) filter_desc += ", ";
        if (filter_level_min == filter_level_max) {
            filter_desc += fmt::format("level {}", filter_level_min);
        } else {
            filter_desc += fmt::format("level {}-{}", filter_level_min, filter_level_max);
        }
    }
    if (filter_desc.empty()) filter_desc = "all";

    ctx.send(fmt::format("<b:cyan>--- Mobiles matching: {} ---</>", filter_desc));

    int count = 0;
    const int MAX_RESULTS = 100;

    WorldManager::instance().for_each_mobile([&](const std::shared_ptr<Mobile>& mobile) {
        if (count >= MAX_RESULTS) return;

        // Zone filter
        if (filter_zone >= 0 && static_cast<int>(mobile->id().zone_id()) != filter_zone) return;

        // Level filter
        if (filter_level_min >= 0) {
            int level = mobile->stats().level;
            if (level < filter_level_min || level > filter_level_max) return;
        }

        // Name filter (if provided)
        if (!search_lower.empty()) {
            std::string mob_name = std::string(mobile->short_description());
            std::string mob_name_lower = mob_name;
            std::transform(mob_name_lower.begin(), mob_name_lower.end(), mob_name_lower.begin(), ::tolower);

            bool matched = mob_name_lower.find(search_lower) != std::string::npos;
            if (!matched) {
                for (const auto& kw : mobile->keywords()) {
                    std::string kw_lower = kw;
                    std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                    if (kw_lower.find(search_lower) != std::string::npos) {
                        matched = true;
                        break;
                    }
                }
            }
            if (!matched) return;
        }

        auto room = mobile->current_room();
        std::string room_str = room ?
            fmt::format("{}:{} ({})", room->id().zone_id(), room->id().local_id(), room->name()) :
            "nowhere";

        ctx.send(fmt::format("  [{}:{}] {} (L{}) - {}",
            mobile->id().zone_id(), mobile->id().local_id(),
            mobile->short_description(), mobile->stats().level, room_str));
        count++;
    });

    if (count == 0) {
        ctx.send("No mobiles found matching criteria.");
    } else if (count >= MAX_RESULTS) {
        ctx.send(fmt::format("(Showing first {} results, search may have more)", MAX_RESULTS));
    } else {
        ctx.send(fmt::format("Found {} mobile(s).", count));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_osearch(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.show_help();
        return CommandResult::Success;
    }

    // Parse arguments
    int filter_zone = -1;
    int filter_level_min = -1;
    int filter_level_max = -1;
    std::string filter_type;
    std::string search_name;

    for (size_t i = 0; i < ctx.arg_count(); ++i) {
        std::string arg{ctx.arg(i)};

        if (arg == "--zone" && i + 1 < ctx.arg_count()) {
            try {
                filter_zone = std::stoi(std::string{ctx.arg(++i)});
            } catch (const std::exception&) {
                ctx.send_error("Invalid zone ID.");
                return CommandResult::InvalidSyntax;
            }
        } else if (arg == "--level" && i + 1 < ctx.arg_count()) {
            std::string level_str{ctx.arg(++i)};
            auto dash_pos = level_str.find('-');
            try {
                if (dash_pos != std::string::npos) {
                    filter_level_min = std::stoi(level_str.substr(0, dash_pos));
                    filter_level_max = std::stoi(level_str.substr(dash_pos + 1));
                } else {
                    filter_level_min = filter_level_max = std::stoi(level_str);
                }
            } catch (const std::exception&) {
                ctx.send_error("Invalid level format. Use --level N or --level N-M.");
                return CommandResult::InvalidSyntax;
            }
        } else if (arg == "--type" && i + 1 < ctx.arg_count()) {
            filter_type = std::string{ctx.arg(++i)};
            std::transform(filter_type.begin(), filter_type.end(), filter_type.begin(), ::tolower);
        } else if (!arg.starts_with("--")) {
            // Not an option, treat as search name
            if (!search_name.empty()) search_name += " ";
            search_name += arg;
        }
    }

    std::string search_lower = search_name;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    // Build filter description
    std::string filter_desc;
    if (!search_name.empty()) filter_desc += fmt::format("name '{}'", search_name);
    if (filter_zone >= 0) {
        if (!filter_desc.empty()) filter_desc += ", ";
        filter_desc += fmt::format("zone {}", filter_zone);
    }
    if (!filter_type.empty()) {
        if (!filter_desc.empty()) filter_desc += ", ";
        filter_desc += fmt::format("type '{}'", filter_type);
    }
    if (filter_level_min >= 0) {
        if (!filter_desc.empty()) filter_desc += ", ";
        if (filter_level_min == filter_level_max) {
            filter_desc += fmt::format("level {}", filter_level_min);
        } else {
            filter_desc += fmt::format("level {}-{}", filter_level_min, filter_level_max);
        }
    }
    if (filter_desc.empty()) filter_desc = "all";

    ctx.send(fmt::format("<b:cyan>--- Objects matching: {} ---</>", filter_desc));

    int count = 0;
    const int MAX_RESULTS = 100;

    // Helper to check if object matches all criteria
    auto matches_criteria = [&](const Object& obj) -> bool {
        // Zone filter
        if (filter_zone >= 0 && static_cast<int>(obj.id().zone_id()) != filter_zone) return false;

        // Level filter
        if (filter_level_min >= 0) {
            int level = obj.level();
            if (level < filter_level_min || level > filter_level_max) return false;
        }

        // Type filter
        if (!filter_type.empty()) {
            std::string obj_type = fmt::format("{}", obj.type());
            std::transform(obj_type.begin(), obj_type.end(), obj_type.begin(), ::tolower);
            if (obj_type.find(filter_type) == std::string::npos) return false;
        }

        // Name filter (if provided)
        if (!search_lower.empty()) {
            std::string obj_name = std::string(obj.short_description());
            std::string obj_name_lower = obj_name;
            std::transform(obj_name_lower.begin(), obj_name_lower.end(), obj_name_lower.begin(), ::tolower);

            bool matched = obj_name_lower.find(search_lower) != std::string::npos;
            if (!matched) {
                for (const auto& kw : obj.keywords()) {
                    std::string kw_lower = kw;
                    std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
                    if (kw_lower.find(search_lower) != std::string::npos) {
                        matched = true;
                        break;
                    }
                }
            }
            if (!matched) return false;
        }

        return true;
    };

    // Helper to display an object
    auto display_object = [&ctx, &count, MAX_RESULTS](
        const Object& obj, std::string_view holder, std::string_view location) {
        if (count >= MAX_RESULTS) return;

        std::string holder_str = holder.empty() ? "" : fmt::format(" [held by {}]", holder);
        ctx.send(fmt::format("  [{}:{}] {} ({}, L{}){} - {}",
            obj.id().zone_id(), obj.id().local_id(),
            obj.short_description(), obj.type(), obj.level(),
            holder_str, location));
        count++;
    };

    // Search objects in rooms
    for (const auto& zone : WorldManager::instance().get_all_zones()) {
        // Zone filter optimization
        if (filter_zone >= 0 && static_cast<int>(zone->id().zone_id()) != filter_zone) continue;

        for (const auto& room : WorldManager::instance().get_rooms_in_zone(zone->id())) {
            if (!room) continue;

            std::string room_str = fmt::format("{}:{} ({})",
                room->id().zone_id(), room->id().local_id(), room->name());

            // Objects directly in room
            for (const auto& obj : room->contents().objects) {
                if (!obj || !matches_criteria(*obj)) continue;
                display_object(*obj, "", room_str);

                // Check container contents
                if (obj->is_container()) {
                    auto* container = dynamic_cast<Container*>(obj.get());
                    if (container) {
                        for (const auto& inner : container->get_contents()) {
                            if (inner && matches_criteria(*inner)) {
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
                    if (!obj || !matches_criteria(*obj)) continue;
                    display_object(*obj, actor_name, room_str);

                    // Check container contents in inventory
                    if (obj->is_container()) {
                        auto* container = dynamic_cast<Container*>(obj.get());
                        if (container) {
                            for (const auto& inner : container->get_contents()) {
                                if (inner && matches_criteria(*inner)) {
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
        ctx.send("No objects found matching criteria.");
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
        .alias("go")
        .category("Admin")
        .privilege(PrivilegeLevel::God)
        .description("Teleport to a room, player, or mobile")
        .usage("goto <target>")
        .help(
            "<b:yellow>Target can be:</>\n"
            "  home        - Teleport to your home room\n"
            "  30:89       - Teleport to room 30:89 (zone:id format)\n"
            "  89          - Teleport to room 89 in current zone\n"
            "  playerName  - Teleport to a player's location\n"
            "  mobName     - Teleport to first matching mobile's location")
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

    Commands()
        .command("sstat", cmd_sstat)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("Display shop statistics for shopkeeper in a room")
        .usage("sstat [zone:id | id]")
        .help(
            "Display shop statistics for a shopkeeper.\n"
            "  sstat          - Show shop in current room\n"
            "  sstat 91       - Show shop in room 91 of current zone\n"
            "  sstat 30:91    - Show shop in room 30:91")
        .build();

    Commands()
        .command("slist", cmd_slist)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("List all shops with optional zone filter")
        .usage("slist [zone]")
        .help(
            "List all shops in the game.\n"
            "  slist          - List all shops\n"
            "  slist 30       - List shops in zone 30\n"
            "  slist --zone 30 - Same as above")
        .build();

    // Search commands for debugging
    Commands()
        .command("msearch", cmd_msearch)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("Search for mobiles by name with optional filters")
        .usage("msearch [options] <name>")
        .help(
            "<b:yellow>Options:</>\n"
            "  --zone <id>      Filter by zone ID\n"
            "  --level <n>      Filter by level\n"
            "  --level <n>-<m>  Filter by level range\n"
            "\n<b:yellow>Examples:</>\n"
            "  msearch guard           - Find mobiles named 'guard'\n"
            "  msearch --zone 30 guard - Find 'guard' in zone 30\n"
            "  msearch --zone 30       - List all mobs in zone 30\n"
            "  msearch --level 10-20   - Find mobs level 10-20")
        .build();

    Commands()
        .command("osearch", cmd_osearch)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("Search for objects by name with optional filters")
        .usage("osearch [options] <name>")
        .help(
            "<b:yellow>Options:</>\n"
            "  --zone <id>      Filter by zone ID\n"
            "  --type <t>       Filter by type (weapon, armor, etc)\n"
            "  --level <n>      Filter by level\n"
            "  --level <n>-<m>  Filter by level range\n"
            "\n<b:yellow>Examples:</>\n"
            "  osearch sword           - Find objects named 'sword'\n"
            "  osearch --zone 30 sword - Find 'sword' in zone 30\n"
            "  osearch --zone 30       - List all objects in zone 30\n"
            "  osearch --type weapon   - Find all weapons")
        .build();

    // List command for room contents
    Commands()
        .command("list", cmd_list)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .build();

    return Success();
}

} // namespace AdminCommands
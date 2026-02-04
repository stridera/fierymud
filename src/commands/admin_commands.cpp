#include "admin_commands.hpp"
#include "information_commands.hpp"
#include "commands/command_system.hpp"
#include "core/ability_executor.hpp"
#include "core/actor.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "core/formula_parser.hpp"
#include "core/log_subscriber.hpp"
#include "core/logging.hpp"
#include "core/money.hpp"
#include "core/shopkeeper.hpp"
#include "database/connection_pool.hpp"
#include "database/trigger_queries.hpp"
#include "scripting/script_context.hpp"
#include "scripting/script_engine.hpp"
#include "scripting/trigger_manager.hpp"
#include "world/weather.hpp"
#include "world/world_manager.hpp"

#include <algorithm>
#include <set>
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

    // Parse the argument: "now", "cancel", or a number of seconds
    auto& world_manager = WorldManager::instance();

    if (ctx.arg_count() == 0) {
        // Show usage and current shutdown status
        if (world_manager.is_shutdown_requested()) {
            auto time_left = std::chrono::duration_cast<std::chrono::seconds>(
                world_manager.get_shutdown_time() - std::chrono::steady_clock::now()
            ).count();
            if (time_left > 0) {
                ctx.send(fmt::format("Shutdown scheduled in {} seconds. Reason: {}",
                         time_left, world_manager.get_shutdown_reason()));
            } else {
                ctx.send("Shutdown in progress...");
            }
        } else {
            ctx.send("Usage: shutdown <now|cancel|seconds>");
            ctx.send("  shutdown now     - Immediate shutdown");
            ctx.send("  shutdown cancel  - Cancel pending shutdown");
            ctx.send("  shutdown 60      - Shutdown in 60 seconds");
        }
        return CommandResult::Success;
    }

    auto arg = std::string{ctx.arg(0)};

    // Handle "cancel"
    if (arg == "cancel") {
        if (!world_manager.is_shutdown_requested()) {
            ctx.send("No shutdown is currently scheduled.");
            return CommandResult::Success;
        }
        world_manager.cancel_shutdown();
        ctx.send_to_all("SYSTEM: Shutdown has been cancelled.");
        Log::info("Shutdown cancelled by {}", ctx.actor->name());
        return CommandResult::Success;
    }

    // Handle "now" or a number of seconds
    int seconds = 0;
    if (arg == "now") {
        seconds = 0;
    } else {
        try {
            seconds = std::stoi(arg);
            if (seconds < 0) {
                ctx.send_error("Seconds must be a positive number.");
                return CommandResult::InvalidTarget;
            }
        } catch (const std::exception&) {
            ctx.send_error(fmt::format("Invalid argument: '{}'. Use 'now', 'cancel', or a number of seconds.", arg));
            return CommandResult::InvalidTarget;
        }
    }

    // Get optional reason from remaining arguments
    std::string reason;
    if (ctx.arg_count() > 1) {
        reason = ctx.args_from(1);
    } else {
        reason = fmt::format("Initiated by {}", ctx.actor->name());
    }

    // Request the shutdown
    world_manager.request_shutdown(seconds, reason);

    // Notify all players
    if (seconds == 0) {
        ctx.send_to_all("SYSTEM: MUD is shutting down NOW!");
    } else {
        ctx.send_to_all(fmt::format("SYSTEM: MUD is shutting down in {} seconds. Reason: {}",
                       seconds, reason));
    }

    Log::warn("Shutdown initiated by {} (in {} seconds): {}", ctx.actor->name(), seconds, reason);

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

        ctx.send("\nUsage: tstat <zone:id> - Show trigger script by ID");
        return CommandResult::Success;
    }

    // Parse trigger ID (zone:id format, or just id using current zone)
    auto trigger_eid = ctx.parse_entity_id(ctx.arg(0));
    if (!trigger_eid.is_valid()) {
        ctx.send_error("Invalid trigger ID format. Use zone:id (e.g., 489:2) or just id for current zone.");
        return CommandResult::InvalidSyntax;
    }
    int zone_id = static_cast<int>(trigger_eid.zone_id());
    int trigger_id = static_cast<int>(trigger_eid.local_id());

    // Load trigger from database
    auto& pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        ctx.send_error("Database not available.");
        return CommandResult::SystemError;
    }

    auto result = pool.execute([zone_id, trigger_id](pqxx::work& txn)
        -> Result<FieryMUD::TriggerDataPtr> {
        return TriggerQueries::load_trigger_by_id(txn, zone_id, trigger_id);
    });

    if (!result) {
        ctx.send_error(fmt::format("Trigger {}:{} not found.", zone_id, trigger_id));
        return CommandResult::InvalidTarget;
    }

    auto& trigger = *result;

    ctx.send(fmt::format("<b:white>Trigger {}:{}</> - {}", zone_id, trigger_id, trigger->name));
    ctx.send(fmt::format("Type: {}  Flags: {}",
        trigger->attach_type == FieryMUD::ScriptType::MOB ? "MOB" :
        trigger->attach_type == FieryMUD::ScriptType::OBJECT ? "OBJECT" : "WORLD",
        trigger->flags_string()));
    if (trigger->num_args > 0) {
        std::string args_str;
        for (size_t i = 0; i < trigger->arg_list.size(); ++i) {
            if (i > 0) args_str += ", ";
            args_str += trigger->arg_list[i];
        }
        ctx.send(fmt::format("Args: {} - {}", trigger->num_args, args_str));
    }
    ctx.send("<b:white>Script:</>");
    // Split script by lines for cleaner display
    std::istringstream script_stream(trigger->commands);
    std::string line;
    while (std::getline(script_stream, line)) {
        ctx.send(line);
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
    ctx.send(fmt::format("Base Light Level: {}, Effective: {}",
        room->base_light_level(), room->calculate_effective_light()));
    ctx.send(fmt::format("Capacity: {}", room->capacity()));

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

    // Display mobile statistics - comprehensive output matching legacy stat
    const auto& stats = target->stats();

    // Header with ID and location
    ctx.send(fmt::format("MOB '<b:yellow>{}</>' ID: [<b:cyan>{}:{}</>], In room [{}:{}]",
        target->short_description(),
        target->id().zone_id(), target->id().local_id(),
        target->current_room() ? target->current_room()->id().zone_id() : 0,
        target->current_room() ? target->current_room()->id().local_id() : 0));

    // Keywords
    std::string mob_keywords_str;
    for (const auto& kw : target->keywords()) {
        if (!mob_keywords_str.empty()) mob_keywords_str += "', '";
        mob_keywords_str += kw;
    }
    ctx.send(fmt::format("Keywords: '{}'", mob_keywords_str));

    // Descriptions
    ctx.send(fmt::format("Short Desc: {}", target->short_description()));
    ctx.send(fmt::format("Room Desc: {}", target->ground().empty() ? "(none)" : std::string(target->ground())));
    ctx.send(fmt::format("Look Desc: {}", target->description().empty() ? "(none)" : std::string(target->description())));

    // Basic info line
    ctx.send(fmt::format("Race: <b:white>{}</>, Size: <b:white>{}</>, Gender: <b:white>{}</>",
        target->race(), target->size(), target->gender()));
    ctx.send(fmt::format("Life force: <b:white>{}</>, Composition: <b:white>{}</>",
        target->life_force(), target->composition()));
    ctx.send(fmt::format("Class: <b:white>NPC</>, Lev: [<b:yellow>{}</>], XP: [{}], Align: [{:>4}]",
        stats.level, stats.experience, stats.alignment));

    // Prototype ID if different from instance ID
    if (target->prototype_id().is_valid() && target->prototype_id() != target->id()) {
        ctx.send(fmt::format("Prototype: [{}:{}]",
            target->prototype_id().zone_id(), target->prototype_id().local_id()));
    }

    // Stats table
    ctx.send("         STR   INT   WIS   DEX   CON   CHA");
    ctx.send(fmt::format("STATS   {:>4}  {:>4}  {:>4}  {:>4}  {:>4}  {:>4}",
        stats.strength, stats.intelligence, stats.wisdom,
        stats.dexterity, stats.constitution, stats.charisma));

    // HP and Stamina
    ctx.send(fmt::format("HP: [<b:green>{}/{}</>]  Focus: [{}]",
        stats.hit_points, stats.max_hit_points, stats.focus));
    ctx.send(fmt::format("Stamina: [<b:cyan>{}/{}</>]",
        stats.stamina, stats.max_stamina));

    // Money
    const auto& money = target->money();
    ctx.send(fmt::format("Coins: [{}]", money.to_string(false)));

    // Combat stats
    ctx.send(fmt::format("Accuracy: [{:>3}], Evasion: [{:>3}], Attack Power: [{:>3}]",
        stats.accuracy, stats.evasion, stats.attack_power));
    ctx.send(fmt::format("Armor Rating: [{:>3}], DR%%: [{:>3}], Spell Power: [{:>3}]",
        stats.armor_rating, stats.damage_reduction_percent, stats.spell_power));
    ctx.send(fmt::format("Perception: [{:>4}], Concealment: [{:>4}]",
        stats.perception, stats.concealment));

    // Resistances
    ctx.send(fmt::format("Resistances: Fire[{}] Cold[{}] Lightning[{}] Acid[{}] Poison[{}]",
        stats.resistance_fire, stats.resistance_cold, stats.resistance_lightning,
        stats.resistance_acid, stats.resistance_poison));

    // Position and combat state
    auto fighting = target->get_fighting_target();
    ctx.send(fmt::format("Pos: {}, Fighting: {}",
        target->position(),
        fighting ? fmt::format("{}", fighting->short_description()) : "<none>"));

    // Dice
    const int dam_num = target->bare_hand_damage_dice_num();
    const int dam_size = target->bare_hand_damage_dice_size();
    const int dam_bonus = target->bare_hand_damage_dice_bonus();
    ctx.send(fmt::format("HP Dice: {}d{}+{}, Damage Dice: {}d{}+{}, Attack type: {}",
        target->hp_dice_num(), target->hp_dice_size(), target->hp_dice_bonus(),
        dam_num, dam_size, dam_bonus, target->damage_type()));

    // Traits, behaviors, and professions
    std::string all_flags;
    for (auto trait : magic_enum::enum_values<MobTrait>()) {
        if (target->has_trait(trait)) {
            if (!all_flags.empty()) all_flags += " ";
            all_flags += std::string(magic_enum::enum_name(trait));
        }
    }
    for (auto behavior : magic_enum::enum_values<MobBehavior>()) {
        if (target->has_behavior(behavior)) {
            if (!all_flags.empty()) all_flags += " ";
            all_flags += std::string(magic_enum::enum_name(behavior));
        }
    }
    for (auto profession : magic_enum::enum_values<MobProfession>()) {
        if (target->has_profession(profession)) {
            if (!all_flags.empty()) all_flags += " ";
            all_flags += std::string(magic_enum::enum_name(profession));
        }
    }
    ctx.send(fmt::format("NPC flags: {}", all_flags.empty() ? "<None>" : all_flags));

    // Aggression condition (Lua expression from database)
    const auto& aggro = target->aggro_condition();
    if (aggro.has_value() && !aggro->empty()) {
        ctx.send(fmt::format("Aggro Condition: <b:red>{}</>", *aggro));
    }

    // Carrying capacity and inventory
    auto equipped = target->equipment().get_all_equipped();
    int total_items = static_cast<int>(target->inventory().item_count());
    int equipped_count = static_cast<int>(equipped.size());
    ctx.send(fmt::format("Carrying: {} items, Equipped: {} items", total_items, equipped_count));

    // List inventory if any
    if (total_items > 0) {
        ctx.send("Inventory:");
        int count = 0;
        for (const auto& item : target->inventory().get_all_items()) {
            if (!item) continue;
            ctx.send(fmt::format("  [{}:{}] {}", item->id().zone_id(), item->id().local_id(),
                item->short_description()));
            if (++count >= 10) {
                ctx.send(fmt::format("  ... and {} more", total_items - count));
                break;
            }
        }
    }

    // List equipment if any
    if (equipped_count > 0) {
        ctx.send("Equipment:");
        for (const auto& item : equipped) {
            if (!item) continue;
            ctx.send(fmt::format("  [{}:{}] {}", item->id().zone_id(), item->id().local_id(),
                item->short_description()));
        }
    }

    // Master and followers
    auto master = target->get_master();
    if (master) {
        ctx.send(fmt::format("Master is: {}", master->short_description()));
    }
    auto& followers = target->get_followers();
    if (!followers.empty()) {
        std::string follower_str;
        for (const auto& f : followers) {
            auto follower = f.lock();
            if (follower) {
                if (!follower_str.empty()) follower_str += ", ";
                follower_str += std::string(follower->short_description());
            }
        }
        if (!follower_str.empty()) {
            ctx.send(fmt::format("Followers are: {}", follower_str));
        }
    }

    // Effect flags
    const auto& effects = target->effect_flags();
    if (!effects.empty()) {
        std::string effect_str;
        for (const auto& effect : effects) {
            if (!effect_str.empty()) effect_str += " ";
            effect_str += std::string(magic_enum::enum_name(effect));
        }
        ctx.send(fmt::format("EFF: <b:green>{}</>", effect_str));
    } else {
        ctx.send("EFF: <None>");
    }

    // Active effects (DoTs, HoTs, etc.)
    const auto& active = target->active_effects();
    const auto& dots = target->dot_effects();
    const auto& hots = target->hot_effects();
    if (!active.empty() || !dots.empty() || !hots.empty()) {
        ctx.send("<b:white>Active Effects:</>");
        for (const auto& eff : active) {
            if (eff.is_permanent()) {
                ctx.send(fmt::format("  {} (permanent)", eff.name));
            } else {
                ctx.send(fmt::format("  {} ({:.1f} hours remaining)", eff.name, eff.duration_hours));
            }
        }
        for (const auto& dot : dots) {
            ctx.send(fmt::format("  DoT: {} {} dmg ({} ticks left)",
                dot.flat_damage, dot.damage_type, dot.remaining_ticks));
        }
        for (const auto& hot : hots) {
            ctx.send(fmt::format("  HoT: {} {} heal ({} ticks left)",
                hot.flat_heal, hot.heal_type, hot.remaining_ticks));
        }
    }

    // Trigger/Script information
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();
    if (trigger_mgr.is_initialized()) {
        auto trigger_set = trigger_mgr.get_mob_triggers(target->prototype_id());
        if (!trigger_set.empty()) {
            ctx.send(fmt::format("<b:white>Scripts ({} triggers):</>", trigger_set.size()));
            for (const auto& trigger : trigger_set) {
                if (!trigger) continue;
                ctx.send(fmt::format("  [{}:{}] {} - {}",
                    trigger->zone_id.value_or(0), trigger->id,
                    trigger->name, trigger->flags_string()));
            }
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

// Debug command to diagnose mob aggression system
Result<CommandResult> cmd_aggrodebug(const CommandContext &ctx) {
    // Alignment thresholds
    constexpr int ALIGN_GOOD = 350;
    constexpr int ALIGN_EVIL = -350;

    auto& spawned_mobiles = WorldManager::instance().spawned_mobiles();
    ctx.send(fmt::format("<b:cyan>--- Aggression Debug ---</>"));
    ctx.send(fmt::format("Total spawned mobiles: {}", spawned_mobiles.size()));

    // Count aggressive mobs globally
    // Note: Aggression is now controlled by aggressionFormula (Lua expression)
    int total_aggro = 0;
    for (const auto& [id, mob] : spawned_mobiles) {
        if (mob->is_aggressive()) {
            total_aggro++;
        }
    }
    ctx.send(fmt::format("Total aggressive mobs (spawned): {}", total_aggro));

    // Show current room info
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidTarget;
    }

    ctx.send(fmt::format("\n<b:white>Current Room: {} ({})</>", ctx.room->name(), ctx.room->id()));

    // List all actors in room
    ctx.send(fmt::format("<b:white>Actors in room ({}):</>", ctx.room->contents().actors.size()));
    for (const auto& actor : ctx.room->contents().actors) {
        if (!actor) continue;

        std::string type_str = std::string(actor->type_name());
        int level = actor->stats().level;
        int align = actor->stats().alignment;

        std::string align_str;
        if (align >= ALIGN_GOOD) align_str = "good";
        else if (align <= ALIGN_EVIL) align_str = "evil";
        else align_str = "neutral";

        if (auto mob = std::dynamic_pointer_cast<Mobile>(actor)) {
            // It's a mob
            const auto& aggro = mob->aggro_condition();
            bool is_in_spawned = spawned_mobiles.contains(mob->id());

            if (aggro.has_value() && !aggro->empty()) {
                ctx.send(fmt::format("  [MOB] {} (L{}) - <b:red>aggro: {}</> in_spawned:{}",
                    mob->display_name(), level, *aggro,
                    is_in_spawned ? "<b:green>YES</>" : "<b:red>NO</>"));
            } else {
                ctx.send(fmt::format("  [MOB] {} (L{}) - <b:green>passive</> in_spawned:{}",
                    mob->display_name(), level,
                    is_in_spawned ? "<b:green>YES</>" : "<b:red>NO</>"));
            }
        } else {
            // It's a player
            ctx.send(fmt::format("  [{}] {} (L{}, align:{} {}) {}",
                type_str, actor->display_name(), level, align, align_str,
                level >= 100 ? "<b:yellow>(IMMORTAL - won't be attacked)</>" : ""));
        }
    }

    // Check if any aggressive mob in this room would attack any player
    ctx.send(fmt::format("\n<b:white>Attack Analysis:</>"));
    bool any_attacks_possible = false;

    for (const auto& actor : ctx.room->contents().actors) {
        auto mob = std::dynamic_pointer_cast<Mobile>(actor);
        if (!mob) continue;

        // Aggression is now controlled by aggressionFormula (Lua expression)
        bool is_aggro = mob->is_aggressive();
        if (!is_aggro) continue;

        // Check if mob is in spawned_mobiles
        if (!spawned_mobiles.contains(mob->id())) {
            ctx.send(fmt::format("  <b:red>WARNING:</> {} is aggressive but NOT in spawned_mobiles!",
                mob->display_name()));
            continue;
        }

        // Check mob's room
        auto mob_room = mob->current_room();
        if (!mob_room) {
            ctx.send(fmt::format("  <b:red>WARNING:</> {} has no current_room set!", mob->display_name()));
            continue;
        }
        if (mob_room.get() != ctx.room.get()) {
            ctx.send(fmt::format("  <b:red>WARNING:</> {} current_room ({}) != actual room ({})!",
                mob->display_name(), mob_room->id(), ctx.room->id()));
            continue;
        }

        // Check against each player in room
        for (const auto& target : ctx.room->contents().actors) {
            if (!target || target->type_name() != "Player") continue;

            int target_level = target->stats().level;
            int target_align = target->stats().alignment;

            if (target_level >= 100) {
                ctx.send(fmt::format("  {} vs {} - <b:yellow>SKIP (immortal)</>",
                    mob->display_name(), target->display_name()));
                continue;
            }

            // With Lua-based aggro_condition, we show the condition and relevant target info
            // Actual evaluation happens in the game AI loop
            const auto& aggro = mob->aggro_condition();
            any_attacks_possible = true;
            ctx.send(fmt::format("  {} vs {} - condition: <b:cyan>{}</> (target align: {})",
                mob->display_name(), target->display_name(),
                aggro ? *aggro : "none",
                target_align));
        }
    }

    if (!any_attacks_possible) {
        ctx.send("\n<b:yellow>No attacks possible in this room - check flags, levels, and alignment.</>");
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
// Ability Debug Commands
// =============================================================================

/**
 * Convert EffectType enum to string for display.
 */
static std::string_view effect_type_to_string(FieryMUD::EffectType type) {
    using FieryMUD::EffectType;
    switch (type) {
        case EffectType::Damage: return "Damage";
        case EffectType::Heal: return "Heal";
        case EffectType::Modify: return "Modify";
        case EffectType::Status: return "Status";
        case EffectType::Cleanse: return "Cleanse";
        case EffectType::Dispel: return "Dispel";
        case EffectType::Reveal: return "Reveal";
        case EffectType::Teleport: return "Teleport";
        case EffectType::Extract: return "Extract";
        case EffectType::Move: return "Move";
        case EffectType::Interrupt: return "Interrupt";
        case EffectType::Transform: return "Transform";
        case EffectType::Resurrect: return "Resurrect";
        case EffectType::Create: return "Create";
        case EffectType::Summon: return "Summon";
        case EffectType::Enchant: return "Enchant";
        case EffectType::Globe: return "Globe";
        case EffectType::Room: return "Room";
        case EffectType::Inspect: return "Inspect";
        case EffectType::Dot: return "DoT";
        case EffectType::Hot: return "HoT";
        default: return "Unknown";
    }
}

/**
 * Convert EffectTrigger enum to string for display.
 */
static std::string_view effect_trigger_to_string(FieryMUD::EffectTrigger trigger) {
    using FieryMUD::EffectTrigger;
    switch (trigger) {
        case EffectTrigger::OnHit: return "on_hit";
        case EffectTrigger::OnCast: return "on_cast";
        case EffectTrigger::OnMiss: return "on_miss";
        case EffectTrigger::Periodic: return "periodic";
        case EffectTrigger::OnEnd: return "on_end";
        case EffectTrigger::OnTrigger: return "on_trigger";
        default: return "unknown";
    }
}

/**
 * Convert AbilityType enum to string for display.
 */
static std::string_view ability_type_to_string(WorldQueries::AbilityType type) {
    using WorldQueries::AbilityType;
    switch (type) {
        case AbilityType::Spell: return "SPELL";
        case AbilityType::Skill: return "SKILL";
        case AbilityType::Chant: return "CHANT";
        case AbilityType::Song: return "SONG";
    }
    return "UNKNOWN";
}

/**
 * alist - List abilities from the AbilityCache.
 * Usage: alist [--type spell|skill|chant|song] [--circle N] [--effect <type>] [--limit N]
 */
Result<CommandResult> cmd_alist(const CommandContext &ctx) {
    auto& cache = FieryMUD::AbilityCache::instance();
    if (!cache.is_initialized()) {
        auto init_result = cache.initialize();
        if (!init_result) {
            ctx.send_error(fmt::format("Failed to initialize ability cache: {}", init_result.error().message));
            return CommandResult::SystemError;
        }
    }

    // Parse options
    std::string filter_type;
    int filter_circle = -1;
    std::string filter_effect;
    int limit = 50;

    for (size_t i = 0; i < ctx.arg_count(); ++i) {
        auto arg = std::string{ctx.arg(i)};
        if (arg == "--type" && i + 1 < ctx.arg_count()) {
            filter_type = std::string{ctx.arg(++i)};
            std::transform(filter_type.begin(), filter_type.end(), filter_type.begin(), ::toupper);
        } else if (arg == "--circle" && i + 1 < ctx.arg_count()) {
            try {
                filter_circle = std::stoi(std::string{ctx.arg(++i)});
            } catch (...) {
                ctx.send_error("Invalid circle number.");
                return CommandResult::InvalidSyntax;
            }
        } else if (arg == "--effect" && i + 1 < ctx.arg_count()) {
            filter_effect = std::string{ctx.arg(++i)};
            std::transform(filter_effect.begin(), filter_effect.end(), filter_effect.begin(), ::tolower);
        } else if (arg == "--limit" && i + 1 < ctx.arg_count()) {
            try {
                limit = std::stoi(std::string{ctx.arg(++i)});
            } catch (...) {
                ctx.send_error("Invalid limit number.");
                return CommandResult::InvalidSyntax;
            }
        } else if (arg == "--help" || arg == "-h") {
            ctx.send("<b:cyan>alist - List abilities from cache</>");
            ctx.send("Usage: alist [options]");
            ctx.send("Options:");
            ctx.send("  --type <spell|skill|chant|song>  Filter by ability type");
            ctx.send("  --circle <N>                     Filter by spell circle (1-9)");
            ctx.send("  --effect <type>                  Filter by effect type (damage, heal, status, etc.)");
            ctx.send("  --limit <N>                      Maximum results (default 50)");
            return CommandResult::Success;
        }
    }

    // Build filter description
    std::string filter_desc;
    if (!filter_type.empty()) filter_desc += fmt::format("type={}", filter_type);
    if (filter_circle > 0) {
        if (!filter_desc.empty()) filter_desc += ", ";
        filter_desc += fmt::format("circle={}", filter_circle);
    }
    if (!filter_effect.empty()) {
        if (!filter_desc.empty()) filter_desc += ", ";
        filter_desc += fmt::format("effect={}", filter_effect);
    }
    if (filter_desc.empty()) filter_desc = "all";

    ctx.send(fmt::format("<b:cyan>--- Abilities ({}) ---</>", filter_desc));

    // Collect and display abilities
    // Note: We iterate through abilities via the name index which is available
    // We'll need to iterate abilities - for now we'll use a different approach
    int count = 0;
    int total_in_cache = 0;

    // Access abilities through the cache - we need to iterate all
    // Since AbilityCache doesn't expose an iterator, we'll search by ID range
    for (int id = 1; id <= 1000; ++id) {  // Reasonable upper bound
        const auto* ability = cache.get_ability(id);
        if (!ability) continue;

        total_in_cache++;

        // Apply filters
        if (!filter_type.empty()) {
            std::string type_str{ability_type_to_string(ability->type)};
            if (type_str != filter_type) continue;
        }

        if (filter_circle > 0) {
            // Check if any class has this ability at the specified circle
            auto classes = cache.get_ability_classes(id);
            bool found_circle = false;
            for (const auto& cls : classes) {
                if (cls.circle == filter_circle) {
                    found_circle = true;
                    break;
                }
            }
            if (!found_circle) continue;
        }

        if (!filter_effect.empty()) {
            auto effects = cache.get_ability_effects(id);
            bool found_effect = false;
            for (const auto& eff : effects) {
                const auto* effect_def = cache.get_effect(eff.effect_id);
                if (effect_def) {
                    std::string eff_type_str{effect_type_to_string(effect_def->type)};
                    std::transform(eff_type_str.begin(), eff_type_str.end(), eff_type_str.begin(), ::tolower);
                    if (eff_type_str.find(filter_effect) != std::string::npos) {
                        found_effect = true;
                        break;
                    }
                }
            }
            if (!found_effect) continue;
        }

        if (count >= limit) {
            ctx.send(fmt::format("... (limited to {} results, use --limit to show more)", limit));
            break;
        }

        // Display ability
        auto effects = cache.get_ability_effects(id);
        auto classes = cache.get_ability_classes(id);

        std::string circle_str;
        if (!classes.empty()) {
            std::set<int> circles;
            for (const auto& cls : classes) {
                if (cls.circle > 0) circles.insert(cls.circle);
            }
            if (!circles.empty()) {
                circle_str = " C";
                for (int c : circles) {
                    circle_str += std::to_string(c);
                }
            }
        }

        std::string effect_summary;
        if (effects.empty()) {
            effect_summary = "<r:yellow>(NO EFFECTS)</>";
        } else {
            for (size_t i = 0; i < effects.size() && i < 3; ++i) {
                const auto* effect_def = cache.get_effect(effects[i].effect_id);
                if (effect_def) {
                    if (!effect_summary.empty()) effect_summary += ",";
                    effect_summary += effect_type_to_string(effect_def->type);
                }
            }
            if (effects.size() > 3) {
                effect_summary += fmt::format("+{}", effects.size() - 3);
            }
        }

        ctx.send(fmt::format("  [{:4}] {:<25} {:6}{:4} [{}]",
            id, ability->plain_name, ability_type_to_string(ability->type),
            circle_str, effect_summary));

        count++;
    }

    if (count == 0) {
        ctx.send("No abilities found matching criteria.");
    } else {
        ctx.send(fmt::format("Found {} abilities (cache has {} total).", count, total_in_cache));
    }

    return CommandResult::Success;
}

/**
 * astat - Show detailed ability information.
 * Usage: astat <ability name or ID>
 */
Result<CommandResult> cmd_astat(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Usage: astat <ability name or ID>");
        ctx.send("Example: astat magic missile");
        ctx.send("Example: astat 42");
        return CommandResult::InvalidSyntax;
    }

    auto& cache = FieryMUD::AbilityCache::instance();
    if (!cache.is_initialized()) {
        auto init_result = cache.initialize();
        if (!init_result) {
            ctx.send_error(fmt::format("Failed to initialize ability cache: {}", init_result.error().message));
            return CommandResult::SystemError;
        }
    }

    // Collect all arguments as ability name
    std::string ability_name = std::string{ctx.command.full_argument_string};

    // Try to find ability by ID first
    const WorldQueries::AbilityData* ability = nullptr;
    try {
        int id = std::stoi(ability_name);
        ability = cache.get_ability(id);
    } catch (...) {
        // Not a number, search by name
        ability = cache.get_ability_by_name(ability_name);
    }

    if (!ability) {
        ctx.send_error(fmt::format("Ability '{}' not found in cache.", ability_name));
        ctx.send("Use 'alist' to see available abilities.");
        return CommandResult::InvalidTarget;
    }

    // Display detailed ability information
    ctx.send(fmt::format("<b:cyan>--- Ability: {} (ID: {}) ---</>", ability->name, ability->id));
    ctx.send(fmt::format("  Plain Name: {}", ability->plain_name));
    ctx.send(fmt::format("  Type: {}", ability_type_to_string(ability->type)));

    if (!ability->description.empty()) {
        ctx.send(fmt::format("  Description: {}", ability->description));
    }

    // Flags and properties
    std::vector<std::string> flags;
    if (ability->violent) flags.push_back("VIOLENT");
    if (ability->is_area) flags.push_back("AREA");
    if (ability->is_toggle) flags.push_back("TOGGLE");
    if (ability->combat_ok) flags.push_back("COMBAT_OK");
    if (ability->in_combat_only) flags.push_back("COMBAT_ONLY");
    if (ability->quest_only) flags.push_back("QUEST_ONLY");
    if (ability->humanoid_only) flags.push_back("HUMANOID_ONLY");

    if (!flags.empty()) {
        std::string flag_str;
        for (size_t i = 0; i < flags.size(); ++i) {
            if (i > 0) flag_str += ", ";
            flag_str += flags[i];
        }
        ctx.send(fmt::format("  Flags: {}", flag_str));
    }

    if (ability->min_position > 0) {
        ctx.send(fmt::format("  Min Position: {} ({})", ability->min_position,
            ActorUtils::get_position_name(static_cast<Position>(ability->min_position))));
    }

    if (ability->cast_time_rounds > 0) {
        ctx.send(fmt::format("  Cast Time: {} rounds", ability->cast_time_rounds));
    }

    if (ability->cooldown_ms > 0) {
        ctx.send(fmt::format("  Cooldown: {}ms ({:.1f}s)", ability->cooldown_ms, ability->cooldown_ms / 1000.0));
    }

    if (!ability->sphere.empty()) {
        ctx.send(fmt::format("  Sphere: {}", ability->sphere));
    }

    if (!ability->damage_type.empty()) {
        ctx.send(fmt::format("  Damage Type: {}", ability->damage_type));
    }

    // Classes that can learn this ability
    auto classes = cache.get_ability_classes(ability->id);
    if (!classes.empty()) {
        ctx.send("");
        ctx.send("<b:yellow>  Classes:</>");
        for (const auto& cls : classes) {
            if (cls.circle > 0) {
                ctx.send(fmt::format("    {} - Circle {}", cls.class_name, cls.circle));
            } else {
                ctx.send(fmt::format("    {}", cls.class_name));
            }
        }
    }

    // Effects (the critical debugging info!)
    auto effects = cache.get_ability_effects(ability->id);
    ctx.send("");
    if (effects.empty()) {
        ctx.send("<r:red>  Effects: NONE DEFINED!</>");
        ctx.send("<r:yellow>  WARNING: This ability has no effects linked in the database.</>");
        ctx.send("<r:yellow>  The ability will cast but do nothing.</>");
    } else {
        ctx.send(fmt::format("<b:green>  Effects: {} defined</>", effects.size()));
        for (size_t i = 0; i < effects.size(); ++i) {
            const auto& eff = effects[i];
            const auto* effect_def = cache.get_effect(eff.effect_id);

            ctx.send(fmt::format("    [{}] Effect ID: {}", i + 1, eff.effect_id));
            if (effect_def) {
                ctx.send(fmt::format("        Name: {}", effect_def->name));
                ctx.send(fmt::format("        Type: {}", effect_type_to_string(effect_def->type)));
                if (!effect_def->description.empty()) {
                    ctx.send(fmt::format("        Desc: {}", effect_def->description));
                }
            } else {
                ctx.send("        <r:red>ERROR: Effect definition not found in cache!</>");
            }

            ctx.send(fmt::format("        Trigger: {}", effect_trigger_to_string(eff.trigger)));
            ctx.send(fmt::format("        Chance: {}%", eff.chance_percent));
            ctx.send(fmt::format("        Order: {}", eff.order));

            if (!eff.condition.empty()) {
                ctx.send(fmt::format("        Condition: {}", eff.condition));
            }

            // Display relevant params based on effect type
            if (effect_def) {
                const auto& p = eff.params;
                switch (effect_def->type) {
                    case FieryMUD::EffectType::Damage: {
                        // Only show single damage type if no damage components exist
                        auto dmg_comps = cache.get_damage_components(ability->id);
                        if (dmg_comps.empty()) {
                            ctx.send(fmt::format("        Damage Type: {}", p.damage_type));
                        } else {
                            ctx.send("        Damage Type: (see Damage Components below)");
                        }
                        if (!p.amount_formula.empty())
                            ctx.send(fmt::format("        Amount Formula: {}", p.amount_formula));
                        break;
                    }

                    case FieryMUD::EffectType::Heal:
                        ctx.send(fmt::format("        Heal Resource: {}", p.heal_resource));
                        if (!p.heal_formula.empty())
                            ctx.send(fmt::format("        Heal Formula: {}", p.heal_formula));
                        break;

                    case FieryMUD::EffectType::Modify:
                        if (!p.modify_target.empty())
                            ctx.send(fmt::format("        Target Stat: {}", p.modify_target));
                        if (!p.modify_amount.empty())
                            ctx.send(fmt::format("        Amount: {}", p.modify_amount));
                        if (p.modify_duration > 0) {
                            if (!p.modify_duration_unit.empty())
                                ctx.send(fmt::format("        Duration: {} {}", p.modify_duration, p.modify_duration_unit));
                            else
                                ctx.send(fmt::format("        Duration: {} ticks", p.modify_duration));
                        }
                        break;

                    case FieryMUD::EffectType::Status:
                        ctx.send(fmt::format("        Status: {}", p.status_name));
                        if (!p.status_duration_formula.empty())
                            ctx.send(fmt::format("        Duration Formula: {}", p.status_duration_formula));
                        else if (p.status_duration > 0)
                            ctx.send(fmt::format("        Duration: {} ticks", p.status_duration));
                        if (p.is_toggle_duration)
                            ctx.send("        Mode: TOGGLE (permanent until removed)");
                        break;

                    case FieryMUD::EffectType::Dot:
                        ctx.send(fmt::format("        Cure Category: {}", p.cure_category));
                        if (!p.flat_damage_formula.empty())
                            ctx.send(fmt::format("        Flat Damage: {}", p.flat_damage_formula));
                        if (!p.percent_damage_formula.empty())
                            ctx.send(fmt::format("        %HP Damage: {}%", p.percent_damage_formula));
                        if (!p.dot_duration_formula.empty())
                            ctx.send(fmt::format("        Duration: {}", p.dot_duration_formula));
                        ctx.send(fmt::format("        Tick Interval: {}", p.tick_interval));
                        if (p.blocks_regen)
                            ctx.send("        Blocks Regen: YES");
                        break;

                    case FieryMUD::EffectType::Hot:
                        ctx.send(fmt::format("        Category: {}", p.hot_category));
                        if (!p.flat_heal_formula.empty())
                            ctx.send(fmt::format("        Flat Heal: {}", p.flat_heal_formula));
                        if (!p.percent_heal_formula.empty())
                            ctx.send(fmt::format("        %HP Heal: {}%", p.percent_heal_formula));
                        if (!p.hot_duration_formula.empty())
                            ctx.send(fmt::format("        Duration: {}", p.hot_duration_formula));
                        break;

                    case FieryMUD::EffectType::Teleport:
                        ctx.send(fmt::format("        Teleport Type: {}", p.teleport_type));
                        if (p.teleport_room_id > 0)
                            ctx.send(fmt::format("        Fixed Room: {}", p.teleport_room_id));
                        break;

                    case FieryMUD::EffectType::Cleanse:
                        ctx.send(fmt::format("        Cleanse Category: {}", p.cleanse_category));
                        if (!p.cleanse_power_formula.empty())
                            ctx.send(fmt::format("        Power: {}", p.cleanse_power_formula));
                        break;

                    case FieryMUD::EffectType::Move:
                        ctx.send(fmt::format("        Move Type: {}", p.move_type));
                        if (!p.move_distance_formula.empty())
                            ctx.send(fmt::format("        Distance: {}", p.move_distance_formula));
                        break;

                    default:
                        // For other types, just show they exist
                        break;
                }
            }
        }
    }

    // Damage components
    auto damage_components = cache.get_damage_components(ability->id);
    if (!damage_components.empty()) {
        ctx.send("");
        ctx.send("<b:yellow>  Damage Components:</>");
        for (const auto& comp : damage_components) {
            ctx.send(fmt::format("    {} ({}%): {}", comp.element, comp.percentage, comp.damage_formula));
        }
    }

    // Damage range estimation
    // Find the first damage formula to estimate (from components or effect params)
    std::string damage_formula;
    if (!damage_components.empty()) {
        // Use total damage formula from first component (they share the same base formula usually)
        damage_formula = damage_components[0].damage_formula;
    } else {
        // Check effects for a damage effect
        for (const auto& eff : effects) {
            const auto* effect_def = cache.get_effect(eff.effect_id);
            if (effect_def && effect_def->type == FieryMUD::EffectType::Damage) {
                if (!eff.params.amount_formula.empty()) {
                    damage_formula = eff.params.amount_formula;
                }
                break;
            }
        }
    }

    if (!damage_formula.empty()) {
        ctx.send("");
        ctx.send("<b:yellow>  Damage Estimates:</>");

        // Skill 0 (untrained)
        FieryMUD::FormulaContext ctx_low;
        ctx_low.skill_level = 0;
        ctx_low.actor_level = 1;
        ctx_low.base_damage = 10;  // Approximate base damage for low level
        auto range_low = FieryMUD::FormulaParser::estimate_damage_range(damage_formula, ctx_low);
        ctx.send(fmt::format("    Skill 0:   {}", range_low.to_string()));

        // Skill 100 (mastered)
        FieryMUD::FormulaContext ctx_high;
        ctx_high.skill_level = 100;
        ctx_high.actor_level = 50;
        ctx_high.base_damage = 100;  // Approximate base damage for high level
        auto range_high = FieryMUD::FormulaParser::estimate_damage_range(damage_formula, ctx_high);
        ctx.send(fmt::format("    Skill 100: {}", range_high.to_string()));
    }

    // Custom messages
    const auto* messages = cache.get_ability_messages(ability->id);
    if (messages) {
        ctx.send("");
        ctx.send("<b:yellow>  Messages:</>");
        if (!messages->success_to_caster.empty())
            ctx.send(fmt::format("    Success (to caster): {}", messages->success_to_caster));
        if (!messages->success_to_victim.empty())
            ctx.send(fmt::format("    Success (to victim): {}", messages->success_to_victim));
        if (!messages->success_to_room.empty())
            ctx.send(fmt::format("    Success (to room): {}", messages->success_to_room));
        if (!messages->success_to_self.empty())
            ctx.send(fmt::format("    Success (self-cast): {}", messages->success_to_self));
        if (!messages->fail_to_caster.empty())
            ctx.send(fmt::format("    Fail (to caster): {}", messages->fail_to_caster));
        if (!messages->wearoff_to_target.empty())
            ctx.send(fmt::format("    Wear-off: {}", messages->wearoff_to_target));
        if (!messages->look_message.empty())
            ctx.send(fmt::format("    Look message: {}", messages->look_message));
    }

    // Restrictions
    const auto* restrictions = cache.get_ability_restrictions(ability->id);
    if (restrictions && (!restrictions->requirements.empty() || !restrictions->custom_lua.empty())) {
        ctx.send("");
        ctx.send("<b:yellow>  Restrictions:</>");
        for (const auto& req : restrictions->requirements) {
            ctx.send(fmt::format("    {}{}: {}", req.negated ? "NOT " : "", req.type, req.value));
        }
        if (!restrictions->custom_lua.empty()) {
            ctx.send(fmt::format("    Custom Lua: {}", restrictions->custom_lua));
        }
    }

    ctx.send(fmt::format("<b:cyan>--- End of Ability {} ---</>", ability->id));
    return CommandResult::Success;
}

/**
 * asearch - Search abilities by name or effect.
 * Usage: asearch <name> or asearch --effect <type>
 */
Result<CommandResult> cmd_asearch(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Usage: asearch <partial name>");
        ctx.send("       asearch --effect <type>");
        ctx.send("       asearch --no-effects");
        ctx.send("Example: asearch nova");
        ctx.send("Example: asearch --effect damage");
        ctx.send("Example: asearch --no-effects");
        return CommandResult::InvalidSyntax;
    }

    auto& cache = FieryMUD::AbilityCache::instance();
    if (!cache.is_initialized()) {
        auto init_result = cache.initialize();
        if (!init_result) {
            ctx.send_error(fmt::format("Failed to initialize ability cache: {}", init_result.error().message));
            return CommandResult::SystemError;
        }
    }

    std::string search_name;
    std::string search_effect;
    bool search_no_effects = false;

    for (size_t i = 0; i < ctx.arg_count(); ++i) {
        auto arg = std::string{ctx.arg(i)};
        if (arg == "--effect" && i + 1 < ctx.arg_count()) {
            search_effect = std::string{ctx.arg(++i)};
            std::transform(search_effect.begin(), search_effect.end(), search_effect.begin(), ::tolower);
        } else if (arg == "--no-effects") {
            search_no_effects = true;
        } else if (!arg.starts_with("--")) {
            if (!search_name.empty()) search_name += " ";
            search_name += arg;
        }
    }

    std::string search_lower = search_name;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    ctx.send(fmt::format("<b:cyan>--- Searching abilities ---</>"));

    int count = 0;
    const int MAX_RESULTS = 50;

    for (int id = 1; id <= 1000; ++id) {
        const auto* ability = cache.get_ability(id);
        if (!ability) continue;

        // Name filter
        if (!search_lower.empty()) {
            std::string plain_lower = ability->plain_name;
            std::transform(plain_lower.begin(), plain_lower.end(), plain_lower.begin(), ::tolower);
            if (plain_lower.find(search_lower) == std::string::npos) continue;
        }

        auto effects = cache.get_ability_effects(id);

        // Effect type filter
        if (!search_effect.empty()) {
            bool found = false;
            for (const auto& eff : effects) {
                const auto* effect_def = cache.get_effect(eff.effect_id);
                if (effect_def) {
                    std::string eff_type{effect_type_to_string(effect_def->type)};
                    std::transform(eff_type.begin(), eff_type.end(), eff_type.begin(), ::tolower);
                    if (eff_type.find(search_effect) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) continue;
        }

        // No effects filter (find broken abilities)
        if (search_no_effects && !effects.empty()) continue;

        if (count >= MAX_RESULTS) {
            ctx.send(fmt::format("... (limited to {} results)", MAX_RESULTS));
            break;
        }

        // Display
        std::string effect_summary;
        if (effects.empty()) {
            effect_summary = "<r:red>NO EFFECTS</>";
        } else {
            for (size_t i = 0; i < effects.size() && i < 3; ++i) {
                const auto* effect_def = cache.get_effect(effects[i].effect_id);
                if (effect_def) {
                    if (!effect_summary.empty()) effect_summary += ", ";
                    effect_summary += effect_type_to_string(effect_def->type);
                }
            }
            if (effects.size() > 3) {
                effect_summary += fmt::format(" +{}", effects.size() - 3);
            }
        }

        ctx.send(fmt::format("  [{:4}] {:<25} {:6} [{}]",
            id, ability->plain_name, ability_type_to_string(ability->type), effect_summary));
        count++;
    }

    if (count == 0) {
        if (search_no_effects) {
            ctx.send("No abilities without effects found (that's good!).");
        } else {
            ctx.send("No abilities found matching search criteria.");
        }
    } else {
        ctx.send(fmt::format("Found {} matching abilities.", count));
    }

    return CommandResult::Success;
}

/**
 * areload - Reload the ability cache from database.
 */
Result<CommandResult> cmd_areload(const CommandContext &ctx) {
    ctx.send("Reloading ability cache from database...");

    auto& cache = FieryMUD::AbilityCache::instance();
    auto result = cache.reload();

    if (!result) {
        ctx.send_error(fmt::format("Failed to reload ability cache: {}", result.error().message));
        return CommandResult::SystemError;
    }

    ctx.send("<b:green>Ability cache reloaded successfully.</>");
    return CommandResult::Success;
}

// =============================================================================
// Script Debugging Commands
// =============================================================================

/**
 * syslog - Subscribe to real-time log messages.
 *
 * Usage:
 *   syslog                    - Show current subscription status
 *   syslog off               - Stop watching logs
 *   syslog <level>           - Watch logs at level (debug/info/warn/error)
 *   syslog <level> <comp>    - Watch specific component at level
 *   syslog <level> all       - Watch all components at level
 */
Result<CommandResult> cmd_syslog(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use syslog.");
        return CommandResult::InvalidState;
    }

    auto& subscriber = LogSubscriber::instance();

    // No arguments - show status
    if (ctx.arg_count() == 0) {
        if (subscriber.is_subscribed(player)) {
            auto sub = subscriber.get_subscription(player);
            if (sub) {
                std::string level_str{LogSubscriber::level_to_string(sub->min_level)};
                std::string comp_str;
                if (sub->components.empty()) {
                    comp_str = "all components";
                } else {
                    for (const auto& c : sub->components) {
                        if (!comp_str.empty()) comp_str += ", ";
                        comp_str += c;
                    }
                }
                ctx.send(fmt::format("<b:cyan>Syslog subscription active:</> {} level, {}", level_str, comp_str));
            }
        } else {
            ctx.send("<b:yellow>Syslog subscription:</> inactive");
        }
        ctx.send("\n<b:white>Usage:</>");
        ctx.send("  syslog off               - Stop watching logs");
        ctx.send("  syslog <level>           - Watch all logs at level");
        ctx.send("  syslog <level> <comp>    - Watch specific component");
        ctx.send("  syslog <level> all       - Watch all components");
        ctx.send("\n<b:white>Levels:</> trace, debug, info, warn, error, critical");
        auto comps = LogSubscriber::available_components();
        std::string comp_list;
        for (size_t i = 0; i < comps.size(); ++i) {
            if (i > 0) comp_list += ", ";
            comp_list += comps[i];
        }
        ctx.send("<b:white>Components:</> " + comp_list);
        return CommandResult::Success;
    }

    auto arg1 = std::string{ctx.arg(0)};
    std::transform(arg1.begin(), arg1.end(), arg1.begin(), ::tolower);

    // syslog off
    if (arg1 == "off") {
        if (subscriber.is_subscribed(player)) {
            subscriber.unsubscribe(player);
            ctx.send("<b:green>Syslog subscription stopped.</>");
        } else {
            ctx.send("You are not subscribed to syslog.");
        }
        return CommandResult::Success;
    }

    // Parse level
    auto level = LogSubscriber::parse_level(arg1);
    if (!level) {
        ctx.send_error(fmt::format("Invalid log level: '{}'. Use: trace, debug, info, warn, error, critical", arg1));
        return CommandResult::InvalidSyntax;
    }

    // Parse optional component filter
    std::set<std::string> components;
    if (ctx.arg_count() > 1) {
        auto comp = std::string{ctx.arg(1)};
        std::transform(comp.begin(), comp.end(), comp.begin(), ::tolower);

        if (comp != "all") {
            // Check if valid component
            auto available = LogSubscriber::available_components();
            bool found = false;
            for (const auto& c : available) {
                if (c == comp) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::string avail_str;
                for (size_t i = 0; i < available.size(); ++i) {
                    if (i > 0) avail_str += ", ";
                    avail_str += available[i];
                }
                ctx.send_error(fmt::format("Unknown component: '{}'. Available: {}",
                    comp, avail_str));
                return CommandResult::InvalidSyntax;
            }
            components.insert(comp);
        }
        // "all" leaves components empty, which matches all
    }

    // Subscribe
    subscriber.subscribe(player, *level, components);

    std::string comp_msg;
    if (components.empty()) {
        comp_msg = "all components";
    } else {
        for (const auto& c : components) {
            if (!comp_msg.empty()) comp_msg += ", ";
            comp_msg += c;
        }
    }
    ctx.send(fmt::format("<b:green>Syslog subscription active:</> {} level, {}", LogSubscriber::level_to_string(*level), comp_msg));

    return CommandResult::Success;
}

/**
 * dtrig - Debug trigger command.
 *
 * Usage:
 *   dtrig list <target>           - List all triggers on mob/object
 *   dtrig info <zone:id>          - Show trigger details and last error
 *   dtrig fire <target> <trigger> - Manually execute a trigger
 */
Result<CommandResult> cmd_dtrig(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("<b:white>Debug Trigger Command</>");
        ctx.send("Usage:");
        ctx.send("  dtrig list <target>           - List all triggers on mob/object");
        ctx.send("  dtrig info <zone:id>          - Show trigger details and last error");
        ctx.send("  dtrig fire <target> <zone:id> - Manually execute a trigger by ID");
        ctx.send("  (If zone omitted, uses current zone)");
        return CommandResult::Success;
    }

    auto subcmd = std::string{ctx.arg(0)};
    std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::tolower);

    auto& trigger_mgr = FieryMUD::TriggerManager::instance();

    if (subcmd == "list") {
        if (ctx.arg_count() < 2) {
            ctx.send_error("Usage: dtrig list <target>");
            return CommandResult::InvalidSyntax;
        }

        // Find target - try actor first, then object
        auto actor_target = ctx.find_actor_target(ctx.arg(1));
        auto object_target = ctx.find_object_target(ctx.arg(1));

        if (!actor_target && !object_target) {
            ctx.send_error(fmt::format("Target '{}' not found.", ctx.arg(1)));
            return CommandResult::InvalidTarget;
        }

        // Get triggers based on target type
        FieryMUD::TriggerSet triggers;
        std::string target_name;

        if (actor_target) {
            triggers = trigger_mgr.get_mob_triggers(actor_target->id());
            target_name = actor_target->display_name();
        } else {
            triggers = trigger_mgr.get_object_triggers(object_target->id());
            target_name = object_target->display_name();
        }

        if (triggers.empty()) {
            ctx.send(fmt::format("{} has no triggers attached.", target_name));
            return CommandResult::Success;
        }

        ctx.send(fmt::format("<b:white>Triggers on {}:</>", target_name));
        for (const auto& trigger : triggers) {
            int trig_zone = trigger->zone_id.value_or(
                trigger->mob_id ? static_cast<int>(trigger->mob_id->zone_id()) :
                trigger->object_id ? static_cast<int>(trigger->object_id->zone_id()) : 0);
            ctx.send(fmt::format("  [{}:{}] {} - flags: {}",
                trig_zone, trigger->id,
                trigger->name, trigger->flags_string()));
        }
        ctx.send(fmt::format("Total: {} triggers", triggers.size()));
        return CommandResult::Success;
    }

    if (subcmd == "info") {
        if (ctx.arg_count() < 2) {
            ctx.send_error("Usage: dtrig info <zone:id>");
            return CommandResult::InvalidSyntax;
        }

        // Parse trigger ID (zone:id format, or just id using current zone)
        auto trigger_eid = ctx.parse_entity_id(ctx.arg(1));
        if (!trigger_eid.is_valid()) {
            ctx.send_error("Invalid trigger ID format. Use zone:id (e.g., 489:1) or just id for current zone.");
            return CommandResult::InvalidSyntax;
        }
        int zone_id = static_cast<int>(trigger_eid.zone_id());
        int trigger_id = static_cast<int>(trigger_eid.local_id());

        // Load trigger from database
        auto& pool = ConnectionPool::instance();
        if (!pool.is_initialized()) {
            ctx.send_error("Database not available.");
            return CommandResult::SystemError;
        }

        auto result = pool.execute([zone_id, trigger_id](pqxx::work& txn)
            -> Result<std::tuple<FieryMUD::TriggerDataPtr, std::vector<TriggerQueries::ScriptErrorEntry>>> {
            auto trigger_result = TriggerQueries::load_trigger_by_id(txn, zone_id, trigger_id);
            if (!trigger_result) {
                return std::unexpected(trigger_result.error());
            }
            auto errors_result = TriggerQueries::get_error_log_for_trigger(txn, zone_id, trigger_id, 5);
            std::vector<TriggerQueries::ScriptErrorEntry> errors;
            if (errors_result) {
                errors = *errors_result;
            }
            return std::make_tuple(*trigger_result, errors);
        });

        if (!result) {
            ctx.send_error(fmt::format("Trigger {}:{} not found.", zone_id, trigger_id));
            return CommandResult::InvalidTarget;
        }

        auto& [trigger, errors] = *result;

        ctx.send(fmt::format("<b:white>Trigger {}:{}</> - {}", zone_id, trigger_id, trigger->name));
        ctx.send(fmt::format("Type: {}  Flags: {}",
            trigger->attach_type == FieryMUD::ScriptType::MOB ? "MOB" :
            trigger->attach_type == FieryMUD::ScriptType::OBJECT ? "OBJECT" : "WORLD",
            trigger->flags_string()));

        // Show script (truncated)
        auto script_preview = trigger->commands.substr(0, 200);
        if (trigger->commands.length() > 200) script_preview += "...";
        ctx.send("<b:white>Script:</>");
        ctx.send(script_preview);

        // Show recent errors
        if (errors.empty()) {
            ctx.send("\n<b:green>No recent errors.</>");
        } else {
            ctx.send("\n<b:white>Recent errors:</>");
            for (const auto& error : errors) {
                ctx.send(fmt::format("  [{}] <b:red>{}:</> {}", error.occurred_at, error.error_type, error.error_message));
            }
        }

        return CommandResult::Success;
    }

    if (subcmd == "fire") {
        if (ctx.arg_count() < 3) {
            ctx.send_error("Usage: dtrig fire <target> <zone:id or id>");
            return CommandResult::InvalidSyntax;
        }

        // Find target - try actor first, then object
        auto actor_target = ctx.find_actor_target(ctx.arg(1));
        auto object_target = ctx.find_object_target(ctx.arg(1));

        if (!actor_target && !object_target) {
            ctx.send_error(fmt::format("Target '{}' not found.", ctx.arg(1)));
            return CommandResult::InvalidTarget;
        }

        // Parse trigger ID (zone:id format, or just id using current zone)
        auto trigger_eid = ctx.parse_entity_id(ctx.arg(2));
        if (!trigger_eid.is_valid()) {
            ctx.send_error("Invalid trigger ID format. Use zone:id (e.g., 489:1) or just id for current zone.");
            return CommandResult::InvalidSyntax;
        }

        // Get triggers and target info based on type
        FieryMUD::TriggerSet triggers;
        std::string target_name;
        EntityId target_id;

        if (actor_target) {
            triggers = trigger_mgr.get_mob_triggers(actor_target->id());
            target_name = actor_target->display_name();
            target_id = actor_target->id();
        } else {
            triggers = trigger_mgr.get_object_triggers(object_target->id());
            target_name = object_target->display_name();
            target_id = object_target->id();
        }

        // Find trigger by ID from the target's triggers
        FieryMUD::TriggerDataPtr found_trigger;
        for (const auto& t : triggers) {
            int trig_zone = t->zone_id.value_or(
                t->mob_id ? static_cast<int>(t->mob_id->zone_id()) :
                t->object_id ? static_cast<int>(t->object_id->zone_id()) : 0);
            if (static_cast<std::uint32_t>(trig_zone) == trigger_eid.zone_id() &&
                t->id == static_cast<int>(trigger_eid.local_id())) {
                found_trigger = t;
                break;
            }
        }

        if (!found_trigger) {
            ctx.send_error(fmt::format("Trigger {} not found on {}.", trigger_eid, target_name));
            return CommandResult::InvalidTarget;
        }

        ctx.send(fmt::format("<b:yellow>Firing trigger {} '{}' on {}...</>",
            trigger_eid, found_trigger->name, target_name));

        // Create context and execute based on target type
        FieryMUD::ScriptContext script_ctx;
        if (actor_target) {
            script_ctx = FieryMUD::ScriptContext::Builder()
                .set_trigger(found_trigger)
                .set_owner(actor_target)
                .set_actor(ctx.actor)
                .set_room(actor_target->current_room())
                .build();
        } else {
            script_ctx = FieryMUD::ScriptContext::Builder()
                .set_trigger(found_trigger)
                .set_owner(object_target)
                .set_actor(ctx.actor)
                .set_room(ctx.room)
                .build();
        }

        auto result = trigger_mgr.debug_execute_trigger(found_trigger, script_ctx);

        if (result == FieryMUD::TriggerResult::Error) {
            ctx.send_error(fmt::format("Trigger execution failed: {}", trigger_mgr.last_error()));
            return CommandResult::SystemError;
        }

        ctx.send("<b:green>Trigger executed.</>");
        return CommandResult::Success;
    }

    ctx.send_error(fmt::format("Unknown subcommand: '{}'. Use 'list', 'info', or 'fire'.", subcmd));
    return CommandResult::InvalidSyntax;
}

/**
 * scripterrors - View and manage script errors.
 *
 * Usage:
 *   scripterrors              - List triggers with needsReview=true
 *   scripterrors <zone.id>    - Show error history for specific trigger
 *   scripterrors clear <zone.id> - Clear needsReview flag after fix
 */
Result<CommandResult> cmd_scripterrors(const CommandContext &ctx) {
    auto& pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        ctx.send_error("Database not available.");
        return CommandResult::SystemError;
    }

    if (ctx.arg_count() == 0) {
        // List all triggers needing review
        auto result = pool.execute([](pqxx::work& txn) {
            return TriggerQueries::get_triggers_needing_review(txn);
        });

        if (!result) {
            ctx.send_error(fmt::format("Database error: {}", result.error().message));
            return CommandResult::SystemError;
        }

        if (result->empty()) {
            ctx.send("<b:green>No script errors found! All triggers are clean.</>");
            return CommandResult::Success;
        }

        ctx.send("<b:white>Triggers with errors:</>");
        for (const auto& trigger : *result) {
            int zone_id = trigger->zone_id.value_or(
                trigger->mob_id ? static_cast<int>(trigger->mob_id->zone_id()) :
                trigger->object_id ? static_cast<int>(trigger->object_id->zone_id()) : 0);

            ctx.send(fmt::format("  <b:red>[{}:{}]</> {} - {}",
                zone_id, trigger->id, trigger->name, trigger->flags_string()));
        }
        ctx.send(fmt::format("\nTotal: {} triggers with errors", result->size()));
        ctx.send("Use 'scripterrors <zone.id>' to see error details.");
        return CommandResult::Success;
    }

    auto arg1 = std::string{ctx.arg(0)};
    std::transform(arg1.begin(), arg1.end(), arg1.begin(), ::tolower);

    // scripterrors clear <zone:id>
    if (arg1 == "clear") {
        if (ctx.arg_count() < 2) {
            ctx.send_error("Usage: scripterrors clear <zone:id>");
            return CommandResult::InvalidSyntax;
        }

        // Parse trigger ID (zone:id format, or just id using current zone)
        auto trigger_eid = ctx.parse_entity_id(ctx.arg(1));
        if (!trigger_eid.is_valid()) {
            ctx.send_error("Invalid trigger ID format. Use zone:id (e.g., 489:1) or just id for current zone.");
            return CommandResult::InvalidSyntax;
        }
        int zone_id = static_cast<int>(trigger_eid.zone_id());
        int trigger_id = static_cast<int>(trigger_eid.local_id());

        auto result = pool.execute([zone_id, trigger_id](pqxx::work& txn) {
            return TriggerQueries::clear_trigger_error(txn, zone_id, trigger_id);
        });

        if (!result) {
            ctx.send_error(fmt::format("Failed to clear error: {}", result.error().message));
            return CommandResult::SystemError;
        }

        ctx.send(fmt::format("<b:green>Cleared error flag for trigger {}:{}</>", zone_id, trigger_id));
        return CommandResult::Success;
    }

    // scripterrors <zone:id> - show error history
    auto trigger_eid = ctx.parse_entity_id(arg1);
    if (!trigger_eid.is_valid()) {
        ctx.send_error("Invalid trigger ID format. Use zone:id (e.g., 489:1) or just id for current zone.");
        return CommandResult::InvalidSyntax;
    }
    int zone_id = static_cast<int>(trigger_eid.zone_id());
    int trigger_id = static_cast<int>(trigger_eid.local_id());

    auto result = pool.execute([zone_id, trigger_id](pqxx::work& txn) {
        return TriggerQueries::get_error_log_for_trigger(txn, zone_id, trigger_id, 10);
    });

    if (!result) {
        ctx.send_error(fmt::format("Database error: {}", result.error().message));
        return CommandResult::SystemError;
    }

    if (result->empty()) {
        ctx.send(fmt::format("No errors logged for trigger {}:{}", zone_id, trigger_id));
        return CommandResult::Success;
    }

    ctx.send(fmt::format("<b:white>Error history for trigger {}:{}:</>", zone_id, trigger_id));
    for (const auto& error : *result) {
        ctx.send(fmt::format("  [{}] <b:red>{}:</>", error.occurred_at, error.error_type));
        ctx.send(fmt::format("    {}", error.error_message));
    }

    return CommandResult::Success;
}

/**
 * validate_scripts - Batch validation of script triggers.
 *
 * Usage:
 *   validate_scripts              - Validate all triggers
 *   validate_scripts zone <id>    - Validate triggers in specific zone
 */
Result<CommandResult> cmd_validate_scripts(const CommandContext &ctx) {
    auto& pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        ctx.send_error("Database not available.");
        return CommandResult::SystemError;
    }

    auto& engine = FieryMUD::ScriptEngine::instance();
    if (!engine.is_initialized()) {
        ctx.send_error("Script engine not initialized.");
        return CommandResult::SystemError;
    }

    std::optional<int> zone_filter;
    if (ctx.arg_count() >= 2) {
        auto subcmd = std::string{ctx.arg(0)};
        std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::tolower);
        if (subcmd == "zone") {
            try {
                zone_filter = std::stoi(std::string{ctx.arg(1)});
            } catch (...) {
                ctx.send_error("Invalid zone ID.");
                return CommandResult::InvalidSyntax;
            }
        }
    }

    ctx.send(zone_filter ?
        fmt::format("<b:yellow>Validating scripts in zone {}...</>", *zone_filter) :
        "<b:yellow>Validating all scripts...</>");

    // Load triggers from database
    auto result = pool.execute([zone_filter](pqxx::work& txn)
        -> Result<std::vector<FieryMUD::TriggerDataPtr>> {
        if (zone_filter) {
            return TriggerQueries::load_triggers_for_zone(txn, *zone_filter);
        }
        return TriggerQueries::load_all_triggers(txn);
    });

    if (!result) {
        ctx.send_error(fmt::format("Failed to load triggers: {}", result.error().message));
        return CommandResult::SystemError;
    }

    int total = 0, passed = 0, failed = 0;
    std::vector<std::tuple<int, int, std::string, std::string>> failures; // zone, id, name, error

    for (const auto& trigger : *result) {
        ++total;

        // Try to compile the script
        auto compile_result = engine.compile_script(trigger->commands, trigger->name);
        if (!compile_result) {
            ++failed;
            int zone_id = trigger->zone_id.value_or(
                trigger->mob_id ? static_cast<int>(trigger->mob_id->zone_id()) :
                trigger->object_id ? static_cast<int>(trigger->object_id->zone_id()) : 0);
            failures.emplace_back(zone_id, trigger->id, trigger->name, engine.last_error());

            // Log to database
            pool.execute([zone_id, trigger_id = trigger->id, error = engine.last_error()](pqxx::work& txn) {
                return TriggerQueries::log_script_error(txn, zone_id, trigger_id, "compilation", error);
            });
        } else {
            ++passed;
        }
    }

    // Display results
    ctx.send(fmt::format("\n<b:white>Validation Results:</>"));
    ctx.send(fmt::format("  Total:  {}", total));
    ctx.send(fmt::format("  <b:green>Passed:</> {}", passed));
    ctx.send(fmt::format("  <b:red>Failed:</> {}", failed));

    if (!failures.empty()) {
        ctx.send("\n<b:white>Failed triggers:</>");
        int shown = 0;
        for (const auto& [zone_id, trigger_id, name, error] : failures) {
            if (++shown > 20) {
                ctx.send(fmt::format("  ... and {} more", failures.size() - 20));
                break;
            }
            ctx.send(fmt::format("  <b:red>[{}:{}]</> {} - {}", zone_id, trigger_id, name, error.substr(0, 60)));
        }
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
        .description("Show trigger script by ID")
        .usage("tstat [zone:id]")
        .help(
            "<b:yellow>Show trigger statistics or script by ID:</>\n"
            "  tstat                 - Show trigger system statistics\n"
            "  tstat <zone:id>       - Show full script for trigger\n"
            "\n<b:yellow>Examples:</>\n"
            "  tstat                 - System stats and execution counts\n"
            "  tstat 489:2           - Show script for trigger 489:2\n"
            "  tstat 5               - Show script for trigger 5 (current zone)")
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
        .command("aggrodebug", cmd_aggrodebug)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("Debug mob aggression system in current room")
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

    // Ability debug commands
    Commands()
        .command("alist", cmd_alist)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("List abilities from cache with optional filters")
        .usage("alist [--type spell|skill] [--circle N] [--effect type] [--limit N]")
        .help(
            "List abilities loaded in the AbilityCache.\n"
            "<b:yellow>Options:</>\n"
            "  --type <type>    Filter by ability type (spell, skill, chant, song)\n"
            "  --circle <N>     Filter by spell circle (1-9)\n"
            "  --effect <type>  Filter by effect type (damage, heal, status, etc.)\n"
            "  --limit <N>      Maximum results (default 50)\n"
            "\n<b:yellow>Examples:</>\n"
            "  alist                   - List all abilities\n"
            "  alist --type spell      - List only spells\n"
            "  alist --effect damage   - List abilities with damage effects")
        .build();

    Commands()
        .command("astat", cmd_astat)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("Show detailed ability information including effects")
        .usage("astat <ability name or ID>")
        .help(
            "Display detailed information about an ability.\n"
            "Shows: type, flags, effects, damage components, messages, restrictions.\n"
            "<b:yellow>WARNING indicators:</>\n"
            "  - NO EFFECTS: Ability will cast but do nothing\n"
            "  - Effect not found: Database inconsistency\n"
            "\n<b:yellow>Examples:</>\n"
            "  astat magic missile  - Look up by name\n"
            "  astat 42             - Look up by ID\n"
            "  astat supernova      - Debug a broken spell")
        .build();

    Commands()
        .command("asearch", cmd_asearch)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("Search abilities by name or find broken ones")
        .usage("asearch <name> | --effect <type> | --no-effects")
        .help(
            "Search abilities in the cache.\n"
            "<b:yellow>Options:</>\n"
            "  <name>         Partial name match\n"
            "  --effect <t>   Find abilities with specific effect type\n"
            "  --no-effects   Find abilities with NO effects (broken!)\n"
            "\n<b:yellow>Examples:</>\n"
            "  asearch fire         - Find 'fire' in ability names\n"
            "  asearch --effect heal - Find healing abilities\n"
            "  asearch --no-effects  - Find broken abilities")
        .build();

    Commands()
        .command("areload", cmd_areload)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .description("Reload ability cache from database")
        .help("Reloads all abilities, effects, and related data from the database.")
        .build();

    // Script debugging commands
    Commands()
        .command("syslog", cmd_syslog)
        .category("Development")
        .privilege(PrivilegeLevel::God)
        .description("Subscribe to real-time log messages")
        .usage("syslog [off|<level> [component]]")
        .help(
            "<b:yellow>Subscribe to real-time log messages (like legacy syslog):</>\n"
            "  syslog                    - Show current subscription status\n"
            "  syslog off               - Stop watching logs\n"
            "  syslog <level>           - Watch all logs at level\n"
            "  syslog <level> <comp>    - Watch specific component\n"
            "  syslog <level> all       - Watch all components\n"
            "\n<b:yellow>Levels:</> trace, debug, info, warn, error, critical\n"
            "<b:yellow>Components:</> game, combat, movement, commands, network, persistence, scripting, database")
        .build();

    Commands()
        .command("dtrig", cmd_dtrig)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .description("Debug trigger commands")
        .usage("dtrig <list|info|fire> <args>")
        .help(
            "<b:yellow>Debug trigger commands for script development:</>\n"
            "  dtrig list <target>       - List all triggers on mob/object\n"
            "  dtrig info <zone:id>      - Show trigger details and recent errors\n"
            "  dtrig fire <target> <id>  - Manually execute a trigger by ID\n"
            "\n<b:yellow>Examples:</>\n"
            "  dtrig list lokari         - List triggers on mob 'lokari'\n"
            "  dtrig info 489:1          - Show details for trigger 489:1\n"
            "  dtrig fire lokari 489:1   - Fire trigger 489:1 on lokari\n"
            "  dtrig fire lokari 5       - Fire trigger 5 (current zone) on lokari")
        .build();

    Commands()
        .command("scripterrors", cmd_scripterrors)
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .description("View and manage script errors")
        .usage("scripterrors [zone:id|clear zone:id]")
        .help(
            "<b:yellow>View and manage script errors logged from trigger execution:</>\n"
            "  scripterrors              - List all triggers with errors\n"
            "  scripterrors <zone:id>    - Show error history for trigger\n"
            "  scripterrors clear <zone:id> - Clear error flag after fixing\n"
            "\n<b:yellow>Examples:</>\n"
            "  scripterrors              - See all broken triggers\n"
            "  scripterrors 489:1        - See error history for trigger 489:1\n"
            "  scripterrors clear 489:1  - Mark trigger as fixed")
        .build();

    Commands()
        .command("validate_scripts", cmd_validate_scripts)
        .alias("vscripts")
        .category("Development")
        .privilege(PrivilegeLevel::Coder)
        .description("Batch validate script triggers")
        .usage("validate_scripts [zone <id>]")
        .help(
            "<b:yellow>Batch validate all script triggers for compilation errors:</>\n"
            "  validate_scripts          - Validate ALL triggers\n"
            "  validate_scripts zone <id> - Validate triggers in specific zone\n"
            "\n<b:yellow>Examples:</>\n"
            "  validate_scripts          - Check all triggers across all zones\n"
            "  validate_scripts zone 489 - Check only zone 489 triggers")
        .build();

    return Success();
}

} // namespace AdminCommands

/***************************************************************************
 *   File: src/commands/builtin_commands.cpp           Part of FieryMUD *
 *  Usage: Core built-in command implementations                          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "builtin_commands.hpp"
// #include "clan_commands.hpp"  // Temporarily disabled

#include "../core/actor.hpp"
#include "../core/combat.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "../game/player_output.hpp"
#include "../world/room.hpp"
#include "../world/weather.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace BuiltinCommands {

// Forward declarations for combat helpers
namespace CombatHelpers {
Result<CommandResult> perform_attack(const CommandContext &ctx, std::shared_ptr<Actor> target);
bool is_valid_target(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target);
} // namespace CombatHelpers

// Forward declarations
Result<CommandResult> cmd_prompt(const CommandContext &ctx);

// Forward declarations for zone development commands
Result<CommandResult> cmd_reload_zone(const CommandContext &ctx);
Result<CommandResult> cmd_save_zone(const CommandContext &ctx);
Result<CommandResult> cmd_reload_all_zones(const CommandContext &ctx);
Result<CommandResult> cmd_toggle_file_watching(const CommandContext &ctx);
Result<CommandResult> cmd_dump_world(const CommandContext &ctx);

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_all_commands() {
    Log::info("Registering built-in commands...");

    // Information Commands
    Commands().command("look", cmd_look).alias("l").category("Information").privilege(PrivilegeLevel::Player).build();
    Commands().command("examine", cmd_examine).category("Information").privilege(PrivilegeLevel::Player).build();
    Commands().command("who", cmd_who).category("Information").privilege(PrivilegeLevel::Player).build();
    Commands().command("where", cmd_where).category("Information").privilege(PrivilegeLevel::Player).build();
    Commands()
        .command("inventory", cmd_inventory)
        .alias("i")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();
    Commands().command("equipment", cmd_equipment).category("Information").privilege(PrivilegeLevel::Player).build();
    Commands().command("score", cmd_score).category("Information").privilege(PrivilegeLevel::Player).build();
    Commands().command("time", cmd_time).category("Information").privilege(PrivilegeLevel::Player).build();
    Commands().command("weather", cmd_weather).category("Information").privilege(PrivilegeLevel::Player).build();

    // Communication Commands
    Commands().command("say", cmd_say).alias("'").category("Communication").privilege(PrivilegeLevel::Player).build();
    Commands().command("tell", cmd_tell).category("Communication").privilege(PrivilegeLevel::Player).build();
    Commands().command("emote", cmd_emote).category("Communication").privilege(PrivilegeLevel::Player).build();
    Commands().command("whisper", cmd_whisper).category("Communication").privilege(PrivilegeLevel::Player).build();
    Commands().command("shout", cmd_shout).category("Communication").privilege(PrivilegeLevel::Player).build();
    Commands().command("gossip", cmd_gossip).category("Communication").privilege(PrivilegeLevel::Player).build();

    // Movement Commands
    Commands()
        .command("north", cmd_north)
        .alias("n")
        .category("Movement")
        .description("Move north")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("south", cmd_south)
        .alias("s")
        .category("Movement")
        .description("Move south")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("east", cmd_east)
        .alias("e")
        .category("Movement")
        .description("Move east")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("west", cmd_west)
        .alias("w")
        .category("Movement")
        .description("Move west")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("up", cmd_up)
        .alias("u")
        .category("Movement")
        .description("Move up")
        .usable_while_sitting(false)
        .build();

    Commands()
        .command("down", cmd_down)
        .alias("d")
        .category("Movement")
        .description("Move down")
        .usable_while_sitting(false)
        .build();

    Commands().command("exits", cmd_exits).category("Movement").privilege(PrivilegeLevel::Player).build();

    // Combat Commands
    Commands().command("kill", cmd_kill).category("Combat").privilege(PrivilegeLevel::Player).build();
    Commands().command("hit", cmd_hit).category("Combat").privilege(PrivilegeLevel::Player).build();
    Commands().command("cast", cmd_cast).category("Combat").privilege(PrivilegeLevel::Player).build();
    Commands().command("flee", cmd_flee).category("Combat").privilege(PrivilegeLevel::Player).build();
    Commands().command("release", cmd_release).category("Death").privilege(PrivilegeLevel::Player).build();

    // Object Commands
    Commands().command("get", cmd_get).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("drop", cmd_drop).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("put", cmd_put).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("give", cmd_give).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("wear", cmd_wear).alias("equip").category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("open", cmd_open).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("close", cmd_close).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("lock", cmd_lock).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("unlock", cmd_unlock).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("light", cmd_light).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("eat", cmd_eat).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("drink", cmd_drink).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands()
        .command("remove", cmd_remove)
        .alias("unequip")
        .category("Object")
        .privilege(PrivilegeLevel::Player)
        .build();

    // System Commands
    Commands().command("quit", cmd_quit).category("System").privilege(PrivilegeLevel::Player).build();
    Commands().command("save", cmd_save).category("System").privilege(PrivilegeLevel::Player).build();
    Commands().command("help", cmd_help).category("System").privilege(PrivilegeLevel::Player).build();
    Commands().command("commands", cmd_commands).category("System").privilege(PrivilegeLevel::Player).build();
    Commands().command("prompt", cmd_prompt).category("System").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("testcmd",
                 [](const CommandContext &ctx) -> Result<CommandResult> {
                     ctx.send_line("Test command executed.");
                     return CommandResult::Success;
                 })
        .category("System")
        .description("A test command that returns instantly.")
        .build();

    // Social Commands
    Commands().command("smile", cmd_smile).category("Social").privilege(PrivilegeLevel::Player).build();
    Commands().command("nod", cmd_nod).category("Social").privilege(PrivilegeLevel::Player).build();
    Commands().command("wave", cmd_wave).category("Social").privilege(PrivilegeLevel::Player).build();
    Commands().command("bow", cmd_bow).category("Social").privilege(PrivilegeLevel::Player).build();
    Commands().command("laugh", cmd_laugh).category("Social").privilege(PrivilegeLevel::Player).build();

    // Administrative Commands
    Commands()
        .command("shutdown", cmd_shutdown)
        .category("Administration")
        .privilege(PrivilegeLevel::Overlord)
        .permission(Permissions::SHUTDOWN)
        .description("Shutdown the MUD")
        .build();

    Commands()
        .command("goto", cmd_goto)
        .category("Administration")
        .privilege(PrivilegeLevel::Builder)
        .permission(Permissions::TELEPORT)
        .description("Teleport to a room")
        .build();

    Commands()
        .command("teleport", cmd_teleport)
        .alias("trans")
        .category("Administration")
        .privilege(PrivilegeLevel::Builder)
        .permission(Permissions::TRANSFER)
        .description("Teleport a player to a room")
        .build();

    Commands()
        .command("summon", cmd_summon)
        .category("Administration")
        .privilege(PrivilegeLevel::Builder)
        .permission(Permissions::SUMMON)
        .description("Summon a player to your location")
        .build();

    Commands()
        .command("setweather", cmd_weather_control)
        .alias("weather_control")
        .category("Administration")
        .privilege(PrivilegeLevel::Builder)
        .description("Control weather conditions")
        .build();

    // Zone Development Commands
    Commands()
        .command("reloadzone", cmd_reload_zone)
        .alias("rzreload")
        .category("Development")
        .privilege(PrivilegeLevel::Builder)
        .description("Reload a zone from its JSON file on disk")
        .build();

    Commands()
        .command("savezone", cmd_save_zone)
        .alias("rzsave")
        .category("Development")
        .privilege(PrivilegeLevel::Builder)
        .description("Save a zone's current state to its JSON file on disk")
        .build();

    Commands()
        .command("reloadallzones", cmd_reload_all_zones)
        .alias("rzreloadall")
        .category("Development")
        .privilege(PrivilegeLevel::Builder)
        .description("Reload all zones from their JSON files on disk")
        .build();

    Commands()
        .command("filewatch", cmd_toggle_file_watching)
        .alias("watch")
        .category("Development")
        .privilege(PrivilegeLevel::Builder)
        .description("Toggle automatic file watching for zone hot-reload")
        .build();

    Commands()
        .command("dumpworld", cmd_dump_world)
        .alias("worlddump")
        .category("Development")
        .privilege(PrivilegeLevel::Builder)
        .description("Dump current world state to a JSON file")
        .build();

    // Register clan commands - temporarily disabled
    // if (auto result = ClanCommands::register_all_clan_commands(); !result) {
    //     Log::error("Failed to register clan commands: {}", result.error());
    //     return result;
    // }

    Log::info("Built-in commands registered successfully");
    return Success();
}

void unregister_all_commands() {
    // Command system handles cleanup automatically
    Log::info("Built-in commands unregistered");
}

// =============================================================================
// Information Commands
// =============================================================================

Result<CommandResult> cmd_look(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        // Look at room - get current room from actor, not from context
        auto current_room = ctx.actor ? ctx.actor->current_room() : nullptr;
        if (!current_room) {
            ctx.send_error("You are not in a room.");
            return CommandResult::InvalidState;
        }

        // Use the room's own get_room_description method which properly handles lighting
        std::string description = current_room->get_room_description(ctx.actor.get());

        // Only add additional formatting (exits, objects, actors) if the room is visible
        if (!current_room->can_see_in_room(ctx.actor.get())) {
            // Room is dark, just send the darkness message
            ctx.send(description);
            return CommandResult::Success;
        }

        // Room is visible, add room name as header, then description, exits, objects, and actors
        std::ostringstream full_desc;
        full_desc << current_room->name() << "\n";
        full_desc << description << "\n";

        // Add exits
        std::string exits = Helpers::format_exits(current_room);
        if (!exits.empty()) {
            full_desc << fmt::format("\n{}\n", exits);
        }

        // Add objects in room
        auto objects = current_room->contents().objects;
        if (!objects.empty()) {
            full_desc << "\nYou see:\n";
            for (const auto &obj : objects) {
                if (obj) {
                    full_desc << fmt::format("  {}\n", obj->short_description());
                }
            }
        }

        // Add other actors
        auto actors = current_room->contents().actors;
        bool found_others = false;
        for (const auto &actor : actors) {
            if (actor && actor != ctx.actor) {
                if (!found_others) {
                    full_desc << "\nAlso here:\n";
                    found_others = true;
                }
                full_desc << fmt::format("  {}\n", actor->short_description());
            }
        }

        ctx.send(full_desc.str());
        return CommandResult::Success;
    }

    // Look at target
    auto target_info = ctx.resolve_target(ctx.arg(0));
    if (!target_info.is_valid()) {
        ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    std::string description;
    switch (target_info.type) {
    case TargetType::Actor:
        description = Helpers::format_actor_description(target_info.actor, ctx.actor);
        break;
    case TargetType::Object:
        description = Helpers::format_object_description(target_info.object, ctx.actor);

        // Check for extra descriptions if there's a second argument
        if (ctx.arg_count() >= 2) {
            auto extra_desc = target_info.object->get_extra_description(ctx.arg(1));
            if (extra_desc.has_value()) {
                description = std::string(extra_desc.value());
            } else {
                ctx.send_error(fmt::format("You don't see anything special about the {} when looking at '{}'.",
                                           ctx.format_object_name(target_info.object), ctx.arg(1)));
                return CommandResult::InvalidTarget;
            }
        }
        break;
    case TargetType::Room:
        description = Helpers::format_room_description(target_info.room, ctx.actor);
        break;
    case TargetType::Direction: {
        if (!ctx.room || !ctx.room->has_exit(target_info.direction)) {
            ctx.send_error("There's no exit in that direction.");
            return CommandResult::InvalidTarget;
        }
        auto exit_info = ctx.room->get_exit(target_info.direction);
        if (exit_info && exit_info->to_room.is_valid()) {
            description =
                fmt::format("You look {} and see an exit.", RoomUtils::get_direction_name(target_info.direction));
        } else {
            description = "You can't see anything in that direction.";
        }
        break;
    }
    default:
        ctx.send_error("You can't look at that.");
        return CommandResult::InvalidTarget;
    }

    ctx.send(description);
    return CommandResult::Success;
}

Result<CommandResult> cmd_examine(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<target>"); !result) {
        ctx.send_error(result.error().message);
        return CommandResult::InvalidSyntax;
    }

    auto target_info = ctx.resolve_target(ctx.arg(0));
    if (!target_info.is_valid()) {
        ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    std::ostringstream detailed_desc;

    switch (target_info.type) {
    case TargetType::Object: {
        auto obj = target_info.object;

        // Basic description
        detailed_desc << fmt::format("=== {} ===\n", obj->short_description());
        detailed_desc << fmt::format("{}\n", obj->description());

        // Technical details
        detailed_desc << "\n--- Technical Details ---\n";
        detailed_desc << fmt::format("Type: {}\n", obj->type_name());
        detailed_desc << fmt::format("Weight: {} pounds\n", obj->weight());
        if (obj->value() > 0) {
            detailed_desc << fmt::format("Value: {} gold coins\n", obj->value());
        }
        detailed_desc << fmt::format("Condition: {} ({} - {}%)\n", obj->quality_description(), obj->condition(),
                                     obj->condition());

        // Equipment slot information
        if (obj->is_wearable()) {
            detailed_desc << fmt::format("Worn on: {}\n", ObjectUtils::get_slot_name(obj->equip_slot()));
        }

        // Type-specific details
        if (obj->is_weapon()) {
            const auto &damage = obj->damage_profile();
            detailed_desc << fmt::format("Weapon damage: {} (avg: {:.1f})\n", damage.to_dice_string(),
                                         damage.average_damage());
            if (auto weapon = std::dynamic_pointer_cast<Weapon>(obj)) {
                detailed_desc << fmt::format("Reach: {}, Speed: {}\n", weapon->reach(), weapon->speed());
            }
        }

        if (obj->is_armor()) {
            detailed_desc << fmt::format("Armor class: {}\n", obj->armor_class());
            if (auto armor = std::dynamic_pointer_cast<Armor>(obj)) {
                detailed_desc << fmt::format("Material: {}\n", armor->material());
            }
        }

        // Container details
        if (obj->is_container()) {
            const auto &container_info = obj->container_info();
            detailed_desc << "\n--- Container Properties ---\n";
            detailed_desc << fmt::format("Capacity: {} items\n", container_info.capacity);
            detailed_desc << fmt::format("Weight limit: {} pounds\n", container_info.weight_capacity);
            detailed_desc << fmt::format("State: {}\n", container_info.closed ? "closed" : "open");

            if (container_info.lockable) {
                detailed_desc << fmt::format("Lock: {} ({})\n", container_info.locked ? "locked" : "unlocked",
                                             container_info.key_id.is_valid() ? "requires key" : "no key needed");
            }

            // Show contents if open and accessible
            if (!container_info.closed) {
                if (auto container_obj = std::dynamic_pointer_cast<Container>(obj)) {
                    const auto &contents = container_obj->get_contents();
                    if (!contents.empty()) {
                        detailed_desc << "\n--- Contents ---\n";
                        for (const auto &item : contents) {
                            if (item) {
                                detailed_desc << fmt::format("  {}\n", item->short_description());
                            }
                        }
                    } else {
                        detailed_desc << "The container is empty.\n";
                    }
                }
            }
        }

        // Light source details
        if (obj->is_light_source()) {
            const auto &light_info = obj->light_info();
            detailed_desc << "\n--- Light Properties ---\n";
            detailed_desc << fmt::format("Brightness: {}\n", light_info.brightness);
            detailed_desc << fmt::format("Duration: {} hours\n", light_info.duration);
            detailed_desc << fmt::format("Status: {}\n", light_info.lit ? "lit" : "unlit");
        }

        // Object flags
        detailed_desc << "\n--- Properties ---\n";
        std::vector<std::string> flags;
        if (obj->has_flag(ObjectFlag::Magic))
            flags.push_back("magical");
        if (obj->has_flag(ObjectFlag::Glow))
            flags.push_back("glowing");
        if (obj->has_flag(ObjectFlag::Hum))
            flags.push_back("humming");
        if (obj->has_flag(ObjectFlag::Invisible))
            flags.push_back("invisible");
        if (obj->has_flag(ObjectFlag::Cursed))
            flags.push_back("cursed");
        if (obj->has_flag(ObjectFlag::Bless))
            flags.push_back("blessed");
        if (obj->has_flag(ObjectFlag::NoDrop))
            flags.push_back("no drop");
        if (obj->has_flag(ObjectFlag::Unique))
            flags.push_back("unique");
        if (obj->has_flag(ObjectFlag::TwoHanded))
            flags.push_back("two-handed");

        if (flags.empty()) {
            detailed_desc << "No special properties.\n";
        } else {
            detailed_desc << fmt::format("Special: {}\n",
                                         CommandParserUtils::join(std::span<const std::string>{flags}, ", "));
        }

        // Extra descriptions
        const auto &extra_descs = obj->get_all_extra_descriptions();
        if (!extra_descs.empty()) {
            detailed_desc << "\n--- Additional Details ---\n";
            detailed_desc << "You can examine specific features: ";
            std::vector<std::string> all_keywords;
            for (const auto &extra : extra_descs) {
                // Combine all keywords from this extra description
                for (const auto &keyword : extra.keywords) {
                    all_keywords.push_back(keyword);
                }
            }
            detailed_desc << fmt::format("{}\n",
                                         CommandParserUtils::join(std::span<const std::string>{all_keywords}, ", "));
        }

        break;
    }
    case TargetType::Actor:
        // Enhanced actor examination
        detailed_desc << Helpers::format_actor_description(target_info.actor, ctx.actor);
        detailed_desc << "\n\nDetailed examination shows their equipment and status...";
        break;
    default:
        // Fall back to look for other types
        return cmd_look(ctx);
    }

    ctx.send(detailed_desc.str());
    return CommandResult::Success;
}

Result<CommandResult> cmd_who(const CommandContext &ctx) {
    // TODO: Implement actual online actors retrieval
    std::vector<std::shared_ptr<Actor>> online_actors;

    if (online_actors.empty()) {
        ctx.send_line("No players are currently online.");
        return CommandResult::Success;
    }

    std::string who_list = Helpers::format_who_list(online_actors);
    ctx.send(who_list);

    return CommandResult::Success;
}

Result<CommandResult> cmd_where(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        // Show current location
        if (!ctx.room) {
            ctx.send_error("You are nowhere.");
            return CommandResult::InvalidState;
        }

        ctx.send_line(fmt::format("You are in: {} [{}]", ctx.room->name(), ctx.room->id().value()));
        return CommandResult::Success;
    }

    // Show location of target
    auto target = ctx.find_actor_global(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not online.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto target_room = target->current_room();
    if (!target_room) {
        ctx.send_line(fmt::format("{} is nowhere.", target->name()));
    } else {
        ctx.send_line(fmt::format("{} is in: {} [{}]", target->name(), target_room->name(), target_room->id().value()));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_inventory(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::string inventory = Helpers::format_inventory(ctx.actor);
    ctx.send(inventory);

    return CommandResult::Success;
}

Result<CommandResult> cmd_equipment(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::string equipment = Helpers::format_equipment(ctx.actor);
    ctx.send(equipment);

    return CommandResult::Success;
}

Result<CommandResult> cmd_score(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::ostringstream score;
    score << fmt::format("Name: {}\n", ctx.actor->name());
    score << fmt::format("Level: {}\n", ctx.actor->stats().level);
    score << fmt::format("Health: {}/{}\n", ctx.actor->stats().hit_points, ctx.actor->stats().max_hit_points);

    // Add more stats as the Actor class gets expanded
    if (ctx.room) {
        score << fmt::format("Location: {} [{}]\n", ctx.room->name(), ctx.room->id().value());
    }

    ctx.send(score.str());
    return CommandResult::Success;
}

Result<CommandResult> cmd_time(const CommandContext &ctx) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream time_str;
    time_str << "Current time: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

    ctx.send_line(time_str.str());
    return CommandResult::Success;
}

Result<CommandResult> cmd_weather(const CommandContext &ctx) {
    auto current_room = ctx.actor->current_room();
    if (!current_room) {
        ctx.send_error("You are not in a valid location.");
        return CommandResult::ResourceError;
    }

    EntityId zone_id = current_room->zone_id();

    // Get current weather state for the zone
    auto weather_state = Weather().get_zone_weather(zone_id);
    auto weather_effects = Weather().get_weather_effects(zone_id);

    // Format weather description
    std::ostringstream weather_report;
    weather_report << "\n&C--- Weather Report ---&n\n";
    weather_report << weather_state.get_description() << "\n\n";

    // Add weather effects information
    if (weather_effects.visibility_modifier < 1.0f) {
        weather_report << "Visibility is reduced due to weather conditions.\n";
    }
    if (weather_effects.movement_modifier < 1.0f) {
        weather_report << "Movement is hindered by the weather.\n";
    }
    if (weather_effects.provides_water) {
        weather_report << "You can collect water from the precipitation.\n";
    }
    if (weather_effects.blocks_flying) {
        weather_report << "The weather makes flying dangerous.\n";
    }
    if (weather_effects.lightning_chance) {
        weather_report << "Lightning flashes in the distance.\n";
    }

    // Add forecast if available
    auto forecast = Weather().get_forecast(zone_id, std::chrono::hours(6));
    if (!forecast.empty()) {
        weather_report << "\n&YUpcoming Weather:&n\n";
        for (const auto &entry : forecast) {
            weather_report << entry.describe() << "\n";
        }
    }

    ctx.send(weather_report.str());
    return CommandResult::Success;
}

// =============================================================================
// Communication Commands
// =============================================================================

Result<CommandResult> cmd_say(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("say <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    Helpers::send_communication(ctx, message, MessageType::Say, "say");

    return CommandResult::Success;
}

Result<CommandResult> cmd_tell(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<player> <message>"); !result) {
        ctx.send_usage("tell <player> <message>");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_global(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not online.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send_error("You can't tell yourself.");
        return CommandResult::InvalidTarget;
    }

    std::string message = ctx.args_from(1);

    // Send to target
    std::string target_msg = fmt::format("{} tells you, '{}'", ctx.actor->name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("You tell {}, '{}'", target->name(), message);
    ctx.send(sender_msg);

    return CommandResult::Success;
}

Result<CommandResult> cmd_emote(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<action>"); !result) {
        ctx.send_usage("emote <action>");
        return CommandResult::InvalidSyntax;
    }

    std::string action = ctx.args_from(0);
    std::string emote_msg = fmt::format("{} {}", ctx.actor->name(), action);

    // Send to everyone in the room including self - emotes show the same message to everyone
    ctx.send_to_room(emote_msg, false); // Include self - everyone sees the same thing

    return CommandResult::Success;
}

Result<CommandResult> cmd_whisper(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<player> <message>"); !result) {
        ctx.send_usage("whisper <player> <message>");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send_error("You can't whisper to yourself.");
        return CommandResult::InvalidTarget;
    }

    std::string message = ctx.args_from(1);

    // Send to target
    std::string target_msg = fmt::format("{} whispers to you, '{}'", ctx.actor->name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("You whisper to {}, '{}'", target->name(), message);
    ctx.send(sender_msg);

    // Let others see the whisper (but not the content)
    std::string room_msg = fmt::format("{} whispers something to {}.", ctx.actor->name(), target->name());
    ctx.send_to_room(room_msg, true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_shout(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("shout <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    Helpers::send_communication(ctx, message, MessageType::Broadcast, "shout");

    return CommandResult::Success;
}

Result<CommandResult> cmd_gossip(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("gossip <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    Helpers::send_communication(ctx, message, MessageType::Channel, "gossip");

    return CommandResult::Success;
}

// =============================================================================
// Movement Commands
// =============================================================================

Result<CommandResult> cmd_north(const CommandContext &ctx) { return Helpers::execute_movement(ctx, Direction::North); }

Result<CommandResult> cmd_south(const CommandContext &ctx) { return Helpers::execute_movement(ctx, Direction::South); }

Result<CommandResult> cmd_east(const CommandContext &ctx) { return Helpers::execute_movement(ctx, Direction::East); }

Result<CommandResult> cmd_west(const CommandContext &ctx) { return Helpers::execute_movement(ctx, Direction::West); }

Result<CommandResult> cmd_up(const CommandContext &ctx) { return Helpers::execute_movement(ctx, Direction::Up); }

Result<CommandResult> cmd_down(const CommandContext &ctx) { return Helpers::execute_movement(ctx, Direction::Down); }

Result<CommandResult> cmd_exits(const CommandContext &ctx) {
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    std::string exits = Helpers::format_exits(ctx.room);
    ctx.send_line(exits);

    return CommandResult::Success;
}

// =============================================================================
// Object Commands
// =============================================================================

Result<CommandResult> cmd_get(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("get <object> [from <container>]");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    std::shared_ptr<Object> target_object = nullptr;
    std::shared_ptr<Object> source_container = nullptr;

    // Check if this is "get <object> from <container>" syntax
    if (ctx.arg_count() >= 3 && ctx.arg(1) == "from") {
        // Get from container
        auto inventory_items = ctx.actor->inventory().get_all_items();

        // First check inventory for the object
        std::shared_ptr<Object> potential_container = nullptr;
        for (const auto &obj : inventory_items) {
            if (obj && obj->matches_keyword(ctx.arg(2))) {
                potential_container = obj;
                if (obj->is_container()) {
                    source_container = obj;
                }
                break;
            }
        }

        // If not found in inventory, check room
        if (!potential_container) {
            auto &room_objects = ctx.room->contents_mutable().objects;
            for (auto &obj : room_objects) {
                if (obj && obj->matches_keyword(ctx.arg(2))) {
                    potential_container = obj;
                    if (obj->is_container()) {
                        source_container = obj;
                    }
                    break;
                }
            }
        }

        if (!potential_container) {
            ctx.send_error(fmt::format("You don't see a container called '{}' here.", ctx.arg(2)));
            return CommandResult::InvalidTarget;
        }

        if (!source_container) {
            ctx.send_error(fmt::format("You can't get anything from {} - it's not a container.", ctx.format_object_name(potential_container)));
            return CommandResult::InvalidTarget;
        }

        // Check container properties
        const auto &container_info = source_container->container_info();

        // Check if container is closed
        if (container_info.closeable && container_info.closed) {
            ctx.send_error(fmt::format("The {} is closed.", ctx.format_object_name(source_container)));
            return CommandResult::InvalidState;
        }

        // Cast to Container to access container functionality
        auto container_obj = std::dynamic_pointer_cast<Container>(source_container);
        if (!container_obj) {
            ctx.send_error(fmt::format("You can't get anything from {}.", ctx.format_object_name(source_container)));
            return CommandResult::InvalidState;
        }

        // Search for the object in the container
        auto items_found = container_obj->find_items_by_keyword(ctx.arg(0));
        if (items_found.empty()) {
            ctx.send_error(fmt::format("There's no '{}' in {}.", ctx.arg(0), ctx.format_object_name(source_container)));
            return CommandResult::InvalidTarget;
        }

        // Get the first matching item
        auto item_to_get = items_found[0];

        // Check if actor can carry more weight
        int object_weight = item_to_get->weight();
        if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
            ctx.send_error("You can't carry any more.");
            return CommandResult::ResourceError;
        }

        // Remove from container and add to inventory
        if (!container_obj->remove_item(item_to_get)) {
            ctx.send_error("Failed to get item from container.");
            return CommandResult::ResourceError;
        }

        auto add_result = ctx.actor->inventory().add_item(item_to_get);
        if (!add_result) {
            // Return item to container if inventory add failed
            container_obj->add_item(item_to_get);
            ctx.send_error(add_result.error().message);
            return CommandResult::ResourceError;
        }

        ctx.send_success(fmt::format("You get {} from {}.", ctx.format_object_name(item_to_get),
                                     ctx.format_object_name(source_container)));
        ctx.send_to_room(fmt::format("{} gets {} from {}.", ctx.actor->name(), ctx.format_object_name(item_to_get),
                                     ctx.format_object_name(source_container)),
                         true);

        return CommandResult::Success;
    } else {
        // Get from room (original functionality)
        auto &room_objects = ctx.room->contents_mutable().objects;

        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(ctx.arg(0))) {
                target_object = obj;
                break;
            }
        }

        if (!target_object) {
            ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }

        // Check if actor can carry more weight
        int object_weight = 1; // Default weight for now
        if (!ctx.actor->inventory().can_carry(object_weight, ctx.actor->max_carry_weight())) {
            ctx.send_error("You can't carry any more.");
            return CommandResult::ResourceError;
        }

        // Remove from room and add to inventory
        auto it = std::find(room_objects.begin(), room_objects.end(), target_object);
        if (it != room_objects.end()) {
            room_objects.erase(it);
        }

        auto add_result = ctx.actor->inventory().add_item(target_object);
        if (!add_result) {
            // Return object to room if inventory add failed
            room_objects.push_back(target_object);
            ctx.send_error(add_result.error().message);
            return CommandResult::ResourceError;
        }

        ctx.send_success(fmt::format("You get {}.", ctx.format_object_name(target_object)));
        ctx.send_to_room(fmt::format("{} gets {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

        return CommandResult::Success;
    }
}

Result<CommandResult> cmd_drop(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("drop <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Find object in actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Remove from inventory
    if (!ctx.actor->inventory().remove_item(target_object)) {
        ctx.send_error("You can't drop that.");
        return CommandResult::ResourceError;
    }

    // Add to room
    ctx.room->contents_mutable().objects.push_back(target_object);

    ctx.send_success(fmt::format("You drop {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} drops {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_put(const CommandContext &ctx) {
    if (ctx.arg_count() < 2) {
        ctx.send_usage("put <object> [in] <container>");
        return CommandResult::InvalidSyntax;
    }
    
    // Handle both "put object container" and "put object in container" syntax
    std::string_view object_name = ctx.arg(0);
    std::string_view container_name;
    
    if (ctx.arg_count() >= 3 && ctx.arg(1) == "in") {
        // "put object in container" syntax
        container_name = ctx.arg(2);
    } else {
        // "put object container" syntax  
        container_name = ctx.arg(1);
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Find object in actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> item_to_put = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(object_name)) {
            item_to_put = obj;
            break;
        }
    }

    if (!item_to_put) {
        ctx.send_error(fmt::format("You don't have '{}'.", object_name));
        return CommandResult::InvalidTarget;
    }

    // Find container in the room or inventory
    std::shared_ptr<Object> container = nullptr;
    std::shared_ptr<Object> potential_container = nullptr;

    // First check inventory 
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(container_name) && obj != item_to_put) {
            potential_container = obj;
            if (obj->is_container()) {
                container = obj;
            }
            break;
        }
    }

    // If not found in inventory, check room
    if (!potential_container) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(container_name)) {
                potential_container = obj;
                if (obj->is_container()) {
                    container = obj;
                }
                break;
            }
        }
    }

    if (!potential_container) {
        ctx.send_error(fmt::format("You don't see '{}'.", container_name));
        return CommandResult::InvalidTarget;
    }

    if (!container) {
        ctx.send_error(fmt::format("You can't put anything in {} - it's not a container.", ctx.format_object_name(potential_container)));
        return CommandResult::InvalidTarget;
    }

    // Check container properties
    const auto &container_info = container->container_info();

    // Check if container is closed
    if (container_info.closeable && container_info.closed) {
        ctx.send_error(fmt::format("The {} is closed.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Cast to Container to access container functionality
    auto container_obj = std::dynamic_pointer_cast<Container>(container);
    if (!container_obj) {
        ctx.send_error(fmt::format("You can't put anything in {}.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Try to add item to container
    auto add_result = container_obj->add_item(item_to_put);
    if (!add_result) {
        ctx.send_error(fmt::format("The {} is full.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Remove from actor's inventory
    if (!ctx.actor->inventory().remove_item(item_to_put)) {
        // If we can't remove from inventory, remove from container to maintain consistency
        container_obj->remove_item(item_to_put);
        ctx.send_error("Failed to remove item from inventory.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(
        fmt::format("You put {} in {}.", ctx.format_object_name(item_to_put), ctx.format_object_name(container)));
    ctx.send_to_room(fmt::format("{} puts {} in {}.", ctx.actor->name(), ctx.format_object_name(item_to_put),
                                 ctx.format_object_name(container)),
                     true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_give(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<object> <player>"); !result) {
        ctx.send_usage("give <object> <player>");
        return CommandResult::InvalidSyntax;
    }

    auto obj = ctx.find_object_target(ctx.arg(0));
    if (!obj) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    auto target = ctx.find_actor_target(ctx.arg(1));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(1)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send_error("You can't give something to yourself.");
        return CommandResult::InvalidTarget;
    }

    // TODO: Implement actual give logic
    ctx.send_success(fmt::format("You give {} to {}.", ctx.format_object_name(obj), target->name()));

    ctx.send_to_actor(target, fmt::format("{} gives you {}.", ctx.actor->name(), ctx.format_object_name(obj)));

    ctx.send_to_room(fmt::format("{} gives {} to {}.", ctx.actor->name(), ctx.format_object_name(obj), target->name()),
                     true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_wear(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("wear <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find object in actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> target_object = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You don't have '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Try to equip the item
    auto equip_result = ctx.actor->equipment().equip_item(target_object);
    if (!equip_result) {
        ctx.send_error(
            fmt::format("You can't wear {}: {}", ctx.format_object_name(target_object), equip_result.error().message));
        return CommandResult::ResourceError;
    }

    // Remove from inventory since it's now equipped
    ctx.actor->inventory().remove_item(target_object);

    ctx.send_success(fmt::format("You wear {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} wears {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_remove(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("remove <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find object in actor's equipment
    auto equipped_items = ctx.actor->equipment().get_all_equipped();
    std::shared_ptr<Object> target_object = nullptr;
    EntityId target_item_id = INVALID_ENTITY_ID;

    for (const auto &obj : equipped_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            target_object = obj;
            target_item_id = obj->id();
            break;
        }
    }

    if (!target_object) {
        ctx.send_error(fmt::format("You're not wearing '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Unequip the item
    auto unequipped_item = ctx.actor->equipment().unequip_item(target_item_id);
    if (!unequipped_item) {
        ctx.send_error("You can't remove that.");
        return CommandResult::ResourceError;
    }

    // Add back to inventory
    auto add_result = ctx.actor->inventory().add_item(unequipped_item);
    if (!add_result) {
        // If can't add to inventory, put back in equipment
        auto reequip_result = ctx.actor->equipment().equip_item(unequipped_item);
        if (!reequip_result) {
            Log::error("Failed to reequip item after inventory add failure");
        }
        ctx.send_error("You can't carry any more items.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You remove {}.", ctx.format_object_name(target_object)));
    ctx.send_to_room(fmt::format("{} removes {}.", ctx.actor->name(), ctx.format_object_name(target_object)), true);

    return CommandResult::Success;
}

// =============================================================================
// Object Interaction Commands
// =============================================================================

Result<CommandResult> cmd_open(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("open <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find container object - check inventory first, then room
    std::shared_ptr<Object> container = nullptr;

    // Check actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
            container = obj;
            break;
        }
    }

    // If not found in inventory, check room
    if (!container) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
                container = obj;
                break;
            }
        }
    }

    if (!container) {
        ctx.send_error(fmt::format("You don't see a '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if it's a container
    if (!container->is_container()) {
        ctx.send_error(fmt::format("You can't open {}.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Get container info
    const auto &container_info = container->container_info();

    // Check if it can be opened
    if (!container_info.closeable) {
        ctx.send_error(fmt::format("{} is not something that can be opened.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check if already open
    if (!container_info.closed) {
        ctx.send_error(fmt::format("{} is already open.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check if locked
    if (container_info.lockable && container_info.locked) {
        ctx.send_error(fmt::format("{} is locked.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Open it - modify the container state
    auto &mutable_info = const_cast<ContainerInfo &>(container_info);
    mutable_info.closed = false;

    ctx.send_success(fmt::format("You open {}.", ctx.format_object_name(container)));
    ctx.send_to_room(fmt::format("{} opens {}.", ctx.actor->name(), ctx.format_object_name(container)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_close(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("close <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find container object - check inventory first, then room
    std::shared_ptr<Object> container = nullptr;

    // Check actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
            container = obj;
            break;
        }
    }

    // If not found in inventory, check room
    if (!container) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
                container = obj;
                break;
            }
        }
    }

    if (!container) {
        ctx.send_error(fmt::format("You don't see a '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if it's a container
    if (!container->is_container()) {
        ctx.send_error(fmt::format("You can't close {}.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Get container info
    const auto &container_info = container->container_info();

    // Check if it can be closed
    if (!container_info.closeable) {
        ctx.send_error(fmt::format("{} is not something that can be closed.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check if already closed
    if (container_info.closed) {
        ctx.send_error(fmt::format("{} is already closed.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Close it - modify the container state
    auto &mutable_info = const_cast<ContainerInfo &>(container_info);
    mutable_info.closed = true;

    ctx.send_success(fmt::format("You close {}. It is now closed.", ctx.format_object_name(container)));
    ctx.send_to_room(fmt::format("{} closes {}.", ctx.actor->name(), ctx.format_object_name(container)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_lock(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("lock <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find container object - check inventory first, then room
    std::shared_ptr<Object> container = nullptr;

    // Check actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
            container = obj;
            break;
        }
    }

    // If not found in inventory, check room
    if (!container) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
                container = obj;
                break;
            }
        }
    }

    if (!container) {
        ctx.send_error(fmt::format("You don't see a '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if it's a container
    if (!container->is_container()) {
        ctx.send_error(fmt::format("You can't lock {}.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Get container info
    const auto &container_info = container->container_info();

    // Check if it can be locked
    if (!container_info.lockable) {
        ctx.send_error(fmt::format("{} doesn't have a lock.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check if already locked
    if (container_info.locked) {
        ctx.send_error(fmt::format("{} is already locked.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check if closed (must be closed to lock)
    if (!container_info.closed) {
        ctx.send_error(fmt::format("You must close {} first.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check for key requirement if the container has a key_id
    if (container_info.key_id.is_valid()) {
        // Look for the key in the actor's inventory
        bool has_key = false;
        auto inventory_items = ctx.actor->inventory().get_all_items();
        for (const auto &item : inventory_items) {
            if (item && item->id() == container_info.key_id && item->type() == ObjectType::Key) {
                has_key = true;
                break;
            }
        }

        if (!has_key) {
            ctx.send_error(fmt::format("You don't have the key for {}.", ctx.format_object_name(container)));
            return CommandResult::InvalidState;
        }
    }

    // Lock it - modify the container state
    auto &mutable_info = const_cast<ContainerInfo &>(container_info);
    mutable_info.locked = true;

    ctx.send_success(fmt::format("You lock {}.", ctx.format_object_name(container)));
    ctx.send_to_room(fmt::format("{} locks {}.", ctx.actor->name(), ctx.format_object_name(container)), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_unlock(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("unlock <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find container object - check inventory first, then room
    std::shared_ptr<Object> container = nullptr;

    // Check actor's inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
            container = obj;
            break;
        }
    }

    // If not found in inventory, check room
    if (!container) {
        auto &room_objects = ctx.room->contents_mutable().objects;
        for (auto &obj : room_objects) {
            if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_container()) {
                container = obj;
                break;
            }
        }
    }

    if (!container) {
        ctx.send_error(fmt::format("You don't see a '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if it's a container
    if (!container->is_container()) {
        ctx.send_error(fmt::format("You can't unlock {}.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Get container info
    const auto &container_info = container->container_info();

    // Check if it can be locked/unlocked
    if (!container_info.lockable) {
        ctx.send_error(fmt::format("{} doesn't have a lock.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check if already unlocked
    if (!container_info.locked) {
        ctx.send_error(fmt::format("{} is already unlocked.", ctx.format_object_name(container)));
        return CommandResult::InvalidState;
    }

    // Check for key requirement if the container has a key_id
    if (container_info.key_id.is_valid()) {
        // Look for the key in the actor's inventory
        bool has_key = false;
        auto inventory_items = ctx.actor->inventory().get_all_items();
        for (const auto &item : inventory_items) {
            if (item && item->id() == container_info.key_id && item->type() == ObjectType::Key) {
                has_key = true;
                break;
            }
        }

        if (!has_key) {
            ctx.send_error(fmt::format("You don't have the key for {}.", ctx.format_object_name(container)));
            return CommandResult::InvalidState;
        }
    }

    // Unlock it - modify the container state
    auto &mutable_info = const_cast<ContainerInfo &>(container_info);
    mutable_info.locked = false;

    ctx.send_success(fmt::format("You unlock {}.", ctx.format_object_name(container)));
    ctx.send_to_room(fmt::format("{} unlocks {}.", ctx.actor->name(), ctx.format_object_name(container)), true);

    return CommandResult::Success;
}

// =============================================================================
// System Commands
// =============================================================================

Result<CommandResult> cmd_quit(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    ctx.send_line("Thank you for playing FieryMUD!");
    ctx.send_to_room(fmt::format("{} has left the game.", ctx.actor->name()), true);

    // Disconnect the player if it's a Player with an output interface
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        if (auto output = player->get_output()) {
            output->disconnect("Player quit");
        }
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_save(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // TODO: Implement actual save logic
    ctx.send_success("Your character has been saved.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_help(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        // General help
        ctx.send_line("FieryMUD Help System\n");
        ctx.send("Available help topics:");
        ctx.send("  commands    - List available commands");
        ctx.send("  newbie      - Information for new players");
        ctx.send("  combat      - Combat system help");
        ctx.send("  socials     - Social commands");
        ctx.send("\nUse 'help <topic>' for specific help.");
        return CommandResult::Success;
    }

    std::string topic{ctx.arg(0)};

    if (topic == "commands") {
        return cmd_commands(ctx);
    } else if (topic == "newbie") {
        ctx.send_line("New Player Guide\n");
        ctx.send("Welcome to FieryMUD! Here are some basic commands to get started:");
        ctx.send("  look        - Examine your surroundings");
        ctx.send("  inventory   - See what you're carrying");
        ctx.send("  who         - See who else is online");
        ctx.send("  say <msg>   - Talk to people in the same room");
        ctx.send("  north/south/east/west - Move around");
        ctx.send("  quit        - Leave the game safely");
    } else {
        ctx.send_error(fmt::format("No help available for '{}'.", topic));
        return CommandResult::InvalidTarget;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_commands(const CommandContext &ctx) {
    auto available = Commands().get_available_commands(ctx.actor);

    ctx.send_line("Available commands:");

    // Group by category
    std::unordered_map<std::string, std::vector<std::string>> by_category;
    for (const auto &cmd : available) {
        auto cmd_info = Commands().find_command(cmd);
        if (cmd_info) {
            by_category[cmd_info->category].push_back(cmd);
        }
    }

    for (const auto &[category, commands] : by_category) {
        ctx.send(fmt::format("\n{}:", category));
        std::string cmd_list;
        for (size_t i = 0; i < commands.size(); ++i) {
            if (i > 0)
                cmd_list += ", ";
            cmd_list += commands[i];
        }
        ctx.send(fmt::format("  {}", cmd_list));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_prompt(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Show the player what their prompt looks like
    const auto& stats = ctx.actor->stats();
    std::string prompt = fmt::format("{}H {}M", stats.hit_points, stats.movement);
    
    if (ctx.actor->is_fighting()) {
        prompt += " (Fighting)";
    }
    prompt += ">";

    ctx.send("Your current prompt format:");
    ctx.send(fmt::format("  {}", prompt));
    ctx.send("");
    ctx.send("Legend:");
    ctx.send("  H = Hit Points");
    ctx.send("  M = Movement");
    ctx.send("  (Fighting) = You are in combat");
    ctx.send("  (Fighting: <enemy> is <condition>) = Enemy health status");

    return CommandResult::Success;
}

// =============================================================================
// Social Commands
// =============================================================================

Result<CommandResult> cmd_smile(const CommandContext &ctx) {
    std::string message;
    if (ctx.arg_count() == 0) {
        message = fmt::format("{} smiles.", ctx.actor->name());
        ctx.send("You smile.");
    } else {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }

        message = fmt::format("{} smiles at {}.", ctx.actor->name(), target->name());
        ctx.send(fmt::format("You smile at {}.", target->name()));
        ctx.send_to_actor(target, fmt::format("{} smiles at you.", ctx.actor->name()));
    }

    ctx.send_to_room(message, true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_nod(const CommandContext &ctx) {
    std::string message = fmt::format("{} nods.", ctx.actor->name());
    ctx.send("You nod.");
    ctx.send_to_room(message, true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_wave(const CommandContext &ctx) {
    std::string message;
    if (ctx.arg_count() == 0) {
        message = fmt::format("{} waves.", ctx.actor->name());
        ctx.send("You wave.");
    } else {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }

        message = fmt::format("{} waves at {}.", ctx.actor->name(), target->name());
        ctx.send(fmt::format("You wave at {}.", target->name()));
        ctx.send_to_actor(target, fmt::format("{} waves at you.", ctx.actor->name()));
    }

    ctx.send_to_room(message, true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_bow(const CommandContext &ctx) {
    std::string message = fmt::format("{} bows gracefully.", ctx.actor->name());
    ctx.send("You bow gracefully.");
    ctx.send_to_room(message, true);
    return CommandResult::Success;
}

Result<CommandResult> cmd_laugh(const CommandContext &ctx) {
    std::string message = fmt::format("{} laughs.", ctx.actor->name());
    ctx.send("You laugh.");
    ctx.send_to_room(message, true);
    return CommandResult::Success;
}

// =============================================================================
// Administrative Commands
// =============================================================================

Result<CommandResult> cmd_shutdown(const CommandContext &ctx) {
    // TODO: Implement permission checking
    // if (!ctx.has_permission(Permissions::SHUTDOWN)) {
    //     ctx.send_error("You don't have permission to shutdown the MUD.");
    //     return CommandResult::InsufficientPrivs;
    // }

    ctx.send_to_all("SYSTEM: MUD is shutting down in 30 seconds!");
    Log::warn("Shutdown initiated by {}", ctx.actor->name());

    // TODO: Implement actual shutdown sequence

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

    // TODO: Parse entity ID from string argument
    // auto room_id = ctx.parse_entity_id_arg(1);
    // For now, create a dummy room ID
    auto room_id = EntityId{1000};
    if (!room_id.is_valid()) {
        ctx.send_error("Invalid room ID.");
        return CommandResult::InvalidSyntax;
    }

    // TODO: Implement actor movement
    // auto result = WorldManager::instance().move_actor_to_room(target, room_id);
    // For now, just simulate success
    auto result = Success();
    if (!result) {
        ctx.send_error("Failed to teleport player.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("Teleported {} to room {}.", target->name(), room_id.value()));

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

    // TODO: Implement actor movement
    // auto result = WorldManager::instance().move_actor_to_room(target, ctx.room->id());
    // For now, just simulate success
    auto result = Success();
    if (!result) {
        ctx.send_error("Failed to summon player.");
        return CommandResult::ResourceError;
    }

    ctx.send_success(fmt::format("You summon {}.", target->name()));
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
        ctx.send("&C--- Available Weather Types ---&n");
        ctx.send("Clear, Partly_Cloudy, Cloudy, Light_Rain, Heavy_Rain,");
        ctx.send("Thunderstorm, Light_Snow, Heavy_Snow, Fog, Windy, Hot, Cold, Magical_Storm");
        ctx.send("\n&YAvailable Intensities:&n");
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
            // TODO: Parse zone ID from argument
            ctx.send_error("Zone-specific reports not yet implemented. Showing current zone report.");
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

// Zone Development Commands
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
// Combat Commands
// =============================================================================

Result<CommandResult> cmd_kill(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Kill who?");
        return CommandResult::InvalidTarget;
    }

    // Find target in current room
    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Can't kill yourself
    if (target == ctx.actor) {
        ctx.send_error("You can't kill yourself!");
        return CommandResult::InvalidState;
    }

    // Check if target is already fighting this actor or someone else
    if (target->position() == Position::Fighting) {
        ctx.send(fmt::format("{} is already fighting!", target->name()));
        return CommandResult::InvalidState;
    }

    // Check if actor is already fighting
    if (ctx.actor->position() == Position::Fighting) {
        ctx.send("You are already fighting!");
        return CommandResult::InvalidState;
    }

    // Check if actor is dead (including ghost form)
    if (!ctx.actor->is_alive()) {
        if (ctx.actor->position() == Position::Ghost) {
            ctx.send("You are a ghost and cannot engage in combat! Use 'release' to return to the living.");
        } else {
            ctx.send("You can't attack while you're dead!");
        }
        return CommandResult::InvalidState;
    }

    // Start combat
    ctx.actor->set_position(Position::Fighting);
    target->set_position(Position::Fighting);

    // Add to combat manager for ongoing combat rounds
    FieryMUD::CombatManager::start_combat(ctx.actor, target);

    ctx.send(fmt::format("You attack {}!", target->name()));
    ctx.send_to_room(fmt::format("{} attacks {}!", ctx.actor->name(), target->name()), true);
    ctx.send_to_actor(target, fmt::format("{} attacks you!", ctx.actor->name()));

    // Perform initial attack
    return CombatHelpers::perform_attack(ctx, target);
}

Result<CommandResult> cmd_hit(const CommandContext &ctx) {
    // Hit is the same as kill for now
    return cmd_kill(ctx);
}

Result<CommandResult> cmd_cast(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Cast what spell?");
        return CommandResult::InvalidTarget;
    }

    // TODO: Implement spell system
    ctx.send_error("Spell casting is not yet implemented.");
    return CommandResult::SystemError;
}

Result<CommandResult> cmd_flee(const CommandContext &ctx) {
    // Check if fighting
    if (ctx.actor->position() != Position::Fighting) {
        ctx.send_error("You are not fighting anyone!");
        return CommandResult::InvalidState;
    }

    // Get current room and find available exits
    auto room = ctx.actor->current_room();
    if (!room) {
        ctx.send_error("You are nowhere to flee to!");
        return CommandResult::InvalidState;
    }

    auto exits = room->get_visible_exits();
    if (exits.empty()) {
        ctx.send_error("There are no exits! You are trapped!");
        return CommandResult::InvalidState;
    }

    // Pick a random exit to flee to
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> exit_choice(0, exits.size() - 1);
    Direction flee_direction = exits[exit_choice(gen)];

    // End combat through combat manager
    FieryMUD::CombatManager::end_combat(ctx.actor);

    ctx.send("You flee from combat!");
    ctx.send_to_room(fmt::format("{} flees from combat!", ctx.actor->name()), true);

    // Move in the chosen direction
    auto result = ctx.move_actor_direction(flee_direction);
    if (!result) {
        ctx.send_error(fmt::format("You failed to flee: {}", result.error().message));
        return CommandResult::ResourceError;
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_release(const CommandContext &ctx) {
    // Only ghosts can use the release command
    if (ctx.actor->position() != Position::Ghost) {
        ctx.send_error("You are not dead! The 'release' command is only for ghosts.");
        return CommandResult::InvalidState;
    }

    // Create a corpse with the player's items in their current location
    auto current_room = ctx.actor->current_room();
    if (current_room) {
        create_player_corpse(ctx.actor, current_room);
        ctx.send_to_room(fmt::format("The ghost of {} fades away, leaving behind a corpse.", ctx.actor->name()), true);
    }

    // Move player to their personal starting room
    auto world_manager = &WorldManager::instance();
    
    // Try to get player's personal start room first
    EntityId start_room_id = INVALID_ENTITY_ID;
    if (auto player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        start_room_id = player->start_room();
    }
    
    // If player doesn't have a personal start room, use world default
    if (!start_room_id.is_valid()) {
        start_room_id = world_manager->get_start_room();
        Log::debug("Player {} has no personal start room, using world default: {}", ctx.actor->name(), start_room_id);
    }
    
    auto start_room = world_manager->get_room(start_room_id);

    if (!start_room) {
        ctx.send_error("Error: Could not find starting room! Contact an administrator.");
        Log::error("Failed to find starting room for {}: {}", ctx.actor->name(), start_room_id);
        return CommandResult::SystemError;
    }

    // Move the player to starting room and restore to living
    auto move_result = ctx.actor->move_to(start_room);
    if (!move_result) {
        ctx.send_error("Error: Could not move to starting room! Contact an administrator.");
        Log::error("Failed to move {} to starting room {}: {}", ctx.actor->name(), start_room_id,
                   move_result.error().message);
        return CommandResult::SystemError;
    }
    ctx.actor->set_position(Position::Standing);

    // Reset player's HP to full (they get a new body)
    auto &stats = ctx.actor->stats();
    stats.hit_points = stats.max_hit_points;

    ctx.send("You feel yourself drawn back to the mortal realm in a new body...");
    ctx.send("You have been returned to life!");
    ctx.send_to_room(fmt::format("{} materializes out of thin air!", ctx.actor->name()), true);

    return CommandResult::Success;
}

// =============================================================================
// Death Helper Functions
// =============================================================================

void create_player_corpse(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room) {
    // Create a unique ID for the corpse (using player ID + offset for corpses)
    EntityId corpse_id{actor->id().value() + 100000}; // Add offset to avoid conflicts

    // Create corpse object
    std::string corpse_name = fmt::format("the corpse of {}", actor->name());
    auto corpse_result = Object::create(corpse_id, corpse_name, ObjectType::Corpse);
    if (!corpse_result) {
        Log::error("Failed to create corpse for {}: {}", actor->name(), corpse_result.error().message);
        return;
    }

    auto corpse = std::shared_ptr<Object>(corpse_result.value().release());

    // Set corpse description
    corpse->set_description(fmt::format("The lifeless body of {} lies here, still and cold.", actor->name()));

    // Transfer all equipment to the corpse
    auto equipped_items = actor->equipment().get_all_equipped();
    for (auto &item : equipped_items) {
        if (item) {
            // Remove from player equipment and add to corpse
            actor->equipment().unequip_item(item->id());
            // TODO: Add items to corpse container (need Container class)
            Log::info("Transferred equipped item '{}' to {}'s corpse", item->name(), actor->name());
        }
    }

    // Transfer all inventory to the corpse
    auto inventory_items = actor->inventory().get_all_items();
    for (auto &item : inventory_items) {
        if (item) {
            // Remove from player inventory and add to corpse
            actor->inventory().remove_item(item->id());
            // TODO: Add items to corpse container (need Container class)
            Log::info("Transferred inventory item '{}' to {}'s corpse", item->name(), actor->name());
        }
    }

    // Add corpse to the room
    room->add_object(corpse);

    Log::info("Created corpse for {} in room {}", actor->name(), room->id().value());
}

// =============================================================================
// Combat Helper Functions
// =============================================================================

namespace CombatHelpers {

Result<CommandResult> perform_attack(const CommandContext &ctx, std::shared_ptr<Actor> target) {
    if (!ctx.actor || !target) {
        return std::unexpected(Errors::InvalidArgument("actor or target", "cannot be null"));
    }

    // Use the modern combat system
    FieryMUD::CombatResult result = FieryMUD::CombatSystem::perform_attack(ctx.actor, target);

    // Send messages from the combat result
    if (!result.attacker_message.empty()) {
        ctx.send(result.attacker_message);
    }

    if (!result.target_message.empty()) {
        ctx.send_to_actor(target, result.target_message);
    }

    if (!result.room_message.empty()) {
        ctx.send_to_room(result.room_message, true);
    }

    return CommandResult::Success;
}

bool is_valid_target(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target) {
    if (!attacker || !target) {
        return false;
    }

    // Can't attack yourself
    if (attacker == target) {
        return false;
    }

    // Can't attack dead actors
    if (target->position() == Position::Dead) {
        return false;
    }

    // Must be in same room
    if (attacker->current_room() != target->current_room()) {
        return false;
    }

    return true;
}

} // namespace CombatHelpers

// =============================================================================
// Helper Functions
// =============================================================================

namespace Helpers {

std::string format_room_description(std::shared_ptr<Room> room, std::shared_ptr<Actor> viewer) {
    if (!room) {
        return "You are in the void.";
    }

    std::ostringstream desc;
    desc << fmt::format("{}\n", room->name());
    desc << fmt::format("{}\n", room->description());

    // Add exits
    std::string exits = format_exits(room);
    if (!exits.empty()) {
        desc << fmt::format("\n{}\n", exits);
    }

    // Add objects in room
    auto objects = room->contents().objects;
    if (!objects.empty()) {
        desc << "\nYou see:\n";
        for (const auto &obj : objects) {
            if (obj) {
                desc << fmt::format("  {}\n", obj->short_description());
            }
        }
    }

    // Add other actors
    auto actors = room->contents().actors;
    bool found_others = false;
    for (const auto &actor : actors) {
        if (actor && actor != viewer) {
            if (!found_others) {
                desc << "\nAlso here:\n";
                found_others = true;
            }
            desc << fmt::format("  {}\n", actor->short_description());
        }
    }

    return desc.str();
}

std::string format_object_description(std::shared_ptr<Object> obj, std::shared_ptr<Actor> viewer) {
    if (!obj) {
        return "You see nothing special.";
    }

    std::ostringstream desc;
    desc << fmt::format("{}\n", obj->short_description());
    desc << fmt::format("{}\n", obj->description());

    // Add object-specific details based on type
    if (obj->is_container()) {
        const auto &container_info = obj->container_info();
        if (container_info.closeable) {
            desc << fmt::format("It appears to be {}.\n", container_info.closed ? "closed" : "open");
            if (container_info.lockable) {
                desc << fmt::format("It is {}.\n", container_info.locked ? "locked" : "unlocked");
            }
        }
        // Show capacity if not full/empty and container is open
        if (!container_info.closed) {
            if (container_info.capacity > 0) {
                desc << fmt::format("It can hold {} items.\n", container_info.capacity);
            }
        }
    }

    if (obj->is_weapon()) {
        const auto &damage = obj->damage_profile();
        desc << fmt::format("Weapon damage: {}\n", damage.to_dice_string());
    }

    if (obj->is_armor()) {
        desc << fmt::format("Armor class: {}\n", obj->armor_class());
    }

    // Show condition and quality
    if (obj->condition() < 100) {
        desc << fmt::format("Condition: {} ({})\n", obj->condition(), obj->quality_description());
    }

    // Show weight and value for examination
    desc << fmt::format("It weighs {} pounds", obj->weight());
    if (obj->value() > 0) {
        desc << fmt::format(" and is worth {} gold coins", obj->value());
    }
    desc << ".\n";

    // Show extra descriptions available (if any exist)
    const auto &extras = obj->get_all_extra_descriptions();
    if (!extras.empty()) {
        desc << "\nYou notice some details you could examine more closely: ";
        bool first = true;
        for (const auto &extra : extras) {
            if (!first)
                desc << ", ";
            // Format the keywords vector as a string
            bool first_keyword = true;
            for (const auto &keyword : extra.keywords) {
                if (!first_keyword)
                    desc << " ";
                desc << keyword;
                first_keyword = false;
            }
            first = false;
        }
        desc << "\n";
    }

    return desc.str();
}

std::string format_actor_description(std::shared_ptr<Actor> target, std::shared_ptr<Actor> viewer) {
    if (!target) {
        return "You see nothing special.";
    }

    std::ostringstream desc;
    desc << fmt::format("{}\n", target->short_description());
    desc << fmt::format("{} is here.\n", target->short_description());

    // TODO: Add equipment, health status, etc.

    return desc.str();
}

std::string format_inventory(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return "You have no inventory.";
    }

    auto inventory = actor->inventory().get_all_items();
    if (inventory.empty()) {
        return "You are not carrying anything.";
    }

    std::ostringstream inv;
    inv << "You are carrying:\n";
    for (const auto &item : inventory) {
        if (item) {
            inv << fmt::format("  {}\n", item->short_description());
        }
    }

    return inv.str();
}

std::string format_equipment(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return "You have no equipment.";
    }

    auto equipment = actor->equipment().get_all_equipped();
    if (equipment.empty()) {
        return "You are not wearing anything.";
    }

    std::ostringstream eq;
    eq << "You are wearing:\n";
    for (const auto &item : equipment) {
        if (item) {
            eq << fmt::format("  {}\n", item->name());
        }
    }

    return eq.str();
}

std::string format_who_list(const std::vector<std::shared_ptr<Actor>> &actors) {
    std::ostringstream who;
    who << fmt::format("Players currently online ({}):\n", actors.size());

    for (const auto &actor : actors) {
        if (actor) {
            who << fmt::format("  {} (Level {})\n", actor->name(), actor->stats().level);
        }
    }

    return who.str();
}

std::string format_exits(std::shared_ptr<Room> room) {
    if (!room) {
        return "";
    }

    auto exits = room->get_visible_exits();
    if (exits.empty()) {
        return "There are no obvious exits.";
    }

    std::ostringstream exit_str;
    exit_str << "Obvious exits: ";

    bool first = true;
    for (auto dir : exits) {
        if (!first)
            exit_str << ", ";
        exit_str << RoomUtils::get_direction_name(dir);
        first = false;
    }

    return exit_str.str();
}

bool can_move_direction(std::shared_ptr<Actor> actor, Direction dir, std::string &failure_reason) {
    if (!actor) {
        failure_reason = "Invalid actor";
        return false;
    }

    auto room = actor->current_room();
    if (!room) {
        failure_reason = "You are not in a room";
        return false;
    }

    if (!room->has_exit(dir)) {
        failure_reason = "There's no exit in that direction";
        return false;
    }

    // Check if actor is in combat
    if (actor->position() == Position::Fighting) {
        failure_reason = "You can't leave while fighting! Use 'flee' to escape combat";
        return false;
    }

    // Check if actor is dead (but allow ghosts limited movement)
    if (actor->position() == Position::Dead) {
        failure_reason = "You can't move while you're dead!";
        return false;
    }
    // Ghosts can move but with restrictions (this will be handled elsewhere)

    // TODO: Add more movement checks (doors, etc.)

    return true;
}

Result<CommandResult> execute_movement(const CommandContext &ctx, Direction dir) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::string failure_reason;
    if (!can_move_direction(ctx.actor, dir, failure_reason)) {
        ctx.send_error(fmt::format("Cannot move {}: {}", magic_enum::enum_name(dir), failure_reason));
        return CommandResult::InvalidState;
    }

    auto result = ctx.move_actor_direction(dir);
    if (!result) {
        ctx.send_error(fmt::format("Movement failed: {}", result.error().message));
        return CommandResult::ResourceError;
    }

    // Send movement confirmation message
    std::string dir_name{magic_enum::enum_name(dir)};
    std::transform(dir_name.begin(), dir_name.end(), dir_name.begin(), ::tolower);
    ctx.send(fmt::format("You move {}.", dir_name));
    
    // Automatically show the new room after movement
    auto look_result = cmd_look(ctx);
    if (!look_result) {
        ctx.send_error("Movement succeeded but could not display new room");
    }

    return CommandResult::Success;
}

void send_communication(const CommandContext &ctx, std::string_view message, MessageType type,
                        std::string_view channel_name) {
    std::string formatted_message = format_communication(ctx.actor, message, channel_name);

    switch (type) {
    case MessageType::Say:
        ctx.send(fmt::format("You say, '{}'", message));
        ctx.send_to_room(formatted_message, true);
        break;
    case MessageType::Broadcast:
        ctx.send(fmt::format("You {}, '{}'", channel_name, message));
        ctx.send_to_all(formatted_message);
        break;
    case MessageType::Channel:
        ctx.send(fmt::format("You {}, '{}'", channel_name, message));
        ctx.send_to_all(formatted_message);
        break;
    default:
        ctx.send_to_room(formatted_message, false);
        break;
    }
}

std::string format_communication(std::shared_ptr<Actor> sender, std::string_view message, std::string_view channel) {
    if (!sender) {
        return std::string{message};
    }

    if (channel.empty()) {
        return fmt::format("{} says, '{}'", sender->name(), message);
    } else {
        return fmt::format("{} {}s, '{}'", sender->name(), channel, message);
    }
}

} // namespace Helpers

/**
 * Light Command: Light torches, lanterns, and other light sources
 */
Result<CommandResult> cmd_light(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("light <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find light source in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> light_source = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0)) && obj->is_light_source()) {
            light_source = obj;
            break;
        }
    }

    if (!light_source) {
        ctx.send_error(fmt::format("You don't have a light source called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if already lit
    const auto &light_info = light_source->light_info();
    if (light_info.lit) {
        ctx.send_error(fmt::format("The {} is already lit.", ctx.format_object_name(light_source)));
        return CommandResult::InvalidState;
    }

    // Check if light source has duration remaining
    if (light_info.duration <= 0) {
        ctx.send_error(fmt::format("The {} is burnt out and cannot be lit.", ctx.format_object_name(light_source)));
        return CommandResult::InvalidState;
    }

    // Light the object
    auto new_light_info = light_info;
    new_light_info.lit = true;
    light_source->set_light_info(new_light_info);

    ctx.send_success(fmt::format("You light the {}. It glows brightly.", ctx.format_object_name(light_source)));
    ctx.send_to_room(fmt::format("{} lights {}.", ctx.actor->name(), ctx.format_object_name(light_source)), true);

    return CommandResult::Success;
}

/**
 * Eat Command: Consume food items
 */
Result<CommandResult> cmd_eat(const CommandContext &ctx) {
    if (ctx.arg_count() < 1) {
        ctx.send_usage("eat <food>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find food item in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> food_item = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(ctx.arg(0))) {
            if (obj->type() == ObjectType::Food || obj->type() == ObjectType::Potion) {
                food_item = obj;
                break;
            }
        }
    }

    if (!food_item) {
        ctx.send_error(fmt::format("You don't have any food called '{}'.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    // Check if food is spoiled (timer expired)
    if (food_item->is_expired()) {
        ctx.send_error(fmt::format("The {} is spoiled and inedible.", ctx.format_object_name(food_item)));
        return CommandResult::InvalidState;
    }

    // Consume the food
    if (!ctx.actor->inventory().remove_item(food_item)) {
        ctx.send_error("Failed to consume the food item.");
        return CommandResult::ResourceError;
    }

    if (food_item->type() == ObjectType::Food) {
        ctx.send_success(fmt::format("You eat the {}. It's delicious!", ctx.format_object_name(food_item)));
        ctx.send_to_room(fmt::format("{} eats {}.", ctx.actor->name(), ctx.format_object_name(food_item)), true);
    } else {
        ctx.send_success(fmt::format("You drink the {}. You feel refreshed.", ctx.format_object_name(food_item)));
        ctx.send_to_room(fmt::format("{} drinks {}.", ctx.actor->name(), ctx.format_object_name(food_item)), true);
    }

    return CommandResult::Success;
}

/**
 * Drink Command: Drink from liquid containers or potions
 */
Result<CommandResult> cmd_drink(const CommandContext &ctx) {
    std::string target_name;
    
    if (ctx.arg_count() >= 2 && ctx.arg(0) == "from") {
        // "drink from <container>" syntax
        target_name = ctx.arg(1);
    } else if (ctx.arg_count() >= 1) {
        // "drink <item>" syntax
        target_name = ctx.arg(0);
    } else {
        ctx.send_usage("drink <item> | drink from <container>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    // Find drinkable item in inventory
    auto inventory_items = ctx.actor->inventory().get_all_items();
    std::shared_ptr<Object> drink_item = nullptr;

    for (const auto &obj : inventory_items) {
        if (obj && obj->matches_keyword(target_name)) {
            if (obj->type() == ObjectType::Liquid_Container || 
                obj->type() == ObjectType::Potion || 
                obj->type() == ObjectType::Food) {
                drink_item = obj;
                break;
            }
        }
    }

    if (!drink_item) {
        ctx.send_error(fmt::format("You don't have anything to drink called '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    // Handle different drinkable types
    if (drink_item->type() == ObjectType::Potion) {
        // Potions are consumed completely
        if (!ctx.actor->inventory().remove_item(drink_item)) {
            ctx.send_error("Failed to drink the potion.");
            return CommandResult::ResourceError;
        }
        
        ctx.send_success(fmt::format("You drink the {}. You feel its effects course through your body.", 
                                   ctx.format_object_name(drink_item)));
        ctx.send_to_room(fmt::format("{} drinks {}.", ctx.actor->name(), ctx.format_object_name(drink_item)), true);
    } else if (drink_item->type() == ObjectType::Liquid_Container) {
        // Liquid containers can be drunk from multiple times
        ctx.send_success(fmt::format("You drink from the {}. The liquid is refreshing.", 
                                   ctx.format_object_name(drink_item)));
        ctx.send_to_room(fmt::format("{} drinks from {}.", ctx.actor->name(), ctx.format_object_name(drink_item)), true);
    } else {
        // Food items (like fruits with juice)
        ctx.send_success(fmt::format("You drink from the {}. It quenches your thirst.", 
                                   ctx.format_object_name(drink_item)));
        ctx.send_to_room(fmt::format("{} drinks from {}.", ctx.actor->name(), ctx.format_object_name(drink_item)), true);
    }

    return CommandResult::Success;
}

} // namespace BuiltinCommands
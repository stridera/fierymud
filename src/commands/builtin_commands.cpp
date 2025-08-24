/***************************************************************************
 *   File: src/commands/builtin_commands.cpp           Part of FieryMUD *
 *  Usage: Core built-in command implementations                          *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "builtin_commands.hpp"

#include "../core/actor.hpp"
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

    // Object Commands
    Commands().command("get", cmd_get).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("drop", cmd_drop).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("give", cmd_give).category("Object").privilege(PrivilegeLevel::Player).build();
    Commands().command("wear", cmd_wear).alias("equip").category("Object").privilege(PrivilegeLevel::Player).build();
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
    fmt::print("cmd_look: actor name: {}\n", ctx.actor->name());
    if (ctx.arg_count() == 0) {
        // Look at room - get current room from actor, not from context
        auto current_room = ctx.actor ? ctx.actor->current_room() : nullptr;
        if (!current_room) {
            ctx.send_error("You are not in a room.");
            return CommandResult::InvalidState;
        }

        std::string description = Helpers::format_room_description(current_room, ctx.actor);
        ctx.send(description);
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

    std::string description;
    switch (target_info.type) {
    case TargetType::Object:
        description = Helpers::format_object_description(target_info.object, ctx.actor);
        // Add detailed examination info
        description += "\n\nDetailed examination reveals additional information...";
        break;
    case TargetType::Actor:
        description = Helpers::format_actor_description(target_info.actor, ctx.actor);
        break;
    default:
        // Fall back to look
        return cmd_look(ctx);
    }

    ctx.send(description);
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
    if (auto result = ctx.require_args(1, "<object>"); !result) {
        ctx.send_usage("get <object>");
        return CommandResult::InvalidSyntax;
    }

    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Find object in the current room
    auto &room_objects = ctx.room->contents_mutable().objects;
    std::shared_ptr<Object> target_object = nullptr;

    for (auto &obj : room_objects) {
        if (obj && obj->name().find(ctx.arg(0)) != std::string::npos) {
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
        if (obj && obj->name().find(ctx.arg(0)) != std::string::npos) {
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
        if (obj && obj->name().find(ctx.arg(0)) != std::string::npos) {
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
        if (obj && obj->name().find(ctx.arg(0)) != std::string::npos) {
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

    // Start combat
    ctx.actor->set_position(Position::Fighting);
    target->set_position(Position::Fighting);

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

    // Stop fighting
    ctx.actor->set_position(Position::Standing);

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

// =============================================================================
// Combat Helper Functions
// =============================================================================

namespace CombatHelpers {

Result<CommandResult> perform_attack(const CommandContext &ctx, std::shared_ptr<Actor> target) {
    if (!ctx.actor || !target) {
        return std::unexpected(Errors::InvalidArgument("actor or target", "cannot be null"));
    }

    // Calculate to-hit roll
    int attacker_hit = ctx.actor->stats().hit_roll;
    int attacker_level = ctx.actor->stats().level;
    int target_ac = target->stats().armor_class;

    // Simple hit calculation (roll d20 + hit_roll vs armor class)
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<int> d20(1, 20);
    int roll = d20(gen); // d20 roll
    int total_hit = roll + attacker_hit + attacker_level;

    if (total_hit < target_ac) {
        // Miss
        ctx.send("Your attack misses.");
        ctx.send_to_room(fmt::format("{}'s attack misses {}.", ctx.actor->name(), target->name()), true);
        ctx.send_to_actor(target, fmt::format("{} misses you.", ctx.actor->name()));
        return CommandResult::Success;
    }

    // Hit - calculate damage
    std::uniform_int_distribution<int> d6(1, 6);
    int base_damage = d6(gen); // d6 base damage
    int damage_bonus = ctx.actor->stats().damage_roll;
    int str_modifier = Stats::attribute_modifier(ctx.actor->stats().strength);
    int total_damage = std::max(1, base_damage + damage_bonus + str_modifier);

    // Apply damage
    auto &target_stats = target->stats();
    target_stats.hit_points -= total_damage;

    ctx.send(fmt::format("You hit {} for {} damage.", target->name(), total_damage));
    ctx.send_to_room(fmt::format("{} hits {} for {} damage.", ctx.actor->name(), target->name(), total_damage), true);
    ctx.send_to_actor(target, fmt::format("{} hits you for {} damage.", ctx.actor->name(), total_damage));

    // Check if target died
    if (target_stats.hit_points <= 0) {
        target_stats.hit_points = 0;
        target->set_position(Position::Dead);
        ctx.actor->set_position(Position::Standing);

        // Calculate experience gain (simple)
        long exp_gain = target->stats().level * 100;
        ctx.actor->gain_experience(exp_gain);

        ctx.send(fmt::format("You have killed {}! You gain {} experience.", target->name(), exp_gain));
        ctx.send_to_room(fmt::format("{} is DEAD!", target->name()), true);
        ctx.send_to_actor(target, "You are DEAD!");

        return CommandResult::Success;
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

    // Add object-specific details
    // TODO: Add wear flags, weight, value, etc.

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

} // namespace BuiltinCommands
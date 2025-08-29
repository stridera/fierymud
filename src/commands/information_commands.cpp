/***************************************************************************
 *   File: src/commands/information_commands.cpp           Part of FieryMUD *
 *  Usage: Information and status command implementations                    *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "information_commands.hpp"
#include "builtin_commands.hpp"

#include "../core/actor.hpp"
#include "../core/object.hpp"
#include "../world/room.hpp"
#include "../world/weather.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace InformationCommands {

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
        std::string exits = BuiltinCommands::Helpers::format_exits(current_room);
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

    // Check for "look in <container>" command
    if (ctx.arg_count() >= 2 && ctx.arg(0) == "in") {
        // Look inside a container
        std::string container_name(ctx.arg(1));
        
        // Find the container
        auto target_info = ctx.resolve_target(container_name);
        if (!target_info.is_valid() || target_info.type != TargetType::Object) {
            ctx.send_error(fmt::format("You don't see '{}' here.", container_name));
            return CommandResult::InvalidTarget;
        }
        
        auto container = target_info.object;
        
        // Check if it's a container
        if (!container->is_container()) {
            ctx.send_error(fmt::format("The {} is not a container.", ctx.format_object_name(container)));
            return CommandResult::InvalidTarget;
        }
        
        // Try to cast to Container to access contents
        auto container_obj = std::dynamic_pointer_cast<Container>(container);
        if (!container_obj) {
            ctx.send_error(fmt::format("You can't look inside {}.", ctx.format_object_name(container)));
            return CommandResult::InvalidState;
        }
        
        // Check container properties
        const auto& container_info = container_obj->container_info();
        
        // Check if container is closed
        if (container_info.closeable && container_info.closed) {
            ctx.send_error(fmt::format("The {} is closed.", ctx.format_object_name(container)));
            return CommandResult::InvalidState;
        }
        
        // Get container contents
        auto contents = container_obj->get_contents();
        
        if (contents.empty()) {
            ctx.send(fmt::format("The {} is empty.", ctx.format_object_name(container)));
        } else {
            ctx.send(fmt::format("Looking inside {}, you see:", ctx.format_object_name(container)));
            for (const auto& item : contents) {
                if (item) {
                    ctx.send(fmt::format("  {}", item->short_description()));
                }
            }
        }
        
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
        description = BuiltinCommands::Helpers::format_actor_description(target_info.actor, ctx.actor);
        break;
    case TargetType::Object:
        description = BuiltinCommands::Helpers::format_object_description(target_info.object, ctx.actor);

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
        description = BuiltinCommands::Helpers::format_room_description(target_info.room, ctx.actor);
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
        detailed_desc << BuiltinCommands::Helpers::format_actor_description(target_info.actor, ctx.actor);
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

    std::string who_list = BuiltinCommands::Helpers::format_who_list(online_actors);
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

    std::string inventory = BuiltinCommands::Helpers::format_inventory(ctx.actor);
    ctx.send(inventory);

    return CommandResult::Success;
}

Result<CommandResult> cmd_equipment(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    std::string equipment = BuiltinCommands::Helpers::format_equipment(ctx.actor);
    ctx.send(equipment);

    return CommandResult::Success;
}

Result<CommandResult> cmd_score(const CommandContext &ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }

    const auto& stats = ctx.actor->stats();
    std::ostringstream score;

    // Character header - centered name
    std::string name{ctx.actor->name()};
    int padding = std::max(0, (45 - static_cast<int>(name.length())) / 2);
    score << fmt::format("{:{}}{}\n\n", "", padding, fmt::format("Character attributes for {}", name));

    // Basic character info  
    score << fmt::format("Level: {}  Class: {}  Race: Human  Size: Medium  Gender: Neutral\n", 
                        stats.level, "Warrior");

    // Age, height, weight (placeholder values)
    score << fmt::format("Age: {} years, {} months  Height: 5'10\"  Weight: 180 lbs\n", 
                        20 + stats.level, (stats.level * 3) % 12);

    // Abilities 
    score << fmt::format("Str: {}    Int: {}     Wis: {}\n", 
                        stats.strength, stats.intelligence, stats.wisdom);
    score << fmt::format("Dex: {}    Con: {}     Cha: {}\n", 
                        stats.dexterity, stats.constitution, stats.charisma);

    // Hit points, mana, movement
    score << fmt::format("Hit points: {}/{}   Mana: {}/{}   Moves: {}/{}\n",
                        stats.hit_points, stats.max_hit_points,
                        stats.mana, stats.max_mana,
                        stats.movement, stats.max_movement);

    // Armor class and combat stats
    score << fmt::format("Armor Class: {}   Hit roll: {}   Damage roll: {}\n",
                        stats.armor_class, stats.hit_roll, stats.damage_roll);

    // Alignment
    std::string align_desc;
    if (stats.alignment < -350) align_desc = "Evil";
    else if (stats.alignment > 350) align_desc = "Good";
    else align_desc = "Neutral";
    score << fmt::format("Alignment: {} ({})  ", align_desc, stats.alignment);

    // Position/Status
    score << fmt::format("Status: {}\n", magic_enum::enum_name(ctx.actor->position()));

    // Encumbrance 
    int current_weight = ctx.actor->current_carry_weight();
    int max_weight = ctx.actor->max_carry_weight();
    score << fmt::format("Encumbrance: {}/{} lbs  ", current_weight, max_weight);

    // Experience and gold
    if (stats.level < 100) {  // Not immortal
        score << fmt::format("Experience: {}  ", stats.experience);
    }
    score << fmt::format("Gold: {}\n", stats.gold);

    // Current location
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

Result<CommandResult> cmd_stat(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send("Usage: stat <target>");
        return CommandResult::InvalidSyntax;
    }

    // Resolve the target
    auto target_info = ctx.resolve_target(ctx.arg(0));
    if (!target_info.is_valid()) {
        ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    std::string stat_info;
    switch (target_info.type) {
    case TargetType::Object:
        // Use the object's get_stat_info method which handles polymorphism correctly
        stat_info = target_info.object->get_stat_info();
        ctx.send(stat_info);
        return CommandResult::Success;
        
    case TargetType::Actor:
        // Use the actor's get_stat_info method which handles polymorphism correctly
        stat_info = target_info.actor->get_stat_info();
        ctx.send(stat_info);
        return CommandResult::Success;
        
    case TargetType::Room:
        // Use the room's get_stat_info method which provides comprehensive room statistics
        stat_info = target_info.room->get_stat_info();
        ctx.send(stat_info);
        return CommandResult::Success;
        
    default:
        ctx.send_error("You can't get statistics for that.");
        return CommandResult::InvalidTarget;
    }
}

} // namespace InformationCommands
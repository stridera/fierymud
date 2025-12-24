#include "information_commands.hpp"

#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "../database/game_data_cache.hpp"
#include "../server/world_server.hpp"
#include "../world/room.hpp"
#include "../world/weather.hpp"
#include "../world/world_manager.hpp"
#include "builtin_commands.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace InformationCommands {

// Helper function to get class display name with colors from cache
std::string get_class_display_name(std::string_view player_class) {
    if (player_class.empty()) return "Unknown";

    const auto* class_data = GameDataCache::instance().find_class_by_name(player_class);
    if (class_data) {
        return class_data->name;  // Returns name with color codes
    }

    // Fallback: simple title case
    std::string result;
    bool capitalize_next = true;
    for (char c : player_class) {
        if (c == '_') {
            result += '-';
            capitalize_next = true;
        } else if (capitalize_next) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize_next = false;
        } else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return result;
}

// Helper function to get race display name with colors from cache
std::string get_race_display_name(std::string_view race) {
    if (race.empty()) return "Human";

    const auto* race_data = GameDataCache::instance().find_race_by_name(race);
    if (race_data) {
        return race_data->name;  // Returns name with color codes
    }

    // Also try by key (for "HALF_ELF" style lookups)
    race_data = GameDataCache::instance().find_race_by_key(race);
    if (race_data) {
        return race_data->name;
    }

    // Fallback: simple title case
    std::string result;
    bool capitalize_next = true;
    for (char c : race) {
        if (c == '_') {
            result += '-';
            capitalize_next = true;
        } else if (capitalize_next) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize_next = false;
        } else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return result;
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

        // Cache visibility check to avoid redundant calculate_effective_light calls
        bool can_see = current_room->can_see_in_room(ctx.actor.get());

        // Use the room's own get_room_description method which properly handles lighting
        std::string description = current_room->get_room_description(ctx.actor.get());

        // Only add additional formatting (exits, objects, actors) if the room is visible
        if (!can_see) {
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
        bool found_objects = false;
        for (const auto &obj : objects) {
            if (obj) {
                auto ground_desc = obj->ground();
                if (ground_desc.empty()) {
                    ground_desc = obj->short_description();
                }
                if (ground_desc.empty()) {
                    continue;  // Skip objects with no description
                }
                if (!found_objects) {
                    full_desc << "\nYou see:\n";
                    found_objects = true;
                }
                full_desc << fmt::format("  {}\n", ground_desc);
            }
        }

        // Add other actors
        auto actors = current_room->contents().actors;
        bool found_others = false;
        for (const auto &actor : actors) {
            if (actor && actor != ctx.actor) {
                // Get description - prefer ground(), fall back to display_name()
                auto actor_desc = actor->ground();
                if (actor_desc.empty()) {
                    actor_desc = actor->display_name();
                }
                if (actor_desc.empty()) {
                    continue;  // Skip actors with no description
                }

                if (!found_others) {
                    full_desc << "\nAlso here:\n";
                    found_others = true;
                }

                // Check if the actor is a ghost
                bool is_ghost = (actor->position() == Position::Ghost);
                std::string ghost_prefix = is_ghost ? "<cyan>(ghost)</> " : "";

                full_desc << fmt::format("  {}{}\n", ghost_prefix, actor_desc);
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
        const auto &container_info = container_obj->container_info();

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
            for (const auto &item : contents) {
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
    case TargetType::Self:
        // Looking at yourself
        description = BuiltinCommands::Helpers::format_actor_description(ctx.actor, ctx.actor);
        break;
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
            std::vector<std::string> examinable_features;
            for (const auto &extra : extra_descs) {
                // Show the first keyword as the examinable feature name
                if (!extra.keywords.empty()) {
                    examinable_features.push_back(extra.keywords[0]);
                }
            }
            detailed_desc << fmt::format(
                "{}\n", CommandParserUtils::join(std::span<const std::string>{examinable_features}, ", "));
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
    // Get online actors from WorldServer
    std::vector<std::shared_ptr<Actor>> online_actors;

    if (auto *world_server = WorldServer::instance()) {
        online_actors = world_server->get_online_actors();
    }

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

        ctx.send_line(fmt::format("You are in: {} [{}]", ctx.room->name(), ctx.room->id()));
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
        ctx.send_line(fmt::format("{} is nowhere.", target->display_name()));
    } else {
        ctx.send_line(fmt::format("{} is in: {} [{}]", target->display_name(), target_room->display_name(),
                                  target_room->id()));
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

    const auto &stats = ctx.actor->stats();
    std::ostringstream score;

    // Character header - centered name
    std::string name{ctx.actor->name()};
    int padding = std::max(0, (45 - static_cast<int>(name.length())) / 2);
    score << fmt::format("{:{}}{}\n\n", "", padding, fmt::format("Character attributes for {}", name));

    // Get player-specific info if available
    std::string player_class = "Unknown";
    std::string race = "Human";

    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (player) {
        player_class = get_class_display_name(player->player_class());
        race = get_race_display_name(player->race());
    }

    // Get size and gender from the actor
    std::string_view size = ctx.actor->size();
    std::string gender_str = std::string(ctx.actor->gender());
    // Capitalize first letter
    if (!gender_str.empty()) {
        gender_str[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(gender_str[0])));
    }

    // Basic character info
    score << fmt::format("Level: {}  Class: {}  Race: {}  Size: {}  Gender: {}\n",
                         stats.level, player_class, race, size, gender_str);

    // Age, height, weight (placeholder values)
    score << fmt::format("Age: {} years, {} months  Height: 5'10\"  Weight: 180 lbs\n", 20 + stats.level,
                         (stats.level * 3) % 12);

    // Abilities
    score << fmt::format("Str: {}    Int: {}     Wis: {}\n", stats.strength, stats.intelligence, stats.wisdom);
    score << fmt::format("Dex: {}    Con: {}     Cha: {}\n", stats.dexterity, stats.constitution, stats.charisma);

    // Hit points and stamina
    score << fmt::format("Hit points: {}/{}   Stamina: {}/{}\n", stats.hit_points, stats.max_hit_points,
                         stats.movement, stats.max_movement);

    // Combat stats (new ACC/EVA system)
    score << fmt::format("Accuracy: {}   Evasion: {}   Attack Power: {}\n", stats.accuracy, stats.evasion,
                         stats.attack_power);
    score << fmt::format("Armor Rating: {}   Damage Reduction: {}%\n", stats.armor_rating, stats.damage_reduction_percent);

    // Alignment
    std::string align_desc;
    if (stats.alignment < -350)
        align_desc = "Evil";
    else if (stats.alignment > 350)
        align_desc = "Good";
    else
        align_desc = "Neutral";
    score << fmt::format("Alignment: {} ({})  ", align_desc, stats.alignment);

    // Position/Status
    score << fmt::format("Status: {}\n", magic_enum::enum_name(ctx.actor->position()));

    // Encumbrance
    int current_weight = ctx.actor->current_carry_weight();
    int max_weight = ctx.actor->max_carry_weight();
    score << fmt::format("Encumbrance: {}/{} lbs  ", current_weight, max_weight);

    // Experience, progress bar, and gold
    if (stats.level < 100) { // Not immortal
        long current_exp = stats.experience;
        long current_level_exp = ActorUtils::experience_for_level(stats.level);
        long next_level_exp = ActorUtils::experience_for_level(stats.level + 1);
        long exp_this_level = current_exp - current_level_exp;
        long exp_needed = next_level_exp - current_level_exp;

        // Calculate percentage and progress bar
        int percent = 0;
        if (exp_needed > 0) {
            percent = static_cast<int>((exp_this_level * 100) / exp_needed);
            percent = std::clamp(percent, 0, 100);
        }

        // Build progress bar (20 chars wide)
        constexpr int bar_width = 20;
        int filled = (percent * bar_width) / 100;
        std::string bar;
        bar += "<green>";
        for (int i = 0; i < filled; ++i) bar += "=";
        bar += "</>";
        bar += "<red>";
        for (int i = filled; i < bar_width; ++i) bar += "-";
        bar += "</>";

        score << fmt::format("Exp: {} / {}  [{}] {}%\n",
                             current_exp, next_level_exp, bar, percent);
    }
    // Currency - show full breakdown for players
    if (player) {
        score << fmt::format("Coins: {} plat, {} gold, {} silver, {} copper\n",
                             player->platinum(), player->gold(), player->silver(), player->copper());
    } else {
        score << fmt::format("Gold: {}\n", stats.gold);
    }

    // Current location
    if (ctx.room) {
        score << fmt::format("Location: {} [{}]\n", ctx.room->name(), ctx.room->id());
    }

    // Active effects (spells, buffs, etc.)
    const auto& effects = ctx.actor->active_effects();
    if (!effects.empty()) {
        score << "\n<cyan>Active Effects:</>\n";
        for (const auto& effect : effects) {
            std::string duration_str;
            if (effect.is_permanent()) {
                duration_str = "permanent";
            } else if (effect.duration_rounds == 1) {
                duration_str = "1 round";
            } else {
                duration_str = fmt::format("{} rounds", effect.duration_rounds);
            }

            // Show modifier if applicable
            if (!effect.modifier_stat.empty() && effect.modifier_value != 0) {
                std::string sign = effect.modifier_value > 0 ? "+" : "";
                score << fmt::format("  <green>{}</> ({}{} {}) - {}\n",
                                     effect.name, sign, effect.modifier_value,
                                     effect.modifier_stat, duration_str);
            } else {
                score << fmt::format("  <green>{}</> - {}\n", effect.name, duration_str);
            }
        }
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
    weather_report << "\n<b:cyan>--- Weather Report ---</>\n";
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
        weather_report << "\n<b:yellow>Upcoming Weather:</>\n";
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

    case TargetType::Self:
        // Handle "stat me" and "stat self" - use the current actor
        stat_info = ctx.actor->get_stat_info();
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

// =============================================================================
// Target Evaluation Commands
// =============================================================================

Result<CommandResult> cmd_consider(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Consider who?");
        return CommandResult::InvalidTarget;
    }

    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send("Easy! Very easy indeed!");
        return CommandResult::Success;
    }

    // Calculate level difference
    int level_diff = target->stats().level - ctx.actor->stats().level;

    std::string assessment;
    if (level_diff <= -10) {
        assessment = fmt::format("{} looks like an easy kill.", target->display_name());
    } else if (level_diff <= -5) {
        assessment = fmt::format("{} is no match for you.", target->display_name());
    } else if (level_diff <= -2) {
        assessment = fmt::format("{} shouldn't be too tough.", target->display_name());
    } else if (level_diff <= 0) {
        assessment = fmt::format("You are fairly evenly matched with {}.", target->display_name());
    } else if (level_diff <= 2) {
        assessment = fmt::format("{} looks slightly tougher than you.", target->display_name());
    } else if (level_diff <= 5) {
        assessment = fmt::format("{} looks pretty tough.", target->display_name());
    } else if (level_diff <= 10) {
        assessment = fmt::format("{} looks very dangerous!", target->display_name());
    } else {
        assessment = fmt::format("Are you CRAZY?! {} would destroy you!", target->display_name());
    }

    ctx.send(assessment);

    // Add health assessment if in combat range
    if (level_diff >= -5 && level_diff <= 5) {
        float hp_percent = static_cast<float>(target->stats().hit_points) /
                          static_cast<float>(target->stats().max_hit_points);

        std::string health_note;
        if (hp_percent >= 0.95) {
            health_note = "They appear to be in perfect health.";
        } else if (hp_percent >= 0.75) {
            health_note = "They have a few scratches.";
        } else if (hp_percent >= 0.50) {
            health_note = "They look moderately wounded.";
        } else if (hp_percent >= 0.25) {
            health_note = "They look badly hurt.";
        } else {
            health_note = "They appear to be near death!";
        }
        ctx.send(health_note);
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_diagnose(const CommandContext &ctx) {
    std::shared_ptr<Actor> target;

    if (ctx.arg_count() == 0) {
        target = ctx.actor;
    } else {
        target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
    }

    const auto& stats = target->stats();
    float hp_percent = static_cast<float>(stats.hit_points) / static_cast<float>(stats.max_hit_points);

    std::ostringstream diagnosis;

    if (target == ctx.actor) {
        diagnosis << "You diagnose yourself:\n";
    } else {
        diagnosis << fmt::format("You diagnose {}:\n", target->display_name());
    }

    // Health status
    std::string health_status;
    if (hp_percent >= 1.0) {
        health_status = "in perfect health";
    } else if (hp_percent >= 0.90) {
        health_status = "in excellent condition";
    } else if (hp_percent >= 0.75) {
        health_status = "in good condition with minor wounds";
    } else if (hp_percent >= 0.50) {
        health_status = "moderately wounded";
    } else if (hp_percent >= 0.25) {
        health_status = "badly wounded";
    } else if (hp_percent >= 0.10) {
        health_status = "near death";
    } else {
        health_status = "barely clinging to life";
    }

    if (target == ctx.actor) {
        diagnosis << fmt::format("  You are {}.\n", health_status);
        diagnosis << fmt::format("  HP: {}/{}, Stamina: {}/{}\n",
            stats.hit_points, stats.max_hit_points,
            stats.movement, stats.max_movement);
    } else {
        diagnosis << fmt::format("  {} is {}.\n", target->display_name(), health_status);
    }

    // Position status
    std::string_view pos_name = magic_enum::enum_name(target->position());
    if (target == ctx.actor) {
        diagnosis << fmt::format("  You are currently {}.\n", pos_name);
    } else {
        diagnosis << fmt::format("  {} is currently {}.\n", target->display_name(), pos_name);
    }

    ctx.send(diagnosis.str());
    return CommandResult::Success;
}

Result<CommandResult> cmd_glance(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Glance at who or what?");
        return CommandResult::InvalidTarget;
    }

    // Try to find an actor first
    auto target_actor = ctx.find_actor_target(ctx.arg(0));
    if (target_actor) {
        const auto& stats = target_actor->stats();
        float hp_percent = static_cast<float>(stats.hit_points) / static_cast<float>(stats.max_hit_points);

        std::string condition;
        if (hp_percent >= 0.95) {
            condition = "in excellent condition";
        } else if (hp_percent >= 0.75) {
            condition = "has some small wounds";
        } else if (hp_percent >= 0.50) {
            condition = "has quite a few wounds";
        } else if (hp_percent >= 0.25) {
            condition = "has some big nasty wounds";
        } else {
            condition = "is in awful condition";
        }

        ctx.send(fmt::format("{} is {}.", target_actor->display_name(), condition));
        ctx.send(fmt::format("{} is {}.", target_actor->display_name(),
            magic_enum::enum_name(target_actor->position())));

        return CommandResult::Success;
    }

    // Try to find an object
    auto target_info = ctx.resolve_target(ctx.arg(0));
    if (target_info.type == TargetType::Object && target_info.object) {
        ctx.send(fmt::format("You glance at {}.", target_info.object->short_description()));
        ctx.send(target_info.object->description());
        return CommandResult::Success;
    }

    ctx.send_error(fmt::format("You don't see {} here.", ctx.arg(0)));
    return CommandResult::InvalidTarget;
}

// =============================================================================
// Game Information Commands
// =============================================================================

Result<CommandResult> cmd_credits(const CommandContext &ctx) {
    ctx.send("<b:white>=== FieryMUD Credits ===</>\n");
    ctx.send("FieryMUD is based on CircleMUD, which was developed by:");
    ctx.send("  Jeremy Elson (creator)");
    ctx.send("  George Greer (maintainer)");
    ctx.send("\nCircleMUD is based on DikuMUD, created by:");
    ctx.send("  Hans Henrik Staerfeldt, Katja Nyboe, Tom Madsen,");
    ctx.send("  Michael Seifert, and Sebastian Hammer");
    ctx.send("\nFieryMUD development and modernization by the Fiery Consortium.");
    ctx.send("\nType 'help' for more information on available commands.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_motd(const CommandContext &ctx) {
    ctx.send("<b:cyan>=== Message of the Day ===</>\n");
    ctx.send("Welcome to FieryMUD!");
    ctx.send("This is a modern implementation of a classic MUD experience.");
    ctx.send("Please be respectful of other players and have fun!");
    ctx.send("\nType 'help' to see available commands.");
    ctx.send("Type 'who' to see who else is online.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_news(const CommandContext &ctx) {
    ctx.send("<b:yellow>=== FieryMUD News ===</>\n");
    ctx.send("No current news items.");
    ctx.send("\nCheck back later for updates!");
    return CommandResult::Success;
}

Result<CommandResult> cmd_policy(const CommandContext &ctx) {
    ctx.send("<b:red>=== FieryMUD Policies ===</>\n");
    ctx.send("1. Be respectful to other players.");
    ctx.send("2. No harassment, hate speech, or discriminatory behavior.");
    ctx.send("3. No cheating, exploiting bugs, or using automation.");
    ctx.send("4. Player killing requires consent from both parties.");
    ctx.send("5. Report bugs using the 'bug' command.");
    ctx.send("6. The staff reserves the right to remove players who violate policies.");
    ctx.send("\nViolation of these policies may result in removal from the game.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_version(const CommandContext &ctx) {
    ctx.send("<b:green>=== FieryMUD Version Information ===</>\n");
    ctx.send("FieryMUD Modern C++23 Edition");
    ctx.send("Version: 1.0.0-dev");
    ctx.send("Based on: CircleMUD 3.0 bpl 21");
    ctx.send("\nBuilt with modern C++23 features including:");
    ctx.send("  - std::expected for error handling");
    ctx.send("  - std::ranges for data processing");
    ctx.send("  - ASIO for asynchronous networking");
    ctx.send("  - nlohmann/json for world data");
    return CommandResult::Success;
}

// =============================================================================
// Scanning Commands
// =============================================================================

Result<CommandResult> cmd_scan(const CommandContext &ctx) {
    auto current_room = ctx.actor->current_room();
    if (!current_room) {
        ctx.send_error("You can't scan from here.");
        return CommandResult::InvalidState;
    }

    // Check if the room is visible
    if (!current_room->can_see_in_room(ctx.actor.get())) {
        ctx.send_error("It's too dark to see anything.");
        return CommandResult::InvalidState;
    }

    // List of directions to check in a sensible order
    constexpr std::array<Direction, 10> directions = {
        Direction::North, Direction::East, Direction::South, Direction::West,
        Direction::Up, Direction::Down,
        Direction::Northeast, Direction::Northwest, Direction::Southeast, Direction::Southwest
    };

    std::ostringstream scan_output;
    bool found_anything = false;

    for (Direction dir : directions) {
        if (!current_room->has_exit(dir)) {
            continue;
        }

        const auto* exit = current_room->get_exit(dir);
        if (!exit || !exit->to_room.is_valid()) {
            continue;
        }

        // Check if door is closed
        if (exit->has_door && exit->is_closed) {
            continue; // Can't see through closed doors
        }

        // Get the destination room
        auto dest_room = WorldManager::instance().get_room(exit->to_room);
        if (!dest_room) {
            continue;
        }

        // Get actors in that room
        auto actors = dest_room->contents().actors;
        if (actors.empty()) {
            continue;
        }

        // Found something - build the output
        std::string dir_name{RoomUtils::get_direction_name(dir)};
        // Capitalize first letter
        if (!dir_name.empty()) {
            dir_name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(dir_name[0])));
        }

        scan_output << fmt::format("<b:cyan>{}:</>\n", dir_name);

        for (const auto& actor : actors) {
            if (!actor) continue;

            // Get actor display info
            auto ground_desc = actor->ground();
            std::string actor_display;
            if (!ground_desc.empty()) {
                actor_display = std::string(ground_desc);
            } else {
                actor_display = std::string(actor->display_name());
            }

            // Add health/status indicator
            std::string condition;
            if (actor->position() == Position::Ghost) {
                condition = "<cyan>(ghost)</>";
            } else {
                const auto& stats = actor->stats();
                float hp_percent = static_cast<float>(stats.hit_points) /
                                  static_cast<float>(stats.max_hit_points);

                if (hp_percent >= 0.95) {
                    condition = "<green>(healthy)</>";
                } else if (hp_percent >= 0.75) {
                    condition = "<yellow>(wounded)</>";
                } else if (hp_percent >= 0.50) {
                    condition = "<yellow>(hurt)</>";
                } else if (hp_percent >= 0.25) {
                    condition = "<red>(badly hurt)</>";
                } else {
                    condition = "<b:red>(near death)</>";
                }
            }

            scan_output << fmt::format("  {} {}\n", actor_display, condition);
        }

        found_anything = true;
    }

    if (!found_anything) {
        ctx.send("You scan the area but don't see anyone nearby.");
    } else {
        ctx.send("You scan the area and see:\n" + scan_output.str());
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("look", cmd_look)
        .alias("l")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("examine", cmd_examine)
        .alias("exa")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("who", cmd_who)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("where", cmd_where)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("inventory", cmd_inventory)
        .alias("i")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("equipment", cmd_equipment)
        .alias("eq")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("score", cmd_score)
        .alias("sc")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("time", cmd_time)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("weather", cmd_weather)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Stat command - detailed information about targets (temporarily available to all for testing)
    Commands()
        .command("stat", cmd_stat)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .description("View detailed statistics for a target")
        .usage("stat <target|room|me>")
        .build();

    // Target Evaluation Commands
    Commands()
        .command("consider", cmd_consider)
        .alias("con")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("diagnose", cmd_diagnose)
        .alias("diag")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .build();

    Commands()
        .command("glance", cmd_glance)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    // Scanning Commands
    Commands()
        .command("scan", cmd_scan)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .description("Scan adjacent rooms for creatures")
        .build();

    // Game Information Commands
    Commands()
        .command("credits", cmd_credits)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .usable_while_sleeping(true)
        .build();

    Commands()
        .command("motd", cmd_motd)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .usable_while_sleeping(true)
        .build();

    Commands()
        .command("news", cmd_news)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .usable_while_sleeping(true)
        .build();

    Commands()
        .command("policy", cmd_policy)
        .alias("policies")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .usable_while_sleeping(true)
        .build();

    Commands()
        .command("version", cmd_version)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .usable_while_sitting(true)
        .usable_while_sleeping(true)
        .build();

    return Success();
}

} // namespace InformationCommands
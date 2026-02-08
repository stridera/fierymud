#include "information_commands.hpp"

#include "../core/actor.hpp"
#include "../core/board.hpp"
#include "../core/logging.hpp"
#include "../core/money.hpp"
#include "../core/object.hpp"
#include "../database/connection_pool.hpp"
#include "../database/game_data_cache.hpp"
#include "../database/world_queries.hpp"
#include "../game/composer_system.hpp"
#include "../net/player_connection.hpp"
#include "../server/world_server.hpp"
#include "../world/room.hpp"
#include "../world/time_system.hpp"
#include "../world/weather.hpp"
#include "../world/world_manager.hpp"
#include "builtin_commands.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace InformationCommands {

// Helper to format container contents with grouping of identical items
std::vector<std::string> format_grouped_contents(std::span<const std::shared_ptr<Object>> contents) {

    std::vector<std::string> result;

    // Group items by their entity ID (same template = same item type)
    std::unordered_map<std::string, int> item_counts;
    std::unordered_map<std::string, std::string> item_names;
    std::vector<std::string> order; // Preserve display order

    for (const auto &item : contents) {
        if (!item)
            continue;

        std::string key = fmt::format("{}", item->id());
        if (item_counts.find(key) == item_counts.end()) {
            order.push_back(key);
            item_names[key] = std::string(item->short_description());
        }
        item_counts[key]++;
    }

    // Format output with counts
    for (const auto &key : order) {
        int count = item_counts[key];
        if (count > 1) {
            result.push_back(fmt::format("  {} (x{})", item_names[key], count));
        } else {
            result.push_back(fmt::format("  {}", item_names[key]));
        }
    }

    return result;
}

/**
 * Convert effect duration from rounds to human-readable MUD time.
 * 1 round = 4 real seconds, 1 MUD hour = 75 real seconds
 * So approximately 19 rounds = 1 MUD hour
 */
static std::string format_effect_duration(int hours) {
    if (hours < 0) {
        return "permanent";
    }
    if (hours == 1) {
        return "1 hour";
    }
    return fmt::format("{} hours", hours);
}

// Helper function to get class display name with colors from cache
std::string get_class_display_name(std::string_view player_class) {
    if (player_class.empty())
        return "Unknown";

    const auto *class_data = GameDataCache::instance().find_class_by_name(player_class);
    if (class_data) {
        return class_data->name; // Returns name with color codes
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
    if (race.empty())
        return "Human";

    const auto *race_data = GameDataCache::instance().find_race_by_name(race);
    if (race_data) {
        return race_data->name; // Returns name with color codes
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
// Room Display Helper
// =============================================================================

std::string format_room_for_actor(const std::shared_ptr<Actor> &actor, const std::shared_ptr<Room> &room_param) {
    if (!actor) {
        return "";
    }

    auto room = room_param ? room_param : actor->current_room();
    if (!room) {
        return "You are nowhere.";
    }

    // Check visibility
    bool can_see = room->can_see_in_room(actor.get());

    if (!can_see) {
        // Room is dark - just return darkness message
        return room->get_room_description(actor.get());
    }

    // Check if actor is a player with brief mode enabled
    bool is_brief = false;
    bool show_exits = true;
    bool show_ids = false;
    bool has_holylight = actor->is_holylight();

    if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
        is_brief = player->is_brief();
        show_exits = player->is_autoexit();
        show_ids = player->is_show_ids();
    }

    std::ostringstream result;

    // Always show room name
    result << "<green>" << room->name() << "</>\n";

    // Show description only if not in brief mode
    if (!is_brief) {
        result << room->get_room_description(actor.get()) << "\n";
    }

    // Show exits if autoexit enabled
    if (show_exits) {
        std::string exits = BuiltinCommands::Helpers::format_exits(room);
        if (!exits.empty()) {
            result << "\n" << exits << "\n";
        }
    }

    // Show objects in room
    auto objects = room->contents().objects;
    bool found_objects = false;
    for (const auto &obj : objects) {
        if (obj) {
            auto ground_desc = obj->ground();
            if (ground_desc.empty()) {
                ground_desc = obj->short_description();
            }
            if (ground_desc.empty()) {
                continue; // Skip objects with no description
            }
            if (!found_objects) {
                result << "\n<yellow>You see:</>\n";
                found_objects = true;
            }
            result << fmt::format("  {}\n", ground_desc);
        }
    }

    // Show other actors
    auto actors = room->contents().actors;
    bool found_others = false;
    for (const auto &other : actors) {
        if (other && other != actor && other->is_visible_to(*actor)) {
            std::string actor_desc = other->room_presence(actor);
            if (actor_desc.empty()) {
                continue;
            }

            // Build status indicators
            std::string indicators;

            // Invisible
            if (other->has_flag(ActorFlag::Invisible) && (actor->has_flag(ActorFlag::Detect_Invis) || has_holylight)) {
                indicators += "(invis) ";
            }
            // Alignment
            if (actor->has_flag(ActorFlag::Detect_Align) || has_holylight) {
                int alignment = other->stats().alignment;
                if (alignment <= -350) {
                    indicators += "(evil) ";
                } else if (alignment >= 350) {
                    indicators += "(good) ";
                }
            }
            // Magic detection
            if ((actor->has_flag(ActorFlag::Detect_Magic) || has_holylight) && !other->active_effects().empty()) {
                indicators += "(magical) ";
            }
            // Hidden
            if (other->has_flag(ActorFlag::Hide) && (actor->has_flag(ActorFlag::Sense_Life) || has_holylight)) {
                indicators += "(hidden) ";
            }
            // Poison
            if (other->has_flag(ActorFlag::Poison) && (actor->has_flag(ActorFlag::Detect_Poison) || has_holylight)) {
                indicators += "<magenta>(poisoned)</> ";
            }
            // On fire
            if (other->has_flag(ActorFlag::On_Fire)) {
                indicators += "<red>(burning)</> ";
            }
            // AFK
            if (other->is_afk()) {
                indicators += "<yellow>[AFK]</> ";
            }
            // Show ID for immortals
            if (show_ids) {
                indicators += fmt::format("<cyan>[{}.{}]</> ", other->id().zone_id(), other->id().local_id());
            }

            if (!found_others) {
                result << "\n";
                found_others = true;
            }
            result << fmt::format("  {}{}\n", indicators, actor_desc);
        }
    }

    return result.str();
}

// =============================================================================
// Information Commands
// =============================================================================

Result<CommandResult> cmd_look(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        // Look at room - use helper function that respects brief mode
        std::string room_display = format_room_for_actor(ctx.actor);
        if (room_display.empty()) {
            ctx.send_error("You are not in a room.");
            return CommandResult::InvalidState;
        }
        ctx.send(room_display);
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

        // Check if it's a drink container first
        if (container->type() == ObjectType::Drinkcontainer) {
            const auto &liquid = container->liquid_info();
            // Get the object name and replace article with "The" for definite reference
            std::string obj_name = ctx.format_object_name(container);
            // Strip leading "a " or "an " and replace with "The "
            if (obj_name.starts_with("a ")) {
                obj_name = "The " + obj_name.substr(2);
            } else if (obj_name.starts_with("an ")) {
                obj_name = "The " + obj_name.substr(3);
            } else if (!obj_name.empty()) {
                // Capitalize first letter if no article to replace
                obj_name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(obj_name[0])));
            }

            if (liquid.remaining <= 0) {
                ctx.send(fmt::format("{} is empty.", obj_name));
            } else {
                // Calculate fullness description
                double fullness_ratio = static_cast<double>(liquid.remaining) / liquid.capacity;
                std::string fullness_desc;
                if (fullness_ratio >= 0.95) {
                    fullness_desc = "full of";
                } else if (fullness_ratio >= 0.75) {
                    fullness_desc = "three-quarters full of";
                } else if (fullness_ratio >= 0.50) {
                    fullness_desc = "half full of";
                } else if (fullness_ratio >= 0.25) {
                    fullness_desc = "a quarter full of";
                } else {
                    fullness_desc = "nearly empty, with just a bit of";
                }

                // Get liquid description - use color description if not identified
                // Note: This uses the liquid's identified flag, not the container's Identified flag
                std::string liquid_desc;
                bool is_identified = liquid.identified;

                // Look up liquid data from cache
                auto &cache = GameDataCache::instance();
                const LiquidData *liquid_data = cache.find_liquid_by_name(liquid.liquid_type);

                if (liquid_data) {
                    if (is_identified) {
                        liquid_desc = liquid_data->name; // "water", "dark ale"
                    } else {
                        // Not identified - show appearance-based description
                        liquid_desc =
                            fmt::format("a {} liquid", liquid_data->color_desc); // "a clear liquid", "a brown liquid"
                    }
                } else if (!liquid.liquid_type.empty()) {
                    // Unknown liquid type - fallback
                    if (is_identified) {
                        liquid_desc = liquid.liquid_type;
                        std::transform(liquid_desc.begin(), liquid_desc.end(), liquid_desc.begin(), ::tolower);
                    } else {
                        liquid_desc = "some strange liquid";
                    }
                } else {
                    liquid_desc = "some strange liquid";
                }

                // Only show effect hint if liquid is identified and has effects
                std::string effect_hint = "";
                if (liquid.identified && !liquid.effects.empty()) {
                    effect_hint = " (tainted!)";
                }
                ctx.send(fmt::format("{} is {} {}{}.", obj_name, fullness_desc, liquid_desc, effect_hint));
            }
            return CommandResult::Success;
        }

        // Check if it's a regular container
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
            for (const auto &line : format_grouped_contents(contents)) {
                ctx.send(line);
            }
        }

        return CommandResult::Success;
    }

    // Look at target
    auto target_info = ctx.resolve_target(ctx.arg(0));

    // If target is a string (not found as object/actor/room), try extra descriptions first
    // This allows "look words" to work when "words" is an extra desc keyword
    if (!target_info.is_valid() || target_info.type == TargetType::String) {
        std::string keyword(ctx.arg(0));

        // Search inventory for extra descriptions
        if (ctx.actor) {
            for (const auto &item : ctx.actor->inventory().get_all_items()) {
                if (item) {
                    auto extra = item->get_extra_description(keyword);
                    if (extra.has_value()) {
                        ctx.send(std::string(extra.value()));
                        return CommandResult::Success;
                    }
                }
            }
        }

        // Search room objects for extra descriptions
        if (ctx.room) {
            for (const auto &item : ctx.room->contents().objects) {
                if (item) {
                    auto extra = item->get_extra_description(keyword);
                    if (extra.has_value()) {
                        ctx.send(std::string(extra.value()));
                        return CommandResult::Success;
                    }
                }
            }
        }

        // Nothing found - report the error
        if (!target_info.is_valid() || target_info.type == TargetType::String) {
            ctx.send_error(fmt::format("You don't see '{}' here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
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

        // For boards, append a hint about the board command
        if (target_info.object->is_board()) {
            description += "\n<yellow>Use 'board' to view and interact with this bulletin board.</>";
        }

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
            auto value_money = fiery::Money::from_copper(obj->value());
            detailed_desc << fmt::format("Value: {}\n", value_money.to_string());
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
            if (container_info.weight_reduction > 0) {
                detailed_desc << fmt::format("Weight reduction: {}% (bag of holding)\n",
                                             container_info.weight_reduction);
            }
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
                        for (const auto &line : format_grouped_contents(contents)) {
                            detailed_desc << line << "\n";
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
            if (light_info.permanent) {
                detailed_desc << "Duration: permanent\n";
                detailed_desc << "Status: always lit\n";
            } else {
                detailed_desc << fmt::format("Duration: {} hours{}\n", light_info.duration,
                                             light_info.duration < 0 ? " (infinite)" : "");
                detailed_desc << fmt::format("Status: {}\n", light_info.lit ? "lit" : "unlit");
            }
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

// Helper to convert ConnectionState to string
static std::string connection_state_string(ConnectionState state) {
    switch (state) {
    case ConnectionState::Connected:
        return "Connecting";
    case ConnectionState::Login:
        return "Login";
    case ConnectionState::Playing:
        return "Playing";
    case ConnectionState::AFK:
        return "AFK";
    case ConnectionState::Linkdead:
        return "Linkdead";
    case ConnectionState::Reconnecting:
        return "Reconnect";
    case ConnectionState::Disconnecting:
        return "Closing";
    case ConnectionState::Disconnected:
        return "Closed";
    default:
        return "Unknown";
    }
}

// Helper to format idle time
static std::string format_idle_time(std::chrono::seconds idle) {
    auto seconds = idle.count();
    if (seconds < 60) {
        return fmt::format("{}s", seconds);
    } else if (seconds < 3600) {
        return fmt::format("{}m", seconds / 60);
    } else {
        return fmt::format("{}h", seconds / 3600);
    }
}

// Helper to format connect time
static std::string format_connect_time(std::chrono::steady_clock::time_point connect_time) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - connect_time);
    auto seconds = duration.count();

    if (seconds < 60) {
        return fmt::format("{}s", seconds);
    } else if (seconds < 3600) {
        return fmt::format("{}m", seconds / 60);
    } else if (seconds < 86400) {
        return fmt::format("{}h{}m", seconds / 3600, (seconds % 3600) / 60);
    } else {
        return fmt::format("{}d{}h", seconds / 86400, (seconds % 86400) / 3600);
    }
}

Result<CommandResult> cmd_users(const CommandContext &ctx) {
    // This is an immortal command - show all connections with details
    auto *world_server = WorldServer::instance();
    if (!world_server) {
        ctx.send_error("Server not available.");
        return CommandResult::InvalidState;
    }

    auto connections = world_server->get_active_connections();

    if (connections.empty()) {
        ctx.send_line("No connections.");
        return CommandResult::Success;
    }

    std::ostringstream output;

    // Header
    output << fmt::format("{:<12} {:<10} {:<20} {:>5} {:>8} {:<20} {}\n", "Name", "State", "Host", "Idle", "Online",
                          "Room", "Client");
    output << std::string(95, '-') << "\n";

    int playing_count = 0;
    int total_count = 0;

    for (const auto &conn : connections) {
        if (!conn)
            continue;

        // Skip disconnected connections - they're stale and will be cleaned up soon
        if (!conn->is_connected())
            continue;

        total_count++;

        // Get connection info
        auto state = conn->state();
        std::string state_str = connection_state_string(state);
        std::string host = conn->remote_address();
        std::string idle_str = format_idle_time(conn->idle_time());
        std::string online_str = format_connect_time(conn->connect_time());

        // Truncate host if too long
        if (host.length() > 20) {
            host = host.substr(0, 17) + "...";
        }

        // Get player name and room if logged in
        std::string name = "---";
        std::string room_str = "---";
        auto actor = world_server->get_actor_for_connection(conn);
        if (actor) {
            name = actor->name();
            if (name.length() > 12) {
                name = name.substr(0, 9) + "...";
            }
            if (auto room = actor->current_room()) {
                room_str = fmt::format("{} {}", room->id(), room->name());
                if (room_str.length() > 20) {
                    room_str = room_str.substr(0, 17) + "...";
                }
            }
            if (state == ConnectionState::Playing) {
                playing_count++;
            }
        }

        // Get client info from GMCP
        std::string client_str = "---";
        auto &caps = conn->get_terminal_capabilities();
        if (!caps.client_name.empty() && caps.client_name != "Unknown") {
            if (!caps.client_version.empty()) {
                client_str = fmt::format("{} {}", caps.client_name, caps.client_version);
            } else {
                client_str = caps.client_name;
            }
        } else if (!caps.terminal_name.empty()) {
            client_str = caps.terminal_name;
        }

        output << fmt::format("{:<12} {:<10} {:<20} {:>5} {:>8} {:<20} {}\n", name, state_str, host, idle_str,
                              online_str, room_str, client_str);
    }

    output << std::string(95, '-') << "\n";
    output << fmt::format("{} connections total, {} playing.\n", total_count, playing_count);

    ctx.send(output.str());
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
        ctx.send_line(
            fmt::format("{} is in: {} [{}]", target->display_name(), target_room->display_name(), target_room->id()));
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
    score << fmt::format("Level: {}  Class: {}  Race: {}  Size: {}  Gender: {}\n", stats.level, player_class, race,
                         size, gender_str);

    // Age, height, weight (placeholder values)
    score << fmt::format("Age: {} years, {} months  Height: 5'10\"  Weight: 180 lbs\n", 20 + stats.level,
                         (stats.level * 3) % 12);

    // Abilities
    score << fmt::format("Str: {}    Int: {}     Wis: {}\n", stats.strength, stats.intelligence, stats.wisdom);
    score << fmt::format("Dex: {}    Con: {}     Cha: {}\n", stats.dexterity, stats.constitution, stats.charisma);

    // Hit points and stamina
    score << fmt::format("Hit points: {}/{}   Stamina: {}/{}\n", stats.hit_points, stats.max_hit_points, stats.stamina,
                         stats.max_stamina);

    // Condition status (buffs and drunk)
    std::string condition_line;
    bool has_condition = false;

    // Nourished buff (from eating - +50% HP regen)
    if (ctx.actor->has_effect("Nourished")) {
        condition_line += "<green>Nourished</>";
        has_condition = true;
    }

    // Refreshed buff (from drinking - +50% stamina regen)
    if (ctx.actor->has_effect("Refreshed")) {
        if (has_condition)
            condition_line += "   ";
        condition_line += "<cyan>Refreshed</>";
        has_condition = true;
    }

    // Drunk status
    if (stats.is_too_drunk()) {
        if (has_condition)
            condition_line += "   ";
        condition_line += "<magenta>Very Drunk!</>";
        has_condition = true;
    } else if (stats.is_slurring()) {
        if (has_condition)
            condition_line += "   ";
        condition_line += "<magenta>Tipsy</>";
        has_condition = true;
    } else if (stats.drunk > 0) {
        if (has_condition)
            condition_line += "   ";
        condition_line += "<magenta>Buzzed</>";
        has_condition = true;
    }

    if (has_condition) {
        score << fmt::format("Condition: {}\n", condition_line);
    }

    // Combat stats (new ACC/EVA system)
    score << fmt::format("Accuracy: {}   Evasion: {}   Attack Power: {}\n", stats.accuracy, stats.evasion,
                         stats.attack_power);
    score << fmt::format("Armor Rating: {}   Damage Reduction: {}%\n", stats.armor_rating,
                         stats.damage_reduction_percent);

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
        for (int i = 0; i < filled; ++i)
            bar += "=";
        bar += "</>";
        bar += "<red>";
        for (int i = filled; i < bar_width; ++i)
            bar += "-";
        bar += "</>";

        score << fmt::format("Exp: {} / {}  [{}] {}%\n", current_exp, next_level_exp, bar, percent);
    }
    // Currency - show full breakdown for players
    if (player) {
        score << fmt::format("Coins: {}\n", player->wallet().to_string());
    } else {
        score << fmt::format("Gold: {}\n", stats.gold);
    }

    // Current location
    if (ctx.room) {
        score << fmt::format("Location: {} [{}]\n", ctx.room->name(), ctx.room->id());
    }

    // Active effects (spells, buffs, etc.)
    const auto &effects = ctx.actor->active_effects();
    if (!effects.empty()) {
        score << "\n<cyan>Active Effects:</>\n";
        for (const auto &effect : effects) {
            std::string duration_str = format_effect_duration(effect.duration_hours);

            // Show modifier if applicable
            if (!effect.modifier_stat.empty() && effect.modifier_value != 0) {
                std::string sign = effect.modifier_value > 0 ? "+" : "";
                score << fmt::format("  <green>{}</> ({}{} {}) - {}\n", effect.name, sign, effect.modifier_value,
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
    // Get in-game time from TimeSystem
    const auto &game_time = TimeSystem::instance().current_time();

    // Day names for the week (7 days)
    static constexpr std::array<std::string_view, 7> DAY_NAMES = {
        "the Moon", "the Bull", "the Deception", "Thunder", "Freedom", "the Great Gods", "the Sun"};

    // Format hour as 12-hour time with am/pm
    int display_hour = game_time.hour % 12;
    if (display_hour == 0)
        display_hour = 12;
    std::string_view am_pm = (game_time.hour >= 12) ? "pm" : "am";

    // Get day of week (0-6)
    int day_of_week = game_time.day % 7;
    std::string_view day_name = DAY_NAMES[day_of_week];

    // Get ordinal suffix for day of month
    int day_of_month = game_time.day + 1; // 1-indexed for display
    std::string ordinal;
    if (day_of_month == 1 || day_of_month == 21) {
        ordinal = "st";
    } else if (day_of_month == 2 || day_of_month == 22) {
        ordinal = "nd";
    } else if (day_of_month == 3 || day_of_month == 23) {
        ordinal = "rd";
    } else {
        ordinal = "th";
    }

    // Format the time display like legacy
    std::ostringstream time_str;
    time_str << "\nIt is " << display_hour << " o'clock " << am_pm << ", on the Day of " << day_name << ";\n"
             << "The " << day_of_month << ordinal << " Day of " << game_time.month_name() << ", Year "
             << (game_time.year + 1000) << ".\n";

    ctx.send(time_str.str());
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
        float hp_percent =
            static_cast<float>(target->stats().hit_points) / static_cast<float>(target->stats().max_hit_points);

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

    const auto &stats = target->stats();
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
        diagnosis << fmt::format("  HP: {}/{}, Stamina: {}/{}\n", stats.hit_points, stats.max_hit_points, stats.stamina,
                                 stats.max_stamina);
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
        const auto &stats = target_actor->stats();
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
        ctx.send(
            fmt::format("{} is {}.", target_actor->display_name(), magic_enum::enum_name(target_actor->position())));

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
// Game Information Commands (Database-Driven)
// =============================================================================

// Helper function to display system text from database with fallback
static void display_system_text(const CommandContext &ctx, const std::string &key, const std::string &fallback_title,
                                const std::string &fallback_content) {
    // Try to load from database
    auto result = ConnectionPool::instance().execute(
        [&key](pqxx::work &txn) -> Result<std::optional<WorldQueries::SystemTextData>> {
            return WorldQueries::load_system_text(txn, key);
        });

    if (result && *result) {
        const auto &text = **result;
        // Display title if present
        if (text.title) {
            ctx.send(fmt::format("=== {} ===\n", *text.title));
        }
        // Display content (supports rich text color codes)
        ctx.send(text.content);
    } else {
        // Fallback to hardcoded content if database unavailable
        ctx.send(fmt::format("=== {} ===\n", fallback_title));
        ctx.send(fallback_content);
    }
}

Result<CommandResult> cmd_credits(const CommandContext &ctx) {
    display_system_text(ctx, "credits", "FieryMUD Credits",
                        "FieryMUD is based on CircleMUD, which was developed by:\n"
                        "  Jeremy Elson (creator)\n"
                        "  George Greer (maintainer)\n"
                        "\nCircleMUD is based on DikuMUD, created by:\n"
                        "  Hans Henrik Staerfeldt, Katja Nyboe, Tom Madsen,\n"
                        "  Michael Seifert, and Sebastian Hammer\n"
                        "\nFieryMUD development and modernization by the Fiery Consortium.\n"
                        "\nType 'help' for more information on available commands.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_motd(const CommandContext &ctx) {
    display_system_text(ctx, "motd", "Message of the Day",
                        "Welcome to FieryMUD!\n"
                        "This is a modern implementation of a classic MUD experience.\n"
                        "Please be respectful of other players and have fun!\n"
                        "\nType 'help' to see available commands.\n"
                        "Type 'who' to see who else is online.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_news(const CommandContext &ctx) {
    display_system_text(ctx, "news", "FieryMUD News",
                        "No current news items.\n"
                        "\nCheck back later for updates!");
    return CommandResult::Success;
}

Result<CommandResult> cmd_policy(const CommandContext &ctx) {
    display_system_text(ctx, "policy", "FieryMUD Policies",
                        "1. Be respectful to other players.\n"
                        "2. No harassment, hate speech, or discriminatory behavior.\n"
                        "3. No cheating, exploiting bugs, or using automation.\n"
                        "4. Player killing requires consent from both parties.\n"
                        "5. Report bugs using the 'bug' command.\n"
                        "6. The staff reserves the right to remove players who violate policies.\n"
                        "\nViolation of these policies may result in removal from the game.");
    return CommandResult::Success;
}

Result<CommandResult> cmd_version(const CommandContext &ctx) {
    display_system_text(ctx, "version", "FieryMUD Version Information",
                        "FieryMUD Modern C++23 Edition\n"
                        "Version: 1.0.0-dev\n"
                        "Based on: CircleMUD 3.0 bpl 21\n"
                        "\nBuilt with modern C++23 features including:\n"
                        "  - std::expected for error handling\n"
                        "  - std::ranges for data processing\n"
                        "  - ASIO for asynchronous networking\n"
                        "  - nlohmann/json for world data");
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
        Direction::North, Direction::East,      Direction::South,     Direction::West,      Direction::Up,
        Direction::Down,  Direction::Northeast, Direction::Northwest, Direction::Southeast, Direction::Southwest};

    std::ostringstream scan_output;
    bool found_anything = false;

    for (Direction dir : directions) {
        if (!current_room->has_exit(dir)) {
            continue;
        }

        const auto *exit = current_room->get_exit(dir);
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

        // Build the list of visible actors first
        std::ostringstream dir_actors;
        bool found_visible = false;

        for (const auto &actor : actors) {
            if (!actor || !actor->is_visible_to(*ctx.actor))
                continue;

            // Get actor display info
            auto ground_desc = actor->ground();
            std::string actor_display;
            if (!ground_desc.empty()) {
                actor_display = std::string(ground_desc);
            } else {
                actor_display = actor->display_name();
            }

            // Build status indicator prefix
            std::string indicators;
            bool has_holylight = ctx.actor->is_holylight();

            if (actor->position() == Position::Ghost) {
                indicators = "<cyan>(ghost)</> ";
            }
            // Invisible - holylight sees all invisible
            if (actor->has_flag(ActorFlag::Invisible) &&
                (ctx.actor->has_flag(ActorFlag::Detect_Invis) || has_holylight)) {
                indicators += "(invis) ";
            }
            // Alignment - holylight sees all alignments
            if (ctx.actor->has_flag(ActorFlag::Detect_Align) || has_holylight) {
                int alignment = actor->stats().alignment;
                if (alignment <= -350) {
                    indicators += "(evil) ";
                } else if (alignment >= 350) {
                    indicators += "(good) ";
                }
            }
            // Magic detection - holylight sees all magical auras
            if ((ctx.actor->has_flag(ActorFlag::Detect_Magic) || has_holylight) && !actor->active_effects().empty()) {
                indicators += "(magical) ";
            }
            // Hidden - holylight sees all hidden
            if (actor->has_flag(ActorFlag::Hide) && (ctx.actor->has_flag(ActorFlag::Sense_Life) || has_holylight)) {
                indicators += "(hidden) ";
            }
            // Poison detection - holylight sees all poisoned
            if (actor->has_flag(ActorFlag::Poison) &&
                (ctx.actor->has_flag(ActorFlag::Detect_Poison) || has_holylight)) {
                indicators += "<magenta>(poisoned)</> ";
            }
            // Status effects visible to all
            if (actor->has_flag(ActorFlag::On_Fire)) {
                indicators += "<red>(burning)</> ";
            }
            if (actor->has_flag(ActorFlag::Immobilized) || actor->has_flag(ActorFlag::Paralyzed)) {
                indicators += "<red>(immobilized)</> ";
            }
            // AFK indicator for players
            if (actor->is_afk()) {
                indicators += "<yellow>[AFK]</> ";
            }
            // Show ID for immortals with ShowIds enabled
            if (ctx.actor->is_show_ids()) {
                indicators += fmt::format("<cyan>[{}.{}]</> ", actor->id().zone_id(), actor->id().local_id());
            }

            // Add health/status indicator
            std::string condition;
            if (actor->position() != Position::Ghost) {
                const auto &stats = actor->stats();
                float hp_percent = static_cast<float>(stats.hit_points) / static_cast<float>(stats.max_hit_points);

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

            dir_actors << fmt::format("  {}{} {}\n", indicators, actor_display, condition);
            found_visible = true;
        }

        // Only output direction header if we found visible actors
        if (found_visible) {
            std::string dir_name{RoomUtils::get_direction_name(dir)};
            // Capitalize first letter
            if (!dir_name.empty()) {
                dir_name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(dir_name[0])));
            }
            scan_output << fmt::format("<b:cyan>{}:</>\n", dir_name);
            scan_output << dir_actors.str();
            found_anything = true;
        }
    }

    if (!found_anything) {
        ctx.send("You scan the area but don't see anyone nearby.");
    } else {
        ctx.send("You scan the area and see:\n" + scan_output.str());
    }

    return CommandResult::Success;
}

// =============================================================================
// Board Commands
// =============================================================================

/**
 * Find a board object in the current room.
 */
static std::shared_ptr<Object> find_board_in_room(const CommandContext &ctx) {
    if (!ctx.room)
        return nullptr;

    for (const auto &obj : ctx.room->contents().objects) {
        if (obj && obj->is_board()) {
            return obj;
        }
    }
    return nullptr;
}

/**
 * Display the list of messages on a board.
 */
static void show_board_list(const CommandContext &ctx, const BoardData *board) {
    std::ostringstream output;
    output << fmt::format("<cyan>{}</>\n", board->title);

    if (board->locked) {
        output << "<red>(This board is currently locked - no new posts allowed)</>\n";
    }

    if (board->messages.empty()) {
        output << "\nThe board is empty.\n";
    } else {
        output << fmt::format("\nThis board has {} message{}.\n", board->messages.size(),
                              board->messages.size() == 1 ? "" : "s");
        output << "Usage: board read <number>, board write <subject>, board remove <number>\n\n";

        // Display messages (numbered from 1, newest first)
        int msg_num = static_cast<int>(board->messages.size());
        for (const auto &msg : board->messages) {
            std::string sticky_marker = msg.sticky ? "<yellow>[STICKY]</> " : "";
            auto msg_time = std::chrono::system_clock::to_time_t(msg.posted_at);
            std::tm msg_tm = *std::localtime(&msg_time);
            output << fmt::format("{:3}. {}{} ({:04d}-{:02d}-{:02d}) :: {}\n", msg_num--, sticky_marker, msg.poster,
                                  msg_tm.tm_year + 1900, msg_tm.tm_mon + 1, msg_tm.tm_mday, msg.subject);
        }
    }

    ctx.send(output.str());
}

/**
 * Display a specific message on a board.
 */
static CommandResult show_board_message(const CommandContext &ctx, const BoardData *board, int msg_num) {
    // Messages are displayed in reverse order (newest first), but numbered 1-N from newest
    if (msg_num < 1 || msg_num > static_cast<int>(board->messages.size())) {
        ctx.send_error(fmt::format("That message doesn't exist. Valid range: 1-{}.", board->messages.size()));
        return CommandResult::InvalidTarget;
    }

    // Get message (index from the end since messages are sorted newest first)
    size_t index = board->messages.size() - static_cast<size_t>(msg_num);
    const auto &msg = board->messages[index];

    // Format the message for display
    std::ostringstream output;
    output << fmt::format("<cyan>Message {} on {}</>\n\n", msg_num, board->title);

    // Format the posted_at time
    auto posted_time = std::chrono::system_clock::to_time_t(msg.posted_at);
    std::tm posted_tm = *std::localtime(&posted_time);
    output << fmt::format("<yellow>Date:</> {:04d}-{:02d}-{:02d} {:02d}:{:02d}\n", posted_tm.tm_year + 1900,
                          posted_tm.tm_mon + 1, posted_tm.tm_mday, posted_tm.tm_hour, posted_tm.tm_min);
    output << fmt::format("<yellow>Author:</> {} (level {})\n", msg.poster, msg.poster_level);
    output << fmt::format("<yellow>Subject:</> {}\n\n", msg.subject);
    output << msg.content;

    if (!msg.edits.empty()) {
        auto edit_time = std::chrono::system_clock::to_time_t(msg.edits.back().edited_at);
        std::tm edit_tm = *std::localtime(&edit_time);
        output << fmt::format("\n\n<red>(Edited {} time{} - last by {} on {:04d}-{:02d}-{:02d} {:02d}:{:02d})</>",
                              msg.edits.size(), msg.edits.size() == 1 ? "" : "s", msg.edits.back().editor,
                              edit_tm.tm_year + 1900, edit_tm.tm_mon + 1, edit_tm.tm_mday, edit_tm.tm_hour,
                              edit_tm.tm_min);
    }

    ctx.send(output.str());
    return CommandResult::Success;
}

Result<CommandResult> cmd_board(const CommandContext &ctx) {
    // Find a board in the room
    auto board_obj = find_board_in_room(ctx);
    if (!board_obj) {
        ctx.send_error("There is no bulletin board here.");
        return CommandResult::InvalidTarget;
    }

    int board_id = board_obj->board_number();
    const auto *board = board_system().get_board(board_id);

    if (!board) {
        ctx.send_error("This board appears to be broken.");
        return CommandResult::InvalidState;
    }

    // No arguments or "list" - show the message list
    if (ctx.arg_count() == 0) {
        show_board_list(ctx, board);
        return CommandResult::Success;
    }

    std::string subcmd(ctx.arg(0));
    std::transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::tolower);

    // board list
    if (subcmd == "list" || subcmd == "l") {
        show_board_list(ctx, board);
        return CommandResult::Success;
    }

    // board read <number>
    if (subcmd == "read" || subcmd == "r") {
        if (ctx.arg_count() < 2) {
            ctx.send_error("Usage: board read <message number>");
            return CommandResult::InvalidSyntax;
        }

        std::string num_str(ctx.arg(1));
        bool is_number = !num_str.empty() && std::all_of(num_str.begin(), num_str.end(), ::isdigit);
        if (!is_number) {
            ctx.send_error("Please specify a message number.");
            return CommandResult::InvalidSyntax;
        }

        int msg_num = std::stoi(num_str);
        return show_board_message(ctx, board, msg_num);
    }

    // board write <subject>
    if (subcmd == "write" || subcmd == "w" || subcmd == "post") {
        auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
        if (!player) {
            ctx.send_error("Only players can post to boards.");
            return CommandResult::InvalidState;
        }

        if (board->locked) {
            ctx.send_error("This board is currently locked. No new posts allowed.");
            return CommandResult::InvalidState;
        }

        if (ctx.arg_count() < 2) {
            ctx.send_error("Usage: board write <subject>");
            return CommandResult::InvalidSyntax;
        }

        // Get the subject (everything after "write")
        std::string subject;
        for (size_t i = 1; i < ctx.arg_count(); ++i) {
            if (!subject.empty())
                subject += " ";
            subject += ctx.arg(i);
        }

        // Configure the composer
        ComposerConfig config;
        config.header_message = fmt::format("Composing post: {}\n(. to save, ~q to cancel, ~h for help):", subject);
        config.save_message = "Post composed.";
        config.cancel_message = "Post cancelled.";

        // Capture values for the callback
        std::string captured_subject = subject;
        std::string captured_poster(player->name());
        int captured_level = player->stats().level;
        std::string captured_board_title = board->title;

        auto composer = std::make_shared<ComposerSystem>(std::weak_ptr<Player>(player), config);

        composer->set_completion_callback([player, board_id, captured_subject, captured_poster, captured_level,
                                           captured_board_title](ComposerResult result) {
            if (result.success && !result.combined_text.empty()) {
                auto post_result = board_system().post_message(board_id, captured_poster, captured_level,
                                                               captured_subject, result.combined_text);

                if (post_result) {
                    player->send_message(fmt::format("<green>Message posted to {}.</> (Message #{})",
                                                     captured_board_title, *post_result));
                } else {
                    player->send_message(
                        fmt::format("<red>Failed to post message: {}</>", post_result.error().message));
                }
            } else if (result.success && result.combined_text.empty()) {
                player->send_message("No message entered. Post not saved.");
            }
            // Cancelled post doesn't need additional message - composer already sent "Post cancelled."
        });

        player->start_composing(composer);
        return CommandResult::Success;
    }

    // board remove <number>
    if (subcmd == "remove" || subcmd == "delete" || subcmd == "rem" || subcmd == "del") {
        if (!ctx.actor) {
            ctx.send_error("You must be logged in to remove messages.");
            return CommandResult::InvalidState;
        }

        if (ctx.arg_count() < 2) {
            ctx.send_error("Usage: board remove <message number>");
            return CommandResult::InvalidSyntax;
        }

        std::string num_str(ctx.arg(1));
        bool is_number = !num_str.empty() && std::all_of(num_str.begin(), num_str.end(), ::isdigit);
        if (!is_number) {
            ctx.send_error("Please specify a message number.");
            return CommandResult::InvalidSyntax;
        }

        int msg_num = std::stoi(num_str);

        // Get the message to check ownership
        const auto *msg = board_system().get_message(board_id, msg_num);
        if (!msg) {
            ctx.send_error(fmt::format("Message {} not found.", msg_num));
            return CommandResult::InvalidTarget;
        }

        // Check if user can remove this message
        // Can remove if: own message, or god level (level >= 60)
        bool is_owner = (msg->poster == ctx.actor->name());
        bool is_god = (ctx.actor->stats().level >= 60); // LVL_GOD

        if (!is_owner && !is_god) {
            ctx.send_error("You can only remove your own messages.");
            return CommandResult::InsufficientPrivs;
        }

        auto result = board_system().remove_message(board_id, msg_num);
        if (!result || !*result) {
            ctx.send_error("Failed to remove message.");
            return CommandResult::SystemError;
        }

        ctx.send(fmt::format("<green>Message {} removed from {}.</>", msg_num, board->title));
        return CommandResult::Success;
    }

    // Unknown subcommand - maybe it's a number for reading
    bool is_number = !subcmd.empty() && std::all_of(subcmd.begin(), subcmd.end(), ::isdigit);
    if (is_number) {
        int msg_num = std::stoi(subcmd);
        return show_board_message(ctx, board, msg_num);
    }

    ctx.send_error("Usage: board [list|read <n>|write <subject>|remove <n>]");
    return CommandResult::InvalidSyntax;
}

// =============================================================================
// Search and Discovery
// =============================================================================

Result<CommandResult> cmd_search(const CommandContext &ctx) {
    if (!ctx.room) {
        ctx.send_error("You are not in a room.");
        return CommandResult::InvalidState;
    }

    // Can't search while fighting
    if (ctx.actor->is_fighting()) {
        ctx.send_error("You're too busy fighting to search!");
        return CommandResult::InvalidState;
    }

    ctx.send("You search the area carefully...");
    ctx.send_to_room(fmt::format("{} searches the area.", ctx.actor->display_name()), true);

    // TODO: Check for hidden exits
    // TODO: Check for hidden items
    // TODO: Check for hidden NPCs
    // TODO: Apply search skill check

    ctx.send("You find nothing hidden.");
    ctx.send("Note: Full search functionality not yet implemented.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_read(const CommandContext &ctx) {
    if (ctx.arg_count() == 0) {
        ctx.send_error("Read what?");
        return CommandResult::InvalidSyntax;
    }

    std::string_view target_name = ctx.arg(0);

    // Check if they're trying to read a board
    if (target_name == "board") {
        // Forward to board command
        return cmd_board(ctx);
    }

    // Look for an object to read
    auto objects = ctx.find_objects_matching(target_name, true);
    if (objects.empty()) {
        ctx.send_error(fmt::format("You don't see any '{}' to read.", target_name));
        return CommandResult::InvalidTarget;
    }

    auto obj = objects.front();

    // TODO: Check if object is readable (has description or notes)
    ctx.send(fmt::format("You read {}:", obj->short_description()));

    // For now, just show the object's description
    auto desc = obj->description();
    if (!desc.empty()) {
        ctx.send(std::string{desc});
    } else {
        ctx.send("There is nothing written on it.");
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands().command("look", cmd_look).alias("l").category("Information").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("examine", cmd_examine)
        .alias("exa")
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands().command("who", cmd_who).category("Information").privilege(PrivilegeLevel::Player).build();

    Commands()
        .command("users", cmd_users)
        .category("Information")
        .privilege(PrivilegeLevel::Helper)
        .help("Show all connections with details (immortal only)")
        .build();

    Commands().command("where", cmd_where).category("Information").privilege(PrivilegeLevel::Player).build();

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

    Commands().command("time", cmd_time).category("Information").privilege(PrivilegeLevel::Player).build();

    Commands().command("weather", cmd_weather).category("Information").privilege(PrivilegeLevel::Player).build();

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

    Commands().command("glance", cmd_glance).category("Information").privilege(PrivilegeLevel::Player).build();

    // Scanning Commands
    Commands()
        .command("scan", cmd_scan)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .description("Scan adjacent rooms for creatures")
        .build();

    // Board Commands
    Commands()
        .command("board", cmd_board)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .description("Interact with bulletin boards")
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

    // Search and discovery
    Commands()
        .command("search", cmd_search)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .description("Search for hidden objects or exits")
        .usable_in_combat(false)
        .build();

    Commands()
        .command("read", cmd_read)
        .category("Information")
        .privilege(PrivilegeLevel::Player)
        .description("Read something")
        .usage("read <object>")
        .build();

    return Success();
}

} // namespace InformationCommands

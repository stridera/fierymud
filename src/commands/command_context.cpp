#include "command_context.hpp"

#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../core/object.hpp"
#include "../server/world_server.hpp"
#include "../world/room.hpp"
#include "../world/world_manager.hpp"
#include "../text/rich_text.hpp"
#include "../text/string_utils.hpp"
#include "../text/terminal_capabilities.hpp"
#include "command_parser.hpp"
#include "command_system.hpp"

#include <algorithm>
#include <cctype>
#include <fmt/format.h>
#include <iostream>

using namespace std::string_view_literals;

//////////////////////////////////////////////////////////////////////////////
// TargetInfo Implementation
//////////////////////////////////////////////////////////////////////////////

std::string TargetInfo::describe() const {
    switch (type) {
    case TargetType::None:
        return "no target";
    case TargetType::Self:
        return "yourself";
    case TargetType::Actor:
        return actor ? std::string{actor->name()} : "unknown actor";
    case TargetType::Object:
        return object ? std::string{object->name()} : "unknown object";
    case TargetType::Room:
        return room ? fmt::format("room '{}'", room->name()) : "unknown room";
    case TargetType::Direction:
        return fmt::format("direction {}", CommandContextUtils::direction_to_string(direction));
    case TargetType::String:
        return fmt::format("string '{}'", string_value);
    case TargetType::Number:
        return fmt::format("number {}", numeric_value);
    case TargetType::Multiple:
        return fmt::format("{} targets", multiple_targets.size());
    default:
        return "unknown target type";
    }
}

//////////////////////////////////////////////////////////////////////////////
// Targeting Helper - Parse "N.keyword" numbered targeting syntax
//////////////////////////////////////////////////////////////////////////////

namespace {

/**
 * Parse numbered targeting syntax like "2.guard" or "all.sword"
 * Returns: (keyword, match_index, match_all)
 *   - match_index is 1-indexed (1 = first match)
 *   - match_all is true for "all.keyword" syntax
 */
struct TargetSpec {
    std::string keyword;
    int match_index = 1;
    bool match_all = false;

    static TargetSpec parse(std::string_view name) {
        TargetSpec spec;

        size_t dot_pos = name.find('.');
        if (dot_pos != std::string_view::npos) {
            std::string_view prefix = name.substr(0, dot_pos);
            spec.keyword = std::string{name.substr(dot_pos + 1)};

            // Check for "all" prefix
            if (prefix == "all") {
                spec.match_all = true;
                spec.match_index = 1;
            } else {
                // Try to parse prefix as number
                try {
                    spec.match_index = std::stoi(std::string{prefix});
                    if (spec.match_index < 1) {
                        spec.match_index = 1;
                    }
                } catch (...) {
                    // Not a number, treat whole thing as keyword
                    spec.keyword = std::string{name};
                    spec.match_index = 1;
                }
            }
        } else {
            spec.keyword = std::string{name};
        }

        return spec;
    }
};

}  // namespace

//////////////////////////////////////////////////////////////////////////////
// CommandExecutionState Implementation
//////////////////////////////////////////////////////////////////////////////

std::vector<std::string> CommandExecutionState::keys() const {
    std::vector<std::string> result;
    result.reserve(data_.size());
    for (const auto &[key, value] : data_) {
        result.push_back(key);
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////
// CommandContext Methods Implementation
//////////////////////////////////////////////////////////////////////////////

void CommandContext::send(std::string_view message) const {
    if (actor) {
        actor->send_message(message);
    }
}

void CommandContext::send_line(std::string_view message) const {
    if (actor) {
        fmt::print("CommandContext::send_line: {}\n", message);
        actor->send_message(fmt::format("{}\n", message));
    }
}

void CommandContext::send_error(std::string_view message) const {
    if (actor) {
        auto formatted = CommandContextUtils::format_message(message, MessageType::Error);
        actor->send_message(formatted);
    }
}

void CommandContext::send_success(std::string_view message) const {
    if (actor) {
        auto formatted = CommandContextUtils::format_message(message, MessageType::Success);
        actor->send_message(formatted);
    }
}

void CommandContext::send_info(std::string_view message) const {
    if (actor) {
        auto formatted = CommandContextUtils::format_message(message, MessageType::Info);
        actor->send_message(formatted);
    }
}

void CommandContext::send_usage(std::string_view usage) const {
    if (actor) {
        auto formatted = fmt::format("Usage: {}", usage);
        send_error(formatted);
    }
}

void CommandContext::show_help() const {
    if (!actor) return;

    // Look up the command in the system
    auto& cmd_system = CommandSystem::instance();
    const CommandInfo* cmd_info = cmd_system.find_command(command.command);

    if (!cmd_info) {
        send_error("No help available for this command.");
        return;
    }

    // Display command name as header
    send(fmt::format("<b:cyan>Help: {}</>", cmd_info->name));
    send(std::string(6 + cmd_info->name.length(), '='));

    // Show description if available
    if (!cmd_info->description.empty()) {
        send(fmt::format("\n{}", cmd_info->description));
    }

    // Show usage if available
    if (!cmd_info->usage.empty()) {
        send(fmt::format("\n<b:yellow>Usage:</> {}", cmd_info->usage));
    }

    // Show extended help if available
    if (!cmd_info->help_text.empty()) {
        send(fmt::format("\n{}", cmd_info->help_text));
    }

    // Show aliases if any
    if (!cmd_info->aliases.empty()) {
        std::string alias_str;
        for (size_t i = 0; i < cmd_info->aliases.size(); ++i) {
            if (i > 0) alias_str += ", ";
            alias_str += cmd_info->aliases[i];
        }
        send(fmt::format("\n<dim>Aliases: {}</>", alias_str));
    }

    // If no help content at all, indicate that
    if (cmd_info->description.empty() && cmd_info->usage.empty() && cmd_info->help_text.empty()) {
        send("\nNo detailed help available for this command yet.");
    }
}

void CommandContext::send_to_room(std::string_view message, bool exclude_self) const {
    if (!room)
        return;

    // Send message to all actors in the room
    const auto &actors = room->contents().actors;
    for (const auto &room_actor : actors) {
        if (!room_actor)
            continue;

        // Skip self if exclude_self is true
        if (exclude_self && room_actor == actor) {
            continue;
        }

        room_actor->send_message(message);
    }
}

void CommandContext::send_to_actor(std::shared_ptr<Actor> target, std::string_view message) const {
    if (target) {
        target->send_message(message);
    }
}

void CommandContext::send_to_all(std::string_view message) const {
    // Broadcast to all online players using WorldServer's player tracking
    if (auto *world_server = WorldServer::instance()) {
        auto online_actors = world_server->get_online_actors();
        for (const auto &online_actor : online_actors) {
            if (online_actor) {
                online_actor->send_message(message);
            }
        }
    }
}

std::shared_ptr<Actor> CommandContext::find_actor_target(std::string_view name) const {
    if (!room)
        return nullptr;

    // Check for self-targeting keywords
    if (name == "self" || name == "me" || name == "myself") {
        return actor;
    }

    // Parse dot-notation: "N.keyword" for Nth match, or just "keyword" for first match
    auto spec = TargetSpec::parse(name);
    if (spec.keyword.empty()) {
        return nullptr;
    }

    // Count matches and return the Nth one
    int current_match = 0;

    // Search for actor by keyword in current room
    const auto &actors = room->contents().actors;
    for (const auto &room_actor : actors) {
        if (!room_actor || room_actor == actor)
            continue; // Skip self in normal search

        // Check visibility - can't target what you can't see
        if (!room_actor->is_visible_to(*actor))
            continue;

        if (room_actor->matches_keyword_prefix(spec.keyword)) {
            current_match++;
            if (current_match == spec.match_index) {
                return room_actor;
            }
        }
    }

    return nullptr;
}

std::shared_ptr<Actor> CommandContext::find_actor_global(std::string_view name) const {
    // Parse dot-notation: "N.keyword" for Nth match, or just "keyword" for first match
    auto spec = TargetSpec::parse(name);
    if (spec.keyword.empty()) {
        return nullptr;
    }

    // Count matches and return the Nth one
    int current_match = 0;

    // Search for online actors using the WorldServer's player tracking
    if (auto *world_server = WorldServer::instance()) {
        auto online_actors = world_server->get_online_actors();
        for (const auto &online_actor : online_actors) {
            if (!online_actor || online_actor == actor)
                continue; // Skip self

            if (online_actor->matches_keyword_prefix(spec.keyword)) {
                current_match++;
                if (current_match == spec.match_index) {
                    return online_actor;
                }
            }
        }
    }

    return nullptr;
}

std::shared_ptr<Object> CommandContext::find_object_target(std::string_view name) const {
    if (!actor)
        return nullptr;

    // Parse dot-notation: "N.keyword" for Nth match, or just "keyword" for first match
    auto spec = TargetSpec::parse(name);
    if (spec.keyword.empty()) {
        return nullptr;
    }

    // Count matches and return the Nth one
    int current_match = 0;

    // First search in actor's equipped items (prioritize what you're holding/wearing)
    auto equipped_items = actor->equipment().get_all_equipped();
    for (const auto &obj : equipped_items) {
        if (!obj)
            continue;

        if (obj->matches_keyword_prefix(spec.keyword)) {
            current_match++;
            if (current_match == spec.match_index) {
                return obj;
            }
        }
    }

    // Then search in actor's inventory
    auto inventory_items = actor->inventory().get_all_items();
    for (const auto &obj : inventory_items) {
        if (!obj)
            continue;

        if (obj->matches_keyword_prefix(spec.keyword)) {
            current_match++;
            if (current_match == spec.match_index) {
                return obj;
            }
        }
    }

    // Finally search in current room
    if (room) {
        const auto &room_objects = room->contents().objects;
        for (const auto &obj : room_objects) {
            if (!obj)
                continue;

            if (obj->matches_keyword_prefix(spec.keyword)) {
                current_match++;
                if (current_match == spec.match_index) {
                    return obj;
                }
            }
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Object>> CommandContext::find_objects_matching(
    std::string_view name, bool search_room) const {
    std::vector<std::shared_ptr<Object>> results;

    if (!actor)
        return results;

    // Parse dot-notation: "all.keyword", "N.keyword", or just "keyword"
    std::string keyword;
    bool match_all = false;
    int match_index = 0;  // 0 means first match (default), N means Nth match

    size_t dot_pos = name.find('.');
    if (dot_pos != std::string_view::npos) {
        std::string_view prefix = name.substr(0, dot_pos);
        keyword = std::string{name.substr(dot_pos + 1)};

        if (prefix == "all") {
            match_all = true;
        } else {
            // Try to parse as number
            try {
                match_index = std::stoi(std::string{prefix});
                if (match_index < 1) {
                    match_index = 1;  // Minimum is 1 (1-indexed)
                }
            } catch (...) {
                // Not a number, treat whole thing as keyword
                keyword = std::string{name};
            }
        }
    } else {
        keyword = std::string{name};
    }

    if (keyword.empty()) {
        return results;
    }

    // Collect all matching objects (equipped first, then inventory)
    std::vector<std::shared_ptr<Object>> all_matches;

    // Search equipped items first (prioritize what you're holding/wearing)
    for (const auto &obj : actor->equipment().get_all_equipped()) {
        if (!obj)
            continue;
        if (obj->matches_keyword_prefix(keyword)) {
            all_matches.push_back(obj);
        }
    }

    // Then search inventory
    for (const auto &obj : actor->inventory().get_all_items()) {
        if (!obj)
            continue;
        if (obj->matches_keyword_prefix(keyword)) {
            all_matches.push_back(obj);
        }
    }

    // Optionally search room
    if (search_room && room) {
        for (const auto &obj : room->contents().objects) {
            if (!obj)
                continue;
            if (obj->matches_keyword_prefix(keyword)) {
                all_matches.push_back(obj);
            }
        }
    }

    // Apply selection logic
    if (match_all) {
        // Return all matching objects
        results = std::move(all_matches);
    } else if (match_index > 0) {
        // Return Nth matching object (1-indexed)
        if (static_cast<size_t>(match_index) <= all_matches.size()) {
            results.push_back(all_matches[match_index - 1]);
        }
    } else {
        // Return first matching object (default)
        if (!all_matches.empty()) {
            results.push_back(all_matches[0]);
        }
    }

    return results;
}

std::shared_ptr<Room> CommandContext::find_room_target(std::string_view name) const {
    if (name.empty()) {
        return nullptr;
    }

    // Try to parse as room ID first
    auto room_id_opt = CommandParserUtils::parse_entity_id(name);
    if (room_id_opt && room_id_opt->is_valid()) {
        return WorldManager::instance().get_room(*room_id_opt);
    }

    // Try to find by keyword
    auto rooms = WorldManager::instance().find_rooms_by_keyword(name);
    return rooms.empty() ? nullptr : rooms[0];
}

TargetInfo CommandContext::resolve_target(std::string_view name) const {
    if (name.empty()) {
        return TargetInfo{};
    }

    // Handle special keywords
    if (name == "self" || name == "me") {
        TargetInfo info;
        info.type = TargetType::Self;
        info.actor = actor;
        return info;
    }

    // Check if the name matches the current actor (allows "stat samui" to work on yourself)
    if (actor && actor->matches_keyword_prefix(name)) {
        TargetInfo info;
        info.type = TargetType::Self;
        info.actor = actor;
        return info;
    }

    if (name == "room" || name == "here") {
        TargetInfo info;
        info.type = TargetType::Room;
        info.room = room;
        return info;
    }

    // Try to parse as direction
    if (CommandContextUtils::is_direction_string(name)) {
        TargetInfo info;
        info.type = TargetType::Direction;
        info.direction = CommandContextUtils::parse_direction_string(name);
        return info;
    }

    // Try to parse as number
    if (CommandContextUtils::is_numeric_string(name)) {
        TargetInfo info;
        info.type = TargetType::Number;
        try {
            info.numeric_value = std::stoi(std::string{name});
        } catch (const std::exception &) {
            info.numeric_value = 0;
        }
        return info;
    }

    // Try to find actor in room
    if (auto found_actor = find_actor_target(name)) {
        TargetInfo info;
        info.type = TargetType::Actor;
        info.actor = found_actor;
        return info;
    }

    // Try to find object in room/inventory
    if (auto found_object = find_object_target(name)) {
        TargetInfo info;
        info.type = TargetType::Object;
        info.object = found_object;
        return info;
    }

    // Try to find room by name
    if (auto found_room = find_room_target(name)) {
        TargetInfo info;
        info.type = TargetType::Room;
        info.room = found_room;
        return info;
    }

    // Default to string target
    TargetInfo info;
    info.type = TargetType::String;
    info.string_value = std::string{name};
    return info;
}

std::string CommandContext::format_object_name(std::shared_ptr<Object> obj) const {
    if (!obj)
        return "nothing";

    // Use short_description for user-facing display, not name (which is for keywords)
    return std::string{obj->short_description()};
}

Result<void> CommandContext::move_actor_direction(Direction dir) const {
    if (!actor || !room) {
        return std::unexpected(Error{ErrorCode::InvalidState, "No actor or room for movement"});
    }

    // Get the WorldManager instance
    auto &world_manager = WorldManager::instance();

    // Use WorldManager's movement system to move the actor
    auto movement_result = world_manager.move_actor(actor, dir);

    if (!movement_result.success) {
        return std::unexpected(Error{ErrorCode::InvalidState, movement_result.failure_reason});
    }

    // Success - movement messages are handled by WorldManager callbacks
    return Result<void>{};
}

//////////////////////////////////////////////////////////////////////////////
// Enhanced Act-Style Messaging Methods (inspired by legacy system)
//////////////////////////////////////////////////////////////////////////////

void CommandContext::act_to_char(std::string_view message) const {
    if (!actor)
        return;

    std::string substituted = substitute_variables(message);
    send(substituted);
}

void CommandContext::act_to_room(std::string_view message, bool exclude_actor) const {
    if (!room)
        return;

    std::string substituted = substitute_variables(message);
    send_to_room(substituted, exclude_actor);
}

void CommandContext::act_to_target(std::shared_ptr<Actor> target, std::string_view message) const {
    if (!target)
        return;

    std::string substituted = substitute_variables(message, target);
    send_to_actor(target, substituted);
}

void CommandContext::act(std::string_view message, std::shared_ptr<Actor> target, ActTarget type) const {
    switch (type) {
    case ActTarget::ToChar:
        act_to_char(message);
        break;
    case ActTarget::ToTarget:
        if (target) {
            act_to_target(target, message);
        }
        break;
    case ActTarget::ToRoom:
        act_to_room(message, true);
        break;
    case ActTarget::ToAll:
    default:
        // Send to all relevant parties (legacy-style behavior)
        act_to_char(message);
        if (target) {
            act_to_target(target, message);
        }
        act_to_room(message, true);
        break;
    }
}

std::string CommandContext::substitute_variables(std::string_view message, std::shared_ptr<Actor> target) const {
    std::string result{message};

    // Actor substitutions ($n patterns)
    if (actor) {
        // Replace $n with actor name
        size_t pos = 0;
        while ((pos = result.find("$n", pos)) != std::string::npos) {
            result.replace(pos, 2, actor->display_name());
            pos += actor->display_name().length();
        }

        // Replace $s with actor possessive (simplified for now)
        pos = 0;
        while ((pos = result.find("$s", pos)) != std::string::npos) {
            result.replace(pos, 2, "their"); // Gender-neutral default
            pos += 5;
        }
    }

    // Target substitutions ($N patterns)
    if (target) {
        // Replace $N with target name
        size_t pos = 0;
        while ((pos = result.find("$N", pos)) != std::string::npos) {
            result.replace(pos, 2, target->display_name());
            pos += target->display_name().length();
        }

        // Replace $S with target possessive (simplified for now)
        pos = 0;
        while ((pos = result.find("$S", pos)) != std::string::npos) {
            result.replace(pos, 2, "their"); // Gender-neutral default
            pos += 5;
        }
    }

    return result;
}

Result<CommandResult> CommandContext::execute_social(const SocialMessage &social, std::string_view target_name) const {
    if (!actor) {
        return std::unexpected(Error{ErrorCode::InvalidState, "No actor context"});
    }

    if (target_name.empty()) {
        // No target - use simple messages
        act_to_char(social.to_actor_no_arg);
        act_to_room(social.to_room_no_arg);
    } else {
        // Find target
        auto target = find_actor_target(target_name);
        if (!target) {
            if (!social.target_not_found.empty()) {
                send_error(social.target_not_found);
            } else {
                send_error(fmt::format("'{}' is not here.", target_name));
            }
            return CommandResult::InvalidTarget;
        }

        // Send targeted messages
        act_to_char(substitute_variables(social.to_actor_with_target, target));
        act_to_target(target, substitute_variables(social.to_target, target));
        act_to_room(substitute_variables(social.to_room_with_target, target));
    }

    return CommandResult::Success;
}

//////////////////////////////////////////////////////////////////////////////
// CommandContextUtils Implementation
//////////////////////////////////////////////////////////////////////////////

namespace CommandContextUtils {

std::vector<std::string> parse_target_list(std::string_view target_string) {
    std::vector<std::string> targets;

    // Simple comma-separated parsing for now
    std::string str{target_string};
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;

    while ((pos = str.find(delimiter)) != std::string::npos) {
        token = std::string(trim(str.substr(0, pos)));
        if (!token.empty()) {
            targets.push_back(token);
        }
        str.erase(0, pos + delimiter.length());
    }

    // Add the last token
    std::string last_token{trim(str)};
    if (!last_token.empty()) {
        targets.push_back(last_token);
    }

    return targets;
}

std::string describe_target(const TargetInfo &target) { return target.describe(); }

bool is_direction_string(std::string_view str) {
    std::string lower_str = to_lowercase(str);

    return lower_str == "north" || lower_str == "n" || lower_str == "south" || lower_str == "s" ||
           lower_str == "east" || lower_str == "e" || lower_str == "west" || lower_str == "w" || lower_str == "up" ||
           lower_str == "u" || lower_str == "down" || lower_str == "d" || lower_str == "northeast" ||
           lower_str == "ne" || lower_str == "northwest" || lower_str == "nw" || lower_str == "southeast" ||
           lower_str == "se" || lower_str == "southwest" || lower_str == "sw";
}

bool is_numeric_string(std::string_view str) {
    if (str.empty())
        return false;

    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
        if (str.size() == 1)
            return false; // Just a sign
    }

    for (size_t i = start; i < str.size(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }

    return true;
}

Direction parse_direction_string(std::string_view str) {
    std::string lower_str = to_lowercase(str);

    if (lower_str == "north" || lower_str == "n")
        return Direction::North;
    if (lower_str == "south" || lower_str == "s")
        return Direction::South;
    if (lower_str == "east" || lower_str == "e")
        return Direction::East;
    if (lower_str == "west" || lower_str == "w")
        return Direction::West;
    if (lower_str == "up" || lower_str == "u")
        return Direction::Up;
    if (lower_str == "down" || lower_str == "d")
        return Direction::Down;
    if (lower_str == "northeast" || lower_str == "ne")
        return Direction::Northeast;
    if (lower_str == "northwest" || lower_str == "nw")
        return Direction::Northwest;
    if (lower_str == "southeast" || lower_str == "se")
        return Direction::Southeast;
    if (lower_str == "southwest" || lower_str == "sw")
        return Direction::Southwest;

    return Direction::None;
}

std::string_view direction_to_string(Direction dir) {
    switch (dir) {
    case Direction::North:
        return "north"sv;
    case Direction::South:
        return "south"sv;
    case Direction::East:
        return "east"sv;
    case Direction::West:
        return "west"sv;
    case Direction::Up:
        return "up"sv;
    case Direction::Down:
        return "down"sv;
    case Direction::Northeast:
        return "northeast"sv;
    case Direction::Northwest:
        return "northwest"sv;
    case Direction::Southeast:
        return "southeast"sv;
    case Direction::Southwest:
        return "southwest"sv;
    case Direction::None:
    default:
        return "none"sv;
    }
}

std::string_view direction_to_preposition(Direction dir) {
    switch (dir) {
    case Direction::North:
        return "to the north"sv;
    case Direction::South:
        return "to the south"sv;
    case Direction::East:
        return "to the east"sv;
    case Direction::West:
        return "to the west"sv;
    case Direction::Up:
        return "above"sv;
    case Direction::Down:
        return "below"sv;
    case Direction::Northeast:
        return "to the northeast"sv;
    case Direction::Northwest:
        return "to the northwest"sv;
    case Direction::Southeast:
        return "to the southeast"sv;
    case Direction::Southwest:
        return "to the southwest"sv;
    case Direction::None:
    default:
        return "nowhere"sv;
    }
}

Direction reverse_direction(Direction dir) {
    switch (dir) {
    case Direction::North:
        return Direction::South;
    case Direction::South:
        return Direction::North;
    case Direction::East:
        return Direction::West;
    case Direction::West:
        return Direction::East;
    case Direction::Up:
        return Direction::Down;
    case Direction::Down:
        return Direction::Up;
    case Direction::Northeast:
        return Direction::Southwest;
    case Direction::Northwest:
        return Direction::Southeast;
    case Direction::Southeast:
        return Direction::Northwest;
    case Direction::Southwest:
        return Direction::Northeast;
    case Direction::None:
    default:
        return Direction::None;
    }
}

std::string format_message(std::string_view message, MessageType type) {
    auto color_code = get_color_code(type);
    if (color_code.empty()) {
        return std::string{message};
    }
    return fmt::format("{}{}\033[0m", color_code, message);
}

std::string strip_color_codes(std::string_view message) {
    std::string result;
    result.reserve(message.size());

    bool in_escape = false;
    for (char c : message) {
        if (c == '\033') {
            in_escape = true;
        } else if (in_escape && c == 'm') {
            in_escape = false;
        } else if (!in_escape) {
            result.push_back(c);
        }
    }

    return result;
}

std::string_view get_color_code(MessageType type) {
    switch (type) {
    case MessageType::Error:
        return "\033[31m"sv; // Red
    case MessageType::Success:
        return "\033[32m"sv; // Green
    case MessageType::Warning:
        return "\033[33m"sv; // Yellow
    case MessageType::Info:
        return "\033[34m"sv; // Blue
    case MessageType::System:
        return "\033[36m"sv; // Cyan
    case MessageType::Debug:
        return "\033[37m"sv; // Gray
    case MessageType::Combat:
        return "\033[35m"sv; // Magenta
    case MessageType::Social:
        return "\033[33m"sv; // Yellow
    case MessageType::Tell:
        return "\033[36m"sv; // Cyan
    case MessageType::Say:
        return "\033[37m"sv; // White
    case MessageType::Emote:
        return "\033[33m"sv; // Yellow
    case MessageType::Channel:
        return "\033[35m"sv; // Magenta
    case MessageType::Broadcast:
        return "\033[31m"sv; // Red
    case MessageType::Normal:
    default:
        return ""sv;
    }
}

} // namespace CommandContextUtils

//////////////////////////////////////////////////////////////////////////////
// CommandContext Rich Text Methods Implementation
//////////////////////////////////////////////////////////////////////////////

void CommandContext::send_rich(const RichText &rich_text) const {
    if (actor) {
        actor->send_message(rich_text.to_ansi());
    }
}

void CommandContext::send_colored(std::string_view message, Color color) const {
    if (actor) {
        RichText rich;
        rich.colored(message, color);
        actor->send_message(rich.to_ansi());
    }
}

void CommandContext::send_progress_bar(std::string_view label, float percentage, int width) const {
    if (actor) {
        // Detect capabilities for adaptive progress bars
        auto caps = TerminalCapabilities::detect_capabilities();
        RichText rich(caps);
        rich.text(label).text(": ");
        rich.progress_bar(percentage, width); // Uses adaptive characters
        rich.text(fmt::format(" {:.1f}%", percentage * 100.0f));
        actor->send_message(rich.to_ansi());
    }
}

void CommandContext::send_table(const std::vector<std::string> &headers,
                                const std::vector<std::vector<std::string>> &rows) const {
    if (actor) {
        // Detect terminal capabilities for adaptive formatting
        auto caps = TerminalCapabilities::detect_capabilities();
        auto table = Format::table(headers, rows, caps);
        actor->send_message(table.to_ansi());
    }
}

void CommandContext::send_health_status(int current, int max) const {
    if (actor) {
        // Use adaptive formatting for health bar
        auto caps = TerminalCapabilities::detect_capabilities();
        RichText health_bar(caps);

        float percentage = (max > 0) ? static_cast<float>(current) / max : 0.0f;
        health_bar.text("Health: ");
        health_bar.progress_bar(percentage, 20); // Uses adaptive characters
        health_bar.text(fmt::format(" {}/{}", current, max));

        actor->send_message(health_bar.to_ansi());
    }
}

void CommandContext::send_damage_report(int amount, std::string_view source) const {
    if (actor) {
        RichText msg;
        msg.text("You deal ");
        msg.rgb(std::to_string(amount), Colors::Damage);
        msg.text(" damage");

        if (!source.empty()) {
            msg.text(" with your ");
            msg.text(source);
        }
        msg.text("!");

        actor->send_message(msg.to_ansi());
    }
}

void CommandContext::send_healing_report(int amount, std::string_view source) const {
    if (actor) {
        RichText msg;
        msg.text("You ");
        if (!source.empty()) {
            msg.text(source);
            msg.text(" for ");
        } else {
            msg.text("heal ");
        }
        msg.rgb("+" + std::to_string(amount), Colors::Healing);
        msg.text(" hit points!");

        actor->send_message(msg.to_ansi());
    }
}

void CommandContext::send_object_description(std::string_view name, std::string_view quality) const {
    if (actor) {
        auto object_display = Format::object_name(name, quality);
        actor->send_message(object_display.to_ansi());
    }
}

void CommandContext::send_header(std::string_view title, char border_char) const {
    if (actor) {
        RichText header;
        std::string border_line(60, border_char);

        header.colored(border_line, Colors::Border);
        header.text("\n");

        // Center the title
        int padding = (60 - static_cast<int>(title.length())) / 2;
        if (padding > 0) {
            header.text(std::string(padding, ' '));
        }

        header.bold(title);
        header.text("\n");
        header.colored(border_line, Colors::Border);

        actor->send_message(header.to_ansi());
    }
}

void CommandContext::send_separator(char ch, int width) const {
    if (actor) {
        RichText sep;
        sep.colored(std::string(width, ch), Colors::Border);
        actor->send_message(sep.to_ansi());
    }
}

void CommandContext::send_command_help(std::string_view command, std::string_view syntax) const {
    if (actor) {
        auto help_display = Format::command_syntax(command, syntax);
        actor->send_message(help_display.to_ansi());
    }
}
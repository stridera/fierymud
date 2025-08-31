/***************************************************************************
 *   File: src/commands/command_context.hpp               Part of FieryMUD *
 *  Usage: Command execution helper types and utilities                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "../core/result.hpp"
#include "../core/ids.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <any>
#include <span>
#include <fmt/format.h>

// Forward declaration for rich text system
class RichText;

// Forward declarations
class Actor;
class Object;
class Room;
class Entity;
struct ParsedCommand;
enum class PrivilegeLevel;

// Need to include the Direction enum from room.hpp since it's used in TargetInfo
// Forward declaration isn't sufficient for enum values
#include "../world/room.hpp"

/**
 * Helper types for command execution context.
 * 
 * The actual CommandContext is defined in command_system.hpp.
 * This file provides supporting enums and utility types.
 */

/** Message formatting and delivery options */
enum class MessageType {
    Normal,         // Standard output
    Error,          // Error messages (red text)
    Success,        // Success messages (green text)
    Warning,        // Warning messages (yellow text)
    Info,           // Informational messages (blue text)
    System,         // System messages (cyan text)
    Debug,          // Debug messages (gray text)
    Combat,         // Combat-related messages
    Social,         // Social action messages
    Tell,           // Private tell messages
    Say,            // Say/speech messages
    Emote,          // Emote/action messages
    Channel,        // Channel communication
    Broadcast       // System-wide broadcasts
};

/** Target types for command resolution */
enum class TargetType {
    None,           // No target
    Self,           // Target is the actor
    Actor,          // Another actor/player
    Object,         // An object
    Room,           // A room
    Direction,      // A direction
    String,         // Arbitrary string
    Number,         // Numeric value
    Multiple        // Multiple targets
};

/** Act message targeting (inspired by legacy system) */
enum class ActTarget {
    ToChar,         // Message to the actor performing the action
    ToTarget,       // Message to the target/victim of the action  
    ToRoom,         // Message to everyone else in the room
    ToAll           // All of the above (default behavior)
};

/** Social message template (inspired by legacy social_messg) */
struct SocialMessage {
    // No argument supplied
    std::string to_actor_no_arg;     // "You smile happily."
    std::string to_room_no_arg;      // "$n smiles happily."
    
    // Target found
    std::string to_actor_with_target;    // "You smile at $N."
    std::string to_target;               // "$n smiles at you."
    std::string to_room_with_target;     // "$n smiles at $N."
    
    // Target not found
    std::string target_not_found;       // "Smile at whom?"
    
    // Convenience constructor for simple socials
    SocialMessage(std::string_view action) {
        to_actor_no_arg = fmt::format("You {}.", action);
        to_room_no_arg = fmt::format("$n {}.", action);
        to_actor_with_target = fmt::format("You {} $N.", action);
        to_target = fmt::format("$n {} you.", action);
        to_room_with_target = fmt::format("$n {} $N.", action);
        target_not_found = fmt::format("{} whom?", action);
    }
    
    // Full constructor for custom messages
    SocialMessage(std::string_view actor_no_arg, std::string_view room_no_arg,
                 std::string_view actor_with_target, std::string_view target_msg,
                 std::string_view room_with_target, std::string_view not_found = "")
        : to_actor_no_arg(actor_no_arg), to_room_no_arg(room_no_arg)
        , to_actor_with_target(actor_with_target), to_target(target_msg)
        , to_room_with_target(room_with_target), target_not_found(not_found) {}
};

/** Target resolution result */
struct TargetInfo {
    TargetType type = TargetType::None;
    std::shared_ptr<Actor> actor;
    std::shared_ptr<Object> object;
    std::shared_ptr<Room> room;
    Direction direction = Direction::None;
    std::string string_value;
    int numeric_value = 0;
    std::vector<std::shared_ptr<Entity>> multiple_targets;
    
    bool is_valid() const { return type != TargetType::None; }
    std::string describe() const;
};

/** Command execution state and temporary data */
class CommandExecutionState {
public:
    /** Store arbitrary data during command execution */
    template<typename T>
    void set(std::string_view key, T&& value) {
        data_[std::string{key}] = std::forward<T>(value);
    }
    
    /** Retrieve stored data */
    template<typename T>
    std::optional<T> get(std::string_view key) const {
        auto it = data_.find(std::string{key});
        if (it == data_.end()) {
            return std::nullopt;
        }
        
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }
    
    /** Check if key exists */
    bool has(std::string_view key) const {
        return data_.contains(std::string{key});
    }
    
    /** Remove stored data */
    void remove(std::string_view key) {
        data_.erase(std::string{key});
    }
    
    /** Clear all data */
    void clear() {
        data_.clear();
    }
    
    /** Get all keys */
    std::vector<std::string> keys() const;
    
private:
    std::unordered_map<std::string, std::any> data_;
};

/** Utility functions for command context */
namespace CommandContextUtils {
    /** Parse multiple target names from string */
    std::vector<std::string> parse_target_list(std::string_view target_string);
    
    /** Format target description for output */
    std::string describe_target(const TargetInfo& target);
    
    /** Check if string is a valid direction */
    bool is_direction_string(std::string_view str);
    
    /** Check if string is a valid numeric value */
    bool is_numeric_string(std::string_view str);
    
    /** Parse direction from string */
    Direction parse_direction_string(std::string_view str);
    
    /** Convert Direction to string */
    std::string_view direction_to_string(Direction dir);
    
    /** Convert Direction to preposition string */
    std::string_view direction_to_preposition(Direction dir);
    
    /** Get opposite direction */
    Direction reverse_direction(Direction dir);
    
    /** Format message type with color codes */
    std::string format_message(std::string_view message, MessageType type);
    
    /** Strip color codes from message */
    std::string strip_color_codes(std::string_view message);
    
    /** Get color code for message type */
    std::string_view get_color_code(MessageType type);
}
#pragma once

#include <cstdint>
#include <string_view>
#include <magic_enum/magic_enum.hpp>

namespace FieryMUD {

/// Script attachment type - matches Prisma ScriptType enum
enum class ScriptType : std::uint8_t {
    MOB,    ///< Attached to a mobile/NPC
    OBJECT, ///< Attached to an object/item
    WORLD   ///< Attached to a room/zone
};

/// Trigger flags - matches Prisma TriggerFlag enum
/// These define when a trigger activates
enum class TriggerFlag : std::uint32_t {
    GLOBAL      = 1 << 0,   ///< Trigger is global (checked everywhere)
    RANDOM      = 1 << 1,   ///< Fires randomly on pulse
    COMMAND     = 1 << 2,   ///< Fires when a command is used
    SPEECH      = 1 << 3,   ///< Fires when speech is detected
    ACT         = 1 << 4,   ///< Fires on action messages
    DEATH       = 1 << 5,   ///< Fires when entity dies
    GREET       = 1 << 6,   ///< Fires when visible actor enters room
    GREET_ALL   = 1 << 7,   ///< Fires when any actor enters room
    ENTRY       = 1 << 8,   ///< Fires when mob enters a room
    RECEIVE     = 1 << 9,   ///< Fires when mob receives an object
    FIGHT       = 1 << 10,  ///< Fires each combat round
    HIT_PERCENT = 1 << 11,  ///< Fires when HP drops below threshold
    BRIBE       = 1 << 12,  ///< Fires when mob is given money
    LOAD        = 1 << 13,  ///< Fires when entity is loaded/spawned
    MEMORY      = 1 << 14,  ///< Fires when remembered actor is seen
    CAST        = 1 << 15,  ///< Fires when spell is cast
    LEAVE       = 1 << 16,  ///< Fires when actor leaves room
    DOOR        = 1 << 17,  ///< Fires on door interaction
    TIME        = 1 << 18,  ///< Fires at specific MUD time
    AUTO        = 1 << 19,  ///< Fires automatically on timer
    SPEECH_TO   = 1 << 20,  ///< Fires when directly addressed
    LOOK        = 1 << 21,  ///< Fires when entity is looked at
};

/// Enable bitwise operations on TriggerFlag
constexpr TriggerFlag operator|(TriggerFlag lhs, TriggerFlag rhs) {
    return static_cast<TriggerFlag>(
        static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr TriggerFlag operator&(TriggerFlag lhs, TriggerFlag rhs) {
    return static_cast<TriggerFlag>(
        static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr TriggerFlag& operator|=(TriggerFlag& lhs, TriggerFlag rhs) {
    lhs = lhs | rhs;
    return lhs;
}

constexpr bool has_flag(TriggerFlag flags, TriggerFlag check) {
    return (static_cast<std::uint32_t>(flags) &
            static_cast<std::uint32_t>(check)) != 0;
}

/// Convert string to TriggerFlag (case-insensitive)
constexpr std::optional<TriggerFlag> string_to_trigger_flag(std::string_view str) {
    return magic_enum::enum_cast<TriggerFlag>(str, magic_enum::case_insensitive);
}

/// Convert TriggerFlag to string
constexpr std::string_view trigger_flag_to_string(TriggerFlag flag) {
    return magic_enum::enum_name(flag);
}

/// Convert string to ScriptType (case-insensitive)
constexpr std::optional<ScriptType> string_to_script_type(std::string_view str) {
    return magic_enum::enum_cast<ScriptType>(str, magic_enum::case_insensitive);
}

/// Convert ScriptType to string
constexpr std::string_view script_type_to_string(ScriptType type) {
    return magic_enum::enum_name(type);
}

} // namespace FieryMUD

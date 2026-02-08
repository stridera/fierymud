#pragma once

#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

namespace FieryMUD {

/// Script attachment type - matches Prisma ScriptType enum
enum class ScriptType : std::uint8_t {
    MOB,    ///< Attached to a mobile/NPC
    OBJECT, ///< Attached to an object/item
    WORLD   ///< Attached to a room/zone
};

/// Trigger flags - matches Prisma TriggerFlag enum
/// These define when a trigger activates
/// Note: Flags have different meanings for MOB, OBJECT, and WORLD triggers
enum class TriggerFlag : std::uint32_t {
    // Common flags (all types)
    GLOBAL = 1 << 0,  ///< Trigger is global (checked everywhere)
    RANDOM = 1 << 1,  ///< Fires randomly on pulse
    COMMAND = 1 << 2, ///< Fires when a command is used
    LOAD = 1 << 3,    ///< Fires when entity is loaded/spawned
    CAST = 1 << 4,    ///< Fires when spell is cast
    LEAVE = 1 << 5,   ///< Fires when actor leaves room
    TIME = 1 << 6,    ///< Fires at specific MUD time

    // MOB-specific flags
    SPEECH = 1 << 7,       ///< Fires when speech is detected
    ACT = 1 << 8,          ///< Fires on action messages
    DEATH = 1 << 9,        ///< Fires when entity dies
    GREET = 1 << 10,       ///< Fires when visible actor enters room
    GREET_ALL = 1 << 11,   ///< Fires when any actor enters room
    ENTRY = 1 << 12,       ///< Fires when mob enters a room
    RECEIVE = 1 << 13,     ///< Fires when mob receives an object
    FIGHT = 1 << 14,       ///< Fires each combat round
    HIT_PERCENT = 1 << 15, ///< Fires when HP drops below threshold
    BRIBE = 1 << 16,       ///< Fires when mob is given money
    MEMORY = 1 << 17,      ///< Fires when remembered actor is seen
    DOOR = 1 << 18,        ///< Fires on door interaction
    SPEECH_TO = 1 << 19,   ///< Fires when directly addressed
    LOOK = 1 << 20,        ///< Fires when entity is looked at
    AUTO = 1 << 21,        ///< Legacy: fires automatically on timer

    // OBJECT-specific flags
    ATTACK = 1 << 22,   ///< Weapon triggers on attack
    DEFEND = 1 << 23,   ///< Weapon triggers on defense
    TIMER = 1 << 24,    ///< Item timer expires
    GET = 1 << 25,      ///< Item picked up
    DROP = 1 << 26,     ///< Character tries to drop
    GIVE = 1 << 27,     ///< Character tries to give
    WEAR = 1 << 28,     ///< Character tries to wear
    REMOVE = 1 << 29,   ///< Character tries to remove
    USE = 1 << 30,      ///< Object is used
    CONSUME = 1u << 31, ///< Character eats/drinks object (use 1u for bit 31)

    // WORLD-specific flags (reuse some bits since types are mutually exclusive)
    RESET = 1 << 22,     ///< Zone has been reset (shares with ATTACK)
    PREENTRY = 1 << 23,  ///< Someone about to enter room (shares with DEFEND)
    POSTENTRY = 1 << 24, ///< Someone just entered room (shares with TIMER)
};

/// Enable bitwise operations on TriggerFlag
constexpr TriggerFlag operator|(TriggerFlag lhs, TriggerFlag rhs) {
    return static_cast<TriggerFlag>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr TriggerFlag operator&(TriggerFlag lhs, TriggerFlag rhs) {
    return static_cast<TriggerFlag>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr TriggerFlag &operator|=(TriggerFlag &lhs, TriggerFlag rhs) {
    lhs = lhs | rhs;
    return lhs;
}

constexpr bool has_flag(TriggerFlag flags, TriggerFlag check) {
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(check)) != 0;
}

/// Convert string to TriggerFlag (case-insensitive)
/// Note: magic_enum doesn't work with bitfield enums (values outside -128 to 256 range)
inline std::optional<TriggerFlag> string_to_trigger_flag(std::string_view str) {
    // Common flags
    if (str == "GLOBAL" || str == "global")
        return TriggerFlag::GLOBAL;
    if (str == "RANDOM" || str == "random")
        return TriggerFlag::RANDOM;
    if (str == "COMMAND" || str == "command")
        return TriggerFlag::COMMAND;
    if (str == "LOAD" || str == "load")
        return TriggerFlag::LOAD;
    if (str == "CAST" || str == "cast")
        return TriggerFlag::CAST;
    if (str == "LEAVE" || str == "leave")
        return TriggerFlag::LEAVE;
    if (str == "TIME" || str == "time")
        return TriggerFlag::TIME;
    // MOB-specific flags
    if (str == "SPEECH" || str == "speech")
        return TriggerFlag::SPEECH;
    if (str == "ACT" || str == "act")
        return TriggerFlag::ACT;
    if (str == "DEATH" || str == "death")
        return TriggerFlag::DEATH;
    if (str == "GREET" || str == "greet")
        return TriggerFlag::GREET;
    if (str == "GREET_ALL" || str == "greet_all")
        return TriggerFlag::GREET_ALL;
    if (str == "ENTRY" || str == "entry")
        return TriggerFlag::ENTRY;
    if (str == "RECEIVE" || str == "receive")
        return TriggerFlag::RECEIVE;
    if (str == "FIGHT" || str == "fight")
        return TriggerFlag::FIGHT;
    if (str == "HIT_PERCENT" || str == "hit_percent")
        return TriggerFlag::HIT_PERCENT;
    if (str == "BRIBE" || str == "bribe")
        return TriggerFlag::BRIBE;
    if (str == "MEMORY" || str == "memory")
        return TriggerFlag::MEMORY;
    if (str == "DOOR" || str == "door")
        return TriggerFlag::DOOR;
    if (str == "SPEECH_TO" || str == "speech_to")
        return TriggerFlag::SPEECH_TO;
    if (str == "LOOK" || str == "look")
        return TriggerFlag::LOOK;
    if (str == "AUTO" || str == "auto")
        return TriggerFlag::AUTO;
    // OBJECT-specific flags
    if (str == "ATTACK" || str == "attack")
        return TriggerFlag::ATTACK;
    if (str == "DEFEND" || str == "defend")
        return TriggerFlag::DEFEND;
    if (str == "TIMER" || str == "timer")
        return TriggerFlag::TIMER;
    if (str == "GET" || str == "get")
        return TriggerFlag::GET;
    if (str == "DROP" || str == "drop")
        return TriggerFlag::DROP;
    if (str == "GIVE" || str == "give")
        return TriggerFlag::GIVE;
    if (str == "WEAR" || str == "wear")
        return TriggerFlag::WEAR;
    if (str == "REMOVE" || str == "remove")
        return TriggerFlag::REMOVE;
    if (str == "USE" || str == "use")
        return TriggerFlag::USE;
    if (str == "CONSUME" || str == "consume")
        return TriggerFlag::CONSUME;
    // WORLD-specific flags
    if (str == "RESET" || str == "reset")
        return TriggerFlag::RESET;
    if (str == "PREENTRY" || str == "preentry")
        return TriggerFlag::PREENTRY;
    if (str == "POSTENTRY" || str == "postentry")
        return TriggerFlag::POSTENTRY;
    return std::nullopt;
}

/// Convert TriggerFlag to string
/// Note: magic_enum doesn't work with bitfield enums (values outside -128 to 256 range)
inline std::string_view trigger_flag_to_string(TriggerFlag flag) {
    switch (flag) {
    // Common flags
    case TriggerFlag::GLOBAL:
        return "GLOBAL";
    case TriggerFlag::RANDOM:
        return "RANDOM";
    case TriggerFlag::COMMAND:
        return "COMMAND";
    case TriggerFlag::LOAD:
        return "LOAD";
    case TriggerFlag::CAST:
        return "CAST";
    case TriggerFlag::LEAVE:
        return "LEAVE";
    case TriggerFlag::TIME:
        return "TIME";
    // MOB-specific flags
    case TriggerFlag::SPEECH:
        return "SPEECH";
    case TriggerFlag::ACT:
        return "ACT";
    case TriggerFlag::DEATH:
        return "DEATH";
    case TriggerFlag::GREET:
        return "GREET";
    case TriggerFlag::GREET_ALL:
        return "GREET_ALL";
    case TriggerFlag::ENTRY:
        return "ENTRY";
    case TriggerFlag::RECEIVE:
        return "RECEIVE";
    case TriggerFlag::FIGHT:
        return "FIGHT";
    case TriggerFlag::HIT_PERCENT:
        return "HIT_PERCENT";
    case TriggerFlag::BRIBE:
        return "BRIBE";
    case TriggerFlag::MEMORY:
        return "MEMORY";
    case TriggerFlag::DOOR:
        return "DOOR";
    case TriggerFlag::SPEECH_TO:
        return "SPEECH_TO";
    case TriggerFlag::LOOK:
        return "LOOK";
    case TriggerFlag::AUTO:
        return "AUTO";
    // OBJECT-specific flags (note: RESET/PREENTRY/POSTENTRY share bit positions)
    case TriggerFlag::ATTACK:
        return "ATTACK";
    case TriggerFlag::DEFEND:
        return "DEFEND";
    case TriggerFlag::TIMER:
        return "TIMER";
    case TriggerFlag::GET:
        return "GET";
    case TriggerFlag::DROP:
        return "DROP";
    case TriggerFlag::GIVE:
        return "GIVE";
    case TriggerFlag::WEAR:
        return "WEAR";
    case TriggerFlag::REMOVE:
        return "REMOVE";
    case TriggerFlag::USE:
        return "USE";
    case TriggerFlag::CONSUME:
        return "CONSUME";
    default:
        return "";
    }
}

/// Convert string to ScriptType (case-insensitive)
constexpr std::optional<ScriptType> string_to_script_type(std::string_view str) {
    return magic_enum::enum_cast<ScriptType>(str, magic_enum::case_insensitive);
}

/// Convert ScriptType to string
constexpr std::string_view script_type_to_string(ScriptType type) { return magic_enum::enum_name(type); }

} // namespace FieryMUD

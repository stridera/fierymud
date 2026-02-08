#pragma once

#include <cstdint>

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

} // namespace FieryMUD

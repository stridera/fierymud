#pragma once

#include <string>

/**
 * Environmental effect attached to a room, loaded from
 * the RoomEnvironmentalEffect junction table joined with Effect.
 *
 * Used by both the Room class and WorldQueries.
 */
struct RoomEnvEffect {
    int effect_id = 0;
    std::string name;
    std::string description;   // Colored text for room display
    std::string effect_type;   // "damage", "heal", "status", etc.
    std::string on_apply;      // Lua: executed when actor enters room
    std::string on_tick;       // Lua: executed periodically per actor
    std::string on_remove;     // Lua: executed when actor leaves room
    int tick_interval_sec = 0; // How often on_tick fires (0 = never)
    bool prevents_speaking = false;
    bool prevents_casting = false;
    bool prevents_movement = false;
};

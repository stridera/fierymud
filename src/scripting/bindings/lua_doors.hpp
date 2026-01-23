#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register doors namespace bindings with Lua.
 *
 * Lua API:
 *   doors.open(room, direction)             - Open a door
 *   doors.close(room, direction)            - Close a door
 *   doors.lock(room, direction)             - Lock a door
 *   doors.unlock(room, direction)           - Unlock a door
 *   doors.set_state(room, direction, flags) - Set door state directly
 */
void register_door_bindings(sol::state& lua);

} // namespace FieryMUD

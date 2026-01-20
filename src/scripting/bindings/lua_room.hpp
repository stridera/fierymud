#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register Room bindings with Lua.
 *
 * Lua API:
 *   room.name            - Room name/title
 *   room.id              - Room entity ID as string
 *   room.sector          - Sector type (string)
 *   room.is_dark         - True if room is dark
 *   room.is_peaceful     - True if no combat allowed
 *   room.actors          - Table of actors in room
 *   room.objects         - Table of objects in room
 *   room.exits           - Table of exit directions
 *
 *   room:send(msg)       - Send message to all in room
 *   room:send_except(actor, msg) - Send to all except actor
 *   room:has_exit(dir)   - Check if exit exists
 *   room:get_exit_room(dir) - Get destination room
 *   room:find_actor(kw)  - Find actor by keyword
 *   room:find_object(kw) - Find object by keyword
 *   room:spawn_mob(id)   - Spawn mobile in room (future)
 *   room:spawn_obj(id)   - Spawn object in room (future)
 */
void register_room_bindings(sol::state& lua);

} // namespace FieryMUD

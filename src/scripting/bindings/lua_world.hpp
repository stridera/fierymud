#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register world namespace bindings with Lua.
 *
 * Lua API:
 *   world.find_player(name)  - Find a player by name (case-insensitive)
 *   world.find_mobile(name)  - Find a mobile by name in the world
 *   world.destroy(entity)    - Remove an entity (mobile or object) from the world
 */
void register_world_bindings(sol::state& lua);

} // namespace FieryMUD

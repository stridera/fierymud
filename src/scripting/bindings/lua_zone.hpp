#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register zone namespace bindings with Lua.
 *
 * Lua API:
 *   zone.echo(zone_id, message) - Send message to all players in a zone
 *   zone.reset(zone_id)         - Trigger zone reset
 */
void register_zone_bindings(sol::state &lua);

} // namespace FieryMUD

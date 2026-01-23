#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register combat namespace bindings with Lua.
 *
 * Lua API:
 *   combat.engage(attacker, defender) - Start combat between two actors
 *   combat.rescue(rescuer, target)    - Rescuer intervenes to protect target
 *   combat.disengage(actor)           - Remove actor from combat
 *   combat.is_fighting(actor)         - Check if actor is in combat
 */
void register_combat_bindings(sol::state& lua);

} // namespace FieryMUD

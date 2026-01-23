#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register effects namespace bindings with Lua.
 *
 * Lua API:
 *   effects.apply(actor, effect, options?) - Apply an effect to an actor
 *   effects.remove(actor, effect)          - Remove an effect from an actor
 *   effects.has(actor, effect)             - Check if actor has an effect
 *   effects.duration(actor, effect)        - Get remaining duration
 */
void register_effect_bindings(sol::state& lua);

} // namespace FieryMUD

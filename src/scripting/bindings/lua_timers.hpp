#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register timers namespace bindings with Lua.
 *
 * Non-blocking delayed execution for scripts.
 *
 * Lua API:
 *   timers.after(seconds, callback) - Execute callback after delay
 *   timers.cancel(timer_id)         - Cancel a pending timer
 */
void register_timer_bindings(sol::state& lua);

} // namespace FieryMUD

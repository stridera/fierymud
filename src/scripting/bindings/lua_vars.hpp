#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register vars namespace bindings with Lua.
 *
 * Entity variable storage - persistent key-value store for any entity.
 *
 * Lua API:
 *   vars.set(entity, key, value) - Set a variable on an entity
 *   vars.get(entity, key)        - Get a variable from an entity
 *   vars.has(entity, key)        - Check if entity has a variable
 *   vars.clear(entity, key)      - Remove a variable from an entity
 *   vars.all(entity)             - Get all variables for an entity
 */
void register_var_bindings(sol::state &lua);

} // namespace FieryMUD

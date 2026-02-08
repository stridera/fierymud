#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register Object bindings with Lua.
 *
 * Lua API:
 *   object.name          - Object name
 *   object.short_desc    - Short description
 *   object.description   - Detailed description
 *   object.id            - Entity ID as string
 *   object.type          - Object type (string)
 *   object.weight        - Weight in pounds
 *   object.value         - Value in copper
 *   object.level         - Minimum level to use
 *   object.wear_flags    - Where item can be worn
 *
 *   object:is_type(type) - Check object type
 *   object:has_flag(flag)- Check extra flag
 *   object:is_container  - True if container
 *   object:is_weapon     - True if weapon
 *   object:is_armor      - True if armor
 */
void register_object_bindings(sol::state &lua);

} // namespace FieryMUD

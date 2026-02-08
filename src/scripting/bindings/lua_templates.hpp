#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register template bindings (objects.template, mobiles.template) with Lua.
 *
 * Templates are read-only prototypes that can be used to inspect entity
 * properties before spawning.
 *
 * Lua API:
 *   objects.template(zone_id, local_id) - Get read-only object template
 *   mobiles.template(zone_id, local_id) - Get read-only mobile template
 */
void register_template_bindings(sol::state &lua);

} // namespace FieryMUD

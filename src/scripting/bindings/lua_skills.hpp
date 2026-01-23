#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register skills namespace bindings with Lua.
 *
 * Data-driven skill execution - skills are looked up from database.
 *
 * Lua API:
 *   skills.execute(actor, skill_name, target?) - Execute a skill
 *   skills.exists(skill_name)                  - Check if skill exists
 *   skills.get_level(actor, skill_name)        - Get actor's skill level
 *   skills.set_level(actor, skill_name, level) - Set actor's skill level
 */
void register_skill_bindings(sol::state& lua);

} // namespace FieryMUD

#pragma once

#include <sol/forward.hpp>

namespace FieryMUD {

/**
 * Register spells namespace bindings with Lua.
 *
 * Lua API:
 *   spells.cast(caster, spell, target?, level?) - Cast a spell
 *   spells.exists(spell_name)                   - Check if spell exists
 */
void register_spell_bindings(sol::state &lua);

} // namespace FieryMUD

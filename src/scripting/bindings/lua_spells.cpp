#include "lua_spells.hpp"

#include "../../core/actor.hpp"
#include "../../core/spell_system.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_spell_bindings(sol::state &lua) {
    auto spells_table = lua.create_named_table("spells");

    // spells.cast(caster, spell, target?, level?) - Cast a spell
    // Returns: (bool success, string? error)
    spells_table["cast"] = [](Actor *caster, const std::string &spell_name, sol::optional<Actor *> target,
                              sol::optional<int> level) -> std::tuple<bool, sol::optional<std::string>> {
        if (!caster) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        if (spell_name.empty()) {
            return std::make_tuple(false, std::string("spell_not_found"));
        }

        auto &spell_system = SpellSystem::instance();

        // Check if spell exists
        if (!spell_system.spell_exists(spell_name)) {
            spdlog::debug("spells.cast: spell '{}' not found", spell_name);
            return std::make_tuple(false, std::string("spell_not_found"));
        }

        // Determine cast level
        int cast_level = level.value_or(caster->stats().level);

        // Cast the spell
        Actor *target_actor = target.value_or(nullptr);
        auto result = spell_system.cast_spell(*caster, spell_name, target_actor, cast_level);

        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("spells.cast: {} cast '{}' at level {}", caster->name(), spell_name, cast_level);
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // spells.exists(spell_name) - Check if spell exists in database
    // Returns: bool
    spells_table["exists"] = [](const std::string &spell_name) -> bool {
        if (spell_name.empty()) {
            return false;
        }
        return SpellSystem::instance().spell_exists(spell_name);
    };

    spdlog::debug("Registered spells Lua bindings");
}

} // namespace FieryMUD

#include "lua_effects.hpp"

#include "../../core/actor.hpp"
#include "../../core/effect_system.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_effect_bindings(sol::state &lua) {
    auto effects_table = lua.create_named_table("effects");

    // effects.apply(actor, effect, options?) - Apply an effect to an actor
    // options table: { duration = int, power = int, source = Actor }
    // Returns: (bool success, string? error)
    effects_table["apply"] = [](Actor *actor, const std::string &effect_name,
                                sol::optional<sol::table> options) -> std::tuple<bool, sol::optional<std::string>> {
        if (!actor) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        if (effect_name.empty()) {
            return std::make_tuple(false, std::string("effect_not_found"));
        }

        auto &effect_system = EffectSystem::instance();

        // Check if effect type exists
        if (!effect_system.effect_exists(effect_name)) {
            spdlog::debug("effects.apply: effect '{}' not found", effect_name);
            return std::make_tuple(false, std::string("effect_not_found"));
        }

        // Parse options
        EffectOptions opts;
        if (options) {
            opts.duration = options->get_or("duration", 0);
            opts.power = options->get_or("power", 1);
            // Note: source actor could be added here if needed
        }

        auto result = effect_system.apply_effect(*actor, effect_name, opts);
        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("effects.apply: applied '{}' to {}", effect_name, actor->name());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // effects.remove(actor, effect) - Remove an effect from an actor
    // Returns: (bool success, string? error)
    effects_table["remove"] = [](Actor *actor,
                                 const std::string &effect_name) -> std::tuple<bool, sol::optional<std::string>> {
        if (!actor) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        if (effect_name.empty()) {
            return std::make_tuple(false, std::string("effect_not_found"));
        }

        auto &effect_system = EffectSystem::instance();
        auto result = effect_system.remove_effect(*actor, effect_name);

        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("effects.remove: removed '{}' from {}", effect_name, actor->name());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // effects.has(actor, effect) - Check if actor has an effect
    // Returns: bool
    effects_table["has"] = [](Actor *actor, const std::string &effect_name) -> bool {
        if (!actor || effect_name.empty()) {
            return false;
        }
        return EffectSystem::instance().has_effect(*actor, effect_name);
    };

    // effects.duration(actor, effect) - Get remaining duration in seconds
    // Returns: int? (nil if effect not present)
    effects_table["duration"] = [](Actor *actor, const std::string &effect_name) -> sol::optional<int> {
        if (!actor || effect_name.empty()) {
            return sol::nullopt;
        }

        auto duration = EffectSystem::instance().get_effect_duration(*actor, effect_name);
        if (duration) {
            return *duration;
        }
        return sol::nullopt;
    };

    spdlog::debug("Registered effects Lua bindings");
}

} // namespace FieryMUD

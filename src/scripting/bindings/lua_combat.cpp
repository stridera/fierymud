#include "lua_combat.hpp"

#include "../../core/actor.hpp"
#include "../../core/combat.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_combat_bindings(sol::state &lua) {
    auto combat_table = lua.create_named_table("combat");

    // combat.engage(attacker, defender) - Start combat between two actors
    // Returns: (bool success, string? error)
    combat_table["engage"] = [](std::shared_ptr<Actor> attacker,
                                std::shared_ptr<Actor> defender) -> std::tuple<bool, sol::optional<std::string>> {
        if (!attacker) {
            return std::make_tuple(false, std::string("invalid_target"));
        }
        if (!defender) {
            return std::make_tuple(false, std::string("invalid_target"));
        }
        if (attacker == defender) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        // Check if already fighting
        if (CombatManager::is_in_combat(*attacker)) {
            return std::make_tuple(false, std::string("already_in_combat"));
        }

        // Start combat - clears existing enemies for fresh 1v1
        CombatManager::start_combat(attacker, defender);

        spdlog::debug("combat.engage: {} engaged {}", attacker->name(), defender->name());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // combat.rescue(rescuer, target) - Rescuer intervenes to protect target
    // Rescuer takes over aggro from target's attackers
    // Returns: (bool success, string? error)
    combat_table["rescue"] = [](std::shared_ptr<Actor> rescuer,
                                std::shared_ptr<Actor> target) -> std::tuple<bool, sol::optional<std::string>> {
        if (!rescuer) {
            return std::make_tuple(false, std::string("invalid_target"));
        }
        if (!target) {
            return std::make_tuple(false, std::string("invalid_target"));
        }
        if (rescuer == target) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        // Target must be in combat to be rescued
        if (!CombatManager::is_in_combat(*target)) {
            return std::make_tuple(false, std::string("not_in_combat"));
        }

        // Get target's attackers and make them attack rescuer instead
        auto target_attackers = target->get_all_enemies();
        for (auto &attacker : target_attackers) {
            if (attacker && attacker != rescuer) {
                // Add combat pair: attacker now fights rescuer
                CombatManager::add_combat_pair(attacker, rescuer);
                // Remove target from attacker's list
                attacker->remove_enemy(target);
            }
        }

        // Rescuer enters combat with target's attackers
        if (!CombatManager::is_in_combat(*rescuer) && !target_attackers.empty()) {
            rescuer->set_position(Position::Fighting);
        }

        spdlog::debug("combat.rescue: {} rescued {}", rescuer->name(), target->name());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // combat.disengage(actor) - Remove actor from combat
    // Returns: (bool success, string? error)
    combat_table["disengage"] = [](std::shared_ptr<Actor> actor) -> std::tuple<bool, sol::optional<std::string>> {
        if (!actor) {
            return std::make_tuple(false, std::string("invalid_target"));
        }

        if (!CombatManager::is_in_combat(*actor)) {
            return std::make_tuple(false, std::string("not_in_combat"));
        }

        CombatManager::end_combat(actor);

        spdlog::debug("combat.disengage: {} left combat", actor->name());
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    // combat.is_fighting(actor) - Check if actor is in combat
    // Returns: bool
    combat_table["is_fighting"] = [](std::shared_ptr<Actor> actor) -> bool {
        if (!actor) {
            return false;
        }
        return CombatManager::is_in_combat(*actor);
    };

    spdlog::debug("Registered combat Lua bindings");
}

} // namespace FieryMUD

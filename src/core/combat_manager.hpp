#pragma once

#include "result.hpp"

#include <expected>
#include <memory>

// Forward declarations
class Actor;

namespace FieryMUD {

/**
 * Error information for combat operations.
 */
struct CombatError {
    std::string message;
};

/**
 * CombatManager handles combat state and operations.
 *
 * This is a stub implementation for the Lua scripting API.
 * TODO: Implement full combat system.
 */
class CombatManager {
  public:
    static CombatManager &instance() {
        static CombatManager instance;
        return instance;
    }

    /**
     * Start combat between attacker and defender.
     */
    std::expected<void, CombatError> engage(Actor &attacker, Actor &defender) {
        // TODO: Implement combat engagement
        return std::unexpected(CombatError{"Combat system not yet implemented"});
    }

    /**
     * Rescuer intervenes to protect target.
     */
    std::expected<void, CombatError> rescue(Actor &rescuer, Actor &target) {
        // TODO: Implement rescue
        return std::unexpected(CombatError{"Combat system not yet implemented"});
    }

    /**
     * Remove actor from combat.
     */
    std::expected<void, CombatError> disengage(Actor &actor) {
        // TODO: Implement disengage
        return std::unexpected(CombatError{"Combat system not yet implemented"});
    }

    /**
     * Check if actor is in combat.
     */
    bool is_fighting(const Actor &actor) const {
        // TODO: Check combat state
        return false;
    }

  private:
    CombatManager() = default;
};

} // namespace FieryMUD

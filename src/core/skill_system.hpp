#pragma once

#include "result.hpp"

#include <expected>
#include <string>
#include <string_view>

// Forward declarations
class Actor;

namespace FieryMUD {

/**
 * Error information for skill operations.
 */
struct SkillError {
    std::string message;
};

/**
 * SkillSystem handles data-driven skill execution.
 *
 * Skills are looked up from the database (AbilityCache) and executed dynamically.
 * Uses the AbilityExecutor for actual execution.
 */
class SkillSystem {
  public:
    static SkillSystem &instance();

    /**
     * Execute a skill (looks up from database and executes via AbilityExecutor).
     */
    std::expected<void, SkillError> execute_skill(Actor &actor, std::string_view skill_name, Actor *target = nullptr);

    /**
     * Check if a skill exists in the ability cache.
     */
    bool skill_exists(std::string_view skill_name) const;

    /**
     * Get actor's skill level (proficiency).
     */
    int get_skill_level(const Actor &actor, std::string_view skill_name) const;

    /**
     * Set actor's skill level (proficiency).
     */
    void set_skill_level(Actor &actor, std::string_view skill_name, int level);

  private:
    SkillSystem() = default;
};

} // namespace FieryMUD

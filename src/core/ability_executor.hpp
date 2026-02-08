#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "commands/command_system.hpp"
#include "core/effect_system.hpp"
#include "core/result.hpp"
#include "database/world_queries.hpp"

// Forward declarations
class Actor;
class Room;

namespace FieryMUD {

/**
 * AbilityExecutionResult captures the full outcome of ability execution.
 */
struct AbilityExecutionResult {
    bool success = false;
    int total_damage = 0;                     // Total damage dealt
    int total_healing = 0;                    // Total healing done
    std::vector<EffectResult> effect_results; // Results from each effect
    std::string error_message;                // If failed, why?

    // Consolidated messages
    std::string attacker_message;
    std::string target_message;
    std::string room_message;

    static AbilityExecutionResult failure(std::string_view error);
};

/**
 * Cached ability messages for fast lookup.
 */
struct CachedAbilityMessages {
    std::string start_to_caster;
    std::string start_to_victim;
    std::string start_to_room;
    std::string success_to_caster;
    std::string success_to_victim;
    std::string success_to_room;
    std::string success_to_self;   // Message to caster when targeting self
    std::string success_self_room; // Room message when caster targets self
    std::string fail_to_caster;
    std::string fail_to_victim;
    std::string fail_to_room;
    std::string wearoff_to_target;
    std::string wearoff_to_room;
    std::string look_message; // Message when looking at someone affected by this ability
};

/**
 * Cached ability restrictions for fast lookup.
 */
struct CachedAbilityRestrictions {
    std::vector<WorldQueries::AbilityRequirement> requirements;
    std::string custom_lua;
};

/**
 * Cached damage components for abilities.
 */
struct CachedDamageComponent {
    std::string element;
    std::string damage_formula;
    int percentage;
    int sequence;
};

/** Cached class info for an ability (which classes can learn it) */
struct CachedAbilityClass {
    int class_id;
    std::string class_name; // Plain class name (no color codes)
    int circle;             // Spell circle (1-9)
};

/**
 * AbilityCache stores loaded abilities and effects for fast access.
 * Call initialize() at startup to load all abilities from database.
 */
class AbilityCache {
  public:
    static AbilityCache &instance();

    /** Initialize cache by loading all abilities and effects from database */
    Result<void> initialize();

    /** Check if cache is initialized */
    bool is_initialized() const { return initialized_; }

    /** Get ability by ID */
    const WorldQueries::AbilityData *get_ability(int ability_id) const;

    /** Get ability by name (case-insensitive) */
    const WorldQueries::AbilityData *get_ability_by_name(std::string_view name) const;

    /** Get effect definition by ID */
    const EffectDefinition *get_effect(int effect_id) const;

    /** Get all effects for an ability */
    std::vector<AbilityEffect> get_ability_effects(int ability_id) const;

    /** Get custom messages for an ability (nullptr if not set) */
    const CachedAbilityMessages *get_ability_messages(int ability_id) const;

    /** Get restrictions for an ability (nullptr if none) */
    const CachedAbilityRestrictions *get_ability_restrictions(int ability_id) const;

    /** Get damage components for an ability (empty if none) */
    std::vector<CachedDamageComponent> get_damage_components(int ability_id) const;

    /** Get classes that can learn an ability (empty if none) */
    std::vector<CachedAbilityClass> get_ability_classes(int ability_id) const;

    /** Reload cache from database */
    Result<void> reload();

  private:
    AbilityCache() = default;

    bool initialized_ = false;
    std::unordered_map<int, WorldQueries::AbilityData> abilities_;
    std::unordered_map<std::string, int> ability_name_index_; // lowercase name -> id
    std::unordered_map<int, EffectDefinition> effects_;
    std::unordered_map<int, std::vector<AbilityEffect>> ability_effects_;           // ability_id -> effects
    std::unordered_map<int, CachedAbilityMessages> ability_messages_;               // ability_id -> messages
    std::unordered_map<int, CachedAbilityRestrictions> ability_restrictions_;       // ability_id -> restrictions
    std::unordered_map<int, std::vector<CachedDamageComponent>> damage_components_; // ability_id -> damage components
    std::unordered_map<int, std::vector<CachedAbilityClass>>
        ability_classes_; // ability_id -> classes that can learn it
};

/**
 * AbilityExecutor handles the complete execution of data-driven abilities.
 *
 * Usage:
 *   auto result = AbilityExecutor::execute(ctx, "kick", target, skill_level);
 *   if (result && result->success) {
 *       // Send messages from result
 *   }
 */
class AbilityExecutor {
  public:
    /**
     * Execute an ability by name.
     * @param ctx Command context (actor, room, etc.)
     * @param ability_name Name of the ability to execute
     * @param target Target of the ability (can be null for self-targeted)
     * @param skill_level Proficiency level (0-100)
     * @return Execution result with damage, messages, etc.
     */
    static std::expected<AbilityExecutionResult, Error> execute(const CommandContext &ctx,
                                                                std::string_view ability_name,
                                                                std::shared_ptr<Actor> target, int skill_level = 50);

    /**
     * Execute an ability by ID.
     */
    static std::expected<AbilityExecutionResult, Error>
    execute_by_id(const CommandContext &ctx, int ability_id, std::shared_ptr<Actor> target, int skill_level = 50);

    /**
     * Check if actor can use an ability (position, cooldown, etc.)
     */
    static std::expected<void, Error> check_prerequisites(const CommandContext &ctx,
                                                          const WorldQueries::AbilityData &ability,
                                                          std::shared_ptr<Actor> target);

    /**
     * Execute a spell after casting time completes.
     * Called by the world server when casting finishes.
     * @param caster The actor who was casting
     * @param ability_id ID of the ability being cast
     * @param target Target of the spell (may be null)
     * @return Success or error
     */
    static std::expected<AbilityExecutionResult, Error>
    execute_completed_cast(std::shared_ptr<Actor> caster, int ability_id, std::shared_ptr<Actor> target);

    /**
     * Begin casting a spell (initiate casting time).
     * @param caster The actor casting
     * @param ability_id ID of the ability to cast
     * @param target Target of the spell (may be null)
     * @param spell_circle Circle of the spell (for casting time calc)
     * @return Success or error
     */
    static std::expected<void, Error> begin_casting(std::shared_ptr<Actor> caster, int ability_id,
                                                    std::shared_ptr<Actor> target, int spell_circle);

  private:
    /**
     * Execute all effects for an ability.
     */
    static std::expected<std::vector<EffectResult>, Error>
    execute_effects(const WorldQueries::AbilityData &ability, EffectContext &context, EffectTrigger trigger);
};

/**
 * Generic skill command handler that uses the data-driven ability system.
 * Can be used to implement kick, bash, and other combat skills.
 */
Result<CommandResult> execute_skill_command(const CommandContext &ctx, std::string_view skill_name,
                                            bool requires_target = true, bool can_initiate_combat = true);

} // namespace FieryMUD

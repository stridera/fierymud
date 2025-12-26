/**
 * @file active_effect.hpp
 * @brief Data-driven active effect system for DoT, buffs, debuffs, etc.
 *
 * DotEffect represents a damage-over-time effect currently active on an actor,
 * with all parameters resolved at cast time from the database Effect/AbilityEffect
 * tables.
 */

#pragma once

#include <string>

namespace fiery {

/**
 * @brief Result of attempting to cure an effect
 */
enum class CureResult {
    FullCure,      ///< Effect completely removed
    PartialCure,   ///< Duration/potency reduced but not fully cured
    WrongCategory, ///< Cure doesn't match effect's cure category
    TooPowerful,   ///< Effect potency too high for this cure power
    NoEffect       ///< No matching effects found on target
};

/**
 * @brief Detailed result from a cure attempt
 */
struct CureAttemptResult {
    CureResult result = CureResult::NoEffect;
    std::string message;
    int effects_removed = 0;
    int effects_reduced = 0;
};

/**
 * @brief A damage-over-time effect currently applied to an actor
 *
 * This struct holds all the resolved parameters for a DoT effect that has been
 * applied to an actor. The values are computed at cast time from the database
 * Effect.defaultParams merged with AbilityEffect.overrideParams, with formulas
 * resolved using the caster's level and skill.
 */
struct DotEffect {
    // --- Effect identification ---
    int ability_id = 0;            ///< Which ability applied this effect
    int effect_id = 0;             ///< Which Effect type this is
    std::string effect_type;       ///< "dot", "modify", "status", etc.

    // --- Resolved parameters (computed at cast time) ---
    std::string damage_type;       ///< "poison", "fire", "disease", etc.
    std::string cure_category;     ///< What type of cure removes this
    int potency = 5;               ///< 1-10 scale, cure must have >= curePower
    int flat_damage = 0;           ///< Pre-computed flat damage per tick
    int percent_damage = 0;        ///< Percentage of max HP per tick
    bool blocks_regen = false;     ///< Completely blocks natural HP regen
    int reduces_regen = 0;         ///< 0-100, percentage reduction to regen
    int max_resistance = 75;       ///< Cap on resistance reduction

    // --- Timing ---
    int remaining_ticks = -1;      ///< Ticks until expiry (-1 = permanent until cured)
    int tick_interval = 1;         ///< Ticks between damage applications
    int ticks_since_last = 0;      ///< Counter for tick interval

    // --- Source tracking ---
    std::string source_actor_id;   ///< Who applied this effect
    int source_level = 1;          ///< Caster's level when applied

    // --- Stacking ---
    int stack_count = 1;           ///< Current number of stacks
    int max_stacks = 1;            ///< Maximum allowed stacks
    bool stackable = false;        ///< Can this effect stack?

    /**
     * @brief Check if this effect should apply damage this tick
     * @return true if tick_interval has elapsed
     */
    [[nodiscard]] bool should_tick() const {
        return ticks_since_last >= tick_interval;
    }

    /**
     * @brief Advance the tick counter
     */
    void advance_tick() {
        ticks_since_last++;
    }

    /**
     * @brief Reset tick counter after applying damage
     */
    void reset_tick_counter() {
        ticks_since_last = 0;
    }

    /**
     * @brief Check if effect has expired
     * @return true if remaining_ticks has reached 0
     */
    [[nodiscard]] bool is_expired() const {
        return remaining_ticks == 0;
    }

    /**
     * @brief Decrement remaining duration
     */
    void decrement_duration() {
        if (remaining_ticks > 0) {
            remaining_ticks--;
        }
    }

    /**
     * @brief Check if this effect matches a cure category
     * @param category The cure category to check ("poison", "fire", "all", etc.)
     * @return true if this effect can be cured by the given category
     */
    [[nodiscard]] bool matches_cure_category(const std::string& category) const {
        return category == "all" || cure_category == category;
    }
};

/**
 * @brief A heal-over-time effect currently applied to an actor
 *
 * This struct holds all the resolved parameters for a HoT effect that has been
 * applied to an actor. Similar to DotEffect but heals instead of damages.
 */
struct HotEffect {
    // --- Effect identification ---
    int ability_id = 0;            ///< Which ability applied this effect
    int effect_id = 0;             ///< Which Effect type this is
    std::string effect_type;       ///< "hot"

    // --- Resolved parameters (computed at cast time) ---
    std::string heal_type;         ///< "heal", "regen", "divine", "nature", etc.
    std::string hot_category;      ///< Category for dispel matching
    int flat_heal = 0;             ///< Pre-computed flat healing per tick
    int percent_heal = 0;          ///< Percentage of max HP per tick
    bool boosts_regen = false;     ///< Enhances natural HP regen
    int regen_boost = 0;           ///< 0-100, percentage boost to regen

    // --- Timing ---
    int remaining_ticks = -1;      ///< Ticks until expiry (-1 = permanent until dispelled)
    int tick_interval = 1;         ///< Ticks between heal applications
    int ticks_since_last = 0;      ///< Counter for tick interval

    // --- Source tracking ---
    std::string source_actor_id;   ///< Who applied this effect
    int source_level = 1;          ///< Caster's level when applied

    // --- Stacking ---
    int stack_count = 1;           ///< Current number of stacks
    int max_stacks = 1;            ///< Maximum allowed stacks
    bool stackable = false;        ///< Can this effect stack?

    /**
     * @brief Check if this effect should apply healing this tick
     * @return true if tick_interval has elapsed
     */
    [[nodiscard]] bool should_tick() const {
        return ticks_since_last >= tick_interval;
    }

    /**
     * @brief Advance the tick counter
     */
    void advance_tick() {
        ticks_since_last++;
    }

    /**
     * @brief Reset tick counter after applying healing
     */
    void reset_tick_counter() {
        ticks_since_last = 0;
    }

    /**
     * @brief Check if effect has expired
     * @return true if remaining_ticks has reached 0
     */
    [[nodiscard]] bool is_expired() const {
        return remaining_ticks == 0;
    }

    /**
     * @brief Decrement remaining duration
     */
    void decrement_duration() {
        if (remaining_ticks > 0) {
            remaining_ticks--;
        }
    }

    /**
     * @brief Check if this effect matches a dispel category
     * @param category The dispel category to check ("heal", "divine", "all", etc.)
     * @return true if this effect can be dispelled by the given category
     */
    [[nodiscard]] bool matches_dispel_category(const std::string& category) const {
        return category == "all" || hot_category == category;
    }
};

/**
 * @brief Result of processing HoT effects during a tick
 */
struct HotTickResult {
    int total_healing = 0;
    std::vector<std::string> expired_effects;
};

} // namespace fiery

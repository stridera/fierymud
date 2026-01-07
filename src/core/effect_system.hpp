#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/active_effect.hpp"
#include "core/result.hpp"
#include "core/formula_parser.hpp"

// Forward declarations
class Actor;

namespace FieryMUD {

/**
 * EffectType enum matching the database effectType values.
 */
enum class EffectType {
    Damage,
    Heal,
    Modify,
    Status,
    Cleanse,
    Dispel,
    Reveal,
    Teleport,
    Extract,
    Move,
    Interrupt,
    Transform,
    Resurrect,
    Create,
    Summon,
    Enchant,
    Globe,
    Room,
    Inspect,
    Dot,        // Damage over time (poison, fire, etc.)
    Hot,        // Heal over time (regeneration, etc.)
    Unknown
};

/**
 * Parse effect type string from database to enum.
 */
EffectType parse_effect_type(std::string_view type_str);

/**
 * Effect trigger types - when the effect activates.
 */
enum class EffectTrigger {
    OnHit,      // Activates when ability successfully hits
    OnCast,     // Activates when ability is cast (before hit check)
    OnMiss,     // Activates when ability misses
    Periodic,   // Activates periodically (DoT/HoT)
    OnEnd,      // Activates when effect duration ends
    OnTrigger   // Activates based on external trigger
};

EffectTrigger parse_effect_trigger(std::string_view trigger_str);

/**
 * EffectParams holds merged parameters for effect execution.
 * Parameters come from Effect.default_params merged with AbilityEffect.override_params.
 */
struct EffectParams {
    // Damage effect params
    std::string damage_type = "physical";   // fire, cold, physical, etc.
    std::string amount_formula;             // "skill / 2", "4d6 + level"
    std::string scaling_stat;               // "level", "dex", "str"

    // Interrupt effect params
    std::string interrupt_power_formula;    // "skill"
    std::string interrupt_filter = "all";   // "all", "magic", "skill"

    // Heal effect params
    std::string heal_resource = "hp";       // "hp", "move"
    std::string heal_formula;               // "2d8 + wis_bonus"

    // Status effect params
    std::string status_name;                // "poison", "stun", "invisible"
    std::string status_duration_formula;    // "level", "level * 2", "toggle", or fixed "10"
    int status_duration = 0;                // Duration in ticks (for fixed values)
    std::string status_modifier_stat;       // Optional stat to modify (e.g., "concealment")
    bool is_toggle_duration = false;        // True if duration is "toggle" (permanent until toggled off)

    // Move effect params
    std::string move_type;                  // "knockback", "pull"
    std::string move_distance_formula;      // Formula for distance
    int move_distance = 0;                  // Rooms to move (for fixed values)

    // DoT (Damage Over Time) effect params
    std::string cure_category = "poison";   // "poison", "fire", "disease", "curse", etc.
    std::string potency_formula = "5";      // "4 + (skill / 33)" - determines cure resistance
    std::string flat_damage_formula;        // Flat damage per tick: "1 + (skill / 20)"
    std::string percent_damage_formula;     // % of max HP per tick: "2"
    std::string dot_duration_formula;       // Duration in ticks: "level + 5"
    int tick_interval = 1;                  // Ticks between damage/heal applications
    bool blocks_regen = false;              // Completely blocks natural HP regen
    std::string reduces_regen_formula;      // 0-100 percentage reduction: "25"
    int max_resistance = 75;                // Cap on damage reduction from resistance
    bool stackable = false;                 // Can this DoT/HoT stack?
    int max_stacks = 1;                     // Maximum number of stacks

    // HoT (Heal Over Time) effect params - uses tick_interval, stackable, max_stacks from above
    std::string hot_category = "heal";      // "heal", "regen", "divine", "nature", etc.
    std::string flat_heal_formula;          // Flat healing per tick: "2 + (skill / 15)"
    std::string percent_heal_formula;       // % of max HP per tick: "3"
    std::string hot_duration_formula;       // Duration in ticks: "level + 10"
    bool boosts_regen = false;              // Enhances natural HP regen
    std::string boosts_regen_formula;       // 0-100 percentage boost: "50"

    // Teleport effect params
    std::string teleport_type = "random";   // "random" (same zone), "recall" (homeroom), "fixed"
    int teleport_room_id = 0;               // For fixed teleport destination
    std::string success_formula = "90";     // Base success chance formula

    // Reveal effect params
    std::string reveal_type = "all";        // "hidden", "invisible", "doors", "all"
    bool clear_fog = true;                  // Clear room fog effects

    // Cleanse effect params
    std::string cleanse_category = "all";   // "poison", "disease", "curse", "all"
    std::string cleanse_power_formula;      // Power vs potency check: "skill + wis_bonus"
    int max_effects_removed = 0;            // 0 = unlimited

    // Dispel effect params
    std::string dispel_type = "beneficial"; // "beneficial", "harmful", "all"
    std::string dispel_power_formula;       // Power for save check: "skill"
    bool dispel_objects = false;            // Also dispel object enchantments

    // Summon effect params
    std::string summon_type = "creature";   // "creature", "corpse", "player"
    std::string max_distance_formula = "10";// Max distance: "skill / 5"
    std::string max_level_formula;          // Max target level: "skill + 3"
    bool requires_consent = true;           // Requires target consent for players

    // Resurrect effect params
    std::string exp_return_formula = "60";  // % of death exp returned: "60"
    bool requires_corpse = true;            // Must have corpse in room

    // Generic
    std::string chance_formula;             // Formula for chance
    int chance_percent = 100;               // Chance to apply (0-100)
    std::string condition;                  // Lua condition expression

    // Parse from JSON-like parameter map
    static EffectParams from_json_string(std::string_view json_str);
    void merge_override(std::string_view override_json);
};

/**
 * EffectDefinition represents a reusable effect from the Effect table.
 */
struct EffectDefinition {
    int id = 0;
    std::string name;
    std::string description;
    EffectType type = EffectType::Unknown;
    EffectParams default_params;
};

/**
 * AbilityEffect represents a linked effect for a specific ability.
 * Stored in AbilityEffect table.
 */
struct AbilityEffect {
    int ability_id = 0;
    int effect_id = 0;
    EffectParams params;           // Merged default + override params
    int order = 0;                 // Execution order
    EffectTrigger trigger = EffectTrigger::OnHit;
    int chance_percent = 100;
    std::string condition;         // Optional Lua condition
};

/**
 * EffectResult captures the outcome of executing an effect.
 */
struct EffectResult {
    bool success = false;
    int value = 0;                 // Damage dealt, HP healed, etc.
    EffectType type = EffectType::Unknown;  // Type of effect that produced this result
    std::string attacker_message;  // Message to the actor using the ability
    std::string target_message;    // Message to the target
    std::string room_message;      // Message to others in the room
    std::string dice_details;      // Detailed dice roll info (formula, result, type)

    static EffectResult success_result(int value, std::string_view attacker_msg,
                                       std::string_view target_msg, std::string_view room_msg,
                                       std::string_view dice_details = "",
                                       EffectType type = EffectType::Unknown);
    static EffectResult failure_result(std::string_view reason);
};

/**
 * EffectContext provides all context needed to execute an effect.
 */
struct EffectContext {
    std::shared_ptr<Actor> actor;      // The one using the ability
    std::shared_ptr<Actor> target;     // The target of the ability
    FormulaContext formula_ctx;        // Variables for formula evaluation
    int skill_level = 0;               // Proficiency in this ability (0-100)
    int spell_circle = 1;              // Spell circle (1-9), used for base_damage calculation
    std::string ability_name;          // Display name of the ability (for effect names)
    int ability_id = 0;                // Database ID of the ability
    int effect_id = 0;                 // Database ID of the effect being executed

    // Build formula context from actor and skill
    void build_formula_context();
};

/**
 * EffectExecutor executes effects based on their type and parameters.
 */
class EffectExecutor {
public:
    /**
     * Execute a single effect with the given context.
     */
    static std::expected<EffectResult, Error> execute(
        const EffectDefinition& effect_def,
        const EffectParams& params,
        EffectContext& context);

    /**
     * Execute all effects for an ability in order.
     */
    static std::expected<std::vector<EffectResult>, Error> execute_ability_effects(
        const std::vector<AbilityEffect>& effects,
        const std::unordered_map<int, EffectDefinition>& effect_defs,
        EffectContext& context,
        EffectTrigger trigger_filter);

private:
    static std::expected<EffectResult, Error> execute_damage(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_heal(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_modify(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_interrupt(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_status(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_move(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_dot(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_hot(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_teleport(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_reveal(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_cleanse(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_dispel(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_summon(
        const EffectParams& params, EffectContext& context);

    static std::expected<EffectResult, Error> execute_resurrect(
        const EffectParams& params, EffectContext& context);

    // Roll for effect chance
    static bool roll_chance(int percent);
};

} // namespace FieryMUD

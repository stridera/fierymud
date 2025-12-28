#include "core/effect_system.hpp"

#include "core/actor.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>

// Effect system constants
namespace {
    // Percentage bounds for chance calculations
    constexpr int CHANCE_MIN_PERCENT = 0;
    constexpr int CHANCE_MAX_PERCENT = 100;
}

namespace FieryMUD {

using json = nlohmann::json;

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * Convert an effect name to display format (title case).
 * "armor" -> "Armor"
 * "detect_invis" -> "Detect Invis"
 * "SANCTUARY" -> "Sanctuary"
 */
static std::string format_effect_name(std::string_view name) {
    if (name.empty()) return "";

    std::string result{name};

    // Replace underscores with spaces
    std::replace(result.begin(), result.end(), '_', ' ');

    // Convert to title case
    bool capitalize_next = true;
    for (char& c : result) {
        if (c == ' ') {
            capitalize_next = true;
        } else if (capitalize_next) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize_next = false;
        } else {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    return result;
}

// =============================================================================
// Type Parsing
// =============================================================================

EffectType parse_effect_type(std::string_view type_str) {
    if (type_str == "damage") return EffectType::Damage;
    if (type_str == "heal") return EffectType::Heal;
    if (type_str == "modify") return EffectType::Modify;
    if (type_str == "status") return EffectType::Status;
    if (type_str == "cleanse") return EffectType::Cleanse;
    if (type_str == "dispel") return EffectType::Dispel;
    if (type_str == "reveal") return EffectType::Reveal;
    if (type_str == "teleport") return EffectType::Teleport;
    if (type_str == "extract") return EffectType::Extract;
    if (type_str == "move") return EffectType::Move;
    if (type_str == "interrupt") return EffectType::Interrupt;
    if (type_str == "transform") return EffectType::Transform;
    if (type_str == "resurrect") return EffectType::Resurrect;
    if (type_str == "create") return EffectType::Create;
    if (type_str == "summon") return EffectType::Summon;
    if (type_str == "enchant") return EffectType::Enchant;
    if (type_str == "globe") return EffectType::Globe;
    if (type_str == "room") return EffectType::Room;
    if (type_str == "inspect") return EffectType::Inspect;
    if (type_str == "dot") return EffectType::Dot;
    if (type_str == "hot") return EffectType::Hot;
    return EffectType::Unknown;
}

EffectTrigger parse_effect_trigger(std::string_view trigger_str) {
    if (trigger_str == "on_hit") return EffectTrigger::OnHit;
    if (trigger_str == "on_cast") return EffectTrigger::OnCast;
    if (trigger_str == "on_miss") return EffectTrigger::OnMiss;
    if (trigger_str == "periodic") return EffectTrigger::Periodic;
    if (trigger_str == "on_end") return EffectTrigger::OnEnd;
    if (trigger_str == "on_trigger") return EffectTrigger::OnTrigger;
    return EffectTrigger::OnHit; // Default
}

// =============================================================================
// EffectParams
// =============================================================================

EffectParams EffectParams::from_json_string(std::string_view json_str) {
    EffectParams params;
    if (json_str.empty()) return params;

    try {
        auto j = json::parse(json_str);

        // Damage params
        if (j.contains("type")) params.damage_type = j["type"].get<std::string>();
        if (j.contains("amount")) {
            if (j["amount"].is_string()) {
                params.amount_formula = j["amount"].get<std::string>();
            } else if (j["amount"].is_number()) {
                params.amount_formula = std::to_string(j["amount"].get<int>());
            }
        }
        if (j.contains("scaling")) params.scaling_stat = j["scaling"].get<std::string>();

        // Interrupt params
        if (j.contains("power")) {
            if (j["power"].is_string()) {
                params.interrupt_power_formula = j["power"].get<std::string>();
            } else if (j["power"].is_number()) {
                params.interrupt_power_formula = std::to_string(j["power"].get<int>());
            }
        }
        if (j.contains("filter")) params.interrupt_filter = j["filter"].get<std::string>();

        // Heal params
        if (j.contains("resource")) params.heal_resource = j["resource"].get<std::string>();
        if (j.contains("heal")) {
            if (j["heal"].is_string()) {
                params.heal_formula = j["heal"].get<std::string>();
            } else if (j["heal"].is_number()) {
                params.heal_formula = std::to_string(j["heal"].get<int>());
            }
        }

        // Status params
        if (j.contains("status")) params.status_name = j["status"].get<std::string>();
        if (j.contains("flag")) params.status_name = j["flag"].get<std::string>();  // alias
        if (j.contains("stat")) params.status_modifier_stat = j["stat"].get<std::string>();
        if (j.contains("duration")) {
            if (j["duration"].is_string()) {
                std::string dur_str = j["duration"].get<std::string>();
                if (dur_str == "toggle") {
                    params.is_toggle_duration = true;
                    params.status_duration = -1;  // Permanent until toggled off
                } else {
                    params.status_duration_formula = dur_str;
                }
            } else if (j["duration"].is_number()) {
                params.status_duration = j["duration"].get<int>();
                params.status_duration_formula = std::to_string(params.status_duration);
            }
        }

        // Move params
        if (j.contains("move_type")) params.move_type = j["move_type"].get<std::string>();
        if (j.contains("distance")) {
            if (j["distance"].is_string()) {
                params.move_distance_formula = j["distance"].get<std::string>();
            } else if (j["distance"].is_number()) {
                params.move_distance = j["distance"].get<int>();
                params.move_distance_formula = std::to_string(params.move_distance);
            }
        }

        // DoT params
        if (j.contains("cureCategory")) params.cure_category = j["cureCategory"].get<std::string>();
        if (j.contains("potency")) {
            if (j["potency"].is_string()) {
                params.potency_formula = j["potency"].get<std::string>();
            } else if (j["potency"].is_number()) {
                params.potency_formula = std::to_string(j["potency"].get<int>());
            }
        }
        if (j.contains("flatDamage")) {
            if (j["flatDamage"].is_string()) {
                params.flat_damage_formula = j["flatDamage"].get<std::string>();
            } else if (j["flatDamage"].is_number()) {
                params.flat_damage_formula = std::to_string(j["flatDamage"].get<int>());
            }
        }
        if (j.contains("percentDamage")) {
            if (j["percentDamage"].is_string()) {
                params.percent_damage_formula = j["percentDamage"].get<std::string>();
            } else if (j["percentDamage"].is_number()) {
                params.percent_damage_formula = std::to_string(j["percentDamage"].get<int>());
            }
        }
        if (j.contains("dotDuration")) {
            if (j["dotDuration"].is_string()) {
                params.dot_duration_formula = j["dotDuration"].get<std::string>();
            } else if (j["dotDuration"].is_number()) {
                params.dot_duration_formula = std::to_string(j["dotDuration"].get<int>());
            }
        }
        if (j.contains("tickInterval")) params.tick_interval = j["tickInterval"].get<int>();
        if (j.contains("blocksRegen")) params.blocks_regen = j["blocksRegen"].get<bool>();
        if (j.contains("reducesRegen")) {
            if (j["reducesRegen"].is_string()) {
                params.reduces_regen_formula = j["reducesRegen"].get<std::string>();
            } else if (j["reducesRegen"].is_number()) {
                params.reduces_regen_formula = std::to_string(j["reducesRegen"].get<int>());
            }
        }
        if (j.contains("maxResistance")) params.max_resistance = j["maxResistance"].get<int>();
        if (j.contains("stackable")) params.stackable = j["stackable"].get<bool>();
        if (j.contains("maxStacks")) params.max_stacks = j["maxStacks"].get<int>();

        // HoT params
        if (j.contains("hotCategory")) params.hot_category = j["hotCategory"].get<std::string>();
        if (j.contains("flatHeal")) {
            if (j["flatHeal"].is_string()) {
                params.flat_heal_formula = j["flatHeal"].get<std::string>();
            } else if (j["flatHeal"].is_number()) {
                params.flat_heal_formula = std::to_string(j["flatHeal"].get<int>());
            }
        }
        if (j.contains("percentHeal")) {
            if (j["percentHeal"].is_string()) {
                params.percent_heal_formula = j["percentHeal"].get<std::string>();
            } else if (j["percentHeal"].is_number()) {
                params.percent_heal_formula = std::to_string(j["percentHeal"].get<int>());
            }
        }
        if (j.contains("hotDuration")) {
            if (j["hotDuration"].is_string()) {
                params.hot_duration_formula = j["hotDuration"].get<std::string>();
            } else if (j["hotDuration"].is_number()) {
                params.hot_duration_formula = std::to_string(j["hotDuration"].get<int>());
            }
        }
        if (j.contains("boostsRegen")) params.boosts_regen = j["boostsRegen"].get<bool>();
        if (j.contains("regenBoost")) {
            if (j["regenBoost"].is_string()) {
                params.boosts_regen_formula = j["regenBoost"].get<std::string>();
            } else if (j["regenBoost"].is_number()) {
                params.boosts_regen_formula = std::to_string(j["regenBoost"].get<int>());
            }
        }

        // Generic
        if (j.contains("chance")) {
            if (j["chance"].is_string()) {
                params.chance_formula = j["chance"].get<std::string>();
            } else if (j["chance"].is_number()) {
                params.chance_percent = j["chance"].get<int>();
                params.chance_formula = std::to_string(params.chance_percent);
            }
        }
        if (j.contains("condition")) params.condition = j["condition"].get<std::string>();

    } catch (const json::exception& e) {
        Log::warn("Failed to parse effect params JSON: {}", e.what());
    }

    return params;
}

void EffectParams::merge_override(std::string_view override_json) {
    if (override_json.empty()) return;

    try {
        auto j = json::parse(override_json);

        // Merge only non-empty overrides
        if (j.contains("type")) damage_type = j["type"].get<std::string>();
        if (j.contains("amount")) {
            if (j["amount"].is_string()) {
                amount_formula = j["amount"].get<std::string>();
            } else if (j["amount"].is_number()) {
                amount_formula = std::to_string(j["amount"].get<int>());
            }
        }
        if (j.contains("scaling")) scaling_stat = j["scaling"].get<std::string>();
        if (j.contains("power")) {
            if (j["power"].is_string()) {
                interrupt_power_formula = j["power"].get<std::string>();
            } else if (j["power"].is_number()) {
                interrupt_power_formula = std::to_string(j["power"].get<int>());
            }
        }
        if (j.contains("filter")) interrupt_filter = j["filter"].get<std::string>();
        if (j.contains("resource")) heal_resource = j["resource"].get<std::string>();
        if (j.contains("heal")) {
            if (j["heal"].is_string()) {
                heal_formula = j["heal"].get<std::string>();
            } else if (j["heal"].is_number()) {
                heal_formula = std::to_string(j["heal"].get<int>());
            }
        }
        if (j.contains("status")) status_name = j["status"].get<std::string>();
        if (j.contains("flag")) status_name = j["flag"].get<std::string>();  // alias
        if (j.contains("stat")) status_modifier_stat = j["stat"].get<std::string>();
        if (j.contains("duration")) {
            if (j["duration"].is_string()) {
                std::string dur_str = j["duration"].get<std::string>();
                if (dur_str == "toggle") {
                    is_toggle_duration = true;
                    status_duration = -1;  // Permanent until toggled off
                } else {
                    status_duration_formula = dur_str;
                }
            } else if (j["duration"].is_number()) {
                status_duration = j["duration"].get<int>();
                status_duration_formula = std::to_string(status_duration);
            }
        }
        if (j.contains("move_type")) move_type = j["move_type"].get<std::string>();
        if (j.contains("distance")) {
            if (j["distance"].is_string()) {
                move_distance_formula = j["distance"].get<std::string>();
            } else if (j["distance"].is_number()) {
                move_distance = j["distance"].get<int>();
                move_distance_formula = std::to_string(move_distance);
            }
        }
        // DoT params
        if (j.contains("cureCategory")) cure_category = j["cureCategory"].get<std::string>();
        if (j.contains("potency")) {
            if (j["potency"].is_string()) {
                potency_formula = j["potency"].get<std::string>();
            } else if (j["potency"].is_number()) {
                potency_formula = std::to_string(j["potency"].get<int>());
            }
        }
        if (j.contains("flatDamage")) {
            if (j["flatDamage"].is_string()) {
                flat_damage_formula = j["flatDamage"].get<std::string>();
            } else if (j["flatDamage"].is_number()) {
                flat_damage_formula = std::to_string(j["flatDamage"].get<int>());
            }
        }
        if (j.contains("percentDamage")) {
            if (j["percentDamage"].is_string()) {
                percent_damage_formula = j["percentDamage"].get<std::string>();
            } else if (j["percentDamage"].is_number()) {
                percent_damage_formula = std::to_string(j["percentDamage"].get<int>());
            }
        }
        if (j.contains("dotDuration")) {
            if (j["dotDuration"].is_string()) {
                dot_duration_formula = j["dotDuration"].get<std::string>();
            } else if (j["dotDuration"].is_number()) {
                dot_duration_formula = std::to_string(j["dotDuration"].get<int>());
            }
        }
        if (j.contains("tickInterval")) tick_interval = j["tickInterval"].get<int>();
        if (j.contains("blocksRegen")) blocks_regen = j["blocksRegen"].get<bool>();
        if (j.contains("reducesRegen")) {
            if (j["reducesRegen"].is_string()) {
                reduces_regen_formula = j["reducesRegen"].get<std::string>();
            } else if (j["reducesRegen"].is_number()) {
                reduces_regen_formula = std::to_string(j["reducesRegen"].get<int>());
            }
        }
        if (j.contains("maxResistance")) max_resistance = j["maxResistance"].get<int>();
        if (j.contains("stackable")) stackable = j["stackable"].get<bool>();
        if (j.contains("maxStacks")) max_stacks = j["maxStacks"].get<int>();
        // HoT params
        if (j.contains("hotCategory")) hot_category = j["hotCategory"].get<std::string>();
        if (j.contains("flatHeal")) {
            if (j["flatHeal"].is_string()) {
                flat_heal_formula = j["flatHeal"].get<std::string>();
            } else if (j["flatHeal"].is_number()) {
                flat_heal_formula = std::to_string(j["flatHeal"].get<int>());
            }
        }
        if (j.contains("percentHeal")) {
            if (j["percentHeal"].is_string()) {
                percent_heal_formula = j["percentHeal"].get<std::string>();
            } else if (j["percentHeal"].is_number()) {
                percent_heal_formula = std::to_string(j["percentHeal"].get<int>());
            }
        }
        if (j.contains("hotDuration")) {
            if (j["hotDuration"].is_string()) {
                hot_duration_formula = j["hotDuration"].get<std::string>();
            } else if (j["hotDuration"].is_number()) {
                hot_duration_formula = std::to_string(j["hotDuration"].get<int>());
            }
        }
        if (j.contains("boostsRegen")) boosts_regen = j["boostsRegen"].get<bool>();
        if (j.contains("regenBoost")) {
            if (j["regenBoost"].is_string()) {
                boosts_regen_formula = j["regenBoost"].get<std::string>();
            } else if (j["regenBoost"].is_number()) {
                boosts_regen_formula = std::to_string(j["regenBoost"].get<int>());
            }
        }
        if (j.contains("chance")) {
            if (j["chance"].is_string()) {
                chance_formula = j["chance"].get<std::string>();
            } else if (j["chance"].is_number()) {
                chance_percent = j["chance"].get<int>();
                chance_formula = std::to_string(chance_percent);
            }
        }
        if (j.contains("condition")) condition = j["condition"].get<std::string>();

    } catch (const json::exception& e) {
        Log::warn("Failed to parse effect override JSON: {}", e.what());
    }
}

// =============================================================================
// EffectResult
// =============================================================================

EffectResult EffectResult::success_result(int value, std::string_view attacker_msg,
                                          std::string_view target_msg, std::string_view room_msg) {
    EffectResult result;
    result.success = true;
    result.value = value;
    result.attacker_message = attacker_msg;
    result.target_message = target_msg;
    result.room_message = room_msg;
    return result;
}

EffectResult EffectResult::failure_result(std::string_view reason) {
    EffectResult result;
    result.success = false;
    result.attacker_message = std::string(reason);
    return result;
}

// =============================================================================
// EffectContext
// =============================================================================

void EffectContext::build_formula_context() {
    if (!actor) return;

    const auto& stats = actor->stats();

    formula_ctx.skill_level = skill_level;
    formula_ctx.actor_level = stats.level;

    // Calculate stat bonuses (standard D&D-style: (stat - 10) / 2)
    formula_ctx.str_bonus = (stats.strength - 10) / 2;
    formula_ctx.dex_bonus = (stats.dexterity - 10) / 2;
    formula_ctx.con_bonus = (stats.constitution - 10) / 2;
    formula_ctx.int_bonus = (stats.intelligence - 10) / 2;
    formula_ctx.wis_bonus = (stats.wisdom - 10) / 2;
    formula_ctx.cha_bonus = (stats.charisma - 10) / 2;

    // Actor's detection stats
    formula_ctx.perception = stats.perception;
    formula_ctx.concealment = stats.concealment;

    // Calculate weapon damage from equipped weapon
    formula_ctx.weapon_damage = 0;
    auto weapon = actor->equipment().get_main_weapon();
    if (weapon) {
        const auto& damage_profile = weapon->damage_profile();
        // Use average dice roll as the weapon damage value
        formula_ctx.weapon_damage = damage_profile.base_damage + damage_profile.damage_bonus +
            (damage_profile.dice_count * (damage_profile.dice_sides + 1)) / 2;
    } else {
        // Bare-handed attack - use base unarmed damage (1d4 = ~2.5)
        formula_ctx.weapon_damage = 2;
    }

    if (target) {
        const auto& target_stats = target->stats();
        formula_ctx.target_level = target_stats.level;
        formula_ctx.armor_rating = target_stats.armor_rating;
        formula_ctx.target_perception = target_stats.perception;
        formula_ctx.target_concealment = target_stats.concealment;
    }
}

// =============================================================================
// EffectExecutor
// =============================================================================

bool EffectExecutor::roll_chance(int percent) {
    if (percent >= CHANCE_MAX_PERCENT) return true;
    if (percent <= CHANCE_MIN_PERCENT) return false;

    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, CHANCE_MAX_PERCENT);
    return dist(gen) <= percent;
}

std::expected<EffectResult, Error> EffectExecutor::execute(
    const EffectDefinition& effect_def,
    const EffectParams& params,
    EffectContext& context) {

    // Check chance
    if (!roll_chance(params.chance_percent)) {
        return EffectResult::failure_result("Effect did not trigger");
    }

    // Build formula context
    context.build_formula_context();

    // Execute based on type
    switch (effect_def.type) {
        case EffectType::Damage:
            return execute_damage(params, context);
        case EffectType::Heal:
            return execute_heal(params, context);
        case EffectType::Modify:
            return execute_modify(params, context);
        case EffectType::Interrupt:
            return execute_interrupt(params, context);
        case EffectType::Status:
            return execute_status(params, context);
        case EffectType::Move:
            return execute_move(params, context);
        case EffectType::Dot:
            return execute_dot(params, context);
        case EffectType::Hot:
            return execute_hot(params, context);
        default:
            return std::unexpected(Errors::NotImplemented(
                fmt::format("Effect type '{}' not yet implemented", effect_def.name)));
    }
}

std::expected<std::vector<EffectResult>, Error> EffectExecutor::execute_ability_effects(
    const std::vector<AbilityEffect>& effects,
    const std::unordered_map<int, EffectDefinition>& effect_defs,
    EffectContext& context,
    EffectTrigger trigger_filter) {

    std::vector<EffectResult> results;

    for (const auto& ability_effect : effects) {
        // Filter by trigger
        if (ability_effect.trigger != trigger_filter) {
            continue;
        }

        // Find effect definition
        auto def_it = effect_defs.find(ability_effect.effect_id);
        if (def_it == effect_defs.end()) {
            Log::warn("Effect definition {} not found for ability {}",
                     ability_effect.effect_id, ability_effect.ability_id);
            continue;
        }

        // Set context IDs for this effect execution
        context.ability_id = ability_effect.ability_id;
        context.effect_id = ability_effect.effect_id;

        // Execute the effect
        auto result = execute(def_it->second, ability_effect.params, context);
        if (result) {
            results.push_back(*result);
        } else {
            Log::debug("Effect {} failed: {}", def_it->second.name, result.error().message);
        }
    }

    return results;
}

// =============================================================================
// Effect Implementations
// =============================================================================

std::expected<EffectResult, Error> EffectExecutor::execute_damage(
    const EffectParams& params, EffectContext& context) {

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for damage effect"));
    }

    // Evaluate damage formula
    std::string formula = params.amount_formula;
    if (formula.empty()) {
        formula = "1d6"; // Default
    }

    auto damage_result = FormulaParser::evaluate(formula, context.formula_ctx);
    if (!damage_result) {
        return std::unexpected(damage_result.error());
    }

    int damage = *damage_result;

    // Apply scaling bonus if specified
    if (!params.scaling_stat.empty()) {
        auto bonus = context.formula_ctx.get_variable(params.scaling_stat + "_bonus");
        if (bonus) {
            damage += *bonus;
        }
    }

    // Ensure minimum 1 damage
    damage = std::max(1, damage);

    // Apply damage to target
    context.target->stats().hit_points -= damage;

    // Generate messages
    std::string attacker_msg = fmt::format("You hit {} for {} {} damage!",
        context.target->display_name(), damage, params.damage_type);
    std::string target_msg = fmt::format("{} hits you for {} {} damage!",
        context.actor->display_name(), damage, params.damage_type);
    std::string room_msg = fmt::format("{} hits {} with a {} attack!",
        context.actor->display_name(), context.target->display_name(), params.damage_type);

    return EffectResult::success_result(damage, attacker_msg, target_msg, room_msg);
}

std::expected<EffectResult, Error> EffectExecutor::execute_heal(
    const EffectParams& params, EffectContext& context) {

    std::shared_ptr<Actor> heal_target = context.target ? context.target : context.actor;
    if (!heal_target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for heal effect"));
    }

    // Evaluate heal formula
    std::string formula = params.heal_formula.empty() ? params.amount_formula : params.heal_formula;
    if (formula.empty()) {
        formula = "1d8"; // Default
    }

    auto heal_result = FormulaParser::evaluate(formula, context.formula_ctx);
    if (!heal_result) {
        return std::unexpected(heal_result.error());
    }

    int heal_amount = std::max(1, *heal_result);
    auto& stats = heal_target->stats();

    if (params.heal_resource == "move" || params.heal_resource == "movement") {
        int old_move = stats.movement;
        stats.movement = std::min(stats.movement + heal_amount, stats.max_movement);
        heal_amount = stats.movement - old_move;
    } else {
        // Default to HP
        int old_hp = stats.hit_points;
        stats.hit_points = std::min(stats.hit_points + heal_amount, stats.max_hit_points);
        heal_amount = stats.hit_points - old_hp;
    }

    std::string resource_name = (params.heal_resource == "move") ? "movement" : "health";
    std::string attacker_msg = fmt::format("You restore {} {} to {}.",
        heal_amount, resource_name, heal_target->display_name());
    std::string target_msg = fmt::format("{} restores {} {} to you.",
        context.actor->display_name(), heal_amount, resource_name);
    std::string room_msg = fmt::format("{} heals {}.",
        context.actor->display_name(), heal_target->display_name());

    return EffectResult::success_result(heal_amount, attacker_msg, target_msg, room_msg);
}

std::expected<EffectResult, Error> EffectExecutor::execute_modify(
    const EffectParams& params, EffectContext& context) {

    // Modify effects (like Armor spell) can target self or others
    std::shared_ptr<Actor> effect_target = context.target ? context.target : context.actor;
    if (!effect_target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for modify effect"));
    }

    // Calculate modifier amount from formula
    int modifier = 10;  // Default
    if (!params.amount_formula.empty()) {
        auto mod_result = FormulaParser::evaluate(params.amount_formula, context.formula_ctx);
        if (mod_result) {
            modifier = *mod_result;
        }
    }

    // Calculate duration from formula or use default (in MUD hours)
    int duration_hours = 1;  // Default 1 MUD hour
    if (!params.status_duration_formula.empty()) {
        auto duration_result = FormulaParser::evaluate(params.status_duration_formula, context.formula_ctx);
        if (duration_result) {
            duration_hours = std::max(1, *duration_result);
        }
    } else if (params.status_duration > 0) {
        duration_hours = params.status_duration;
    }

    // Determine the effect name and flag based on damage_type (which holds target type for modify)
    std::string effect_name = "armor";  // Default
    ActorFlag effect_flag = ActorFlag::Armor;
    std::string modifier_stat = "armor_rating";

    // Map common modification targets
    if (params.damage_type == "ward" || params.damage_type == "armor") {
        effect_name = "armor";
        effect_flag = ActorFlag::Armor;
        modifier_stat = "armor_rating";
    } else if (params.damage_type == "hitroll" || params.damage_type == "accuracy") {
        effect_name = "enhanced_accuracy";
        effect_flag = ActorFlag::Bless;
        modifier_stat = "accuracy";
    } else if (params.damage_type == "damroll" || params.damage_type == "attack_power") {
        effect_name = "enhanced_damage";
        effect_flag = ActorFlag::Strength;
        modifier_stat = "attack_power";
    }

    // Create the active effect - use ability display name if available
    ActiveEffect effect;
    effect.name = context.ability_name.empty() ? format_effect_name(effect_name) : context.ability_name;
    effect.source = context.ability_name.empty() ? "spell" : context.ability_name;
    effect.flag = effect_flag;
    effect.duration_hours = duration_hours;
    effect.modifier_value = modifier;
    effect.modifier_stat = modifier_stat;
    effect.applied_at = std::chrono::steady_clock::now();

    // Apply the effect to the target
    effect_target->add_effect(effect);

    // Generate appropriate messages
    std::string attacker_msg, target_msg, room_msg;
    bool self_cast = (effect_target == context.actor);

    if (self_cast) {
        attacker_msg = fmt::format("You feel protected by a magical {} (+{}).",
            effect_name, modifier);
        room_msg = fmt::format("{} is surrounded by a protective aura.",
            context.actor->display_name());
    } else {
        attacker_msg = fmt::format("You protect {} with magical {} (+{})!",
            effect_target->display_name(), effect_name, modifier);
        target_msg = fmt::format("{} protects you with magical {} (+{})!",
            context.actor->display_name(), effect_name, modifier);
        room_msg = fmt::format("{} is surrounded by a protective aura from {}.",
            effect_target->display_name(), context.actor->display_name());
    }

    return EffectResult::success_result(modifier, attacker_msg, target_msg, room_msg);
}

std::expected<EffectResult, Error> EffectExecutor::execute_interrupt(
    const EffectParams& params, EffectContext& context) {

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for interrupt effect"));
    }

    // Evaluate interrupt power
    std::string power_formula = params.interrupt_power_formula;
    if (power_formula.empty()) {
        power_formula = "skill"; // Default
    }

    auto power_result = FormulaParser::evaluate(power_formula, context.formula_ctx);
    if (!power_result) {
        return std::unexpected(power_result.error());
    }

    int power = *power_result;

    std::string attacker_msg;
    std::string target_msg;
    std::string room_msg;

    // Check if target is actually casting (has a casting blackout in effect)
    if (context.target->is_casting_blocked()) {
        // Interrupt successful - clear the casting blackout
        context.target->clear_casting_blackout();

        attacker_msg = fmt::format("You interrupt {}'s spell!", context.target->display_name());
        target_msg = fmt::format("{} interrupts your spell!", context.actor->display_name());
        room_msg = fmt::format("{} interrupts {}'s spellcasting!",
            context.actor->display_name(), context.target->display_name());
    } else {
        // Target wasn't casting - interrupt has no effect
        attacker_msg = fmt::format("You try to interrupt {}, but they aren't casting.",
                                   context.target->display_name());
        target_msg = "";  // No message to target
        room_msg = "";    // No room message
        power = 0;        // No power for failed interrupt
    }

    return EffectResult::success_result(power, attacker_msg, target_msg, room_msg);
}

// Helper to convert status name string to ActorFlag
static std::optional<ActorFlag> parse_status_flag(std::string_view status_name) {
    std::string lower_name = to_lowercase(status_name);

    // Map common status names to flags
    static const std::unordered_map<std::string, ActorFlag> flag_map = {
        // Stealth/detection
        {"hidden", ActorFlag::Hide},
        {"hide", ActorFlag::Hide},
        {"sneak", ActorFlag::Sneak},
        {"sneaking", ActorFlag::Sneak},
        {"invisible", ActorFlag::Invisible},
        {"detect_invis", ActorFlag::Detect_Invis},
        {"detect_invisible", ActorFlag::Detect_Invis},
        {"detect_magic", ActorFlag::Detect_Magic},
        {"detect_align", ActorFlag::Detect_Align},
        {"infravision", ActorFlag::Infravision},
        {"sense_life", ActorFlag::Sense_Life},
        {"aware", ActorFlag::Aware},
        // Buffs
        {"bless", ActorFlag::Bless},
        {"blessed", ActorFlag::Bless},
        {"armor", ActorFlag::Armor},
        {"shield", ActorFlag::Shield},
        {"sanctuary", ActorFlag::Sanctuary},
        {"stoneskin", ActorFlag::Stoneskin},
        {"barkskin", ActorFlag::Barkskin},
        {"haste", ActorFlag::Haste},
        {"blur", ActorFlag::Blur},
        {"fireshield", ActorFlag::Fireshield},
        {"coldshield", ActorFlag::Coldshield},
        // Movement
        {"waterwalk", ActorFlag::Waterwalk},
        {"fly", ActorFlag::Flying},
        {"flying", ActorFlag::Flying},
        // Toggle states
        {"meditate", ActorFlag::Meditating},
        {"meditating", ActorFlag::Meditating},
        {"berserk", ActorFlag::Berserk},
        // Debuffs
        {"slow", ActorFlag::Slow},
        {"taunted", ActorFlag::Taunted},
        {"webbed", ActorFlag::Webbed},
        {"paralyzed", ActorFlag::Paralyzed},
        {"poison", ActorFlag::Poison},
        {"curse", ActorFlag::Curse},
        {"cursed", ActorFlag::Curse},
        {"charm", ActorFlag::Charm},
        {"charmed", ActorFlag::Charm},
        {"sleep", ActorFlag::Sleep},
        {"blind", ActorFlag::Blind},
        {"blindness", ActorFlag::Blind},
        // Visual
        {"glowing", ActorFlag::Glowing},
    };

    auto it = flag_map.find(lower_name);
    if (it != flag_map.end()) {
        return it->second;
    }

    // Return nullopt if no matching flag - effect will still be tracked by name
    return std::nullopt;
}

// Helper to infer which stat a status effect should modify based on the flag
static std::string infer_status_modifier_stat(std::string_view status_name) {
    std::string lower_name = to_lowercase(status_name);

    // Stealth effects modify concealment
    if (lower_name == "hidden" || lower_name == "hide" ||
        lower_name == "sneak" || lower_name == "sneaking" ||
        lower_name == "invisible") {
        return "concealment";
    }

    // Detection effects modify perception
    if (lower_name == "aware" || lower_name == "sense_life" ||
        lower_name == "detect_invis") {
        return "perception";
    }

    // Movement effects modify evasion
    if (lower_name == "haste" || lower_name == "blur") {
        return "evasion";
    }

    // Meditative effects modify focus (spell memorization speed)
    if (lower_name == "meditating" || lower_name == "meditate") {
        return "focus";
    }

    // Default - no stat modification
    return "";
}

std::expected<EffectResult, Error> EffectExecutor::execute_status(
    const EffectParams& params, EffectContext& context) {

    // Status effects can target self if no explicit target
    std::shared_ptr<Actor> effect_target = context.target ? context.target : context.actor;
    if (!effect_target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for status effect"));
    }

    std::string status = params.status_name;
    if (status.empty()) {
        return std::unexpected(Errors::InvalidArgument("status", "name is required"));
    }

    // Calculate duration in MUD hours - toggle abilities use -1 for permanent
    int duration_hours = 1;  // Default 1 MUD hour
    if (params.is_toggle_duration) {
        duration_hours = -1;  // Permanent until toggled off
    } else if (!params.status_duration_formula.empty()) {
        auto duration_result = FormulaParser::evaluate(params.status_duration_formula, context.formula_ctx);
        if (duration_result) {
            duration_hours = std::max(1, *duration_result);
        }
    } else if (params.status_duration != 0) {
        duration_hours = params.status_duration;
    }

    // Calculate modifier value from amount formula if specified
    int modifier_value = 0;
    if (!params.amount_formula.empty()) {
        auto mod_result = FormulaParser::evaluate(params.amount_formula, context.formula_ctx);
        if (mod_result) {
            modifier_value = *mod_result;
        }
    }

    // Determine which stat to modify - use explicit stat or infer from flag
    std::string modifier_stat = params.status_modifier_stat;
    if (modifier_stat.empty() && modifier_value != 0) {
        modifier_stat = infer_status_modifier_stat(status);
    }

    // Parse the flag (optional - effect can exist without a matching flag)
    auto parsed_flag = parse_status_flag(status);

    // Create the active effect - use ability display name if available
    ActiveEffect effect;
    effect.name = context.ability_name.empty() ? format_effect_name(status) : context.ability_name;
    effect.source = context.ability_name.empty() ? "spell" : context.ability_name;  // Could be spell name, item, etc.
    effect.flag = parsed_flag.value_or(ActorFlag::Bless);  // Use Bless as placeholder if no match
    effect.duration_hours = duration_hours;
    effect.modifier_value = modifier_value;
    effect.modifier_stat = modifier_stat;
    effect.applied_at = std::chrono::steady_clock::now();

    // If we have a valid flag, set it on the actor for quick flag checks
    if (parsed_flag.has_value()) {
        effect_target->set_flag(*parsed_flag, true);
    }

    // Apply the effect to the target
    effect_target->add_effect(effect);

    // Generate appropriate messages (fallback - ability executor uses database messages when available)
    std::string attacker_msg, target_msg, room_msg;
    bool self_cast = (effect_target == context.actor);
    std::string formatted_status = format_effect_name(status);

    if (self_cast) {
        if (modifier_value > 0) {
            attacker_msg = fmt::format("You are now {} (+{} {}).",
                formatted_status, modifier_value, modifier_stat);
        } else {
            attacker_msg = fmt::format("You are now {}.", formatted_status);
        }
        room_msg = fmt::format("{} is now {}.",
            context.actor->display_name(), formatted_status);
    } else {
        attacker_msg = fmt::format("You apply {} to {}!",
            formatted_status, effect_target->display_name());
        target_msg = fmt::format("{} applies {} to you!",
            context.actor->display_name(), formatted_status);
        room_msg = fmt::format("{} applies {} to {}!",
            context.actor->display_name(), formatted_status, effect_target->display_name());
    }

    return EffectResult::success_result(modifier_value > 0 ? modifier_value : duration_hours,
                                        attacker_msg, target_msg, room_msg);
}

std::expected<EffectResult, Error> EffectExecutor::execute_move(
    const EffectParams& params, EffectContext& context) {

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for move effect"));
    }

    std::string move_type = params.move_type;
    if (move_type.empty()) {
        move_type = "knockback";
    }

    // Get target's current room
    auto target_room = context.target->current_room();
    if (!target_room) {
        return std::unexpected(Errors::InvalidState("Target has no room"));
    }

    // Cardinal directions to check
    static const std::array<Direction, 6> cardinal_dirs = {
        Direction::North, Direction::East, Direction::South,
        Direction::West, Direction::Up, Direction::Down
    };

    // Determine direction based on move type
    Direction move_dir = Direction::None;
    bool is_pull = (move_type == "pull" || move_type == "drag");

    if (is_pull) {
        // Pull moves target toward the actor
        // Find which exit from target's room leads to actor's room
        if (context.actor && context.actor->current_room()) {
            auto actor_room = context.actor->current_room();
            for (Direction d : cardinal_dirs) {
                auto exit = target_room->get_exit(d);
                if (exit && exit->to_room == actor_room->id()) {
                    move_dir = d;
                    break;
                }
            }
        }
    } else {
        // Knockback moves target away from actor - pick a random valid exit
        static thread_local std::mt19937 gen{std::random_device{}()};
        std::vector<Direction> valid_exits;
        for (Direction d : cardinal_dirs) {
            auto exit = target_room->get_exit(d);
            if (exit && exit->to_room.is_valid()) {
                valid_exits.push_back(d);
            }
        }
        if (!valid_exits.empty()) {
            std::uniform_int_distribution<size_t> dist(0, valid_exits.size() - 1);
            move_dir = valid_exits[dist(gen)];
        }
    }

    std::string attacker_msg;
    std::string target_msg;
    std::string room_msg;

    // Execute the movement if we found a valid direction
    if (move_dir != Direction::None) {
        auto exit = target_room->get_exit(move_dir);
        auto dest_room = WorldManager::instance().get_room(exit->to_room);
        if (dest_room) {
            // Move the target
            target_room->remove_actor(context.target->id());
            dest_room->add_actor(context.target);
            context.target->set_current_room(dest_room);

            std::string dir_name{RoomUtils::get_direction_name(move_dir)};
            attacker_msg = fmt::format("You {} {} {}!",
                move_type, context.target->display_name(), dir_name);
            target_msg = fmt::format("{} {} you {}!",
                context.actor->display_name(), is_pull ? "pulls" : "knocks", dir_name);
            room_msg = fmt::format("{} is {} {} by {}!",
                context.target->display_name(), is_pull ? "pulled" : "knocked", dir_name,
                context.actor->display_name());

            // Send arrival message to new room
            Direction opposite = RoomUtils::get_opposite_direction(move_dir);
            for (const auto& actor : dest_room->contents().actors) {
                if (actor && actor.get() != context.target.get()) {
                    actor->receive_message(fmt::format("{} stumbles in from {}!",
                        context.target->display_name(),
                        RoomUtils::get_direction_name(opposite)));
                }
            }
        } else {
            attacker_msg = fmt::format("You try to {} {}, but they hold their ground.",
                move_type, context.target->display_name());
            target_msg = "";
            room_msg = "";
        }
    } else {
        attacker_msg = fmt::format("You try to {} {}, but there's nowhere for them to go.",
            move_type, context.target->display_name());
        target_msg = "";
        room_msg = "";
    }

    return EffectResult::success_result(params.move_distance, attacker_msg, target_msg, room_msg);
}

std::expected<EffectResult, Error> EffectExecutor::execute_dot(
    const EffectParams& params, EffectContext& context) {

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for DoT effect"));
    }

    // Evaluate potency from formula
    int potency = 5;  // Default
    if (!params.potency_formula.empty()) {
        auto potency_result = FormulaParser::evaluate(params.potency_formula, context.formula_ctx);
        if (potency_result) {
            potency = std::clamp(*potency_result, 1, 10);  // Clamp to 1-10 range
        }
    }

    // Evaluate flat damage per tick
    int flat_damage = 0;
    if (!params.flat_damage_formula.empty()) {
        auto flat_result = FormulaParser::evaluate(params.flat_damage_formula, context.formula_ctx);
        if (flat_result) {
            flat_damage = std::max(0, *flat_result);
        }
    }

    // Evaluate percent damage per tick
    int percent_damage = 0;
    if (!params.percent_damage_formula.empty()) {
        auto pct_result = FormulaParser::evaluate(params.percent_damage_formula, context.formula_ctx);
        if (pct_result) {
            percent_damage = std::clamp(*pct_result, 0, 100);
        }
    }

    // Evaluate duration in ticks
    int duration = -1;  // Default: permanent until cured
    if (!params.dot_duration_formula.empty()) {
        auto dur_result = FormulaParser::evaluate(params.dot_duration_formula, context.formula_ctx);
        if (dur_result) {
            duration = std::max(1, *dur_result);
        }
    }

    // Evaluate regen reduction
    int reduces_regen = 0;
    if (!params.reduces_regen_formula.empty()) {
        auto regen_result = FormulaParser::evaluate(params.reduces_regen_formula, context.formula_ctx);
        if (regen_result) {
            reduces_regen = std::clamp(*regen_result, 0, 100);
        }
    }

    // Create the DoT effect with resolved parameters
    fiery::DotEffect dot;
    dot.ability_id = context.ability_id;
    dot.effect_id = context.effect_id;
    dot.effect_type = "dot";
    dot.damage_type = params.damage_type;
    dot.cure_category = params.cure_category;
    dot.potency = potency;
    dot.flat_damage = flat_damage;
    dot.percent_damage = percent_damage;
    dot.blocks_regen = params.blocks_regen;
    dot.reduces_regen = reduces_regen;
    dot.max_resistance = params.max_resistance;
    dot.remaining_ticks = duration;
    dot.tick_interval = params.tick_interval;
    dot.ticks_since_last = 0;
    if (context.actor && context.actor->id().is_valid()) {
        dot.source_actor_id = fmt::format("{}:{}",
            context.actor->id().zone_id(),
            context.actor->id().local_id());
    }
    dot.source_level = context.formula_ctx.actor_level;
    dot.stack_count = 1;
    dot.max_stacks = params.max_stacks;
    dot.stackable = params.stackable;

    // Apply the DoT effect to the target
    context.target->add_dot_effect(dot);

    // Generate messages
    std::string effect_desc = params.cure_category;
    if (effect_desc == "poison") {
        effect_desc = "poisoned";
    } else if (effect_desc == "fire") {
        effect_desc = "burning";
    } else if (effect_desc == "disease") {
        effect_desc = "diseased";
    } else if (effect_desc == "bleed") {
        effect_desc = "bleeding";
    } else if (effect_desc == "acid") {
        effect_desc = "corroding";
    }

    std::string attacker_msg = fmt::format("You inflict {} on {}!",
        effect_desc, context.target->display_name());
    std::string target_msg = fmt::format("{} inflicts {} on you!",
        context.actor->display_name(), effect_desc);
    std::string room_msg = fmt::format("{} inflicts {} on {}!",
        context.actor->display_name(), effect_desc, context.target->display_name());

    return EffectResult::success_result(potency, attacker_msg, target_msg, room_msg);
}

std::expected<EffectResult, Error> EffectExecutor::execute_hot(
    const EffectParams& params, EffectContext& context) {

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for HoT effect"));
    }

    // Parse formulas to get resolved values
    FormulaParser parser;

    // Calculate flat healing per tick
    int flat_heal = 0;
    if (!params.flat_heal_formula.empty()) {
        auto result = parser.evaluate(params.flat_heal_formula, context.formula_ctx);
        if (result) {
            flat_heal = static_cast<int>(*result);
        }
    }

    // Calculate percent healing per tick
    int percent_heal = 0;
    if (!params.percent_heal_formula.empty()) {
        auto result = parser.evaluate(params.percent_heal_formula, context.formula_ctx);
        if (result) {
            percent_heal = static_cast<int>(*result);
        }
    }

    // Calculate duration
    int duration = 10; // Default 10 ticks
    if (!params.hot_duration_formula.empty()) {
        auto result = parser.evaluate(params.hot_duration_formula, context.formula_ctx);
        if (result) {
            duration = static_cast<int>(*result);
        }
    }

    // Calculate regen boost percentage
    int regen_boost = 0;
    if (params.boosts_regen && !params.boosts_regen_formula.empty()) {
        auto result = parser.evaluate(params.boosts_regen_formula, context.formula_ctx);
        if (result) {
            regen_boost = static_cast<int>(*result);
        }
    }

    // Create the HoT effect
    fiery::HotEffect hot;
    hot.ability_id = context.ability_id;
    hot.effect_id = context.effect_id;
    hot.effect_type = "hot";
    hot.heal_type = params.heal_resource;
    hot.hot_category = params.hot_category;
    hot.flat_heal = flat_heal;
    hot.percent_heal = percent_heal;
    hot.boosts_regen = params.boosts_regen;
    hot.regen_boost = regen_boost;
    hot.remaining_ticks = duration;
    hot.tick_interval = params.tick_interval;
    hot.ticks_since_last = 0;
    if (context.actor && context.actor->id().is_valid()) {
        hot.source_actor_id = fmt::format("{}:{}",
            context.actor->id().zone_id(),
            context.actor->id().local_id());
    }
    hot.source_level = context.formula_ctx.actor_level;
    hot.stack_count = 1;
    hot.max_stacks = params.max_stacks;
    hot.stackable = params.stackable;

    // Apply the HoT effect to the target
    context.target->add_hot_effect(hot);

    // Generate messages based on heal type
    std::string effect_desc = params.hot_category;
    if (effect_desc == "heal") {
        effect_desc = "healing";
    } else if (effect_desc == "regen" || effect_desc == "regeneration") {
        effect_desc = "regenerating";
    } else if (effect_desc == "divine") {
        effect_desc = "divine healing";
    } else if (effect_desc == "nature") {
        effect_desc = "natural healing";
    }

    std::string attacker_msg;
    std::string target_msg;
    std::string room_msg;

    if (context.actor == context.target) {
        // Self-cast
        attacker_msg = fmt::format("You begin {}.", effect_desc);
        room_msg = fmt::format("{} begins {}.",
            context.actor->display_name(), effect_desc);
    } else {
        attacker_msg = fmt::format("You grant {} to {}!",
            effect_desc, context.target->display_name());
        target_msg = fmt::format("{} grants you {}!",
            context.actor->display_name(), effect_desc);
        room_msg = fmt::format("{} grants {} to {}!",
            context.actor->display_name(), effect_desc, context.target->display_name());
    }

    return EffectResult::success_result(flat_heal, attacker_msg, target_msg, room_msg);
}

} // namespace FieryMUD

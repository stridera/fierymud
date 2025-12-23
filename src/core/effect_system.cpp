#include "core/effect_system.hpp"

#include "core/actor.hpp"
#include "core/logging.hpp"
#include "text/string_utils.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>
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
        if (j.contains("duration")) {
            if (j["duration"].is_string()) {
                params.status_duration_formula = j["duration"].get<std::string>();
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
        if (j.contains("duration")) {
            if (j["duration"].is_string()) {
                status_duration_formula = j["duration"].get<std::string>();
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

    if (target) {
        const auto& target_stats = target->stats();
        formula_ctx.target_level = target_stats.level;
        formula_ctx.armor_rating = target_stats.armor_rating;
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

    // Calculate duration from formula or use default
    int duration = 10;  // Default 10 rounds
    if (!params.status_duration_formula.empty()) {
        auto duration_result = FormulaParser::evaluate(params.status_duration_formula, context.formula_ctx);
        if (duration_result) {
            duration = std::max(1, *duration_result);
        }
    } else if (params.status_duration > 0) {
        duration = params.status_duration;
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
    effect.source = "spell";
    effect.flag = effect_flag;
    effect.duration_rounds = duration;
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

    // TODO: Actually interrupt spellcasting if target is casting
    // For now, just report the interrupt attempt

    std::string attacker_msg = fmt::format("You interrupt {}!", context.target->display_name());
    std::string target_msg = fmt::format("{} interrupts you!", context.actor->display_name());
    std::string room_msg = fmt::format("{} interrupts {}!",
        context.actor->display_name(), context.target->display_name());

    return EffectResult::success_result(power, attacker_msg, target_msg, room_msg);
}

// Helper to convert status name string to ActorFlag
static ActorFlag parse_status_flag(std::string_view status_name) {
    std::string lower_name = to_lowercase(status_name);

    // Map common status names to flags
    static const std::unordered_map<std::string, ActorFlag> flag_map = {
        {"bless", ActorFlag::Bless},
        {"blessed", ActorFlag::Bless},
        {"armor", ActorFlag::Armor},
        {"shield", ActorFlag::Shield},
        {"sanctuary", ActorFlag::Sanctuary},
        {"invisible", ActorFlag::Invisible},
        {"detect_invis", ActorFlag::Detect_Invis},
        {"detect_magic", ActorFlag::Detect_Magic},
        {"detect_align", ActorFlag::Detect_Align},
        {"infravision", ActorFlag::Infravision},
        {"sense_life", ActorFlag::Sense_Life},
        {"waterwalk", ActorFlag::Waterwalk},
        {"fly", ActorFlag::Flying},
        {"flying", ActorFlag::Flying},
        {"haste", ActorFlag::Haste},
        {"slow", ActorFlag::Slow},
        {"blur", ActorFlag::Blur},
        {"stoneskin", ActorFlag::Stoneskin},
        {"fireshield", ActorFlag::Fireshield},
        {"coldshield", ActorFlag::Coldshield},
        {"aware", ActorFlag::Aware},
        {"berserk", ActorFlag::Berserk},
        {"taunted", ActorFlag::Taunted},
        {"webbed", ActorFlag::Webbed},
        {"paralyzed", ActorFlag::Paralyzed},
        {"glowing", ActorFlag::Glowing},
        {"poison", ActorFlag::Poison},
        {"curse", ActorFlag::Curse},
        {"cursed", ActorFlag::Curse},
        {"charm", ActorFlag::Charm},
        {"charmed", ActorFlag::Charm},
        {"sleep", ActorFlag::Sleep},
        {"blind", ActorFlag::Blind},
        {"blindness", ActorFlag::Blind},
    };

    auto it = flag_map.find(lower_name);
    if (it != flag_map.end()) {
        return it->second;
    }

    // Default - we'll still track the effect even without a matching flag
    return ActorFlag::Bless;  // Placeholder, will be handled specially
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

    // Calculate duration from formula or use default
    int duration = 10;  // Default 10 rounds
    if (!params.status_duration_formula.empty()) {
        auto duration_result = FormulaParser::evaluate(params.status_duration_formula, context.formula_ctx);
        if (duration_result) {
            duration = std::max(1, *duration_result);
        }
    } else if (params.status_duration > 0) {
        duration = params.status_duration;
    }

    // Create the active effect - use ability display name if available
    ActiveEffect effect;
    effect.name = context.ability_name.empty() ? format_effect_name(status) : context.ability_name;
    effect.source = "spell";  // Could be spell name, item, etc.
    effect.flag = parse_status_flag(status);
    effect.duration_rounds = duration;
    effect.modifier_value = 0;
    effect.modifier_stat = "";
    effect.applied_at = std::chrono::steady_clock::now();

    // Apply the effect to the target
    effect_target->add_effect(effect);

    // Generate appropriate messages
    std::string attacker_msg, target_msg, room_msg;
    bool self_cast = (effect_target == context.actor);

    if (self_cast) {
        attacker_msg = fmt::format("You are now affected by {}.", status);
        room_msg = fmt::format("{} is now affected by {}.",
            context.actor->display_name(), status);
    } else {
        attacker_msg = fmt::format("You cast {} on {}!",
            status, effect_target->display_name());
        target_msg = fmt::format("{} casts {} on you!",
            context.actor->display_name(), status);
        room_msg = fmt::format("{} casts {} on {}!",
            context.actor->display_name(), status, effect_target->display_name());
    }

    return EffectResult::success_result(duration, attacker_msg, target_msg, room_msg);
}

std::expected<EffectResult, Error> EffectExecutor::execute_move(
    const EffectParams& params, EffectContext& context) {

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for move effect"));
    }

    // TODO: Implement knockback/pull movement
    std::string move_type = params.move_type;
    if (move_type.empty()) {
        move_type = "knockback";
    }

    std::string attacker_msg = fmt::format("You {} {}!",
        move_type, context.target->display_name());
    std::string target_msg = fmt::format("{} {} you!",
        context.actor->display_name(), move_type);
    std::string room_msg = fmt::format("{} {} {}!",
        context.actor->display_name(), move_type, context.target->display_name());

    return EffectResult::success_result(params.move_distance, attacker_msg, target_msg, room_msg);
}

} // namespace FieryMUD

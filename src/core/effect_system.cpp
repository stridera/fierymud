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

// =============================================================================
// Dice Roll Display Helpers
// =============================================================================

/**
 * Format spell damage formula details for display.
 * Shows the formula evaluated and the final result.
 */
static std::string format_spell_damage_details(
    std::string_view formula,
    int final_damage,
    std::string_view damage_type) {

    // Use red for damage spells
    return fmt::format("\n  <red>[Spell: {} = {} {}]</>", formula, final_damage, damage_type);
}

/**
 * Format heal formula details for display.
 * Shows the formula evaluated and the final result.
 */
static std::string format_heal_details(
    std::string_view formula,
    int heal_amount,
    std::string_view resource) {

    // Use green for heals
    return fmt::format("\n  <green>[Heal: {} = {} {}]</>", formula, heal_amount, resource);
}

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

        // Teleport params
        if (j.contains("teleportType")) params.teleport_type = j["teleportType"].get<std::string>();
        if (j.contains("teleport_type")) params.teleport_type = j["teleport_type"].get<std::string>();
        if (j.contains("roomId")) params.teleport_room_id = j["roomId"].get<int>();
        if (j.contains("room_id")) params.teleport_room_id = j["room_id"].get<int>();
        if (j.contains("successChance")) {
            if (j["successChance"].is_string()) {
                params.success_formula = j["successChance"].get<std::string>();
            } else if (j["successChance"].is_number()) {
                params.success_formula = std::to_string(j["successChance"].get<int>());
            }
        }

        // Reveal params
        if (j.contains("revealType")) params.reveal_type = j["revealType"].get<std::string>();
        if (j.contains("reveal_type")) params.reveal_type = j["reveal_type"].get<std::string>();
        if (j.contains("clearFog")) params.clear_fog = j["clearFog"].get<bool>();

        // Cleanse params
        if (j.contains("cleanseCategory")) params.cleanse_category = j["cleanseCategory"].get<std::string>();
        if (j.contains("cleanse_category")) params.cleanse_category = j["cleanse_category"].get<std::string>();
        if (j.contains("cleansePower")) {
            if (j["cleansePower"].is_string()) {
                params.cleanse_power_formula = j["cleansePower"].get<std::string>();
            } else if (j["cleansePower"].is_number()) {
                params.cleanse_power_formula = std::to_string(j["cleansePower"].get<int>());
            }
        }
        if (j.contains("maxEffectsRemoved")) params.max_effects_removed = j["maxEffectsRemoved"].get<int>();

        // Dispel params
        if (j.contains("dispelType")) params.dispel_type = j["dispelType"].get<std::string>();
        if (j.contains("dispel_type")) params.dispel_type = j["dispel_type"].get<std::string>();
        if (j.contains("dispelPower")) {
            if (j["dispelPower"].is_string()) {
                params.dispel_power_formula = j["dispelPower"].get<std::string>();
            } else if (j["dispelPower"].is_number()) {
                params.dispel_power_formula = std::to_string(j["dispelPower"].get<int>());
            }
        }
        if (j.contains("dispelObjects")) params.dispel_objects = j["dispelObjects"].get<bool>();

        // Summon params
        if (j.contains("summonType")) params.summon_type = j["summonType"].get<std::string>();
        if (j.contains("summon_type")) params.summon_type = j["summon_type"].get<std::string>();
        if (j.contains("maxDistance")) {
            if (j["maxDistance"].is_string()) {
                params.max_distance_formula = j["maxDistance"].get<std::string>();
            } else if (j["maxDistance"].is_number()) {
                params.max_distance_formula = std::to_string(j["maxDistance"].get<int>());
            }
        }
        if (j.contains("maxLevel")) {
            if (j["maxLevel"].is_string()) {
                params.max_level_formula = j["maxLevel"].get<std::string>();
            } else if (j["maxLevel"].is_number()) {
                params.max_level_formula = std::to_string(j["maxLevel"].get<int>());
            }
        }
        if (j.contains("requiresConsent")) params.requires_consent = j["requiresConsent"].get<bool>();

        // Resurrect params
        if (j.contains("expReturn")) {
            if (j["expReturn"].is_string()) {
                params.exp_return_formula = j["expReturn"].get<std::string>();
            } else if (j["expReturn"].is_number()) {
                params.exp_return_formula = std::to_string(j["expReturn"].get<int>());
            }
        }
        if (j.contains("requiresCorpse")) params.requires_corpse = j["requiresCorpse"].get<bool>();

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

        // Teleport params
        if (j.contains("teleportType")) teleport_type = j["teleportType"].get<std::string>();
        if (j.contains("teleport_type")) teleport_type = j["teleport_type"].get<std::string>();
        if (j.contains("roomId")) teleport_room_id = j["roomId"].get<int>();
        if (j.contains("room_id")) teleport_room_id = j["room_id"].get<int>();
        if (j.contains("successChance")) {
            if (j["successChance"].is_string()) {
                success_formula = j["successChance"].get<std::string>();
            } else if (j["successChance"].is_number()) {
                success_formula = std::to_string(j["successChance"].get<int>());
            }
        }

        // Reveal params
        if (j.contains("revealType")) reveal_type = j["revealType"].get<std::string>();
        if (j.contains("reveal_type")) reveal_type = j["reveal_type"].get<std::string>();
        if (j.contains("clearFog")) clear_fog = j["clearFog"].get<bool>();

        // Cleanse params
        if (j.contains("cleanseCategory")) cleanse_category = j["cleanseCategory"].get<std::string>();
        if (j.contains("cleanse_category")) cleanse_category = j["cleanse_category"].get<std::string>();
        if (j.contains("cleansePower")) {
            if (j["cleansePower"].is_string()) {
                cleanse_power_formula = j["cleansePower"].get<std::string>();
            } else if (j["cleansePower"].is_number()) {
                cleanse_power_formula = std::to_string(j["cleansePower"].get<int>());
            }
        }
        if (j.contains("maxEffectsRemoved")) max_effects_removed = j["maxEffectsRemoved"].get<int>();

        // Dispel params
        if (j.contains("dispelType")) dispel_type = j["dispelType"].get<std::string>();
        if (j.contains("dispel_type")) dispel_type = j["dispel_type"].get<std::string>();
        if (j.contains("dispelPower")) {
            if (j["dispelPower"].is_string()) {
                dispel_power_formula = j["dispelPower"].get<std::string>();
            } else if (j["dispelPower"].is_number()) {
                dispel_power_formula = std::to_string(j["dispelPower"].get<int>());
            }
        }
        if (j.contains("dispelObjects")) dispel_objects = j["dispelObjects"].get<bool>();

        // Summon params
        if (j.contains("summonType")) summon_type = j["summonType"].get<std::string>();
        if (j.contains("summon_type")) summon_type = j["summon_type"].get<std::string>();
        if (j.contains("maxDistance")) {
            if (j["maxDistance"].is_string()) {
                max_distance_formula = j["maxDistance"].get<std::string>();
            } else if (j["maxDistance"].is_number()) {
                max_distance_formula = std::to_string(j["maxDistance"].get<int>());
            }
        }
        if (j.contains("maxLevel")) {
            if (j["maxLevel"].is_string()) {
                max_level_formula = j["maxLevel"].get<std::string>();
            } else if (j["maxLevel"].is_number()) {
                max_level_formula = std::to_string(j["maxLevel"].get<int>());
            }
        }
        if (j.contains("requiresConsent")) requires_consent = j["requiresConsent"].get<bool>();

        // Resurrect params
        if (j.contains("expReturn")) {
            if (j["expReturn"].is_string()) {
                exp_return_formula = j["expReturn"].get<std::string>();
            } else if (j["expReturn"].is_number()) {
                exp_return_formula = std::to_string(j["expReturn"].get<int>());
            }
        }
        if (j.contains("expPenalty")) {
            // Convert penalty (0.1 = 10% lost) to return (90% returned)
            if (j["expPenalty"].is_number()) {
                double penalty = j["expPenalty"].get<double>();
                int return_pct = static_cast<int>((1.0 - penalty) * 100);
                exp_return_formula = std::to_string(return_pct);
            }
        }
        if (j.contains("hpPercent")) {
            if (j["hpPercent"].is_number()) {
                // Convert 0.5 to "50" percent
                double pct = j["hpPercent"].get<double>();
                hp_percent_formula = std::to_string(static_cast<int>(pct * 100));
            } else if (j["hpPercent"].is_string()) {
                hp_percent_formula = j["hpPercent"].get<std::string>();
            }
        }
        if (j.contains("requiresCorpse")) requires_corpse = j["requiresCorpse"].get<bool>();

        // Modify params (stat buffs/debuffs)
        if (j.contains("target")) modify_target = j["target"].get<std::string>();
        if (j.contains("amount")) {
            // Also store in modify_amount for Modify effects (amount_formula is set above)
            if (j["amount"].is_string()) {
                modify_amount = j["amount"].get<std::string>();
            } else if (j["amount"].is_number()) {
                modify_amount = std::to_string(j["amount"].get<int>());
            }
        }
        if (j.contains("duration")) {
            // Also store in modify_duration for Modify effects
            if (j["duration"].is_number()) {
                modify_duration = j["duration"].get<int>();
            } else if (j["duration"].is_string()) {
                std::string dur_str = j["duration"].get<std::string>();
                if (dur_str != "toggle") {
                    try {
                        modify_duration = std::stoi(dur_str);
                    } catch (...) {
                        // Formula-based duration, leave as 0
                    }
                }
            }
        }
        if (j.contains("durationUnit")) modify_duration_unit = j["durationUnit"].get<std::string>();

    } catch (const json::exception& e) {
        Log::warn("Failed to parse effect override JSON: {}", e.what());
    }
}

// =============================================================================
// EffectResult
// =============================================================================

EffectResult EffectResult::success_result(int value, std::string_view attacker_msg,
                                          std::string_view target_msg, std::string_view room_msg,
                                          std::string_view dice_details,
                                          EffectType effect_type) {
    EffectResult result;
    result.success = true;
    result.value = value;
    result.type = effect_type;
    result.attacker_message = attacker_msg;
    result.target_message = target_msg;
    result.room_message = room_msg;
    result.dice_details = dice_details;
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

    // Calculate base_damage for spell/ability formulas
    // Formula: level + (circle * 2) + relevant stat bonus
    // For spells, use the higher of INT or WIS bonus
    int spell_stat_bonus = std::max(formula_ctx.int_bonus, formula_ctx.wis_bonus);
    formula_ctx.base_damage = stats.level + (spell_circle * 2) + spell_stat_bonus;
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
        case EffectType::Teleport:
            return execute_teleport(params, context);
        case EffectType::Reveal:
            return execute_reveal(params, context);
        case EffectType::Cleanse:
            return execute_cleanse(params, context);
        case EffectType::Dispel:
            return execute_dispel(params, context);
        case EffectType::Summon:
            return execute_summon(params, context);
        case EffectType::Resurrect:
            return execute_resurrect(params, context);
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

    Log::game()->debug("execute_ability_effects: {} effects, filter={}, target={}",
                       effects.size(),
                       static_cast<int>(trigger_filter),
                       context.target ? context.target->display_name() : "none");

    for (const auto& ability_effect : effects) {
        // Filter by trigger
        if (ability_effect.trigger != trigger_filter) {
            Log::game()->info("  Skipping effect {} (trigger {} != filter {})",
                               ability_effect.effect_id,
                               static_cast<int>(ability_effect.trigger),
                               static_cast<int>(trigger_filter));
            continue;
        }

        // Find effect definition
        auto def_it = effect_defs.find(ability_effect.effect_id);
        if (def_it == effect_defs.end()) {
            Log::warn("Effect definition {} not found for ability {}",
                     ability_effect.effect_id, ability_effect.ability_id);
            continue;
        }

        Log::game()->info("  Executing effect {} (type={}, chance={}%)",
                           def_it->second.name,
                           static_cast<int>(def_it->second.type),
                           ability_effect.params.chance_percent);

        // Set context IDs for this effect execution
        context.ability_id = ability_effect.ability_id;
        context.effect_id = ability_effect.effect_id;

        // Execute the effect
        auto result = execute(def_it->second, ability_effect.params, context);
        if (result) {
            Log::game()->info("  Effect {} succeeded: value={}", def_it->second.name, result->value);
            results.push_back(*result);
        } else {
            Log::game()->info("Effect {} failed: {}", def_it->second.name, result.error().message);
        }
    }

    Log::game()->debug("execute_ability_effects: {} results", results.size());
    return results;
}

// =============================================================================
// Effect Implementations
// =============================================================================

std::expected<EffectResult, Error> EffectExecutor::execute_damage(
    const EffectParams& params, EffectContext& context) {

    Log::game()->info("execute_damage: target={}, formula={}",
                       context.target ? context.target->display_name() : "none",
                       params.amount_formula);

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
        Log::game()->warn("execute_damage: formula '{}' evaluation failed: {}", formula, damage_result.error().message);
        return std::unexpected(damage_result.error());
    }

    int damage = *damage_result;
    Log::game()->info("execute_damage: base damage={}", damage);

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

    // Generate dice details string (stored separately, appended by ability_executor when needed)
    std::string dice_details = format_spell_damage_details(formula, damage, params.damage_type);

    return EffectResult::success_result(damage, attacker_msg, target_msg, room_msg, dice_details, EffectType::Damage);
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

    if (params.heal_resource == "stamina" || params.heal_resource == "move" || params.heal_resource == "movement") {
        int old_stamina = stats.stamina;
        stats.stamina = std::min(stats.stamina + heal_amount, stats.max_stamina);
        heal_amount = stats.stamina - old_stamina;
    } else {
        // Default to HP
        int old_hp = stats.hit_points;
        stats.hit_points = std::min(stats.hit_points + heal_amount, stats.max_hit_points);
        heal_amount = stats.hit_points - old_hp;
    }

    std::string resource_name = (params.heal_resource == "stamina" || params.heal_resource == "move") ? "stamina" : "health";
    std::string attacker_msg = fmt::format("You restore {} {} to {}.",
        heal_amount, resource_name, heal_target->display_name());
    std::string target_msg = fmt::format("{} restores {} {} to you.",
        context.actor->display_name(), heal_amount, resource_name);
    std::string room_msg = fmt::format("{} heals {}.",
        context.actor->display_name(), heal_target->display_name());

    // Generate dice details string (stored separately, appended by ability_executor when needed)
    std::string dice_details = format_heal_details(formula, heal_amount, resource_name);

    return EffectResult::success_result(heal_amount, attacker_msg, target_msg, room_msg, dice_details, EffectType::Heal);
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
            context.target->move_to(dest_room);

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

std::expected<EffectResult, Error> EffectExecutor::execute_teleport(
    const EffectParams& params, EffectContext& context) {

    // Teleport can target self or others
    std::shared_ptr<Actor> teleport_target = context.target ? context.target : context.actor;
    if (!teleport_target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for teleport effect"));
    }

    // Get current room
    auto current_room = teleport_target->current_room();
    if (!current_room) {
        return std::unexpected(Errors::InvalidState("Target has no room"));
    }

    // Evaluate success chance
    int success_chance = 90;  // Default 90%
    if (!params.success_formula.empty()) {
        auto result = FormulaParser::evaluate(params.success_formula, context.formula_ctx);
        if (result) {
            success_chance = std::clamp(*result, 0, 100);
        }
    }

    // Roll for success
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, 100);
    if (dist(gen) > success_chance) {
        return EffectResult::failure_result("The teleportation fizzles and fails!");
    }

    std::shared_ptr<Room> dest_room;
    std::string teleport_desc;

    if (params.teleport_type == "recall" || params.teleport_type == "home") {
        // Teleport to a default recall location (zone 30, room 0 - Midgaard Temple)
        // In the future, this could be stored per-actor
        EntityId recall_room_id(30, 0);  // Default recall point
        dest_room = WorldManager::instance().get_room(recall_room_id);
        if (!dest_room) {
            return std::unexpected(Errors::InvalidState("Recall destination does not exist"));
        }
        teleport_desc = "recalled home";
    } else if (params.teleport_type == "fixed" && params.teleport_room_id > 0) {
        // Teleport to a fixed destination
        // Parse the room ID - could be a composite (zone_id * 100 + local_id) or just local
        int zone_id = params.teleport_room_id / 100;
        int local_id = params.teleport_room_id % 100;
        EntityId room_id(zone_id, local_id);
        dest_room = WorldManager::instance().get_room(room_id);
        teleport_desc = "transported";
    } else {
        // Random teleport within the same zone
        EntityId zone_entity_id(current_room->id().zone_id(), 0);
        auto zone_rooms = WorldManager::instance().get_rooms_in_zone(zone_entity_id);

        if (zone_rooms.empty()) {
            return std::unexpected(Errors::InvalidState("No valid rooms in zone"));
        }

        // Try up to 100 times to find a valid destination
        constexpr int MAX_ATTEMPTS = 100;
        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
            std::uniform_int_distribution<size_t> room_dist(0, zone_rooms.size() - 1);
            auto candidate = zone_rooms[room_dist(gen)];

            // Skip private or death rooms (check room flags if available)
            if (candidate && candidate.get() != current_room.get()) {
                dest_room = candidate;
                break;
            }
        }

        if (!dest_room) {
            return std::unexpected(Errors::InvalidState("Could not find valid teleport destination"));
        }
        teleport_desc = "teleported";
    }

    if (!dest_room) {
        return std::unexpected(Errors::InvalidState("Destination room does not exist"));
    }

    // Perform the teleport
    current_room->remove_actor(teleport_target->id());
    dest_room->add_actor(teleport_target);
    teleport_target->move_to(dest_room);

    // Generate messages
    std::string attacker_msg, target_msg, room_msg;
    bool self_cast = (teleport_target == context.actor);

    if (self_cast) {
        attacker_msg = fmt::format("You are {} in a flash of light!", teleport_desc);
        room_msg = fmt::format("{} disappears in a flash of light!",
            context.actor->display_name());
    } else {
        attacker_msg = fmt::format("You teleport {} away!", teleport_target->display_name());
        target_msg = fmt::format("{} teleports you away!", context.actor->display_name());
        room_msg = fmt::format("{} disappears in a flash of light!",
            teleport_target->display_name());
    }

    // Send arrival message to new room
    for (const auto& actor : dest_room->contents().actors) {
        if (actor && actor.get() != teleport_target.get()) {
            actor->receive_message(fmt::format("{} appears in a flash of light!",
                teleport_target->display_name()));
        }
    }

    return EffectResult::success_result(1, attacker_msg, target_msg, room_msg, "", EffectType::Teleport);
}

std::expected<EffectResult, Error> EffectExecutor::execute_reveal(
    const EffectParams& params, EffectContext& context) {

    if (!context.actor) {
        return std::unexpected(Errors::InvalidArgument("actor", "is required for reveal effect"));
    }

    auto room = context.actor->current_room();
    if (!room) {
        return std::unexpected(Errors::InvalidState("Actor has no room"));
    }

    int revealed_count = 0;
    std::vector<std::string> revealed_names;

    // Reveal hidden/invisible actors in the room
    bool reveal_hidden = (params.reveal_type == "all" || params.reveal_type == "hidden");
    bool reveal_invisible = (params.reveal_type == "all" || params.reveal_type == "invisible");

    for (const auto& actor : room->contents().actors) {
        if (!actor || actor.get() == context.actor.get()) continue;

        bool was_hidden = false;

        // Check for hide flag
        if (reveal_hidden && actor->has_flag(ActorFlag::Hide)) {
            actor->set_flag(ActorFlag::Hide, false);
            was_hidden = true;
        }

        // Check for invisible flag
        if (reveal_invisible && actor->has_flag(ActorFlag::Invisible)) {
            actor->set_flag(ActorFlag::Invisible, false);
            was_hidden = true;
        }

        if (was_hidden) {
            revealed_count++;
            revealed_names.push_back(actor->display_name());

            // Notify the revealed actor
            actor->receive_message("You have been revealed!");
        }
    }

    // Generate result message
    std::string attacker_msg;
    std::string room_msg;

    if (revealed_count > 0) {
        if (revealed_count == 1) {
            attacker_msg = fmt::format("You reveal {}!", revealed_names[0]);
            room_msg = fmt::format("{} magically reveals {}!",
                context.actor->display_name(), revealed_names[0]);
        } else {
            attacker_msg = fmt::format("You reveal {} hidden beings!", revealed_count);
            room_msg = fmt::format("{} magically reveals {} hidden beings!",
                context.actor->display_name(), revealed_count);
        }
    } else {
        attacker_msg = "You sense nothing hidden here.";
    }

    return EffectResult::success_result(revealed_count, attacker_msg, "", room_msg, "", EffectType::Reveal);
}

std::expected<EffectResult, Error> EffectExecutor::execute_cleanse(
    const EffectParams& params, EffectContext& context) {

    // Cleanse can target self or others
    std::shared_ptr<Actor> cleanse_target = context.target ? context.target : context.actor;
    if (!cleanse_target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for cleanse effect"));
    }

    // Evaluate cleanse power from formula
    int cleanse_power = 50;  // Default
    if (!params.cleanse_power_formula.empty()) {
        auto result = FormulaParser::evaluate(params.cleanse_power_formula, context.formula_ctx);
        if (result) {
            cleanse_power = *result;
        }
    }

    // Track what we've cleansed
    int cleansed_count = 0;
    int max_to_cleanse = params.max_effects_removed > 0 ? params.max_effects_removed : 999;

    // Get the DoT effects on the target and try to cure them
    auto& dots = cleanse_target->dot_effects();
    std::vector<size_t> to_remove;

    // Random generator for cleanse checks
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, 100);

    for (size_t i = 0; i < dots.size() && cleansed_count < max_to_cleanse; ++i) {
        const auto& dot = dots[i];

        // Check if this DoT matches our cleanse category
        bool category_match = (params.cleanse_category == "all" ||
                              params.cleanse_category == dot.cure_category);

        if (category_match) {
            // Power check: cleanse_power vs potency
            // Higher power = more likely to cleanse
            int roll = dist(gen);

            // Success if roll <= (cleanse_power - potency * 10 + 50)
            // This gives a 50% base chance, modified by power vs potency
            int success_threshold = cleanse_power - (dot.potency * 10) + 50;
            success_threshold = std::clamp(success_threshold, 5, 95);  // Always 5-95% chance

            if (roll <= success_threshold) {
                to_remove.push_back(i);
                cleansed_count++;
            }
        }
    }

    // Remove cleansed DoTs (in reverse order to preserve indices)
    for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
        dots.erase(dots.begin() + static_cast<ptrdiff_t>(*it));
    }

    // Also remove harmful status effects matching the category
    // Map cleanse categories to ActorFlags
    std::vector<ActorFlag> flags_to_check;
    if (params.cleanse_category == "all" || params.cleanse_category == "poison") {
        flags_to_check.push_back(ActorFlag::Poison);
    }
    if (params.cleanse_category == "all" || params.cleanse_category == "curse") {
        flags_to_check.push_back(ActorFlag::Curse);
    }
    if (params.cleanse_category == "all" || params.cleanse_category == "disease") {
        // Disease could be represented by various flags
        flags_to_check.push_back(ActorFlag::Slow);
    }
    if (params.cleanse_category == "all" || params.cleanse_category == "blind") {
        flags_to_check.push_back(ActorFlag::Blind);
    }

    for (auto flag : flags_to_check) {
        if (cleanse_target->has_flag(flag) && cleansed_count < max_to_cleanse) {
            cleanse_target->set_flag(flag, false);
            cleansed_count++;
        }
    }

    // Generate messages
    std::string attacker_msg, target_msg, room_msg;
    bool self_cast = (cleanse_target == context.actor);

    if (cleansed_count > 0) {
        if (self_cast) {
            attacker_msg = fmt::format("You feel purified as {} affliction{} {} removed!",
                cleansed_count, cleansed_count > 1 ? "s" : "",
                cleansed_count > 1 ? "are" : "is");
            room_msg = fmt::format("A wave of purity washes over {}.",
                context.actor->display_name());
        } else {
            attacker_msg = fmt::format("You cleanse {} of {} affliction{}!",
                cleanse_target->display_name(), cleansed_count,
                cleansed_count > 1 ? "s" : "");
            target_msg = fmt::format("{} cleanses you of {} affliction{}!",
                context.actor->display_name(), cleansed_count,
                cleansed_count > 1 ? "s" : "");
            room_msg = fmt::format("{} cleanses {} with purifying magic!",
                context.actor->display_name(), cleanse_target->display_name());
        }
    } else {
        if (self_cast) {
            attacker_msg = "You have no afflictions to cleanse.";
        } else {
            attacker_msg = fmt::format("{} has no afflictions to cleanse.",
                cleanse_target->display_name());
        }
    }

    return EffectResult::success_result(cleansed_count, attacker_msg, target_msg, room_msg, "", EffectType::Cleanse);
}

std::expected<EffectResult, Error> EffectExecutor::execute_dispel(
    const EffectParams& params, EffectContext& context) {

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for dispel effect"));
    }

    // Evaluate dispel power from formula
    int dispel_power = 50;  // Default
    if (!params.dispel_power_formula.empty()) {
        auto result = FormulaParser::evaluate(params.dispel_power_formula, context.formula_ctx);
        if (result) {
            dispel_power = *result;
        }
    }

    // Determine which effects to dispel
    bool dispel_beneficial = (params.dispel_type == "beneficial" || params.dispel_type == "all");
    bool dispel_harmful = (params.dispel_type == "harmful" || params.dispel_type == "all");

    int dispelled_count = 0;

    // Get all active effects on the target
    const auto& effects = context.target->active_effects();
    std::vector<std::string> effects_to_remove;

    // Define beneficial flags (buffs that enemies would want to dispel)
    static const std::vector<ActorFlag> beneficial_flags = {
        ActorFlag::Sanctuary, ActorFlag::Shield, ActorFlag::Armor, ActorFlag::Bless,
        ActorFlag::Haste, ActorFlag::Blur, ActorFlag::Stoneskin, ActorFlag::Barkskin,
        ActorFlag::Fireshield, ActorFlag::Coldshield, ActorFlag::Invisible,
        ActorFlag::Flying, ActorFlag::Waterwalk, ActorFlag::Detect_Invis,
        ActorFlag::Detect_Magic, ActorFlag::Infravision, ActorFlag::Sense_Life
    };

    // Define harmful flags (debuffs that allies would want to dispel)
    static const std::vector<ActorFlag> harmful_flags = {
        ActorFlag::Curse, ActorFlag::Poison, ActorFlag::Blind, ActorFlag::Slow,
        ActorFlag::Paralyzed, ActorFlag::Webbed, ActorFlag::Sleep, ActorFlag::Charm
    };

    // Random generator for save checks
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dist(1, 100);

    for (const auto& effect : effects) {
        // Determine if this effect is beneficial or harmful
        bool is_beneficial = false;
        bool is_harmful = false;

        for (auto bf : beneficial_flags) {
            if (effect.flag == bf) {
                is_beneficial = true;
                break;
            }
        }
        for (auto hf : harmful_flags) {
            if (effect.flag == hf) {
                is_harmful = true;
                break;
            }
        }

        // Check if we should try to dispel this effect
        bool should_dispel = (is_beneficial && dispel_beneficial) ||
                            (is_harmful && dispel_harmful);

        if (should_dispel) {
            // Save check: dispel_power vs effect modifier value (as resistance)
            int resistance = std::abs(effect.modifier_value);
            int success_threshold = dispel_power - resistance + 50;
            success_threshold = std::clamp(success_threshold, 5, 95);

            if (dist(gen) <= success_threshold) {
                effects_to_remove.push_back(effect.name);
                dispelled_count++;
            }
        }
    }

    // Remove dispelled effects by name
    for (const auto& name : effects_to_remove) {
        context.target->remove_effect(name);
    }

    // Also remove HoT effects if dispelling beneficial
    if (dispel_beneficial) {
        auto& hots = context.target->hot_effects();
        for (size_t i = hots.size(); i > 0; --i) {
            if (dist(gen) <= dispel_power) {
                hots.erase(hots.begin() + static_cast<ptrdiff_t>(i - 1));
                dispelled_count++;
            }
        }
    }

    // Generate messages
    std::string attacker_msg, target_msg, room_msg;

    if (dispelled_count > 0) {
        attacker_msg = fmt::format("You dispel {} magical effect{} from {}!",
            dispelled_count, dispelled_count > 1 ? "s" : "",
            context.target->display_name());
        target_msg = fmt::format("{} dispels {} magical effect{} from you!",
            context.actor->display_name(), dispelled_count,
            dispelled_count > 1 ? "s" : "");
        room_msg = fmt::format("{}'s magic flickers and fades as {} dispels it!",
            context.target->display_name(), context.actor->display_name());
    } else {
        attacker_msg = fmt::format("You fail to dispel any magic from {}.",
            context.target->display_name());
        target_msg = fmt::format("{} tries to dispel your magic, but fails!",
            context.actor->display_name());
    }

    return EffectResult::success_result(dispelled_count, attacker_msg, target_msg, room_msg, "", EffectType::Dispel);
}

std::expected<EffectResult, Error> EffectExecutor::execute_summon(
    const EffectParams& params, EffectContext& context) {

    if (!context.actor) {
        return std::unexpected(Errors::InvalidArgument("actor", "is required for summon effect"));
    }

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for summon effect"));
    }

    auto actor_room = context.actor->current_room();
    if (!actor_room) {
        return std::unexpected(Errors::InvalidState("Actor has no room"));
    }

    auto target_room = context.target->current_room();
    if (!target_room) {
        return std::unexpected(Errors::InvalidState("Target has no room"));
    }

    // Check if target is already in the same room
    if (target_room.get() == actor_room.get()) {
        return EffectResult::failure_result("Target is already here!");
    }

    // Evaluate max level formula
    int max_level = 100;  // Default: no level limit
    if (!params.max_level_formula.empty()) {
        auto result = FormulaParser::evaluate(params.max_level_formula, context.formula_ctx);
        if (result) {
            max_level = *result;
        }
    }

    // Check level restriction
    if (context.target->stats().level > max_level) {
        return EffectResult::failure_result(fmt::format("{} is too powerful to summon!",
            context.target->display_name()));
    }

    // Check zone restriction (same zone only for basic summon)
    if (target_room->id().zone_id() != actor_room->id().zone_id()) {
        return EffectResult::failure_result("Target is too far away to summon!");
    }

    // Perform the summon
    target_room->remove_actor(context.target->id());
    actor_room->add_actor(context.target);
    context.target->move_to(actor_room);

    // Generate messages
    std::string attacker_msg = fmt::format("You summon {} to your location!",
        context.target->display_name());
    std::string target_msg = fmt::format("{} summons you!",
        context.actor->display_name());
    std::string room_msg = fmt::format("{} arrives in a flash of light, summoned by {}!",
        context.target->display_name(), context.actor->display_name());

    // Notify the room the target left
    for (const auto& actor : target_room->contents().actors) {
        if (actor) {
            actor->receive_message(fmt::format("{} is summoned away!",
                context.target->display_name()));
        }
    }

    return EffectResult::success_result(1, attacker_msg, target_msg, room_msg, "", EffectType::Summon);
}

std::expected<EffectResult, Error> EffectExecutor::execute_resurrect(
    const EffectParams& params, EffectContext& context) {

    if (!context.actor) {
        return std::unexpected(Errors::InvalidArgument("actor", "is required for resurrect effect"));
    }

    if (!context.target) {
        return std::unexpected(Errors::InvalidArgument("target", "is required for resurrect effect"));
    }

    // Check if target is actually dead (ghost or dead position only)
    // Note: Incapacitated/Mortally_Wounded with negative HP is NOT dead - use heal spells instead
    bool is_dead = context.target->position() == Position::Ghost ||
                   context.target->position() == Position::Dead;
    if (!is_dead) {
        if (context.target->stats().hit_points <= 0) {
            return EffectResult::failure_result(fmt::format("{} is dying but not dead yet! Try healing them instead.",
                context.target->display_name()));
        }
        return EffectResult::failure_result(fmt::format("{} is not dead!",
            context.target->display_name()));
    }

    // Evaluate exp return percentage
    int exp_return_percent = 60;  // Default: 60% of death exp returned
    if (!params.exp_return_formula.empty()) {
        auto result = FormulaParser::evaluate(params.exp_return_formula, context.formula_ctx);
        if (result) {
            exp_return_percent = std::clamp(*result, 0, 100);
        }
    }

    // Resurrect the target
    auto& stats = context.target->stats();

    // Evaluate HP percent to restore
    int hp_percent = 10;  // Default: 10% of max HP
    if (!params.hp_percent_formula.empty()) {
        auto result = FormulaParser::evaluate(params.hp_percent_formula, context.formula_ctx);
        if (result) {
            hp_percent = std::clamp(*result, 1, 100);
        }
    }

    // Restore HP based on percentage of max
    int restored_hp = std::max(1, stats.max_hit_points * hp_percent / 100);
    stats.hit_points = restored_hp;

    // Set position back to standing (they're alive!)
    context.target->set_position(Position::Standing);

    // Clear death-related flags if any
    context.target->set_flag(ActorFlag::Sleep, false);
    context.target->set_flag(ActorFlag::Paralyzed, false);

    // TODO: Implement exp recovery when death exp tracking is added
    // int exp_gained = stats.level * 100 * exp_return_percent / 100;
    (void)exp_return_percent;  // Suppress unused warning until exp tracking is implemented

    // Move target to actor's room if they're not there
    auto actor_room = context.actor->current_room();
    auto target_room = context.target->current_room();

    if (actor_room && target_room && actor_room.get() != target_room.get()) {
        target_room->remove_actor(context.target->id());
        actor_room->add_actor(context.target);
        context.target->move_to(actor_room);
    }

    // Generate messages
    std::string attacker_msg = fmt::format("You resurrect {}! They return to life with {} HP.",
        context.target->display_name(), restored_hp);
    std::string target_msg = fmt::format("{} resurrects you! You return to life with {} HP.",
        context.actor->display_name(), restored_hp);
    std::string room_msg = fmt::format("{} calls upon divine power to resurrect {}!",
        context.actor->display_name(), context.target->display_name());

    return EffectResult::success_result(restored_hp, attacker_msg, target_msg, room_msg, "", EffectType::Resurrect);
}

} // namespace FieryMUD

#include "combat.hpp"
#include "actor.hpp"
#include "ability_executor.hpp"
#include "logging.hpp"
#include "money.hpp"
#include "../game/composer_system.hpp"
#include "../scripting/coroutine_scheduler.hpp"
#include "../scripting/trigger_manager.hpp"
#include "../text/text_format.hpp"
#include "../world/room.hpp"
#include <random>
#include <algorithm>
#include <unordered_map>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

namespace FieryMUD {

// Static member definitions
std::vector<std::pair<CombatEvent::EventType, CombatEventHandler>> CombatSystem::event_handlers_;
std::vector<CombatPair> CombatManager::active_combats_;

// Thread-local random generators
static thread_local std::random_device rd;
static thread_local std::mt19937 gen(rd());

// ============================================================================
// Dice Roll Display Helpers
// ============================================================================

/** Check if an actor has ShowDiceRolls preference enabled */
static bool wants_dice_details(const std::shared_ptr<Actor>& actor) {
    if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
        return player->is_show_dice_rolls();
    }
    return false;
}

/** Detailed mitigation breakdown for ShowDiceRolls display */
struct MitigationDetails {
    double damage_before_mitigation = 0.0;
    double damage_after_soak = 0.0;
    double damage_after_dr = 0.0;
    double final_damage = 0.0;
    double effective_soak = 0.0;
    double effective_dr_percent = 0.0;
    double glancing_multiplier = 1.0;
};

/** Get color tag based on hit result */
static std::string_view get_dice_color(HitResult hit_result) {
    switch (hit_result) {
        case HitResult::Critical: return "<b:red>";
        case HitResult::Hit:      return "<yellow>";
        case HitResult::Glancing: return "<dim>";
        case HitResult::Miss:     return "<cyan>";
        default:                  return "<dim>";
    }
}

/** Format attack roll details line */
static std::string format_roll_details(const HitCalcResult& hit_calc,
                                       const CombatStats& attacker_stats,
                                       const CombatStats& defender_stats,
                                       HitResult result) {
    auto color = get_dice_color(result);
    return fmt::format("\r\n  {}[Roll: {}+{:.0f}={:.0f} vs {}+{:.0f}={:.0f}, margin {:+d}]</>",
        color,
        hit_calc.attacker_roll, attacker_stats.acc, hit_calc.attacker_total,
        hit_calc.defender_roll, defender_stats.eva, hit_calc.defender_total,
        hit_calc.margin);
}

/** Format damage mitigation breakdown line */
static std::string format_mitigation_details(const MitigationDetails& mit, HitResult result) {
    auto color = get_dice_color(result);

    if (result == HitResult::Glancing) {
        return fmt::format("\r\n  {}[Base: {:.0f} -> Soak: -{:.0f} -> DR: {:.0f}% -> x{:.1f} glance -> Final: {:.0f}]</>",
            color,
            mit.damage_before_mitigation,
            mit.effective_soak,
            mit.effective_dr_percent * 100,
            mit.glancing_multiplier,
            mit.final_damage);
    }

    return fmt::format("\r\n  {}[Base: {:.0f} -> Soak: -{:.0f} -> DR: {:.0f}% -> Final: {:.0f}]</>",
        color,
        mit.damage_before_mitigation,
        mit.effective_soak,
        mit.effective_dr_percent * 100,
        mit.final_damage);
}

/** Calculate mitigation details for ShowDiceRolls display */
static MitigationDetails calculate_mitigation_details(double raw_damage,
                                                       const CombatStats& attacker_stats,
                                                       const CombatStats& defender_stats,
                                                       double final_damage,
                                                       HitResult hit_result) {
    MitigationDetails mit;
    mit.damage_before_mitigation = raw_damage;
    mit.final_damage = final_damage;

    // Calculate effective soak (same logic as apply_mitigation)
    mit.effective_soak = std::max(0.0, defender_stats.soak - attacker_stats.pen_flat);
    mit.damage_after_soak = std::max(1.0, raw_damage - mit.effective_soak);

    // Calculate effective DR% (same logic as apply_mitigation)
    mit.effective_dr_percent = defender_stats.dr_pct * (1.0 - attacker_stats.pen_pct);
    mit.effective_dr_percent = std::min(mit.effective_dr_percent, CombatConstants::DR_CAP);
    mit.damage_after_dr = mit.damage_after_soak * (1.0 - mit.effective_dr_percent);

    // Track glancing multiplier
    if (hit_result == HitResult::Glancing) {
        mit.glancing_multiplier = CombatConstants::GLANCING_MULTIPLIER;
    } else if (hit_result == HitResult::Critical) {
        mit.glancing_multiplier = CombatConstants::CRITICAL_MULTIPLIER;
    }

    return mit;
}

// ============================================================================
// CombatStats Implementation
// ============================================================================

double CombatStats::calculate_dr_percent(double ar, int level) {
    if (ar <= 0) return 0.0;

    double k = get_dr_k_constant(level);
    double dr = ar / (ar + k);

    // Cap at 75%
    return std::min(dr, CombatConstants::DR_CAP);
}

double CombatStats::get_dr_k_constant(int level) {
    if (level <= 20) {
        return CombatConstants::DR_K_LOW_TIER;
    } else if (level <= 50) {
        return CombatConstants::DR_K_MID_TIER;
    } else {
        return CombatConstants::DR_K_HIGH_TIER;
    }
}

CombatStats CombatStats::operator+(const CombatStats& other) const {
    CombatStats result;

    // Offensive stats (additive)
    result.acc = acc + other.acc;
    result.ap = ap + other.ap;
    result.pen_flat = pen_flat + other.pen_flat;
    result.pen_pct = std::min(CombatConstants::PEN_PCT_CAP, pen_pct + other.pen_pct);
    result.crit_bonus = crit_bonus + other.crit_bonus;

    // Defensive stats (additive)
    result.eva = eva + other.eva;
    result.ar = ar + other.ar;
    result.soak = soak + other.soak;
    result.ward_pct = ward_pct + other.ward_pct;

    // Resistances - use most favorable (lowest)
    result.res_fire = std::min(res_fire, other.res_fire);
    result.res_cold = std::min(res_cold, other.res_cold);
    result.res_shock = std::min(res_shock, other.res_shock);
    result.res_acid = std::min(res_acid, other.res_acid);
    result.res_poison = std::min(res_poison, other.res_poison);
    result.res_physical = std::min(res_physical, other.res_physical);

    // Weapon stats - use first non-default
    if (weapon_dice_num > 1 || weapon_dice_size > 6 || weapon_base_damage > 0) {
        result.weapon_dice_num = weapon_dice_num;
        result.weapon_dice_size = weapon_dice_size;
        result.weapon_base_damage = weapon_base_damage;
    } else {
        result.weapon_dice_num = other.weapon_dice_num;
        result.weapon_dice_size = other.weapon_dice_size;
        result.weapon_base_damage = other.weapon_base_damage;
    }

    return result;
}

// ============================================================================
// CombatSystem Implementation
// ============================================================================

int CombatSystem::roll_d100() {
    std::uniform_int_distribution<int> dist(CombatConstants::D100_MIN, CombatConstants::D100_MAX);
    return dist(gen);
}

double CombatSystem::roll_damage(int num_dice, int dice_size, int bonus) {
    if (num_dice <= 0 || dice_size <= 0) {
        return static_cast<double>(std::max(0, bonus));
    }

    std::uniform_int_distribution<int> die(1, dice_size);
    double total = 0.0;
    for (int i = 0; i < num_dice; ++i) {
        total += die(gen);
    }
    return total + bonus;
}

CombatStats CombatSystem::calculate_combat_stats(const Actor& actor) {
    CombatStats stats;
    const Stats& actor_stats = actor.stats();
    int level = actor_stats.level;

    // Base ACC from level
    stats.acc = CombatConstants::ACC_BASE;

    // Class-specific ACC rate
    if (const auto* player = dynamic_cast<const Player*>(&actor)) {
        CharacterClass char_class = string_to_class(player->player_class());
        double acc_rate = get_class_acc_rate(char_class);
        stats.acc += level * acc_rate;

        // Add race bonuses
        CharacterRace char_race = string_to_race(player->race());
        CombatStats race_bonus = get_race_combat_bonus(char_race);
        stats = stats + race_bonus;
    } else {
        // Mobs get a moderate ACC rate
        stats.acc += level * 0.5;
    }

    // Stat contributions to ACC
    // STR: (STR - 50) / 5 = -10 to +10
    stats.acc += (actor_stats.strength - CombatConstants::STAT_NEUTRAL) / CombatConstants::STR_TO_ACC_DIVISOR;
    // INT: (INT - 50) / 10 = -5 to +5
    stats.acc += (actor_stats.intelligence - CombatConstants::STAT_NEUTRAL) / CombatConstants::INT_TO_ACC_DIVISOR;
    // WIS: (WIS - 50) / 10 = -5 to +5
    stats.acc += (actor_stats.wisdom - CombatConstants::STAT_NEUTRAL) / CombatConstants::WIS_TO_ACC_DIVISOR;

    // Use new accuracy stat directly (replaces legacy hit_roll)
    stats.acc += actor_stats.accuracy;

    // Cap ACC
    stats.acc = std::min(stats.acc, static_cast<double>(CombatConstants::ACC_SOFT_CAP));

    // Base EVA from level
    stats.eva = CombatConstants::EVA_BASE + (level * CombatConstants::EVA_LEVEL_RATE);

    // DEX contribution to EVA: (DEX - 50) / 3 = -16 to +16
    stats.eva += (actor_stats.dexterity - CombatConstants::STAT_NEUTRAL) / CombatConstants::DEX_TO_EVA_DIVISOR;

    // Add evasion stat directly (replaces legacy AC conversion)
    stats.eva += actor_stats.evasion;

    // Add skill contributions (dodge, parry, riposte) from player abilities
    if (const auto* player = dynamic_cast<const Player*>(&actor)) {
        auto& cache = AbilityCache::instance();

        // Look up skill abilities and get proficiency
        if (const auto* dodge = cache.get_ability_by_name("dodge")) {
            int dodge_skill = player->get_proficiency(dodge->id);
            stats.eva += dodge_skill * CombatConstants::DODGE_TO_EVA;
        }
        if (const auto* parry = cache.get_ability_by_name("parry")) {
            int parry_skill = player->get_proficiency(parry->id);
            stats.eva += parry_skill * CombatConstants::PARRY_TO_EVA;
        }
        if (const auto* riposte = cache.get_ability_by_name("riposte")) {
            int riposte_skill = player->get_proficiency(riposte->id);
            stats.eva += riposte_skill * CombatConstants::RIPOSTE_TO_EVA;
        }
    }

    // Cap EVA
    stats.eva = std::min(stats.eva, static_cast<double>(CombatConstants::EVA_SOFT_CAP));

    // Attack Power from new stat + STR modifier (replaces legacy damage_roll)
    stats.ap = actor_stats.attack_power;
    stats.ap += Stats::attribute_modifier(actor_stats.strength);

    // Use armor_rating directly (replaces legacy AC inversion)
    stats.ar = actor_stats.armor_rating;

    // Add damage reduction percent directly from stat
    stats.dr_pct = actor_stats.damage_reduction_percent;

    // Add additional DR from armor rating scaled by level
    stats.dr_pct += CombatStats::calculate_dr_percent(stats.ar, level);

    // Get weapon stats from mob if applicable
    if (const auto* mobile = dynamic_cast<const Mobile*>(&actor)) {
        stats.weapon_dice_num = mobile->bare_hand_damage_dice_num();
        stats.weapon_dice_size = mobile->bare_hand_damage_dice_size();
        stats.weapon_base_damage = mobile->bare_hand_damage_dice_bonus();
    } else {
        // Default player unarmed
        stats.weapon_dice_num = 1;
        stats.weapon_dice_size = 4;
        stats.weapon_base_damage = 0;
    }

    return stats;
}

double CombatSystem::get_class_acc_rate(CharacterClass character_class) {
    switch (character_class) {
        case CharacterClass::Warrior:
            return ClassAccRates::WARRIOR;
        case CharacterClass::Rogue:
            return ClassAccRates::ROGUE;
        case CharacterClass::Cleric:
            return ClassAccRates::CLERIC;
        case CharacterClass::Sorcerer:
            return ClassAccRates::SORCERER;
        default:
            return 0.5; // Default moderate rate
    }
}

CombatStats CombatSystem::get_race_combat_bonus(CharacterRace race) {
    CombatStats bonus;

    switch (race) {
        case CharacterRace::Human:
            // Versatile - small bonuses
            bonus.acc = 2.0;
            bonus.eva = 2.0;
            bonus.ap = 1.0;
            break;

        case CharacterRace::Elf:
            // Dexterous - high EVA, crit bonus
            bonus.acc = 3.0;
            bonus.eva = 5.0;
            bonus.crit_bonus = 5.0;  // +5 to critical margin
            break;

        case CharacterRace::Dwarf:
            // Tough - high AR/DR, damage resistance
            bonus.acc = 0.0;
            bonus.eva = -2.0;
            bonus.ap = 3.0;
            bonus.ar = 10.0;
            bonus.res_physical = 90.0;  // 10% physical resistance
            break;

        case CharacterRace::Halfling:
            // Lucky and evasive
            bonus.acc = 2.0;
            bonus.eva = 8.0;
            bonus.crit_bonus = 3.0;
            break;

        default:
            break;
    }

    return bonus;
}

CharacterClass CombatSystem::string_to_class(const std::string& class_name) {
    static const std::unordered_map<std::string, CharacterClass> class_map = {
        {"warrior", CharacterClass::Warrior},
        {"cleric", CharacterClass::Cleric},
        {"sorcerer", CharacterClass::Sorcerer},
        {"rogue", CharacterClass::Rogue},
        {"thief", CharacterClass::Rogue},  // Alias
    };

    std::string lower_name;
    lower_name.reserve(class_name.size());
    for (char c : class_name) {
        lower_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    auto it = class_map.find(lower_name);
    return (it != class_map.end()) ? it->second : CharacterClass::Warrior;
}

CharacterRace CombatSystem::string_to_race(std::string_view race_name) {
    static const std::unordered_map<std::string, CharacterRace> race_map = {
        {"human", CharacterRace::Human},
        {"elf", CharacterRace::Elf},
        {"halfelf", CharacterRace::Elf},
        {"half-elf", CharacterRace::Elf},
        {"dwarf", CharacterRace::Dwarf},
        {"halfling", CharacterRace::Halfling},
    };

    std::string lower_name;
    lower_name.reserve(race_name.size());
    for (char c : race_name) {
        lower_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    auto it = race_map.find(lower_name);
    return (it != race_map.end()) ? it->second : CharacterRace::Human;
}

HitCalcResult CombatSystem::calculate_hit(const CombatStats& attacker_stats,
                                          const CombatStats& defender_stats) {
    HitCalcResult result;

    // Roll d100 for each side
    result.attacker_roll = roll_d100();
    result.defender_roll = roll_d100();

    // Check for auto-hit/miss
    if (result.attacker_roll == 1) {
        result.auto_miss = true;
        result.result = HitResult::Miss;
        return result;
    }
    if (result.attacker_roll == 100) {
        result.auto_hit = true;
        result.result = HitResult::Critical;  // Natural 100 = crit
        return result;
    }

    // Calculate totals
    result.attacker_total = result.attacker_roll + attacker_stats.acc;
    result.defender_total = result.defender_roll + defender_stats.eva;

    // Calculate margin (with crit bonus applied)
    result.margin = static_cast<int>(result.attacker_total - result.defender_total + attacker_stats.crit_bonus);

    // Determine hit result based on margin
    if (result.margin >= CombatConstants::CRITICAL_HIT_MARGIN) {
        result.result = HitResult::Critical;
    } else if (result.margin >= CombatConstants::HIT_MARGIN) {
        result.result = HitResult::Hit;
    } else if (result.margin >= CombatConstants::GLANCING_MARGIN) {
        result.result = HitResult::Glancing;
    } else {
        result.result = HitResult::Miss;
    }

    return result;
}

double CombatSystem::apply_mitigation(double raw_damage,
                                      const CombatStats& attacker_stats,
                                      const CombatStats& defender_stats,
                                      int /* damage_type */) {
    if (raw_damage <= 0) return 0.0;

    // Step 1: Apply Soak (flat reduction) - reduced by attacker's flat penetration
    double effective_soak = std::max(0.0, defender_stats.soak - attacker_stats.pen_flat);
    double damage_after_soak = std::max(1.0, raw_damage - effective_soak);

    // Step 2: Apply DR% - reduced by attacker's percentage penetration
    double effective_dr = defender_stats.dr_pct * (1.0 - attacker_stats.pen_pct);
    effective_dr = std::min(effective_dr, CombatConstants::DR_CAP);
    double damage_after_dr = damage_after_soak * (1.0 - effective_dr);

    // Step 3: Apply elemental/type resistances (TODO: implement damage types)
    // For now, apply physical resistance
    double resistance_mult = defender_stats.res_physical / 100.0;
    double final_damage = damage_after_dr * resistance_mult;

    // Minimum 1 damage
    return std::max(CombatConstants::MIN_DAMAGE, final_damage);
}

double CombatSystem::calculate_damage(const Actor& attacker, const CombatStats& attacker_stats,
                                      const Actor& defender, const CombatStats& defender_stats,
                                      HitResult hit_result) {
    if (hit_result == HitResult::Miss) {
        return 0.0;
    }

    // Roll weapon damage
    double base_damage = roll_damage(attacker_stats.weapon_dice_num,
                                     attacker_stats.weapon_dice_size,
                                     attacker_stats.weapon_base_damage);

    // Add Attack Power
    base_damage += attacker_stats.ap;

    // Apply hit type multiplier
    double multiplier = 1.0;
    switch (hit_result) {
        case HitResult::Critical:
            multiplier = CombatConstants::CRITICAL_MULTIPLIER;
            break;
        case HitResult::Glancing:
            multiplier = CombatConstants::GLANCING_MULTIPLIER;
            break;
        case HitResult::Hit:
        default:
            multiplier = 1.0;
            break;
    }
    base_damage *= multiplier;

    // Apply sneak attack for rogues (if conditions are met)
    if (ClassAbilities::rogue_sneak_attack_available(attacker, defender)) {
        double sneak_bonus = ClassAbilities::rogue_sneak_attack_damage(attacker.stats().level);
        base_damage += sneak_bonus;
    }

    // Apply mitigation
    double final_damage = apply_mitigation(base_damage, attacker_stats, defender_stats, 0);

    return final_damage;
}

CombatResult CombatSystem::perform_attack(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target) {
    CombatResult result;

    if (!attacker || !target) {
        result.attacker_message = "Invalid combat participants.";
        return result;
    }

    // Start combat if not already fighting
    if (!CombatManager::is_in_combat(*attacker) && !CombatManager::is_in_combat(*target)) {
        CombatManager::start_combat(attacker, target);
        fire_event(CombatEvent(CombatEvent::EventType::CombatStart, attacker, target));
    }

    fire_event(CombatEvent(CombatEvent::EventType::AttackAttempt, attacker, target));

    // Calculate combat stats for both participants
    CombatStats attacker_stats = calculate_combat_stats(*attacker);
    CombatStats defender_stats = calculate_combat_stats(*target);

    // Calculate hit using opposed d100 rolls
    result.hit_calc = calculate_hit(attacker_stats, defender_stats);

    // Handle miss
    if (result.hit_calc.result == HitResult::Miss) {
        result.type = CombatResult::Type::Miss;
        result.attacker_message = fmt::format("Your attack misses <cyan>{}</>.",
            target->display_name());
        result.target_message = TextFormat::capitalize(fmt::format("{}'s attack misses you.",
            attacker->display_name()));
        result.room_message = TextFormat::capitalize(fmt::format("{}'s attack misses <cyan>{}</>.",
            attacker->display_name(), target->display_name()));

        // Add dice roll details if attacker wants to see them
        if (wants_dice_details(attacker)) {
            result.attacker_message += format_roll_details(result.hit_calc, attacker_stats, defender_stats, HitResult::Miss);
        }

        fire_event(CombatEvent(CombatEvent::EventType::AttackMiss, attacker, target));
        return result;
    }

    // Calculate damage
    double damage = calculate_damage(*attacker, attacker_stats, *target, defender_stats, result.hit_calc.result);
    result.damage_dealt = damage;

    // Set result type based on hit type
    switch (result.hit_calc.result) {
        case HitResult::Critical:
            result.type = CombatResult::Type::CriticalHit;
            fire_event(CombatEvent(CombatEvent::EventType::AttackCritical, attacker, target));
            break;
        case HitResult::Glancing:
            result.type = CombatResult::Type::Glancing;
            fire_event(CombatEvent(CombatEvent::EventType::AttackGlancing, attacker, target));
            break;
        case HitResult::Hit:
        default:
            result.type = CombatResult::Type::Hit;
            fire_event(CombatEvent(CombatEvent::EventType::AttackHit, attacker, target));
            break;
    }

    // Apply damage to target
    auto& target_stats = target->stats();
    int damage_int = static_cast<int>(std::round(damage));
    target_stats.hit_points -= damage_int;

    fire_event(CombatEvent(CombatEvent::EventType::DamageDealt, attacker, target));

    // Execute ATTACK trigger for wielded weapon
    auto& trigger_mgr = TriggerManager::instance();
    if (trigger_mgr.is_initialized()) {
        auto weapon = attacker->equipment().get_equipped(EquipSlot::Wield);
        if (weapon) {
            auto room = attacker->current_room();
            trigger_mgr.dispatch_attack(weapon, attacker, target, damage_int, room);
        }
    }

    // Check HIT_PERCENT triggers for mobs
    if (target_stats.max_hit_points > 0) {
        int hp_percent = (target_stats.hit_points * 100) / target_stats.max_hit_points;
        auto& trigger_mgr = TriggerManager::instance();
        if (trigger_mgr.is_initialized()) {
            trigger_mgr.dispatch_hit_percent(target, attacker, hp_percent);
        }
    }

    // Create combat messages with color coding
    std::string hit_type_text;
    std::string damage_color;
    switch (result.hit_calc.result) {
        case HitResult::Critical:
            hit_type_text = " <b:red>CRITICALLY</>";
            damage_color = "b:red";
            break;
        case HitResult::Glancing:
            hit_type_text = " <dim>glancingly</>";
            damage_color = "dim";
            break;
        default:
            hit_type_text = "";
            damage_color = "yellow";
            break;
    }

    result.attacker_message = fmt::format("You{} hit <cyan>{}</> for <{}>{:.0f}</> damage.",
        hit_type_text, target->display_name(), damage_color, damage);
    result.target_message = TextFormat::capitalize(fmt::format("{}{} hits you for <{}>{:.0f}</> damage.",
        attacker->display_name(), hit_type_text, damage_color, damage));
    result.room_message = TextFormat::capitalize(fmt::format("{}{} hits <cyan>{}</> for <{}>{:.0f}</> damage.",
        attacker->display_name(), hit_type_text, target->display_name(), damage_color, damage));

    // Add dice roll and mitigation details if attacker wants to see them
    if (wants_dice_details(attacker)) {
        result.attacker_message += format_roll_details(result.hit_calc, attacker_stats, defender_stats, result.hit_calc.result);

        // Estimate base damage for mitigation breakdown
        // Use average weapon damage + AP, then apply multiplier
        double avg_weapon = attacker_stats.weapon_dice_num *
                           (attacker_stats.weapon_dice_size / 2.0 + 0.5) +
                           attacker_stats.weapon_base_damage + attacker_stats.ap;
        double multiplier = (result.hit_calc.result == HitResult::Critical) ? CombatConstants::CRITICAL_MULTIPLIER :
                           (result.hit_calc.result == HitResult::Glancing) ? CombatConstants::GLANCING_MULTIPLIER : 1.0;
        double estimated_base = avg_weapon * multiplier;

        MitigationDetails mit = calculate_mitigation_details(estimated_base, attacker_stats, defender_stats,
                                                              damage, result.hit_calc.result);
        result.attacker_message += format_mitigation_details(mit, result.hit_calc.result);
    }

    // Send GMCP vitals update
    if (auto player = std::dynamic_pointer_cast<Player>(target)) {
        player->send_gmcp_vitals_update();
    }

    // Check for death
    if (target_stats.hit_points <= 0) {
        target_stats.hit_points = 0;

        CombatManager::end_combat(attacker);
        CombatManager::end_combat(target);
        attacker->set_position(Position::Standing);

        long exp_gain = calculate_experience_gain(*attacker, *target);
        result.experience_gained = exp_gain;
        attacker->gain_experience(exp_gain);

        result.type = CombatResult::Type::Death;
        result.attacker_message += fmt::format(" You have killed <cyan>{}</>! You gain <experience>{}</> experience.",
                                               target->display_name(), exp_gain);

        // Set death messages based on actor type (before die() modifies state)
        bool is_player = (target->type_name() == "Player");

        if (is_player) {
            result.target_message += "\r\n<b:red>You are DEAD!</> You feel your spirit leave your body...";
            result.room_message += " " + TextFormat::capitalize(fmt::format("{} is <b:red>DEAD</>! Their spirit rises from their body.",
                target->display_name()));
        } else {
            result.target_message += "\r\n<b:red>You are DEAD!</>";
            result.room_message += " " + TextFormat::capitalize(fmt::format("{} is <b:red>DEAD</>! Its corpse falls to the ground.",
                target->display_name()));
        }

        // Execute DEATH trigger for mobs before they die
        auto& trigger_mgr = TriggerManager::instance();
        if (trigger_mgr.is_initialized()) {
            trigger_mgr.dispatch_death(target, attacker);
        }

        // Cancel any pending Lua script coroutines for the dying entity
        auto& scheduler = get_coroutine_scheduler();
        if (scheduler.is_initialized()) {
            scheduler.cancel_for_entity(target->id());
        }

        // Polymorphic death handling - Player becomes ghost, Mobile creates corpse and despawns
        // die() returns the corpse for Mobiles, nullptr for Players
        auto corpse = target->die();

        // Handle autoloot and autogold for Player attackers killing Mobiles
        if (!is_player && attacker->type_name() == "Player" && corpse) {
            auto* player = dynamic_cast<Player*>(attacker.get());
            if (player) {
                // Autogold: Find and take money from corpse
                if (player->is_autogold()) {
                    auto contents = corpse->get_contents();
                    std::vector<std::shared_ptr<Object>> items_copy(contents.begin(), contents.end());
                    for (const auto& item : items_copy) {
                        if (item && item->type() == ObjectType::Money) {
                            int coin_value = item->value();
                            if (coin_value > 0) {
                                player->receive(coin_value);
                                corpse->remove_item(item);
                                auto received = fiery::Money::from_copper(coin_value);
                                result.attacker_message += fmt::format("\r\nYou get {} from the corpse.", received.to_string());
                            }
                        }
                    }
                }

                // Autoloot: Take all non-money items from corpse
                if (player->is_autoloot()) {
                    auto contents = corpse->get_contents();
                    std::vector<std::shared_ptr<Object>> items_copy(contents.begin(), contents.end());
                    int looted_count = 0;
                    for (const auto& item : items_copy) {
                        if (!item || item->type() == ObjectType::Money) continue;

                        int obj_weight = item->weight();
                        if (!player->inventory().can_carry(obj_weight, player->max_carry_weight())) {
                            result.attacker_message += "\r\nYou can't carry any more weight.";
                            break;
                        }

                        if (corpse->remove_item(item)) {
                            auto add_result = player->inventory().add_item(item);
                            if (add_result) {
                                looted_count++;
                            } else {
                                corpse->add_item(item);
                            }
                        }
                    }
                    if (looted_count > 0) {
                        result.attacker_message += fmt::format("\r\nYou autoloot {} item{}.",
                            looted_count, looted_count == 1 ? "" : "s");
                    }
                }
            }
        }

        fire_event(CombatEvent(CombatEvent::EventType::ActorDeath, attacker, target));
        fire_event(CombatEvent(CombatEvent::EventType::CombatEnd, attacker, target));
    }

    return result;
}

long CombatSystem::calculate_experience_gain(const Actor& victor, const Actor& defeated) {
    constexpr int EXP_BASE_PER_LEVEL = 100;
    constexpr double EXP_HIGHER_LEVEL_BONUS = 0.1;
    constexpr double EXP_LOWER_LEVEL_PENALTY = 0.05;
    constexpr double EXP_MIN_MULTIPLIER = 0.1;

    int level_diff = defeated.stats().level - victor.stats().level;
    long base_exp = defeated.stats().level * EXP_BASE_PER_LEVEL;

    double multiplier = 1.0;
    if (level_diff > 0) {
        multiplier = 1.0 + (level_diff * EXP_HIGHER_LEVEL_BONUS);
    } else if (level_diff < 0) {
        multiplier = std::max(EXP_MIN_MULTIPLIER, 1.0 + (level_diff * EXP_LOWER_LEVEL_PENALTY));
    }

    return static_cast<long>(base_exp * multiplier);
}

void CombatSystem::register_event_handler(CombatEvent::EventType event_type, CombatEventHandler handler) {
    event_handlers_.emplace_back(event_type, std::move(handler));
}

void CombatSystem::fire_event(const CombatEvent& event) {
    for (const auto& [type, handler] : event_handlers_) {
        if (type == event.type()) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                Log::game()->error("Combat event handler error: {}", e.what());
            }
        }
    }
}

// ============================================================================
// ClassAbilities Implementation
// ============================================================================

bool ClassAbilities::warrior_extra_attack_available(const Actor& actor) {
    constexpr int EXTRA_ATTACK_LEVEL = 6;
    return actor.stats().level >= EXTRA_ATTACK_LEVEL;
}

bool ClassAbilities::rogue_sneak_attack_available(const Actor& attacker, const Actor& target) {
    // Must be a rogue
    if (const auto* player = dynamic_cast<const Player*>(&attacker)) {
        if (CombatSystem::string_to_class(player->player_class()) != CharacterClass::Rogue) {
            return false;
        }
    } else {
        return false;
    }

    // Sneak attack conditions (any one of these enables sneak attack):

    // 1. Attacker is hidden or sneaking
    if (attacker.has_flag(ActorFlag::Hide) || attacker.has_flag(ActorFlag::Sneak)) {
        return true;
    }

    // 2. Attacker is invisible and target can't see invisible
    if (attacker.has_flag(ActorFlag::Invisible) && !target.has_flag(ActorFlag::Detect_Invis)) {
        return true;
    }

    // 3. Target is incapacitated (sleeping, stunned, paralyzed, etc.)
    Position target_pos = target.position();
    if (target_pos == Position::Sleeping ||
        target_pos == Position::Stunned ||
        target_pos == Position::Incapacitated) {
        return true;
    }

    // 4. Target has Aware flag negated - check if target is fighting someone else (flanking)
    if (!target.has_flag(ActorFlag::Aware)) {
        // If target is fighting someone other than attacker, it's a flank
        auto opponent = CombatManager::get_opponent(target);
        if (opponent && opponent.get() != &attacker) {
            return true;
        }
    }

    return false;
}

double ClassAbilities::rogue_sneak_attack_damage(int level) {
    // Sneak attack: 1d6 per 2 levels
    int dice_count = (level + 1) / 2;
    std::uniform_real_distribution<double> d6(1.0, 6.0);

    double total = 0.0;
    for (int i = 0; i < dice_count; ++i) {
        total += d6(gen);
    }
    return total;
}

bool ClassAbilities::cleric_can_turn_undead(const Actor& actor) {
    constexpr int TURN_UNDEAD_MIN_LEVEL = 2;
    if (const auto* player = dynamic_cast<const Player*>(&actor)) {
        return CombatSystem::string_to_class(player->player_class()) == CharacterClass::Cleric &&
               actor.stats().level >= TURN_UNDEAD_MIN_LEVEL;
    }
    return false;
}

double ClassAbilities::sorcerer_spell_damage_bonus(int level, int intelligence) {
    double level_bonus = level * 0.5;
    double int_bonus = Stats::attribute_modifier(intelligence) * 2.0;
    return level_bonus + int_bonus;
}

// ============================================================================
// CombatManager Implementation
// ============================================================================

void CombatManager::start_combat(std::shared_ptr<Actor> actor1, std::shared_ptr<Actor> actor2) {
    if (!actor1 || !actor2) return;

    // Check if already in combat with each other
    for (const auto& combat : active_combats_) {
        if (combat.involves_actor(*actor1) && combat.involves_actor(*actor2)) {
            return;
        }
    }

    // Interrupt text composition and meditation (combat takes priority)
    if (auto player1 = std::dynamic_pointer_cast<Player>(actor1)) {
        if (player1->is_composing()) {
            player1->interrupt_composing("You are under attack!");
        }
        if (player1->is_meditating()) {
            player1->stop_meditation();
            player1->send_message("Your meditation is interrupted!");
        }
    }
    if (auto player2 = std::dynamic_pointer_cast<Player>(actor2)) {
        if (player2->is_composing()) {
            player2->interrupt_composing("You are under attack!");
        }
        if (player2->is_meditating()) {
            player2->stop_meditation();
            player2->send_message("Your meditation is interrupted!");
        }
    }

    // End existing combat for both actors
    end_combat(actor1);
    end_combat(actor2);

    // Set fighting position
    actor1->set_position(Position::Fighting);
    actor2->set_position(Position::Fighting);

    // Add combat pair
    active_combats_.emplace_back(actor1, actor2);
}

void CombatManager::end_combat(std::shared_ptr<Actor> actor) {
    if (!actor) return;

    auto it = std::remove_if(active_combats_.begin(), active_combats_.end(),
        [&actor](const CombatPair& combat) {
            bool involves = combat.involves_actor(*actor);
            if (involves) {
                if (combat.actor1 && combat.actor1->is_alive()) {
                    combat.actor1->set_position(Position::Standing);
                }
                if (combat.actor2 && combat.actor2->is_alive()) {
                    combat.actor2->set_position(Position::Standing);
                }
            }
            return involves;
        });

    active_combats_.erase(it, active_combats_.end());
}

bool CombatManager::process_combat_rounds() {
    cleanup_invalid_combats();

    bool rounds_processed = false;

    for (auto& combat : active_combats_) {
        if (combat.is_ready_for_next_round()) {
            execute_round(combat);
            combat.update_last_round();
            rounds_processed = true;
        }
    }

    return rounds_processed;
}

bool CombatManager::is_in_combat(const Actor& actor) {
    return std::any_of(active_combats_.begin(), active_combats_.end(),
        [&actor](const CombatPair& combat) {
            return combat.involves_actor(actor);
        });
}

std::shared_ptr<Actor> CombatManager::get_opponent(const Actor& actor) {
    for (const auto& combat : active_combats_) {
        if (combat.involves_actor(actor)) {
            return combat.get_opponent(actor);
        }
    }
    return nullptr;
}

void CombatManager::clear_all_combat() {
    for (const auto& combat : active_combats_) {
        if (combat.actor1 && combat.actor1->position() == Position::Fighting) {
            combat.actor1->set_position(Position::Standing);
        }
        if (combat.actor2 && combat.actor2->position() == Position::Fighting) {
            combat.actor2->set_position(Position::Standing);
        }
    }

    active_combats_.clear();
    Log::debug("All combat cleared");
}

void CombatManager::cleanup_invalid_combats() {
    auto it = std::remove_if(active_combats_.begin(), active_combats_.end(),
        [](const CombatPair& combat) {
            bool invalid = !combat.actor1 || !combat.actor2 ||
                          (!combat.actor1->is_alive()) ||
                          (!combat.actor2->is_alive());

            if (!invalid && combat.actor1->current_room() != combat.actor2->current_room()) {
                invalid = true;
                Log::debug("Combat ended: actors in different rooms");
            }

            if (invalid) {
                if (combat.actor1 && combat.actor1->is_alive()) {
                    combat.actor1->set_position(Position::Standing);
                }
                if (combat.actor2 && combat.actor2->is_alive()) {
                    combat.actor2->set_position(Position::Standing);
                }
            }

            return invalid;
        });

    active_combats_.erase(it, active_combats_.end());
}

void CombatManager::execute_round(CombatPair& combat_pair) {
    if (!combat_pair.actor1 || !combat_pair.actor2) return;

    Log::info("execute_round: starting between {} and {}",
              combat_pair.actor1->name(), combat_pair.actor2->name());

    // Execute FIGHT triggers for both combatants (allows scripted combat behavior)
    auto& trigger_mgr = TriggerManager::instance();
    if (trigger_mgr.is_initialized()) {
        Log::info("execute_round: calling dispatch_fight for actor1");
        trigger_mgr.dispatch_fight(combat_pair.actor1, combat_pair.actor2);
        Log::info("execute_round: calling dispatch_fight for actor2");
        trigger_mgr.dispatch_fight(combat_pair.actor2, combat_pair.actor1);
        Log::info("execute_round: FIGHT triggers done");
    }

    // Random initiative
    std::uniform_int_distribution<int> coinflip(0, 1);

    std::shared_ptr<Actor> first_attacker, first_target;
    if (coinflip(gen) == 0) {
        first_attacker = combat_pair.actor1;
        first_target = combat_pair.actor2;
    } else {
        first_attacker = combat_pair.actor2;
        first_target = combat_pair.actor1;
    }

    // First attack
    if (first_attacker->is_alive() && first_target->is_alive()) {
        auto result1 = CombatSystem::perform_attack(first_attacker, first_target);

        if (auto room = first_attacker->current_room()) {
            first_attacker->send_message(result1.attacker_message + "\r\n");
            first_target->send_message(result1.target_message + "\r\n");

            if (!result1.room_message.empty()) {
                for (const auto& actor : room->contents().actors) {
                    if (actor && actor != first_attacker && actor != first_target) {
                        actor->send_message(result1.room_message + "\r\n");
                    }
                }
            }
        }

        if (result1.type == CombatResult::Type::Death) {
            return;
        }
    }

    // Counter-attack
    if (first_target->is_alive() && first_attacker->is_alive()) {
        auto result2 = CombatSystem::perform_attack(first_target, first_attacker);

        if (auto room = first_target->current_room()) {
            first_target->send_message(result2.attacker_message + "\r\n");
            first_attacker->send_message(result2.target_message + "\r\n");

            if (!result2.room_message.empty()) {
                for (const auto& actor : room->contents().actors) {
                    if (actor && actor != first_target && actor != first_attacker) {
                        actor->send_message(result2.room_message + "\r\n");
                    }
                }
            }
        }
    }
}

std::vector<std::shared_ptr<Actor>> CombatManager::get_all_fighting_actors() {
    std::vector<std::shared_ptr<Actor>> fighting_actors;

    for (const auto& combat : active_combats_) {
        fighting_actors.push_back(combat.actor1);
        fighting_actors.push_back(combat.actor2);
    }

    return fighting_actors;
}

} // namespace FieryMUD

#include "combat.hpp"
#include "actor.hpp"
#include "logging.hpp"
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

    // Hitroll from gear adds directly to ACC
    stats.acc += actor_stats.hit_roll;

    // Cap ACC
    stats.acc = std::min(stats.acc, static_cast<double>(CombatConstants::ACC_SOFT_CAP));

    // Base EVA from level
    stats.eva = CombatConstants::EVA_BASE + (level * CombatConstants::EVA_LEVEL_RATE);

    // DEX contribution to EVA: (DEX - 50) / 3 = -16 to +16
    stats.eva += (actor_stats.dexterity - CombatConstants::STAT_NEUTRAL) / CombatConstants::DEX_TO_EVA_DIVISOR;

    // TODO: Add skill contributions (dodge, parry, riposte) when skill system is integrated
    // stats.eva += dodge_skill * CombatConstants::DODGE_TO_EVA;
    // stats.eva += parry_skill * CombatConstants::PARRY_TO_EVA;
    // stats.eva += riposte_skill * CombatConstants::RIPOSTE_TO_EVA;

    // Cap EVA
    stats.eva = std::min(stats.eva, static_cast<double>(CombatConstants::EVA_SOFT_CAP));

    // Attack Power from damroll + STR modifier
    stats.ap = actor_stats.damage_roll;
    stats.ap += Stats::attribute_modifier(actor_stats.strength);

    // Armor Rating from AC (inverted: lower AC = higher AR)
    // AR = 100 - AC (so AC of -50 = AR of 150)
    stats.ar = std::max(0.0, 100.0 - actor_stats.armor_class);

    // Calculate DR% from AR
    stats.dr_pct = CombatStats::calculate_dr_percent(stats.ar, level);

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
                                      const Actor& /* defender */, const CombatStats& defender_stats,
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

    // Apply sneak attack for rogues
    if (const auto* player = dynamic_cast<const Player*>(&attacker)) {
        CharacterClass char_class = string_to_class(player->player_class());
        if (char_class == CharacterClass::Rogue) {
            // TODO: Check sneak attack conditions
            double sneak_bonus = ClassAbilities::rogue_sneak_attack_damage(attacker.stats().level);
            base_damage += sneak_bonus;
        }
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
        result.attacker_message = fmt::format("Your attack misses {} (roll: {} vs {}).",
            target->display_name(), result.hit_calc.attacker_roll, result.hit_calc.defender_roll);
        result.target_message = fmt::format("{}'s attack misses you.", attacker->display_name());
        result.room_message = fmt::format("{}'s attack misses {}.", attacker->display_name(), target->display_name());

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

    // Create combat messages
    std::string hit_type_text;
    switch (result.hit_calc.result) {
        case HitResult::Critical:
            hit_type_text = " CRITICALLY";
            break;
        case HitResult::Glancing:
            hit_type_text = " glancingly";
            break;
        default:
            hit_type_text = "";
            break;
    }

    result.attacker_message = fmt::format("You{} hit {} for {:.0f} damage (margin: {:+d}).",
        hit_type_text, target->name(), damage, result.hit_calc.margin);
    result.target_message = fmt::format("{}{} hits you for {:.0f} damage.",
        attacker->name(), hit_type_text, damage);
    result.room_message = fmt::format("{}{} hits {} for {:.0f} damage.",
        attacker->name(), hit_type_text, target->name(), damage);

    // Send GMCP vitals update
    if (auto player = std::dynamic_pointer_cast<Player>(target)) {
        player->send_gmcp_vitals_update();
    }

    // Check for death
    if (target_stats.hit_points <= 0) {
        target_stats.hit_points = 0;
        target->set_position(Position::Ghost);

        Log::info("DEATH: {} killed {} - ending combat", attacker->name(), target->name());

        CombatManager::end_combat(attacker);
        CombatManager::end_combat(target);
        attacker->set_position(Position::Standing);

        Log::info("DEATH: Combat ended");

        long exp_gain = calculate_experience_gain(*attacker, *target);
        result.experience_gained = exp_gain;
        attacker->gain_experience(exp_gain);

        result.type = CombatResult::Type::Death;
        result.attacker_message += fmt::format(" You have killed {}! You gain {} experience.",
                                               target->display_name(), exp_gain);
        result.target_message += "\r\nYou are DEAD!";
        result.room_message += fmt::format(" {} is DEAD!", target->display_name());

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

bool ClassAbilities::rogue_sneak_attack_available(const Actor& attacker, const Actor& /* target */) {
    if (const auto* player = dynamic_cast<const Player*>(&attacker)) {
        return CombatSystem::string_to_class(player->player_class()) == CharacterClass::Rogue;
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

    // End existing combat for both actors
    end_combat(actor1);
    end_combat(actor2);

    // Set fighting position
    actor1->set_position(Position::Fighting);
    actor2->set_position(Position::Fighting);

    // Add combat pair
    active_combats_.emplace_back(actor1, actor2);

    Log::info("Combat started between {} and {} - {} active combats",
               actor1->name(), actor2->name(), active_combats_.size());
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
                Log::info("Ending combat involving {}", actor->name());
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
            Log::info("Executing combat round between {} and {}",
                      combat.actor1->name(), combat.actor2->name());
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
                Log::info("Cleaning up invalid combat pair");
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
            Log::info("Combat round - {}: {} | {}: {} | Room: {}",
                      first_attacker->name(), result1.attacker_message,
                      first_target->name(), result1.target_message,
                      result1.room_message);

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
            Log::info("Combat counter - {}: {} | {}: {} | Room: {}",
                      first_target->name(), result2.attacker_message,
                      first_attacker->name(), result2.target_message,
                      result2.room_message);

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

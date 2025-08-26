#include "combat.hpp"
#include "actor.hpp"
#include "logging.hpp"
#include <random>
#include <algorithm>
#include <unordered_map>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

namespace FieryMUD {

// Static member definitions
std::vector<std::pair<CombatEvent::EventType, CombatEventHandler>> CombatSystem::event_handlers_;
std::vector<CombatPair> CombatManager::active_combats_;

// ============================================================================
// CombatModifiers Implementation
// ============================================================================

CombatModifiers CombatModifiers::operator+(const CombatModifiers& other) const {
    CombatModifiers result;
    result.hit_bonus = hit_bonus + other.hit_bonus;
    result.damage_bonus = damage_bonus + other.damage_bonus;
    result.armor_class_bonus = armor_class_bonus + other.armor_class_bonus;
    result.critical_chance = std::min(0.95, critical_chance + other.critical_chance); // Cap at 95%
    result.damage_resistance = std::min(0.9, damage_resistance + other.damage_resistance); // Cap at 90%
    return result;
}

// ============================================================================
// CombatSystem Implementation
// ============================================================================

CombatModifiers CombatSystem::calculate_combat_modifiers(const Actor& actor) {
    CombatModifiers total_mods;
    
    // Try to cast to Player to get class and race
    if (const auto* player = dynamic_cast<const Player*>(&actor)) {
        CharacterClass char_class = string_to_class(player->player_class());
        CharacterRace char_race = string_to_race(player->race());
        
        CombatModifiers class_mods = get_class_combat_bonus(char_class, actor.stats().level);
        CombatModifiers race_mods = get_race_combat_bonus(char_race);
        
        total_mods = total_mods + class_mods + race_mods;
    }
    
    // Add base attribute modifiers
    const Stats& stats = actor.stats();
    total_mods.hit_bonus += Stats::attribute_modifier(stats.dexterity);
    total_mods.damage_bonus += Stats::attribute_modifier(stats.strength);
    total_mods.armor_class_bonus += Stats::attribute_modifier(stats.dexterity); // DEX improves AC
    
    // Add equipment bonuses (from existing stats)
    total_mods.hit_bonus += stats.hit_roll;
    total_mods.damage_bonus += stats.damage_roll;
    
    return total_mods;
}

CombatModifiers CombatSystem::get_class_combat_bonus(CharacterClass character_class, int level) {
    CombatModifiers mods;
    
    switch (character_class) {
        case CharacterClass::Warrior:
            // Warriors get significant combat bonuses
            mods.hit_bonus = level * 0.75;  // +0.75 hit per level
            mods.damage_bonus = level * 0.5; // +0.5 damage per level
            mods.armor_class_bonus = level * 0.25; // Better AC progression
            mods.critical_chance = 0.05 + (level * 0.002); // Slightly better crit chance
            break;
            
        case CharacterClass::Cleric:
            // Moderate combat with some defensive bonuses
            mods.hit_bonus = level * 0.5;
            mods.damage_bonus = level * 0.25;
            mods.armor_class_bonus = level * 0.15;
            mods.damage_resistance = level * 0.005; // Small damage resistance
            break;
            
        case CharacterClass::Sorcerer:
            // Weak physical combat but will get spell bonuses later
            mods.hit_bonus = level * 0.25;
            mods.damage_bonus = level * 0.1;
            mods.armor_class_bonus = 0; // No AC improvement
            // Sorcerers get spell damage bonuses (handled in ClassAbilities)
            break;
            
        case CharacterClass::Rogue:
            // High precision, lower raw damage, but critical hits
            mods.hit_bonus = level * 0.6;
            mods.damage_bonus = level * 0.3;
            mods.armor_class_bonus = level * 0.2; // DEX-based defense
            mods.critical_chance = 0.1 + (level * 0.003); // Better crit chance
            break;
    }
    
    return mods;
}

CombatModifiers CombatSystem::get_race_combat_bonus(CharacterRace race) {
    CombatModifiers mods;
    
    switch (race) {
        case CharacterRace::Human:
            // Versatile - small bonuses to everything
            mods.hit_bonus = 1.0;
            mods.damage_bonus = 1.0;
            mods.armor_class_bonus = 0.5;
            break;
            
        case CharacterRace::Elf:
            // Dexterous and magical
            mods.hit_bonus = 2.0;  // Good with ranged weapons
            mods.damage_bonus = 0.5;
            mods.armor_class_bonus = 1.5; // Natural grace
            mods.critical_chance = 0.02; // 2% bonus crit chance
            break;
            
        case CharacterRace::Dwarf:
            // Tough and strong
            mods.hit_bonus = 0.5;
            mods.damage_bonus = 2.0; // Strong attacks
            mods.armor_class_bonus = 0.0; // Slow but tough
            mods.damage_resistance = 0.05; // 5% damage resistance
            break;
            
        case CharacterRace::Halfling:
            // Lucky and evasive
            mods.hit_bonus = 1.5;
            mods.damage_bonus = -0.5; // Smaller build
            mods.armor_class_bonus = 2.0; // Hard to hit
            mods.critical_chance = 0.03; // Lucky crits
            break;
    }
    
    return mods;
}

CharacterClass CombatSystem::string_to_class(const std::string& class_name) {
    static const std::unordered_map<std::string, CharacterClass> class_map = {
        {"warrior", CharacterClass::Warrior},
        {"cleric", CharacterClass::Cleric}, 
        {"sorcerer", CharacterClass::Sorcerer},
        {"rogue", CharacterClass::Rogue}
    };
    
    auto it = class_map.find(class_name);
    return (it != class_map.end()) ? it->second : CharacterClass::Warrior; // Default to warrior
}

CharacterRace CombatSystem::string_to_race(const std::string& race_name) {
    static const std::unordered_map<std::string, CharacterRace> race_map = {
        {"human", CharacterRace::Human},
        {"elf", CharacterRace::Elf},
        {"dwarf", CharacterRace::Dwarf}, 
        {"halfling", CharacterRace::Halfling}
    };
    
    auto it = race_map.find(race_name);
    return (it != race_map.end()) ? it->second : CharacterRace::Human; // Default to human
}

CombatResult CombatSystem::perform_attack(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target) {
    if (!attacker || !target) {
        CombatResult result;
        result.attacker_message = "Invalid combat participants.";
        return result;
    }

    // Start combat if not already fighting
    if (!CombatManager::is_in_combat(*attacker) && !CombatManager::is_in_combat(*target)) {
        CombatManager::start_combat(attacker, target);
        fire_event(CombatEvent(CombatEvent::EventType::CombatStart, attacker, target));
    }
    
    // Fire combat start event
    fire_event(CombatEvent(CombatEvent::EventType::AttackAttempt, attacker, target));
    
    // Calculate modifiers for both participants
    CombatModifiers attacker_mods = calculate_combat_modifiers(*attacker);
    CombatModifiers target_mods = calculate_combat_modifiers(*target);
    
    CombatResult result;
    int roll, target_number;
    
    // Check for hit
    bool hit = calculate_hit(*attacker, attacker_mods, *target, target_mods, roll, target_number);
    
    if (!hit) {
        result.type = CombatResult::Type::Miss;
        result.attacker_message = "Your attack misses.";
        result.target_message = fmt::format("{} misses you.", attacker->name());
        result.room_message = fmt::format("{}'s attack misses {}.", attacker->name(), target->name());
        
        fire_event(CombatEvent(CombatEvent::EventType::AttackMiss, attacker, target));
        return result;
    }
    
    // Check for critical hit
    bool is_crit = is_critical_hit(attacker_mods);
    result.type = is_crit ? CombatResult::Type::CriticalHit : CombatResult::Type::Hit;
    
    // Calculate damage
    double damage = calculate_damage(*attacker, attacker_mods, *target, target_mods, is_crit);
    result.damage_dealt = damage;
    
    // Apply damage
    auto& target_stats = target->stats();
    int damage_int = static_cast<int>(std::round(damage));
    target_stats.hit_points -= damage_int;
    
    // Fire damage event
    fire_event(CombatEvent(CombatEvent::EventType::DamageDealt, attacker, target));
    
    // Create combat messages
    std::string crit_text = is_crit ? " CRITICALLY" : "";
    result.attacker_message = fmt::format("You{} hit {} for {:.1f} damage.", crit_text, target->name(), damage);
    result.target_message = fmt::format("{}{} hits you for {:.1f} damage.", attacker->name(), crit_text, damage);
    result.room_message = fmt::format("{}{} hits {} for {:.1f} damage.", attacker->name(), crit_text, target->name(), damage);
    
    // Send GMCP vitals update if target is a player
    if (auto player = std::dynamic_pointer_cast<Player>(target)) {
        player->send_gmcp_vitals_update();
    }
    
    fire_event(CombatEvent(CombatEvent::EventType::AttackHit, attacker, target));
    
    // Check for death
    if (target_stats.hit_points <= 0) {
        target_stats.hit_points = 0;
        target->set_position(Position::Ghost);
        
        Log::info("DEATH: {} killed {} - ending combat", attacker->name(), target->name());
        
        // End combat for both participants
        CombatManager::end_combat(attacker);
        CombatManager::end_combat(target);
        attacker->set_position(Position::Standing);
        
        Log::info("DEATH: Combat ended");
        
        // Calculate experience gain
        long exp_gain = calculate_experience_gain(*attacker, *target);
        result.experience_gained = exp_gain;
        attacker->gain_experience(exp_gain);
        
        result.type = CombatResult::Type::Death;
        result.attacker_message += fmt::format(" You have killed {}! You gain {} experience.", target->name(), exp_gain);
        result.target_message = "You are DEAD!";
        result.room_message += fmt::format(" {} is DEAD!", target->name());
        
        fire_event(CombatEvent(CombatEvent::EventType::ActorDeath, attacker, target));
        fire_event(CombatEvent(CombatEvent::EventType::CombatEnd, attacker, target));
    }
    
    return result;
}

double CombatSystem::calculate_damage(const Actor& attacker, const CombatModifiers& attacker_mods,
                                     const Actor& target, const CombatModifiers& target_mods, 
                                     bool is_critical) {
    // Base damage calculation
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<double> damage_die(1.0, 6.0); // d6 as double
    
    double base_damage = damage_die(gen);
    double total_damage = base_damage + attacker_mods.damage_bonus;
    
    // Critical hit doubles damage
    if (is_critical) {
        total_damage *= 2.0;
    }
    
    // Check for class-specific bonuses (rogue sneak attack)
    if (const auto* player = dynamic_cast<const Player*>(&attacker)) {
        CharacterClass char_class = string_to_class(player->player_class());
        if (char_class == CharacterClass::Rogue && ClassAbilities::rogue_sneak_attack_available(attacker, target)) {
            double sneak_bonus = ClassAbilities::rogue_sneak_attack_damage(attacker.stats().level);
            total_damage += sneak_bonus;
        }
    }
    
    // Apply target damage resistance
    total_damage *= (1.0 - target_mods.damage_resistance);
    
    // Ensure minimum damage
    return std::max(1.0, total_damage);
}

bool CombatSystem::calculate_hit(const Actor& attacker, const CombatModifiers& attacker_mods,
                                const Actor& target, const CombatModifiers& target_mods,
                                int& roll, int& target_number) {
    // d20 roll + modifiers vs target AC
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<int> d20(1, 20);
    
    roll = d20(gen);
    double total_hit = roll + attacker_mods.hit_bonus + attacker.stats().level;
    
    // Calculate effective AC (base AC - bonuses)
    double effective_ac = target.stats().armor_class - target_mods.armor_class_bonus;
    target_number = static_cast<int>(effective_ac);
    
    return total_hit >= effective_ac;
}

bool CombatSystem::is_critical_hit(const CombatModifiers& attacker_mods) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<double> chance(0.0, 1.0);
    
    return chance(gen) < attacker_mods.critical_chance;
}

long CombatSystem::calculate_experience_gain(const Actor& victor, const Actor& defeated) {
    // Experience based on level difference and defeated actor's level
    int level_diff = defeated.stats().level - victor.stats().level;
    long base_exp = defeated.stats().level * 100;
    
    // Bonus/penalty for level difference
    double multiplier = 1.0;
    if (level_diff > 0) {
        multiplier = 1.0 + (level_diff * 0.1); // Bonus for fighting higher level
    } else if (level_diff < 0) {
        multiplier = std::max(0.1, 1.0 + (level_diff * 0.05)); // Penalty for much lower level
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
    // Warriors get extra attacks at levels 6, 11, 16, 20
    int level = actor.stats().level;
    return level >= 6 && (level >= 20 || level >= 16 || level >= 11 || level >= 6);
}

bool ClassAbilities::rogue_sneak_attack_available(const Actor& attacker, const Actor& target) {
    // For now, simple check: rogue vs non-rogue
    // In future: check if target is unaware, flanked, etc.
    if (const auto* player = dynamic_cast<const Player*>(&attacker)) {
        return CombatSystem::string_to_class(player->player_class()) == CharacterClass::Rogue;
    }
    return false;
}

double ClassAbilities::rogue_sneak_attack_damage(int level) {
    // Sneak attack damage scales with level: 1d6 per 2 levels
    int dice_count = (level + 1) / 2; // 1 at level 1-2, 2 at level 3-4, etc.
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<double> d6(1.0, 6.0);
    
    double total = 0.0;
    for (int i = 0; i < dice_count; ++i) {
        total += d6(gen);
    }
    return total;
}

bool ClassAbilities::cleric_can_turn_undead(const Actor& actor) {
    // Placeholder for future undead system
    if (const auto* player = dynamic_cast<const Player*>(&actor)) {
        return CombatSystem::string_to_class(player->player_class()) == CharacterClass::Cleric && actor.stats().level >= 2;
    }
    return false;
}

double ClassAbilities::sorcerer_spell_damage_bonus(int level, int intelligence) {
    // Sorcerers get spell damage bonuses based on level and intelligence
    double level_bonus = level * 0.5;
    double int_bonus = Stats::attribute_modifier(intelligence) * 2.0;
    return level_bonus + int_bonus;
}

// ============================================================================
// CombatManager Implementation
// ============================================================================

void CombatManager::start_combat(std::shared_ptr<Actor> actor1, std::shared_ptr<Actor> actor2) {
    if (!actor1 || !actor2) return;
    
    // Check if either actor is already in combat with the other
    for (const auto& combat : active_combats_) {
        if (combat.involves_actor(*actor1) && combat.involves_actor(*actor2)) {
            // Already fighting each other
            return;
        }
    }
    
    // End any existing combat for both actors first
    end_combat(actor1);
    end_combat(actor2);
    
    // Set both actors to fighting position
    actor1->set_position(Position::Fighting);
    actor2->set_position(Position::Fighting);
    
    // Add new combat pair
    active_combats_.emplace_back(actor1, actor2);
    
    Log::info("Combat started between {} and {} - {} active combats", 
               actor1->name(), actor2->name(), active_combats_.size());
}

void CombatManager::end_combat(std::shared_ptr<Actor> actor) {
    if (!actor) return;
    
    // Remove all combat pairs involving this actor
    auto it = std::remove_if(active_combats_.begin(), active_combats_.end(),
        [&actor](const CombatPair& combat) {
            bool involves = combat.involves_actor(*actor);
            if (involves) {
                // Set opponents back to standing if they're not dead
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

void CombatManager::process_combat_rounds() {
    cleanup_invalid_combats();
    
    auto now = std::chrono::steady_clock::now();
    
    for (auto& combat : active_combats_) {
        if (combat.is_ready_for_next_round()) {
            Log::info("Executing combat round between {} and {}", 
                      combat.actor1->name(), combat.actor2->name());
            execute_round(combat);
            combat.update_last_round();
        } else {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - combat.last_round).count();
            if (elapsed < 5000) { // Only log for first 5 seconds to avoid spam
                Log::info("Combat not ready - elapsed: {}ms (need 3000ms)", elapsed);
            }
        }
    }
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
    // Set all fighting actors back to standing
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
            // Remove if either actor is null, dead, or no longer in fighting position
            bool invalid = !combat.actor1 || !combat.actor2 ||
                          (!combat.actor1->is_alive()) ||
                          (!combat.actor2->is_alive());
            
            // Also remove if actors are no longer in the same room
            if (!invalid && combat.actor1->current_room() != combat.actor2->current_room()) {
                invalid = true;
                Log::debug("Combat ended: actors in different rooms");
            }
            
            if (invalid) {
                Log::info("Cleaning up invalid combat pair - actor1: {} (alive: {}, pos: {}), actor2: {} (alive: {}, pos: {})", 
                         combat.actor1 ? combat.actor1->name() : "null", 
                         combat.actor1 ? (combat.actor1->is_alive() ? "yes" : "no") : "n/a",
                         combat.actor1 ? magic_enum::enum_name(combat.actor1->position()) : "n/a",
                         combat.actor2 ? combat.actor2->name() : "null",
                         combat.actor2 ? (combat.actor2->is_alive() ? "yes" : "no") : "n/a",
                         combat.actor2 ? magic_enum::enum_name(combat.actor2->position()) : "n/a");
                // Set survivors back to standing
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
    
    // Randomly determine who attacks first this round
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<int> coinflip(0, 1);
    
    std::shared_ptr<Actor> first_attacker, first_target;
    if (coinflip(gen) == 0) {
        first_attacker = combat_pair.actor1;
        first_target = combat_pair.actor2;
    } else {
        first_attacker = combat_pair.actor2;
        first_target = combat_pair.actor1;
    }
    
    // First actor attacks
    if (first_attacker->is_alive() && first_target->is_alive()) {
        auto result1 = CombatSystem::perform_attack(first_attacker, first_target);
        
        // Send messages to room and participants
        if (auto room = first_attacker->current_room()) {
            Log::info("Combat round - {}: {} | {}: {} | Room: {}", 
                      first_attacker->name(), result1.attacker_message,
                      first_target->name(), result1.target_message,
                      result1.room_message);
            
            // Send messages to the actors
            first_attacker->send_message(result1.attacker_message + "\r\n");
            first_target->send_message(result1.target_message + "\r\n");
            
            // TODO: Send room message to other people in room
        }
        
        // If target died, combat will be ended by perform_attack
        if (result1.type == CombatResult::Type::Death) {
            return;
        }
    }
    
    // Second actor counter-attacks if still alive
    if (first_target->is_alive() && first_attacker->is_alive()) {
        auto result2 = CombatSystem::perform_attack(first_target, first_attacker);
        
        // Send messages to room and participants
        if (auto room = first_target->current_room()) {
            Log::info("Combat counter - {}: {} | {}: {} | Room: {}", 
                      first_target->name(), result2.attacker_message,
                      first_attacker->name(), result2.target_message,
                      result2.room_message);
            
            // Send messages to the actors
            first_target->send_message(result2.attacker_message + "\r\n");
            first_attacker->send_message(result2.target_message + "\r\n");
            
            // TODO: Send room message to other people in room
        }
    }
}

} // namespace FieryMUD
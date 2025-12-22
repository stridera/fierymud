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

// Combat system constants
namespace {
    // Combat modifier caps
    constexpr double MAX_CRITICAL_CHANCE = 0.95;    // Cap at 95%
    constexpr double MAX_DAMAGE_RESISTANCE = 0.9;   // Cap at 90%

    // Class combat scaling per level
    constexpr double WARRIOR_HIT_PER_LEVEL = 0.75;
    constexpr double WARRIOR_DAMAGE_PER_LEVEL = 0.5;
    constexpr double WARRIOR_AC_PER_LEVEL = 0.25;
    constexpr double WARRIOR_CRIT_BASE = 0.05;
    constexpr double WARRIOR_CRIT_PER_LEVEL = 0.002;

    constexpr double CLERIC_HIT_PER_LEVEL = 0.5;
    constexpr double CLERIC_DAMAGE_PER_LEVEL = 0.25;
    constexpr double CLERIC_AC_PER_LEVEL = 0.15;
    constexpr double CLERIC_RESIST_PER_LEVEL = 0.005;

    constexpr double SORCERER_HIT_PER_LEVEL = 0.25;
    constexpr double SORCERER_DAMAGE_PER_LEVEL = 0.1;

    constexpr double ROGUE_HIT_PER_LEVEL = 0.6;
    constexpr double ROGUE_DAMAGE_PER_LEVEL = 0.3;
    constexpr double ROGUE_AC_PER_LEVEL = 0.2;
    constexpr double ROGUE_CRIT_BASE = 0.1;
    constexpr double ROGUE_CRIT_PER_LEVEL = 0.003;

    // Race bonus constants - hit/damage/AC
    constexpr double HUMAN_HIT_BONUS = 1.0;
    constexpr double HUMAN_DAMAGE_BONUS = 1.0;
    constexpr double HUMAN_AC_BONUS = 0.5;

    constexpr double ELF_HIT_BONUS = 2.0;
    constexpr double ELF_DAMAGE_BONUS = 0.5;
    constexpr double ELF_AC_BONUS = 1.5;
    constexpr double ELF_CRIT_BONUS = 0.02;

    constexpr double DWARF_HIT_BONUS = 0.5;
    constexpr double DWARF_DAMAGE_BONUS = 2.0;
    constexpr double DWARF_AC_BONUS = 0.0;
    constexpr double DWARF_RESIST_BONUS = 0.05;

    constexpr double HALFLING_HIT_BONUS = 1.5;
    constexpr double HALFLING_DAMAGE_BONUS = -0.5;
    constexpr double HALFLING_AC_BONUS = 2.0;
    constexpr double HALFLING_CRIT_BONUS = 0.03;

    // Experience calculation
    constexpr int EXP_BASE_PER_LEVEL = 100;
    constexpr double EXP_HIGHER_LEVEL_BONUS = 0.1;   // Bonus per level when fighting higher
    constexpr double EXP_LOWER_LEVEL_PENALTY = 0.05; // Penalty per level when fighting lower
    constexpr double EXP_MIN_MULTIPLIER = 0.1;

    // Extra attack level thresholds (warrior class)
    constexpr int EXTRA_ATTACK_LEVEL_1 = 6;
    constexpr int EXTRA_ATTACK_LEVEL_2 = 11;
    constexpr int EXTRA_ATTACK_LEVEL_3 = 16;
    constexpr int EXTRA_ATTACK_LEVEL_4 = 20;

    // Combat round timing (ms)
    constexpr int COMBAT_ROUND_MS = 3000;
    constexpr int COMBAT_LOG_SPAM_THRESHOLD_MS = 5000;

    // Die bounds
    constexpr int D20_MIN = 1;
    constexpr int D20_MAX = 20;
    constexpr double D6_MIN = 1.0;
    constexpr double D6_MAX = 6.0;

    // Damage constants
    constexpr double CRITICAL_HIT_MULTIPLIER = 2.0;
    constexpr double MIN_DAMAGE = 1.0;

    // Sneak attack scaling
    constexpr int SNEAK_ATTACK_LEVEL_DIVISOR = 2;

    // Cleric abilities
    constexpr int CLERIC_TURN_UNDEAD_MIN_LEVEL = 2;

    // Sorcerer spell damage scaling
    constexpr double SORCERER_SPELL_LEVEL_BONUS = 0.5;
    constexpr double SORCERER_INT_BONUS_MULTIPLIER = 2.0;
}

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
    result.critical_chance = std::min(MAX_CRITICAL_CHANCE, critical_chance + other.critical_chance);
    result.damage_resistance = std::min(MAX_DAMAGE_RESISTANCE, damage_resistance + other.damage_resistance);
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
            mods.hit_bonus = level * WARRIOR_HIT_PER_LEVEL;
            mods.damage_bonus = level * WARRIOR_DAMAGE_PER_LEVEL;
            mods.armor_class_bonus = level * WARRIOR_AC_PER_LEVEL;
            mods.critical_chance = WARRIOR_CRIT_BASE + (level * WARRIOR_CRIT_PER_LEVEL);
            break;

        case CharacterClass::Cleric:
            // Moderate combat with some defensive bonuses
            mods.hit_bonus = level * CLERIC_HIT_PER_LEVEL;
            mods.damage_bonus = level * CLERIC_DAMAGE_PER_LEVEL;
            mods.armor_class_bonus = level * CLERIC_AC_PER_LEVEL;
            mods.damage_resistance = level * CLERIC_RESIST_PER_LEVEL;
            break;

        case CharacterClass::Sorcerer:
            // Weak physical combat but will get spell bonuses later
            mods.hit_bonus = level * SORCERER_HIT_PER_LEVEL;
            mods.damage_bonus = level * SORCERER_DAMAGE_PER_LEVEL;
            mods.armor_class_bonus = 0; // No AC improvement
            // Sorcerers get spell damage bonuses (handled in ClassAbilities)
            break;

        case CharacterClass::Rogue:
            // High precision, lower raw damage, but critical hits
            mods.hit_bonus = level * ROGUE_HIT_PER_LEVEL;
            mods.damage_bonus = level * ROGUE_DAMAGE_PER_LEVEL;
            mods.armor_class_bonus = level * ROGUE_AC_PER_LEVEL;
            mods.critical_chance = ROGUE_CRIT_BASE + (level * ROGUE_CRIT_PER_LEVEL);
            break;
    }

    return mods;
}

CombatModifiers CombatSystem::get_race_combat_bonus(CharacterRace race) {
    CombatModifiers mods;

    switch (race) {
        case CharacterRace::Human:
            // Versatile - small bonuses to everything
            mods.hit_bonus = HUMAN_HIT_BONUS;
            mods.damage_bonus = HUMAN_DAMAGE_BONUS;
            mods.armor_class_bonus = HUMAN_AC_BONUS;
            break;

        case CharacterRace::Elf:
            // Dexterous and magical
            mods.hit_bonus = ELF_HIT_BONUS;
            mods.damage_bonus = ELF_DAMAGE_BONUS;
            mods.armor_class_bonus = ELF_AC_BONUS;
            mods.critical_chance = ELF_CRIT_BONUS;
            break;

        case CharacterRace::Dwarf:
            // Tough and strong
            mods.hit_bonus = DWARF_HIT_BONUS;
            mods.damage_bonus = DWARF_DAMAGE_BONUS;
            mods.armor_class_bonus = DWARF_AC_BONUS;
            mods.damage_resistance = DWARF_RESIST_BONUS;
            break;

        case CharacterRace::Halfling:
            // Lucky and evasive
            mods.hit_bonus = HALFLING_HIT_BONUS;
            mods.damage_bonus = HALFLING_DAMAGE_BONUS;
            mods.armor_class_bonus = HALFLING_AC_BONUS;
            mods.critical_chance = HALFLING_CRIT_BONUS;
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

    // Convert to lowercase for case-insensitive matching
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
        {"halfelf", CharacterRace::Elf},  // Half-elf uses elf bonuses
        {"half-elf", CharacterRace::Elf},
        {"dwarf", CharacterRace::Dwarf},
        {"halfling", CharacterRace::Halfling}
    };

    // Convert to lowercase for case-insensitive matching
    std::string lower_name;
    lower_name.reserve(race_name.size());
    for (char c : race_name) {
        lower_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    auto it = race_map.find(lower_name);
    return (it != race_map.end()) ? it->second : CharacterRace::Human;
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
        result.target_message = fmt::format("{} misses you.", attacker->display_name());
        result.room_message = fmt::format("{}'s attack misses {}.", attacker->display_name(), target->display_name());
        
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
        result.attacker_message += fmt::format(" You have killed {}! You gain {} experience.",
                                               target->display_name(), exp_gain);
        result.target_message += "\r\nYou are DEAD!";
        result.room_message += fmt::format(" {} is DEAD!", target->display_name());
        
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

    double base_damage = 0.0;

    // Check if attacker is a Mobile with custom damage dice
    if (const auto* mobile = dynamic_cast<const Mobile*>(&attacker)) {
        // Use mob's bare hand damage dice: num d size + bonus
        int dice_num = mobile->bare_hand_damage_dice_num();
        int dice_size = mobile->bare_hand_damage_dice_size();
        int dice_bonus = mobile->bare_hand_damage_dice_bonus();

        // Roll each die and sum
        std::uniform_int_distribution<int> die(1, std::max(1, dice_size));
        for (int i = 0; i < dice_num; ++i) {
            base_damage += die(gen);
        }
        base_damage += dice_bonus;
    } else {
        // Default to 1d6 for players (will be replaced with weapon damage later)
        std::uniform_real_distribution<double> damage_die(D6_MIN, D6_MAX);
        base_damage = damage_die(gen);
    }

    double total_damage = base_damage + attacker_mods.damage_bonus;

    // Critical hit multiplies damage
    if (is_critical) {
        total_damage *= CRITICAL_HIT_MULTIPLIER;
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
    return std::max(MIN_DAMAGE, total_damage);
}

bool CombatSystem::calculate_hit(const Actor& attacker, const CombatModifiers& attacker_mods,
                                const Actor& target, const CombatModifiers& target_mods,
                                int& roll, int& target_number) {
    // d20 roll + modifiers vs target AC
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<int> d20(D20_MIN, D20_MAX);
    
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
    long base_exp = defeated.stats().level * EXP_BASE_PER_LEVEL;

    // Bonus/penalty for level difference
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
    // Warriors get extra attacks at levels 6, 11, 16, 20
    int level = actor.stats().level;
    return level >= EXTRA_ATTACK_LEVEL_1;
}

bool ClassAbilities::rogue_sneak_attack_available(const Actor& attacker, const Actor& /* target */) {
    // For now, simple check: rogue vs non-rogue
    // In future: check if target is unaware, flanked, etc.
    if (const auto* player = dynamic_cast<const Player*>(&attacker)) {
        return CombatSystem::string_to_class(player->player_class()) == CharacterClass::Rogue;
    }
    return false;
}

double ClassAbilities::rogue_sneak_attack_damage(int level) {
    // Sneak attack damage scales with level: 1d6 per 2 levels
    int dice_count = (level + 1) / SNEAK_ATTACK_LEVEL_DIVISOR;
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<double> d6(D6_MIN, D6_MAX);

    double total = 0.0;
    for (int i = 0; i < dice_count; ++i) {
        total += d6(gen);
    }
    return total;
}

bool ClassAbilities::cleric_can_turn_undead(const Actor& actor) {
    // Placeholder for future undead system
    if (const auto* player = dynamic_cast<const Player*>(&actor)) {
        return CombatSystem::string_to_class(player->player_class()) == CharacterClass::Cleric &&
               actor.stats().level >= CLERIC_TURN_UNDEAD_MIN_LEVEL;
    }
    return false;
}

double ClassAbilities::sorcerer_spell_damage_bonus(int level, int intelligence) {
    // Sorcerers get spell damage bonuses based on level and intelligence
    double level_bonus = level * SORCERER_SPELL_LEVEL_BONUS;
    double int_bonus = Stats::attribute_modifier(intelligence) * SORCERER_INT_BONUS_MULTIPLIER;
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

bool CombatManager::process_combat_rounds() {
    cleanup_invalid_combats();
    
    // Process combat rounds
    // auto now = std::chrono::steady_clock::now();
    bool rounds_processed = false;
    
    for (auto& combat : active_combats_) {
        if (combat.is_ready_for_next_round()) {
            Log::info("Executing combat round between {} and {}", 
                      combat.actor1->name(), combat.actor2->name());
            execute_round(combat);
            combat.update_last_round();
            rounds_processed = true;
        } else {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - combat.last_round).count();
            if (elapsed < COMBAT_LOG_SPAM_THRESHOLD_MS) {
                Log::info("Combat not ready - elapsed: {}ms (need {}ms)", elapsed, COMBAT_ROUND_MS);
            }
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
            
            // Send room message to others in room
            if (!result1.room_message.empty()) {
                if (auto room = first_target->current_room()) {
                    // Send message to everyone except the combatants
                    for (const auto& actor : room->contents().actors) {
                        if (actor && actor != first_attacker && actor != first_target) {
                            actor->send_message(result1.room_message + "\r\n");
                        }
                    }
                }
            }
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
            
            // Send room message to others in room
            if (!result2.room_message.empty()) {
                // Send message to everyone except the combatants
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
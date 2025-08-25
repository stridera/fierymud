#pragma once

#include "actor.hpp"
#include "../game/login_system.hpp"  // For CharacterClass and CharacterRace enums
#include <memory>
#include <functional>
#include <vector>
#include <chrono>

namespace FieryMUD {

/**
 * @brief Combat-related modifiers and calculations
 */
struct CombatModifiers {
    double hit_bonus = 0.0;        // Attack roll bonus
    double damage_bonus = 0.0;     // Damage bonus
    double armor_class_bonus = 0.0; // AC improvement (subtracted from AC)
    double critical_chance = 0.05;  // 5% base critical chance
    double damage_resistance = 0.0; // Damage reduction (0.0 = no reduction, 0.5 = 50% reduction)
    
    /** Combine two modifier sets */
    CombatModifiers operator+(const CombatModifiers& other) const;
};

/**
 * @brief Combat result information
 */
struct CombatResult {
    enum class Type { Miss, Hit, CriticalHit, Death };
    
    Type type = Type::Miss;
    double damage_dealt = 0.0;
    int experience_gained = 0;
    std::string attacker_message;
    std::string target_message;  
    std::string room_message;
};

/**
 * @brief Combat event base class for event system
 */
class CombatEvent {
public:
    enum class EventType { 
        AttackAttempt,  // Before attack roll
        AttackHit,      // After successful hit
        AttackMiss,     // After missed attack
        DamageDealt,    // After damage calculation
        ActorDeath,     // When actor dies
        CombatStart,    // When combat begins
        CombatEnd       // When combat ends
    };
    
    CombatEvent(EventType type, std::shared_ptr<Actor> source, std::shared_ptr<Actor> target = nullptr)
        : type_(type), source_(source), target_(target) {}
        
    virtual ~CombatEvent() = default;
    
    EventType type() const { return type_; }
    std::shared_ptr<Actor> source() const { return source_; }
    std::shared_ptr<Actor> target() const { return target_; }
    
private:
    EventType type_;
    std::shared_ptr<Actor> source_;
    std::shared_ptr<Actor> target_;
};

/**
 * @brief Combat event handler function type
 */
using CombatEventHandler = std::function<void(const CombatEvent&)>;

/**
 * @brief Modern combat system with class/race specific bonuses and floating-point calculations
 */
class CombatSystem {
public:
    /**
     * @brief Calculate combat modifiers based on actor's class and race
     */
    static CombatModifiers calculate_combat_modifiers(const Actor& actor);
    
    /**
     * @brief Calculate class-specific combat bonuses
     */
    static CombatModifiers get_class_combat_bonus(CharacterClass character_class, int level);
    
    /**
     * @brief Calculate race-specific combat bonuses  
     */
    static CombatModifiers get_race_combat_bonus(CharacterRace race);
    
    /**
     * @brief Convert string class name to enum (for Player class compatibility)
     */
    static CharacterClass string_to_class(const std::string& class_name);
    
    /**
     * @brief Convert string race name to enum (for Player class compatibility)
     */
    static CharacterRace string_to_race(const std::string& race_name);
    
    /**
     * @brief Perform a modern combat attack with all modifiers
     * @param attacker The attacking actor
     * @param target The target being attacked
     * @return CombatResult with detailed information about the attack
     */
    static CombatResult perform_attack(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target);
    
    /**
     * @brief Calculate total damage with all modifiers applied
     */
    static double calculate_damage(const Actor& attacker, const CombatModifiers& attacker_mods, 
                                   const Actor& target, const CombatModifiers& target_mods,
                                   bool is_critical = false);
    
    /**
     * @brief Calculate to-hit chance with all modifiers
     */
    static bool calculate_hit(const Actor& attacker, const CombatModifiers& attacker_mods,
                             const Actor& target, const CombatModifiers& target_mods,
                             int& roll, int& target_number);
    
    /**
     * @brief Check if attack is a critical hit
     */
    static bool is_critical_hit(const CombatModifiers& attacker_mods);
    
    /**
     * @brief Calculate experience gain from defeating target
     */
    static long calculate_experience_gain(const Actor& victor, const Actor& defeated);
    
    /**
     * @brief Register combat event handler
     */
    static void register_event_handler(CombatEvent::EventType event_type, CombatEventHandler handler);
    
    /**
     * @brief Fire combat event to all registered handlers
     */
    static void fire_event(const CombatEvent& event);
    
private:
    // Event system storage
    static std::vector<std::pair<CombatEvent::EventType, CombatEventHandler>> event_handlers_;
};

/**
 * @brief Combat pairing structure to track opponents and timing
 */
struct CombatPair {
    std::shared_ptr<Actor> actor1;
    std::shared_ptr<Actor> actor2;
    std::chrono::steady_clock::time_point last_round;
    std::chrono::seconds round_interval{3}; // 3 seconds between rounds
    
    CombatPair(std::shared_ptr<Actor> a1, std::shared_ptr<Actor> a2) 
        : actor1(std::move(a1)), actor2(std::move(a2)), last_round(std::chrono::steady_clock::now()) {}
        
    bool involves_actor(const Actor& actor) const {
        return (actor1.get() == &actor) || (actor2.get() == &actor);
    }
    
    std::shared_ptr<Actor> get_opponent(const Actor& actor) const {
        if (actor1.get() == &actor) return actor2;
        if (actor2.get() == &actor) return actor1;
        return nullptr;
    }
    
    bool is_ready_for_next_round() const {
        auto now = std::chrono::steady_clock::now();
        return (now - last_round) >= round_interval;
    }
    
    void update_last_round() {
        last_round = std::chrono::steady_clock::now();
    }
};

/**
 * @brief Manages ongoing combat encounters with time-based rounds
 */
class CombatManager {
public:
    /**
     * @brief Start combat between two actors
     */
    static void start_combat(std::shared_ptr<Actor> actor1, std::shared_ptr<Actor> actor2);
    
    /**
     * @brief End combat for specific actor (removes from all fights)
     */
    static void end_combat(std::shared_ptr<Actor> actor);
    
    /**
     * @brief Process all active combat rounds (called from WorldServer::update())
     */
    static void process_combat_rounds();
    
    /**
     * @brief Check if actor is currently in combat
     */
    static bool is_in_combat(const Actor& actor);
    
    /**
     * @brief Get opponent for given actor (if in combat)
     */
    static std::shared_ptr<Actor> get_opponent(const Actor& actor);
    
    /**
     * @brief Clear all active combat (for shutdown/testing)
     */
    static void clear_all_combat();
    
private:
    static std::vector<CombatPair> active_combats_;
    
    /**
     * @brief Remove invalid combat pairs (dead actors, etc.)
     */
    static void cleanup_invalid_combats();
    
    /**
     * @brief Execute combat round between two actors
     */
    static void execute_round(CombatPair& combat_pair);
};

/**
 * @brief Class-specific special abilities
 */
class ClassAbilities {
public:
    /**
     * @brief Check if warrior can perform extra attack
     */
    static bool warrior_extra_attack_available(const Actor& actor);
    
    /**
     * @brief Check if rogue can perform sneak attack
     */
    static bool rogue_sneak_attack_available(const Actor& attacker, const Actor& target);
    
    /**
     * @brief Calculate rogue sneak attack damage bonus
     */
    static double rogue_sneak_attack_damage(int level);
    
    /**
     * @brief Check if cleric can turn undead (placeholder for future undead system)
     */
    static bool cleric_can_turn_undead(const Actor& actor);
    
    /**
     * @brief Calculate sorcerer spell damage bonus
     */
    static double sorcerer_spell_damage_bonus(int level, int intelligence);
};

} // namespace FieryMUD
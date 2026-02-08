#pragma once

#include "../game/login_system.hpp" // For CharacterClass and CharacterRace enums
#include "actor.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace FieryMUD {

// ============================================================================
// Combat Constants (from COMBAT_REVERSE_ENGINEERING.md)
// ============================================================================

namespace CombatConstants {
// Timing - Base attack speed (medium weapons)
constexpr int COMBAT_ROUND_SECONDS = 4;
constexpr int COMBAT_ROUND_MS = 4000;
constexpr int BASE_ATTACK_SPEED_MS = 4000;

// Attack speed by weapon category (milliseconds)
constexpr int ATTACK_SPEED_VERY_FAST = 2500; // Daggers, punches
constexpr int ATTACK_SPEED_FAST = 3000;      // Short swords, clubs
constexpr int ATTACK_SPEED_MEDIUM = 4000;    // Long swords, maces
constexpr int ATTACK_SPEED_SLOW = 5000;      // Two-handed swords, axes
constexpr int ATTACK_SPEED_VERY_SLOW = 6000; // Massive weapons, polearms

// Haste/Slow effect multipliers (percentage of base speed)
constexpr double HASTE_MULTIPLIER = 0.65;        // 35% faster attacks
constexpr double SLOW_MULTIPLIER = 1.50;         // 50% slower attacks
constexpr double DOUBLE_HASTE_MULTIPLIER = 0.50; // 50% faster (stacked haste)
constexpr int MIN_ATTACK_SPEED_MS = 1500;        // Minimum attack interval (caps haste)
} // namespace CombatConstants

// ============================================================================
// Weapon Speed Categories
// ============================================================================

enum class WeaponSpeed {
    VeryFast, // Daggers, punches, claws
    Fast,     // Short swords, clubs, whips
    Medium,   // Long swords, maces, spears (default)
    Slow,     // Two-handed swords, battleaxes
    VerySlow  // Massive weapons, polearms, mauls
};

/** Get attack speed in milliseconds for weapon speed category */
constexpr int get_attack_speed_ms(WeaponSpeed speed) {
    switch (speed) {
    case WeaponSpeed::VeryFast:
        return CombatConstants::ATTACK_SPEED_VERY_FAST;
    case WeaponSpeed::Fast:
        return CombatConstants::ATTACK_SPEED_FAST;
    case WeaponSpeed::Medium:
        return CombatConstants::ATTACK_SPEED_MEDIUM;
    case WeaponSpeed::Slow:
        return CombatConstants::ATTACK_SPEED_SLOW;
    case WeaponSpeed::VerySlow:
        return CombatConstants::ATTACK_SPEED_VERY_SLOW;
    default:
        return CombatConstants::ATTACK_SPEED_MEDIUM;
    }
}

namespace CombatConstants {
// Hit calculation bounds
constexpr int D100_MIN = 1;
constexpr int D100_MAX = 100;
constexpr int ACC_SOFT_CAP = 200;
constexpr int EVA_SOFT_CAP = 150;

// Hit thresholds (margin = attacker_roll - defender_roll)
constexpr int CRITICAL_HIT_MARGIN = 50; // margin >= 50 = critical
constexpr int HIT_MARGIN = 10;          // margin >= 10 = hit
constexpr int GLANCING_MARGIN = -10;    // margin >= -10 = glancing
// margin < -10 = miss

// Damage multipliers
constexpr double CRITICAL_MULTIPLIER = 2.0;
constexpr double GLANCING_MULTIPLIER = 0.5;
constexpr double MIN_DAMAGE = 1.0;

// Damage reduction caps
constexpr double DR_CAP = 0.75;      // 75% max damage reduction
constexpr double PEN_PCT_CAP = 0.50; // 50% max penetration

// DR K constants (AR / (AR + K) = DR%)
constexpr double DR_K_LOW_TIER = 40.0;  // Levels 1-20
constexpr double DR_K_MID_TIER = 60.0;  // Levels 21-50
constexpr double DR_K_HIGH_TIER = 80.0; // Levels 51+

// Base stats
constexpr double ACC_BASE = 50.0;
constexpr double EVA_BASE = 30.0;
constexpr double EVA_LEVEL_RATE = 0.3;

// Stat contribution rates
constexpr double STAT_NEUTRAL = 50.0;       // Stats are centered at 50
constexpr double STR_TO_ACC_DIVISOR = 5.0;  // (STR - 50) / 5
constexpr double INT_TO_ACC_DIVISOR = 10.0; // (INT - 50) / 10
constexpr double WIS_TO_ACC_DIVISOR = 10.0; // (WIS - 50) / 10
constexpr double DEX_TO_EVA_DIVISOR = 3.0;  // (DEX - 50) / 3

// Skill contribution rates
constexpr double WEAPON_PROF_TO_ACC = 0.5; // prof / 2
constexpr double DODGE_TO_EVA = 0.2;       // dodge / 5
constexpr double PARRY_TO_EVA = 0.2;       // parry / 5
constexpr double RIPOSTE_TO_EVA = 0.1;     // riposte / 10

// Soak values by armor type
constexpr int SOAK_PLATE = 5;
constexpr int SOAK_CHAIN = 3;
constexpr int SOAK_LEATHER = 1;

// Penetration by weapon type
constexpr double PEN_PIERCING = 0.20;
constexpr double PEN_SLASHING = 0.10;
constexpr double PEN_CRUSHING = 0.05;

// Stance multipliers (damage received)
constexpr double STANCE_FIGHTING = 1.0;
constexpr double STANCE_ALERT = 1.33;
constexpr double STANCE_RESTING = 1.66;
constexpr double STANCE_SLEEPING = 2.0;
constexpr double STANCE_STUNNED = 2.33;
constexpr double STANCE_INCAP = 2.66;
constexpr double STANCE_MORT = 3.0;
} // namespace CombatConstants

// ============================================================================
// Class ACC Rates (per level)
// ============================================================================

namespace ClassAccRates {
constexpr double WARRIOR = 0.65; // Reaches 115 ACC at level 100
constexpr double PALADIN = 0.65;
constexpr double RANGER = 0.65;
constexpr double ROGUE = 0.55;    // Reaches 105 ACC at level 100 (was Thief)
constexpr double CLERIC = 0.40;   // Reaches 90 ACC at level 100
constexpr double SORCERER = 0.30; // Reaches 80 ACC at level 100
} // namespace ClassAccRates

// ============================================================================
// Combat Stats Structure (modern ACC/EVA/AR/DR% system)
// ============================================================================

/**
 * @brief Combat statistics for the modern combat system
 *
 * Based on COMBAT_REVERSE_ENGINEERING.md proposal:
 * - ACC vs EVA: Bounded accuracy with opposed d100 rolls
 * - AR → DR%: Armor Rating converted to % damage reduction
 * - Soak: Flat damage reduction from heavy armor
 * - Penetration: AP (flat) and Pen% counter Soak and DR%
 */
struct CombatStats {
    // Offensive stats
    double acc = 50.0;       // Accuracy (hit chance modifier)
    double ap = 0.0;         // Attack Power (flat damage bonus)
    double pen_flat = 0.0;   // Flat penetration (reduces target Soak)
    double pen_pct = 0.0;    // Percentage penetration (reduces target DR%)
    double crit_bonus = 0.0; // Bonus to critical hit margin

    // Defensive stats
    double eva = 30.0;     // Evasion (dodge chance modifier)
    double ar = 0.0;       // Armor Rating (converted to DR%)
    double dr_pct = 0.0;   // Damage Reduction % (calculated from AR)
    double soak = 0.0;     // Flat damage reduction (before DR%)
    double ward_pct = 0.0; // Magic damage reduction %

    // Resistance percentages by damage type (0-100 scale, 100 = normal)
    double res_fire = 100.0;
    double res_cold = 100.0;
    double res_shock = 100.0;
    double res_acid = 100.0;
    double res_poison = 100.0;
    double res_physical = 100.0;

    // Weapon stats (for current equipped weapon)
    int weapon_dice_num = 1;
    int weapon_dice_size = 6;
    int weapon_base_damage = 0;
    WeaponSpeed weapon_speed = WeaponSpeed::Medium;

    // Attack speed modifiers (from effects like haste/slow)
    double attack_speed_multiplier = 1.0; // <1.0 = faster, >1.0 = slower
    bool has_haste = false;
    bool has_slow = false;

    /** Get effective attack speed in milliseconds */
    int effective_attack_speed_ms() const {
        int base_speed = FieryMUD::get_attack_speed_ms(weapon_speed);
        double modified = base_speed * attack_speed_multiplier;
        return std::max(CombatConstants::MIN_ATTACK_SPEED_MS, static_cast<int>(modified));
    }

    /** Calculate DR% from AR using diminishing returns formula */
    static double calculate_dr_percent(double ar, int level);

    /** Get K constant for DR calculation based on level tier */
    static double get_dr_k_constant(int level);

    /** Combine two stat sets (additive) */
    CombatStats operator+(const CombatStats &other) const;
};

// ============================================================================
// Hit Result Types
// ============================================================================

enum class HitResult {
    Miss,     // Complete miss
    Glancing, // Partial hit (50% damage)
    Hit,      // Normal hit
    Critical  // Critical hit (200% damage)
};

/**
 * @brief Result of a hit calculation with details
 */
struct HitCalcResult {
    HitResult result = HitResult::Miss;
    int attacker_roll = 0;     // Natural d100 roll
    int defender_roll = 0;     // Natural d100 roll
    double attacker_total = 0; // Roll + ACC
    double defender_total = 0; // Roll + EVA
    int margin = 0;            // attacker_total - defender_total
    bool auto_hit = false;     // Natural 100 on d100
    bool auto_miss = false;    // Natural 1 on d100
};

// ============================================================================
// Combat Result
// ============================================================================

/**
 * @brief Combat result information
 */
struct CombatResult {
    enum class Type { Miss, Glancing, Hit, CriticalHit, Death };

    Type type = Type::Miss;
    double damage_dealt = 0.0;
    double damage_before_mitigation = 0.0;
    double damage_after_soak = 0.0;
    double damage_after_dr = 0.0;
    long experience_gained = 0;
    std::string attacker_message;
    std::string target_message;
    std::string room_message;

    // Detailed hit info
    HitCalcResult hit_calc;
};

// ============================================================================
// Combat Events
// ============================================================================

/**
 * @brief Combat event base class for event system
 */
class CombatEvent {
  public:
    enum class EventType {
        AttackAttempt,  // Before attack roll
        AttackHit,      // After successful hit
        AttackMiss,     // After missed attack
        AttackGlancing, // After glancing hit
        AttackCritical, // After critical hit
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

using CombatEventHandler = std::function<void(const CombatEvent &)>;

// ============================================================================
// Combat System (Modern ACC/EVA Implementation)
// ============================================================================

/**
 * @brief Modern combat system with ACC/EVA bounded accuracy
 *
 * Implements the system from COMBAT_REVERSE_ENGINEERING.md:
 * - Opposed d100 rolls (ACC + d100 vs EVA + d100)
 * - Bounded accuracy with 5-95% hit range
 * - Multi-layer damage mitigation (Soak → DR% → RES%)
 * - Glancing blows and margin-based criticals
 */
class CombatSystem {
  public:
    /**
     * @brief Calculate complete combat stats for an actor
     */
    static CombatStats calculate_combat_stats(const Actor &actor);

    /**
     * @brief Get class-specific ACC rate per level
     */
    static double get_class_acc_rate(CharacterClass character_class);

    /**
     * @brief Get race-specific combat bonuses
     */
    static CombatStats get_race_combat_bonus(CharacterRace race);

    /**
     * @brief Convert string class name to enum
     */
    static CharacterClass string_to_class(const std::string &class_name);

    /**
     * @brief Convert string race name to enum
     */
    static CharacterRace string_to_race(std::string_view race_name);

    /**
     * @brief Perform a combat attack using the modern system
     * @param attacker The attacking actor
     * @param target The target being attacked
     * @return CombatResult with detailed information
     */
    static CombatResult perform_attack(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target);

    /**
     * @brief Calculate hit result using opposed d100 rolls
     * @param attacker_stats Attacker's combat stats
     * @param defender_stats Defender's combat stats
     * @return Detailed hit calculation result
     */
    static HitCalcResult calculate_hit(const CombatStats &attacker_stats, const CombatStats &defender_stats);

    /**
     * @brief Calculate damage with full mitigation pipeline
     * @param attacker The attacker
     * @param attacker_stats Attacker's combat stats
     * @param defender The defender
     * @param defender_stats Defender's combat stats
     * @param hit_result The type of hit (miss/glancing/hit/crit)
     * @return Final damage after all mitigation
     */
    static double calculate_damage(const Actor &attacker, const CombatStats &attacker_stats, const Actor &defender,
                                   const CombatStats &defender_stats, HitResult hit_result);

    /**
     * @brief Apply damage mitigation (Soak → DR% → RES%)
     * @param raw_damage Initial damage before mitigation
     * @param attacker_stats For penetration values
     * @param defender_stats For soak/DR/resistance values
     * @param damage_type Type of damage for resistance lookup
     * @return Mitigated damage (minimum 1)
     */
    static double apply_mitigation(double raw_damage, const CombatStats &attacker_stats,
                                   const CombatStats &defender_stats, int damage_type);

    /**
     * @brief Calculate experience gain from defeating target
     */
    static long calculate_experience_gain(const Actor &victor, const Actor &defeated);

    /**
     * @brief Register combat event handler
     */
    static void register_event_handler(CombatEvent::EventType event_type, CombatEventHandler handler);

    /**
     * @brief Fire combat event to all registered handlers
     */
    static void fire_event(const CombatEvent &event);

  private:
    static std::vector<std::pair<CombatEvent::EventType, CombatEventHandler>> event_handlers_;

    /**
     * @brief Roll a d100 (1-100)
     */
    static int roll_d100();

    /**
     * @brief Roll damage dice
     */
    static double roll_damage(int num_dice, int dice_size, int bonus);
};

// ============================================================================
// Combat Manager
// ============================================================================

/**
 * @brief Manages ongoing combat encounters with time-based rounds
 *
 * Simplified architecture:
 * - Tracks fighting actors in a set (no duplicate tracking)
 * - Each actor maintains their own fighting_list_ of enemies
 * - Global round timer instead of per-pair timing
 * - Position::Fighting indicates combat state
 */
class CombatManager {
  public:
    /** Start combat between two actors (clears existing enemies for fresh 1v1) */
    static void start_combat(std::shared_ptr<Actor> actor1, std::shared_ptr<Actor> actor2);

    /** Add combat relationship without clearing existing enemies (for AOE, etc.) */
    static void add_combat_pair(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> defender);

    /** End combat for an actor (removes from all enemy lists) */
    static void end_combat(std::shared_ptr<Actor> actor);

    /** Process combat rounds for all fighting actors (returns true if any processed) */
    static bool process_combat_rounds();

    /** Check if actor is in combat */
    static bool is_in_combat(const Actor &actor);

    /** Get actor's primary opponent */
    static std::shared_ptr<Actor> get_opponent(const Actor &actor);

    /** Clear all combat state */
    static void clear_all_combat();

    /** Get all actors currently fighting */
    static std::vector<std::shared_ptr<Actor>> get_all_fighting_actors();

    /** Check if combat round is ready to process */
    static bool is_round_ready();

    /** Check if specific actor is ready to attack based on their attack speed */
    static bool is_actor_attack_ready(const std::shared_ptr<Actor> &actor);

    /** Get calculated attack speed for an actor (considering weapon + effects) */
    static int get_actor_attack_speed(const std::shared_ptr<Actor> &actor);

  private:
    /** Set of all actors currently in combat */
    static std::unordered_set<std::shared_ptr<Actor>> fighting_actors_;

    /** Per-actor last attack timestamps for variable attack speeds */
    static std::unordered_map<std::shared_ptr<Actor>, std::chrono::steady_clock::time_point> last_attack_times_;

    /** Global combat round timer (kept for backwards compatibility / effect timing) */
    static std::chrono::steady_clock::time_point last_round_time_;

    /** Remove dead/invalid actors from tracking */
    static void cleanup_invalid_actors();

    /** Execute a single actor's attack against their target */
    static void execute_actor_attack(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target);

    /** Record that an actor just attacked */
    static void record_attack_time(const std::shared_ptr<Actor> &actor);
};

// ============================================================================
// Class Abilities
// ============================================================================

/**
 * @brief Class-specific special abilities
 */
class ClassAbilities {
  public:
    static bool warrior_extra_attack_available(const Actor &actor);
    static bool rogue_sneak_attack_available(const Actor &attacker, const Actor &target);
    static double rogue_sneak_attack_damage(int level);
    static bool cleric_can_turn_undead(const Actor &actor);
    static double sorcerer_spell_damage_bonus(int level, int intelligence);
};

} // namespace FieryMUD

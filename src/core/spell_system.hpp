#pragma once

#include "core/entity.hpp"
#include "core/result.hpp"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <expected>

// Forward declarations
class Actor;
class CommandContext;

namespace fierymud {
    struct ClassSpellConfig;
    enum class SpellProgression;
}
using fierymud::ClassSpellConfig;
using fierymud::SpellProgression;

/** Spell types available in the game */
enum class SpellType {
    Offensive,    // Damage-dealing spells
    Defensive,    // Protective spells
    Healing,      // Restoration spells
    Utility,      // Non-combat utility spells
    Enchantment,  // Status effects
    Divination,   // Information gathering
    Conjuration   // Summoning and creation
};

/** Individual spell definition */
struct Spell {
    std::string name;
    std::string description;
    SpellType type = SpellType::Utility;
    int circle = 1;           // Spell circle (1-9)
    int cast_time_seconds = 3; // Time to cast
    int range_meters = 0;     // 0 = touch/self
    int duration_seconds = 0; // 0 = instant
    
    /** Check if spell can be cast by actor */
    bool can_cast(const Actor& caster) const;
    
    /** Execute spell effect (modifies caster for healing, buffs, etc.) */
    Result<void> cast(Actor& caster, const CommandContext& ctx) const;
    
    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<Spell> from_json(const nlohmann::json& json);
};

/** Spell slot tracking for a specific circle */
struct SpellSlotCircle {
    int current_slots = 0;
    int max_slots = 0;

    /** Check if slots are available */
    bool has_slots() const { return current_slots > 0; }

    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<SpellSlotCircle> from_json(const nlohmann::json& json);
};

/** Entry in the spell slot restoration queue (FIFO) */
struct SpellSlotRestoring {
    int circle;           // Circle of the slot being restored
    int ticks_remaining;  // Countdown to restoration (seconds)

    nlohmann::json to_json() const;
    static Result<SpellSlotRestoring> from_json(const nlohmann::json& json);
};

/** Base recovery times per circle (in seconds/ticks) */
constexpr std::array<int, 10> CIRCLE_RECOVERY_TICKS = {
    0,    // Circle 0 (unused)
    30,   // Circle 1
    35,   // Circle 2
    50,   // Circle 3
    65,   // Circle 4
    80,   // Circle 5
    95,   // Circle 6
    130,  // Circle 7
    145,  // Circle 8
    165   // Circle 9
};

/** Complete spell slot system for an actor */
class SpellSlots {
public:
    SpellSlots() = default;

    /** Initialize spell slots for a character class and level */
    void initialize_for_class(std::string_view character_class, int level);

    /** Check if actor has slots for a specific circle */
    bool has_slots(int circle) const;

    /** Check if actor has any spell slots at all (any circle) */
    bool has_any_slots() const;

    /**
     * Consume a spell slot for casting.
     * Tries exact circle first, then higher circles (upcast).
     * Adds consumed slot to restoration queue.
     * @param spell_circle The circle of the spell being cast
     * @return true if a slot was consumed, false if none available
     */
    bool consume_slot(int spell_circle);

    /**
     * Process restoration tick (called every second).
     * Subtracts focus_rate from front of queue, restores slots when ready.
     * @param focus_rate The caster's focus-based restoration rate
     * @return Circle of restored slot, or 0 if none restored
     */
    int restore_tick(int focus_rate);

    /** Get current/max slots for a circle */
    std::pair<int, int> get_slot_info(int circle) const;

    /** Get count of slots currently restoring for a circle */
    int get_restoring_count(int circle) const;

    /** Get total number of slots in restoration queue */
    int get_total_restoring() const;

    /** Get the restoration queue (for display purposes) */
    const std::vector<SpellSlotRestoring>& restoration_queue() const { return restoration_queue_; }

    /** Get all available spell circles */
    std::vector<int> get_available_circles() const;

    /** Get base recovery ticks for a circle */
    static int get_base_ticks(int circle);

    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<SpellSlots> from_json(const nlohmann::json& json);

private:
    /** Initialize slots from class configuration */
    void initialize_slots_from_config(const ClassSpellConfig& config, int level);

    /** Calculate slot count based on progression type and level */
    static int calculate_slots(SpellProgression progression, int level,
                               int required_level, int max_slots);

    std::unordered_map<int, SpellSlotCircle> slots_;       // circle -> slot info
    std::vector<SpellSlotRestoring> restoration_queue_;    // FIFO restoration queue
};

/** Global spell registry */
class SpellRegistry {
public:
    /** Get singleton instance */
    static SpellRegistry& instance();

    /** Register a spell */
    void register_spell(const Spell& spell);

    /** Find spell by name (supports partial matching) */
    const Spell* find_spell(std::string_view name) const;

    /** Get all spells of a specific circle */
    std::vector<const Spell*> get_spells_by_circle(int circle) const;

    /** Get all spells of a specific type */
    std::vector<const Spell*> get_spells_by_type(SpellType type) const;

    /** Initialize default spells */
    void initialize_default_spells();

private:
    SpellRegistry() = default;
    std::unordered_map<std::string, Spell> spells_;
};

/**
 * Error information for spell operations.
 */
struct SpellError {
    std::string message;
};

/**
 * SpellSystem provides the scripting API for spell casting.
 * Wraps SpellRegistry with additional casting logic.
 */
class SpellSystem {
public:
    static SpellSystem& instance();

    /**
     * Check if a spell exists.
     */
    bool spell_exists(std::string_view spell_name) const {
        return SpellRegistry::instance().find_spell(spell_name) != nullptr;
    }

    /**
     * Cast a spell from a script context.
     * Simplified version that doesn't require a full CommandContext.
     */
    std::expected<void, SpellError> cast_spell(Actor& caster, std::string_view spell_name,
                                                Actor* target, int level);

private:
    SpellSystem() = default;
};
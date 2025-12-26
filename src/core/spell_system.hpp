#pragma once

#include "core/entity.hpp"
#include "core/result.hpp"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <vector>

// Forward declarations
class Actor;
class CommandContext;

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
    std::chrono::seconds refresh_time{1800}; // 30 minutes default
    std::chrono::steady_clock::time_point last_refresh;
    
    /** Check if slots are available */
    bool has_slots() const { return current_slots > 0; }
    
    /** Use a spell slot */
    bool use_slot();
    
    /** Refresh slots based on time elapsed */
    void update_refresh();
    
    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<SpellSlotCircle> from_json(const nlohmann::json& json);
};

/** Complete spell slot system for an actor */
class SpellSlots {
public:
    SpellSlots() = default;
    
    /** Initialize spell slots for a character class and level */
    void initialize_for_class(std::string_view character_class, int level);
    
    /** Check if actor has slots for a specific circle */
    bool has_slots(int circle) const;
    
    /** Use a spell slot of specific circle */
    bool use_slot(int circle);
    
    /** Get current/max slots for a circle */
    std::pair<int, int> get_slot_info(int circle) const;
    
    /** Update all slot refreshes */
    void update_refresh();
    
    /** Get all available spell circles */
    std::vector<int> get_available_circles() const;
    
    /** JSON serialization */
    nlohmann::json to_json() const;
    static Result<SpellSlots> from_json(const nlohmann::json& json);
    
private:
    std::unordered_map<int, SpellSlotCircle> slots_; // circle -> slot info
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
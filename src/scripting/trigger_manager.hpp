#pragma once

#include "script_context.hpp"
#include "script_engine.hpp"
#include "triggers/trigger_data.hpp"
#include "triggers/trigger_types.hpp"
#include "core/ids.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace FieryMUD {

/// Result of trigger execution
enum class TriggerResult {
    Continue,       ///< Continue normal processing
    Halt,           ///< Stop command processing (trigger handled it)
    Error           ///< Script execution failed
};

/**
 * TriggerManager - Central trigger dispatch and management system
 *
 * Responsibilities:
 * - Load triggers from database on zone load
 * - Cache triggers by entity ID for fast lookup
 * - Dispatch triggers based on event type (flag)
 * - Execute scripts with proper context
 * - Track script errors and diagnostics
 *
 * Thread Safety: NOT thread-safe. Must be called from game loop strand.
 */
class TriggerManager {
public:
    /// Get the singleton instance
    static TriggerManager& instance();

    /// Initialize the trigger manager (requires ScriptEngine to be initialized)
    [[nodiscard]] bool initialize();

    /// Shutdown and clean up
    void shutdown();

    /// Check if manager is initialized
    [[nodiscard]] bool is_initialized() const { return initialized_; }

    // ========================================================================
    // Trigger Loading
    // ========================================================================

    /// Load all triggers for a zone from database
    /// @param zone_id The zone to load triggers for
    /// @return Number of triggers loaded, or error
    std::expected<std::size_t, std::string> load_zone_triggers(std::uint32_t zone_id);

    /// Reload all triggers for a zone (clears existing and reloads)
    std::expected<std::size_t, std::string> reload_zone_triggers(std::uint32_t zone_id);

    /// Clear all cached triggers for a zone
    void clear_zone_triggers(std::uint32_t zone_id);

    /// Clear all cached triggers
    void clear_all_triggers();

    /// Manually register a trigger (for testing or dynamic triggers)
    void register_trigger(TriggerDataPtr trigger);

    // ========================================================================
    // Trigger Lookup
    // ========================================================================

    /// Get all triggers attached to a mob
    [[nodiscard]] TriggerSet get_mob_triggers(const EntityId& mob_id) const;

    /// Get all triggers attached to an object
    [[nodiscard]] TriggerSet get_object_triggers(const EntityId& obj_id) const;

    /// Get all world triggers for a zone
    [[nodiscard]] TriggerSet get_world_triggers(std::uint32_t zone_id) const;

    /// Find triggers by flag for a specific entity
    [[nodiscard]] std::vector<TriggerDataPtr> find_triggers(
        const EntityId& entity_id,
        ScriptType type,
        TriggerFlag flag) const;

    // ========================================================================
    // Trigger Dispatch
    // ========================================================================

    /// Execute a COMMAND trigger
    /// @param owner The mob that owns the trigger
    /// @param actor The actor who typed the command
    /// @param command The command word
    /// @param argument The command arguments
    /// @return Continue if command should proceed normally, Halt to stop
    TriggerResult dispatch_command(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> actor,
        std::string_view command,
        std::string_view argument);

    /// Execute a SPEECH trigger
    /// @param owner The mob that owns the trigger
    /// @param actor The actor who spoke
    /// @param speech What was said
    /// @return Continue or Halt
    TriggerResult dispatch_speech(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> actor,
        std::string_view speech);

    /// Execute a GREET trigger (actor enters room with mob)
    /// @param owner The mob that owns the trigger
    /// @param actor The actor who entered
    /// @param from_direction Direction they came from
    /// @return Continue or Halt
    TriggerResult dispatch_greet(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> actor,
        Direction from_direction);

    /// Execute an ENTRY trigger (mob enters a room)
    /// @param owner The mob that moved
    /// @param room The room entered
    /// @param from_direction Direction traveled from
    /// @return Continue or Halt
    TriggerResult dispatch_entry(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Room> room,
        Direction from_direction);

    /// Execute a LEAVE trigger (actor leaves room)
    /// @param owner The mob/room that owns the trigger
    /// @param actor The actor leaving
    /// @param to_direction Direction being traveled
    /// @return Continue or Halt
    TriggerResult dispatch_leave(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> actor,
        Direction to_direction);

    /// Execute a RECEIVE trigger (mob receives an object)
    /// @param owner The mob that received the object
    /// @param actor The actor who gave the object
    /// @param object The object received
    /// @return Continue or Halt
    TriggerResult dispatch_receive(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> actor,
        std::shared_ptr<Object> object);

    /// Execute a BRIBE trigger (actor gives gold to mob)
    /// @param owner The mob that received gold
    /// @param actor The actor who gave gold
    /// @param amount Amount of gold given
    /// @return Continue or Halt
    TriggerResult dispatch_bribe(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> actor,
        int amount);

    /// Execute a DEATH trigger (mob is killed)
    /// @param owner The mob that died
    /// @param killer The actor that killed them (may be null)
    /// @return Continue (for death processing) or Halt
    TriggerResult dispatch_death(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> killer);

    /// Execute a FIGHT trigger (combat round)
    /// @param owner The mob in combat
    /// @param opponent The opponent
    /// @return Continue or Halt
    TriggerResult dispatch_fight(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> opponent);

    /// Execute a HIT_PERCENT trigger (HP dropped below threshold)
    /// @param owner The mob that was hit
    /// @param attacker The actor that hit them
    /// @param hp_percent Current HP as percentage of max (0-100)
    /// @return Continue or Halt
    TriggerResult dispatch_hit_percent(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> attacker,
        int hp_percent);

    /// Execute a RANDOM trigger (periodic random chance)
    /// @param owner The mob/object with random trigger
    /// @return Continue
    TriggerResult dispatch_random(
        std::shared_ptr<Actor> owner);

    /// Execute a LOAD trigger (mob/object loaded into world)
    /// @param owner The entity that was loaded
    /// @param room The room it was loaded into
    /// @return Continue
    TriggerResult dispatch_load(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Room> room);

    /// Execute a CAST trigger (spell cast on mob)
    /// @param owner The mob that was the target of the spell
    /// @param caster The actor who cast the spell
    /// @param spell_name Name of the spell cast
    /// @return Continue or Halt (to block the spell)
    TriggerResult dispatch_cast(
        std::shared_ptr<Actor> owner,
        std::shared_ptr<Actor> caster,
        std::string_view spell_name);

    /// Execute a TIME trigger (fires at specific game time)
    /// @param owner The mob/object with the time trigger
    /// @param hour Current game hour (0-23)
    /// @return Continue
    TriggerResult dispatch_time(
        std::shared_ptr<Actor> owner,
        int hour);

    // ========================================================================
    // OBJECT Trigger Dispatch
    // ========================================================================

    /// Execute an ATTACK trigger (weapon being used to attack)
    /// @param weapon The weapon object with the trigger
    /// @param attacker The actor wielding the weapon
    /// @param target The target being attacked
    /// @param damage The amount of damage dealt (0 for miss)
    /// @param room The room where combat is happening
    /// @return Continue or Halt
    TriggerResult dispatch_attack(
        std::shared_ptr<Object> weapon,
        std::shared_ptr<Actor> attacker,
        std::shared_ptr<Actor> target,
        int damage,
        std::shared_ptr<Room> room);

    /// Execute a DEFEND trigger (armor/shield being used defensively)
    /// @param armor The armor/shield object with the trigger
    /// @param defender The actor wearing the armor
    /// @param attacker The actor attacking
    /// @param damage The incoming damage amount
    /// @param room The room where combat is happening
    /// @return Continue or Halt (Halt can reduce/negate damage)
    TriggerResult dispatch_defend(
        std::shared_ptr<Object> armor,
        std::shared_ptr<Actor> defender,
        std::shared_ptr<Actor> attacker,
        int damage,
        std::shared_ptr<Room> room);

    // ========================================================================
    // Diagnostics
    // ========================================================================

    /// Get total number of cached triggers
    [[nodiscard]] std::size_t trigger_count() const;

    /// Get number of triggers for a specific type
    [[nodiscard]] std::size_t trigger_count(ScriptType type) const;

    /// Get last script error message
    [[nodiscard]] std::string_view last_error() const { return last_error_; }

    /// Execution statistics
    struct ExecutionStats {
        std::size_t total_executions = 0;
        std::size_t successful_executions = 0;
        std::size_t halted_executions = 0;
        std::size_t failed_executions = 0;
        std::size_t yielded_executions = 0;  // Scripts that called wait()
    };

    /// Get execution statistics
    [[nodiscard]] const ExecutionStats& stats() const { return stats_; }

    /// Reset execution statistics
    void reset_stats() { stats_ = ExecutionStats{}; }

    // Delete copy/move operations (singleton)
    TriggerManager(const TriggerManager&) = delete;
    TriggerManager& operator=(const TriggerManager&) = delete;
    TriggerManager(TriggerManager&&) = delete;
    TriggerManager& operator=(TriggerManager&&) = delete;

private:
    TriggerManager() = default;
    ~TriggerManager() = default;

    /// Execute a trigger with the given context
    TriggerResult execute_trigger(const TriggerDataPtr& trigger, ScriptContext& context);

    /// Set up Lua environment variables from context
    void setup_lua_context(sol::state_view lua, const ScriptContext& context);

    /// Cache key for entity triggers
    struct CacheKey {
        ScriptType type;
        EntityId entity_id;

        bool operator==(const CacheKey& other) const {
            return type == other.type && entity_id == other.entity_id;
        }
    };

    struct CacheKeyHash {
        std::size_t operator()(const CacheKey& key) const {
            std::size_t h1 = std::hash<int>{}(static_cast<int>(key.type));
            std::size_t h2 = std::hash<std::uint64_t>{}(
                (static_cast<std::uint64_t>(key.entity_id.zone_id()) << 32) |
                key.entity_id.local_id()
            );
            return h1 ^ (h2 << 1);
        }
    };

    /// Trigger cache: (type, entity_id) -> TriggerSet
    std::unordered_map<CacheKey, TriggerSet, CacheKeyHash> trigger_cache_;

    /// Track which zones have been loaded
    std::unordered_set<std::uint32_t> loaded_zones_;

    std::string last_error_;
    bool initialized_ = false;

    /// Execution statistics
    ExecutionStats stats_;
};

} // namespace FieryMUD

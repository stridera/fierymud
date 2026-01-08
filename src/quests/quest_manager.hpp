/***************************************************************************
 *   File: src/quests/quest_manager.hpp                    Part of FieryMUD *
 *  Usage: Quest system management and tracking                             *
 ***************************************************************************/

#pragma once

#include "database/quest_queries.hpp"
#include "core/ids.hpp"
#include "core/result.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations (in global namespace)
class Actor;
class Room;
class Object;

namespace FieryMUD {

/// Quest status for a character
enum class QuestStatus {
    None,       ///< Quest not started or doesn't exist
    Available,  ///< Quest is available to accept
    InProgress, ///< Quest is active
    Completed,  ///< Quest has been completed
    Failed,     ///< Quest was failed
    Abandoned   ///< Quest was abandoned
};

/**
 * QuestManager - Central quest system management
 *
 * Responsibilities:
 * - Load and cache quest definitions from database
 * - Track character quest progress
 * - Handle quest triggers (level-up, item pickup, room entry, etc.)
 * - Provide quest-related commands interface
 *
 * Thread Safety: NOT thread-safe. Must be called from game loop strand.
 */
class QuestManager {
public:
    /// Get the singleton instance
    static QuestManager& instance();

    /// Initialize the quest manager (requires database connection)
    [[nodiscard]] bool initialize();

    /// Shutdown and clean up
    void shutdown();

    /// Check if manager is initialized
    [[nodiscard]] bool is_initialized() const { return initialized_; }

    // ========================================================================
    // Quest Loading
    // ========================================================================

    /// Load all quests for a zone from database
    /// @param zone_id The zone to load quests for
    /// @return Number of quests loaded, or error
    std::expected<std::size_t, std::string> load_zone_quests(int zone_id);

    /// Reload all quests for a zone (clears existing and reloads)
    std::expected<std::size_t, std::string> reload_zone_quests(int zone_id);

    /// Clear all cached quests for a zone
    void clear_zone_quests(int zone_id);

    /// Clear all cached quests
    void clear_all_quests();

    // ========================================================================
    // Quest Lookup
    // ========================================================================

    /// Get a quest by composite ID
    [[nodiscard]] const QuestQueries::QuestData* get_quest(int zone_id, int quest_id) const;

    /// Get a quest by EntityId
    [[nodiscard]] const QuestQueries::QuestData* get_quest(const EntityId& id) const;

    /// Get all quests in a zone
    [[nodiscard]] std::vector<const QuestQueries::QuestData*> get_zone_quests(int zone_id) const;

    /// Get quests available to a character
    [[nodiscard]] std::vector<const QuestQueries::QuestData*> get_available_quests(
        std::string_view character_id,
        int level) const;

    // ========================================================================
    // Character Quest Progress
    // ========================================================================

    /// Start a quest for a character
    /// @param character_id Character's database ID
    /// @param zone_id Quest zone ID
    /// @param quest_id Quest local ID
    /// @return Success or error message
    Result<void> start_quest(
        std::string_view character_id,
        int zone_id, int quest_id);

    /// Update objective progress
    /// @param character_id Character's database ID
    /// @param zone_id Quest zone ID
    /// @param quest_id Quest local ID
    /// @param phase_id Phase ID
    /// @param objective_id Objective ID
    /// @param new_count New progress count
    /// @return Success or error message
    Result<void> update_objective(
        std::string_view character_id,
        int zone_id, int quest_id,
        int phase_id, int objective_id,
        int new_count);

    /// Mark an objective as completed
    Result<void> complete_objective(
        std::string_view character_id,
        int zone_id, int quest_id,
        int phase_id, int objective_id);

    /// Advance to next phase
    Result<void> advance_phase(
        std::string_view character_id,
        int zone_id, int quest_id,
        int next_phase_id);

    /// Complete a quest
    Result<void> complete_quest(
        std::string_view character_id,
        int zone_id, int quest_id);

    /// Abandon a quest
    Result<void> abandon_quest(
        std::string_view character_id,
        int zone_id, int quest_id);

    /// Get character's active quests
    [[nodiscard]] Result<std::vector<QuestQueries::CharacterQuestProgress>> get_active_quests(
        std::string_view character_id) const;

    /// Get character's completed quests
    [[nodiscard]] Result<std::vector<QuestQueries::CharacterQuestProgress>> get_completed_quests(
        std::string_view character_id) const;

    /// Check if character has quest in progress
    [[nodiscard]] bool has_quest_in_progress(
        std::string_view character_id,
        int zone_id, int quest_id) const;

    /// Check if character has completed quest
    [[nodiscard]] bool has_completed_quest(
        std::string_view character_id,
        int zone_id, int quest_id) const;

    // ========================================================================
    // Actor-Based Convenience Methods (for Lua bindings)
    // ========================================================================

    /// Start a quest for an actor
    Result<void> start_quest(Actor& actor, int zone_id, int quest_id);

    /// Complete a quest for an actor
    Result<void> complete_quest(Actor& actor, int zone_id, int quest_id);

    /// Abandon a quest for an actor
    Result<void> abandon_quest(Actor& actor, int zone_id, int quest_id);

    /// Get quest status for an actor
    [[nodiscard]] QuestStatus get_quest_status(const Actor& actor, int zone_id, int quest_id) const;

    /// Check if quest is available to an actor
    [[nodiscard]] bool is_quest_available(const Actor& actor, int zone_id, int quest_id) const;

    /// Advance an objective by count for an actor
    Result<void> advance_objective(Actor& actor, int zone_id, int quest_id,
                                    int objective_id, int count = 1);

    /// Set a quest variable for an actor
    Result<void> set_quest_variable(Actor& actor, int zone_id, int quest_id,
                                     const std::string& name, const nlohmann::json& value);

    /// Get a quest variable for an actor
    [[nodiscard]] std::expected<nlohmann::json, Error> get_quest_variable(
        const Actor& actor, int zone_id, int quest_id,
        const std::string& name) const;

    // ========================================================================
    // Trigger-Based Quest Activation
    // ========================================================================

    /// Check and start quests triggered by level up
    /// @param actor The character that leveled up
    /// @param new_level The new level
    /// @return List of quests started
    std::vector<const QuestQueries::QuestData*> check_level_triggers(
        std::shared_ptr<Actor> actor,
        int new_level);

    /// Check and start quests triggered by picking up an item
    /// @param actor The character that picked up the item
    /// @param item The item picked up
    /// @return List of quests started
    std::vector<const QuestQueries::QuestData*> check_item_triggers(
        std::shared_ptr<Actor> actor,
        std::shared_ptr<Object> item);

    /// Check and start quests triggered by entering a room
    /// @param actor The character that entered the room
    /// @param room The room entered
    /// @return List of quests started
    std::vector<const QuestQueries::QuestData*> check_room_triggers(
        std::shared_ptr<Actor> actor,
        std::shared_ptr<Room> room);

    /// Check and start quests triggered by using an ability
    /// @param actor The character that used the ability
    /// @param ability_id The ability used
    /// @return List of quests started
    std::vector<const QuestQueries::QuestData*> check_skill_triggers(
        std::shared_ptr<Actor> actor,
        int ability_id);

    /// Check and start event-triggered quests
    /// @param event_id The event ID
    /// @return List of quests that became available
    std::vector<const QuestQueries::QuestData*> check_event_triggers(int event_id);

    /// Get all auto-triggered quests for new characters
    /// @return List of tutorial/auto-start quests
    std::vector<const QuestQueries::QuestData*> get_auto_quests() const;

    // ========================================================================
    // Objective Progress Tracking
    // ========================================================================

    /// Called when a mob is killed - updates KILL_MOB objectives
    void on_mob_killed(
        std::shared_ptr<Actor> killer,
        const EntityId& killed_mob_id);

    /// Called when an item is collected - updates COLLECT_ITEM objectives
    void on_item_collected(
        std::shared_ptr<Actor> collector,
        const EntityId& item_id);

    /// Called when an item is delivered to an NPC - updates DELIVER_ITEM objectives
    void on_item_delivered(
        std::shared_ptr<Actor> deliverer,
        const EntityId& recipient_mob_id,
        const EntityId& item_id);

    /// Called when an NPC is talked to - updates TALK_TO_NPC objectives
    void on_npc_talked(
        std::shared_ptr<Actor> talker,
        const EntityId& npc_id);

    /// Called when a room is visited - updates VISIT_ROOM objectives
    void on_room_visited(
        std::shared_ptr<Actor> visitor,
        const EntityId& room_id);

    /// Called when a skill is used - updates USE_SKILL objectives
    void on_skill_used(
        std::shared_ptr<Actor> user,
        int ability_id);

    // ========================================================================
    // Diagnostics
    // ========================================================================

    /// Get total number of cached quests
    [[nodiscard]] std::size_t quest_count() const;

    /// Get number of quests in a zone
    [[nodiscard]] std::size_t quest_count(int zone_id) const;

    /// Get last error message
    [[nodiscard]] std::string_view last_error() const { return last_error_; }

    /// Quest system statistics
    struct QuestStats {
        std::size_t quests_started = 0;
        std::size_t quests_completed = 0;
        std::size_t quests_abandoned = 0;
        std::size_t objectives_completed = 0;
        std::size_t trigger_activations = 0;
    };

    /// Get quest statistics
    [[nodiscard]] const QuestStats& stats() const { return stats_; }

    /// Reset statistics
    void reset_stats() { stats_ = QuestStats{}; }

    // Delete copy/move operations (singleton)
    QuestManager(const QuestManager&) = delete;
    QuestManager& operator=(const QuestManager&) = delete;
    QuestManager(QuestManager&&) = delete;
    QuestManager& operator=(QuestManager&&) = delete;

private:
    QuestManager() = default;
    ~QuestManager() = default;

    /// Cache key for quests
    struct QuestKey {
        int zone_id;
        int quest_id;

        bool operator==(const QuestKey& other) const {
            return zone_id == other.zone_id && quest_id == other.quest_id;
        }
    };

    struct QuestKeyHash {
        std::size_t operator()(const QuestKey& key) const {
            return std::hash<std::uint64_t>{}(
                (static_cast<std::uint64_t>(key.zone_id) << 32) |
                static_cast<std::uint32_t>(key.quest_id)
            );
        }
    };

    /// Quest cache: (zone_id, quest_id) -> QuestData
    std::unordered_map<QuestKey, QuestQueries::QuestData, QuestKeyHash> quest_cache_;

    /// Quests indexed by zone for fast zone lookup
    std::unordered_map<int, std::vector<QuestKey>> zone_quests_;

    /// Quests indexed by trigger type for fast trigger lookup
    std::unordered_map<int, std::vector<QuestKey>> level_triggered_quests_;  // level -> quests
    std::unordered_map<QuestKey, std::vector<QuestKey>, QuestKeyHash> item_triggered_quests_;  // item_id -> quests
    std::unordered_map<QuestKey, std::vector<QuestKey>, QuestKeyHash> room_triggered_quests_;  // room_id -> quests
    std::unordered_map<int, std::vector<QuestKey>> skill_triggered_quests_;  // ability_id -> quests
    std::unordered_map<int, std::vector<QuestKey>> event_triggered_quests_;  // event_id -> quests
    std::vector<QuestKey> auto_triggered_quests_;  // auto-start quests

    /// Track which zones have been loaded
    std::unordered_set<int> loaded_zones_;

    std::string last_error_;
    bool initialized_ = false;

    /// Quest statistics
    QuestStats stats_;

    /// Helper: Check if character can accept quest
    bool can_accept_quest(
        std::string_view character_id,
        int zone_id, int quest_id,
        int level) const;

    /// Helper: Add quest to trigger index
    void index_quest_triggers(const QuestQueries::QuestData& quest);

    /// Helper: Remove quest from trigger index
    void unindex_quest_triggers(const QuestQueries::QuestData& quest);
};

} // namespace FieryMUD

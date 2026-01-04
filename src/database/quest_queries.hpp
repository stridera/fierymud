#pragma once

#include "core/result.hpp"
#include "core/ids.hpp"
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <chrono>

/**
 * Quest query layer for PostgreSQL database.
 *
 * Provides SQL queries for loading quest definitions and managing
 * character quest progress. Uses composite primary keys (zone_id, id)
 * for all quest entities.
 *
 * All queries use prepared statements for security and performance.
 */
namespace QuestQueries {

// ============================================================================
// Quest Objective Types (matches database enum)
// ============================================================================

enum class QuestObjectiveType {
    KILL_MOB,
    COLLECT_ITEM,
    DELIVER_ITEM,
    VISIT_ROOM,
    TALK_TO_NPC,
    USE_SKILL,
    CUSTOM_LUA
};

enum class QuestRewardType {
    EXPERIENCE,
    GOLD,
    ITEM,
    ABILITY
};

enum class QuestStatus {
    AVAILABLE,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    ABANDONED
};

enum class QuestTriggerType {
    MOB,        // Talk to NPC (traditional quest giver)
    LEVEL,      // Auto-granted when reaching certain level
    ITEM,       // Triggered when picking up/examining specific item
    ROOM,       // Triggered when entering specific room
    SKILL,      // Triggered when using specific skill
    EVENT,      // Triggered by game event (seasonal, etc.)
    AUTO,       // Auto-granted to all characters (tutorial)
    MANUAL      // Only granted via GM command or scripts
};

// ============================================================================
// Data Structures
// ============================================================================

/** Quest objective definition */
struct QuestObjective {
    int phase_id;
    int id;
    QuestObjectiveType type;
    std::string player_description;
    std::string internal_note;
    bool show_progress;
    int required_count;

    // Target entity (based on type)
    std::optional<EntityId> target_mob;
    std::optional<EntityId> target_object;
    std::optional<EntityId> target_room;
    std::optional<int> target_ability_id;
    std::optional<EntityId> deliver_to_mob;
    std::string lua_expression;
};

/** Quest phase containing objectives */
struct QuestPhase {
    int id;
    std::string name;
    std::string description;
    int order;
    std::vector<QuestObjective> objectives;
};

/** Quest reward definition */
struct QuestReward {
    int id;
    QuestRewardType type;
    std::optional<int> amount;
    std::optional<EntityId> object;
    std::optional<int> ability_id;
    std::optional<int> choice_group;
};

/** Quest prerequisite */
struct QuestPrerequisite {
    int id;
    EntityId prerequisite_quest;
};

/** Complete quest definition */
struct QuestData {
    EntityId id;
    std::string name;
    std::string description;
    int min_level;
    int max_level;
    bool repeatable;
    bool hidden;

    // Quest trigger configuration
    QuestTriggerType trigger_type;
    std::optional<int> trigger_level;             // For LEVEL trigger
    std::optional<EntityId> trigger_item;         // For ITEM trigger
    std::optional<EntityId> trigger_room;         // For ROOM trigger
    std::optional<int> trigger_ability_id;        // For SKILL trigger
    std::optional<int> trigger_event_id;          // For EVENT trigger

    // Quest giver/completer (still used for MOB trigger and turn-in)
    std::optional<EntityId> giver_mob;
    std::optional<EntityId> completer_mob;

    std::vector<QuestPhase> phases;
    std::vector<QuestReward> rewards;
    std::vector<QuestPrerequisite> prerequisites;
};

/** Character's progress on a specific objective */
struct CharacterObjectiveProgress {
    int phase_id;
    int objective_id;
    int current_count;
    bool completed;
    std::optional<std::chrono::system_clock::time_point> completed_at;
};

/** Character's overall quest progress */
struct CharacterQuestProgress {
    std::string character_id;
    EntityId quest_id;
    QuestStatus status;
    std::optional<int> current_phase_id;
    std::chrono::system_clock::time_point accepted_at;
    std::optional<std::chrono::system_clock::time_point> completed_at;
    int completion_count;

    std::vector<CharacterObjectiveProgress> objective_progress;
};

// ============================================================================
// Quest Definition Queries
// ============================================================================

/**
 * Load a quest definition by composite key.
 *
 * @param txn Database transaction
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @return Result containing QuestData or error
 */
Result<QuestData> load_quest(pqxx::work& txn, int zone_id, int quest_id);

/**
 * Load all quests in a zone.
 *
 * @param txn Database transaction
 * @param zone_id Zone ID to load quests from
 * @return Result containing vector of QuestData or error
 */
Result<std::vector<QuestData>> load_quests_in_zone(pqxx::work& txn, int zone_id);

/**
 * Load all available quests for a character (based on level and prerequisites).
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param level Character's current level
 * @return Result containing vector of available QuestData or error
 */
Result<std::vector<QuestData>> load_available_quests(
    pqxx::work& txn,
    std::string_view character_id,
    int level);

/**
 * Check if a character can accept a quest (level, prerequisites, not already active).
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @param level Character's current level
 * @return Result containing true if acceptable, false otherwise
 */
Result<bool> can_accept_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int level);

/**
 * Load quests triggered by reaching a specific level.
 *
 * @param txn Database transaction
 * @param level Level to check for
 * @return Result containing vector of QuestData or error
 */
Result<std::vector<QuestData>> load_level_triggered_quests(pqxx::work& txn, int level);

/**
 * Load quests triggered by picking up a specific item.
 *
 * @param txn Database transaction
 * @param item_zone_id Item's zone ID
 * @param item_id Item's local ID
 * @return Result containing vector of QuestData or error
 */
Result<std::vector<QuestData>> load_item_triggered_quests(
    pqxx::work& txn,
    int item_zone_id, int item_id);

/**
 * Load quests triggered by entering a specific room.
 *
 * @param txn Database transaction
 * @param room_zone_id Room's zone ID
 * @param room_id Room's local ID
 * @return Result containing vector of QuestData or error
 */
Result<std::vector<QuestData>> load_room_triggered_quests(
    pqxx::work& txn,
    int room_zone_id, int room_id);

/**
 * Load quests triggered by using a specific ability.
 *
 * @param txn Database transaction
 * @param ability_id Ability ID
 * @return Result containing vector of QuestData or error
 */
Result<std::vector<QuestData>> load_skill_triggered_quests(pqxx::work& txn, int ability_id);

/**
 * Load quests triggered by a specific event.
 *
 * @param txn Database transaction
 * @param event_id Event ID
 * @return Result containing vector of QuestData or error
 */
Result<std::vector<QuestData>> load_event_triggered_quests(pqxx::work& txn, int event_id);

/**
 * Load all auto-triggered quests (tutorial quests, etc.).
 *
 * @param txn Database transaction
 * @return Result containing vector of QuestData or error
 */
Result<std::vector<QuestData>> load_auto_triggered_quests(pqxx::work& txn);

// ============================================================================
// Character Quest Progress Queries
// ============================================================================

/**
 * Load a character's progress on a specific quest.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @return Result containing CharacterQuestProgress or NotFound error
 */
Result<CharacterQuestProgress> load_character_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id);

/**
 * Load all active quests for a character.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @return Result containing vector of CharacterQuestProgress or error
 */
Result<std::vector<CharacterQuestProgress>> load_character_active_quests(
    pqxx::work& txn,
    std::string_view character_id);

/**
 * Load all completed quests for a character.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @return Result containing vector of CharacterQuestProgress or error
 */
Result<std::vector<CharacterQuestProgress>> load_character_completed_quests(
    pqxx::work& txn,
    std::string_view character_id);

// ============================================================================
// Quest Progress Mutations
// ============================================================================

/**
 * Start a quest for a character.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @return Result<void> on success, error on failure
 */
Result<void> start_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id);

/**
 * Update objective progress for a character.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @param phase_id Phase ID
 * @param objective_id Objective ID
 * @param new_count New progress count
 * @return Result<void> on success, error on failure
 */
Result<void> update_objective_progress(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int phase_id, int objective_id,
    int new_count);

/**
 * Mark an objective as completed.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @param phase_id Phase ID
 * @param objective_id Objective ID
 * @return Result<void> on success, error on failure
 */
Result<void> complete_objective(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int phase_id, int objective_id);

/**
 * Advance to the next phase of a quest.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @param next_phase_id Next phase ID to advance to
 * @return Result<void> on success, error on failure
 */
Result<void> advance_phase(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    int next_phase_id);

/**
 * Complete a quest for a character.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @return Result<void> on success, error on failure
 */
Result<void> complete_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id);

/**
 * Abandon a quest for a character.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @return Result<void> on success, error on failure
 */
Result<void> abandon_quest(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id);

// ============================================================================
// Quest Variables
// ============================================================================

/**
 * Set a quest variable for a character's quest progress.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @param name Variable name
 * @param value Variable value (JSON)
 * @return Result<void> on success, error on failure
 */
Result<void> set_quest_variable(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    const std::string& name,
    const nlohmann::json& value);

/**
 * Get a quest variable for a character's quest progress.
 *
 * @param txn Database transaction
 * @param character_id Character's database ID
 * @param zone_id Quest zone ID
 * @param quest_id Quest local ID
 * @param name Variable name
 * @return Result containing the variable value (JSON) or NotFound error
 */
Result<nlohmann::json> get_quest_variable(
    pqxx::work& txn,
    std::string_view character_id,
    int zone_id, int quest_id,
    const std::string& name);

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Parse objective type from database string.
 */
QuestObjectiveType parse_objective_type(std::string_view type_str);

/**
 * Parse reward type from database string.
 */
QuestRewardType parse_reward_type(std::string_view type_str);

/**
 * Parse quest status from database string.
 */
QuestStatus parse_quest_status(std::string_view status_str);

/**
 * Convert objective type to string.
 */
std::string_view objective_type_to_string(QuestObjectiveType type);

/**
 * Convert reward type to string.
 */
std::string_view reward_type_to_string(QuestRewardType type);

/**
 * Convert quest status to string.
 */
std::string_view quest_status_to_string(QuestStatus status);

/**
 * Parse trigger type from database string.
 */
QuestTriggerType parse_trigger_type(std::string_view type_str);

/**
 * Convert trigger type to string.
 */
std::string_view trigger_type_to_string(QuestTriggerType type);

} // namespace QuestQueries

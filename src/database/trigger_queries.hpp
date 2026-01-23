#pragma once

#include "core/result.hpp"
#include "core/ids.hpp"
#include "scripting/triggers/trigger_data.hpp"
#include <pqxx/pqxx>
#include <vector>
#include <memory>

/**
 * Trigger loading query layer for PostgreSQL database.
 *
 * Loads script triggers from the Triggers table, converting
 * database records to TriggerData structures for the scripting system.
 */
namespace TriggerQueries {

/// Load all triggers for a specific zone (MOB, OBJECT, and WORLD types)
Result<std::vector<FieryMUD::TriggerDataPtr>> load_triggers_for_zone(
    pqxx::work& txn, int zone_id);

/// Load triggers attached to a specific mob prototype
Result<std::vector<FieryMUD::TriggerDataPtr>> load_mob_triggers(
    pqxx::work& txn, int zone_id, int mob_id);

/// Load triggers attached to a specific object prototype
Result<std::vector<FieryMUD::TriggerDataPtr>> load_object_triggers(
    pqxx::work& txn, int zone_id, int object_id);

/// Load world/room triggers for a zone
Result<std::vector<FieryMUD::TriggerDataPtr>> load_world_triggers(
    pqxx::work& txn, int zone_id);

/// Load a single trigger by zone and ID (composite key)
Result<FieryMUD::TriggerDataPtr> load_trigger_by_id(
    pqxx::work& txn, int zone_id, int trigger_id);

/// Reload all triggers (clears cache and reloads from DB)
Result<std::vector<FieryMUD::TriggerDataPtr>> load_all_triggers(
    pqxx::work& txn);

// ============================================================================
// Error Logging Functions
// ============================================================================

/// Log a script error to the database
/// Also updates the trigger's needsReview flag and syntaxError field
/// @param txn Database transaction
/// @param zone_id Trigger's zone ID
/// @param trigger_id Trigger's local ID
/// @param error_type Type of error: "compilation", "runtime", "timeout"
/// @param error_message The error message
/// @param script_line Optional line number where error occurred
/// @param context_info Optional JSON context info (entity, variables, etc.)
Result<void> log_script_error(
    pqxx::work& txn,
    int zone_id,
    int trigger_id,
    std::string_view error_type,
    std::string_view error_message,
    std::optional<int> script_line = std::nullopt,
    std::optional<std::string> context_info = std::nullopt);

/// Get all triggers that have needsReview=true
Result<std::vector<FieryMUD::TriggerDataPtr>> get_triggers_needing_review(
    pqxx::work& txn);

/// Get all triggers needing review for a specific zone
Result<std::vector<FieryMUD::TriggerDataPtr>> get_triggers_needing_review_for_zone(
    pqxx::work& txn, int zone_id);

/// Script error log entry
struct ScriptErrorEntry {
    int id;
    int zone_id;
    int trigger_id;
    std::string trigger_name;
    std::string error_type;
    std::string error_message;
    std::optional<int> script_line;
    std::string occurred_at;  // ISO timestamp string
};

/// Get recent error log entries for a specific trigger
Result<std::vector<ScriptErrorEntry>> get_error_log_for_trigger(
    pqxx::work& txn, int zone_id, int trigger_id, int limit = 10);

/// Get recent error log entries (most recent errors across all triggers)
Result<std::vector<ScriptErrorEntry>> get_recent_script_errors(
    pqxx::work& txn, int limit = 25);

/// Clear the needsReview flag on a trigger (after fixing)
Result<void> clear_trigger_error(
    pqxx::work& txn, int zone_id, int trigger_id);

} // namespace TriggerQueries

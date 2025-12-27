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

/// Load a single trigger by ID (for hot-reload)
Result<FieryMUD::TriggerDataPtr> load_trigger_by_id(
    pqxx::work& txn, int trigger_id);

/// Reload all triggers (clears cache and reloads from DB)
Result<std::vector<FieryMUD::TriggerDataPtr>> load_all_triggers(
    pqxx::work& txn);

} // namespace TriggerQueries

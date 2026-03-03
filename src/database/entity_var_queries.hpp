#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

#include "core/result.hpp"

namespace EntityVarQueries {

/// Load all variables for a specific entity
Result<std::unordered_map<std::string, nlohmann::json>> load_entity_vars(pqxx::work &txn, std::string_view entity_type,
                                                                         int zone_id, int entity_id);

/// Load all variables for all entities of a given type in a zone
Result<std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>>>
load_zone_entity_vars(pqxx::work &txn, std::string_view entity_type, int zone_id);

/// Save or update a single variable (upsert)
Result<void> save_entity_var(pqxx::work &txn, std::string_view entity_type, int zone_id, int entity_id,
                             const std::string &key, const nlohmann::json &value);

/// Delete a single variable
Result<void> delete_entity_var(pqxx::work &txn, std::string_view entity_type, int zone_id, int entity_id,
                               const std::string &key);

/// Save all variables for an entity (delete + re-insert)
Result<void> save_all_entity_vars(pqxx::work &txn, std::string_view entity_type, int zone_id, int entity_id,
                                  const std::unordered_map<std::string, nlohmann::json> &vars);

} // namespace EntityVarQueries

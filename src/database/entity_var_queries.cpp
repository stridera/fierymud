#include "database/entity_var_queries.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace EntityVarQueries {

Result<std::unordered_map<std::string, nlohmann::json>> load_entity_vars(pqxx::work &txn, std::string_view entity_type,
                                                                         int zone_id, int entity_id) {
    try {
        auto rows = txn.exec_params(R"(
            SELECT key, value FROM entity_variables
            WHERE entity_type = $1 AND zone_id = $2 AND entity_id = $3
        )",
                                    std::string(entity_type), zone_id, entity_id);

        std::unordered_map<std::string, nlohmann::json> vars;
        for (const auto &row : rows) {
            auto key = row["key"].as<std::string>();
            auto value_str = row["value"].as<std::string>();
            vars[key] = nlohmann::json::parse(value_str);
        }
        return vars;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_entity_vars: {}", e.what())));
    }
}

Result<std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>>>
load_zone_entity_vars(pqxx::work &txn, std::string_view entity_type, int zone_id) {
    try {
        auto rows = txn.exec_params(R"(
            SELECT entity_id, key, value FROM entity_variables
            WHERE entity_type = $1 AND zone_id = $2
            ORDER BY entity_id
        )",
                                    std::string(entity_type), zone_id);

        std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>> result;
        for (const auto &row : rows) {
            auto eid = row["entity_id"].as<int>();
            auto key = row["key"].as<std::string>();
            auto value_str = row["value"].as<std::string>();
            auto entity_key = fmt::format("{}:{}:{}", entity_type, zone_id, eid);
            result[entity_key][key] = nlohmann::json::parse(value_str);
        }
        return result;
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("load_zone_entity_vars: {}", e.what())));
    }
}

Result<void> save_entity_var(pqxx::work &txn, std::string_view entity_type, int zone_id, int entity_id,
                             const std::string &key, const nlohmann::json &value) {
    try {
        txn.exec_params(R"(
            INSERT INTO entity_variables (entity_type, zone_id, entity_id, key, value, updated_at)
            VALUES ($1, $2, $3, $4, $5::jsonb, NOW())
            ON CONFLICT (entity_type, zone_id, entity_id, key)
            DO UPDATE SET value = $5::jsonb, updated_at = NOW()
        )",
                        std::string(entity_type), zone_id, entity_id, key, value.dump());
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("save_entity_var: {}", e.what())));
    }
}

Result<void> delete_entity_var(pqxx::work &txn, std::string_view entity_type, int zone_id, int entity_id,
                               const std::string &key) {
    try {
        txn.exec_params(R"(
            DELETE FROM entity_variables
            WHERE entity_type = $1 AND zone_id = $2 AND entity_id = $3 AND key = $4
        )",
                        std::string(entity_type), zone_id, entity_id, key);
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("delete_entity_var: {}", e.what())));
    }
}

Result<void> save_all_entity_vars(pqxx::work &txn, std::string_view entity_type, int zone_id, int entity_id,
                                  const std::unordered_map<std::string, nlohmann::json> &vars) {
    try {
        // Delete existing vars for this entity
        txn.exec_params(R"(
            DELETE FROM entity_variables
            WHERE entity_type = $1 AND zone_id = $2 AND entity_id = $3
        )",
                        std::string(entity_type), zone_id, entity_id);

        // Insert all current vars
        for (const auto &[key, value] : vars) {
            txn.exec_params(R"(
                INSERT INTO entity_variables (entity_type, zone_id, entity_id, key, value, updated_at)
                VALUES ($1, $2, $3, $4, $5::jsonb, NOW())
            )",
                            std::string(entity_type), zone_id, entity_id, key, value.dump());
        }
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(Errors::DatabaseError(fmt::format("save_all_entity_vars: {}", e.what())));
    }
}

} // namespace EntityVarQueries

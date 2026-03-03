#include "core/entity_var_store.hpp"

#include <spdlog/spdlog.h>

#include "database/connection_pool.hpp"
#include "database/entity_var_queries.hpp"

namespace FieryMUD {

bool EntityVarStore::parse_entity_key(const std::string &ek, std::string &entity_type, int &zone_id, int &entity_id) {
    // Format: "TYPE:zone:id" (e.g., "MOB:30:5")
    auto first_colon = ek.find(':');
    if (first_colon == std::string::npos)
        return false;
    auto second_colon = ek.find(':', first_colon + 1);
    if (second_colon == std::string::npos)
        return false; // Legacy "zone:id" format — no entity type

    entity_type = ek.substr(0, first_colon);
    if (entity_type != "MOB" && entity_type != "OBJECT" && entity_type != "ROOM")
        return false; // Legacy format has numeric zone as first segment

    try {
        zone_id = std::stoi(ek.substr(first_colon + 1, second_colon - first_colon - 1));
        entity_id = std::stoi(ek.substr(second_colon + 1));
    } catch (...) {
        return false;
    }
    return true;
}

Result<int> EntityVarStore::save_dirty() {
    // Snapshot dirty set under lock, then release lock for DB I/O
    std::unordered_set<std::string> to_save;
    std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>> snapshots;
    {
        std::lock_guard lock(mutex_);
        if (dirty_.empty())
            return 0;
        to_save = dirty_;
        for (const auto &ek : to_save) {
            auto it = vars_.find(ek);
            if (it != vars_.end())
                snapshots[ek] = it->second;
        }
        dirty_.clear();
    }

    int saved_count = 0;
    auto result = ConnectionPool::instance().execute([&](pqxx::work &txn) -> Result<void> {
        for (const auto &[ek, var_map] : snapshots) {
            std::string entity_type;
            int zone_id, entity_id;
            if (!parse_entity_key(ek, entity_type, zone_id, entity_id)) {
                // Legacy format without entity type — skip (can't persist without type)
                spdlog::warn("EntityVarStore: skipping untyped entity key '{}' — cannot persist", ek);
                continue;
            }
            auto save_result = EntityVarQueries::save_all_entity_vars(txn, entity_type, zone_id, entity_id, var_map);
            if (!save_result)
                return std::unexpected(save_result.error());
            saved_count++;
        }
        return {};
    });

    if (!result) {
        // Re-mark as dirty on failure so they'll be retried
        std::lock_guard lock(mutex_);
        dirty_.insert(to_save.begin(), to_save.end());
        return std::unexpected(result.error());
    }

    if (saved_count > 0) {
        spdlog::debug("EntityVarStore: saved {} dirty entities", saved_count);
    }
    return saved_count;
}

} // namespace FieryMUD

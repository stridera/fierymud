#include "legacy_quest_bridge.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "database/connection_pool.hpp"
#include "quests/quest_manager.hpp"

namespace FieryMUD {

LegacyQuestBridge &LegacyQuestBridge::instance() {
    static LegacyQuestBridge instance;
    return instance;
}

void LegacyQuestBridge::initialize() {
    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        spdlog::warn("LegacyQuestBridge: Database not available, skipping initialization");
        return;
    }

    std::lock_guard lock(mutex_);
    name_cache_.clear();
    next_local_id_ = 0;

    auto result = pool.execute([this](pqxx::work &txn) -> Result<void> {
        try {
            auto rows = txn.exec_params(R"(
                SELECT id, plain_name FROM "Quest"
                WHERE zone_id = $1
                ORDER BY id
            )",
                                        LEGACY_ZONE_ID);

            for (const auto &row : rows) {
                int local_id = row["id"].as<int>();
                std::string plain_name = row["plain_name"].as<std::string>();
                name_cache_[plain_name] = EntityId(LEGACY_ZONE_ID, local_id);
                if (local_id >= next_local_id_) {
                    next_local_id_ = local_id + 1;
                }
            }
            return Success();
        } catch (const pqxx::sql_error &e) {
            return std::unexpected(Errors::DatabaseError(e.what()));
        }
    });

    if (result) {
        spdlog::info("LegacyQuestBridge: Loaded {} cached quest names from zone {}", name_cache_.size(),
                     LEGACY_ZONE_ID);

        // Load legacy quests into QuestManager's cache so they appear in quest logs
        if (!name_cache_.empty()) {
            auto &manager = QuestManager::instance();
            auto load_result = manager.load_zone_quests(LEGACY_ZONE_ID);
            if (load_result) {
                spdlog::info("LegacyQuestBridge: Loaded {} quests into QuestManager", *load_result);
            }
        }
    } else {
        spdlog::warn("LegacyQuestBridge: Failed to load cache: {}", result.error().message);
    }
}

EntityId LegacyQuestBridge::find(const std::string &name) const {
    std::lock_guard lock(mutex_);
    auto it = name_cache_.find(name);
    if (it != name_cache_.end()) {
        return it->second;
    }
    return INVALID_ENTITY_ID;
}

EntityId LegacyQuestBridge::get_or_create(const std::string &name) {
    // Fast path: check cache
    {
        std::lock_guard lock(mutex_);
        auto it = name_cache_.find(name);
        if (it != name_cache_.end()) {
            return it->second;
        }
    }

    auto &pool = ConnectionPool::instance();
    if (!pool.is_initialized()) {
        spdlog::warn("LegacyQuestBridge: Database not available");
        return INVALID_ENTITY_ID;
    }

    // Check DB for quest by plain_name (may exist in any zone for pre-migration)
    auto db_result = pool.execute([&](pqxx::work &txn) -> Result<EntityId> {
        try {
            auto rows = txn.exec_params(R"(
                SELECT zone_id, id FROM "Quest"
                WHERE plain_name = $1
                LIMIT 1
            )",
                                        name);

            if (!rows.empty()) {
                int zone_id = rows[0]["zone_id"].as<int>();
                int local_id = rows[0]["id"].as<int>();
                return EntityId(zone_id, local_id);
            }
            return INVALID_ENTITY_ID;
        } catch (const pqxx::sql_error &e) {
            return std::unexpected(Errors::DatabaseError(e.what()));
        }
    });

    if (db_result && db_result->is_valid()) {
        std::lock_guard lock(mutex_);
        name_cache_[name] = *db_result;
        return *db_result;
    }

    // Not found anywhere — auto-create in zone 9998
    if (!ensure_zone_exists()) {
        return INVALID_ENTITY_ID;
    }

    return create_quest(name);
}

bool LegacyQuestBridge::ensure_zone_exists() {
    auto &pool = ConnectionPool::instance();

    auto result = pool.execute([](pqxx::work &txn) -> Result<void> {
        try {
            txn.exec_params(R"(
                INSERT INTO "Zones" (id, name, updated_at)
                VALUES ($1, $2, NOW())
                ON CONFLICT (id) DO NOTHING
            )",
                            LEGACY_ZONE_ID, "Legacy Quests");
            return Success();
        } catch (const pqxx::sql_error &e) {
            return std::unexpected(Errors::DatabaseError(e.what()));
        }
    });

    if (!result) {
        spdlog::error("LegacyQuestBridge: Failed to ensure zone {}: {}", LEGACY_ZONE_ID, result.error().message);
    }
    return result.has_value();
}

EntityId LegacyQuestBridge::create_quest(const std::string &name) {
    auto &pool = ConnectionPool::instance();

    int local_id;
    {
        std::lock_guard lock(mutex_);
        local_id = next_local_id_++;
    }

    auto result = pool.execute([&](pqxx::work &txn) -> Result<EntityId> {
        try {
            txn.exec_params(R"(
                INSERT INTO "Quest" (zone_id, id, name, plain_name, trigger_type, hidden, updated_at)
                VALUES ($1, $2, $3, $4, 'MANUAL', false, NOW())
            )",
                            LEGACY_ZONE_ID, local_id, name, name);

            EntityId eid(LEGACY_ZONE_ID, local_id);
            return eid;
        } catch (const pqxx::sql_error &e) {
            return std::unexpected(Errors::DatabaseError(e.what()));
        }
    });

    if (result) {
        std::lock_guard lock(mutex_);
        name_cache_[name] = *result;
        spdlog::info("LegacyQuestBridge: Created quest '{}' as {}:{}", name, LEGACY_ZONE_ID, local_id);

        // Reload zone into QuestManager so quest is available for lookups
        QuestManager::instance().reload_zone_quests(LEGACY_ZONE_ID);

        return *result;
    }

    spdlog::error("LegacyQuestBridge: Failed to create quest '{}': {}", name, result.error().message);
    return INVALID_ENTITY_ID;
}

} // namespace FieryMUD

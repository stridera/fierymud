#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "ids.hpp"
#include "result.hpp"

namespace FieryMUD {

/**
 * EntityVarStore provides persistent key-value storage for entities.
 *
 * Variables are stored per-entity and can be any JSON-serializable value.
 * In-memory cache with dirty tracking; flushed to PostgreSQL on periodic
 * save and server shutdown, and bulk-loaded per zone at startup.
 *
 * Keys are scoped by entity type to avoid collisions between rooms, mobs,
 * and objects that share the same (zone_id, local_id).
 */
class EntityVarStore {
  public:
    static EntityVarStore &instance() {
        static EntityVarStore instance;
        return instance;
    }

    /**
     * Set a variable on an entity. Marks entity as dirty.
     * @param entity_type "MOB", "OBJECT", or "ROOM"
     */
    void set(std::string_view entity_type, const EntityId &entity_id, const std::string &key,
             const nlohmann::json &value) {
        std::lock_guard lock(mutex_);
        auto ek = entity_key(entity_type, entity_id);
        vars_[ek][key] = value;
        dirty_.insert(ek);
    }

    /** Backward-compatible overload (no entity type — uses untyped key). */
    void set(const EntityId &entity_id, const std::string &key, const nlohmann::json &value) {
        set("", entity_id, key, value);
    }

    /**
     * Get a variable from an entity.
     */
    std::optional<nlohmann::json> get(std::string_view entity_type, const EntityId &entity_id,
                                      const std::string &key) const {
        std::lock_guard lock(mutex_);
        auto ek = entity_key(entity_type, entity_id);
        auto entity_it = vars_.find(ek);
        if (entity_it == vars_.end())
            return std::nullopt;
        auto var_it = entity_it->second.find(key);
        if (var_it == entity_it->second.end())
            return std::nullopt;
        return var_it->second;
    }

    std::optional<nlohmann::json> get(const EntityId &entity_id, const std::string &key) const {
        return get("", entity_id, key);
    }

    /**
     * Check if an entity has a variable.
     */
    bool has(std::string_view entity_type, const EntityId &entity_id, const std::string &key) const {
        std::lock_guard lock(mutex_);
        auto ek = entity_key(entity_type, entity_id);
        auto entity_it = vars_.find(ek);
        if (entity_it == vars_.end())
            return false;
        return entity_it->second.contains(key);
    }

    bool has(const EntityId &entity_id, const std::string &key) const { return has("", entity_id, key); }

    /**
     * Remove a variable from an entity. Marks entity as dirty.
     */
    void clear(std::string_view entity_type, const EntityId &entity_id, const std::string &key) {
        std::lock_guard lock(mutex_);
        auto ek = entity_key(entity_type, entity_id);
        auto entity_it = vars_.find(ek);
        if (entity_it != vars_.end()) {
            entity_it->second.erase(key);
            dirty_.insert(ek);
        }
    }

    void clear(const EntityId &entity_id, const std::string &key) { clear("", entity_id, key); }

    /**
     * Get all variables for an entity.
     */
    std::unordered_map<std::string, nlohmann::json> all(std::string_view entity_type, const EntityId &entity_id) const {
        std::lock_guard lock(mutex_);
        auto ek = entity_key(entity_type, entity_id);
        auto entity_it = vars_.find(ek);
        if (entity_it == vars_.end())
            return {};
        return entity_it->second;
    }

    std::unordered_map<std::string, nlohmann::json> all(const EntityId &entity_id) const { return all("", entity_id); }

    /**
     * Bulk-load variables for all entities of a given type in a zone.
     * Called during zone loading.
     */
    void load_zone_vars(std::string_view entity_type, int zone_id,
                        const std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>> &data) {
        std::lock_guard lock(mutex_);
        for (const auto &[ek, var_map] : data) {
            vars_[ek] = var_map;
            // Not dirty — just loaded from DB
        }
    }

    /**
     * Save all dirty entities to the database.
     * Returns the number of entities saved.
     */
    Result<int> save_dirty();

    /** Check if there are unsaved changes. */
    bool has_dirty() const {
        std::lock_guard lock(mutex_);
        return !dirty_.empty();
    }

  private:
    EntityVarStore() = default;

    static std::string entity_key(std::string_view entity_type, const EntityId &id) {
        if (entity_type.empty())
            return id.to_string();
        return std::string(entity_type) + ":" + std::to_string(id.zone_id()) + ":" + std::to_string(id.local_id());
    }

    /** Parse an entity_key back into (entity_type, zone_id, entity_id).
     *  Returns false if the key is in legacy format (no entity_type). */
    static bool parse_entity_key(const std::string &ek, std::string &entity_type, int &zone_id, int &entity_id);

    // In-memory storage: entity_key -> (var_key -> value)
    std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>> vars_;

    // Dirty entity keys that need to be flushed to DB
    std::unordered_set<std::string> dirty_;

    mutable std::mutex mutex_;
};

} // namespace FieryMUD

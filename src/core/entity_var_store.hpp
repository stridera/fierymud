#pragma once

#include "ids.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>

namespace FieryMUD {

/**
 * EntityVarStore provides persistent key-value storage for entities.
 *
 * Variables are stored per-entity and can be any JSON-serializable value.
 * This is a stub implementation for the Lua scripting API.
 * TODO: Implement persistence to database.
 */
class EntityVarStore {
public:
    static EntityVarStore& instance() {
        static EntityVarStore instance;
        return instance;
    }

    /**
     * Set a variable on an entity.
     */
    void set(const EntityId& entity_id, const std::string& key,
             const nlohmann::json& value) {
        auto entity_key = entity_id.to_string();
        vars_[entity_key][key] = value;
    }

    /**
     * Get a variable from an entity.
     * @return Variable value, or nullopt if not found
     */
    std::optional<nlohmann::json> get(const EntityId& entity_id,
                                       const std::string& key) const {
        auto entity_key = entity_id.to_string();
        auto entity_it = vars_.find(entity_key);
        if (entity_it == vars_.end()) {
            return std::nullopt;
        }

        auto var_it = entity_it->second.find(key);
        if (var_it == entity_it->second.end()) {
            return std::nullopt;
        }

        return var_it->second;
    }

    /**
     * Check if an entity has a variable.
     */
    bool has(const EntityId& entity_id, const std::string& key) const {
        auto entity_key = entity_id.to_string();
        auto entity_it = vars_.find(entity_key);
        if (entity_it == vars_.end()) {
            return false;
        }
        return entity_it->second.contains(key);
    }

    /**
     * Remove a variable from an entity.
     */
    void clear(const EntityId& entity_id, const std::string& key) {
        auto entity_key = entity_id.to_string();
        auto entity_it = vars_.find(entity_key);
        if (entity_it != vars_.end()) {
            entity_it->second.erase(key);
        }
    }

    /**
     * Get all variables for an entity.
     */
    std::unordered_map<std::string, nlohmann::json> all(const EntityId& entity_id) const {
        auto entity_key = entity_id.to_string();
        auto entity_it = vars_.find(entity_key);
        if (entity_it == vars_.end()) {
            return {};
        }
        return entity_it->second;
    }

private:
    EntityVarStore() = default;

    // In-memory storage: entity_id -> (key -> value)
    // TODO: Persist to database
    std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>> vars_;
};

} // namespace FieryMUD

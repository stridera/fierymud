#include "lua_vars.hpp"

#include "../../core/actor.hpp"
#include "../../core/entity_var_store.hpp"
#include "../../core/object.hpp"
#include "../../world/room.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

namespace {

// Get entity ID from various entity types
std::optional<EntityId> get_entity_id(sol::object entity) {
    // Try as Actor pointer
    if (entity.is<Actor *>()) {
        auto *actor = entity.as<Actor *>();
        if (actor)
            return actor->id();
    }

    // Try as shared_ptr<Room>
    if (entity.is<std::shared_ptr<Room>>()) {
        auto room = entity.as<std::shared_ptr<Room>>();
        if (room)
            return room->id();
    }

    // Try as shared_ptr<Object>
    if (entity.is<std::shared_ptr<Object>>()) {
        auto obj = entity.as<std::shared_ptr<Object>>();
        if (obj)
            return obj->id();
    }

    return std::nullopt;
}

// Convert Lua value to JSON
nlohmann::json lua_to_json(sol::object value) {
    if (value.is<bool>()) {
        return value.as<bool>();
    } else if (value.is<int>()) {
        return value.as<int>();
    } else if (value.is<double>()) {
        return value.as<double>();
    } else if (value.is<std::string>()) {
        return value.as<std::string>();
    } else if (value.is<sol::table>()) {
        // Convert table to JSON object or array
        sol::table tbl = value.as<sol::table>();
        bool is_array = true;
        int expected_index = 1;

        // Check if it's an array (sequential integer keys starting at 1)
        for (auto &[k, v] : tbl) {
            if (!k.is<int>() || k.as<int>() != expected_index) {
                is_array = false;
                break;
            }
            expected_index++;
        }

        if (is_array) {
            nlohmann::json arr = nlohmann::json::array();
            for (auto &[k, v] : tbl) {
                arr.push_back(lua_to_json(v));
            }
            return arr;
        } else {
            nlohmann::json obj = nlohmann::json::object();
            for (auto &[k, v] : tbl) {
                std::string key;
                if (k.is<std::string>()) {
                    key = k.as<std::string>();
                } else if (k.is<int>()) {
                    key = std::to_string(k.as<int>());
                } else {
                    continue; // Skip non-string/int keys
                }
                obj[key] = lua_to_json(v);
            }
            return obj;
        }
    }
    return nullptr;
}

// Convert JSON to Lua value
sol::object json_to_lua(sol::state &lua, const nlohmann::json &json) {
    if (json.is_boolean()) {
        return sol::make_object(lua, json.get<bool>());
    } else if (json.is_number_integer()) {
        return sol::make_object(lua, json.get<int>());
    } else if (json.is_number_float()) {
        return sol::make_object(lua, json.get<double>());
    } else if (json.is_string()) {
        return sol::make_object(lua, json.get<std::string>());
    } else if (json.is_array()) {
        sol::table tbl = lua.create_table();
        int index = 1;
        for (const auto &item : json) {
            tbl[index++] = json_to_lua(lua, item);
        }
        return tbl;
    } else if (json.is_object()) {
        sol::table tbl = lua.create_table();
        for (auto &[key, val] : json.items()) {
            tbl[key] = json_to_lua(lua, val);
        }
        return tbl;
    }
    return sol::nil;
}

} // anonymous namespace

void register_var_bindings(sol::state &lua) {
    auto vars_table = lua.create_named_table("vars");

    // vars.set(entity, key, value) - Set a variable on an entity
    // Returns: void
    vars_table["set"] = [](sol::object entity, const std::string &key, sol::object value) {
        if (key.empty()) {
            spdlog::warn("vars.set: empty key");
            return;
        }

        auto entity_id = get_entity_id(entity);
        if (!entity_id) {
            spdlog::warn("vars.set: invalid entity");
            return;
        }

        auto json_value = lua_to_json(value);
        EntityVarStore::instance().set(*entity_id, key, json_value);

        spdlog::debug("vars.set: {}[{}] = {}", entity_id->to_string(), key, json_value.dump());
    };

    // vars.get(entity, key) - Get a variable from an entity
    // Returns: any (or nil if not found)
    vars_table["get"] = [&lua](sol::object entity, const std::string &key) -> sol::object {
        if (key.empty()) {
            return sol::nil;
        }

        auto entity_id = get_entity_id(entity);
        if (!entity_id) {
            return sol::nil;
        }

        auto value = EntityVarStore::instance().get(*entity_id, key);
        if (!value) {
            return sol::nil;
        }

        return json_to_lua(lua, *value);
    };

    // vars.has(entity, key) - Check if entity has a variable
    // Returns: bool
    vars_table["has"] = [](sol::object entity, const std::string &key) -> bool {
        if (key.empty()) {
            return false;
        }

        auto entity_id = get_entity_id(entity);
        if (!entity_id) {
            return false;
        }

        return EntityVarStore::instance().has(*entity_id, key);
    };

    // vars.clear(entity, key) - Remove a variable from an entity
    // Returns: void
    vars_table["clear"] = [](sol::object entity, const std::string &key) {
        if (key.empty()) {
            return;
        }

        auto entity_id = get_entity_id(entity);
        if (!entity_id) {
            return;
        }

        EntityVarStore::instance().clear(*entity_id, key);
        spdlog::debug("vars.clear: {}[{}]", entity_id->to_string(), key);
    };

    // vars.all(entity) - Get all variables for an entity
    // Returns: table
    vars_table["all"] = [&lua](sol::object entity) -> sol::table {
        sol::table result = lua.create_table();

        auto entity_id = get_entity_id(entity);
        if (!entity_id) {
            return result;
        }

        auto vars = EntityVarStore::instance().all(*entity_id);
        for (const auto &[key, value] : vars) {
            result[key] = json_to_lua(lua, value);
        }

        return result;
    };

    spdlog::debug("Registered vars Lua bindings");
}

} // namespace FieryMUD

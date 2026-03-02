#include "lua_vars.hpp"

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

#include "../../core/actor.hpp"
#include "../../core/entity_var_store.hpp"
#include "../../core/object.hpp"
#include "../../world/room.hpp"
#include "lua_script_helpers.hpp"

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

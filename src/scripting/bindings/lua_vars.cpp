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

struct EntityInfo {
    EntityId id;
    std::string_view type; // "MOB", "OBJECT", or "ROOM"
};

// Get entity ID and type from various entity types
std::optional<EntityInfo> get_entity_info(sol::object entity) {
    // Try as Actor pointer
    if (entity.is<Actor *>()) {
        auto *actor = entity.as<Actor *>();
        if (actor)
            return EntityInfo{actor->id(), "MOB"};
    }

    // Try as shared_ptr<Room>
    if (entity.is<std::shared_ptr<Room>>()) {
        auto room = entity.as<std::shared_ptr<Room>>();
        if (room)
            return EntityInfo{room->id(), "ROOM"};
    }

    // Try as shared_ptr<Object>
    if (entity.is<std::shared_ptr<Object>>()) {
        auto obj = entity.as<std::shared_ptr<Object>>();
        if (obj)
            return EntityInfo{obj->id(), "OBJECT"};
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

        auto info = get_entity_info(entity);
        if (!info) {
            spdlog::warn("vars.set: invalid entity");
            return;
        }

        auto json_value = lua_to_json(value);
        EntityVarStore::instance().set(info->type, info->id, key, json_value);

        spdlog::debug("vars.set: {}:{}[{}] = {}", info->type, info->id.to_string(), key, json_value.dump());
    };

    // vars.get(entity, key) - Get a variable from an entity
    // Returns: any (or nil if not found)
    vars_table["get"] = [&lua](sol::object entity, const std::string &key) -> sol::object {
        if (key.empty()) {
            return sol::nil;
        }

        auto info = get_entity_info(entity);
        if (!info) {
            return sol::nil;
        }

        auto value = EntityVarStore::instance().get(info->type, info->id, key);
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

        auto info = get_entity_info(entity);
        if (!info) {
            return false;
        }

        return EntityVarStore::instance().has(info->type, info->id, key);
    };

    // vars.clear(entity, key) - Remove a variable from an entity
    // Returns: void
    vars_table["clear"] = [](sol::object entity, const std::string &key) {
        if (key.empty()) {
            return;
        }

        auto info = get_entity_info(entity);
        if (!info) {
            return;
        }

        EntityVarStore::instance().clear(info->type, info->id, key);
        spdlog::debug("vars.clear: {}:{}[{}]", info->type, info->id.to_string(), key);
    };

    // vars.all(entity) - Get all variables for an entity
    // Returns: table
    vars_table["all"] = [&lua](sol::object entity) -> sol::table {
        sol::table result = lua.create_table();

        auto info = get_entity_info(entity);
        if (!info) {
            return result;
        }

        auto vars = EntityVarStore::instance().all(info->type, info->id);
        for (const auto &[key, value] : vars) {
            result[key] = json_to_lua(lua, value);
        }

        return result;
    };

    spdlog::debug("Registered vars Lua bindings");
}

} // namespace FieryMUD

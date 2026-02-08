#include "lua_templates.hpp"

#include "../../core/ids.hpp"
#include "../../world/templates.hpp"
#include "../../world/world_manager.hpp"

using FieryMUD::MobileTemplate;
using FieryMUD::ObjectTemplate;

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_template_bindings(sol::state &lua) {
    // Register ObjectTemplate usertype
    lua.new_usertype<ObjectTemplate>(
        "ObjectTemplate", sol::no_constructor,

        // Read-only properties
        "name", sol::property([](const ObjectTemplate &t) { return std::string(t.name()); }), "keywords",
        sol::property([](const ObjectTemplate &t) { return std::string(t.keywords()); }), "description",
        sol::property([](const ObjectTemplate &t) { return std::string(t.description()); }), "weight",
        sol::property(&ObjectTemplate::weight), "value", sol::property(&ObjectTemplate::value), "level",
        sol::property(&ObjectTemplate::level), "type", sol::property(&ObjectTemplate::type), "zone_id",
        sol::property([](const ObjectTemplate &t) { return t.id().zone_id(); }), "local_id",
        sol::property([](const ObjectTemplate &t) { return t.id().local_id(); }), "id",
        sol::property([](const ObjectTemplate &t) { return t.id().to_string(); }));

    // Register MobileTemplate usertype
    lua.new_usertype<MobileTemplate>(
        "MobileTemplate", sol::no_constructor,

        // Read-only properties
        "name", sol::property([](const MobileTemplate &t) { return std::string(t.name()); }), "keywords",
        sol::property([](const MobileTemplate &t) { return std::string(t.keywords()); }), "description",
        sol::property([](const MobileTemplate &t) { return std::string(t.description()); }), "level",
        sol::property(&MobileTemplate::level), "alignment", sol::property(&MobileTemplate::alignment), "max_hp",
        sol::property(&MobileTemplate::max_hp), "experience", sol::property(&MobileTemplate::experience), "gold",
        sol::property(&MobileTemplate::gold), "zone_id",
        sol::property([](const MobileTemplate &t) { return t.id().zone_id(); }), "local_id",
        sol::property([](const MobileTemplate &t) { return t.id().local_id(); }), "id",
        sol::property([](const MobileTemplate &t) { return t.id().to_string(); }));

    // Create objects namespace (or get existing one)
    sol::table objects_table;
    if (lua["objects"].valid()) {
        objects_table = lua["objects"];
    } else {
        objects_table = lua.create_named_table("objects");
    }

    // objects.template(zone_id, local_id) - Get read-only object template
    // Returns: ObjectTemplate or nil
    objects_table["template"] = [](int zone_id, int local_id) -> std::shared_ptr<ObjectTemplate> {
        auto &world = WorldManager::instance();
        EntityId id(zone_id, local_id);

        auto tmpl = world.get_object_template(id);
        if (tmpl) {
            spdlog::debug("objects.template: found {}:{}", zone_id, local_id);
        }
        return tmpl;
    };

    // Create mobiles namespace (or get existing one)
    sol::table mobiles_table;
    if (lua["mobiles"].valid()) {
        mobiles_table = lua["mobiles"];
    } else {
        mobiles_table = lua.create_named_table("mobiles");
    }

    // mobiles.template(zone_id, local_id) - Get read-only mobile template
    // Returns: MobileTemplate or nil
    mobiles_table["template"] = [](int zone_id, int local_id) -> std::shared_ptr<MobileTemplate> {
        auto &world = WorldManager::instance();
        EntityId id(zone_id, local_id);

        auto tmpl = world.get_mobile_template(id);
        if (tmpl) {
            spdlog::debug("mobiles.template: found {}:{}", zone_id, local_id);
        }
        return tmpl;
    };

    spdlog::debug("Registered template Lua bindings (objects.template, mobiles.template)");
}

} // namespace FieryMUD

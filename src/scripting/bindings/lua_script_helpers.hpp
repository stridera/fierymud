#pragma once

#include <string>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

namespace FieryMUD {

/// Format trigger identity from Lua thread globals for error messages.
/// Returns e.g. "trigger 'Green Woman Apothecary shop load' (60:3)"
inline std::string trigger_context(sol::this_state ts) {
    sol::state_view lua(ts);
    auto name = lua.get<sol::optional<std::string>>("__trigger_name");
    auto zone = lua.get<sol::optional<int>>("__trigger_zone_id");
    auto id = lua.get<sol::optional<int>>("__trigger_id");
    if (name && zone && id) {
        return fmt::format("trigger '{}' ({}:{})", *name, *zone, *id);
    }
    return "unknown trigger";
}

/// Convert a Lua value to nlohmann::json. Handles bool, int, double, string,
/// and tables (arrays with sequential 1-based int keys, otherwise objects).
inline nlohmann::json lua_to_json(sol::object value) {
    if (value.is<bool>()) {
        return value.as<bool>();
    } else if (value.is<int>()) {
        return value.as<int>();
    } else if (value.is<double>()) {
        return value.as<double>();
    } else if (value.is<std::string>()) {
        return value.as<std::string>();
    } else if (value.is<sol::table>()) {
        sol::table tbl = value.as<sol::table>();
        bool is_array = true;
        int expected_index = 1;

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
                    continue;
                }
                obj[key] = lua_to_json(v);
            }
            return obj;
        }
    }
    return nullptr;
}

/// Convert nlohmann::json to a Lua value. Handles bool, int, float, string,
/// arrays (1-indexed tables), and objects (string-keyed tables).
inline sol::object json_to_lua(sol::state &lua, const nlohmann::json &json) {
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

} // namespace FieryMUD

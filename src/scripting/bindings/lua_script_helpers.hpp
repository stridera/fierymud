#pragma once

#include <string>

#include <fmt/format.h>
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

} // namespace FieryMUD

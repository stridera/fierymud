#include "lua_zone.hpp"
#include "../../world/world_manager.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <spdlog/spdlog.h>

namespace FieryMUD {

void register_zone_bindings(sol::state& lua) {
    auto zone_table = lua.create_named_table("zone");

    // zone.echo(zone_id, message) - Send message to all players in a zone
    // Returns: void
    zone_table["echo"] = [](int zone_id, const std::string& message) {
        if (message.empty()) {
            return;
        }

        auto& world = WorldManager::instance();
        auto result = world.zone_echo(zone_id, message);

        if (result) {
            spdlog::debug("zone.echo: sent to zone {}: {}", zone_id, message);
        } else {
            spdlog::warn("zone.echo: failed for zone {}: {}", zone_id, result.error().message);
        }
    };

    // zone.reset(zone_id) - Trigger zone reset
    // Returns: (bool success, string? error)
    zone_table["reset"] = [](int zone_id) -> std::tuple<bool, sol::optional<std::string>> {
        auto& world = WorldManager::instance();
        auto result = world.reset_zone(zone_id);

        if (!result) {
            return std::make_tuple(false, result.error().message);
        }

        spdlog::debug("zone.reset: triggered reset for zone {}", zone_id);
        return std::make_tuple(true, sol::optional<std::string>{});
    };

    spdlog::debug("Registered zone Lua bindings");
}

} // namespace FieryMUD

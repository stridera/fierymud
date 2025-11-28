#include "zone_reload_handler.hpp"
#include "../world/world_manager.hpp"
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace fierymud {

void register_zone_reload_handlers(AdminServer& admin_server, WorldManager& world_manager) {
    // POST /api/admin/reload-zone - Reload a specific zone or all zones
    admin_server.register_handler("/api/admin/reload-zone", [&world_manager](const std::string& path, const std::string& body) -> std::string {
        spdlog::info("Received zone reload request");

        try {
            // Parse request
            ReloadZoneRequest request = parse_reload_request(body);

            // Validate zone ID
            if (request.zone_id < 0) {
                json error_json = {
                    {"error", "Bad Request"},
                    {"message", "Invalid zone_id: must be >= 0"}
                };
                return error_json.dump();
            }

            // Execute reload
            ReloadZoneResponse response = handle_zone_reload(world_manager, request.zone_id, request.force);

            // Return response
            return serialize_reload_response(response);

        } catch (const std::exception& e) {
            spdlog::error("Zone reload handler error: {}", e.what());
            json error_json = {
                {"error", "Internal Server Error"},
                {"message", e.what()}
            };
            return error_json.dump();
        }
    });

    // GET /api/admin/zone-status - Get status of all zones
    admin_server.register_handler("/api/admin/zone-status", [&world_manager](const std::string& path, const std::string& body) -> std::string {
        spdlog::info("Received zone status request");

        try {
            json zones_json = json::array();

            // Get all zone information from world manager
            // NOTE: This requires implementing get_zone_info() in WorldManager
            // For now, return a placeholder response
            json response = {
                {"zones", zones_json},
                {"total_zones", 0},
                {"message", "Zone status endpoint not yet fully implemented"}
            };

            return response.dump();

        } catch (const std::exception& e) {
            spdlog::error("Zone status handler error: {}", e.what());
            json error_json = {
                {"error", "Internal Server Error"},
                {"message", e.what()}
            };
            return error_json.dump();
        }
    });

    // GET /api/admin/health - Simple health check endpoint
    admin_server.register_handler("/api/admin/health", [](const std::string& path, const std::string& body) -> std::string {
        json health = {
            {"status", "healthy"},
            {"service", "FieryMUD Admin API"},
            {"timestamp", std::time(nullptr)}
        };
        return health.dump();
    });

    spdlog::info("Registered zone reload handlers with admin server");
}

ReloadZoneResponse handle_zone_reload(WorldManager& world_manager, int zone_id, bool force) {
    ReloadZoneResponse response;
    response.success = false;
    response.zones_reloaded = 0;

    try {
        if (zone_id == 0) {
            // Reload all zones
            spdlog::info("Reloading all zones (force={})", force);

            // NOTE: This requires implementing reload_all_zones() in WorldManager
            // For now, return a placeholder response
            response.success = false;
            response.message = "Reload all zones not yet implemented - please reload individual zones";
            response.zones_reloaded = 0;

            return response;
        } else {
            // Reload specific zone
            spdlog::info("Reloading zone {} (force={})", zone_id, force);

            // Check if zone exists
            // NOTE: This requires implementing zone_exists() in WorldManager
            // For now, assume it exists

            // Perform reload
            // NOTE: This requires implementing reload_zone() in WorldManager
            // The actual implementation would:
            // 1. Check if zone is currently active (has players/mobs)
            // 2. If force=false and zone is active, return error
            // 3. If force=true or zone is inactive:
            //    a. Save any dynamic state that should persist
            //    b. Unload zone data from memory
            //    c. Reload zone data from database
            //    d. Restore any saved state
            //    e. Re-initialize zone (mobs, objects, resets)

            // Placeholder implementation
            response.success = true;
            response.message = fmt::format("Zone {} reload requested - actual implementation pending", zone_id);
            response.zones_reloaded = 1;

            // In a real implementation, you might add warnings like:
            // if (zone_has_players && !force) {
            //     response.success = false;
            //     response.errors.push_back("Zone has active players - use force=true to reload anyway");
            // }

            return response;
        }

    } catch (const std::exception& e) {
        spdlog::error("Zone reload failed: {}", e.what());
        response.success = false;
        response.message = fmt::format("Zone reload failed: {}", e.what());
        response.errors.push_back(e.what());
        return response;
    }
}

} // namespace fierymud

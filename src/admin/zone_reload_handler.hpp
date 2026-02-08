#pragma once

#include "admin_server.hpp"

#include <string>

namespace fierymud {

// Forward declarations
class WorldManager;

/**
 * Register zone reload handlers with the admin server
 * This allows remote zone reloading via HTTP API
 */
void register_zone_reload_handlers(AdminServer &admin_server, WorldManager &world_manager);

/**
 * Handle a zone reload request
 * @param world_manager Reference to the world manager
 * @param zone_id Zone ID to reload (0 for all zones)
 * @param force Force reload even if zone is currently active
 * @return ReloadZoneResponse with results
 */
ReloadZoneResponse handle_zone_reload(WorldManager &world_manager, int zone_id, bool force);

} // namespace fierymud

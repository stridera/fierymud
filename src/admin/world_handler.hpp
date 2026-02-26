#pragma once

#include "admin_server.hpp"

// Forward declarations
class ModernMUDServer;

namespace fierymud {

/**
 * Register world control and inspection handlers with the admin server.
 *
 * Endpoints:
 *   POST /api/admin/world/pause    - Freeze all game systems
 *   POST /api/admin/world/unpause  - Resume normal operation
 *   GET  /api/admin/world/status   - Paused/running state
 *   POST /api/admin/world/tick     - Advance N game ticks while paused
 *   GET  /api/admin/room/:zoneId/:id  - Room details
 *   GET  /api/admin/mob/:zoneId/:id   - Mob prototype details
 *   GET  /api/admin/actor/:name       - Live actor state
 *   POST /api/admin/spawn             - Spawn mob/object into world
 */
void register_world_handlers(AdminServer &admin_server, ModernMUDServer &mud_server);

} // namespace fierymud

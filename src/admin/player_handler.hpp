#pragma once

#include "admin_server.hpp"

// Forward declarations
class ModernMUDServer;

namespace fierymud {

/**
 * Register player and admin management handlers with the admin server
 *
 * Endpoints:
 *   GET  /api/admin/players   - List online players
 *   POST /api/admin/command   - Execute a god command
 *   POST /api/admin/broadcast - Send message to all players
 *   POST /api/admin/kick      - Disconnect a player
 *   GET  /api/admin/stats     - Server statistics
 */
void register_player_handlers(AdminServer &admin_server, ModernMUDServer &mud_server);

} // namespace fierymud

#pragma once

#include <memory>
#include <string_view>

#include "admin_server.hpp"

// Forward declarations
class ModernMUDServer;
class Player;

namespace fierymud {

/**
 * Register virtual session handlers with the admin server.
 *
 * Virtual sessions allow automated testing without a telnet connection.
 * Players are loaded from the database and placed in the world with
 * output accumulating in their output_queue_ (no network output).
 *
 * Endpoints:
 *   POST /api/admin/session/create   - Create a virtual test player session
 *   POST /api/admin/session/destroy  - Remove a virtual player from the world
 *   GET  /api/admin/session/output   - Get accumulated output for a virtual player
 */
void register_session_handlers(AdminServer &admin_server, ModernMUDServer &mud_server);

/**
 * Find a virtual session player by name.
 * Used by ModernMUDServer::find_player() to locate virtual session players
 * that don't have a network connection.
 */
std::shared_ptr<Player> find_virtual_session(std::string_view name);

} // namespace fierymud

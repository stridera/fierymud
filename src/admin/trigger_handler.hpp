#pragma once

#include "admin_server.hpp"

// Forward declarations
class ModernMUDServer;

namespace fierymud {

/**
 * Register trigger debugging handlers with the admin server.
 *
 * Endpoints:
 *   GET  /api/admin/triggers/errors            - List triggers needing review
 *   GET  /api/admin/triggers/errors/recent      - Recent script error log
 *   GET  /api/admin/triggers/zone/{zoneId}      - List triggers in a zone
 *   GET  /api/admin/triggers/{zoneId}/{id}      - Single trigger details
 *   POST /api/admin/triggers/{zoneId}/{id}/fire - Execute trigger for debugging
 *   POST /api/admin/triggers/reload/{zoneId}    - Reload zone triggers from DB
 *   GET  /api/admin/triggers/stats              - Execution statistics
 */
void register_trigger_handlers(AdminServer &admin_server, ModernMUDServer &mud_server);

} // namespace fierymud

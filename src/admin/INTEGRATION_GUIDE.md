# FieryMUD Admin API Integration Guide

This guide shows how to integrate the admin API into FieryMUD's main server.

## Overview

The admin API provides a simple HTTP-based interface for remote management operations:

- **Zone Reloading**: Reload zones from database without restarting the server
- **Health Checks**: Monitor server status
- **Zone Status**: Query zone information

## Files

- `admin_server.hpp/cpp` - Core HTTP server implementation
- `zone_reload_handler.hpp/cpp` - Zone reload command handlers
- Example integration in `src/main.cpp` (see below)

## Integration Steps

### 1. Add Admin Server to Main Server

In `src/main.cpp` (or wherever your main server initialization is):

```cpp
#include "admin/admin_server.hpp"
#include "admin/zone_reload_handler.hpp"
#include "world/world_manager.hpp"

int main(int argc, char** argv) {
    // ... existing initialization code ...

    // Create world manager (your existing code)
    WorldManager world_manager;

    // Create and configure admin server
    uint16_t admin_port = 8080; // Configure from environment or config file
    std::string admin_bind = "127.0.0.1"; // Localhost only for security
    AdminServer admin_server(admin_port, admin_bind);

    // Register zone reload handlers
    register_zone_reload_handlers(admin_server, world_manager);

    // Start admin server
    admin_server.start();

    // ... start game server and main loop ...

    // On shutdown
    admin_server.stop();

    return 0;
}
```

### 2. Configure Authentication

Set the `FIERYMUD_ADMIN_TOKEN` environment variable:

```bash
export FIERYMUD_ADMIN_TOKEN="your-secret-token-here"
./fierymud
```

**Important**: Always use a strong, randomly-generated token in production!

### 3. Update WorldManager (Required)

The zone reload functionality requires these methods in `WorldManager`:

```cpp
class WorldManager {
public:
    // ... existing methods ...

    /**
     * Reload a zone from the database
     * @param zone_id Zone ID to reload
     * @param force Force reload even if zone has active players
     * @return true if reload succeeded
     */
    bool reload_zone(int zone_id, bool force = false);

    /**
     * Reload all zones from the database
     * @param force Force reload even if zones have active players
     * @return number of zones successfully reloaded
     */
    int reload_all_zones(bool force = false);

    /**
     * Check if a zone exists
     * @param zone_id Zone ID to check
     * @return true if zone exists
     */
    bool zone_exists(int zone_id) const;

    /**
     * Check if a zone has active players
     * @param zone_id Zone ID to check
     * @return true if zone has players
     */
    bool zone_has_active_players(int zone_id) const;
};
```

### 4. Update CMakeLists.txt

Add the new files to your CMake configuration:

```cmake
add_executable(fierymud
    # ... existing sources ...
    src/admin/admin_server.cpp
    src/admin/zone_reload_handler.cpp
)

target_link_libraries(fierymud
    # ... existing libraries ...
    asio::asio
    fmt::fmt
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
```

## API Usage

### From Muditor Deployment Service

The deployment service can call the admin API after completing a deployment:

```typescript
// After successful deployment to prod database
const response = await fetch('http://localhost:8080/api/admin/reload-zone', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Authorization': `Bearer ${process.env.FIERYMUD_ADMIN_TOKEN}`,
  },
  body: JSON.stringify({
    zone_id: 30,
    force: false,
  }),
});

const result = await response.json();
console.log(`Zone reload: ${result.message}`);
```

### Manual Testing with curl

```bash
# Health check
curl http://localhost:8080/api/admin/health

# Reload zone 30
curl -X POST http://localhost:8080/api/admin/reload-zone \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer your-secret-token" \
  -d '{"zone_id": 30, "force": false}'

# Get zone status
curl http://localhost:8080/api/admin/zone-status \
  -H "Authorization: Bearer your-secret-token"
```

## Security Considerations

1. **Bind to localhost only**: The default configuration binds to `127.0.0.1`, preventing external access
2. **Use authentication**: Always set `FIERYMUD_ADMIN_TOKEN` in production
3. **Firewall rules**: If binding to external interfaces, use firewall rules to restrict access
4. **TLS/SSL**: Consider adding HTTPS support for production deployments
5. **Rate limiting**: Add rate limiting to prevent abuse

## Future Enhancements

- **Command queueing**: Queue reload commands to prevent concurrent modifications
- **Progress tracking**: Return progress updates for long-running operations
- **Rollback support**: Implement rollback functionality for failed reloads
- **WebSocket support**: Add WebSocket endpoint for real-time status updates
- **Additional commands**: Player kicks, server status, configuration reload, etc.

## Troubleshooting

### Admin server fails to start

- Check if port 8080 is already in use: `lsof -i :8080`
- Verify bind address is correct for your network configuration
- Check file permissions and firewall rules

### Authentication failures

- Verify `FIERYMUD_ADMIN_TOKEN` is set correctly
- Check that the token matches between server and client
- Ensure the `Authorization: Bearer` header format is correct

### Zone reload failures

- Check WorldManager implementation is complete
- Verify database connectivity
- Check zone_id exists in database
- Review server logs for detailed error messages

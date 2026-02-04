#pragma once

#include <asio.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// Silence spurious warnings in <functional> header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <functional>
#pragma GCC diagnostic pop

namespace fierymud {

/**
 * Simple HTTP-based admin server for remote management commands
 * Listens on a separate port from the game server for admin operations
 */
class AdminServer {
public:
    /**
     * Handler function type for admin commands
     * Returns a JSON response string
     */
    using CommandHandler = std::function<std::string(const std::string& path, const std::string& body)>;

    /**
     * Constructor
     * @param port Port to listen on (default: 8080)
     * @param bind_address Address to bind to (default: 127.0.0.1 for localhost only)
     */
    explicit AdminServer(uint16_t port = 8080, const std::string& bind_address = "127.0.0.1");

    ~AdminServer();

    /**
     * Start the admin server in a background thread
     */
    void start();

    /**
     * Stop the admin server gracefully
     */
    void stop();

    /**
     * Register a handler for a specific path (e.g., "/reload-zone")
     */
    void register_handler(const std::string& path, CommandHandler handler);

    /**
     * Check if server is running
     */
    bool is_running() const { return running_.load(); }

private:
    void start_accept();
    void handle_connection(std::shared_ptr<asio::ip::tcp::socket> socket);
    std::string parse_request(const std::string& request, std::string& path, std::string& body);
    std::string build_response(int status_code, const std::string& content_type, const std::string& body);

    uint16_t port_;
    std::string bind_address_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> server_thread_;
    std::unique_ptr<asio::io_context> io_context_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    std::unordered_map<std::string, CommandHandler> handlers_;

    // Authentication token (should be configured from environment)
    std::string auth_token_;
};

/**
 * Zone reload command structure
 */
struct ReloadZoneRequest {
    int zone_id;
    bool force = false;
};

/**
 * Zone reload response structure
 */
struct ReloadZoneResponse {
    bool success;
    std::string message;
    int zones_reloaded;
    std::vector<std::string> errors;
};

/**
 * Parse a reload zone request from JSON
 */
ReloadZoneRequest parse_reload_request(const std::string& json);

/**
 * Serialize a reload zone response to JSON
 */
std::string serialize_reload_response(const ReloadZoneResponse& response);

} // namespace fierymud

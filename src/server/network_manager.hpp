#pragma once

#include "core/result.hpp"
#include "net/tls_context.hpp"
#include <asio.hpp>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

// Forward declarations
struct ServerConfig;
class Player;
class WorldServer;
class PlayerConnection;

/**
 * Modern Network Manager - Asynchronous I/O for player connections
 */
class NetworkManager {
public:
    NetworkManager(asio::io_context& io_context, const ServerConfig& config);
    ~NetworkManager();

    // Set world server for command processing
    void set_world_server(WorldServer* world_server) { world_server_ = world_server; }

    Result<void> initialize();
    Result<void> start();
    void stop();

    size_t connection_count() const;
    std::vector<std::shared_ptr<Player>> get_connected_players() const;

    // TLS support
    bool is_tls_enabled() const { return tls_context_manager_ && tls_context_manager_->is_initialized(); }
    const ServerConfig& get_config() const { return config_; } // Access for MSSP handler

    // Reconnection support
    bool handle_reconnection(std::shared_ptr<PlayerConnection> new_connection, std::string_view player_name);

private:
    void start_accept();
    void handle_accept(std::shared_ptr<PlayerConnection> connection,
                      const asio::error_code& error);

    // TLS accept methods
    void start_tls_accept();
    void handle_tls_accept(std::shared_ptr<PlayerConnection> connection,
                          const asio::error_code& error);

    // Internal helper - assumes connections_mutex_ is already locked
    std::shared_ptr<PlayerConnection> find_connection_by_player_name_unlocked(std::string_view player_name) const;

    asio::io_context& io_context_;
    const ServerConfig& config_;
    std::atomic<bool> running_{false};
    WorldServer* world_server_{nullptr};

    // Asio networking
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<asio::ip::tcp::acceptor> tls_acceptor_;

    // TLS support
    std::unique_ptr<TLSContextManager> tls_context_manager_;

    // Connection management
    mutable std::mutex connections_mutex_;
    std::vector<std::shared_ptr<PlayerConnection>> connections_;
};

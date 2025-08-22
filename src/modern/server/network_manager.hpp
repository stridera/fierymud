/***************************************************************************
 *   File: src/server/network_manager.hpp                 Part of FieryMUD *
 *  Usage: Modern network management for player connections                *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "../core/result.hpp"
#include "player_connection.hpp"
#include <asio.hpp>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

// Forward declarations
struct ServerConfig;
class Player;
class WorldServer;

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
    
    // Connection management
    void cleanup_disconnected_connections();
    
private:
    void start_accept();
    void handle_accept(std::shared_ptr<PlayerConnection> connection, 
                      const asio::error_code& error);
    
    asio::io_context& io_context_;
    const ServerConfig& config_;
    std::atomic<bool> running_{false};
    WorldServer* world_server_{nullptr};
    
    // Asio networking
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    
    // Connection management
    mutable std::mutex connections_mutex_;
    std::vector<std::shared_ptr<PlayerConnection>> connections_;
};
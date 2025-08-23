/***************************************************************************
 *   File: src/server/player_connection.hpp               Part of FieryMUD *
 *  Usage: Player connection management                                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "../core/result.hpp"
#include <asio.hpp>
#include <memory>
#include <string>
#include <atomic>
#include <array>

// Forward declarations
class Player;
class WorldServer;
class GMCPHandler;

/** Connection states for login and character creation flow */
enum class ConnectionState {
    WaitingForName,     // Waiting for player to enter their name
    WaitingForPassword, // Waiting for password (future)
    CharacterCreation,  // In character creation process (future)
    Playing             // In game, processing commands normally
};

/**
 * Player Connection - Manages individual player network connections with Asio
 */
class PlayerConnection : public std::enable_shared_from_this<PlayerConnection> {
public:
    explicit PlayerConnection(asio::io_context& io_context);
    ~PlayerConnection();
    
    // Socket access
    asio::ip::tcp::socket& socket() { return socket_; }
    
    // Connection management
    virtual Result<void> initialize();
    virtual void close();
    bool is_connected() const;
    
    // Data transmission
    virtual Result<void> send(std::string_view message);
    virtual Result<void> send_prompt(std::string_view prompt);
    virtual void start_read();
    
    // Player association
    void set_player(std::shared_ptr<Player> player);
    std::shared_ptr<Player> get_player() const;
    
    // World server for command processing
    void set_world_server(WorldServer* world_server) { world_server_ = world_server; }
    
    // GMCP support
    GMCPHandler& gmcp_handler() { return *gmcp_handler_; }
    
    // Connection state management
    ConnectionState state() const { return state_; }
    void set_state(ConnectionState new_state) { state_ = new_state; }
    
private:
    void handle_read(const asio::error_code& error, size_t bytes_transferred);
    void handle_write(const asio::error_code& error, size_t bytes_transferred);
    void do_write();
    void process_input();
    void process_telnet_data(std::string_view data);
    void handle_input_by_state(const std::string& input);
    
    asio::ip::tcp::socket socket_;
    std::atomic<bool> connected_{false};
    
    // Read buffer
    static constexpr size_t buffer_size = 1024;
    std::array<char, buffer_size> read_buffer_;
    std::string input_buffer_;
    
    // Write buffer for async operations
    std::vector<std::string> write_queue_;
    std::atomic<bool> writing_{false};
    
    // Player association
    std::weak_ptr<Player> player_;
    protected:
    WorldServer* world_server_{nullptr};
    
    // GMCP support
    std::unique_ptr<GMCPHandler> gmcp_handler_;
    
    // Connection state for login/character creation flow
    protected:
    ConnectionState state_{ConnectionState::WaitingForName};
    std::string pending_character_name_;
};
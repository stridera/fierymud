#include "network_manager.hpp"

#include "../core/logging.hpp"
#include "../net/player_connection.hpp"
#include "mud_server.hpp"
#include "world_server.hpp"

#include <algorithm>

NetworkManager::NetworkManager(asio::io_context &io_context, const ServerConfig &config)
    : io_context_(io_context), config_(config) {}

NetworkManager::~NetworkManager() { stop(); }

Result<void> NetworkManager::initialize() {
    Log::info("Initializing NetworkManager with Asio on port {}", config_.port);

    try {
        // Create TCP acceptor
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), config_.port);
        acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io_context_, endpoint);

        // Set socket options
        acceptor_->set_option(asio::ip::tcp::acceptor::reuse_address(true));

        Log::info("Plain TCP acceptor initialized on port {}", config_.port);
        
        // Initialize TLS if enabled
        if (config_.enable_tls) {
            Log::info("Initializing TLS support on port {}", config_.tls_port);
            
            // Create TLS context manager
            tls_context_manager_ = std::make_unique<TLSContextManager>(config_);
            auto tls_init_result = tls_context_manager_->initialize();
            if (!tls_init_result) {
                Log::warn("TLS initialization failed: {}", tls_init_result.error().message);
                Log::info("Continuing without TLS support");
                tls_context_manager_.reset();
            } else {
                // Create TLS acceptor
                asio::ip::tcp::endpoint tls_endpoint(asio::ip::tcp::v4(), config_.tls_port);
                tls_acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io_context_, tls_endpoint);
                tls_acceptor_->set_option(asio::ip::tcp::acceptor::reuse_address(true));
                
                Log::info("TLS acceptor initialized on port {}", config_.tls_port);
            }
        }

        Log::info("NetworkManager initialized successfully");
        return Success();

    } catch (const std::exception &e) {
        return std::unexpected(
            Error(ErrorCode::NetworkError, fmt::format("Failed to initialize acceptor: {}", e.what())));
    }
}

Result<void> NetworkManager::start() {
    if (!acceptor_) {
        return std::unexpected(Errors::InvalidState("NetworkManager not initialized"));
    }

    running_.store(true);

    try {
        // Start accepting connections
        start_accept();
        
        // Start TLS accepting if enabled
        if (tls_acceptor_ && tls_context_manager_) {
            start_tls_accept();
            Log::info("NetworkManager started - accepting connections on port {} (plain) and {} (TLS)", 
                      config_.port, config_.tls_port);
        } else {
            Log::info("NetworkManager started - accepting connections on port {} (plain only)", config_.port);
        }

        return Success();

    } catch (const std::exception &e) {
        running_.store(false);
        return std::unexpected(Error(ErrorCode::NetworkError, fmt::format("Failed to start NetworkManager: {}", e.what())));
    }
}

void NetworkManager::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);
    Log::info("Stopping NetworkManager...");

    // Stop accepting new connections
    if (acceptor_) {
        acceptor_->close();
    }
    if (tls_acceptor_) {
        tls_acceptor_->close();
    }

    // Note: io_context is managed by ModernMUDServer

    // Close all existing connections safely
    std::vector<std::shared_ptr<PlayerConnection>> connections_to_close;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_to_close = connections_;
        connections_.clear();  // Clear first to prevent callback deadlock
    }
    
    // Now disconnect all connections outside the mutex lock
    for (auto& connection : connections_to_close) {
        if (connection) {
            connection->force_disconnect();
        }
    }

    Log::info("NetworkManager stopped");
}

size_t NetworkManager::connection_count() const { return connections_.size(); }

std::vector<std::shared_ptr<Player>> NetworkManager::get_connected_players() const {
    std::vector<std::shared_ptr<Player>> players;
    std::lock_guard<std::mutex> lock(connections_mutex_);

    for (const auto &connection : connections_) {
        if (auto player = connection->get_player()) {
            players.push_back(player);
        }
    }
    return players;
}

// Removed cleanup_disconnected_connections() - linkdead connections should not be auto-cleaned
// Removed find_connection_by_player_name() - only internal unlocked version needed

std::shared_ptr<PlayerConnection> NetworkManager::find_connection_by_player_name_unlocked(std::string_view player_name) const {
    // NOTE: Assumes connections_mutex_ is already locked by caller
    for (const auto &connection : connections_) {
        if (auto player = connection->get_player()) {
            if (player->name() == player_name) {
                return connection;
            }
        }
    }
    return nullptr;
}

bool NetworkManager::handle_reconnection(std::shared_ptr<PlayerConnection> new_connection, std::string_view player_name) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Find existing connection for this player (mutex already locked)
    auto existing_connection = find_connection_by_player_name_unlocked(player_name);
    if (!existing_connection) {
        return false; // No existing connection found
    }

    // Get the player from the existing connection
    auto player = existing_connection->get_player();
    if (!player) {
        return false; // No player associated with existing connection
    }

    Log::info("Handling reconnection for player '{}' - transferring from old connection to new", player_name);

    // Update WorldServer's connection tracking BEFORE transferring
    // This atomically removes old connection and adds new connection to connection_actors_
    if (world_server_) {
        world_server_->handle_player_reconnection(existing_connection, new_connection, player);
    }

    // Transfer player to new connection
    new_connection->attach_player(player);

    // Mark old connection as being replaced (prevents it from trying to clean up WorldServer)
    existing_connection->mark_as_replaced();

    // Disconnect and remove the old connection from NetworkManager
    existing_connection->disconnect("Reconnected from another location");
    connections_.erase(
        std::remove(connections_.begin(), connections_.end(), existing_connection),
        connections_.end()
    );

    // Add the new connection to NetworkManager's list
    connections_.push_back(new_connection);

    Log::info("Successfully transferred player '{}' to new connection", player_name);
    return true;
}

// Private Asio methods

void NetworkManager::start_accept() {
    // Create a new connection for this accept operation using the factory method
    // Convert raw pointer to shared_ptr if world_server_ exists
    std::shared_ptr<WorldServer> world_server_shared;
    if (world_server_) {
        // For now, we need to handle this differently since NetworkManager has a raw pointer
        // but PlayerConnection expects a shared_ptr. We'll create the connection without
        // the world server and set it later.
        world_server_shared = std::shared_ptr<WorldServer>(world_server_, [](WorldServer *) {
            // Empty deleter since we don't own the WorldServer
        });
    }

    auto new_connection = PlayerConnection::create(io_context_, world_server_shared, this);

    // Accept the next connection
    acceptor_->async_accept(new_connection->socket(), [this, new_connection](const asio::error_code &error) {
        handle_accept(new_connection, error);
    });
}

void NetworkManager::handle_accept(std::shared_ptr<PlayerConnection> connection, const asio::error_code &error) {
    if (!running_.load()) {
        return;
    }

    if (!error) {
        Log::info("New connection accepted from {}", connection->socket().remote_endpoint().address().to_string());

        // Add to connection list
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.push_back(connection);
        }

        // Start the connection (which will initialize login system)
        connection->start();

        Log::info("Connection initialized successfully. Total connections: {}", connection_count());
    } else if (error != asio::error::operation_aborted) {
        Log::error("Accept error: {}", error.message());
    }

    // Continue accepting new connections
    if (running_.load()) {
        start_accept();
    }
}

void NetworkManager::start_tls_accept() {
    if (!tls_acceptor_ || !tls_context_manager_) {
        return;
    }
    
    // Create a new TLS connection for this accept operation
    std::shared_ptr<WorldServer> world_server_shared;
    if (world_server_) {
        world_server_shared = std::shared_ptr<WorldServer>(world_server_, [](WorldServer *) {
            // Empty deleter since we don't own the WorldServer
        });
    }

    auto new_connection = PlayerConnection::create_tls(io_context_, world_server_shared, this, *tls_context_manager_);

    // Accept the next TLS connection
    tls_acceptor_->async_accept(new_connection->socket(), [this, new_connection](const asio::error_code &error) {
        handle_tls_accept(new_connection, error);
    });
}

void NetworkManager::handle_tls_accept(std::shared_ptr<PlayerConnection> connection, const asio::error_code &error) {
    if (!running_.load()) {
        return;
    }

    if (!error) {
        Log::info("New TLS connection accepted from {}", connection->socket().remote_endpoint().address().to_string());

        // Add to connection list
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.push_back(connection);
        }

        // Start the TLS connection (which will handle TLS handshake)
        connection->start();

        Log::info("TLS connection initialized successfully. Total connections: {}", connection_count());
    } else if (error != asio::error::operation_aborted) {
        Log::error("TLS accept error: {}", error.message());
    }

    // Continue accepting new TLS connections
    if (running_.load()) {
        start_tls_accept();
    }
}
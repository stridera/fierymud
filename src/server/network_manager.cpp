/***************************************************************************
 *   File: src/server/network_manager.cpp                 Part of FieryMUD *
 *  Usage: Modern network management with Asio                            *
 ***************************************************************************/

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

        Log::info("NetworkManager initialized successfully on port {}", config_.port);
        return Success();

    } catch (const std::exception &e) {
        return std::unexpected(
            Errors::SystemError(fmt::format("Failed to initialize acceptor on port {}: {}", config_.port, e.what())));
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

        Log::info("NetworkManager started, accepting connections on port {}", config_.port);
        return Success();

    } catch (const std::exception &e) {
        running_.store(false);
        return std::unexpected(Errors::SystemError(fmt::format("Failed to start NetworkManager: {}", e.what())));
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

    // Note: io_context is managed by ModernMUDServer

    // Close all existing connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
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

void NetworkManager::cleanup_disconnected_connections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(),
                       [](const std::shared_ptr<PlayerConnection> &conn) { return !conn->is_connected(); }),
        connections_.end());
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

    auto new_connection = PlayerConnection::create(io_context_, world_server_shared);

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
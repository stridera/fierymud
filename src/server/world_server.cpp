/***************************************************************************
 *   File: src/server/world_server.cpp                    Part of FieryMUD *
 *  Usage: Strand-based world server implementation                       *
 ***************************************************************************/

#include "world_server.hpp"

#include "../commands/builtin_commands.hpp"
#include "../commands/command_system.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../core/result.hpp"
#include "../net/player_connection.hpp"
#include "../world/room.hpp"
#include "../world/world_manager.hpp"
#include "mud_server.hpp"
#include "networked_actor.hpp"

#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>

WorldServer::WorldServer(asio::io_context &io_context, const ServerConfig &config)
    : io_context_(io_context), world_strand_(asio::make_strand(io_context)), config_(config),
      start_time_(std::chrono::steady_clock::now()) {

    Log::info("WorldServer created with strand-based architecture");
}

WorldServer::~WorldServer() { stop(); }

Result<void> WorldServer::initialize(bool is_test_mode) {
    Log::info("Initializing WorldServer...");

    // Initialize game systems (singletons)
    world_manager_ = &WorldManager::instance();
    command_system_ = &CommandSystem::instance();

    // Initialize systems (don't load world data yet - do that async)
    auto world_init = world_manager_->initialize(config_.world_directory, false);
    if (!world_init) {
        return std::unexpected(world_init.error());
    }

    auto command_init = command_system_->initialize();
    if (!command_init) {
        return std::unexpected(command_init.error());
    }

    // Register all built-in commands
    auto register_result = BuiltinCommands::register_all_commands();
    if (!register_result) {
        return std::unexpected(register_result.error());
    }

    // World loading will be done asynchronously on the world strand
    Log::info("WorldServer initialized successfully (world loading deferred to strand)");
    return Success();
}

Result<void> WorldServer::start() {
    if (running_.load()) {
        return std::unexpected(Errors::InvalidState("WorldServer already running"));
    }

    running_.store(true);

    // Load world data on the world strand
    asio::post(world_strand_, [this]() {
        auto world_load = world_manager_->load_world();
        if (!world_load) {
            Log::error("Failed to load world data: {}", world_load.error().message);
            return;
        }
        Log::info("World data loaded successfully on world strand");
        world_loaded_promise_.set_value();
    });

    // Schedule periodic operations on the world strand
    schedule_periodic_cleanup();
    schedule_heartbeat();

    Log::info("WorldServer started with strand-based execution");
    return Success();
}

void WorldServer::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);
    Log::info("Stopping WorldServer...");

    // Cancel all timers
    if (save_timer_)
        save_timer_->cancel();
    if (cleanup_timer_)
        cleanup_timer_->cancel();
    if (heartbeat_timer_)
        heartbeat_timer_->cancel();

    // Disconnect all players
    asio::post(world_strand_, [this]() {
        for (auto &connection : active_connections_) {
            connection->send_message("Server is shutting down. Goodbye!\r\n");
            connection->disconnect();
        }
        active_connections_.clear();
        connection_actors_.clear(); // Clean up actors
    });

    Log::info("WorldServer stopped");
}

std::future<void> WorldServer::get_world_loaded_future() { return world_loaded_promise_.get_future(); }

// Command Processing

void WorldServer::process_command(CommandRequest command) {
    commands_processed_.fetch_add(1);

    // Post to world strand for serialized execution
    asio::post(world_strand_, [this, cmd = std::move(command)]() { handle_command(std::move(cmd)); });
}

void WorldServer::process_command(std::shared_ptr<PlayerConnection> connection, std::string_view command_text) {
    if (!running_.load()) {
        return;
    }

    // Create command request
    CommandRequest cmd;
    cmd.command = std::string(command_text);
    cmd.timestamp = std::chrono::steady_clock::now();

    // For now, we don't have player IDs yet, so use connection-based processing
    asio::post(world_strand_, [this, connection, cmd = std::move(cmd)]() {
        // Process the command and send response
        if (!cmd.command.empty()) {
            Log::debug("Processing command '{}' on world strand", cmd.command);

            // Use the full CommandSystem for processing
            std::string response;

            // Handle system commands that bypass normal command processing
            if (cmd.command == "quit" || cmd.command == "q") {
                response = "Goodbye!\r\n";
                connection->send_message(response);
                connection->disconnect();
                return;
            }

            // Get the persistent actor for this connection
            auto actor_it = connection_actors_.find(connection);
            if (actor_it == connection_actors_.end()) {
                // Connection doesn't have an associated actor (shouldn't happen)
                response = "Error: No character associated with connection.\r\n";
                connection->send_message(response);
                return;
            }

            auto actor = actor_it->second;

            // Use CommandSystem to execute the command
            auto result = command_system_->execute_command(actor, cmd.command);
            if (!result) {
                response = fmt::format("Error executing command: {}\r\n", result.error().message);
                connection->send_message(response);
            } else {
                // Response will be sent via the CommandContext/connection mechanism
                // No need to send additional response here
            }
        }
    });
}

void WorldServer::process_command(std::shared_ptr<Actor> actor, std::string_view command_text) {
    if (!running_.load()) {
        return;
    }

    Log::debug("WorldServer::process_command (Actor): Received command '{}' for actor '{}'", command_text,
               actor->name());

    // Create command request
    CommandRequest cmd;
    cmd.command = std::string(command_text);
    cmd.timestamp = std::chrono::steady_clock::now();

    asio::post(world_strand_, [this, actor, cmd = std::move(cmd)]() {
        Log::debug("WorldServer::process_command (Actor) - posted lambda: Processing command '{}' for actor '{}'",
                   cmd.command, actor->name());
        // Process the command and send response
        if (!cmd.command.empty()) {
            Log::debug("Processing command '{}' on world strand", cmd.command);

            // Use CommandSystem to execute the command
            auto result = command_system_->execute_command(actor, cmd.command);
            if (!result) {
                actor->send_message(fmt::format("Error executing command: {}\r\n", result.error().message));
            } else {
                // Response will be sent via the CommandContext/connection mechanism
                // No need to send additional response here
            }
        }
    });
}

// Player Management

void WorldServer::add_player_connection(std::shared_ptr<PlayerConnection> connection) {
    asio::post(world_strand_, [this, connection]() { handle_player_connection(connection); });
}

void WorldServer::remove_player_connection(std::shared_ptr<PlayerConnection> connection) {
    asio::post(world_strand_, [this, connection]() { handle_player_disconnection(connection); });
}

void WorldServer::set_actor_for_connection(std::shared_ptr<PlayerConnection> connection, std::shared_ptr<Actor> actor) {
    asio::post(world_strand_, [this, connection, actor]() { connection_actors_[connection] = actor; });
}

void WorldServer::add_player(std::shared_ptr<Player> player) {
    // Players are managed through their connections
    // This method can be used for additional player-specific logic if needed
    Log::debug("Player added to world server: {}", player->name());
}

void WorldServer::remove_player(std::shared_ptr<Player> player) {
    // Players are managed through their connections
    // This method can be used for additional player-specific cleanup if needed
    Log::debug("Player removed from world server: {}", player->name());
}

CommandSystem *WorldServer::get_command_system() const { return command_system_; }

// Private strand-based operations

void WorldServer::handle_command(CommandRequest command) {
    // This runs on the world strand - no synchronization needed!

    Log::debug("Handling command: {}", command.command);

    // TODO: Integrate with full CommandSystem
    // For now, basic command handling is done in process_command
}

void WorldServer::handle_player_connection(std::shared_ptr<PlayerConnection> connection) {
    // This runs on the world strand - thread safe!

    active_connections_.push_back(connection);
    total_connections_.fetch_add(1);

    // Create a persistent player for this connection
    auto player = std::make_shared<NetworkedPlayer>(connection, "Guest");
    player->initialize(); // Place in starting room
    connection_actors_[connection] = player;

    Log::info("Player connected. Active connections: {}", active_connections_.size());

    // Send additional welcome information
    connection->send_message("Type 'help' for available commands.\r\n");

    // Send initial room info via GMCP (if client supports it)
    send_room_info_to_player(connection);
}

void WorldServer::handle_player_disconnection(std::shared_ptr<PlayerConnection> connection) {
    // This runs on the world strand - thread safe!

    auto it = std::find(active_connections_.begin(), active_connections_.end(), connection);
    if (it != active_connections_.end()) {
        active_connections_.erase(it);
        Log::info("Player disconnected. Active connections: {}", active_connections_.size());
    }

    // Clean up the associated actor
    auto actor_it = connection_actors_.find(connection);
    if (actor_it != connection_actors_.end()) {
        // Actor destructor will handle room cleanup
        connection_actors_.erase(actor_it);
    }
}

// Periodic Operations

void WorldServer::schedule_periodic_cleanup() {
    cleanup_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(std::chrono::minutes(5), [this]() { perform_cleanup(); }, cleanup_timer_);
}

void WorldServer::schedule_heartbeat() {
    heartbeat_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(std::chrono::seconds(30), [this]() { perform_heartbeat(); }, heartbeat_timer_);
}

void WorldServer::schedule_timer(std::chrono::milliseconds interval, std::function<void()> task,
                                 std::shared_ptr<asio::steady_timer> &timer) {
    if (!running_.load()) {
        return;
    }

    timer->expires_after(interval);
    timer->async_wait([this, task, interval, timer](const asio::error_code &ec) {
        if (!ec && running_.load()) {
            // Execute task on world strand
            task();
            // Reschedule
            schedule_timer(interval, task, const_cast<std::shared_ptr<asio::steady_timer> &>(timer));
        }
    });
}

void WorldServer::perform_cleanup() {
    // This runs on the world strand - safe to modify world state!
    Log::debug("Performing periodic cleanup...");

    // Remove disconnected connections
    active_connections_.erase(
        std::remove_if(active_connections_.begin(), active_connections_.end(),
                       [](const std::shared_ptr<PlayerConnection> &conn) { return !conn->is_connected(); }),
        active_connections_.end());

    // TODO: Other cleanup tasks (expired objects, etc.)
}

void WorldServer::perform_heartbeat() {
    // This runs on the world strand - thread safe!

    auto uptime =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time_).count();

    Log::debug("WorldServer heartbeat - Uptime: {}s, Active connections: {}, Commands processed: {}", uptime,
               active_connections_.size(), commands_processed_.load());
}

// Statistics

size_t WorldServer::active_player_count() const { return active_connections_.size(); }

GameLoopStats WorldServer::get_performance_stats() const {
    GameLoopStats stats;
    stats.start_time = start_time_;
    stats.commands_processed = commands_processed_.load();
    // Note: GameLoopStats doesn't have total_connections field
    return stats;
}

Result<void> WorldServer::create_default_world() {
    // This must run on the world strand.
    asio::post(world_strand_, [this]() {
        Log::info("Creating default world...");

        // Create a simple starting room
        auto room_result = Room::create(EntityId{1000}, "The Starting Room", SectorType::Inside);
        if (!room_result) {
            Log::error("Failed to create starting room: {}", room_result.error().message);
            return;
        }

        auto start_room = std::shared_ptr<Room>(room_result.value().release());
        start_room->set_description(
            "This is a simple starting room for the modern MUD server. "
            "You can look around, say things, and explore the basic commands.");

        auto add_result = world_manager_->add_room(start_room);
        if (!add_result) {
            Log::error("Failed to add starting room to world manager: {}", add_result.error().message);
            return;
        }

        world_manager_->set_start_room(EntityId{1000});

        Log::info("Default world created with starting room [1000]");
    });
    return Success();
}

void WorldServer::send_room_info_to_player(std::shared_ptr<PlayerConnection> connection) {
    if (!world_manager_) {
        return;
    }

    // Try to get the player's current room if they have an actor
    std::shared_ptr<Room> room = nullptr;

    auto actor_it = connection_actors_.find(connection);
    if (actor_it != connection_actors_.end() && actor_it->second) {
        room = actor_it->second->current_room();
    }

    // If no room found, try to find a starting room (ID 100)
    if (!room) {
        room = world_manager_->get_room(EntityId{100}); // Test world starting room
    }

    if (room) {
        // Send Room.Info GMCP data
        if (connection->supports_gmcp()) {
            nlohmann::json room_data;
            room_data["num"] = room->id().value();
            room_data["name"] = room->name();
            room_data["zone"] = "test";          // TODO: Get actual zone name
            room_data["environment"] = "inside"; // TODO: Map sector types

            auto result = connection->send_gmcp("Room.Info", room_data);
            if (result) {
                Log::debug("Sent Room.Info GMCP for room {}", room->id().value());
            } else {
                Log::debug("Failed to send Room.Info GMCP: {}", result.error().message);
            }
        }
    } else {
        Log::debug("No room found to send GMCP info");
    }
}
#include "world_server.hpp"

#include "../commands/builtin_commands.hpp"
#include "../commands/command_system.hpp"
#include "../core/ability_executor.hpp"
#include "../core/actor.hpp"
#include "../game/composer_system.hpp"
#include "../core/combat.hpp"
#include "../core/logging.hpp"
#include "../core/result.hpp"
#include "../net/player_connection.hpp"
#include "../scripting/trigger_manager.hpp"
#include "../world/room.hpp"
#include "../world/world_manager.hpp"
#include "mud_server.hpp"
#include "networked_actor.hpp"
#include "persistence_manager.hpp"

#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>

// World server constants
namespace {
    // Timer intervals
    constexpr auto CLEANUP_INTERVAL = std::chrono::minutes(5);
    constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(30);
    constexpr auto COMBAT_PROCESSING_INTERVAL = std::chrono::milliseconds(100);
    constexpr auto PLAYER_SAVE_INTERVAL = std::chrono::minutes(5);

    // Game time defaults
    constexpr int DEFAULT_LORE_YEAR = 1000;
    constexpr int DEFAULT_START_HOUR = 12;  // Noon

    // Starting room IDs (zone_id, local_id composite keys)
    constexpr std::uint32_t DEFAULT_START_ZONE_ID = 30;
    constexpr std::uint32_t DEFAULT_START_LOCAL_ID = 1;   // Forest Temple of Mielikki
    constexpr std::uint32_t TEST_START_ZONE_ID = 1;
    constexpr std::uint32_t TEST_START_LOCAL_ID = 0;      // Test world starting room

    // Combat condition thresholds (HP percentage)
    constexpr int CONDITION_PERFECT = 95;
    constexpr int CONDITION_SCRATCHED = 85;
    constexpr int CONDITION_HURT = 70;
    constexpr int CONDITION_WOUNDED = 50;
    constexpr int CONDITION_BLEEDING = 30;
    constexpr int CONDITION_CRITICAL = 15;

    // Percentage calculation base
    constexpr int PERCENT_MULTIPLIER = 100;
}

// Static instance pointer for singleton access
static WorldServer* g_world_server_instance = nullptr;

WorldServer::WorldServer(asio::io_context &io_context, const ServerConfig &config)
    : io_context_(io_context), world_strand_(asio::make_strand(io_context)), config_(config),
      start_time_(std::chrono::steady_clock::now()) {

    Log::info("WorldServer created with strand-based architecture");
    g_world_server_instance = this;
}

WorldServer::~WorldServer() { 
    g_world_server_instance = nullptr;
    stop(); 
}

WorldServer* WorldServer::instance() {
    return g_world_server_instance;
}

Result<void> WorldServer::initialize(bool /* is_test_mode */) {
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

    // Initialize ability cache (spells/skills database)
    auto ability_init = FieryMUD::AbilityCache::instance().initialize();
    if (!ability_init) {
        Log::warn("Failed to initialize ability cache: {}", ability_init.error().message);
        // Non-fatal - abilities will be looked up on-demand
    }

    // Initialize time system with default starting time
    GameTime start_time;
    start_time.hour = DEFAULT_START_HOUR;
    start_time.day = 0;
    start_time.month = Month::Deepwinter;
    start_time.year = DEFAULT_LORE_YEAR;

    auto time_init = TimeSystem::instance().initialize(start_time);
    if (!time_init) {
        return std::unexpected(time_init.error());
    }

    // Register callbacks for time events
    TimeSystem::instance().on_sunlight_changed([this](SunlightState /* old_state */, SunlightState new_state, Hemisphere /* hemisphere */) {
        // Broadcast sunlight changes to outdoor players
        std::string message;

        switch (new_state) {
            case SunlightState::Rise:
                message = "\x1B[33mThe sun rises in the east, painting the sky with brilliant colors.\x1B[0m\r\n";
                break;
            case SunlightState::Light:
                message = "\x1B[33mThe bright sun shines warmly upon the land.\x1B[0m\r\n";
                break;
            case SunlightState::Set:
                message = "\x1B[35mThe sun slowly sinks below the western horizon.\x1B[0m\r\n";
                break;
            case SunlightState::Dark:
                message = "\x1B[34mThe night falls, shrouding the land in darkness.\x1B[0m\r\n";
                break;
        }

        // Send message to all outdoor players in this hemisphere
        // Post to world strand to ensure thread safety
        asio::post(world_strand_, [this, message]() {
            for (auto& conn : active_connections_) {
                if (auto player = conn->get_player()) {
                    // TODO: Check if player is outdoor and in correct hemisphere
                    // For now, send to all connected players
                    conn->send_message(message);
                }
            }
        });
    });

    // Register callback for month changes to update season
    TimeSystem::instance().on_month_changed([this](const GameTime& /* old_time */, const GameTime& new_time) {
        Season new_season = new_time.get_season();
        WeatherSystem::instance().set_season(new_season);

        // Broadcast season change to all players
        std::string season_message;
        switch (new_season) {
            case Season::Spring:
                season_message = "\x1B[32mSpring has arrived! The world awakens with new life and growth.\x1B[0m\r\n";
                break;
            case Season::Summer:
                season_message = "\x1B[33mSummer begins! The days grow long and warm under the bright sun.\x1B[0m\r\n";
                break;
            case Season::Autumn:
                season_message = "\x1B[31mAutumn descends! Leaves turn golden and the air grows crisp.\x1B[0m\r\n";
                break;
            case Season::Winter:
                season_message = "\x1B[36mWinter arrives! The cold settles in and frost covers the land.\x1B[0m\r\n";
                break;
        }

        asio::post(world_strand_, [this, season_message]() {
            for (auto& conn : active_connections_) {
                if (auto player = conn->get_player()) {
                    conn->send_message(season_message);
                }
            }
        });
    });

    // Register callback for hour changes to tick spell effects
    TimeSystem::instance().on_hour_changed([this](const GameTime& /* old_time */, const GameTime& /* new_time */) {
        // Tick down spell effect durations for all actors in the world
        asio::post(world_strand_, [this]() {
            world_manager_->tick_all_effects();
        });
    });

    // Register callback for hour changes to fire TIME triggers
    TimeSystem::instance().on_hour_changed([this](const GameTime& /* old_time */, const GameTime& new_time) {
        // Fire TIME triggers for all mobs in the world
        asio::post(world_strand_, [this, hour = new_time.hour]() {
            auto& trigger_mgr = FieryMUD::TriggerManager::instance();
            if (!trigger_mgr.is_initialized()) {
                return;
            }
            // Process TIME triggers for all loaded mobs
            world_manager_->for_each_mobile([&trigger_mgr, hour](std::shared_ptr<Mobile> mob) {
                trigger_mgr.dispatch_time(mob, hour);
            });
        });
    });

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
            // Database loading failed - this is a critical error, not recoverable
            Log::error("FATAL: Failed to load world data from database: {}", world_load.error().message);
            Log::error("The server cannot start without a valid database connection.");
            Log::error("Please check your database configuration and ensure PostgreSQL is running.");

            // Signal failure via exception in the promise
            try {
                throw std::runtime_error(fmt::format(
                    "Failed to load world data: {}", world_load.error().message));
            } catch (...) {
                world_loaded_promise_.set_exception(std::current_exception());
            }

            // Mark server as not running to trigger shutdown
            running_.store(false);
            return;
        }

        Log::info("World data loaded successfully on world strand");
        world_manager_->set_start_room(EntityId{DEFAULT_START_ZONE_ID, DEFAULT_START_LOCAL_ID});
        Log::info("Default starting room set to [{}] - The Forest Temple of Mielikki",
                 EntityId{DEFAULT_START_ZONE_ID, DEFAULT_START_LOCAL_ID});
        world_loaded_promise_.set_value();
    });

    // Schedule periodic operations on the world strand
    schedule_periodic_cleanup();
    schedule_heartbeat();
    schedule_combat_processing();
    schedule_periodic_save();

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
    if (combat_timer_)
        combat_timer_->cancel();

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

            // Check if player is in composer mode - route input to composer instead
            if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
                if (player->is_composing()) {
                    auto composer = player->get_composer();
                    if (composer) {
                        composer->process_input(cmd.command);
                        // If composer finished, clean up
                        if (!composer->is_active()) {
                            player->stop_composing();
                        }
                        return;
                    }
                }
            }

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

            // Check if player is in composer mode - route input to composer instead
            if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
                if (player->is_composing()) {
                    auto composer = player->get_composer();
                    if (composer) {
                        composer->process_input(cmd.command);
                        // If composer finished, clean up and send normal prompt
                        if (!composer->is_active()) {
                            player->stop_composing();
                            send_prompt_to_actor(actor);
                        }
                        // No prompt while composing - composer sends its own
                        return;
                    }
                }
            }

            // Use CommandSystem to execute the command
            auto result = command_system_->execute_command(actor, cmd.command);
            if (!result) {
                actor->send_message(fmt::format("Error executing command: {}\r\n", result.error().message));
            } else {
                // Response will be sent via the CommandContext/connection mechanism
                // No need to send additional response here
            }

            // Send prompt after command execution (unless player entered composer mode)
            if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
                if (player->is_composing()) {
                    return;  // Don't send game prompt while composing
                }
            }
            send_prompt_to_actor(actor);
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

void WorldServer::handle_player_reconnection(std::shared_ptr<PlayerConnection> old_connection,
                                             std::shared_ptr<PlayerConnection> new_connection,
                                             std::shared_ptr<Player> player) {
    asio::post(world_strand_, [this, old_connection, new_connection, player]() {
        // Remove old connection's entry from connection_actors_
        auto old_it = connection_actors_.find(old_connection);
        if (old_it != connection_actors_.end()) {
            Log::debug("Removing old connection from connection_actors_ for player: {}", player->name());
            connection_actors_.erase(old_it);
        }

        // Remove old connection from active_connections_
        auto active_it = std::find(active_connections_.begin(), active_connections_.end(), old_connection);
        if (active_it != active_connections_.end()) {
            active_connections_.erase(active_it);
        }

        // Add new connection with the player as its actor
        connection_actors_[new_connection] = player;
        active_connections_.push_back(new_connection);

        Log::info("Player '{}' reconnection: transferred from old to new connection in WorldServer",
                  player->name());
    });
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

std::vector<std::shared_ptr<Player>> WorldServer::get_online_players() const {
    // Extract Player objects from active connections
    std::vector<std::shared_ptr<Player>> players;
    
    // Note: This accesses connection_actors_ which should be thread-safe for const access
    // since we're only reading the map, not modifying it
    for (const auto& [connection, actor] : connection_actors_) {
        if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
            players.push_back(player);
        }
    }
    
    return players;
}

std::vector<std::shared_ptr<Actor>> WorldServer::get_online_actors() const {
    // Extract all Actor objects from active connections
    std::vector<std::shared_ptr<Actor>> actors;
    
    // Note: This accesses connection_actors_ which should be thread-safe for const access
    // since we're only reading the map, not modifying it
    for (const auto& [connection, actor] : connection_actors_) {
        if (actor) {
            actors.push_back(actor);
        }
    }
    
    return actors;
}

// Private strand-based operations

void WorldServer::handle_command(CommandRequest command) {
    // This runs on the world strand - no synchronization needed!

    Log::debug("Handling command: {}", command.command);

    // Full CommandSystem integration is pending player registry implementation
    // For now, just log the command - the existing process_command methods
    // in WorldManager will handle the actual command execution
    
    if (command.player_id != INVALID_ENTITY_ID) {
        std::string input = command.command;
        if (!command.args.empty()) {
            input += " " + command.args;
        }
        Log::debug("Command received: '{}' from player {}", input, command.player_id);
    } else {
        Log::warn("Received command '{}' without valid player ID", command.command);
    }
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
    schedule_timer(CLEANUP_INTERVAL, [this]() { perform_cleanup(); }, cleanup_timer_);
}

void WorldServer::schedule_heartbeat() {
    heartbeat_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(HEARTBEAT_INTERVAL, [this]() { perform_heartbeat(); }, heartbeat_timer_);
}

void WorldServer::schedule_combat_processing() {
    Log::info("Scheduling combat processing timer ({}ms interval)", COMBAT_PROCESSING_INTERVAL.count());
    combat_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(COMBAT_PROCESSING_INTERVAL, [this]() { perform_combat_processing(); }, combat_timer_);
}

void WorldServer::schedule_periodic_save() {
    Log::info("Scheduling periodic player save timer ({}min interval)",
              std::chrono::duration_cast<std::chrono::minutes>(PLAYER_SAVE_INTERVAL).count());
    save_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(PLAYER_SAVE_INTERVAL, [this]() { perform_player_save(); }, save_timer_);
}

void WorldServer::schedule_timer(std::chrono::milliseconds interval, std::function<void()> task,
                                 const std::shared_ptr<asio::steady_timer> &timer) {
    if (!running_.load()) {
        return;
    }

    timer->expires_after(interval);
    timer->async_wait([this, task, interval, timer](const asio::error_code &ec) {
        if (!ec && running_.load()) {
            // Execute task on world strand
            asio::post(world_strand_, [this, task, interval, timer]() {
                task();
                // Reschedule
                schedule_timer(interval, task, timer);
            });
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

    // Additional cleanup tasks
    
    // Refresh spell slots for all connected players
    for (auto& conn : active_connections_) {
        if (auto player = conn->get_player()) {
            player->update_spell_slots();
        }
    }
    
    // Additional world updates can be added here as needed
    Log::trace("Cleanup tasks completed");
}

void WorldServer::perform_heartbeat() {
    // This runs on the world strand - thread safe!

    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

    // Update game time system (30 seconds real time since last heartbeat)
    static auto last_heartbeat_time = now;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat_time);
    TimeSystem::instance().update(elapsed);
    last_heartbeat_time = now;

    // Log current game time
    auto& time = TimeSystem::instance().current_time();
    Log::trace("WorldServer heartbeat - Uptime: {}s, Active connections: {}, Commands processed: {}, Game time: {} of {}, Year {}",
               uptime,
               active_connections_.size(),
               commands_processed_.load(),
               time.hour,
               time.month_name(),
               time.year);
}

void WorldServer::perform_combat_processing() {
    // This runs on the world strand - thread safe!
    // Process all active combat rounds
    bool rounds_processed = FieryMUD::CombatManager::process_combat_rounds();

    // Only send prompts if actual combat rounds were processed
    if (rounds_processed) {
        auto fighting_actors = FieryMUD::CombatManager::get_all_fighting_actors();
        for (auto &actor : fighting_actors) {
            if (actor && actor->is_alive()) {
                send_prompt_to_actor(actor);
            }
        }
    }

    // Process queued spells for players whose blackout has expired
    for (auto& conn : active_connections_) {
        auto it = connection_actors_.find(conn);
        if (it == connection_actors_.end() || !it->second) {
            continue;
        }

        auto& actor = it->second;

        // Check if actor has a queued spell and blackout has expired
        if (actor->has_queued_spell() && !actor->is_casting_blocked()) {
            auto queued = actor->pop_queued_spell();
            if (queued) {
                // Execute the queued spell command
                actor->send_message("Your queued spell is ready to cast!\n");

                // Build and execute the cast command with the queued spell args
                std::string cast_command = "cast " + queued->spell_name;
                process_command(actor, cast_command);

                Log::debug("Processed queued spell for {}: {}", actor->name(), queued->spell_name);
            }
        }
    }
}

void WorldServer::perform_player_save() {
    // This runs on the world strand - thread safe!
    // Periodically save all online players to persist their state
    int saved_count = 0;
    int failed_count = 0;

    for (auto& conn : active_connections_) {
        if (auto player = conn->get_player()) {
            auto save_result = PersistenceManager::instance().save_player(*player);
            if (save_result) {
                saved_count++;
            } else {
                failed_count++;
                Log::warn("Failed to auto-save player {}: {}",
                         player->name(), save_result.error().message);
            }
        }
    }

    if (saved_count > 0 || failed_count > 0) {
        Log::debug("Periodic player save: {} saved, {} failed", saved_count, failed_count);
    }
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
        auto room_result = Room::create(EntityId{DEFAULT_START_ZONE_ID, DEFAULT_START_LOCAL_ID}, "The Forest Temple of Mielikki", SectorType::Inside);
        if (!room_result) {
            Log::error("Failed to create starting room: {}", room_result.error().message);
            return;
        }

        auto start_room = std::shared_ptr<Room>(room_result.value().release());
        start_room->set_description(
            "This is the Forest Temple of Mielikki, a peaceful sanctuary in the heart of nature. "
            "Ancient trees surround this sacred grove, and a gentle breeze carries the scent of wildflowers. "
            "You feel safe and welcome here.");

        auto add_result = world_manager_->add_room(start_room);
        if (!add_result) {
            Log::error("Failed to add starting room to world manager: {}", add_result.error().message);
            return;
        }

        world_manager_->set_start_room(EntityId{DEFAULT_START_ZONE_ID, DEFAULT_START_LOCAL_ID});

        Log::info("Default world created with starting room [{}] - The Forest Temple of Mielikki",
                 EntityId{DEFAULT_START_ZONE_ID, DEFAULT_START_LOCAL_ID});
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

    // If no room found, try to find a starting room
    if (!room) {
        room = world_manager_->get_room(EntityId{TEST_START_ZONE_ID, TEST_START_LOCAL_ID});
    }

    if (room) {
        // Send Room.Info GMCP data
        if (connection->supports_gmcp()) {
            nlohmann::json room_data;
            room_data["num"] = room->id().value();
            room_data["name"] = room->name();
            // Get zone name from room's zone ID
            std::string zone_name = "unknown";
            if (auto zone = world_manager_->get_zone(room->zone_id())) {
                zone_name = zone->name();
            }
            room_data["zone"] = zone_name;
            room_data["environment"] = RoomUtils::get_sector_name(room->sector_type());

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

void WorldServer::send_prompt_to_actor(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return;
    }

    // Get actor's current stats
    const auto &stats = actor->stats();

    // Basic prompt format: <HP>H <Move>M>
    std::string prompt = fmt::format("{}H {}M", stats.hit_points, stats.movement);

    // If fighting, show opponent's condition
    if (actor->is_fighting()) {
        auto opponent = FieryMUD::CombatManager::get_opponent(*actor);
        if (opponent) {
            // Calculate condition based on HP percentage
            const auto &opp_stats = opponent->stats();
            int hp_percent = (opp_stats.hit_points * PERCENT_MULTIPLIER) / std::max(1, opp_stats.max_hit_points);

            std::string condition;
            if (hp_percent >= CONDITION_PERFECT)
                condition = "perfect";
            else if (hp_percent >= CONDITION_SCRATCHED)
                condition = "scratched";
            else if (hp_percent >= CONDITION_HURT)
                condition = "hurt";
            else if (hp_percent >= CONDITION_WOUNDED)
                condition = "wounded";
            else if (hp_percent >= CONDITION_BLEEDING)
                condition = "bleeding";
            else if (hp_percent >= CONDITION_CRITICAL)
                condition = "critical";
            else
                condition = "dying";

            prompt += fmt::format(" (Fighting: {} is {})", opponent->display_name(), condition);
        } else {
            prompt += " (Fighting)";
        }
    }

    prompt += "> ";

    // Send the prompt - we need to handle different actor types
    if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
        // For players, try to get their output interface
        if (auto output = player->get_output()) {
            output->send_prompt(prompt);
        } else {
            // Fallback to send_message
            player->send_message(prompt);
        }
    } else {
        // For other actors, just send as message (though NPCs typically don't need prompts)
        actor->send_message(prompt);
    }
}
#include "world_server.hpp"

#include "commands/builtin_commands.hpp"
#include "commands/command_system.hpp"
#include "commands/movement_commands.hpp"
#include "core/ability_executor.hpp"
#include "core/actor.hpp"
#include "core/mobile.hpp"
#include "core/player.hpp"
#include "game/composer_system.hpp"
#include "core/combat.hpp"
#include "core/logging.hpp"
#include "core/result.hpp"
#include "net/player_connection.hpp"
#include "scripting/coroutine_scheduler.hpp"
#include "scripting/script_engine.hpp"
#include "scripting/trigger_manager.hpp"
#include "scripting/triggers/trigger_types.hpp"
#include "text/text_format.hpp"
#include "world/room.hpp"
#include "world/weather.hpp"
#include "world/world_manager.hpp"
#include "mud_server.hpp"
#include "persistence_manager.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>

// World server constants
namespace {
    // Timer intervals
    constexpr auto CLEANUP_INTERVAL = std::chrono::minutes(5);
    constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(30);
    constexpr auto COMBAT_PROCESSING_INTERVAL = std::chrono::milliseconds(100);
    constexpr auto PLAYER_SAVE_INTERVAL = std::chrono::minutes(5);
    constexpr auto SPELL_RESTORE_INTERVAL = std::chrono::seconds(1);
    constexpr auto CASTING_TICK_INTERVAL = std::chrono::milliseconds(500);  // 2 ticks per second
    constexpr auto REGEN_TICK_INTERVAL = std::chrono::seconds(4);  // HP/move regen like legacy
    constexpr auto MOB_ACTIVITY_INTERVAL = std::chrono::seconds(2);  // Mob AI tick (aggression, etc.)

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

// Thread-safe static instance pointer for singleton-like access
// Note: WorldServer requires constructor parameters, so this is a "single instance with static accessor"
// pattern rather than a true singleton. Only one WorldServer should exist at a time.
static std::atomic<WorldServer*> g_world_server_instance{nullptr};

WorldServer::WorldServer(asio::io_context &io_context, const ServerConfig &config)
    : io_context_(io_context), world_strand_(asio::make_strand(io_context)), config_(config),
      start_time_(std::chrono::steady_clock::now()) {

    // Ensure only one WorldServer instance exists at a time
    WorldServer* expected = nullptr;
    if (!g_world_server_instance.compare_exchange_strong(expected, this)) {
        Log::error("Attempted to create multiple WorldServer instances - this is not allowed");
        throw std::runtime_error("Multiple WorldServer instances not allowed");
    }

    Log::info("WorldServer created with strand-based architecture");
}

WorldServer::~WorldServer() {
    stop();
    g_world_server_instance.store(nullptr, std::memory_order_release);
}

WorldServer* WorldServer::instance() {
    return g_world_server_instance.load(std::memory_order_acquire);
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

    // Initialize Lua scripting engine
    auto& script_engine = FieryMUD::ScriptEngine::instance();
    if (!script_engine.initialize()) {
        Log::error("Failed to initialize ScriptEngine: {}", script_engine.last_error());
        return std::unexpected(Errors::InternalError("WorldServer::initialize", "ScriptEngine initialization failed"));
    }
    Log::info("ScriptEngine initialized successfully");

    // Initialize trigger manager (requires ScriptEngine)
    auto& trigger_mgr = FieryMUD::TriggerManager::instance();
    if (!trigger_mgr.initialize()) {
        Log::error("Failed to initialize TriggerManager: {}", trigger_mgr.last_error());
        return std::unexpected(Errors::InternalError("WorldServer::initialize", "TriggerManager initialization failed"));
    }
    Log::info("TriggerManager initialized successfully");

    // Initialize coroutine scheduler for wait() support in triggers
    FieryMUD::get_coroutine_scheduler().initialize(io_context_, world_strand_);

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
    TimeSystem::instance().on_sunlight_changed([this](SunlightState /* old_state */, SunlightState new_state, Hemisphere hemisphere) {
        // Only broadcast for the primary hemisphere (Northeast) to avoid duplicate messages
        // The 4 hemispheres have offset day/night cycles, so we pick one as canonical
        if (hemisphere != Hemisphere::Northeast) {
            return;
        }

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

        // Send message to all outdoor players
        // Post to world strand to ensure thread safety
        asio::post(world_strand_, [this, message]() {
            for (auto& conn : active_connections_) {
                if (auto player = conn->get_player()) {
                    if (auto room = player->current_room()) {
                        if (RoomUtils::is_outdoor_sector(room->sector_type())) {
                            conn->send_message(message);
                        }
                    }
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

    // Register callback for hour changes to tick spell durations and conditions
    TimeSystem::instance().on_hour_changed([this](const GameTime& /* old_time */, const GameTime& /* new_time */) {
        // Tick down spell effect durations, process drunk sobering, etc.
        asio::post(world_strand_, [this]() {
            world_manager_->tick_hour_all();
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
    schedule_spell_restoration();
    schedule_casting_processing();
    schedule_regen_tick();
    schedule_mob_activity();

    Log::info("WorldServer started with strand-based execution");
    return Success();
}

void WorldServer::begin_shutdown() {
    if (!running_.load()) {
        return;
    }

    Log::info("WorldServer: Beginning shutdown sequence (blocking new strand work)...");
    running_.store(false);

    // Cancel timers early so they don't post more work
    if (save_timer_)
        save_timer_->cancel();
    if (cleanup_timer_)
        cleanup_timer_->cancel();
    if (heartbeat_timer_)
        heartbeat_timer_->cancel();
    if (combat_timer_)
        combat_timer_->cancel();
    if (spell_restore_timer_)
        spell_restore_timer_->cancel();
    if (casting_timer_)
        casting_timer_->cancel();
}

void WorldServer::stop() {
    // If begin_shutdown() was already called, running_ is false
    // If not, we set it here (for direct stop() calls)
    if (running_.load()) {
        running_.store(false);
    }

    Log::info("Stopping WorldServer...");

    // Cancel all timers (may already be cancelled by begin_shutdown)
    if (save_timer_)
        save_timer_->cancel();
    if (cleanup_timer_)
        cleanup_timer_->cancel();
    if (heartbeat_timer_)
        heartbeat_timer_->cancel();
    if (combat_timer_)
        combat_timer_->cancel();
    if (spell_restore_timer_)
        spell_restore_timer_->cancel();
    if (casting_timer_)
        casting_timer_->cancel();

    // Shutdown scripting systems in proper order:
    // 1. CoroutineScheduler first (cancels all pending Lua coroutines and timers)
    // 2. TriggerManager second (clears trigger cache before Lua state is destroyed)
    // 3. ScriptEngine third (cleans up Lua state after coroutines are gone)
    Log::info("Shutting down scripting systems...");
    FieryMUD::get_coroutine_scheduler().shutdown();
    FieryMUD::TriggerManager::instance().shutdown();
    FieryMUD::ScriptEngine::instance().shutdown();
    Log::info("Scripting systems shutdown complete");

    // Disconnect players directly (don't use post during shutdown - io_context may be stopping)
    Log::info("Disconnecting {} players...", active_connections_.size());
    for (auto &connection : active_connections_) {
        if (connection) {
            connection->send_message("Server is shutting down. Goodbye!\r\n");
            connection->disconnect();
        }
    }
    active_connections_.clear();
    connection_actors_.clear();

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
        } else {
            // Empty input - just refresh the prompt (useful for watching regen)
            auto actor_it = connection_actors_.find(connection);
            if (actor_it != connection_actors_.end()) {
                send_prompt_to_actor(actor_it->second);
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
        } else {
            // Empty input - just refresh the prompt (useful for watching regen)
            send_prompt_to_actor(actor);
        }
    });
}

// Player Management

void WorldServer::add_player_connection(std::shared_ptr<PlayerConnection> connection) {
    if (!running_.load()) return;  // Skip during shutdown
    asio::post(world_strand_, [this, connection]() { handle_player_connection(connection); });
}

void WorldServer::remove_player_connection(std::shared_ptr<PlayerConnection> connection) {
    if (!running_.load()) return;  // Skip during shutdown - connections already cleared
    asio::post(world_strand_, [this, connection]() { handle_player_disconnection(connection); });
}

void WorldServer::set_actor_for_connection(std::shared_ptr<PlayerConnection> connection, std::shared_ptr<Actor> actor) {
    if (!running_.load()) return;  // Skip during shutdown
    asio::post(world_strand_, [this, connection, actor]() {
        // Remove old actor from its room if replacing an existing actor
        auto old_it = connection_actors_.find(connection);
        if (old_it != connection_actors_.end() && old_it->second) {
            auto old_actor = old_it->second;
            if (auto room = old_actor->current_room()) {
                room->remove_actor(old_actor->id());
                Log::debug("Removed old actor '{}' from room {} before replacing with '{}'",
                          old_actor->name(), room->id(), actor ? actor->name() : "null");
            }
        }
        connection_actors_[connection] = actor;
        if (actor) {
            Log::info("Player '{}' logged in. Online players: {}", actor->name(), connection_actors_.size());
        }
    });
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

        Log::info("Player '{}' reconnected. Online players: {}", player->name(), connection_actors_.size());
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

std::vector<std::shared_ptr<PlayerConnection>> WorldServer::get_active_connections() const {
    return active_connections_;
}

std::shared_ptr<Actor> WorldServer::get_actor_for_connection(std::shared_ptr<PlayerConnection> connection) const {
    auto it = connection_actors_.find(connection);
    if (it != connection_actors_.end()) {
        return it->second;
    }
    return nullptr;
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

    // Check for duplicates before adding
    auto it = std::find(active_connections_.begin(), active_connections_.end(), connection);
    if (it != active_connections_.end()) {
        Log::warn("Duplicate connection detected, skipping add");
        return;
    }

    active_connections_.push_back(connection);
    total_connections_.fetch_add(1);

    // Don't create a player yet - wait until login completes
    // The real player will be set via set_actor_for_connection() after login

    Log::info("Connection established. Active connections: {}", active_connections_.size());
}

void WorldServer::handle_player_disconnection(std::shared_ptr<PlayerConnection> connection) {
    // This runs on the world strand - thread safe!

    auto it = std::find(active_connections_.begin(), active_connections_.end(), connection);
    if (it != active_connections_.end()) {
        active_connections_.erase(it);
    }

    // Clean up the associated actor (if they logged in)
    auto actor_it = connection_actors_.find(connection);
    if (actor_it != connection_actors_.end()) {
        auto player_name = actor_it->second ? actor_it->second->name() : "unknown";
        Log::info("Player '{}' disconnected. Active connections: {}", player_name, active_connections_.size());
        // Actor destructor will handle room cleanup
        connection_actors_.erase(actor_it);
    } else {
        Log::info("Connection closed (not logged in). Active connections: {}", active_connections_.size());
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
    Log::debug("Scheduling combat processing timer ({}ms interval)", COMBAT_PROCESSING_INTERVAL.count());
    combat_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(COMBAT_PROCESSING_INTERVAL, [this]() { perform_combat_processing(); }, combat_timer_);
}

void WorldServer::schedule_periodic_save() {
    Log::info("Scheduling periodic player save timer ({}min interval)",
              std::chrono::duration_cast<std::chrono::minutes>(PLAYER_SAVE_INTERVAL).count());
    save_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(PLAYER_SAVE_INTERVAL, [this]() { perform_player_save(); }, save_timer_);
}

void WorldServer::schedule_spell_restoration() {
    Log::info("Scheduling spell restoration timer (1s interval)");
    spell_restore_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(SPELL_RESTORE_INTERVAL, [this]() { perform_spell_restoration(); }, spell_restore_timer_);
}

void WorldServer::schedule_casting_processing() {
    Log::info("Scheduling casting tick timer (500ms interval)");
    casting_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(CASTING_TICK_INTERVAL, [this]() { perform_casting_processing(); }, casting_timer_);
}

void WorldServer::schedule_regen_tick() {
    Log::info("Scheduling regeneration tick timer (4s interval like legacy)");
    regen_tick_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(REGEN_TICK_INTERVAL, [this]() { perform_regen_tick(); }, regen_tick_timer_);
}

void WorldServer::perform_regen_tick() {
    // Fast tick for HP/move regeneration - runs every 4 seconds like legacy MUD
    world_manager_->tick_regen_all();
}

void WorldServer::schedule_mob_activity() {
    Log::info("Scheduling mob activity timer (2s interval for AI/aggression)");
    mob_activity_timer_ = std::make_shared<asio::steady_timer>(world_strand_);
    schedule_timer(MOB_ACTIVITY_INTERVAL, [this]() { perform_mob_activity(); }, mob_activity_timer_);
}

// Alignment thresholds for aggression checks
namespace {
    constexpr int ALIGN_GOOD_THRESHOLD = 350;
    constexpr int ALIGN_EVIL_THRESHOLD = -350;

    bool is_good_alignment(int alignment) { return alignment >= ALIGN_GOOD_THRESHOLD; }
    bool is_evil_alignment(int alignment) { return alignment <= ALIGN_EVIL_THRESHOLD; }

    std::string_view alignment_name(int alignment) {
        if (is_good_alignment(alignment)) return "good";
        if (is_evil_alignment(alignment)) return "evil";
        return "neutral";
    }
}

void WorldServer::perform_mob_activity() {
    // Process mob AI for all spawned mobiles
    // This handles aggression, following, special behaviors, etc.

    if (!world_manager_) return;

    // Get all spawned mobiles
    auto& mobiles = world_manager_->spawned_mobiles();

    for (auto& [id, mobile] : mobiles) {
        if (!mobile || !mobile->is_alive()) continue;

        // Skip mobs already in combat
        if (mobile->is_fighting()) continue;

        // Check if mob has aggro_condition set (Lua expression from database)
        if (!mobile->is_aggressive()) continue;

        auto room = mobile->current_room();
        if (!room) {
            Log::debug("Aggressive mob '{}' has no current room!", mobile->name());
            continue;
        }

        const auto& aggro_condition = mobile->aggro_condition();

        // Look for players in the room to attack
        for (const auto& actor : room->contents().actors) {
            if (!actor || actor.get() == mobile.get()) continue;

            // Only attack players (not other mobs)
            if (actor->type_name() != "Player") continue;

            // Check if player is alive
            if (!actor->is_alive()) {
                Log::debug("Player '{}' is not alive, skipping", actor->name());
                continue;
            }

            // Don't attack immortals (level 100+)
            if (actor->stats().level >= 100) {
                Log::debug("Player '{}' is immortal (level {}), skipping",
                          actor->name(), actor->stats().level);
                continue;
            }

            // Evaluate aggro_condition - simple pattern matching for now
            // TODO: Implement proper Lua evaluation for complex conditions
            int player_align = actor->stats().alignment;
            bool should_attack = false;
            std::string attack_reason;

            if (!aggro_condition || aggro_condition->empty()) {
                continue;  // No condition set, skip
            } else if (*aggro_condition == "true") {
                // Attacks everyone
                should_attack = true;
                attack_reason = "aggressive (all)";
            } else if (aggro_condition->find("ALIGN.EVIL") != std::string::npos ||
                       aggro_condition->find("<= -350") != std::string::npos) {
                // Attacks evil-aligned players
                if (is_evil_alignment(player_align)) {
                    should_attack = true;
                    attack_reason = fmt::format("aggro-evil (player: {})", alignment_name(player_align));
                }
            } else if (aggro_condition->find("ALIGN.GOOD") != std::string::npos ||
                       aggro_condition->find(">= 350") != std::string::npos) {
                // Attacks good-aligned players
                if (is_good_alignment(player_align)) {
                    should_attack = true;
                    attack_reason = fmt::format("aggro-good (player: {})", alignment_name(player_align));
                }
            } else {
                // Unknown condition format - treat as aggressive to all for safety
                should_attack = true;
                attack_reason = fmt::format("condition: {}", *aggro_condition);
            }

            if (!should_attack) {
                Log::debug("Mob '{}' won't attack '{}' ({}) - condition '{}' not met",
                          mobile->name(), actor->name(), alignment_name(player_align),
                          aggro_condition ? *aggro_condition : "none");
                continue;
            }

            // Always attack when condition matches (no random roll)
            Log::debug("Aggro check: {} ({}) vs {} - attacking",
                      mobile->name(), attack_reason, actor->name());

            // Start combat!
            Log::info("{} ({}) attacks {}!",
                     mobile->name(), attack_reason, actor->name());

            // Use add_combat_pair so multiple mobs can attack same player
            FieryMUD::CombatManager::add_combat_pair(mobile, actor);

            // Send attack message
            mobile->send_message(fmt::format("You attack {}!", actor->display_name()));
            actor->send_message(fmt::format("{} attacks you!", mobile->display_name()));

            // Notify room
            for (const auto& witness : room->contents().actors) {
                if (witness && witness != mobile && witness != actor) {
                    witness->send_message(fmt::format("{} attacks {}!",
                        mobile->display_name(), actor->display_name()));
                }
            }

            // Only attack one player per tick
            break;
        }
    }

}

void WorldServer::perform_spell_restoration() {
    // Process spell slot restoration for all connected players
    for (auto& conn : active_connections_) {
        if (auto player = conn->get_player()) {
            // Get player's focus-based restoration rate
            int rate = player->get_spell_restore_rate();

            // Process restoration tick
            int restored_circle = player->spell_slots().restore_tick(rate);

            // Notify player if a slot was restored
            if (restored_circle > 0) {
                player->send_message(fmt::format(
                    "You feel your magical energies return. (Circle {} slot restored)",
                    restored_circle));
            }
        }
    }
}

void WorldServer::perform_casting_processing() {

    // Process casting ticks for all connected actors
    for (auto& conn : active_connections_) {
        auto it = connection_actors_.find(conn);
        if (it == connection_actors_.end() || !it->second) {
            continue;
        }

        auto& actor = it->second;

        // Skip if not casting
        if (!actor->is_casting()) {
            continue;
        }

        // Advance the casting timer
        if (actor->advance_casting()) {
            const auto& state_opt = actor->casting_state();
            if (state_opt.has_value()) {
                const auto& state = state_opt.value();
                Log::debug("Casting complete for {}: {} (target: {})",
                          actor->name(), state.ability_name, state.target_name);

                // Handle recall completion specially
                if (state.ability_id == MovementCommands::RECALL_ABILITY_ID) {
                    MovementCommands::complete_recall(actor);
                    actor->stop_casting();
                    continue;
                }

                // Casting completed - show completion message for spells
                actor->send_message("\nYou complete your spell.\n");

                // Get the target actor if needed
                std::shared_ptr<Actor> target_actor = state.target.lock();

                // If weak_ptr expired but we have a target name, try to re-find the target
                if (!target_actor && !state.target_name.empty()) {
                    auto room = actor->current_room();
                    if (room) {
                        for (const auto& room_actor : room->contents().actors) {
                            if (room_actor && room_actor != actor) {
                                // Match by name (case insensitive prefix match)
                                std::string lower_name{room_actor->name()};
                                std::string lower_target = state.target_name;
                                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
                                std::transform(lower_target.begin(), lower_target.end(), lower_target.begin(), ::tolower);
                                if (lower_name.starts_with(lower_target) || lower_target.starts_with(lower_name)) {
                                    target_actor = room_actor;
                                    Log::debug("Re-targeted {} to {} in room", state.ability_name, room_actor->name());
                                    break;
                                }
                            }
                        }
                    }
                }

                // Execute the completed spell through ability executor
                auto exec_result = FieryMUD::AbilityExecutor::execute_completed_cast(
                    actor, state.ability_id, target_actor);

                if (!exec_result) {
                    actor->send_message(fmt::format("Your spell fizzles! ({})\n",
                                                   exec_result.error().message));
                } else if (exec_result->success) {
                    // Send the generated messages
                    if (!exec_result->attacker_message.empty()) {
                        actor->send_message(exec_result->attacker_message + "\n");
                    }
                    if (target_actor && !exec_result->target_message.empty()) {
                        target_actor->send_message(exec_result->target_message + "\n");
                    }

                    // Send room message to others
                    if (!exec_result->room_message.empty()) {
                        if (auto room = actor->current_room()) {
                            for (const auto& other : room->contents().actors) {
                                if (other != actor && other != target_actor) {
                                    other->send_message(exec_result->room_message + "\n");
                                }
                            }
                        }
                    }

                    // Handle damage and combat if this was a violent spell
                    if (target_actor && exec_result->total_damage > 0) {
                        if (target_actor->stats().hit_points <= 0) {
                            // Target died
                            target_actor->stats().hit_points = 0;
                            FieryMUD::CombatManager::end_combat(target_actor);

                            bool target_is_player = (target_actor->type_name() == "Player");
                            if (target_is_player) {
                                target_actor->send_message("You are DEAD! You feel your spirit leave your body...\n");
                                if (auto room = actor->current_room()) {
                                    std::string death_msg = fmt::format("{} is DEAD! Their spirit rises from their body.\n",
                                                                       target_actor->display_name());
                                    for (const auto& other : room->contents().actors) {
                                        if (other != target_actor) {
                                            other->send_message(death_msg);
                                        }
                                    }
                                }
                            } else {
                                if (auto room = actor->current_room()) {
                                    std::string death_msg = fmt::format("{} is DEAD! Their corpse falls to the ground.\n",
                                                                       target_actor->display_name());
                                    for (const auto& other : room->contents().actors) {
                                        other->send_message(death_msg);
                                    }
                                }
                            }

                            // Create corpse and handle death
                            target_actor->die();
                        } else {
                            // Target is still alive - start combat if not already fighting
                            if (actor->position() != Position::Fighting) {
                                actor->set_position(Position::Fighting);
                                target_actor->set_position(Position::Fighting);
                                FieryMUD::CombatManager::start_combat(actor, target_actor);
                                Log::debug("Started combat between {} and {} after spell damage",
                                          actor->name(), target_actor->name());
                            }
                        }
                    }
                }

                // Clear the casting state
                actor->stop_casting(false);
            }
        } else {
            // Casting still in progress - show progress indicator
            const auto& state_opt = actor->casting_state();
            if (state_opt.has_value()) {
                const auto& state = state_opt.value();
                // Build progress indicator with stars
                std::string stars(state.ticks_remaining, '*');
                actor->send_message(fmt::format("\nCasting: {} {}\n",
                                               state.ability_name, stars));
            }
        }
    }
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
                // Check again before executing - shutdown may have started
                if (!running_.load()) return;
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

    // First, collect disconnected connections so we can clean them from both structures
    std::vector<std::shared_ptr<PlayerConnection>> disconnected;
    for (const auto& conn : active_connections_) {
        if (!conn->is_connected()) {
            disconnected.push_back(conn);
        }
    }

    // Remove disconnected connections from active_connections_
    if (!disconnected.empty()) {
        active_connections_.erase(
            std::remove_if(active_connections_.begin(), active_connections_.end(),
                           [](const std::shared_ptr<PlayerConnection> &conn) { return !conn->is_connected(); }),
            active_connections_.end());

        // Also remove from connection_actors_
        for (const auto& conn : disconnected) {
            auto it = connection_actors_.find(conn);
            if (it != connection_actors_.end()) {
                auto player_name = it->second ? it->second->name() : "unknown";
                Log::debug("Cleanup: removing stale connection for '{}'", player_name);
                connection_actors_.erase(it);
            }
        }

        Log::info("Cleaned up {} stale connections. Active: {}, Playing: {}",
                  disconnected.size(), active_connections_.size(), connection_actors_.size());
    }

    // Also clean up orphaned connection_actors_ entries (connections not in active_connections_)
    std::vector<std::shared_ptr<PlayerConnection>> orphaned_actors;
    for (const auto& [conn, actor] : connection_actors_) {
        if (std::find(active_connections_.begin(), active_connections_.end(), conn) == active_connections_.end()) {
            orphaned_actors.push_back(conn);
        }
    }
    for (const auto& conn : orphaned_actors) {
        auto it = connection_actors_.find(conn);
        if (it != connection_actors_.end()) {
            auto player_name = it->second ? it->second->name() : "unknown";
            Log::warn("Cleanup: removing orphaned actor entry for '{}'", player_name);
            connection_actors_.erase(it);
        }
    }

    Log::trace("Cleanup tasks completed");
}

void WorldServer::perform_heartbeat() {
    // This runs on the world strand - thread safe!

    // Track shutdown warning intervals (shared across calls)
    static int64_t last_shutdown_warning_time = -1;

    // Check for shutdown request
    if (world_manager_ && world_manager_->is_shutdown_requested()) {
        if (world_manager_->should_shutdown_now()) {
            Log::info("Shutdown time reached, initiating graceful shutdown...");
            if (shutdown_callback_) {
                shutdown_callback_();
            } else {
                Log::warn("Shutdown requested but no callback set - triggering stop() directly");
                running_.store(false);
            }
            return;  // Don't continue with heartbeat if shutting down
        }

        // Send periodic shutdown warnings
        auto time_left = std::chrono::duration_cast<std::chrono::seconds>(
            world_manager_->get_shutdown_time() - std::chrono::steady_clock::now()
        ).count();

        // Warn at specific intervals: 5min, 2min, 1min, 30sec, 10sec, 5sec
        bool should_warn = false;
        if (time_left <= 5 && last_shutdown_warning_time != 5) { should_warn = true; last_shutdown_warning_time = 5; }
        else if (time_left <= 10 && time_left > 5 && last_shutdown_warning_time != 10) { should_warn = true; last_shutdown_warning_time = 10; }
        else if (time_left <= 30 && time_left > 10 && last_shutdown_warning_time != 30) { should_warn = true; last_shutdown_warning_time = 30; }
        else if (time_left <= 60 && time_left > 30 && last_shutdown_warning_time != 60) { should_warn = true; last_shutdown_warning_time = 60; }
        else if (time_left <= 120 && time_left > 60 && last_shutdown_warning_time != 120) { should_warn = true; last_shutdown_warning_time = 120; }
        else if (time_left <= 300 && time_left > 120 && last_shutdown_warning_time != 300) { should_warn = true; last_shutdown_warning_time = 300; }

        if (should_warn) {
            std::string time_str;
            if (time_left >= 60) {
                time_str = fmt::format("{} minute{}", time_left / 60, time_left >= 120 ? "s" : "");
            } else {
                time_str = fmt::format("{} second{}", time_left, time_left != 1 ? "s" : "");
            }
            std::string msg = fmt::format("\r\n*** SYSTEM: MUD shutting down in {}. {} ***\r\n",
                                          time_str, world_manager_->get_shutdown_reason());
            for (auto& conn : active_connections_) {
                if (conn) conn->send_line(msg);
            }
        }
    } else {
        // Reset warning tracker when shutdown is cancelled
        last_shutdown_warning_time = -1;
    }

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
    // Early exit if shutting down
    if (!running_.load()) return;

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

    // Process queued spells for players who are ready to cast
    // (not casting and not in blackout)
    for (auto& conn : active_connections_) {
        auto it = connection_actors_.find(conn);
        if (it == connection_actors_.end() || !it->second) {
            continue;
        }

        auto& actor = it->second;

        // Only process queued spell if actor is completely ready to cast:
        // - Has a queued spell
        // - Not currently casting another spell
        // - Not in casting blackout
        if (actor->has_queued_spell() && !actor->is_casting() && !actor->is_casting_blocked()) {
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

/**
 * Get condition string based on HP percentage
 */
static std::string get_condition_string(int hp_percent) {
    if (hp_percent >= CONDITION_PERFECT)
        return "perfect";
    else if (hp_percent >= CONDITION_SCRATCHED)
        return "scratched";
    else if (hp_percent >= CONDITION_HURT)
        return "hurt";
    else if (hp_percent >= CONDITION_WOUNDED)
        return "wounded";
    else if (hp_percent >= CONDITION_BLEEDING)
        return "bleeding";
    else if (hp_percent >= CONDITION_CRITICAL)
        return "critical";
    else
        return "dying";
}

/**
 * Expand prompt format codes into actual values.
 *
 * Format codes:
 *   %h - current hit points    %H - max hit points
 *   %v - current stamina       %V - max stamina
 *   %l - level                 %g - gold
 *   %x - experience            %X - exp to next level
 *   %t - tank condition        %T - target condition
 *   %n - newline               %% - literal %
 *
 * Color markup is preserved in output and processed by terminal rendering.
 */
static std::string expand_prompt_format(
    std::string_view format,
    const Stats& stats,
    std::shared_ptr<Actor> fighting = nullptr
) {
    std::string result;
    result.reserve(format.size() * 2);  // Pre-allocate for efficiency

    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%' && i + 1 < format.size()) {
            char code = format[++i];
            switch (code) {
                case 'h': result += std::to_string(stats.hit_points); break;
                case 'H': result += std::to_string(stats.max_hit_points); break;
                case 'v': result += std::to_string(stats.stamina); break;
                case 'V': result += std::to_string(stats.max_stamina); break;
                case 'l': result += std::to_string(stats.level); break;
                case 'g': result += std::to_string(stats.gold); break;
                case 'x': result += std::to_string(stats.experience); break;
                case 'X': {
                    // Experience to next level (simplified - could be more complex)
                    long next_level_exp = static_cast<long>(stats.level) * 1000;
                    result += std::to_string(std::max(0L, next_level_exp - stats.experience));
                    break;
                }
                case 't':
                case 'T': {
                    // Target/tank condition - show fighting opponent's condition
                    if (fighting) {
                        const auto& opp_stats = fighting->stats();
                        int hp_percent = (opp_stats.hit_points * PERCENT_MULTIPLIER) /
                                        std::max(1, opp_stats.max_hit_points);
                        result += get_condition_string(hp_percent);
                    }
                    break;
                }
                case 'n': result += "\n"; break;
                case '%': result += "%"; break;
                default:
                    // Unknown code - keep original
                    result += '%';
                    result += code;
                    break;
            }
        } else {
            result += format[i];
        }
    }

    return result;
}

void WorldServer::send_prompt_to_actor(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return;
    }

    // Get actor's current stats
    const auto &stats = actor->stats();

    // Get the player's custom prompt format, or use default
    std::string_view format = "<%h/%Hhp %v/%Vs>";  // Default format

    auto player = std::dynamic_pointer_cast<Player>(actor);
    if (player && !player->prompt().empty()) {
        format = player->prompt();
    }

    // Get fighting opponent (if any) for condition codes
    std::shared_ptr<Actor> opponent = nullptr;
    if (actor->is_fighting()) {
        opponent = FieryMUD::CombatManager::get_opponent(*actor);
    }

    // Expand the prompt format (substitutes %h, %H, etc.)
    std::string prompt = expand_prompt_format(format, stats, opponent);

    // Process color markup (renders <red>, <yellow>, etc. to ANSI)
    prompt = TextFormat::apply_colors(prompt);

    // Add space and trailing prompt character if not present
    if (!prompt.empty() && prompt.back() != '>' && prompt.back() != ':' && prompt.back() != ' ') {
        prompt += ' ';
    }

    // Send the prompt - we need to handle different actor types
    if (player) {
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

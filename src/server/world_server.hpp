// ASIO strand-based game loop with serialized world state access

#pragma once

#include "../core/result.hpp"
#include "../core/ids.hpp"
#include "../world/time_system.hpp"
// #include "../game/loop.hpp" // Game loop functionality integrated
#include <asio.hpp>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <unordered_map>

// Forward declarations
struct ServerConfig;
class WorldManager;
class CommandSystem;
class Player;
class PlayerConnection;
class Actor;

/** Command request structure for processing player commands */
struct CommandRequest {
    std::string command;
    std::string args;
    EntityId player_id{INVALID_ENTITY_ID};
    std::chrono::steady_clock::time_point timestamp;
    
    CommandRequest() = default;
    CommandRequest(std::string_view cmd, EntityId pid = INVALID_ENTITY_ID) 
        : command(cmd), player_id(pid), timestamp(std::chrono::steady_clock::now()) {}
};

/** Game loop performance statistics */
struct GameLoopStats {
    std::chrono::steady_clock::time_point start_time;
    std::atomic<uint64_t> commands_processed{0};
    std::atomic<uint64_t> heartbeats{0};
    std::chrono::milliseconds average_tick_time{0};
    
    // Custom constructors for atomic members
    GameLoopStats() : start_time(std::chrono::steady_clock::now()) {}
    
    // Copy constructor (required for atomic members)
    GameLoopStats(const GameLoopStats& other) 
        : start_time(other.start_time)
        , commands_processed(other.commands_processed.load())
        , heartbeats(other.heartbeats.load())
        , average_tick_time(other.average_tick_time) {}
    
    // Move constructor
    GameLoopStats(GameLoopStats&& other) noexcept
        : start_time(std::move(other.start_time))
        , commands_processed(other.commands_processed.load())
        , heartbeats(other.heartbeats.load())
        , average_tick_time(std::move(other.average_tick_time)) {}
    
    // Assignment operators
    GameLoopStats& operator=(const GameLoopStats& other) {
        if (this != &other) {
            start_time = other.start_time;
            commands_processed.store(other.commands_processed.load());
            heartbeats.store(other.heartbeats.load());
            average_tick_time = other.average_tick_time;
        }
        return *this;
    }
    
    GameLoopStats& operator=(GameLoopStats&& other) noexcept {
        if (this != &other) {
            start_time = std::move(other.start_time);
            commands_processed.store(other.commands_processed.load());
            heartbeats.store(other.heartbeats.load());
            average_tick_time = std::move(other.average_tick_time);
        }
        return *this;
    }
    
    std::chrono::seconds uptime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time);
    }
};

/**
 * Strand-based World Server
 * 
 * All world operations are serialized through an Asio strand, eliminating
 * the need for explicit synchronization. This provides:
 * - Thread-safe world state access
 * - Simplified command processing
 * - Clean integration with async networking
 * - Natural serialization for turn-based game logic
 */
class WorldServer {
public:
    explicit WorldServer(asio::io_context& io_context, const ServerConfig& config);
    ~WorldServer();
    
    /** Get current singleton instance (if set) */
    static WorldServer* instance();
    
    // Lifecycle management
    Result<void> initialize(bool is_test_mode = false);
    Result<void> start();
    void stop();
    bool is_running() const { return running_.load(); }

    asio::strand<asio::io_context::executor_type>& get_strand() { return world_strand_; }
    std::future<void> get_world_loaded_future();
    
    // Command processing (thread-safe - posts to strand)
    void process_command(CommandRequest command);
    void process_command(std::shared_ptr<PlayerConnection> connection, 
                        std::string_view command_text);
    void process_command(std::shared_ptr<Actor> actor, 
                        std::string_view command_text);
    
    // Player management (thread-safe - posts to strand)
    void add_player_connection(std::shared_ptr<PlayerConnection> connection);
    void remove_player_connection(std::shared_ptr<PlayerConnection> connection);
    void set_actor_for_connection(std::shared_ptr<PlayerConnection> connection, std::shared_ptr<Actor> actor);
    void handle_player_reconnection(std::shared_ptr<PlayerConnection> old_connection,
                                    std::shared_ptr<PlayerConnection> new_connection,
                                    std::shared_ptr<Player> player);
    void add_player(std::shared_ptr<Player> player);
    void remove_player(std::shared_ptr<Player> player);
    
    // Command system access
    CommandSystem* get_command_system() const;
    
    // Player access
    std::vector<std::shared_ptr<Player>> get_online_players() const;
    std::vector<std::shared_ptr<Actor>> get_online_actors() const;
    
    // Periodic operations (thread-safe - runs on strand)
    void schedule_periodic_cleanup();
    void schedule_heartbeat();
    void schedule_combat_processing();
    
    // Statistics (thread-safe)
    size_t active_player_count() const;
    GameLoopStats get_performance_stats() const;

    // Data management
    Result<void> create_default_world();
    
private:
    // Strand-based operations (must run on world_strand_)
    void handle_command(CommandRequest command);
    void handle_player_connection(std::shared_ptr<PlayerConnection> connection);
    void handle_player_disconnection(std::shared_ptr<PlayerConnection> connection);
    
    // Periodic tasks (run on world_strand_)
    void perform_cleanup();
    void perform_heartbeat();
    void perform_combat_processing();
    
    // GMCP support
    void send_room_info_to_player(std::shared_ptr<PlayerConnection> connection);
    
    // Prompt system
    void send_prompt_to_actor(std::shared_ptr<Actor> actor);
    
    // Timer management
    void schedule_timer(std::chrono::milliseconds interval,
                       std::function<void()> task,
                       const std::shared_ptr<asio::steady_timer>& timer);
    
    // Configuration and state
    asio::io_context& io_context_;
    asio::strand<asio::io_context::executor_type> world_strand_;
    const ServerConfig& config_;
    std::atomic<bool> running_{false};
    
    // Game systems (accessed only from world_strand_)
    WorldManager* world_manager_;
    CommandSystem* command_system_;
    
    // Player connections (managed on world_strand_)
    std::vector<std::shared_ptr<PlayerConnection>> active_connections_;
    
    // Actor management (one persistent actor per connection)
    std::unordered_map<std::shared_ptr<PlayerConnection>, std::shared_ptr<Actor>> connection_actors_;
    
    // Periodic timers
    std::shared_ptr<asio::steady_timer> save_timer_;
    std::shared_ptr<asio::steady_timer> cleanup_timer_;
    std::shared_ptr<asio::steady_timer> heartbeat_timer_;
    std::shared_ptr<asio::steady_timer> combat_timer_;
    
    // Performance tracking
    mutable std::atomic<size_t> commands_processed_{0};
    mutable std::atomic<size_t> total_connections_{0};
    std::chrono::steady_clock::time_point start_time_;
    std::promise<void> world_loaded_promise_;
};
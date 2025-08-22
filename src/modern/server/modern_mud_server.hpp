/***************************************************************************
 *   File: src/server/modern_mud_server.hpp               Part of FieryMUD *
 *  Usage: Standalone modern MUD server implementation                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "../core/result.hpp"
#include "../core/ids.hpp"
// #include "../game/loop.hpp" // Game loop functionality integrated into server
#include "../world/world_manager.hpp"
#include "../commands/command_system.hpp"
#include "world_server.hpp"

#include <asio.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <atomic>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>

// Forward declarations
class NetworkManager;
class PersistenceManager;
class ConfigurationManager;
class Player;

/**
 * Modern FieryMUD Server - Complete standalone implementation
 *
 * Integrates all modern components:
 * - GameLoop for game state management
 * - WorldManager for world simulation
 * - CommandSystem for command processing
 * - NetworkManager for player connections
 * - PersistenceManager for data storage
 */

/** Server configuration */
struct ServerConfig {
    // Network settings
    std::string bind_address = "0.0.0.0";
    int port = 4001;
    int max_connections = 200;
    std::chrono::seconds connection_timeout{300};
    
    // Game settings
    std::string mud_name = "Modern FieryMUD";
    std::string world_directory = "data/world";
    std::string player_directory = "data/players";
    std::string log_directory = "logs";
    
    // Performance settings
    int target_tps = 10; // Ticks per second
    size_t max_command_queue_size = 10000;
    bool enable_performance_monitoring = true;
    
    // Persistence settings
    std::chrono::seconds auto_save_interval{300};
    std::chrono::seconds backup_interval{3600};
    int max_backups = 24;
    
    // Security settings
    int max_login_attempts = 3;
    std::chrono::minutes login_timeout{15};
    bool enable_new_player_creation = true;
    
    // Debugging
    bool enable_debug_commands = false;
    std::string admin_password = "changeme";
    
    // Load from JSON configuration file
    static Result<ServerConfig> load_from_file(const std::string& filename);
    
    // Save to JSON configuration file
    Result<void> save_to_file(const std::string& filename) const;
    
    // Validate configuration
    Result<void> validate() const;
};

/** Server runtime statistics */
struct ServerStats {
    std::chrono::steady_clock::time_point start_time;
    std::atomic<uint64_t> total_connections{0};
    std::atomic<uint64_t> current_connections{0};
    std::atomic<uint64_t> total_commands{0};
    std::atomic<uint64_t> failed_commands{0};
    std::atomic<uint64_t> total_logins{0};
    std::atomic<uint64_t> failed_logins{0};
    
    std::chrono::seconds uptime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time);
    }
    
    double commands_per_second() const {
        auto up = uptime().count();
        return up > 0 ? static_cast<double>(total_commands.load()) / up : 0.0;
    }
    
    std::string summary() const;
};

/** Server operational state */
enum class ServerState {
    Stopped,
    Initializing,
    Starting,
    Running,
    Stopping,
    Error
};

/**
 * Main Modern MUD Server Class
 */
class ModernMUDServer {
public:
    /** Constructor */
    explicit ModernMUDServer(ServerConfig config = ServerConfig{});
    
    /** Destructor */
    ~ModernMUDServer();
    
    // Core lifecycle
    Result<void> initialize();
    Result<void> start();
    void stop();
    void restart();
    
    // State management
    ServerState state() const { return state_.load(); }
    bool is_running() const { return state_.load() == ServerState::Running; }
    
    // Configuration
    const ServerConfig& config() const { return config_; }
    Result<void> update_config(const ServerConfig& new_config);
    Result<void> reload_config();
    
    // Statistics and monitoring
    const ServerStats& stats() const { return stats_; }
    std::string get_status_report() const;
    std::string get_performance_report() const;
    
    // Player management
    std::vector<std::shared_ptr<Player>> get_online_players() const;
    std::shared_ptr<Player> find_player(std::string_view name) const;
    size_t online_player_count() const;
    
    // Administrative functions
    void broadcast_message(std::string_view message);
    void kick_player(std::string_view player_name, std::string_view reason = "");
    void shutdown_with_countdown(std::chrono::seconds countdown);
    
    // Data management
    Result<void> save_all_data();
    Result<void> backup_data();
    Result<void> load_world_data();
    
    // Emergency controls
    void emergency_shutdown();
    void enable_maintenance_mode(bool enabled);
    bool is_maintenance_mode() const { return maintenance_mode_.load(); }
    
private:
    // Configuration and state
    ServerConfig config_;
    std::atomic<ServerState> state_{ServerState::Stopped};
    std::atomic<bool> maintenance_mode_{false};
    ServerStats stats_;
    
    // Asio I/O context shared by all async operations
    asio::io_context io_context_;
    std::thread io_thread_;
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;
    
    // Core systems
    std::unique_ptr<WorldServer> world_server_;
    std::unique_ptr<NetworkManager> network_manager_;
    std::unique_ptr<PersistenceManager> persistence_manager_;
    std::unique_ptr<ConfigurationManager> config_manager_;
    
    // Management threads
    std::thread save_thread_;
    std::thread backup_thread_;
    std::thread monitoring_thread_;
    
    std::atomic<bool> shutdown_requested_{false};
    
    // Initialization phases
    Result<void> initialize_logging();
    Result<void> initialize_directories();
    Result<void> initialize_game_systems();
    Result<void> initialize_networking();
    Result<void> initialize_persistence();
    Result<void> load_initial_data();
    
    // Shutdown phases
    void shutdown_networking();
    void shutdown_game_systems();
    void save_all_data_sync();
    void cleanup_resources();
    
    // Background tasks
    void periodic_save_task();
    void periodic_backup_task();
    void monitoring_task();
    
    // Event handlers
    void on_player_connect(std::shared_ptr<Player> player);
    void on_player_disconnect(std::shared_ptr<Player> player);
    void on_command_executed(EntityId player_id, std::string_view command, bool success);
    
    // Helper methods
    void set_state(ServerState new_state);
    void log_state_change(ServerState old_state, ServerState new_state);
    void update_stats();
    Result<void> validate_directories();
    Result<void> create_default_world();
    
    // Non-copyable
    ModernMUDServer(const ModernMUDServer&) = delete;
    ModernMUDServer& operator=(const ModernMUDServer&) = delete;
};

/** Server management utilities */
namespace ServerUtils {
    /** Check if port is available */
    bool is_port_available(int port);
    
    /** Get system resource usage */
    struct ResourceUsage {
        double cpu_percentage;
        size_t memory_mb;
        size_t disk_free_mb;
        int network_connections;
    };
    
    ResourceUsage get_resource_usage();
    
    /** Validate server environment */
    Result<void> validate_environment();
    
    /** Create default configuration file */
    Result<void> create_default_config(const std::string& filename);
    
    /** Setup logging configuration */
    Result<void> setup_logging(const ServerConfig& config);
    
    /** Daemonize process (Unix only) */
    Result<void> daemonize();
    
    /** Signal handling setup */
    void setup_signal_handlers();
    
    /** Performance profiling */
    struct ProfileData {
        std::chrono::milliseconds game_loop_time;
        std::chrono::milliseconds network_time;
        std::chrono::milliseconds persistence_time;
        size_t memory_usage;
    };
    
    ProfileData profile_performance();
}
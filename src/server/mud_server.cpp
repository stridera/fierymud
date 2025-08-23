/***************************************************************************
 *   File: src/server/mud_server.cpp               Part of FieryMUD *
 *  Usage: Standalone modern MUD server implementation                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "mud_server.hpp"

#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "configuration_manager.hpp"
#include "network_manager.hpp"
#include "persistence_manager.hpp"

#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

using json = nlohmann::json;

// ServerConfig Implementation

Result<ServerConfig> ServerConfig::load_from_file(const std::string &filename) {
    try {
        if (!std::filesystem::exists(filename)) {
            return std::unexpected(Errors::FileNotFound(filename));
        }

        std::ifstream file(filename);
        if (!file.is_open()) {
            return std::unexpected(Errors::FileSystem(fmt::format("Cannot open config file: {}", filename)));
        }

        json j;
        file >> j;

        ServerConfig config;

        // Network settings
        if (j.contains("network")) {
            const auto &net = j["network"];
            config.bind_address = net.value("bind_address", config.bind_address);
            config.port = net.value("port", config.port);
            config.max_connections = net.value("max_connections", config.max_connections);
            config.connection_timeout = std::chrono::seconds(net.value("connection_timeout_seconds", 300));
        }

        // Game settings
        if (j.contains("game")) {
            const auto &game = j["game"];
            config.mud_name = game.value("mud_name", config.mud_name);
            config.world_directory = game.value("world_directory", config.world_directory);
            config.player_directory = game.value("player_directory", config.player_directory);
            config.log_directory = game.value("log_directory", config.log_directory);
            config.default_starting_room = game.value("default_starting_room", config.default_starting_room);
        }

        // Performance settings
        if (j.contains("performance")) {
            const auto &perf = j["performance"];
            config.target_tps = perf.value("target_tps", config.target_tps);
            config.max_command_queue_size = perf.value("max_command_queue_size", config.max_command_queue_size);
            config.enable_performance_monitoring =
                perf.value("enable_monitoring", config.enable_performance_monitoring);
        }

        // Persistence settings
        if (j.contains("persistence")) {
            const auto &persist = j["persistence"];
            config.auto_save_interval = std::chrono::seconds(persist.value("auto_save_interval_seconds", 300));
            config.backup_interval = std::chrono::seconds(persist.value("backup_interval_seconds", 3600));
            config.max_backups = persist.value("max_backups", config.max_backups);
        }

        auto validation_result = config.validate();
        if (!validation_result) {
            return std::unexpected(validation_result.error());
        }

        return config;

    } catch (const json::exception &e) {
        return std::unexpected(Errors::InvalidFormat(fmt::format("JSON parsing error: {}", e.what())));
    } catch (const std::exception &e) {
        return std::unexpected(Errors::SystemError(fmt::format("Error loading config: {}", e.what())));
    }
}

Result<void> ServerConfig::save_to_file(const std::string &filename) const {
    try {
        json j;

        j["network"] = {{"bind_address", bind_address},
                        {"port", port},
                        {"max_connections", max_connections},
                        {"connection_timeout_seconds", connection_timeout.count()}};

        j["game"] = {{"mud_name", mud_name},
                     {"world_directory", world_directory},
                     {"player_directory", player_directory},
                     {"log_directory", log_directory},
                     {"default_starting_room", default_starting_room}};

        j["performance"] = {{"target_tps", target_tps},
                            {"max_command_queue_size", max_command_queue_size},
                            {"enable_monitoring", enable_performance_monitoring}};

        j["persistence"] = {{"auto_save_interval_seconds", auto_save_interval.count()},
                            {"backup_interval_seconds", backup_interval.count()},
                            {"max_backups", max_backups}};

        std::ofstream file(filename);
        if (!file.is_open()) {
            return std::unexpected(Errors::FileSystem(fmt::format("Cannot create config file: {}", filename)));
        }

        file << j.dump(2);
        return Success();

    } catch (const std::exception &e) {
        return std::unexpected(Errors::SystemError(fmt::format("Error saving config: {}", e.what())));
    }
}

Result<void> ServerConfig::validate() const {
    if (port <= 0 || port > 65535) {
        return std::unexpected(Errors::InvalidArgument("port", "Must be between 1 and 65535"));
    }

    if (max_connections <= 0 || max_connections > 10000) {
        return std::unexpected(Errors::InvalidArgument("max_connections", "Must be between 1 and 10000"));
    }

    if (target_tps <= 0 || target_tps > 100) {
        return std::unexpected(Errors::InvalidArgument("target_tps", "Must be between 1 and 100"));
    }

    if (mud_name.empty()) {
        return std::unexpected(Errors::InvalidArgument("mud_name", "Cannot be empty"));
    }

    return Success();
}

// ServerStats Implementation

std::string ServerStats::summary() const {
    std::ostringstream oss;
    oss << fmt::format("Uptime: {}s, Connections: {} current/{} total, ", uptime().count(), current_connections.load(),
                       total_connections.load());
    oss << fmt::format("Commands: {} total ({:.2f}/s), Logins: {} total/{} failed", total_commands.load(),
                       commands_per_second(), total_logins.load(), failed_logins.load());
    return oss.str();
}

// ModernMUDServer Implementation

ModernMUDServer::ModernMUDServer(ServerConfig config) : config_(std::move(config)) {

    stats_.start_time = std::chrono::steady_clock::now();

    // Initialize systems (but don't start them yet)
    world_server_ = std::make_unique<WorldServer>(io_context_, config_);
    network_manager_ = std::make_unique<NetworkManager>(io_context_, config_);
    persistence_manager_ = std::make_unique<PersistenceManager>();
    config_manager_ = std::make_unique<ConfigurationManager>();
}

ModernMUDServer::~ModernMUDServer() {
    if (is_running()) {
        stop();
    }
}

Result<void> ModernMUDServer::initialize() {
    set_state(ServerState::Initializing);

    Log::info("Initializing Modern FieryMUD Server...");

    // Initialize in dependency order
    auto logging_result = initialize_logging();
    if (!logging_result) {
        set_state(ServerState::Error);
        return logging_result;
    }

    // Initialize legacy Config singleton from our ServerConfig
    auto config_result = Config::initialize_from_server_config(config_);
    if (!config_result) {
        set_state(ServerState::Error);
        return config_result;
    }

    auto dirs_result = initialize_directories();
    if (!dirs_result) {
        set_state(ServerState::Error);
        return dirs_result;
    }

    auto systems_result = initialize_game_systems();
    if (!systems_result) {
        set_state(ServerState::Error);
        return systems_result;
    }

    auto persistence_result = initialize_persistence();
    if (!persistence_result) {
        set_state(ServerState::Error);
        return persistence_result;
    }

    auto network_result = initialize_networking();
    if (!network_result) {
        set_state(ServerState::Error);
        return network_result;
    }

    auto data_result = load_initial_data();
    if (!data_result) {
        set_state(ServerState::Error);
        return data_result;
    }

    Log::info("Modern FieryMUD Server initialized successfully");
    return Success();
}

Result<void> ModernMUDServer::start() {
    if (state_.load() != ServerState::Initializing && state_.load() != ServerState::Stopped) {
        return std::unexpected(Errors::InvalidState("Server must be initialized before starting"));
    }

    set_state(ServerState::Starting);
    shutdown_requested_.store(false);

    Log::info("Starting Modern FieryMUD Server on {}:{}", config_.bind_address, config_.port);

    try {
        // Start I/O context thread FIRST (required for strand operations)
        // Add work guard to keep I/O context running
        work_guard_ = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
            asio::make_work_guard(io_context_));

        io_thread_ = std::thread([this]() {
            Log::info("I/O context thread started");
            try {
                io_context_.run();
            } catch (const std::exception &e) {
                Log::error("I/O context error: {}", e.what());
            }
            Log::info("I/O context thread stopped");
        });

        // Give I/O thread a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Now start world server (uses strand operations)
        auto world_result = world_server_->start();
        if (!world_result) {
            set_state(ServerState::Error);
            return world_result;
        }

        // Start networking
        auto network_result = network_manager_->start();
        if (!network_result) {
            set_state(ServerState::Error);
            return network_result;
        }

        // Start background tasks (these are now handled by WorldServer)
        // save_thread_ and backup_thread_ are replaced by WorldServer timers

        set_state(ServerState::Running);
        Log::info("Modern FieryMUD Server started successfully - ready for connections");

        return Success();

    } catch (const std::exception &e) {
        set_state(ServerState::Error);
        return std::unexpected(Errors::SystemError(fmt::format("Failed to start server: {}", e.what())));
    }
}

void ModernMUDServer::stop() {
    if (!is_running() && state_.load() != ServerState::Starting) {
        return;
    }

    set_state(ServerState::Stopping);
    shutdown_requested_.store(true);

    Log::info("Stopping Modern FieryMUD Server...");

    // Stop world server (handles save and cleanup)
    if (world_server_) {
        world_server_->stop();
    }

    // Stop networking
    if (network_manager_) {
        network_manager_->stop();
    }

    // Stop I/O context
    if (work_guard_) {
        work_guard_.reset(); // Release work guard to allow I/O context to stop
    }
    io_context_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }

    // Clean up resources
    cleanup_resources();

    set_state(ServerState::Stopped);
    Log::info("Modern FieryMUD Server stopped");
}

void ModernMUDServer::restart() {
    Log::info("Restarting Modern FieryMUD Server...");
    stop();

    // Small delay for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto start_result = start();
    if (!start_result) {
        Log::error("Failed to restart server: {}", start_result.error().message);
        set_state(ServerState::Error);
    }
}

std::string ModernMUDServer::get_status_report() const {
    std::ostringstream report;

    report << fmt::format("=== {} Status Report ===\n", config_.mud_name);
    report << fmt::format("State: {}\n", magic_enum::enum_name(state_.load()));
    report << fmt::format("Uptime: {}\n", stats_.uptime().count());
    report << fmt::format("Port: {}\n", config_.port);
    report << fmt::format("Players Online: {}/{}\n", stats_.current_connections.load(), config_.max_connections);

    if (world_server_) {
        report << fmt::format("World Server: {} active players\n", world_server_->active_player_count());
    }

    report << fmt::format("Performance: {}\n", stats_.summary());

    return report.str();
}

std::string ModernMUDServer::get_performance_report() const {
    std::ostringstream report;

    report << "=== Performance Report ===\n";

    if (world_server_) {
        auto stats = world_server_->get_performance_stats();
        report << fmt::format("World Server: {} commands processed\n", stats.commands_processed.load());
    }

    auto resource_usage = ServerUtils::get_resource_usage();
    report << fmt::format("Resources: CPU {:.1f}%, Memory {}MB, Disk Free {}GB\n", resource_usage.cpu_percentage,
                          resource_usage.memory_mb, resource_usage.disk_free_mb / 1024);

    return report.str();
}

void ModernMUDServer::broadcast_message(std::string_view message) {
    if (!network_manager_)
        return;

    auto online_players = get_online_players();
    for (auto &player : online_players) {
        // TODO: Implement player messaging when network layer is complete
        Log::info("Broadcasting to {}: {}", player->name(), message);
    }

    Log::info("Broadcast: {}", message);
}

// Initialization Methods

Result<void> ModernMUDServer::initialize_logging() { return ServerUtils::setup_logging(config_); }

Result<void> ModernMUDServer::initialize_directories() {
    namespace fs = std::filesystem;

    std::vector<std::string> required_dirs = {config_.world_directory, config_.player_directory, config_.log_directory,
                                              "data/backups"};

    for (const auto &dir : required_dirs) {
        if (!fs::exists(dir)) {
            std::error_code ec;
            if (!fs::create_directories(dir, ec)) {
                return std::unexpected(
                    Errors::FileSystem(fmt::format("Failed to create directory {}: {}", dir, ec.message())));
            }
            Log::info("Created directory: {}", dir);
        }
    }

    return Success();
}

Result<void> ModernMUDServer::initialize_game_systems() {
    // Initialize world server (which handles game systems internally)
    auto world_result = world_server_->initialize();
    if (!world_result) {
        return world_result;
    }

    return Success();
}

Result<void> ModernMUDServer::initialize_networking() {
    // Connect network manager to world server
    network_manager_->set_world_server(world_server_.get());

    return network_manager_->initialize();
}

Result<void> ModernMUDServer::initialize_persistence() { return persistence_manager_->initialize(config_); }

Result<void> ModernMUDServer::load_initial_data() {
    // World loading is now handled asynchronously by WorldServer on the world strand
    // after the server starts. No synchronous validation needed during initialization.
    Log::info("World data will be loaded asynchronously after server startup");
    return Success();
}

Result<void> ModernMUDServer::create_default_world() {
    if (world_server_) {
        return world_server_->create_default_world();
    }
    return std::unexpected(Errors::InvalidState("World server not initialized"));
}

// Background Tasks

void ModernMUDServer::periodic_save_task() {
    while (!shutdown_requested_.load()) {
        std::this_thread::sleep_for(config_.auto_save_interval);

        if (shutdown_requested_.load())
            break;

        auto save_result = save_all_data();
        if (!save_result) {
            Log::error("Periodic save failed: {}", save_result.error().message);
        } else {
            Log::debug("Periodic save completed successfully");
        }
    }
}

void ModernMUDServer::periodic_backup_task() {
    while (!shutdown_requested_.load()) {
        std::this_thread::sleep_for(config_.backup_interval);

        if (shutdown_requested_.load())
            break;

        auto backup_result = backup_data();
        if (!backup_result) {
            Log::error("Periodic backup failed: {}", backup_result.error().message);
        } else {
            Log::info("Periodic backup completed successfully");
        }
    }
}

void ModernMUDServer::monitoring_task() {
    while (!shutdown_requested_.load()) {
        std::this_thread::sleep_for(std::chrono::minutes(5));

        if (shutdown_requested_.load())
            break;

        update_stats();
        Log::info("Server Status: {}", get_status_report());
    }
}

Result<void> ModernMUDServer::save_all_data() {
    if (world_server_) {
        world_server_->save_all_data();
    }

    if (persistence_manager_) {
        auto persist_result = persistence_manager_->save_all_players();
        if (!persist_result) {
            return persist_result;
        }
    }

    return Success();
}

void ModernMUDServer::save_all_data_sync() {
    auto result = save_all_data();
    if (!result) {
        Log::error("Failed to save data during shutdown: {}", result.error().message);
    }
}

// Helper Methods

void ModernMUDServer::set_state(ServerState new_state) {
    auto old_state = state_.exchange(new_state);
    if (old_state != new_state) {
        log_state_change(old_state, new_state);
    }
}

void ModernMUDServer::log_state_change(ServerState old_state, ServerState new_state) {
    Log::info("Server state changed: {} -> {}", magic_enum::enum_name(old_state), magic_enum::enum_name(new_state));
}

// Placeholder implementations for missing classes
std::vector<std::shared_ptr<Player>> ModernMUDServer::get_online_players() const {
    // TODO: Implement when Player class and network layer are complete
    return {};
}

void ModernMUDServer::shutdown_networking() {
    if (network_manager_) {
        network_manager_->stop();
    }
}

void ModernMUDServer::shutdown_game_systems() {
    // Game systems are shut down as part of world_server_->stop()
}

void ModernMUDServer::cleanup_resources() {
    // Resources will be cleaned up automatically by destructors
}

Result<void> ModernMUDServer::reload_config() {
    Log::info("Reloading server configuration...");

    // TODO: Implement actual config reloading
    // For now, just log that we received the request
    Log::info("Configuration reload completed (placeholder)");
    return Success();
}

Result<void> ModernMUDServer::backup_data() {
    Log::info("Starting data backup...");

    if (persistence_manager_) {
        auto backup_result = persistence_manager_->backup_data();
        if (!backup_result) {
            return backup_result;
        }
    }

    Log::info("Data backup completed");
    return Success();
}

void ModernMUDServer::update_stats() {
    // Update current connection count
    if (network_manager_) {
        stats_.current_connections = network_manager_->connection_count();
    }

    // TODO: Update other stats like memory usage, CPU usage, etc.
}

// ServerUtils Implementation

namespace ServerUtils {
bool is_port_available(int port) {
    // TODO: Implement actual port checking
    return true;
}

ResourceUsage get_resource_usage() {
    // TODO: Implement actual resource monitoring
    return {0.0, 0, 0, 0};
}

Result<void> validate_environment() {
    // TODO: Implement environment validation
    return Success();
}

Result<void> setup_logging(const ServerConfig &config) {
    // TODO: Implement comprehensive logging setup
    Log::info("Logging initialized for {}", config.mud_name);
    return Success();
}
} // namespace ServerUtils
#include "mud_server.hpp"

#include "../core/actor.hpp"
#include "../core/class_config.hpp"
#include "../game/player_output.hpp"
#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "../database/config_loader.hpp"
#include "../database/connection_pool.hpp"
#include "../database/database_config.hpp"
#include "../text/string_utils.hpp"
#include "configuration_manager.hpp"
#include "network_manager.hpp"
#include "persistence_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
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
            config.log_level = game.value("log_level", config.log_level);
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

Result<ServerConfig> ServerConfig::load_from_database() {
    using namespace fierymud::config;

    Log::info("Loading server configuration from database...");

    // Load all config from database
    auto &loader = ConfigLoader::instance();
    auto load_result = loader.load_from_database();
    if (!load_result) {
        return std::unexpected(Errors::SystemError(
            fmt::format("Failed to load config from database: {}", load_result.error())));
    }

    ServerConfig config;

    // Network settings from "server" category
    config.port = loader.get_int_or("server", "port", 4003);  // v3 uses 4003, legacy uses 4000
    config.tls_port = loader.get_int_or("server", "tls_port", 4443);
    config.max_connections = loader.get_int_or("server", "max_connections", 200);
    config.connection_timeout =
        loader.get_seconds("server", "connection_timeout_seconds").value_or(std::chrono::seconds{300});
    config.target_tps = loader.get_int_or("server", "target_tps", 10);
    config.max_command_queue_size =
        static_cast<size_t>(loader.get_int_or("server", "max_command_queue_size", 10000));

    // Persistence settings
    config.auto_save_interval =
        loader.get_seconds("server", "auto_save_interval_seconds").value_or(std::chrono::seconds{300});
    config.backup_interval =
        loader.get_seconds("server", "backup_interval_seconds").value_or(std::chrono::seconds{3600});
    config.max_backups = loader.get_int_or("server", "max_backups", 24);

    // Display settings
    config.mud_name = loader.get_string_or("display", "mud_name", "FieryMUD");
    config.default_starting_room =
        static_cast<uint64_t>(loader.get_int_or("display", "default_starting_room", 3001));

    // Security settings
    config.max_login_attempts = loader.get_int_or("security", "max_login_attempts", 3);
    config.login_timeout =
        loader.get_minutes("security", "login_timeout_minutes").value_or(std::chrono::minutes{15});
    config.enable_new_player_creation = loader.get_bool_or("security", "enable_new_player_creation", true);
    config.enable_debug_commands = loader.get_bool_or("security", "enable_debug_commands", false);
    config.enable_tls = loader.get_bool_or("security", "enable_tls", true);

    // TLS certificate paths - stay in .env or use defaults (security-sensitive)
    const char *tls_cert = std::getenv("TLS_CERTIFICATE_FILE");
    const char *tls_key = std::getenv("TLS_PRIVATE_KEY_FILE");
    const char *tls_dh = std::getenv("TLS_DH_PARAMS_FILE");
    config.tls_certificate_file = tls_cert ? tls_cert : "certs/server.crt";
    config.tls_private_key_file = tls_key ? tls_key : "certs/server.key";
    config.tls_dh_params_file = tls_dh ? tls_dh : "certs/dhparams.pem";

    // Paths stay in .env or use defaults (needed before database is available)
    const char *log_dir = std::getenv("LOG_DIRECTORY");
    config.log_directory = log_dir ? log_dir : "logs";

    // World/player directories are N/A for database-driven MUD (data in PostgreSQL)
    // Keep defaults for any legacy code that might reference them
    config.world_directory = "data/world";
    config.player_directory = "data/players";

    // Log level from environment (needed before database)
    const char *log_level = std::getenv("LOG_LEVEL");
    config.log_level = log_level ? log_level : "info";

    // Validate the loaded configuration
    auto validation_result = config.validate();
    if (!validation_result) {
        return std::unexpected(validation_result.error());
    }

    Log::info("Server configuration loaded from database ({} entries)", loader.entry_count());
    return config;
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
                     {"log_level", log_level},
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
    // PersistenceManager is a singleton, accessed via PersistenceManager::instance()
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

    // Initialize database connection pool before game systems
    auto db_result = initialize_database();
    if (!db_result) {
        set_state(ServerState::Error);
        return db_result;
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
                // Use run() but check for stopped periodically
                while (!io_context_.stopped()) {
                    // Run handlers for up to 100ms, then check if stopped
                    io_context_.run_for(std::chrono::milliseconds(100));
                }
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

void ModernMUDServer::stop(bool exit_process) {
    if (!is_running() && state_.load() != ServerState::Starting) {
        return;
    }

    set_state(ServerState::Stopping);
    shutdown_requested_.store(true);

    Log::info("Stopping Modern FieryMUD Server...");

    // CRITICAL: Signal world server to stop accepting new strand work BEFORE network cleanup.
    // When network_manager_->stop() closes connections, the cleanup code posts work to the
    // world strand (remove_player_connection). If running_ is still true, that work will be
    // accepted and try to run during/after shutdown, causing io_context to hang waiting for
    // strand work that never completes.
    if (world_server_) {
        world_server_->begin_shutdown();  // Sets running_ = false, cancels timers
    }

    // Stop networking - disconnect all players
    // Any cleanup work posted to the strand will now be rejected (running_ = false)
    if (network_manager_) {
        network_manager_->stop();
    }

    // Complete world server shutdown (cleanup active connections, save state)
    if (world_server_) {
        world_server_->stop();
    }

    // Release work guard so io_context can stop naturally when work is done
    if (work_guard_) {
        work_guard_.reset();
    }

    // Stop the io_context - this causes io_context_.run() to return
    io_context_.stop();

    // Now join the io_thread - it should complete quickly since io_context is stopped
    if (io_thread_.joinable()) {

        // Use a shared_ptr for the flag so it stays valid even if we detach the watcher
        auto join_completed = std::make_shared<std::atomic<bool>>(false);
        std::thread join_watcher([this, join_completed]() {
            io_thread_.join();
            join_completed->store(true);
        });

        // Wait up to 2 seconds for the join to complete
        auto start = std::chrono::steady_clock::now();
        while (!join_completed->load()) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > std::chrono::seconds(2)) {
                Log::error("io_thread failed to stop within 2 seconds - forcing termination");
                // Detach the watcher thread - it holds a shared_ptr to keep the flag alive
                join_watcher.detach();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (join_completed->load() && join_watcher.joinable()) {
            join_watcher.join();
            Log::info("io_thread finished cleanly");
        } else {
            // io_thread didn't stop - we have a detached watcher thread that may cause
            // issues during static destruction. Use quick_exit to avoid segfault.
            Log::warn("io_thread didn't stop cleanly - using quick_exit to avoid segfault");

            // Cleanup what we can
            set_state(ServerState::Stopped);
            Log::info("Modern FieryMUD Server stopped (forced)");
            Logger::shutdown();

            // Use quick_exit to skip static destructors (which would segfault)
            std::quick_exit(0);
        }
    } else {
        Log::info("io_thread not joinable");
    }

    // Clean up resources
    cleanup_resources();

    set_state(ServerState::Stopped);
    Log::info("Modern FieryMUD Server stopped");

    // Flush all logs before shutdown
    Logger::shutdown();

    if (exit_process) {
        // Use quick_exit to skip static destruction order issues.
        // This is safe because we've properly cleaned up all resources:
        // - Players disconnected and saved
        // - Scripting systems shut down (coroutines, triggers, Lua state)
        // - Database connections closed
        // - Logger flushed
        // Static destructors would only cause problems (e.g., singletons trying
        // to log after Logger is destroyed).
        std::quick_exit(0);
    }
    // When exit_process=false (restart case), we return normally
}

void ModernMUDServer::restart() {
    Log::info("Restarting Modern FieryMUD Server...");
    stop(false);  // Don't exit process, we want to start again

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
        player->send_message(std::string(message) + "\r\n");
        Log::debug("Broadcasted to {}: {}", player->name(), message);
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

Result<void> ModernMUDServer::initialize_database() {
    auto logger = Log::database();
    logger->info("Initializing database connection pool");

    // Load database configuration from .env file
    auto db_config_result = DatabaseConfig::from_env("../.env");
    if (!db_config_result) {
        logger->error("Failed to load database configuration: {}", db_config_result.error().message);
        return std::unexpected(db_config_result.error());
    }

    logger->info("Database configuration loaded: {}:{}/{}",
                db_config_result->host, db_config_result->port, db_config_result->dbname);

    // Initialize connection pool with 10 connections
    auto pool_init = ConnectionPool::instance().initialize(*db_config_result, 10);
    if (!pool_init) {
        logger->error("Failed to initialize connection pool: {}", pool_init.error().message);
        return std::unexpected(pool_init.error());
    }

    logger->info("Database connection pool initialized successfully");
    return Success();
}

Result<void> ModernMUDServer::initialize_game_systems() {
    // Initialize class configuration registry (for spell slots, abilities, etc.)
    fierymud::ClassConfigRegistry::instance().initialize_defaults();

    // Initialize world server (which handles game systems internally)
    auto world_result = world_server_->initialize();
    if (!world_result) {
        return world_result;
    }

    // Set shutdown callback so WorldServer can trigger graceful shutdown
    world_server_->set_shutdown_callback([this]() {
        Log::info("Shutdown callback triggered from WorldServer");
        stop();
    });

    return Success();
}

Result<void> ModernMUDServer::initialize_networking() {
    // Connect network manager to world server
    network_manager_->set_world_server(world_server_.get());

    return network_manager_->initialize();
}

Result<void> ModernMUDServer::initialize_persistence() { return PersistenceManager::instance().initialize(config_); }

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
    // Save all players using singleton PersistenceManager
    auto persist_result = PersistenceManager::instance().save_all_players();
    if (!persist_result) {
        return persist_result;
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

// Player management
std::vector<std::shared_ptr<Player>> ModernMUDServer::get_online_players() const {
    if (!world_server_) {
        return {};
    }
    return world_server_->get_online_players();
}

std::shared_ptr<Player> ModernMUDServer::find_player(std::string_view name) const {
    auto players = get_online_players();
    for (const auto& player : players) {
        if (player && player->name() == name) {
            return player;
        }
    }
    return nullptr;
}

size_t ModernMUDServer::online_player_count() const {
    return get_online_players().size();
}

void ModernMUDServer::kick_player(std::string_view player_name, std::string_view reason) {
    auto player = find_player(player_name);
    if (!player) {
        Log::warn("Attempt to kick non-existent player: {}", player_name);
        return;
    }

    // Send kick message to player
    std::string kick_msg = fmt::format("\r\nYou have been disconnected by an administrator.\r\n");
    if (!reason.empty()) {
        kick_msg += fmt::format("Reason: {}\r\n", reason);
    }
    player->send_message(kick_msg);

    // Disconnect the player's connection
    if (auto output = player->get_output()) {
        output->disconnect(std::string(reason));
    }

    Log::info("Player {} kicked. Reason: {}", player_name, reason.empty() ? "none given" : std::string(reason));
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
    // Explicitly shutdown the database connection pool to avoid
    // static destruction order issues with logging
    ConnectionPool::instance().shutdown();
}

Result<void> ModernMUDServer::reload_config() {
    Log::info("Reloading server configuration...");

    // Re-read configuration file
    auto config_result = ServerConfig::load_from_file("config/server.json");
    if (!config_result) {
        Log::error("Failed to reload configuration: {}", config_result.error().message);
        return std::unexpected(config_result.error());
    }

    // Update the server configuration
    config_ = config_result.value();

    // Re-setup logging with new configuration
    auto logging_result = ServerUtils::setup_logging(config_);
    if (!logging_result) {
        Log::warn("Failed to update logging configuration: {}", logging_result.error().message);
    }

    // Validate new environment settings
    auto validation_result = ServerUtils::validate_environment();
    if (!validation_result) {
        Log::warn("Environment validation failed after reload: {}", validation_result.error().message);
    }

    Log::info("Configuration reload completed successfully");
    return Success();
}

Result<void> ModernMUDServer::backup_data() {
    Log::info("Starting data backup...");

    // Backup data using singleton PersistenceManager
    auto backup_result = PersistenceManager::instance().backup_data();
    if (!backup_result) {
        return backup_result;
    }

    Log::info("Data backup completed");
    return Success();
}

void ModernMUDServer::update_stats() {
    // Update current connection count
    if (network_manager_) {
        stats_.current_connections = network_manager_->connection_count();
    }

    // Update memory and performance stats
    stats_.peak_connections.store(std::max(stats_.peak_connections.load(), stats_.current_connections.load()));

    // Get world server stats if available
    if (world_server_) {
        auto world_stats = world_server_->get_performance_stats();
        stats_.total_commands.store(world_stats.commands_processed);
    }
}

// ServerUtils Implementation

namespace ServerUtils {
bool is_port_available(int port) {
    // Try to bind to the port to check availability
    try {
        asio::io_context io_context;
        asio::ip::tcp::acceptor acceptor(io_context);
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);

        acceptor.open(endpoint.protocol());
        acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.close();

        return true;
    } catch (const std::exception &e) {
        Log::debug("Port {} is not available: {}", port, e.what());
        return false;
    }
}

ResourceUsage get_resource_usage() {
    ResourceUsage usage = {0.0, 0, 0, 0};

    try {
        // Memory usage using /proc/self/status (Linux specific)
        std::ifstream status("/proc/self/status");
        if (status.is_open()) {
            std::string line;
            while (std::getline(status, line)) {
                if (line.find("VmRSS:") == 0) {
                    std::istringstream iss(line);
                    std::string label, value, unit;
                    iss >> label >> value >> unit;
                    usage.memory_mb = std::stoul(value) / 1024; // Convert KB to MB
                    break;
                }
            }
        }

        // CPU usage is more complex to calculate, leave as 0.0 for now
        usage.cpu_percentage = 0.0;

        // Network connections - count current network connections
        usage.network_connections = 0; // Could count via /proc/net/tcp if needed

        // Disk usage
        try {
            auto space_info = std::filesystem::space(".");
            usage.disk_free_mb = space_info.available / (1024 * 1024);
        } catch (...) {
            usage.disk_free_mb = 0;
        }

    } catch (const std::exception &e) {
        Log::debug("Error getting resource usage: {}", e.what());
    }

    return usage;
}

Result<void> validate_environment() {
    // Check required directories exist
    std::vector<std::string> required_dirs = {"data", "lib", "lib/etc", "lib/text"};

    for (const auto &dir : required_dirs) {
        if (!std::filesystem::exists(dir)) {
            return std::unexpected(Errors::FileSystem(fmt::format("Required directory '{}' does not exist", dir)));
        }
        if (!std::filesystem::is_directory(dir)) {
            return std::unexpected(Errors::FileSystem(fmt::format("Required path '{}' is not a directory", dir)));
        }
    }

    // Check write permissions to data directory
    std::filesystem::path data_dir = "data";
    auto perms = std::filesystem::status(data_dir).permissions();
    if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
        return std::unexpected(Errors::FileSystem("Data directory is not writable"));
    }

    // Check for minimum required files
    std::vector<std::string> required_files = {"lib/etc/motd", "lib/etc/news", "lib/etc/policy"};

    for (const auto &file : required_files) {
        if (!std::filesystem::exists(file)) {
            Log::warn("Recommended file '{}' does not exist", file);
        }
    }

    Log::debug("Environment validation passed");
    return Success();
}

Result<void> setup_logging(const ServerConfig &config) {
    // Parse log level string to enum
    LogLevel level = LogLevel::Info; // default
    std::string level_str = to_lowercase(config.log_level);

    if (level_str == "trace") {
        level = LogLevel::Trace;
    } else if (level_str == "debug") {
        level = LogLevel::Debug;
    } else if (level_str == "info") {
        level = LogLevel::Info;
    } else if (level_str == "warn" || level_str == "warning") {
        level = LogLevel::Warning;
    } else if (level_str == "error") {
        level = LogLevel::Error;
    } else if (level_str == "critical") {
        level = LogLevel::Critical;
    } else if (level_str == "off") {
        level = LogLevel::Off;
    }

    // Initialize logging with the configured level
    std::string log_file = config.log_directory + "/fierymud.log";
    Logger::initialize(log_file, level, true);

    Log::info("Logging initialized for {} with level: {}", config.mud_name, config.log_level);
    return Success();
}
} // namespace ServerUtils
#include "core/logging.hpp"
#include "core/result.hpp"
#include "server/mud_server.hpp"
#include "admin/admin_server.hpp"
#include "admin/zone_reload_handler.hpp"
#include "admin/player_handler.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

// Global server instances for signal handling
std::unique_ptr<ModernMUDServer> g_server;
std::unique_ptr<fierymud::AdminServer> g_admin_server;
std::atomic<bool> g_shutdown_requested{false};
std::atomic<bool> g_reload_requested{false};

/**
 * Print user-friendly suggestions based on error type.
 * Helps users diagnose and fix common startup issues.
 */
void print_error_suggestions(const Error& error) {
    std::cerr << "\n";

    switch (error.code) {
    case ErrorCode::NetworkError:
        // Database connection failure - most common issue
        std::cerr << "Suggestions:\n";
        std::cerr << "  1. Ensure PostgreSQL is running:\n";
        std::cerr << "     cd /home/strider/Code/mud && docker compose up -d\n";
        std::cerr << "  2. Check database configuration in .env file\n";
        std::cerr << "  3. Verify the database has been seeded:\n";
        std::cerr << "     cd /home/strider/Code/mud/fierylib && poetry run fierylib import-legacy\n";
        std::cerr << "  4. Check container status:\n";
        std::cerr << "     docker compose ps\n";
        break;

    case ErrorCode::ParseError:
        std::cerr << "Suggestions:\n";
        std::cerr << "  1. Check .env file for valid DATABASE_URL\n";
        break;

    case ErrorCode::PermissionDenied:
    case ErrorCode::AccessDenied:
        std::cerr << "Suggestions:\n";
        std::cerr << "  1. Verify database user has correct privileges\n";
        std::cerr << "  2. Check PostgreSQL connection settings\n";
        break;

    default:
        // No specific suggestions for other error types
        break;
    }
}

void signal_handler(int signal) {
    // Signal handlers must only set atomic flags - no complex operations!
    // The main loop will handle the actual shutdown/reload.
    switch (signal) {
    case SIGINT:
    case SIGTERM:
        g_shutdown_requested.store(true);
        break;
    case SIGHUP:
        g_reload_requested.store(true);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[]) {
    try {
        std::cout << "Starting Modern FieryMUD Server...\n";

        // Parse command line options
        cxxopts::Options options("fierymud", "Modern FieryMUD Server v3.0");

        options.add_options()("h,help", "Show help message")(
            "c,config", "Configuration file path", cxxopts::value<std::string>()->default_value("config/prod.json"))(
            "p,port", "Port number to listen on", cxxopts::value<std::string>()->default_value("4003"))(
            "l,log-level", "Log level (debug, info, warn, error)", cxxopts::value<std::string>()->default_value("info"))(
            "s,tls", "Enable TLS/SSL support")(
            "d,daemon", "Run as daemon")("t,test", "Test configuration and exit")("v,version",
                                                                                  "Show version information");

        auto result = options.parse(argc, argv);

        // Handle help option
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            std::cout << "\nExamples:\n";
            std::cout << "  fierymud                         # Run with default config\n";
            std::cout << "  fierymud -c my_config.json       # Use custom config\n";
            std::cout << "  fierymud -p 4003                 # Default port (v3), legacy uses 4000\n";
            std::cout << "  fierymud -l debug                # Enable debug logging\n";
            return 0;
        }

        // Handle version option
        if (result.count("version")) {
            std::cout << "Modern FieryMUD Server v3.0\n";
            std::cout << "Built with C++23 and modern libraries\n";
            std::cout << "Copyright (C) 1998-2025 Fiery Consortium\n";
            return 0;
        }

        // Extract configuration
        ServerConfig config;
        config.port = std::stoi(result["port"].as<std::string>());
        config.log_level = result["log-level"].as<std::string>();

        std::string config_file = result["config"].as<std::string>();

        // Load configuration file if it exists
        if (std::filesystem::exists(config_file)) {
            auto load_result = ServerConfig::load_from_file(config_file);
            if (!load_result) {
                std::cerr << "Failed to load configuration: " << load_result.error().message << "\n";
                return 1;
            }
            config = load_result.value();

            // Override port if specified on command line
            if (result.count("port")) {
                config.port = std::stoi(result["port"].as<std::string>());
            }
            // Override log level if specified on command line
            if (result.count("log-level")) {
                config.log_level = result["log-level"].as<std::string>();
            }
            
            // Override TLS setting if specified on command line
            if (result.count("tls")) {
                config.enable_tls = true;
            }
        } else {
            std::cout << "Configuration file not found: " << config_file << "\n";
            std::cout << "Using default configuration with port " << config.port << "\n";
            
            // Apply TLS setting even without config file
            if (result.count("tls")) {
                config.enable_tls = true;
            }
        }

        // Test mode
        if (result.count("test")) {
            std::cout << "Testing configuration...\n";
            auto validate_result = config.validate();
            if (!validate_result) {
                std::cerr << "Configuration validation failed: " << validate_result.error().message << "\n";
                return 1;
            }
            std::cout << "Configuration is valid.\n";
            return 0;
        }

        // Setup signal handlers
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        std::signal(SIGHUP, signal_handler);
        std::signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE (broken connections)

        // Create and initialize server
        g_server = std::make_unique<ModernMUDServer>(config);

        auto init_result = g_server->initialize();
        if (!init_result) {
            std::cerr << "Server initialization failed: " << init_result.error().message << "\n";
            print_error_suggestions(init_result.error());
            // Clean up the server before exiting to avoid use-after-free during static destruction
            g_server.reset();
            Logger::shutdown();
            return 1;
        }

        // Start the server
        auto start_result = g_server->start();
        if (!start_result) {
            std::cerr << "Server start failed: " << start_result.error().message << "\n";
            print_error_suggestions(start_result.error());
            // Clean up the server before exiting to avoid use-after-free during static destruction
            g_server.reset();
            Logger::shutdown();
            return 1;
        }

        std::cout << "Server running on port " << config.port << ". Press Ctrl+C to stop.\n";

        // Start admin API server on port 8080
        constexpr uint16_t admin_port = 8080;
        g_admin_server = std::make_unique<fierymud::AdminServer>(admin_port, "127.0.0.1");

        // Register admin handlers
        // Note: WorldManager access would require getting it from ModernMUDServer
        // For now, we register player handlers which use ModernMUDServer directly
        fierymud::register_player_handlers(*g_admin_server, *g_server);

        g_admin_server->start();
        std::cout << "Admin API running on port " << admin_port << " (localhost only).\n";

        // Main server loop - check for shutdown/reload signals
        while (g_server->is_running() && !g_shutdown_requested.load()) {
            // Check for reload request
            if (g_reload_requested.exchange(false)) {
                std::cout << "Reloading configuration...\n";
                auto result = g_server->reload_config();
                if (!result) {
                    std::cerr << "Failed to reload config: " << result.error().message << "\n";
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Handle shutdown outside of signal context
        if (g_shutdown_requested.load()) {
            std::cout << "\nShutting down...\n";
            if (g_admin_server) {
                g_admin_server->stop();
            }
            if (g_server) {
                g_server->stop();
            }
        }

        std::cout << "Server shutdown completed.\n";
        return 0;

    } catch (const cxxopts::exceptions::exception &e) {
        std::cerr << "Command line error: " << e.what() << std::endl;
        g_server.reset();
        g_admin_server.reset();
        Logger::shutdown();
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        g_server.reset();
        g_admin_server.reset();
        Logger::shutdown();
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred\n";
        g_server.reset();
        g_admin_server.reset();
        Logger::shutdown();
        return 1;
    }
}
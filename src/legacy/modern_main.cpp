/***************************************************************************
 *   File: src/modern_main.cpp                            Part of FieryMUD *
 *  Usage: Modern FieryMUD server main entry point                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "core/logging.hpp"
#include "server/mud_server.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

// Global server instance for signal handling
std::unique_ptr<ModernMUDServer> g_server;

void signal_handler(int signal) {
    switch (signal) {
    case SIGINT:
    case SIGTERM:
        std::cout << "\nReceived shutdown signal, stopping server...\n";
        if (g_server) {
            g_server->stop();
        }
        break;
    case SIGHUP:
        std::cout << "Received SIGHUP, reloading configuration...\n";
        if (g_server) {
            auto result = g_server->reload_config();
            if (!result) {
                std::cerr << "Failed to reload config: " << result.error().message << "\n";
            }
        }
        break;
    default:
        break;
    }
}

void setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGHUP, signal_handler);

    // Ignore SIGPIPE (broken connections)
    std::signal(SIGPIPE, SIG_IGN);
}

void print_usage(const char *program_name) {
    std::cout << "Modern FieryMUD Server\n";
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -c, --config FILE    Configuration file (default: config/modern_mud.json)\n";
    std::cout << "  -p, --port PORT      Override port number\n";
    std::cout << "  -d, --daemon         Run as daemon\n";
    std::cout << "  -t, --test           Test configuration and exit\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -v, --version        Show version information\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << "                           # Run with default config\n";
    std::cout << "  " << program_name << " -c my_config.json        # Use custom config\n";
    std::cout << "  " << program_name << " -p 4002 -d               # Override port and run as daemon\n";
}

void print_version() {
    std::cout << "Modern FieryMUD Server v1.0\n";
    std::cout << "Built with C++23 and modern libraries\n";
    std::cout << "Copyright (C) 1998-2025 Fiery Consortium\n";
}

Result<ServerConfig> parse_arguments(int argc, char *argv[]) {
    ServerConfig config;
    std::string config_file = "config/modern_mud.json";
    bool daemon_mode = false;
    bool test_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "-v" || arg == "--version") {
            print_version();
            std::exit(0);
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 >= argc) {
                return std::unexpected(Errors::InvalidArgument("config", "Missing configuration file"));
            }
            config_file = argv[++i];
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 >= argc) {
                return std::unexpected(Errors::InvalidArgument("port", "Missing port number"));
            }
            try {
                config.port = std::stoi(argv[++i]);
            } catch (const std::exception &) {
                return std::unexpected(Errors::InvalidArgument("port", "Invalid port number"));
            }
        } else if (arg == "-d" || arg == "--daemon") {
            daemon_mode = true;
        } else if (arg == "-t" || arg == "--test") {
            test_mode = true;
        } else {
            return std::unexpected(Errors::InvalidArgument("argument", fmt::format("Unknown argument: {}", arg)));
        }
    }

    // Load configuration file if it exists
    if (std::filesystem::exists(config_file)) {
        auto load_result = ServerConfig::load_from_file(config_file);
        if (!load_result) {
            return load_result;
        }
        config = load_result.value();
    } else {
        std::cout << "Configuration file not found: " << config_file << "\n";
        std::cout << "Creating default configuration...\n";

        // Create directory if needed
        std::filesystem::create_directories(std::filesystem::path(config_file).parent_path());

        auto save_result = config.save_to_file(config_file);
        if (!save_result) {
            std::cerr << "Failed to create default config: " << save_result.error().message << "\n";
            return std::unexpected(save_result.error());
        }

        std::cout << "Default configuration created at: " << config_file << "\n";
        std::cout << "Please review and modify as needed, then restart the server.\n";
        std::exit(0);
    }

    if (test_mode) {
        std::cout << "Testing configuration...\n";
        auto validate_result = config.validate();
        if (!validate_result) {
            std::cerr << "Configuration validation failed: " << validate_result.error().message << "\n";
            return std::unexpected(validate_result.error());
        }
        std::cout << "Configuration is valid.\n";
        std::exit(0);
    }

    if (daemon_mode) {
        // TODO: Implement daemonization
        std::cout << "Daemon mode not yet implemented\n";
    }

    return config;
}

int main(int argc, char *argv[]) {
    try {
        std::cout << "Starting Modern FieryMUD Server...\n";

        // Parse command line arguments and load configuration
        auto config_result = parse_arguments(argc, argv);
        if (!config_result) {
            std::cerr << "Configuration error: " << config_result.error().message << "\n";
            return 1;
        }

        auto config = config_result.value();

        // Setup signal handlers
        setup_signal_handlers();

        // Create and initialize server
        g_server = std::make_unique<ModernMUDServer>(config);

        auto init_result = g_server->initialize();
        if (!init_result) {
            std::cerr << "Server initialization failed: " << init_result.error().message << "\n";
            return 1;
        }

        // Start the server
        auto start_result = g_server->start();
        if (!start_result) {
            std::cerr << "Server start failed: " << start_result.error().message << "\n";
            return 1;
        }

        // Main server loop - wait for shutdown
        while (g_server->is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "Server shutdown completed.\n";
        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred\n";
        return 1;
    }
}
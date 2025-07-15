/***************************************************************************
 *   File: main.cpp                                       Part of FieryMUD *
 *  Usage: Main entry point for FieryMUD                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "comm.hpp"
#include "conf.hpp"
#include "constants.hpp"
#include "db.hpp"
#include "defines.hpp"
#include "logging.hpp"
#include "version.hpp"

#include <cxxopts.hpp>
#include <fmt/format.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// External variables from comm.cpp
extern ush_int port;
extern int scheck;
extern int should_restrict;
extern int restrict_reason;
extern int no_specials;
extern int environment;

// Default values from conf.hpp
extern const char *DFLT_DIR;
extern const char *DFLT_ENV;
extern int DFLT_PORT;

// Function declarations from other modules
void init_flagvectors();
void init_races();
void init_classes();
void init_objtypes();
void init_exp_table();
void boot_world();
void init_game(int port);
void destroy_db();

int main(int argc, char **argv) {
    try {
        cxxopts::Options options("fierymud", "FieryMUD - A Multi-User Dungeon Server");
        
        options.add_options()
            ("d,directory", "Data directory", cxxopts::value<std::string>()->default_value(DFLT_DIR))
            ("e,environment", "Environment (test, dev, prod)", cxxopts::value<std::string>()->default_value(DFLT_ENV))
            ("p,port", "Port number", cxxopts::value<int>()->default_value(std::to_string(DFLT_PORT)))
            ("c,check", "Syntax check mode")
            ("q,quick", "Quick boot mode")
            ("r,restrict", "Restrict game - no new players")
            ("s,suppress", "Suppress special routines")
            ("h,help", "Show help message")
            ("v,version", "Show version information");
        
        // Parse positional argument for port
        options.parse_positional({"port"});
        options.positional_help("[port]");
        
        auto result = options.parse(argc, argv);
        
        if (result.count("help")) {
            fmt::print("{}\n", options.help());
            return 0;
        }
        
        if (result.count("version")) {
            fmt::print("FieryMUD Git Hash: {}\n", get_git_hash());
            return 0;
        }
        
        // Extract values
        std::string dir = result["directory"].as<std::string>();
        std::string env = result["environment"].as<std::string>();
        port = result["port"].as<int>();
        
        
        // Handle flags
        scheck = result.count("check") ? 1 : 0;
        should_restrict = result.count("restrict") ? 1 : 0;
        no_specials = result.count("suppress") ? 1 : 0;
        
        if (should_restrict) {
            restrict_reason = 1; // RESTRICT_ARGUMENT
        }
        
        // Validate port
        if (port <= 1024) {
            fprintf(stderr, "Error: Port number must be greater than 1024.\n");
            return 1;
        }
        
        // Change to data directory
        if (chdir(dir.c_str()) < 0) {
            perror("Fatal error changing to data directory");
            return 1;
        }
        log("Using {} as data directory.", dir);
        
        // Set environment
        if (env == "test") {
            environment = ENV_TEST;
            log("Running in test mode.");
        } else if (env == "dev") {
            environment = ENV_DEV;
            log("Running in dev mode.");
        } else if (env == "prod") {
            environment = ENV_PROD;
            log("Running in production mode.");
        } else {
            log("Unknown environment '{}'; valid choices are 'test', 'dev', and 'prod'.", env);
            return 1;
        }
        
        // Log mode information
        if (scheck) {
            log("Syntax check mode enabled.");
        }
        if (result.count("quick")) {
            log("Quick boot mode.");
        }
        if (should_restrict) {
            log("Restricting game -- no new players allowed.");
        }
        if (no_specials) {
            log("Suppressing assignment of special routines.");
        }
        
        // Initialize game constants
        log("Initializing runtime game constants.");
        init_flagvectors();
        init_races();
        init_classes();
        init_objtypes();
        init_exp_table();
        
        // Start the game or check syntax
        if (scheck) {
            boot_world();
        } else {
            log("Running game on port {}.", port);
            init_game(port);
        }
        
        // Cleanup
        log("Clearing game world.");
        destroy_db();
        
        return 0;
        
    } catch (const cxxopts::exceptions::exception& e) {
        fmt::print(stderr, "Error parsing options: {}\n", e.what());
        return 1;
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}
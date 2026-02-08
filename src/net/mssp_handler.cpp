#include "mssp_handler.hpp"
#include "server/mud_server.hpp"
#include "server/network_manager.hpp"
#include "core/logging.hpp"

MSSPHandler::MSSPHandler(const ServerConfig& config) : config_(config) {
    Log::debug("MSSP Handler initialized");
}

std::vector<uint8_t> MSSPHandler::generate_mssp_data(const NetworkManager* network_manager) const {
    std::vector<uint8_t> data;

    // Start MSSP subnegotiation: IAC SB MSSP_OPTION
    data.push_back(TELNET_IAC);
    data.push_back(TELNET_SB);
    data.push_back(MSSP_OPTION);

    // Add basic server information
    auto basic_info = get_basic_info();
    for (const auto& [key, value] : basic_info) {
        add_mssp_pair(data, key, value);
    }

    // Add features
    auto features = get_features();
    for (const auto& [key, value] : features) {
        add_mssp_pair(data, key, value);
    }

    // Add current statistics (updated with network manager if available)
    if (network_manager) {
        update_statistics(network_manager);
    }
    auto stats = get_statistics();
    for (const auto& [key, value] : stats) {
        add_mssp_pair(data, key, value);
    }

    // End MSSP subnegotiation: IAC SE
    data.push_back(TELNET_IAC);
    data.push_back(TELNET_SE);

    Log::debug("Generated MSSP data: {} bytes", data.size());
    return data;
}

void MSSPHandler::update_statistics(const NetworkManager* network_manager) const {
    if (!network_manager) {
        return;
    }

    // Update player count
    dynamic_stats_["PLAYERS"] = std::to_string(network_manager->connection_count());

    // Update uptime (placeholder - would need actual server start time)
    dynamic_stats_["UPTIME"] = "3600"; // 1 hour in seconds

    Log::debug("Updated MSSP statistics: {} players online", dynamic_stats_["PLAYERS"]);
}

void MSSPHandler::add_mssp_pair(std::vector<uint8_t>& data, const std::string& variable, const std::string& value) const {
    // Add variable marker and variable name
    data.push_back(MSSP_VAR);
    for (char c : variable) {
        data.push_back(static_cast<uint8_t>(c));
    }

    // Add value marker and value
    data.push_back(MSSP_VAL);
    for (char c : value) {
        data.push_back(static_cast<uint8_t>(c));
    }
}

std::unordered_map<std::string, std::string> MSSPHandler::get_basic_info() const {
    return {
        {"NAME", config_.mud_name},
        {"PLAYERS", "0"}, // Will be updated dynamically
        {"UPTIME", "0"},  // Will be updated dynamically

        // Server details
        {"HOSTNAME", "localhost"},  // Could be made configurable
        {"PORT", std::to_string(config_.port)},
        {"CODEBASE", "FieryMUD"},
        {"VERSION", "3.0.0"},

        // Game information
        {"CONTACT", "admin@fierymud.org"},
        {"CREATED", "1998"},
        {"LANGUAGE", "English"},
        {"LOCATION", "United States"},
        {"MINIMUM AGE", "13"},
        {"WEBSITE", "https://fierymud.org"},

        // Game world
        {"AREAS", "50"},      // Placeholder values
        {"HELPFILES", "200"},
        {"MOBILES", "1000"},
        {"OBJECTS", "2000"},
        {"ROOMS", "5000"},
        {"CLASSES", "4"},
        {"LEVELS", "60"},
        {"RACES", "12"},
        {"SKILLS", "100"},
        {"SPELLS", "150"},

        // Game type
        {"GENRE", "Fantasy"},
        {"GAMEPLAY", "Adventure"},
        {"GAMESYSTEM", "Custom"},
        {"INTERMUD", "0"},
        {"STATUS", "Live"},
        {"SUBGENRE", "High Fantasy"},

        // Technical details
        {"FAMILY", "CircleMUD"},
        {"MUDLIB", "FieryMUD"},
    };
}

std::unordered_map<std::string, std::string> MSSPHandler::get_features() const {
    return {
        // Client support
        {"ANSI", "1"},
        {"GMCP", "1"},
        {"MSSP", "1"},
        {"UTF-8", "1"},
        {"256 COLORS", "1"},
        {"TRUECOLOR", "1"},

        // SSL/TLS support - dynamically set based on server configuration
        {"SSL", config_.enable_tls ? "1" : "0"},
        {"TLS", config_.enable_tls ? "1" : "0"},

        // Protocol support
        {"MCCP", "0"},  // MUD Client Compression Protocol - not yet implemented
        {"MXP", "0"},   // MUD Extension Protocol - not yet implemented

        // Game features
        {"ADULT MATERIAL", "0"},
        {"MULTICLASSING", "1"},
        {"NEWBIE FRIENDLY", "1"},
        {"PLAYER CITIES", "0"},
        {"PLAYER CLANS", "1"},
        {"PLAYER CRAFTING", "0"},
        {"PLAYER GUILDS", "1"},
        {"ROLEPLAYING", "1"},
        {"TRAINING", "1"},

        // World features
        {"WORLD ORIGINALITY", "1"}, // Custom world content
    };
}

std::unordered_map<std::string, std::string> MSSPHandler::get_statistics() const {
    std::unordered_map<std::string, std::string> stats = dynamic_stats_;

    // Add any static statistics here
    if (stats.find("PLAYERS") == stats.end()) {
        stats["PLAYERS"] = "0";
    }
    if (stats.find("UPTIME") == stats.end()) {
        stats["UPTIME"] = "0";
    }

    return stats;
}

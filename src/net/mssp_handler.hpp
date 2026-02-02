// MSSP (MUD Server Status Protocol) for crawler discovery

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
struct ServerConfig;
class NetworkManager;

/**
 * @brief MSSP (MUD Server Status Protocol) Handler
 *
 * MSSP is a protocol for advertising MUD server information to clients
 * and MUD listing services. It uses telnet subnegotiation to send
 * key-value pairs containing server details.
 *
 * Protocol specification: https://tintin.mudhalla.net/protocols/mssp/
 */
class MSSPHandler {
public:
    // Telnet constants for MSSP
    static constexpr uint8_t TELNET_IAC = 255;
    static constexpr uint8_t TELNET_SB = 250;
    static constexpr uint8_t TELNET_SE = 240;
    static constexpr uint8_t TELNET_WILL = 251;
    static constexpr uint8_t TELNET_DO = 253;
    static constexpr uint8_t MSSP_OPTION = 70;

    // MSSP variable and value delimiters
    static constexpr uint8_t MSSP_VAR = 1;
    static constexpr uint8_t MSSP_VAL = 2;

    explicit MSSPHandler(const ServerConfig& config);

    // Generate MSSP data based on current server state
    std::vector<uint8_t> generate_mssp_data(const NetworkManager* network_manager = nullptr) const;

    // Update dynamic server statistics (const because dynamic_stats_ is mutable cache)
    void update_statistics(const NetworkManager* network_manager) const;

private:
    const ServerConfig& config_;

    // Server statistics (updated periodically)
    mutable std::unordered_map<std::string, std::string> dynamic_stats_;

    // Add a key-value pair to MSSP data
    void add_mssp_pair(std::vector<uint8_t>& data, const std::string& variable, const std::string& value) const;

    // Get basic server information
    std::unordered_map<std::string, std::string> get_basic_info() const;

    // Get supported features
    std::unordered_map<std::string, std::string> get_features() const;

    // Get current statistics
    std::unordered_map<std::string, std::string> get_statistics() const;
};

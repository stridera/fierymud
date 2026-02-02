// GMCP (Generic MUD Communication Protocol) handler for modern clients

#pragma once

#include <nlohmann/json_fwd.hpp>
#include "core/result.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_set>

// Forward declarations
class PlayerConnection;
class Room;
class Player;

/**
 * @brief Handles GMCP (Generic Mud Communication Protocol) for a player connection
 *
 * GMCP uses telnet option 201 to send structured JSON data between client and server.
 * This enables modern MUD clients to display rich UI elements like health bars,
 * room information, and real-time updates.
 */
class GMCPHandler {
public:
    // Telnet constants for GMCP and protocol
    static constexpr uint8_t TELNET_IAC = 255;
    static constexpr uint8_t TELNET_WILL = 251;
    static constexpr uint8_t TELNET_WONT = 252;
    static constexpr uint8_t TELNET_DO = 253;
    static constexpr uint8_t TELNET_DONT = 254;
    static constexpr uint8_t TELNET_SB = 250;
    static constexpr uint8_t TELNET_SE = 240;
    static constexpr uint8_t TELNET_GA = 249;  // Go Ahead for prompts
    static constexpr uint8_t GMCP_OPTION = 201;

    explicit GMCPHandler(PlayerConnection& connection);
    ~GMCPHandler() = default;

    // GMCP capability management
    bool is_enabled() const { return gmcp_enabled_; }
    void enable() { gmcp_enabled_ = true; }
    void disable() { gmcp_enabled_ = false; }

    // Telnet option negotiation
    Result<void> handle_telnet_option(uint8_t command, uint8_t option);
    Result<void> handle_gmcp_subnegotiation(std::string_view data);

    // GMCP message processing
    Result<void> process_gmcp_message(std::string_view message);
    Result<void> send_gmcp(std::string_view module, const nlohmann::json& data);

    // Core GMCP modules
    void send_hello();
    void send_supports_set();

    // Room module
    void send_room_info(const Room& room);

    // Char module
    void send_char_vitals(const class Player& player);

    // Client capability management
    void add_client_support(std::string_view module);
    bool client_supports(std::string_view module) const;

private:
    PlayerConnection& connection_;
    bool gmcp_enabled_{false};
    std::unordered_set<std::string> client_supports_;

    // Helper methods
    std::string escape_telnet_data(std::string_view data);
    Result<void> send_telnet_option(uint8_t command, uint8_t option);
};

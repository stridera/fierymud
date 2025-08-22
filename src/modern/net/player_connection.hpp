/***************************************************************************
 *   File: src/modern/net/player_connection.hpp     Part of FieryMUD *
 *  Usage: Modern player connection with GMCP and login support               *
 ***************************************************************************/

#pragma once

#include "../core/result.hpp"
#include "../core/ids.hpp"
#include "../game/login_system.hpp"
#include "../game/player_output.hpp"
#include "../world/room.hpp"
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <deque>
#include <unordered_set>

// Forward declarations
class Player;
class WorldServer;

/**
 * @brief Player connection state tracking
 */
enum class ConnectionState {
    Connected,          // Just connected, telnet negotiation
    Login,              // In login process
    Playing,            // Playing the game
    Disconnecting,      // Graceful disconnect in progress
    Disconnected        // Connection closed
};

/**
 * @brief GMCP handler integrated into PlayerConnection
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

    explicit GMCPHandler(class PlayerConnection& connection);
    ~GMCPHandler() = default;

    // GMCP capability management
    bool supports_gmcp() const { return gmcp_enabled_; }
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

    // Client capability management
    void add_client_support(std::string_view module);
    bool client_supports(std::string_view module) const;

private:
    class PlayerConnection& connection_;
    bool gmcp_enabled_{false};
    std::unordered_set<std::string> client_supports_;

    // Helper methods
    std::string escape_telnet_data(std::string_view data);
    Result<void> send_telnet_option(uint8_t command, uint8_t option);
};

/**
 * @brief Modern player connection with GMCP and advanced telnet support
 * 
 * Handles:
 * - Telnet protocol negotiation including GMCP
 * - Complete login and character creation flow
 * - GMCP message routing for modern clients
 * - Thread-safe message sending with Asio strand
 * - Graceful connection lifecycle management
 */
class PlayerConnection : public std::enable_shared_from_this<PlayerConnection>, public PlayerOutput {
public:
    using MessageCallback = std::function<void(std::string_view)>;
    using DisconnectCallback = std::function<void(std::shared_ptr<PlayerConnection>)>;
    
    // Factory method for safe shared_ptr creation
    static std::shared_ptr<PlayerConnection> create(
        asio::io_context& io_context,
        std::shared_ptr<WorldServer> world_server
    );
    
    ~PlayerConnection();
    
    // Connection management
    void start();
    void disconnect(std::string_view reason = "");
    void force_disconnect();
    
    // Socket access for accept operations
    asio::ip::tcp::socket& socket() { return socket_; }
    const asio::ip::tcp::socket& socket() const { return socket_; }
    
    // State queries
    ConnectionState state() const { return state_; }
    bool is_connected() const { return state_ != ConnectionState::Disconnected; }
    bool is_playing() const { return state_ == ConnectionState::Playing; }
    bool has_player() const { return player_ != nullptr; }
    
    // Player access (only valid when playing)
    std::shared_ptr<Player> get_player() const { return player_; }
    
    // Message sending (thread-safe)
    void send_message(std::string_view message);
    void send_prompt(std::string_view prompt = "> ");
    void send_line(std::string_view line);
    
    // GMCP support
    bool supports_gmcp() const { return gmcp_handler_.supports_gmcp(); }
    Result<void> send_gmcp(std::string_view module, const nlohmann::json& data);
    void send_room_info();
    void send_vitals();
    void send_status();
    
    // Raw data sending for telnet protocol
    void send_raw_data(const std::vector<uint8_t>& data);
    
    // Connection info
    std::string remote_address() const;
    std::chrono::steady_clock::time_point connect_time() const { return connect_time_; }
    std::chrono::seconds connection_duration() const;
    
    // Callbacks
    void set_disconnect_callback(DisconnectCallback callback) {
        disconnect_callback_ = std::move(callback);
    }
    
private:
    explicit PlayerConnection(
        asio::io_context& io_context,
        std::shared_ptr<WorldServer> world_server
    );
    
    // Async operation handlers
    void handle_connect();
    void start_read();
    void handle_read(const asio::error_code& error, std::size_t bytes_transferred);
    void handle_write(const asio::error_code& error, std::size_t bytes_transferred);
    
    // Input processing
    void process_input(std::string_view input);
    void process_telnet_data(const std::vector<uint8_t>& data);
    void handle_telnet_command(uint8_t command, const std::vector<uint8_t>& options);
    
    // State management
    void transition_to(ConnectionState new_state);
    void on_login_completed(std::shared_ptr<Player> player);
    void cleanup_connection();
    
    // Output queue management (thread-safe with strand)
    void queue_output(std::string data);
    void flush_output();
    
    // Telnet protocol support
    void send_telnet_negotiation();
    void send_ga(); // Go Ahead for prompt support
    
    // Core components
    asio::io_context& io_context_;
    asio::strand<asio::io_context::executor_type> strand_;
    asio::ip::tcp::socket socket_;
    std::shared_ptr<WorldServer> world_server_;
    
    // Protocol handlers
    GMCPHandler gmcp_handler_;
    std::unique_ptr<LoginSystem> login_system_;
    
    // Connection state
    ConnectionState state_{ConnectionState::Connected};
    std::shared_ptr<Player> player_;
    std::chrono::steady_clock::time_point connect_time_;
    
    // I/O buffers and queues
    std::array<char, 4096> read_buffer_;
    std::string input_buffer_;
    std::deque<std::string> output_queue_;
    bool write_in_progress_{false};
    
    // Telnet state
    bool in_telnet_negotiation_{false};
    std::vector<uint8_t> telnet_buffer_;
    
    // Callbacks
    DisconnectCallback disconnect_callback_;
    
    // Connection limits and timeouts
    static constexpr std::chrono::seconds LOGIN_TIMEOUT{300}; // 5 minutes
    static constexpr std::chrono::seconds IDLE_TIMEOUT{1800}; // 30 minutes
    static constexpr size_t MAX_OUTPUT_QUEUE_SIZE{100};
    static constexpr size_t MAX_INPUT_LINE_LENGTH{512};
};
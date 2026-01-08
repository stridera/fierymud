// Async TCP connection with telnet protocol, GMCP, and MTTS support

#pragma once

#include "../core/ids.hpp"
#include "../core/result.hpp"
#include "../game/login_system.hpp"
#include "../game/player_output.hpp"
#include "../world/room.hpp"
#include "../text/terminal_capabilities.hpp"
#include "tls_context.hpp"

#include <asio.hpp>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

// Forward declarations
class Player;
class WorldServer;
class NetworkManager;

/**
 * @brief Player connection state tracking
 */
enum class ConnectionState {
    Connected,     // Just connected, telnet negotiation
    Login,         // In login process  
    Playing,       // Playing the game
    AFK,           // Away from keyboard (no input for extended period)
    Linkdead,      // Connection lost but player still in game
    Reconnecting,  // Player attempting to reconnect to existing character
    Disconnecting, // Graceful disconnect in progress
    Disconnected   // Connection closed
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
    static constexpr uint8_t TELNET_GA = 249; // Go Ahead for prompts
    static constexpr uint8_t GMCP_OPTION = 201;
    static constexpr uint8_t MSSP_OPTION = 70;
    static constexpr uint8_t TTYPE_OPTION = 24;    // Terminal Type
    static constexpr uint8_t NEW_ENVIRON_OPTION = 39; // NEW-ENVIRON
    static constexpr uint8_t SB_IS = 0;
    static constexpr uint8_t SB_SEND = 1;
    static constexpr uint8_t SB_INFO = 2;
    static constexpr uint8_t NEW_ENVIRON_VAR = 0;
    static constexpr uint8_t NEW_ENVIRON_VALUE = 1;
    static constexpr uint8_t NEW_ENVIRON_ESC = 2;
    static constexpr uint8_t NEW_ENVIRON_USERVAR = 3;

    explicit GMCPHandler(class PlayerConnection &connection);
    ~GMCPHandler() = default;

    // GMCP capability management
    bool supports_gmcp() const { return gmcp_enabled_; }
    void enable();
    void disable() { gmcp_enabled_ = false; }

    // Telnet option negotiation
    Result<void> handle_telnet_option(uint8_t command, uint8_t option);
    Result<void> handle_gmcp_subnegotiation(std::string_view data);

    // GMCP message processing
    Result<void> process_gmcp_message(std::string_view message);
    Result<void> send_gmcp(std::string_view module, const nlohmann::json &data);
    
    // MSSP message processing
    Result<void> handle_mssp_request();
    Result<void> send_mssp_data();

    // Core GMCP modules  
    void send_server_services();
    void send_supports_set();

    // Room module
    void send_room_info(const Room &room);

    // Client capability management
    void add_client_support(std::string_view module);
    bool client_supports(std::string_view module) const;
    
    // Terminal capability detection
    void handle_terminal_type(std::string_view ttype);
    void handle_mtts_capability(uint32_t bitvector);
    void handle_new_environ_data(const std::unordered_map<std::string, std::string>& env_vars);
    const TerminalCapabilities::Capabilities& get_terminal_capabilities() const;
    
    // Request client capabilities
    void request_terminal_type();
    void subnegotiate_terminal_type();

    void request_new_environ();
    void subnegotiate_new_environ();
    std::unordered_map<std::string, std::string> parse_new_environ_data(std::string_view data);

  private:
    class PlayerConnection &connection_;
    bool gmcp_enabled_{false};
    std::unordered_set<std::string> client_supports_;
    TerminalCapabilities::Capabilities terminal_capabilities_;
    std::unique_ptr<class MSSPHandler> mssp_handler_;

    // Helper methods
    std::string escape_telnet_data(std::string_view data);
    Result<void> send_telnet_option(uint8_t command, uint8_t option);
    void merge_terminal_capabilities(const TerminalCapabilities::Capabilities& new_caps);
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

    // Factory methods for safe shared_ptr creation
    static std::shared_ptr<PlayerConnection> create(asio::io_context &io_context,
                                                    std::shared_ptr<WorldServer> world_server,
                                                    NetworkManager* network_manager);
    
    // Factory method for TLS connections
    static std::shared_ptr<PlayerConnection> create_tls(asio::io_context &io_context,
                                                        std::shared_ptr<WorldServer> world_server,
                                                        NetworkManager* network_manager,
                                                        TLSContextManager& tls_manager);

    ~PlayerConnection();

    // Connection management
    void start();
    void disconnect(std::string_view reason = "") override;
    void force_disconnect();

    // Socket access for accept operations
    asio::ip::tcp::socket &socket() { return connection_socket_->tcp_socket(); }
    const asio::ip::tcp::socket &socket() const { return connection_socket_->tcp_socket(); }
    
    // TLS support queries
    bool is_tls_connection() const { return connection_socket_ && connection_socket_->is_tls(); }

    // State queries
    ConnectionState state() const { return state_; }
    bool is_connected() const override { return state_ != ConnectionState::Disconnected; }
    bool is_playing() const { return state_ == ConnectionState::Playing; }
    bool has_player() const { return player_ != nullptr; }
    
    // Enhanced session state queries
    bool is_afk() const { return is_afk_; }
    bool is_linkdead() const { return is_linkdead_; }
    std::chrono::seconds idle_time() const;
    std::chrono::seconds afk_time() const;

    // Player access (only valid when playing)
    std::shared_ptr<Player> get_player() const { return player_; }
    
    // Reconnection support - attach existing player to new connection
    void attach_player(std::shared_ptr<Player> player);

    // Mark connection as replaced by reconnection (prevents cleanup from removing from WorldServer)
    void mark_as_replaced() { was_replaced_ = true; }

    // Message sending (thread-safe)
    void send_message(std::string_view message) override;
    void send_prompt(std::string_view prompt = "> ") override;
    void send_line(std::string_view line) override;

    // GMCP support
    bool supports_gmcp() const { return gmcp_handler_.supports_gmcp(); }
    Result<void> send_gmcp(std::string_view module, const nlohmann::json &data);
    void send_room_info();
    void send_vitals();
    void send_status();
    
    // Terminal capability detection
    const TerminalCapabilities::Capabilities& get_terminal_capabilities() const { 
        return gmcp_handler_.get_terminal_capabilities(); 
    }

    // Raw data sending for telnet protocol
    void send_raw_data(const std::vector<uint8_t> &data);

    // Forward a command to the game (used by LoginSystem for buffered commands after login)
    void forward_command_to_game(std::string_view command);

    // Connection info
    std::string remote_address() const override;
    std::chrono::steady_clock::time_point connect_time() const { return connect_time_; }
    std::chrono::seconds connection_duration() const;
    
    // Network manager access for reconnection handling
    NetworkManager* get_network_manager() const { return network_manager_; }

    // Callbacks
    void set_disconnect_callback(DisconnectCallback callback) { disconnect_callback_ = std::move(callback); }

  protected:
    explicit PlayerConnection(asio::io_context &io_context, std::shared_ptr<WorldServer> world_server, 
                              NetworkManager* network_manager);
    
    // TLS constructor
    explicit PlayerConnection(asio::io_context &io_context, std::shared_ptr<WorldServer> world_server, 
                              NetworkManager* network_manager, TLSContextManager& tls_manager);

    // Async operation handlers
    void handle_connect();
    void start_read();
    void handle_read(const asio::error_code &error, std::size_t bytes_transferred);
    void handle_write(const asio::error_code &error, std::size_t bytes_transferred);

    // Input processing
    void process_input(std::string_view input);
    void process_telnet_data(const std::vector<uint8_t> &data);
    void handle_telnet_command(uint8_t command, const std::vector<uint8_t> &options);

    // State management
    void transition_to(ConnectionState new_state);
    void on_login_completed(std::shared_ptr<Player> player);
    void cleanup_connection();
    
    // Enhanced session management
    void update_last_input_time();
    void check_idle_timeout();
    void set_afk(bool afk);
    void set_linkdead(bool linkdead, std::string_view reason = "");
    void start_idle_timer();
    void handle_idle_timer(const asio::error_code &error);

    // Output queue management (thread-safe with strand)
    void queue_output(std::string data);
    void flush_output();

    // Telnet protocol support
    void send_telnet_negotiation();
    void send_ga(); // Go Ahead for prompt support

    // Core components
    asio::io_context &io_context_;
    asio::strand<asio::io_context::executor_type> strand_;
    std::unique_ptr<TLSSocket> connection_socket_;
    std::shared_ptr<WorldServer> world_server_;
    NetworkManager* network_manager_;
    asio::steady_timer idle_check_timer_;

    // Protocol handlers
    GMCPHandler gmcp_handler_;
    std::unique_ptr<LoginSystem> login_system_;

    // Connection state
    ConnectionState state_{ConnectionState::Connected};
    std::shared_ptr<Player> player_;
    std::chrono::steady_clock::time_point connect_time_;
    
    // Enhanced session management
    std::chrono::steady_clock::time_point last_input_time_{std::chrono::steady_clock::now()};
    std::chrono::steady_clock::time_point afk_start_time_{};
    bool is_afk_{false};
    bool is_linkdead_{false};
    bool was_replaced_{false};  // Set when connection is replaced by reconnection
    std::string disconnect_reason_;
    std::string original_host_;  // For reconnection validation

    // Connection limits and timeouts (declared first as some are used below)
    static constexpr std::chrono::seconds LOGIN_TIMEOUT{300}; // 5 minutes
    static constexpr std::chrono::seconds IDLE_TIMEOUT{1800}; // 30 minutes
    static constexpr std::chrono::seconds AFK_TIMEOUT{900};   // 15 minutes for AFK detection
    static constexpr std::chrono::seconds LINKDEAD_TIMEOUT{180}; // 3 minutes before going linkdead
    static constexpr size_t MAX_OUTPUT_QUEUE_SIZE{500};
    static constexpr size_t MAX_INPUT_LINE_LENGTH{512};
    static constexpr size_t MAX_TELNET_SUBNEG_LENGTH{8192}; // Max GMCP/subneg message size
    static constexpr size_t READ_BUFFER_SIZE{4096}; // Socket read buffer size

    // I/O buffers and queues
    std::array<char, READ_BUFFER_SIZE> read_buffer_;
    std::string input_buffer_;
    std::deque<std::string> output_queue_;
    bool write_in_progress_{false};

    // Telnet state
    bool in_telnet_negotiation_{false};
    std::vector<uint8_t> telnet_buffer_;

    // Callbacks
    DisconnectCallback disconnect_callback_;
};

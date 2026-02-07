#pragma once

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
    void set_tls_status(bool using_tls) { terminal_capabilities_.supports_tls = using_tls; }

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


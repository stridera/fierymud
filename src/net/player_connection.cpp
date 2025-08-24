/***************************************************************************
 *   File: src/net/player_connection.cpp     Part of FieryMUD *
 *  Usage: Modern player connection implementation                            *
 ***************************************************************************/

#include "player_connection.hpp"

#include "../commands/command_system.hpp"
#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "../server/world_server.hpp"
#include "../world/world_manager.hpp"

#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <regex>

// GMCPHandler implementation
GMCPHandler::GMCPHandler(PlayerConnection &connection) : connection_(connection) {}

Result<void> GMCPHandler::handle_telnet_option(uint8_t command, uint8_t option) {
    if (option != GMCP_OPTION) {
        return Result<void>{}; // Not our option, ignore
    }

    switch (command) {
    case TELNET_WILL:
        // Client wants to use GMCP
        enable();
        return send_telnet_option(TELNET_DO, GMCP_OPTION);

    case TELNET_WONT:
        // Client doesn't want GMCP
        disable();
        return send_telnet_option(TELNET_DONT, GMCP_OPTION);

    case TELNET_DO:
        // Client wants us to use GMCP (we already do)
        enable();
        send_hello();
        send_supports_set();
        return Result<void>{};

    case TELNET_DONT:
        // Client doesn't want us to use GMCP
        disable();
        return Result<void>{};

    default:
        return std::unexpected(Error{ErrorCode::InvalidArgument, fmt::format("Unknown telnet command: {}", command)});
    }
}

Result<void> GMCPHandler::handle_gmcp_subnegotiation(std::string_view data) {
    if (!gmcp_enabled_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "GMCP not enabled"});
    }

    return process_gmcp_message(data);
}

Result<void> GMCPHandler::process_gmcp_message(std::string_view message) {
    try {
        auto json_data = nlohmann::json::parse(message);

        // Handle common GMCP client messages
        if (json_data.contains("Core.Supports.Set")) {
            auto supports = json_data["Core.Supports.Set"];
            for (const auto &module : supports) {
                add_client_support(module.get<std::string>());
            }
            Log::debug("GMCP client supports {} modules", client_supports_.size());
        } else if (json_data.contains("Core.Hello")) {
            Log::info("GMCP client hello: {}", json_data["Core.Hello"].dump());
        }

        return Result<void>{};
    } catch (const std::exception &e) {
        return std::unexpected(Error{ErrorCode::ParseError, fmt::format("Failed to parse GMCP message: {}", e.what())});
    }
}

Result<void> GMCPHandler::send_gmcp(std::string_view module, const nlohmann::json &data) {
    if (!gmcp_enabled_) {
        return Result<void>{}; // Skip if GMCP not enabled
    }

    try {
        nlohmann::json message;
        message[std::string(module)] = data;

        std::string json_str = message.dump();
        std::string escaped = escape_telnet_data(json_str);

        // Send GMCP subnegotiation: IAC SB GMCP_OPTION <data> IAC SE
        std::vector<uint8_t> gmcp_data;
        gmcp_data.push_back(TELNET_IAC);
        gmcp_data.push_back(TELNET_SB);
        gmcp_data.push_back(GMCP_OPTION);

        for (char c : escaped) {
            gmcp_data.push_back(static_cast<uint8_t>(c));
        }

        gmcp_data.push_back(TELNET_IAC);
        gmcp_data.push_back(TELNET_SE);

        connection_.send_raw_data(gmcp_data);
        return Result<void>{};
    } catch (const std::exception &e) {
        return std::unexpected(Error{ErrorCode::SerializationError, fmt::format("Failed to send GMCP: {}", e.what())});
    }
}

void GMCPHandler::send_hello() {
    nlohmann::json hello = {{"client", "FieryMUD"},
                            {"version", "3.0.0-modern"},
                            {"features", nlohmann::json::array({"GMCP", "MXP", "UTF-8"})}};

    send_gmcp("Core.Hello", hello);
}

void GMCPHandler::send_supports_set() {
    nlohmann::json supports = nlohmann::json::array({"Core 1", "Room 1", "Char 1", "Char.Vitals 1", "Char.Status 1"});

    send_gmcp("Core.Supports.Set", supports);
}

void GMCPHandler::send_room_info(const Room &room) {
    if (!client_supports("Room"))
        return;

    nlohmann::json room_data = {{"num", room.id().value()},
                                {"name", room.name()},
                                {"zone", "Unknown"}, // TODO: Add zone support to Room
                                {"environment", static_cast<int>(room.sector_type())},
                                {"coord",
                                 {{"x", 0}, // TODO: Implement coordinates when available
                                  {"y", 0},
                                  {"z", 0}}},
                                {"exits", nlohmann::json::object()}};

    // TODO: Add exits when Room interface supports iteration

    send_gmcp("Room.Info", room_data);
}

void GMCPHandler::add_client_support(std::string_view module) { client_supports_.emplace(module); }

bool GMCPHandler::client_supports(std::string_view module) const {
    return client_supports_.find(std::string(module)) != client_supports_.end();
}

std::string GMCPHandler::escape_telnet_data(std::string_view data) {
    std::string escaped;
    escaped.reserve(data.size() * 2); // Worst case: every byte is IAC

    for (char c : data) {
        if (static_cast<uint8_t>(c) == TELNET_IAC) {
            escaped.push_back(static_cast<char>(TELNET_IAC));
            escaped.push_back(static_cast<char>(TELNET_IAC)); // Double IAC to escape
        } else {
            escaped.push_back(c);
        }
    }

    return escaped;
}

Result<void> GMCPHandler::send_telnet_option(uint8_t command, uint8_t option) {
    std::vector<uint8_t> data = {TELNET_IAC, command, option};
    connection_.send_raw_data(data);
    return Result<void>{};
}

// PlayerConnection implementation
std::shared_ptr<PlayerConnection> PlayerConnection::create(asio::io_context &io_context,
                                                           std::shared_ptr<WorldServer> world_server) {

    // Use private constructor with shared_ptr
    return std::shared_ptr<PlayerConnection>(new PlayerConnection(io_context, std::move(world_server)));
}

PlayerConnection::PlayerConnection(asio::io_context &io_context, std::shared_ptr<WorldServer> world_server)
    : io_context_(io_context), strand_(asio::make_strand(io_context)), socket_(io_context),
      world_server_(std::move(world_server)), gmcp_handler_(*this), connect_time_(std::chrono::steady_clock::now()) {}

PlayerConnection::~PlayerConnection() { cleanup_connection(); }

void PlayerConnection::start() {
    connect_time_ = std::chrono::steady_clock::now();

    Log::info("New connection from {}", remote_address());

    // Initialize login system
    login_system_ = std::make_unique<LoginSystem>(shared_from_this());
    login_system_->set_player_loaded_callback([this](std::shared_ptr<Player> player) { on_login_completed(player); });

    handle_connect();
}

void PlayerConnection::handle_connect() {
    transition_to(ConnectionState::Login);

    // Send immediate welcome to keep connection alive
    send_line("=== Welcome to Modern FieryMUD ===");

    // Begin reading
    start_read();

    // Send telnet negotiation and start login
    send_telnet_negotiation();
    login_system_->start_login();
}

void PlayerConnection::send_telnet_negotiation() {
    // Offer GMCP support: IAC WILL GMCP_OPTION
    std::vector<uint8_t> gmcp_offer = {GMCPHandler::TELNET_IAC, GMCPHandler::TELNET_WILL, GMCPHandler::GMCP_OPTION};
    send_raw_data(gmcp_offer);
}

void PlayerConnection::start_read() {
    if (!is_connected())
        return;

    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(read_buffer_),
        asio::bind_executor(strand_, [this, self](const asio::error_code &error, std::size_t bytes_transferred) {
            handle_read(error, bytes_transferred);
        }));
}

void PlayerConnection::handle_read(const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        // Only disconnect for serious errors, not temporary conditions
        if (error == asio::error::eof || error == asio::error::connection_reset ||
            error == asio::error::connection_aborted) {
            Log::debug("Connection closed from {}: {}", remote_address(), error.message());
            disconnect("Connection closed");
        } else if (error == asio::error::operation_aborted) {
            // Operation was cancelled, ignore
            return;
        } else {
            // For other errors, log but try to continue
            Log::debug("Read error from {} (continuing): {}", remote_address(), error.message());
            // Continue reading instead of disconnecting
            start_read();
        }
        return;
    }

    // Process received data
    std::vector<uint8_t> data(read_buffer_.begin(), read_buffer_.begin() + bytes_transferred);
    process_telnet_data(data);

    // Continue reading
    start_read();
}

void PlayerConnection::process_telnet_data(const std::vector<uint8_t> &data) {
    for (size_t i = 0; i < data.size(); ++i) {
        uint8_t byte = data[i];

        if (byte == GMCPHandler::TELNET_IAC && !in_telnet_negotiation_) {
            // Start of telnet command
            in_telnet_negotiation_ = true;
            telnet_buffer_.clear();
            telnet_buffer_.push_back(byte);
        } else if (in_telnet_negotiation_) {
            telnet_buffer_.push_back(byte);

            // Check for complete telnet commands
            if (telnet_buffer_.size() >= 3) {
                uint8_t command = telnet_buffer_[1];
                uint8_t option = telnet_buffer_[2];

                if (command == GMCPHandler::TELNET_WILL || command == GMCPHandler::TELNET_WONT ||
                    command == GMCPHandler::TELNET_DO || command == GMCPHandler::TELNET_DONT) {

                    // Simple 3-byte command
                    gmcp_handler_.handle_telnet_option(command, option);
                    in_telnet_negotiation_ = false;
                } else if (command == GMCPHandler::TELNET_SB) {
                    // Subnegotiation - look for IAC SE ending
                    if (telnet_buffer_.size() >= 5 &&
                        telnet_buffer_[telnet_buffer_.size() - 2] == GMCPHandler::TELNET_IAC &&
                        telnet_buffer_[telnet_buffer_.size() - 1] == GMCPHandler::TELNET_SE) {

                        // Extract subnegotiation data (skip IAC SB option ... IAC SE)
                        std::string sub_data(telnet_buffer_.begin() + 3, telnet_buffer_.end() - 2);

                        if (option == GMCPHandler::GMCP_OPTION) {
                            gmcp_handler_.handle_gmcp_subnegotiation(sub_data);
                        }

                        in_telnet_negotiation_ = false;
                    }
                }
            }
        } else {
            // Regular text data
            if (byte == '\n' || byte == '\r') {
                if (!input_buffer_.empty()) {
                    process_input(input_buffer_);
                    input_buffer_.clear();
                }
            } else if (byte >= 32 && byte <= 126) { // Printable ASCII
                if (input_buffer_.size() < MAX_INPUT_LINE_LENGTH) {
                    input_buffer_.push_back(static_cast<char>(byte));
                }
            }
        }
    }
}

void PlayerConnection::process_input(std::string_view input) {
    Log::debug("PlayerConnection::process_input: state={}, input='{}'", static_cast<int>(state_), input);

    if (state_ == ConnectionState::Login && login_system_) {
        login_system_->process_input(input);
    } else if (state_ == ConnectionState::Playing && player_) {
        Log::debug("Processing command for player '{}'", player_->name());
        // Route command execution through the world strand to avoid cross-thread access
        if (world_server_) {
            world_server_->process_command(player_, input);
        } else {
            Log::error("World server not available!");
            send_message("World server not available.");
        }
        send_prompt();
    } else {
        Log::warn("Unexpected state in process_input: state={}, has_player={}", static_cast<int>(state_),
                  player_ != nullptr);
    }
}

void PlayerConnection::transition_to(ConnectionState new_state) {
    if (state_ == new_state)
        return;

    Log::debug("Connection {} transitioning from {} to {}", remote_address(), static_cast<int>(state_),
               static_cast<int>(new_state));

    state_ = new_state;
}

void PlayerConnection::on_login_completed(std::shared_ptr<Player> player) {
    player_ = std::move(player);

    // Set the player's output interface
    player_->set_output(shared_from_this());

    // Ensure player is placed in a room
    if (!player_->current_room()) {
        // Try to place in the configured starting room
        auto starting_room_id = Config::instance().default_starting_room();
        auto starting_room = WorldManager::instance().get_room(starting_room_id);
        
        if (starting_room) {
            auto move_result = player_->move_to(starting_room);
            if (!move_result) {
                Log::warn("Failed to place player '{}' in starting room {}: {}", 
                         player_->name(), starting_room_id.value(), move_result.error().message);
            } else {
                starting_room->add_actor(player_);
                Log::info("Placed player '{}' in starting room {}", player_->name(), starting_room_id.value());
            }
        } else {
            Log::error("Starting room {} not found - player '{}' will be in nowhere", 
                      starting_room_id.value(), player_->name());
        }
    }

    // Add player to the world server's online players list
    world_server_->add_player(player_);

    transition_to(ConnectionState::Playing);

    Log::info("Player '{}' logged in from {}", player_->name(), remote_address());

    // Send room description after login
    if (auto room = player_->current_room()) {
        send_message(room->get_room_description(player_.get()));
    }

    // Send initial GMCP data if supported
    if (supports_gmcp()) {
        send_vitals();
        send_status();
        send_room_info();
    }

    // Send welcome to game
    send_message("Welcome to FieryMUD!");
    send_prompt();
}

void PlayerConnection::send_message(std::string_view message) {
    if (!is_connected())
        return;

    queue_output(fmt::format("{}\r\n", message));
}

void PlayerConnection::send_prompt(std::string_view prompt) {
    if (!is_connected())
        return;

    queue_output(std::string(prompt));
    send_ga(); // Send Go Ahead for better client support
}

void PlayerConnection::send_line(std::string_view line) {
    if (!is_connected())
        return;

    queue_output(fmt::format("{}\r\n", line));
}

void PlayerConnection::send_ga() {
    std::vector<uint8_t> ga_data = {GMCPHandler::TELNET_IAC, GMCPHandler::TELNET_GA};
    send_raw_data(ga_data);
}

void PlayerConnection::queue_output(std::string data) {
    auto self = shared_from_this();
    asio::dispatch(strand_, [this, self, data = std::move(data)]() mutable {
        bool was_empty = output_queue_.empty();

        if (output_queue_.size() >= MAX_OUTPUT_QUEUE_SIZE) {
            Log::warn("Output queue full for {}, dropping message", remote_address());
            return;
        }

        output_queue_.emplace_back(std::move(data));

        if (was_empty && !write_in_progress_) {
            flush_output();
        }
    });
}

void PlayerConnection::flush_output() {
    if (output_queue_.empty() || write_in_progress_ || !is_connected()) {
        return;
    }

    write_in_progress_ = true;

    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(output_queue_.front()),
        asio::bind_executor(strand_, [this, self](const asio::error_code &error, std::size_t bytes_transferred) {
            handle_write(error, bytes_transferred);
        }));
}

void PlayerConnection::handle_write(const asio::error_code &error, std::size_t /*bytes_transferred*/) {
    write_in_progress_ = false;

    if (error) {
        Log::error("Write error to {}: {}", remote_address(), error.message());
        disconnect("Write error");
        return;
    }

    // Remove sent message and continue with queue
    if (!output_queue_.empty()) {
        output_queue_.pop_front();

        if (!output_queue_.empty()) {
            flush_output();
        }
    }
}

void PlayerConnection::send_raw_data(const std::vector<uint8_t> &data) {
    if (!is_connected())
        return;

    std::string str_data(data.begin(), data.end());
    queue_output(std::move(str_data));
}

Result<void> PlayerConnection::send_gmcp(std::string_view module, const nlohmann::json &data) {
    return gmcp_handler_.send_gmcp(module, data);
}

void PlayerConnection::send_room_info() {
    if (!player_ || !supports_gmcp())
        return;

    auto room = player_->current_room_ptr();
    if (room) {
        gmcp_handler_.send_room_info(*room);
    }
}

void PlayerConnection::send_vitals() {
    if (!player_ || !supports_gmcp())
        return;

    auto vitals_data = player_->get_vitals_gmcp();
    send_gmcp("Char.Vitals", vitals_data);
}

void PlayerConnection::send_status() {
    if (!player_ || !supports_gmcp())
        return;

    nlohmann::json status = {{"name", player_->name()},
                             {"level", player_->level()},
                             {"class", player_->player_class()},
                             {"race", player_->race()}};

    send_gmcp("Char.Status", status);
}

void PlayerConnection::disconnect(std::string_view reason) {
    if (state_ == ConnectionState::Disconnected)
        return;

    Log::info("Disconnecting {} ({})", remote_address(), reason);

    transition_to(ConnectionState::Disconnecting);

    if (!reason.empty()) {
        send_message(fmt::format("Disconnecting: {}", reason));
    }

    force_disconnect();
}

void PlayerConnection::force_disconnect() {
    transition_to(ConnectionState::Disconnected);
    cleanup_connection();

    if (disconnect_callback_) {
        disconnect_callback_(shared_from_this());
    }
}

void PlayerConnection::cleanup_connection() {
    if (socket_.is_open()) {
        asio::error_code ec;
        socket_.close(ec);
    }

    // Remove player from world server if they were playing
    if (player_) {
        world_server_->remove_player(player_);
    }

    player_.reset();
    login_system_.reset();
    output_queue_.clear();
    write_in_progress_ = false;
}

std::string PlayerConnection::remote_address() const {
    try {
        if (socket_.is_open()) {
            return socket_.remote_endpoint().address().to_string();
        }
    } catch (const std::exception &) {
        // Socket might be closed
    }
    return "unknown";
}

std::chrono::seconds PlayerConnection::connection_duration() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - connect_time_);
}
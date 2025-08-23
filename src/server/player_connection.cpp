/***************************************************************************
 *   File: src/server/player_connection.cpp               Part of FieryMUD *
 *  Usage: Player connection implementation with Asio                     *
 ***************************************************************************/

#include "player_connection.hpp"
#include "world_server.hpp"
#include "gmcp_handler.hpp"
#include "../core/logging.hpp"

PlayerConnection::PlayerConnection(asio::io_context& io_context) 
    : socket_(io_context), gmcp_handler_(std::make_unique<GMCPHandler>(*this)) {}

PlayerConnection::~PlayerConnection() {
    close();
}

Result<void> PlayerConnection::initialize() {
    connected_ = true;
    
    // Start reading from the socket
    start_read();
    
    // Send welcome message with proper prompt
    send("Welcome to Modern FieryMUD!\r\n");
    send_prompt("What is your name? ");
    
    Log::debug("PlayerConnection initialized");
    return Success();
}

void PlayerConnection::close() {
    if (connected_.load()) {
        connected_ = false;
        writing_ = false;
        write_queue_.clear();
        
        try {
            socket_.close();
        } catch (const std::exception& e) {
            Log::debug("Error closing socket: {}", e.what());
        }
        
        Log::debug("PlayerConnection closed");
    }
}

bool PlayerConnection::is_connected() const {
    return connected_;
}

Result<void> PlayerConnection::send(std::string_view message) {
    if (!connected_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Connection is not active"});
    }
    
    try {
        write_queue_.emplace_back(message);
        
        // If not currently writing, start a write operation
        if (!writing_.exchange(true)) {
            do_write();
        }
        return Success();
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::NetworkError, 
            fmt::format("Failed to queue message: {}", e.what())});
    }
}

Result<void> PlayerConnection::send_prompt(std::string_view prompt) {
    if (!connected_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Connection is not active"});
    }
    
    try {
        // Send prompt with telnet GA (Go Ahead) to indicate ready for input
        std::string prompt_str = std::string(prompt);
        prompt_str += fmt::format("{}{}", 
            static_cast<char>(GMCPHandler::TELNET_IAC),
            static_cast<char>(GMCPHandler::TELNET_GA));
        
        write_queue_.emplace_back(std::move(prompt_str));
        
        // If not currently writing, start a write operation
        if (!writing_.exchange(true)) {
            do_write();
        }
        return Success();
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::NetworkError, 
            fmt::format("Failed to queue prompt: {}", e.what())});
    }
}

void PlayerConnection::start_read() {
    if (!connected_) {
        return;
    }
    
    socket_.async_read_some(asio::buffer(read_buffer_),
        [self = shared_from_this()](const asio::error_code& error, size_t bytes_transferred) {
            self->handle_read(error, bytes_transferred);
        });
}

void PlayerConnection::handle_read(const asio::error_code& error, size_t bytes_transferred) {
    if (!error) {
        // Append received data to input buffer
        input_buffer_.append(read_buffer_.data(), bytes_transferred);
        
        // Process complete lines
        process_input();
        
        // Continue reading
        start_read();
    } else {
        if (error != asio::error::eof && error != asio::error::connection_reset) {
            Log::debug("Read error: {}", error.message());
        }
        close();
    }
}

void PlayerConnection::do_write() {
    if (!connected_ || write_queue_.empty()) {
        writing_ = false;
        return;
    }
    
    const std::string& message = write_queue_.front();
    asio::async_write(socket_, asio::buffer(message),
        [self = shared_from_this()](const asio::error_code& error, size_t bytes_transferred) {
            self->handle_write(error, bytes_transferred);
        });
}

void PlayerConnection::handle_write(const asio::error_code& error, size_t bytes_transferred) {
    if (error) {
        Log::debug("Write error: {}", error.message());
        writing_ = false;
        close();
        return;
    }
    
    // Remove the message that was just sent
    if (!write_queue_.empty()) {
        write_queue_.erase(write_queue_.begin());
    }
    
    // Continue with next message in queue, or stop writing
    if (!write_queue_.empty() && connected_) {
        do_write();
    } else {
        writing_ = false;
    }
}

void PlayerConnection::process_input() {
    // First, check for telnet IAC sequences
    process_telnet_data(input_buffer_);
    
    // Then process regular text lines
    size_t pos;
    while ((pos = input_buffer_.find('\n')) != std::string::npos) {
        std::string line = input_buffer_.substr(0, pos);
        input_buffer_.erase(0, pos + 1);
        
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Process the complete line
        if (!line.empty()) {
            Log::debug("Received from client: '{}'", line);
            
            // Handle input based on connection state
            handle_input_by_state(line);
        }
    }
}

void PlayerConnection::process_telnet_data(std::string_view data) {
    size_t i = 0;
    std::string processed_buffer;
    
    while (i < input_buffer_.size()) {
        if (static_cast<uint8_t>(input_buffer_[i]) == GMCPHandler::TELNET_IAC && i + 1 < input_buffer_.size()) {
            uint8_t command = static_cast<uint8_t>(input_buffer_[i + 1]);
            
            if (command == GMCPHandler::TELNET_SB) {
                // Subnegotiation - look for IAC SE
                size_t sb_start = i + 2;
                size_t se_pos = input_buffer_.find(fmt::format("{}{}",
                    static_cast<char>(GMCPHandler::TELNET_IAC),
                    static_cast<char>(GMCPHandler::TELNET_SE)), sb_start);
                
                if (se_pos != std::string::npos) {
                    // Found complete subnegotiation
                    if (sb_start < input_buffer_.size() && 
                        static_cast<uint8_t>(input_buffer_[sb_start]) == GMCPHandler::GMCP_OPTION) {
                        // GMCP subnegotiation
                        std::string gmcp_data = input_buffer_.substr(sb_start + 1, se_pos - sb_start - 1);
                        auto result = gmcp_handler_->handle_gmcp_subnegotiation(gmcp_data);
                        if (!result) {
                            Log::error("GMCP subnegotiation failed: {}", result.error().message);
                        }
                    }
                    
                    // Remove the processed subnegotiation from buffer
                    input_buffer_.erase(i, se_pos + 2 - i);
                    continue;
                }
            } else if (i + 2 < input_buffer_.size() && 
                      (command == GMCPHandler::TELNET_WILL || command == GMCPHandler::TELNET_WONT ||
                       command == GMCPHandler::TELNET_DO || command == GMCPHandler::TELNET_DONT)) {
                // 3-byte telnet option negotiation
                uint8_t option = static_cast<uint8_t>(input_buffer_[i + 2]);
                
                auto result = gmcp_handler_->handle_telnet_option(command, option);
                if (!result) {
                    Log::error("Telnet option handling failed: {}", result.error().message);
                }
                
                // Remove the processed option from buffer
                input_buffer_.erase(i, 3);
                continue;
            }
        }
        ++i;
    }
}

void PlayerConnection::set_player(std::shared_ptr<Player> player) {
    player_ = player;
}

#include "networked_actor.hpp"
#include "../core/actor.hpp"

void PlayerConnection::handle_input_by_state(const std::string& input) {
    switch (state_) {
        case ConnectionState::WaitingForName:
            // Store the character name and create the player
            pending_character_name_ = input;
            set_state(ConnectionState::Playing);
            if (world_server_) {
                auto player = std::make_shared<NetworkedPlayer>(shared_from_this(), input);
                player->initialize();
                world_server_->set_actor_for_connection(shared_from_this(), player);
                send(fmt::format("Welcome, {}!\r\n", input));
                world_server_->process_command(shared_from_this(), "look");
            }
            break;
        case ConnectionState::Playing:
            if (world_server_) {
                world_server_->process_command(shared_from_this(), input);
            }
            break;
        default:
            // Should not happen
            break;
    }
}

std::shared_ptr<Player> PlayerConnection::get_player() const {
    return player_.lock();
}

/***************************************************************************
 *   File: src/server/gmcp_handler.cpp                    Part of FieryMUD *
 *  Usage: GMCP (Generic Mud Communication Protocol) implementation        *
 ***************************************************************************/

#include "gmcp_handler.hpp"
#include "player_connection.hpp"
#include "../world/room.hpp"
#include "../core/logging.hpp"
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

GMCPHandler::GMCPHandler(PlayerConnection& connection) 
    : connection_(connection) {
    Log::debug("GMCPHandler created for connection");
}

Result<void> GMCPHandler::handle_telnet_option(uint8_t command, uint8_t option) {
    if (option != GMCP_OPTION) {
        return Success(); // Not our option, ignore
    }

    switch (command) {
        case TELNET_WILL:
            // Client wants to enable GMCP
            Log::debug("Client requested GMCP support - sending DO GMCP");
            enable();
            TRY(send_telnet_option(TELNET_DO, GMCP_OPTION));
            
            // Send initial GMCP handshake
            send_hello();
            send_supports_set();
            break;
            
        case TELNET_WONT:
            // Client doesn't support GMCP
            Log::debug("Client declined GMCP support");
            disable();
            break;
            
        case TELNET_DO:
            // Client wants us to enable GMCP (we already support it)
            Log::debug("Client requested server GMCP - sending WILL GMCP");
            enable();
            TRY(send_telnet_option(TELNET_WILL, GMCP_OPTION));
            break;
            
        case TELNET_DONT:
            // Client wants us to disable GMCP
            Log::debug("Client requested GMCP disable");
            disable();
            break;
    }
    
    return Success();
}

Result<void> GMCPHandler::handle_gmcp_subnegotiation(std::string_view data) {
    if (!gmcp_enabled_) {
        return std::unexpected(Errors::InvalidState("GMCP not enabled"));
    }
    
    Log::debug("Received GMCP subnegotiation: {}", data);
    return process_gmcp_message(data);
}

Result<void> GMCPHandler::process_gmcp_message(std::string_view message) {
    // Find the first space to separate module from JSON data
    size_t space_pos = message.find(' ');
    if (space_pos == std::string::npos) {
        // No data, just module name
        Log::debug("GMCP message with no data: {}", message);
        return Success();
    }
    
    std::string module(message.substr(0, space_pos));
    std::string json_str(message.substr(space_pos + 1));
    
    Log::debug("GMCP module: '{}', data: '{}'", module, json_str);
    
    try {
        nlohmann::json data = nlohmann::json::parse(json_str);
        
        // Handle different GMCP modules
        if (module == "Core.Supports.Set") {
            if (data.is_array()) {
                client_supports_.clear();
                for (const auto& support : data) {
                    if (support.is_string()) {
                        add_client_support(support.get<std::string>());
                    }
                }
                Log::debug("Client supports {} GMCP modules", client_supports_.size());
            }
        } else if (module == "Core.Ping") {
            // Echo ping back
            TRY(send_gmcp("Core.Ping", data));
        } else {
            Log::debug("Unhandled GMCP module: {}", module);
        }
        
    } catch (const nlohmann::json::exception& e) {
        Log::error("Failed to parse GMCP JSON: {}", e.what());
        return std::unexpected(Errors::ParseError("GMCP JSON", e.what()));
    }
    
    return Success();
}

Result<void> GMCPHandler::send_gmcp(std::string_view module, const nlohmann::json& data) {
    if (!gmcp_enabled_) {
        return Success(); // Silently ignore if GMCP not enabled
    }
    
    std::string json_str = data.dump();
    std::string gmcp_message = fmt::format("{} {}", module, json_str);
    
    // Escape telnet data and wrap in subnegotiation
    std::string escaped_data = escape_telnet_data(gmcp_message);
    std::string telnet_message = fmt::format("{}{}{}{}{}",
        static_cast<char>(TELNET_IAC),
        static_cast<char>(TELNET_SB),
        static_cast<char>(GMCP_OPTION),
        escaped_data,
        fmt::format("{}{}", static_cast<char>(TELNET_IAC), static_cast<char>(TELNET_SE))
    );
    
    Log::debug("Sending GMCP: {} {}", module, json_str);
    return connection_.send(telnet_message);
}

void GMCPHandler::send_hello() {
    nlohmann::json hello_data;
    hello_data["client"] = "Modern FieryMUD";
    hello_data["version"] = "1.0.0";
    
    auto result = send_gmcp("Core.Hello", hello_data);
    if (!result) {
        Log::error("Failed to send Core.Hello: {}", result.error().message);
    }
}

void GMCPHandler::send_supports_set() {
    nlohmann::json supports = nlohmann::json::array({
        "Core 1",
        "Room 1"
        // Add more modules as we implement them
    });
    
    auto result = send_gmcp("Core.Supports.Set", supports);
    if (!result) {
        Log::error("Failed to send Core.Supports.Set: {}", result.error().message);
    }
}

void GMCPHandler::send_room_info(const Room& room) {
    if (!client_supports("Room")) {
        return; // Client doesn't support Room module
    }
    
    nlohmann::json room_data;
    room_data["num"] = room.id().value();
    room_data["name"] = room.name();
    room_data["desc"] = room.description();
    room_data["sector"] = magic_enum::enum_name(room.sector_type());
    
    // Add exits when available
    room_data["exits"] = nlohmann::json::object();
    // TODO: Add exits when Room class supports them
    
    auto result = send_gmcp("Room.Info", room_data);
    if (!result) {
        Log::error("Failed to send Room.Info: {}", result.error().message);
    }
}

void GMCPHandler::add_client_support(std::string_view module) {
    client_supports_.insert(std::string(module));
}

bool GMCPHandler::client_supports(std::string_view module) const {
    return client_supports_.count(std::string(module)) > 0;
}

std::string GMCPHandler::escape_telnet_data(std::string_view data) {
    std::string escaped;
    escaped.reserve(data.size() * 2); // Reserve space for potential doubling
    
    for (char c : data) {
        if (static_cast<uint8_t>(c) == TELNET_IAC) {
            // Double IAC bytes for escaping
            escaped += static_cast<char>(TELNET_IAC);
            escaped += static_cast<char>(TELNET_IAC);
        } else {
            escaped += c;
        }
    }
    
    return escaped;
}

Result<void> GMCPHandler::send_telnet_option(uint8_t command, uint8_t option) {
    std::string telnet_message = fmt::format("{}{}{}",
        static_cast<char>(TELNET_IAC),
        static_cast<char>(command),
        static_cast<char>(option)
    );
    
    return connection_.send(telnet_message);
}
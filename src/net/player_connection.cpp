#include "player_connection.hpp"

#include "../commands/command_system.hpp"
#include "../commands/information_commands.hpp"
#include "../core/actor.hpp"
#include "../core/config.hpp"
#include "../core/logging.hpp"
#include "../events/event_publisher.hpp"
#include "../server/network_manager.hpp"
#include "../server/world_server.hpp"
#include "../text/text_format.hpp"
#include "../world/world_manager.hpp"
#include "mssp_handler.hpp"

#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <regex>

// GMCPHandler implementation
GMCPHandler::GMCPHandler(PlayerConnection &connection) : connection_(connection) {
    // Initialize with conservative defaults until client capabilities are detected
    terminal_capabilities_ = TerminalCapabilities::detect_capabilities();
    
    // Initialize MSSP handler with network manager's config
    if (connection_.get_network_manager()) {
        // Note: NetworkManager constructor takes ServerConfig reference but we need access to it
        // For now, we'll create the handler when needed in handle_mssp_request
        mssp_handler_ = nullptr;
    }
}

void GMCPHandler::enable() {
    gmcp_enabled_ = true;
    terminal_capabilities_.supports_gmcp = true;  // Mark GMCP as supported
    Log::info("GMCP enabled. Using conservative defaults until client capabilities detected: terminal={}, unicode={}, color={}, level={}", 
              terminal_capabilities_.terminal_name,
              terminal_capabilities_.supports_unicode,
              terminal_capabilities_.supports_color,
              static_cast<int>(terminal_capabilities_.overall_level));
}

Result<void> GMCPHandler::handle_telnet_option(uint8_t command, uint8_t option) {
    switch (option) {
    case GMCP_OPTION:
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
            send_server_services();
            // Request terminal capabilities after GMCP handshake
            request_terminal_type();
            request_new_environ();
            return Result<void>{};
        case TELNET_DONT:
            // Client doesn't want us to use GMCP
            disable();
            return Result<void>{};
        default:
            return std::unexpected(Error{ErrorCode::InvalidArgument, fmt::format("Unknown telnet command: {}", command)});
        }
        break;
        
    case TTYPE_OPTION:
        switch (command) {
        case TELNET_WILL:
            // Client supports terminal type
            subnegotiate_terminal_type();
            return Result<void>{};
        case TELNET_WONT:
            // Client doesn't support terminal type
            // No action required
            return Result<void>{};
        default:
            // Malformed Telnet command
            return Result<void>{};
        }
        break;
        
    case NEW_ENVIRON_OPTION:
        switch (command) {
        case TELNET_WILL:
            // Client supports NEW-ENVIRON
            subnegotiate_new_environ();
            return Result<void>{};
        case TELNET_WONT:
            // Client doesn't support NEW-ENVIRON
            // No action required
            return Result<void>{};
        default:
            // Malformed Telnet command
            return Result<void>{};
        }
        break;
        
    case MSSP_OPTION:
        switch (command) {
        case TELNET_DO:
            // Client wants MSSP data
            return handle_mssp_request();
        default:
            return Result<void>{}; // Ignore other MSSP commands
        }
        break;
        
    default:
        return Result<void>{}; // Not our option, ignore
    }
}

Result<void> GMCPHandler::handle_gmcp_subnegotiation(std::string_view data) {
    if (!gmcp_enabled_) {
        return std::unexpected(Error{ErrorCode::InvalidState, "GMCP not enabled"});
    }

    return process_gmcp_message(data);
}

Result<void> GMCPHandler::process_gmcp_message(std::string_view message) {
    Log::debug("Processing GMCP message: {}", message);
    
    try {
        // GMCP messages are in the format "Module.SubModule JSON_DATA"
        // Find the first space to separate module from JSON data
        size_t space_pos = message.find(' ');
        if (space_pos == std::string::npos) {
            Log::debug("GMCP message has no JSON data: {}", message);
            return Result<void>{};  // No JSON data, just module name
        }
        
        std::string module = std::string(message.substr(0, space_pos));
        std::string json_str = std::string(message.substr(space_pos + 1));
        
        Log::debug("GMCP module: '{}', JSON: '{}'", module, json_str);
        
        auto json_data = nlohmann::json::parse(json_str);

        // Handle Core.Hello - client introduces itself
        if (module == "Core.Hello") {
            Log::info("GMCP client hello: {}", json_data.dump());
            
            Log::debug("Processing Core.Hello - step 1: extracting client info");
            // Extract client information and update terminal capabilities
            if (json_data.contains("client")) {
                terminal_capabilities_.client_name = json_data["client"].get<std::string>();
                Log::debug("Extracted client name: {}", terminal_capabilities_.client_name);
            }
            if (json_data.contains("version")) {
                terminal_capabilities_.client_version = json_data["version"].get<std::string>();
                Log::debug("Extracted client version: {}", terminal_capabilities_.client_version);
            }
            
            Log::debug("Processing Core.Hello - step 2: calling detect_capabilities_from_gmcp");
            try {
                // Preserve TLS status (based on actual connection, not client-reported)
                bool was_tls = terminal_capabilities_.supports_tls;

                // Update capabilities based on known clients
                terminal_capabilities_ = TerminalCapabilities::detect_capabilities_from_gmcp(json_data);
                terminal_capabilities_.supports_gmcp = true;  // Obviously true if we're getting GMCP
                terminal_capabilities_.supports_tls = was_tls;  // Restore actual TLS status
                Log::debug("Processing Core.Hello - step 3: capabilities updated successfully");
            } catch (const std::exception& e) {
                Log::error("Exception in detect_capabilities_from_gmcp: {}", e.what());
                terminal_capabilities_.supports_gmcp = true;  // At least mark GMCP as supported
            }
            
            Log::info("Updated capabilities from GMCP Hello: client={} {}, unicode={}, color={}, level={}", 
                      terminal_capabilities_.client_name,
                      terminal_capabilities_.client_version,
                      terminal_capabilities_.supports_unicode,
                      terminal_capabilities_.supports_color,
                      static_cast<int>(terminal_capabilities_.overall_level));
        }
        // Handle Core.Supports.Set - client tells us what modules it supports
        else if (module == "Core.Supports.Set") {
            for (const auto &support_module : json_data) {
                add_client_support(support_module.get<std::string>());
            }
            Log::debug("GMCP client supports {} modules", client_supports_.size());
        }
        // Handle External.Discord.Hello - client requests Discord integration info
        else if (module == "External.Discord.Hello") {
            Log::info("Client requested Discord integration");
            // Send Discord info in response
            send_gmcp("External.Discord.Info", {
                {"inviteurl", "https://discord.gg/aqhapUCgFz"},
                {"applicationid", ""}
            });
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
        std::string json_str = data.dump();
        std::string escaped = escape_telnet_data(json_str);

        // Send GMCP subnegotiation: IAC SB GMCP_OPTION <module> SP <data> IAC SE
        std::vector<uint8_t> gmcp_data;
        gmcp_data.push_back(TELNET_IAC);
        gmcp_data.push_back(TELNET_SB);
        gmcp_data.push_back(GMCP_OPTION);

        gmcp_data.insert(gmcp_data.end(), module.begin(), module.end());

        gmcp_data.push_back(' ');

        gmcp_data.insert(gmcp_data.end(), escaped.begin(), escaped.end());

        gmcp_data.push_back(TELNET_IAC);
        gmcp_data.push_back(TELNET_SE);

        connection_.send_raw_data(gmcp_data);
        return Result<void>{};
    } catch (const std::exception &e) {
        return std::unexpected(Error{ErrorCode::SerializationError, fmt::format("Failed to send GMCP: {}", e.what())});
    }
}

void GMCPHandler::send_server_services() {
    // Send server-side services to the client (like legacy offer_gmcp_services)
    nlohmann::json client_gui = {{"version", "3.0.0-modern"}, {"url", "https://fierymud.org"}};
    send_gmcp("Client.GUI", client_gui);

    nlohmann::json client_map = {{"url", "https://fierymud.org/map"}};
    send_gmcp("Client.Map", client_map);

    // Send Discord integration info
    send_gmcp("External.Discord.Info", {
        {"inviteurl", "https://discord.gg/aqhapUCgFz"},
        {"applicationid", "998826809686765569"}
    });
}

void GMCPHandler::send_supports_set() {
    // Advertise all supported GMCP modules
    nlohmann::json supports = nlohmann::json::array({
        "Core 1",
        "Room 1",
        "Char 1",
        "Char.Vitals 1",
        "Char.Status 1",
        "Char.Skills 1",
        "Char.Items 1",
        "External.Discord 1"
    });

    send_gmcp("Core.Supports.Set", supports);
}

void GMCPHandler::send_room_info(const Room &room) {
    if (!client_supports("Room"))
        return;

    // Helper function to convert Direction to string
    auto direction_to_string = [](Direction dir) -> std::string {
        switch (dir) {
            case Direction::North: return "n";
            case Direction::East: return "e";
            case Direction::South: return "s";
            case Direction::West: return "w";
            case Direction::Up: return "u";
            case Direction::Down: return "d";
            case Direction::Northeast: return "ne";
            case Direction::Northwest: return "nw";
            case Direction::Southeast: return "se";
            case Direction::Southwest: return "sw";
            case Direction::In: return "in";
            case Direction::Out: return "out";
            case Direction::Portal: return "portal";
            default: return "unknown";
        }
    };

    // Get zone name from WorldManager
    std::string zone_name = "Unknown";
    if (room.zone_id().is_valid()) {
        auto zone = WorldManager::instance().get_zone(room.zone_id());
        if (zone) {
            zone_name = zone->name();
        }
    }

    // Build exits information with door details (matches legacy format)
    nlohmann::json exits = nlohmann::json::object();
    for (Direction dir : room.get_visible_exits()) {
        const auto* exit_info = room.get_exit(dir);
        if (exit_info && exit_info->to_room.is_valid()) {
            nlohmann::json exit_json = {{"to_room", exit_info->to_room.value()}};

            // Add door information if this exit has a door
            if (exit_info->has_door) {
                exit_json["is_door"] = true;
                exit_json["door_name"] = exit_info->keyword.empty() ? "door" : exit_info->keyword;
                if (exit_info->is_locked) {
                    exit_json["door"] = "locked";
                } else if (exit_info->is_closed) {
                    exit_json["door"] = "closed";
                } else {
                    exit_json["door"] = "open";
                }
            } else {
                exit_json["is_door"] = false;
            }

            exits[direction_to_string(dir)] = exit_json;
        }
    }

    // Get sector type name for human-readable display
    std::string sector_name = std::string(RoomUtils::get_sector_name(room.sector_type()));

    // Build Room GMCP data (matches legacy format + enhancements)
    nlohmann::json room_data = {
        {"zone", zone_name},
        {"id", room.id().value()},
        {"name", room.name()},
        {"type", sector_name},
        {"environment", static_cast<int>(room.sector_type())},
        {"Exits", exits}
    };

    // Add layout coordinates for mapping if available
    if (room.layout_x().has_value()) {
        room_data["x"] = room.layout_x().value();
    }
    if (room.layout_y().has_value()) {
        room_data["y"] = room.layout_y().value();
    }
    if (room.layout_z().has_value()) {
        room_data["z"] = room.layout_z().value();
    }

    // Send as "Room" to match legacy format (clients may expect this)
    send_gmcp("Room", room_data);
}

void GMCPHandler::add_client_support(std::string_view module) {
    // Store the full module string (e.g., "Room 1")
    client_supports_.emplace(module);

    // Also store just the module name without version for easier lookups
    // GMCP modules are sent as "ModuleName Version" (e.g., "Room 1", "Char.Vitals 1")
    std::string module_str{module};
    size_t space_pos = module_str.find(' ');
    if (space_pos != std::string::npos) {
        client_supports_.emplace(module_str.substr(0, space_pos));
    }
}

bool GMCPHandler::client_supports(std::string_view module) const {
    return client_supports_.find(std::string(module)) != client_supports_.end();
}

void GMCPHandler::handle_terminal_type(std::string_view ttype) {
    // Parse MTTS terminal type - format is "terminal-name-MTTS n"
    std::string ttype_str{ttype};
    size_t mtts_pos = ttype_str.find("-MTTS ");

    if (mtts_pos != std::string::npos) {
        // Extract terminal name and MTTS number
        std::string terminal_name = ttype_str.substr(0, mtts_pos);
        std::string mtts_str = ttype_str.substr(mtts_pos + 6);

        try {
            uint32_t mtts_bitvector = std::stoul(mtts_str);
            handle_mtts_capability(mtts_bitvector);

            // Also update terminal name
            terminal_capabilities_.terminal_name = terminal_name;
            Log::info("MTTS terminal type: {} with bitvector: {}", terminal_name, mtts_bitvector);
        } catch (const std::exception& e) {
            Log::warn("Failed to parse MTTS bitvector from '{}': {}", ttype, e.what());
            // Fall back to basic terminal type detection - but don't overwrite GMCP
            merge_terminal_capabilities(TerminalCapabilities::get_capabilities_for_terminal(ttype));
        }
    } else {
        // Standard terminal type without MTTS - don't overwrite better GMCP detection
        auto ttype_caps = TerminalCapabilities::get_capabilities_for_terminal(ttype);

        // Only update terminal name, don't downgrade capabilities
        if (!ttype_str.empty()) {
            terminal_capabilities_.terminal_name = ttype_str;
        }

        // Merge only if TTYPE provides better capabilities
        if (ttype_caps.overall_level > terminal_capabilities_.overall_level) {
            merge_terminal_capabilities(ttype_caps);
            Log::info("TTYPE upgraded capabilities: terminal={}, level={}",
                      ttype_str, static_cast<int>(terminal_capabilities_.overall_level));
        } else {
            Log::debug("TTYPE terminal type: {} (not overwriting existing level {})",
                      ttype_str, static_cast<int>(terminal_capabilities_.overall_level));
        }
    }
}

void GMCPHandler::handle_mtts_capability(uint32_t bitvector) {
    auto mtts_caps = TerminalCapabilities::detect_capabilities_from_mtts(bitvector, terminal_capabilities_.terminal_name);

    // MTTS is reliable - merge with existing capabilities
    // Detection priority: GMCP > MTTS > others, but MTTS can provide complementary info
    using TerminalCapabilities::DetectionMethod;

    if (terminal_capabilities_.detection_method == DetectionMethod::GMCP) {
        // GMCP already detected, only upgrade from MTTS if it provides better level
        if (mtts_caps.overall_level > terminal_capabilities_.overall_level) {
            merge_terminal_capabilities(mtts_caps);
            terminal_capabilities_.detection_method = DetectionMethod::MTTS;
            Log::info("MTTS upgraded GMCP-detected capabilities to level {}",
                      static_cast<int>(terminal_capabilities_.overall_level));
        } else {
            Log::debug("MTTS bitvector {} parsed, but not overwriting GMCP level {}",
                      bitvector, static_cast<int>(terminal_capabilities_.overall_level));
        }
    } else {
        // No GMCP, use MTTS as authoritative
        merge_terminal_capabilities(mtts_caps);
        terminal_capabilities_.detection_method = DetectionMethod::MTTS;
    }

    Log::info("MTTS capabilities (bitvector: {}): terminal={}, color={}, 256color={}, truecolor={}, unicode={}, mouse={}, screen_reader={}, tls={}, level={}",
              bitvector,
              terminal_capabilities_.terminal_name,
              terminal_capabilities_.supports_color,
              terminal_capabilities_.supports_256_color,
              terminal_capabilities_.supports_true_color,
              terminal_capabilities_.supports_unicode,
              terminal_capabilities_.supports_mouse,
              terminal_capabilities_.supports_screen_reader,
              terminal_capabilities_.supports_tls,
              static_cast<int>(terminal_capabilities_.overall_level));
}

void GMCPHandler::merge_terminal_capabilities(const TerminalCapabilities::Capabilities& new_caps) {
    // Merge capabilities - only upgrade, never downgrade individual features
    if (new_caps.supports_color) terminal_capabilities_.supports_color = true;
    if (new_caps.supports_256_color) terminal_capabilities_.supports_256_color = true;
    if (new_caps.supports_true_color) terminal_capabilities_.supports_true_color = true;
    if (new_caps.supports_unicode) terminal_capabilities_.supports_unicode = true;
    if (new_caps.supports_bold) terminal_capabilities_.supports_bold = true;
    if (new_caps.supports_italic) terminal_capabilities_.supports_italic = true;
    if (new_caps.supports_underline) terminal_capabilities_.supports_underline = true;
    if (new_caps.supports_mouse) terminal_capabilities_.supports_mouse = true;
    if (new_caps.supports_screen_reader) terminal_capabilities_.supports_screen_reader = true;
    if (new_caps.supports_tls) terminal_capabilities_.supports_tls = true;
    if (new_caps.supports_hyperlinks) terminal_capabilities_.supports_hyperlinks = true;

    // Update overall level only if improved
    if (new_caps.overall_level > terminal_capabilities_.overall_level) {
        terminal_capabilities_.overall_level = new_caps.overall_level;
    }
}

std::unordered_map<std::string, std::string> GMCPHandler::parse_new_environ_data(std::string_view data) {
    std::unordered_map<std::string, std::string> env_vars;

    std::string var;
    std::string value;

    bool first = true;
    bool read_var = false;
    bool read_value = false;
    bool escaped = false;
    for (char c : data) {
        if (first) {
            if (c == SB_IS || c == SB_INFO) {
                continue;
            }
            first = false;
        }
        if (!escaped) {
            if (c == NEW_ENVIRON_ESC) {
                escaped = true;
                continue;
            } else if (c == NEW_ENVIRON_VAR || c == NEW_ENVIRON_USERVAR) {
                if (read_value) {
                    Log::debug("NEW_ENVIRON for {}: var={} value={}", connection_.remote_address(), var, value);
                    env_vars[var] = value;
                    var.clear();
                    value.clear();
                }
                read_var = true;
                continue;
            } else if (c == NEW_ENVIRON_VALUE) {
                read_value = true;
                continue;
            }
        } else {
            escaped = false;
        }
        if (read_var) {
            var.push_back(c);
        } else if (read_value) {
            value.push_back(c);
        } else {
            Log::debug("Invalid NEW_ENVIRON subnegotiation data from {}", connection_.remote_address());
        }
    }

    if (read_value) {
        Log::debug("NEW_ENVIRON for {}: var={} value={}", connection_.remote_address(), var, value);
        env_vars[var] = value;
    } else {
        Log::debug("Incomplete NEW_ENVIRON subnegotiation data from {}", connection_.remote_address());
    }

    return env_vars;
}

void GMCPHandler::handle_new_environ_data(const std::unordered_map<std::string, std::string>& env_vars) {
    // Get NEW-ENVIRON capabilities
    auto new_caps = TerminalCapabilities::detect_capabilities_from_new_environ(env_vars);

    // Log what NEW-ENVIRON provided
    Log::debug("NEW-ENVIRON raw detection from {} variables: terminal={}, client={} {}, color={}, 256color={}, truecolor={}, unicode={}, level={}",
              env_vars.size(),
              new_caps.terminal_name,
              new_caps.client_name,
              new_caps.client_version,
              new_caps.supports_color,
              new_caps.supports_256_color,
              new_caps.supports_true_color,
              new_caps.supports_unicode,
              static_cast<int>(new_caps.overall_level));

    // Log some key environment variables for debugging
    for (const auto& [key, value] : env_vars) {
        Log::debug("NEW-ENVIRON {}: {}", key, value);
    }

    // Detection priority: GMCP > MTTS > NewEnviron > Environment
    // Only merge NEW-ENVIRON data if it provides BETTER capabilities, never downgrade
    using TerminalCapabilities::DetectionMethod;
    using TerminalCapabilities::SupportLevel;

    bool should_merge = false;

    // If we have GMCP detection, it's the most reliable - don't downgrade
    if (terminal_capabilities_.detection_method == DetectionMethod::GMCP) {
        // Only upgrade from GMCP if NEW-ENVIRON provides better level (unlikely)
        if (new_caps.overall_level > terminal_capabilities_.overall_level) {
            should_merge = true;
            Log::info("NEW-ENVIRON upgrading GMCP-detected capabilities from level {} to {}",
                      static_cast<int>(terminal_capabilities_.overall_level),
                      static_cast<int>(new_caps.overall_level));
        } else {
            Log::debug("Ignoring NEW-ENVIRON: GMCP already detected level {} (NEW-ENVIRON: {})",
                      static_cast<int>(terminal_capabilities_.overall_level),
                      static_cast<int>(new_caps.overall_level));
        }
    }
    // If we have MTTS detection, it's also reliable
    else if (terminal_capabilities_.detection_method == DetectionMethod::MTTS) {
        if (new_caps.overall_level > terminal_capabilities_.overall_level) {
            should_merge = true;
            Log::info("NEW-ENVIRON upgrading MTTS-detected capabilities from level {} to {}",
                      static_cast<int>(terminal_capabilities_.overall_level),
                      static_cast<int>(new_caps.overall_level));
        } else {
            Log::debug("Ignoring NEW-ENVIRON: MTTS already detected level {} (NEW-ENVIRON: {})",
                      static_cast<int>(terminal_capabilities_.overall_level),
                      static_cast<int>(new_caps.overall_level));
        }
    }
    // For Environment or unknown detection, NEW-ENVIRON can upgrade
    else {
        if (new_caps.overall_level >= terminal_capabilities_.overall_level) {
            should_merge = true;
        }
    }

    if (should_merge) {
        // Merge capabilities - only upgrade, never downgrade individual features
        if (new_caps.supports_color) terminal_capabilities_.supports_color = true;
        if (new_caps.supports_256_color) terminal_capabilities_.supports_256_color = true;
        if (new_caps.supports_true_color) terminal_capabilities_.supports_true_color = true;
        if (new_caps.supports_unicode) terminal_capabilities_.supports_unicode = true;
        if (new_caps.supports_bold) terminal_capabilities_.supports_bold = true;
        if (new_caps.supports_italic) terminal_capabilities_.supports_italic = true;
        if (new_caps.supports_underline) terminal_capabilities_.supports_underline = true;

        // Update terminal/client info if NEW-ENVIRON provides useful data
        if (!new_caps.terminal_name.empty() && new_caps.terminal_name != "unknown") {
            terminal_capabilities_.terminal_name = new_caps.terminal_name;
        }
        if (!new_caps.client_name.empty() && new_caps.client_name != "Unknown") {
            terminal_capabilities_.client_name = new_caps.client_name;
        }
        if (!new_caps.client_version.empty()) {
            terminal_capabilities_.client_version = new_caps.client_version;
        }

        // Update overall level only if improved
        if (new_caps.overall_level > terminal_capabilities_.overall_level) {
            terminal_capabilities_.overall_level = new_caps.overall_level;
            terminal_capabilities_.detection_method = DetectionMethod::NewEnviron;
        }

        Log::info("NEW-ENVIRON merged: terminal={}, client={} {}, color={}, 256color={}, truecolor={}, unicode={}, level={}",
                  terminal_capabilities_.terminal_name,
                  terminal_capabilities_.client_name,
                  terminal_capabilities_.client_version,
                  terminal_capabilities_.supports_color,
                  terminal_capabilities_.supports_256_color,
                  terminal_capabilities_.supports_true_color,
                  terminal_capabilities_.supports_unicode,
                  static_cast<int>(terminal_capabilities_.overall_level));
    }
}

const TerminalCapabilities::Capabilities& GMCPHandler::get_terminal_capabilities() const {
    return terminal_capabilities_;
}

void GMCPHandler::request_terminal_type() {
    // Send IAC DO TTYPE to request terminal type
    std::vector<uint8_t> ttype_request = {TELNET_IAC, TELNET_DO, TTYPE_OPTION};
    connection_.send_raw_data(ttype_request);
    Log::debug("Requested terminal type");
}

void GMCPHandler::subnegotiate_terminal_type() {
    connection_.send_raw_data({
        TELNET_IAC,
        TELNET_SB,
        TTYPE_OPTION,
        SB_SEND,
        TELNET_IAC,
        TELNET_SE,
    });
    Log::debug("Requested terminal subnegotiation from {}", connection_.remote_address());
}

void GMCPHandler::request_new_environ() {
    // Send IAC DO NEW-ENVIRON to request environment variables
    std::vector<uint8_t> environ_request = {TELNET_IAC, TELNET_DO, NEW_ENVIRON_OPTION};
    connection_.send_raw_data(environ_request);
    Log::debug("Requested NEW-ENVIRON data");
}

void GMCPHandler::subnegotiate_new_environ() {
    connection_.send_raw_data({
        TELNET_IAC,
        TELNET_SB,
        NEW_ENVIRON_OPTION,
        SB_SEND,
        TELNET_IAC,
        TELNET_SE,
    });
    Log::debug("Requested NEW-ENVIRON subnegotiation from {}", connection_.remote_address());
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
    Log::debug("Sent telnet option to {}: command={} option={}", connection_.remote_address(), command, option);
    return Result<void>{};
}

Result<void> GMCPHandler::handle_mssp_request() {
    Log::debug("Client requested MSSP data");
    return send_mssp_data();
}

Result<void> GMCPHandler::send_mssp_data() {
    // Create MSSP handler if not already created
    if (!mssp_handler_ && connection_.get_network_manager()) {
        // Create MSSP handler with access to the actual ServerConfig
        const auto& config = connection_.get_network_manager()->get_config();
        mssp_handler_ = std::make_unique<MSSPHandler>(config);
        
        // Generate MSSP data using the proper handler
        auto mssp_data = mssp_handler_->generate_mssp_data(connection_.get_network_manager());
        
        connection_.send_raw_data(mssp_data);
        Log::info("Sent MSSP data ({} bytes) to client", mssp_data.size());
        return Result<void>{};
    }
    
    // If we have a proper handler, use it
    if (mssp_handler_) {
        auto mssp_data = mssp_handler_->generate_mssp_data(connection_.get_network_manager());
        connection_.send_raw_data(mssp_data);
        Log::info("Sent MSSP data ({} bytes) to client via handler", mssp_data.size());
        return Result<void>{};
    }
    
    return std::unexpected(Error{ErrorCode::InternalError, "Unable to generate MSSP data"});
}

// PlayerConnection implementation
std::shared_ptr<PlayerConnection> PlayerConnection::create(asio::io_context &io_context,
                                                           std::shared_ptr<WorldServer> world_server,
                                                           NetworkManager* network_manager) {

    // Use private constructor with shared_ptr
    return std::shared_ptr<PlayerConnection>(new PlayerConnection(io_context, std::move(world_server), network_manager));
}

std::shared_ptr<PlayerConnection> PlayerConnection::create_tls(asio::io_context &io_context,
                                                              std::shared_ptr<WorldServer> world_server,
                                                              NetworkManager* network_manager,
                                                              TLSContextManager& tls_manager) {
    // Use private TLS constructor with shared_ptr
    return std::shared_ptr<PlayerConnection>(new PlayerConnection(io_context, std::move(world_server), network_manager, tls_manager));
}

PlayerConnection::PlayerConnection(asio::io_context &io_context, std::shared_ptr<WorldServer> world_server,
                                   NetworkManager* network_manager)
    : io_context_(io_context), strand_(asio::make_strand(io_context)),
      world_server_(std::move(world_server)), network_manager_(network_manager), idle_check_timer_(io_context), 
      gmcp_handler_(*this), connect_time_(std::chrono::steady_clock::now()) {
    // Create plain TCP socket
    connection_socket_ = std::make_unique<TLSSocket>(asio::ip::tcp::socket(io_context));
}

PlayerConnection::PlayerConnection(asio::io_context &io_context, std::shared_ptr<WorldServer> world_server,
                                   NetworkManager* network_manager, TLSContextManager& tls_manager)
    : io_context_(io_context), strand_(asio::make_strand(io_context)),
      world_server_(std::move(world_server)), network_manager_(network_manager), idle_check_timer_(io_context), 
      gmcp_handler_(*this), connect_time_(std::chrono::steady_clock::now()) {
    // Create TLS socket
    connection_socket_ = std::make_unique<TLSSocket>(asio::ip::tcp::socket(io_context), tls_manager.get_context());
}

PlayerConnection::~PlayerConnection() { cleanup_connection(); }

void PlayerConnection::start() {
    connect_time_ = std::chrono::steady_clock::now();

    Log::info("New {} connection from {}", (is_tls_connection() ? "TLS" : "TCP"), remote_address());

    // Set TLS capability based on actual connection type (not just client-reported MTTS)
    gmcp_handler_.set_tls_status(is_tls_connection());

    // Initialize login system
    login_system_ = std::make_unique<LoginSystem>(shared_from_this());
    login_system_->set_player_loaded_callback([this](std::shared_ptr<Player> player) { on_login_completed(player); });

    // For TLS connections, perform handshake first with timeout
    if (is_tls_connection()) {
        // Set up handshake timeout to prevent slowloris-style attacks
        idle_check_timer_.expires_after(TLS_HANDSHAKE_TIMEOUT);
        idle_check_timer_.async_wait([this, self = shared_from_this()](const asio::error_code& ec) {
            if (!ec) {
                // Timer fired - handshake took too long
                Log::warn("TLS handshake timeout for {} after {} seconds",
                         remote_address(), TLS_HANDSHAKE_TIMEOUT.count());
                disconnect("TLS handshake timeout");
            }
            // If ec is operation_aborted, the timer was cancelled (handshake completed)
        });

        connection_socket_->async_handshake([this, self = shared_from_this()](const asio::error_code& error) {
            // Cancel the timeout timer
            idle_check_timer_.cancel();

            if (!error) {
                Log::debug("TLS handshake completed for {}", remote_address());
                handle_connect();
            } else {
                Log::error("TLS handshake failed for {}: {}", remote_address(), error.message());
                disconnect("TLS handshake failed");
            }
        });
    } else {
        // Plain connection - proceed directly
        handle_connect();
    }
}

void PlayerConnection::handle_connect() {
    transition_to(ConnectionState::Login);

    // Register this connection with WorldServer for game loop processing
    // (timers, casting, combat, etc.) - must happen AFTER TLS handshake
    if (world_server_) {
        world_server_->add_player_connection(shared_from_this());
    }

    // Begin reading first so we can receive telnet responses
    start_read();

    // Send telnet negotiation (GMCP offer)
    send_telnet_negotiation();

    // Delay the login welcome message by 200ms to allow capability negotiation
    // This gives GMCP clients time to send Core.Hello before we display the logo
    // IMPORTANT: Must run on strand to avoid race conditions with input processing
    auto self = shared_from_this();
    auto welcome_timer = std::make_shared<asio::steady_timer>(io_context_);
    welcome_timer->expires_after(std::chrono::milliseconds(200));
    welcome_timer->async_wait(asio::bind_executor(strand_,
        [self, welcome_timer](const asio::error_code& error) {
            if (!error && self->is_connected()) {
                self->login_system_->start_login();
            }
        }));
}

void PlayerConnection::send_telnet_negotiation() {
    // Offer GMCP support: IAC WILL GMCP
    std::vector<uint8_t> gmcp_offer = {GMCPHandler::TELNET_IAC, GMCPHandler::TELNET_WILL, GMCPHandler::GMCP_OPTION};
    Log::info("Sending GMCP offer to {}: IAC WILL GMCP (255 251 201)", remote_address());
    send_raw_data(gmcp_offer);

    // Offer MSSP support: IAC WILL MSSP
    std::vector<uint8_t> mssp_offer = {GMCPHandler::TELNET_IAC, GMCPHandler::TELNET_WILL, GMCPHandler::MSSP_OPTION};
    Log::info("Sending MSSP offer to {}: IAC WILL MSSP (255 251 70)", remote_address());
    send_raw_data(mssp_offer);

    // Don't immediately request capabilities - wait for client response first
    // This prevents potential infinite loops with aggressive clients
    // Capabilities are requested in handle_telnet_option() when client responds with DO
}

void PlayerConnection::start_read() {
    if (!is_connected())
        return;

    auto self = shared_from_this();
    connection_socket_->async_read_some(
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
            
            // If player is actively playing, go linkdead instead of disconnecting
            if (state_ == ConnectionState::Playing && player_) {
                set_linkdead(true, "Connection lost");
            } else {
                disconnect("Connection closed");
            }
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
            // Check size limit before adding to buffer (prevent DoS)
            if (telnet_buffer_.size() >= MAX_TELNET_SUBNEG_LENGTH) {
                Log::warn("Telnet subnegotiation from {} exceeded max size ({}), aborting",
                          remote_address(), MAX_TELNET_SUBNEG_LENGTH);
                in_telnet_negotiation_ = false;
                telnet_buffer_.clear();
                continue;
            }
            telnet_buffer_.push_back(byte);

            // Check for complete telnet commands
            if (telnet_buffer_.size() >= 3) {
                uint8_t command = telnet_buffer_[1];
                uint8_t option = telnet_buffer_[2];

                if (command == GMCPHandler::TELNET_WILL || command == GMCPHandler::TELNET_WONT ||
                    command == GMCPHandler::TELNET_DO || command == GMCPHandler::TELNET_DONT) {

                    // Simple 3-byte command
                    Log::debug("Received telnet option from {}: command={} option={}", remote_address(), command, option);
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
                        } else if (option == GMCPHandler::TTYPE_OPTION) {
                            gmcp_handler_.handle_terminal_type(sub_data);
                        } else if (option == GMCPHandler::NEW_ENVIRON_OPTION) {
                            auto env_vars = gmcp_handler_.parse_new_environ_data(sub_data);
                            gmcp_handler_.handle_new_environ_data(env_vars);
                        }

                        in_telnet_negotiation_ = false;
                    }
                }
            }
        } else {
            // Regular text data
            if (byte == '\n') {
                // Process input on newline (even if empty - allows prompt refresh)
                // We only trigger on \n to avoid double-processing \r\n (CRLF)
                process_input(input_buffer_);
                input_buffer_.clear();
            } else if (byte == '\r') {
                // Ignore carriage return - we process on \n only
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

    // Update activity tracking for session management
    update_last_input_time();

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
        // Prompt is sent by WorldServer::send_prompt_to_actor after command processing
    } else if (state_ == ConnectionState::AFK && player_) {
        // AFK players can still execute commands, this brings them back
        Log::debug("AFK player '{}' returning with command", player_->name());
        set_afk(false);
        // Process the command normally after returning from AFK
        if (world_server_) {
            world_server_->process_command(player_, input);
        }
        // Prompt is sent by WorldServer::send_prompt_to_actor after command processing
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
    login_time_ = std::chrono::system_clock::now();  // Track actual login time

    // Set the player's output interface
    player_->set_output(shared_from_this());

    // Ensure player is placed in a room
    if (!player_->current_room()) {
        // Use player's saved start_room if valid, otherwise fall back to default
        auto target_room_id = player_->start_room();
        if (!target_room_id.is_valid()) {
            target_room_id = Config::instance().default_starting_room();
        }

        auto move_result = WorldManager::instance().move_actor_to_room(player_, target_room_id);

        if (!move_result.success) {
            Log::warn("Failed to place player '{}' in room {}: {}",
                     player_->name(), target_room_id, move_result.failure_reason);
            // Try default as fallback if saved room failed
            if (target_room_id != Config::instance().default_starting_room()) {
                auto default_room = Config::instance().default_starting_room();
                move_result = WorldManager::instance().move_actor_to_room(player_, default_room);
                if (move_result.success) {
                    Log::info("Placed player '{}' in default room {} (saved room failed)",
                             player_->name(), default_room);
                }
            }
        } else {
            Log::info("Placed player '{}' in room {}", player_->name(), target_room_id);
        }
    }

    // Add player to the world server's online players list
    world_server_->add_player(player_);

    // Set the connection->actor mapping now that login is complete
    world_server_->set_actor_for_connection(shared_from_this(), player_);

    transition_to(ConnectionState::Playing);

    // Store original host for reconnection validation
    original_host_ = remote_address();
    
    // Start idle checking timer for session management
    start_idle_timer();

    Log::info("Player '{}' logged in from {}", player_->name(), remote_address());

    // Publish login event to Muditor bridge
    auto login_event = fierymud::events::GameEvent::player_event(
        fierymud::events::GameEventType::PLAYER_LOGIN,
        std::string(player_->name()),
        fmt::format("{} has entered the game", player_->name()));
    if (auto room = player_->current_room()) {
        login_event.zone_id = static_cast<int>(room->id().zone_id());
        login_event.room_vnum = static_cast<int>(room->id().local_id());
    }
    login_event.metadata["ip"] = remote_address();
    login_event.metadata["level"] = player_->level();
    fierymud::events::EventPublisher::instance().publish(std::move(login_event));

    // Send room info after login (respects brief mode, shows name/contents/actors)
    send_message(InformationCommands::format_room_for_actor(player_));

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

void PlayerConnection::attach_player(std::shared_ptr<Player> player) {
    player_ = std::move(player);
    
    // Set the player's output interface to this connection
    player_->set_output(shared_from_this());
    
    // Transition to playing state
    transition_to(ConnectionState::Playing);
    
    // Store original host for reconnection validation
    original_host_ = remote_address();
    
    // Start idle checking timer for session management
    start_idle_timer();
    
    Log::info("Player '{}' reconnected from {}", player_->name(), remote_address());
    
    // Send reconnection message and current room info (respects brief mode)
    send_message("=== Reconnected ===");
    send_message(InformationCommands::format_room_for_actor(player_));
    
    // Send room info for reconnections (vitals/status already established)
    if (supports_gmcp()) {
        send_room_info();
    }
    
    send_prompt();
}

void PlayerConnection::send_message(std::string_view message) {
    if (!is_connected())
        return;

    // Process XML-lite markup (e.g., <red>, <b:green>) to ANSI escape codes
    std::string processed = TextFormat::apply_colors(message);
    queue_output(fmt::format("{}\r\n", processed));
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

    // Process XML-lite markup (e.g., <red>, <b:green>) to ANSI escape codes
    std::string processed = TextFormat::apply_colors(line);
    queue_output(fmt::format("{}\r\n", processed));
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
    
    // Use async_write_some for TLS compatibility
    const std::string& data = output_queue_.front();
    connection_socket_->async_write_some(
        asio::buffer(data),
        asio::bind_executor(strand_, [this, self](const asio::error_code &error, std::size_t bytes_transferred) {
            handle_write(error, bytes_transferred);
        }));
}

void PlayerConnection::handle_write(const asio::error_code &error, std::size_t /*bytes_transferred*/) {
    write_in_progress_ = false;

    // If we're already disconnecting/disconnected, don't process further
    if (state_ == ConnectionState::Disconnecting || state_ == ConnectionState::Disconnected) {
        output_queue_.clear();
        return;
    }

    if (error) {
        // Log at debug level for expected errors (broken pipe during disconnect)
        if (error == asio::error::broken_pipe || error == asio::error::connection_reset) {
            Log::debug("Write error to {} (expected during disconnect): {}", remote_address(), error.message());
        } else {
            Log::error("Write error to {}: {}", remote_address(), error.message());
        }
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

void PlayerConnection::forward_command_to_game(std::string_view command) {
    if (state_ != ConnectionState::Playing || !player_) {
        Log::warn("Cannot forward command - not in Playing state or no player");
        return;
    }

    Log::debug("Forwarding buffered command to game: '{}'", command);
    if (world_server_) {
        world_server_->process_command(player_, command);
    }
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

    // Send comprehensive Char data (matches legacy format)
    auto char_data = player_->get_vitals_gmcp();
    send_gmcp("Char", char_data);
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

void PlayerConnection::send_discord_status() {
    if (!player_ || !supports_gmcp())
        return;

    // Send Discord rich presence status
    std::string details = fmt::format("Character: {}  Class: {}  Level: {}",
        player_->name(),
        player_->player_class(),
        player_->level());

    send_gmcp("External.Discord.Status", {
        {"state", "Playing FieryMUD (fierymud.org:4000)"},
        {"details", details},
        {"game", "FieryMUD"},
        {"smallimage", nlohmann::json::array({"servericon"})},
        {"smallimagetext", "FieryMUD"},
        {"starttime", std::chrono::duration_cast<std::chrono::seconds>(
            login_time_.time_since_epoch()).count()}
    });
}

void PlayerConnection::disconnect(std::string_view reason) {
    // Check if already disconnecting or disconnected to prevent re-entry
    if (state_ == ConnectionState::Disconnected || state_ == ConnectionState::Disconnecting)
        return;

    Log::info("Disconnecting {} ({})", remote_address(), reason);

    transition_to(ConnectionState::Disconnecting);

    // Only try to send farewell message if socket is still valid and open
    if (!reason.empty() && connection_socket_ && connection_socket_->is_open()) {
        // Check if socket is actually writable (not already errored)
        try {
            send_message(fmt::format("Disconnecting: {}", reason));
        } catch (...) {
            // Socket already broken, ignore
        }
    }

    force_disconnect();
}

void PlayerConnection::force_disconnect() {
    transition_to(ConnectionState::Disconnected);
    cleanup_connection();

    // Note: Disconnect callback is skipped during NetworkManager shutdown
    // to prevent deadlocks since connections are being bulk-disconnected
}

void PlayerConnection::cleanup_connection() {
    // During shutdown, do synchronous cleanup to avoid strand deadlocks
    if (connection_socket_ && connection_socket_->is_open()) {
        connection_socket_->close();
    }

    // Cancel any pending timers
    idle_check_timer_.cancel();

    // Clear I/O state
    output_queue_.clear();
    write_in_progress_ = false;

    // Clean up resources
    if (player_) {
        // Publish logout event to Muditor bridge
        fierymud::events::EventPublisher::instance().publish_player(
            fierymud::events::GameEventType::PLAYER_LOGOUT,
            player_->name(),
            fmt::format("{} has left the game", player_->name()));

        Log::debug("Cleaning up connection for player: {}", player_->name());

        // If this connection was replaced by a reconnection, WorldServer was already updated
        // by handle_player_reconnection() - skip duplicate cleanup
        if (!was_replaced_ && world_server_) {
            // Normal disconnect: remove this connection from WorldServer tracking
            world_server_->remove_player_connection(shared_from_this());
        }
    }

    player_.reset();
    login_system_.reset();
}

std::string PlayerConnection::remote_address() const {
    try {
        if (connection_socket_ && connection_socket_->is_open()) {
            return connection_socket_->remote_endpoint();
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

// Enhanced session management implementations

std::chrono::seconds PlayerConnection::idle_time() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - last_input_time_);
}

std::chrono::seconds PlayerConnection::afk_time() const {
    if (!is_afk_) return std::chrono::seconds{0};
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - afk_start_time_);
}

void PlayerConnection::update_last_input_time() {
    last_input_time_ = std::chrono::steady_clock::now();
    
    // If player was AFK, bring them back
    if (is_afk_) {
        set_afk(false);
    }
}

void PlayerConnection::check_idle_timeout() {
    if (state_ != ConnectionState::Playing || !player_) {
        return;
    }
    
    auto idle_duration = idle_time();
    
    // Check for AFK state transition
    if (!is_afk_ && idle_duration >= AFK_TIMEOUT) {
        set_afk(true);
    }
    
    // Check for linkdead state transition (only if connection is active)
    if (is_connected() && idle_duration >= IDLE_TIMEOUT) {
        set_linkdead(true, "Idle timeout");
    }
}

void PlayerConnection::set_afk(bool afk) {
    if (is_afk_ == afk) return;
    
    is_afk_ = afk;
    
    if (afk) {
        afk_start_time_ = std::chrono::steady_clock::now();
        transition_to(ConnectionState::AFK);
        send_message("You are now marked as AFK (Away From Keyboard).");
        Log::info("Player {} went AFK after {} seconds idle", 
                  player_ ? player_->name() : "unknown", idle_time().count());
    } else {
        afk_start_time_ = {};
        transition_to(ConnectionState::Playing);
        send_message("Welcome back! You are no longer AFK.");
        Log::info("Player {} returned from AFK after {} seconds", 
                  player_ ? player_->name() : "unknown", afk_time().count());
    }
}

void PlayerConnection::set_linkdead(bool linkdead, std::string_view reason) {
    if (is_linkdead_ == linkdead) return;
    
    is_linkdead_ = linkdead;
    disconnect_reason_ = reason;
    
    if (linkdead) {
        transition_to(ConnectionState::Linkdead);
        Log::info("Player {} went linkdead: {}", 
                  player_ ? player_->name() : "unknown", reason);
        
        // Keep player in world but mark as linkdead
        // The world server will handle linkdead cleanup after timeout
        if (player_) {
            player_->set_linkdead(true);
            Log::debug("Player {} marked linkdead, staying in world", player_->name());
        }
    } else {
        Log::info("Player {} recovered from linkdead state", 
                  player_ ? player_->name() : "unknown");
        if (player_) {
            player_->set_linkdead(false);
        }
        transition_to(ConnectionState::Playing);
        disconnect_reason_.clear();
    }
}

// Removed attempt_reconnection() method - not used, login system handles reconnection directly

void PlayerConnection::start_idle_timer() {
    // Start periodic idle checking every 60 seconds
    idle_check_timer_.expires_after(std::chrono::seconds(60));
    idle_check_timer_.async_wait(
        asio::bind_executor(strand_, 
            [self = shared_from_this()](const asio::error_code &error) {
                self->handle_idle_timer(error);
            }));
}

void PlayerConnection::handle_idle_timer(const asio::error_code &error) {
    if (error) {
        // Timer was cancelled or other error occurred
        if (error != asio::error::operation_aborted) {
            Log::warn("Idle timer error for {}: {}", remote_address(), error.message());
        }
        return;
    }
    
    // Check idle timeout for playing connections
    if (state_ == ConnectionState::Playing || state_ == ConnectionState::AFK) {
        check_idle_timeout();
    }

    // Check login timeout for connections that haven't completed login
    // This prevents connections from sitting at the login screen forever
    if (state_ == ConnectionState::Connected || state_ == ConnectionState::Login) {
        auto connected_duration = std::chrono::steady_clock::now() - connect_time_;
        if (connected_duration >= LOGIN_TIMEOUT) {
            Log::info("Disconnecting {} - login timeout after {} seconds",
                      remote_address(),
                      std::chrono::duration_cast<std::chrono::seconds>(connected_duration).count());
            disconnect("Login timeout - please reconnect to try again");
            return;
        }
    }

    // Continue the timer if connection is still active
    if (is_connected()) {
        start_idle_timer();
    }
}

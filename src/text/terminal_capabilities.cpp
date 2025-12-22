#include "terminal_capabilities.hpp"
#include "string_utils.hpp"
#include <cstdlib>
#include <string>
#include <algorithm>
#include <locale>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace TerminalCapabilities {

Capabilities detect_capabilities_from_mtts(uint32_t bitvector, std::string_view terminal_type) {
    Capabilities caps;
    caps.detection_method = DetectionMethod::MTTS;
    caps.mtts_bitvector = bitvector;
    caps.terminal_name = terminal_type;
    
    // Parse MTTS bitvector
    caps.supports_color = (bitvector & MTTS::ANSI) != 0;
    caps.supports_unicode = (bitvector & MTTS::UTF8) != 0;
    caps.supports_256_color = (bitvector & MTTS::COLOR_256) != 0;
    caps.supports_true_color = (bitvector & MTTS::TRUECOLOR) != 0;
    caps.supports_mouse = (bitvector & MTTS::MOUSE) != 0;
    caps.supports_screen_reader = (bitvector & MTTS::SCREEN_READER) != 0;
    caps.supports_tls = (bitvector & MTTS::TLS) != 0;
    
    // Basic formatting support assumed with ANSI
    if (caps.supports_color) {
        caps.supports_bold = true;
        caps.supports_underline = true;
        caps.supports_italic = true; // Most modern ANSI terminals support italic
    }
    
    // Set overall support level
    if (caps.supports_true_color && caps.supports_unicode) {
        caps.overall_level = SupportLevel::Full;
    } else if (caps.supports_256_color) {
        caps.overall_level = SupportLevel::Extended;
    } else if (caps.supports_color) {
        caps.overall_level = SupportLevel::Standard;
    } else {
        caps.overall_level = SupportLevel::None;
    }
    
    return caps;
}

Capabilities detect_capabilities_from_gmcp(const nlohmann::json& client_info) {
    Capabilities caps;
    caps.detection_method = DetectionMethod::GMCP;
    caps.supports_gmcp = true;
    
    try {
        // Parse client information from GMCP
        if (client_info.contains("client")) {
            caps.client_name = client_info["client"].get<std::string>();
        }
        if (client_info.contains("version")) {
            caps.client_version = client_info["version"].get<std::string>();
        }
        
        // Known client capabilities
        std::string client_lower = to_lowercase(caps.client_name);

        if (client_lower.find("mudlet") != std::string::npos) {
            // Mudlet supports most modern features
            caps.supports_color = true;
            caps.supports_256_color = true;
            caps.supports_true_color = true;
            caps.supports_unicode = true;
            caps.supports_bold = true;
            caps.supports_italic = true;
            caps.supports_underline = true;
            caps.supports_hyperlinks = true;
            caps.overall_level = SupportLevel::Full;
        } else if (client_lower.find("mushclient") != std::string::npos) {
            caps.supports_color = true;
            caps.supports_256_color = true;
            caps.supports_unicode = true;
            caps.supports_bold = true;
            caps.supports_underline = true;
            caps.overall_level = SupportLevel::Extended;
        } else if (client_lower.find("tintin") != std::string::npos) {
            caps.supports_color = true;
            caps.supports_256_color = true;
            caps.supports_unicode = true;
            caps.supports_bold = true;
            caps.supports_underline = true;
            caps.overall_level = SupportLevel::Extended;
        } else {
            // Unknown GMCP client - assume basic capabilities
            caps.supports_color = true;
            caps.supports_bold = true;
            caps.supports_underline = true;
            caps.overall_level = SupportLevel::Standard;
        }
    } catch (const nlohmann::json::exception& e) {
        // Fallback to basic capabilities on JSON parse error
        caps.supports_color = true;
        caps.supports_bold = true;
        caps.overall_level = SupportLevel::Standard;
    }
    
    return caps;
}

Capabilities detect_capabilities_from_new_environ(const std::unordered_map<std::string, std::string>& env_vars) {
    Capabilities caps;
    caps.detection_method = DetectionMethod::NewEnviron;
    
    // Parse NEW-ENVIRON variables
    auto it = env_vars.find("TERM");
    if (it != env_vars.end()) {
        caps.terminal_name = it->second;
    }
    
    it = env_vars.find("COLORTERM");
    if (it != env_vars.end() && (it->second == "truecolor" || it->second == "24bit")) {
        caps.supports_true_color = true;
        caps.supports_256_color = true;
        caps.supports_color = true;
    }
    
    it = env_vars.find("CLIENT_NAME");
    if (it != env_vars.end()) {
        caps.client_name = it->second;
    }
    
    it = env_vars.find("CLIENT_VERSION");
    if (it != env_vars.end()) {
        caps.client_version = it->second;
    }
    
    it = env_vars.find("CHARSET");
    if (it != env_vars.end()) {
        std::string charset = to_lowercase(it->second);
        caps.supports_unicode = charset.find("utf") != std::string::npos;
    }
    
    // Apply terminal-specific capabilities if we have a terminal name
    if (!caps.terminal_name.empty()) {
        Capabilities term_caps = get_capabilities_for_terminal(caps.terminal_name);
        // Merge capabilities, preferring NEW-ENVIRON data where available
        if (!caps.supports_color) caps.supports_color = term_caps.supports_color;
        if (!caps.supports_256_color) caps.supports_256_color = term_caps.supports_256_color;
        if (!caps.supports_true_color) caps.supports_true_color = term_caps.supports_true_color;
        if (!caps.supports_unicode) caps.supports_unicode = term_caps.supports_unicode;
        caps.supports_bold = term_caps.supports_bold;
        caps.supports_italic = term_caps.supports_italic;
        caps.supports_underline = term_caps.supports_underline;
    }
    
    // Set overall support level
    if (caps.supports_true_color && caps.supports_unicode) {
        caps.overall_level = SupportLevel::Full;
    } else if (caps.supports_256_color) {
        caps.overall_level = SupportLevel::Extended;
    } else if (caps.supports_color) {
        caps.overall_level = SupportLevel::Standard;
    } else {
        caps.overall_level = SupportLevel::None;
    }
    
    return caps;
}

Capabilities detect_capabilities() {
    // Return conservative defaults for unknown clients
    // Actual detection should use client-specific methods: MTTS, GMCP, or NEW-ENVIRON
    Capabilities caps;
    caps.terminal_name = "Telnet Client";
    caps.client_name = "Unknown";
    caps.client_version = "";
    caps.detection_method = DetectionMethod::Environment;
    caps.overall_level = SupportLevel::Basic;
    
    // Assume basic capabilities for standard telnet clients
    caps.supports_color = true;
    caps.supports_bold = true;
    caps.supports_underline = true;
    
    return caps;
}

Capabilities get_capabilities_for_terminal(std::string_view terminal_name) {
    Capabilities caps;
    caps.terminal_name = terminal_name;

    std::string term_lower = to_lowercase(terminal_name);

    // Modern terminals with full support
    if (term_lower.find("alacritty") != std::string::npos ||
        term_lower.find("kitty") != std::string::npos ||
        term_lower.find("wezterm") != std::string::npos ||
        term_lower.find("iterm2") != std::string::npos) {
        caps.supports_color = true;
        caps.supports_256_color = true;
        caps.supports_true_color = true;
        caps.supports_unicode = true;
        caps.supports_bold = true;
        caps.supports_italic = true;
        caps.supports_underline = true;
        caps.overall_level = SupportLevel::Full;
    }
    // Good modern terminals
    else if (term_lower.find("xterm-256") != std::string::npos ||
             term_lower.find("screen-256") != std::string::npos ||
             term_lower.find("tmux-256") != std::string::npos ||
             term_lower.find("gnome-terminal") != std::string::npos ||
             term_lower.find("konsole") != std::string::npos) {
        caps.supports_color = true;
        caps.supports_256_color = true;
        caps.supports_unicode = true;
        caps.supports_bold = true;
        caps.supports_italic = true;
        caps.supports_underline = true;
        caps.overall_level = SupportLevel::Extended;
    }
    // Basic xterm and compatible
    else if (term_lower.find("xterm") != std::string::npos ||
             term_lower.find("rxvt") != std::string::npos ||
             term_lower.find("screen") != std::string::npos ||
             term_lower.find("tmux") != std::string::npos) {
        caps.supports_color = true;
        caps.supports_bold = true;
        caps.supports_underline = true;
        caps.overall_level = SupportLevel::Standard;
    }
    // Linux console
    else if (term_lower == "linux") {
        caps.supports_color = true;
        caps.supports_bold = true;
        caps.overall_level = SupportLevel::Basic;
    }
    
    return caps;
}

ProgressChars get_progress_chars(const Capabilities& caps) {
    ProgressChars chars;
    
    if (caps.supports_unicode) {
        chars.filled = "█";  // Full block
        chars.empty = "░";   // Light shade
    } else {
        chars.filled = "#";  // ASCII hash
        chars.empty = "-";   // ASCII dash
    }
    
    return chars;
}

TableChars get_table_chars(const Capabilities& caps) {
    TableChars chars;
    
    if (caps.supports_unicode) {
        chars.corner = "┌";     // Box drawing
        chars.horizontal = "─";
        chars.vertical = "│";
        chars.cross = "┼";
    } else {
        chars.corner = "+";     // ASCII fallback
        chars.horizontal = "-";
        chars.vertical = "|";
        chars.cross = "+";
    }
    
    return chars;
}

} // namespace TerminalCapabilities
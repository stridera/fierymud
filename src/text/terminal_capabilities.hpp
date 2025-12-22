// MTTS and TTYPE terminal capability detection via telnet negotiation

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

/**
 * Terminal capability detection for optimal rich text formatting.
 *
 * This system detects what features the user's terminal supports
 * and adapts the rich text output accordingly.
 */
namespace TerminalCapabilities {

/** Terminal feature support levels */
enum class SupportLevel {
    None,     // No support
    Basic,    // Basic ANSI colors only
    Standard, // 16 colors + basic formatting
    Extended, // 256 colors + advanced formatting
    Full      // True color (24-bit) + Unicode + all features
};

/** Client capability detection methods */
enum class DetectionMethod {
    Environment, // Server-side environment variables (legacy)
    MTTS,        // Mud Terminal Type Standard
    GMCP,        // Generic Mud Communication Protocol
    NewEnviron   // NEW-ENVIRON telnet protocol
};

/** Detected terminal capabilities */
struct Capabilities {
    // Basic display support
    bool supports_color = false;
    bool supports_256_color = false;
    bool supports_true_color = false;
    bool supports_unicode = false;
    bool supports_bold = false;
    bool supports_italic = false;
    bool supports_underline = false;

    // Client information
    std::string terminal_name = "unknown";
    std::string client_name = "unknown";
    std::string client_version = "unknown";

    // Advanced capabilities
    bool supports_gmcp = false;
    bool supports_screen_reader = false;
    bool supports_tls = false;
    bool supports_mouse = false;
    bool supports_hyperlinks = false;

    // Capability source
    DetectionMethod detection_method = DetectionMethod::Environment;
    SupportLevel overall_level = SupportLevel::None;

    // MTTS bitvector (when using MTTS)
    uint32_t mtts_bitvector = 0;
};

/** Detect capabilities from MTTS bitvector */
Capabilities detect_capabilities_from_mtts(uint32_t bitvector, std::string_view terminal_type = "");

/** Detect capabilities from GMCP client information */
Capabilities detect_capabilities_from_gmcp(const nlohmann::json &client_info);

/** Detect capabilities from NEW-ENVIRON data */
Capabilities detect_capabilities_from_new_environ(const std::unordered_map<std::string, std::string> &env_vars);

/** Get best available capabilities (tries multiple detection methods) */
Capabilities detect_capabilities();

/** Get capabilities for specific terminal type */
Capabilities get_capabilities_for_terminal(std::string_view terminal_name);

/** MTTS bitvector constants */
namespace MTTS {
constexpr uint32_t ANSI = 1;               // ANSI color codes
constexpr uint32_t VT100 = 2;              // VT100 sequences
constexpr uint32_t UTF8 = 4;               // UTF-8 character encoding
constexpr uint32_t COLOR_256 = 8;          // 256 color support
constexpr uint32_t MOUSE = 16;             // Mouse tracking
constexpr uint32_t OSC_COLOR_PALETTE = 32; // OSC color palette
constexpr uint32_t SCREEN_READER = 64;     // Screen reader friendly
constexpr uint32_t PROXY = 128;            // Behind a proxy
constexpr uint32_t TRUECOLOR = 256;        // 24-bit truecolor support
constexpr uint32_t MNES = 512;             // Mud New Environment Standard
constexpr uint32_t TLS = 1024;             // TLS encryption support
} // namespace MTTS

/** Get appropriate progress bar characters for terminal */
struct ProgressChars {
    std::string filled;
    std::string empty;
};
ProgressChars get_progress_chars(const Capabilities &caps);

/** Get appropriate table border characters for terminal */
struct TableChars {
    std::string corner = "+";
    std::string horizontal = "-";
    std::string vertical = "|";
    std::string cross = "+";
};
TableChars get_table_chars(const Capabilities &caps);
} // namespace TerminalCapabilities
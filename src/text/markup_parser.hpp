// XML-like markup parser for player-authored content

#pragma once

#include "terminal_capabilities.hpp"
#include "rich_text.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <expected>

/**
 * XML-Lite Markup Parser for FieryMUD
 *
 * Parses player-authored markup like:
 *   <red>Red text</red>
 *   <b:blue>Bold blue</b>
 *   <#FF0000>RGB red</#FF0000>
 *   <c196:bgc235>256-color indexed</c196>
 *
 * See docs/COLOR_CODES_XMLLITE.md for complete specification.
 */

namespace MarkupParser {

//////////////////////////////////////////////////////////////////////////////
// Color Representation
//////////////////////////////////////////////////////////////////////////////

/** Color types supported by markup system */
enum class ColorType {
    None,           // No color specified
    Named,          // Named 16-color (red, green, blue, etc.)
    Indexed,        // 256-color palette (c0-c255)
    RGB             // 24-bit truecolor (#RRGGBB)
};

/** Color value with type discrimination */
struct ColorValue {
    ColorType type = ColorType::None;

    // Type-specific values
    Color named_color = Color::Default;                  // For ColorType::Named
    int indexed_value = 0;                               // For ColorType::Indexed (0-255)
    struct { int r, g, b; } rgb_value = {0, 0, 0};      // For ColorType::RGB

    /** Create no color */
    static ColorValue none() { return ColorValue{}; }

    /** Create named color */
    static ColorValue named(Color color);

    /** Create indexed color (0-255) */
    static ColorValue indexed(int value);

    /** Create RGB color */
    static ColorValue rgb(int r, int g, int b);

    /** Convert to ANSI escape sequence for foreground */
    std::string to_foreground_ansi(const TerminalCapabilities::Capabilities& caps) const;

    /** Convert to ANSI escape sequence for background */
    std::string to_background_ansi(const TerminalCapabilities::Capabilities& caps) const;

    /** Check if color is specified */
    bool has_color() const { return type != ColorType::None; }
};

//////////////////////////////////////////////////////////////////////////////
// Style Layer (Stack Element)
//////////////////////////////////////////////////////////////////////////////

/** Single layer in the style stack */
struct StyleLayer {
    std::string tag_name;                      // Empty for anonymous tags

    // Text attributes
    bool bold = false;
    bool dim = false;
    bool underline = false;
    bool italic = false;
    bool strikethrough = false;
    bool blink = false;
    bool reverse = false;
    bool hidden = false;

    // Colors
    std::optional<ColorValue> foreground;
    std::optional<ColorValue> background;

    /** Check if layer has any styling */
    bool has_styling() const;

    /** Generate ANSI codes for this layer only */
    std::string to_ansi(const TerminalCapabilities::Capabilities& caps) const;
};

/** Complete style state (combined from all stack layers) */
struct StyleState {
    bool bold = false;
    bool dim = false;
    bool underline = false;
    bool italic = false;
    bool strikethrough = false;
    bool blink = false;
    bool reverse = false;
    bool hidden = false;
    std::optional<ColorValue> foreground;
    std::optional<ColorValue> background;

    /** Generate ANSI escape sequence for current state */
    std::string to_ansi(const TerminalCapabilities::Capabilities& caps) const;
};

//////////////////////////////////////////////////////////////////////////////
// Modifiers
//////////////////////////////////////////////////////////////////////////////

/** Modifier type classification */
enum class ModifierType {
    Attribute,              // b, dim, u, i, s, blink, reverse, hide
    NamedForeground,        // red, green, blue, etc.
    NamedBackground,        // bg-red, bg-green, etc.
    IndexedForeground,      // cN (0-255)
    IndexedBackground,      // bgcN (0-255)
    RGBForeground,          // #RRGGBB
    RGBBackground,          // bg#RRGGBB
    Unknown
};

/** Single parsed modifier */
struct Modifier {
    ModifierType type;
    std::string value;      // Original token

    // Type-specific parsed values
    Color named_color = Color::Default;
    int indexed_value = 0;
    struct { int r, g, b; } rgb_value = {0, 0, 0};
};

/** Parse a single modifier token */
std::expected<Modifier, std::string> parse_modifier(std::string_view token);

/** Parse a color name to Color enum */
std::optional<Color> parse_named_color(std::string_view name);

/** Parse hex color string to RGB */
std::optional<std::tuple<int, int, int>> parse_hex_color(std::string_view hex);

//////////////////////////////////////////////////////////////////////////////
// Tag Parsing
//////////////////////////////////////////////////////////////////////////////

/** Tag type */
enum class TagType {
    Opening,        // <name> or <name:mod>
    ClosingNamed,   // </name>
    ClosingReset,   // </>
    Text            // Not a tag
};

/** Parsed tag information */
struct Tag {
    TagType type;
    std::string tag_name;                  // Empty for anonymous or reset
    std::vector<Modifier> modifiers;       // For opening tags
    size_t start_pos;                      // Position in source string
    size_t end_pos;                        // Position after tag
    std::string_view text;                 // For TagType::Text only
};

/** Parse next tag or text segment from input */
std::expected<Tag, std::string> parse_next_tag(std::string_view input, size_t start_pos);

//////////////////////////////////////////////////////////////////////////////
// Style Stack
//////////////////////////////////////////////////////////////////////////////

/** Stack-based style manager */
class StyleStack {
public:
    StyleStack() = default;

    /** Push a new style layer */
    void push(const StyleLayer& layer);

    /** Pop the most recent layer with given tag name */
    bool pop_named(std::string_view tag_name);

    /** Clear entire stack (full reset) */
    void clear();

    /** Get current combined style state */
    StyleState get_current_state() const;

    /** Get stack depth */
    size_t depth() const { return stack_.size(); }

    /** Check if stack is empty */
    bool empty() const { return stack_.empty(); }

private:
    std::vector<StyleLayer> stack_;
};

//////////////////////////////////////////////////////////////////////////////
// Parser Configuration
//////////////////////////////////////////////////////////////////////////////

/** Parser configuration and limits */
struct ParserConfig {
    size_t max_markup_length = 4096;       // Maximum total string length
    size_t max_tag_name_length = 32;       // Maximum tag name length
    size_t max_nesting_depth = 16;         // Maximum style stack depth
    size_t max_modifier_count = 8;         // Maximum modifiers per tag
    size_t max_tag_count = 100;            // Maximum total tags in string

    bool strict_mode = false;              // Strict error handling vs. best-effort
    bool allow_anonymous_tags = true;      // Allow tags without names
};

/** Default parser configuration */
constexpr ParserConfig default_config() {
    return ParserConfig{};
}

//////////////////////////////////////////////////////////////////////////////
// Parsing Modes
//////////////////////////////////////////////////////////////////////////////

/** Markup processing mode */
enum class ProcessingMode {
    Parse,      // Convert markup to ANSI
    Strip,      // Remove all markup (plain text)
    Validate    // Check syntax without rendering
};

//////////////////////////////////////////////////////////////////////////////
// Main Parser
//////////////////////////////////////////////////////////////////////////////

/** Result of markup parsing */
struct ParseResult {
    std::string output;                    // Rendered output (ANSI or plain)
    std::vector<std::string> warnings;     // Non-fatal issues
    bool success = true;                   // Overall success flag
};

/** Parse and render XML-Lite markup to ANSI */
ParseResult parse_markup(
    std::string_view markup,
    const TerminalCapabilities::Capabilities& caps = TerminalCapabilities::detect_capabilities(),
    const ParserConfig& config = default_config()
);

/** Strip all markup and return plain text */
std::string strip_markup(
    std::string_view markup,
    const ParserConfig& config = default_config()
);

/** Validate markup syntax without rendering */
std::expected<void, std::vector<std::string>> validate_markup(
    std::string_view markup,
    const ParserConfig& config = default_config()
);

//////////////////////////////////////////////////////////////////////////////
// Utility Functions
//////////////////////////////////////////////////////////////////////////////

/** Calculate visible length of markup string (excluding tags) */
size_t markup_length(std::string_view markup);

/** Truncate markup string to visible length (preserving tag structure) */
std::string truncate_markup(std::string_view markup, size_t max_visible_length);

/** Escape markup characters for literal display */
std::string escape_markup(std::string_view text);

/** Preview markup with terminal capabilities */
std::string preview_markup(
    std::string_view markup,
    TerminalCapabilities::SupportLevel level
);

//////////////////////////////////////////////////////////////////////////////
// Named Color Mapping
//////////////////////////////////////////////////////////////////////////////

/** Map of named color strings to Color enum */
extern const std::unordered_map<std::string_view, Color> named_color_map;

/** Map of Color enum to RGB values (for downsampling) */
extern const std::unordered_map<Color, std::tuple<int, int, int>> color_rgb_map;

//////////////////////////////////////////////////////////////////////////////
// Color Downsampling
//////////////////////////////////////////////////////////////////////////////

/** Find nearest color in 256-color palette */
int nearest_256_color(int r, int g, int b);

/** Find nearest ANSI 16-color */
Color nearest_ansi_color(int r, int g, int b);

/** Downsample ColorValue based on terminal capabilities */
ColorValue downsample_color(
    const ColorValue& color,
    const TerminalCapabilities::Capabilities& caps
);

//////////////////////////////////////////////////////////////////////////////
// Error Types
//////////////////////////////////////////////////////////////////////////////

/** Parse error types */
enum class ParseErrorType {
    InvalidTag,             // Malformed tag syntax
    UnclosedTag,            // Tag opened but never closed
    InvalidModifier,        // Unknown or malformed modifier
    NestingTooDeep,         // Stack depth exceeded
    StringTooLong,          // Input exceeds length limit
    TooManyTags,            // Tag count exceeded
    InvalidColor,           // Invalid color format
    TagNameTooLong,         // Tag name exceeds limit
    TooManyModifiers        // Modifier count exceeded
};

/** Detailed parse error */
struct ParseError {
    ParseErrorType type;
    std::string message;
    size_t position = 0;        // Position in input where error occurred
    std::string context;        // Surrounding text for context
};

/** Format parse error for display */
std::string format_parse_error(const ParseError& error, std::string_view input);

} // namespace MarkupParser

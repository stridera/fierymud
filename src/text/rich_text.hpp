// ANSI color codes and terminal formatting for MUD output

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <fmt/format.h>

// Forward declarations
namespace TerminalCapabilities { struct Capabilities; }

/**
 * Rich text formatting system for FieryMUD.
 * 
 * Provides comprehensive ANSI escape code support including:
 * - 16 standard colors + 256 extended colors + 16.7M RGB colors
 * - Text styles (bold, italic, underline, strikethrough, etc.)
 * - Background colors
 * - Builder pattern for easy composition
 * - Backward compatibility with existing color system
 */

/** Standard ANSI colors */
enum class Color : int {
    // Standard colors (30-37)
    Black = 30, Red = 31, Green = 32, Yellow = 33,
    Blue = 34, Magenta = 35, Cyan = 36, White = 37,
    
    // Bright colors (90-97)
    BrightBlack = 90, BrightRed = 91, BrightGreen = 92, BrightYellow = 93,
    BrightBlue = 94, BrightMagenta = 95, BrightCyan = 96, BrightWhite = 97,
    
    // Special
    Reset = 0, Default = 39
};

/** Background colors */
enum class BackgroundColor : int {
    // Standard backgrounds (40-47)
    Black = 40, Red = 41, Green = 42, Yellow = 43,
    Blue = 44, Magenta = 45, Cyan = 46, White = 47,
    
    // Bright backgrounds (100-107)  
    BrightBlack = 100, BrightRed = 101, BrightGreen = 102, BrightYellow = 103,
    BrightBlue = 104, BrightMagenta = 105, BrightCyan = 106, BrightWhite = 107,
    
    // Special
    Default = 49
};

/** Text styling options */
enum class TextStyle : int {
    Normal = 0, Bold = 1, Dim = 2, Italic = 3, Underline = 4,
    Blink = 5, Reverse = 7, Strikethrough = 9,
    
    // Reset individual styles
    NoBold = 22, NoDim = 22, NoItalic = 23, NoUnderline = 24,
    NoBlink = 25, NoReverse = 27, NoStrikethrough = 29
};

/** 256-color palette support (0-255) */
struct Color256 {
    int value; // 0-255
    
    constexpr Color256(int v) : value(v) {}
    std::string to_foreground() const { return fmt::format("\033[38;5;{}m", value); }
    std::string to_background() const { return fmt::format("\033[48;5;{}m", value); }
};

/** RGB/True color support (24-bit) */
struct ColorRGB {
    int r, g, b;
    
    constexpr ColorRGB(int r, int g, int b) : r(r), g(g), b(b) {}
    std::string to_foreground() const { return fmt::format("\033[38;2;{};{};{}m", r, g, b); }
    std::string to_background() const { return fmt::format("\033[48;2;{};{};{}m", r, g, b); }
};

/** Combined text formatting specification for ANSI output */
struct RichTextFormat {
    Color foreground = Color::Default;
    BackgroundColor background = BackgroundColor::Default;
    std::vector<TextStyle> styles;

    /** Generate ANSI escape sequence for this format */
    std::string to_ansi() const;

    /** Check if format has any styling */
    bool has_formatting() const;

    /** Create format with foreground color */
    static RichTextFormat with_color(Color color);

    /** Create format with background color */
    static RichTextFormat with_background(BackgroundColor color);

    /** Create format with text style */
    static RichTextFormat with_style(TextStyle style);

    /** Create format with RGB color */
    static RichTextFormat with_rgb(const ColorRGB& color);

    /** Create format with 256-color palette */
    static RichTextFormat with_256(const Color256& color);
};

/** Rich text builder with formatting support */
class RichText {
public:
    /** Create empty rich text */
    RichText() = default;
    
    /** Create rich text from plain string */
    explicit RichText(std::string_view text);
    
    /** Create rich text with terminal capabilities */
    explicit RichText(const TerminalCapabilities::Capabilities& caps);
    
    /** Create rich text from plain string with terminal capabilities */
    RichText(std::string_view text, const TerminalCapabilities::Capabilities& caps);
    
    /** Set terminal capabilities for adaptive formatting */
    void set_capabilities(const TerminalCapabilities::Capabilities& caps);
    
    // Builder pattern methods
    
    /** Append plain text */
    RichText& text(std::string_view str);
    
    /** Append colored text */
    RichText& colored(std::string_view str, Color color);
    
    /** Append RGB colored text */
    RichText& rgb(std::string_view str, const ColorRGB& color);
    
    /** Append 256-color text */
    RichText& color256(std::string_view str, const Color256& color);
    
    /** Append bold text */
    RichText& bold(std::string_view str);
    
    /** Append italic text */
    RichText& italic(std::string_view str);
    
    /** Append underlined text */
    RichText& underline(std::string_view str);
    
    /** Append strikethrough text */
    RichText& strikethrough(std::string_view str);
    
    /** Append highlighted text (foreground + background) */
    RichText& highlight(std::string_view str, Color fg, BackgroundColor bg);
    
    /** Append text with custom formatting */
    RichText& formatted(std::string_view str, const RichTextFormat& format);
    
    // Special formatting methods
    
    /** Add progress bar (uses adaptive characters based on terminal capabilities) */
    RichText& progress_bar(float percentage, int width = 20);
    
    /** Add progress bar with custom characters */
    RichText& progress_bar(float percentage, int width, 
                          std::string_view filled, std::string_view empty);
    
    /** Add table row with columns */
    RichText& table_row(const std::vector<std::string>& columns, 
                       const std::vector<int>& widths = {});
    
    /** Add horizontal separator line */
    RichText& separator(char ch = '-', int width = 60, Color color = Color::BrightBlack);
    
    /** Add code block with syntax highlighting hint */
    RichText& code_block(std::string_view code, std::string_view language = "");
    
    // Output methods
    
    /** Generate ANSI-formatted string */
    std::string to_ansi() const;
    
    /** Generate plain text (strip all formatting) */
    std::string to_plain() const;
    
    /** Get character count (excluding formatting codes) */
    size_t plain_length() const;
    
    /** Check if empty */
    bool empty() const;
    
    /** Clear all content and formatting */
    void clear();

private:
    std::string content_;
    std::vector<std::pair<size_t, std::string>> format_inserts_;
    const TerminalCapabilities::Capabilities* capabilities_ = nullptr;
    
    void insert_format(const RichTextFormat& format);
    void insert_reset();
};

/** Predefined semantic colors for game elements */
namespace Colors {
    // Game status colors
    constexpr ColorRGB Health{220, 20, 60};        // Crimson
    constexpr ColorRGB Mana{30, 144, 255};         // DodgerBlue  
    constexpr ColorRGB Movement{50, 205, 50};      // LimeGreen
    constexpr ColorRGB Experience{255, 215, 0};    // Gold
    
    // Combat colors
    constexpr ColorRGB Damage{255, 69, 0};         // OrangeRed
    constexpr ColorRGB Healing{50, 205, 50};       // LimeGreen
    constexpr ColorRGB Miss{128, 128, 128};        // Gray
    constexpr ColorRGB Critical{255, 20, 147};     // DeepPink
    
    // Object quality colors
    constexpr ColorRGB Common{192, 192, 192};      // Silver
    constexpr ColorRGB Uncommon{30, 255, 0};       // Lime
    constexpr ColorRGB Rare{0, 112, 221};          // Blue
    constexpr ColorRGB Epic{163, 53, 238};         // Purple
    constexpr ColorRGB Legendary{255, 128, 0};     // Orange
    
    // UI element colors
    constexpr Color Border = Color::BrightBlack;
    constexpr Color Header = Color::BrightWhite;
    constexpr Color Accent = Color::BrightCyan;
    constexpr Color Success = Color::BrightGreen;
    constexpr Color Warning = Color::BrightYellow;
    constexpr Color Error = Color::BrightRed;
    constexpr Color Info = Color::BrightBlue;
    
    // Environment colors
    constexpr ColorRGB Grass{34, 139, 34};         // ForestGreen
    constexpr ColorRGB Water{0, 191, 255};         // DeepSkyBlue
    constexpr ColorRGB Fire{255, 69, 0};           // OrangeRed
    constexpr ColorRGB Stone{105, 105, 105};       // DimGray
    constexpr ColorRGB Wood{139, 69, 19};          // SaddleBrown
    constexpr ColorRGB Metal{192, 192, 192};       // Silver
}

/** Convenient formatting functions */
namespace Format {
    /** Format health display with color coding */
    RichText health_bar(int current, int max, int width = 20);
    
    /** Format damage number with appropriate color */
    RichText damage_text(int amount);
    
    /** Format healing number with appropriate color */
    RichText healing_text(int amount);
    
    /** Format object name with quality color */
    RichText object_name(std::string_view name, std::string_view quality = "common");
    
    /** Format table with headers and borders (adaptive based on capabilities) */
    RichText table(const std::vector<std::string>& headers, 
                  const std::vector<std::vector<std::string>>& rows);
                  
    /** Format table with specific terminal capabilities */
    RichText table(const std::vector<std::string>& headers, 
                  const std::vector<std::vector<std::string>>& rows,
                  const TerminalCapabilities::Capabilities& caps);
    
    /** Format command syntax help */
    RichText command_syntax(std::string_view command, std::string_view syntax);
    
    /** Format error message with appropriate styling */
    RichText error_message(std::string_view message);
    
    /** Format success message with appropriate styling */
    RichText success_message(std::string_view message);
}
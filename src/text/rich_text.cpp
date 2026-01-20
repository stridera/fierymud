#include "rich_text.hpp"
#include "terminal_capabilities.hpp"
#include <algorithm>
#include <numeric>
#include <regex>

//////////////////////////////////////////////////////////////////////////////
// RichTextFormat Implementation
//////////////////////////////////////////////////////////////////////////////

std::string RichTextFormat::to_ansi() const {
    std::vector<int> codes;
    
    if (foreground != Color::Default) {
        codes.push_back(static_cast<int>(foreground));
    }
    if (background != BackgroundColor::Default) {
        codes.push_back(static_cast<int>(background));
    }
    for (auto style : styles) {
        codes.push_back(static_cast<int>(style));
    }
    
    if (codes.empty()) {
        return "";
    }
    
    std::string result = "\033[";
    for (size_t i = 0; i < codes.size(); ++i) {
        if (i > 0) result += ";";
        result += std::to_string(codes[i]);
    }
    result += "m";
    return result;
}

bool RichTextFormat::has_formatting() const {
    return foreground != Color::Default ||
           background != BackgroundColor::Default ||
           !styles.empty();
}

RichTextFormat RichTextFormat::with_color(Color color) {
    RichTextFormat format;
    format.foreground = color;
    return format;
}

RichTextFormat RichTextFormat::with_background(BackgroundColor color) {
    RichTextFormat format;
    format.background = color;
    return format;
}

RichTextFormat RichTextFormat::with_style(TextStyle style) {
    RichTextFormat format;
    format.styles.push_back(style);
    return format;
}


//////////////////////////////////////////////////////////////////////////////
// RichText Implementation
//////////////////////////////////////////////////////////////////////////////

RichText::RichText(std::string_view text) : content_{text} {}

RichText::RichText(const TerminalCapabilities::Capabilities& caps) : capabilities_(&caps) {}

RichText::RichText(std::string_view text, const TerminalCapabilities::Capabilities& caps) 
    : content_{text}, capabilities_(&caps) {}

void RichText::set_capabilities(const TerminalCapabilities::Capabilities& caps) {
    capabilities_ = &caps;
}

RichText& RichText::text(std::string_view str) {
    content_ += str;
    return *this;
}

RichText& RichText::colored(std::string_view str, Color color) {
    if (color != Color::Default) {
        insert_format(RichTextFormat::with_color(color));
    }
    content_ += str;
    if (color != Color::Default) {
        insert_reset();
    }
    return *this;
}

RichText& RichText::rgb(std::string_view str, const ColorRGB& color) {
    format_inserts_.emplace_back(content_.size(), color.to_foreground());
    content_ += str;
    format_inserts_.emplace_back(content_.size(), "\033[0m");
    return *this;
}

RichText& RichText::color256(std::string_view str, const Color256& color) {
    format_inserts_.emplace_back(content_.size(), color.to_foreground());
    content_ += str;
    format_inserts_.emplace_back(content_.size(), "\033[0m");
    return *this;
}

RichText& RichText::bold(std::string_view str) {
    insert_format(RichTextFormat::with_style(TextStyle::Bold));
    content_ += str;
    insert_reset();
    return *this;
}

RichText& RichText::italic(std::string_view str) {
    insert_format(RichTextFormat::with_style(TextStyle::Italic));
    content_ += str;
    insert_reset();
    return *this;
}

RichText& RichText::underline(std::string_view str) {
    insert_format(RichTextFormat::with_style(TextStyle::Underline));
    content_ += str;
    insert_reset();
    return *this;
}

RichText& RichText::strikethrough(std::string_view str) {
    insert_format(RichTextFormat::with_style(TextStyle::Strikethrough));
    content_ += str;
    insert_reset();
    return *this;
}

RichText& RichText::highlight(std::string_view str, Color fg, BackgroundColor bg) {
    RichTextFormat format;
    format.foreground = fg;
    format.background = bg;
    insert_format(format);
    content_ += str;
    insert_reset();
    return *this;
}

RichText& RichText::formatted(std::string_view str, const RichTextFormat& format) {
    if (format.has_formatting()) {
        insert_format(format);
    }
    content_ += str;
    if (format.has_formatting()) {
        insert_reset();
    }
    return *this;
}

RichText& RichText::progress_bar(float percentage, int width) {
    // Use adaptive characters based on terminal capabilities
    std::string filled_char = "#";  // ASCII fallback
    std::string empty_char = "-";   // ASCII fallback
    
    if (capabilities_ && capabilities_->supports_unicode) {
        filled_char = "█";  // Full block
        empty_char = "░";   // Light shade
    }
    
    return progress_bar(percentage, width, filled_char, empty_char);
}

RichText& RichText::progress_bar(float percentage, int width, std::string_view filled, std::string_view empty) {
    percentage = std::clamp(percentage, 0.0f, 1.0f);
    int filled_count = static_cast<int>(percentage * width);
    int empty_count = width - filled_count;
    
    // Green for healthy, yellow for moderate, red for low
    Color bar_color = Color::BrightGreen;
    if (percentage < 0.3f) bar_color = Color::BrightRed;
    else if (percentage < 0.6f) bar_color = Color::BrightYellow;
    
    // Build progress bar with repeated characters
    std::string filled_str, empty_str;
    for (int i = 0; i < filled_count; ++i) {
        filled_str += filled;
    }
    for (int i = 0; i < empty_count; ++i) {
        empty_str += empty;
    }
    
    colored(filled_str, bar_color);
    colored(empty_str, Color::BrightBlack);
    
    return *this;
}

RichText& RichText::table_row(const std::vector<std::string>& columns, const std::vector<int>& widths) {
    text("│");
    
    for (size_t i = 0; i < columns.size(); ++i) {
        text(" ");
        
        int width = (i < widths.size()) ? widths[i] : 15;
        std::string padded = columns[i];
        if (static_cast<int>(padded.length()) > width) {
            padded = padded.substr(0, width - 3) + "...";
        } else {
            padded.resize(width, ' ');
        }
        
        text(padded);
        text(" │");
    }
    text("\n");
    
    return *this;
}

RichText& RichText::separator(char ch, int width, Color color) {
    colored(std::string(width, ch), color);
    text("\n");
    return *this;
}

RichText& RichText::code_block(std::string_view code, std::string_view language) {
    // Simple syntax highlighting based on language hint
    if (language == "cpp" || language == "c++") {
        // Basic C++ keyword highlighting
        std::string code_str{code};
        
        // This is a simplified example - real syntax highlighting would be much more complex
        text("```\n");
        colored(code_str, Color::BrightWhite);
        text("\n```");
    } else {
        text("```\n");
        colored(code, Color::White);
        text("\n```");
    }
    
    return *this;
}

std::string RichText::to_ansi() const {
    if (format_inserts_.empty()) {
        return content_;
    }
    
    std::string result;
    result.reserve(content_.size() + format_inserts_.size() * 10); // Rough estimate
    
    // Sort format inserts by position
    auto sorted_formats = format_inserts_;
    std::sort(sorted_formats.begin(), sorted_formats.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    
    size_t content_pos = 0;
    for (const auto& [pos, format_code] : sorted_formats) {
        // Add content up to this format position
        if (pos > content_pos) {
            result += content_.substr(content_pos, pos - content_pos);
            content_pos = pos;
        }
        
        // Add format code
        result += format_code;
    }
    
    // Add remaining content
    if (content_pos < content_.size()) {
        result += content_.substr(content_pos);
    }
    
    return result;
}

std::string RichText::to_plain() const {
    return content_;
}

size_t RichText::plain_length() const {
    return content_.size();
}

bool RichText::empty() const {
    return content_.empty();
}

void RichText::clear() {
    content_.clear();
    format_inserts_.clear();
}

void RichText::insert_format(const RichTextFormat& format) {
    if (format.has_formatting()) {
        format_inserts_.emplace_back(content_.size(), format.to_ansi());
    }
}

void RichText::insert_reset() {
    format_inserts_.emplace_back(content_.size(), "\033[0m");
}

//////////////////////////////////////////////////////////////////////////////
// Format Namespace Helper Functions
//////////////////////////////////////////////////////////////////////////////

namespace Format {

RichText health_bar(int current, int max, int width) {
    RichText result;
    
    float percentage = (max > 0) ? static_cast<float>(current) / max : 0.0f;
    
    result.text("Health: ");
    result.progress_bar(percentage, width);
    result.text(fmt::format(" {}/{}", current, max));
    
    return result;
}

RichText damage_text(int amount) {
    RichText result;
    result.rgb(std::to_string(amount), Colors::Damage);
    return result;
}

RichText healing_text(int amount) {
    RichText result;
    result.rgb("+" + std::to_string(amount), Colors::Healing);
    return result;
}

RichText object_name(std::string_view name, std::string_view quality) {
    RichText result;
    
    ColorRGB color = Colors::Common; // Default
    if (quality == "uncommon") color = Colors::Uncommon;
    else if (quality == "rare") color = Colors::Rare;
    else if (quality == "epic") color = Colors::Epic;
    else if (quality == "legendary") color = Colors::Legendary;
    
    result.rgb(name, color);
    return result;
}

RichText table(const std::vector<std::string>& headers, 
              const std::vector<std::vector<std::string>>& rows) {
    // Use default capabilities (will fall back to ASCII)
    auto caps = TerminalCapabilities::detect_capabilities();
    return table(headers, rows, caps);
}

RichText table(const std::vector<std::string>& headers, 
              const std::vector<std::vector<std::string>>& rows,
              const TerminalCapabilities::Capabilities& caps) {
    RichText result;
    result.set_capabilities(caps);
    
    if (headers.empty()) return result;
    
    // Calculate column widths
    std::vector<int> widths(headers.size(), 0);
    for (size_t i = 0; i < headers.size(); ++i) {
        widths[i] = std::max(widths[i], static_cast<int>(headers[i].length()));
    }
    
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], static_cast<int>(row[i].length()));
        }
    }
    
    // Add padding
    for (auto& width : widths) {
        width += 2; // Space on each side
    }
    
    // Get adaptive table characters
    std::string corner = "+";
    std::string horizontal = "-";
    std::string vertical = "|";
    
    if (caps.supports_unicode) {
        corner = "┌";
        horizontal = "─";
        vertical = "│";
    }
    
    // Top border
    result.colored(corner, Colors::Border);
    for (size_t i = 0; i < headers.size(); ++i) {
        // Build border line by repeating the horizontal character
        std::string border_line;
        for (int j = 0; j < widths[i]; ++j) {
            border_line += horizontal;
        }
        result.colored(border_line, Colors::Border);
        if (i < headers.size() - 1) {
            std::string cross = caps.supports_unicode ? "┬" : "+";
            result.colored(cross, Colors::Border);
        }
    }
    std::string top_right = caps.supports_unicode ? "┐" : "+";
    result.colored(top_right + "\n", Colors::Border);
    
    // Headers
    result.colored(vertical, Colors::Border);
    for (size_t i = 0; i < headers.size(); ++i) {
        result.text(" ");
        result.bold(headers[i]);
        // Pad to width
        int padding = widths[i] - static_cast<int>(headers[i].length()) - 1;
        result.text(std::string(padding, ' '));
        result.colored(vertical, Colors::Border);
    }
    result.text("\n");
    
    // Header separator
    std::string left_sep = caps.supports_unicode ? "├" : "+";
    std::string mid_sep = caps.supports_unicode ? "┼" : "+"; 
    std::string right_sep = caps.supports_unicode ? "┤" : "+";
    
    result.colored(left_sep, Colors::Border);
    for (size_t i = 0; i < headers.size(); ++i) {
        // Build border line by repeating the horizontal character
        std::string border_line;
        for (int j = 0; j < widths[i]; ++j) {
            border_line += horizontal;
        }
        result.colored(border_line, Colors::Border);
        if (i < headers.size() - 1) {
            result.colored(mid_sep, Colors::Border);
        }
    }
    result.colored(right_sep + "\n", Colors::Border);
    
    // Data rows
    for (const auto& row : rows) {
        result.colored(vertical, Colors::Border);
        for (size_t i = 0; i < headers.size(); ++i) {
            result.text(" ");
            
            std::string cell_content = (i < row.size()) ? row[i] : "";
            result.text(cell_content);
            
            int padding = widths[i] - static_cast<int>(cell_content.length()) - 1;
            result.text(std::string(padding, ' '));
            result.colored(vertical, Colors::Border);
        }
        result.text("\n");
    }
    
    // Bottom border
    std::string bottom_left = caps.supports_unicode ? "└" : "+";
    std::string bottom_mid = caps.supports_unicode ? "┴" : "+";
    std::string bottom_right = caps.supports_unicode ? "┘" : "+";
    
    result.colored(bottom_left, Colors::Border);
    for (size_t i = 0; i < headers.size(); ++i) {
        // Build border line by repeating the horizontal character
        std::string border_line;
        for (int j = 0; j < widths[i]; ++j) {
            border_line += horizontal;
        }
        result.colored(border_line, Colors::Border);
        if (i < headers.size() - 1) {
            result.colored(bottom_mid, Colors::Border);
        }
    }
    result.colored(bottom_right + "\n", Colors::Border);
    
    return result;
}

RichText command_syntax(std::string_view command, std::string_view syntax) {
    RichText result;
    
    result.colored("Usage: ", Colors::Info);
    result.bold(command);
    result.text(" ");
    result.text(syntax);
    
    return result;
}

RichText error_message(std::string_view message) {
    RichText result;
    result.colored("Error: ", Colors::Error);
    result.text(message);
    return result;
}

RichText success_message(std::string_view message) {
    RichText result;
    result.colored("✓ ", Colors::Success);
    result.text(message);
    return result;
}

} // namespace Format
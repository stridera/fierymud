#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/ids.hpp"
#include "core/result.hpp"

// Forward declarations
class Actor;
class Room;

/**
 * Modern command parsing system for FieryMUD.
 *
 * Handles:
 * - Input tokenization and argument parsing
 * - Command name resolution with abbreviations
 * - Fuzzy matching for user-friendly command finding
 * - Quote handling and string escaping
 * - Multi-word argument parsing
 * - Command history and repetition
 */

/** Parsed command structure */
struct ParsedCommand {
    std::string command;                // Resolved command name
    std::string original_input;         // Original user input
    std::vector<std::string> arguments; // Parsed arguments
    std::string full_argument_string;   // All arguments as single string

    // Parsing metadata
    bool is_abbreviated = false;  // Was command abbreviated?
    bool has_quoted_args = false; // Contains quoted arguments?
    int confidence_score = 100;   // Match confidence (0-100)

    ParsedCommand() = default;
    ParsedCommand(std::string_view cmd, std::vector<std::string> args = {}) : command(cmd), arguments(std::move(args)) {
        rebuild_argument_string();
    }

    /** Get argument count */
    size_t arg_count() const { return arguments.size(); }

    /** Check if has arguments */
    bool has_args() const { return !arguments.empty(); }

    /** Get argument by index (safe) */
    std::string_view arg(size_t index) const {
        return index < arguments.size() ? arguments[index] : std::string_view{};
    }

    /** Get argument as string with default */
    std::string arg_or(size_t index, std::string_view default_value = "") const {
        return index < arguments.size() ? arguments[index] : std::string{default_value};
    }

    /** Get all arguments from index onwards */
    std::string args_from(size_t start_index) const;

    /** Join arguments with separator */
    std::string join_args(std::string_view separator = " ", size_t start_index = 0) const;

    /** Check if command matches name (case insensitive) */
    bool is_command(std::string_view name) const;

    /** Validate command has minimum arguments */
    Result<void> require_args(size_t min_count, std::string_view usage = "") const;

  private:
    void rebuild_argument_string();
};

/** Command parsing options */
struct ParseOptions {
    bool allow_abbreviations = true; // Allow abbreviated commands
    bool case_sensitive = false;     // Case sensitive matching
    bool fuzzy_matching = true;      // Allow fuzzy/partial matches
    int min_abbreviation_length = 2; // Minimum abbreviation length
    int max_fuzzy_distance = 2;      // Maximum edit distance for fuzzy matching
    bool preserve_quotes = false;    // Keep quote characters in arguments
    bool expand_variables = false;   // Expand $variables in arguments

    // Special character handling
    char quote_char = '"';               // Quote character for multi-word args
    char escape_char = '\\';             // Escape character
    std::string comment_prefixes = "#;"; // Comment line prefixes
};

/** Command parsing result */
struct ParseResult {
    ParsedCommand command;
    std::vector<std::string> suggestions; // Alternative command suggestions
    std::string error_message;            // Error description if parsing failed
    bool success = false;

    ParseResult() = default;
    ParseResult(ParsedCommand cmd) : command(std::move(cmd)), success(true) {}
    ParseResult(std::string_view error) : error_message(error), success(false) {}

    operator bool() const { return success; }
};

/** Command tokenization result */
struct TokenizeResult {
    std::vector<std::string> tokens;
    std::vector<bool> quoted_flags; // Which tokens were quoted
    bool has_unterminated_quote = false;
    std::string error_message;

    bool success() const { return error_message.empty(); }
    operator bool() const { return success(); }
};

/** Command parser class */
class CommandParser {
  public:
    /** Create parser with options */
    explicit CommandParser(ParseOptions options = {});

    /** Parse input string into command */
    ParseResult parse(std::string_view input) const;

    /** Parse with command list for validation */
    ParseResult parse(std::string_view input, std::span<const std::string> available_commands) const;

    /** Tokenize input into arguments */
    TokenizeResult tokenize(std::string_view input) const;

    /** Find best command match from available list */
    std::optional<std::string> find_command_match(std::string_view partial,
                                                  std::span<const std::string> commands) const;

    /** Get command suggestions for partial input */
    std::vector<std::string> get_suggestions(std::string_view partial, std::span<const std::string> commands,
                                             size_t max_suggestions = 5) const;

    /** Check if command is abbreviation of full name */
    bool is_abbreviation(std::string_view abbrev, std::string_view full_name) const;

    /** Calculate edit distance for fuzzy matching */
    int edit_distance(std::string_view a, std::string_view b) const;

    /** Validate command name format */
    static bool is_valid_command_name(std::string_view name);

    /** Escape string for safe parsing */
    static std::string escape_argument(std::string_view arg);

    /** Unescape parsed argument */
    static std::string unescape_argument(std::string_view arg);

    // Configuration
    const ParseOptions &options() const { return options_; }
    void set_options(ParseOptions options) { options_ = std::move(options); }

  private:
    ParseOptions options_;

    // Helper methods
    std::string normalize_command(std::string_view cmd) const;
    int calculate_match_score(std::string_view partial, std::string_view full) const;
    bool matches_with_options(std::string_view partial, std::string_view full) const;
    std::vector<std::string> split_with_quotes(std::string_view input) const;
    bool is_comment_line(std::string_view input) const;
};

/** Utility functions for command parsing */
namespace CommandParserUtils {
/** Trim whitespace from string */
std::string_view trim(std::string_view str);

/** Convert to lowercase */
std::string to_lower(std::string_view str);

/** Check if string starts with prefix (case insensitive) */
bool starts_with_ci(std::string_view str, std::string_view prefix);

/** Split string by delimiter */
std::vector<std::string> split(std::string_view str, char delimiter);

/** Split string by whitespace */
std::vector<std::string> split_whitespace(std::string_view str);

/** Join strings with separator */
std::string join(std::span<const std::string> strings, std::string_view separator);

/** Format command usage string */
std::string format_usage(std::string_view command, std::string_view usage);

/** Extract quoted string (handles escaping) */
std::pair<std::string, size_t> extract_quoted_string(std::string_view input, size_t start_pos, char quote_char);

/** Check if character needs escaping */
bool needs_escape(char c);

/** Expand common abbreviations */
std::string expand_abbreviations(std::string_view input);

/** Validate argument as number */
std::optional<int> parse_int(std::string_view arg);

/** Validate argument as EntityId */
std::optional<EntityId> parse_entity_id(std::string_view arg);
} // namespace CommandParserUtils

/** Command history management */
class CommandHistory {
  public:
    explicit CommandHistory(size_t max_size = 100);

    /** Add command to history */
    void add(std::string_view command);

    /** Get command by index (0 = most recent) */
    std::optional<std::string> get(size_t index) const;

    /** Get last command */
    std::optional<std::string> last() const { return get(0); }

    /** Search history for pattern */
    std::vector<std::string> search(std::string_view pattern) const;

    /** Get all history */
    std::span<const std::string> all() const { return history_; }

    /** Clear history */
    void clear() { history_.clear(); }

    /** Get history size */
    size_t size() const { return history_.size(); }

    /** Check if empty */
    bool empty() const { return history_.empty(); }

  private:
    std::vector<std::string> history_;
    size_t max_size_;
};

/** Command completion engine */
class CommandCompletion {
  public:
    /** Add commands to completion database */
    void add_commands(std::span<const std::string> commands);

    /** Add command with aliases */
    void add_command(std::string_view command, std::span<const std::string> aliases = {});

    /** Get completions for partial input */
    std::vector<std::string> complete(std::string_view partial) const;

    /** Get completions with context (current room, inventory, etc.) */
    std::vector<std::string> complete_with_context(std::string_view partial, std::shared_ptr<Actor> actor,
                                                   std::shared_ptr<Room> room) const;

    /** Clear completion database */
    void clear() {
        commands_.clear();
        aliases_.clear();
    }

  private:
    std::vector<std::string> commands_;
    std::unordered_map<std::string, std::string> aliases_; // alias -> command

    std::vector<std::string> get_context_objects(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room) const;
};

/** Command suggestion engine */
class CommandSuggestionEngine {
  public:
    /** Set available commands */
    void set_commands(std::span<const std::string> commands);

    /** Get suggestions for invalid command */
    std::vector<std::string> suggest(std::string_view invalid_command, size_t max_suggestions = 5) const;

    /** Add usage statistics for better suggestions */
    void record_usage(std::string_view command);

    /** Get most used commands */
    std::vector<std::string> get_popular_commands(size_t count = 10) const;

  private:
    std::vector<std::string> commands_;
    std::unordered_map<std::string, int> usage_counts_;

    int calculate_suggestion_score(std::string_view invalid, std::string_view candidate) const;
};

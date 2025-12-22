#include "command_parser.hpp"

#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../text/string_utils.hpp"
#include "../world/room.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

// ParsedCommand Implementation

std::string ParsedCommand::args_from(size_t start_index) const {
    if (start_index >= arguments.size()) {
        return "";
    }

    std::ostringstream oss;
    for (size_t i = start_index; i < arguments.size(); ++i) {
        if (i > start_index) {
            oss << " ";
        }
        oss << arguments[i];
    }
    return oss.str();
}

std::string ParsedCommand::join_args(std::string_view separator, size_t start_index) const {
    if (start_index >= arguments.size()) {
        return "";
    }

    std::ostringstream oss;
    for (size_t i = start_index; i < arguments.size(); ++i) {
        if (i > start_index) {
            oss << separator;
        }
        oss << arguments[i];
    }
    return oss.str();
}

bool ParsedCommand::is_command(std::string_view name) const {
    return CommandParserUtils::starts_with_ci(command, name) && (command.length() == name.length() || is_abbreviated);
}

Result<void> ParsedCommand::require_args(size_t min_count, std::string_view usage) const {
    if (arguments.size() < min_count) {
        std::string error =
            fmt::format("Command '{}' requires at least {} argument{}", command, min_count, min_count == 1 ? "" : "s");
        if (!usage.empty()) {
            error += fmt::format("\nUsage: {}", CommandParserUtils::format_usage(command, usage));
        }
        return std::unexpected(Errors::InvalidArgument("arguments", error));
    }
    return Success();
}

void ParsedCommand::rebuild_argument_string() { full_argument_string = join_args(" "); }

// CommandParser Implementation

CommandParser::CommandParser(ParseOptions options) : options_(std::move(options)) {}

ParseResult CommandParser::parse(std::string_view input) const {
    if (is_comment_line(input)) {
        return ParseResult("Comment line");
    }

    auto tokens = tokenize(input);
    if (!tokens) {
        return ParseResult(tokens.error_message);
    }

    if (tokens.tokens.empty()) {
        return ParseResult("Empty command");
    }

    std::string command = normalize_command(tokens.tokens[0]);
    std::vector<std::string> arguments;

    // Copy arguments (skip first token which is the command)
    if (tokens.tokens.size() > 1) {
        arguments.assign(tokens.tokens.begin() + 1, tokens.tokens.end());
    }

    ParsedCommand cmd(command, std::move(arguments));
    cmd.original_input = input;
    cmd.has_quoted_args =
        std::any_of(tokens.quoted_flags.begin() + 1, tokens.quoted_flags.end(), [](bool quoted) { return quoted; });

    return ParseResult(std::move(cmd));
}

ParseResult CommandParser::parse(std::string_view input, std::span<const std::string> available_commands) const {
    auto result = parse(input);
    if (!result.success) {
        return result;
    }

    // Try to find exact or abbreviated match
    auto match = find_command_match(result.command.command, available_commands);
    if (match) {
        result.command.command = *match;
        result.command.is_abbreviated = (result.command.command != match->data());
        return result;
    }

    // Get suggestions for invalid command
    result.suggestions = get_suggestions(result.command.command, available_commands);
    if (!result.suggestions.empty()) {
        result.error_message = fmt::format("Unknown command '{}'. Did you mean: {}?", result.command.command,
                                           CommandParserUtils::join(result.suggestions, ", "));
    } else {
        result.error_message = fmt::format("Unknown command '{}'", result.command.command);
    }

    result.success = false;
    return result;
}

TokenizeResult CommandParser::tokenize(std::string_view input) const {
    TokenizeResult result;

    std::string current_token;
    bool in_quotes = false;
    bool escaped = false;

    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];

        if (escaped) {
            current_token += c;
            escaped = false;
            continue;
        }

        if (c == options_.escape_char) {
            if (options_.preserve_quotes) {
                current_token += c;
            }
            escaped = true;
            continue;
        }

        if (c == options_.quote_char) {
            if (in_quotes) {
                // End quote
                if (!current_token.empty()) {
                    result.tokens.push_back(current_token);
                    result.quoted_flags.push_back(true);
                    current_token.clear();
                }
                in_quotes = false;
            } else {
                // Start quote
                if (!current_token.empty()) {
                    result.tokens.push_back(current_token);
                    result.quoted_flags.push_back(false);
                    current_token.clear();
                }
                in_quotes = true;
            }
            continue;
        }

        if (!in_quotes && std::isspace(c)) {
            if (!current_token.empty()) {
                result.tokens.push_back(current_token);
                result.quoted_flags.push_back(false);
                current_token.clear();
            }
            continue;
        }

        current_token += c;
    }

    // Handle remaining token
    if (!current_token.empty()) {
        result.tokens.push_back(current_token);
        result.quoted_flags.push_back(in_quotes);
    }

    if (in_quotes) {
        result.has_unterminated_quote = true;
        result.error_message = "Unterminated quote in command";
    }

    return result;
}

std::optional<std::string> CommandParser::find_command_match(std::string_view partial,
                                                             std::span<const std::string> commands) const {
    std::string normalized_partial = normalize_command(partial);

    // First try exact match
    for (const auto &cmd : commands) {
        if (normalize_command(cmd) == normalized_partial) {
            return std::string{cmd};
        }
    }

    // Try abbreviation match
    if (options_.allow_abbreviations &&
        normalized_partial.length() >= static_cast<size_t>(options_.min_abbreviation_length)) {
        for (const auto &cmd : commands) {
            if (is_abbreviation(normalized_partial, cmd)) {
                return std::string{cmd};
            }
        }
    }

    return std::nullopt;
}

std::vector<std::string> CommandParser::get_suggestions(std::string_view partial, std::span<const std::string> commands,
                                                        size_t max_suggestions) const {
    std::vector<std::pair<std::string, int>> scored_commands;

    for (const auto &cmd : commands) {
        int score = calculate_match_score(partial, cmd);
        if (score > 0) {
            scored_commands.emplace_back(cmd, score);
        }
    }

    // Sort by score (highest first)
    std::sort(scored_commands.begin(), scored_commands.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    std::vector<std::string> suggestions;
    suggestions.reserve(std::min(max_suggestions, scored_commands.size()));

    for (size_t i = 0; i < std::min(max_suggestions, scored_commands.size()); ++i) {
        suggestions.push_back(scored_commands[i].first);
    }

    return suggestions;
}

bool CommandParser::is_abbreviation(std::string_view abbrev, std::string_view full_name) const {
    if (abbrev.length() >= full_name.length()) {
        return false;
    }

    std::string normalized_abbrev = normalize_command(abbrev);
    std::string normalized_full = normalize_command(full_name);

    return normalized_full.starts_with(normalized_abbrev);
}

int CommandParser::edit_distance(std::string_view a, std::string_view b) const {
    if (a.empty())
        return static_cast<int>(b.length());
    if (b.empty())
        return static_cast<int>(a.length());

    std::vector<std::vector<int>> dp(a.length() + 1, std::vector<int>(b.length() + 1));

    for (size_t i = 0; i <= a.length(); ++i) {
        dp[i][0] = static_cast<int>(i);
    }
    for (size_t j = 0; j <= b.length(); ++j) {
        dp[0][j] = static_cast<int>(j);
    }

    for (size_t i = 1; i <= a.length(); ++i) {
        for (size_t j = 1; j <= b.length(); ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,       // deletion
                dp[i][j - 1] + 1,       // insertion
                dp[i - 1][j - 1] + cost // substitution
            });
        }
    }

    return dp[a.length()][b.length()];
}

bool CommandParser::is_valid_command_name(std::string_view name) {
    if (name.empty() || name.length() > 30) {
        return false;
    }

    if (!std::isalpha(name[0])) {
        return false;
    }

    return std::all_of(name.begin(), name.end(), [](char c) { return std::isalnum(c) || c == '_' || c == '-'; });
}

std::string CommandParser::escape_argument(std::string_view arg) {
    std::string escaped;
    escaped.reserve(arg.length() * 2);

    for (char c : arg) {
        if (CommandParserUtils::needs_escape(c)) {
            escaped += '\\';
        }
        escaped += c;
    }

    return escaped;
}

std::string CommandParser::unescape_argument(std::string_view arg) {
    std::string unescaped;
    unescaped.reserve(arg.length());

    bool escaped = false;
    for (char c : arg) {
        if (escaped) {
            unescaped += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            unescaped += c;
        }
    }

    return unescaped;
}

std::string CommandParser::normalize_command(std::string_view cmd) const {
    return options_.case_sensitive ? std::string{cmd} : CommandParserUtils::to_lower(cmd);
}

int CommandParser::calculate_match_score(std::string_view partial, std::string_view full) const {
    std::string norm_partial = normalize_command(partial);
    std::string norm_full = normalize_command(full);

    // Exact match
    if (norm_partial == norm_full) {
        return 100;
    }

    // Prefix match
    if (norm_full.starts_with(norm_partial)) {
        return 90 - static_cast<int>(norm_full.length() - norm_partial.length());
    }

    // Fuzzy match
    if (options_.fuzzy_matching) {
        int distance = edit_distance(norm_partial, norm_full);
        if (distance <= options_.max_fuzzy_distance) {
            return 50 - (distance * 10);
        }
    }

    return 0;
}

bool CommandParser::matches_with_options(std::string_view partial, std::string_view full) const {
    return calculate_match_score(partial, full) > 0;
}

std::vector<std::string> CommandParser::split_with_quotes(std::string_view input) const {
    auto tokens = tokenize(input);
    return tokens.success() ? std::move(tokens.tokens) : std::vector<std::string>{};
}

bool CommandParser::is_comment_line(std::string_view input) const {
    auto trimmed = CommandParserUtils::trim(input);
    if (trimmed.empty()) {
        return false;
    }

    // Ensure comment_prefixes is not empty and trimmed is not empty before accessing [0]
    if (options_.comment_prefixes.empty() || trimmed.empty()) {
        return false;
    }
    return options_.comment_prefixes.find(trimmed.front()) != std::string::npos;
}

// CommandParserUtils Implementation

namespace CommandParserUtils {
std::string_view trim(std::string_view str) {
    // Delegate to the global trim function from string_utils.hpp
    return ::trim(str);
}

std::string to_lower(std::string_view str) {
    return to_lowercase(str);
}

bool starts_with_ci(std::string_view str, std::string_view prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }

    return std::equal(prefix.begin(), prefix.end(), str.begin(),
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

std::vector<std::string> split(std::string_view str, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t pos = 0;

    while ((pos = str.find(delimiter, start)) != std::string_view::npos) {
        if (pos > start) {
            tokens.emplace_back(str.substr(start, pos - start));
        }
        start = pos + 1;
    }

    if (start < str.length()) {
        tokens.emplace_back(str.substr(start));
    }

    return tokens;
}

std::vector<std::string> split_whitespace(std::string_view str) {
    std::vector<std::string> tokens;
    std::istringstream iss{std::string{str}};
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string join(std::span<const std::string> strings, std::string_view separator) {
    if (strings.empty()) {
        return "";
    }

    std::ostringstream oss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) {
            oss << separator;
        }
        oss << strings[i];
    }

    return oss.str();
}

std::string format_usage(std::string_view command, std::string_view usage) {
    return fmt::format("{} {}", command, usage);
}

std::pair<std::string, size_t> extract_quoted_string(std::string_view input, size_t start_pos, char quote_char) {
    std::string result;
    size_t pos = start_pos + 1; // Skip opening quote

    while (pos < input.length()) {
        char c = input[pos];

        if (c == quote_char) {
            return {result, pos + 1};
        }

        if (c == '\\' && pos + 1 < input.length()) {
            result += input[pos + 1];
            pos += 2;
        } else {
            result += c;
            pos++;
        }
    }

    // Unterminated quote
    return {result, input.length()};
}

bool needs_escape(char c) { return c == '"' || c == '\'' || c == '\\' || c == ' ' || c == '\t' || c == '\n'; }

std::string expand_abbreviations(std::string_view input) {
    // Common abbreviations
    static const std::unordered_map<std::string, std::string> abbreviations = {
        {"l", "look"},  {"i", "inventory"}, {"eq", "equipment"}, {"who", "who"}, {"n", "north"},
        {"s", "south"}, {"e", "east"},      {"w", "west"},       {"u", "up"},    {"d", "down"}};

    std::string lower_input = to_lower(input);
    auto it = abbreviations.find(lower_input);
    return it != abbreviations.end() ? it->second : std::string{input};
}

std::optional<int> parse_int(std::string_view arg) {
    try {
        return std::stoi(std::string{arg});
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<EntityId> parse_entity_id(std::string_view arg) {
    auto num = parse_int(arg);
    return num ? std::make_optional(EntityId{static_cast<std::uint64_t>(*num)}) : std::nullopt;
}
} // namespace CommandParserUtils

// CommandHistory Implementation

CommandHistory::CommandHistory(size_t max_size) : max_size_(max_size) {}

void CommandHistory::add(std::string_view command) {
    if (command.empty()) {
        return;
    }

    std::string cmd{command};

    // Remove duplicate if it's the last command
    if (!history_.empty() && history_.front() == cmd) {
        return;
    }

    history_.insert(history_.begin(), cmd);

    if (history_.size() > max_size_) {
        history_.resize(max_size_);
    }
}

std::optional<std::string> CommandHistory::get(size_t index) const {
    return index < history_.size() ? std::make_optional(history_[index]) : std::nullopt;
}

std::vector<std::string> CommandHistory::search(std::string_view pattern) const {
    std::vector<std::string> matches;
    std::string lower_pattern = CommandParserUtils::to_lower(pattern);

    for (const auto &cmd : history_) {
        if (CommandParserUtils::to_lower(cmd).find(lower_pattern) != std::string::npos) {
            matches.push_back(cmd);
        }
    }

    return matches;
}

// CommandCompletion Implementation

void CommandCompletion::add_commands(std::span<const std::string> commands) {
    for (const auto &cmd : commands) {
        commands_.push_back(cmd);
    }
    std::sort(commands_.begin(), commands_.end());
}

void CommandCompletion::add_command(std::string_view command, std::span<const std::string> aliases) {
    commands_.emplace_back(command);

    for (const auto &alias : aliases) {
        aliases_[alias] = command;
    }
}

std::vector<std::string> CommandCompletion::complete(std::string_view partial) const {
    std::vector<std::string> completions;
    std::string lower_partial = CommandParserUtils::to_lower(partial);

    for (const auto &cmd : commands_) {
        if (CommandParserUtils::starts_with_ci(cmd, partial)) {
            completions.push_back(cmd);
        }
    }

    for (const auto &[alias, command] : aliases_) {
        if (CommandParserUtils::starts_with_ci(alias, partial)) {
            completions.push_back(alias);
        }
    }

    std::sort(completions.begin(), completions.end());
    completions.erase(std::unique(completions.begin(), completions.end()), completions.end());

    return completions;
}

std::vector<std::string> CommandCompletion::complete_with_context(std::string_view partial,
                                                                  std::shared_ptr<Actor> actor,
                                                                  std::shared_ptr<Room> room) const {
    auto basic_completions = complete(partial);
    auto context_objects = get_context_objects(actor, room);

    // Add context objects that match the partial
    for (const auto &obj : context_objects) {
        if (CommandParserUtils::starts_with_ci(obj, partial)) {
            basic_completions.push_back(obj);
        }
    }

    std::sort(basic_completions.begin(), basic_completions.end());
    basic_completions.erase(std::unique(basic_completions.begin(), basic_completions.end()), basic_completions.end());

    return basic_completions;
}

std::vector<std::string> CommandCompletion::get_context_objects(std::shared_ptr<Actor> actor,
                                                                std::shared_ptr<Room> room) const {
    std::vector<std::string> objects;

    // Add room contents
    if (room) {
        // Add visible actors
        for (const auto &other_actor : room->contents().actors) {
            if (other_actor && other_actor != actor) {
                objects.emplace_back(other_actor->name());
                for (const auto &keyword : other_actor->keywords()) {
                    objects.emplace_back(keyword);
                }
            }
        }

        // Add visible objects
        for (const auto &obj : room->contents().objects) {
            if (obj) {
                objects.emplace_back(obj->name());
                for (const auto &keyword : obj->keywords()) {
                    objects.emplace_back(keyword);
                }
            }
        }

        // Add exits
        for (auto dir : room->get_visible_exits()) {
            objects.push_back(std::string{RoomUtils::get_direction_name(dir)});
            objects.push_back(std::string{RoomUtils::get_direction_abbrev(dir)});
        }
    }

    // Add inventory items
    if (actor) {
        for (const auto &item : actor->inventory().get_all_items()) {
            if (item) {
                objects.emplace_back(item->name());
                for (const auto &keyword : item->keywords()) {
                    objects.emplace_back(keyword);
                }
            }
        }
    }

    return objects;
}

// CommandSuggestionEngine Implementation

void CommandSuggestionEngine::set_commands(std::span<const std::string> commands) {
    commands_.assign(commands.begin(), commands.end());
}

std::vector<std::string> CommandSuggestionEngine::suggest(std::string_view invalid_command,
                                                          size_t max_suggestions) const {
    std::vector<std::pair<std::string, int>> scored_commands;

    for (const auto &cmd : commands_) {
        int score = calculate_suggestion_score(invalid_command, cmd);
        if (score > 0) {
            scored_commands.emplace_back(cmd, score);
        }
    }

    // Sort by score (highest first), then by usage count
    std::sort(scored_commands.begin(), scored_commands.end(), [this](const auto &a, const auto &b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }

        auto usage_a = usage_counts_.find(a.first);
        auto usage_b = usage_counts_.find(b.first);
        int count_a = usage_a != usage_counts_.end() ? usage_a->second : 0;
        int count_b = usage_b != usage_counts_.end() ? usage_b->second : 0;
        return count_a > count_b;
    });

    std::vector<std::string> suggestions;
    suggestions.reserve(std::min(max_suggestions, scored_commands.size()));

    for (size_t i = 0; i < std::min(max_suggestions, scored_commands.size()); ++i) {
        suggestions.push_back(scored_commands[i].first);
    }

    return suggestions;
}

void CommandSuggestionEngine::record_usage(std::string_view command) { usage_counts_[std::string{command}]++; }

std::vector<std::string> CommandSuggestionEngine::get_popular_commands(size_t count) const {
    std::vector<std::pair<std::string, int>> command_usage;

    for (const auto &[cmd, usage] : usage_counts_) {
        command_usage.emplace_back(cmd, usage);
    }

    std::sort(command_usage.begin(), command_usage.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    std::vector<std::string> popular;
    popular.reserve(std::min(count, command_usage.size()));

    for (size_t i = 0; i < std::min(count, command_usage.size()); ++i) {
        popular.push_back(command_usage[i].first);
    }

    return popular;
}

int CommandSuggestionEngine::calculate_suggestion_score(std::string_view invalid, std::string_view candidate) const {
    std::string lower_invalid = CommandParserUtils::to_lower(invalid);
    std::string lower_candidate = CommandParserUtils::to_lower(candidate);

    // Exact match (shouldn't happen but handle it)
    if (lower_invalid == lower_candidate) {
        return 100;
    }

    // Prefix match
    if (lower_candidate.starts_with(lower_invalid)) {
        return 90;
    }

    // Contains match
    if (lower_candidate.find(lower_invalid) != std::string::npos) {
        return 70;
    }

    // Edit distance
    CommandParser parser;
    int distance = parser.edit_distance(lower_invalid, lower_candidate);
    int max_length = std::max(lower_invalid.length(), lower_candidate.length());

    if (distance <= max_length / 3) {
        return 50 - (distance * 5);
    }

    return 0;
}
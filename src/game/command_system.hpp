/***************************************************************************
 *   File: src/game/command_system.hpp          Part of FieryMUD *
 *  Usage: Modern command processing system                                *
 ***************************************************************************/

#pragma once

#include "../core/ids.hpp"
#include "../core/result.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Forward declarations
class Player;
class GameWorld;

/**
 * @brief Command execution result
 */
enum class CommandResult {
    Success,      // Command executed successfully
    NotFound,     // Command not recognized
    InvalidArgs,  // Invalid arguments provided
    AccessDenied, // Player lacks permission
    PlayerError,  // Player in invalid state
    SystemError   // Internal system error
};

/**
 * @brief Command context for execution
 */
struct CommandContext {
    std::shared_ptr<Player> player;     // Player executing command
    std::shared_ptr<GameWorld> world; // World server reference
    std::string full_input;             // Complete input line
    std::string command;                // Parsed command name
    std::vector<std::string> args;      // Parsed arguments

    // Helper methods
    std::string args_as_string() const;
    bool has_args() const { return !args.empty(); }
    size_t arg_count() const { return args.size(); }
};

/**
 * @brief Function signature for command handlers
 */
using CommandHandler = std::function<CommandResult(const CommandContext &ctx)>;

/**
 * @brief Command definition with metadata
 */
struct CommandDef {
    std::string name;                 // Primary command name
    std::vector<std::string> aliases; // Alternative names
    CommandHandler handler;           // Execution function
    std::string description;          // Help text
    std::string usage;                // Usage syntax
    int min_level = 1;                // Minimum level required
    bool hidden = false;              // Hidden from help/command lists

    // Helper to check if a name matches this command
    bool matches(std::string_view input) const;
};

/**
 * @brief Modern command processing system
 *
 * Features:
 * - Fast command lookup with fuzzy matching
 * - Extensible command registration
 * - Built-in help system
 * - Permission checking
 * - Argument parsing and validation
 */
class CommandSystem {
  public:
    explicit CommandSystem(std::shared_ptr<GameWorld> world);
    ~CommandSystem() = default;

    // Command registration
    void register_command(CommandDef command);
    void register_builtin_commands();

    // Command execution
    CommandResult process_input(std::shared_ptr<Player> player, std::string_view input);

    // Command lookup
    const CommandDef *find_command(std::string_view name) const;
    std::vector<const CommandDef *> find_matching_commands(std::string_view partial) const;

    // Help system
    std::vector<std::string> get_help_text(std::string_view command = "") const;
    std::vector<std::string> list_available_commands(std::shared_ptr<Player> player) const;

  private:
    // Input parsing
    CommandContext parse_input(std::shared_ptr<Player> player, std::string_view input) const;
    std::vector<std::string> tokenize_args(std::string_view args) const;

    // Permission checking
    bool can_use_command(std::shared_ptr<Player> player, const CommandDef &cmd) const;

    // Core data
    std::shared_ptr<GameWorld> world_;
    std::vector<CommandDef> commands_;
    std::unordered_map<std::string, size_t> command_index_; // Fast lookup by name
};
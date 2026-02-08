#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "command_context.hpp"
#include "command_fwd.hpp"
#include "command_parser.hpp"
#include "core/ids.hpp"
#include "core/result.hpp"

// Forward declarations
class Actor;
class Room;
class Object;
struct CommandContext;
struct TargetInfo;
class RichText;
enum class Color : int;

/**
 * Modern command system for FieryMUD.
 *
 * Features:
 * - Hierarchical privilege levels with fine-grained permissions
 * - Dynamic command registration and unregistration
 * - Command aliasing and abbreviation support
 * - Cooldown and rate limiting
 * - Command logging and auditing
 * - Context-sensitive command availability
 * - Plugin-style command modules
 */

/** Privilege levels for command access */
enum class PrivilegeLevel {
    // Player levels
    Guest = 0,      // Unverified players
    Player = 1,     // Verified players
    Veteran = 90,   // Experienced players
    MaxMortal = 99, // Maximum Mortal

    // Helper levels
    Helper = 100, // Player helpers - Immortals

    // God levels
    DemiGod = 101, // Lesser God
    God = 102,     // Full God
    Builder = 103, // World builders
    Coder = 104,   // Code developers
    Overlord = 105 // Overlord level (highest)
};

/** Command execution result */
enum class CommandResult {
    Success,           // Command executed successfully
    InvalidSyntax,     // Invalid command syntax
    InsufficientPrivs, // Insufficient privileges
    NotFound,          // Command not found
    Cooldown,          // Command on cooldown
    Busy,              // Actor is busy (e.g., already casting)
    InvalidTarget,     // Invalid target specified
    InvalidState,      // Invalid state for command
    ResourceError,     // Insufficient resources
    SystemError,       // Internal system error
    Cancelled          // Command was cancelled
};

/** Command execution statistics */
struct CommandStats {
    std::string command_name;
    int total_executions = 0;
    int successful_executions = 0;
    int failed_executions = 0;
    std::chrono::steady_clock::time_point last_executed{};
    std::chrono::milliseconds total_execution_time{0};
    std::chrono::milliseconds average_execution_time{0};

    void record_execution(bool success, std::chrono::milliseconds duration);
    double success_rate() const;
};

/** Command cooldown information */
struct CommandCooldown {
    std::chrono::seconds duration;
    std::chrono::steady_clock::time_point last_used;

    bool is_on_cooldown() const;
    std::chrono::seconds remaining() const;
};

/** Command handler function signature */
using CommandHandler = std::function<Result<CommandResult>(const CommandContext &context)>;

/** Command information and metadata */
struct CommandInfo {
    std::string name;                 // Primary command name
    std::vector<std::string> aliases; // Alternative names
    std::string category;             // Command category (combat, social, etc.)
    std::string description;          // Brief description
    std::string usage;                // Usage syntax
    std::string help_text;            // Extended help text

    PrivilegeLevel required_privilege = PrivilegeLevel::Player;
    std::unordered_set<std::string> required_permissions; // Additional permissions

    bool requires_target = false;        // Does command need a target?
    bool requires_room = true;           // Can be used without being in a room?
    bool can_use_while_fighting = true;  // Can be used during combat?
    bool can_use_while_sitting = true;   // Can be used while sitting?
    bool can_use_while_sleeping = false; // Can be used while sleeping?

    std::chrono::seconds cooldown{0}; // Command cooldown
    int minimum_level = 1;            // Minimum character level
    int maximum_level = 999;          // Maximum character level (includes immortals 100+)

    CommandHandler handler; // Command execution function

    // Constructor
    CommandInfo(std::string_view cmd_name, CommandHandler cmd_handler)
        : name(cmd_name), handler(std::move(cmd_handler)) {}
};

/** Command execution context */
struct CommandContext {
    std::shared_ptr<Actor> actor; // Command executor
    std::shared_ptr<Room> room;   // Current room
    ParsedCommand command;        // Parsed command
    std::string raw_input;        // Original input

    // Execution metadata
    std::chrono::steady_clock::time_point start_time;
    PrivilegeLevel actor_privilege;
    std::unordered_set<std::string> actor_permissions;

    // Helper methods
    std::string_view arg(size_t index) const { return command.arg(index); }
    std::string arg_or(size_t index, std::string_view default_val = "") const {
        return command.arg_or(index, default_val);
    }
    size_t arg_count() const { return command.arg_count(); }
    std::string args_from(size_t index) const { return command.args_from(index); }

    Result<void> require_args(size_t min_count, std::string_view usage = "") const {
        return command.require_args(min_count, usage);
    }

    // Send output to actor
    void send(std::string_view message) const;
    void send_line(std::string_view message) const;
    void send_error(std::string_view message) const;
    void send_success(std::string_view message) const;
    void send_info(std::string_view message) const;
    void send_usage(std::string_view usage) const;

    /**
     * Display the registered help for the current command.
     * Looks up the command in the CommandSystem and shows description, usage, help_text.
     * Use this instead of manually writing help text in command functions.
     */
    void show_help() const;

    // Extended messaging
    void send_to_room(std::string_view message, bool exclude_self = true,
                      std::span<const std::shared_ptr<Actor>> also_exclude = {}) const;
    void send_to_actor(std::shared_ptr<Actor> target, std::string_view message) const;
    void send_to_all(std::string_view message, bool exclude_self = false) const;

    // Target finding helpers
    std::shared_ptr<Actor> find_actor_target(std::string_view name) const;
    std::shared_ptr<Actor> find_actor_global(std::string_view name) const;
    std::shared_ptr<Object> find_object_target(std::string_view name) const;
    std::shared_ptr<Room> find_room_target(std::string_view name) const;

    /**
     * Find multiple objects matching a name with dot-notation support.
     * Supports: "all.keyword", "N.keyword" (Nth item), or "keyword" (first match)
     * @param name The name/keyword to search for (may include dot-notation prefix)
     * @param search_room Whether to also search the current room
     * @return Vector of matching objects (may be empty, one item, or multiple)
     */
    std::vector<std::shared_ptr<Object>> find_objects_matching(std::string_view name, bool search_room = false) const;

    // Target resolution
    TargetInfo resolve_target(std::string_view name) const;

    // Entity ID parsing with current zone as default
    // Accepts "zone:id" or just "id" (uses current room's zone)
    EntityId parse_entity_id(std::string_view str) const;

    // Utility methods
    std::string format_object_name(std::shared_ptr<Object> obj) const;
    Result<void> move_actor_direction(Direction dir) const;

    // Enhanced act-style messaging methods (inspired by legacy system)
    void act_to_char(std::string_view message) const;
    void act_to_room(std::string_view message, bool exclude_actor = true) const;
    void act_to_target(std::shared_ptr<Actor> target, std::string_view message) const;
    void act(std::string_view message, std::shared_ptr<Actor> target = nullptr,
             ActTarget type = ActTarget::ToAll) const;

    // Variable substitution support
    std::string substitute_variables(std::string_view message, std::shared_ptr<Actor> target = nullptr) const;

    // Social message execution helper
    Result<CommandResult> execute_social(const SocialMessage &social, std::string_view target_name = "") const;

    // Rich text formatting methods (requires #include "rich_text.hpp")
    void send_rich(const RichText &rich_text) const;
    void send_colored(std::string_view message, Color color) const;
    void send_progress_bar(std::string_view label, float percentage, int width = 20) const;
    void send_table(const std::vector<std::string> &headers, const std::vector<std::vector<std::string>> &rows) const;

    // Semantic formatting helpers
    void send_health_status(int current, int max) const;
    void send_damage_report(int amount, std::string_view source = "") const;
    void send_healing_report(int amount, std::string_view source = "") const;
    void send_object_description(std::string_view name, std::string_view quality = "common") const;

    // UI element helpers
    void send_header(std::string_view title, char border_char = '=') const;
    void send_separator(char ch = '-', int width = 60) const;
    void send_command_help(std::string_view command, std::string_view syntax) const;
};

/** Command category for organization */
struct CommandCategory {
    std::string name;
    std::string description;
    std::vector<std::string> commands;
    PrivilegeLevel min_privilege = PrivilegeLevel::Player;

    CommandCategory() = default;
    CommandCategory(std::string_view cat_name, std::string_view desc = "") : name(cat_name), description(desc) {}
};

/** Main command system class */
class CommandSystem {
  public:
    /** Get singleton instance */
    static CommandSystem &instance();

    /** Initialize command system */
    Result<void> initialize();

    /** Shutdown command system */
    void shutdown();

    // Command Registration
    Result<void> register_command(CommandInfo command_info);
    Result<void> register_command(std::string_view name, CommandHandler handler);
    void unregister_command(std::string_view name);

    /** Register command with fluent builder pattern */
    class CommandBuilder {
      public:
        CommandBuilder(CommandSystem &system, std::string_view name, CommandHandler handler);

        CommandBuilder &alias(std::string_view alias_name);
        CommandBuilder &aliases(std::span<const std::string> alias_names);
        CommandBuilder &category(std::string_view cat);
        CommandBuilder &description(std::string_view desc);
        CommandBuilder &usage(std::string_view usage_text);
        CommandBuilder &help(std::string_view help_text);
        CommandBuilder &privilege(PrivilegeLevel level);
        CommandBuilder &permission(std::string_view perm);
        CommandBuilder &permissions(std::span<const std::string> perms);
        CommandBuilder &requires_target(bool required = true);
        CommandBuilder &requires_room(bool required = true);
        CommandBuilder &usable_in_combat(bool allowed = true);
        CommandBuilder &usable_while_sitting(bool allowed = true);
        CommandBuilder &usable_while_sleeping(bool allowed = false);
        CommandBuilder &cooldown(std::chrono::seconds duration);
        CommandBuilder &level_range(int min_level, int max_level = 999);

        Result<void> build();

      private:
        CommandSystem &system_;
        CommandInfo info_;
    };

    CommandBuilder command(std::string_view name, CommandHandler handler);

    // Command Execution
    Result<CommandResult> execute_command(std::shared_ptr<Actor> actor, std::string_view input);
    Result<CommandResult> execute_parsed_command(std::shared_ptr<Actor> actor, const ParsedCommand &command);

    // Command Lookup
    const CommandInfo *find_command(std::string_view name) const;
    std::vector<std::string> get_available_commands(std::shared_ptr<Actor> actor) const;
    std::vector<std::string> get_command_suggestions(std::string_view partial, std::shared_ptr<Actor> actor) const;

    // Privilege and Permission System
    bool has_privilege(std::shared_ptr<Actor> actor, PrivilegeLevel required) const;
    bool has_permission(std::shared_ptr<Actor> actor, std::string_view permission) const;
    bool can_execute_command(std::shared_ptr<Actor> actor, const CommandInfo &command) const;

    PrivilegeLevel get_actor_privilege(std::shared_ptr<Actor> actor) const;
    std::unordered_set<std::string> get_actor_permissions(std::shared_ptr<Actor> actor) const;

    // Category Management
    void register_category(CommandCategory category);
    std::vector<CommandCategory> get_categories() const;
    std::vector<std::string> get_commands_in_category(std::string_view category) const;

    // Statistics and Monitoring
    const CommandStats *get_command_stats(std::string_view command) const;
    std::vector<CommandStats> get_all_stats() const;
    void reset_stats();

    // Cooldown Management
    bool is_on_cooldown(std::shared_ptr<Actor> actor, std::string_view command) const;
    std::chrono::seconds get_cooldown_remaining(std::shared_ptr<Actor> actor, std::string_view command) const;
    void clear_cooldown(std::shared_ptr<Actor> actor, std::string_view command);
    void clear_all_cooldowns(std::shared_ptr<Actor> actor);

    // Command History and Logging
    void log_command_execution(const CommandContext &context, CommandResult result, std::chrono::milliseconds duration);
    std::vector<std::string> get_command_history(std::shared_ptr<Actor> actor, size_t count = 10) const;

    // Help System
    std::string get_command_help(std::string_view command) const;
    std::string get_category_help(std::string_view category) const;
    std::vector<std::string> search_help(std::string_view query) const;

    // Configuration
    void set_global_cooldown(std::chrono::milliseconds duration) { global_cooldown_ = duration; }
    void set_max_command_history(size_t max_size) { max_command_history_ = max_size; }
    void enable_command_logging(bool enabled) { command_logging_enabled_ = enabled; }

  private:
    CommandSystem() = default;
    ~CommandSystem() = default;
    CommandSystem(const CommandSystem &) = delete;
    CommandSystem &operator=(const CommandSystem &) = delete;

    // Core data structures
    std::unordered_map<std::string, std::unique_ptr<CommandInfo>> commands_;
    std::unordered_map<std::string, std::string> aliases_; // alias -> command name
    std::unordered_map<std::string, CommandCategory> categories_;

    // Statistics and monitoring
    std::unordered_map<std::string, CommandStats> command_stats_;
    std::unordered_map<EntityId, std::unordered_map<std::string, CommandCooldown>> cooldowns_;
    std::unordered_map<EntityId, std::vector<std::string>> command_history_;

    // Configuration
    std::chrono::milliseconds global_cooldown_{100};
    size_t max_command_history_ = 100;
    bool command_logging_enabled_ = true;
    bool initialized_ = false;

    mutable std::recursive_mutex system_mutex_;

    // Parser for command input
    CommandParser parser_;

    // Helper methods
    Result<CommandContext> create_context(std::shared_ptr<Actor> actor, const ParsedCommand &command) const;
    bool check_command_conditions(std::shared_ptr<Actor> actor, const CommandInfo &command) const;
    void record_command_usage(std::shared_ptr<Actor> actor, std::string_view command);
    void update_command_stats(std::string_view command, bool success, std::chrono::milliseconds duration);

    std::string normalize_command_name(std::string_view name) const;
    bool is_valid_permission_name(std::string_view permission) const;
};

/** Utility functions for command system */
namespace CommandSystemUtils {
/** Convert privilege level to string */
std::string_view privilege_to_string(PrivilegeLevel level);

/** Parse privilege level from string */
std::optional<PrivilegeLevel> parse_privilege_level(std::string_view str);

/** Convert command result to string */
std::string_view result_to_string(CommandResult result);

/** Format command execution time */
std::string format_execution_time(std::chrono::milliseconds duration);

/** Format privilege level with color */
std::string format_privilege_level(PrivilegeLevel level, bool use_color = true);

/** Check if privilege level can execute command */
bool privilege_allows(PrivilegeLevel actor_level, PrivilegeLevel required_level);

/** Get privilege level hierarchy */
std::vector<PrivilegeLevel> get_privilege_hierarchy();

/** Get default permissions for privilege level */
std::unordered_set<std::string> get_default_permissions(PrivilegeLevel level);

/** Validate command name format */
bool is_valid_command_name(std::string_view name);

/** Validate category name format */
bool is_valid_category_name(std::string_view name);

/** Get command complexity score (for rate limiting) */
int get_command_complexity(std::string_view command);

/** Format cooldown time remaining */
std::string format_cooldown_remaining(std::chrono::seconds remaining);
} // namespace CommandSystemUtils

/** Command registration macros for convenience */
#define REGISTER_COMMAND(name, handler) CommandSystem::instance().command(name, handler).build()

#define REGISTER_COMMAND_WITH_PRIVILEGE(name, handler, priv)                                                           \
    CommandSystem::instance().command(name, handler).privilege(priv).build()

#define REGISTER_BUILDER_COMMAND(name, handler)                                                                        \
    CommandSystem::instance().command(name, handler).privilege(PrivilegeLevel::Builder).build()

#define REGISTER_CODER_COMMAND(name, handler)                                                                          \
    CommandSystem::instance().command(name, handler).privilege(PrivilegeLevel::Coder).build()

#define REGISTER_GOD_COMMAND(name, handler)                                                                            \
    CommandSystem::instance().command(name, handler).privilege(PrivilegeLevel::God).build()

#define REGISTER_OVERLORD_COMMAND(name, handler)                                                                       \
    CommandSystem::instance().command(name, handler).privilege(PrivilegeLevel::Overlord).build()

/** Global command system access */
inline CommandSystem &Commands() { return CommandSystem::instance(); }

/** Permission constants */
namespace Permissions {
inline constexpr std::string_view BUILD = "build";
inline constexpr std::string_view CODE = "code";
inline constexpr std::string_view ADMIN = "admin";
inline constexpr std::string_view GOD = "god";
inline constexpr std::string_view SHUTDOWN = "shutdown";
inline constexpr std::string_view FORCE = "force";
inline constexpr std::string_view SNOOP = "snoop";
inline constexpr std::string_view TELEPORT = "teleport";
inline constexpr std::string_view INVISIBLE = "invisible";
inline constexpr std::string_view NOHASSLE = "nohassle";
inline constexpr std::string_view SUMMON = "summon";
inline constexpr std::string_view TRANSFER = "transfer";
inline constexpr std::string_view WIZLOCK = "wizlock";
inline constexpr std::string_view LOG = "log";
inline constexpr std::string_view SYSLOG = "syslog";
inline constexpr std::string_view ZONE_RESET = "zone_reset";
inline constexpr std::string_view ADVANCE = "advance";
inline constexpr std::string_view RESTORE = "restore";
inline constexpr std::string_view NOTITLE = "notitle";
inline constexpr std::string_view SQUELCH = "squelch";
inline constexpr std::string_view FREEZE = "freeze";
inline constexpr std::string_view THAW = "thaw";
inline constexpr std::string_view BAN = "ban";
inline constexpr std::string_view UNBAN = "unban";
inline constexpr std::string_view DC = "dc";
inline constexpr std::string_view WIZNET = "wiznet";
inline constexpr std::string_view OLC = "olc";
} // namespace Permissions

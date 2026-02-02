#include "command_system.hpp"

#include "core/actor.hpp"
#include "core/player.hpp"
#include "core/logging.hpp"
#include "core/ability_executor.hpp"
#include "database/world_queries.hpp"
#include "scripting/trigger_manager.hpp"
#include "text/string_utils.hpp"
#include "world/room.hpp"
#include "world/world_manager.hpp"
#include "command_parser.hpp"

#include <algorithm>
#include <set>

// CommandStats Implementation

void CommandStats::record_execution(bool success, std::chrono::milliseconds duration) {
    total_executions++;
    if (success) {
        successful_executions++;
    } else {
        failed_executions++;
    }

    last_executed = std::chrono::steady_clock::now();
    total_execution_time += duration;

    if (total_executions > 0) {
        average_execution_time = total_execution_time / total_executions;
    }
}

double CommandStats::success_rate() const {
    return total_executions > 0 ? static_cast<double>(successful_executions) / total_executions : 0.0;
}

// CommandCooldown Implementation

bool CommandCooldown::is_on_cooldown() const {
    auto now = std::chrono::steady_clock::now();
    return (now - last_used) < duration;
}

std::chrono::seconds CommandCooldown::remaining() const {
    if (!is_on_cooldown()) {
        return std::chrono::seconds{0};
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_used);
    return std::chrono::duration_cast<std::chrono::seconds>(duration) - elapsed;
}

// CommandSystem Implementation

CommandSystem &CommandSystem::instance() {
    static CommandSystem instance;
    return instance;
}

Result<void> CommandSystem::initialize() {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    if (initialized_) {
        return Success();
    }

    // Register default categories
    register_category(CommandCategory("Communication", "Commands for talking to other players"));
    register_category(CommandCategory("Movement", "Commands for moving around the world"));
    register_category(CommandCategory("Position", "Commands for changing position (sit, stand, rest)"));
    register_category(CommandCategory("Information", "Commands for getting information"));
    register_category(CommandCategory("Object", "Commands for manipulating objects"));
    register_category(CommandCategory("Combat", "Commands used in combat"));
    register_category(CommandCategory("Social", "Social interaction commands"));
    register_category(CommandCategory("Building", "World building commands"));
    register_category(CommandCategory("Administration", "Administrative commands"));
    register_category(CommandCategory("System", "System maintenance commands"));

    initialized_ = true;

    Log::info("Command system initialized");
    return Success();
}

void CommandSystem::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    commands_.clear();
    aliases_.clear();
    categories_.clear();
    command_stats_.clear();
    cooldowns_.clear();
    command_history_.clear();

    initialized_ = false;

    Log::info("Command system shutdown");
}

Result<void> CommandSystem::register_command(CommandInfo command_info) {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    if (!CommandSystemUtils::is_valid_command_name(command_info.name)) {
        return std::unexpected(Errors::InvalidArgument("command_name", "invalid format"));
    }

    if (!command_info.handler) {
        return std::unexpected(Errors::InvalidArgument("handler", "cannot be null"));
    }

    std::string normalized_name = normalize_command_name(command_info.name);

    // Check for conflicts
    if (commands_.contains(normalized_name)) {
        return std::unexpected(Errors::AlreadyExists(fmt::format("command '{}'", normalized_name)));
    }

    // Register aliases
    for (const auto &alias : command_info.aliases) {
        std::string normalized_alias = normalize_command_name(alias);
        if (aliases_.contains(normalized_alias) || commands_.contains(normalized_alias)) {
            return std::unexpected(Errors::AlreadyExists(fmt::format("alias '{}'", normalized_alias)));
        }
        aliases_[normalized_alias] = normalized_name;
    }

    // Store command
    commands_[normalized_name] = std::make_unique<CommandInfo>(std::move(command_info));
    commands_[normalized_name]->name = normalized_name;

    // Initialize stats
    command_stats_[normalized_name] = CommandStats{normalized_name};

    Log::debug("Registered command: {}", normalized_name);
    return Success();
}

Result<void> CommandSystem::register_command(std::string_view name, CommandHandler handler) {
    CommandInfo info{name, std::move(handler)};
    return register_command(std::move(info));
}

void CommandSystem::unregister_command(std::string_view name) {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    std::string normalized_name = normalize_command_name(name);

    // Remove command
    commands_.erase(normalized_name);
    command_stats_.erase(normalized_name);

    // Remove aliases pointing to this command
    for (auto it = aliases_.begin(); it != aliases_.end();) {
        if (it->second == normalized_name) {
            it = aliases_.erase(it);
        } else {
            ++it;
        }
    }

    Log::debug("Unregistered command: {}", normalized_name);
}

CommandSystem::CommandBuilder CommandSystem::command(std::string_view name, CommandHandler handler) {
    return CommandBuilder(*this, name, std::move(handler));
}

Result<CommandResult> CommandSystem::execute_command(std::shared_ptr<Actor> actor, std::string_view input) {
    if (!actor) {
        return std::unexpected(Errors::InvalidArgument("actor", "cannot be null"));
    }

    // Check for player aliases
    auto player = std::dynamic_pointer_cast<Player>(actor);
    if (player) {
        // Extract first word from input
        std::string_view trimmed = input;
        while (!trimmed.empty() && std::isspace(trimmed.front())) {
            trimmed.remove_prefix(1);
        }

        size_t cmd_end = 0;
        while (cmd_end < trimmed.size() && !std::isspace(trimmed[cmd_end])) {
            cmd_end++;
        }

        if (cmd_end > 0) {
            std::string_view first_word = trimmed.substr(0, cmd_end);
            std::string_view rest_of_input = trimmed.substr(cmd_end);

            // Check if this is an alias (but not "alias" command itself to allow managing aliases)
            if (first_word != "alias") {
                auto alias_cmd = player->get_alias(first_word);
                if (alias_cmd.has_value()) {
                    // Expand alias - the alias value replaces the first word
                    std::string expanded = std::string(*alias_cmd);
                    expanded += rest_of_input;

                    // Handle semicolon-separated commands
                    std::vector<std::string> commands;
                    size_t start = 0;
                    for (size_t i = 0; i <= expanded.size(); i++) {
                        if (i == expanded.size() || expanded[i] == ';') {
                            std::string cmd = expanded.substr(start, i - start);
                            // Trim whitespace
                            size_t begin = cmd.find_first_not_of(" \t");
                            size_t end = cmd.find_last_not_of(" \t");
                            if (begin != std::string::npos) {
                                commands.push_back(cmd.substr(begin, end - begin + 1));
                            }
                            start = i + 1;
                        }
                    }

                    // Execute each command
                    Result<CommandResult> last_result = CommandResult::Success;
                    for (const auto& cmd : commands) {
                        if (!cmd.empty()) {
                            last_result = execute_command(actor, cmd);
                        }
                    }
                    return last_result;
                }
            }
        }
    }

    CommandParser parser;
    auto parse_result = parser.parse(input);
    if (!parse_result) {
        actor->send_message(fmt::format("Invalid command syntax: {}\n", parse_result.error_message));
        return CommandResult::InvalidSyntax;
    }

    return execute_parsed_command(actor, parse_result.command);
}

Result<CommandResult> CommandSystem::execute_parsed_command(std::shared_ptr<Actor> actor,
                                                            const ParsedCommand &command) {
    auto start_time = std::chrono::steady_clock::now();

    // Check for COMMAND triggers on mobs in the room
    // Triggers can intercept commands before normal processing
    if (auto room = actor->current_room()) {
        auto& trigger_mgr = FieryMUD::TriggerManager::instance();
        if (trigger_mgr.is_initialized()) {
            Log::debug("execute_parsed_command: checking COMMAND triggers in room");
            // Check each mob in the room for COMMAND triggers
            int mob_count = 0;
            for (const auto& other : room->contents().actors) {
                // Skip self and players (only mobs have triggers)
                if (other == actor || other->type_name() != "Mobile") {
                    continue;
                }
                mob_count++;
                Log::debug("execute_parsed_command: checking mob #{} '{}'", mob_count, other->name());

                // Build the argument string from parsed command
                std::string argument = command.args_from(0);

                // Dispatch to mob's COMMAND triggers
                auto result = trigger_mgr.dispatch_command(
                    other,      // The mob owning the trigger
                    actor,      // The actor who typed the command
                    command.command,
                    argument
                );
                Log::debug("execute_parsed_command: dispatch_command returned {}", static_cast<int>(result));

                // If trigger returned Halt, the command was handled by the script
                if (result == FieryMUD::TriggerResult::Halt) {
                    Log::debug("Command '{}' intercepted by trigger on {}",
                        command.command, other->name());
                    return CommandResult::Success;  // Trigger handled it
                }
            }
            Log::debug("execute_parsed_command: finished checking {} mobs", mob_count);
        }
    }
    Log::debug("execute_parsed_command: proceeding to command lookup");

    // Lookup command under lock and make a local copy to avoid holding the lock during execution
    std::optional<CommandInfo> cmd_copy_opt;
    {
        std::lock_guard<std::recursive_mutex> lock(system_mutex_);
        const CommandInfo *cmd_info_ptr = find_command(command.command);
        if (cmd_info_ptr) {
            cmd_copy_opt = *cmd_info_ptr; // copies handler and metadata
        }
    }

    if (!cmd_copy_opt) {
        // Try data-driven skill lookup from ability database
        auto& ability_cache = FieryMUD::AbilityCache::instance();
        if (!ability_cache.is_initialized()) {
            auto init_result = ability_cache.initialize();
            if (!init_result) {
                Log::game()->warn("Failed to initialize ability cache: {}", init_result.error().message);
            }
        }
        const auto* ability = ability_cache.get_ability_by_name(command.command);

        // Check if it's a non-spell ability (skills, chants, songs can be used as commands)
        if (ability && ability->type != WorldQueries::AbilityType::Spell) {
            // Create execution context for the skill
            auto context_result = create_context(actor, command);
            if (!context_result) {
                actor->send_message(fmt::format("Error executing {}\n", command.command));
                return CommandResult::SystemError;
            }
            auto context = std::move(context_result.value());

            // Determine if this is a toggle skill (check if actor already has this effect)
            if (ability->is_toggle && actor->has_effect(ability->name)) {
                // Toggle off
                actor->remove_effect(ability->name);
                const auto* messages = ability_cache.get_ability_messages(ability->id);
                if (messages && !messages->wearoff_to_target.empty()) {
                    actor->send_message(messages->wearoff_to_target + "\n");
                } else {
                    actor->send_message(fmt::format("You stop {}.\n", to_lowercase(ability->name)));
                }
                return CommandResult::Success;
            }

            // Execute via data-driven ability system
            // For now, assume non-violent skills are self-targeted toggles
            bool requires_target = ability->violent;
            bool can_initiate_combat = ability->violent;

            return FieryMUD::execute_skill_command(context, command.command,
                                                    requires_target, can_initiate_combat);
        }

        // Compute suggestions without holding the global lock
        auto suggestions = get_command_suggestions(command.command, actor);
        if (!suggestions.empty()) {
            actor->send_message(fmt::format("Unknown command '{}'. Did you mean: {}?\n", command.command,
                                            CommandParserUtils::join(suggestions, ", ")));
        } else {
            actor->send_message(fmt::format("Unknown command '{}'.\n", command.command));
        }
        return CommandResult::NotFound;
    }

    const CommandInfo &cmd_info = *cmd_copy_opt;

    // Check if can execute (no global lock required)
    if (!can_execute_command(actor, cmd_info)) {
        if (!has_privilege(actor, cmd_info.required_privilege)) {
            actor->send_message("You don't have sufficient privileges for that command.\n");
            return CommandResult::InsufficientPrivs;
        }
        // Provide specific error messages based on position
        Position pos = actor->position();
        if (pos == Position::Dead || pos == Position::Ghost) {
            actor->send_message("You can't do that while dead!\n");
        } else if (pos == Position::Sleeping) {
            actor->send_message("You can't do that while sleeping. Try 'wake' first.\n");
        } else if (pos == Position::Sitting || pos == Position::Resting) {
            actor->send_message("You need to stand up first.\n");
        } else if (pos == Position::Fighting) {
            actor->send_message("You can't do that while fighting!\n");
        } else {
            actor->send_message("You can't do that right now.\n");
        }
        return CommandResult::InvalidState;
    }

    // Check cooldown (method guards its own access)
    if (is_on_cooldown(actor, cmd_info.name)) {
        auto remaining = get_cooldown_remaining(actor, cmd_info.name);
        actor->send_message(fmt::format("Command is on cooldown for {} more seconds.\n", remaining.count()));
        return CommandResult::Cooldown;
    }

    // Create execution context
    auto context_result = create_context(actor, command);
    if (!context_result) {
        Log::error("Failed to create command context: {}", context_result.error().message);
        return CommandResult::SystemError;
    }
    auto context = std::move(context_result.value());

    // Execute command (no lock held)
    CommandResult result = CommandResult::SystemError;
    try {
        auto handler_result = cmd_info.handler(context);
        if (handler_result) {
            result = handler_result.value();
        } else {
            context.send_error(handler_result.error().message);
            result = CommandResult::SystemError;
        }
    } catch (const std::exception &e) {
        Log::error("Command '{}' threw exception: {}", cmd_info.name, e.what());
        context.send_error("An internal error occurred while executing the command.");
        result = CommandResult::SystemError;
    }

    // Record execution and update system state under lock
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    {
        std::lock_guard<std::recursive_mutex> lock(system_mutex_);
        update_command_stats(cmd_info.name, result == CommandResult::Success, duration);
        record_command_usage(actor, cmd_info.name);

        // Set cooldown if command succeeded
        if (result == CommandResult::Success && cmd_info.cooldown.count() > 0) {
            cooldowns_[actor->id()][cmd_info.name] =
                CommandCooldown{cmd_info.cooldown, std::chrono::steady_clock::now()};
        }
    }

    if (command_logging_enabled_) {
        // Perform logging outside the lock to reduce contention
        log_command_execution(context, result, duration);
    }

    return result;
}

const CommandInfo *CommandSystem::find_command(std::string_view name) const {
    // Thread-safety note: This function does not acquire the mutex itself.
    // Callers that need a consistent view should hold system_mutex_ while calling it.
    // Historically some tests register commands without calling initialize(); allow lookups regardless.
    std::string normalized_name = normalize_command_name(name);

    // Check direct command
    auto cmd_it = commands_.find(normalized_name);
    if (cmd_it != commands_.end()) {
        return cmd_it->second.get();
    }

    // Check aliases
    auto alias_it = aliases_.find(normalized_name);
    if (alias_it != aliases_.end()) {
        auto real_cmd_it = commands_.find(alias_it->second);
        if (real_cmd_it != commands_.end()) {
            return real_cmd_it->second.get();
        }
    }

    // Try abbreviation matching
    std::vector<std::string> command_names;
    for (const auto &[name, info] : commands_) {
        command_names.push_back(name);
    }
    for (const auto &[alias, real_name] : aliases_) {
        command_names.push_back(alias);
    }

    CommandParser parser;
    auto match = parser.find_command_match(normalized_name, command_names);
    if (match) {
        // Check if it's an alias
        auto alias_match = aliases_.find(*match);
        if (alias_match != aliases_.end()) {
            auto real_cmd_it = commands_.find(alias_match->second);
            if (real_cmd_it != commands_.end()) {
                return real_cmd_it->second.get();
            }
        }

        // Direct command match
        auto cmd_match = commands_.find(*match);
        if (cmd_match != commands_.end()) {
            return cmd_match->second.get();
        }
    }

    return nullptr;
}

std::vector<std::string> CommandSystem::get_available_commands(std::shared_ptr<Actor> actor) const {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    std::vector<std::string> available;

    for (const auto &[name, info] : commands_) {
        if (can_execute_command(actor, *info)) {
            available.push_back(name);
        }
    }

    std::sort(available.begin(), available.end());
    return available;
}

std::vector<std::string> CommandSystem::get_command_suggestions(std::string_view partial,
                                                                std::shared_ptr<Actor> actor) const {
    auto available = get_available_commands(actor);
    CommandParser parser;
    return parser.get_suggestions(partial, available, 5);
}

bool CommandSystem::has_privilege(std::shared_ptr<Actor> actor, PrivilegeLevel required) const {
    if (!actor) {
        return false;
    }

    auto actor_level = get_actor_privilege(actor);
    return CommandSystemUtils::privilege_allows(actor_level, required);
}

bool CommandSystem::has_permission(std::shared_ptr<Actor> actor, std::string_view permission) const {
    if (!actor) {
        return false;
    }

    auto permissions = get_actor_permissions(actor);
    return permissions.contains(std::string{permission});
}

bool CommandSystem::can_execute_command(std::shared_ptr<Actor> actor, const CommandInfo &command) const {
    if (!actor) {
        return false;
    }

    // Check privilege level
    if (!has_privilege(actor, command.required_privilege)) {
        return false;
    }

    // Check additional permissions
    for (const auto &permission : command.required_permissions) {
        if (!has_permission(actor, permission)) {
            return false;
        }
    }

    // Check level requirements
    int actor_level = actor->stats().level;
    if (actor_level < command.minimum_level || actor_level > command.maximum_level) {
        return false;
    }

    // Check conditions based on actor state
    if (!check_command_conditions(actor, command)) {
        return false;
    }

    return true;
}

PrivilegeLevel CommandSystem::get_actor_privilege(std::shared_ptr<Actor> actor) const {
    if (!actor) {
        return PrivilegeLevel::Guest;
    }

    // Try to cast to Player to check god level
    if (auto player = std::dynamic_pointer_cast<Player>(actor)) {
        if (player->is_god()) {
            // Use player's actual level to determine privilege
            int level = player->level();
            if (level >= static_cast<int>(PrivilegeLevel::Overlord)) {
                return PrivilegeLevel::Overlord;
            } else if (level >= static_cast<int>(PrivilegeLevel::Coder)) {
                return PrivilegeLevel::Coder;
            } else if (level >= static_cast<int>(PrivilegeLevel::Builder)) {
                return PrivilegeLevel::Builder;
            } else if (level >= static_cast<int>(PrivilegeLevel::God)) {
                return PrivilegeLevel::God;
            } else if (level >= static_cast<int>(PrivilegeLevel::DemiGod)) {
                return PrivilegeLevel::DemiGod;
            } else {
                return PrivilegeLevel::Helper;
            }
        }
        return PrivilegeLevel::Player;
    }

    // For mobiles and other actors, return Player as default
    return PrivilegeLevel::Player;
}

std::unordered_set<std::string> CommandSystem::get_actor_permissions(std::shared_ptr<Actor> actor) const {
    if (!actor) {
        return {};
    }

    // Get base permissions for privilege level
    auto privilege = get_actor_privilege(actor);
    auto permissions = CommandSystemUtils::get_default_permissions(privilege);

    // Add actor-specific permissions (placeholder for future implementation)
    // In the future, this could check actor->get_custom_permissions() or similar

    return permissions;
}

void CommandSystem::register_category(CommandCategory category) {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);
    categories_[category.name] = std::move(category);
}

std::vector<CommandCategory> CommandSystem::get_categories() const {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    std::vector<CommandCategory> cats;
    for (const auto &[name, category] : categories_) {
        cats.push_back(category);
    }

    std::sort(cats.begin(), cats.end(), [](const auto &a, const auto &b) { return a.name < b.name; });

    return cats;
}

std::vector<std::string> CommandSystem::get_commands_in_category(std::string_view category) const {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    std::vector<std::string> commands;

    for (const auto &[name, info] : commands_) {
        if (info->category == category) {
            commands.push_back(name);
        }
    }

    std::sort(commands.begin(), commands.end());
    return commands;
}

const CommandStats *CommandSystem::get_command_stats(std::string_view command) const {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    std::string normalized_name = normalize_command_name(command);
    auto it = command_stats_.find(normalized_name);
    return it != command_stats_.end() ? &it->second : nullptr;
}

std::vector<CommandStats> CommandSystem::get_all_stats() const {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    std::vector<CommandStats> stats;
    for (const auto &[name, stat] : command_stats_) {
        stats.push_back(stat);
    }

    std::sort(stats.begin(), stats.end(),
              [](const auto &a, const auto &b) { return a.total_executions > b.total_executions; });

    return stats;
}

void CommandSystem::reset_stats() {
    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    for (auto &[name, stats] : command_stats_) {
        stats = CommandStats{name};
    }
}

bool CommandSystem::is_on_cooldown(std::shared_ptr<Actor> actor, std::string_view command) const {
    if (!actor) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    auto actor_it = cooldowns_.find(actor->id());
    if (actor_it == cooldowns_.end()) {
        return false;
    }

    std::string normalized_name = normalize_command_name(command);
    auto cmd_it = actor_it->second.find(normalized_name);
    if (cmd_it == actor_it->second.end()) {
        return false;
    }

    return cmd_it->second.is_on_cooldown();
}

std::chrono::seconds CommandSystem::get_cooldown_remaining(std::shared_ptr<Actor> actor,
                                                           std::string_view command) const {
    if (!actor) {
        return std::chrono::seconds{0};
    }

    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    auto actor_it = cooldowns_.find(actor->id());
    if (actor_it == cooldowns_.end()) {
        return std::chrono::seconds{0};
    }

    std::string normalized_name = normalize_command_name(command);
    auto cmd_it = actor_it->second.find(normalized_name);
    if (cmd_it == actor_it->second.end()) {
        return std::chrono::seconds{0};
    }

    return cmd_it->second.remaining();
}

void CommandSystem::clear_cooldown(std::shared_ptr<Actor> actor, std::string_view command) {
    if (!actor) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    auto actor_it = cooldowns_.find(actor->id());
    if (actor_it != cooldowns_.end()) {
        std::string normalized_name = normalize_command_name(command);
        actor_it->second.erase(normalized_name);
    }
}

void CommandSystem::clear_all_cooldowns(std::shared_ptr<Actor> actor) {
    if (!actor) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(system_mutex_);
    cooldowns_.erase(actor->id());
}

void CommandSystem::log_command_execution(const CommandContext &context, CommandResult result,
                                          std::chrono::milliseconds duration) {
    Log::trace("Command executed: {} by {} in room {} - {} ({}ms)", context.command.command,
               context.actor ? context.actor->name() : "unknown",
               context.room ? fmt::format("{}", context.room->id()) : "none",
               CommandSystemUtils::result_to_string(result), duration.count());
}

std::vector<std::string> CommandSystem::get_command_history(std::shared_ptr<Actor> actor, size_t count) const {
    if (!actor) {
        return {};
    }

    std::lock_guard<std::recursive_mutex> lock(system_mutex_);

    auto it = command_history_.find(actor->id());
    if (it == command_history_.end()) {
        return {};
    }

    const auto &history = it->second;
    size_t start = history.size() > count ? history.size() - count : 0;

    return std::vector<std::string>(history.begin() + start, history.end());
}

Result<CommandContext> CommandSystem::create_context(std::shared_ptr<Actor> actor, const ParsedCommand &command) const {
    CommandContext context;
    context.actor = actor;

    context.room = actor->current_room();
    context.command = command;
    context.start_time = std::chrono::steady_clock::now();
    context.actor_privilege = get_actor_privilege(actor);
    context.actor_permissions = get_actor_permissions(actor);

    return context;
}

bool CommandSystem::check_command_conditions(std::shared_ptr<Actor> actor, const CommandInfo &command) const {
    if (!actor) {
        return false;
    }

    // Check if actor is dead or ghost - allow informational commands for both states
    if (actor->position() == Position::Ghost || actor->position() == Position::Dead) {
        // Allow informational commands and release for dead/ghost players
        std::string cmd_name = to_lowercase(command.name);

        // List of commands dead/ghost players can use
        static const std::set<std::string> ghost_allowed_commands = {
            "look",      "l",       // Look around
            "release",              // Return to life
            "help",      "?",       // Help system
            "score",     "sc",      // Character stats
            "inventory", "i",       // Inventory
            "equipment", "eq",      // Equipment
            "commands",  "comm",    // Available commands
            "who",       "users",   // Who's online
            "time",                 // Game time
            "weather",              // Weather
            "exits",                // Available exits
            "emote",     ":",       // Emotes
            "say",       "'",       // Communication
            "tell",      "whisper", // Private communication
            "gossip",    "ooc",     // OOC channels
            "info",      "news",    // Information commands
            "title",                // View/set title
            "prompt"                // Prompt settings
        };

        if (ghost_allowed_commands.contains(cmd_name)) {
            return true;
        }

        // Check aliases too
        for (const auto &alias : command.aliases) {
            std::string alias_lower = to_lowercase(alias);
            if (ghost_allowed_commands.contains(alias_lower)) {
                return true;
            }
        }

        // Allow all socials for ghosts (waving, sighing, etc. make sense for spirits)
        if (command.category == "Social") {
            return true;
        }

        return false; // Dead/ghost players cannot use any other commands
    }

    // Check position-based restrictions
    Position actor_position = actor->position();

    // Get command name for position checks
    std::string cmd_name = to_lowercase(command.name);

    // Commands that require the actor to be awake
    static const std::unordered_set<std::string> awake_commands = {
        "look", "l", "who", "time", "weather", "exits", "inventory", "i", "equipment", "eq", "commands", "comm"};

    // Commands that can be used while sleeping
    static const std::unordered_set<std::string> sleep_commands = {"wake", "quit", "save", "ooc", "gossip"};

    if (actor_position == Position::Sleeping) {
        return sleep_commands.contains(cmd_name) || awake_commands.contains(cmd_name);
    }

    // Commands that require standing (movement, combat, etc.)
    static const std::unordered_set<std::string> standing_commands = {
        "north", "south", "east", "west",   "up",    "down",    "ne",   "nw",    "se",   "sw",     "get",   "drop",
        "put",   "give",  "wear", "remove", "wield", "unwield", "open", "close", "lock", "unlock", "enter", "leave"};

    if ((actor_position == Position::Sitting || actor_position == Position::Resting) &&
        standing_commands.contains(cmd_name)) {
        return false; // Must stand up first
    }

    // All other commands allowed for living actors
    return true;
}

void CommandSystem::record_command_usage(std::shared_ptr<Actor> actor, std::string_view command) {
    if (!actor) {
        return;
    }

    auto &history = command_history_[actor->id()];
    history.emplace_back(command);

    if (history.size() > max_command_history_) {
        history.erase(history.begin());
    }
}

void CommandSystem::update_command_stats(std::string_view command, bool success, std::chrono::milliseconds duration) {
    auto &stats = command_stats_[std::string{command}];
    stats.record_execution(success, duration);
}

std::string CommandSystem::normalize_command_name(std::string_view name) const {
    return CommandParserUtils::to_lower(name);
}

bool CommandSystem::is_valid_permission_name(std::string_view permission) const {
    if (permission.empty() || permission.length() > 30) {
        return false;
    }

    return std::all_of(permission.begin(), permission.end(), [](char c) { return std::isalnum(c) || c == '_'; });
}

// CommandBuilder Implementation

CommandSystem::CommandBuilder::CommandBuilder(CommandSystem &system, std::string_view name, CommandHandler handler)
    : system_(system), info_(name, std::move(handler)) {}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::alias(std::string_view alias_name) {
    info_.aliases.emplace_back(alias_name);
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::aliases(std::span<const std::string> alias_names) {
    for (const auto &alias : alias_names) {
        info_.aliases.push_back(alias);
    }
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::category(std::string_view cat) {
    info_.category = cat;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::description(std::string_view desc) {
    info_.description = desc;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::usage(std::string_view usage_text) {
    info_.usage = usage_text;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::help(std::string_view help_text) {
    info_.help_text = help_text;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::privilege(PrivilegeLevel level) {
    info_.required_privilege = level;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::permission(std::string_view perm) {
    info_.required_permissions.emplace(perm);
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::permissions(std::span<const std::string> perms) {
    for (const auto &perm : perms) {
        info_.required_permissions.emplace(perm);
    }
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::requires_target(bool required) {
    info_.requires_target = required;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::requires_room(bool required) {
    info_.requires_room = required;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::usable_in_combat(bool allowed) {
    info_.can_use_while_fighting = allowed;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::usable_while_sitting(bool allowed) {
    info_.can_use_while_sitting = allowed;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::usable_while_sleeping(bool allowed) {
    info_.can_use_while_sleeping = allowed;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::cooldown(std::chrono::seconds duration) {
    info_.cooldown = duration;
    return *this;
}

CommandSystem::CommandBuilder &CommandSystem::CommandBuilder::level_range(int min_level, int max_level) {
    info_.minimum_level = min_level;
    info_.maximum_level = max_level;
    return *this;
}

Result<void> CommandSystem::CommandBuilder::build() { return system_.register_command(std::move(info_)); }

// CommandSystemUtils Implementation

namespace CommandSystemUtils {
std::string_view privilege_to_string(PrivilegeLevel level) {
    switch (level) {
    case PrivilegeLevel::Guest:
        return "Guest";
    case PrivilegeLevel::Player:
        return "Player";
    case PrivilegeLevel::Veteran:
        return "Veteran";
    case PrivilegeLevel::MaxMortal:
        return "MaxMortal";
    case PrivilegeLevel::Helper:
        return "Helper";
    case PrivilegeLevel::DemiGod:
        return "DemiGod";
    case PrivilegeLevel::God:
        return "God";
    case PrivilegeLevel::Builder:
        return "Builder";
    case PrivilegeLevel::Coder:
        return "Coder";
    case PrivilegeLevel::Overlord:
        return "Overlord";
    }
    return "Unknown";
}

std::optional<PrivilegeLevel> parse_privilege_level(std::string_view str) {
    std::string lower = CommandParserUtils::to_lower(str);

    if (lower == "guest")
        return PrivilegeLevel::Guest;
    if (lower == "player")
        return PrivilegeLevel::Player;
    if (lower == "veteran")
        return PrivilegeLevel::Veteran;
    if (lower == "maxmortal")
        return PrivilegeLevel::MaxMortal;
    if (lower == "helper")
        return PrivilegeLevel::Helper;
    if (lower == "demigod")
        return PrivilegeLevel::DemiGod;
    if (lower == "god")
        return PrivilegeLevel::God;
    if (lower == "builder")
        return PrivilegeLevel::Builder;
    if (lower == "coder")
        return PrivilegeLevel::Coder;
    if (lower == "overlord")
        return PrivilegeLevel::Overlord;

    return std::nullopt;
}

std::string_view result_to_string(CommandResult result) {
    switch (result) {
    case CommandResult::Success:
        return "Success";
    case CommandResult::InvalidSyntax:
        return "InvalidSyntax";
    case CommandResult::InsufficientPrivs:
        return "InsufficientPrivs";
    case CommandResult::NotFound:
        return "NotFound";
    case CommandResult::Cooldown:
        return "Cooldown";
    case CommandResult::Busy:
        return "Busy";
    case CommandResult::InvalidTarget:
        return "InvalidTarget";
    case CommandResult::InvalidState:
        return "InvalidState";
    case CommandResult::ResourceError:
        return "ResourceError";
    case CommandResult::SystemError:
        return "SystemError";
    case CommandResult::Cancelled:
        return "Cancelled";
    }
    return "Unknown";
}

std::string format_execution_time(std::chrono::milliseconds duration) {
    if (duration < std::chrono::milliseconds{1}) {
        return "< 1ms";
    } else if (duration < std::chrono::seconds{1}) {
        return fmt::format("{}ms", duration.count());
    } else {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        return fmt::format("{}.{:03d}s", seconds.count(), (duration - seconds).count());
    }
}

std::string format_privilege_level(PrivilegeLevel level, bool use_color) {
    if (!use_color) {
        return std::string{privilege_to_string(level)};
    }

    // Add color codes based on privilege level
    switch (level) {
    case PrivilegeLevel::Guest:
        return fmt::format("\033[37m{}\033[0m", privilege_to_string(level)); // White
    case PrivilegeLevel::Player:
        return fmt::format("\033[32m{}\033[0m", privilege_to_string(level)); // Green
    case PrivilegeLevel::God:
        return fmt::format("\033[33m{}\033[0m", privilege_to_string(level)); // Yellow
    default:
        return std::string{privilege_to_string(level)};
    }
}

bool privilege_allows(PrivilegeLevel actor_level, PrivilegeLevel required_level) {
    return static_cast<int>(actor_level) >= static_cast<int>(required_level);
}

std::vector<PrivilegeLevel> get_privilege_hierarchy() {
    return {PrivilegeLevel::Guest,  PrivilegeLevel::Player,  PrivilegeLevel::Veteran, PrivilegeLevel::MaxMortal,
            PrivilegeLevel::Helper, PrivilegeLevel::DemiGod, PrivilegeLevel::God,     PrivilegeLevel::Builder,
            PrivilegeLevel::Coder,  PrivilegeLevel::Overlord};
}

std::unordered_set<std::string> get_default_permissions(PrivilegeLevel level) {
    std::unordered_set<std::string> permissions;

    // Everyone gets basic permissions
    // Higher levels inherit all lower permissions

    if (level >= PrivilegeLevel::Builder) {
        permissions.insert(std::string{Permissions::BUILD});
        permissions.insert(std::string{Permissions::OLC});
    }

    if (level >= PrivilegeLevel::Coder) {
        permissions.insert(std::string{Permissions::CODE});
    }

    if (level >= PrivilegeLevel::Builder) {
        permissions.insert(std::string{Permissions::ADMIN});
        permissions.insert(std::string{Permissions::FORCE});
        permissions.insert(std::string{Permissions::TELEPORT});
        permissions.insert(std::string{Permissions::SUMMON});
        permissions.insert(std::string{Permissions::TRANSFER});
        permissions.insert(std::string{Permissions::ADVANCE});
        permissions.insert(std::string{Permissions::RESTORE});
        permissions.insert(std::string{Permissions::FREEZE});
        permissions.insert(std::string{Permissions::THAW});
        permissions.insert(std::string{Permissions::BAN});
        permissions.insert(std::string{Permissions::UNBAN});
        permissions.insert(std::string{Permissions::DC});
    }

    if (level >= PrivilegeLevel::God) {
        permissions.insert(std::string{Permissions::GOD});
        permissions.insert(std::string{Permissions::SNOOP});
        permissions.insert(std::string{Permissions::INVISIBLE});
        permissions.insert(std::string{Permissions::NOHASSLE});
        permissions.insert(std::string{Permissions::WIZLOCK});
        permissions.insert(std::string{Permissions::LOG});
        permissions.insert(std::string{Permissions::SYSLOG});
        permissions.insert(std::string{Permissions::ZONE_RESET});
        permissions.insert(std::string{Permissions::WIZNET});
    }

    if (level >= PrivilegeLevel::Coder) {
        permissions.insert(std::string{Permissions::SHUTDOWN});
    }

    return permissions;
}

bool is_valid_command_name(std::string_view name) { return CommandParser::is_valid_command_name(name); }

bool is_valid_category_name(std::string_view name) {
    if (name.empty() || name.length() > 30) {
        return false;
    }

    return std::all_of(name.begin(), name.end(), [](char c) { return std::isalnum(c) || c == '_' || c == ' '; });
}

int get_command_complexity(std::string_view command) {
    // Simple heuristic for command complexity
    // Could be expanded based on actual command behavior

    static const std::unordered_map<std::string, int> complexity_map = {
        {"look", 1},      {"l", 1},      {"say", 1},  {"tell", 1}, {"who", 2},   {"where", 2},
        {"inventory", 1}, {"i", 1},      {"quit", 1}, {"save", 2}, {"get", 2},   {"drop", 2},
        {"wear", 2},      {"remove", 2}, {"kill", 3}, {"cast", 3}, {"build", 5}, {"reboot", 5}};

    std::string lower_cmd = CommandParserUtils::to_lower(command);
    auto it = complexity_map.find(lower_cmd);
    return it != complexity_map.end() ? it->second : 2; // Default complexity
}

std::string format_cooldown_remaining(std::chrono::seconds remaining) {
    if (remaining.count() == 0) {
        return "ready";
    } else if (remaining.count() < 60) {
        return fmt::format("{}s", remaining.count());
    } else {
        auto minutes = remaining.count() / 60;
        auto seconds = remaining.count() % 60;
        return fmt::format("{}m{}s", minutes, seconds);
    }
}
} // namespace CommandSystemUtils

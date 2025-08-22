/***************************************************************************
 *   File: src/modern/game/command_system.cpp          Part of FieryMUD *
 *  Usage: Modern command processing system implementation                 *
 ***************************************************************************/

#include "command_system.hpp"
#include "../core/actor.hpp"
#include "../net/player_connection.hpp"
#include "../world/world_server.hpp"
#include "../world/room.hpp"
#include "../core/logging.hpp"
#include <algorithm>
#include <sstream>
#include <fmt/format.h>

// CommandContext implementation
std::string CommandContext::args_as_string() const {
    if (args.empty()) return "";
    
    std::string result = args[0];
    for (size_t i = 1; i < args.size(); ++i) {
        result += " " + args[i];
    }
    return result;
}

// CommandDef implementation
bool CommandDef::matches(std::string_view input) const {
    // Check primary name
    if (name.starts_with(input)) return true;
    
    // Check aliases
    for (const auto& alias : aliases) {
        if (alias.starts_with(input)) return true;
    }
    
    return false;
}

// CommandSystem implementation
CommandSystem::CommandSystem(std::shared_ptr<WorldServer> world) 
    : world_(std::move(world)) {
    register_builtin_commands();
}

void CommandSystem::register_command(CommandDef command) {
    // Add to commands list
    size_t index = commands_.size();
    commands_.push_back(std::move(command));
    
    // Update fast lookup index
    const auto& cmd = commands_[index];
    command_index_[cmd.name] = index;
    
    for (const auto& alias : cmd.aliases) {
        command_index_[alias] = index;
    }
}

CommandResult CommandSystem::process_input(std::shared_ptr<Player> player, std::string_view input) {
    Log::debug("CommandSystem::process_input: player='{}', input='{}'", 
               player ? player->name() : "null", input);
    
    if (!player) {
        Log::error("CommandSystem::process_input called with null player");
        return CommandResult::PlayerError;
    }
    
    if (input.empty()) {
        Log::debug("Empty input, ignoring");
        return CommandResult::Success; // Empty input is valid, just ignore
    }
    
    // Parse the input
    auto ctx = parse_input(player, input);
    
    Log::debug("Parsed command: '{}', args count: {}", ctx.command, ctx.args.size());
    
    if (ctx.command.empty()) {
        Log::debug("Command is empty after parsing, ignoring");
        return CommandResult::Success; // Only whitespace
    }
    
    // Find the command
    const CommandDef* cmd = find_command(ctx.command);
    if (!cmd) {
        Log::debug("Command '{}' not found", ctx.command);
        player->send_message("Huh?");
        return CommandResult::NotFound;
    }
    
    Log::debug("Found command: '{}'", cmd->name);
    
    // Check permissions
    if (!can_use_command(player, *cmd)) {
        Log::debug("Player '{}' doesn't have permission for command '{}'", player->name(), cmd->name);
        player->send_message("You don't have permission to use that command.");
        return CommandResult::AccessDenied;
    }
    
    // Execute the command
    try {
        Log::debug("Executing command '{}' for player '{}'", cmd->name, player->name());
        auto result = cmd->handler(ctx);
        Log::debug("Command '{}' completed with result: {}", cmd->name, static_cast<int>(result));
        return result;
    } catch (const std::exception& e) {
        Log::error("Command '{}' threw exception: {}", cmd->name, e.what());
        player->send_message("An error occurred while processing that command.");
        return CommandResult::SystemError;
    }
}

const CommandDef* CommandSystem::find_command(std::string_view name) const {
    // Convert to lowercase for case-insensitive matching
    std::string lower_name;
    lower_name.reserve(name.size());
    std::transform(name.begin(), name.end(), std::back_inserter(lower_name), 
                   [](char c) { return std::tolower(c); });
    
    // Try exact match first
    auto it = command_index_.find(lower_name);
    if (it != command_index_.end()) {
        return &commands_[it->second];
    }
    
    // Try prefix matching
    for (const auto& cmd : commands_) {
        if (cmd.matches(lower_name)) {
            return &cmd;
        }
    }
    
    return nullptr;
}

std::vector<const CommandDef*> CommandSystem::find_matching_commands(std::string_view partial) const {
    std::vector<const CommandDef*> matches;
    
    std::string lower_partial;
    lower_partial.reserve(partial.size());
    std::transform(partial.begin(), partial.end(), std::back_inserter(lower_partial),
                   [](char c) { return std::tolower(c); });
    
    for (const auto& cmd : commands_) {
        if (cmd.matches(lower_partial)) {
            matches.push_back(&cmd);
        }
    }
    
    return matches;
}

std::vector<std::string> CommandSystem::get_help_text(std::string_view command) const {
    std::vector<std::string> help;
    
    if (command.empty()) {
        help.push_back("Available commands:");
        help.push_back("");
        
        for (const auto& cmd : commands_) {
            if (!cmd.hidden) {
                help.push_back(fmt::format("  {:<12} - {}", cmd.name, cmd.description));
            }
        }
        
        help.push_back("");
        help.push_back("Type 'help <command>' for detailed information about a specific command.");
    } else {
        const CommandDef* cmd = find_command(command);
        if (cmd) {
            help.push_back(fmt::format("Command: {}", cmd->name));
            if (!cmd->aliases.empty()) {
                help.push_back(fmt::format("Aliases: {}", 
                    [&]() {
                        std::string result = cmd->aliases[0];
                        for (size_t i = 1; i < cmd->aliases.size(); ++i) {
                            result += ", " + cmd->aliases[i];
                        }
                        return result;
                    }()));
            }
            help.push_back(fmt::format("Usage: {}", cmd->usage));
            help.push_back("");
            help.push_back(cmd->description);
        } else {
            help.push_back(fmt::format("No help available for '{}'.", command));
        }
    }
    
    return help;
}

std::vector<std::string> CommandSystem::list_available_commands(std::shared_ptr<Player> player) const {
    std::vector<std::string> available;
    
    for (const auto& cmd : commands_) {
        if (!cmd.hidden && can_use_command(player, cmd)) {
            available.push_back(cmd.name);
        }
    }
    
    std::sort(available.begin(), available.end());
    return available;
}

CommandContext CommandSystem::parse_input(std::shared_ptr<Player> player, std::string_view input) const {
    CommandContext ctx;
    ctx.player = player;
    ctx.world = world_;
    ctx.full_input = input;
    
    // Find first non-whitespace
    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return ctx; // Only whitespace
    }
    
    // Find end of command (first whitespace after command)
    size_t cmd_end = input.find_first_of(" \t\r\n", start);
    if (cmd_end == std::string::npos) {
        // No arguments
        ctx.command = input.substr(start);
    } else {
        // Extract command and arguments
        ctx.command = input.substr(start, cmd_end - start);
        
        // Find start of arguments
        size_t args_start = input.find_first_not_of(" \t\r\n", cmd_end);
        if (args_start != std::string::npos) {
            std::string_view args_str = input.substr(args_start);
            ctx.args = tokenize_args(args_str);
        }
    }
    
    // Convert command to lowercase
    std::transform(ctx.command.begin(), ctx.command.end(), ctx.command.begin(),
                   [](char c) { return std::tolower(c); });
    
    return ctx;
}

std::vector<std::string> CommandSystem::tokenize_args(std::string_view args) const {
    std::vector<std::string> tokens;
    std::string args_str{args};
    std::istringstream iss{args_str};
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

bool CommandSystem::can_use_command(std::shared_ptr<Player> player, const CommandDef& cmd) const {
    if (!player) return false;
    
    // Check level requirement
    if (player->stats().level < cmd.min_level) {
        return false;
    }
    
    // Additional permission checks could go here
    // (immortal commands, class restrictions, etc.)
    
    return true;
}

void CommandSystem::register_builtin_commands() {
    // Look command
    register_command({
        .name = "look",
        .aliases = {"l"},
        .handler = [](const CommandContext& ctx) -> CommandResult {
            auto room = ctx.player->current_room();
            if (!room) {
                ctx.player->send_message("You are nowhere.");
                return CommandResult::SystemError;
            }
            
            // Send room information
            ctx.player->send_message(fmt::format("{}", room->name()));
            ctx.player->send_message(fmt::format("{}", room->description()));
            
            // Show exits
            auto exits = room->get_visible_exits();
            if (!exits.empty()) {
                std::string exit_list = "Exits: ";
                bool first = true;
                for (const auto& direction : exits) {
                    if (!first) exit_list += ", ";
                    exit_list += RoomUtils::get_direction_name(direction);
                    first = false;
                }
                ctx.player->send_message(exit_list);
            } else {
                ctx.player->send_message("No visible exits.");
            }
            
            return CommandResult::Success;
        },
        .description = "Look around the current room",
        .usage = "look"
    });
    
    // Movement commands
    auto create_move_command = [](const std::string& direction) {
        return CommandDef{
            .name = direction,
            .aliases = {direction.substr(0, 1)}, // First letter as alias
            .handler = [direction](const CommandContext& ctx) -> CommandResult {
                auto dir_enum = RoomUtils::parse_direction(direction);
                if (!dir_enum) {
                    ctx.player->send_message(fmt::format("Invalid direction: {}", direction));
                    return CommandResult::InvalidArgs;
                }
                auto current_room = ctx.player->current_room();
                if (!current_room) {
                    ctx.player->send_message("You are nowhere.");
                    return CommandResult::SystemError;
                }
                
                if (!current_room->has_exit(*dir_enum)) {
                    ctx.player->send_message(fmt::format("You can't go {} from here.", direction));
                    return CommandResult::InvalidArgs;
                }
                
                const ExitInfo* exit_info = current_room->get_exit(*dir_enum);
                if (!exit_info) {
                    ctx.player->send_message("That exit is blocked.");
                    return CommandResult::InvalidArgs;
                }
                
                EntityId destination = exit_info->to_room;
                auto dest_room = ctx.world->get_room(destination);
                if (!dest_room) {
                    ctx.player->send_message("That exit leads nowhere.");
                    return CommandResult::SystemError;
                }
                
                // Move the player
                ctx.player->set_current_room(dest_room);
                ctx.player->send_message(fmt::format("You go {}.", direction));
                
                // Auto-look at new room
                ctx.player->send_message(fmt::format("{}", dest_room->name()));
                ctx.player->send_message(fmt::format("{}", dest_room->description()));
                
                return CommandResult::Success;
            },
            .description = fmt::format("Move {}", direction),
            .usage = direction
        };
    };
    
    register_command(create_move_command("north"));
    register_command(create_move_command("south"));
    register_command(create_move_command("east"));
    register_command(create_move_command("west"));
    register_command(create_move_command("up"));
    register_command(create_move_command("down"));
    
    // Who command
    register_command({
        .name = "who",
        .aliases = {},
        .handler = [](const CommandContext& ctx) -> CommandResult {
            auto online_players = ctx.world->get_online_players();
            
            ctx.player->send_message("Players currently online:");
            ctx.player->send_message("========================");
            
            if (online_players.empty()) {
                ctx.player->send_message("No one is online.");
            } else {
                for (const auto& player : online_players) {
                    ctx.player->send_message(fmt::format("  {} (Level {})", 
                        player->name(), player->stats().level));
                }
            }
            
            ctx.player->send_message(fmt::format("Total: {} player{}", 
                online_players.size(), online_players.size() == 1 ? "" : "s"));
            
            return CommandResult::Success;
        },
        .description = "List players currently online",
        .usage = "who"
    });
    
    // Help command
    register_command({
        .name = "help",
        .aliases = {"?"},
        .handler = [this](const CommandContext& ctx) -> CommandResult {
            std::string topic;
            if (ctx.has_args()) {
                topic = ctx.args[0];
            }
            
            auto help_lines = get_help_text(topic);
            for (const auto& line : help_lines) {
                ctx.player->send_message(line);
            }
            
            return CommandResult::Success;
        },
        .description = "Get help on commands",
        .usage = "help [command]"
    });
    
    // Quit command
    register_command({
        .name = "quit",
        .aliases = {"q", "bye"},
        .handler = [](const CommandContext& ctx) -> CommandResult {
            ctx.player->send_message("Goodbye!");
            
            // Remove player from world server (this will trigger disconnection)
            ctx.world->remove_player(ctx.player);
            
            return CommandResult::Success;
        },
        .description = "Quit the game",
        .usage = "quit"
    });
}
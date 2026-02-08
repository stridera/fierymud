// Character position management commands (sit, stand, rest, sleep, wake, etc.)

#pragma once

#include <memory>

#include "core/result.hpp"

class CommandContext;
enum class CommandResult;

class Actor;
enum class Position;

namespace PositionCommands {

// Module registration
Result<void> register_commands();

// Basic position commands
Result<CommandResult> cmd_sit(const CommandContext &ctx);
Result<CommandResult> cmd_stand(const CommandContext &ctx);
Result<CommandResult> cmd_rest(const CommandContext &ctx);
Result<CommandResult> cmd_sleep(const CommandContext &ctx);
Result<CommandResult> cmd_wake(const CommandContext &ctx);

// Extended position commands
Result<CommandResult> cmd_kneel(const CommandContext &ctx);
Result<CommandResult> cmd_recline(const CommandContext &ctx);

// Helper functions
std::string_view position_name(Position pos);
bool can_change_position(std::shared_ptr<Actor> actor, Position new_pos, std::string &reason);

} // namespace PositionCommands

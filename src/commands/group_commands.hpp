#pragma once

#include "core/result.hpp"

class CommandContext;
enum class CommandResult;

namespace GroupCommands {

// Module registration
Result<void> register_commands();

// Group management commands
Result<CommandResult> cmd_group(const CommandContext &ctx);
Result<CommandResult> cmd_follow(const CommandContext &ctx);
Result<CommandResult> cmd_unfollow(const CommandContext &ctx);
Result<CommandResult> cmd_report(const CommandContext &ctx);

// Group extension commands
Result<CommandResult> cmd_split(const CommandContext &ctx);
Result<CommandResult> cmd_disband(const CommandContext &ctx);
Result<CommandResult> cmd_abandon(const CommandContext &ctx);
Result<CommandResult> cmd_gtell(const CommandContext &ctx);
Result<CommandResult> cmd_gsay(const CommandContext &ctx);

} // namespace GroupCommands

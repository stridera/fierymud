#pragma once

#include "core/result.hpp"

class CommandContext;
enum class CommandResult;

namespace AccountCommands {

// Module registration
Result<void> register_commands();

// Account commands
Result<CommandResult> cmd_account(const CommandContext &ctx);
Result<CommandResult> cmd_account_link(const CommandContext &ctx);
Result<CommandResult> cmd_account_unlink(const CommandContext &ctx);
Result<CommandResult> cmd_account_delete(const CommandContext &ctx);
Result<CommandResult> cmd_account_characters(const CommandContext &ctx);

} // namespace AccountCommands

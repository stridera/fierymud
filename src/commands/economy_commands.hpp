#pragma once

#include "command_system.hpp"

namespace EconomyCommands {

// Module registration
Result<void> register_commands();

// Shop enhancement commands
Result<CommandResult> cmd_value(const CommandContext &ctx);
Result<CommandResult> cmd_identify(const CommandContext &ctx);
Result<CommandResult> cmd_repair(const CommandContext &ctx);

// Banking commands
Result<CommandResult> cmd_deposit(const CommandContext &ctx);
Result<CommandResult> cmd_withdraw(const CommandContext &ctx);
Result<CommandResult> cmd_balance(const CommandContext &ctx);
Result<CommandResult> cmd_transfer(const CommandContext &ctx);

// Currency commands
Result<CommandResult> cmd_coins(const CommandContext &ctx);
Result<CommandResult> cmd_exchange(const CommandContext &ctx);

// Account storage commands
Result<CommandResult> cmd_account(const CommandContext &ctx);

} // namespace EconomyCommands

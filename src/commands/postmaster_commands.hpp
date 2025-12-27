#pragma once

#include "command_system.hpp"

namespace PostmasterCommands {

// Module registration
Result<void> register_commands();

// Mail commands (require postmaster in room)
Result<CommandResult> cmd_mail(const CommandContext &ctx);
Result<CommandResult> cmd_check(const CommandContext &ctx);
Result<CommandResult> cmd_receive(const CommandContext &ctx);

} // namespace PostmasterCommands

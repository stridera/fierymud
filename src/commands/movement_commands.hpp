#pragma once

#include "command_system.hpp"

namespace MovementCommands {

// Module registration
Result<void> register_commands();

// Movement commands
Result<CommandResult> cmd_north(const CommandContext &ctx);
Result<CommandResult> cmd_south(const CommandContext &ctx);
Result<CommandResult> cmd_east(const CommandContext &ctx);
Result<CommandResult> cmd_west(const CommandContext &ctx);
Result<CommandResult> cmd_up(const CommandContext &ctx);
Result<CommandResult> cmd_down(const CommandContext &ctx);
Result<CommandResult> cmd_exits(const CommandContext &ctx);

// Position commands
Result<CommandResult> cmd_fly(const CommandContext &ctx);
Result<CommandResult> cmd_land(const CommandContext &ctx);

} // namespace MovementCommands
#pragma once

#include "command_system.hpp"

namespace MagicCommands {

// Module registration
Result<void> register_commands();

// Spell management
Result<CommandResult> cmd_spells(const CommandContext &ctx);
Result<CommandResult> cmd_memorize(const CommandContext &ctx);
Result<CommandResult> cmd_forget(const CommandContext &ctx);

// Skill/ability display
Result<CommandResult> cmd_abilities(const CommandContext &ctx);
Result<CommandResult> cmd_innate(const CommandContext &ctx);

// Recovery commands
Result<CommandResult> cmd_meditate(const CommandContext &ctx);
Result<CommandResult> cmd_concentrate(const CommandContext &ctx);

} // namespace MagicCommands

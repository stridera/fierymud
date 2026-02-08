#pragma once

#include "command_context.hpp"
#include "command_system.hpp"

/**
 * System commands for FieryMUD.
 *
 * This module provides core system commands for players including
 * session management, help systems, and player information.
 */

namespace SystemCommands {
// Module registration
Result<void> register_commands();

// System Commands
Result<CommandResult> cmd_quit(const CommandContext &ctx);
Result<CommandResult> cmd_save(const CommandContext &ctx);
Result<CommandResult> cmd_help(const CommandContext &ctx);
Result<CommandResult> cmd_commands(const CommandContext &ctx);
Result<CommandResult> cmd_socials(const CommandContext &ctx);
Result<CommandResult> cmd_prompt(const CommandContext &ctx);
Result<CommandResult> cmd_richtest(const CommandContext &ctx);
Result<CommandResult> cmd_clientinfo(const CommandContext &ctx);

// Feedback commands
Result<CommandResult> cmd_bug(const CommandContext &ctx);
Result<CommandResult> cmd_idea(const CommandContext &ctx);
Result<CommandResult> cmd_typo(const CommandContext &ctx);

// Utility commands
Result<CommandResult> cmd_date(const CommandContext &ctx);
Result<CommandResult> cmd_clear(const CommandContext &ctx);
Result<CommandResult> cmd_color(const CommandContext &ctx);
Result<CommandResult> cmd_display(const CommandContext &ctx);
Result<CommandResult> cmd_alias(const CommandContext &ctx);
Result<CommandResult> cmd_ignore(const CommandContext &ctx);
} // namespace SystemCommands

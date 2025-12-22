#pragma once

#include "command_system.hpp"

namespace InformationCommands {

// Module registration
Result<void> register_commands();

// Information and status commands
Result<CommandResult> cmd_look(const CommandContext &ctx);
Result<CommandResult> cmd_examine(const CommandContext &ctx);
Result<CommandResult> cmd_who(const CommandContext &ctx);
Result<CommandResult> cmd_where(const CommandContext &ctx);
Result<CommandResult> cmd_inventory(const CommandContext &ctx);
Result<CommandResult> cmd_equipment(const CommandContext &ctx);
Result<CommandResult> cmd_score(const CommandContext &ctx);
Result<CommandResult> cmd_time(const CommandContext &ctx);
Result<CommandResult> cmd_weather(const CommandContext &ctx);
Result<CommandResult> cmd_stat(const CommandContext &ctx);

// Target evaluation commands
Result<CommandResult> cmd_consider(const CommandContext &ctx);
Result<CommandResult> cmd_diagnose(const CommandContext &ctx);
Result<CommandResult> cmd_glance(const CommandContext &ctx);

// Game information commands
Result<CommandResult> cmd_credits(const CommandContext &ctx);
Result<CommandResult> cmd_motd(const CommandContext &ctx);
Result<CommandResult> cmd_news(const CommandContext &ctx);
Result<CommandResult> cmd_policy(const CommandContext &ctx);
Result<CommandResult> cmd_version(const CommandContext &ctx);

// Scanning commands
Result<CommandResult> cmd_scan(const CommandContext &ctx);

} // namespace InformationCommands
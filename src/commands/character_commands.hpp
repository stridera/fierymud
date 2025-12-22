#pragma once

#include "command_system.hpp"

namespace CharacterCommands {

// Module registration
Result<void> register_commands();

// Character status commands
Result<CommandResult> cmd_affects(const CommandContext &ctx);
Result<CommandResult> cmd_skills(const CommandContext &ctx);

// Trainer commands
Result<CommandResult> cmd_practice(const CommandContext &ctx);
Result<CommandResult> cmd_train(const CommandContext &ctx);

// Toggle commands
Result<CommandResult> cmd_wimpy(const CommandContext &ctx);
Result<CommandResult> cmd_brief(const CommandContext &ctx);
Result<CommandResult> cmd_compact(const CommandContext &ctx);
Result<CommandResult> cmd_autoloot(const CommandContext &ctx);
Result<CommandResult> cmd_autogold(const CommandContext &ctx);
Result<CommandResult> cmd_autosplit(const CommandContext &ctx);

// Social interaction commands
Result<CommandResult> cmd_afk(const CommandContext &ctx);
Result<CommandResult> cmd_title(const CommandContext &ctx);
Result<CommandResult> cmd_description(const CommandContext &ctx);
Result<CommandResult> cmd_toggle(const CommandContext &ctx);

// Consent/PvP commands
Result<CommandResult> cmd_consent(const CommandContext &ctx);
Result<CommandResult> cmd_pk(const CommandContext &ctx);

} // namespace CharacterCommands

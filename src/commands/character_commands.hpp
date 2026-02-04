#pragma once

#include "core/result.hpp"

class CommandContext;
enum class CommandResult;

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
Result<CommandResult> cmd_toggle(const CommandContext &ctx);

// Social interaction commands
Result<CommandResult> cmd_afk(const CommandContext &ctx);
Result<CommandResult> cmd_title(const CommandContext &ctx);
Result<CommandResult> cmd_description(const CommandContext &ctx);

// Consent/PvP commands
Result<CommandResult> cmd_consent(const CommandContext &ctx);
Result<CommandResult> cmd_pk(const CommandContext &ctx);

// Follower commands
Result<CommandResult> cmd_call(const CommandContext &ctx);
Result<CommandResult> cmd_order(const CommandContext &ctx);

// Class ability commands
Result<CommandResult> cmd_subclass(const CommandContext &ctx);
Result<CommandResult> cmd_shapechange(const CommandContext &ctx);

// Communication commands
Result<CommandResult> cmd_write(const CommandContext &ctx);

} // namespace CharacterCommands

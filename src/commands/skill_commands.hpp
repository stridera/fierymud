#pragma once

#include "core/result.hpp"

class CommandContext;
enum class CommandResult;

namespace SkillCommands {

// Module registration
Result<void> register_commands();

// Stealth skill commands (self-targeted toggles)
Result<CommandResult> cmd_hide(const CommandContext &ctx);
Result<CommandResult> cmd_sneak(const CommandContext &ctx);
Result<CommandResult> cmd_meditate(const CommandContext &ctx);

} // namespace SkillCommands

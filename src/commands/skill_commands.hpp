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

// Effect management
Result<CommandResult> cmd_cancel(const CommandContext &ctx);

// Utility / movement skills
Result<CommandResult> cmd_visible(const CommandContext &ctx);
Result<CommandResult> cmd_pick(const CommandContext &ctx);
Result<CommandResult> cmd_hunt(const CommandContext &ctx);
Result<CommandResult> cmd_track(const CommandContext &ctx);
Result<CommandResult> cmd_steal(const CommandContext &ctx);
Result<CommandResult> cmd_conceal(const CommandContext &ctx);
Result<CommandResult> cmd_douse(const CommandContext &ctx);
Result<CommandResult> cmd_palm(const CommandContext &ctx);
Result<CommandResult> cmd_walk(const CommandContext &ctx);
Result<CommandResult> cmd_drag(const CommandContext &ctx);
Result<CommandResult> cmd_stow(const CommandContext &ctx);
Result<CommandResult> cmd_tame(const CommandContext &ctx);
Result<CommandResult> cmd_lure(const CommandContext &ctx);
Result<CommandResult> cmd_corner(const CommandContext &ctx);
Result<CommandResult> cmd_firstaid(const CommandContext &ctx);

} // namespace SkillCommands

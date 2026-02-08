#pragma once

#include "core/result.hpp"

class CommandContext;
enum class CommandResult;

namespace QuestCommands {

// Module registration
Result<void> register_commands();

// Player quest commands
Result<CommandResult> cmd_quest(const CommandContext &ctx);     // Main quest command
Result<CommandResult> cmd_quests(const CommandContext &ctx);    // View quest log
Result<CommandResult> cmd_questinfo(const CommandContext &ctx); // Detailed quest info

// God/Admin quest commands
Result<CommandResult> cmd_qstat(const CommandContext &ctx);     // View player's quest progress
Result<CommandResult> cmd_qgive(const CommandContext &ctx);     // Give quest to player
Result<CommandResult> cmd_qcomplete(const CommandContext &ctx); // Force complete quest
Result<CommandResult> cmd_qlist(const CommandContext &ctx);     // List loaded quests
Result<CommandResult> cmd_qload(const CommandContext &ctx);     // Load quests for zone
Result<CommandResult> cmd_qreload(const CommandContext &ctx);   // Reload quests for zone

} // namespace QuestCommands

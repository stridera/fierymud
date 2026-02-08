#pragma once

#include "core/result.hpp"
#include <memory>

class Actor;
class CommandContext;
enum class CommandResult;

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

// Enter/Leave commands
Result<CommandResult> cmd_enter(const CommandContext &ctx);
Result<CommandResult> cmd_leave(const CommandContext &ctx);

// Mount commands
Result<CommandResult> cmd_mount(const CommandContext &ctx);
Result<CommandResult> cmd_dismount(const CommandContext &ctx);

// Recall command
Result<CommandResult> cmd_recall(const CommandContext &ctx);

// Recall completion (called by WorldServer when concentration finishes)
void complete_recall(std::shared_ptr<Actor> actor);

// Recall ability ID constant (used to identify recall in CastingState)
constexpr int RECALL_ABILITY_ID = -1;

} // namespace MovementCommands

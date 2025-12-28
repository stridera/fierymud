#pragma once

#include "command_system.hpp"

namespace ObjectCommands {

// Module registration
Result<void> register_commands();

// Object interaction commands
Result<CommandResult> cmd_get(const CommandContext &ctx);
Result<CommandResult> cmd_drop(const CommandContext &ctx);
Result<CommandResult> cmd_put(const CommandContext &ctx);
Result<CommandResult> cmd_give(const CommandContext &ctx);
Result<CommandResult> cmd_wear(const CommandContext &ctx);
Result<CommandResult> cmd_wield(const CommandContext &ctx);
Result<CommandResult> cmd_remove(const CommandContext &ctx);

// Container and object interaction commands
Result<CommandResult> cmd_open(const CommandContext &ctx);
Result<CommandResult> cmd_close(const CommandContext &ctx);
Result<CommandResult> cmd_lock(const CommandContext &ctx);
Result<CommandResult> cmd_unlock(const CommandContext &ctx);

// Consumable and utility commands
Result<CommandResult> cmd_light(const CommandContext &ctx);
Result<CommandResult> cmd_extinguish(const CommandContext &ctx);
Result<CommandResult> cmd_eat(const CommandContext &ctx);
Result<CommandResult> cmd_drink(const CommandContext &ctx);

// Shop commands
Result<CommandResult> cmd_list(const CommandContext &ctx);
Result<CommandResult> cmd_buy(const CommandContext &ctx);
Result<CommandResult> cmd_sell(const CommandContext &ctx);

// Additional object manipulation commands (Phase 1.4)
Result<CommandResult> cmd_hold(const CommandContext &ctx);
Result<CommandResult> cmd_grab(const CommandContext &ctx);
Result<CommandResult> cmd_quaff(const CommandContext &ctx);
Result<CommandResult> cmd_recite(const CommandContext &ctx);
Result<CommandResult> cmd_use(const CommandContext &ctx);
Result<CommandResult> cmd_junk(const CommandContext &ctx);
Result<CommandResult> cmd_donate(const CommandContext &ctx);
Result<CommandResult> cmd_compare(const CommandContext &ctx);

// Liquid container commands
Result<CommandResult> cmd_fill(const CommandContext &ctx);
Result<CommandResult> cmd_pour(const CommandContext &ctx);

} // namespace ObjectCommands
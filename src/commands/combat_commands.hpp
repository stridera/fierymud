#pragma once

#include <memory>

#include "command_fwd.hpp"

namespace CombatCommands {

// Module registration
Result<void> register_commands();

// Combat commands
Result<CommandResult> cmd_kill(const CommandContext &ctx);
Result<CommandResult> cmd_hit(const CommandContext &ctx);
Result<CommandResult> cmd_cast(const CommandContext &ctx);
Result<CommandResult> cmd_abort(const CommandContext &ctx);
Result<CommandResult> cmd_flee(const CommandContext &ctx);
Result<CommandResult> cmd_release(const CommandContext &ctx);

// Combat skill commands
Result<CommandResult> cmd_rescue(const CommandContext &ctx);
Result<CommandResult> cmd_assist(const CommandContext &ctx);
Result<CommandResult> cmd_kick(const CommandContext &ctx);
Result<CommandResult> cmd_bash(const CommandContext &ctx);
Result<CommandResult> cmd_backstab(const CommandContext &ctx);

// Combat helper functions
Result<CommandResult> perform_attack(const CommandContext &ctx, std::shared_ptr<Actor> target);
bool is_valid_target(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target);

// Death helper functions (for player release - mobs use Mobile::die() directly)
void create_actor_corpse(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room);

} // namespace CombatCommands

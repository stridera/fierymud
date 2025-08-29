/***************************************************************************
 *   File: src/commands/combat_commands.hpp                Part of FieryMUD *
 *  Usage: Combat command declarations                                      *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"

namespace CombatCommands {

// Combat commands
Result<CommandResult> cmd_kill(const CommandContext &ctx);
Result<CommandResult> cmd_hit(const CommandContext &ctx);
Result<CommandResult> cmd_cast(const CommandContext &ctx);
Result<CommandResult> cmd_flee(const CommandContext &ctx);
Result<CommandResult> cmd_release(const CommandContext &ctx);

// Combat helper functions
Result<CommandResult> perform_attack(const CommandContext &ctx, std::shared_ptr<Actor> target);
bool is_valid_target(std::shared_ptr<Actor> attacker, std::shared_ptr<Actor> target);

// Death helper functions
void create_player_corpse(std::shared_ptr<Actor> actor, std::shared_ptr<Room> room);

} // namespace CombatCommands
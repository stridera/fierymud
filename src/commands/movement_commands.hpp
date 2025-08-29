/***************************************************************************
 *   File: src/commands/movement_commands.hpp              Part of FieryMUD *
 *  Usage: Movement command declarations                                     *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"

namespace MovementCommands {

// Movement commands
Result<CommandResult> cmd_north(const CommandContext &ctx);
Result<CommandResult> cmd_south(const CommandContext &ctx);
Result<CommandResult> cmd_east(const CommandContext &ctx);
Result<CommandResult> cmd_west(const CommandContext &ctx);
Result<CommandResult> cmd_up(const CommandContext &ctx);
Result<CommandResult> cmd_down(const CommandContext &ctx);
Result<CommandResult> cmd_exits(const CommandContext &ctx);

} // namespace MovementCommands
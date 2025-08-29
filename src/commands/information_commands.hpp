/***************************************************************************
 *   File: src/commands/information_commands.hpp           Part of FieryMUD *
 *  Usage: Information and status command declarations                       *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"

namespace InformationCommands {

// Information and status commands
Result<CommandResult> cmd_look(const CommandContext &ctx);
Result<CommandResult> cmd_examine(const CommandContext &ctx);
Result<CommandResult> cmd_who(const CommandContext &ctx);
Result<CommandResult> cmd_where(const CommandContext &ctx);
Result<CommandResult> cmd_inventory(const CommandContext &ctx);
Result<CommandResult> cmd_equipment(const CommandContext &ctx);
Result<CommandResult> cmd_score(const CommandContext &ctx);
Result<CommandResult> cmd_time(const CommandContext &ctx);
Result<CommandResult> cmd_weather(const CommandContext &ctx);
Result<CommandResult> cmd_stat(const CommandContext &ctx);

} // namespace InformationCommands
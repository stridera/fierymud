/***************************************************************************
 *   File: src/commands/system_commands.hpp         Part of FieryMUD *
 *  Usage: System command implementations                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"
#include "command_context.hpp"

/**
 * System commands for FieryMUD.
 * 
 * This module provides core system commands for players including
 * session management, help systems, and player information.
 */

namespace SystemCommands {
    // System Commands
    Result<CommandResult> cmd_quit(const CommandContext& ctx);
    Result<CommandResult> cmd_save(const CommandContext& ctx);
    Result<CommandResult> cmd_help(const CommandContext& ctx);
    Result<CommandResult> cmd_commands(const CommandContext& ctx);
    Result<CommandResult> cmd_prompt(const CommandContext& ctx);
}
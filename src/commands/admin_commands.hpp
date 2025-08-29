/***************************************************************************
 *   File: src/commands/admin_commands.hpp                 Part of FieryMUD *
 *  Usage: Administrative command declarations                               *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"

namespace AdminCommands {

// Administrative commands
Result<CommandResult> cmd_shutdown(const CommandContext &ctx);
Result<CommandResult> cmd_goto(const CommandContext &ctx);
Result<CommandResult> cmd_teleport(const CommandContext &ctx);
Result<CommandResult> cmd_summon(const CommandContext &ctx);
Result<CommandResult> cmd_weather_control(const CommandContext &ctx);

// Zone development commands
Result<CommandResult> cmd_reload_zone(const CommandContext &ctx);
Result<CommandResult> cmd_save_zone(const CommandContext &ctx);
Result<CommandResult> cmd_reload_all_zones(const CommandContext &ctx);
Result<CommandResult> cmd_toggle_file_watching(const CommandContext &ctx);
Result<CommandResult> cmd_dump_world(const CommandContext &ctx);

} // namespace AdminCommands
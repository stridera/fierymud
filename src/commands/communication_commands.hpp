/***************************************************************************
 *   File: src/commands/communication_commands.hpp         Part of FieryMUD *
 *  Usage: Communication command declarations                                *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"

namespace CommunicationCommands {

// Communication commands
Result<CommandResult> cmd_say(const CommandContext &ctx);
Result<CommandResult> cmd_tell(const CommandContext &ctx);
Result<CommandResult> cmd_emote(const CommandContext &ctx);
Result<CommandResult> cmd_whisper(const CommandContext &ctx);
Result<CommandResult> cmd_shout(const CommandContext &ctx);
Result<CommandResult> cmd_gossip(const CommandContext &ctx);

} // namespace CommunicationCommands
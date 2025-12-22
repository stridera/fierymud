#pragma once

#include "command_system.hpp"

namespace CommunicationCommands {

// Module registration
Result<void> register_commands();

// Basic communication commands
Result<CommandResult> cmd_say(const CommandContext &ctx);
Result<CommandResult> cmd_tell(const CommandContext &ctx);
Result<CommandResult> cmd_emote(const CommandContext &ctx);
Result<CommandResult> cmd_whisper(const CommandContext &ctx);
Result<CommandResult> cmd_shout(const CommandContext &ctx);
Result<CommandResult> cmd_gossip(const CommandContext &ctx);

// Additional communication commands
Result<CommandResult> cmd_reply(const CommandContext &ctx);
Result<CommandResult> cmd_ask(const CommandContext &ctx);
Result<CommandResult> cmd_petition(const CommandContext &ctx);
Result<CommandResult> cmd_lasttells(const CommandContext &ctx);

// Group communication (requires group system)
Result<CommandResult> cmd_gtell(const CommandContext &ctx);

} // namespace CommunicationCommands
#pragma once

#include "command_system.hpp"

namespace AdminCommands {

// Module registration
Result<void> register_commands();

// Administrative commands
Result<CommandResult> cmd_shutdown(const CommandContext &ctx);
Result<CommandResult> cmd_goto(const CommandContext &ctx);
Result<CommandResult> cmd_teleport(const CommandContext &ctx);
Result<CommandResult> cmd_summon(const CommandContext &ctx);
Result<CommandResult> cmd_weather_control(const CommandContext &ctx);
Result<CommandResult> cmd_load(const CommandContext &ctx);

// Zone development commands
Result<CommandResult> cmd_reload_zone(const CommandContext &ctx);
Result<CommandResult> cmd_save_zone(const CommandContext &ctx);
Result<CommandResult> cmd_reload_all_zones(const CommandContext &ctx);
Result<CommandResult> cmd_toggle_file_watching(const CommandContext &ctx);
Result<CommandResult> cmd_dump_world(const CommandContext &ctx);

} // namespace AdminCommands
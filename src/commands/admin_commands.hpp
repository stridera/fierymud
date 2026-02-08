#pragma once

#include "core/result.hpp"

class CommandContext;
enum class CommandResult;

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

// Script debugging commands
Result<CommandResult> cmd_syslog(const CommandContext &ctx);
Result<CommandResult> cmd_dtrig(const CommandContext &ctx);
Result<CommandResult> cmd_scripterrors(const CommandContext &ctx);
Result<CommandResult> cmd_validate_scripts(const CommandContext &ctx);

} // namespace AdminCommands

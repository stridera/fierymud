#pragma once

#include "../core/result.hpp"
#include "command_system.hpp"

// Forward declarations
class CommandContext;
class Clan;

namespace ClanCommands {

// =============================================================================
// Clan Command Functions
// =============================================================================

/**
 * Main clan command - handles subcommands and basic clan info
 * Usage: clan [info|who|motd|chat <message>|ranks]
 */
Result<CommandResult> cmd_clan(const CommandContext &ctx);

/**
 * Show detailed clan information
 */
Result<CommandResult> cmd_clan_info(const CommandContext &ctx);
Result<CommandResult> cmd_clan_info(const CommandContext &ctx, const Clan &clan);

/**
 * Show clan member list with online status
 */
Result<CommandResult> cmd_clan_who(const CommandContext &ctx);

/**
 * Show clan message of the day
 */
Result<CommandResult> cmd_clan_motd(const CommandContext &ctx);

/**
 * Send message to clan chat channel
 * Usage: clan chat <message>
 */
Result<CommandResult> cmd_clan_chat(const CommandContext &ctx);

/**
 * Show clan rank structure
 */
Result<CommandResult> cmd_clan_ranks(const CommandContext &ctx);

// =============================================================================
// Administrative Commands
// =============================================================================

/**
 * Set clan properties (god-level command)
 * Usage: cset <clan> <property> <value>
 */
Result<CommandResult> cmd_cset(const CommandContext &ctx);

// =============================================================================
// Registration Functions
// =============================================================================

/**
 * Register all clan commands with the command system
 */
Result<void> register_all_clan_commands();

/**
 * Unregister all clan commands
 */
Result<void> unregister_all_clan_commands();

} // namespace ClanCommands

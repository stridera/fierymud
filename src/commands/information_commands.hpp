#pragma once

#include <memory>
#include "command_fwd.hpp"

namespace InformationCommands {

// Module registration
Result<void> register_commands();

/**
 * Display room information to an actor, respecting brief mode.
 * This is the canonical way to show a room after any room change.
 *
 * @param actor The actor viewing the room
 * @param room The room to display (uses actor's current room if null)
 * @return The formatted room string (caller should send to actor)
 *
 * Display behavior:
 * - Always shows room name (green)
 * - Shows description only if brief mode is OFF
 * - Shows exits if autoexit is enabled
 * - Shows visible objects
 * - Shows visible other actors
 */
std::string format_room_for_actor(const std::shared_ptr<Actor>& actor,
                                   const std::shared_ptr<Room>& room = nullptr);

// Information and status commands
Result<CommandResult> cmd_look(const CommandContext &ctx);
Result<CommandResult> cmd_examine(const CommandContext &ctx);
Result<CommandResult> cmd_who(const CommandContext &ctx);
Result<CommandResult> cmd_users(const CommandContext &ctx);
Result<CommandResult> cmd_where(const CommandContext &ctx);
Result<CommandResult> cmd_inventory(const CommandContext &ctx);
Result<CommandResult> cmd_equipment(const CommandContext &ctx);
Result<CommandResult> cmd_score(const CommandContext &ctx);
Result<CommandResult> cmd_time(const CommandContext &ctx);
Result<CommandResult> cmd_weather(const CommandContext &ctx);
Result<CommandResult> cmd_stat(const CommandContext &ctx);

// Target evaluation commands
Result<CommandResult> cmd_consider(const CommandContext &ctx);
Result<CommandResult> cmd_diagnose(const CommandContext &ctx);
Result<CommandResult> cmd_glance(const CommandContext &ctx);

// Game information commands
Result<CommandResult> cmd_credits(const CommandContext &ctx);
Result<CommandResult> cmd_motd(const CommandContext &ctx);
Result<CommandResult> cmd_news(const CommandContext &ctx);
Result<CommandResult> cmd_policy(const CommandContext &ctx);
Result<CommandResult> cmd_version(const CommandContext &ctx);

// Scanning commands
Result<CommandResult> cmd_scan(const CommandContext &ctx);
Result<CommandResult> cmd_search(const CommandContext &ctx);

// Board commands
Result<CommandResult> cmd_board(const CommandContext &ctx);

// Reading commands
Result<CommandResult> cmd_read(const CommandContext &ctx);

} // namespace InformationCommands

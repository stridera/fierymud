#pragma once

#include "command_context.hpp"
#include "command_system.hpp"

/**
 * Core built-in commands for FieryMUD.
 *
 * This module provides essential command implementations that demonstrate
 * the modern command system functionality, including:
 *
 * - Information commands (look, who, inventory)
 * - Communication commands (say, tell, emote)
 * - Movement commands (north, south, etc.)
 * - Object manipulation (get, drop, wear, remove)
 * - System commands (quit, save, help)
 */

namespace BuiltinCommands {
/** Register all built-in commands with the command system */
Result<void> register_all_commands();

/** Unregister all built-in commands */
Result<void> unregister_all_commands();

// Note: Individual command implementations are organized into specialized modules:
// - InformationCommands (look, who, inventory, etc.)
// - CommunicationCommands (say, tell, emote, etc.)
// - MovementCommands (north, south, exits, etc.)
// - ObjectCommands (get, drop, put, wear, etc.)
// - CombatCommands (kill, hit, cast, flee, etc.)
// - SystemCommands (quit, save, help, etc.)
// - SocialCommands (smile, nod, wave, etc.)
// - AdminCommands (shutdown, goto, teleport, etc.)

// Helper functions
namespace Helpers {
/** Format room description for look command */
std::string format_room_description(std::shared_ptr<Room> room, std::shared_ptr<Actor> viewer);

/** Format object description for examine command */
std::string format_object_description(std::shared_ptr<Object> obj, std::shared_ptr<Actor> viewer);

/** Format actor description for look command */
std::string format_actor_description(std::shared_ptr<Actor> target, std::shared_ptr<Actor> viewer);

/** Format inventory list */
std::string format_inventory(std::shared_ptr<Actor> actor);

/** Format equipment list */
std::string format_equipment(std::shared_ptr<Actor> actor);

/** Format who list */
std::string format_who_list(const std::vector<std::shared_ptr<Actor>> &actors);

/** Format exits list */
std::string format_exits(std::shared_ptr<Room> room);

/** Check if movement is possible in direction */
bool can_move_direction(std::shared_ptr<Actor> actor, Direction dir, std::string &failure_reason);

/** Execute movement in direction */
Result<CommandResult> execute_movement(const CommandContext &ctx, Direction dir);

/** Format social action message */
std::string format_social_message(std::string_view action, std::shared_ptr<Actor> actor,
                                  std::shared_ptr<Actor> target = nullptr);

/** Validate target for social command */
bool validate_social_target(const CommandContext &ctx, std::shared_ptr<Actor> target);

/** Send communication message with appropriate colors */
void send_communication(const CommandContext &ctx, std::string_view message, MessageType type,
                        std::string_view channel_name = "");

/** Format communication message with appropriate color */
std::string format_communication(std::shared_ptr<Actor> sender, std::string_view message, std::string_view channel = "",
                                 MessageType type = MessageType::Normal);
} // namespace Helpers
} // namespace BuiltinCommands

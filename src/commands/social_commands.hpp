// Dynamic social commands loaded from database

#pragma once

#include "command_system.hpp"
#include "database/social_queries.hpp"

#include <memory>
#include <string>
#include <string_view>

/**
 * Social commands for FieryMUD.
 *
 * This module provides social interaction commands that allow players to
 * express emotions and interact with other players in non-combat ways.
 *
 * Social commands are loaded from the database at server startup and
 * registered dynamically. The database stores message templates with
 * variable substitution support:
 *
 *   {actor.name} - Actor's display name
 *   {target.name} - Target's display name
 *   {actor.pronoun.subjective} - he/she/they
 *   {actor.pronoun.objective} - him/her/them
 *   {actor.pronoun.possessive} - his/her/their
 *   {target.pronoun.*} - Same for target
 *
 * Color codes are supported using XML-like tags:
 *   <red>text</red> - Standard colors
 *   <b:cyan>text</b:cyan> - Bright colors
 */
namespace SocialCommands {

/**
 * Initialize the social command system.
 * Loads socials from the database and registers them as commands.
 *
 * @return Result indicating success or failure
 */
Result<void> initialize();

/**
 * Reload socials from the database.
 * Unregisters existing social commands and re-registers from database.
 *
 * @return Result indicating success or failure
 */
Result<void> reload_socials();

/**
 * Get the number of registered social commands.
 */
std::size_t social_count();

/**
 * Check if a command name is a registered social.
 *
 * @param name Command name to check
 * @return true if this is a registered social command
 */
bool is_social(std::string_view name);

/**
 * Execute a social command by name.
 * This is the generic handler that looks up the social in the cache
 * and executes it with the appropriate messages.
 *
 * @param ctx Command context
 * @param social_name Name of the social to execute
 * @return Command result
 */
Result<CommandResult> execute_social_by_name(
    const CommandContext& ctx,
    std::string_view social_name);

/**
 * Create a command handler for a specific social.
 * This returns a lambda that can be registered with the command system.
 *
 * @param social_name Name of the social
 * @return Command handler function
 */
CommandHandler create_social_handler(std::string_view social_name);

// =============================================================================
// Helper Functions
// =============================================================================

namespace Helpers {
    /** Format social action message */
    std::string format_social_message(std::string_view action,
                                      std::shared_ptr<Actor> actor,
                                      std::shared_ptr<Actor> target = nullptr);

    /** Validate target for social command */
    bool validate_social_target(const CommandContext& ctx, std::shared_ptr<Actor> target);

    /**
     * Execute a social from a Social struct (database format).
     * Handles message template processing and sending to appropriate targets.
     *
     * @param ctx Command context
     * @param social The social to execute
     * @param target_name Name of the target (empty for no-target socials)
     * @return Command result
     */
    Result<CommandResult> execute_database_social(
        const CommandContext& ctx,
        const Social& social,
        std::string_view target_name);
}

} // namespace SocialCommands

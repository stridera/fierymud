/***************************************************************************
 *   File: src/commands/social_commands.hpp         Part of FieryMUD *
 *  Usage: Social command implementations                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#pragma once

#include "command_system.hpp"
#include "command_context.hpp"

/**
 * Social commands for FieryMUD.
 * 
 * This module provides social interaction commands that allow players to
 * express emotions and interact with other players in non-combat ways.
 */

namespace SocialCommands {
    // Social Commands
    Result<CommandResult> cmd_smile(const CommandContext& ctx);
    Result<CommandResult> cmd_nod(const CommandContext& ctx);
    Result<CommandResult> cmd_wave(const CommandContext& ctx);
    Result<CommandResult> cmd_bow(const CommandContext& ctx);
    Result<CommandResult> cmd_laugh(const CommandContext& ctx);
    
    // Helper functions
    namespace Helpers {
        /** Format social action message */
        std::string format_social_message(std::string_view action, 
                                         std::shared_ptr<Actor> actor, 
                                         std::shared_ptr<Actor> target = nullptr);
        
        /** Validate target for social command */
        bool validate_social_target(const CommandContext& ctx, std::shared_ptr<Actor> target);
    }
}
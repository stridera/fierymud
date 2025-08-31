/***************************************************************************
 *   File: src/commands/social_commands.cpp         Part of FieryMUD *
 *  Usage: Social command implementations                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.       *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "social_commands.hpp"
#include "../core/actor.hpp"
#include "../core/logging.hpp"
#include "../world/room.hpp"

#include <algorithm>
#include <memory>
#include <string>

namespace SocialCommands {

// =============================================================================
// Social Command Implementations
// =============================================================================

Result<CommandResult> cmd_smile(const CommandContext& ctx) {
    static const SocialMessage smile_msg("You smile happily.", "$n smiles happily.",
                                        "You smile at $N.", "$n smiles at you.",
                                        "$n smiles at $N.", "Smile at whom?");
    
    return ctx.execute_social(smile_msg, ctx.arg_count() > 0 ? ctx.arg(0) : "");
}

Result<CommandResult> cmd_nod(const CommandContext& ctx) {
    static const SocialMessage nod_msg("nod");  // Uses convenience constructor
    return ctx.execute_social(nod_msg, ctx.arg_count() > 0 ? ctx.arg(0) : "");
}

Result<CommandResult> cmd_wave(const CommandContext& ctx) {
    static const SocialMessage wave_msg("wave");  // Uses convenience constructor  
    return ctx.execute_social(wave_msg, ctx.arg_count() > 0 ? ctx.arg(0) : "");
}

Result<CommandResult> cmd_bow(const CommandContext& ctx) {
    static const SocialMessage bow_msg("You bow gracefully.", "$n bows gracefully.",
                                      "You bow to $N.", "$n bows to you.",
                                      "$n bows to $N.", "Bow to whom?");
    
    return ctx.execute_social(bow_msg, ctx.arg_count() > 0 ? ctx.arg(0) : "");
}

Result<CommandResult> cmd_laugh(const CommandContext& ctx) {
    static const SocialMessage laugh_msg("You laugh out loud.", "$n laughs out loud.",
                                        "You laugh at $N.", "$n laughs at you.",
                                        "$n laughs at $N.", "Laugh at whom?");
    
    return ctx.execute_social(laugh_msg, ctx.arg_count() > 0 ? ctx.arg(0) : "");
}

// =============================================================================
// Helper Functions
// =============================================================================

namespace Helpers {

std::string format_social_message(std::string_view action, 
                                 std::shared_ptr<Actor> actor, 
                                 std::shared_ptr<Actor> target) {
    if (target) {
        return fmt::format("{} {} {}.", actor->display_name(), action, target->display_name());
    } else {
        return fmt::format("{} {}.", actor->display_name(), action);
    }
}

bool validate_social_target(const CommandContext& ctx, std::shared_ptr<Actor> target) {
    if (!target) {
        return true; // No target is valid for most social commands
    }
    
    // Target must be in the same room
    if (!ctx.actor || !ctx.actor->current_room() || 
        target->current_room() != ctx.actor->current_room()) {
        return false;
    }
    
    return true;
}

} // namespace Helpers

} // namespace SocialCommands
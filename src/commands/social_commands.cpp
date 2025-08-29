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
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }
    
    if (ctx.arg_count() == 0) {
        ctx.send("You smile happily.");
        ctx.send_to_room(fmt::format("{} smiles happily.", ctx.actor->name()), true);
    } else {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
        
        ctx.send(fmt::format("You smile at {}.", target->name()));
        ctx.send_to_actor(target, fmt::format("{} smiles at you.", ctx.actor->name()));
        ctx.send_to_room(fmt::format("{} smiles at {}.", ctx.actor->name(), target->name()), true);
    }
    
    return CommandResult::Success;
}

Result<CommandResult> cmd_nod(const CommandContext& ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }
    
    if (ctx.arg_count() == 0) {
        ctx.send("You nod.");
        ctx.send_to_room(fmt::format("{} nods.", ctx.actor->name()), true);
    } else {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
        
        ctx.send(fmt::format("You nod at {}.", target->name()));
        ctx.send_to_actor(target, fmt::format("{} nods at you.", ctx.actor->name()));
        ctx.send_to_room(fmt::format("{} nods at {}.", ctx.actor->name(), target->name()), true);
    }
    
    return CommandResult::Success;
}

Result<CommandResult> cmd_wave(const CommandContext& ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }
    
    if (ctx.arg_count() == 0) {
        ctx.send("You wave.");
        ctx.send_to_room(fmt::format("{} waves.", ctx.actor->name()), true);
    } else {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
        
        ctx.send(fmt::format("You wave at {}.", target->name()));
        ctx.send_to_actor(target, fmt::format("{} waves at you.", ctx.actor->name()));
        ctx.send_to_room(fmt::format("{} waves at {}.", ctx.actor->name(), target->name()), true);
    }
    
    return CommandResult::Success;
}

Result<CommandResult> cmd_bow(const CommandContext& ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }
    
    if (ctx.arg_count() == 0) {
        ctx.send("You bow gracefully.");
        ctx.send_to_room(fmt::format("{} bows gracefully.", ctx.actor->name()), true);
    } else {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
        
        ctx.send(fmt::format("You bow to {}.", target->name()));
        ctx.send_to_actor(target, fmt::format("{} bows to you.", ctx.actor->name()));
        ctx.send_to_room(fmt::format("{} bows to {}.", ctx.actor->name(), target->name()), true);
    }
    
    return CommandResult::Success;
}

Result<CommandResult> cmd_laugh(const CommandContext& ctx) {
    if (!ctx.actor) {
        return std::unexpected(Errors::InvalidState("No actor context"));
    }
    
    if (ctx.arg_count() == 0) {
        ctx.send("You laugh out loud.");
        ctx.send_to_room(fmt::format("{} laughs out loud.", ctx.actor->name()), true);
    } else {
        auto target = ctx.find_actor_target(ctx.arg(0));
        if (!target) {
            ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
            return CommandResult::InvalidTarget;
        }
        
        ctx.send(fmt::format("You laugh at {}.", target->name()));
        ctx.send_to_actor(target, fmt::format("{} laughs at you.", ctx.actor->name()));
        ctx.send_to_room(fmt::format("{} laughs at {}.", ctx.actor->name(), target->name()), true);
    }
    
    return CommandResult::Success;
}

// =============================================================================
// Helper Functions
// =============================================================================

namespace Helpers {

std::string format_social_message(std::string_view action, 
                                 std::shared_ptr<Actor> actor, 
                                 std::shared_ptr<Actor> target) {
    if (target) {
        return fmt::format("{} {} {}.", actor->name(), action, target->name());
    } else {
        return fmt::format("{} {}.", actor->name(), action);
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
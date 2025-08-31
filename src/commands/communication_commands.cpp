/***************************************************************************
 *   File: src/commands/communication_commands.cpp         Part of FieryMUD *
 *  Usage: Communication command implementations                             *
 *                                                                           *
 *  All rights reserved.  See license.doc for complete information.         *
 *                                                                           *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 ***************************************************************************/

#include "communication_commands.hpp"
#include "builtin_commands.hpp"

#include "../core/actor.hpp"

namespace CommunicationCommands {

// =============================================================================
// Communication Commands
// =============================================================================

Result<CommandResult> cmd_say(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("say <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Say, "say");

    return CommandResult::Success;
}

Result<CommandResult> cmd_tell(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<player> <message>"); !result) {
        ctx.send_usage("tell <player> <message>");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_global(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not online.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send_error("You can't tell yourself.");
        return CommandResult::InvalidTarget;
    }

    std::string message = ctx.args_from(1);

    // Send to target
    std::string target_msg = fmt::format("{} tells you, '{}'", ctx.actor->display_name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("You tell {}, '{}'", target->display_name(), message);
    ctx.send(sender_msg);

    return CommandResult::Success;
}

Result<CommandResult> cmd_emote(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<action>"); !result) {
        ctx.send_usage("emote <action>");
        return CommandResult::InvalidSyntax;
    }

    std::string action = ctx.args_from(0);
    std::string emote_msg = fmt::format("{} {}", ctx.actor->display_name(), action);

    // Send to everyone in the room including self - emotes show the same message to everyone
    ctx.send_to_room(emote_msg, false); // Include self - everyone sees the same thing

    return CommandResult::Success;
}

Result<CommandResult> cmd_whisper(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<player> <message>"); !result) {
        ctx.send_usage("whisper <player> <message>");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send_error("You can't whisper to yourself.");
        return CommandResult::InvalidTarget;
    }

    std::string message = ctx.args_from(1);

    // Send to target
    std::string target_msg = fmt::format("{} whispers to you, '{}'", ctx.actor->display_name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("You whisper to {}, '{}'", target->display_name(), message);
    ctx.send(sender_msg);

    // Let others see the whisper (but not the content)
    std::string room_msg = fmt::format("{} whispers something to {}.", ctx.actor->display_name(), target->display_name());
    ctx.send_to_room(room_msg, true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_shout(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("shout <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Broadcast, "shout");

    return CommandResult::Success;
}

Result<CommandResult> cmd_gossip(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("gossip <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = ctx.args_from(0);
    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Channel, "gossip");

    return CommandResult::Success;
}

} // namespace CommunicationCommands
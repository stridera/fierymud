#include "communication_commands.hpp"
#include "builtin_commands.hpp"

#include "../core/actor.hpp"

namespace CommunicationCommands {

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * Sanitize player-provided message content by closing any unclosed markup tags.
 * This prevents players from accidentally or intentionally breaking color rendering
 * for other players by leaving tags unclosed (e.g., "<b:blue>text" without "</>").
 *
 * @param message The raw player-provided message
 * @return The message with a closing tag appended
 */
std::string sanitize_player_message(std::string_view message) {
    // Always append a reset tag to close any unclosed formatting
    return fmt::format("{}</>", message);
}

// =============================================================================
// Communication Commands
// =============================================================================

Result<CommandResult> cmd_say(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("say <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0));
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

    std::string message = sanitize_player_message(ctx.args_from(1));

    // Send to target
    std::string target_msg = fmt::format("{} tells you, '{}'", ctx.actor->display_name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("You tell {}, '{}'", target->display_name(), message);
    ctx.send(sender_msg);

    // Track the tell for reply functionality
    if (auto target_player = std::dynamic_pointer_cast<Player>(target)) {
        target_player->set_last_tell_sender(ctx.actor->name());
        target_player->add_tell_to_history(fmt::format("{} told you: {}", ctx.actor->display_name(), message));
    }

    // Track outgoing tell for sender
    if (auto sender_player = std::dynamic_pointer_cast<Player>(ctx.actor)) {
        sender_player->add_tell_to_history(fmt::format("You told {}: {}", target->display_name(), message));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_emote(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<action>"); !result) {
        ctx.send_usage("emote <action>");
        return CommandResult::InvalidSyntax;
    }

    std::string action = sanitize_player_message(ctx.args_from(0));
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

    std::string message = sanitize_player_message(ctx.args_from(1));

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

    std::string message = sanitize_player_message(ctx.args_from(0));
    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Broadcast, "shout");

    return CommandResult::Success;
}

Result<CommandResult> cmd_gossip(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("gossip <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0));
    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Channel, "gossip");

    return CommandResult::Success;
}

// =============================================================================
// Additional Communication Commands
// =============================================================================

Result<CommandResult> cmd_reply(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("reply <message>");
        return CommandResult::InvalidSyntax;
    }

    // Get the player to check for last tell sender
    auto sender_player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!sender_player) {
        ctx.send_error("Only players can use reply.");
        return CommandResult::InvalidState;
    }

    if (sender_player->last_tell_sender().empty()) {
        ctx.send_error("You haven't received any tells to reply to.");
        return CommandResult::InvalidState;
    }

    auto target = ctx.find_actor_global(sender_player->last_tell_sender());
    if (!target) {
        ctx.send_error(fmt::format("'{}' is no longer online.", sender_player->last_tell_sender()));
        return CommandResult::InvalidTarget;
    }

    std::string message = sanitize_player_message(ctx.args_from(0));

    // Send to target
    std::string target_msg = fmt::format("{} tells you, '{}'", ctx.actor->display_name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("You tell {}, '{}'", target->display_name(), message);
    ctx.send(sender_msg);

    // Update tracking for reply chain
    if (auto target_player = std::dynamic_pointer_cast<Player>(target)) {
        target_player->set_last_tell_sender(ctx.actor->name());
        target_player->add_tell_to_history(fmt::format("{} told you: {}", ctx.actor->display_name(), message));
    }

    sender_player->add_tell_to_history(fmt::format("You told {}: {}", target->display_name(), message));

    return CommandResult::Success;
}

Result<CommandResult> cmd_ask(const CommandContext &ctx) {
    if (auto result = ctx.require_args(2, "<target> <question>"); !result) {
        ctx.send_usage("ask <target> <question>");
        return CommandResult::InvalidSyntax;
    }

    auto target = ctx.find_actor_target(ctx.arg(0));
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not here.", ctx.arg(0)));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send_error("You can't ask yourself.");
        return CommandResult::InvalidTarget;
    }

    std::string question = sanitize_player_message(ctx.args_from(1));

    // Send to target
    std::string target_msg = fmt::format("{} asks you, '{}'", ctx.actor->display_name(), question);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("You ask {}, '{}'", target->display_name(), question);
    ctx.send(sender_msg);

    // Let others in the room see the question
    std::string room_msg = fmt::format("{} asks {}, '{}'", ctx.actor->display_name(), target->display_name(), question);
    ctx.send_to_room(room_msg, true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_petition(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("petition <message>");
        ctx.send("Use this to send a message to online immortals for assistance.");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0));

    // Format the petition message for immortals
    // TODO: Add privilege-based filtering when send_to_all_with_privilege is implemented
    // For now, broadcast to all online players (immortals will see it)
    std::string imm_msg = fmt::format("[PETITION] {} petitions: {}", ctx.actor->display_name(), message);
    ctx.send_to_all(imm_msg);

    // Confirmation to the petitioner
    ctx.send("Your petition has been sent to the immortals.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_lasttells(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can view tell history.");
        return CommandResult::InvalidState;
    }

    const auto &history = player->tell_history();
    if (history.empty()) {
        ctx.send("You have no tells in your history.");
        return CommandResult::Success;
    }

    ctx.send("--- Recent Tells ---");
    for (const auto &entry : history) {
        ctx.send(entry);
    }
    ctx.send("--- End of Tell History ---");

    return CommandResult::Success;
}

Result<CommandResult> cmd_gtell(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("gtell <message>");
        return CommandResult::InvalidSyntax;
    }

    // Check if player is in a group
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use group tell.");
        return CommandResult::InvalidState;
    }

    // TODO: Check for group membership when group system is implemented
    // For now, just report that no group exists
    if (!player->has_group()) {
        ctx.send_error("You are not in a group.");
        return CommandResult::InvalidState;
    }

    std::string message = sanitize_player_message(ctx.args_from(0));

    // Format and send to group members
    std::string group_msg = fmt::format("[GROUP] {} tells the group: {}", ctx.actor->display_name(), message);
    player->send_to_group(group_msg);

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands()
        .command("say", cmd_say)
        .alias("'")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("tell", cmd_tell)
        .alias("t")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("emote", cmd_emote)
        .alias(":")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("whisper", cmd_whisper)
        .alias("wh")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("shout", cmd_shout)
        .alias("sh")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("gossip", cmd_gossip)
        .alias(";")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("reply", cmd_reply)
        .alias("r")
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("ask", cmd_ask)
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("petition", cmd_petition)
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("lasttells", cmd_lasttells)
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("gtell", cmd_gtell)
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    return Success();
}

} // namespace CommunicationCommands
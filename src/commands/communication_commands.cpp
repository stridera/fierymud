#include "communication_commands.hpp"
#include "builtin_commands.hpp"

#include "core/actor.hpp"
#include "core/logging.hpp"
#include "core/player.hpp"
#include "events/event_types.hpp"
#include "events/event_publisher.hpp"
#include "scripting/trigger_manager.hpp"
#include "server/world_server.hpp"
#include "text/text_format.hpp"
#include "world/room.hpp"

#include <array>

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
std::string sanitize_player_message(std::string_view message, std::shared_ptr<Actor> actor = nullptr) {
    // Gods can use color codes in their messages
    if (actor) {
        auto priv = CommandSystem::instance().get_actor_privilege(actor);
        if (priv >= PrivilegeLevel::God) {
            // Process god message: balance color tags so they don't affect outer wrapper
            std::string result;
            result.reserve(message.length() + 10);

            int open_count = 0;  // Color tags opened by god in this message

            size_t i = 0;
            while (i < message.length()) {
                if (message[i] == '<') {
                    size_t end = message.find('>', i + 1);
                    if (end != std::string_view::npos) {
                        std::string_view tag_content = message.substr(i + 1, end - i - 1);

                        if (!tag_content.empty() && tag_content[0] == '/') {
                            // Close tag - only include if god has opens to close
                            if (open_count > 0) {
                                result += message.substr(i, end - i + 1);
                                open_count--;
                            }
                            // Otherwise skip this </> - it would pop outer color
                            i = end + 1;
                            continue;
                        } else {
                            // Opening tag - check if it's a color/style tag
                            std::string tag_lower;
                            for (char c : tag_content) {
                                tag_lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                            }
                            // Count as open if it looks like a color tag (not empty, starts with letter or #)
                            if (!tag_content.empty() &&
                                (std::isalpha(static_cast<unsigned char>(tag_content[0])) || tag_content[0] == '#')) {
                                open_count++;
                            }
                        }
                    }
                }
                result += message[i];
                i++;
            }

            // Close any unclosed tags the god opened
            for (int j = 0; j < open_count; j++) {
                result += "</>";
            }

            return result;
        }
    }

    // Strip any color tags from regular player input to prevent them from
    // breaking the message formatting or injecting unwanted colors.
    // The message will be wrapped in appropriate color tags by the
    // calling command, so player-supplied tags are not needed.
    return TextFormat::strip_colors(message);
}

// =============================================================================
// Communication Commands
// =============================================================================

Result<CommandResult> cmd_say(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("say <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);

    // Check for SPEECH triggers on mobs in the room
    if (auto room = ctx.actor->current_room()) {
        auto& trigger_mgr = FieryMUD::TriggerManager::instance();
        if (trigger_mgr.is_initialized()) {
            for (const auto& other : room->contents().actors) {
                // Only mobs have triggers
                if (other == ctx.actor || other->type_name() != "Mobile") {
                    continue;
                }

                // Dispatch SPEECH trigger - triggers can react to what was said
                auto result = trigger_mgr.dispatch_speech(other, ctx.actor, message);
                if (result == FieryMUD::TriggerResult::Halt) {
                    Log::debug("Speech intercepted by trigger on {}", other->name());
                    // Note: We still show the say to the room, trigger just reacted
                }
            }
        }
    }

    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Say, "say");

    // Publish to Muditor bridge (local room chat - include room info)
    auto event = fierymud::events::GameEvent::chat_event(
        fierymud::events::GameEventType::CHAT_SAY,
        std::string(ctx.actor->name()),
        std::string(ctx.args_from(0)));
    if (auto room = ctx.actor->current_room()) {
        event.zone_id = static_cast<int>(room->id().zone_id());
        event.room_id = room->id().to_string();
    }
    fierymud::events::EventPublisher::instance().publish(std::move(event));

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

    std::string message = sanitize_player_message(ctx.args_from(1), ctx.actor);

    // Send to target (cyan for tells)
    std::string target_msg = fmt::format("<cyan>{} tells you, '{}'</>", ctx.actor->display_name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender (cyan for tells)
    std::string sender_msg = fmt::format("<cyan>You tell {}, '{}'</>", target->display_name(), message);
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

    // Publish to Muditor bridge (private message with target)
    fierymud::events::EventPublisher::instance().publish_chat(
        fierymud::events::GameEventType::CHAT_TELL,
        ctx.actor->name(),
        ctx.args_from(1),
        target->name());

    return CommandResult::Success;
}

Result<CommandResult> cmd_emote(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<action>"); !result) {
        ctx.send_usage("emote <action>");
        return CommandResult::InvalidSyntax;
    }

    std::string action = sanitize_player_message(ctx.args_from(0), ctx.actor);
    std::string emote_msg = fmt::format("<yellow>{} {}</>", ctx.actor->display_name(), action);

    // Send to everyone in the room including self - emotes show the same message to everyone
    ctx.send_to_room(emote_msg, false); // Include self - everyone sees the same thing

    // Publish to Muditor bridge (emote with room info)
    auto event = fierymud::events::GameEvent::chat_event(
        fierymud::events::GameEventType::CHAT_EMOTE,
        std::string(ctx.actor->name()),
        std::string(ctx.args_from(0)));
    if (auto room = ctx.actor->current_room()) {
        event.zone_id = static_cast<int>(room->id().zone_id());
        event.room_id = room->id().to_string();
    }
    fierymud::events::EventPublisher::instance().publish(std::move(event));

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

    std::string message = sanitize_player_message(ctx.args_from(1), ctx.actor);

    // Send to target (dim cyan for whispers - more subtle than tells)
    std::string target_msg = fmt::format("<dim><cyan>{} whispers to you, '{}'</></>", ctx.actor->display_name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("<dim><cyan>You whisper to {}, '{}'</></>", target->display_name(), message);
    ctx.send(sender_msg);

    // Let others see the whisper (but not the content) - dim/gray
    // Exclude both sender (exclude_self=true) and target from this message
    std::string room_msg = fmt::format("<dim>{} whispers something to {}.</>", ctx.actor->display_name(), target->display_name());
    ctx.send_to_room(room_msg, true, std::array{target});

    return CommandResult::Success;
}

Result<CommandResult> cmd_shout(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("shout <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);
    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Broadcast, "shout");

    // Publish to Muditor bridge
    fierymud::events::EventPublisher::instance().publish_chat(
        fierymud::events::GameEventType::CHAT_SHOUT,
        ctx.actor->name(),
        ctx.args_from(0));

    return CommandResult::Success;
}

Result<CommandResult> cmd_gossip(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("gossip <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);
    BuiltinCommands::Helpers::send_communication(ctx, message, MessageType::Channel, "gossip");

    // Publish to Muditor bridge
    fierymud::events::EventPublisher::instance().publish_chat(
        fierymud::events::GameEventType::CHAT_GOSSIP,
        ctx.actor->name(),
        ctx.args_from(0));  // Original message without sanitization suffix

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

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);

    // Send to target (cyan for tells)
    std::string target_msg = fmt::format("<cyan>{} tells you, '{}'</>", ctx.actor->display_name(), message);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender (cyan for tells)
    std::string sender_msg = fmt::format("<cyan>You tell {}, '{}'</>", target->display_name(), message);
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

    std::string question = sanitize_player_message(ctx.args_from(1), ctx.actor);

    // Send to target (white for ask, similar to say)
    std::string target_msg = fmt::format("<white>{} asks you, '{}'</>", ctx.actor->display_name(), question);
    ctx.send_to_actor(target, target_msg);

    // Send confirmation to sender
    std::string sender_msg = fmt::format("<white>You ask {}, '{}'</>", target->display_name(), question);
    ctx.send(sender_msg);

    // Let others in the room see the question
    std::string room_msg = fmt::format("<white>{} asks {}, '{}'</>", ctx.actor->display_name(), target->display_name(), question);
    ctx.send_to_room(room_msg, true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_petition(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("petition <message>");
        ctx.send("Use this to send a message to online immortals for assistance.");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);

    // Format the petition message for immortals
    // TODO: Add privilege-based filtering when send_to_all_with_privilege is implemented
    // For now, broadcast to all online players (immortals will see it)
    std::string imm_msg = fmt::format("[PETITION] {} petitions: {}", ctx.actor->display_name(), message);
    ctx.send_to_all(imm_msg);

    // Confirmation to the petitioner
    ctx.send("Your petition has been sent to the immortals.");

    return CommandResult::Success;
}

Result<CommandResult> cmd_wiznet(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("wiznet <message>");
        ctx.send("Broadcast a message to all online immortals.");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);

    // Send to actor first
    ctx.send(fmt::format("You wiznet, '{}'", message));

    // Format the wiznet message for other immortals
    std::string wiz_msg = fmt::format("{} wiznet, '{}'", ctx.actor->display_name(), message);

    // Send to all immortals except self
    if (auto *world_server = WorldServer::instance()) {
        auto online_actors = world_server->get_online_actors();
        for (const auto &online_actor : online_actors) {
            if (online_actor && online_actor != ctx.actor) {
                // Check if they have wiznet permission (God level or higher)
                if (auto player = std::dynamic_pointer_cast<Player>(online_actor)) {
                    if (player->is_god() && player->level() >= static_cast<int>(PrivilegeLevel::God)) {
                        online_actor->send_message(wiz_msg);
                    }
                }
            }
        }
    }

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

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);

    // Format and send to group members
    std::string group_msg = fmt::format("[GROUP] {} tells the group: {}", ctx.actor->display_name(), message);
    player->send_to_group(group_msg);

    return CommandResult::Success;
}

Result<CommandResult> cmd_gecho(const CommandContext &ctx) {
    if (auto result = ctx.require_args(1, "<message>"); !result) {
        ctx.send_usage("gecho <message>");
        ctx.send("Send a message to all players without showing sender.");
        return CommandResult::InvalidSyntax;
    }

    std::string message = sanitize_player_message(ctx.args_from(0), ctx.actor);

    // Send to all online actors
    ctx.send_to_all(message, false);  // Don't exclude self

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
        .alias(".")
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
        .command("wiznet", cmd_wiznet)
        .alias(";")
        .category("Communication")
        .privilege(PrivilegeLevel::God)
        .build();

    Commands()
        .command("gtell", cmd_gtell)
        .category("Communication")
        .privilege(PrivilegeLevel::Player)
        .build();

    Commands()
        .command("gecho", cmd_gecho)
        .category("Communication")
        .privilege(PrivilegeLevel::God)
        .description("Send a global message without showing sender")
        .build();

    return Success();
}

} // namespace CommunicationCommands

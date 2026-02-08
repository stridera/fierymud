#include "group_commands.hpp"

#include "../core/actor.hpp"
#include "../core/money.hpp"
#include "../database/generated/db_enums.hpp"

#include <algorithm>

namespace GroupCommands {

// =============================================================================
// Group Commands
// =============================================================================

Result<CommandResult> cmd_group(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use group commands.");
        return CommandResult::InvalidState;
    }

    // No argument - show group status
    if (ctx.arg_count() == 0) {
        if (!player->has_group()) {
            ctx.send("You are not in a group.");
            ctx.send("Use 'group <player>' to invite someone, or 'follow <player>' to join a group.");
            return CommandResult::Success;
        }

        ctx.send("--- Your Group ---");

        // Get the leader
        auto leader = player->get_leader();
        if (leader) {
            // We're following someone
            auto leader_player = std::dynamic_pointer_cast<Player>(leader);
            if (leader_player) {
                ctx.send(fmt::format("  {} (Leader) - HP: {}/{}, ST: {}/{}", leader_player->display_name(),
                                     leader_player->stats().hit_points, leader_player->stats().max_hit_points,
                                     leader_player->stats().stamina, leader_player->stats().max_stamina));

                // Show all followers of the leader
                for (const auto &weak_follower : leader_player->get_followers()) {
                    if (auto follower = weak_follower.lock()) {
                        std::string marker = (follower == ctx.actor) ? " (You)" : "";
                        ctx.send(fmt::format("  {}{} - HP: {}/{}, ST: {}/{}", follower->display_name(), marker,
                                             follower->stats().hit_points, follower->stats().max_hit_points,
                                             follower->stats().stamina, follower->stats().max_stamina));
                    }
                }
            }
        } else {
            // We're the leader
            ctx.send(fmt::format("  {} (Leader - You) - HP: {}/{}, ST: {}/{}", player->display_name(),
                                 player->stats().hit_points, player->stats().max_hit_points, player->stats().stamina,
                                 player->stats().max_stamina));

            for (const auto &weak_follower : player->get_followers()) {
                if (auto follower = weak_follower.lock()) {
                    ctx.send(fmt::format("  {} - HP: {}/{}, ST: {}/{}", follower->display_name(),
                                         follower->stats().hit_points, follower->stats().max_hit_points,
                                         follower->stats().stamina, follower->stats().max_stamina));
                }
            }
        }
        ctx.send("--- End of Group ---");
        return CommandResult::Success;
    }

    // With argument - toggle group flag or show message
    std::string_view arg = ctx.arg(0);
    if (arg == "all" || arg == "on") {
        player->set_group_flag(true);
        ctx.send("You will now accept followers.");
        return CommandResult::Success;
    } else if (arg == "none" || arg == "off") {
        player->set_group_flag(false);
        ctx.send("You will no longer accept followers.");
        return CommandResult::Success;
    }

    // Try to find the target player to invite/manage
    auto target = ctx.find_actor_target(arg);
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not here.", arg));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send("You can't group with yourself.");
        return CommandResult::InvalidTarget;
    }

    auto target_player = std::dynamic_pointer_cast<Player>(target);
    if (!target_player) {
        ctx.send_error("You can only group with other players.");
        return CommandResult::InvalidTarget;
    }

    // Check if they're already in our group
    if (player->is_group_leader()) {
        for (const auto &weak_follower : player->get_followers()) {
            if (auto follower = weak_follower.lock()) {
                if (follower == target) {
                    ctx.send(fmt::format("{} is already in your group.", target->display_name()));
                    return CommandResult::InvalidTarget;
                }
            }
        }
    }

    ctx.send(fmt::format("{} must use 'follow {}' to join your group.", target->display_name(), player->name()));
    return CommandResult::Success;
}

Result<CommandResult> cmd_follow(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use follow.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        if (player->is_following()) {
            auto leader = player->get_leader();
            if (leader) {
                ctx.send(fmt::format("You are following {}.", leader->display_name()));
            }
        } else {
            ctx.send("You are not following anyone.");
        }
        ctx.send("Use 'follow <player>' to follow someone, or 'unfollow' to stop following.");
        return CommandResult::Success;
    }

    std::string_view arg = ctx.arg(0);

    // "follow self" means stop following
    if (arg == "self" || arg == "me") {
        if (!player->is_following()) {
            ctx.send("You are not following anyone.");
            return CommandResult::InvalidState;
        }

        auto old_leader = player->get_leader();
        if (old_leader) {
            if (auto old_leader_player = std::dynamic_pointer_cast<Player>(old_leader)) {
                old_leader_player->remove_follower(player);
            }
            ctx.send(fmt::format("You stop following {}.", old_leader->display_name()));
            ctx.send_to_actor(old_leader, fmt::format("{} stops following you.", player->display_name()));
        }
        player->clear_leader();
        return CommandResult::Success;
    }

    auto target = ctx.find_actor_target(arg);
    if (!target) {
        ctx.send_error(fmt::format("'{}' is not here.", arg));
        return CommandResult::InvalidTarget;
    }

    if (target == ctx.actor) {
        ctx.send("You can't follow yourself. Use 'unfollow' to stop following someone.");
        return CommandResult::InvalidTarget;
    }

    auto target_player = std::dynamic_pointer_cast<Player>(target);
    if (!target_player) {
        ctx.send_error("You can only follow other players.");
        return CommandResult::InvalidTarget;
    }

    // Check if target accepts followers
    if (!target_player->group_flag()) {
        ctx.send(fmt::format("{} is not accepting followers right now.", target->display_name()));
        return CommandResult::InvalidTarget;
    }

    // Check if already following this person
    if (player->get_leader() == target) {
        ctx.send(fmt::format("You are already following {}.", target->display_name()));
        return CommandResult::InvalidState;
    }

    // If currently following someone else, stop following them first
    if (player->is_following()) {
        auto old_leader = player->get_leader();
        if (old_leader) {
            if (auto old_leader_player = std::dynamic_pointer_cast<Player>(old_leader)) {
                old_leader_player->remove_follower(player);
            }
            ctx.send_to_actor(old_leader, fmt::format("{} stops following you.", player->display_name()));
        }
    }

    // If we have followers, they need to be handled
    if (!player->get_followers().empty()) {
        ctx.send("You can't follow someone while you have followers. Have them unfollow first.");
        return CommandResult::InvalidState;
    }

    // Start following the target
    player->set_leader(target);
    target_player->add_follower(player);

    ctx.send(fmt::format("You start following {}.", target->display_name()));
    ctx.send_to_actor(target, fmt::format("{} starts following you.", player->display_name()));
    ctx.send_to_room(fmt::format("{} starts following {}.", player->display_name(), target->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_unfollow(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use unfollow.");
        return CommandResult::InvalidState;
    }

    if (!player->is_following()) {
        ctx.send("You are not following anyone.");
        return CommandResult::InvalidState;
    }

    auto leader = player->get_leader();
    if (leader) {
        if (auto leader_player = std::dynamic_pointer_cast<Player>(leader)) {
            leader_player->remove_follower(player);
        }
        ctx.send(fmt::format("You stop following {}.", leader->display_name()));
        ctx.send_to_actor(leader, fmt::format("{} stops following you.", player->display_name()));
        ctx.send_to_room(fmt::format("{} stops following {}.", player->display_name(), leader->display_name()), true);
    }
    player->clear_leader();

    return CommandResult::Success;
}

Result<CommandResult> cmd_report(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can report.");
        return CommandResult::InvalidState;
    }

    if (!player->has_group()) {
        ctx.send("You are not in a group.");
        return CommandResult::InvalidState;
    }

    const auto &stats = player->stats();
    std::string report =
        fmt::format("{} reports: HP: {}/{}, ST: {}/{}, XP: {}", player->display_name(), stats.hit_points,
                    stats.max_hit_points, stats.stamina, stats.max_stamina, stats.experience);

    player->send_to_group(report);

    return CommandResult::Success;
}

// =============================================================================
// Group Extension Commands
// =============================================================================

Result<CommandResult> cmd_split(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can split gold.");
        return CommandResult::InvalidState;
    }

    if (!player->has_group()) {
        ctx.send("You're not in a group.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Split how much gold?");
        ctx.send("Usage: split <amount>");
        return CommandResult::InvalidSyntax;
    }

    // Parse money amount - can be "100" (copper), "5g", "2p3g", etc.
    auto money = fiery::parse_money(ctx.args_from(0));
    if (!money || money->is_zero()) {
        ctx.send_error("Invalid amount. Usage: split <amount> [currency]");
        ctx.send("Examples: split 100 gold, split 5p3g, split 500");
        return CommandResult::InvalidSyntax;
    }

    // Check if player can afford to split this amount
    if (!player->wallet().can_afford(*money)) {
        ctx.send_error(fmt::format("You don't have {} to split.", money->to_string()));
        return CommandResult::InvalidState;
    }

    // Count group members in the same room
    int members_in_room = 1; // Include self
    std::vector<std::shared_ptr<Player>> recipients;

    auto leader = player->get_leader();
    std::shared_ptr<Player> group_leader = leader ? std::dynamic_pointer_cast<Player>(leader) : player;

    if (group_leader) {
        // Check if leader is in same room
        if (group_leader != player && group_leader->current_room() == player->current_room()) {
            members_in_room++;
            recipients.push_back(group_leader);
        }

        // Check all followers
        for (const auto &weak_follower : group_leader->get_followers()) {
            if (auto follower = weak_follower.lock()) {
                if (follower != ctx.actor && follower->current_room() == player->current_room()) {
                    if (auto follower_player = std::dynamic_pointer_cast<Player>(follower)) {
                        members_in_room++;
                        recipients.push_back(follower_player);
                    }
                }
            }
        }
    }

    if (members_in_room <= 1) {
        ctx.send("There's no one in your group here to split with.");
        return CommandResult::InvalidState;
    }

    // Calculate share in copper for fair division
    long total_copper = money->value();
    long share_copper = total_copper / members_in_room;
    long remainder_copper = total_copper % members_in_room;

    // Deduct the full amount from the splitter
    player->spend(*money);

    // Create share amount
    auto share = fiery::Money::from_copper(share_copper);

    // Splitter gets their share (plus any remainder)
    player->receive(share);
    if (remainder_copper > 0) {
        player->wallet().receive(remainder_copper);
    }

    // Give each recipient their share
    for (auto &recipient : recipients) {
        recipient->receive(share);
        ctx.send_to_actor(recipient, fmt::format("{} splits {}; you receive {}.", player->display_name(),
                                                 money->to_string(), share.to_string()));
    }

    ctx.send(fmt::format("You split {} among {} group members.", money->to_string(), members_in_room));
    ctx.send(fmt::format("Each person receives {}.", share.to_string()));
    if (remainder_copper > 0) {
        ctx.send(fmt::format("(You keep the {} copper remainder)", remainder_copper));
    }

    return CommandResult::Success;
}

Result<CommandResult> cmd_disband(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can disband groups.");
        return CommandResult::InvalidState;
    }

    if (!player->is_group_leader()) {
        ctx.send("You must be the group leader to disband.");
        return CommandResult::InvalidState;
    }

    auto followers = player->get_followers();
    if (followers.empty()) {
        ctx.send("You have no followers to disband.");
        return CommandResult::InvalidState;
    }

    // Remove all followers
    for (const auto &weak_follower : followers) {
        if (auto follower = weak_follower.lock()) {
            if (auto follower_player = std::dynamic_pointer_cast<Player>(follower)) {
                follower_player->clear_leader();
                ctx.send_to_actor(follower, fmt::format("{} has disbanded the group.", player->display_name()));
            }
        }
    }

    // Clear our followers list
    while (!player->get_followers().empty()) {
        if (auto follower = player->get_followers().front().lock()) {
            player->remove_follower(follower);
        } else {
            // Remove expired weak_ptr
            player->remove_follower(nullptr);
            break;
        }
    }

    ctx.send("You have disbanded your group.");
    ctx.send_to_room(fmt::format("{} has disbanded their group.", player->display_name()), true);

    return CommandResult::Success;
}

Result<CommandResult> cmd_abandon(const CommandContext &ctx) {
    // Abandon is like disband but just kicks everyone without notification
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can abandon followers.");
        return CommandResult::InvalidState;
    }

    if (!player->is_group_leader()) {
        ctx.send("You have no followers to abandon.");
        return CommandResult::InvalidState;
    }

    auto followers = player->get_followers();
    if (followers.empty()) {
        ctx.send("You have no followers to abandon.");
        return CommandResult::InvalidState;
    }

    int count = 0;
    for (const auto &weak_follower : followers) {
        if (auto follower = weak_follower.lock()) {
            if (auto follower_player = std::dynamic_pointer_cast<Player>(follower)) {
                follower_player->clear_leader();
                ctx.send_to_actor(follower, fmt::format("{} has abandoned you!", player->display_name()));
                count++;
            }
        }
    }

    // Clear followers
    while (!player->get_followers().empty()) {
        if (auto follower = player->get_followers().front().lock()) {
            player->remove_follower(follower);
        } else {
            break;
        }
    }

    ctx.send(fmt::format("You abandon {} follower{}.", count, count == 1 ? "" : "s"));

    return CommandResult::Success;
}

Result<CommandResult> cmd_gtell(const CommandContext &ctx) {
    auto player = std::dynamic_pointer_cast<Player>(ctx.actor);
    if (!player) {
        ctx.send_error("Only players can use group tell.");
        return CommandResult::InvalidState;
    }

    if (!player->has_group()) {
        ctx.send("You're not in a group.");
        return CommandResult::InvalidState;
    }

    if (ctx.arg_count() == 0) {
        ctx.send("Tell your group what?");
        ctx.send("Usage: gtell <message>");
        return CommandResult::InvalidSyntax;
    }

    std::string message = std::string(ctx.command.full_argument_string);
    std::string formatted = fmt::format("{} tells the group: '{}'", player->display_name(), message);

    player->send_to_group(formatted);

    return CommandResult::Success;
}

Result<CommandResult> cmd_gsay(const CommandContext &ctx) {
    // gsay is an alias for gtell
    return cmd_gtell(ctx);
}

// =============================================================================
// Summon/Pet Commands
// =============================================================================

Result<CommandResult> cmd_dismiss(const CommandContext &ctx) {
    // Dismiss a summoned creature or pet
    if (ctx.arg_count() == 0) {
        // No argument - list dismissable followers
        std::vector<std::shared_ptr<Actor>> dismissable;

        for (const auto &weak_follower : ctx.actor->get_followers()) {
            if (auto follower = weak_follower.lock()) {
                if (auto mobile = std::dynamic_pointer_cast<Mobile>(follower)) {
                    if (mobile->has_trait(MobTrait::Summoned) || mobile->has_trait(MobTrait::Pet)) {
                        dismissable.push_back(follower);
                    }
                }
            }
        }

        if (dismissable.empty()) {
            ctx.send("You have no summoned creatures or pets to dismiss.");
            return CommandResult::Success;
        }

        ctx.send("You can dismiss the following:");
        for (const auto &creature : dismissable) {
            ctx.send(fmt::format("  - {}", creature->display_name()));
        }
        ctx.send("Usage: dismiss <name>");
        return CommandResult::Success;
    }

    std::string_view target_name = ctx.arg(0);

    // Find the summoned creature among our followers
    std::shared_ptr<Mobile> target_mobile;

    for (const auto &weak_follower : ctx.actor->get_followers()) {
        if (auto follower = weak_follower.lock()) {
            if (auto mobile = std::dynamic_pointer_cast<Mobile>(follower)) {
                // Check if it's a summon or pet
                if (mobile->has_trait(MobTrait::Summoned) || mobile->has_trait(MobTrait::Pet)) {
                    // Check if name matches (case-insensitive partial match)
                    std::string follower_name = std::string(mobile->name());
                    std::string search_name = std::string(target_name);

                    // Convert both to lowercase for comparison
                    std::transform(follower_name.begin(), follower_name.end(), follower_name.begin(), ::tolower);
                    std::transform(search_name.begin(), search_name.end(), search_name.begin(), ::tolower);

                    if (follower_name.find(search_name) != std::string::npos) {
                        target_mobile = mobile;
                        break;
                    }
                }
            }
        }
    }

    if (!target_mobile) {
        ctx.send_error(fmt::format("You don't have a summoned creature named '{}'.", target_name));
        return CommandResult::InvalidTarget;
    }

    // Get the name before we modify anything
    std::string creature_name = std::string(target_mobile->display_name());

    // Remove from our followers list
    ctx.actor->remove_follower(target_mobile);

    // Clear their master reference
    target_mobile->clear_master();

    // Send messages
    ctx.send(fmt::format("You dismiss {}.", creature_name));
    ctx.send_to_room(fmt::format("{} dismisses {}.", ctx.actor->display_name(), creature_name), true);

    // The creature fades away / is removed from the game
    // Remove from room and mark for cleanup
    if (auto room = target_mobile->current_room()) {
        room->remove_actor(target_mobile->id());
    }

    return CommandResult::Success;
}

// =============================================================================
// Command Registration
// =============================================================================

Result<void> register_commands() {
    Commands().command("group", cmd_group).alias("gr").category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("follow", cmd_follow).alias("fol").category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("unfollow", cmd_unfollow).category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("report", cmd_report).alias("rep").category("Group").privilege(PrivilegeLevel::Player).build();

    // Group extension commands
    Commands().command("split", cmd_split).category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("disband", cmd_disband).category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("abandon", cmd_abandon).category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("gtell", cmd_gtell).alias("gt").category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("gsay", cmd_gsay).category("Group").privilege(PrivilegeLevel::Player).build();

    Commands().command("dismiss", cmd_dismiss).category("Group").privilege(PrivilegeLevel::Player).build();

    return Success();
}

} // namespace GroupCommands

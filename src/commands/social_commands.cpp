#include "social_commands.hpp"
#include "../text/text_format.hpp"
#include "../core/actor.hpp"
#include "../world/room.hpp"
#include "../database/connection_pool.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>

namespace SocialCommands {

namespace {

/**
 * Set of registered social command names for quick lookup.
 */
std::unordered_set<std::string> registered_socials_;

/**
 * Find target actor in the current room by name.
 * Uses CommandContext's built-in find_actor_target method.
 */
std::shared_ptr<Actor> find_target(const CommandContext& ctx, std::string_view target_name) {
    if (target_name.empty()) {
        return nullptr;
    }
    return ctx.find_actor_target(target_name);
}

/**
 * Check if target is self.
 */
bool is_self_target(const CommandContext& ctx, std::string_view target_name) {
    if (target_name.empty() || !ctx.actor) {
        return false;
    }

    std::string lower_name = std::string{target_name};
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Check common self-targeting keywords
    if (lower_name == "self" || lower_name == "me" || lower_name == "myself") {
        return true;
    }

    // Check if target matches actor's name
    std::string actor_name = std::string{ctx.actor->name()};
    std::transform(actor_name.begin(), actor_name.end(), actor_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return actor_name.starts_with(lower_name);
}

/**
 * Convert database SocialPosition to core Position.
 */
Position convert_position(SocialPosition db_pos) {
    switch (db_pos) {
        case SocialPosition::Dead: return Position::Dead;
        case SocialPosition::MortallyWounded: return Position::Mortally_Wounded;
        case SocialPosition::Incapacitated: return Position::Incapacitated;
        case SocialPosition::Stunned: return Position::Stunned;
        case SocialPosition::Sleeping: return Position::Sleeping;
        case SocialPosition::Resting: return Position::Resting;
        case SocialPosition::Sitting: return Position::Sitting;
        case SocialPosition::Prone: return Position::Prone;
        case SocialPosition::Kneeling: return Position::Sitting; // Map Kneeling to Sitting for now
        case SocialPosition::Fighting: return Position::Fighting;
        case SocialPosition::Standing: return Position::Standing;
        case SocialPosition::Flying: return Position::Flying;
    }
    return Position::Standing;
}

} // anonymous namespace

// =============================================================================
// Initialization and Registration
// =============================================================================

Result<void> initialize() {
    // Load socials from database (required)
    auto load_result = SocialCache::instance().load_from_database();
    if (!load_result) {
        return std::unexpected(load_result.error());
    }

    // Register each social as a command
    auto& cmd = CommandSystem::instance();
    const auto& socials = SocialCache::instance().all();

    registered_socials_.clear();

    for (const auto& [name, social] : socials) {
        // Note: The 'hide' flag means "hide from socials list", NOT "disable the command"
        // Hidden socials still work, they're just not shown in the socials list command
        auto handler = create_social_handler(name);
        auto result = cmd.command(name, handler)
            .category("Social")
            .privilege(PrivilegeLevel::Player)
            .description(fmt::format("Perform the '{}' social action", name))
            .usable_in_combat(true)
            .usable_while_sitting(true)
            .build();

        if (result) {
            registered_socials_.insert(std::string{name});
        } else {
            spdlog::warn("Failed to register social command '{}': {}", name, result.error().message);
        }
    }

    spdlog::info("Registered {} social commands from database", registered_socials_.size());
    return {};
}

Result<void> reload_socials() {
    // Unregister existing social commands
    auto& cmd = CommandSystem::instance();
    for (const auto& name : registered_socials_) {
        cmd.unregister_command(name);
    }
    registered_socials_.clear();

    // Re-initialize
    return initialize();
}

std::size_t social_count() {
    return registered_socials_.size();
}

bool is_social(std::string_view name) {
    std::string lower_name{name};
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return registered_socials_.contains(lower_name);
}

// =============================================================================
// Social Execution
// =============================================================================

Result<CommandResult> execute_social_by_name(const CommandContext& ctx, std::string_view social_name) {
    const Social* social = SocialCache::instance().get(social_name);
    if (!social) {
        return std::unexpected(Error{ErrorCode::NotFound,
                         fmt::format("Unknown social: {}", social_name)});
    }

    std::string_view target_name = ctx.arg_count() > 0 ? ctx.arg(0) : "";
    return Helpers::execute_database_social(ctx, *social, target_name);
}

CommandHandler create_social_handler(std::string_view social_name) {
    // Capture the social name in the lambda
    std::string name{social_name};
    return [name](const CommandContext& ctx) -> Result<CommandResult> {
        return execute_social_by_name(ctx, name);
    };
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

    // Note: Minimum position check is done in execute_database_social
    // since it requires access to the Social struct's min_victim_position

    return true;
}

Result<CommandResult> execute_database_social(
    const CommandContext& ctx,
    const Social& social,
    std::string_view target_name) {

    if (!ctx.actor) {
        return std::unexpected(Error{ErrorCode::InvalidState, "No actor for social command"});
    }

    // Determine the scenario: no target, self target, or other target
    bool has_target = !target_name.empty();
    bool is_self = has_target && is_self_target(ctx, target_name);

    if (!has_target) {
        // No argument provided - use charNoArg/othersNoArg
        if (!social.char_no_arg.has_value()) {
            ctx.send_error("You can't do that without a target.");
            return CommandResult::InvalidSyntax;
        }

        std::string to_actor = TextFormat::format(*social.char_no_arg, ctx.actor.get(), nullptr);
        ctx.send(to_actor);

        if (social.others_no_arg.has_value()) {
            std::string to_room = TextFormat::format(*social.others_no_arg, ctx.actor.get(), nullptr);
            ctx.send_to_room(to_room, true);
        }

        return CommandResult::Success;
    }

    if (is_self) {
        // Self-targeting - use charAuto/othersAuto
        if (!social.char_auto.has_value()) {
            ctx.send_error("You can't do that to yourself.");
            return CommandResult::InvalidTarget;
        }

        std::string to_actor = TextFormat::format(*social.char_auto, ctx.actor.get(), ctx.actor.get());
        ctx.send(to_actor);

        if (social.others_auto.has_value()) {
            std::string to_room = TextFormat::format(*social.others_auto, ctx.actor.get(), ctx.actor.get());
            ctx.send_to_room(to_room, true);
        }

        return CommandResult::Success;
    }

    // Try to find the target
    auto target = find_target(ctx, target_name);
    if (!target) {
        // Target not found
        if (social.not_found.has_value()) {
            std::string msg = TextFormat::format(*social.not_found, ctx.actor.get(), nullptr);
            ctx.send(msg);
        } else {
            ctx.send_error(fmt::format("You don't see '{}' here.", target_name));
        }
        return CommandResult::InvalidTarget;
    }

    // Check target position
    Position min_pos = convert_position(social.min_victim_position);
    if (target->position() < min_pos) {
        ctx.send_error(fmt::format("{} is not in a position to receive that.",
                                   target->display_name()));
        return CommandResult::InvalidTarget;
    }

    // Target found - use charFound/othersFound/victFound
    if (!social.char_found.has_value()) {
        ctx.send_error("You can't do that.");
        return CommandResult::InvalidSyntax;
    }

    std::string to_actor = TextFormat::format(*social.char_found, ctx.actor.get(), target.get());
    ctx.send(to_actor);

    if (social.vict_found.has_value()) {
        std::string to_target = TextFormat::format(*social.vict_found, ctx.actor.get(), target.get());
        ctx.send_to_actor(target, to_target);
    }

    if (social.others_found.has_value()) {
        std::string to_room = TextFormat::format(*social.others_found, ctx.actor.get(), target.get());
        // Note: This sends to room excluding actor. Target receives both their personalized
        // message (vict_found) and this room message. This is intentional - they see their
        // own perspective first, then the general description to others.
        ctx.send_to_room(to_room, true);
    }

    return CommandResult::Success;
}

} // namespace Helpers

} // namespace SocialCommands
